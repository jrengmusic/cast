namespace jam
{
/*____________________________________________________________________________*/
/** @brief Engine-owned device-lifetime image texture store. Idempotent
 *  cacheImageTexture() creates, uploads, and re-uploads DEVICE_LOCAL images
 *  via its own transient command pool and fence — one synchronous upload
 *  mechanism, no dependency on any window's frame command buffer.
 */
class VulkanTextureCache : public jam::Instance<VulkanTextureCache>, private juce::ImagePixelData::Listener
{
public:
    /** @brief One cached foreign (software) juce::Image's GPU residency. */
    struct Entry
    {
        /** @brief The DEVICE_LOCAL, bindless-registered uploaded texture. */
        VulkanBindlessTexture texture;

        /** @brief Cached pixel dimensions — compared against a changed root's
         *  own width/height to detect a dimension change vs. a same-size
         *  content update. */
        jam::Size<int> size { 0, 0 };

        /** @brief True when the source content changed since the last
         *  upload — cacheImageTexture() re-uploads and clears this on success. */
        bool dirty { false };
    };

    /** @brief Stores the device reference. Owns no Vulkan handles until initialise().
     *  @param gpuDevice  Shared Vulkan device — not owned, must outlive this VulkanTextureCache.
     */
    explicit VulkanTextureCache (VulkanDevice& gpuDevice) noexcept
        : device (gpuDevice)
    {
    }

    /** @brief Deregisters this listener from every cached entry's root, then
     *  destroys the upload command pool/fence directly — every cached and
     *  retired VulkanBindlessTexture self-destructs via RAII. */
    ~VulkanTextureCache() override;

    /** @brief Destroys every device-derived handle this cache owns (cached
     *  textures, retired textures, the upload command pool/fence) and
     *  withdraws this cache's juce::ImagePixelData::Listener registrations —
     *  the object remains reinitialisable via initialise(). Safe to call
     *  against a lost device: every step below is a destroy call, never a
     *  data-read or wait. */
    void shutdown();

    /** @brief Creates the transient upload command pool, its single primary
     *  command buffer, and the upload fence — deferred out of the constructor
     *  so the engine can call this only once its device has become valid. */
    void initialise();

    /** @brief Idempotent full operation: absent root — createEntry() (DEVICE_LOCAL
     *  image + synchronous upload); present + dirty — uploadEntry() re-upload,
     *  clears dirty on success; present + clean — no-op. Normalizes rootImage
     *  to ARGB before upload (convertedToFormat is a documented no-op when
     *  already ARGB, and correctly preserves the alpha value when converting
     *  from SingleChannel — JUCE's standard alpha-mask format — the source
     *  alpha byte becomes R=G=B=A in the converted image, so callers reading
     *  .a still see the original mask value unchanged).
     *  @param rootImage  The resolved root pixel store's own juce::Image (foreign,
     *                    software-backed — engine-backed roots never reach this cache).
     *  @param width      Root pixel width.
     *  @param height     Root pixel height.
     */
    void cacheImageTexture (const juce::Image& rootImage, int width, int height);

    /** @brief Returns true if @p pixelData has a cached entry.
     *  @param pixelData  The resolved root pixel store to look up.
     */
    bool hasEntry (juce::ImagePixelData* pixelData) const noexcept { return textures.contains (pixelData); }

    /** @brief Returns the cached Entry for @p pixelData.
     *  @param pixelData  The resolved root pixel store to look up — must
     *                    satisfy hasEntry (pixelData).
     */
    Entry& getEntry (juce::ImagePixelData* pixelData) noexcept { return textures.at (pixelData); }

    /** @brief Withdraws @p handle's registered bindless slot from every
     *  entry's own per-window registry — called by VulkanEngine::removePeer()
     *  before the dying window's own descriptor set is destroyed, mirroring
     *  VulkanBindlessInstance's identical withdrawal for the glyph atlas.
     *  @param handle  The dying window's native peer handle.
     */
    void removePeer (void* handle) noexcept;

    /** @brief Drains every entry retired since the last drain — called by
     *  VulkanGraphics::beginFrame's full-init arm, before any frame is
     *  recording on the message thread, so a single waitIdle here covers
     *  every window's in-flight GPU work. No-op when nothing is retired.
     *  A failed (non-eSuccess, non-device-lost) waitIdle is logged and leaves
     *  retiredTextures untouched for the next call to retry; eErrorDeviceLost
     *  additionally defers a VulkanEngine::reinitialiseDevice() call. */
    void destroyRetiredTextures();

private:
    /** @brief Submits uploadCommandBuffer and waits on uploadFence only —
     *  not the whole queue.
     *  @return eSuccess once the upload is complete on the GPU; otherwise the
     *          first failing step's Result (submit, wait, reset fence, or
     *          reset command buffer).
     */
    vk::Result submitAndWait();

    /** @brief Creates entry.texture (DEVICE_LOCAL, transfer-dst + sampled),
     *  registers this cache as a juce::ImagePixelData::Listener on @p pixelData,
     *  stores the new Entry in textures, and uploads it (uploadEntry()).
     *  @param pixelData  The resolved root pixel store this entry is keyed by.
     *  @param bmp        Read-only bitmap view of the ARGB-normalized source.
     *  @param width      Root pixel width.
     *  @param height     Root pixel height.
     */
    void createEntry (juce::ImagePixelData* pixelData, const juce::Image::BitmapData& bmp, int width, int height);

    /** @brief Synchronous one-shot GPU upload: local staging buffer, begin/end
     *  uploadCommandBuffer with one-time-submit, entry.texture.upload() records
     *  the copy, submitAndWait(). Clears entry.dirty only on eSuccess; a
     *  non-eSuccess Result is logged and entry.dirty stays set for the next
     *  cacheImageTexture() call to retry, with eErrorDeviceLost additionally
     *  deferring a VulkanEngine::reinitialiseDevice() call.
     *  @param entry   The entry being (re-)uploaded.
     *  @param bmp     Read-only bitmap view of the ARGB-normalized source.
     *  @param width   Root pixel width.
     *  @param height  Root pixel height.
     */
    void uploadEntry (Entry& entry, const juce::Image::BitmapData& bmp, int width, int height);

    /** @brief A dying juce::Image's pixelData may still be referenced by a
     *  bindless slot in the currently-recording frame of any window — its
     *  GPU resources are moved into retiredTextures rather than destroyed
     *  inline, and reclaimed later by destroyRetiredTextures() once no frame
     *  is recording (VulkanGraphics::beginFrame's full-init arm).
     *  @param pixelData  The root being deleted, previously registered via createEntry().
     */
    void imageDataBeingDeleted (juce::ImagePixelData* pixelData) override;

    /** @brief Same-dimension content change: marks the entry dirty for the
     *  next cacheImageTexture() call to re-upload. Dimension change: retires
     *  the entry (same move-to-retiredTextures path as imageDataBeingDeleted())
     *  and deregisters this listener.
     *  @param pixelData  The root that changed, previously registered via createEntry().
     */
    void imageDataChanged (juce::ImagePixelData* pixelData) override;

    /** @brief Shared Vulkan device — not owned, must outlive this VulkanTextureCache. */
    VulkanDevice& device;

    /** @brief Cached foreign (software) textures, keyed by resolved root pixel store. */
    jam::HashMap<juce::ImagePixelData*, Entry> textures;

    /** @brief Entries retired by imageDataBeingDeleted()/imageDataChanged()
     *  since the last destroyRetiredTextures() drain — see that method's own
     *  doc comment. */
    jam::Array<Entry> retiredTextures {};

    /** @brief Command pool backing uploadCommandBuffer. */
    vk::CommandPool uploadPool {};

    /** @brief Single primary command buffer for submitAndWait(). */
    vk::CommandBuffer uploadCommandBuffer {};

    /** @brief CPU-side fence waited by submitAndWait() — this upload's own
     *  fence, never the whole queue. */
    vk::Fence uploadFence {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanTextureCache)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
