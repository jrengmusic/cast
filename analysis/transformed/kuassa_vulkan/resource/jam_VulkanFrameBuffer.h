namespace jam
{
/*____________________________________________________________________________*/
/** @brief Per-frame host-visible vertex and index buffer pair.
 *
 *  Unified replacement for both GlyphFrameVB and PathFrameVB. Owns two
 *  persistently-mapped VMA buffers (vertex + index) that grow on demand
 *  via reserve(). Usage counters are reset each frame by resetUsage()
 *  without reallocating.
 *
 *  The vertex stride (floats per vertex) is set at construction and governs
 *  byte-size calculations on every grow: 12 for glyphs, 2 for paths.
 *
 *  Move-only. Non-copyable.
 */
class VulkanFrameBuffer
{
public:
    /** @brief Default constructor — produces an empty, uninitialised VulkanFrameBuffer. */
    VulkanFrameBuffer() = default;

    /** @brief Allocates the initial vertex and index buffers.
     *
     *  Both buffers are created with VMA_MEMORY_USAGE_CPU_ONLY and
     *  VMA_ALLOCATION_CREATE_MAPPED_BIT for persistent host access.
     *
     *  @param allocator       The VMA allocator that will own the backing memory.
     *  @param initialCapacity Initial element capacity for both vertex and index buffers.
     *  @param vertexStride    Number of floats per vertex (e.g. 12 for glyphs, 2 for paths).
     */
    VulkanFrameBuffer (VmaAllocator allocator, int initialCapacity, int vertexStride);

    /** @brief Destroys owned vertex and index buffers via RAII VulkanBuffer destructors. */
    ~VulkanFrameBuffer() = default;

    VulkanFrameBuffer (const VulkanFrameBuffer&) = delete;
    VulkanFrameBuffer& operator= (const VulkanFrameBuffer&) = delete;

    /** @brief Move constructor — transfers ownership, leaves source empty. */
    VulkanFrameBuffer (VulkanFrameBuffer&&) = default;

    /** @brief Move assignment — transfers ownership, leaves source empty. */
    VulkanFrameBuffer& operator= (VulkanFrameBuffer&&) = default;

    /** @brief Resets usage counters to zero and frees the previous buffers.
     *
     *  Call once per frame at beginFrame() AFTER the beginFrame() fence wait
     *  (vk::Device::waitForFences()) — the fence guarantees the GPU has
     *  finished executing all prior command buffers, so the previous buffers
     *  (those grown away mid-frame) are safe to destroy here.
     */
    void resetUsage() noexcept
    {
        usedVertices = 0;
        usedIndices  = 0;
        previousBuffers.clear();
    }

    /** @brief Reserves capacity for at least the requested vertex and index counts.
     *
     *  Grows using power-of-2 doubling if either capacity is exceeded.
     *  On grow, old buffers are moved to previousBuffers (kept alive until the next
     *  resetUsage() call) so GPU commands recorded against them remain valid.
     *
     *  @param requiredVertices  Minimum vertex element capacity required.
     *  @param requiredIndices   Minimum index element capacity required.
     */
    void reserve (int requiredVertices, int requiredIndices);

    /** @brief Returns the underlying vk::Buffer handle for vertex data. */
    vk::Buffer getVertexBuffer() const noexcept { return vertexBuffer.getBuffer(); }

    /** @brief Returns the underlying vk::Buffer handle for index data. */
    vk::Buffer getIndexBuffer() const noexcept { return indexBuffer.getBuffer(); }

    /** @brief Returns the persistently-mapped pointer for vertex data. */
    void* getVertexMapped() const noexcept { return vertexBuffer.getMapped(); }

    /** @brief Returns the persistently-mapped pointer for index data. */
    void* getIndexMapped() const noexcept { return indexBuffer.getMapped(); }

    /** @brief Returns the number of vertex elements currently written. */
    int getUsedVertices() const noexcept { return usedVertices; }

    /** @brief Returns the number of index elements currently written. */
    int getUsedIndices() const noexcept { return usedIndices; }

    /** @brief Returns the current vertex element capacity. */
    int getCapacityVertices() const noexcept { return capacityVertices; }

    /** @brief Returns the current index element capacity. */
    int getCapacityIndices() const noexcept { return capacityIndices; }

    /** @brief Returns true if both the vertex and index buffers were successfully allocated. */
    bool isValid() const noexcept { return vertexBuffer.isValid() and indexBuffer.isValid(); }

    /** @brief Sets the number of vertex elements written by an external writer.
     *
     *  @param count  Number of vertex elements written into the mapped vertex buffer.
     */
    void setUsedVertices (int count) noexcept { usedVertices = count; }

    /** @brief Sets the number of index elements written by an external writer.
     *
     *  @param count  Number of index elements written into the mapped index buffer.
     */
    void setUsedIndices (int count) noexcept { usedIndices = count; }

private:
    /** @brief The VMA allocator used for buffer creation and growth. */
    VmaAllocator allocator { VK_NULL_HANDLE };

    /** @brief RAII-owned vertex buffer, persistently mapped. */
    VulkanBuffer vertexBuffer;

    /** @brief RAII-owned index buffer, persistently mapped. */
    VulkanBuffer indexBuffer;

    /** @brief Number of vertex elements currently in use this frame. */
    int usedVertices { 0 };

    /** @brief Number of index elements currently in use this frame. */
    int usedIndices { 0 };

    /** @brief Current vertex element capacity. */
    int capacityVertices { 0 };

    /** @brief Current index element capacity. */
    int capacityIndices { 0 };

    /** @brief Number of floats per vertex — governs vertex buffer byte-size calculations. */
    int vertexStride { 0 };

    /** @brief Old buffers kept alive until the GPU finishes the current command
     *  buffer. Owner sweep — vector-of-unique_ptr, pointer-stable, auto-deleting
     *  (jam_core/utilities/jam_Owner.h). */
    jam::Owner<VulkanBuffer> previousBuffers {};
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam