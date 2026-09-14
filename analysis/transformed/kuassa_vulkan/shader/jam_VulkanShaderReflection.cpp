// exception (external, vendored under jam_vulkan/spirv-cross/ — deliberately kept out of
// jam_VulkanShaderReflection.h so no spirv_cross symbol reaches any consumer of jam_vulkan,
// same treatment shaderc.hpp gets in jam_VulkanShaderCompiler.cpp). core-library headers only
// (spirv_cross.hpp) — no GLSL/HLSL/MSL backend headers, this is reflection-only, never recompiles.
#include <spirv_cross/spirv_cross.hpp>

namespace jam
{
/*____________________________________________________________________________*/
// RetroArch's own builtin-texture-name vocabulary — see VulkanShaderReflection.h's
// own doc comment for the SSOT
// rationale (multiple consumers resolve TextureResource::name against these).
const juce::String& VulkanShaderReflection::getPassOutputPrefix()
{
    static const juce::String value { Id::passOutput.toString() };
    return value;
}

const juce::String& VulkanShaderReflection::getPassFeedbackPrefix()
{
    static const juce::String value { Id::passFeedback.toString() };
    return value;
}

const juce::String& VulkanShaderReflection::getSourceName()
{
    static const juce::String value { jam::Format::toPascalCase (Id::source) };
    return value;
}

const juce::String& VulkanShaderReflection::getOriginalName()
{
    static const juce::String value { Id::original.toString() };
    return value;
}

const juce::String& VulkanShaderReflection::getFeedbackSuffix()
{
    static const juce::String value { Id::feedback.toString() };
    return value;
}

const juce::String& VulkanShaderReflection::getOriginalHistoryPrefix()
{
    static const juce::String value { Id::originalHistory.toString() };
    return value;
}

/*____________________________________________________________________________*/
VulkanShaderReflection VulkanShaderReflection::reflect (const juce::MemoryBlock& spirv)
{
    jassert (spirv.getSize() > 0 and spirv.getSize() % sizeof (uint32_t) == 0);

    const spirv_cross::Compiler compiler {
        static_cast<const uint32_t*> (spirv.getData()), spirv.getSize() / sizeof (uint32_t)
    };

    const auto resources { compiler.get_shader_resources() };

    VulkanShaderReflection reflection;

    if (not resources.uniform_buffers.empty())
    {
        // spirv_cross::SmallVector is an external vendored type (spirv_cross/spirv_cross.hpp)
        // with no .at() -- operator[] access is guarded by the enclosing not-empty positive
        // check above, never programmer error, so no jassert is needed either.
        const auto& uniformBufferResource { resources.uniform_buffers[0] };
        const auto& uniformBufferType { compiler.get_type (uniformBufferResource.base_type_id) };

        reflection.uniformBufferSize = static_cast<uint32_t> (compiler.get_declared_struct_size (uniformBufferType));

        // Symmetric with the sampled_images loop's own descriptorSet/binding
        // population below — a .slang author's own UBO binding index varies
        // per shader, read from the compiled SPIR-V here
        // rather than hardcoded.
        reflection.uniformBufferSet = compiler.get_decoration (uniformBufferResource.id, spv::DecorationDescriptorSet);
        reflection.uniformBufferBinding = compiler.get_decoration (uniformBufferResource.id, spv::DecorationBinding);

        for (uint32_t memberIndex = 0; memberIndex < uniformBufferType.member_types.size(); ++memberIndex)
            reflection.uniformMembers.add ({
                juce::String (compiler.get_member_name (uniformBufferResource.base_type_id, memberIndex)),
                compiler.type_struct_member_offset (uniformBufferType, memberIndex),
                static_cast<uint32_t> (compiler.get_declared_struct_member_size (uniformBufferType, memberIndex))
            });
    }

    // A real .slang shader may declare its fixed-vocabulary members in a push_constant block
    // instead of, or split alongside, a UBO (stock.slang: push_constant only; night_mode.slang:
    // split across both) — reflected symmetrically to uniform_buffers above, same call chain.
    if (not resources.push_constant_buffers.empty())
    {
        // spirv_cross::SmallVector is an external vendored type (spirv_cross/spirv_cross.hpp)
        // with no .at() -- operator[] access is guarded by the enclosing not-empty positive
        // check above, never programmer error, so no jassert is needed either.
        const auto& pushConstantResource { resources.push_constant_buffers[0] };
        const auto& pushConstantType { compiler.get_type (pushConstantResource.base_type_id) };

        reflection.pushConstantSize = static_cast<uint32_t> (compiler.get_declared_struct_size (pushConstantType));

        for (uint32_t memberIndex = 0; memberIndex < pushConstantType.member_types.size(); ++memberIndex)
            reflection.pushConstantMembers.add ({
                juce::String (compiler.get_member_name (pushConstantResource.base_type_id, memberIndex)),
                compiler.type_struct_member_offset (pushConstantType, memberIndex),
                static_cast<uint32_t> (compiler.get_declared_struct_member_size (pushConstantType, memberIndex))
            });
    }

    for (const auto& sampledImageResource : resources.sampled_images)
        reflection.textures.add ({
            juce::String (sampledImageResource.name),
            compiler.get_decoration (sampledImageResource.id, spv::DecorationDescriptorSet),
            compiler.get_decoration (sampledImageResource.id, spv::DecorationBinding)
        });

    return reflection;
}

/*____________________________________________________________________________*/
void VulkanShaderReflection::writeSizeVec4 (juce::MemoryBlock& buffer, uint32_t offset, vk::Extent2D extent) noexcept
{
    const float sizeVec4[4] {
        static_cast<float> (extent.width),
        static_cast<float> (extent.height),
        1.0f / static_cast<float> (extent.width),
        1.0f / static_cast<float> (extent.height)
    };

    std::memcpy (static_cast<uint8_t*> (buffer.getData()) + offset, sizeVec4, sizeof (sizeVec4));
}

/*____________________________________________________________________________*/
juce::MemoryBlock VulkanShaderReflection::populateMemberBuffer (uint32_t bufferSize, const jam::Array<UniformMember>& members,
                                                          vk::Extent2D passExtent, vk::Extent2D finalViewportExtent,
                                                          uint32_t frameCount,
                                                          const jam::HashMap<juce::String, vk::Extent2D>& textureExtents,
                                                          const jam::HashMap<juce::String, float>& parameterDefaults)
{
    juce::MemoryBlock buffer (bufferSize, true);

    // RetroArch's fixed/reserved UBO member vocabulary — named once here, never
    // a repeated magic string at a call site. Every spelling below is read from
    // Id:: (lexicon/jam_vulkan.md) — never a locally duplicated string literal.
    static const juce::String mvpMemberName              { Id::mvp.toString() };
    static const juce::String outputSizeMemberName        { jam::Format::toPascalCase (Id::output.toString() + juce::String::charToString (Chars::space) + Id::size.toString()) };
    static const juce::String finalViewportSizeMemberName { Id::finalViewport.toString() + jam::Format::upperFirstChar (Id::size) };
    static const juce::String frameCountMemberName        { Id::frameCount.toString() };

    // FrameDirection is fixed vocabulary too -- this engine
    // never rewinds, so its value is the constant 1, never a real reverse/rewind
    // signal (RetroArch's own "1 = forward, -1 = rewind" semantics; this engine
    // only ever plays forward).
    static const juce::String frameDirectionMemberName { Id::frameDirection.toString() };

    // Every OTHER "<X>Size" or indexed "<X>Size<N>" member is a NAMING PATTERN, not a fixed
    // list — resolved dynamically against populateUniformBuffer()'s
    // textureExtents parameter. RetroArch emits per-pass texture-size uniforms in the indexed
    // "<Base>Size<N>" shape (real corpus: feedback.slang declares "PassFeedbackSize0", whose
    // sampler is "PassFeedback0") — the trailing decimal digits, if any, are carried through
    // from the size member's name to its texture-resource key.
    static const juce::String textureSizeMemberSuffix { jam::Format::upperFirstChar (Id::size) };

    // "OutputSize" and "FinalViewportSize" are themselves instances of the "<X>Size"/"<X>Size<N>"
    // naming pattern (X = "Output", X = "FinalViewport") — seeding them here lets the loop below
    // resolve every size member through the single generic branch, fixed names included.
    jam::HashMap<juce::String, vk::Extent2D> allSizeSources { textureExtents };
    allSizeSources.emplace (outputSizeMemberName.dropLastCharacters (textureSizeMemberSuffix.length()), passExtent);
    allSizeSources.emplace (finalViewportSizeMemberName.dropLastCharacters (textureSizeMemberSuffix.length()), finalViewportExtent);

    // Every exact-name fixed member (MVP, FrameCount, FrameDirection) PLUS
    // every author #pragma parameter default, collapsed into ONE pre-built
    // byte map — see populateMemberBuffer()'s own doc comment for why this
    // keeps the member loop below at exactly two branches.
    const glm::mat4 identityMvp { 1.0f };
    const int32_t forwardFrameDirection { 1 };// this engine never rewinds

    juce::MemoryBlock mvpValue;
    mvpValue.append (&identityMvp, sizeof (identityMvp));

    juce::MemoryBlock frameCountValue;
    frameCountValue.append (&frameCount, sizeof (frameCount));

    juce::MemoryBlock frameDirectionValue;
    frameDirectionValue.append (&forwardFrameDirection, sizeof (forwardFrameDirection));

    jam::HashMap<juce::String, juce::MemoryBlock> exactNameValues {
        { mvpMemberName, mvpValue },
        { frameCountMemberName, frameCountValue },
        { frameDirectionMemberName, frameDirectionValue },
    };

    for (const auto& [parameterName, parameterDefault] : parameterDefaults)
    {
        juce::MemoryBlock parameterValue;
        parameterValue.append (&parameterDefault, sizeof (parameterDefault));

        exactNameValues.emplace (parameterName, std::move (parameterValue));
    }

    for (const auto& member : members)
    {
        if (exactNameValues.contains (member.name))
        {
            const auto& exactMemberValue { exactNameValues.at (member.name) };

            jassert (member.size == static_cast<uint32_t> (exactMemberValue.getSize()));
            std::memcpy (static_cast<uint8_t*> (buffer.getData()) + member.offset, exactMemberValue.getData(), member.size);
        }
        // Chars::isNumeric (jam_Chars.h) isolates the optional trailing pass
        // ordinal (e.g. "PassFeedbackSize0" -> stem "PassFeedbackSize", digits
        // "0") — identifiers never end in '.'/'-', so the trim is digits-only
        // in practice, never a hand-rolled digit list.
        else if (const auto sizeMemberStem { [&member] {
                     int trimmedLength { member.name.length() };
                     while (trimmedLength > 0
                            and Chars::isNumeric (member.name[trimmedLength - 1]))
                         --trimmedLength;

                     return member.name.substring (0, trimmedLength);
                 }() };
                 sizeMemberStem.endsWith (textureSizeMemberSuffix))
        {
            const auto sizeMemberOrdinal { member.name.substring (sizeMemberStem.length()) };
            const auto textureName { sizeMemberStem.dropLastCharacters (textureSizeMemberSuffix.length()) + sizeMemberOrdinal };

            if (allSizeSources.contains (textureName))
            {
                writeSizeVec4 (buffer, member.offset, allSizeSources.at (textureName));
            }
        }
    }

    return buffer;
}

/*____________________________________________________________________________*/
juce::MemoryBlock VulkanShaderReflection::populateUniformBuffer (vk::Extent2D passExtent, vk::Extent2D finalViewportExtent,
                                                           uint32_t frameCount,
                                                           const jam::HashMap<juce::String, vk::Extent2D>& textureExtents,
                                                           const jam::HashMap<juce::String, float>& parameterDefaults) const
{
    return populateMemberBuffer (uniformBufferSize, uniformMembers, passExtent, finalViewportExtent, frameCount,
                                 textureExtents, parameterDefaults);
}

/*____________________________________________________________________________*/
juce::MemoryBlock VulkanShaderReflection::populatePushConstantBuffer (vk::Extent2D passExtent, vk::Extent2D finalViewportExtent,
                                                                uint32_t frameCount,
                                                                const jam::HashMap<juce::String, vk::Extent2D>& textureExtents,
                                                                const jam::HashMap<juce::String, float>& parameterDefaults) const
{
    return populateMemberBuffer (pushConstantSize, pushConstantMembers, passExtent, finalViewportExtent, frameCount,
                                 textureExtents, parameterDefaults);
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam