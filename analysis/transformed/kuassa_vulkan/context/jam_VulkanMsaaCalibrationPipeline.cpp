//
// Calibration pipeline file: createCalibrationPipelineSetup() — the single
// largest step of measureCandidateSampleCount()'s five-step decomposition,
// split into its own sibling TU to keep jam_VulkanMsaaCalibrationMeasurement.cpp
// under file size. See jam_vulkan.cpp for the full list of sibling translation
// units making up this one class's setup helpers.

namespace jam
{
/*____________________________________________________________________________*/
std::tuple<bool, vk::RenderPass, vk::Framebuffer, vk::PipelineLayout, vk::ShaderModule, vk::ShaderModule, vk::Pipeline>
    VulkanMsaaCalibration::createCalibrationPipelineSetup (
        vk::SampleCountFlagBits candidate, const VulkanImage& msaaColorImage, const VulkanImage& resolveImage,
        vk::SurfaceFormatKHR swapchainFormat, vk::Extent2D swapchainExtent) const
{
    // Index-enum sweep local to this calibration pass — its 2-attachment shape
    // (color + resolve, no stencil) does not match the scene render pass's
    // RenderPassAttachment (jam_VulkanGraphics.h), so it is not reused here.
    enum class CalibrationAttachment : uint8_t
    {
        color = 0,
        resolve = 1
    };
    constexpr size_t calibrationAttachmentCount { 2 };

    vk::RenderPass renderPass {};
    vk::Framebuffer framebuffer {};
    vk::PipelineLayout pipelineLayout {};
    vk::ShaderModule vertModule {};
    vk::ShaderModule fragModule {};
    vk::Pipeline pipeline {};

    std::array<vk::AttachmentDescription, calibrationAttachmentCount> attachments {};
    auto& colorAttachment { attachments.at (static_cast<size_t> (CalibrationAttachment::color)) };
    colorAttachment.format = swapchainFormat.format;
    colorAttachment.samples = candidate;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

    auto& resolveAttachment { attachments.at (static_cast<size_t> (CalibrationAttachment::resolve)) };
    resolveAttachment.format = swapchainFormat.format;
    resolveAttachment.samples = vk::SampleCountFlagBits::e1;
    resolveAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
    resolveAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    resolveAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    resolveAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    resolveAttachment.initialLayout = vk::ImageLayout::eUndefined;
    resolveAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

    const vk::AttachmentReference colorRef { static_cast<uint32_t> (CalibrationAttachment::color), vk::ImageLayout::eColorAttachmentOptimal };
    const vk::AttachmentReference resolveRef { static_cast<uint32_t> (CalibrationAttachment::resolve), vk::ImageLayout::eColorAttachmentOptimal };

    const vk::SubpassDescription subpass { {}, vk::PipelineBindPoint::eGraphics, {}, {}, 1, &colorRef, &resolveRef };

    const vk::RenderPassCreateInfo rpInfo { {}, attachments, subpass };

    // Each step below has a hard Vulkan dependency on the previous step's
    // handle (framebuffer needs renderPass, the pipeline needs
    // pipelineLayout/renderPass) — ready gates every later step, so a failure
    // leaves whatever handle is already populated (or empty if never reached)
    // for the single return below; this function performs no cleanup — the
    // caller's unconditional destroyCalibrationResources() call handles
    // cleanup for exactly the subset that was actually created.
    bool ready { device.getDevice().createRenderPass (&rpInfo, nullptr, &renderPass) == vk::Result::eSuccess };

    if (ready)
    {
        const std::array<vk::ImageView, calibrationAttachmentCount> fbAttachments {
            msaaColorImage.getAttachmentView(), resolveImage.getAttachmentView()
        };
        const vk::FramebufferCreateInfo fbInfo { {}, renderPass, fbAttachments,
                                                 swapchainExtent.width, swapchainExtent.height, 1 };

        ready = device.getDevice().createFramebuffer (&fbInfo, nullptr, &framebuffer) == vk::Result::eSuccess;
    }

    if (ready)
    {
        const vk::PipelineLayoutCreateInfo calibrationLayoutInfo { {}, 0, nullptr, 0, nullptr };

        ready = device.getDevice().createPipelineLayout (&calibrationLayoutInfo, nullptr, &pipelineLayout)
                == vk::Result::eSuccess;
    }

    if (ready)
    {
        const BinaryData::Raw calibrationVert { files::calibrationVert };
        vertModule = VulkanPipelines::createShaderModule (device.getDevice(), calibrationVert.data, calibrationVert.size);
        const BinaryData::Raw calibrationFrag { files::calibrationFrag };
        fragModule = VulkanPipelines::createShaderModule (device.getDevice(), calibrationFrag.data, calibrationFrag.size);

        ready = vertModule != nullptr and fragModule != nullptr;
    }

    if (ready)
    {
        const std::array<vk::PipelineShaderStageCreateInfo, 2> stages {
            vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eVertex, vertModule, "main" },
            vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eFragment, fragModule, "main" }
        };

        const vk::PipelineVertexInputStateCreateInfo vertexInputInfo {};

        const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo { {}, vk::PrimitiveTopology::eTriangleList };

        const vk::PipelineViewportStateCreateInfo viewportInfo { {}, 1, nullptr, 1, nullptr };

        const vk::PipelineRasterizationStateCreateInfo rasterizerInfo { {}, false, false, vk::PolygonMode::eFill,
                                                                        vk::CullModeFlagBits::eNone,
                                                                        vk::FrontFace::eCounterClockwise,
                                                                        false, 0.0f, 0.0f, 0.0f, 1.0f };

        const vk::PipelineMultisampleStateCreateInfo multisampleInfo { {}, candidate };

        const vk::PipelineColorBlendAttachmentState blendAttachment { false, {}, {}, {}, {}, {}, {},
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };

        const vk::PipelineColorBlendStateCreateInfo colorBlendInfo { {}, false, vk::LogicOp::eClear, 1, &blendAttachment };

        const std::array<vk::DynamicState, 2> dynamicStates { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        const vk::PipelineDynamicStateCreateInfo dynamicStateInfo { {}, dynamicStates };

        vk::GraphicsPipelineCreateInfo pipelineInfo {};
        pipelineInfo.stageCount = static_cast<uint32_t> (stages.size());
        pipelineInfo.pStages = stages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
        pipelineInfo.pViewportState = &viewportInfo;
        pipelineInfo.pRasterizationState = &rasterizerInfo;
        pipelineInfo.pMultisampleState = &multisampleInfo;
        pipelineInfo.pColorBlendState = &colorBlendInfo;
        pipelineInfo.pDynamicState = &dynamicStateInfo;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        ready = device.getDevice().createGraphicsPipelines (
            nullptr, 1, &pipelineInfo, nullptr, &pipeline) == vk::Result::eSuccess;
    }

    return { ready, renderPass, framebuffer, pipelineLayout, vertModule, fragModule, pipeline };
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
