namespace jam
{
/*____________________________________________________________________________*/
//==============================================================================
// Constructor / Destructor
//==============================================================================

VulkanBindlessRegistry::VulkanBindlessRegistry (void* windowHandle, uint32_t vulkanCapacity) noexcept
    : nativeHandle (windowHandle)
    , capacity (vulkanCapacity)
{
}

VulkanBindlessRegistry::~VulkanBindlessRegistry()
{
    // bindlessIndices holds both engine-backed and foreign (software) roots;
    // only an engine-backed root owns a VulkanBindlessTexture whose per-window
    // slot needs clearing here — dynamic_cast identifies which without a
    // stored type flag on the entry.
    for (auto& [pixelData, index] : bindlessIndices)
    {
        pixelData->listeners.remove (this);

        if (auto* vulkanPixelData { dynamic_cast<VulkanImagePixelData*> (pixelData) })
            vulkanPixelData->getResolveTexture().clearBindlessIndex (nativeHandle);
    }
}

//==============================================================================
// Bindless index allocation
//==============================================================================

int VulkanBindlessRegistry::assignBindlessIndex() noexcept
{
    if (not freeBindlessIndices.isEmpty())
    {
        const int recycled { freeBindlessIndices.last() };
        freeBindlessIndices.remove (freeBindlessIndices.size() - 1);

        return recycled;
    }

    jassert (nextBindlessTextureIndex < static_cast<int> (capacity));// capacity exhaustion is a design bug
    return nextBindlessTextureIndex++;
}

void VulkanBindlessRegistry::releaseBindlessIndex (int index) noexcept
{
    jassert (index >= 0);
    freeBindlessIndices.add (index);
}

void VulkanBindlessRegistry::registerBindlessIndex (juce::ImagePixelData* root, int index)
{
    root->listeners.add (this);
    bindlessIndices[root] = index;
}

//==============================================================================
// Drain
//==============================================================================

void VulkanBindlessRegistry::releaseRetiredIndices() noexcept
{
    for (const int previousImageIndex : previousImageBindlessIndices)
        releaseBindlessIndex (previousImageIndex);

    previousImageBindlessIndices.clear();
}

//==============================================================================
// juce::ImagePixelData::Listener — per-window bindless slot recycling
//==============================================================================

void VulkanBindlessRegistry::imageDataBeingDeleted (juce::ImagePixelData* pixelData)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if (bindlessIndices.contains (pixelData))
    {
        retireIndex (bindlessIndices.at (pixelData));
        bindlessIndices.erase (pixelData);
    }
}

void VulkanBindlessRegistry::imageDataChanged (juce::ImagePixelData* pixelData)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if (bindlessIndices.contains (pixelData))
    {
        // Engine-backed roots (VulkanImagePixelData) repaint in place — the
        // view, descriptor, and slot stay valid for the pixel data's whole
        // life; content change requires no slot action. Cache-backed
        // (foreign/software) images — same-dims dirty needs no slot action:
        // the slot stays valid, the descriptor still points to the same
        // vk::ImageView/vk::Image, and the re-upload path in
        // VulkanGraphics::cacheImageTexture writes new pixels into that same
        // image in-place. Dimension change: the cache's own imageDataChanged
        // erases the entry (may have fired before or after this callback —
        // ListenerList order not guaranteed), so release this window's slot.
        if (dynamic_cast<VulkanImagePixelData*> (pixelData) == nullptr)
        {
            bool sameDims { false };

            if (auto* cache { VulkanTextureCache::getInstance() }; cache != nullptr and cache->hasEntry (pixelData))
            {
                const auto [textureWidth, textureHeight] { cache->getEntry (pixelData).size };
                sameDims = pixelData->width == textureWidth and pixelData->height == textureHeight;
            }

            if (not sameDims)
            {
                retireIndex (bindlessIndices.at (pixelData));
                bindlessIndices.erase (pixelData);

                pixelData->listeners.remove (this);
            }
        }
    }
}

//==============================================================================
// Private helpers
//==============================================================================

void VulkanBindlessRegistry::retireIndex (int index)
{
    previousImageBindlessIndices.add (index);
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
