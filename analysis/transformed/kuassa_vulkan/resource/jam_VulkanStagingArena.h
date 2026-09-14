namespace jam
{
/*____________________________________________________________________________*/
/** @brief Owns the per-frame growable staging buffer arena backing
 *  VulkanShaderRegistry::buildExternalTexture (the RetroArch LUT/texture upload
 *  path) — the sole caller, gated behind KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
 *  (jam_VulkanShaderRegistryMesh.cpp). cacheImageTexture uploads through
 *  VulkanTextureCache's own per-upload buffer (jam_VulkanTextureCache.cpp:169)
 *  and glyph uploads own theirs (jam_VulkanLowLevelGraphicsContextGlyph.cpp:32) —
 *  neither shares this arena.
 *
 *  Growable arena (offset-allocated, replace-on-grow): getOrCreateAllocation()
 *  replaces the buffer with one at least double the prior capacity, and large
 *  enough to hold the requesting allocation past the exhausted cursor, whenever
 *  the remaining space is insufficient. On grow, the prior buffer is retired
 *  (moved to previousStagingBuffers) rather than destroyed immediately — any
 *  vk::CommandBuffer::copyBufferToImage already recorded against it earlier
 *  this frame remains valid until resetResources()'s post-fence-wait
 *  drain. Every two allocations within the same frame land at disjoint
 *  offsets — never overlapping.
 *
 *  Not copyable, not movable — holds a VulkanDevice& reference, same pattern
 *  as VulkanTransparencyStack/VulkanWindingScratch.
 */
class VulkanStagingArena
{
public:
    //==========================================================================
    // Named constants
    //==========================================================================

    /** @brief Byte alignment required for every getOrCreateAllocation() offset —
     *  satisfies vk::BufferImageCopy::bufferOffset's texel-multiple requirement
     *  for both R8 (1 byte/texel glyph mono atlas) and BGRA8 (4 bytes/texel
     *  glyph emoji atlas and generic texture) uploads sharing the same
     *  per-frame staging arena. */
    static constexpr vk::DeviceSize stagingAllocationAlignment { 16 };

    //==========================================================================
    // Nested types
    //==========================================================================

    /** @brief Result of getOrCreateAllocation() — the byte offset reserved
     *  within this arena's current per-frame staging buffer, or an invalid
     *  result if VMA buffer creation failed when growth was required. */
    struct StagingAllocation
    {
        /** @brief The staging buffer this allocation belongs to — pass directly
         *  into vk::CommandBuffer::copyBufferToImage(). */
        vk::Buffer buffer {};

        /** @brief Persistently-mapped pointer for buffer — the memcpy destination
         *  base; the caller writes at mapped + offset. */
        void* mapped { nullptr };

        /** @brief Byte offset within buffer reserved for this allocation, already
         *  aligned to stagingAllocationAlignment — pass directly as
         *  vk::BufferImageCopy::bufferOffset. */
        vk::DeviceSize offset { 0 };

        /** @brief True if the allocation succeeded. */
        bool valid { false };
    };

    //==========================================================================
    // Constructor / Destructor
    //==========================================================================

    /** @brief Stores the device reference. Owns no Vulkan handles until the
     *  first getOrCreateAllocation() call.
     *  @param device  Shared Vulkan device — not owned, must outlive this VulkanStagingArena.
     */
    explicit VulkanStagingArena (VulkanDevice& device);

    /** @brief RAII members self-destruct — textureStagingBuffer and every
     *  entry in previousStagingBuffers release their VMA memory here. */
    ~VulkanStagingArena() = default;

    //==========================================================================
    // Allocation
    //==========================================================================

    /** @brief Allocates sizeBytes from this arena, growing to at least double
     *  the current capacity — and large enough to cover this allocation past
     *  the exhausted cursor — whenever the remaining space is insufficient.
     *  On grow, the prior buffer is retired (moved to previousStagingBuffers)
     *  rather than destroyed — any vk::CommandBuffer::copyBufferToImage
     *  already recorded against it earlier this frame remains valid until
     *  resetResources()'s post-fence-wait drain (mirrors
     *  VulkanFrameBuffer::reserve's move-to-previousBuffers-on-grow pattern,
     *  resource/jam_VulkanFrameBuffer.h).
     *  @param sizeBytes  Required allocation size in bytes.
     *  @return The reserved buffer/mapped-pointer/offset, or an invalid result if
     *          VMA buffer creation failed on grow.
     */
    StagingAllocation getOrCreateAllocation (vk::DeviceSize sizeBytes);

    //==========================================================================
    // Drain
    //==========================================================================

    /** @brief Frame-boundary reset — destroys every buffer retired by
     *  getOrCreateAllocation() since the last drain, then resets the
     *  allocation cursor to zero (see the private drainRetiredBuffers()/
     *  resetAllocationCursor() split below).
     *  Call once per frame at the fence-safe point (VulkanGraphics::resetResources(),
     *  after beginFrame()'s fence wait; VulkanGraphics::beginOffscreenFrame()'s
     *  depth-0 branch, after commandBuffer.reset()) — the fence/reset guarantees
     *  the GPU has finished executing all prior command buffers, so the retired
     *  buffers (those grown away mid-frame) are safe to destroy here. */
    void resetResources() noexcept;

private:
    //==========================================================================
    // Private helpers
    //==========================================================================

    /** @brief Moves outgoingBuffer into previousStagingBuffers — kept alive
     *  until the next resetResources() call, so in-flight GPU work recorded
     *  against the old buffer completes before its VMA release.
     *  @param outgoingBuffer  The buffer being grown away.
     */
    void retireBuffer (VulkanBuffer&& outgoingBuffer);

    /** @brief Destroys every buffer in previousStagingBuffers and clears the
     *  list — resetResources()'s own retired-buffer half. */
    void drainRetiredBuffers() noexcept;

    /** @brief Resets stagingUsed to zero — resetResources()'s own
     *  cursor half, run after drainRetiredBuffers(). */
    void resetAllocationCursor() noexcept;

    //==========================================================================
    // Members
    //==========================================================================

    /** @brief Shared Vulkan device — not owned, must outlive this VulkanStagingArena. */
    VulkanDevice& device;

    /** @brief Persistent per-frame staging buffer backing getOrCreateAllocation(). */
    VulkanBuffer textureStagingBuffer {};

    /** @brief Byte cursor into textureStagingBuffer for this frame's
     *  getOrCreateAllocation() calls. Reset to 0 by resetResources(). */
    vk::DeviceSize stagingUsed { 0 };

    /** @brief Staging buffers grown away mid-frame by getOrCreateAllocation(),
     *  kept alive until resetResources()'s drain — mirrors
     *  VulkanFrameBuffer::previousBuffers's exact pattern (resource/jam_VulkanFrameBuffer.h)
     *  so vk::CommandBuffer::copyBufferToImage commands already recorded against
     *  a buffer moved to previousStagingBuffers earlier this frame remain valid.
     *  Owner sweep — vector-of-unique_ptr, pointer-stable, auto-deleting
     *  (jam_core/utilities/jam_Owner.h). */
    jam::Owner<VulkanBuffer> previousStagingBuffers {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanStagingArena)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
