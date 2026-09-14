namespace jam
{
/*____________________________________________________________________________*/
static constexpr int defaultRecordCapacity { 256 };

VulkanPrimitiveRecordBuffer::VulkanPrimitiveRecordBuffer (VmaAllocator allocator, int initialCapacity)
    : allocator (allocator), capacityRecords (initialCapacity)
{
    const vk::DeviceSize bufferSize { static_cast<vk::DeviceSize> (initialCapacity) * sizeof (VulkanPrimitiveRecord) };

    const vk::BufferCreateInfo bufferInfo { {}, bufferSize, vk::BufferUsageFlagBits::eStorageBuffer };

    VmaAllocationCreateInfo allocInfo {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    recordBuffer = VulkanBuffer (allocator, bufferInfo, allocInfo);
}

void VulkanPrimitiveRecordBuffer::reserve (int requiredRecords)
{
    if (requiredRecords > capacityRecords)
    {
        int newCapacity { capacityRecords > 0 ? capacityRecords : defaultRecordCapacity };
        while (newCapacity < requiredRecords) newCapacity *= 2;

        const vk::DeviceSize bufferSize { static_cast<vk::DeviceSize> (newCapacity) * sizeof (VulkanPrimitiveRecord) };

        const vk::BufferCreateInfo bufferInfo { {}, bufferSize, vk::BufferUsageFlagBits::eStorageBuffer };

        VmaAllocationCreateInfo allocInfo {};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VulkanBuffer newRecordBuffer (allocator, bufferInfo, allocInfo);
        jassert (newRecordBuffer.isValid()); // VRAM exhaustion

        // Move the old buffer to previousBuffers — kept alive until resetUsage() after the next fence wait.
        previousBuffers.add (std::make_unique<VulkanBuffer> (std::move (recordBuffer)));
        recordBuffer = std::move (newRecordBuffer);
        capacityRecords = newCapacity;
        usedRecords = 0;
    }
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam