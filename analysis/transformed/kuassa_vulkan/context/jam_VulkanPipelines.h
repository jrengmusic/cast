namespace jam
{
/*____________________________________________________________________________*/
/** @brief Owns the 24 vk::Pipeline + shared vk::PipelineLayout. The
 *  vk::PipelineCache itself is owned by VulkanEngine (SSOT) and passed in
 *  to load() — not owned or destroyed by this collection.
 *
 *  Created after VulkanGraphics::createRenderPass() (pipelines need a render pass).
 *  Destroyed in reverse order: pipelines → layout → descriptor set
 *  layouts → shader modules.
 */
class VulkanPipelines
{
public:
    /** @brief Logical pipeline identity — index into `pipelines[]`. */
    enum class ID : uint8_t
    {
        opaqueRectInstanced              = 0,
        alphaBlendRectInstanced           = 1,
        imageInstanced                    = 2,
        tiledImage                        = 3,   // UNUSED — reserved slot
        glyphMonoInstanced                = 4,
        glyphEmojiInstanced               = 5,
        background                        = 6,   // UNUSED — reserved slot
        stencilWrite                      = 7,   // UNUSED — reserved slot (replaced by stencilWriteTriList = 15)
        opaqueRectInstancedStencil        = 8,
        alphaBlendRectInstancedStencil    = 9,
        imageInstancedStencil             = 10,
        glyphMonoInstancedStencil         = 11,
        glyphEmojiInstancedStencil        = 12,
        opaqueTriList           = 13,   // fillPath: triangle list, quad vertex input, opaque blend
        alphaBlendTriList       = 14,   // fillPath: triangle list, quad vertex input, alpha blend
        stencilWriteTriList     = 15,   // clipToPath stencil write with triangle list topology
        opaqueTriListStencil    = 16,   // fillPath inside clipToPath region: stencil-test + opaque blend
        alphaBlendTriListStencil = 17,  // fillPath inside clipToPath region: stencil-test + alpha blend
        clipMaskInstanced       = 18,  // clipToImageAlpha stencil write, alpha-test against image alpha channel
        windingStencilAccumulate = 19, // Isolated scratch-target winding accumulate — per-subpath fan, INCR_WRAP/DECR_WRAP, colorWriteMask=0
        windingCoverOpaque      = 20,  // Winding cover, bbox quad, opaque blend (blend disabled, direct write — see recordWindingAccumulateAndCoverDrawCommands()'s doc comment for why the cover draw is always opaque)
        gradientFill            = 21, // Linear/radial LUT-sampled gradient fill (shaders/gradient_fill.frag), single transformed quad — same draw shape as opaqueRectInstanced/alphaBlendRectInstanced
        gradientFillStencil     = 22, // gradientFill inside a stencil-clip region: stencil-test + alpha blend
        maskedImageInstancedStencil = 23, // Masked image draw inside a matte clip (clipToImageAlpha region): stencil-test + alpha blend + per-pixel mask-alpha multiply (shaders/masked_image.frag)
    };

    static constexpr uint8_t pipelineCount = 24;

    /** @brief Logical compute pipeline identity — index into `computePipelines[]`.
     *  Stack-blur pair: stackBlurTexture reads through the bindless sampled-image
     *  array (shaders/stack_blur_texture.comp), stackBlurBuffer reads the transposed
     *  output of the first pass from its own storage buffer
     *  (shaders/stack_blur_buffer.comp) — two dispatches per VulkanImageEffects::blurImage().
     *  Matte kernels: matteChoke — morphological erosion (shaders/matte_choke.comp);
     *  matteFeather — distance-field feather (shaders/matte_feather.comp). */
    enum class ComputeID : uint8_t
    {
        stackBlurTexture = 0,
        stackBlurBuffer  = 1,
        matteChoke       = 2, ///< Morphological erosion (sliding-window minimum) — shaders/matte_choke.comp.
        matteFeather     = 3, ///< Distance-field feather — shaders/matte_feather.comp.
    };

    static constexpr uint8_t computePipelineCount = 4;

    /** @brief VulkanVertex count for every fullscreen-triangle draw call —
     *  the shared vertex stage (shaders/shader_pass.vert, and
     *  shaders/calibration.vert's documented identical technique) generates
     *  its triangle purely from gl_VertexIndex, zero vertex buffers, so every
     *  draw of it (buffer passes and the Image pass in
     *  VulkanGraphics::recordShaderBufferPasses()/recordPostProcessCompositeDrawCommands()
     *  (jam_VulkanGraphicsSlangPass.cpp),
     *  VulkanLowLevelGraphicsContext::recordShaderImagePassDrawCommands()
     *  (jam_VulkanLowLevelGraphicsContextRender.cpp), and the MSAA calibration
     *  probe draw (VulkanMsaaCalibration::measureCandidateSampleCount(),
     *  jam_VulkanMsaaCalibrationMeasurement.cpp)) issues this same count,
     *  one instance, zero first vertex/instance (SSOT). */
    static constexpr uint32_t fullscreenTriangleVertexCount { 3 };

    /** @brief Builds all descriptor set layouts, the pipeline layout,
     *         and the 24 graphics pipelines against a caller-owned cache.
     *
     *  @param device                    Logical Vulkan device (must outlive this object).
     *  @param renderPass                Render pass the pipelines target.
     *  @param extent                    Swapchain extent for the viewport/scissor state.
     *  @param bindlessTextureCapacity   Effective array bound for set 1 binding 0 — the
     *                                   hardware-clamped `VulkanGraphics::maxBindlessTextures`
     *                                   (the queried Vulkan 1.2 descriptor-indexing
     *                                   capabilities are hard requirements for this engine,
     *                                   asserted at device creation — no runtime branch here).
     *  @param sampleCount               `VulkanDevice::getActiveSampleCount()`, the
     *                                   session-locked MSAA sample count. Read by every
     *                                   pipeline's `multisampleState()` call (SSOT) — must
     *                                   match `renderPass`'s color/depth-stencil attachment
     *                                   sample counts (Vulkan render-pass-compatibility rule).
     *  @param pipelineCache             Shared vk::PipelineCache handle owned by VulkanEngine —
     *                                   not owned by this object.
     *  @return true on success; false on any Vulkan failure — VulkanGraphics::create()
     *          returns nullptr in that case, and JUCE renders via its native/
     *          software renderer instead.
     */
    bool load (vk::Device device,
               vk::RenderPass renderPass,
               vk::Extent2D extent,
               uint32_t bindlessTextureCapacity,
               vk::SampleCountFlagBits sampleCount,
               vk::PipelineCache pipelineCache);

    /** @brief Destroys all Vulkan objects owned by this collection. The
     *  pipeline cache itself is not destroyed — VulkanEngine owns it.
     *  @param device  Logical Vulkan device used at load() time.
     */
    void shutdown (vk::Device device);

    /** @brief Indexed pipeline access by logical ID. */
    vk::Pipeline operator[] (ID id) const
    {
        return pipelines[static_cast<uint8_t> (id)];
    }

    /** @brief Shared pipeline layout (set 0 = empty, set 1 = bindless sampled-image
     *         array + single shared sampler, set 2 = readonly VulkanPrimitiveRecord storage
     *         buffer, set 3 = projection storage buffer). */
    vk::PipelineLayout getLayout() const { return layout; }

    /** @brief Set 3 descriptor layout (projection storage buffer at binding 0). */
    vk::DescriptorSetLayout getProjectionLayout() const { return projectionLayout; }
    /** @brief Set 1 descriptor layout — binding 0 is a `texture2D[]` sampled-image
     *         array (size = the `bindlessTextureCapacity` passed to load()), binding 1 is a
     *         single shared linear `sampler` (VulkanGraphics's one `linearSampler` covers every
     *         image — no per-image filter variation exists, so a sampler array would be
     *         YAGNI), binding 2 is a single shared nearest `sampler` (VulkanGraphics's one
     *         `nearestSampler`) — a VulkanShader-execution pass's compile-time-resolved resample mode
     *         (map::ImageResample::value, resolved by jam::VulkanShaderCompiler, never stored on VulkanShader
     *         itself) samples through binding 1 or binding 2 directly in its
     *         own GLSL (see jam_VulkanShaderUniforms.h's documented GLSL contract); this
     *         same layout is reused verbatim as set 0 of
     *         VulkanGraphics::getOrCreateShaderInstanceLayout().
     *         Bindings 3/4 are `sampler2D[]` combined-image-sampler arrays exposing the
     *         SAME image views as binding 0, pre-paired with the linear (binding 3) /
     *         nearest (binding 4) sampler respectively — a first-class passable sampler2D
     *         value user-shader GLSL can hand to its own helper functions (glslang rejects
     *         the binding-0 + binding-1/2 constructor form as a function argument); see
     *         jam::VulkanShaderCompiler::channelMacros()/sceneMacro().
     *         Backs the one persistent bindless-array set (VulkanGraphics::bindlessTextureDescriptorSet)
     *         bound by every draw — no per-call descriptor set is ever allocated against
     *         this layout. */
    vk::DescriptorSetLayout getLayoutSet1() const { return samplerLayout; }
    /** @brief Set 2 descriptor layout (readonly VulkanPrimitiveRecord storage buffer at binding 0, vertex stage). */
    vk::DescriptorSetLayout getLayoutSet2() const { return storageLayout; }

    /** @brief Compute pipeline layout (set 0 = empty, set 1 = bindless sampled-image
     *         array + shared samplers — reused verbatim from the graphics layout so
     *         shaders/bindless_texture.glsl's `set = 1` declarations resolve identically
     *         for compute, set 2 = computeStorageLayout, set 3 = projection storage buffer). Push range is `eCompute`-only,
     *         20 bytes (VulkanMatteFeatherPushConstants), independent of
     *         the graphics layout's sharedPushConstantRangeSize. */
    vk::PipelineLayout getComputeLayout() const noexcept { return computeLayout; }

    /** @brief Set 2 descriptor layout for compute — two `eStorageBuffer` bindings
     *         (0 = read, 1 = write), `eCompute` stage. Mirrors getLayoutSet2()'s role
     *         for the graphics set 2 storage layout. */
    vk::DescriptorSetLayout getComputeLayoutSet2() const { return computeStorageLayout; }

    /** @brief Indexed compute pipeline access by logical ComputeID. */
    vk::Pipeline getComputePipeline (ComputeID id) const noexcept
    {
        return computePipelines[static_cast<uint8_t> (id)];
    }

    vk::ShaderModule getShaderModule (const juce::String& name) const
    {
        if (shaderModules.contains (name))
            return shaderModules.at (name);

        return {};
    }

    /** @brief Compiles a SPIR-V shader module. Public (not member-state-dependent) so
     *         VulkanGraphics's own one-off pipeline setup (createCompositePipeline(),
     *         measureCandidateSampleCount()) shares this ONE implementation instead of
     *         each hand-rolling an identical local lambda. */
    static vk::ShaderModule createShaderModule (vk::Device device,
                                                const char* bytes,
                                                int sizeBytes);

    /** @brief Depth/stencil state with every test/write disabled. Public (not
     *         member-state-dependent) so VulkanShaderInstance::createFullscreenPipeline()
     *         shares this ONE definition — user-shader draws never touch stencil,
     *         and the scene render pass's stencil attachment requires an explicit
     *         pDepthStencilState on every pipeline built against it. */
    static vk::PipelineDepthStencilStateCreateInfo noStencilState();

    /** @brief Opaque colour blend attachment state (blendEnable == false,
     *         full RGBA colorWriteMask — a direct, unblended write). Public
     *         (not member-state-dependent) so VulkanShaderInstance::createFullscreenPipeline()
     *         shares this ONE definition — every VulkanRenderResources-backed
     *         pipeline this execution owns draws with this, unconditionally:
     *         buffer passes, and the Image pass's own offscreen gather
     *         target (both the background and post-process modes,
     *         VulkanShaderInstance::imagePassGatherTarget) — a full overwrite is
     *         correct for all three, since each writes its own isolated
     *         offscreen image every draw with nothing painted beneath it to
     *         blend against. */
    static vk::PipelineColorBlendAttachmentState opaqueBlendAttachment();

    /** @brief Premultiplied-over blend attachment state (one /
     *         one-minus-srcAlpha colour, one / one-minus-srcAlpha alpha) — every
     *         consumer's fragment shader is required to output colour already
     *         scaled by its own alpha.
     *         Public (not member-state-dependent) — used by VulkanPipelines' own
     *         main 24-pipeline collection build, and by
     *         VulkanShaderRegistry::getOrCreateBackgroundCombinePipeline()
     *         (jam_VulkanShaderRegistry.cpp): the background mode's
     *         dedicated combine pass (background_combine.frag) draws into
     *         the scene render pass and must composite over whatever the
     *         scene already painted beneath it, unlike the in-scene VulkanImage
     *         pass's own gather draw, which still targets its isolated
     *         offscreen gather target with opaqueBlendAttachment() like
     *         every other VulkanRenderResources target (nothing painted beneath
     *         it there to alpha-blend against). Not consumed by
     *         VulkanShaderInstance::createFullscreenPipeline() itself — every
     *         VulkanRenderResources-backed pipeline that class builds stays
     *         opaqueBlendAttachment(), only the two VulkanGraphics-owned combine
     *         pipelines choose per their own mode's contract. */
    static vk::PipelineColorBlendAttachmentState alphaBlendAttachment();

private:
    // ---- Pipeline state (owned) ----
    vk::PipelineLayout layout {};
    vk::PipelineCache cache {};
    vk::Pipeline pipelines[pipelineCount] {};

    // ---- Compute pipeline state (owned) ----
    /** @brief Shared compute pipeline layout — see getComputeLayout()'s doc comment. */
    vk::PipelineLayout computeLayout {};
    /** @brief The computePipelineCount compute pipelines, indexed by ComputeID. */
    vk::Pipeline computePipelines[computePipelineCount] {};

    // ---- Descriptor set layouts (owned — destroyed on shutdown) ----
    /** @brief Set 0 — zero-binding placeholder. Descriptor set indices in
     *  vk::PipelineLayoutCreateInfo are positional and contiguous: set 1 is
     *  samplerLayout, set 2 storageLayout, set 3 projectionLayout (see
     *  getLayout()) — none of them sit at index 0, so an otherwise-unused
     *  layout with no bindings is required to fill that slot. Never allocated
     *  a descriptor set of its own and never bound. */
    vk::DescriptorSetLayout emptyLayout {};
    /** @brief Set 3 descriptor layout — see getProjectionLayout(). */
    vk::DescriptorSetLayout projectionLayout {};
    vk::DescriptorSetLayout samplerLayout {};
    vk::DescriptorSetLayout storageLayout {};
    /** @brief Set 2 descriptor layout for compute — two `eStorageBuffer` bindings
     *  (0 = read, 1 = write), `eCompute` stage — see getComputeLayoutSet2(). */
    vk::DescriptorSetLayout computeStorageLayout {};

    // ---- Shader modules (owned — destroyed on shutdown) ----
    jam::HashMap<juce::String, vk::ShaderModule> shaderModules {};

    // ---- Load steps ----
    bool createDescriptorSetLayouts (vk::Device device, uint32_t bindlessTextureCapacity);
    bool createPipelineLayout (vk::Device device);
    bool loadShaderModules (vk::Device device);
    bool createGraphicsPipelines (vk::Device device, vk::RenderPass renderPass, vk::Extent2D extent,
                                  vk::SampleCountFlagBits sampleCount);
    bool createComputeStorageLayout (vk::Device device);
    bool createComputePipelineLayout (vk::Device device);
    bool createComputePipelines (vk::Device device);

    // ---- Static state constructors (build vk::*CreateInfo structs) ----
    /** @brief VulkanVertex input state for VertexInput::position2D — single vec2 position
     *  attribute, used exclusively by triangulated path draws (fillPath/clipToPath). */
    static vk::PipelineVertexInputStateCreateInfo position2DVertexInputState();
    /** @brief VulkanVertex input state for VertexInput::instanced — zero bindings/attributes;
     *  all per-primitive data is pulled from the VulkanPrimitiveRecord SSBO via gl_InstanceIndex. */
    static vk::PipelineVertexInputStateCreateInfo instancedVertexInputState();
    static vk::PipelineInputAssemblyStateCreateInfo triangleStripInputAssemblyState();
    static vk::PipelineInputAssemblyStateCreateInfo triangleListInputAssemblyState();
    static vk::PipelineViewportStateCreateInfo viewportState (vk::Extent2D extent);
    static vk::PipelineRasterizationStateCreateInfo rasterizationState();
    /** @brief Reads the caller-supplied session-locked MSAA sample count
     *  (VulkanDevice::getActiveSampleCount()) instead of a hardcoded single sample. Shared
     *  by 23 of the 24 real pipelines (SSOT) — see initializeFixedFunctionState().
     *
     *  @param sampleCount       Session-locked MSAA sample count (VulkanDevice::getActiveSampleCount()).
     *  @param alphaToCoverage   Sets vk::PipelineMultisampleStateCreateInfo::alphaToCoverageEnable.
     *         Only ID::clipMaskInstanced passes true (build.multisampleAlphaToCoverage) —
     *         image_alpha_mask.frag's `discard`-gated single per-fragment alpha decision
     *         is broadcast identically to every MSAA sample by default, leaving the
     *         subsequent stencil write with no sub-pixel coverage at the mask edge.
     *         Alpha-to-coverage converts the shader's output alpha (location 0) into a
     *         per-sample coverage mask instead, so the stencil write applies only to
     *         covered samples — restoring MSAA at the clipToImageAlpha boundary. Every
     *         other pipeline passes false and shares build.multisample. */
    static vk::PipelineMultisampleStateCreateInfo multisampleState (vk::SampleCountFlagBits sampleCount,
                                                                    bool alphaToCoverage);
    static vk::PipelineDepthStencilStateCreateInfo stencilWriteState();
    static vk::PipelineDepthStencilStateCreateInfo stencilTestState();
    /** @brief Isolated scratch-target winding accumulate: stencilTestEnable,
     *  compareOp ALWAYS (no prior gating value — the scratch target is freshly cleared
     *  every complex fillPath() call), front passOp INCREMENT_AND_WRAP / back passOp
     *  DECREMENT_AND_WRAP (classic stencil winding-number accumulation; cullMode is NONE
     *  so both facings of every per-subpath fan triangle rasterize). */
    static vk::PipelineDepthStencilStateCreateInfo windingAccumulateState();
    /** @brief Isolated scratch-target winding cover: stencilTestEnable,
     *  compareOp NOT_EQUAL against reference 0. compareMask is set DYNAMICALLY per draw
     *  (0xFF selects nonzero winding, 0x01 selects even-odd parity — both derived from
     *  the SAME windingAccumulateState() output, see
     *  VulkanLowLevelGraphicsContext::recordWindingAccumulateAndCoverDrawCommands()'s doc
     *  comment for the parity-preserving reasoning). */
    static vk::PipelineDepthStencilStateCreateInfo windingCoverState();
    static vk::PipelineColorBlendStateCreateInfo colorBlendState (const vk::PipelineColorBlendAttachmentState& attachment);

    // ---- Data-driven pipeline construction ----

    /** @brief One of the distinct (vertex, fragment) shader stage pairs reused across
     *         the 24 pipelines. Indexes PipelineBuildState::stagesByPair.
     *
     *  `instancedRectOpaque`/`instancedRectAlpha`/`windingCover`
     *  and `triListOpaque`/`triListAlpha`/`windingStencil` used to be six separate
     *  slots, split only because `fill_rect_alpha.frag` was a distinct shader module
     *  from `fill_rect.frag`. `fill_rect_alpha.frag` was byte-identical to
     *  `fill_rect.frag` (verified via diff) and was deleted as its own SSOT violation
     *  — every former alpha-module consumer now points at `fill_rect.frag`, which
     *  collapses each trio down to one (vertex, fragment) pair:
     *  - `instancedRect` — `instanced_rect.vert` + `fill_rect.frag` (the shared
     *    `instanced.vert` minus its unconsumed `vTextureIndex` output). Serves the former
     *    `instancedRectOpaque`/`instancedRectAlpha` (rect fill, opaque and alpha-blend
     *    — Blend is already an orthogonal PipelineSpec axis) and the former
     *    `windingCover` (`windingCoverOpaque`'s bbox-quad cover draw — identical
     *    instanced/TRIANGLE_STRIP geometry shape, see pipelineSpecs).
     *  - `triList` — `fill_rect.vert` + `fill_rect.frag`. Serves the former
     *    `triListOpaque`/`triListAlpha` (triangulated path draws — the earlier stopgap
     *    `path`/`pathAlpha`, needed because `instancedRectOpaque`/`instancedRectAlpha`'s
     *    vertex stage moved to the shared `instanced.vert`, but path draws stay on the
     *    position2D vertex-attribute shape) and the former `windingStencil`
     *    (`windingStencilAccumulate`'s per-subpath fan draw — fragment output discarded,
     *    colorWriteMask=0 via Blend::none, identical position2D/TRIANGLE_LIST shape).
     *  - `maskedImage` — `instanced_masked.vert` + `masked_image.frag`; the only pair
     *    whose vertex stage outputs `vMaskTextureIndex` and whose fragment stage multiplies
     *    by the mask slot's alpha; serves ID::maskedImageInstancedStencil only.
     *
     *  Blend::opaque on `windingCoverOpaque` (StagePair::instancedRect) means blend
     *  disabled/direct write, not alpha forced to 1: the cover draw's target is always
     *  a freshly LOAD_OP_CLEAR'd, single-draw-into-empty scratch image, so alpha-blending
     *  it would double-apply the fill alpha against the later composite draw's own blend
     *  (see recordWindingAccumulateAndCoverDrawCommands()'s doc comment) — there is no
     *  correct use for an alpha-blended cover variant. */
    enum class StagePair : uint8_t
    {
        instancedRect       = 0,
        instancedImage      = 1,
        tiled               = 2,
        instancedGlyphMono  = 3,
        instancedGlyphEmoji = 4,
        background          = 5,
        instancedClipMask   = 6,
        triList             = 7,
        gradientFill        = 8,
        maskedImage         = 9,
    };

    static constexpr uint8_t stagePairCount = 10;

    /** @brief Selects the colour blend attachment state a pipeline draws with. */
    enum class Blend : uint8_t { opaque, alpha, none };

    /** @brief Selects the depth-stencil state a pipeline draws with.
     *
     *  `windingAccumulate` (INCR_WRAP/DECR_WRAP, compareOp ALWAYS — no prior
     *  gating value, the scratch target is freshly cleared every fillPath() complex-path
     *  call) and `windingCover` (compareOp NOT_EQUAL against the accumulated value,
     *  reference/compareMask set dynamically — see recordWindingAccumulateAndCoverDrawCommands())
     *  are distinct from the main clip-depth stencil's `stencilWrite`/`stencilTest` — the
     *  isolated scratch target never shares state with the main clip-depth stencil attachment. */
    enum class DepthStencil : uint8_t { noStencil, stencilWrite, stencilTest, windingAccumulate, windingCover };

    /** @brief Selects the vertex input binding/attribute layout a pipeline consumes.
     *
     *  `position2D` (renamed from `quad`) remains the vertex-attribute shape, now used
     *  ONLY by triangulated path geometry (2-float position). `instanced` (replaces
     *  the removed `glyph`) is the SSBO-pulled, zero-vertex-attribute shape shared by every
     *  unified quad primitive (rect fill, image, glyph, clip mask). */
    enum class VertexInput : uint8_t { position2D, instanced };

    /** @brief Declarative row describing one of the 24 graphics pipelines — every
     *         axis of variation expressed as data instead of imperative per-ID
     *         overrides. Table index (see pipelineSpecs) == static_cast<uint8_t> (id).
     *
     *  `alphaToCoverage` is explicit on every row (no default) — the table is
     *  exhaustive by design (see the pipelineSpecs doc comment). Only
     *  ID::clipMaskInstanced sets it true; see multisampleState()'s doc comment
     *  for why clipMaskInstanced alone needs alpha-to-coverage. */
    struct PipelineSpec
    {
        ID id;
        StagePair stagePair;
        Blend blend;
        DepthStencil depthStencil;
        vk::PrimitiveTopology topology;
        VertexInput vertexInput;
        bool alphaToCoverage;
    };

    /** @brief One row per VulkanPipelines::ID, in ID order. Cross-checked field-for-field
     *         against the pre-refactor per-ID vk::GraphicsPipelineCreateInfo overrides.
     *
     *  Every rect/image/glyph/clip-mask row moved to VertexInput::instanced
     *  (SSBO-pulled). Path rows (opaqueTriList/alphaBlendTriList/stencilWriteTriList/
     *  opaqueTriListStencil/alphaBlendTriListStencil) stay VertexInput::position2D,
     *  unchanged, and are repointed to StagePair::triList (see the StagePair doc comment
     *  above — the former triListOpaque/triListAlpha split collapsed into one slot).
     *  Former VertexInput::glyph rows (glyphMonoInstanced/glyphEmojiInstanced,
     *  glyphMonoInstancedStencil/glyphEmojiInstancedStencil, background) also changed
     *  topology TRIANGLE_LIST -> TRIANGLE_STRIP:
     *  the old topology existed only to support indexed 6-index-per-quad draws against
     *  the glyph VulkanFrameBuffer; instanced draws issue 4 non-indexed vertices per instance,
     *  which TRIANGLE_LIST cannot represent as a complete quad (a strip is required).
     *
     *  windingStencilAccumulate is a triangle-LIST/position2D row (per-subpath
     *  fan geometry, CPU-expanded to a triangle list — NOT a triangle fan,
     *  which MoltenVK/Metal do not support natively; see fillComplexPath()'s
     *  appendFanTriangulatedRing()), repointed to StagePair::triList (same
     *  bytecode as the path rows above). windingCoverOpaque is an instanced/TRIANGLE_STRIP
     *  bbox-quad row, repointed to StagePair::instancedRect (same bytecode
     *  as opaqueRectInstanced). Always Blend::opaque — see the ID::windingCoverOpaque
     *  enumerator's doc comment and recordWindingAccumulateAndCoverDrawCommands()'s doc
     *  comment for why an alpha-blended cover variant was deleted as never-correct
     *  (double-alpha fix). */
    static constexpr PipelineSpec pipelineSpecs[pipelineCount]
    {
        { ID::opaqueRectInstanced,             StagePair::instancedRect,       Blend::opaque, DepthStencil::noStencil,    vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::alphaBlendRectInstanced,         StagePair::instancedRect,       Blend::alpha,  DepthStencil::noStencil,    vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::imageInstanced,                  StagePair::instancedImage,      Blend::alpha,  DepthStencil::noStencil,    vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::tiledImage,                      StagePair::tiled,               Blend::opaque, DepthStencil::noStencil,    vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::glyphMonoInstanced,              StagePair::instancedGlyphMono,  Blend::alpha,  DepthStencil::noStencil,    vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::glyphEmojiInstanced,             StagePair::instancedGlyphEmoji, Blend::alpha,  DepthStencil::noStencil,    vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::background,                      StagePair::background,         Blend::opaque, DepthStencil::noStencil,    vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::stencilWrite,                    StagePair::instancedRect,       Blend::none,   DepthStencil::stencilWrite, vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::opaqueRectInstancedStencil,      StagePair::instancedRect,       Blend::opaque, DepthStencil::stencilTest,  vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::alphaBlendRectInstancedStencil,  StagePair::instancedRect,       Blend::alpha,  DepthStencil::stencilTest,  vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::imageInstancedStencil,           StagePair::instancedImage,      Blend::alpha,  DepthStencil::stencilTest,  vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::glyphMonoInstancedStencil,       StagePair::instancedGlyphMono,  Blend::alpha,  DepthStencil::stencilTest,  vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::glyphEmojiInstancedStencil,      StagePair::instancedGlyphEmoji, Blend::alpha,  DepthStencil::stencilTest,  vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::opaqueTriList,                   StagePair::triList,             Blend::opaque, DepthStencil::noStencil,    vk::PrimitiveTopology::eTriangleList,  VertexInput::position2D, false },
        { ID::alphaBlendTriList,               StagePair::triList,             Blend::alpha,  DepthStencil::noStencil,    vk::PrimitiveTopology::eTriangleList,  VertexInput::position2D, false },
        { ID::stencilWriteTriList,             StagePair::triList,             Blend::none,   DepthStencil::stencilWrite, vk::PrimitiveTopology::eTriangleList,  VertexInput::position2D, false },
        { ID::opaqueTriListStencil,            StagePair::triList,             Blend::opaque, DepthStencil::stencilTest,  vk::PrimitiveTopology::eTriangleList,  VertexInput::position2D, false },
        { ID::alphaBlendTriListStencil,        StagePair::triList,             Blend::alpha,  DepthStencil::stencilTest,  vk::PrimitiveTopology::eTriangleList,  VertexInput::position2D, false },
        // alphaToCoverage = true — the ONLY row that sets it. clipToImageAlpha's stencil
        // write follows image_alpha_mask.frag's per-fragment `discard`; without A2C that
        // single alpha decision is broadcast to all MSAA samples, so the stencil-write
        // edge gets no sub-pixel coverage. See multisampleState()'s doc comment.
        { ID::clipMaskInstanced,               StagePair::instancedClipMask,  Blend::none,   DepthStencil::stencilWrite, vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  true  },
        { ID::windingStencilAccumulate,        StagePair::triList,            Blend::none,   DepthStencil::windingAccumulate, vk::PrimitiveTopology::eTriangleList,  VertexInput::position2D, false },
        { ID::windingCoverOpaque,              StagePair::instancedRect,      Blend::opaque, DepthStencil::windingCover,      vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::gradientFill,                    StagePair::gradientFill,       Blend::alpha,  DepthStencil::noStencil,         vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::gradientFillStencil,             StagePair::gradientFill,       Blend::alpha,  DepthStencil::stencilTest,       vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
        { ID::maskedImageInstancedStencil,     StagePair::maskedImage,        Blend::alpha,  DepthStencil::stencilTest,       vk::PrimitiveTopology::eTriangleStrip, VertexInput::instanced,  false },
    };

    /** @brief Index-enum sweep — names initializeDynamicState()'s raw
     *  build.dynamicStates[] slots (each index is a fixed, specific dynamic state —
     *  never a homogeneous/interchangeable loop). Cardinality is dynamicStateCount (SSOT). */
    enum class DynamicState : uint8_t
    {
        viewport           = 0,
        scissor            = 1,
        stencilCompareMask = 2,
        stencilWriteMask   = 3,
        stencilReference   = 4
    };

    static constexpr uint8_t dynamicStateCount = 5;

    /** @brief Fixed-function + per-axis state shared by all 24 pipelines, built
     *         once per createGraphicsPipelines() call. Members pointed-to by
     *         other members (pDynamicStates, pAttachments) live here — never
     *         copy/move this instance after initializePipelineBuildState() fills it. */
    struct PipelineBuildState
    {
        vk::RenderPass renderPass;
        vk::PipelineViewportStateCreateInfo viewport;
        vk::PipelineRasterizationStateCreateInfo rasterizer;
        vk::PipelineMultisampleStateCreateInfo multisample;
        // alphaToCoverageEnable = true variant — ID::clipMaskInstanced only,
        // see multisampleState()'s doc comment for why.
        vk::PipelineMultisampleStateCreateInfo multisampleAlphaToCoverage;
        std::array<vk::DynamicState, dynamicStateCount> dynamicStates;
        vk::PipelineDynamicStateCreateInfo dynamicState;
        vk::PipelineVertexInputStateCreateInfo vertexInputByMode[2];
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStrip;
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyList;
        vk::PipelineDepthStencilStateCreateInfo depthStencilByMode[5];
        vk::PipelineColorBlendAttachmentState opaqueAttachment;
        vk::PipelineColorBlendAttachmentState alphaAttachment;
        vk::PipelineColorBlendAttachmentState noColorAttachment;
        vk::PipelineColorBlendStateCreateInfo colorBlendByMode[3];
        vk::PipelineShaderStageCreateInfo stagesInstancedRect[2];
        vk::PipelineShaderStageCreateInfo stagesInstancedImage[2];
        vk::PipelineShaderStageCreateInfo stagesTiled[2];
        vk::PipelineShaderStageCreateInfo stagesInstancedGlyphMono[2];
        vk::PipelineShaderStageCreateInfo stagesInstancedGlyphEmoji[2];
        vk::PipelineShaderStageCreateInfo stagesBackground[2];
        vk::PipelineShaderStageCreateInfo stagesInstancedClipMask[2];
        vk::PipelineShaderStageCreateInfo stagesTriList[2];
        vk::PipelineShaderStageCreateInfo stagesGradientFill[2];
        vk::PipelineShaderStageCreateInfo stagesMaskedImage[2];
        const vk::PipelineShaderStageCreateInfo* stagesByPair[stagePairCount];
    };

    /** @brief Fills `build` with the fixed-function state shared by all 24
     *         pipelines, keyed off shaderModules (already loaded by
     *         loadShaderModules() by the time this runs). Orchestrates the
     *         per-section initializers below in sequence.
     *  @param build      Out-param — see PipelineBuildState for lifetime rules.
     *  @param renderPass Render pass the pipelines target.
     *  @param extent     Swapchain extent for the viewport/scissor state.
     *  @param sampleCount Session-locked MSAA sample count (VulkanGraphics::
     *                     activeSampleCount), threaded through exactly like extent.
     */
    void initializePipelineBuildState (PipelineBuildState& build,
                                       vk::RenderPass renderPass,
                                       vk::Extent2D extent,
                                       vk::SampleCountFlagBits sampleCount) const;

    /** @brief Fills build.viewport, build.rasterizer, build.multisample, and
     *         build.multisampleAlphaToCoverage. */
    void initializeFixedFunctionState (PipelineBuildState& build, vk::Extent2D extent,
                                       vk::SampleCountFlagBits sampleCount) const;

    /** @brief Fills build.vertexInputByMode and build.inputAssemblyStrip/List. */
    void initializeVertexAndTopologyState (PipelineBuildState& build) const;

    /** @brief Fills build.depthStencilByMode and the blend attachments + build.colorBlendByMode. */
    void initializeDepthStencilAndBlendState (PipelineBuildState& build) const;

    /** @brief Fills build.dynamicStates and build.dynamicState. */
    void initializeDynamicState (PipelineBuildState& build) const;

    /** @brief Fills the 10 build.stagesXxx shader-stage-pair arrays and build.stagesByPair. */
    void initializeShaderStages (PipelineBuildState& build) const;

    /** @brief Assembles one vk::GraphicsPipelineCreateInfo from a PipelineSpec row
     *         plus the shared/per-axis state built once per load() call. */
    vk::GraphicsPipelineCreateInfo buildPipelineCreateInfo (const PipelineSpec& spec,
                                                            const PipelineBuildState& build) const;
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam