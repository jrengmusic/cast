namespace jam
{
/*____________________________________________________________________________*/
/** @brief RAII owner of a VMA-backed vk::Buffer.
 *
 *  Move-only. Ownership transfers on move, source is nulled.
 *  A persistent mapped pointer is available when VMA_ALLOCATION_CREATE_MAPPED_BIT
 *  is set at construction — no explicit map/unmap calls required.
 */
class VulkanBuffer
{
public:
    /** @brief Default constructor — produces a null/empty buffer. */
    VulkanBuffer() = default;

    /** @brief Allocates and creates a vk::Buffer via VMA.
     *
     *  @param newAllocator  The VMA allocator owning this buffer.
     *  @param bufferInfo    Vulkan buffer creation parameters.
     *  @param allocInfo     VMA allocation parameters.
     *
     *  If VMA_ALLOCATION_CREATE_MAPPED_BIT is set in allocInfo.flags the
     *  persistent mapped pointer is retrieved from VmaAllocationInfo::pMappedData.
     */
    VulkanBuffer (VmaAllocator newAllocator, const vk::BufferCreateInfo& bufferInfo, const VmaAllocationCreateInfo& allocInfo)
        : allocator (newAllocator), size (bufferInfo.size)
    {
        VmaAllocationInfo info {};
        VkBuffer createdBuffer { VK_NULL_HANDLE };
        vmaCreateBuffer (allocator, bufferInfo, &allocInfo, &createdBuffer, &allocation, &info);
        buffer = createdBuffer;

        if ((allocInfo.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0)
            mapped = info.pMappedData;
    }

    /** @brief Destroys the buffer and frees VMA memory. */
    ~VulkanBuffer()
    {
        release();
    }

    VulkanBuffer (const VulkanBuffer&) = delete;
    VulkanBuffer& operator= (const VulkanBuffer&) = delete;

    /** @brief Move constructor — transfers ownership, nulls source. */
    VulkanBuffer (VulkanBuffer&& other) noexcept
        : allocator (other.allocator), buffer (other.buffer), allocation (other.allocation),
          mapped (other.mapped), size (other.size)
    {
        other.allocator  = VK_NULL_HANDLE;
        other.buffer     = vk::Buffer {};
        other.allocation = VK_NULL_HANDLE;
        other.mapped     = nullptr;
        other.size       = 0;
    }

    /** @brief Move assignment — destroys current resource then transfers ownership. */
    VulkanBuffer& operator= (VulkanBuffer&& other) noexcept
    {
        if (this != &other)
        {
            release();

            allocator  = other.allocator;
            buffer     = other.buffer;
            allocation = other.allocation;
            mapped     = other.mapped;
            size       = other.size;

            other.allocator  = VK_NULL_HANDLE;
            other.buffer     = vk::Buffer {};
            other.allocation = VK_NULL_HANDLE;
            other.mapped     = nullptr;
            other.size       = 0;
        }

        return *this;
    }

    /** @brief Returns the underlying vk::Buffer handle. */
    vk::Buffer getBuffer() const noexcept { return buffer; }

    /** @brief Returns the persistent mapped pointer, or nullptr if not mapped. */
    void* getMapped() const noexcept { return mapped; }

    /** @brief Returns the size of the buffer in bytes. */
    vk::DeviceSize getSize() const noexcept { return size; }

    /** @brief Returns true if the buffer holds a valid vk::Buffer handle. */
    bool isValid() const noexcept { return static_cast<bool> (buffer); }

private:
    /** @brief Destroys the buffer and frees VMA memory. Guarded on buffer
     *  validity — a default-constructed VulkanBuffer has no allocator to
     *  pass to vmaDestroyBuffer(). Safe to call from the destructor and
     *  move assignment. */
    void release() noexcept
    {
        if (buffer != nullptr)
            vmaDestroyBuffer (allocator, buffer, allocation);
    }

    /** @brief The VMA allocator that owns this buffer's memory. */
    VmaAllocator allocator { VK_NULL_HANDLE };

    /** @brief The Vulkan buffer handle. */
    vk::Buffer buffer {};

    /** @brief The VMA allocation backing this buffer. */
    VmaAllocation allocation { VK_NULL_HANDLE };

    /** @brief Persistent mapped pointer, or nullptr if not persistently mapped. */
    void* mapped { nullptr };

    /** @brief Size of the buffer in bytes. */
    vk::DeviceSize size { 0 };
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam