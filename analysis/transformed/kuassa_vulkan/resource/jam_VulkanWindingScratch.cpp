namespace jam
{
/*____________________________________________________________________________*/
// Initial scratch extent, per axis — grown via power-of-2 doubling exactly like
// jam_FrameBuffer.cpp's defaultVertexCapacity/defaultIndexCapacity precedent.
static constexpr uint32_t defaultWindingScratchDimension { 256 };

//==============================================================================
// Constructor / Destructor
//==============================================================================

VulkanWindingScratch::VulkanWindingScratch (VulkanDevice& vulkanDevice)
    : device (vulkanDevice)
{
}

VulkanWindingScratch::~VulkanWindingScratch()
{
    for (auto fb : previousFramebuffers)
        device.getDevice().destroyFramebuffer (fb, nullptr);

    device.getDevice().destroyFramebuffer (framebuffer, nullptr);

    // previousResolveImages / resolveImage — RAII, self-destruct.
}

//==============================================================================
// Capacity
//==============================================================================

void VulkanWindingScratch::resetPrevious()
{
    for (auto fb : previousFramebuffers)
        device.getDevice().destroyFramebuffer (fb, nullptr);

    previousFramebuffers.clear();
    previousColorImages.clear();
    previousStencilImages.clear();
    previousResolveImages.clear();

    // Caller (VulkanGraphics::beginFrame()) must have already drained
    // getPreviousBindlessIndices() via VulkanGraphics::bindlessRegistry.
    // releaseBindlessIndex() before reaching this call — see that accessor's
    // doc comment for drain order.
    previousBindlessIndices.clear();
}

void VulkanWindingScratch::reserve (vk::RenderPass renderPass,
                                    vk::SurfaceFormatKHR colorFormat,
                                    vk::SampleCountFlagBits sampleCount,
                                    vk::Extent2D requiredExtent)
{
    if (requiredExtent.width > capacityExtent.width or requiredExtent.height > capacityExtent.height)
    {
        uint32_t newWidth { capacityExtent.width > 0 ? capacityExtent.width : defaultWindingScratchDimension };
        while (newWidth < requiredExtent.width) newWidth *= 2;

        uint32_t newHeight { capacityExtent.height > 0 ? capacityExtent.height : defaultWindingScratchDimension };
        while (newHeight < requiredExtent.height) newHeight *= 2;

        const vk::Extent2D newExtent { newWidth, newHeight };

        // MSAA color image — same format as swapchain. Not eSampled:
        // resolveImage below is the sampler2D target (multisample images cannot be sampled).
        VulkanImage newColorImage { VulkanImage::create2D (device.getAllocator(), device.getDevice(), colorFormat.format,
                                               newExtent, sampleCount,
                                               vk::ImageUsageFlagBits::eColorAttachment,
                                               vk::ImageAspectFlagBits::eColor) };

        // Stencil image — fully owned (never shared with the main stencil
        // attachment's clip-depth bits), MSAA.
        VulkanImage newStencilImage { VulkanImage::create2D (device.getAllocator(), device.getDevice(), vk::Format::eS8Uint,
                                                 newExtent, sampleCount,
                                                 vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                 vk::ImageAspectFlagBits::eStencil) };

        // Single-sample resolve target. eSampled for the
        // bindless composite read (VulkanGraphics::getOrCreateWindingScratch()).
        VulkanImage newResolveImage { VulkanImage::create2D (device.getAllocator(), device.getDevice(), colorFormat.format,
                                                 newExtent, vk::SampleCountFlagBits::e1,
                                                 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                                                 vk::ImageAspectFlagBits::eColor) };

        bool ready { newColorImage.isValid() };
        ready = ready and newStencilImage.isValid();
        ready = ready and newResolveImage.isValid();

        const vk::ImageView fbAttachments[3] {
            newColorImage.getAttachmentView(), newStencilImage.getAttachmentView(), newResolveImage.getAttachmentView()
        };

        const vk::FramebufferCreateInfo fbInfo { {}, renderPass, fbAttachments, newWidth, newHeight, 1 };

        vk::Framebuffer newFramebuffer {};

        const auto fbResult { device.getDevice().createFramebuffer (&fbInfo, nullptr, &newFramebuffer) };
        ready = ready and fbResult == vk::Result::eSuccess;

        jassert (ready);// VRAM / framebuffer exhaustion

        // Move to previous* — never immediately destroy. A still-recording
        // command buffer earlier this same frame may already reference the
        // OLD framebuffer/images (a prior, smaller-bbox complex fillPath()
        // call); resetPrevious() only runs after beginFrame()'s fence wait
        // confirms the GPU has finished all of THIS frame's predecessor's
        // commands.
        if (framebuffer != nullptr)
            previousFramebuffers.add (framebuffer);

        previousColorImages.add (std::make_unique<VulkanImage> (std::move (colorImage)));
        previousStencilImages.add (std::make_unique<VulkanImage> (std::move (stencilImage)));
        previousResolveImages.add (std::make_unique<VulkanImage> (std::move (resolveImage)));

        colorImage = std::move (newColorImage);
        stencilImage = std::move (newStencilImage);
        resolveImage = std::move (newResolveImage);
        framebuffer = newFramebuffer;
        capacityExtent = vk::Extent2D { newWidth, newHeight };

        // New resolve image identity — move the outgoing slot to
        // previousBindlessIndices (deferred release, see
        // getPreviousBindlessIndices()'s doc comment) before
        // invalidating; the caller (VulkanGraphics::getOrCreateWindingScratch()) then
        // re-assigns + re-writes on seeing -1, mirroring TransparencyLayer's
        // exact invalidation-on-recreate convention.
        if (bindlessIndex >= 0)
            previousBindlessIndices.add (bindlessIndex);

        bindlessIndex = -1;
    }
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam