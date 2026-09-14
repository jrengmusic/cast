namespace jam
{
/*____________________________________________________________________________*/
void VulkanImageRenderer::initialise (double targetFrameBudgetMs, vk::PipelineCache pipelineCache, vk::Extent2D maxImageExtent)
{
    graphics = VulkanGraphics::createOffscreen (device, targetFrameBudgetMs, pipelineCache, maxImageExtent);
    jassert (graphics != nullptr);

    const vk::CommandPoolCreateInfo poolInfo { vk::CommandPoolCreateFlagBits::eTransient
                                               | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                               device.getQueueFamily() };

    auto [result, pool] { device.getDevice().createCommandPool (poolInfo) };
    oneShotPool = pool;

    const vk::CommandBufferAllocateInfo cmdAllocInfo { oneShotPool, vk::CommandBufferLevel::ePrimary, 1 };

    if (result == vk::Result::eSuccess) result = device.getDevice().allocateCommandBuffers (&cmdAllocInfo, &oneShotCommandBuffer);

    const vk::FenceCreateInfo fenceInfo {};

    if (result == vk::Result::eSuccess)
    {
        auto [fenceResult, fence] { device.getDevice().createFence (fenceInfo) };
        result = fenceResult;
        oneShotFence = fence;
    }

    jassert (result == vk::Result::eSuccess);
}

void VulkanImageRenderer::shutdown()
{
    for (auto& [targetKey, resources] : renderTargets)
        device.getDevice().destroyFramebuffer (resources.framebuffer, nullptr);

    renderTargets.clear();

    device.getDevice().destroyFence (oneShotFence, nullptr);
    device.getDevice().destroyCommandPool (oneShotPool, nullptr);

    graphics.reset();

    oneShotPool = vk::CommandPool {};
    oneShotCommandBuffer = vk::CommandBuffer {};
    oneShotFence = vk::Fence {};
}

vk::CommandBuffer VulkanImageRenderer::beginCommands()
{
    JUCE_ASSERT_MESSAGE_THREAD

    const vk::CommandBufferBeginInfo beginInfo { vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    const vk::Result beginResult { oneShotCommandBuffer.begin (&beginInfo) };
    jassert (beginResult == vk::Result::eSuccess);
    juce::ignoreUnused (beginResult);

    return oneShotCommandBuffer;
}

vk::Result VulkanImageRenderer::submitAndWait()
{
    JUCE_ASSERT_MESSAGE_THREAD

    const vk::SubmitInfo submitInfo { nullptr, nullptr, oneShotCommandBuffer };

    vk::Result result { oneShotCommandBuffer.end() };
    if (result == vk::Result::eSuccess) result = device.getQueue().submit (1, &submitInfo, oneShotFence);
    if (result == vk::Result::eSuccess) result = device.getDevice().waitForFences (1, &oneShotFence, VK_TRUE, UINT64_MAX);
    if (result == vk::Result::eSuccess) result = device.getDevice().resetFences (1, &oneShotFence);
    if (result == vk::Result::eSuccess) result = oneShotCommandBuffer.reset ({});

    jassert (result == vk::Result::eSuccess or result == vk::Result::eErrorDeviceLost);
    return result;
}

std::unique_ptr<VulkanLowLevelGraphicsContext> VulkanImageRenderer::renderTarget (vk::Framebuffer framebuffer,
                                                                                   vk::Extent2D extent,
                                                                                   vk::Image resolveImage,
                                                                                   float scale)
{
    JUCE_ASSERT_MESSAGE_THREAD

    jassert (graphics != nullptr);

    const auto logicalWidth  { static_cast<int> (extent.width  / scale) };
    const auto logicalHeight { static_cast<int> (extent.height / scale) };

    if (graphics->beginOffscreenFrame (framebuffer, extent, resolveImage))
    {
        graphics->incrementContextCount();
        return std::make_unique<VulkanLowLevelGraphicsContext> (*graphics, logicalWidth, logicalHeight, scale);
    }

    return nullptr;
}

VulkanRenderTargetResources* VulkanImageRenderer::getOrCreateRenderTarget (void* key, vk::Extent2D extent)
{
    if (not renderTargets.contains (key))
    {
        VulkanRenderTargetResources resources { buildRenderTarget (extent) };

        if (resources.framebuffer == nullptr)
            return nullptr;

        renderTargets.emplace (key, std::move (resources));
    }

    return &renderTargets.at (key);
}

void VulkanImageRenderer::removeRenderTarget (void* key)
{
    if (renderTargets.contains (key))
    {
        device.getDevice().destroyFramebuffer (renderTargets.at (key).framebuffer, nullptr);
        renderTargets.erase (key);
    }
}

VulkanRenderTargetResources VulkanImageRenderer::buildRenderTarget (vk::Extent2D extent)
{
    const auto allocator { device.getAllocator() };
    const auto vulkanDevice { device.getDevice() };
    const auto sampleCount { getGraphics().getActiveSampleCount() };
    const auto renderPass { getGraphics().getRenderPass() };

    VulkanRenderTargetResources resources {};

    resources.msaaColorImage = VulkanImage::create2D (allocator, vulkanDevice, vk::Format::eB8G8R8A8Unorm,
                                                       extent, sampleCount,
                                                       vk::ImageUsageFlagBits::eColorAttachment
                                                           | vk::ImageUsageFlagBits::eTransferDst,
                                                       vk::ImageAspectFlagBits::eColor);

    resources.stencilImage = VulkanImage::create2D (allocator, vulkanDevice, vk::Format::eS8Uint,
                                                     extent, sampleCount,
                                                     vk::ImageUsageFlagBits::eDepthStencilAttachment
                                                         | vk::ImageUsageFlagBits::eTransferDst,
                                                     vk::ImageAspectFlagBits::eStencil);

    bool ready { resources.msaaColorImage.isValid() };
    ready = ready and resources.stencilImage.isValid();
    ready = ready and resources.resolveTexture.create (device, static_cast<int> (extent.width),
                                                        static_cast<int> (extent.height),
                                                        vk::Format::eB8G8R8A8Unorm,
                                                        vk::ImageUsageFlagBits::eColorAttachment
                                                            | vk::ImageUsageFlagBits::eSampled
                                                            | vk::ImageUsageFlagBits::eTransferSrc
                                                            | vk::ImageUsageFlagBits::eTransferDst);

    if (ready)
    {
        const vk::ImageView fbAttachments[3] {
            resources.msaaColorImage.getAttachmentView(),
            resources.stencilImage.getAttachmentView(),
            resources.resolveTexture.getAttachmentView()
        };

        const vk::FramebufferCreateInfo fbInfo { {}, renderPass, fbAttachments,
                                                  extent.width, extent.height, 1 };

        const vk::Result framebufferResult { vulkanDevice.createFramebuffer (&fbInfo, nullptr, &resources.framebuffer) };
        ready = framebufferResult == vk::Result::eSuccess;

        if (not ready)
        {
            debug::Log::write ("VulkanImageRenderer::buildRenderTarget: createFramebuffer failed:", vk::to_string (framebufferResult));

            if (framebufferResult == vk::Result::eErrorDeviceLost)
            {
                juce::MessageManager::callAsync ([]
                {
                    if (auto* reinitialiseEngine { VulkanEngine::getInstance() })
                        reinitialiseEngine->reinitialiseDevice();
                });
            }
        }
    }

    if (ready)
    {
        const vk::Result clearResult { clearResolveImage (resources.resolveTexture.getImage()) };
        ready = clearResult == vk::Result::eSuccess;

        if (not ready)
        {
            vulkanDevice.destroyFramebuffer (resources.framebuffer, nullptr);
            resources.framebuffer = vk::Framebuffer {};
        }
    }

    return resources;
}

vk::Result VulkanImageRenderer::clearResolveImage (vk::Image resolveImage)
{
    const vk::CommandBuffer cmd { beginCommands() };

    recordImageMemoryBarrier (cmd, resolveImage,
                              vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlags {}, vk::AccessFlagBits::eTransferWrite,
                              vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer);

    const vk::ClearColorValue clearColor { std::array<float, 4> { 0.0f, 0.0f, 0.0f, 0.0f } };
    const vk::ImageSubresourceRange range { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
    cmd.clearColorImage (resolveImage, vk::ImageLayout::eTransferDstOptimal, &clearColor, 1, &range);

    recordImageMemoryBarrier (cmd, resolveImage,
                              vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                              vk::ImageAspectFlagBits::eColor,
                              vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                              vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader);

    const vk::Result result { submitAndWait() };

    if (result != vk::Result::eSuccess)
    {
        debug::Log::write ("VulkanImageRenderer::clearResolveImage: submitAndWait failed:", vk::to_string (result));

        if (result == vk::Result::eErrorDeviceLost)
        {
            juce::MessageManager::callAsync ([]
            {
                if (auto* reinitialiseEngine { VulkanEngine::getInstance() })
                    reinitialiseEngine->reinitialiseDevice();
            });
        }
    }

    return result;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
