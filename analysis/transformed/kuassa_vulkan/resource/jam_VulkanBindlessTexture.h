namespace jam
{
/*____________________________________________________________________________*/
/** @brief GPU-resident texture: owns a jam::VulkanImage plus a per-window
 *  bindless-array registry and the staging upload/re-upload. One SSOT upload
 *  path shared by every producer of a bindless-registered texture — today the
 *  glyph atlas's GPU mirror (mutable, one VulkanEngine-wide shared instance).
 *
 *  Two-tier vocabulary: jam::VulkanImage is the low-level RAII vk::Image
 *  handle; VulkanBindlessTexture is the GPU-resident, bindless-registered,
 *  uploadable resource built on top of it.
 *
 *  CPU pixel source is caller-supplied (juce::Image::BitmapData) via upload()
 *  — the glyph rasterizer and any future juce::ImageFileFormat file-decoder
 *  producer share this same upload path. Re-upload re-copies into the
 *  existing vk::Image (VMA has no in-place image update); it never
 *  reallocates.
 *
 *  The underlying image may be shared across every window (the glyph atlas is
 *  one VulkanEngine-wide instance), while each window's bindless descriptor
 *  array is per-window — the assigned slot for this SAME image therefore
 *  differs per window. registry below is that per-window table,
 *  keyed by the native window handle (juce::ComponentPeer::getNativeHandle())
 *  — the same void* key idiom as jam::VulkanEngine::contexts.
 *
 *  This type does not assign a slot or write a descriptor itself —
 *  jam::VulkanGraphics remains the orchestrator: it assigns a slot
 *  (VulkanGraphics::bindlessRegistry.assignBindlessIndex()), writes the descriptor
 *  (VulkanGraphics::writeBindlessTextureDescriptor()), and records the assigned
 *  index here via registerBindlessIndex() below — exactly today's division of
 *  labor, relocated bookkeeping only.
 *
 *  Move-only (RAII). Non-copyable.
 */
class VulkanBindlessTexture
{
public:
    VulkanBindlessTexture() = default;

    /** @brief Creates the DEVICE_LOCAL image backing this texture, mirroring
     *  the former GlyphAtlas::createAtlasImage()'s image-creation shape (2D,
     *  1 sample, OPTIMAL tiling, VMA_MEMORY_USAGE_GPU_ONLY) via
     *  jam::VulkanImage::create2D() — width/height/format/usage/mip levels
     *  are the only fields that ever varied across call sites.
     *  @param device        Shared Vulkan device.
     *  @param width         VulkanImage width in texels.
     *  @param height        VulkanImage height in texels.
     *  @param format        Pixel format.
     *  @param usage         VulkanImage usage flags (e.g. eTransferDst | eSampled
     *                       for a staging-uploaded texture).
     *  @param numMipLevels  Mip levels — defaulted to 1, every current
     *                       caller's exact prior behavior.
     *  @return true if the image was created successfully.
     */
    bool create (VulkanDevice& device, int width, int height, vk::Format format,
                vk::ImageUsageFlags usage, uint32_t numMipLevels = 1)
    {
        extent = vk::Extent2D { static_cast<uint32_t> (width), static_cast<uint32_t> (height) };
        image = VulkanImage::create2D (device.getAllocator(), device.getDevice(), format,
                                 extent, vk::SampleCountFlagBits::e1, usage,
                                 vk::ImageAspectFlagBits::eColor, numMipLevels);

        return image.isValid();
    }

    /** @brief (Re)uploads pixels into this texture's own owned VulkanImage via
     *  the caller's own staging arena: memcpy into the reserved staging
     *  region, UNDEFINED -> TRANSFER_DST_OPTIMAL barrier, buffer-to-image
     *  copy, TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL barrier.
     *  @param commandBuffer  Active command buffer to record the upload into.
     *  @param stagingBuffer  The caller's own staging-arena vk::Buffer,
     *                        already sized to hold at least stagingOffset +
     *                        this upload's byte count.
     *  @param stagingMapped  Persistently-mapped pointer for stagingBuffer —
     *                        the memcpy destination base; this upload writes
     *                        at stagingMapped + stagingOffset.
     *  @param stagingOffset  Byte offset within stagingBuffer already
     *                        reserved for this upload by the caller.
     *  @param bmp            Source bitmap data.
     *  @param width          Upload width in pixels.
     *  @param height         Upload height in pixels.
     *  @return true on success.
     */
    bool upload (vk::CommandBuffer commandBuffer, vk::Buffer stagingBuffer, void* stagingMapped,
                vk::DeviceSize stagingOffset, const juce::Image::BitmapData& bmp, int width, int height)
    {
        jassert (bmp.data != nullptr and bmp.pixelStride > 0);
        jassert (stagingMapped != nullptr);

        bool uploaded { false };

        if (bmp.data != nullptr and bmp.pixelStride > 0 and stagingMapped != nullptr)
        {
            const vk::DeviceSize stagingSize { static_cast<vk::DeviceSize> (bmp.lineStride)
                                             * static_cast<vk::DeviceSize> (height) };

            // Write into the caller's already-reserved arena region — never a
            // buffer owned here (see class doc's staging note). The region was
            // sized and offset by the caller's own allocateStaging() call.
            std::memcpy (static_cast<uint8_t*> (stagingMapped) + stagingOffset,
                         bmp.data, static_cast<size_t> (stagingSize));

            const vk::Image dst { image.getImage() };

            recordUploadBarrier (commandBuffer, dst,
                                 vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

            const vk::BufferImageCopy region { stagingOffset,
                                               static_cast<uint32_t> (bmp.lineStride / bmp.pixelStride),
                                               0,
                                               vk::ImageSubresourceLayers { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
                                               vk::Offset3D { 0, 0, 0 },
                                               vk::Extent3D { static_cast<uint32_t> (width), static_cast<uint32_t> (height), 1 } };

            commandBuffer.copyBufferToImage (stagingBuffer, dst, vk::ImageLayout::eTransferDstOptimal, region);

            recordUploadBarrier (commandBuffer, dst,
                                 vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

            uploaded = true;
        }

        return uploaded;
    }

    /** @brief Returns the bindless array slot assigned to this texture for
     *  the given window, or -1 if not yet assigned — mirrors the former
     *  VulkanGraphics::getAtlasBindlessIndex()'s contains+at idiom, keyed by window
     *  handle instead of atlas type.
     *  @param windowHandle  Native window handle
     *                       (juce::ComponentPeer::getNativeHandle()). */
    int getBindlessIndex (void* windowHandle) const noexcept
    {
        return registry.contains (windowHandle) ? registry.at (windowHandle) : -1;
    }

    /** @brief Records the bindless array slot @p index assigned to this
     *  texture for the given window. Called by VulkanGraphics after it assigns the
     *  slot (VulkanGraphics::bindlessRegistry.assignBindlessIndex()) and writes the descriptor
     *  (VulkanGraphics::writeBindlessTextureDescriptor()) — this texture itself
     *  never assigns a slot or writes a descriptor.
     *  @param windowHandle  Native window handle.
     *  @param index         Assigned bindless array slot. */
    void registerBindlessIndex (void* windowHandle, int index) noexcept
    {
        auto [slotEntry, inserted] = registry.emplace (windowHandle, index);

        if (not inserted)
        {
            auto& [handleKey, slot] = *slotEntry;
            slot = index;
        }
    }

    /** @brief Clears the given window's assigned slot — called once that
     *  window's VulkanGraphics (and its bindless descriptor array) is torn down,
     *  so a stale registered slot assignment never lingers in this texture's
     *  registry. Matches VulkanGraphics::bindlessRegistry.releaseBindlessIndex()'s
     *  tolerance: erasing an absent key is a harmless no-op.
     *  @param windowHandle  Native window handle whose slot is released. */
    void clearBindlessIndex (void* windowHandle) noexcept
    {
        registry.erase (windowHandle);
    }

    /** @brief Returns the underlying vk::Image handle. */
    vk::Image getImage() const noexcept { return image.getImage(); }

    /** @brief Returns the pixel extent this texture was created() at —
     *  VulkanShaderReflection::populate*Buffer()'s "<X>Size" source for a named
     *  LUT/texture binding (VulkanShaderTextureBindings::extents, VulkanGraphics::
     *  getSlangTextureBindings()), and VulkanGraphics::
     *  recordMipChainGeneration()'s own level-0 basis when
     *  this texture's own mipmapInput directive requested a full chain. */
    vk::Extent2D getExtent() const noexcept { return extent; }

    /** @brief Returns the full-mip-range sampling view — see
     *  jam::VulkanImage::getView(). */
    vk::ImageView getView() const noexcept { return image.getView(); }

    /** @brief Returns the single-mip attachment view — see
     *  jam::VulkanImage::getAttachmentView(). */
    vk::ImageView getAttachmentView() const noexcept { return image.getAttachmentView(); }


    /** @brief Returns true once create() has produced a valid image. */
    bool isValid() const noexcept { return image.isValid(); }

    /** @brief Move constructor — transfers image ownership and the registry. */
    VulkanBindlessTexture (VulkanBindlessTexture&&) noexcept = default;

    /** @brief Move assignment — transfers image ownership and the registry. */
    VulkanBindlessTexture& operator= (VulkanBindlessTexture&&) noexcept = default;

private:
    VulkanImage image;

    /** @brief Pixel extent this texture was created() at — see getExtent(). */
    vk::Extent2D extent {};

    /** @brief Per-window registry: window handle → this texture's assigned
     *  bindless array slot in that window — see class doc comment. */
    jam::HashMap<void*, int> registry;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanBindlessTexture)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam