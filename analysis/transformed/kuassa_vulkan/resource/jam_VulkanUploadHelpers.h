namespace jam
{
/*____________________________________________________________________________*/
/** @brief Bytes per texel in the blur readback staging buffer's BGRA8 layout
 *  — the resolve buffer's own packing, shared by VulkanGraphics::reserveBlurBuffers()
 *  and VulkanEngine::recordBlurAndReadback()/copyStagingToDestination(). */
constexpr int bgraPixelStride { 4 };

/** @brief Byte offset of the alpha channel within one BGRA8 texel — the
 *  channel VulkanEngine::copyStagingToDestination() reads when destination's
 *  format is juce::Image::SingleChannel. */
constexpr int bgraAlphaOffset { 3 };

/** @brief Records a single vk::ImageMemoryBarrier + vk::CommandBuffer::pipelineBarrier call.
 *
 *  Generalizes what was recordUploadBarrier()'s inline vk::ImageMemoryBarrier
 *  construction into a shared helper for every hand-filled image-transition site in
 *  jam_vulkan: this file's own recordUploadBarrier() below, plus the transitions in
 *  LowLevelGraphicsContextTransparency.cpp (7 sites) and
 *  LowLevelGraphicsContextPath.cpp (1 site). Diffing all 9 sites found image,
 *  oldLayout/newLayout, aspectMask, srcAccessMask/dstAccessMask, and srcStage/dstStage
 *  all vary independently — the SAME (oldLayout, newLayout) pair
 *  (UNDEFINED -> TRANSFER_DST_OPTIMAL) recurs across recordUploadBarrier(),
 *  colorToTransferDst, and layerStencilToTransferDst with three DIFFERENT
 *  (srcAccessMask, srcStage) combinations, so access/stage cannot be derived from the
 *  layout pair alone — every one of these 7 fields must be an explicit parameter.
 *  @param commandBuffer   Active command buffer to record the barrier into.
 *  @param image           Image being transitioned.
 *  @param oldLayout       Current layout.
 *  @param newLayout       Target layout.
 *  @param aspectMask      View subresource aspect mask (color/stencil).
 *  @param srcAccessMask   Access mask valid before the barrier.
 *  @param dstAccessMask   Access mask valid after the barrier.
 *  @param srcStage        Pipeline stage the barrier waits on.
 *  @param dstStage        Pipeline stage the barrier unblocks.
 *  @param baseMipLevel    First mip level this barrier's subresourceRange
 *                         covers — defaulted to 0, every pre-existing call
 *                         site's exact prior behavior (unchanged).
 *  @param levelCount      Mip levels this barrier's subresourceRange
 *                         covers, starting at @p baseMipLevel — defaulted to
 *                         1. VulkanGraphics::recordMipChainGeneration() is the one
 *                         caller supplying non-default values, one mip level
 *                         at a time, for a buffer-pass target whose
 *                         VulkanRenderResources::numMipLevels > 1.
 */
void recordImageMemoryBarrier (vk::CommandBuffer commandBuffer, vk::Image image,
                               vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                               vk::ImageAspectFlags aspectMask,
                               vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
                               vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage,
                               uint32_t baseMipLevel = 0, uint32_t levelCount = 1);

/** @brief Records the upload pipeline barrier transitioning image from oldLayout to newLayout.
 *  Thin wrapper over recordImageMemoryBarrier() — kept because its 4 external
 *  call sites (VulkanGraphics::cacheImageTexture x2, VulkanBindlessTexture::upload x2)
 *  read more clearly as a 4-argument call than repeating COLOR_BIT + the derived
 *  access/stage masks at every call site. Supports the two transitions used by GPU
 *  upload paths: UNDEFINED -> TRANSFER_DST_OPTIMAL (TOP_OF_PIPE -> TRANSFER stage, write
 *  access) and TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL (TRANSFER ->
 *  FRAGMENT_SHADER stage, write -> read access).
 *  @param commandBuffer  Active command buffer to record the barrier into.
 *  @param image          Image being transitioned.
 *  @param oldLayout      Current layout.
 *  @param newLayout      Target layout.
 */
void recordUploadBarrier (vk::CommandBuffer commandBuffer, vk::Image image,
                         vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

/** @brief Constructs a CPU-visible, persistently-mapped staging buffer of the given size.
 *  Called directly by each upload path that owns its own staging buffer:
 *  VulkanStagingArena::getOrCreateAllocation() (jam_VulkanStagingArena.cpp:37),
 *  VulkanTextureCache (jam_VulkanTextureCache.cpp:169),
 *  VulkanLowLevelGraphicsContextGlyph (jam_VulkanLowLevelGraphicsContextGlyph.cpp:32),
 *  and VulkanMesh (jam_VulkanMesh.h:636) — none share a buffer constructed here.
 *  @param device        Shared Vulkan device — supplies the VMA allocator.
 *  @param stagingSize   Required buffer size in bytes.
 *  @return The constructed VulkanBuffer — check isValid() before use.
 */
VulkanBuffer createStagingBuffer (VulkanDevice& device, vk::DeviceSize stagingSize);

/** @brief Constructs a CPU-visible, persistently-mapped, host-random-access
 *  staging buffer of the given size for GPU-to-CPU readback — the download
 *  counterpart to createStagingBuffer()'s upload role above: usage carries
 *  both eTransferDst (copyImageToBuffer's target) and eTransferSrc
 *  (VulkanImagePixelData::DataReleaser's writeback copies this same buffer
 *  back into the image), and allocation flags
 *  VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
 *  (vs. createStagingBuffer()'s MAPPED_BIT-only, CPU_ONLY-usage upload
 *  arena) — random access matches this buffer's own read-then-optionally-
 *  write-back access pattern. Callers: VulkanEngine::recordBlurAndReadback()
 *  (blurStaging), VulkanImagePixelData::DataReleaser (per-readback staging).
 *  @param allocator    VMA allocator to construct the buffer against.
 *  @param bufferSize   Required buffer size in bytes.
 *  @return The constructed VulkanBuffer — check isValid() before use.
 */
VulkanBuffer createReadbackStagingBuffer (VmaAllocator allocator, vk::DeviceSize bufferSize);

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam