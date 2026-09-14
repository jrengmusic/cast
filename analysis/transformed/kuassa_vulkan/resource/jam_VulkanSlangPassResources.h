/** @file jam_VulkanSlangPassResources.h
 *  @brief One .slang-format VulkanShaderPass's own reflected descriptor set,
 *         per-pass uniform buffer, and dedicated pipeline layout — the
 *         real-world .slang binding-collision fix: a .slang author's own
 *         layout(set=0, binding=N)
 *         declarations vary per shader, so every slang VulkanShaderPass gets its
 *         OWN pipeline layout/descriptor set shaped by that pass's own
 *         SPIR-V reflection, entirely separate from this engine's fixed
 *         set-0 bindless contract every Shadertoy-format pass shares
 *         (jam_VulkanShaderUniforms.h).
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief One slang VulkanShaderPass's own GPU-side reflected resources — built
 *  once, eagerly, by VulkanShaderInstance::buildSlangPassResources() (mirrors
 *  VulkanRenderResources' own "built once by build(), never rebuilt in place"
 *  contract) and refreshed per-frame by VulkanShaderInstance::refreshSlangPass()
 *  (uniformBuffer's mapped bytes memcpy'd fresh, descriptorSet's texture
 *  bindings vk::updateDescriptorSets fresh — see refreshSlangPass()'s own
 *  doc comment for the full build-once/per-frame split and why).
 *
 *  Every field stays default/empty for a Shadertoy-format VulkanShaderInstance —
 *  VulkanShaderInstance::slangPasses is only ever populated when shader.format ==
 *  VulkanShaderFormat::slang (VulkanShaderInstance::build()'s own top-level
 *  branch), so a Shadertoy execution allocates none of this.
 */
struct VulkanSlangPassResources
{
    /** @brief This pass's own fragment-stage SPIR-V reflection
     *  (VulkanShaderReflection::reflect (pass.spirv)) — the UBO/push_constant
     *  member layout and texture resource names/bindings every other field
     *  below is built from. Reflecting the fragment stage only is
     *  sufficient even for a UBO/push_constant member the pass's own vertex
     *  stage also reads (e.g. MVP): splitSlangStages() prepends the exact
     *  same shared-prelude text (where a .slang author's UBO/push_constant
     *  struct is always declared, per RetroArch's own "the same UBO visible
     *  to both vertex and fragment as common code" convention) to BOTH
     *  stages before compilation, so the type's
     *  full member layout is identical and present in either stage's own
     *  SPIR-V regardless of which stage's main() actually reads a given
     *  member. sampler2D resources cannot appear in a .slang vertex stage at
     *  all (RetroArch's own rule: "sampler2D cannot be used in vertex"), so
     *  reflecting only the fragment stage never misses a texture resource. */
    VulkanShaderReflection reflection;

    /** @brief This pass's own author-default parameter map, copied once from
     *  VulkanShaderPass::parameterDefaults at VulkanShaderInstance::
     *  buildSlangPassResources() — mirrors reflection's own "built once,
     *  read every frame" shape: VulkanShaderInstance::refreshSlangPass() threads
     *  this map into VulkanShaderReflection::populateUniformBuffer()/
     *  populatePushConstantBuffer() on every call, since a parameter's
     *  default value is this pass's own exact-name value vocabulary right
     *  alongside the fixed MVP/FrameCount/FrameDirection members (see
     *  VulkanShaderReflection::populateMemberBuffer()'s own doc comment). Empty
     *  for every pass declaring no \#pragma parameter — same "empty means
     *  absent" contract as VulkanShaderPass::parameterDefaults itself. */
    jam::HashMap<juce::String, float> parameterDefaults {};

    /** @brief This pass's own preset directives, copied once from
     *  jam::VulkanShaderPreset::passes at VulkanShaderInstance::
     *  buildSlangPassResources() — default-constructed (every field at its
     *  own documented "absent" value, jam_VulkanShaderPreset.h's own Pass
     *  doc comment) when this pass's ordinal has no entry in the owning
     *  VulkanShader's own preset.passes (every VulkanShaderFormat::shadertoy execution;
     *  a VulkanShaderFormat::slang execution whose preset declares fewer passes
     *  than its own pass chain). Per-pass consumption home for every
     *  RetroArch .slangp preset directive this pass's own draw needs —
     *  frameCountMod is consumed NOW, at VulkanShaderInstance::refreshSlangPass()
     *  (frameCount % settings.frameCountMod when frameCountMod > 0, cited to
     *  RetroArch's own frame_count_modN); filterLinear/wrapMode are consumed
     *  via VulkanShaderInstance::getSlangPassSettings(), read by
     *  VulkanGraphics::getOrCreateSlangPassSampler() (jam_VulkanGraphics.cpp) to
     *  select this pass's own concrete sampler for every texture binding
     *  refreshSlangPass() writes; srgbFramebuffer/floatFramebuffer still sit
     *  here UNCONSUMED (via THIS field — a separate, direct
     *  shader.preset.passes.at (passIndex) read in VulkanShaderInstance::build()'s
     *  own per-buffer-pass loop resolves them instead, before this pass's own
     *  VulkanSlangPassResources instance is even constructed). mipmapInput is
     *  consumed the same indirect way — see VulkanShaderInstance::build()'s own
     *  doc comment for the exact consumer-ordinal derivation
     *  (jam::VulkanShaderPreset::Pass::mipmapInput's own doc comment for
     *  the RetroArch directive itself). scaleTypeX/Y and scaleX/Y are
     *  likewise consumed separately, at build() time, by VulkanShaderInstance's own
     *  per-pass extent computation (never read from here — that computation
     *  runs before this struct even exists for the pass in question). */
    VulkanShaderPreset::Pass settings {};

    /** @brief This pass's own descriptor set layout, built from reflection's
     *  own bindings (one uniform-buffer binding at
     *  reflection.uniformBufferBinding when reflection.uniformBufferSize > 0,
     *  one combined-image-sampler binding per reflection.textures entry at
     *  its own TextureResource::binding) — author-shaped, since a real
     *  .slang shader's binding indices genuinely vary per shader (unlike
     *  this engine's own fixed set-0 bindless contract). Empty (null handle)
     *  when this pass declares neither a UBO nor any texture — a fully
     *  parameter-free pass needs no descriptor set at all. */
    vk::DescriptorSetLayout descriptorSetLayout {};

    /** @brief This pass's own single descriptor set, allocated from
     *  VulkanShaderInstance::slangDescriptorPool against descriptorSetLayout above
     *  — empty (null handle) exactly when descriptorSetLayout is empty. */
    vk::DescriptorSet descriptorSet {};

    /** @brief This pass's own per-pass uniform buffer, sized to
     *  reflection.uniformBufferSize and written into descriptorSet's UBO
     *  binding ONCE at build time (mirrors VulkanGraphics::projectionBuffer's
     *  exact persistent-mapped shape, jam_VulkanGraphicsSetupDrawState.cpp) —
     *  only its MAPPED BYTES are refreshed every frame
     *  (refreshSlangPass()'s memcpy), never the descriptor write itself
     *  (the buffer's vk::Buffer handle never changes once created, so one
     *  vk::WriteDescriptorSet at build time remains valid for this
     *  VulkanShaderInstance's entire lifetime). Left default/invalid when
     *  reflection.uniformBufferSize is 0 (this pass declares no UBO — every
     *  fixed-vocabulary member it needs lives in its push_constant block
     *  instead, or it needs neither). */
    VulkanBuffer uniformBuffer {};

    /** @brief This pass's own dedicated pipeline layout — set 0 =
     *  descriptorSetLayout (or no sets at all when descriptorSetLayout is
     *  empty) plus a single vk::PushConstantRange sized to
     *  reflection.pushConstantSize when non-zero (or no push-constant range
     *  at all otherwise) — REPLACES the shared VulkanShaderUniforms-shaped
     *  getOrCreateShaderInstanceLayout() for this pass's own pipeline only;
     *  every Shadertoy-format pass continues building against the shared
     *  layout, completely unaffected. Built once by
     *  VulkanShaderInstance::buildSlangPassResources(), consumed by
     *  VulkanShaderInstance::buildRenderResources() as this pass's pipeline's own
     *  layout (replacing storedPipelineLayout for this one pass). */
    vk::PipelineLayout pipelineLayout {};
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam