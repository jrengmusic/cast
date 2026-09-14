/** @file jam_VulkanShaderCompiler.h
 *  @brief GLSL -> SPIR-V compile unit for Shadertoy-compatible and
 *         RetroArch-slang-compatible multi-pass shader projects.
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief Compiles a shaderState ValueTree, keyed by jam::
 *  VulkanShaderFormat's own canon pass-name vocabulary, into a compiled
 *  jam::VulkanShader.
 *
 *  Static-only -- no instance state to carry between calls (the same
 *  compile() serves every caller -- a background component, the app-global
 *  post-process chain, or any future consumer alike). Wraps every present
 *  pass -- Common
 *  and Image resolved via jam::VulkanShaderFormat::common/image, every
 *  buffer pass discovered by walking ordinal from 0
 *  to VulkanShaderUniforms::maxChannelCount - 1 and testing shaderState for a
 *  non-empty property under that ordinal's canon name (ordinal order = pass
 *  order, no sort needed) -- with the format-dispatched wrapper template
 *  (jam::VulkanShaderFormat resolves @p format to its wrapper .frag
 *  BinaryData filename -- uniform/binding declarations matching
 *  VulkanShaderUniforms and the set-0 GLSL contract documented in
 *  jam_VulkanShaderUniforms.h), compiles each assembled pass via
 *  shaderc::Compiler, and assembles the ordered jam::VulkanShader pass
 *  chain.
 *
 *  jam_vulkan owns the pass-name vocabulary itself (jam::
 *  VulkanShaderFormat, jam_vulkan/bimap/jam_VulkanShaderFormat.h) -- every caller (e.g.
 *  the host application's own shader loader) reads/writes shaderState through
 *  that SAME canon table, so no consumer-specific property-name identifier
 *  is passed into compile() at all; format DETECTION is entirely the
 *  caller's job, format DISPATCH is entirely @p format's job.
 *
 *  Every externally declared LUT/texture the parsed preset carries
 *  (VulkanShaderPreset::textures -- the project's own .slangp textures= list, THE
 *  resource manifest for either format) is assigned a channel slot on the
 *  SAME push-constant channels[] array every buffer pass already indexes --
 *  pure declaration order, texture K's own slot is always
 *  bufferPassNames.size() + K (RetroArch's own textures= directive quartet
 *  carries no channel key at all, so no explicit-slot override exists to
 *  honor -- see channelMacros()'s own doc comment for the exact
 *  slot-assignment convention) -- so this discovery loop's own buffer-pass
 *  count and preset.textures' own count together are asserted, before a
 *  single pass is compiled, to stay in-bound (VulkanShaderUniforms::
 *  maxChannelCount); order-assigned slots can never collide with each other
 *  or with a buffer pass's own ordinal, so no separate collision check is
 *  needed (mirrors jam::VulkanShader's own ctor pass-count ceiling assert,
 *  jam_VulkanShader.h).
 *
 *  Opacity and resolution scale are NOT compile()'s concern -- neither is a
 *  jam::VulkanShader field (jam_VulkanShader.h's VulkanShader doc comment); both
 *  are supplied by the caller at render/build time instead (jam::
 *  render(), jam::VulkanShaderComponent::setShader()/setParams(),
 *  jam::VulkanEngine::setPostProcess()/setPostProcessParams()). Only
 *  ImageResample is a compile()-time input -- baked directly into the
 *  generated sampler macros, never stored on the resulting VulkanShader.
 *
 *  shaderc is used only inside jam_VulkanShaderCompiler.cpp -- no shaderc
 *  type appears in this header, so no shaderc symbol reaches any consumer.
 */
struct VulkanShaderCompiler
{
    /** @brief Compiles @p shaderState into a jam::VulkanShader.
     *
     *  @p shaderState is one shader instance's state tree -- a flat property
     *  set holding GLSL source per present pass, keyed by jam::
     *  VulkanShaderFormat's own canon pass-name vocabulary (Common, Image, and every
     *  buffer-pass ordinal's canon name). Common and Image are resolved via
     *  VulkanShaderFormat::common/image; every buffer pass is discovered by
     *  walking ordinal from 0 to VulkanShaderUniforms::
     *  maxChannelCount - 1 and testing shaderState for a non-empty property
     *  under that ordinal's canon name -- ordinal order is already pass
     *  order, no sort needed. @p isBackground is this compile's sole mode
     *  signal (SSOT, resolved by the caller from its own tree-type/mode
     *  knowledge, never re-derived here) -- it decides whether the generated
     *  wrapper exposes iScene at all (see assemblePass()'s doc comment) --
     *  the mode-specific opacity mix itself lives in the
     *  dedicated combine shaders downstream (jam_vulkan/shaders/
     *  background_combine.frag/post_process_combine.frag), not in anything
     *  this compile() call produces. @p format selects which engine-owned
     *  wrapper template (jam::VulkanShaderFormat) every pass is assembled
     *  through and whether each pass's own source is split into
     *  vertex+fragment stages first (VulkanShaderFormat::slang's \#pragma
     *  stage vertex/\#pragma stage fragment convention -- see
     *  splitSlangStages()'s doc comment; VulkanShaderFormat::shadertoy passes
     *  are fragment-only, and every resulting jam::
     *  VulkanShaderPass::vertexSpirv stays empty). The Image pass is mandatory --
     *  an empty image source, or any present pass failing to compile
     *  (diagnostic logged via jam::debug::Log, shaderc error text + pass
     *  name), yields nullptr and no jam::VulkanShader is constructed.
     *  Callers keep their last-good VulkanShader on nullptr.
     *
     *  Also reads @p shaderState's own VulkanShaderFormat::preset property (raw
     *  .slangp manifest text -- the project's own full preset for @p format
     *  == VulkanShaderFormat::slang; an optional resource-manifest-only .slangp,
     *  textures=/mesh=, no shaders=/passes, for @p format ==
     *  VulkanShaderFormat::shadertoy when the project declares one at all, empty
     *  otherwise) through jam::VulkanShaderPreset::parse(), overlays every
     *  resulting parameterOverrides entry onto the matching \#pragma
     *  parameter identifier's own author default (RetroArch's own
     *  override-wins semantics -- see jam_VulkanShaderPreset.h's own doc
     *  comment), overlays parsePassName()'s own \#pragma name fallback onto
     *  every VulkanShaderPreset::Pass whose own aliasN directive is empty (aliasN
     *  outranks \#pragma name, RetroArch's own documented precedence --
     *  slang_process.cpp:941-942). Every preset.textures entry's own path,
     *  preset.meshPath, and preset.meshShaderPath are then absolutized
     *  against @p shaderState's own root-level Id::path property
     *  (the host application's own shader loader's stamp)
     *  -- every one is as-written, relative to the project directory,
     *  RetroArch's own textures= convention extended verbatim to
     *  mesh=/mesh_shader=. The resulting VulkanShaderPreset -- absolute paths
     *  throughout -- is carried forward into the constructed jam::
     *  VulkanShader's own preset field, preset.meshPath into VulkanShader::meshPath, and
     *  preset.meshShaderPath -- its own file content read and \#include-
     *  expanded (VulkanShaderFormat::expandIncludes()), the SAME expansion every
     *  other .slang source gets -- into VulkanShader::meshShaderPath/
     *  meshShaderSource (mesh_shader= is a plain GLSL snippet
     *  defining exactly `void mainMesh (inout vec3 position, inout vec3
     *  normal)`, a vertex-ANIMATION HOOK into the engine's own default mesh
     *  look, mirroring Shadertoy's mainImage paradigm one level down -- never
     *  a full-stage replacement, zero bespoke uniform vocabulary beyond the
     *  standard iTime/iTimeDelta/iFrame/iResolution/iMouse names every other
     *  shader pass already reads).
     *
     *  @param shaderState  The shader instance's state tree, keyed by
     *                      jam::VulkanShaderFormat's own canon pass-name
     *                      vocabulary, plus its own root-level Id::path
     *                      property (the shader project directory).
     *  @param isBackground True for a background-mode tree shape (no
     *                      resolved scene to mix against -- the
     *                      component-transparency formula), false for a
     *                      post-processing-mode tree shape (the effect-
     *                      intensity rgb mix against the straight-alpha
     *                      resolved scene, re-premultiplied by the scene's
     *                      own immutable alpha afterward -- see
     *                      jam::VulkanShaderUniforms::opacity's doc comment
     *                      for the exact GLSL formula).
     *  @param format       The shader project's source format -- selects the
     *                      wrapper template (jam::VulkanShaderFormat) and
     *                      whether pass sources are split into
     *                      vertex+fragment stages first.
     *  @param filter       VulkanImage resample mode for intermediate-pass
     *                      upscaling -- selects, at generation time, which
     *                      set-0 sampler binding (linearSampler /
     *                      nearestSampler) the generated prelude's channel
     *                      macros sample through.
     *  @return             The compiled VulkanShader, or nullptr.
     */
    static std::unique_ptr<jam::VulkanShader> compile (const juce::ValueTree& shaderState,
                                                          bool isBackground,
                                                          int format,
                                                          map::ImageResample::value filter);

    static std::unique_ptr<jam::VulkanShader> compile (const juce::String& imageSource,
                                                          bool isBackground,
                                                          int format,
                                                          map::ImageResample::value filter);

    static std::unique_ptr<jam::VulkanShader> compile (const juce::File& shaderDir,
                                                          bool isBackground,
                                                          int format,
                                                          map::ImageResample::value filter);

    /** @brief Compiles one mesh vertex-stage wrapper template (@p templateFilename
     *  -- Id::meshDefaultVertex or Id::meshEdgeVertex) with its
     *  own \%\%mainMesh\%\% placeholder spliced to @p meshHookSource when
     *  non-empty, or this engine's own no-op `void mainMesh (inout vec3
     *  position, inout vec3 normal) {}` definition otherwise (the default,
     *  unanimated look) -- mirrors assemblePass()'s own wrapper-template-
     *  plus-replaceholder-plus-compileSpirv precedent (shadertoy_wrapper.frag)
     *  one level down: ONE template source per stage, spliced fresh for every
     *  caller (jam::VulkanGraphics, for the engine-default hookless
     *  pipelines; jam::VulkanShaderInstance::buildMeshHookPipelines(), for
     *  a VulkanShader's own hooked pipelines) rather than a duplicated hooked/
     *  hookless copy of either vertex stage. Always compiled at
     *  VulkanShaderFormat::shadertoy's own performance optimization level -- this
     *  vertex stage is never SPIR-V-reflected (jam::VulkanShaderReflection
     *  matches only VulkanShaderFormat::slang passes), so VulkanShaderFormat::slang's own
     *  opt-zero name-preservation reason does not apply here.
     *  @param templateFilename  Id::meshDefaultVertex.toString() or Id::meshEdgeVertex.toString().
     *  @param meshHookSource    The project's own mesh_shader= mainMesh
     *                           snippet (jam::VulkanShader::
     *                           meshShaderSource, already \#include-expanded),
     *                           or empty for the default, unanimated look.
     *  @return                 The compiled SPIR-V, or an empty juce::MemoryBlock
     *                           on failure (diagnostic logged via jam::debug::Log).
     */
    static juce::MemoryBlock compileMeshVertexStage (const juce::String& templateFilename,
                                                      const juce::String& meshHookSource);

private:
    /** @brief Compile-stage selector for compileSpirv() -- keeps shaderc's
     *  own shader-type enum out of this header entirely (shaderc/shaderc.hpp
     *  is included only inside jam_VulkanShaderCompiler.cpp).
     */
    enum class Stage
    {
        fragment,
        vertex
    };

    /** @brief One .slang pass's split vertex-stage and fragment-stage
     *  compilable source text -- see splitSlangStages()'s doc comment.
     */
    struct SlangStages
    {
        /** @brief Shared prelude (text preceding the first \#pragma stage
         *  marker) followed by the \#pragma stage vertex section. */
        juce::String vertex;
        /** @brief Shared prelude followed by the \#pragma stage fragment
         *  section. */
        juce::String fragment;
    };

    /** @brief Parses every RetroArch \#pragma parameter declaration
     *  (`\#pragma parameter <identifier> "<label>" <default>
     *  <min> <max> [<step>]`) out of @p source into an identifier -> author-
     *  default map. glslang silently ignores this pragma -- the declaration
     *  line never reaches compiled SPIR-V, so this parse over the pass's own
     *  raw, unsplit source text is the only place a parameter's author
     *  DEFAULT is ever read; RetroArch's own runtime UI is an override
     *  mechanism layered on top of this default, never the source of truth
     *  (the default IS the author's rendered intent absent any override).
     *  Every trimmed source line starting with the pragma is tokenized on
     *  whitespace, treating the quoted `"<label>"` field (which may itself
     *  contain spaces) as a single token, so the identifier and default
     *  fields always land at the same fixed token positions regardless of
     *  label content.
     *  @param source  A pass's raw, unsplit source text (for
     *                  VulkanShaderFormat::slang, the shared prelude a
     *                  \#pragma parameter is conventionally declared in,
     *                  before either \#pragma stage marker).
     *  @return         Every declared parameter identifier mapped to its
     *                  own author-default value. Empty when @p source
     *                  declares no \#pragma parameter at all.
     */
    static jam::HashMap<juce::String, float> parseParameterDefaults (const juce::String& source);

    /** @brief Parses RetroArch's own \#pragma name declaration
     *  (RetroArch's own source-level convention, slang_process.cpp:
     *  941-942's own fallback read of it) out of @p source -- the author's
     *  own pass-output name, consumed ONLY as a preset aliasN fallback: when
     *  a .slangp preset declares no aliasN directive for a pass at all
     *  (jam::VulkanShaderPreset::Pass::alias stays empty), that SAME
     *  RetroArch behavior is this pass's own \#pragma name instead, never a
     *  second, independent alias source layered on top of aliasN (RetroArch's
     *  own aliasN-outranks-\#pragma-name precedence -- see compile()'s own
     *  doc comment). Every trimmed source line starting with the pragma is
     *  read past the pragma text itself, trimmed again -- RetroArch's own
     *  \#pragma name syntax carries a single bare identifier token, no quoted
     *  label field the way \#pragma parameter does, so no tokenizing is
     *  needed here.
     *  @param source  A pass's raw, unsplit source text (the shared prelude
     *                 a \#pragma name is conventionally declared in, before
     *                 either \#pragma stage marker -- same convention
     *                 \#pragma parameter uses, see parseParameterDefaults()'s
     *                 own doc comment).
     *  @return        The declared name, or an empty juce::String when
     *                 @p source declares no \#pragma name at all.
     */
    static juce::String parsePassName (const juce::String& source);

    /** @brief Assembles one pass's full fragment source: the format-
     *  dispatched wrapper template (jam::VulkanShaderFormat resolves
     *  @p format to its BinaryData filename -- uniform/binding declarations,
     *  push-constant block -- textually identical for every pass and both
     *  modes, so it always matches jam::VulkanShaderUniforms' C++ layout
     *  even when iScene is unused -- named sampler macros per buffer pass
     *  plus its iChannelN aliases, and the iScene compatibility macro when
     *  @p isBackground is false), then @p commonSource (if present), then
     *  @p passSource. Every pass -- buffer passes and the Image pass alike --
     *  writes its mainImage() output raw, regardless of mode: the mode-
     *  specific opacity mix documented on jam::VulkanShaderUniforms::opacity
     *  is no longer generated here -- it lives in the two dedicated, engine-
     *  owned combine shaders (jam_vulkan/shaders/background_combine.frag/
     *  post_process_combine.frag) that sample the Image pass's own offscreen
     *  gather target back afterward (see VulkanShaderInstance::imagePassGatherTarget's
     *  doc comment). Only ever reached for VulkanShaderFormat::shadertoy —
     *  compile()'s compilePass bypasses this call entirely for
     *  VulkanShaderFormat::slang (the 9b zero-injection contract documented at
     *  that bypass); the shadertoy wrapper carries its own literal trailing
     *  main() calling mainImage().
     *  @param passSource       This pass's fragment-stage GLSL source (its
     *                          mainImage()).
     *  @param commonSource     The project's Common source, prepended before
     *                          @p passSource when present; empty otherwise.
     *  @param bufferPassNames  Every buffer pass name present on this VulkanShader,
     *                          already in ordinal order (jam::
     *                          VulkanShaderFormat's own canon ordering -- compile()'s
     *                          own discovery loop, no sort needed) --
     *                          channelMacros()'s SSOT for both the named
     *                          sampler macro per pass and its iChannelN
     *                          aliases.
     *  @param textures         Every externally declared LUT/texture this
     *                          VulkanShader's own preset carries (jam::
     *                          VulkanShaderPreset::textures -- the project's own
     *                          .slangp textures= list, in declaration order)
     *                          -- channelMacros()'s SSOT for the named-texture
     *                          sampler macro, each assigned the channel slot
     *                          immediately after every @p bufferPassNames
     *                          ordinal, pure declaration order (see
     *                          channelMacros()'s own doc comment for the
     *                          exact slot-assignment convention).
     *  @param filter           Resample mode selecting which set-0 bindless
     *                          `sampler2D[]` array (binding 3 = linear,
     *                          binding 4 = nearest) the generated channel
     *                          macros index into.
     *  @param isBackground     True for the background tree shape, false for
     *                          the post-processing tree shape -- selects
     *                          whether the generated prelude exposes iScene
     *                          at all.
     *  @param format           Selects which wrapper template (jam::
     *                          VulkanShaderFormat) this pass is assembled through.
     */
    static juce::String assemblePass (const juce::String& passSource, const juce::String& commonSource,
                                       const juce::StringArray& bufferPassNames,
                                       const jam::Array<VulkanShaderPreset::Texture>& textures,
                                       map::ImageResample::value filter,
                                       bool isBackground, int format);

    /** @brief Returns one \#define line per buffer pass entry, rebinding each
     *  pass's own name to a sampler2D expression indexing @p filter's set-0
     *  bindless `sampler2D[]` array (binding 3 = linear, binding 4 = nearest)
     *  at its ordinal's push-constant channels[] slot -- a first-class
     *  sampler2D value, legal as a user-function argument (glslang rejects
     *  the binding-0 `texture2D` + binding-1/2 `sampler` constructor form
     *  there) -- plus shadertoyChannelAliases()'s own iChannel0-3 paste-compat
     *  aliases (capped at 4), one additional \#define per alias binding to
     *  that SAME expression. channelMacros() is only ever reached for
     *  VulkanShaderFormat::shadertoy -- compile()'s own compilePass lambda skips
     *  assemblePass() (and everything it calls) entirely for VulkanShaderFormat::
     *  slang, per the slang zero-injection contract (see compile()'s own doc
     *  comment) -- so no per-format alias dispatch is needed here.
     *
     *  @p textures (jam::VulkanShaderPreset::textures, the project's own
     *  .slangp textures= list, in declaration order) then is assigned a
     *  channel slot each -- pure declaration order, texture K's own slot is
     *  always bufferPassNames.size() + K, the SAME push-constant channels[]
     *  array every buffer pass above already indexes, never a separate array
     *  (RetroArch's own textures= directive quartet carries no channel key
     *  at all, so no explicit-slot override exists to honor) -- and gets
     *  exactly one \#define line binding its own author-assigned name
     *  directly to that same sampler2D-expression shape, no channel alias (a
     *  named texture carries no iChannelN paste-compat concern -- the name
     *  itself IS the channel binding: an entry literally named iChannel2
     *  binds that reference directly). Each entry's own VulkanShaderPreset::
     *  VulkanTexture::filterLinear picks texturesLinear vs texturesNearest
     *  independently of @p filter when present (value_or (@p filter)
     *  otherwise) -- a per-texture choice, unlike every buffer pass above,
     *  which always samples through the SAME @p filter. The caller
     *  (VulkanShaderCompiler::compile()) asserts every slot stays in-bound
     *  (VulkanShaderUniforms::maxChannelCount) before this function is ever
     *  reached -- order-assigned slots can never collide, so no separate
     *  collision check is needed; every slot assigned here always lands
     *  inside the push-constant channels[] array's own bound.
     */
    static juce::String channelMacros (const juce::StringArray& bufferPassNames,
                                        const jam::Array<VulkanShaderPreset::Texture>& textures,
                                        map::ImageResample::value filter);

    /** @brief Returns the \#define iScene line rebinding the push-constant
     *  bindless index to a sampler2D expression indexing @p filter's set-0
     *  bindless `sampler2D[]` array (binding 3 = linear, binding 4 = nearest)
     *  -- a first-class sampler2D value, legal as a user-function argument
     *  (glslang rejects the binding-0 `texture2D` + binding-1/2 `sampler`
     *  constructor form there). Only ever appended by assemblePass() on the
     *  post-processing path -- the background path never appends it, so a
     *  background shader referencing iScene fails to compile with a clear
     *  diagnostic instead of silently sampling an invalid bindless index
     *  (see jam::VulkanShaderUniforms::iScene's doc comment). sceneMacro()
     *  is only ever reached for VulkanShaderFormat::shadertoy -- compile()'s own
     *  compilePass lambda skips assemblePass() (and everything it calls)
     *  entirely for VulkanShaderFormat::slang, per the slang zero-injection
     *  contract (see compile()'s own doc comment) -- so no per-format alias
     *  dispatch is needed here.
     */
    static juce::String sceneMacro (map::ImageResample::value filter);

    /** @brief Splits a raw RetroArch-.slang pass source into its vertex-stage
     *  and fragment-stage compilable units at the \#pragma stage vertex /
     *  \#pragma stage fragment markers. The text preceding the first marker
     *  (RetroArch's documented shared-prelude convention -- \#version and any
     *  declarations common to both stages) is prepended to both returned
     *  stages, each of which is then independently a complete compilable GLSL
     *  unit.
     *  @param source  The pass's raw, unsplit .slang source text.
     *  @return        The pass's vertex-stage and fragment-stage source text.
     */
    static SlangStages splitSlangStages (const juce::String& source);

    /** @brief Compiles @p source (already assembled by assemblePass(), for
     *  @p stage == Stage::fragment; already split by splitSlangStages(), for
     *  @p stage == Stage::vertex) to SPIR-V via shaderc, targeting Vulkan 1.0,
     *  with @p format's own optimization level. @p format == VulkanShaderFormat::
     *  slang compiles unoptimized -- jam::VulkanShaderReflection's SPIR-V
     *  reflection (jam_VulkanShaderReflection.cpp) matches UBO/push_constant
     *  members and texture resources BY NAME via OpName/OpMemberName, which
     *  shaderc's performance optimization strips (spirv-opt's performance
     *  passes remove all debug names), matching RetroArch's own reference
     *  behavior of running glslang with no optimizer at all. @p format ==
     *  VulkanShaderFormat::shadertoy keeps performance optimization, since its
     *  SPIR-V is never reflected. @p passName tags shaderc diagnostics
     *  and identifies the pass in the logged error line.
     *  @return  The compiled SPIR-V, or an empty juce::MemoryBlock on
     *           failure (diagnostic logged via jam::debug::Log).
     */
    static juce::MemoryBlock compileSpirv (const juce::String& source, const juce::String& passName, Stage stage,
                                            int format);
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam