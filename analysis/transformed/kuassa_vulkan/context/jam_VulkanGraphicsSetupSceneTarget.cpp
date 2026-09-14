//
// Scene-target file: createStencilImage(), createSceneTarget(),
// createStraightAlphaTarget(), and createCompositePipeline() — the scene
// render target (MSAA color + stencil + resolve), the post-process path's
// un-premultiplied scene target, and the identity composite pipeline that
// samples the resolved scene onto the swapchain. See jam_vulkan.cpp for the
// full list of sibling translation units making up this one class's setup
// helpers.

namespace jam
{
/*____________________________________________________________________________*/
bool VulkanGraphics::createStencilImage()
{
    const vk::FormatProperties formatProps { device.getPhysicalDevice().getFormatProperties (vk::Format::eS8Uint) };

    if ((formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) != vk::FormatFeatureFlags {})
    {
        // MSAA (activeSampleCount), matching the scene render pass's
        // attachment 1. Deliberately NOT vk::ImageUsageFlagBits::eTransientAttachment —
        // see createSceneTarget()'s doc comment (clip-depth must survive
        // renderPassLoad's LOAD_OP_LOAD across the same-frame resumes that
        // endTransparencyLayer()/recordWindingCompositeDrawCommands() perform, which
        // the Vulkan spec forbids on a transient-usage image).
        // eTransferSrc — beginOffscreenTransparencyRenderPass() copies
        // this image's content into a transparency layer's own isolated stencil
        // when an outer clip is active, via vk::CommandBuffer::copyImage (requires the
        // source image to have been created with this usage bit). Collapsed
        // via VulkanImage::create2D(). Move-assign destroys the old stencilImage (RAII)
        // before constructing the new one.
        stencilImage = VulkanImage::create2D (device.getAllocator(), device.getDevice(), vk::Format::eS8Uint,
                                        swapchain.getExtent(), getActiveSampleCount(),
                                        vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                                        vk::ImageAspectFlagBits::eStencil);

        return stencilImage.isValid();
    }

    return false;
}

//==============================================================================
// Scene render target + identity composite
//==============================================================================

bool VulkanGraphics::createSceneTarget (vk::Extent2D targetExtent)
{
    // The old framebuffer's attachments are about to be replaced — destroy it
    // first (mirrors createStencilImage()'s move-assign-destroys-old pattern,
    // which VulkanImage's RAII handles automatically; vk::Framebuffer has no RAII wrapper
    // in this codebase, so this is done explicitly, same as resize()'s existing
    // swapchainFramebuffers destroy-loop).
    if (sceneFramebuffer != nullptr)
    {
        device.getDevice().destroyFramebuffer (sceneFramebuffer, nullptr);
        sceneFramebuffer = nullptr;
    }

    // Fresh attachment images below carry no prior-frame content — the next
    // frame must CLEAR (beginRenderPass()), not LOAD.
    sceneContentValid = false;

    // MSAA color — see this method's header doc comment for why this is plain
    // GPU_ONLY memory, not vk::ImageUsageFlagBits::eTransientAttachment/lazily-allocated.
    // Collapsed via VulkanImage::create2D(). Move-assign destroys the old
    // sceneMsaaColorImage (RAII) before constructing the new one.
    sceneMsaaColorImage = VulkanImage::create2D (device.getAllocator(), device.getDevice(), swapchain.getFormat().format,
                                           targetExtent, getActiveSampleCount(),
                                           vk::ImageUsageFlagBits::eColorAttachment,
                                           vk::ImageAspectFlagBits::eColor);

    if (not sceneMsaaColorImage.isValid())
        return false;

    // Single-sample resolve target — the persistent scene texture. eSampled for
    // the identity composite's bindless read. Move-assign destroys the old
    // sceneColorImage (RAII) before constructing the new one.
    sceneColorImage = VulkanImage::create2D (device.getAllocator(), device.getDevice(), swapchain.getFormat().format,
                                       targetExtent, vk::SampleCountFlagBits::e1,
                                       vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                                       vk::ImageAspectFlagBits::eColor);

    if (not sceneColorImage.isValid())
        return false;

    // New vk::ImageView identity — sceneColorBindlessIndex itself stays
    // lifetime-stable (see its doc comment); updateSceneColorBindlessDescriptor()
    // rewrites the descriptor at that same slot to point at the new view.
    // No index churn per resize (unlike TransparencyLayer/VulkanWindingScratch,
    // which recreate on a per-level/per-growth basis rather than once per
    // VulkanGraphics instance).

    return true;
}

#if JAM_VULKAN_RUNTIME_SHADER_COMPILER
bool VulkanGraphics::createStraightAlphaTarget (vk::Extent2D targetExtent)
{
    // Mirrors createSceneTarget()'s destroy-old-framebuffer-first pattern —
    // straightAlphaFramebuffer's attachment is about to be replaced.
    if (straightAlphaFramebuffer != nullptr)
    {
        device.getDevice().destroyFramebuffer (straightAlphaFramebuffer, nullptr);
        straightAlphaFramebuffer = nullptr;
    }

    // Single-sample, full swapchain extent — the un-premultiplied straight-alpha
    // scene the post-process user shader samples through iScene/channels[]
    // instead of the premultiplied sceneColorImage directly (see
    // VulkanGraphics::recordStraightAlphaPass()'s doc comment). eSampled for that
    // bindless read; eColorAttachment as this pass's own render target;
    // eTransferSrc for VulkanGraphics::recordOriginalHistoryCopy()'s own
    // vk::CommandBuffer::copyImage read (see straightAlphaImage's own doc
    // comment, jam_VulkanGraphics.h).
    straightAlphaImage = VulkanImage::create2D (device.getAllocator(), device.getDevice(), swapchain.getFormat().format,
                                          targetExtent, vk::SampleCountFlagBits::e1,
                                          vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
                                              | vk::ImageUsageFlagBits::eTransferSrc,
                                          vk::ImageAspectFlagBits::eColor);

    if (not straightAlphaImage.isValid())
        return false;

    // The shared offscreen render pass every buffer-pass/gather-target
    // framebuffer also targets now carries an unconditional depth attachment
    // (VulkanGraphics::getOrCreateShaderOffscreenRenderPass (vk::Format)'s own doc
    // comment) — straightAlphaDepthImage supplies it here (inert scratch
    // space, never sampled/tested — straightAlphaDepthImage's own doc
    // comment), mirroring VulkanRenderResources::depthImage's own per-target build.
    straightAlphaDepthImage = VulkanImage::create2D (
        device.getAllocator(), device.getDevice(), VulkanShaderInstance::offscreenDepthFormat, targetExtent,
        vk::SampleCountFlagBits::e1, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::ImageAspectFlagBits::eDepth);

    if (not straightAlphaDepthImage.isValid())
        return false;

    // Built against the shared single-sample offscreen render pass every
    // buffer-pass framebuffer also targets — no new render pass for this one
    // full-overwrite pass.
    const std::array<vk::ImageView, 2> attachments {
        straightAlphaImage.getAttachmentView(), straightAlphaDepthImage.getAttachmentView()
    };
    const vk::FramebufferCreateInfo fbInfo { {}, shaderRegistry.getOrCreateShaderOffscreenRenderPass (swapchain.getFormat().format), attachments,
                                             targetExtent.width, targetExtent.height, 1 };

    const vk::Result createFramebufferResult { device.getDevice().createFramebuffer (
        &fbInfo, nullptr, &straightAlphaFramebuffer) };

    // New vk::ImageView identity — straightAlphaBindlessIndex stays
    // lifetime-stable, mirroring sceneColorBindlessIndex's exact convention above.

    return createFramebufferResult == vk::Result::eSuccess;
}
#endif

bool VulkanGraphics::createCompositePipeline (vk::PipelineCache pipelineCache)
{
    // Reuses the exact same shader bytecode as ID::imageInstanced (instanced.vert +
    // image.frag) — see this method's header doc comment for why this must be a
    // DISTINCT vk::Pipeline (single-sample) rather than that pipeline object itself.
    const BinaryData::Raw instancedVert { files::instancedVert };
    vk::ShaderModule vertModule {
        VulkanPipelines::createShaderModule (device.getDevice(), instancedVert.data, instancedVert.size)
    };
    const BinaryData::Raw imageFrag { files::imageFrag };
    vk::ShaderModule fragModule {
        VulkanPipelines::createShaderModule (device.getDevice(), imageFrag.data, imageFrag.size)
    };

    bool created { false };

    if (vertModule != nullptr and fragModule != nullptr)
    {
        const std::array<vk::PipelineShaderStageCreateInfo, 2> stages {
            vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eVertex, vertModule, "main" },
            vk::PipelineShaderStageCreateInfo { {}, vk::ShaderStageFlagBits::eFragment, fragModule, "main" }
        };

        // Zero bindings/attributes — instanced.vert pulls VulkanPrimitiveRecord[gl_InstanceIndex]
        // from the real set-2 SSBO, same as every other instanced pipeline.
        const vk::PipelineVertexInputStateCreateInfo vertexInputInfo {};

        const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo { {}, vk::PrimitiveTopology::eTriangleStrip };

        const vk::PipelineViewportStateCreateInfo viewportInfo { {}, 1, nullptr, 1, nullptr };

        const vk::PipelineRasterizationStateCreateInfo rasterizerInfo { {}, false, false, vk::PolygonMode::eFill,
                                                                        vk::CullModeFlagBits::eNone,
                                                                        vk::FrontFace::eCounterClockwise,
                                                                        false, 0.0f, 0.0f, 0.0f, 1.0f };

        // Single-sample — compositeRenderPass's sole attachment IS the swapchain
        // image, which can never be MSAA. See this method's header doc comment for
        // why this pipeline cannot be activeSampleCount-built like the other 21.
        const vk::PipelineMultisampleStateCreateInfo multisampleInfo { {}, vk::SampleCountFlagBits::e1 };

        // Opaque, direct write — a straight copy of the resolved scene texture.
        const vk::PipelineColorBlendAttachmentState blendAttachment { false, {}, {}, {}, {}, {}, {},
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };

        const vk::PipelineColorBlendStateCreateInfo colorBlendInfo { {}, false, vk::LogicOp::eClear, 1, &blendAttachment };

        const std::array<vk::DynamicState, 2> dynamicStates { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        const vk::PipelineDynamicStateCreateInfo dynamicStateInfo { {}, dynamicStates };

        // No pDepthStencilState — compositeRenderPass's subpass has no depth/
        // stencil attachment, so the field is ignored per Vulkan spec §9.2.
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
        pipelineInfo.layout = pipelines.getLayout();
        pipelineInfo.renderPass = compositeRenderPass;
        pipelineInfo.subpass = 0;

        created = device.getDevice().createGraphicsPipelines (
            pipelineCache, 1, &pipelineInfo, nullptr, &compositePipeline)
            == vk::Result::eSuccess;
    }

    device.getDevice().destroyShaderModule (vertModule, nullptr);
    device.getDevice().destroyShaderModule (fragModule, nullptr);

    return created;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam