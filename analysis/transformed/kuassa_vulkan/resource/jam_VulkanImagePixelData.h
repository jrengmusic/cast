namespace jam
{
/*____________________________________________________________________________*/
/** @brief GPU-backed juce::ImagePixelData — its complete render-target set
 *  (MSAA color + stencil VulkanImage, resolve VulkanBindlessTexture,
 *  framebuffer) is renderer-owned residency (VulkanImageRenderer::
 *  renderTargets, keyed by this object's own native identity), created
 *  lazily on first use via getOrCreateRenderTarget() and released by
 *  removeRenderTarget() from the destructor. Extent fixed for life (JUCE
 *  images never resize). ARGB format contract.
 *
 *  Lifetime invariant: this object may outlive the GPU residency it once
 *  used — VulkanImageRenderer::shutdown() (VulkanEngine::reinitialiseDevice())
 *  releases every registered residency while the device and allocator are
 *  still alive, independently of any VulkanImagePixelData instance's own
 *  lifetime. The next call reaching getOrCreateRenderTarget() after a
 *  shutdown recreates the residency lazily — no guard needed at any call
 *  site.
 */
class VulkanImagePixelData final : public juce::ImagePixelData,
                                    public juce::ImagePixelDataBackupExtensions
{
public:
    using Ptr = juce::ReferenceCountedObjectPtr<VulkanImagePixelData>;

    /** @brief Stores the renderer reference and fixed pixel extent. Creates
     *  no GPU residency here — the first createLowLevelContext()/clone()/
     *  getResolveTexture() call lazily materialises it via
     *  getOrCreateRenderTarget().
     *  @param imageRenderer  Engine-owned shared offscreen renderer — not owned,
     *                        must outlive this VulkanImagePixelData.
     *  @param imageWidth     Fixed pixel width for this image's whole life.
     *  @param imageHeight    Fixed pixel height for this image's whole life.
     */
    VulkanImagePixelData (VulkanImageRenderer& imageRenderer, int imageWidth, int imageHeight)
        : juce::ImagePixelData (juce::Image::ARGB, imageWidth, imageHeight)
        , renderer (imageRenderer)
        , extent { static_cast<uint32_t> (imageWidth), static_cast<uint32_t> (imageHeight) }
    {
        JUCE_ASSERT_MESSAGE_THREAD
    }

    /** @brief Releases this image's own GPU residency via
     *  renderer.removeRenderTarget() — safe even if no residency was ever
     *  created (removeRenderTarget() is a no-op on a missing key). */
    ~VulkanImagePixelData() override
    {
        renderer.removeRenderTarget (this);
    }

    /** @brief Begins an offscreen frame against this image's own render
     *  target and returns the LLGC drawing into it, materialising the
     *  target lazily via getOrCreateRenderTarget() on first use.
     *  @return A VulkanLowLevelGraphicsContext drawing into this image's own
     *          resolve texture, or a software fallback context drawing into a
     *          discarded scratch image when the target could not be
     *          materialised or the offscreen frame could not begin —
     *          juce::Graphics's own constructor dereferences this return
     *          before any validity check, so a null return is never given.
     */
    std::unique_ptr<juce::LowLevelGraphicsContext> createLowLevelContext() override
    {
        sendDataChangeMessage();

        auto* target { getOrCreateRenderTarget() };
        jassert (target != nullptr);

        if (target != nullptr)
        {
            // Scale 1.0f — the image IS device pixels. JUCE's
            // StandardCachedComponentImage applies its own scale transform via
            // addTransform after receiving this context.
            if (auto context { renderer.renderTarget (target->framebuffer, extent, target->resolveTexture.getImage(), 1.0f) };
                context != nullptr)
                return context;
        }

        // JUCE's Graphics ctor dereferences this return before any validity
        // check (juce_GraphicsContext.cpp:150-152); a null return is UB, so a
        // doomed frame draws into a discarded software scratch instead.
        juce::Image scratch { juce::SoftwareImageType().create (juce::Image::ARGB, width, height, true) };
        return std::make_unique<juce::LowLevelGraphicsSoftwareRenderer> (scratch);
    }

    /** @brief Returns a VulkanImageType — the contract answer
     *  getPreferredImageTypeForTemporaryImages() reads to keep JUCE creating
     *  GPU-resident images for any operation this pixel data participates in.
     *  @return A newly constructed VulkanImageType instance.
     */
    std::unique_ptr<juce::ImageType> createType() const override;

    /** @brief Offscreen render passes CLEAR the target on every regeneration
     *  (VulkanImageRenderer::beginOffscreenFrame), so cached content is never
     *  persistent across paints. Reporting needsBackup() && !canBackup() makes
     *  StandardCachedComponentImage clear its validArea and repaint the entire
     *  component on every regeneration, instead of partial-repainting a
     *  region into content that no longer exists.
     */
    BackupExtensions* getBackupExtensions() override { return this; }
    const BackupExtensions* getBackupExtensions() const override { return this; }

    void setBackupEnabled (bool) override {}
    bool isBackupEnabled() const override { return false; }
    bool backupNow() override { return false; }
    bool needsBackup() const override { return true; }
    bool canBackup() const override { return false; }

    /** @brief Builds a fresh VulkanImagePixelData of the same size, materialises
     *  its render target, and copies this image's own content into it via
     *  juce::Graphics::drawImageAt.
     *  @return The cloned pixel data, or an empty Ptr if the clone's own
     *          render target could not be materialised.
     */
    juce::ImagePixelData::Ptr clone() override
    {
        auto cloned { std::make_unique<VulkanImagePixelData> (renderer, width, height) };

        if (cloned->getOrCreateRenderTarget() == nullptr)
            return juce::ImagePixelData::Ptr {};

        juce::Image newImage { cloned.release() };
        juce::Graphics g { newImage };
        g.drawImageAt (juce::Image { *this }, 0, 0, false);

        return juce::ImagePixelData::Ptr { newImage.getPixelData() };
    }

    /** @brief Constructs a DataReleaser sized to @p x/@p y and
     *  bitmapData.width/height, wires bitmapData to its staging buffer's own
     *  mapped pointer, and hands ownership of the releaser to bitmapData —
     *  the readback (constructor) and writeback (destructor) transfers run
     *  on the DataReleaser's own lifetime, bracketing the caller's direct
     *  pixel access.
     *  @param bitmapData  Bitmap view to initialise — receives pixelFormat/
     *                     pixelStride/lineStride/size/data/dataReleaser.
     *  @param x           Region left, in this image's own pixel space.
     *  @param y           Region top, in this image's own pixel space.
     *  @param mode        Read-only/write-only/read-write access requested.
     */
    void initialiseBitmapData (juce::Image::BitmapData& bitmapData, int x, int y,
                               juce::Image::BitmapData::ReadWriteMode mode) override
    {
        bitmapData.pixelFormat = pixelFormat;
        bitmapData.pixelStride = pixelStride;

        auto releaser { std::make_unique<DataReleaser> (this,
            juce::Rectangle<int> { x, y, bitmapData.width, bitmapData.height }, mode) };

        const auto unsignedLineStride { static_cast<size_t> (bitmapData.width) * static_cast<size_t> (pixelStride) };
        bitmapData.lineStride = static_cast<int> (unsignedLineStride);
        bitmapData.size = static_cast<size_t> (bitmapData.height) * unsignedLineStride;
        bitmapData.data = static_cast<uint8_t*> (releaser->staging.getMapped());

        bitmapData.dataReleaser = std::move (releaser);

        if (mode != juce::Image::BitmapData::readOnly)
            sendDataChangeMessage();
    }

    /** @brief Returns this image's own resolve texture, materialising the
     *  render target lazily via getOrCreateRenderTarget() on first use.
     *  @return The resolve texture — the bindless-registered, sampled/
     *          readback-capable image this pixel data exposes to callers.
     */
    VulkanBindlessTexture& getResolveTexture()
    {
        auto* target { getOrCreateRenderTarget() };
        jassert (target != nullptr);
        return target->resolveTexture;
    }

    /** @brief Thin call into renderer.getOrCreateRenderTarget(), keyed by
     *  this object's own address — see that method's own doc comment.
     *  @return Pointer to this image's own residency, or nullptr if it could
     *          not be built.
     */
    VulkanRenderTargetResources* getOrCreateRenderTarget()
    {
        return renderer.getOrCreateRenderTarget (this, extent);
    }

private:
    /** @brief Bytes per pixel for this ARGB-fixed pixel data. */
    static constexpr int pixelStride { 4 };

    // Readback/writeback releaser — mirrors GL's DataReleaser structure:
    // read-on-construct (skip writeOnly), write-back-on-destruct (skip readOnly).
    // Transport is D2D's staging shape: device-local images cannot be host-mapped;
    // copyImageToBuffer → mapped readback staging buffer (VMA_MEMORY_USAGE_AUTO +
    // MAPPED + HOST_ACCESS_RANDOM).
    struct DataReleaser final : public juce::Image::BitmapData::BitmapDataReleaser
    {
        /** @brief Allocates readback staging sized to @p areaIn and reads
         *  back @p selfIn's own resolve image into it (readback()), skipped
         *  for writeOnly access — the caller is about to overwrite the whole
         *  region and reading it first would be wasted work.
         *  @param selfIn  The pixel data this releaser reads/writes.
         *  @param areaIn  Pixel region, in selfIn's own coordinate space.
         *  @param modeIn  Read-only/write-only/read-write access requested.
         */
        DataReleaser (Ptr selfIn, juce::Rectangle<int> areaIn, juce::Image::BitmapData::ReadWriteMode modeIn)
            : self (selfIn)
            , area (areaIn)
            , mode (modeIn)
            , staging (createReadbackStagingBuffer (selfIn->renderer.getDevice().getAllocator(),
                  static_cast<vk::DeviceSize> (areaIn.getWidth()) * static_cast<vk::DeviceSize> (areaIn.getHeight())
                  * static_cast<vk::DeviceSize> (pixelStride)))
        {
            jassert (staging.isValid());

            if (mode != juce::Image::BitmapData::writeOnly)
                readback();
        }

        /** @brief Writes staging's own content back into self's resolve
         *  image (writeback()), skipped for readOnly access. */
        ~DataReleaser() override
        {
            if (mode != juce::Image::BitmapData::readOnly)
                writeback();
        }

        /** @brief The pixel data this releaser reads/writes — keeps it alive
         *  for the releaser's own lifetime. */
        Ptr self;

        /** @brief Pixel region, in self's own coordinate space. */
        juce::Rectangle<int> area;

        /** @brief Read-only/write-only/read-write access requested by the
         *  initialiseBitmapData() call that constructed this releaser. */
        juce::Image::BitmapData::ReadWriteMode mode;

        /** @brief Host-mapped readback/writeback staging buffer — the
         *  caller's own direct pixel access target. */
        VulkanBuffer staging;

    private:
        /** @brief Copies self's own resolve image into staging: barriers the
         *  resolve image for transfer read, records the copy, barriers it
         *  back to shader read, then submits and waits
         *  (self->renderer.submitAndWait()). A non-eSuccess Result is logged;
         *  eErrorDeviceLost additionally defers a
         *  VulkanEngine::reinitialiseDevice() call. */
        void readback()
        {
            auto* target { self->getOrCreateRenderTarget() };
            jassert (target != nullptr);

            const vk::CommandBuffer cmd { self->renderer.beginCommands() };

            // Barrier resolve image for transfer read.
            recordImageMemoryBarrier (cmd, target->resolveTexture.getImage(),
                                      vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferSrcOptimal,
                                      vk::ImageAspectFlagBits::eColor,
                                      vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eTransferRead,
                                      vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eTransfer);

            // Copy image region to staging buffer.
            const vk::BufferImageCopy region { 0, 0, 0,
                vk::ImageSubresourceLayers { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
                vk::Offset3D { area.getX(), area.getY(), 0 },
                vk::Extent3D { static_cast<uint32_t> (area.getWidth()),
                               static_cast<uint32_t> (area.getHeight()), 1 } };

            cmd.copyImageToBuffer (target->resolveTexture.getImage(), vk::ImageLayout::eTransferSrcOptimal,
                                   staging.getBuffer(), region);

            // Barrier resolve image back to shader read.
            recordImageMemoryBarrier (cmd, target->resolveTexture.getImage(),
                                      vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                                      vk::ImageAspectFlagBits::eColor,
                                      vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eShaderRead,
                                      vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader);

            const vk::Result submitResult { self->renderer.submitAndWait() };

            if (submitResult == vk::Result::eSuccess)
                return;

            debug::Log::write ("VulkanImagePixelData::readback: submitAndWait failed:",
                               vk::to_string (submitResult));

            if (submitResult == vk::Result::eErrorDeviceLost)
            {
                juce::MessageManager::callAsync ([]
                {
                    if (auto* reinitialiseEngine { VulkanEngine::getInstance() })
                        reinitialiseEngine->reinitialiseDevice();
                });
            }
        }

        /** @brief Copies staging's own content into self's resolve image:
         *  barriers the resolve image for transfer write, records the copy,
         *  barriers it back to shader read, then submits and waits
         *  (self->renderer.submitAndWait()). A non-eSuccess Result is logged;
         *  eErrorDeviceLost additionally defers a
         *  VulkanEngine::reinitialiseDevice() call. */
        void writeback()
        {
            auto* target { self->getOrCreateRenderTarget() };
            jassert (target != nullptr);

            const vk::CommandBuffer cmd { self->renderer.beginCommands() };

            // Barrier resolve image for transfer write.
            recordImageMemoryBarrier (cmd, target->resolveTexture.getImage(),
                                      vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferDstOptimal,
                                      vk::ImageAspectFlagBits::eColor,
                                      vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eTransferWrite,
                                      vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eTransfer);

            // Copy staging buffer to image.
            const vk::BufferImageCopy region { 0, 0, 0,
                vk::ImageSubresourceLayers { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
                vk::Offset3D { area.getX(), area.getY(), 0 },
                vk::Extent3D { static_cast<uint32_t> (area.getWidth()),
                               static_cast<uint32_t> (area.getHeight()), 1 } };

            cmd.copyBufferToImage (staging.getBuffer(), target->resolveTexture.getImage(),
                                   vk::ImageLayout::eTransferDstOptimal, region);

            // Barrier resolve image back to shader read.
            recordImageMemoryBarrier (cmd, target->resolveTexture.getImage(),
                                      vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                                      vk::ImageAspectFlagBits::eColor,
                                      vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                                      vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader);

            const vk::Result submitResult { self->renderer.submitAndWait() };

            if (submitResult == vk::Result::eSuccess)
                return;

            debug::Log::write ("VulkanImagePixelData::writeback: submitAndWait failed:",
                               vk::to_string (submitResult));

            if (submitResult == vk::Result::eErrorDeviceLost)
            {
                juce::MessageManager::callAsync ([]
                {
                    if (auto* reinitialiseEngine { VulkanEngine::getInstance() })
                        reinitialiseEngine->reinitialiseDevice();
                });
            }
        }
    };

    /** @brief Engine-owned shared offscreen renderer — not owned. Lifetime:
     *  VulkanEngine-owned (device lifetime), structurally outliving every
     *  component-owned VulkanImagePixelData (jam::SharedInstance<jam::VulkanEngine>
     *  ordering) — same pattern as VulkanDevice& in VulkanTextureCache. GPU
     *  residency (renderer.renderTargets, keyed by this) is independent of
     *  this object's own lifetime — released by VulkanImageRenderer::shutdown(),
     *  recreated lazily by getOrCreateRenderTarget(). */
    VulkanImageRenderer& renderer;

    /** @brief Fixed pixel extent for this image's whole life — JUCE images
     *  never resize. */
    vk::Extent2D extent {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanImagePixelData)
};

/*____________________________________________________________________________*/
/** @brief juce::ImageType implementation for Vulkan-backed images — the
 *  contract answer getPreferredImageTypeForTemporaryImages() returns so JUCE's
 *  StandardCachedComponentImage creates GPU-resident render targets instead of
 *  software bitmaps.
 */
class VulkanImageType : public juce::ImageType
{
public:
    VulkanImageType() = default;

    /** @brief This image type's own juce::ImageType::getTypeID() identity. */
    static constexpr int vulkanImageTypeID { 4 };

    /** @brief Returns vulkanImageTypeID. */
    int getTypeID() const override { return vulkanImageTypeID; }

    /** @brief Builds a VulkanImagePixelData sized @p imageWidth x @p imageHeight
     *  against VulkanEngine::getInstance()'s own shared VulkanImageRenderer, and
     *  materialises its render target immediately.
     *  @param imageWidth   Fixed pixel width for the new image's whole life.
     *  @param imageHeight  Fixed pixel height for the new image's whole life.
     *  @return The new pixel data, or an empty Ptr if its render target could
     *          not be materialised. Format parameter ignored — ARGB only
     *          (mirrors GL's OpenGLImageType::create).
     */
    juce::ImagePixelData::Ptr create (juce::Image::PixelFormat, int imageWidth, int imageHeight,
                                      bool /*shouldClearImage*/) const override
    {
        auto* engine { VulkanEngine::getInstance() };
        jassert (engine != nullptr);

        auto pixelData { std::make_unique<VulkanImagePixelData> (engine->getImageRenderer(), imageWidth, imageHeight) };

        if (pixelData->getOrCreateRenderTarget() != nullptr)
            return *pixelData.release();

        return juce::ImagePixelData::Ptr {};
    }

    /** @brief Recognizer — returns the resolve texture from a Vulkan-backed
     *  image, or nullptr for foreign (software/GL) images. Mirrors
     *  OpenGLImageType::getFrameBufferFrom's dynamic_cast shape.
     */
    static VulkanBindlessTexture* getTextureFrom (const juce::Image& image)
    {
        auto* pixelData { dynamic_cast<VulkanImagePixelData*> (image.getPixelData().get()) };

        if (pixelData != nullptr)
            return &pixelData->getResolveTexture();

        return nullptr;
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanImageType)
};

// Deferred definition — VulkanImageType must be complete.
inline std::unique_ptr<juce::ImageType> VulkanImagePixelData::createType() const
{
    return std::make_unique<VulkanImageType>();
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
