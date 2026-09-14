namespace jam
{
/*____________________________________________________________________________*/
void recordImageMemoryBarrier (vk::CommandBuffer commandBuffer, vk::Image image,
                               vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                               vk::ImageAspectFlags aspectMask,
                               vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
                               vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage,
                               uint32_t baseMipLevel, uint32_t levelCount)
{
    const vk::ImageMemoryBarrier barrier { srcAccessMask, dstAccessMask, oldLayout, newLayout,
                                           vk::QueueFamilyIgnored, vk::QueueFamilyIgnored, image,
                                           vk::ImageSubresourceRange { aspectMask, baseMipLevel, levelCount, 0, 1 } };

    commandBuffer.pipelineBarrier (srcStage, dstStage, {}, {}, {}, barrier);
}

struct BarrierTransition
{
    vk::AccessFlags srcAccessMask;
    vk::AccessFlags dstAccessMask;
    vk::PipelineStageFlags srcStage;
    vk::PipelineStageFlags dstStage;
};

void recordUploadBarrier (vk::CommandBuffer commandBuffer, vk::Image image,
                          vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    static const jam::HashMap<std::pair<vk::ImageLayout, vk::ImageLayout>, BarrierTransition> imageLayoutTransitions
    {
        { { vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal },
          { {}, vk::AccessFlagBits::eTransferWrite,
            vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer } },
        { { vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal },
          { vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader } },
    };

    const std::pair<vk::ImageLayout, vk::ImageLayout> layoutTransition { oldLayout, newLayout };

    if (imageLayoutTransitions.contains (layoutTransition))
    {
        const auto& transition { imageLayoutTransitions.at (layoutTransition) };

        recordImageMemoryBarrier (commandBuffer, image, oldLayout, newLayout, vk::ImageAspectFlagBits::eColor,
                                  transition.srcAccessMask, transition.dstAccessMask,
                                  transition.srcStage, transition.dstStage);
    }
    else
    {
        // Unexpected layout combination — programmer error, catch in debug.
        jassertfalse;
    }
}

VulkanBuffer createStagingBuffer (VulkanDevice& device, vk::DeviceSize stagingSize)
{
    const vk::BufferCreateInfo stagingInfo { {}, stagingSize, vk::BufferUsageFlagBits::eTransferSrc };

    VmaAllocationCreateInfo stagingAllocInfo {};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    return VulkanBuffer (device.getAllocator(), stagingInfo, stagingAllocInfo);
}

VulkanBuffer createReadbackStagingBuffer (VmaAllocator allocator, vk::DeviceSize bufferSize)
{
    const vk::BufferCreateInfo bufferInfo { {}, bufferSize, vk::BufferUsageFlagBits::eTransferDst
                                                             | vk::BufferUsageFlagBits::eTransferSrc };

    VmaAllocationCreateInfo allocInfo {};
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    return VulkanBuffer { allocator, bufferInfo, allocInfo };
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam