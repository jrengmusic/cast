/** @file jam_VulkanShaderPass.h
 *  @brief One compiled shader pass — a name and its SPIR-V fragment-stage
 *         bytecode, the unit jam::VulkanShader's pass chain is built from.
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief One compiled shader pass — a name (Owner<VulkanShaderPass>'s O(1)
 *  find-by-name key), its SPIR-V fragment-stage bytecode, and an optional
 *  per-pass vertex-stage bytecode.
 *
 *  VulkanVertex stage is shared across every pass BY DEFAULT (the engine's
 *  fullscreen triangle/quad technique, VulkanShaderInstance::storedVertModule) —
 *  vertexSpirv empty (every Shadertoy-compiled pass) means this pass
 *  renders through that shared module; only the fragment-stage bytecode
 *  varies per pass in that case. A non-empty vertexSpirv (every
 *  VulkanShaderFormat::Type::slang-compiled pass — its own \#pragma stage vertex,
 *  compiled via VulkanShaderCompiler::splitSlangStages(), a real vertex stage
 *  declaring its own Position/TexCoord attributes, drawn against
 *  VulkanShaderInstance::slangQuadVertices/VulkanGraphics::slangQuadVertexBuffer rather
 *  than the shared gl_VertexIndex technique) means this pass instead binds
 *  a dedicated module built from its own bytecode. Every buffer pass (every
 *  entry but the mandatory final Image pass) is unconditionally
 *  ping-pong-feedback-capable — every buffer pass's own previous-frame
 *  output is always sampled back (engine-side rule, VulkanShaderInstance), no
 *  per-pass opt-out.
 */
struct VulkanShaderPass
{
    /** @brief Pass name — the buffer pass's source filename stem (e.g.
     *  "BufferA", or any name a shader project's author chose), or the
     *  mandatory Image pass's own name. Owner<VulkanShaderPass>'s find-by-name key. */
    juce::String name;

    /** @brief Compiled SPIR-V bytecode for this pass's fragment stage. */
    juce::MemoryBlock spirv;

    /** @brief Optional per-pass vertex-stage SPIR-V bytecode. Empty (default)
     *  means this pass renders through the engine-shared fullscreen-triangle
     *  vertex shader (shader_pass.vert.spv, VulkanShaderInstance::storedVertModule) —
     *  today's behavior for every Shadertoy-compiled pass. Non-empty means
     *  this pass supplies its own real vertex stage (e.g. a future .slang
     *  pass's \#pragma stage vertex, compiled separately) — VulkanShaderInstance
     *  builds/uses a dedicated vk::ShaderModule for it instead of the shared
     *  one. */
    juce::MemoryBlock vertexSpirv;

    /** @brief Per-pass author-default map parsed from this pass's own
     *  \#pragma parameter declarations (VulkanShaderCompiler::parseParameterDefaults())
     *  — RetroArch's documented identifier -> default-value author intent
     *  (the author's DEFAULT is what renders absent any user
     *  override — RetroArch's own UI is an override mechanism layered on top,
     *  never the source of truth). Consumed downstream as this pass's own
     *  exact-name value vocabulary — VulkanSlangPassResources::parameterDefaults
     *  (copied once at VulkanShaderInstance::buildSlangPassResources()) feeds
     *  VulkanShaderReflection::populateUniformBuffer()/populatePushConstantBuffer()'s
     *  per-frame member population, alongside the fixed MVP/FrameCount/
     *  FrameDirection members. Empty for every Shadertoy-compiled pass (that
     *  format has no \#pragma parameter convention) and for any slang pass
     *  declaring no parameters. */
    jam::HashMap<juce::String, float> parameterDefaults;

    /** @brief Hashes a VulkanShaderPass by its name only — Owner<VulkanShaderPass>'s O(1)
     *  find-by-name lookup key, reusing jam::Wyhash's byte-hash utility
     *  (jam_HashMap.h), not hand-rolled. */
    struct Hash
    {
        size_t operator() (const VulkanShaderPass& pass) const noexcept
        {
            return static_cast<size_t> (jam::Wyhash::hashBytes (
                pass.name.toRawUTF8(), pass.name.getNumBytesAsUTF8()));
        }
    };

    /** @brief Compares two passes by name only. */
    bool operator== (const VulkanShaderPass& other) const noexcept { return name == other.name; }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam