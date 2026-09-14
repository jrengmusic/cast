namespace jam
{
/*____________________________________________________________________________*/
//==============================================================================
// Constructor
//==============================================================================

VulkanImageEffects::VulkanImageEffects (VulkanDevice& vulkanDevice, VulkanPipelines& vulkanPipelines)
    : device (vulkanDevice), pipelines (vulkanPipelines) {}

//==============================================================================
// Descriptor sets
//==============================================================================

bool VulkanImageEffects::createDescriptorSets (vk::DescriptorPool pool, vk::DescriptorSet newBindlessTextureDescriptorSet)
{
    bindlessTextureDescriptorSet = newBindlessTextureDescriptorSet;

    const vk::DescriptorSetLayout computeSet2 { pipelines.getComputeLayoutSet2() };
    const vk::DescriptorSetAllocateInfo blurTransposeAllocInfo { pool, computeSet2 };

    if (device.getDevice().allocateDescriptorSets (&blurTransposeAllocInfo, &blurTransposeDescriptorSet)
        != vk::Result::eSuccess)
        return false;

    const vk::DescriptorSetAllocateInfo blurOutputAllocInfo { pool, computeSet2 };

    if (device.getDevice().allocateDescriptorSets (&blurOutputAllocInfo, &blurOutputDescriptorSet)
        != vk::Result::eSuccess)
        return false;

    const vk::DescriptorSetAllocateInfo matteAllocInfo { pool, computeSet2 };

    if (device.getDevice().allocateDescriptorSets (&matteAllocInfo, &matteOutputDescriptorSet) != vk::Result::eSuccess)
        return false;

    if (blurTranspose.isValid())
    {
        const vk::DescriptorBufferInfo transposeInfo { blurTranspose.getBuffer(), 0, vk::WholeSize };
        const vk::DescriptorBufferInfo outputInfo { blurOutput.getBuffer(), 0, vk::WholeSize };
        const std::array<vk::WriteDescriptorSet, 3> blurWrites {
            vk::WriteDescriptorSet { blurTransposeDescriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &transposeInfo },
            vk::WriteDescriptorSet { blurOutputDescriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &transposeInfo },
            vk::WriteDescriptorSet { blurOutputDescriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &outputInfo } };
        device.getDevice().updateDescriptorSets (blurWrites, nullptr);
    }

    if (matteOutput.isValid())
    {
        const vk::DescriptorBufferInfo matteInfo { matteOutput.getBuffer(), 0, vk::WholeSize };
        const std::array<vk::WriteDescriptorSet, 1> matteWrites {
            vk::WriteDescriptorSet { matteOutputDescriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &matteInfo } };
        device.getDevice().updateDescriptorSets (matteWrites, nullptr);
    }

    return true;
}

//==============================================================================
// Blur
//==============================================================================

void VulkanImageEffects::reserveBlurBuffers (int width, int height)
{
    const vk::DeviceSize requiredBytes { static_cast<vk::DeviceSize> (width) * static_cast<vk::DeviceSize> (height) * bgraPixelStride };
    VmaAllocationCreateInfo allocInfo {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    const bool buffersGrown { not blurTranspose.isValid() or blurTranspose.getSize() < requiredBytes };

    if (buffersGrown)
    {
        VulkanBuffer newBlurTranspose (device.getAllocator(),
            { {}, requiredBytes, vk::BufferUsageFlagBits::eStorageBuffer }, allocInfo);
        jassert (newBlurTranspose.isValid()); // VRAM exhaustion

        VulkanBuffer newBlurOutput (device.getAllocator(),
            { {}, requiredBytes, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc }, allocInfo);
        jassert (newBlurOutput.isValid()); // VRAM exhaustion

        if (blurTranspose.isValid())
            previousBlurBuffers.add (std::make_unique<VulkanBuffer> (std::move (blurTranspose)));

        if (blurOutput.isValid())
            previousBlurBuffers.add (std::make_unique<VulkanBuffer> (std::move (blurOutput)));

        blurTranspose = std::move (newBlurTranspose);
        blurOutput = std::move (newBlurOutput);

        const vk::DescriptorBufferInfo transposeInfo { blurTranspose.getBuffer(), 0, vk::WholeSize };
        const vk::DescriptorBufferInfo outputInfo { blurOutput.getBuffer(), 0, vk::WholeSize };
        const std::array<vk::WriteDescriptorSet, 3> writes {
            vk::WriteDescriptorSet { blurTransposeDescriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &transposeInfo },
            vk::WriteDescriptorSet { blurOutputDescriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &transposeInfo },
            vk::WriteDescriptorSet { blurOutputDescriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &outputInfo } };
        device.getDevice().updateDescriptorSets (writes, nullptr);
    }
}

void VulkanImageEffects::dispatchStackBlur (vk::CommandBuffer cmd, VulkanPipelines::ComputeID id, vk::DescriptorSet storageSet,
                                            int radius, int lineCount, int lineLength, uint32_t sourceTextureIndex)
{
    const vk::PipelineLayout layout { pipelines.getComputeLayout() };

    cmd.bindPipeline (vk::PipelineBindPoint::eCompute, pipelines.getComputePipeline (id));
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eCompute, layout, 1, bindlessTextureDescriptorSet, nullptr);
    cmd.bindDescriptorSets (vk::PipelineBindPoint::eCompute, layout, 2, storageSet, nullptr);

    const VulkanStackBlurPushConstants pushConstants { radius, lineCount, lineLength, sourceTextureIndex };
    cmd.pushConstants (layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof (pushConstants), &pushConstants);

    const uint32_t groupCountX { static_cast<uint32_t> ((lineCount + stackBlurWorkgroupSize - 1) / stackBlurWorkgroupSize) };
    cmd.dispatch (groupCountX, 1, 1);
}

vk::Buffer VulkanImageEffects::blurImage (vk::CommandBuffer cmd, int width, int height, int bindlessSourceIndex, int radius)
{
    reserveBlurBuffers (width, height);

    dispatchStackBlur (cmd, VulkanPipelines::ComputeID::stackBlurTexture, blurTransposeDescriptorSet,
                       radius, height, width, static_cast<uint32_t> (bindlessSourceIndex));

    const vk::BufferMemoryBarrier transposeBarrier { vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, blurTranspose.getBuffer(), 0, vk::WholeSize };
    cmd.pipelineBarrier (vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
                         {}, nullptr, transposeBarrier, nullptr);

    dispatchStackBlur (cmd, VulkanPipelines::ComputeID::stackBlurBuffer, blurOutputDescriptorSet,
                       radius, width, height, static_cast<uint32_t> (bindlessSourceIndex));

    const vk::BufferMemoryBarrier outputBarrier { vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eTransferRead,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, blurOutput.getBuffer(), 0, vk::WholeSize };
    cmd.pipelineBarrier (vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer,
                         {}, nullptr, outputBarrier, nullptr);

    return blurOutput.getBuffer();
}

//==============================================================================
// Matte
//==============================================================================

void VulkanImageEffects::reserveMatteBuffer (int width, int height)
{
    const vk::DeviceSize requiredBytes { static_cast<vk::DeviceSize> (width) * static_cast<vk::DeviceSize> (height) * bgraPixelStride };
    VmaAllocationCreateInfo allocInfo {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (not matteOutput.isValid() or matteOutput.getSize() < requiredBytes)
    {
        VulkanBuffer newMatteOutput (device.getAllocator(),
            { {}, requiredBytes, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc }, allocInfo);
        jassert (newMatteOutput.isValid()); // VRAM exhaustion

        if (matteOutput.isValid())
            previousMatteBuffers.add (std::make_unique<VulkanBuffer> (std::move (matteOutput)));

        matteOutput = std::move (newMatteOutput);

        const vk::DescriptorBufferInfo outputInfo { matteOutput.getBuffer(), 0, vk::WholeSize };
        const std::array<vk::WriteDescriptorSet, 1> writes {
            vk::WriteDescriptorSet { matteOutputDescriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &outputInfo } };
        device.getDevice().updateDescriptorSets (writes, nullptr);
    }
}

vk::Buffer VulkanImageEffects::applyMatteChoke (vk::CommandBuffer cmd, int width, int height,
                                                int sourceBindlessIndex, int radius)
{
    const VulkanMatteChokePushConstants pushConstants { width, height,
        static_cast<uint32_t> (sourceBindlessIndex), radius };

    dispatchMatte (cmd, VulkanPipelines::ComputeID::matteChoke, pushConstants, width, height);

    return matteOutput.getBuffer();
}

vk::Buffer VulkanImageEffects::applyMatteFeather (vk::CommandBuffer cmd, int width, int height,
                                                  int sourceBindlessIndex, float radius, float curve)
{
    const VulkanMatteFeatherPushConstants pushConstants { width, height,
        static_cast<uint32_t> (sourceBindlessIndex), radius, curve };

    dispatchMatte (cmd, VulkanPipelines::ComputeID::matteFeather, pushConstants, width, height);

    return matteOutput.getBuffer();
}

//==============================================================================
// Deferred release
//==============================================================================

void VulkanImageEffects::releaseRetiredBuffers()
{
    previousBlurBuffers.clear();
    previousMatteBuffers.clear();
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
