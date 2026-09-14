namespace jam
{
/*____________________________________________________________________________*/
/** @brief Per-frame host-visible storage buffer of VulkanPrimitiveRecord elements.
 *
 *  Mirrors VulkanFrameBuffer's growth/replace pattern (resource/jam_FrameBuffer.h) applied
 *  to a single vk::BufferUsageFlagBits::eStorageBuffer buffer instead of a vertex/index
 *  pair — VulkanFrameBuffer is hardwired to VERTEX_BUFFER_BIT + INDEX_BUFFER_BIT and
 *  cannot represent a plain SSBO, so this is a new minimal type following the
 *  identical pattern (same member shapes/names where sensible) rather than a
 *  duplicated growth mechanism.
 *
 *  Owns one persistently-mapped VMA buffer that grows on demand via
 *  reserve(). The used-record counter is reset each frame by resetUsage()
 *  without reallocating.
 *
 *  Move-only. Non-copyable.
 */
class VulkanPrimitiveRecordBuffer
{
public:
    /** @brief Default constructor — produces an empty, uninitialised VulkanPrimitiveRecordBuffer. */
    VulkanPrimitiveRecordBuffer() = default;

    /** @brief Allocates the initial storage buffer.
     *
     *  Created with VMA_MEMORY_USAGE_CPU_ONLY and VMA_ALLOCATION_CREATE_MAPPED_BIT
     *  for persistent host access, matching VulkanFrameBuffer's buffer allocation convention.
     *
     *  @param allocator       The VMA allocator that will own the backing memory.
     *  @param initialCapacity Initial element capacity, in VulkanPrimitiveRecord units.
     */
    VulkanPrimitiveRecordBuffer (VmaAllocator allocator, int initialCapacity);

    /** @brief Destroys the owned buffer via its RAII VulkanBuffer destructor. */
    ~VulkanPrimitiveRecordBuffer() = default;

    VulkanPrimitiveRecordBuffer (const VulkanPrimitiveRecordBuffer&) = delete;
    VulkanPrimitiveRecordBuffer& operator= (const VulkanPrimitiveRecordBuffer&) = delete;

    /** @brief Move constructor — transfers ownership, leaves source empty. */
    VulkanPrimitiveRecordBuffer (VulkanPrimitiveRecordBuffer&&) = default;

    /** @brief Move assignment — transfers ownership, leaves source empty. */
    VulkanPrimitiveRecordBuffer& operator= (VulkanPrimitiveRecordBuffer&&) = default;

    /** @brief Resets the used-record counter to zero and frees the previous buffers.
     *
     *  Call once per frame at beginFrame() AFTER the beginFrame() fence wait
     *  (vk::Device::waitForFences()) — the fence guarantees the GPU has
     *  finished executing all prior command buffers, so the previous buffers
     *  (those grown away mid-frame) are safe to destroy here.
     */
    void resetUsage() noexcept
    {
        usedRecords = 0;
        previousBuffers.clear();
    }

    /** @brief Ensures the buffer can hold at least the requested record count.
     *
     *  Grows using power-of-2 doubling if capacity is exceeded. On grow, the old
     *  buffer is moved to previousBuffers (kept alive until the next resetUsage()
     *  call) so GPU commands recorded against it remain valid, and the caller must
     *  re-write any descriptor set bound to the old vk::Buffer handle.
     *
     *  @param requiredRecords  Minimum element capacity required.
     */
    void reserve (int requiredRecords);

    /** @brief Returns the underlying vk::Buffer handle. */
    vk::Buffer getBuffer() const noexcept { return recordBuffer.getBuffer(); }

    /** @brief Returns the persistently-mapped pointer for record data. */
    void* getMapped() const noexcept { return recordBuffer.getMapped(); }

    /** @brief Returns the number of records currently written. */
    int getUsedRecords() const noexcept { return usedRecords; }

    /** @brief Returns the current record capacity. */
    int getCapacityRecords() const noexcept { return capacityRecords; }

    /** @brief Returns true if the buffer was successfully allocated. */
    bool isValid() const noexcept { return recordBuffer.isValid(); }

    /** @brief Sets the number of records written by an external writer.
     *  @param count  Number of records written into the mapped buffer.
     */
    void setUsedRecords (int count) noexcept { usedRecords = count; }

private:
    /** @brief The VMA allocator used for buffer creation and growth. */
    VmaAllocator allocator { VK_NULL_HANDLE };

    /** @brief RAII-owned storage buffer, persistently mapped. */
    VulkanBuffer recordBuffer;

    /** @brief Number of records currently in use this frame. */
    int usedRecords { 0 };

    /** @brief Current record capacity. */
    int capacityRecords { 0 };

    /** @brief Old buffers kept alive until the GPU finishes the current command
     *  buffer. Owner sweep — vector-of-unique_ptr, pointer-stable, auto-deleting
     *  (jam_core/utilities/jam_Owner.h). */
    jam::Owner<VulkanBuffer> previousBuffers {};
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam