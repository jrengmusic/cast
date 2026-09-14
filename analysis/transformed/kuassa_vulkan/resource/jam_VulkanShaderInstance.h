/** @file jam_VulkanShaderInstance.h
 *  @brief Per-content-hash GPU execution resources for one jam::VulkanShader
 *         instance — offscreen render targets/pipelines for every buffer
 *         pass, and the mandatory Image pass's own offscreen gather target.
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief Owns every GPU-side resource needed to execute one jam::VulkanShader
 *  instance at one compiled contentHash — offscreen render targets/pipelines
 *  for every buffer pass (ping-pong pair per buffer pass — every buffer pass
 *  is unconditionally feedback-capable, VulkanShaderPass's own doc comment) and
 *  the mandatory Image pass's own offscreen gather target (a single,
 *  non-feedback VulkanRenderResources, imagePassGatherTarget) — the Image pass's
 *  draw renders its raw, unmodified colour into this offscreen target
 *  instead of drawing directly into the scene/composite render pass; the
 *  VulkanGraphics-owned background/post-process combine pipelines
 *  (VulkanGraphics::getOrCreateBackgroundCombinePipeline()/
 *  getOrCreatePostProcessCombinePipeline()) then sample it through its own
 *  stable bindless slot (channels[0], stamped directly at each combine draw
 *  site — see jam_vulkan/shaders/background_combine.frag/
 *  post_process_combine.frag).
 *
 *  Built once, eagerly, by build() — never rebuilt in place. VulkanGraphics
 *  constructs a FRESH VulkanShaderInstance whenever a VulkanShader's contentHash or scaled
 *  extent changes, moving the old instance into VulkanShaderRegistry::previousShaderInstances
 *  via the established deferred-destroy pattern rather than
 *  mutating this one (see VulkanGraphics::getOrCreateShaderInstance(),
 *  jam_VulkanGraphicsSlangPass.cpp).
 *
 *  No Vulkan handles are shared with the main 21-pipeline VulkanPipelines
 *  collection except the bindless texture array descriptor set (reused
 *  verbatim, unchanged) — every VulkanShader-execution pipeline is built against
 *  its own dedicated pipeline layout (VulkanGraphics::getOrCreateShaderInstanceLayout()).
 *  Resample mode selection is resolved by jam::VulkanShaderCompiler at GLSL-generation time
 *  (baked directly into each pass's compiled SPIR-V before a VulkanShader is ever
 *  constructed — VulkanShader itself carries no resample-mode field) and sampled directly
 *  through that reused set's binding 1 (linear) or binding 2 (nearest) — see
 *  jam_VulkanShaderUniforms.h's documented GLSL contract — so this execution
 *  owns no sampler descriptor set of its own.
 *
 *  Also owns every external LUT/texture VulkanBindlessTexture this VulkanShader's own
 *  preset.textures declares (externalTextures, keyed by name) plus each
 *  entry's own already-resolved push-constant channels[] slot
 *  (externalTextureChannels). Unlike
 *  bufferPassTargets/imagePassGatherTarget above, this class's own build()
 *  creates neither: VulkanGraphics::getOrCreateShaderInstance()'s own post-build
 *  orchestration builds/uploads them directly (via VulkanGraphics::
 *  buildExternalTexture()), since only that orchestrator has the staging
 *  arena/command buffer/bindless-slot allocator the actual pixel upload and
 *  slot assignment need — see getExternalTextures()'s own doc comment.
 *
 *  Not copyable, not movable — holds a VulkanDevice& reference, same pattern as
 *  VulkanTransparencyStack/VulkanWindingScratch.
 */
class VulkanShaderInstance
{
public:
    //==========================================================================
    // Constants
    //==========================================================================

    /** @brief One vertex of the static slang fullscreen quad — matches
     *  RetroArch's fixed, mandatory .slang vertex-stage attributes exactly
     *  ("I/O interface variables", confirmed
     *  against a real corpus file, ~/Documents/Poems/dev/slang-shaders/
     *  stock.slang:17-18): position is layout(location = 0) in vec4 Position,
     *  texCoord is layout(location = 1) in vec2 TexCoord. Tightly packed (24
     *  bytes, no compiler-inserted padding — float[4] then float[2], both
     *  naturally 4-byte aligned), matching createFullscreenPipeline()'s own
     *  vk::VertexInputBindingDescription stride for a slang pass. */
    struct SlangQuadVertex
    {
        float position[4];
        float texCoord[2];
    };

    static_assert (sizeof (SlangQuadVertex) == 24,
                  "SlangQuadVertex must be 24 bytes (matches createFullscreenPipeline()'s own vk::VertexInputBindingDescription stride)");

    /** @brief The 4 vertices of the static slang fullscreen quad, drawn as
     *  vk::PrimitiveTopology::eTriangleStrip (BL, BR, TL, TR — standard
     *  quad-strip winding, 2 triangles) — VulkanGraphics::slangQuadVertexBuffer's
     *  entire content (VulkanGraphics::createDrawState()), uploaded exactly once
     *  and never rewritten afterward: this quad is identical for every
     *  slang-format VulkanShaderPass in every VulkanShader a VulkanGraphics ever executes, so
     *  one buffer serves them all (mirrors VulkanGraphics::shaderPassVertModule's
     *  own "one shared module regardless of how many Shaders are active"
     *  precedent).
     *
     *  Matches shader_pass.vert's own NDC -> UV mapping exactly (uv = pos *
     *  0.5 + 0.5, shaders/shader_pass.vert:15) so a slang pass renders with
     *  the same orientation as a Shadertoy pass: Position(-1,-1) ->
     *  TexCoord(0,0) (top-left per RetroArch's own TexCoord
     *  semantics), Position(1,1) -> TexCoord(1,1) (bottom-right). */
    static constexpr std::array<SlangQuadVertex, 4> slangQuadVertices {{
        { { -1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
        { {  1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
        { { -1.0f,  1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
        { {  1.0f,  1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }
    }};

    /** @brief VulkanVertex count in slangQuadVertices — SSOT for every slang-pass
     *  draw call (VulkanGraphics::recordSingleBufferPass()/
     *  recordPostProcessCompositeDrawCommands(), VulkanLowLevelGraphicsContext::
     *  recordShaderImagePassDrawCommands()), mirroring
     *  VulkanPipelines::fullscreenTriangleVertexCount's identical role for the Shadertoy
     *  fullscreen-triangle path. */
    static constexpr uint32_t slangQuadVertexCount { static_cast<uint32_t> (slangQuadVertices.size()) };

    /** @brief Depth format for every VulkanRenderResources::depthImage this class
     *  builds — eD32Sfloat, MoltenVK-universal (no other depth-format
     *  constant exists anywhere in this codebase, grep-confirmed this
     *  session; every existing scene/composite render pass —
     *  jam_VulkanGraphicsSetupRenderPass.cpp's 3 main passes — is
     *  stencil-only, eS8Uint, no depth attachment anywhere). Shared by
     *  VulkanGraphics::getOrCreateShaderOffscreenRenderPass (vk::Format) (this SAME
     *  format's unconditional depth-attachment description) and
     *  buildRenderResources() below (this SAME format's per-target depth
     *  VulkanImage::create2D() call) — one constant, one SSOT, mirroring
     *  VulkanPipelines::fullscreenTriangleVertexCount's own cross-class shared-constant
     *  precedent immediately above. */
    static constexpr vk::Format offscreenDepthFormat { vk::Format::eD32Sfloat };

    /** @brief Per-instance MVP + normal-matrix + standard-uniforms +
     *  viewport/edge-thickness uniform block for the mesh-backed gather
     *  target's own descriptor set binding 0 (mesh_default.vert's own
     *  vertex-stage-only UBO — mirrors the existing mvp.glsl's single-mat4
     *  UBO shape, extended with a second mat4 for the normal matrix,
     *  jam::VulkanOrbitCamera::getNormalMatrix()'s own return type).
     *
     *  iMouse/iResolution/iTime/iTimeDelta/iFrame are the SAME standard
     *  uniforms every other shader pass reads (jam::VulkanShaderUniforms)
     *  — delivered here, through this UBO, rather than a push constant (the
     *  mesh pipeline layout's own push-constant range already carries the
     *  fragment-stage MeshMaterialPushConstants below, and VulkanShaderUniforms is
     *  a full 128-byte block — too large to also fit) — bare-global-promoted
     *  by mesh_default.vert/mesh_edge.vert's own UNNAMED block instance
     *  (mirrors shadertoy_wrapper.frag's own anonymous ShaderPC convention),
     *  so an author's mesh_shader= mainMesh snippet reads iTime/etc. exactly
     *  like any other shader pass — zero bespoke uniform vocabulary.
     *  viewportSize/edgeThicknessPixels are read ONLY by mesh_edge.vert's own
     *  screen-space thick-line quad expansion (jam_vulkan/shader/
     *  mesh_edge.vert's own doc comment) — mesh_default.vert's identically-
     *  named block only ever declares the PREFIX up to iFrame and never
     *  reads these trailing bytes, which is safe (a GLSL uniform block only
     *  cares about the members IT declares; both shaders bind the SAME
     *  underlying buffer).
     *
     *  Member order is chosen so every field lands on its natural std140
     *  alignment with zero implicit padding, mirroring jam::
     *  VulkanShaderUniforms' own identical "member order chosen for the byte
     *  layout, not for reading order" doc comment (jam_VulkanShaderUniforms.h):
     *  mvp@0 (64B) / normalMatrix@64 (64B) / iMouse@128 (16B, 16-aligned) /
     *  iResolution@144 (8B, 8-aligned) / iTime@152 (4B) / iTimeDelta@156 (4B)
     *  / iFrame@160 (4B) / edgeThicknessPixels@164 (4B) / viewportSize@168
     *  (8B, 8-aligned) — 176 bytes total, no gaps either side; both
     *  mesh_default.vert/mesh_edge.vert's own declaration order must keep
     *  matching this exactly. Refreshed once per frame by
     *  refreshMeshUniforms() — same "write descriptor once at build, refresh
     *  only the mapped bytes every frame" split as VulkanSlangPassResources::
     *  uniformBuffer (jam_VulkanSlangPassResources.h). */
    struct MeshUniforms
    {
        glm::mat4 mvp;
        glm::mat4 normalMatrix;
        glm::vec4 iMouse;
        glm::vec2 iResolution;
        float iTime;
        float iTimeDelta;
        int32_t iFrame;
        float edgeThicknessPixels;
        glm::vec2 viewportSize;
    };

    /** @brief Per-MaterialRange push-constant payload for the mesh-backed
     *  gather target's own dedicated pipeline layout (VulkanGraphics::
     *  getOrCreateMeshPipelineLayout()) — mesh_default.frag's own material
     *  diffuse colour, pushed fresh before each jam::VulkanMesh::
     *  MaterialRange's own vkCmdDrawIndexed call ("simplest: the material's
     *  diffuse as a push constant per drawIndexed range"). Plain float[4]
     *  (not VulkanColour/glm::vec4) — the exact
     *  byte layout vkCmdPushConstants copies verbatim, no GLSL-alignment
     *  surprises to reason about (mirrors jam::VulkanVertex's own
     *  tightly-packed-floats convention, resource/jam_VulkanVertex.h). */
    struct MeshMaterialPushConstants
    {
        float diffuse[4];
    };

    //==========================================================================
    // Constructor / Destructor
    //==========================================================================

    /** @brief Stores the device reference. Owns no Vulkan handles until build().
     *  @param device  Shared Vulkan device — not owned, must outlive this VulkanShaderInstance.
     */
    explicit VulkanShaderInstance (VulkanDevice& device);

    /** @brief Destroys every owned Vulkan handle (pipelines, framebuffers);
     *  RAII Images self-destruct on member destruction. */
    ~VulkanShaderInstance();

    //==========================================================================
    // Build
    //==========================================================================

    /** @brief Computes the extent every intermediate pass renders at — the one
     *  place resolutionScale is applied (SSOT). Never below 1x1.
     *  @param sceneExtent      Full-resolution scene/swapchain extent.
     *  @param resolutionScale  Intermediate-pass extent fraction, [0, 1] —
     *                          a build()-time caller parameter (not a VulkanShader
     *                          field, jam_VulkanShader.h's doc comment).
     */
    static vk::Extent2D computeScaledExtent (vk::Extent2D sceneExtent, float resolutionScale) noexcept;

    /** @brief Builds every per-pass GPU resource for shader at
     *  computeScaledExtent(sceneExtent, resolutionScale). Called exactly once
     *  per instance, immediately after construction — VulkanGraphics constructs a
     *  fresh VulkanShaderInstance rather than calling this twice on the same
     *  instance (see class doc comment).
     *
     *  When shader.format == VulkanShaderFormat::slang, runs
     *  buildSlangPassResources() FIRST (reflects every pass, builds each
     *  pass's own descriptor set/UBO/dedicated pipeline layout) so the
     *  per-pass buildRenderResources() loop below can build each pass's
     *  pipeline against ITS OWN pipeline layout instead of the shared
     *  @p pipelineLayout — see buildRenderResources()'s own passPipelineLayout
     *  doc comment. Immediately after, still slang-only, scans every pass's
     *  own freshly-reflected texture list for RetroArch's OriginalHistoryN
     *  vocabulary (VulkanShaderReflection::getOriginalHistoryPrefix()) and builds
     *  originalHistoryImages sized to the deepest ordinal found + 1 (see its
     *  own doc comment) — this ring buffer's own build() step, unlike every
     *  other resource here, is keyed off every pass's reflection at once
     *  rather than one pass at a time. A VulkanShaderFormat::shadertoy shader
     *  takes neither branch — every pass continues building against the
     *  shared @p pipelineLayout exactly as before slang support existed, and
     *  originalHistoryImages stays empty.
     *
     *  Per-buffer-pass mip levels (jam::VulkanShaderPreset::Pass::
     *  mipmapInput, RetroArch's own mipmap_inputN — declared on the
     *  CONSUMER of the texture that needs mips, not on its producer):
     *  buffer pass k's own VulkanRenderResources::numMipLevels is > 1 exactly
     *  when shader.preset.passes[k + 1].mipmapInput is true (that ordinal is
     *  always this pass's own consumer — the next buffer pass, or the
     *  mandatory Image pass when k is the last buffer pass, since the VulkanImage
     *  pass's own preset ordinal always equals the buffer pass count), in
     *  which case it is computeMipLevelCount (that pass's own extent);
     *  otherwise 1. Ordinal 0's own mipmapInput is never consulted this way
     *  (this lookup only ever reads ordinals >= 1) — mipmap_input0 would
     *  mean pass 0's own INPUT needs mips, but pass 0's input is this
     *  execution's own external chain input (background: none; post-process:
     *  the engine-global straight-alpha scene image, which predates any
     *  VulkanShader and carries no mip infrastructure at all) — a graceful
     *  base-level fallback, not a bug. The mandatory Image pass's own gather
     *  target (imagePassGatherTarget) always stays at numMipLevels 1 — no
     *  pass past it exists in this engine to declare mipmap_inputN against.
     *  @param shader                   The compiled shader this execution serves.
     *  @param resolutionScale          Intermediate-pass extent fraction, [0, 1] —
     *                                  caller-supplied (not a VulkanShader field).
     *  @param colorFormat              Fallback format for a buffer pass whose
     *                                  jam::VulkanShaderPreset::Pass declares
     *                                  neither srgb_framebufferN nor
     *                                  float_framebufferN (matches the scene
     *                                  resolve target's format), and the
     *                                  UNCONDITIONAL format for the mandatory
     *                                  Image pass's own gather target (never
     *                                  preset-resolved — RetroArch's own final
     *                                  pass always renders to the viewport at
     *                                  the engine's own format, shader_vulkan.cpp's
     *                                  Pass::build() !final_pass framebuffer
     *                                  guard). A buffer
     *                                  pass whose own preset entry sets
     *                                  srgbFramebuffer resolves to
     *                                  vk::Format::eR8G8B8A8Srgb instead (wins
     *                                  when floatFramebuffer is ALSO set,
     *                                  RetroArch shader_vulkan.cpp:2018-2025,
     *                                  if/else-if, srgb checked first), or
     *                                  floatFramebuffer alone resolves to
     *                                  vk::Format::eR16G16B16A16Sfloat — see
     *                                  this method's own per-pass loop
     *                                  (jam_VulkanShaderInstance.cpp).
     *  @param sceneExtent              Full-resolution scene extent.
     *  @param offscreenRenderPassFactory  Resolves a render-target format to
     *                                  its own single-sample offscreen render
     *                                  pass (VulkanGraphics::
     *                                  getOrCreateShaderOffscreenRenderPass
     *                                  (vk::Format), threaded in as a callable
     *                                  rather than one fixed handle since a
     *                                  buffer pass's own srgb_framebufferN/
     *                                  float_framebufferN directive above may
     *                                  resolve to a DIFFERENT format than
     *                                  @p colorFormat, each format needing its
     *                                  own dedicated render pass — every
     *                                  buildRenderResources() call below
     *                                  resolves its own target's format
     *                                  through this SAME factory before
     *                                  building, so VulkanRenderResources::renderPass
     *                                  always matches VulkanRenderResources::format).
     *  @param commandBuffer            The active frame's command buffer — needed
     *                                  ONLY when shader.meshPath is non-empty
     *                                  (jam::VulkanMesh::build()'s one-time
     *                                  staged upload); every other resource this
     *                                  method builds needs no command buffer at
     *                                  all (mirrors VulkanGraphics::buildExternalTexture()'s
     *                                  identical need for its own caller-supplied
     *                                  command buffer).
     *  @param meshDescriptorSetLayout  The mesh-backed material-range/
     *                                  feature-edge draw's own descriptor set
     *                                  layout (binding 0 = MVP+normal-matrix
     *                                  UBO, binding 1 = vertex-pulling SSBO)
     *                                  — VulkanGraphics::getOrCreateMeshDescriptorSetLayout(),
     *                                  engine-owned (built once per VulkanGraphics,
     *                                  shared across every mesh-carrying
     *                                  VulkanShaderInstance, mirrors
     *                                  pipelineLayout/sharedVertModule below).
     *                                  Unused when shader.meshPath is empty.
     *  @param meshPipelineLayout       The engine-owned mesh pipeline layout
     *                                  (set 0 = @p meshDescriptorSetLayout,
     *                                  plus the fragment-stage
     *                                  MeshMaterialPushConstants push range)
     *                                  — VulkanGraphics::getOrCreateMeshPipelineLayout(),
     *                                  the SAME layout the engine's own
     *                                  default mesh pipelines are built
     *                                  against, reused verbatim for this
     *                                  execution's own hooked pipelines when
     *                                  shader.meshShaderSource is non-empty
     *                                  (buildMeshHookPipelines()) — so
     *                                  recordMeshGatherSetup()/
     *                                  recordMeshMaterialRangeDraws() and the
     *                                  fragment material push constants work
     *                                  unchanged whether this execution binds
     *                                  the engine's own default mesh
     *                                  pipelines or this execution's own
     *                                  hooked ones. Unused when
     *                                  shader.meshShaderSource is empty (no
     *                                  mesh_shader= connection).
     *  @param pipelineLayout           Shared pipeline layout every pass pipeline
     *                                  is built against
     *                                  (VulkanGraphics::getOrCreateShaderInstanceLayout()).
     *  @param sharedVertModule         The one shared fullscreen-triangle vertex
     *                                  module every pass pipeline binds
     *                                  (VulkanGraphics::getOrCreateShaderPassVertModule()).
     *  @param chainInputExtent         The basis extent for pass 0's own
     *                                  "source"-typed scale directive
     *                                  (jam::VulkanShaderPreset::Pass::
     *                                  scaleTypeX/Y == source resolves
     *                                  against the PREVIOUS pass's own
     *                                  extent in the chain — pass 0 has no
     *                                  previous pass, so its own basis is
     *                                  this caller-supplied virtual input
     *                                  instead) — VulkanGraphics::
     *                                  getOrCreateShaderInstance()'s own two call
     *                                  sites each supply their own basis: the
     *                                  in-scene background path
     *                                  (VulkanLowLevelGraphicsContext::
     *                                  renderShader()) has no scene, so it
     *                                  passes this shader's own scaledExtent
     *                                  (computeScaledExtent (sceneExtent,
     *                                  resolutionScale)); the post-process
     *                                  composite (VulkanGraphics::endFrame())
     *                                  passes the actual straight-alpha scene
     *                                  extent (swapchainExtent). Every
     *                                  buffer pass past pass 0 resolves its
     *                                  own "source" basis from the PREVIOUS
     *                                  buffer pass's own already-computed
     *                                  extent instead — this parameter feeds
     *                                  only pass 0. Unused whenever this
     *                                  execution's preset.passes has no
     *                                  entry for pass 0 at all (every
     *                                  VulkanShaderFormat::shadertoy shader, whose
     *                                  preset is always empty) — that pass
     *                                  falls back to this shader's own
     *                                  scaledExtent instead (today's exact,
     *                                  byte-identical behavior for every
     *                                  existing project — see
     *                                  computePassExtent()'s own doc
     *                                  comment, jam_VulkanShaderInstance.cpp).
     *  @return true if every resource built successfully.
     */
    bool build (const VulkanShader& shader, float resolutionScale, vk::Format colorFormat, vk::Extent2D sceneExtent,
               const std::function<vk::RenderPass (vk::Format)>& offscreenRenderPassFactory,
               vk::CommandBuffer commandBuffer, vk::DescriptorSetLayout meshDescriptorSetLayout,
               vk::PipelineLayout meshPipelineLayout, vk::PipelineLayout pipelineLayout,
               vk::ShaderModule sharedVertModule, vk::Extent2D chainInputExtent);

    //==========================================================================
    // Accessors
    //==========================================================================

    /** @brief Returns true once build() has completed successfully. */
    bool isReady() const noexcept { return ready; }

    /** @brief Returns the content hash this execution was built for
     *  (VulkanShader::contentHash at build() time). */
    uint64_t getContentHash() const noexcept { return contentHash; }

    /** @brief Returns the scaled extent this execution's buffer-pass targets
     *  were built at — VulkanGraphics::getOrCreateShaderInstance() compares this against
     *  a fresh computeScaledExtent() call to detect a stale build (resize). */
    vk::Extent2D getBuiltExtent() const noexcept { return builtExtent; }

    /** @brief Returns the number of buffer passes (excludes the mandatory Image pass). */
    int getBufferPassCount() const noexcept { return static_cast<int> (bufferPassTargets.size()); }

    /** @brief Returns the mandatory Image pass's own zero-based ordinal into
     *  shader.passes/slangPasses — always equal to getBufferPassCount(),
     *  since every buffer pass precedes it. Named accessor for readability
     *  at slang draw-call sites (VulkanGraphics::recordSingleBufferPass()/
     *  recordPostProcessCompositeDrawCommands(), VulkanLowLevelGraphicsContext::
     *  recordShaderImagePassDrawCommands()) rather than a repeated
     *  getBufferPassCount() call whose meaning ("the Image pass's own
     *  ordinal") is otherwise implicit. */
    int getImagePassIndex() const noexcept { return getBufferPassCount(); }

    /** @brief Returns the buffer pass target at index (BufferA-D order).
     *  @param index  Zero-based BufferA-D index — must be < getBufferPassCount().
     */
    VulkanRenderResources& getBufferPassTarget (int index) noexcept
    {
        return *bufferPassTargets.at (static_cast<size_t> (index));
    }

    /** @brief Returns the mandatory Image pass's own offscreen gather
     *  target — its single framebuffer/pipeline, targeted instead of the
     *  scene/composite render pass directly by
     *  VulkanLowLevelGraphicsContext::recordShaderImagePassDrawCommands() and
     *  VulkanGraphics::recordPostProcessCompositeDrawCommands() (see this class's
     *  own doc comment and imagePassGatherTarget's). */
    VulkanRenderResources& getImagePassGatherTarget() noexcept { return imagePassGatherTarget; }

    //==========================================================================
    // VulkanMesh-backed material-range draw
    //==========================================================================

    /** @brief Returns true once build() has successfully parsed+uploaded
     *  shader.meshPath's own OBJ and built every mesh GPU resource below —
     *  false for every shader with no mesh connection (shader.meshPath
     *  empty) AND for a mesh connection whose parse/upload failed (graceful
     *  last-good: an unreadable/malformed OBJ logs via jam::debug::Log and
     *  leaves this execution rendering its ordinary, unmodified Image pass
     *  into imagePassGatherTarget exactly as before, with no mesh draw
     *  appended). Every mesh draw-site call (VulkanGraphics::
     *  recordMeshGatherDrawCommands()) is gated on this. */
    bool hasMesh() const noexcept { return meshReady; }

    /** @brief Returns this execution's own uploaded GPU mesh (vertex-pulling
     *  SSBO + index buffer + per-shape MaterialRange list) — only valid when
     *  hasMesh() is true. */
    VulkanMesh& getMesh() noexcept { return mesh; }

    /** @brief Returns true once build() has successfully built this
     *  execution's own hooked mesh pipelines (meshHookFillPipeline/
     *  meshHookTransparentFillPipeline/meshHookEdgePipeline,
     *  buildMeshHookPipelines()) — false whenever hasMesh() is false, this
     *  VulkanShader declared no mesh_shader= connection (VulkanShader::meshShaderSource
     *  empty), or the hooked vertex-stage compile/pipeline build failed
     *  (graceful last-good, mirrors hasMesh()'s own parse-failure tolerance —
     *  see build()'s failure contract, jam_VulkanShaderInstance.cpp).
     *  VulkanGraphics::recordMeshMaterialRangeDraws() branches on this per
     *  MaterialRange/feature-edge draw: true binds this execution's own
     *  hooked pipeline (mesh_shader= is a vertex-ANIMATION HOOK
     *  into the engine's own default mesh look — same rendering contract,
     *  animated position/normal); false binds the engine's own default,
     *  unanimated pipeline instead. */
    bool hasMeshHook() const noexcept { return meshHookFillPipeline != nullptr; }

    /** @brief Returns this execution's own hooked fill pipeline (real-
     *  material MaterialRange draw, opaque blend) — only valid when
     *  hasMeshHook() is true. */
    vk::Pipeline getMeshHookFillPipeline() const noexcept { return meshHookFillPipeline; }

    /** @brief Returns this execution's own hooked transparent-fill pipeline
     *  (default-material MaterialRange draw, alpha blend) — only valid when
     *  hasMeshHook() is true. */
    vk::Pipeline getMeshHookTransparentFillPipeline() const noexcept { return meshHookTransparentFillPipeline; }

    /** @brief Returns this execution's own hooked feature-edge overlay
     *  pipeline — only valid when hasMeshHook() is true. */
    vk::Pipeline getMeshHookEdgePipeline() const noexcept { return meshHookEdgePipeline; }

    /** @brief Returns the auto-fit MODEL matrix computed once, at build()
     *  time, from mesh's own AABB (jam::VulkanOrbitCamera::
     *  computeAutoFitModelMatrix()) — see that method's own doc comment for
     *  the full seam rationale (VulkanOrbitCamera.h's own class doc comment).
     *  Identity when hasMesh() is false. */
    const glm::mat4& getMeshModelMatrix() const noexcept { return meshModelMatrix; }

    /** @brief Returns this execution's own mesh descriptor set — binding 0 =
     *  MVP+normal-matrix+viewport/edge-thickness UBO (refreshMeshUniforms()),
     *  binding 1 = this execution's own mesh vertex-pulling SSBO (written
     *  ONCE at build time, mesh.getVertexBuffer()'s handle never changes
     *  afterward), binding 2 = this execution's own mesh feature-edge
     *  endpoint-index SSBO (mesh.getFeatureEdgeIndexBuffer(), written ONCE at
     *  build time, only when mesh.getNumFeatureEdgeIndices() > 0 —
     *  buildMeshGpuResources()'s own doc comment) — allocated against
     *  VulkanGraphics::getOrCreateMeshDescriptorSetLayout(), the SAME layout the
     *  engine-owned mesh pipeline's own pipelineLayout (VulkanGraphics::
     *  getOrCreateMeshPipelineLayout()) declares at set 0. Only valid when
     *  hasMesh() is true. */
    vk::DescriptorSet getMeshDescriptorSet() const noexcept { return meshDescriptorSet; }

    /** @brief Refreshes this execution's own mesh uniform buffer's mapped
     *  bytes (MeshUniforms — see its own doc comment for the exact field
     *  list/byte layout) — called once per frame, immediately before
     *  VulkanGraphics::recordMeshGatherDrawCommands()'s own draw, mirroring
     *  refreshSlangPass()'s identical "descriptor written once at build, only
     *  the mapped bytes refreshed per frame" split. @p mvp is the caller's own
     *  projection * view * getMeshModelMatrix() product (this execution owns
     *  no camera of its own — jam::VulkanShaderComponent owns the
     *  interactive jam::VulkanOrbitCamera, VulkanOrbitCamera.h's own class doc
     *  comment); @p normalMatrix is that SAME camera's own VulkanOrbitCamera::
     *  getNormalMatrix() (view-only inverse-transpose — that method's own doc
     *  comment for why no model term is needed); @p viewportExtent is the
     *  mandatory Image pass's own gather target pixel extent (VulkanGraphics::
     *  recordMeshGatherSetup()'s own execution.getImagePassGatherTarget().extent,
     *  read ONLY by mesh_edge.vert's own screen-space quad expansion); @p
     *  edgeThicknessPixels is VulkanGraphics::recordMeshGatherSetup()'s own
     *  featureEdgeThicknessPixels named constant, the single tuning point for
     *  the feature-edge overlay's own pixel thickness; @p imageUniforms is the
     *  SAME VulkanShaderUniforms the caller's own ordinary Image-pass fullscreen
     *  backdrop draw just pushed (VulkanLowLevelGraphicsContext::
     *  recordShaderImagePassDrawCommands()) — its iMouse/iResolution/iTime/
     *  iTimeDelta/iFrame fields are copied verbatim into this UBO's own
     *  identically-named members, the standard-uniform delivery mechanism a
     *  mesh_shader= mainMesh snippet reads bare (MeshUniforms' own doc
     *  comment) — zero bespoke uniform vocabulary.
     *  @param mvp               This frame's model-view-projection matrix.
     *  @param normalMatrix      This frame's normal matrix.
     *  @param viewportExtent    The mandatory Image pass's own gather target pixel extent.
     *  @param edgeThicknessPixels  The feature-edge overlay's own pixel thickness.
     *  @param imageUniforms     The SAME VulkanShaderUniforms already stamped/pushed for
     *                           this call's own ordinary Image-pass fullscreen draw.
     */
    void refreshMeshUniforms (const glm::mat4& mvp, const glm::mat4& normalMatrix,
                              vk::Extent2D viewportExtent, float edgeThicknessPixels,
                              const VulkanShaderUniforms& imageUniforms) const noexcept
    {
        const MeshUniforms uniforms { mvp, normalMatrix,
            { imageUniforms.iMouse[0], imageUniforms.iMouse[1], imageUniforms.iMouse[2], imageUniforms.iMouse[3] },
            { imageUniforms.iResolution[0], imageUniforms.iResolution[1] },
            imageUniforms.iTime, imageUniforms.iTimeDelta, imageUniforms.iFrame,
            edgeThicknessPixels,
            { static_cast<float> (viewportExtent.width), static_cast<float> (viewportExtent.height) } };
        std::memcpy (meshUniformBuffer.getMapped(), &uniforms, sizeof (uniforms));
    }

    /** @brief Releases this execution's own mesh staging VulkanBuffer
     *  (mesh.releaseStaging()) if build() flagged one pending — called once
     *  per frame by VulkanGraphics::resetResources(), the SAME post-fence-wait
     *  sweep that drains VulkanStagingArena's previousStagingBuffers and
     *  VulkanGraphics's own previousShaderInstances
     *  (jam_VulkanGraphics.h's own resetResources() doc comment) — reuses the existing post-fence release seam to wire
     *  staging release in. A no-op (mesh.releaseStaging() is itself idempotent)
     *  for every VulkanShaderInstance whose mesh staging was already released, or
     *  which never built a mesh at all — VulkanGraphics::resetResources() sweeps
     *  every LIVE shaderInstances entry unconditionally, so this must be
     *  cheap and harmless for the overwhelming majority (meshless) case. */
    void releaseMeshStagingIfPending() noexcept
    {
        if (meshStagingPendingRelease)
        {
            mesh.releaseStaging();
            meshStagingPendingRelease = false;
        }
    }

    /** @brief Returns the external LUT/texture BindlessTextures this
     *  execution owns, keyed by their own author-assigned manifest/
     *  textures= name — see externalTextures' own doc comment. Built (or
     *  left empty on decode failure) by VulkanGraphics::getOrCreateShaderInstance()'s
     *  own post-build orchestration, via VulkanGraphics::buildExternalTexture(),
     *  never by this class's own build() (which creates no GPU-uploaded
     *  resource of its own — see this class's own doc comment); this is
     *  therefore a mutable accessor, unlike getBufferPassTarget()'s
     *  read-and-draw shape, mirroring getOriginalHistoryImages()'s identical
     *  "orchestrator populates this execution's own resource collection
     *  directly" convention. Also this method's own consumer for reading:
     *  VulkanGraphics::getSlangTextureBindings()'s named-LUT lookup. */
    jam::HashMap<juce::String, VulkanBindlessTexture>& getExternalTextures() noexcept { return externalTextures; }

    /** @brief Returns this pass's current readable bindless index — the slot
     *  the Image pass or a later buffer pass should sample for channels[channelIndex].
     *  @param channelIndex  Zero-based buffer-pass ordinal; -1 if out of range
     *                       for this shader's pass count.
     */
    int getChannelBindlessIndex (int channelIndex) const noexcept;

    /** @brief Stamps every uniforms.channels[] slot (SSOT) — shared by every
     *  draw site that pushes a VulkanShaderUniforms for this execution (buffer
     *  passes, the in-scene Image pass, the post-process composite): ordinals
     *  within this execution's buffer-pass count read
     *  getChannelBindlessIndex(ordinal), every ordinal beyond it (up to
     *  VulkanShaderUniforms::maxChannelCount) falls back to
     *  sceneOrNoSceneFallback. Every named external LUT/texture
     *  (externalTextureChannels — this execution's own copy of shader.
     *  preset.textures' own declaration-order-assigned channel slot, built
     *  at build() time) then overwrites its own slot with its
     *  VulkanBindlessTexture's @p windowHandle-registered bindless index
     *  (externalTextures, VulkanBindlessTexture::getBindlessIndex()) — a texture
     *  whose decode/upload/slot-assignment failed (getBindlessIndex()
     *  returns -1) is simply skipped, leaving that slot at whichever value
     *  the buffer-pass/fallback loop above already stamped it to (graceful
     *  degradation, same tolerance every other bindless-index consumer in
     *  this class already relies on). Only ever reached for a
     *  VulkanShaderFormat::shadertoy execution — every call site gates this
     *  behind the same usesSlangPipeline() branch getSlangTextureBindings()
     *  is reserved for instead (VulkanGraphics::recordSingleBufferPass()'s own doc
     *  comment).
     *  @param uniforms                Uniforms block to stamp channels[] into.
     *  @param sceneOrNoSceneFallback  Bindless index of the resolved scene texture
     *                                 (post-process composite path,
     *                                 VulkanGraphics::recordPostProcessCompositeDrawCommands())
     *                                 or VulkanShaderUniforms::noScene (every other
     *                                 path) — the same value threaded through
     *                                 VulkanGraphics::recordShaderBufferPasses()'s
     *                                 sceneBindlessIndex parameter.
     *  @param windowHandle            The calling VulkanGraphics's own native window
     *                                 handle (VulkanGraphics::getNativeHandle()) —
     *                                 a named LUT's own VulkanBindlessTexture is
     *                                 never shared across windows the way the
     *                                 glyph atlas's is (VulkanGraphics::
     *                                 buildExternalTexture()'s own doc
     *                                 comment), but its registry is still
     *                                 keyed the same way.
     */
    void stampChannels (VulkanShaderUniforms& uniforms, int32_t sceneOrNoSceneFallback, void* windowHandle) const noexcept;

    /** @brief Stamps this call's iResolution/iTime/iTimeDelta/iFrame/iMouse —
     *  the single write site for these fields (SSOT); channels[]/opacity are
     *  left at their defaults — the caller fills those per pass via
     *  stampChannels() before pushing.
     *
     *  iMouse is copied verbatim from @p mouse — this execution tracks no
     *  mouse state of its own (BLESSED Stateless: a pure per-call parameter,
     *  supplied fresh by the caller every stampUniforms() call, never stored
     *  as a member here). Shadertoy's own sign-encoding convention applies:
     *  .xy is the current mouse position while a button is held (frozen at
     *  its last value once released); abs(.zw) is the click-start position;
     *  sign(.z) > 0 while a button is held (negative once released); .w > 0
     *  only on the exact frame the click started (negative every frame
     *  after). All four components are zero when no interaction has
     *  occurred yet.
     *  @param targetExtent  Pixel extent of the pass this stamp will be pushed for.
     *  @param mouse         This call's iMouse value, sign-encoded per the
     *                       convention above — the caller's own per-frame
     *                       mouse-capture state (Shadertoy semantics),
     *                       stamped verbatim into the returned uniforms'
     *                       iMouse.
     */
    VulkanShaderUniforms stampUniforms (vk::Extent2D targetExtent, const std::array<float, 4>& mouse) noexcept;

    /** @brief Returns the VulkanGraphics::frameCounter value stamped by the most
     *  recent setLastUsedFrame() call — VulkanGraphics::beginFrame()'s orphan sweep
     *  compares this against the current frameCounter to find a live
     *  shaderInstances entry whose owning VulkanShader was destroyed (its owner
     *  replaced the unique_ptr<VulkanShader>) and so is never visited by
     *  VulkanGraphics::getOrCreateShaderInstance() again. */
    uint64_t getLastUsedFrame() const noexcept { return lastUsedFrame; }

    /** @brief Stamps this execution as used at VulkanGraphics::frameCounter's
     *  current value — called once per call by VulkanGraphics::getOrCreateShaderInstance()
     *  (the SSOT bind site; every caller reaches getShaderInstance() only
     *  after getOrCreateShaderInstance() succeeds, so this is the one place a
     *  live entry is proven still reachable this frame).
     *  @param frame  VulkanGraphics::frameCounter's current value.
     */
    void setLastUsedFrame (uint64_t frame) noexcept { lastUsedFrame = frame; }

    //==========================================================================
    // Slang per-pass reflected resources
    //==========================================================================

    /** @brief Returns true if this execution's VulkanShader compiled through
     *  VulkanShaderFormat::slang — true means every pass indexed below binds
     *  its OWN per-pass descriptor set/pipeline layout at draw time (see
     *  getSlangPassPipelineLayout()/getSlangPassDescriptorSet()) instead of
     *  the shared bindless getOrCreateShaderInstanceLayout()/
     *  getBindlessTextureDescriptorSet() every VulkanShaderFormat::shadertoy
     *  execution continues using, completely unaffected. */
    bool usesSlangPipeline() const noexcept { return slangShader; }

    /** @brief Returns this execution's alias -> RetroArch pass ordinal map --
     *  see passAliases' own doc comment. */
    const jam::HashMap<juce::String, int>& getPassAliases() const noexcept { return passAliases; }

    /** @brief Returns passIndex's own reflected texture list — the input
     *  VulkanGraphics::getSlangTextureBindings() resolves against before
     *  calling refreshSlangPass() for the same passIndex.
     *  @param passIndex  Zero-based ordinal into shader.passes (buffer
     *                    passes then the mandatory Image pass, per
     *                    getImagePassIndex()) — only valid when
     *                    usesSlangPipeline() is true.
     */
    const jam::Array<VulkanShaderReflection::TextureResource>& getSlangPassTextures (int passIndex) const noexcept
    {
        return slangPasses.at (static_cast<size_t> (passIndex))->reflection.textures;
    }

    /** @brief Returns passIndex's own resolved preset directives (
     *  VulkanSlangPassResources::settings) — VulkanGraphics::getOrCreateSlangPassSampler()'s
     *  own input, mapping this pass's own filter_linearN/wrap_modeN (always
     *  concrete for filterLinear post-compile, jam_VulkanShaderPreset.h's own
     *  Pass::filterLinear doc comment) to the concrete sampler every one of
     *  this pass's texture bindings is refreshed against
     *  (refreshSlangPass()'s own @p sampler parameter).
     *  @param passIndex  See getSlangPassTextures().
     */
    const VulkanShaderPreset::Pass& getSlangPassSettings (int passIndex) const noexcept
    {
        return slangPasses.at (static_cast<size_t> (passIndex))->settings;
    }

    /** @brief Returns passIndex's own dedicated pipeline layout, built once
     *  by buildSlangPassResources() from that pass's own SPIR-V reflection —
     *  bind this INSTEAD of getOrCreateShaderInstanceLayout() for this
     *  pass's draw when usesSlangPipeline() is true.
     *  @param passIndex  See getSlangPassTextures().
     */
    vk::PipelineLayout getSlangPassPipelineLayout (int passIndex) const noexcept
    {
        return slangPasses.at (static_cast<size_t> (passIndex))->pipelineLayout;
    }

    /** @brief Returns passIndex's own descriptor set, or an empty handle
     *  when this pass declares neither a UBO nor any texture (no set was
     *  ever allocated for it) — the caller's own bindDescriptorSets() call
     *  must be skipped entirely when this is empty.
     *  @param passIndex  See getSlangPassTextures().
     */
    vk::DescriptorSet getSlangPassDescriptorSet (int passIndex) const noexcept
    {
        return slangPasses.at (static_cast<size_t> (passIndex))->descriptorSet;
    }

    /** @brief Refreshes passIndex's own per-frame GPU state: memcpy's fresh
     *  bytes into its uniform buffer's persistent-mapped pointer (when this
     *  pass declares a UBO — the descriptor binding itself was already
     *  written once at build time, since the buffer's vk::Buffer handle
     *  never changes; only its CONTENTS need refreshing per frame), and
     *  rewrites every one of its texture bindings fresh via
     *  vkUpdateDescriptorSets from @p textureBindings — unconditionally for
     *  EVERY texture binding, not only the ones fed by a ping-ponging buffer
     *  pass: a name resolving to "Source"/"Original" is just as liable to
     *  point at a recreated vk::ImageView after a resize as a
     *  "PassOutputN"/"PassFeedbackN" name is liable to alternate identity
     *  every frame via VulkanRenderResources::currentReadHalf — refreshing
     *  uniformly every frame this pass draws is the one design that is
     *  correct in both cases without needing to classify which category a
     *  given reflected texture name falls into. @p frameCount is itself
     *  first reduced through this pass's own VulkanSlangPassResources::settings.
     *  frameCountMod (RetroArch's own frame_count_modN preset directive,
     *  jam_VulkanShaderPreset.h) before either populate*Buffer() call below
     *  ever sees it — frameCountMod == 0 (absent) passes @p frameCount
     *  through unchanged.
     *  @param passIndex           See getSlangPassTextures().
     *  @param passExtent          This pass's own render target extent —
     *                             VulkanShaderReflection::populate*Buffer()'s
     *                             "OutputSize" source.
     *  @param finalViewportExtent The mandatory Image pass's own render
     *                             target extent (this execution's own
     *                             getBuiltExtent()) — "FinalViewportSize"
     *                             source, readable from any pass per
     *                             RetroArch's own documented semantics.
     *  @param frameCount          This execution's own raw per-frame
     *                             counter — the same value already stamped
     *                             into VulkanShaderUniforms::iFrame by
     *                             stampUniforms() this same call —
     *                             "FrameCount" source.
     *  @param textureBindings     This pass's own resolved texture names
     *                             (VulkanGraphics::getSlangTextureBindings()).
     *  @param sampler             This PASS's own concrete sampler, paired
     *                             with every one of its texture descriptor
     *                             writes this call makes —
     *                             VulkanGraphics::getOrCreateSlangPassSampler
     *                             (getSlangPassSettings (passIndex)), resolved
     *                             from this pass's own filter_linearN/
     *                             wrap_modeN preset directives (never a
     *                             single shared linear sampler across every
     *                             pass).
     *  @return  This pass's own push-constant bytes
     *           (VulkanShaderReflection::populatePushConstantBuffer()'s output)
     *           for the caller to vkCmdPushConstants against
     *           getSlangPassPipelineLayout (passIndex) — an empty
     *           juce::MemoryBlock when this pass declares no push_constant
     *           block, in which case the caller skips the push entirely.
     */
    juce::MemoryBlock refreshSlangPass (int passIndex, vk::Extent2D passExtent, vk::Extent2D finalViewportExtent,
                                        uint32_t frameCount, const VulkanShaderTextureBindings& textureBindings,
                                        vk::Sampler sampler) const;

    //==========================================================================
    // OriginalHistory ring buffer (RetroArch OriginalHistoryN)
    //==========================================================================

    /** @brief Returns the highest OriginalHistoryN ordinal any pass in this
     *  execution's own chain reflects (build()'s own depth-detection scan),
     *  or 0 when no pass reflects OriginalHistoryN at all — 0 doubles as
     *  "feature inactive" since OriginalHistory0 always resolves identically
     *  to Original (VulkanGraphics::getSlangTextureBindings()) and so needs no
     *  ring entry of its own; originalHistoryImages.size() ==
     *  getOriginalHistoryDepth() + 1 whenever this is > 0, and stays empty
     *  (size 0) when this is 0. */
    int getOriginalHistoryDepth() const noexcept
    {
        return originalHistoryImages.isEmpty() ? 0 : originalHistoryImages.size() - 1;
    }

    /** @brief Returns the OriginalHistoryN ring buffer itself — mutable:
     *  VulkanGraphics::recordOriginalHistoryCopy() barriers/copies directly into
     *  these images' own vk::Image/vk::ImageView handles, and VulkanGraphics::
     *  getSlangTextureBindings() reads a computed slot's own view for a
     *  slang pass's own OriginalHistoryN texture binding (mirrors
     *  getImagePassGatherTarget()'s identical mutable-reference convention
     *  for VulkanRenderResources — this vector is this execution's own equivalent
     *  Vulkan-resource-holding member, just without a wrapping struct, since
     *  it owns no framebuffer/pipeline of its own, only images). Empty when
     *  getOriginalHistoryDepth() == 0. */
    jam::Array<VulkanImage>& getOriginalHistoryImages() noexcept { return originalHistoryImages; }

    /** @brief Returns originalHistoryCursor's current value — see its own
     *  doc comment. Read by VulkanGraphics::recordOriginalHistoryCopy() (to locate
     *  this frame's own write slot) and VulkanGraphics::getSlangTextureBindings()
     *  (to derive a retained frame's own read slot, both AFTER
     *  recordOriginalHistoryCopy()'s own per-frame advance below — see that
     *  method's own doc comment for the exact cursor arithmetic). */
    int getOriginalHistoryCursor() const noexcept { return originalHistoryCursor; }

    /** @brief Advances originalHistoryCursor — called exactly once per
     *  frame, at the end of VulkanGraphics::recordOriginalHistoryCopy(), mirroring
     *  setLastUsedFrame()'s identical "orchestrator tells, object stores"
     *  shape (this execution owns no per-frame scheduling of its own, so the
     *  orchestrator — VulkanGraphics — computes the next value and tells this
     *  execution to store it, rather than this execution tracking its own
     *  advance internally).
     *  @param cursor  The next ring slot — VulkanGraphics::recordOriginalHistoryCopy()'s
     *                 own (previous cursor + 1) % getOriginalHistoryImages().size().
     */
    void setOriginalHistoryCursor (int cursor) noexcept { originalHistoryCursor = cursor; }

private:
    //==========================================================================
    // Build orchestration steps (build()'s own call sequence, Lean-decomposed)
    //==========================================================================

    /** @brief build()'s first step — resolves passAliases (aliasN preset
     *  directive, jam_VulkanShaderPreset.h, already carrying VulkanShaderCompiler::
     *  compile()'s own \#pragma name fallback overlay) and externalTextureChannels
     *  (shader.preset.textures' own declaration-order push-constant slot,
     *  texture K at bufferPassCount + K — jam::VulkanShaderCompiler::
     *  compile()'s own identical channelMacros() formula) — both built once
     *  here regardless of format, a no-op for every VulkanShaderFormat::shadertoy
     *  shader (whose preset is always empty).
     *  @param shader  The compiled shader this execution serves.
     */
    void buildPassAliasesAndExternalTextureChannels (const VulkanShader& shader);

    /** @brief build()'s slang-only OriginalHistoryN ring step — scans every
     *  already-reflected slang pass (slangPasses, built by
     *  buildSlangPassResources() immediately before this runs) for the
     *  VulkanShaderReflection::getOriginalHistoryPrefix() vocabulary, and builds
     *  originalHistoryImages sized to the deepest ordinal found + 1 when any
     *  pass declares OriginalHistoryN (N >= 1) — see originalHistoryImages'
     *  own doc comment for why the ring always carries one more slot than the
     *  deepest requested read. A no-op (returns true unchanged) for a
     *  VulkanShaderFormat::shadertoy execution (slangShader false) or a slang chain
     *  reflecting no OriginalHistoryN texture at all.
     *  @param colorFormat  Swapchain-mirroring format the ring's images are
     *                      built at (build()'s own @p colorFormat).
     *  @param sceneExtent  Full-resolution scene extent the ring's images are
     *                      built at (build()'s own @p sceneExtent — this
     *                      ring's own source, VulkanGraphics::straightAlphaImage, is
     *                      swapchain-sized, never resolutionScale-scaled).
     *  @return true if every ring image (if any) was created successfully.
     */
    bool buildOriginalHistoryRing (vk::Format colorFormat, vk::Extent2D sceneExtent);

    /** @brief build()'s per-buffer-pass step — builds every buffer pass's own
     *  ping-pong VulkanRenderResources target (bufferPassTargets), resolving each
     *  pass's own extent (computePassExtent()), format (srgb_framebufferN/
     *  float_framebufferN), and mip levels (its downstream consumer's own
     *  mipmap_inputN) in declared order — see build()'s own doc comment for
     *  the full per-pass resolution contract this loop implements.
     *  @param shader                     The compiled shader this execution serves.
     *  @param colorFormat                Fallback/mirroring format — see build()'s own doc comment.
     *  @param scaledExtent               This execution's own computeScaledExtent() result.
     *  @param chainInputExtent           The basis extent for pass 0's own "source" scale
     *                                    directive — see build()'s own doc comment.
     *  @param offscreenRenderPassFactory  Resolves a colour format to its own offscreen
     *                                    render pass — see build()'s own doc comment.
     *  @return true if every buffer pass target built successfully.
     */
    bool buildBufferPassTargets (const VulkanShader& shader, vk::Format colorFormat, vk::Extent2D scaledExtent,
                                vk::Extent2D chainInputExtent,
                                const std::function<vk::RenderPass (vk::Format)>& offscreenRenderPassFactory);

    /** @brief build()'s mandatory-Image-pass step — resolves whether the
     *  Image pass declares a self-feedback read of its own output
     *  (RetroArch's real-world 1-pass feedback preset shape —
     *  see build()'s own doc comment for the exact "PassFeedback<final pass
     *  ordinal>"/"<alias>Feedback" gate), then builds imagePassGatherTarget at
     *  scaledExtent/@p colorFormat unconditionally (never preset-relative —
     *  RetroArch's own final pass always renders to the viewport at the
     *  engine's own format).
     *  @param shader                     The compiled shader this execution serves.
     *  @param colorFormat                Unconditional format for the gather target.
     *  @param scaledExtent               Unconditional extent for the gather target.
     *  @param offscreenRenderPassFactory  See buildBufferPassTargets()'s own doc comment.
     *  @return true if imagePassGatherTarget built successfully.
     */
    bool buildImagePassTarget (const VulkanShader& shader, vk::Format colorFormat, vk::Extent2D scaledExtent,
                              const std::function<vk::RenderPass (vk::Format)>& offscreenRenderPassFactory);

    /** @brief build()'s final, unconditional mesh step — a no-op for every
     *  shader.meshPath-empty execution (every project before this feature).
     *  Writes meshReady directly (graceful last-good — see buildMeshResources()'s
     *  own doc comment for why a mesh parse/upload failure never feeds back
     *  into build()'s own overall @c built flag). The mesh's own material-
     *  range/feature-edge draw has no render target/pipeline of its own to
     *  build here — it renders directly into imagePassGatherTarget (already
     *  built above, by buildImagePassTarget()) via the engine-owned mesh
     *  pipelines (VulkanGraphics::getOrCreateMeshPipeline() et al.) or this
     *  execution's own hooked ones, so this step needs neither a colour
     *  format/extent nor a render-pass factory of its own. When meshReady AND
     *  shader declares a mesh_shader= connection (shader.meshShaderSource
     *  non-empty — mesh_shader= is a vertex-ANIMATION HOOK into the
     *  engine's own default mesh look), also builds this execution's own
     *  hooked mesh pipelines (buildMeshHookPipelines()) — a no-op otherwise
     *  (no mesh_shader= connection, or meshReady false: no geometry exists to
     *  draw regardless of any hook).
     *  @param shader                     The compiled shader this execution serves.
     *  @param commandBuffer              See build()'s own doc comment.
     *  @param meshDescriptorSetLayout    See build()'s own doc comment.
     *  @param meshPipelineLayout         See build()'s own doc comment.
     */
    void buildMeshIfDeclared (const VulkanShader& shader, vk::CommandBuffer commandBuffer,
                             vk::DescriptorSetLayout meshDescriptorSetLayout,
                             vk::PipelineLayout meshPipelineLayout);

    /** @brief buildMeshIfDeclared() step — compiles shader.meshShaderSource
     *  (the project's own mainMesh snippet) spliced into BOTH engine mesh
     *  vertex-stage templates (VulkanShaderCompiler::compileMeshVertexStage(),
     *  Id::meshDefaultVertex/Id::meshEdgeVertex) and
     *  builds this execution's own 3 hooked pipelines — meshHookFillPipeline/
     *  meshHookTransparentFillPipeline (both: hooked mesh_default.vert
     *  compile + the engine's own precompiled mesh_default.frag, opaque/alpha
     *  blend respectively — same rendering contract as
     *  VulkanGraphics::getOrCreateMeshPipeline()/getOrCreateMeshTransparentFillPipeline())
     *  and meshHookEdgePipeline (hooked mesh_edge.vert compile + the engine's
     *  own precompiled mesh_edge.frag, opaque blend — same rendering contract
     *  as VulkanGraphics::getOrCreateMeshWireframePipeline()). Transient
     *  vk::ShaderModule set, compiled then destroyed immediately after all 3
     *  pipelines build — the SAME "modules transient compile-destroy"
     *  convention buildRenderResources()/createFullscreenPipeline() already
     *  use. Zero vertex-input bindings/attributes (vertex-pulling from the
     *  SAME mesh SSBOs the engine-default pipelines pull from, set 0 binding
     *  1/2, via gl_VertexIndex — identical technique to VulkanGraphics::
     *  getOrCreateMeshPipeline()'s own doc comment). Built against
     *  @p meshPipelineLayout and imagePassGatherTarget.renderPass (the SAME
     *  unified offscreen shader render pass every mesh draw targets, already
     *  built above, by buildImagePassTarget()) — createMeshHookPipeline()'s
     *  own doc comment for the exact per-variant fixed-function state. ALL
     *  three pipelines build or none do (a partial build is torn down and
     *  every handle reset to null) — any failure (vertex-hook compile,
     *  shader-module creation, or pipeline creation) is logged via
     *  jam::debug::Log and leaves every handle at its own null default —
     *  graceful last-good: hasMeshHook() stays false, and the engine's own
     *  default, unanimated mesh pipelines draw instead (VulkanGraphics::
     *  recordMeshMaterialRangeDraws()'s own per-draw dispatch).
     *  @param shader               The compiled shader this execution serves —
     *                              meshShaderSource non-empty (buildMeshIfDeclared()'s
     *                              own gate).
     *  @param meshPipelineLayout   See build()'s own doc comment.
     *  @return true if every pipeline built successfully.
     */
    bool buildMeshHookPipelines (const VulkanShader& shader, vk::PipelineLayout meshPipelineLayout);

    /** @brief buildMeshHookPipelines() step — builds one hooked mesh
     *  pipeline from an already-compiled vertex module + fragment module
     *  pair. vk::PrimitiveTopology::eTriangleList, eFill, depth test/write ON
     *  (vk::CompareOp::eLess — same depth contract as VulkanGraphics::
     *  createMeshPipelineFixedFunctionState()'s own engine-default mesh
     *  pipelines), dynamic viewport/scissor, built against
     *  imagePassGatherTarget.renderPass (already built above, by
     *  buildImagePassTarget()). @p blendAttachment selects the one
     *  per-variant difference between all 3 callers (VulkanPipelines::
     *  opaqueBlendAttachment() for the fill/edge variants,
     *  VulkanPipelines::alphaBlendAttachment() for the transparent-fill variant) —
     *  mirrors VulkanGraphics::createMeshPipeline()'s identical per-variant
     *  parameterization of the SAME 3-pipeline shape.
     *  @param vertModule       Already-compiled hooked vertex module.
     *  @param fragModule       Already-compiled engine fragment module
     *                          (mesh_default.frag or mesh_edge.frag).
     *  @param blendAttachment  This variant's own colour-blend contract.
     *  @param meshPipelineLayout  See build()'s own doc comment.
     *  @return  The built pipeline, or a null handle on failure.
     */
    vk::Pipeline createMeshHookPipeline (vk::ShaderModule vertModule, vk::ShaderModule fragModule,
                                        const vk::PipelineColorBlendAttachmentState& blendAttachment,
                                        vk::PipelineLayout meshPipelineLayout) const;

    /** @brief build()'s final step — stamps contentHash/builtExtent/
     *  startTimeMs/lastStampTimeMs/frameCounter only when @p built is true,
     *  then stamps and returns ready (SSOT — the one write site for this
     *  member).
     *  @param built              Whether every step above this call succeeded.
     *  @param shaderContentHash  shader.contentHash — stored verbatim into contentHash.
     *  @param scaledExtent       This execution's own computeScaledExtent() result.
     *  @return ready's newly-stamped value.
     */
    bool finalizeBuild (bool built, uint64_t shaderContentHash, vk::Extent2D scaledExtent);

    //==========================================================================
    // Build helpers
    //==========================================================================

    /** @brief Builds one VulkanRenderResources' depth image, colour images/
     *  framebuffers, and pipeline — reads storedPipelineLayout/
     *  storedVertModule (already set by build() before this is called).
     *  Serves both call shapes build() needs: a buffer pass's ping-pong pair
     *  (imageCount == 2) and the mandatory Image pass's own single-image
     *  gather target (imageCount == 1) — the per-image loop and the
     *  pipeline-build step below it are identical either way, only the loop
     *  bound varies. Builds exactly ONE depth image per call (outTarget.
     *  depthImage, VulkanRenderResources' own doc comment), shared by every half's
     *  own 2-attachment framebuffer — @p offscreenRenderPass now carries an
     *  unconditional depth attachment for every target this method builds,
     *  never only the mesh-carrying ones (VulkanGraphics::
     *  getOrCreateShaderOffscreenRenderPass (vk::Format)'s own doc comment).
     *  @param pass                 Pass this target is built for — a buffer
     *                              pass (BufferA-D) or the mandatory Image pass.
     *  @param colorFormat          Colour format the target's images are created
     *                              with — stored verbatim into
     *                              VulkanRenderResources::format (this pass's own
     *                              build()-resolved srgb_framebufferN/
     *                              float_framebufferN format, or @p colorFormat's
     *                              own build()-level fallback; see build()'s own
     *                              doc comment).
     *  @param passExtent           Pixel extent to build the target's images/framebuffers
     *                              at, and stored verbatim into VulkanRenderResources::extent —
     *                              this pass's OWN extent (build()'s own
     *                              computePassExtent() result for a buffer pass, per its
     *                              jam::VulkanShaderPreset::Pass scale/scale_type
     *                              directive; always this execution's own scaledExtent
     *                              for the mandatory Image pass's gather target, which
     *                              never resizes per-preset — see build()'s own doc
     *                              comment).
     *  @param offscreenRenderPass  Render pass the target's framebuffer is created
     *                              against — stored verbatim into
     *                              VulkanRenderResources::renderPass (build()'s own
     *                              offscreenRenderPassFactory (colorFormat)
     *                              result for this SAME colorFormat, so
     *                              VulkanRenderResources::format/renderPass always
     *                              agree).
     *  @param imageCount           Images/framebuffers to build — 2 for a
     *                              ping-pong-capable buffer pass, 1 for the
     *                              Image pass's non-feedback gather target.
     *  @param numMipLevels         Mip levels each of this target's own
     *                              images/attachmentViews carries — stored
     *                              verbatim into VulkanRenderResources::numMipLevels
     *                              and threaded into every VulkanImage::create2D()
     *                              call this method makes (build()'s own
     *                              computeMipLevelCount() result when this
     *                              pass's downstream consumer declares
     *                              mipmap_inputN, else 1 — see
     *                              VulkanRenderResources::numMipLevels' own doc
     *                              comment). > 1 also gains eTransferSrc/
     *                              eTransferDst on every image built here
     *                              (VulkanGraphics::recordMipChainGeneration()'s
     *                              own per-level vkCmdBlitImage requirement).
     *  @param passPipelineLayout   The pipeline layout this pass's own
     *                              pipeline is built against — storedPipelineLayout
     *                              (the shared bindless layout) for a
     *                              VulkanShaderFormat::shadertoy execution,
     *                              or this specific pass's own
     *                              slangPasses[passIndex]->pipelineLayout
     *                              (built by buildSlangPassResources() before
     *                              this is ever called) for a
     *                              VulkanShaderFormat::slang execution — see
     *                              build()'s own top-level branch.
     *  @param outTarget            Receives the built images/framebuffers/pipeline/
     *                              extent/format/renderPass.
     */
    bool buildRenderResources (const VulkanShaderPass& pass, vk::Format colorFormat, vk::Extent2D passExtent,
                              vk::RenderPass offscreenRenderPass, int imageCount, uint32_t numMipLevels,
                              vk::PipelineLayout passPipelineLayout, VulkanRenderResources& outTarget) const;

    /** @brief Builds one fullscreen-pass pipeline. Two distinct vertex-input
     *  shapes, selected by @p hasOwnVertexStage: Shadertoy (false) — zero
     *  vertex input, vk::PrimitiveTopology::eTriangleList,
     *  VulkanPipelines::fullscreenTriangleVertexCount vertices generated purely from
     *  gl_VertexIndex (today's exact, unchanged behavior); slang (true) — one
     *  binding/two attributes matching SlangQuadVertex's own layout
     *  (Position @ vk::Format::eR32G32B32A32Sfloat offset 0, TexCoord @
     *  vk::Format::eR32G32Sfloat offset 16) and
     *  vk::PrimitiveTopology::eTriangleStrip, slangQuadVertexCount vertices —
     *  a real slang vertex stage declares its own two mandatory attributes
     *  (RetroArch's own vertex-stage attribute contract) and rejects an empty vertex input state outright
     *  (MoltenVK: "VulkanVertex attribute m_24(0) is missing from the vertex
     *  descriptor"). Dynamic viewport/scissor, depth test/write explicitly
     *  DISABLED either way (VulkanPipelines::noStencilState()) — required even
     *  though the render pass every one of these pipelines is built against
     *  now carries an unconditional depth attachment (VulkanGraphics::
     *  getOrCreateShaderOffscreenRenderPass (vk::Format)'s own doc comment):
     *  a fullscreen-triangle pass draw never tests or writes depth, only the
     *  mesh's own material-range/feature-edge draw does (VulkanGraphics::
     *  getOrCreateMeshPipeline() et al., depth test+write ENABLED, built
     *  against this SAME render pass). Built by buildRenderResources()
     *  (every VulkanRenderResources this execution owns — buffer pass targets and
     *  the Image pass's own gather target alike — is a full, isolated
     *  overwrite of its own offscreen image), so every current caller
     *  supplies VulkanPipelines::opaqueBlendAttachment().
     *  @param vertModule        Already-compiled vertex module the pipeline is built from —
     *                           either the shared storedVertModule (this pass has no own
     *                           vertex stage, VulkanShaderPass::vertexSpirv empty) or a per-pass
     *                           dedicated module (VulkanShaderPass::vertexSpirv non-empty).
     *  @param fragModule        Already-compiled fragment module the pipeline is built from.
     *  @param targetRenderPass  Render pass the pipeline will draw into.
     *  @param sampleCount       Sample count targetRenderPass's colour attachment uses.
     *  @param blendAttachment   This target's blend contract — VulkanPipelines::opaqueBlendAttachment()
     *                           for every current caller (a full-overwrite offscreen write never
     *                           blends); kept as a parameter, not hardcoded, since this is the
     *                           same shared fullscreen-pipeline shape the two VulkanShaderRegistry-owned
     *                           combine pipelines also use (VulkanShaderRegistry::getOrCreateBackgroundCombinePipeline()/
     *                           getOrCreatePostProcessCombinePipeline(), jam_VulkanShaderRegistry.cpp
     *                           — built inline there rather than through this method, mirroring
     *                           getOrCreateStraightAlphaPipeline()'s own precedent, since those
     *                           pipelines are VulkanGraphics-owned, not any one VulkanShaderInstance's).
     *  @param pipelineLayout    The pipeline layout this pipeline is built
     *                           against — see buildRenderResources()'s own
     *                           passPipelineLayout doc comment for the
     *                           shadertoy/slang distinction; this method
     *                           itself stays agnostic to which one it was
     *                           handed (was storedPipelineLayout read
     *                           directly before slang support existed — now
     *                           an explicit parameter so a slang pass's own
     *                           dedicated layout can be substituted).
     *  @param hasOwnVertexStage This pass's own VulkanShaderPass::vertexSpirv
     *                           emptiness (buildRenderResources()'s own
     *                           already-computed value) — selects the
     *                           slang/Shadertoy vertex-input shape above.
     */
    vk::Pipeline createFullscreenPipeline (vk::ShaderModule vertModule, vk::ShaderModule fragModule,
                                          vk::RenderPass targetRenderPass, vk::SampleCountFlagBits sampleCount,
                                          const vk::PipelineColorBlendAttachmentState& blendAttachment,
                                          vk::PipelineLayout pipelineLayout, bool hasOwnVertexStage) const;

    /** @brief Value-only (no internal pointers) fixed-function pipeline state
     *  for createFullscreenPipeline()'s own use — mirrors VulkanGraphics::
     *  MeshPipelineFixedFunctionState's identical "helper returns value-only
     *  state, caller wires every pointer field into its OWN copy" shape
     *  (jam_VulkanGraphics.h). slangQuadBinding/slangQuadAttributes are
     *  included here (unlike the mesh struct, which never carries vertex
     *  input) since createFullscreenPipeline()'s own
     *  vk::PipelineVertexInputStateCreateInfo must point at THIS caller-owned
     *  copy's own binding/attribute storage when hasOwnVertexStage is true —
     *  never at addresses local to createFullscreenPipelineFixedFunctionState()'s
     *  own stack frame, which end at that function's return. */
    struct FullscreenPipelineFixedFunctionState
    {
        vk::VertexInputBindingDescription slangQuadBinding {};
        std::array<vk::VertexInputAttributeDescription, 2> slangQuadAttributes {};
        vk::PrimitiveTopology topology { vk::PrimitiveTopology::eTriangleList };
        vk::PipelineViewportStateCreateInfo viewportInfo {};
        vk::PipelineRasterizationStateCreateInfo rasterizerInfo {};
        vk::PipelineMultisampleStateCreateInfo multisampleInfo {};
        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo {};
        std::array<vk::DynamicState, 2> dynamicStates {};
    };

    /** @brief createFullscreenPipeline() step — builds every fixed-function
     *  pipeline-state value it needs: the slang quad's own vertex binding/
     *  attributes (SlangQuadVertex's own layout, only ever pointed at by the
     *  caller when @p hasOwnVertexStage is true), topology (eTriangleStrip
     *  for slang, eTriangleList for Shadertoy), viewport/scissor counts (1
     *  each, dynamic), rasterizer (eFill, cullMode eNone, 1px line width),
     *  @p sampleCount-driven multisample state, VulkanPipelines::noStencilState()
     *  (a VulkanShader pass draw never touches depth or stencil), and the shared
     *  viewport/scissor dynamic-state pair. See FullscreenPipelineFixedFunctionState's
     *  own doc comment for why this returns by value safely.
     *  @param sampleCount        Sample count the target render pass's colour attachment uses.
     *  @param hasOwnVertexStage  Selects the slang/Shadertoy vertex-input shape — see
     *                            createFullscreenPipeline()'s own doc comment.
     */
    FullscreenPipelineFixedFunctionState createFullscreenPipelineFixedFunctionState (
        vk::SampleCountFlagBits sampleCount, bool hasOwnVertexStage) const;

    /** @brief createFullscreenPipeline() step — builds the vertex input
     *  state: zero bindings/attributes for Shadertoy (@p hasOwnVertexStage
     *  false — today's exact, unchanged behavior); for slang (true), points
     *  at @p fixedState's OWN slangQuadBinding/slangQuadAttributes storage —
     *  the CALLER's own copy (createFullscreenPipeline()'s own @p fixedState
     *  local, passed here by reference and still alive for that whole
     *  function's scope), never any address local to this method's own stack
     *  frame — safe to return by value for the identical reason
     *  FullscreenPipelineFixedFunctionState's own doc comment gives.
     *  @param fixedState         Already-built fixed-function state (createFullscreenPipelineFixedFunctionState()).
     *  @param hasOwnVertexStage  See createFullscreenPipeline()'s own doc comment. */
    vk::PipelineVertexInputStateCreateInfo buildFullscreenVertexInputInfo (
        const FullscreenPipelineFixedFunctionState& fixedState, bool hasOwnVertexStage) const;

    /** @brief createFullscreenPipeline() step — assembles the final
     *  vk::GraphicsPipelineCreateInfo from @p fixedState's own already-built
     *  value fields plus @p vertexInputInfo/@p inputAssemblyInfo/
     *  @p colorBlendInfo/@p dynamicStateInfo, every one of them owned by the
     *  CALLER's own stack frame (createFullscreenPipeline()'s own locals) —
     *  the returned struct's pointer fields reference those same
     *  caller-owned addresses, never anything local to this method's own
     *  stack frame, so returning by value is safe (mirrors
     *  VulkanShaderRegistry::createMeshPipeline()'s identical reasoning for its
     *  own inline vk::GraphicsPipelineCreateInfo assembly).
     *  @param stages               The 2-stage vertex/fragment shader array.
     *  @param fixedState           Already-built fixed-function state (createFullscreenPipelineFixedFunctionState()).
     *  @param vertexInputInfo      Already-built vertex input state — a
     *                              separate parameter from @p fixedState
     *                              since its conditional (@p hasOwnVertexStage)
     *                              pointer-setting logic must run in the
     *                              caller, against the caller's own copy of
     *                              @p fixedState (see createFullscreenPipeline()'s
     *                              own doc comment).
     *  @param inputAssemblyInfo    Already-built input assembly state.
     *  @param colorBlendInfo       Already-built colour-blend state.
     *  @param dynamicStateInfo     Already-built dynamic-state descriptor.
     *  @param pipelineLayout       See createFullscreenPipeline()'s own doc comment.
     *  @param targetRenderPass     See createFullscreenPipeline()'s own doc comment.
     *  @return The filled vk::GraphicsPipelineCreateInfo, ready for createGraphicsPipelines(). */
    vk::GraphicsPipelineCreateInfo buildFullscreenPipelineCreateInfo (
        const vk::PipelineShaderStageCreateInfo (&stages)[2], const FullscreenPipelineFixedFunctionState& fixedState,
        const vk::PipelineVertexInputStateCreateInfo& vertexInputInfo,
        const vk::PipelineInputAssemblyStateCreateInfo& inputAssemblyInfo,
        const vk::PipelineColorBlendStateCreateInfo& colorBlendInfo,
        const vk::PipelineDynamicStateCreateInfo& dynamicStateInfo,
        vk::PipelineLayout pipelineLayout, vk::RenderPass targetRenderPass) const;

    /** @brief build()'s slang-only step (VulkanShaderFormat::slang
     *  executions only) — reflects every pass's own fragment SPIR-V, sizes
     *  and creates ONE descriptor pool shared by every pass this execution
     *  owns (exact sizing from the reflected totals, not a padded ceiling —
     *  every pass's own reflection is already known before this pool is
     *  created, so an exact sum is strictly more precise than a guessed
     *  upper bound), then builds each pass's own descriptor set layout/set/
     *  uniform buffer (buildSlangPassDescriptorSetLayout()) and dedicated
     *  pipeline layout (buildSlangPassPipelineLayout()) into slangPasses,
     *  ordinal-aligned with @p passes (buffer passes then the mandatory
     *  Image pass — every pass, not just buffer passes, since a slang-format
     *  VulkanShader's Image pass is just as author-shaped as its buffer passes).
     *  Runs BEFORE build()'s own buildRenderResources() loop, which reads
     *  each pass's freshly-built slangPasses[index]->pipelineLayout as that
     *  pass's own passPipelineLayout argument. Also copies each pass's own
     *  @p preset.passes entry (or a default-constructed VulkanShaderPreset::Pass
     *  when @p preset declares fewer passes than @p passes) into that pass's
     *  own VulkanSlangPassResources::settings — see its own doc comment
     *  (jam_VulkanSlangPassResources.h) for which directives are consumed
     *  where.
     *  @param passes  shader.passes — every pass this execution owns.
     *  @param preset  shader.preset — this execution's own parsed .slangp
     *                 preset directives.
     *  @return true if every pass's slang resources built successfully.
     */
    bool buildSlangPassResources (const jam::Owner<VulkanShaderPass>& passes, const VulkanShaderPreset& preset);

    /** @brief buildSlangPassResources() step — sizes and creates
     *  slangDescriptorPool from every pass's own already-known reflection
     *  (exact sum, not a padded ceiling) — a no-op (returns true, pool stays
     *  empty) when no reflected pass declares a UBO or any texture.
     *  @param reflections  Every pass's own SPIR-V reflection, ordinal-aligned
     *                      with shader.passes (buildSlangPassResources()'s own
     *                      first step).
     *  @return true on success (including the "nothing to build" case above).
     */
    bool createSlangDescriptorPool (const jam::Array<VulkanShaderReflection>& reflections);

    /** @brief buildSlangPassResources() step — builds ONE pass's own
     *  VulkanSlangPassResources entry (descriptor set layout/set, uniform buffer,
     *  dedicated pipeline layout) and appends it to slangPasses.
     *  @param pass        This pass's own VulkanShaderPass — supplies parameterDefaults.
     *  @param reflection  This pass's own already-reflected SPIR-V — taken by
     *                     value and moved into the built VulkanSlangPassResources
     *                     entry (VulkanShaderReflection is move-only, owning
     *                     jam::Array members).
     *  @param passIndex   Zero-based ordinal into shader.passes.
     *  @param preset      shader.preset — see buildSlangPassResources()'s own doc comment.
     *  @return true if this one entry built successfully.
     */
    bool buildSlangPassEntry (const VulkanShaderPass& pass, VulkanShaderReflection reflection,
                             size_t passIndex, const VulkanShaderPreset& preset);

    /** @brief buildSlangPassEntry() step — builds @p passResources' own
     *  per-pass uniform buffer (persistently mapped, mirrors
     *  VulkanGraphics::projectionBuffer's identical shape) and writes its
     *  descriptor binding ONCE — gated by the caller on
     *  passResources.reflection.uniformBufferSize > 0.
     *  @param passResources  This pass's own slang resources — its
     *                        descriptorSet/reflection are read, its
     *                        uniformBuffer is written.
     *  @return true on success.
     */
    bool buildSlangPassUniformBuffer (VulkanSlangPassResources& passResources);

    /** @brief buildSlangPassResources() step — builds one pass's own
     *  descriptor set layout from its own reflection: one uniform-buffer
     *  binding at reflection.uniformBufferBinding (only if
     *  reflection.uniformBufferSize > 0), one combined-image-sampler binding
     *  per reflection.textures entry at its own TextureResource::binding.
     *  Leaves @p outLayout at its default (empty) handle, returning true,
     *  when this pass declares neither a UBO nor any texture — a fully
     *  parameter-free pass needs no descriptor set layout at all.
     *  @param reflection  This pass's own SPIR-V reflection.
     *  @param outLayout   Receives the built layout, or stays empty.
     *  @return true on success (including the "nothing to build" case above).
     */
    bool buildSlangPassDescriptorSetLayout (const VulkanShaderReflection& reflection, vk::DescriptorSetLayout& outLayout) const;

    /** @brief buildSlangPassResources() step — builds one pass's own
     *  dedicated pipeline layout: set 0 = passResources.descriptorSetLayout
     *  (omitted entirely when empty — a pass with no descriptor set needs no
     *  set in its own layout either) plus a single vk::PushConstantRange
     *  sized to passResources.reflection.pushConstantSize when non-zero
     *  (omitted otherwise). Stores the result directly into
     *  passResources.pipelineLayout.
     *  @param passResources  This pass's own slang resources — its
     *                        descriptorSetLayout/reflection are read, its
     *                        pipelineLayout is written.
     *  @return true on success.
     */
    bool buildSlangPassPipelineLayout (VulkanSlangPassResources& passResources) const;

    /** @brief refreshSlangPass() step — memcpy's fresh uniform bytes
     *  (VulkanShaderReflection::populateUniformBuffer()'s output) into
     *  @p passResources' own persistently-mapped uniformBuffer — gated by the
     *  caller on passResources.reflection.uniformBufferSize > 0.
     *  @param passResources        This pass's own slang resources.
     *  @param passExtent           See refreshSlangPass()'s own doc comment.
     *  @param finalViewportExtent  See refreshSlangPass()'s own doc comment.
     *  @param effectiveFrameCount  refreshSlangPass()'s own frameCountMod-reduced frame count.
     *  @param textureBindings      See refreshSlangPass()'s own doc comment.
     */
    void refreshSlangPassUniformBuffer (VulkanSlangPassResources& passResources, vk::Extent2D passExtent,
                                        vk::Extent2D finalViewportExtent, uint32_t effectiveFrameCount,
                                        const VulkanShaderTextureBindings& textureBindings) const;

    /** @brief refreshSlangPass() step — rewrites every one of
     *  @p passResources' own texture bindings fresh via
     *  vkUpdateDescriptorSets from @p textureBindings — gated by the caller
     *  on passResources.reflection.textures not being empty. A name absent
     *  from @p textureBindings.views (unresolved this frame) is simply
     *  skipped — see refreshSlangPass()'s own doc comment for the full
     *  graceful-degradation contract.
     *  @param passResources    This pass's own slang resources.
     *  @param textureBindings  See refreshSlangPass()'s own doc comment.
     *  @param sampler          This pass's own concrete sampler — see
     *                          refreshSlangPass()'s own @p sampler doc comment.
     */
    void refreshSlangPassTextureBindings (VulkanSlangPassResources& passResources, const VulkanShaderTextureBindings& textureBindings,
                                          vk::Sampler sampler) const;

    //==========================================================================
    // VulkanMesh-backed material-range draw build helpers
    //==========================================================================

    /** @brief build()'s mesh-only step — uploads @p meshShapes into mesh
     *  (jam::VulkanMesh::build(), recording its one-time staged copy into
     *  @p commandBuffer and flagging meshStagingPendingRelease for
     *  releaseMeshStagingIfPending()'s later post-fence release), bakes
     *  meshModelMatrix from mesh's own AABB (jam::VulkanOrbitCamera::
     *  computeAutoFitModelMatrix()), then builds every mesh GPU resource
     *  (buildMeshGpuResources() — the mesh's own MVP/normal-matrix UBO and
     *  descriptor set; no render target/pipeline of its own to build here,
     *  since the mesh's own material-range/feature-edge draw renders
     *  directly into imagePassGatherTarget via the engine-owned mesh
     *  pipelines, VulkanGraphics::recordMeshGatherDrawCommands()). @p meshShapes
     *  is parsed exactly ONCE, by VulkanShaderCompiler::compile() (jam::
     *  WavefrontObj::load(), warnings logged there via jam::debug::Log,
     *  warn-and-continue, jam::WavefrontObj's own contract) — this method
     *  never parses, and never re-parses across however many times this
     *  execution rebuilds (every per-extent window-resize rebuild included).
     *  An empty @p meshShapes (no mesh connection at all, or the compile-
     *  time parse failed) makes jam::VulkanMesh::build() itself return
     *  false (zero interleaved vertices/indices) — graceful last-good, this
     *  execution simply renders without a mesh (hasMesh() stays false,
     *  imagePassGatherTarget's own ordinary Image-pass draw is unaffected);
     *  NEVER fails the caller's own overall build() result (see build()'s
     *  own call site — this method's return feeds meshReady directly, never
     *  ANDed into build()'s own @c built flag).
     *  @param meshShapes                shader.meshShapes — this VulkanShader's
     *                                   own parsed OBJ mesh geometry,
     *                                   already resolved once at compile
     *                                   time (jam::VulkanShader's own
     *                                   @p newMeshShapes doc comment).
     *  @param commandBuffer             See build()'s own doc comment.
     *  @param meshDescriptorSetLayout   See build()'s own doc comment.
     *  @return true if the mesh fully uploaded and every GPU resource built
     *          successfully.
     */
    bool buildMeshResources (const jam::Owner<jam::WavefrontObj::Shape>& meshShapes, vk::CommandBuffer commandBuffer,
                            vk::DescriptorSetLayout meshDescriptorSetLayout);

    /** @brief buildMeshResources() step — builds meshUniformBuffer (MVP+
     *  normal-matrix+viewport/edge-thickness, persistently mapped, mirrors
     *  VulkanSlangPassResources::uniformBuffer's identical CPU_ONLY+MAPPED_BIT
     *  shape), the dedicated descriptor pool + meshDescriptorSet (factored
     *  into createMeshDescriptorPoolAndSet() — see its own doc comment), and
     *  writes binding 0 (meshUniformBuffer) + binding 1
     *  (mesh.getVertexBuffer() — this execution's own vertex-pulling SSBO)
     *  unconditionally, then binding 2 (mesh.getFeatureEdgeIndexBuffer() —
     *  this execution's own feature-edge endpoint-index SSBO) ONLY when
     *  mesh.getNumFeatureEdgeIndices() > 0 (a mesh may classify zero feature
     *  edges, jam::VulkanMesh's own class doc comment — that buffer stays
     *  an empty/invalid handle in that case, never written here). Every
     *  written binding's own vk::Buffer handle never changes after
     *  mesh.build(), so no write here is ever repeated.
     *  @param meshDescriptorSetLayout  See build()'s own doc comment.
     *  @return true on success.
     */
    bool buildMeshGpuResources (vk::DescriptorSetLayout meshDescriptorSetLayout);

    /** @brief buildMeshGpuResources() step — creates a dedicated descriptor
     *  pool (one UBO + up to two storage-buffer descriptors — the vertex-
     *  pulling SSBO always, the feature-edge endpoint-index SSBO only when
     *  this mesh classified at least one feature edge — exactly this
     *  execution's own single mesh descriptor set — mirrors
     *  buildSlangPassResources()'s own exact-sizing precedent, just for one
     *  fixed set instead of a per-pass sweep) and allocates meshDescriptorSet
     *  against @p meshDescriptorSetLayout.
     *  @param meshDescriptorSetLayout  See build()'s own doc comment.
     *  @return true on success.
     */
    bool createMeshDescriptorPoolAndSet (vk::DescriptorSetLayout meshDescriptorSetLayout);

    //==========================================================================
    // Members
    //==========================================================================

    /** @brief Shared Vulkan device — not owned, must outlive this VulkanShaderInstance. */
    VulkanDevice& device;

    /** @brief True once build() has completed successfully. */
    bool ready { false };

    /** @brief Content hash this execution was built for (VulkanShader::contentHash). */
    uint64_t contentHash { 0 };

    /** @brief Scaled extent this execution's buffer-pass targets were built at. */
    vk::Extent2D builtExtent {};

    /** @brief One entry per buffer pass (BufferA-D order, excludes the
     *  mandatory Image pass). Owner sweep — vector-of-unique_ptr, pointer-
     *  stable (jam_core/utilities/jam_Owner.h), mirrors
     *  VulkanTransparencyStack::stack's exact shape. */
    jam::Owner<VulkanRenderResources> bufferPassTargets {};

    /** @brief The mandatory Image pass's own offscreen gather target — a
     *  VulkanRenderResources built the same way as every buffer pass's own target
     *  (buildRenderResources(), from the Image pass's own compiled
     *  spirv/vertexSpirv), holding its own dedicated pipeline. imageCount is
     *  1 (single-image, non-ping-ponging) for every VulkanShaderFormat::shadertoy
     *  execution and for a VulkanShaderFormat::slang execution whose Image pass
     *  declares no self-feedback read; 2 (history-capable ping-pong pair)
     *  only when a real .slang shader's Image pass reflects its own
     *  "PassFeedback<final pass ordinal>" self-feedback texture (RetroArch's
     *  real-world 1-pass feedback preset shape) — see
     *  build()'s own doc comment for the exact condition. The Image pass's
     *  draw (VulkanLowLevelGraphicsContext::recordShaderImagePassDrawCommands(),
     *  VulkanGraphics::recordPostProcessCompositeDrawCommands()) targets this
     *  offscreen framebuffer/render pass instead of the scene/composite
     *  render pass directly, alternating write halves exactly like a buffer
     *  pass when history-capable (both sites' own ping-pong toggle,
     *  mirroring VulkanGraphics::recordSingleBufferPass()'s) — the VulkanGraphics-owned
     *  background/post-process combine pipelines then sample the
     *  just-written half back through its own bindless slot (see this
     *  class's own doc comment). Not an Owner-wrapped collection (unlike
     *  bufferPassTargets) — exactly one instance for this execution's whole
     *  lifetime, so a plain member suffices (mirrors VulkanWindingScratch's own
     *  single-target fields). */
    VulkanRenderResources imagePassGatherTarget {};

    /** @brief External LUT/texture BindlessTextures, keyed by their own
     *  author-assigned manifest/textures= name (jam::VulkanShaderPreset::
     *  VulkanTexture::name) — config declares the path, this execution owns the
     *  GPU resource (mirrors bufferPassTargets' own "engine owns the render
     *  target" shape; this class's own "external LUT" vocabulary — no
     *  incumbent VulkanShaderInstance member name fit this
     *  concept cleanly, flagged to ARCHITECT per NAMES Rule -1). Built (or
     *  left with an invalid VulkanBindlessTexture entry on decode failure) by
     *  VulkanGraphics::getOrCreateShaderInstance()'s own post-build orchestration, via
     *  VulkanGraphics::buildExternalTexture() — this class's own build() creates
     *  no GPU-uploaded resource of its own (see this class's own doc
     *  comment), the same split bufferPassTargets' own bindless-slot
     *  assignment already uses. One entry per shader.preset.textures
     *  declaration; empty for a preset declaring no textures at all (every
     *  existing project before this feature). See getExternalTextures(). */
    jam::HashMap<juce::String, VulkanBindlessTexture> externalTextures {};

    /** @brief Named external LUT/texture -> its own resolved push-constant
     *  channels[] slot — pure declaration order (texture K's own slot is
     *  always bufferPassCount + K, the SAME formula VulkanShaderCompiler::
     *  compile()'s own channelMacros() uses, jam_VulkanShaderCompiler.cpp),
     *  computed at build() time from shader.preset.textures (mirrors
     *  passAliases' own identical "small, name-keyed, built once from
     *  shader.preset" shape) so stampChannels() can stamp a named LUT's own
     *  slot without this execution needing a VulkanShader reference at stamp time
     *  (this class holds no such reference — see class doc comment; also
     *  flagged alongside externalTextures per NAMES Rule -1). Empty exactly
     *  when externalTextures is. */
    jam::HashMap<juce::String, int> externalTextureChannels {};

    /** @brief Shared pipeline layout every pass pipeline is built against —
     *  stored at build() time so future build helpers can reuse it without
     *  the caller re-supplying it. */
    vk::PipelineLayout storedPipelineLayout {};

    /** @brief The one shared fullscreen-triangle vertex module every pass
     *  pipeline binds — stored at build() time, same reason as storedPipelineLayout. */
    vk::ShaderModule storedVertModule {};

    /** @brief Millisecond-counter value captured at build() time — stampUniforms()'s
     *  iTime = (now - startTimeMs) / 1000. */
    double startTimeMs { 0.0 };

    /** @brief Millisecond-counter value captured at the previous stampUniforms()
     *  call — iTimeDelta = (now - lastStampTimeMs) / 1000. */
    double lastStampTimeMs { 0.0 };

    /** @brief Frame counter owned by this execution — incremented once per
     *  stampUniforms() call (SSOT, this execution's own iFrame source). */
    int frameCounter { 0 };

    /** @brief VulkanGraphics::frameCounter's value at this execution's most recent
     *  setLastUsedFrame() stamp (VulkanGraphics::getOrCreateShaderInstance() bind time) —
     *  distinct from this execution's own frameCounter above (that one drives
     *  iFrame; this one drives VulkanGraphics::beginFrame()'s orphan-eviction sweep).
     *  Left at build()'s implicit 0 until the first stamp. */
    uint64_t lastUsedFrame { 0 };

    /** @brief True when shader.format == VulkanShaderFormat::slang at this
     *  execution's build() time — see usesSlangPipeline(). False (default)
     *  for every VulkanShaderFormat::shadertoy execution, which allocates
     *  none of the slang-only members below. */
    bool slangShader { false };

    /** @brief Alias -> RetroArch pass ordinal, from shader.preset.passes' own
     *  aliasN directive (jam_VulkanShaderPreset.h) -- ordinal
     *  0..getBufferPassCount()-1 is a buffer pass, getBufferPassCount() is
     *  the mandatory Image pass. Built once by build() from every non-empty
     *  shader.preset.passes[N].alias entry (jam::VulkanShaderCompiler::
     *  compile() has already overlaid each pass's own \#pragma name fallback
     *  onto this SAME field for a preset entry declaring no aliasN of its
     *  own, jam_VulkanShaderCompiler.cpp) -- consumed by VulkanGraphics::
     *  getSlangTextureBindings()'s own alias-normalization step
     *  (jam_VulkanGraphicsSlangPass.cpp) to resolve an author's own alias
     *  spelling to the SAME ordinal space PassOutputN/PassFeedbackN already
     *  resolve through. Empty for a VulkanShaderFormat::shadertoy execution
     *  (shader.preset.passes is always empty for it) and for a slang
     *  execution whose preset declares no aliasN/\#pragma name at all. */
    jam::HashMap<juce::String, int> passAliases {};

    /** @brief ONE descriptor pool shared by every pass's own slang
     *  descriptor set (buildSlangPassResources()) — sized exactly from the
     *  sum of every pass's own reflected UBO-presence/texture-count, never
     *  reset (this execution's own lifetime, mirroring how the rest of this
     *  class's Vulkan handles are built once, eagerly, by build()). Empty
     *  (null handle) for a Shadertoy-format execution. */
    vk::DescriptorPool slangDescriptorPool {};

    /** @brief One entry per pass (buffer passes then the mandatory VulkanImage
     *  pass — ordinal-aligned with shader.passes, unlike bufferPassTargets
     *  which excludes the Image pass), populated only for a
     *  VulkanShaderFormat::slang execution by buildSlangPassResources().
     *  Owner sweep — vector-of-unique_ptr, pointer-stable, mirrors
     *  bufferPassTargets' own shape. Empty for a Shadertoy-format
     *  execution. */
    jam::Owner<VulkanSlangPassResources> slangPasses {};

    /** @brief RetroArch OriginalHistoryN ring buffer — one image per
     *  retained prior frame of the post-process path's straight-alpha scene
     *  (VulkanGraphics::straightAlphaImage), built only when build()'s own
     *  depth-detection scan finds at least one pass reflecting an
     *  "OriginalHistory<K>" texture (VulkanShaderReflection::getOriginalHistoryPrefix())
     *  across this execution's whole pass chain. Sized
     *  getOriginalHistoryDepth() + 1 (never just getOriginalHistoryDepth())
     *  — the +1 keeps the slot VulkanGraphics::recordOriginalHistoryCopy()
     *  overwrites THIS frame from aliasing the deepest retained read
     *  (OriginalHistory<getOriginalHistoryDepth()>) a slang pass may still
     *  sample THIS SAME frame (see that method's own doc comment for the
     *  full cursor-arithmetic derivation). Built at build()'s own
     *  @p sceneExtent (the full, un-scaled scene extent — the source it
     *  copies from, straightAlphaImage, is swapchain-sized, never
     *  resolutionScale-scaled) and @p colorFormat (this execution's own
     *  swapchain-mirroring format, same as every other unconditionally-
     *  colorFormat-built target in this class). Empty (feature inactive)
     *  for every VulkanShaderFormat::shadertoy execution and for a
     *  VulkanShaderFormat::slang execution whose pass chain reflects no
     *  OriginalHistoryN texture at all — see getOriginalHistoryDepth()'s own
     *  "0 = inactive" contract. RAII — self-destructs alongside every other
     *  VulkanImage member (~VulkanShaderInstance() needs no explicit cleanup for this
     *  vector, mirroring bufferPassTargets'/imagePassGatherTarget's
     *  identical VulkanImage-ownership shape). */
    jam::Array<VulkanImage> originalHistoryImages {};

    /** @brief Write cursor into originalHistoryImages — the ring slot
     *  VulkanGraphics::recordOriginalHistoryCopy() writes THIS frame's
     *  straight-alpha snapshot into, advanced (mod
     *  originalHistoryImages.size()) at the end of that same call via
     *  setOriginalHistoryCursor(). Starts at 0 (build()'s implicit
     *  zero-init); meaningless while getOriginalHistoryDepth() == 0
     *  (originalHistoryImages stays empty, this ring is never written or
     *  read). */
    int originalHistoryCursor { 0 };

    //==========================================================================
    // VulkanMesh-backed material-range draw
    //==========================================================================

    /** @brief True once buildMeshResources() has fully succeeded — see
     *  hasMesh(). False (default) for every shader.meshPath-empty execution,
     *  and for a mesh connection whose parse/upload failed. */
    bool meshReady { false };

    /** @brief This execution's own uploaded GPU mesh — see getMesh(). Unbuilt
     *  (isValid() false) unless meshReady. */
    VulkanMesh mesh {};

    /** @brief Auto-fit MODEL matrix baked once from mesh's own AABB — see
     *  getMeshModelMatrix(). Identity until buildMeshResources() computes it. */
    glm::mat4 meshModelMatrix { 1.0f };

    /** @brief True from the moment buildMeshResources() successfully uploads
     *  mesh until releaseMeshStagingIfPending() next runs (VulkanGraphics::
     *  resetResources()'s post-fence-wait sweep) — see
     *  releaseMeshStagingIfPending()'s own doc comment. */
    bool meshStagingPendingRelease { false };

    /** @brief Dedicated descriptor pool backing meshDescriptorSet — see
     *  buildMeshGpuResources()'s own doc comment. Empty (null handle) unless
     *  meshReady. */
    vk::DescriptorPool meshDescriptorPool {};

    /** @brief This execution's own mesh descriptor set — see
     *  getMeshDescriptorSet(). Empty (null handle) unless meshReady. */
    vk::DescriptorSet meshDescriptorSet {};

    /** @brief This execution's own MVP+normal-matrix uniform buffer — see
     *  refreshMeshUniforms(). Invalid unless meshReady. */
    VulkanBuffer meshUniformBuffer {};

    /** @brief This execution's own hooked fill pipeline — see hasMeshHook()/
     *  getMeshHookFillPipeline(). Null handle unless buildMeshHookPipelines()
     *  succeeded (shader.meshShaderSource non-empty AND meshReady —
     *  buildMeshIfDeclared()'s own gate); destroyed by ~VulkanShaderInstance().
     *  Built against VulkanGraphics::getOrCreateMeshPipelineLayout() (engine-owned,
     *  shared, the SAME layout the engine-default mesh pipelines use) but the
     *  vk::Pipeline itself is per-instance — this VulkanShader's own compiled
     *  hooked SPIR-V, never engine-owned. */
    vk::Pipeline meshHookFillPipeline {};

    /** @brief This execution's own hooked transparent-fill pipeline — see
     *  hasMeshHook()/getMeshHookTransparentFillPipeline(). Same build/destroy
     *  contract as meshHookFillPipeline above. */
    vk::Pipeline meshHookTransparentFillPipeline {};

    /** @brief This execution's own hooked feature-edge overlay pipeline —
     *  see hasMeshHook()/getMeshHookEdgePipeline(). Same build/destroy
     *  contract as meshHookFillPipeline above. */
    vk::Pipeline meshHookEdgePipeline {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanShaderInstance)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam