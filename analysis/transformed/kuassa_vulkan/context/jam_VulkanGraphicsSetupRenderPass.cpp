//
// Render pass file: createRenderPass(), createFramebuffers() — the shared scene/
// scene-load/composite render passes and their framebuffers. See jam_vulkan.cpp
// for the full list of sibling translation units making up this one class's
// setup helpers.

namespace jam
{
/*____________________________________________________________________________*/
bool VulkanGraphics::createRenderPass()
{
    // Scene render pass. 3 attachments: msaaColor (resolves into resolve),
    // stencil (clip depth, shared with TransparencyLayer), resolve (single-sample —
    // sceneColorImage, the identity composite's source texture). No longer targets
    // the swapchain directly (see compositeRenderPass below).
    std::array<vk::AttachmentDescription, renderPassAttachmentCount> attachments {};

    // MSAA color
    attachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).format = swapchain.getFormat().format;
    attachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).samples = getActiveSampleCount();
    attachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).loadOp = vk::AttachmentLoadOp::eClear;
    attachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).storeOp = vk::AttachmentStoreOp::eStore;
    attachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).initialLayout = vk::ImageLayout::eUndefined;
    attachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

    // MSAA stencil. stencilStoreOp corrected to STORE (Case 2 — the pre-existing
    // DONT_CARE left clip-depth content undefined across the same cross-instance
    // resumes renderPassLoad's LOAD op exists to preserve).
    attachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).format = vk::Format::eS8Uint;
    attachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).samples = getActiveSampleCount();
    attachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).loadOp = vk::AttachmentLoadOp::eDontCare;
    attachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).storeOp = vk::AttachmentStoreOp::eDontCare;
    attachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).stencilLoadOp = vk::AttachmentLoadOp::eClear;
    attachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).stencilStoreOp = vk::AttachmentStoreOp::eStore;
    attachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).initialLayout = vk::ImageLayout::eUndefined;
    attachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    // Single-sample resolve target. loadOp DONT_CARE is always safe — the automatic
    // resolve fully overwrites this attachment's whole extent every time the subpass
    // ends (renderArea is always the full swapchain extent). finalLayout
    // SHADER_READ_ONLY_OPTIMAL — ready for the identity composite (and, via this
    // same shared render pass object, TransparencyLayer's/VulkanWindingScratch's
    // own per-target resolve images) to sample directly.
    attachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).format = swapchain.getFormat().format;
    attachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).samples = vk::SampleCountFlagBits::e1;
    attachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).loadOp = vk::AttachmentLoadOp::eDontCare;
    attachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).storeOp = vk::AttachmentStoreOp::eStore;
    attachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).initialLayout = vk::ImageLayout::eUndefined;
    attachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    const vk::AttachmentReference colorRef { static_cast<uint32_t> (RenderPassAttachment::msaaColor),
                                             vk::ImageLayout::eColorAttachmentOptimal };

    const vk::AttachmentReference stencilRef { static_cast<uint32_t> (RenderPassAttachment::stencil),
                                               vk::ImageLayout::eDepthStencilAttachmentOptimal };

    const vk::AttachmentReference resolveRef { static_cast<uint32_t> (RenderPassAttachment::resolve),
                                               vk::ImageLayout::eColorAttachmentOptimal };

    const vk::SubpassDescription subpass { {}, vk::PipelineBindPoint::eGraphics, {}, {},
                                           1, &colorRef, &resolveRef, &stencilRef };

    const vk::RenderPassCreateInfo renderPassInfo { {}, attachments, subpass };

    if (device.getDevice().createRenderPass (&renderPassInfo, nullptr, &renderPass)
        != vk::Result::eSuccess)
        return false;

    // renderPassLoad — identical layout but color/stencil use LOAD_OP_LOAD/
    // stencilLoadOp LOAD. Used by resumeRenderPass() to resume the scene pass
    // without clearing content. Compatible with the same sceneFramebuffer (same
    // attachment count, format, samples). Attachment 2 (resolve) still uses
    // loadOp DONT_CARE/initialLayout UNDEFINED — the resolve always fully
    // overwrites it regardless of which variant is active.
    {
        std::array<vk::AttachmentDescription, renderPassAttachmentCount> loadAttachments {};

        // MSAA color: LOAD preserves prior scene content.
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).format = swapchain.getFormat().format;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).samples = getActiveSampleCount();
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).loadOp = vk::AttachmentLoadOp::eLoad;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).storeOp = vk::AttachmentStoreOp::eStore;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::msaaColor)).finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

        // MSAA stencil: LOAD preserves clip-depth state across the transparency/
        // winding composite.
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).format = vk::Format::eS8Uint;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).samples = getActiveSampleCount();
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).loadOp = vk::AttachmentLoadOp::eDontCare;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).storeOp = vk::AttachmentStoreOp::eDontCare;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).stencilLoadOp = vk::AttachmentLoadOp::eLoad;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).stencilStoreOp = vk::AttachmentStoreOp::eStore;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::stencil)).finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).format = swapchain.getFormat().format;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).samples = vk::SampleCountFlagBits::e1;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).loadOp = vk::AttachmentLoadOp::eDontCare;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).storeOp = vk::AttachmentStoreOp::eStore;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).initialLayout = vk::ImageLayout::eUndefined;
        loadAttachments.at (static_cast<size_t> (RenderPassAttachment::resolve)).finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        const vk::AttachmentReference loadColorRef { static_cast<uint32_t> (RenderPassAttachment::msaaColor),
                                                     vk::ImageLayout::eColorAttachmentOptimal };

        const vk::AttachmentReference loadStencilRef { static_cast<uint32_t> (RenderPassAttachment::stencil),
                                                       vk::ImageLayout::eDepthStencilAttachmentOptimal };

        const vk::AttachmentReference loadResolveRef { static_cast<uint32_t> (RenderPassAttachment::resolve),
                                                       vk::ImageLayout::eColorAttachmentOptimal };

        const vk::SubpassDescription loadSubpass { {}, vk::PipelineBindPoint::eGraphics, {}, {},
                                                   1, &loadColorRef, &loadResolveRef, &loadStencilRef };

        const vk::RenderPassCreateInfo loadRpInfo { {}, loadAttachments, loadSubpass };

        if (device.getDevice().createRenderPass (&loadRpInfo, nullptr, &renderPassLoad)
            != vk::Result::eSuccess)
            return false;
    }

    // compositeRenderPass: the one render pass that legitimately still
    // targets the swapchain directly (the final identity composite, not UI
    // content). Single-sample, single color attachment, no depth/stencil.
    {
#if JUCE_WINDOWS
        // The composite target is compositionImage (an imported D3D11 shared
        // texture), not a swapchain image — its queue-family ownership
        // transfer barrier (VulkanGraphics::recordCompositionAcquireBarrier()/
        // recordCompositionReleaseBarrier()) performs the eUndefined ->
        // eGeneral layout transition; VUID-VkImageMemoryBarrier-
        // dstQueueFamilyIndex-12331 requires eGeneral on both sides of that
        // barrier for a D3D11_TEXTURE-backed image, so this render pass
        // leaves the layout at eGeneral throughout rather than transitioning
        // to ePresentSrcKHR.
        const vk::ImageLayout compositeInitialLayout { composition != nullptr ? vk::ImageLayout::eGeneral
                                                                                : vk::ImageLayout::eUndefined };
        const vk::ImageLayout compositeFinalLayout { composition != nullptr ? vk::ImageLayout::eGeneral
                                                                              : vk::ImageLayout::ePresentSrcKHR };
#else
        constexpr vk::ImageLayout compositeInitialLayout { vk::ImageLayout::eUndefined };
        constexpr vk::ImageLayout compositeFinalLayout { vk::ImageLayout::ePresentSrcKHR };
#endif

        const vk::AttachmentDescription compositeAttachment { {}, swapchain.getFormat().format, vk::SampleCountFlagBits::e1,
                                                               vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
                                                               vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                                                               compositeInitialLayout, compositeFinalLayout };

        const vk::AttachmentReference compositeColorRef { 0, vk::ImageLayout::eColorAttachmentOptimal };

        const vk::SubpassDescription compositeSubpass { {}, vk::PipelineBindPoint::eGraphics, {}, {},
                                                        1, &compositeColorRef };

        const vk::RenderPassCreateInfo compositeRpInfo { {}, compositeAttachment, compositeSubpass };

        if (device.getDevice().createRenderPass (&compositeRpInfo, nullptr, &compositeRenderPass)
            != vk::Result::eSuccess)
            return false;
    }

    return true;
}

bool VulkanGraphics::createFramebuffers()
{
    // Composite framebuffers — one per swapchain image, built against
    // compositeRenderPass (the identity composite). Single attachment: the
    // swapchain image itself.
#if JUCE_WINDOWS
    if (composition != nullptr)
    {
        if (compositionFramebuffer != nullptr)
        {
            device.getDevice().destroyFramebuffer (compositionFramebuffer, nullptr);
            compositionFramebuffer = nullptr;
        }

        const vk::ImageView compositionAttachmentView { compositionImage.getAttachmentView() };
        const vk::FramebufferCreateInfo compositionFbInfo { {}, compositeRenderPass, compositionAttachmentView,
                                                            swapchain.getExtent().width, swapchain.getExtent().height, 1 };

        if (device.getDevice().createFramebuffer (&compositionFbInfo, nullptr, &compositionFramebuffer)
            != vk::Result::eSuccess)
            return false;
    }
    else
    {
#endif
        swapchainFramebuffers.resize (swapchain.getImageCount());

        for (int i = 0; i < swapchain.getImageCount(); ++i)
        {
            const vk::ImageView swapchainImageView { swapchain.getImageView (i) };
            const vk::FramebufferCreateInfo fbInfo { {}, compositeRenderPass, swapchainImageView,
                                                     swapchain.getExtent().width, swapchain.getExtent().height, 1 };

            if (device.getDevice().createFramebuffer (&fbInfo, nullptr, &swapchainFramebuffers.at (i))
                != vk::Result::eSuccess)
                return false;
        }
#if JUCE_WINDOWS
    }
#endif

    // Scene framebuffer — the single instance (VulkanGraphics-owned persistent images,
    // independent of currentImageIndex), built against renderPass/renderPassLoad.
    // createSceneTarget() has already destroyed the prior sceneFramebuffer (its
    // attachments are about to be replaced) before this call runs.
    const std::array<vk::ImageView, renderPassAttachmentCount> sceneAttachments {
        sceneMsaaColorImage.getAttachmentView(), stencilImage.getAttachmentView(), sceneColorImage.getAttachmentView()
    };

    const vk::FramebufferCreateInfo sceneFbInfo { {}, renderPass, sceneAttachments,
                                                  swapchain.getExtent().width, swapchain.getExtent().height, 1 };

    return device.getDevice().createFramebuffer (&sceneFbInfo, nullptr, &sceneFramebuffer)
           == vk::Result::eSuccess;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
