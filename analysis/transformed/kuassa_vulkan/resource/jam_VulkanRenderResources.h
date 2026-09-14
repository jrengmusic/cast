/** @file jam_VulkanRenderResources.h
 *  @brief One pass's offscreen render target(s) — count-parameterized so the
 *         same type serves both a buffer pass's ping-pong pair and a
 *         non-feedback single target (the Image pass's offscreen gather
 *         target, jam::VulkanShaderInstance::imagePassGatherTarget).
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief One pass's offscreen render target(s), that pass's own dedicated
 *  pipeline, and the stable bindless slot(s) sampling this target requires —
 *  built once, eagerly, by VulkanShaderInstance::buildRenderResources(). Mirrors
 *  VulkanTransparencyStack::TransparencyLayer's public-field shape: the recording
 *  logic (VulkanGraphics::recordShaderBufferPasses(), jam_VulkanGraphicsSlangPass.cpp)
 *  reads/writes these fields directly.
 *
 *  images.size()/framebuffers.size()/bindlessIndex.size() is 2 for a
 *  ping-pong-capable buffer pass — every buffer pass is unconditionally
 *  feedback-capable (VulkanShaderPass's own doc comment, jam_VulkanShaderPass.h),
 *  so both halves are always populated and alternate as read/write
 *  (self-read must never alias the write target within one draw). The VulkanImage
 *  pass's gather target (VulkanShaderInstance::imagePassGatherTarget) is 1
 *  (non-feedback single target: a single fullscreen write every frame, no
 *  self-read, so a second half would sit permanently unused — one wasted
 *  full-resolution GPU image + framebuffer + bindless slot, for this
 *  instance's entire lifetime, avoidable simply by building at count 1
 *  instead of the ping-pong pair's count 2) UNLESS a real .slang shader's
 *  Image pass declares a self-feedback read of its own output (RetroArch's
 *  real-world "PassFeedback<final pass ordinal>" vocabulary), in which
 *  case it is built at count 2 exactly like a
 *  buffer pass — see VulkanShaderInstance::build()'s own doc comment for the
 *  exact condition.
 *
 *  The ping-pong toggle arithmetic (currentReadHalf = (currentReadHalf + 1)
 *  % images.size()) holds unchanged for either shape — it degenerates to
 *  always 0 when images.size() == 1 (0 + 1 = 1, 1 % 1 == 0), so a
 *  non-feedback single target needs no separate branch, ever.
 */
struct VulkanRenderResources
{
    /** @brief Offscreen single-sample colour target(s) — a ping-pong pair
     *  for a feedback-capable buffer pass, or a single entry for a
     *  non-feedback target (the Image pass's gather target). */
    jam::Array<VulkanImage> images;

    /** @brief Framebuffers matching images[0..], built against renderPass
     *  below (VulkanGraphics::getOrCreateShaderOffscreenRenderPass (format)). */
    jam::Array<vk::Framebuffer> framebuffers {};

    /** @brief This pass's own dedicated pipeline (single-sample, built once,
     *  eagerly, against renderPass below). */
    vk::Pipeline pipeline {};

    /** @brief Colour format images[] above were created with — written once
     *  by VulkanShaderInstance::buildRenderResources(). Always this VulkanGraphics's own
     *  colorFormat parameter for the mandatory Image pass's own gather
     *  target (never preset-driven, VulkanShaderInstance::build()'s own doc
     *  comment); a buffer pass's own format instead resolves per its
     *  jam::VulkanShaderPreset::Pass srgb_framebufferN/float_framebufferN
     *  directive when present (srgb wins when both are set, RetroArch
     *  shader_vulkan.cpp:2018-2025), else falls back to that same
     *  colorFormat parameter unchanged. The SSOT every draw site reads
     *  before choosing a texture-format-dependent sampler/pipeline
     *  decision, mirroring extent's own per-pass SSOT role below. */
    vk::Format format {};

    /** @brief This pass's own single-sample offscreen render pass, keyed by
     *  format above — VulkanGraphics::getOrCreateShaderOffscreenRenderPass
     *  (format), cached and owned by VulkanGraphics's own
     *  shaderOffscreenRenderPasses map, never by this struct (non-owning,
     *  mirrors VulkanSlangPassResources::pipelineLayout's own "built and owned
     *  elsewhere, merely referenced here" precedent for a per-pass GPU
     *  handle stored for draw-time convenience, jam_VulkanSlangPassResources.h)
     *  — every framebuffers[] entry above was built against this SAME
     *  handle, and every draw site records its rpInfo.renderPass from this
     *  field directly rather than re-resolving VulkanGraphics's shared getter,
     *  keeping pipeline/framebuffer/render-pass coherent per pass. */
    vk::RenderPass renderPass {};

    /** @brief Stable bindless array slot per image — assigned + written once
     *  by VulkanGraphics::getOrCreateShaderInstance() (same convention for a buffer
     *  pass's ping-pong pair and the Image pass's own single-slot gather
     *  target alike): a buffer pass's slot is read via VulkanShaderInstance::
     *  stampChannels(), the gather target's single slot is instead stamped
     *  directly into channels[0] by the background/post-process combine draw
     *  sites (mirrors VulkanGraphics::recordStraightAlphaPass()'s own direct-stamp
     *  technique) — see jam_vulkan/shaders/background_combine.frag/
     *  post_process_combine.frag. -1 until assigned. */
    jam::Array<int> bindlessIndex;

    /** @brief Which of images[] currently holds this target's most recently
     *  completed (readable) output. Flipped by
     *  VulkanGraphics::recordShaderBufferPasses() immediately after a buffer
     *  pass's own draw completes, and equally by both Image-pass gather draw
     *  sites (VulkanLowLevelGraphicsContext::recordShaderImagePassDrawCommands(),
     *  VulkanGraphics::recordPostProcessCompositeDrawCommands()) immediately after
     *  their own gather draw completes, when the gather target is
     *  history-capable (images.size() == 2, see this struct's own doc
     *  comment); stays permanently 0 for a single-image target
     *  (images.size() == 1 — nothing ever toggles it). */
    int currentReadHalf { 0 };

    /** @brief The pixel extent images/framebuffers above were built at —
     *  written once by VulkanShaderInstance::buildRenderResources() (per the
     *  per-pass render-target extent contract). A buffer pass's own
     *  extent is resolved per its own jam::VulkanShaderPreset::Pass
     *  scale/scale_type directive (source/viewport/absolute, independent per
     *  axis) — see VulkanShaderInstance::build()'s own doc comment; the mandatory
     *  Image pass's own gather target (VulkanShaderInstance::imagePassGatherTarget)
     *  always stays at this execution's own scaledExtent (RetroArch's final
     *  pass renders to the viewport by default — this engine's resolution
     *  control IS that viewport). The SSOT every draw site
     *  (VulkanGraphics::recordSingleBufferPass()/
     *  recordPostProcessCompositeDrawCommands(), VulkanLowLevelGraphicsContext::
     *  recordShaderImagePassDrawCommands(), VulkanGraphics::
     *  getSlangTextureBindings()) reads for this target's own
     *  renderArea/viewport/scissor and VulkanShaderReflection::populate*Buffer()'s
     *  "<X>Size"/"OutputSize" source — never the shared scaledExtent
     *  directly, which a per-pass extent may now legitimately differ from. */
    vk::Extent2D extent {};

    /** @brief Mip levels images[] above were created with — written once by
     *  VulkanShaderInstance::buildRenderResources(), 1 for every target except a
     *  buffer pass whose downstream consumer pass (VulkanShaderInstance::build()'s
     *  own preset.passes[thisOrdinal + 1] lookup — the buffer pass
     *  immediately after this one, or the mandatory Image pass when this is
     *  the last buffer pass, since its own preset ordinal always equals the
     *  buffer pass count) declares mipmap_inputN — RetroArch's own directive
     *  meaning "the texture feeding INTO pass N needs mip levels", jam_
     *  VulkanShaderPreset.h's own Pass::mipmapInput doc comment. > 1 there,
     *  computed as floor(log2(max(this target's own extent.width,
     *  extent.height))) + 1 (computeMipLevelCount(), jam_VulkanShaderInstance.cpp). The
     *  mandatory Image pass's own gather target (VulkanShaderInstance::
     *  imagePassGatherTarget) always stays at 1 — it has no downstream
     *  consumer pass within this engine to declare mipmap_inputN against.
     *  VulkanGraphics::recordSingleBufferPass() gates its own post-draw
     *  VulkanGraphics::recordMipChainGeneration() call on this field being > 1. */
    uint32_t numMipLevels { 1 };

    /** @brief Depth attachment — built by VulkanShaderInstance::buildRenderResources()
     *  for EVERY VulkanRenderResources this class owns (every buffer pass's own
     *  ping-pong pair, and the mandatory Image pass's own gather target
     *  alike) — single-sample, VulkanShaderInstance::offscreenDepthFormat
     *  (eD32Sfloat, MoltenVK-universal — no other depth-format constant
     *  exists anywhere in this codebase, grep-confirmed this session), aspect
     *  eDepth, built once per target at the same extent as images[], every
     *  framebuffers[] entry's SECOND attachment — one depth image PER
     *  TARGET, SHARED by every half in images[] (never ping-ponged the way
     *  images[] itself is): depth is transient, re-cleared every draw
     *  (VulkanGraphics::getOrCreateShaderOffscreenRenderPass()'s own loadOp=eClear
     *  depth-attachment doc comment), never read back across frames, so one
     *  shared depth image serves whichever half is currently being written.
     *  Every buffer-pass target's own pipeline (VulkanShaderInstance::
     *  createFullscreenPipeline(), VulkanPipelines::noStencilState()) leaves depth
     *  test/write disabled — this attachment is inert scratch space for a
     *  meshless target, present only because every framebuffer built
     *  against the one shared offscreen render pass shape must supply both
     *  attachments (that render pass's own doc comment). The mandatory VulkanImage
     *  pass's own gather target (imagePassGatherTarget) is where a
     *  mesh-carrying execution's own material-range/feature-edge draws
     *  (VulkanGraphics::recordMeshGatherDrawCommands()) actually test/write against
     *  this SAME attachment, directly after the ordinary fullscreen backdrop
     *  draw, in the SAME render-pass instance — never a separate mesh-only
     *  render target. Never bindless-registered — depth is never sampled
     *  outside its own render pass's own depth test. */
    VulkanImage depthImage {};
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam