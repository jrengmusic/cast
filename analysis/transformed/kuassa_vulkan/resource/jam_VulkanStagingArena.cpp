namespace jam
{
/*____________________________________________________________________________*/
//==============================================================================
// Constructor / Destructor
//==============================================================================

VulkanStagingArena::VulkanStagingArena (VulkanDevice& vulkanDevice)
    : device (vulkanDevice)
{
}

//==============================================================================
// Allocation
//==============================================================================

VulkanStagingArena::StagingAllocation VulkanStagingArena::getOrCreateAllocation (vk::DeviceSize sizeBytes)
{
    const vk::DeviceSize alignedOffset { ((stagingUsed + stagingAllocationAlignment - 1) / stagingAllocationAlignment)
                                         * stagingAllocationAlignment };

    StagingAllocation allocation {};

    if (textureStagingBuffer.isValid() and alignedOffset + sizeBytes <= textureStagingBuffer.getSize())
    {
        allocation.buffer = textureStagingBuffer.getBuffer();
        allocation.mapped = textureStagingBuffer.getMapped();
        allocation.offset = alignedOffset;
        allocation.valid = true;
    }
    else
    {
        const vk::DeviceSize requiredCapacity { alignedOffset + sizeBytes };
        vk::DeviceSize newCapacity { textureStagingBuffer.isValid() ? textureStagingBuffer.getSize() * 2 : requiredCapacity };

        while (newCapacity < requiredCapacity)
            newCapacity *= 2;

        VulkanBuffer newStagingBuffer { createStagingBuffer (device, newCapacity) };

        if (newStagingBuffer.isValid())
        {
            if (textureStagingBuffer.isValid())
                retireBuffer (std::move (textureStagingBuffer));

            textureStagingBuffer = std::move (newStagingBuffer);

            allocation.buffer = textureStagingBuffer.getBuffer();
            allocation.mapped = textureStagingBuffer.getMapped();
            allocation.offset = 0;
            allocation.valid = true;
        }
    }

    jassert (allocation.valid);// staging allocation failure / VRAM exhaustion

    if (allocation.valid)
        stagingUsed = allocation.offset + sizeBytes;

    return allocation;
}

//==============================================================================
// Drain
//==============================================================================

void VulkanStagingArena::resetResources() noexcept
{
    drainRetiredBuffers();
    resetAllocationCursor();
}

//==============================================================================
// Private helpers
//==============================================================================

void VulkanStagingArena::retireBuffer (VulkanBuffer&& outgoingBuffer)
{
    previousStagingBuffers.add (std::make_unique<VulkanBuffer> (std::move (outgoingBuffer)));
}

void VulkanStagingArena::drainRetiredBuffers() noexcept
{
    previousStagingBuffers.clear();
}

void VulkanStagingArena::resetAllocationCursor() noexcept
{
    stagingUsed = 0;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
