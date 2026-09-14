namespace jam
{
/*____________________________________________________________________________*/
VulkanTextureCache::~VulkanTextureCache()
{
    for (auto& [pixelData, entry] : textures)
        pixelData->listeners.remove (this);

    retiredTextures.clear();

    device.getDevice().destroyFence (uploadFence, nullptr);
    device.getDevice().destroyCommandPool (uploadPool, nullptr);
}

void VulkanTextureCache::shutdown()
{
    for (auto& [pixelData, entry] : textures)
        pixelData->listeners.remove (this);

    textures.clear();
    retiredTextures.clear();

    device.getDevice().destroyFence (uploadFence, nullptr);
    device.getDevice().destroyCommandPool (uploadPool, nullptr);

    uploadPool = vk::CommandPool {};
    uploadCommandBuffer = vk::CommandBuffer {};
    uploadFence = vk::Fence {};
}

void VulkanTextureCache::initialise()
{
    const vk::CommandPoolCreateInfo poolInfo { vk::CommandPoolCreateFlagBits::eTransient
                                               | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                               device.getQueueFamily() };

    auto [result, pool] { device.getDevice().createCommandPool (poolInfo) };
    uploadPool = pool;

    const vk::CommandBufferAllocateInfo cmdAllocInfo { uploadPool, vk::CommandBufferLevel::ePrimary, 1 };

    if (result == vk::Result::eSuccess) result = device.getDevice().allocateCommandBuffers (&cmdAllocInfo, &uploadCommandBuffer);

    const vk::FenceCreateInfo fenceInfo {};

    if (result == vk::Result::eSuccess)
    {
        auto [fenceResult, fence] { device.getDevice().createFence (fenceInfo) };
        result = fenceResult;
        uploadFence = fence;
    }

    jassert (result == vk::Result::eSuccess);
}

void VulkanTextureCache::cacheImageTexture (const juce::Image& rootImage, int width, int height)
{
    JUCE_ASSERT_MESSAGE_THREAD

    const juce::Image argbImage { rootImage.convertedToFormat (juce::Image::ARGB) };
    const juce::Image::BitmapData bmp { argbImage, juce::Image::BitmapData::readOnly };

    jassert (width > 0 and height > 0);
    jassert (bmp.data != nullptr);
    jassert (bmp.pixelStride == 4);

    juce::ImagePixelData* const pixelData { rootImage.getPixelData().get() };

    if (textures.contains (pixelData))
    {
        auto& entry { textures.at (pixelData) };

        if (entry.dirty)
        {
            uploadEntry (entry, bmp, width, height);
        }
    }
    else
    {
        createEntry (pixelData, bmp, width, height);
    }
}

void VulkanTextureCache::removePeer (void* handle) noexcept
{
    for (auto& [pixelData, entry] : textures)
        entry.texture.clearBindlessIndex (handle);
}

void VulkanTextureCache::destroyRetiredTextures()
{
    JUCE_ASSERT_MESSAGE_THREAD

    if (not retiredTextures.isEmpty())
    {
        const vk::Result waitResult { device.getDevice().waitIdle() };
        jassert (waitResult == vk::Result::eSuccess or waitResult == vk::Result::eErrorDeviceLost);

        if (waitResult == vk::Result::eSuccess)
        {
            retiredTextures.clear();
        }
        else
        {
            debug::Log::write ("VulkanTextureCache: destroyRetiredTextures waitIdle failed:", vk::to_string (waitResult));

            if (waitResult == vk::Result::eErrorDeviceLost)
            {
                juce::MessageManager::callAsync ([]
                {
                    if (auto* reinitialiseEngine { VulkanEngine::getInstance() })
                        reinitialiseEngine->reinitialiseDevice();
                });
            }
        }
    }
}

vk::Result VulkanTextureCache::submitAndWait()
{
    JUCE_ASSERT_MESSAGE_THREAD

    const vk::SubmitInfo submitInfo { nullptr, nullptr, uploadCommandBuffer };

    vk::Result result { device.getQueue().submit (1, &submitInfo, uploadFence) };
    if (result == vk::Result::eSuccess) result = device.getDevice().waitForFences (1, &uploadFence, VK_TRUE, UINT64_MAX);
    if (result == vk::Result::eSuccess) result = device.getDevice().resetFences (1, &uploadFence);
    if (result == vk::Result::eSuccess) result = uploadCommandBuffer.reset ({});

    jassert (result == vk::Result::eSuccess or result == vk::Result::eErrorDeviceLost);
    return result;
}

void VulkanTextureCache::createEntry (juce::ImagePixelData* pixelData, const juce::Image::BitmapData& bmp, int width, int height)
{
    Entry entry {};
    const bool created { entry.texture.create (device, width, height, vk::Format::eB8G8R8A8Unorm,
                                                vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled) };
    jassert (created);// VRAM / device-memory exhaustion
    juce::ignoreUnused (created);

    entry.size = jam::Size<int> { width, height };
    entry.dirty = true;
    pixelData->listeners.add (this);
    textures.emplace (pixelData, std::move (entry));

    auto& emplaced { textures.at (pixelData) };
    uploadEntry (emplaced, bmp, width, height);
}

void VulkanTextureCache::uploadEntry (Entry& entry, const juce::Image::BitmapData& bmp, int width, int height)
{
    const vk::DeviceSize stagingSize { static_cast<vk::DeviceSize> (bmp.lineStride)
                                      * static_cast<vk::DeviceSize> (height) };

    VulkanBuffer staging { createStagingBuffer (device, stagingSize) };
    jassert (staging.isValid());// staging allocation failure / VRAM exhaustion

    const vk::CommandBufferBeginInfo beginInfo { vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    vk::Result result { uploadCommandBuffer.begin (&beginInfo) };

    if (result == vk::Result::eSuccess)
    {
        entry.texture.upload (uploadCommandBuffer, staging.getBuffer(), staging.getMapped(),
                              0, bmp, width, height);
        result = uploadCommandBuffer.end();
    }

    jassert (result == vk::Result::eSuccess);

    if (result == vk::Result::eSuccess)
    {
        const vk::Result submitResult { submitAndWait() };

        if (submitResult == vk::Result::eSuccess)
        {
            entry.dirty = false;
            return;
        }

        debug::Log::write ("VulkanTextureCache: upload failed:", vk::to_string (submitResult));

        if (submitResult == vk::Result::eErrorDeviceLost)
        {
            juce::MessageManager::callAsync ([]
            {
                if (auto* reinitialiseEngine { VulkanEngine::getInstance() })
                    reinitialiseEngine->reinitialiseDevice();
            });
        }
    }
}

void VulkanTextureCache::imageDataBeingDeleted (juce::ImagePixelData* pixelData)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if (textures.contains (pixelData))
    {
        retiredTextures.add (std::move (textures.at (pixelData)));
        textures.erase (pixelData);
    }
}

void VulkanTextureCache::imageDataChanged (juce::ImagePixelData* pixelData)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if (textures.contains (pixelData))
    {
        auto& entry { textures.at (pixelData) };
        const auto [textureWidth, textureHeight] { entry.size };

        if (pixelData->width == textureWidth and pixelData->height == textureHeight)
        {
            entry.dirty = true;
        }
        else
        {
            pixelData->listeners.remove (this);

            retiredTextures.add (std::move (textures.at (pixelData)));
            textures.erase (pixelData);
        }
    }
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
