/** @file jam_VulkanShaderPreset.h
 *  @brief Parsed .slangp manifest directives — per-pass scale/filter/
 *         wrap/framebuffer-format/frame-count-modulo/alias settings (RetroArch
 *         vocabulary), every PRESET-global externally declared texture/LUT
 *         (the top-level "textures=" name list, RetroArch's own per-name
 *         \<name\>/\<name\>_linear/\<name\>_wrap_mode/\<name\>_mipmap directive
 *         quartet), the mesh= key (an END-extension addition, not RetroArch
 *         vocabulary), and every top-level \#pragma-parameter override value,
 *         read from a project's own .slangp manifest text (jam::
 *         VulkanShaderFormat::preset's own canon slot) — THE resource manifest for
 *         both shader source formats (jam::VulkanShaderFormat::slang's own
 *         full preset; jam::VulkanShaderFormat::shadertoy's own optional
 *         resource-manifest-only .slangp, textures=/mesh=, no shaders=/passes).
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief One parsed .slangp manifest — every pass's own scale/filter/wrap/
 *  framebuffer-format/frame-count-modulo/alias directive (RetroArch
 *  gfx/video_shader_parse.c's video_shader_parse_pass(), cited throughout
 *  this file and jam_VulkanShaderPreset.cpp), every externally declared texture/LUT and the mesh=
 *  connection, plus every top-level key=value pair this parse does not
 *  itself consume, carried forward as parameterOverrides (jam::
 *  VulkanShaderCompiler::compile() overlays each entry whose key matches a pass's
 *  own \#pragma parameter identifier onto that pass's VulkanShaderPass::
 *  parameterDefaults — the author's preset value outranks the shader file's
 *  own \#pragma default, RetroArch's own documented override semantics).
 *
 *  Content-based format detection (the host application's own shader loader)
 *  resolves a project's own format from THIS parse's own
 *  result, before either format's own directory reader ever runs: one or
 *  more passes (a shaders= directive present) is jam::
 *  VulkanShaderFormat::slang; an absent .slangp, or a zero-pass one (a shadertoy
 *  project's own resource-manifest-only .slangp, textures=/mesh=, no
 *  shaders=), is jam::VulkanShaderFormat::shadertoy. Empty presetText (no
 *  .slangp exists in the project directory at all) parses to zero passes,
 *  empty overrides — jam::VulkanShaderInstance's own per-pass extent
 *  computation degrades to today's exact scaledExtent-for-every-pass
 *  behavior whenever a VulkanShader's own preset.passes is empty or shorter than
 *  its own pass chain (see jam::VulkanShaderInstance::build()'s own doc
 *  comment).
 */
struct VulkanShaderPreset
{
    /** @brief One buffer pass's own preset directives — ordinal-aligned with
     *  jam::VulkanShader::passes' own buffer-pass ordinal (this preset's
     *  passes[N] describes shader.passes[N]; the mandatory Image pass is
     *  excluded the same way jam::VulkanShaderInstance::
     *  getBufferPassCount() excludes it). Every field defaults to
     *  RetroArch's own documented "key absent" behavior (video_shader_parse.c's
     *  video_shader_parse_pass(), verified this session) — see each field's
     *  own doc comment. A VulkanShader whose preset has no entry for a given pass
     *  ordinal (preset.passes shorter than the pass chain — every
     *  VulkanShaderFormat::shadertoy shader, whose preset is always empty) is
     *  handled by the CONSUMER falling back to today's exact scaledExtent
     *  behavior, never by this struct fabricating a synthetic entry. */
    struct Pass
    {
        /** @brief Horizontal scale-type — jam::VulkanShaderFormat's own
         *  scale-type ordinal (source/viewport/absolute, jam_vulkan/bimap/jam_VulkanShaderFormat.h)
         *  — overridden by scale_typeN when present (both axes at once),
         *  else independently by scale_type_xN. Defaults to
         *  VulkanShaderFormat::source: RetroArch's own no-scale_type-key
         *  pass-through behavior (FBO_SCALE_FLAG_VALID left unset) is
         *  encoded here as source-typed at 1x scale — an identical resulting
         *  extent, never a distinct "unset" state. Resolved via
         *  jam::VulkanShaderFormat::getScaleType(), the SAME registry
         *  every scale_typeN spelling is looked up through — never a
         *  locally duplicated enum/lookup table. */
        int scaleTypeX { VulkanShaderFormat::source };

        /** @brief See scaleTypeX — vertical axis, scale_type_yN when
         *  scale_typeN is absent. */
        int scaleTypeY { VulkanShaderFormat::source };

        /** @brief Horizontal scale factor (scaleTypeX == source/viewport) or
         *  exact pixel count (scaleTypeX == absolute) — scaleN when present
         *  (both axes at once), else independently scale_xN. Defaults to
         *  1.0f. */
        float scaleX { 1.0f };

        /** @brief See scaleX — vertical axis, scale_yN when scaleN is
         *  absent. */
        float scaleY { 1.0f };

        /** @brief filter_linearN. Absent in the raw .slangp manifest (empty
         *  optional, RetroArch's own RARCH_FILTER_UNSPEC) is resolved to a
         *  concrete bool by jam::VulkanShaderCompiler::compile() — filter
         *  == map::ImageResample::linear — immediately after
         *  VulkanShaderPreset::parse() and before the VulkanShader ctor ever takes this
         *  preset (RetroArch shader_vulkan.cpp:1959's own UNSPEC-falls-back-
         *  to-global-filter contract, verified this session; the render path,
         *  jam::VulkanGraphics/VulkanShaderInstance, never sees compile()'s own
         *  global filter argument, so the fallback must happen here, at
         *  compile time — the same compile-time-resolution philosophy
         *  jam::VulkanShader's own Shadertoy-macro path already documents).
         *  This field is therefore ALWAYS concrete (has_value() true) for
         *  every pass a VulkanShaderFormat::slang preset actually declares, once
         *  compile() has run — empty is reachable only pre-resolution (this
         *  struct's own parse() output, before compile()'s loop runs) or for
         *  a VulkanShaderFormat::shadertoy shader's own always-empty preset.passes,
         *  which compile()'s resolution loop is then a no-op against. Never a
         *  third bool state either way. */
        std::optional<bool> filterLinear {};

        /** @brief wrap_modeN — jam::VulkanShaderFormat's own wrap-mode
         *  ordinal (clampToBorder/clampToEdge/repeat/mirroredRepeat,
         *  jam_vulkan/bimap/jam_VulkanShaderFormat.h). Absent or unrecognized resolves to
         *  VulkanShaderFormat::clampToBorder (RetroArch's own documented default)
         *  via jam::VulkanShaderFormat::getWrapMode(). */
        int wrapMode { VulkanShaderFormat::clampToBorder };

        /** @brief srgb_framebufferN. Absent is false — also stays false when
         *  this same ordinal declares no scale_typeN/scale_type_xN/
         *  scale_type_yN directive of its own, RetroArch's own
         *  FBO_SCALE_FLAG_VALID gate (shader_vulkan.cpp:1984 +
         *  video_shader_parse.c:761-764, verified this session): a
         *  srgb_framebufferN/float_framebufferN flag with no scale_type key
         *  at all is a silent no-op in the reference, encoded here at parse
         *  time (VulkanShaderPreset::parse()) since this struct carries no
         *  separate validity bit. When both this field and floatFramebuffer
         *  are true, srgb wins (RetroArch shader_vulkan.cpp:2018-2025,
         *  if/else-if, srgb checked first). */
        bool srgbFramebuffer { false };

        /** @brief float_framebufferN. Absent is false — same
         *  FBO_SCALE_FLAG_VALID scale_type-presence gate as srgbFramebuffer
         *  above, and loses to it when both are true. */
        bool floatFramebuffer { false };

        /** @brief mipmap_inputN. Absent is false. RetroArch's own directive
         *  meaning "the texture feeding INTO pass N needs mip levels" — this
         *  is a directive on the CONSUMER, not the producer: VulkanShaderInstance::
         *  build() reads passes[N].mipmapInput to decide whether passes[N - 1]'s
         *  own render target (or, when N is this shader's own preset.passes
         *  count, the mandatory Image pass reading the LAST buffer pass's
         *  output) needs a real mip chain built (VulkanRenderResources::
         *  numMipLevels, computeMipLevelCount(), jam_VulkanShaderInstance.cpp) — see
         *  VulkanShaderInstance::build()'s own doc comment for the exact
         *  consumer-ordinal derivation. Ordinal 0 is never consulted this
         *  way (build()'s lookup only ever reads ordinals >= 1) — this
         *  engine's own pass-0 input (background: none; post-process: the
         *  engine-global straight-alpha scene image, which predates any
         *  VulkanShader and carries no mip infrastructure) has no producer render
         *  target to mip-chain at all, so mipmap_input0 is a graceful,
         *  documented no-op rather than an error. */
        bool mipmapInput { false };

        /** @brief frame_count_modN. Absent is 0 — no modulo, this pass's own
         *  raw, unwrapped FrameCount is used as-is. */
        uint32_t frameCountMod { 0 };

        /** @brief aliasN. Absent is empty — this pass carries no
         *  author-assigned alternate name. */
        juce::String alias {};

        /** @brief shaderN — this pass's own .slang source file path, relative
         *  to the owning .slangp preset's own parent directory. Absent is
         *  empty. Consumed by jam::VulkanShaderFormat's own slang directory
         *  reader (jam_vulkan/bimap/jam_VulkanShaderFormat.h) — the include-expanded (VulkanShaderFormat::
         *  expandIncludes()) file content becomes this pass's own shaderState
         *  property, the same canon BufferX/Image slot a VulkanShaderFormat::
         *  shadertoy project writes into. */
        juce::String sourcePath {};
    };

    /** @brief One externally declared LUT/texture — RetroArch .slangp's own
     *  top-level "textures=" list convention (gfx/video_shader_parse.c,
     *  verified this session): PRESET-global, never per-pass (unlike Pass's
     *  own filter/wrap/mipmap directives above) — every declared name is
     *  visible to every pass in the chain, resolved by NAME rather than by
     *  pass ordinal. Field spellings mirror Pass::filterLinear/wrapMode/
     *  mipmapInput exactly — the same value vocabulary, just keyed by name:
     *  \<name\> (path), \<name\>_linear, \<name\>_wrap_mode, \<name\>_mipmap
     *  (jam::VulkanShaderFormat::textureLinear/textureWrapMode/
     *  textureMipmap, jam_vulkan/bimap/jam_VulkanShaderFormat.h). */
    struct Texture
    {
        /** @brief This texture's own author-assigned name — the textures=
         *  list entry itself, and the stem every one of this texture's own
         *  manifest keys is composed from (\<name\>, \<name\>_linear,
         *  \<name\>_wrap_mode, \<name\>_mipmap). Never empty for an entry
         *  that reaches this vector — parse() skips a textures= list entry
         *  that resolves to an empty name. */
        juce::String name {};

        /** @brief \<name\> itself — this texture's own image file path,
         *  relative to the owning .slangp preset's own parent directory (the
         *  SAME relative-path convention Pass::sourcePath already
         *  establishes). Never empty for an entry that reaches this vector —
         *  parse() skips a textures= list entry whose own \<name\> key
         *  resolves to an empty or absent path (this struct's own graceful
         *  last-good tolerance, the same shape an empty presetText already
         *  establishes for passes). */
        juce::String path {};

        /** @brief \<name\>_linear. Absent in the raw .slangp manifest (empty
         *  optional) is left UNRESOLVED here — unlike Pass::filterLinear,
         *  which jam::VulkanShaderCompiler::compile() resolves against its
         *  own global filter argument immediately after parse(), this preset
         *  carries no equivalent per-texture global-filter fallback source at
         *  parse time; resolution is left to whichever consumer (later plan
         *  steps) owns that default. */
        std::optional<bool> filterLinear {};

        /** @brief \<name\>_wrap_mode — jam::VulkanShaderFormat's own
         *  wrap-mode ordinal. Absent or unrecognized resolves to
         *  VulkanShaderFormat::clampToBorder, the SAME documented default
         *  Pass::wrapMode already establishes, via the SAME jam::
         *  VulkanShaderFormat::getWrapMode(). */
        int wrapMode { VulkanShaderFormat::clampToBorder };

        /** @brief \<name\>_mipmap. Absent is false — this texture's own
         *  static LUT needs no mip chain unless the author explicitly asks
         *  for one (no FBO_SCALE_FLAG_VALID-style gate applies here — that
         *  gate is Pass::srgbFramebuffer/floatFramebuffer's own
         *  render-target-only concern, N/A for an externally loaded file). */
        bool mipmapInput { false };
    };

    /** @brief Every buffer pass's own preset directives, in ordinal order —
     *  see Pass's own doc comment for the ordinal-alignment contract with
     *  jam::VulkanShader::passes. Empty for an empty presetText
     *  (jam::VulkanShaderFormat::shadertoy). */
    jam::Array<Pass> passes;

    /** @brief Every externally declared LUT/texture, in textures= list
     *  declaration order — Shadertoy channel binding resolves directly by
     *  NAME (jam::VulkanShaderCompiler::channelMacros(): an entry literally
     *  named iChannel2 binds that reference directly), and each entry's own
     *  push-constant channels[] slot is pure declaration order
     *  (bufferPassCount + index, computed where consumed rather than carried
     *  as a field) — both need this order stable, so this is a jam::Array,
     *  never a name-keyed jam::HashMap. Empty for a preset with no textures=
     *  key (or an empty one) — this codebase's established missing-directive
     *  tolerance, the same shape passes' own empty-for-shadertoy default. */
    jam::Array<Texture> textures;

    /** @brief mesh= key value — an END-extension manifest key (not RetroArch
     *  vocabulary, lexicon/jam_vulkan.md's own preset category doc), carried in the SAME .slangp
     *  manifest as every textures=/pass directive above, for either format:
     *  a full slang preset, or a shadertoy project's own resource-manifest-
     *  only .slangp (textures=/mesh=, no shaders=/passes). As-written,
     *  relative to the owning .slangp preset's own parent directory (the SAME
     *  relative-path convention Pass::sourcePath/Texture::path already
     *  establish) — absolutized by jam::VulkanShaderCompiler::compile()
     *  into jam::VulkanShader::meshPath, the same pass every texture's own
     *  path already goes through. Empty for a preset declaring no mesh= key
     *  at all (every project with no mesh connection). */
    juce::String meshPath {};

    /** @brief mesh_shader= key value — a host-application extension manifest key
     *  (mesh_shader= is a vertex-ANIMATION HOOK into the engine's
     *  own default mesh look, mirroring Shadertoy's mainImage paradigm one
     *  level down — never a full-stage replacement), mirroring meshPath's
     *  exact shape one entry further: a plain GLSL snippet file defining
     *  exactly `void mainMesh (inout vec3 position, inout vec3 normal)` — no
     *  uniform blocks, no descriptor sets, no \#pragma stages, no engine
     *  tokens; the author reads the standard iTime/iTimeDelta/iFrame/
     *  iResolution/iMouse names bare, exactly like any other shader pass
     *  (lexicon/jam_vulkan.md's own preset category doc). As-written, relative to the
     *  owning .slangp preset's own parent directory — absolutized by
     *  jam::VulkanShaderCompiler::compile() into jam::
     *  VulkanShader::meshShaderPath, the SAME pass meshPath itself already goes
     *  through, and its own file content read + \#include-expanded into
     *  jam::VulkanShader::meshShaderSource. Empty for a preset declaring no
     *  mesh_shader= key at all (every mesh connection using only the
     *  engine's own default, unanimated mesh look). */
    juce::String meshShaderPath {};

    /** @brief Every top-level key=value pair this parse does not itself
     *  consume — every key other than "shaders", every recognized per-pass
     *  key (within passes.size() worth of ordinals), "feedback_pass", the
     *  "textures" list key itself, "mesh" (carried onto meshPath above),
     *  "mesh_shader" (carried onto meshShaderPath above), and
     *  every per-name key one of that list's own declared names consumes
     *  (\<name\>/\<name\>_linear/\<name\>_wrap_mode/\<name\>_mipmap, now
     *  carried onto this struct's own textures above rather than merely
     *  recognized-and-discarded) — kept as verbatim string values.
     *  jam::VulkanShaderCompiler::compile()
     *  resolves each entry whose key matches a pass's own \#pragma parameter
     *  identifier (VulkanShaderPass::parameterDefaults) against that pass's own
     *  parameterDefaults map, overlaying the author's preset value
     *  (RetroArch's own override-wins semantics); every other entry,
     *  matching no known parameter identifier, is simply never looked up —
     *  exactly RetroArch's own behavior. */
    jam::HashMap<juce::String, juce::String> parameterOverrides;

    /** @brief Parses @p presetText (a .slangp manifest's raw, whole-file
     *  content — jam::VulkanShaderFormat::preset's own canon-slot property)
     *  into a VulkanShaderPreset. Empty @p presetText (a shadertoy project
     *  declaring no .slangp resource manifest of its own at all) yields zero
     *  passes, zero textures, empty meshPath, empty overrides — the SAME
     *  zero-pass shape a shadertoy project's own resource-manifest-only
     *  .slangp (textures=/mesh=, no shaders=) yields, this parse's own
     *  content-based format-detection contract (the host application's own
     *  shader loader).
     *  @param presetText  Raw .slangp manifest text, or empty.
     *  @return            The parsed preset.
     */
    static VulkanShaderPreset parse (const juce::String& presetText);

    /** @brief Content-derived identity — one jam::Wyhash::mix fold over every
     *  pass's every field, in pass order, then every texture's every field,
     *  in textures= declaration order (mirrors jam::VulkanShader::
     *  computeContentHash()'s own per-pass fold shape, jam_VulkanShader.h —
     *  textures fold the SAME ordered way passes do, never order-
     *  independently, since declaration order is itself part of this
     *  struct's own identity, see textures's own doc comment), then
     *  meshPath's own bytes, then meshShaderPath's own bytes, plus parameterOverrides folded
     *  ORDER-INDEPENDENTLY via XOR-accumulate — the same technique
     *  VulkanShader::computeContentHash() already uses for VulkanShaderPass::
     *  parameterDefaults (jam::HashMap's own iteration order is unspecified,
     *  jam_HashMap.h). Folded directly into jam::VulkanShader::
     *  computeContentHash() so a preset-only edit (scale/filter/wrap/alias/
     *  texture/mesh/override — none of which reach compiled SPIR-V) still
     *  rebuilds a VulkanShader's own GPU execution, the same hot-reload-tuning
     *  contract VulkanShaderPass::parameterDefaults already established for
     *  \#pragma parameter defaults.
     *  @return  This preset's own content-derived hash.
     */
    uint64_t hash() const noexcept;
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam