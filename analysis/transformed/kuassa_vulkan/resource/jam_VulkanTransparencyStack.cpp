namespace jam
{
/*____________________________________________________________________________*/
//==============================================================================
// Constructor / Destructor
//==============================================================================

VulkanTransparencyStack::VulkanTransparencyStack (VulkanDevice& vulkanDevice)
    : device (vulkanDevice)
{
}

VulkanTransparencyStack::~VulkanTransparencyStack()
{
    // Framebuffer destruction needs a vk::Device — this owning container holds one,
    // so cleanup happens here instead of at the call site. RAII VulkanImage self-destructs
    // on vector clear.
    for (auto& target : stack)
        target->destroy (device.getDevice());
}

//==============================================================================
// Layer access
//==============================================================================

VulkanTransparencyStack::TransparencyLayer& VulkanTransparencyStack::getTransparencyLayer (int level)
{
    return *stack.at (static_cast<size_t> (level));
}

void VulkanTransparencyStack::TransparencyLayer::create (VmaAllocator allocator, vk::Device device,
                                                          vk::RenderPass renderPass, vk::SurfaceFormatKHR colorFormat,
                                                          vk::SampleCountFlagBits sampleCount, vk::Extent2D newExtent)
{
    // MSAA color — eTransferDst for beginOffscreenTransparencyRenderPass()'s
    // clearColorImage before every use.
    image = VulkanImage::create2D (allocator, device, colorFormat.format,
                                    newExtent, sampleCount,
                                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
                                    vk::ImageAspectFlagBits::eColor);

    // Single-sample resolve target — eSampled for the bindless composite read.
    resolveImage = VulkanImage::create2D (allocator, device, colorFormat.format,
                                          newExtent, vk::SampleCountFlagBits::e1,
                                          vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                                          vk::ImageAspectFlagBits::eColor);

    // This level's own MSAA stencil — eTransferDst for
    // beginOffscreenTransparencyRenderPass()'s copyImage/clearDepthStencilImage.
    stencil = VulkanImage::create2D (allocator, device, vk::Format::eS8Uint,
                                     newExtent, sampleCount,
                                     vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferDst,
                                     vk::ImageAspectFlagBits::eStencil);

    bool ready { image.isValid() };
    ready = ready and resolveImage.isValid();
    ready = ready and stencil.isValid();

    const vk::ImageView fbAttachments[3] {
        image.getAttachmentView(), stencil.getAttachmentView(), resolveImage.getAttachmentView() };

    const vk::FramebufferCreateInfo fbInfo { {}, renderPass, fbAttachments,
                                              newExtent.width, newExtent.height, 1 };

    const auto fbResult { device.createFramebuffer (&fbInfo, nullptr, &framebuffer) };
    ready = ready and fbResult == vk::Result::eSuccess;

    jassert (ready);
    extent = newExtent;
}

VulkanTransparencyStack::TransparencyLayer& VulkanTransparencyStack::getOrCreateLayer (int level)
{
    while (static_cast<int> (stack.size()) <= level)
        stack.add (std::make_unique<TransparencyLayer>());

    auto& target { *stack.at (static_cast<size_t> (level)) };

    if (not target.image.isValid())
    {
        jassert (currentExtent.width > 0 and currentExtent.height > 0);

        target.create (device.getAllocator(), device.getDevice(), storedRenderPass,
                       storedColorFormat, storedSampleCount, currentExtent);
    }

    return target;
}

//==============================================================================
// Extent event
//==============================================================================

void VulkanTransparencyStack::resize (vk::RenderPass renderPass, vk::SurfaceFormatKHR colorFormat,
                                      vk::SampleCountFlagBits sampleCount, vk::Extent2D extent,
                                      jam::Array<int>& outPreviousBindlessIndices)
{
    storedRenderPass = renderPass;
    storedColorFormat = colorFormat;
    storedSampleCount = sampleCount;
    currentExtent = extent;

    // Recreate every existing layer at the new extent.
    for (auto& layerPtr : stack)
    {
        auto& target { *layerPtr };

        if (target.image.isValid())
        {
            if (target.bindlessIndex >= 0)
                outPreviousBindlessIndices.add (target.bindlessIndex);

            target.destroy (device.getDevice());
            target.bindlessIndex = -1;
            target.extent = vk::Extent2D {};

            target.create (device.getAllocator(), device.getDevice(), storedRenderPass,
                           storedColorFormat, storedSampleCount, currentExtent);
        }
    }
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
