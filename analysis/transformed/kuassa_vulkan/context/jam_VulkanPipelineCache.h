namespace jam
{
/*____________________________________________________________________________*/
// Engine-owned vk::PipelineCache RAII holder. Seeds the handle from cacheFile's
// prior contents when present, and persists the handle's data back to cacheFile
// on destruction (best-effort). Safe when the handle never came up (device
// without cache support, or a failed create) — the destructor's serialize and
// destroy steps both no-op against a null handle.
class VulkanPipelineCache
{
public:
    VulkanPipelineCache (VulkanDevice& device, const juce::File& cacheFile)
        : device (device), cacheFile (cacheFile)
    {
    }

    ~VulkanPipelineCache()
    {
        if (cache != nullptr)
        {
            const auto cacheData { device.getDevice().getPipelineCacheData (cache) };

            if (cacheData.result == vk::Result::eSuccess and not cacheData.value.empty())
            {
                cacheFile.getParentDirectory().createDirectory();
                cacheFile.replaceWithData (cacheData.value.data(), cacheData.value.size());
            }

            device.getDevice().destroyPipelineCache (cache, nullptr);
        }
    }

    vk::PipelineCache getPipelineCache() const noexcept { return cache; }

    /** @brief Destroys the device-derived vk::PipelineCache handle this
     *  object owns — the object remains reinitialisable via initialise().
     *  Safe to call against a lost device: unlike the destructor, this skips
     *  the disk-persist read (getPipelineCacheData is a data query, invalid
     *  once the device is lost) and only destroys the handle. */
    void shutdown()
    {
        if (cache != nullptr)
            device.getDevice().destroyPipelineCache (cache, nullptr);

        cache = vk::PipelineCache {};
    }

    // Seeds the handle from cacheFile's prior contents when present, then
    // creates it — deferred out of the constructor so the engine can call
    // this only once its device has become valid.
    void initialise()
    {
        jam::Array<uint8_t> priorCache;
        const bool hasPriorCache { loadCacheFromDisk (priorCache) };

        const vk::PipelineCacheCreateInfo cacheInfo { {}, hasPriorCache ? static_cast<size_t> (priorCache.size()) : 0,
                                                      hasPriorCache ? priorCache.data() : nullptr };

        vk::PipelineCache created {};
        if (device.getDevice().createPipelineCache (&cacheInfo, nullptr, &created) == vk::Result::eSuccess)
            cache = created;
    }

private:
    bool loadCacheFromDisk (jam::Array<uint8_t>& out) const
    {
        juce::MemoryBlock priorCacheData;

        if (cacheFile.loadFileAsData (priorCacheData) and not priorCacheData.isEmpty())
        {
            out.resize (static_cast<int> (priorCacheData.getSize()));
            std::memcpy (out.data(), priorCacheData.getData(), priorCacheData.getSize());
            return true;
        }

        return false;
    }

    VulkanDevice& device;
    juce::File cacheFile;
    vk::PipelineCache cache {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanPipelineCache)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
