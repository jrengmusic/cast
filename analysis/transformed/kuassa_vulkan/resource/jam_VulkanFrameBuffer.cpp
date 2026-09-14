namespace jam
{
/*____________________________________________________________________________*/
static constexpr int defaultVertexCapacity { 256 };
static constexpr int defaultIndexCapacity  { 384 };

VulkanFrameBuffer::VulkanFrameBuffer (VmaAllocator allocator, int initialCapacity, int vertexStride)
    : allocator (allocator), vertexStride (vertexStride),
      capacityVertices (initialCapacity), capacityIndices (initialCapacity)
{
    const vk::DeviceSize vertexBufferSize { static_cast<vk::DeviceSize> (initialCapacity)
        * static_cast<vk::DeviceSize> (vertexStride) * sizeof (float) };

    const vk::BufferCreateInfo vbInfo { {}, vertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer };

    VmaAllocationCreateInfo allocInfo {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    vertexBuffer = VulkanBuffer (allocator, vbInfo, allocInfo);

    const vk::DeviceSize indexBufferSize { static_cast<vk::DeviceSize> (initialCapacity) * sizeof (uint32_t) };

    const vk::BufferCreateInfo ibInfo { {}, indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer };

    indexBuffer = VulkanBuffer (allocator, ibInfo, allocInfo);
}

void VulkanFrameBuffer::reserve (int requiredVertices, int requiredIndices)
{
    if (requiredVertices > capacityVertices or requiredIndices > capacityIndices)
    {
        int newVertexCapacity { capacityVertices > 0 ? capacityVertices : defaultVertexCapacity };
        while (newVertexCapacity < requiredVertices) newVertexCapacity *= 2;

        int newIndexCapacity { capacityIndices > 0 ? capacityIndices : defaultIndexCapacity };
        while (newIndexCapacity < requiredIndices) newIndexCapacity *= 2;

        const vk::DeviceSize newVertexBufferSize { static_cast<vk::DeviceSize> (newVertexCapacity)
            * static_cast<vk::DeviceSize> (vertexStride) * sizeof (float) };

        const vk::BufferCreateInfo vbInfo { {}, newVertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer };

        VmaAllocationCreateInfo allocInfo {};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VulkanBuffer newVertexBuffer (allocator, vbInfo, allocInfo);

        const vk::DeviceSize newIndexBufferSize { static_cast<vk::DeviceSize> (newIndexCapacity) * sizeof (uint32_t) };

        const vk::BufferCreateInfo ibInfo { {}, newIndexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer };

        VulkanBuffer newIndexBuffer (allocator, ibInfo, allocInfo);

        jassert (newVertexBuffer.isValid() and newIndexBuffer.isValid());// VRAM exhaustion

        // Move the old buffers to previousBuffers — kept alive until resetUsage() after the next fence wait
        previousBuffers.add (std::make_unique<VulkanBuffer> (std::move (vertexBuffer)));
        previousBuffers.add (std::make_unique<VulkanBuffer> (std::move (indexBuffer)));
        vertexBuffer = std::move (newVertexBuffer);
        indexBuffer  = std::move (newIndexBuffer);
        capacityVertices = newVertexCapacity;
        capacityIndices  = newIndexCapacity;
        usedVertices     = 0;
        usedIndices      = 0;
    }
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam