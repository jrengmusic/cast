//
// Draw-state file: createCommandPool(), createSyncObjects(), createDrawState() —
// the command buffer, per-frame fence, and the projection/record/
// bindless descriptor sets. See jam_vulkan.cpp for the full list of sibling
// translation units making up this one class's setup helpers.

namespace jam
{
/*____________________________________________________________________________*/
bool VulkanGraphics::createCommandPool()
{
    const vk::CommandPoolCreateInfo poolInfo { vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                               device.getQueueFamily() };

    if (device.getDevice().createCommandPool (&poolInfo, nullptr, &commandPool) != vk::Result::eSuccess)
        return false;

    const vk::CommandBufferAllocateInfo allocInfo { commandPool, vk::CommandBufferLevel::ePrimary, 1 };

    return device.getDevice().allocateCommandBuffers (&allocInfo, &commandBuffer) == vk::Result::eSuccess;
}

bool VulkanGraphics::createSyncObjects()
{
    const vk::FenceCreateInfo fenceInfo { vk::FenceCreateFlagBits::eSignaled };

    return device.getDevice().createFence (&fenceInfo, nullptr, &inFlightFence) == vk::Result::eSuccess;
}

bool VulkanGraphics::createProjectionBuffer()
{
    // Projection storage buffer — mat4, persistently mapped. VMA_MEMORY_USAGE_CPU_ONLY
    // is deliberate, not a placeholder for VMA_MEMORY_USAGE_CPU_TO_GPU: CPU_ONLY requires
    // VMA to pick a HOST_VISIBLE|HOST_COHERENT memory type, which the Vulkan spec
    // guarantees exists on every implementation — a persistently mapped memcpy write is
    // then visible to the GPU with no vkFlushMappedMemoryRanges call. CPU_TO_GPU prefers a
    // device-local host-visible type instead, and device-local host-visible memory is not
    // guaranteed coherent: on a discrete GPU (e.g. MoltenVK's managed-storage mode) an
    // unflushed mapped write can sit in host cache and never reach the GPU. Every other
    // persistently mapped, per-frame-memcpy'd buffer in this codebase (the slang quad
    // vertex buffer below, VulkanShaderInstance's slang uniform buffer and mesh uniform
    // buffer, and each offscreen Recording's own projection buffer in
    // jam_VulkanGraphics.cpp) follows this same CPU_ONLY + MAPPED_BIT shape for the
    // same coherency reason — this comment is the authoritative statement.
    const vk::BufferCreateInfo projInfo { {}, projectionBufferSize, vk::BufferUsageFlagBits::eStorageBuffer,
                                          vk::SharingMode::eExclusive };

    VmaAllocationCreateInfo projAllocInfo {};
    projAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    projAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    projectionBuffer = VulkanBuffer (device.getAllocator(), projInfo, projAllocInfo);

    return projectionBuffer.isValid();
}

bool VulkanGraphics::createDescriptorSets()
{
    // Descriptor pool — reset at depth-0 of each offscreen epoch; each
    // beginOffscreenFrame allocates sets monotonically; each return to an
    // outer recording allocates a fresh set2. Per epoch:
    // maxRecordingsPerFrame projection storage descriptors + 2 * maxRecordingsPerFrame
    // storage descriptors. maxSets = 3 * maxRecordingsPerFrame.
    static constexpr uint32_t computeDescriptorSetCount { 3 };
    static constexpr uint32_t computeStorageDescriptorCount { computeDescriptorSetCount * 2 };

    const std::array<vk::DescriptorPoolSize, 2> poolSizes {
        vk::DescriptorPoolSize { vk::DescriptorType::eStorageBuffer, maxRecordingsPerFrame },
        vk::DescriptorPoolSize { vk::DescriptorType::eStorageBuffer, maxRecordingsPerFrame * 2 + computeStorageDescriptorCount }
    };

    const vk::DescriptorPoolCreateInfo poolInfo { {}, maxDescriptorSets + computeDescriptorSetCount, poolSizes };

    if (device.getDevice().createDescriptorPool (&poolInfo, nullptr, &descriptorPool)
        != vk::Result::eSuccess)
        return false;

    // Allocate initial projection descriptor set
    const vk::DescriptorSetLayout projectionSetLayout { pipelines.getProjectionLayout() };
    const vk::DescriptorSetAllocateInfo dsAllocInfo { descriptorPool, projectionSetLayout };

    if (device.getDevice().allocateDescriptorSets (&dsAllocInfo, &projectionDescriptorSet)
        != vk::Result::eSuccess)
        return false;

    // Write initial projection storage buffer binding
    const vk::DescriptorBufferInfo bufferDescInfo { projectionBuffer.getBuffer(), 0, projectionBufferSize };

    const vk::WriteDescriptorSet write { projectionDescriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer,
                                         nullptr, &bufferDescInfo };

    device.getDevice().updateDescriptorSets (write, nullptr);

    // Allocate initial VulkanPrimitiveRecord storage-buffer descriptor set (set 2)
    const vk::DescriptorSetLayout set2 { pipelines.getLayoutSet2() };
    const vk::DescriptorSetAllocateInfo recordAllocInfo { descriptorPool, set2 };

    if (device.getDevice().allocateDescriptorSets (&recordAllocInfo, &recordDescriptorSet)
        != vk::Result::eSuccess)
        return false;

    updateRecordDescriptorSet();

    return true;
}

bool VulkanGraphics::createSamplers()
{
    // Linear sampler
    getOrCreateLinearSampler();

    // Nearest sampler — ImageResample::nearest's counterpart to linearSampler, same owner
    // (this VulkanGraphics), created eagerly here alongside linearSampler rather than
    // lazily: both are written once, below, into the persistent bindless set's
    // fixed sampler bindings and never queried again afterward (a VulkanShader-execution
    // pass's compile-time-resolved resample mode samples through binding 1 or binding 2
    // directly in its own GLSL — see jam_VulkanShaderUniforms.h's documented GLSL
    // contract — so neither sampler needs a runtime lookup accessor).
    const vk::SamplerCreateInfo nearestSamplerInfo { {}, vk::Filter::eNearest, vk::Filter::eNearest,
                                                     vk::SamplerMipmapMode::eNearest,
                                                     vk::SamplerAddressMode::eClampToEdge,
                                                     vk::SamplerAddressMode::eClampToEdge,
                                                     vk::SamplerAddressMode::eClampToEdge,
                                                     0.0f, vk::False, 0.0f, vk::False, vk::CompareOp::eNever,
                                                     0.0f, 0.0f, vk::BorderColor::eIntOpaqueBlack };

    return device.getDevice().createSampler (&nearestSamplerInfo, nullptr, &nearestSampler) == vk::Result::eSuccess;
}

bool VulkanGraphics::createBindlessTexturePool()
{
    // Persistent bindless texture array set. Unconditional: the queried
    // descriptor-indexing capabilities are hard requirements for this engine, asserted
    // at device creation, not a runtime branch. A dedicated pool is required (not the
    // per-frame-reset descriptorPool above): this set's binding 0/3/4 slots are written
    // incrementally at texture-upload time across many frames, which needs
    // eUpdateAfterBind and must never be reset. descriptorCount 2 for eSampler covers
    // both fixed sampler bindings (1 = linear, 2 = nearest), each allocated once
    // against this one set. eCombinedImageSampler covers bindings 3/4 (the combined
    // linear/nearest sampler2D arrays, VulkanPipelines::createDescriptorSetLayouts()'s doc
    // comment) — (bindlessSampledImageArrayCount - 1) arrays, each sized to
    // bindlessTextureCapacity, since binding 0's own eSampledImage entry above already
    // accounts for one of the bindlessSampledImageArrayCount total.
    const std::array<vk::DescriptorPoolSize, 3> bindlessPoolSizes {
        vk::DescriptorPoolSize { vk::DescriptorType::eSampledImage, bindlessTextureCapacity },
        vk::DescriptorPoolSize { vk::DescriptorType::eSampler, 2 },
        vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler,
                                 (bindlessSampledImageArrayCount - 1) * bindlessTextureCapacity }
    };

    const vk::DescriptorPoolCreateInfo bindlessPoolInfo { vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
                                                          1, bindlessPoolSizes };

    if (device.getDevice().createDescriptorPool (&bindlessPoolInfo, nullptr, &bindlessDescriptorPool)
        != vk::Result::eSuccess)
        return false;

    const vk::DescriptorSetLayout bindlessSet1 { pipelines.getLayoutSet1() };
    const vk::DescriptorSetAllocateInfo bindlessAllocInfo { bindlessDescriptorPool, bindlessSet1 };

    if (device.getDevice().allocateDescriptorSets (&bindlessAllocInfo, &bindlessTextureDescriptorSet)
        != vk::Result::eSuccess)
        return false;

    // Bindings 1/2 (the single shared linear/nearest samplers) never vary —
    // written once, here.
    const vk::DescriptorImageInfo bindlessLinearSamplerInfo { getOrCreateLinearSampler() };
    const vk::DescriptorImageInfo bindlessNearestSamplerInfo { nearestSampler };

    const std::array<vk::WriteDescriptorSet, 2> bindlessSamplerWrites {
        vk::WriteDescriptorSet { bindlessTextureDescriptorSet, 1, 0, 1, vk::DescriptorType::eSampler,
                                 &bindlessLinearSamplerInfo },
        vk::WriteDescriptorSet { bindlessTextureDescriptorSet, 2, 0, 1, vk::DescriptorType::eSampler,
                                 &bindlessNearestSamplerInfo }
    };

    device.getDevice().updateDescriptorSets (bindlessSamplerWrites, nullptr);

    return true;
}

bool VulkanGraphics::createDrawState()
{
    bool ready { createProjectionBuffer() };

    ready = ready and createDescriptorSets();

    ready = ready and createSamplers();

    ready = ready and createBindlessTexturePool();

    ready = ready and imageEffects.createDescriptorSets (descriptorPool, bindlessTextureDescriptorSet);

#if JAM_VULKAN_RUNTIME_SHADER_COMPILER
    ready = ready and shaderRegistry.createSlangVertexBuffer();
#endif

    return ready;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam