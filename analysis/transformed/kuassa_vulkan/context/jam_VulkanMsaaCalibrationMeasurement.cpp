//
// Sample-measurement file: measureCandidateSampleCount() — the throwaway
// render-pass/framebuffer/pipeline GPU timing probe calibrateSampleCount()
// (jam_VulkanMsaaCalibration.cpp) drives per MSAA candidate. Decomposed
// into named private steps (see each step's doc comment in jam_VulkanMsaaCalibration.h) —
// measureCandidateSampleCount() itself is the short orchestrating sequence.
// createCalibrationPipelineSetup() (step 2, the largest step) lives in its own
// sibling TU (jam_VulkanMsaaCalibrationPipeline.cpp) to keep this file
// under file size. See jam_vulkan.cpp for the full list of sibling translation
// units making up this one class's setup helpers.

namespace jam
{
/*____________________________________________________________________________*/
std::pair<VulkanImage, VulkanImage> VulkanMsaaCalibration::createCalibrationTargets (
    vk::SampleCountFlagBits candidate, vk::SurfaceFormatKHR swapchainFormat, vk::Extent2D swapchainExtent) const
{
    const vk::ImageCreateInfo msaaInfo { {}, vk::ImageType::e2D, swapchainFormat.format,
                                        vk::Extent3D { swapchainExtent, 1 },
                                        1, 1, candidate, vk::ImageTiling::eOptimal,
                                        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment };

    VmaAllocationCreateInfo msaaAllocInfo {};
    msaaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;

    uint32_t lazyMemoryTypeIndex { 0 };
    const bool lazyMemorySupported
    {
        vmaFindMemoryTypeIndexForImageInfo (
            device.getAllocator(), msaaInfo, &msaaAllocInfo, &lazyMemoryTypeIndex)
        == VK_SUCCESS
    };

    if (not lazyMemorySupported)
        msaaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    const vk::ImageViewCreateInfo colorViewInfo { {}, {}, vk::ImageViewType::e2D, swapchainFormat.format, {},
                                                  vk::ImageSubresourceRange { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };

    VulkanImage msaaColorImage {
        device.getAllocator(), device.getDevice(), msaaInfo, msaaAllocInfo, colorViewInfo
    };

    if (not msaaColorImage.isValid())
        return { std::move (msaaColorImage), VulkanImage {} };

    VulkanImage resolveImage { VulkanImage::create2D (device.getAllocator(), device.getDevice(), swapchainFormat.format,
                                                       swapchainExtent, vk::SampleCountFlagBits::e1,
                                                       vk::ImageUsageFlagBits::eColorAttachment,
                                                       vk::ImageAspectFlagBits::eColor) };

    return { std::move (msaaColorImage), std::move (resolveImage) };
}

/*____________________________________________________________________________*/
std::optional<std::array<uint64_t, VulkanMsaaCalibration::timestampPoolQueryCount>> VulkanMsaaCalibration::recordAndMeasureCalibrationDraw (
    vk::RenderPass targetRenderPass, vk::Framebuffer framebuffer, vk::Pipeline pipeline,
    vk::Extent2D swapchainExtent, vk::CommandPool commandPool) const
{
    JUCE_ASSERT_MESSAGE_THREAD

    std::array<uint64_t, timestampPoolQueryCount> timestamps {};

    const vk::CommandBufferAllocateInfo cmdAllocInfo { commandPool, vk::CommandBufferLevel::ePrimary, 1 };
    vk::CommandBuffer cmd {};
    vk::Result result { device.getDevice().allocateCommandBuffers (&cmdAllocInfo, &cmd) };

    if (result == vk::Result::eSuccess)
    {
        const vk::CommandBufferBeginInfo beginInfo { vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
        result = cmd.begin (&beginInfo);
    }

    if (result == vk::Result::eSuccess)
    {
        cmd.resetQueryPool (timestampPool, 0, timestampPoolQueryCount);
        cmd.writeTimestamp (vk::PipelineStageFlagBits::eTopOfPipe, timestampPool,
                           static_cast<uint32_t> (Timestamp::begin));

        vk::ClearValue clearValue {};
        clearValue.color = vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 0.0f };

        const vk::RenderPassBeginInfo rpBegin { targetRenderPass, framebuffer,
                                                vk::Rect2D { { 0, 0 }, swapchainExtent },
                                                clearValue };

        cmd.beginRenderPass (&rpBegin, vk::SubpassContents::eInline);

        const vk::Viewport viewport {
            0.0f, 0.0f,
            static_cast<float> (swapchainExtent.width),
            static_cast<float> (swapchainExtent.height),
            0.0f, 1.0f
        };
        const vk::Rect2D scissor { { 0, 0 }, swapchainExtent };
        cmd.setViewport (0, viewport);
        cmd.setScissor (0, scissor);

        cmd.bindPipeline (vk::PipelineBindPoint::eGraphics, pipeline);
        cmd.draw (VulkanPipelines::fullscreenTriangleVertexCount, 1, 0, 0);
        cmd.endRenderPass();

        cmd.writeTimestamp (vk::PipelineStageFlagBits::eBottomOfPipe, timestampPool,
                           static_cast<uint32_t> (Timestamp::end));

        result = cmd.end();
    }

    vk::Fence drawFence {};

    if (result == vk::Result::eSuccess)
    {
        const vk::FenceCreateInfo fenceInfo {};
        result = device.getDevice().createFence (&fenceInfo, nullptr, &drawFence);
    }

    if (result == vk::Result::eSuccess)
    {
        const vk::SubmitInfo submitInfo { nullptr, nullptr, cmd };
        result = device.getQueue().submit (1, &submitInfo, drawFence);
    }

    if (result == vk::Result::eSuccess)
    {
        result = device.getDevice().waitForFences (1, &drawFence, vk::True, UINT64_MAX);

        jassert (result == vk::Result::eSuccess or result == vk::Result::eErrorDeviceLost);

        if (result == vk::Result::eErrorDeviceLost)
        {
            juce::MessageManager::callAsync ([]
            {
                if (auto* reinitialiseEngine { VulkanEngine::getInstance() })
                    reinitialiseEngine->reinitialiseDevice();
            });
        }
    }

    if (result == vk::Result::eSuccess)
        result = device.getDevice().getQueryPoolResults (
            timestampPool, 0, timestampPoolQueryCount,
            sizeof (timestamps), timestamps.data(), sizeof (uint64_t),
            vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait);

    device.getDevice().destroyFence (drawFence, nullptr);
    device.getDevice().freeCommandBuffers (commandPool, cmd);

#if JUCE_DEBUG
    if (result != vk::Result::eSuccess)
        jam::debug::Log::write ("recordAndMeasureCalibrationDraw failed: "
                                    + juce::String (vk::to_string (result)));
#endif

    if (result == vk::Result::eSuccess)
        return timestamps;

    return std::nullopt;
}

/*____________________________________________________________________________*/
std::optional<double> VulkanMsaaCalibration::measureCalibrationIterations (vk::RenderPass targetRenderPass, vk::Framebuffer framebuffer,
                                                                            vk::Pipeline pipeline, vk::Extent2D swapchainExtent,
                                                                            vk::CommandPool commandPool) const
{
    uint64_t accumulatedTicks { 0 };
    int completedIterations { 0 };

    for (int iteration = 0;
         iteration < calibrationWarmupIterations + calibrationMeasuredIterations;
         ++iteration)
    {
        const auto timestamps { recordAndMeasureCalibrationDraw (targetRenderPass, framebuffer, pipeline, swapchainExtent, commandPool) };

        if (not timestamps.has_value())
            break;

        if (iteration >= calibrationWarmupIterations)
        {
            accumulatedTicks += (timestamps->at (static_cast<size_t> (Timestamp::end))
                                 - timestamps->at (static_cast<size_t> (Timestamp::begin)));
            ++completedIterations;
        }
    }

    std::optional<double> averageMs;

    if (completedIterations == calibrationMeasuredIterations)
    {
        const double averageTicks {
            static_cast<double> (accumulatedTicks) / calibrationMeasuredIterations
        };
        averageMs = averageTicks * static_cast<double> (device.getTimestampPeriod()) / 1'000'000.0;
    }

    return averageMs;
}

/*____________________________________________________________________________*/
void VulkanMsaaCalibration::destroyCalibrationResources (vk::RenderPass targetRenderPass, vk::Framebuffer framebuffer,
                                             vk::PipelineLayout pipelineLayout, vk::Pipeline pipeline,
                                             vk::ShaderModule vertModule, vk::ShaderModule fragModule) const
{
    device.getDevice().destroyPipeline (pipeline, nullptr);
    device.getDevice().destroyShaderModule (vertModule, nullptr);
    device.getDevice().destroyShaderModule (fragModule, nullptr);
    device.getDevice().destroyPipelineLayout (pipelineLayout, nullptr);
    device.getDevice().destroyFramebuffer (framebuffer, nullptr);
    device.getDevice().destroyRenderPass (targetRenderPass, nullptr);
}

/*____________________________________________________________________________*/
std::optional<double> VulkanMsaaCalibration::measureCandidateSampleCount (vk::SampleCountFlagBits candidate,
                                                                           vk::SurfaceFormatKHR swapchainFormat,
                                                                           vk::Extent2D swapchainExtent,
                                                                           vk::CommandPool commandPool)
{
    auto [msaaColorImage, resolveImage] { createCalibrationTargets (candidate, swapchainFormat, swapchainExtent) };

    if (not msaaColorImage.isValid() or not resolveImage.isValid())
        return std::nullopt;

    auto [pipelineSetupSucceeded, calibrationRenderPass, calibrationFramebuffer, calibrationPipelineLayout,
          vertModule, fragModule, calibrationPipeline] {
        createCalibrationPipelineSetup (candidate, msaaColorImage, resolveImage, swapchainFormat, swapchainExtent)
    };

    std::optional<double> measuredMs;

    if (pipelineSetupSucceeded)
        measuredMs = measureCalibrationIterations (calibrationRenderPass, calibrationFramebuffer, calibrationPipeline,
                                                    swapchainExtent, commandPool);

    destroyCalibrationResources (calibrationRenderPass, calibrationFramebuffer, calibrationPipelineLayout,
                                  calibrationPipeline, vertModule, fragModule);

    return measuredMs;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
