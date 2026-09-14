/** @file jam_VulkanShaderReflection.h
 *  @brief SPIR-V reflection for one compiled shader stage — every uniform
 *         buffer AND push_constant block member's name/offset/size, and
 *         every sampled-image resource's name/descriptor-set/binding.
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief SPIR-V-Cross-derived reflection of one compiled shader stage's
 *  resource layout — the real-world RetroArch .slang binding-collision fix: a
 *  .slang author's own `layout(set=N, binding=M) uniform UBO {...}`/
 *  `uniform sampler2D Source` declarations vary per shader, so their layout
 *  can only be discovered by reflecting the compiled SPIR-V, never assumed at
 *  GLSL-generation time the way this engine's own fixed set-0 bindless
 *  contract is (jam_VulkanShaderUniforms.h).
 *
 *  Pure introspection only — reflect() reads a compiled SPIR-V blob and
 *  reports what it finds; it neither compiles GLSL nor consumes reflected
 *  data itself (populating a per-pass descriptor set from this data is a
 *  separate, not-yet-built task).
 */
struct VulkanShaderReflection
{
    /** @brief RetroArch's own builtin-texture-name vocabulary —
     *  named once here as the SSOT every consumer resolving a reflected
     *  TextureResource::name against this vocabulary reads (today:
     *  VulkanGraphics::getSlangTextureBindings(), jam_VulkanGraphicsSlangPass.cpp;
     *  VulkanShaderInstance::build(), jam_VulkanShaderInstance.cpp), never a
     *  repeated magic string at a call site.
     *  @return The "PassOutput" builtin-texture-name prefix (e.g.
     *          "PassOutput0", "PassOutput1"). */
    static const juce::String& getPassOutputPrefix();

    /** @brief See getPassOutputPrefix().
     *  @return The "PassFeedback" builtin-texture-name prefix (e.g.
     *          "PassFeedback0", "PassFeedback1"). */
    static const juce::String& getPassFeedbackPrefix();

    /** @brief See getPassOutputPrefix().
     *  @return The "Source" builtin-texture-name, this pass's own input. */
    static const juce::String& getSourceName();

    /** @brief See getPassOutputPrefix().
     *  @return The "Original" builtin-texture-name, the unfiltered input
     *          frame — readable from any pass, before any pass ran on it. */
    static const juce::String& getOriginalName();

    /** @brief Suffix an author's own aliasN preset directive (jam::
     *  VulkanShaderPreset::Pass::alias, jam_VulkanShaderPreset.h) or its \#pragma
     *  name source-level fallback (jam::VulkanShaderCompiler::
     *  parsePassName()) is combined with to name that SAME pass's own
     *  PREVIOUS-frame output -- RetroArch's own "\<alias\>Feedback" vocabulary
     *  (slang_process.cpp:177-207), the alias-spelling sibling of the fixed
     *  getPassFeedbackPrefix() vocabulary above. Consumed by VulkanGraphics::
     *  getSlangTextureBindings()'s own alias-normalization step
     *  (jam_VulkanGraphicsSlangPass.cpp).
     *  @return The "Feedback" suffix appended to an alias to name that
     *          pass's own previous-frame output. */
    static const juce::String& getFeedbackSuffix();

    /** @brief RetroArch's own OriginalHistoryN builtin-texture-name
     *  vocabulary (history-frame semantic: OriginalHistory0
     *  == Original itself, the current frame; OriginalHistoryN samples the
     *  ORIGINAL input N frames back) — same SSOT rationale as
     *  getPassOutputPrefix() above. Consumed by VulkanShaderInstance::build()'s own
     *  depth-detection scan (jam_VulkanShaderInstance.cpp) and VulkanGraphics::
     *  getSlangTextureBindings()'s own OriginalHistory resolution
     *  (jam_VulkanGraphicsSlangPass.cpp).
     *  @return The "OriginalHistory" builtin-texture-name prefix (e.g.
     *          "OriginalHistory0", "OriginalHistory1"). */
    static const juce::String& getOriginalHistoryPrefix();

    /** @brief One uniform-buffer member's name, byte offset, and byte size —
     *  reflect()'s per-member introspection result (Compiler::get_member_name(),
     *  Compiler::type_struct_member_offset(), Compiler::get_declared_struct_member_size(),
     *  jam_VulkanShaderReflection.cpp).
     */
    struct UniformMember
    {
        /** @brief The member's declared name (e.g. "MVP", "FrameCount"). */
        juce::String name;

        /** @brief Byte offset of this member within the uniform buffer. */
        uint32_t offset { 0 };

        /** @brief Byte size of this member. */
        uint32_t size { 0 };
    };

    /** @brief One sampled-image (`uniform sampler2D`) resource's name and its
     *  author-assigned descriptor-set/binding decoration (reflect()'s
     *  Compiler::get_decoration() results, jam_VulkanShaderReflection.cpp).
     */
    struct TextureResource
    {
        /** @brief The resource's declared name (e.g. "Source", "PassOutput0"). */
        juce::String name;

        /** @brief This resource's `layout(set = N, ...)` decoration value. */
        uint32_t descriptorSet { 0 };

        /** @brief This resource's `layout(..., binding = M)` decoration value. */
        uint32_t binding { 0 };
    };

    /** @brief The reflected uniform buffer's total declared byte size, or 0
     *  when this stage declares no `uniform_buffers` resource at all — GLSL
     *  disallows an empty uniform block, so 0 unambiguously means absent,
     *  never a legitimately empty UBO. Only the FIRST uniform_buffers entry
     *  is reflected (RetroArch's own .slang convention declares at most one
     *  UBO per stage). */
    uint32_t uniformBufferSize { 0 };

    /** @brief Every member of the reflected uniform buffer, in declaration
     *  order. Empty when uniformBufferSize is 0. */
    jam::Array<UniformMember> uniformMembers;

    /** @brief The reflected uniform buffer's own `layout(set = N, ...)`
     *  decoration value — symmetric with TextureResource::descriptorSet
     *  above (the same Compiler::get_decoration() call, against the UBO
     *  resource's id instead of a sampled-image resource's). Meaningless
     *  when uniformBufferSize is 0 (no UBO reflected). RetroArch's own
     *  resource-usage rules ("All resources must be using
     *  descriptor set #0, or don't use layout(set = #N) at all") make 0 the
     *  only value a conformant .slang author ever declares here, but this
     *  is read from the compiled SPIR-V rather than assumed. */
    uint32_t uniformBufferSet { 0 };

    /** @brief The reflected uniform buffer's own `layout(..., binding = N)`
     *  decoration value — symmetric with TextureResource::binding above.
     *  Meaningless when uniformBufferSize is 0. Unlike descriptorSet, a
     *  .slang author's own UBO binding index genuinely varies per shader
     *  (RetroArch requires only that "All resources must use different
     *  bindings" and "layout(binding = #N) must be declared for all UBOs and
     *  sampler2Ds"), so this is never assumed/hardcoded even though 0 is the
     *  overwhelmingly common real-world value. */
    uint32_t uniformBufferBinding { 0 };

    /** @brief The reflected `push_constant` block's total declared byte
     *  size, or 0 when this stage declares no `push_constant_buffers`
     *  resource at all — same "0 means absent" contract as
     *  uniformBufferSize. No descriptorSet/binding sibling fields exist for
     *  this block (unlike uniformBufferSet/uniformBufferBinding above) —
     *  a push_constant block is never a descriptor-set resource; it carries
     *  no set/binding decoration in SPIR-V at all (addressed purely via the
     *  pipeline layout's own vk::PushConstantRange). Only the FIRST
     *  push_constant_buffers entry is
     *  reflected (SPIRV-Cross itself only ever populates one — a SPIR-V
     *  module may declare at most one push_constant block). A real .slang
     *  shader may declare its fixed-vocabulary members in a UBO, in a
     *  push_constant block, or split across both simultaneously (verified
     *  this session: stock.slang uses push_constant only; night_mode.slang
     *  splits FinalViewportSize/OutputSize into its UBO and
     *  SourceSize/OriginalSize into push_constant). */
    uint32_t pushConstantSize { 0 };

    /** @brief Every member of the reflected push_constant block, in
     *  declaration order. Empty when pushConstantSize is 0. */
    jam::Array<UniformMember> pushConstantMembers;

    /** @brief Every sampled-image resource declared by this stage, in
     *  reflection order (SPIRV-Cross's own ShaderResources::sampled_images
     *  order — the correct resource category for a GLSL `uniform sampler2D`
     *  variable, jam_VulkanShaderReflection.cpp). */
    jam::Array<TextureResource> textures;

    /** @brief Reflects @p spirv (one compiled shader stage's SPIR-V words,
     *  the same juce::MemoryBlock type VulkanShaderCompiler::compileSpirv()
     *  produces) via spirv_cross::Compiler::get_shader_resources().
     *  @param spirv  Compiled SPIR-V bytecode — a non-empty, word-aligned
     *                (size a multiple of 4 bytes) buffer.
     *  @return       This stage's reflected uniform-buffer, push_constant-
     *                block, and sampled-image resource layout.
     */
    static VulkanShaderReflection reflect (const juce::MemoryBlock& spirv);

    /** @brief Populates one pass's uniform-buffer bytes from this
     *  reflection's own uniformMembers — thin wrapper over
     *  populateMemberBuffer(), see its doc for the full member-dispatch
     *  contract (MVP/FrameCount/FrameDirection/parameter/"\<X\>Size"
     *  resolution, graceful degradation on unrecognized members).
     *
     *  Pure byte-buffer population only — does not wire into any descriptor
     *  set or uniform-buffer upload (a separate, not-yet-built task); the
     *  caller owns getting these bytes onto the GPU.
     *
     *  @param passExtent          This pass's own render target extent, in
     *                             pixels — "OutputSize"'s source.
     *  @param finalViewportExtent The FINAL/Image pass's own render target
     *                             extent, in pixels — "FinalViewportSize"'s
     *                             source (readable from any pass, per
     *                             RetroArch's own documented semantics).
     *  @param frameCount          This pass's own raw, unwrapped per-frame
     *                             counter — "FrameCount"'s source (modulo-
     *                             wrapping is a .slangp preset-level
     *                             directive, out of scope here).
     *  @param textureExtents      This stage's reflected texture resource
     *                             names (TextureResource::name) mapped to
     *                             their own already-resolved pixel
     *                             dimensions — resolving which GPU image
     *                             each name maps to is a separate, not-yet-
     *                             built task; this function only consumes
     *                             the already-resolved mapping.
     *  @param parameterDefaults   This pass's own \#pragma parameter
     *                             identifier -> author-default map
     *                             (VulkanShaderPass::parameterDefaults, threaded
     *                             through VulkanSlangPassResources::
     *                             parameterDefaults) — "FrameCount"'s
     *                             sibling exact-name members, resolved the
     *                             same way (the author's
     *                             default IS the rendered intent absent any
     *                             user override).
     *  @return                    A zero-initialized byte buffer sized to
     *                             uniformBufferSize, with every reflected
     *                             member written at its own offset/size.
     */
    juce::MemoryBlock populateUniformBuffer (vk::Extent2D passExtent, vk::Extent2D finalViewportExtent,
                                             uint32_t frameCount,
                                             const jam::HashMap<juce::String, vk::Extent2D>& textureExtents,
                                             const jam::HashMap<juce::String, float>& parameterDefaults) const;

    /** @brief Populates one pass's push_constant-block bytes from this
     *  reflection's own pushConstantMembers — thin wrapper over
     *  populateMemberBuffer(), see its doc for the full member-dispatch
     *  contract. A real .slang shader's fixed-vocabulary members (MVP/
     *  FrameCount/"\<X\>Size") may live in the push_constant block instead of,
     *  or split alongside, the UBO (verified this session: stock.slang
     *  declares SourceSize/OriginalSize/OutputSize/FrameCount entirely in
     *  push_constant) — this sibling exists so the caller can populate
     *  whichever block(s) this stage actually declared.
     *
     *  Pure byte-buffer population only — does not wire into any pipeline's
     *  push-constant range (a separate, not-yet-built task); the caller owns
     *  getting these bytes onto the GPU.
     *
     *  @param passExtent          See populateUniformBuffer().
     *  @param finalViewportExtent See populateUniformBuffer().
     *  @param frameCount          See populateUniformBuffer().
     *  @param textureExtents      See populateUniformBuffer().
     *  @param parameterDefaults   See populateUniformBuffer().
     *  @return                    A zero-initialized byte buffer sized to
     *                             pushConstantSize, with every reflected
     *                             member written at its own offset/size.
     */
    juce::MemoryBlock populatePushConstantBuffer (vk::Extent2D passExtent, vk::Extent2D finalViewportExtent,
                                                  uint32_t frameCount,
                                                  const jam::HashMap<juce::String, vk::Extent2D>& textureExtents,
                                                  const jam::HashMap<juce::String, float>& parameterDefaults) const;

private:
    /** @brief Writes @p extent as a vec4(width, height, 1/width, 1/height) —
     *  RetroArch's documented size-uniform format — into
     *  @p buffer at @p offset. Shared by populateMemberBuffer()'s
     *  "OutputSize"/"FinalViewportSize" fixed members and its "\<X\>Size"
     *  pattern-matched texture members alike (same vec4 shape either way).
     */
    static void writeSizeVec4 (juce::MemoryBlock& buffer, uint32_t offset, vk::Extent2D extent) noexcept;

    /** @brief The actual member-dispatch logic shared by
     *  populateUniformBuffer() and populatePushConstantBuffer() — takes the
     *  target block's own declared byte size and member list as explicit
     *  parameters (rather than reading this->uniformBufferSize/uniformMembers
     *  implicitly) so the one dispatch loop runs against EITHER a reflected
     *  UBO or a reflected push_constant block without duplication.
     *
     *  Exactly TWO branches (MANIFESTO's 3-branch max), never one branch per
     *  fixed member name:
     *  1. Exact-name hit against a per-call @c exactNameValues map — built
     *     once per call from the fixed engine vocabulary (MVP identity
     *     matrix, FrameCount, FrameDirection — this engine has no rewind
     *     concept, always the constant 1) PLUS one entry per @p
     *     parameterDefaults entry (a real \#pragma parameter's author
     *     default — the default IS the rendered intent
     *     absent any user override, RetroArch's own UI is an override
     *     mechanism, never the source of truth). A hit asserts the
     *     reflected member's own byte size matches the value's byte size,
     *     then memcpy's it in.
     *  2. Else, the "\<X\>Size"/"\<X\>Size\<N\>" NAMING PATTERN branch (RetroArch's
     *     verified member vocabulary) — resolved dynamically against whatever texture
     *     resource names this stage's own reflection actually found
     *     (@p textureExtents), never a hardcoded SourceSize/OriginalSize/
     *     PassOutput0Size list.
     *
     *  Any member matching neither branch (a genuine custom/unsupported
     *  field this engine doesn't recognize) stays zero-initialized — no
     *  error, no assert failure, graceful degradation.
     *
     *  @param bufferSize          The target block's own declared byte size
     *                             (uniformBufferSize or pushConstantSize).
     *  @param members             The target block's own reflected members,
     *                             in declaration order (uniformMembers or
     *                             pushConstantMembers).
     *  @param passExtent          This pass's own render target extent, in
     *                             pixels — "OutputSize"'s source.
     *  @param finalViewportExtent The FINAL/Image pass's own render target
     *                             extent, in pixels — "FinalViewportSize"'s
     *                             source.
     *  @param frameCount          This pass's own raw, unwrapped per-frame
     *                             counter — "FrameCount"'s source.
     *  @param textureExtents      This stage's reflected texture resource
     *                             names mapped to their own already-resolved
     *                             pixel dimensions.
     *  @param parameterDefaults   This pass's own \#pragma parameter
     *                             identifier -> author-default map — folded
     *                             into @c exactNameValues alongside the
     *                             fixed engine vocabulary above.
     *  @return                    A zero-initialized byte buffer sized to
     *                             @p bufferSize, with every member in
     *                             @p members written at its own offset/size.
     */
    static juce::MemoryBlock populateMemberBuffer (uint32_t bufferSize, const jam::Array<UniformMember>& members,
                                                    vk::Extent2D passExtent, vk::Extent2D finalViewportExtent,
                                                    uint32_t frameCount,
                                                    const jam::HashMap<juce::String, vk::Extent2D>& textureExtents,
                                                    const jam::HashMap<juce::String, float>& parameterDefaults);
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam