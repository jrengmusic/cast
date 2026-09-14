/** @file jam_VulkanShaderUniforms.h
 *  @brief Shadertoy-compatible push-constant block stamped once per VulkanShader
 *         execution and pushed before every pass draw (buffer passes and the
 *         Image pass alike).
 *
 *  Set/binding GLSL contract for every VulkanShader-execution pass (buffer passes
 *  and the Image pass alike) — VulkanGraphics::getOrCreateShaderInstanceLayout()'s
 *  one descriptor set is the same vk::DescriptorSetLayout object as the main
 *  21-pipeline layout's set 1 (VulkanPipelines::getLayoutSet1(), declared in
 *  shaders/bindless_texture.glsl), reused verbatim at set index 0 here since
 *  a VulkanShader-execution pipeline layout carries no MVP/UBO set. Sampling
 *  resample mode is resolved by jam::VulkanShaderCompiler at GLSL-generation time
 *  (baked directly into each pass's compiled SPIR-V before a
 *  jam::VulkanShader is ever constructed — VulkanShader itself carries no
 *  resample-mode field, see map::ImageResample's own doc comment
 *  (lexicon/jam_vulkan.md) —
 *  the generated prelude (shadertoy_wrapper.frag) for a shader pass's
 *  fragment code declares this same set-0 contract and samples through the
 *  combined-image-sampler array at binding 3 or binding 4, matching the
 *  chosen resample mode:
 *  @code
 *  layout(set = 0, binding = 0) uniform texture2D textures[];
 *  layout(set = 0, binding = 1) uniform sampler linearSampler;
 *  layout(set = 0, binding = 2) uniform sampler nearestSampler;
 *  layout(set = 0, binding = 3) uniform sampler2D texturesLinear[];
 *  layout(set = 0, binding = 4) uniform sampler2D texturesNearest[];
 *  @endcode
 *  Bindings 3/4 expose the SAME image views as binding 0, pre-paired with
 *  the linear/nearest sampler (written together by
 *  VulkanGraphics::writeBindlessTextureDescriptor()) — a channel macro therefore
 *  expands to a first-class sampler2D VALUE (texturesLinear[...]), legal as
 *  a user-function argument, unlike the sampler2D(texture2D, sampler)
 *  constructor form glslang only allows at the point of use. Engine-owned
 *  shaders (background_combine.frag, post_process_combine.frag,
 *  straight_alpha.frag) still sample through bindings 0 + 1/2's constructor
 *  form — valid at point of use, they never pass samplers to functions.
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief Push-constant block pushed before every jam::VulkanShader pass
 *  draw (buffer passes and the Image pass alike).
 *
 *  VulkanShaderInstance::stampUniforms() (resource/jam_VulkanShaderInstance.h) is
 *  the SOLE write site for iResolution/iTime/iTimeDelta/iFrame/iMouse (SSOT,
 *  one stamp per VulkanShader render — every pass this call records reuses the same
 *  stamped values); the caller overwrites channels/opacity per pass
 *  afterward via VulkanShaderInstance::stampChannels() (SSOT for that assignment,
 *  shared by every draw site), and stamps iScene once via
 *  VulkanGraphics::recordShaderBufferPasses()'s sceneBindlessIndex parameter (SSOT
 *  for that value — copied forward, never re-stamped, by every pass sharing
 *  the same base uniforms).
 *
 *  Member order deliberately does NOT follow Shadertoy's conventional listing
 *  order — it is chosen so every member lands on its natural std430 alignment
 *  with zero implicit padding: iMouse (vec4, 16-byte align) @0, iResolution
 *  (vec2, 8-byte align) @16, iTime/iTimeDelta/iFrame (scalars, 4-byte align)
 *  @24-35, channels[maxChannelCount] (int array, 4-byte align, std430 stride
 *  4) immediately after, then iScene/opacity — sizeof exactly fills
 *  minimumGuaranteedPushConstantSize, no gaps.
 *
 *  Matching GLSL push_constant block declaration — VulkanShaderCompiler's prelude
 *  (jam_VulkanShaderCompiler.cpp, shadertoy_wrapper.frag's \@maxChannelCount\@
 *  token) mirrors this textually for VulkanShaderFormat::shadertoy passes
 *  (channels' array size read directly from maxChannelCount, never a
 *  duplicated literal); keep both sides in sync on any other change. A
 *  VulkanShaderFormat::slang pass carries no wrapper and no
 *  \@maxChannelCount\@ substitution at all — compiled completely as-is,
 *  per the slang zero-injection contract — and never declares this struct's
 *  channels[] array; its own compiled body is shaped instead by its own
 *  SPIR-V-reflected UBO/push_constant layout (VulkanShaderReflection/
 *  VulkanSlangPassResources, jam_vulkan/resource/jam_VulkanSlangPassResources.h):
 *  @code
 *  layout(push_constant) uniform ShaderPC
 *  {
 *      vec4  iMouse;
 *      vec2  iResolution;
 *      float iTime;
 *      float iTimeDelta;
 *      int   iFrame;
 *      int   channels[21]; // literal = jam::VulkanShaderUniforms::maxChannelCount
 *      int   iScene;
 *      float opacity;
 *  };
 *  @endcode
 */
struct VulkanShaderUniforms
{
    /** @brief VulkanDevice push-constant floor guaranteed by the Vulkan spec (every
     *  conformant implementation supports at least this many bytes) — the
     *  ceiling this struct's size must fit under. */
    static constexpr uint32_t minimumGuaranteedPushConstantSize { 128 };

    /** @brief Sentinel meaning "no resolved scene texture available" —
     *  stamped into iScene by the background render() path
     *  (VulkanLowLevelGraphicsContext::renderShader(), jam_VulkanLowLevelGraphicsContextRender.cpp)
     *  via VulkanGraphics::recordShaderBufferPasses()'s sceneBindlessIndex parameter,
     *  where no scene exists to fall back to. Named to avoid a magic -1
     *  literal at that call site. */
    static constexpr int32_t noScene { -1 };

    /** @brief Combined byte size of every fixed-width field below other than
     *  channels[] — iMouse (4 floats) + iResolution (2 floats) + iTime +
     *  iTimeDelta + iFrame + iScene + opacity. The exact quantity
     *  maxChannelCount's derivation subtracts from
     *  minimumGuaranteedPushConstantSize before dividing by an int32_t's
     *  width — see maxChannelCount's own doc comment. */
    static constexpr uint32_t otherFieldsSize {
        sizeof (float) * 4      // iMouse
        + sizeof (float) * 2    // iResolution
        + sizeof (float)        // iTime
        + sizeof (float)        // iTimeDelta
        + sizeof (int32_t)      // iFrame
        + sizeof (int32_t)      // iScene
        + sizeof (float)        // opacity
    };

    /** @brief Maximum number of buffer-pass channel slots this push-constant
     *  block can carry — derived, not hand-picked, from the 128-byte
     *  guaranteed push-constant floor: every byte not already claimed by
     *  otherFieldsSize is divided evenly among 4-byte int32_t channel slots,
     *  filling the floor exactly (zero implicit padding, see this struct's
     *  doc comment). VulkanShaderPass count is asserted against this ceiling +1
     *  (the mandatory Image pass) at construction (jam_VulkanShader.h). */
    static constexpr int32_t maxChannelCount {
        static_cast<int32_t> ((minimumGuaranteedPushConstantSize - otherFieldsSize) / sizeof (int32_t))
    };

    /** @brief Mouse state, sign-encoded per Shadertoy convention — see
     *  VulkanShaderInstance::stampUniforms()'s own doc comment for the exact
     *  per-component encoding (this struct's sole write site, SSOT). All
     *  zero when no interaction has occurred yet. */
    float iMouse[4] { 0.0f, 0.0f, 0.0f, 0.0f };

    /** @brief Render target size in pixels, xy only — Shadertoy's legacy .z
     *  (pixel aspect ratio) is unused by modern shaders and omitted here. */
    float iResolution[2] { 0.0f, 0.0f };

    /** @brief Seconds elapsed since this VulkanShader execution's construction. */
    float iTime { 0.0f };

    /** @brief Seconds elapsed since the previous stampUniforms() call for this execution. */
    float iTimeDelta { 0.0f };

    /** @brief Frame counter owned by this VulkanShader execution — incremented once
     *  per stampUniforms() call. */
    int32_t iFrame { 0 };

    /** @brief Bindless array index per buffer-pass ordinal, or the SSOT
     *  fallback for every ordinal beyond the executing VulkanShader's own buffer-
     *  pass count — VulkanShaderInstance::stampChannels() is the SOLE write site
     *  (SSOT, shared by every draw site: buffer passes, the in-scene VulkanImage
     *  pass, the post-process composite). The fallback is
     *  VulkanShaderUniforms::noScene on the in-scene background path
     *  (VulkanLowLevelGraphicsContext::renderShader() has no resolved scene to
     *  fall back to — END's generated prelude for that mode deliberately
     *  omits the iScene macro, so a background shader referencing iScene
     *  fails to compile with a clear diagnostic instead of silently sampling
     *  an invalid bindless index) or the resolved scene's own bindless index
     *  on the post-process composite path
     *  (VulkanGraphics::recordPostProcessCompositeDrawCommands(),
     *  jam_VulkanGraphicsSlangPass.cpp) — see stampChannels()'s doc comment. */
    int32_t channels[maxChannelCount] { };

    /** @brief Bindless array index of the resolved scene texture. Stamped
     *  meaningfully only at the post-process composite call site
     *  (VulkanGraphics::recordPostProcessCompositeDrawCommands(),
     *  jam_VulkanGraphicsSlangPass.cpp — the same straight-alpha scene index
     *  it already computes for channels[]' fallback, passed through as
     *  recordShaderBufferPasses()'s sceneBindlessIndex parameter). This is
     *  straightAlphaImage's bindless index, NOT sceneColorImage's (the
     *  premultiplied scene) — VulkanGraphics::recordStraightAlphaPass() runs first
     *  and un-premultiplies the resolved scene into straightAlphaImage before
     *  any post-process user shader ever samples iScene, so a user shader's
     *  own alpha replacement never corrupts the immutable glass alpha this
     *  composite re-premultiplies by afterward (see this struct's opacity doc
     *  comment for the exact GLSL formula). VulkanShaderUniforms::noScene (-1) on
     *  the background render() path (VulkanLowLevelGraphicsContext::renderShader())
     *  — the background shader has no resolved scene to sample; END's
     *  generated prelude for the background mode deliberately omits the
     *  \#define iScene macro, so any user shader code referencing iScene
     *  fails to compile with a clear diagnostic instead of silently sampling
     *  an invalid bindless index. */
    int32_t iScene { -1 };

    /** @brief This pass's blend opacity — the caller's own opacity parameter
     *  (jam::render(), VulkanShaderComponent::setShader()/setParams(),
     *  VulkanEngine::setPostProcess()/setPostProcessParams()) for the Image pass
     *  draw; buffer passes push 1.0 (their output is never directly
     *  visible). The Image pass's own offscreen gather target
     *  (VulkanShaderInstance::imagePassGatherTarget, VulkanShaderInstance::buildRenderResources())
     *  is built with VulkanPipelines::opaqueBlendAttachment() like every buffer
     *  pass — a direct, unblended write of the pass's raw, unmodified colour.
     *  A VulkanShaderFormat::shadertoy pass's shader (assembled via
     *  jam_vulkan/shader/shadertoy_wrapper.frag) writes that raw userColor
     *  through this same push-constant block, computing neither formula
     *  below itself. A VulkanShaderFormat::slang pass's own already-complete
     *  main(), compiled as-is with no wrapper, never uses this struct at
     *  all for its own compiled body — its own SPIR-V-reflected UBO/
     *  push_constant (VulkanShaderReflection/VulkanSlangPassResources, jam_vulkan/
     *  resource/jam_VulkanSlangPassResources.h) supplies whatever uniforms
     *  it needs instead, and it likewise writes its own raw userColor,
     *  independently of either formula below. The two formulas below now
     *  live in the two dedicated,
     *  engine-owned combine shaders (jam_vulkan/shaders/background_combine.frag/
     *  post_process_combine.frag) that sample the gather target back through
     *  channels[0] and this same opacity value, pushed unchanged
     *  (VulkanGraphics::getOrCreateBackgroundCombinePipeline()/
     *  getOrCreatePostProcessCombinePipeline(),
     *  VulkanLowLevelGraphicsContext::recordShaderImagePassDrawCommands()/
     *  VulkanGraphics::recordPostProcessCompositeDrawCommands()) — not GLSL text
     *  owned by jam::VulkanShaderCompiler's static main() templates
     *  anymore. This struct only carries the value through; the combine
     *  shader compiled for the executing mode applies one of two DIFFERENT
     *  semantics (a single formula cannot serve both — see
     *  config/lua/display.lua's background_opacity/post_processing_opacity
     *  documentation):
     *  post-process — `vec4 scene = texture (iScene, uv); fragColor = vec4
     *  (mix (scene.rgb, userColor.rgb, opacity) * scene.a, scene.a)`, an
     *  effect-intensity mix over the STRAIGHT-alpha scene (iScene resolves to
     *  straightAlphaImage's bindless index on this path — see this struct's
     *  iScene doc comment — never the premultiplied sceneColorImage): opacity
     *  0 reproduces the resolved scene's rgb exactly, opacity 1 is the user
     *  shader's own unmixed rgb, every value between mixes rgb only; the
     *  result is then re-premultiplied by scene.a and scene.a is carried
     *  through unchanged either way — glass alpha is immutable end-to-end,
     *  never read from or blended against userColor.a; background — `fragColor
     *  = vec4 (userColor.rgb * (userColor.a * opacity), userColor.a * opacity)`,
     *  a component-transparency scale with no scene to mix against (the
     *  background paints behind the scene, not over it) — opacity 0 is fully
     *  transparent, opacity 1 is the user shader's own alpha; the shader
     *  premultiplies rgb by that combined alpha itself because
     *  VulkanPipelines::alphaBlendAttachment()'s srcRgbFactor is eOne (colour
     *  arriving already premultiplied), so the fixed-function blend above
     *  composites the premultiplied result over whatever the scene render
     *  pass already painted beneath it without re-scaling rgb. */
    float opacity { 1.0f };
};

static_assert (sizeof (VulkanShaderUniforms) == VulkanShaderUniforms::minimumGuaranteedPushConstantSize,
              "VulkanShaderUniforms must exactly fill the guaranteed device push-constant floor");

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam