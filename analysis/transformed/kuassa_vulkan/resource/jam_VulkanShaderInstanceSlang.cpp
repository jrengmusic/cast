// Slang per-pass reflected resources file: buildSlangPassResources()/
// buildSlangPassDescriptorSetLayout()/buildSlangPassPipelineLayout() (build()'s
// slang-only step — reflects every pass, sizes/creates the shared slang
// descriptor pool, builds each pass's own descriptor set layout/set/uniform
// buffer/dedicated pipeline layout) and refreshSlangPass() (the per-frame
// UBO memcpy + texture-binding rewrite). Split out of
// jam_VulkanShaderInstance.cpp by concern — that file's own build()
// orchestrates a call into buildSlangPassResources() below; see its own doc
// comment for the full call sequence.

namespace jam
{
/*____________________________________________________________________________*/
//==============================================================================
// Slang per-pass reflected resources
//==============================================================================

bool VulkanShaderInstance::buildSlangPassResources (const jam::Owner<VulkanShaderPass>& passes, const VulkanShaderPreset& preset)
{
    jam::Array<VulkanShaderReflection> reflections;
    reflections.ensureStorageAllocated (static_cast<int> (passes.size()));

    for (auto& pass : passes)
        reflections.add (VulkanShaderReflection::reflect (pass->spirv));

    bool built { createSlangDescriptorPool (reflections) };

    for (size_t passIndex = 0; built and passIndex < passes.size(); ++passIndex)
        built = buildSlangPassEntry (*passes.at (passIndex),
                                     std::move (reflections.at (static_cast<int> (passIndex))), passIndex, preset);

    return built;
}

bool VulkanShaderInstance::createSlangDescriptorPool (const jam::Array<VulkanShaderReflection>& reflections)
{
    // Exact pool sizing from every pass's own already-known reflection —
    // strictly more precise than a padded worst-case ceiling, and available
    // here for free since every pass is reflected above before this pool is
    // ever created.
    uint32_t uniformBufferDescriptorCount { 0 };
    uint32_t textureDescriptorCount { 0 };
    uint32_t setCount { 0 };

    for (auto& reflection : reflections)
    {
        const bool passNeedsSet { reflection.uniformBufferSize > 0 or not reflection.textures.isEmpty() };

        if (passNeedsSet)
        {
            ++setCount;
            uniformBufferDescriptorCount += reflection.uniformBufferSize > 0 ? 1 : 0;
            textureDescriptorCount += static_cast<uint32_t> (reflection.textures.size());
        }
    }

    bool built { true };

    // A fully parameter-free pass chain (no pass declares a UBO or any
    // texture) needs no pool at all — slangDescriptorPool stays empty,
    // matching every degenerate-pass case below staying empty too.
    if (setCount > 0)
    {
        jam::Array<vk::DescriptorPoolSize> poolSizes;

        if (uniformBufferDescriptorCount > 0)
            poolSizes.add ({ vk::DescriptorType::eUniformBuffer, uniformBufferDescriptorCount });

        if (textureDescriptorCount > 0)
            poolSizes.add ({ vk::DescriptorType::eCombinedImageSampler, textureDescriptorCount });

        const vk::DescriptorPoolCreateInfo poolInfo { {}, setCount, poolSizes };

        built = device.getDevice().createDescriptorPool (&poolInfo, nullptr, &slangDescriptorPool)
                == vk::Result::eSuccess;
    }

    return built;
}

bool VulkanShaderInstance::buildSlangPassEntry (const VulkanShaderPass& pass, VulkanShaderReflection reflection,
                                          size_t passIndex, const VulkanShaderPreset& preset)
{
    auto passResources { std::make_unique<VulkanSlangPassResources>() };
    passResources->reflection = std::move (reflection);
    passResources->parameterDefaults = pass.parameterDefaults;

    // Default-constructed VulkanShaderPreset::Pass (every field at its own
    // documented "absent" value) when preset.passes has no entry for
    // this pass ordinal — see VulkanSlangPassResources::settings' own doc
    // comment for the full per-directive consumption map.
    passResources->settings = passIndex < static_cast<size_t> (preset.passes.size())
        ? preset.passes.at (static_cast<int> (passIndex))
        : VulkanShaderPreset::Pass {};

    bool built { buildSlangPassDescriptorSetLayout (passResources->reflection, passResources->descriptorSetLayout) };

    if (built and passResources->descriptorSetLayout != nullptr)
    {
        const vk::DescriptorSetAllocateInfo allocInfo { slangDescriptorPool, passResources->descriptorSetLayout };
        built = device.getDevice().allocateDescriptorSets (&allocInfo, &passResources->descriptorSet)
                == vk::Result::eSuccess;
    }

    if (built and passResources->reflection.uniformBufferSize > 0)
        built = buildSlangPassUniformBuffer (*passResources);

    if (built)
        built = buildSlangPassPipelineLayout (*passResources);

    slangPasses.add (std::move (passResources));

    return built;
}

bool VulkanShaderInstance::buildSlangPassUniformBuffer (VulkanSlangPassResources& passResources)
{
    // Mirrors VulkanGraphics::projectionBuffer's exact persistent-mapped
    // shape and coherency rationale (jam_VulkanGraphicsSetupDrawState.cpp) —
    // the descriptor binding below is written ONCE here; only its mapped
    // bytes are refreshed per frame afterward (refreshSlangPass()'s memcpy).
    const vk::BufferCreateInfo bufferInfo { {}, passResources.reflection.uniformBufferSize,
                                            vk::BufferUsageFlagBits::eUniformBuffer, vk::SharingMode::eExclusive };

    VmaAllocationCreateInfo allocInfo {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    passResources.uniformBuffer = VulkanBuffer (device.getAllocator(), bufferInfo, allocInfo);
    bool built { passResources.uniformBuffer.isValid() };

    if (built)
    {
        const vk::DescriptorBufferInfo bufferDescInfo { passResources.uniformBuffer.getBuffer(), 0,
                                                        passResources.reflection.uniformBufferSize };

        const vk::WriteDescriptorSet write { passResources.descriptorSet,
                                             passResources.reflection.uniformBufferBinding, 0, 1,
                                             vk::DescriptorType::eUniformBuffer, nullptr, &bufferDescInfo };

        device.getDevice().updateDescriptorSets (write, nullptr);
    }

    return built;
}

bool VulkanShaderInstance::buildSlangPassDescriptorSetLayout (const VulkanShaderReflection& reflection,
                                                        vk::DescriptorSetLayout& outLayout) const
{
    jam::Array<vk::DescriptorSetLayoutBinding> bindings;

    if (reflection.uniformBufferSize > 0)
        bindings.add ({ reflection.uniformBufferBinding, vk::DescriptorType::eUniformBuffer, 1,
                              vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr });

    // sampler2D cannot appear in a .slang vertex stage at all, so every
    // texture binding is fragment-only.
    for (auto& texture : reflection.textures)
        bindings.add ({ texture.binding, vk::DescriptorType::eCombinedImageSampler, 1,
                              vk::ShaderStageFlagBits::eFragment, nullptr });

    bool built { true };

    if (not bindings.isEmpty())
    {
        const vk::DescriptorSetLayoutCreateInfo layoutInfo { {}, bindings };
        built = device.getDevice().createDescriptorSetLayout (&layoutInfo, nullptr, &outLayout) == vk::Result::eSuccess;
    }

    return built;
}

bool VulkanShaderInstance::buildSlangPassPipelineLayout (VulkanSlangPassResources& passResources) const
{
    jam::Array<vk::DescriptorSetLayout> setLayouts;

    if (passResources.descriptorSetLayout != nullptr)
        setLayouts.add (passResources.descriptorSetLayout);

    jam::Array<vk::PushConstantRange> pushRanges;

    if (passResources.reflection.pushConstantSize > 0)
        pushRanges.add ({ vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                0, passResources.reflection.pushConstantSize });

    const vk::PipelineLayoutCreateInfo layoutInfo { {}, setLayouts, pushRanges };

    return device.getDevice().createPipelineLayout (&layoutInfo, nullptr, &passResources.pipelineLayout)
           == vk::Result::eSuccess;
}

juce::MemoryBlock VulkanShaderInstance::refreshSlangPass (int passIndex, vk::Extent2D passExtent, vk::Extent2D finalViewportExtent,
                                                    uint32_t frameCount, const VulkanShaderTextureBindings& textureBindings,
                                                    vk::Sampler sampler) const
{
    auto& passResources { *slangPasses.at (static_cast<size_t> (passIndex)) };
    const auto& reflection { passResources.reflection };

    // RetroArch's own frame_count_modN preset directive (jam_VulkanShaderPreset.h) —
    // consumed here, the one FrameCount source every populate*Buffer() call
    // below shares; frameCountMod == 0 (absent, VulkanSlangPassResources::settings'
    // own documented default) means no modulo, this pass's own raw,
    // unwrapped frameCount passes through unchanged.
    const uint32_t effectiveFrameCount { passResources.settings.frameCountMod > 0
        ? frameCount % passResources.settings.frameCountMod
        : frameCount };

    if (reflection.uniformBufferSize > 0)
        refreshSlangPassUniformBuffer (passResources, passExtent, finalViewportExtent, effectiveFrameCount, textureBindings);

    if (not reflection.textures.isEmpty())
        refreshSlangPassTextureBindings (passResources, textureBindings, sampler);

    return reflection.pushConstantSize > 0
        ? reflection.populatePushConstantBuffer (passExtent, finalViewportExtent, effectiveFrameCount, textureBindings.extents,
                                                 passResources.parameterDefaults)
        : juce::MemoryBlock {};
}

void VulkanShaderInstance::refreshSlangPassUniformBuffer (VulkanSlangPassResources& passResources, vk::Extent2D passExtent,
                                                    vk::Extent2D finalViewportExtent, uint32_t effectiveFrameCount,
                                                    const VulkanShaderTextureBindings& textureBindings) const
{
    const juce::MemoryBlock uniformBytes { passResources.reflection.populateUniformBuffer (
        passExtent, finalViewportExtent, effectiveFrameCount, textureBindings.extents, passResources.parameterDefaults) };

    std::memcpy (passResources.uniformBuffer.getMapped(), uniformBytes.getData(), uniformBytes.getSize());
}

void VulkanShaderInstance::refreshSlangPassTextureBindings (VulkanSlangPassResources& passResources,
                                                      const VulkanShaderTextureBindings& textureBindings, vk::Sampler sampler) const
{
    const auto& reflection { passResources.reflection };

    // Pre-allocated to the maximum possible fill count (one entry per
    // reflected texture) before the loop below ever runs — imageInfos'
    // elements are addressed by writes (&imageInfos.last() per iteration),
    // so no reallocation may happen mid-loop; ensureStorageAllocated()
    // grows to an exact target, mirroring std::vector::reserve()'s own
    // no-doubling discipline.
    jam::Array<vk::DescriptorImageInfo> imageInfos;
    imageInfos.ensureStorageAllocated (reflection.textures.size());

    jam::Array<vk::WriteDescriptorSet> writes;
    writes.ensureStorageAllocated (reflection.textures.size());

    // Unconditional per-frame rewrite for EVERY texture binding, ping-
    // ponging or not — see refreshSlangPass()'s own header doc comment
    // for why a write-once-at-build-time descriptor is never safe here.
    // A name absent from textureBindings.views (unresolved this frame —
    // VulkanShaderTextureBindings's own graceful-degradation contract) is
    // simply skipped, leaving that binding's previous frame's write in
    // place rather than asserting/erroring.
    for (auto& texture : reflection.textures)
    {
        if (textureBindings.views.contains (texture.name))
        {
            imageInfos.add ({ sampler, textureBindings.views.at (texture.name), vk::ImageLayout::eShaderReadOnlyOptimal });
            writes.add ({ passResources.descriptorSet, texture.binding, 0, 1,
                                vk::DescriptorType::eCombinedImageSampler, &imageInfos.last() });
        }
    }

    if (not writes.isEmpty())
    {
        device.getDevice().updateDescriptorSets (writes, nullptr);
    }
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam