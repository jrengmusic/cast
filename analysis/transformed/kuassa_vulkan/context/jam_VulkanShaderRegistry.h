namespace jam
{
/*____________________________________________________________________________*/

/** @brief Owns the runtime-shader-compiler lane's GPU execution cache
 *  (VulkanShaderInstance per VulkanShader) and every shared setup helper the
 *  lane's pipelines are built from — the shared pipeline layout, per-format
 *  offscreen render passes, the shared fullscreen vertex module, the
 *  straight-alpha/background/post-process combine pipelines, the mesh-backed
 *  material-range draw's own descriptor/pipeline layouts and pipeline
 *  variants, and the slang quad vertex buffer and per-pass sampler cache.
 *  Owned by VulkanGraphics, which threads its own frame fabric (swapchain
 *  extent/format, render passes, activeSampleCount, commandBuffer,
 *  bindlessRegistry, frameCounter) into this class's public
 *  getOrCreate*/get* surface at each call site — this class never stores a
 *  back-reference to VulkanGraphics. */
class VulkanShaderRegistry
{
public:
    //==========================================================================
    // Constructor / Destructor
    //==========================================================================

    /** @brief Stores the shared device/pipelines references. Allocates
     *  nothing — every pipeline, render pass, and cache entry is built
     *  lazily on first request.
     *  @param device     Shared Vulkan device.
     *  @param pipelines  Shared pipeline collection — supplies the bindless
     *                    texture array layout (set 1) every VulkanShader-execution
     *                    pipeline layout reuses. */
    explicit VulkanShaderRegistry (VulkanDevice& device, VulkanPipelines& pipelines);

    /** @brief Destroys every engine-owned Vulkan handle this registry built —
     *  the slang-pass sampler cache, the straight-alpha/background/post-process
     *  combine pipelines, the shared vertex module, every per-format offscreen
     *  render pass, the shared pipeline layout, and the mesh-backed
     *  material-range draw's own pipelines/layouts. shaderInstances,
     *  previousShaderInstances, and slangQuadVertexBuffer are RAII members and
     *  self-destruct; meshDefaultVertexSpirv/meshEdgeVertexSpirv are plain
     *  juce::MemoryBlock with no Vulkan handle to destroy. */
    ~VulkanShaderRegistry();

    //==========================================================================
    // VulkanShader execution — shared setup (lazy, built once per VulkanShaderRegistry lifetime)
    //==========================================================================

    /** @brief Returns the pipeline layout shared by every VulkanShader-execution
     *  pipeline (buffer passes and the Image pass alike), built lazily on
     *  first request: one set (the bindless texture array, reused verbatim
     *  from VulkanPipelines::getLayoutSet1() — its binding 1/2 linear/nearest
     *  samplers cover every VulkanShader-execution filter choice, resolved
     *  compile-time per shader; see jam_VulkanShaderUniforms.h's documented
     *  GLSL contract), plus a VulkanShaderUniforms push-constant range. No MVP set —
     *  the fullscreen technique needs no per-draw transform. */
    vk::PipelineLayout getOrCreateShaderInstanceLayout();

    /** @brief Lazily builds/returns the single-sample offscreen render pass
     *  for @p colorFormat, cached by format in shaderOffscreenRenderPasses —
     *  one render pass per distinct format ever requested this VulkanShaderRegistry
     *  instance's lifetime, never rebuilt once created. Every buffer pass's
     *  own render target may request a distinct format from its
     *  jam::VulkanShaderPreset::Pass srgb_framebufferN/float_framebufferN
     *  directive (jam_VulkanShaderPreset.h, gated there on that same ordinal
     *  also carrying a scale_type directive — RetroArch's own
     *  FBO_SCALE_FLAG_VALID, shader_vulkan.cpp:1984 +
     *  video_shader_parse.c:761-764) — this overload is VulkanShaderInstance::
     *  build()'s own per-pass render-pass factory (threaded in as a
     *  std::function<vk::RenderPass (vk::Format)>, jam_VulkanShaderInstance.h's
     *  own build() doc comment), never called with a format the caller
     *  hasn't already resolved. Single-sample colour attachment (loadOp=
     *  eDontCare — every full-overwrite fullscreen-triangle draw covers the
     *  whole target every call), identical shape to every other format's own
     *  entry, only colorAttachment.format varies; PLUS an unconditional
     *  second (depth) attachment (VulkanShaderInstance::offscreenDepthFormat,
     *  eD32Sfloat, loadOp=eClear, storeOp=eDontCare,
     *  finalLayout=eDepthStencilAttachmentOptimal, never sampled outside its
     *  own render pass's own depth test) — carried by EVERY target this
     *  render pass shape backs, meshless or not.
     *  @param colorFormat  Format the returned render pass's colour
     *                      attachment is built at.
     */
    vk::RenderPass getOrCreateShaderOffscreenRenderPass (vk::Format colorFormat);

    /** @brief Lazily builds/returns the background combine pipeline —
     *  jam_vulkan/shaders/background_combine.frag + the shared fullscreen
     *  vertex module (getOrCreateShaderPassVertModule()), targeting
     *  @p renderPass (the caller's scene render pass, MSAA at
     *  @p activeSampleCount — NOT the single-sample offscreen render pass
     *  every gather target/buffer pass targets) with
     *  VulkanPipelines::alphaBlendAttachment() (must composite over the scene
     *  content already painted beneath it). Built once, eagerly on first
     *  request, shared across every VulkanShaderInstance/VulkanShader.
     *  @param renderPass        The caller's own scene render pass.
     *  @param activeSampleCount The caller's own calibrated MSAA sample count.
     */
    vk::Pipeline getOrCreateBackgroundCombinePipeline (vk::RenderPass renderPass, vk::SampleCountFlagBits activeSampleCount);

    //==========================================================================
    // VulkanMesh-backed material-range draw
    //==========================================================================

    /** @brief Lazily builds/returns the mesh-backed material-range draw's own
     *  descriptor set layout — binding 0 (MVP+normal-matrix+viewport/edge-
     *  thickness UBO, vertex-stage-only), binding 1 (vertex-pulling SSBO,
     *  jam::VulkanVertex, vertex-stage-only), and binding 2 (feature-edge
     *  endpoint-index SSBO, vertex-stage-only — read ONLY by mesh_edge.vert).
     *  Engine-owned, built ONCE per VulkanShaderRegistry, shared across every
     *  mesh-carrying VulkanShaderInstance. */
    vk::DescriptorSetLayout getOrCreateMeshDescriptorSetLayout();

    /** @brief Lazily builds/returns the mesh-backed material-range draw's own
     *  dedicated pipeline layout — set 0 = getOrCreateMeshDescriptorSetLayout(),
     *  plus a single fragment-stage vk::PushConstantRange sized to
     *  jam::VulkanShaderInstance::MeshMaterialPushConstants. */
    vk::PipelineLayout getOrCreateMeshPipelineLayout();

    /** @brief Records @p execution's mesh-backed draws — called by
     *  VulkanLowLevelGraphicsContext::recordShaderImagePassDrawCommands()
     *  directly AFTER its own ordinary, unmodified Image-pass fullscreen
     *  draw, INTO that SAME already-active render-pass instance, gated on
     *  execution.hasMesh(). Unconditionally runs recordMeshGatherSetup() then
     *  recordMeshMaterialRangeDraws().
     *  @param execution      shader's built execution — hasMesh() must be true.
     *  @param view           jam::VulkanOrbitCamera::getViewMatrix().
     *  @param projection     jam::VulkanOrbitCamera::getProjectionMatrix (aspectRatio).
     *  @param normalMatrix   jam::VulkanOrbitCamera::getNormalMatrix().
     *  @param imageUniforms  The SAME already-stamped VulkanShaderUniforms bytes the
     *                        caller's own ordinary Image-pass fullscreen draw
     *                        just pushed.
     *  @param commandBuffer  The caller's own active primary command buffer.
     *  @param colorFormat    The caller's own swapchain colour format.
     */
    void recordMeshGatherDrawCommands (VulkanShaderInstance& execution,
                                       const glm::mat4& view,
                                       const glm::mat4& projection,
                                       const glm::mat4& normalMatrix,
                                       const VulkanShaderUniforms& imageUniforms,
                                       vk::CommandBuffer commandBuffer,
                                       vk::Format colorFormat);

    /** @brief Returns slangQuadVertexBuffer's own vk::Buffer handle — bind
     *  this at binding 0 immediately before every slang-format pass draw.
     *  Content never changes after createSlangVertexBuffer() uploads it once. */
    vk::Buffer getSlangQuadVertexBuffer() const noexcept { return slangQuadVertexBuffer.getBuffer(); }

    /** @brief Lazily compiles/returns the one shared fullscreen-triangle
     *  vertex module every VulkanShader-execution pipeline (buffer passes and the
     *  Image pass alike) binds — SSOT, one module regardless of how many
     *  Shaders are active. Public: VulkanGraphics::getOrCreateShaderInstance()
     *  (which stays in VulkanGraphics — it touches renderPassActive/
     *  endRenderPass()/resumeRenderPass(), frame fabric this registry never
     *  back-references) threads it directly into VulkanShaderInstance::build(). */
    vk::ShaderModule getOrCreateShaderPassVertModule();

    /** @brief Builds the slang quad vertex buffer — a single, VulkanShaderRegistry-
     *  owned, build-once vertex buffer shared by every slang-format
     *  VulkanShaderPass in every VulkanShader this registry ever executes
     *  (RetroArch's fixed vertex-input contract). Content is written once,
     *  right here, and never rewritten afterward.
     *  @return false if the underlying VMA buffer allocation failed. */
    bool createSlangVertexBuffer();

    /** @brief Lazily creates and returns the sampler matching @p settings'
     *  own filter_linearN/wrap_modeN preset directives (jam_VulkanShaderPreset.h)
     *  — one of up to slangPassSamplers' 8 distinct (2 filter states x 4
     *  wrap modes) compositions, cached by that composition and never
     *  rebuilt once created.
     *  @param settings  This pass's own resolved preset directives
     *                    (VulkanShaderInstance::getSlangPassSettings()).
     */
    vk::Sampler getOrCreateSlangPassSampler (const VulkanShaderPreset::Pass& settings);

    //==========================================================================
    // VulkanShader instance cache
    //==========================================================================

    /** @brief Returns the built VulkanShaderInstance for shader.
     *  @param shader  Must already be ready per VulkanGraphics::getOrCreateShaderInstance()
     *                  (mirrors getOrCreateTransparencyLayer()/getTransparencyLayer()'s
     *                  exact two-call idiom).
     */
    VulkanShaderInstance& getShaderInstance (const VulkanShader& shader);

    /** @brief Answers whether shader already has a live cached execution —
     *  the positive-check counterpart getShaderInstance() itself cannot serve
     *  (jam::HashMap::at() throws std::out_of_range on a missing key,
     *  mirroring the codebase's own established contains()-then-at() idiom
     *  at every other HashMap call site touching this exact map type).
     *  @param shader  The compiled shader to look up.
     */
    bool hasShaderInstance (const VulkanShader& shader) const noexcept;

    /** @brief Inserts or replaces shader's cached execution — retires any
     *  entry already present at this key into previousShaderInstances
     *  (deferred destroy, drained by releaseRetiredInstances()) before
     *  installing @p instance as the new live entry. Joins the register*
     *  family (VulkanBindlessRegistry::registerBindlessIndex precedent).
     *  @param shader    The compiled shader this instance was built for.
     *  @param instance  The freshly built execution to install.
     */
    void registerShaderInstance (const VulkanShader* shader, std::unique_ptr<VulkanShaderInstance> instance);

    /** @brief Reclaims every resource moved into previousShaderInstances since
     *  the last drain (releasing each entry's buffer-pass ping-pong/gather/
     *  external-texture bindless slots via @p bindlessRegistry before
     *  clearing the list), releases every LIVE entry's own pending mesh
     *  staging buffer, then sweeps shaderInstances for any live entry not
     *  re-stamped within @p swapchainImageCount frames (an owner-replaced
     *  VulkanShader's now-unreachable entry), moving each into
     *  previousShaderInstances exactly like a stale-generation rebuild.
     *  Called from the same VulkanGraphics::resetResources() point the
     *  orphan sweep occupied before this extraction.
     *  @param bindlessRegistry     The caller's own per-window bindless registry.
     *  @param frameCounter         The caller's own current (already incremented) frame index.
     *  @param swapchainImageCount  Frames-in-flight bound — the caller's own
     *                              swapchain.getImageCount(), clamped to at least one.
     *  @param nativeHandle         The caller's own getNativeHandle() — this
     *                              window's own per-texture bindless-index key.
     */
    void releaseRetiredInstances (VulkanBindlessRegistry& bindlessRegistry, uint64_t frameCounter,
                                  int swapchainImageCount, void* nativeHandle);

    //==========================================================================
    // Shared recording helpers
    //==========================================================================

    /** @brief Blits a full mip chain for @p image, level 1 through
     *  @p numMipLevels - 1, from level 0's own already-resolved content —
     *  shared by buildExternalTexture() (an external LUT/texture requesting
     *  mipmapInput) and the caller's own buffer-pass write-half (a buffer
     *  pass declaring more than one mip level).
     *  @param commandBuffer  The caller's own active primary command buffer.
     *  @param image          The image whose mip chain is generated.
     *  @param extent         Level 0's own extent.
     *  @param numMipLevels   Total mip level count, including level 0.
     */
    void recordMipChainGeneration (vk::CommandBuffer commandBuffer, vk::Image image,
                                   vk::Extent2D extent, uint32_t numMipLevels);

    /** @brief Builds/uploads ONE external LUT/texture VulkanBindlessTexture for
     *  @p texture's own path/mipmapInput directive (jam::
     *  VulkanShaderPreset::Texture). Decodes @p texture.path via
     *  juce::ImageFileFormat::loadFrom (a missing/undecodable file is logged
     *  via jam::debug::Log and skipped — graceful last-good, never aborts
     *  the caller's own build), converts to ARGB, sizes the returned texture
     *  via VulkanBindlessTexture::create(), uploads via @p stagingArena and
     *  @p commandBuffer, and — when mipmapInput requested a chain — calls
     *  recordMipChainGeneration() immediately after. Does NOT assign a
     *  bindless slot or write its descriptor — the caller does that once
     *  the returned texture's own isValid() is true.
     *  @param texture       This LUT/texture's own parsed preset directive.
     *  @param commandBuffer The caller's own active primary command buffer.
     *  @param stagingArena  The caller's own per-frame staging arena.
     *  @return The built/uploaded image — left invalid (isValid() false) on
     *          decode failure.
     */
    VulkanBindlessTexture buildExternalTexture (const VulkanShaderPreset::Texture& texture,
                                                vk::CommandBuffer commandBuffer, VulkanStagingArena& stagingArena);

    /** @brief Lazily builds/returns straightAlphaPipeline — the engine-owned
     *  straight_alpha.frag fullscreen pass that un-premultiplies the caller's
     *  own resolved scene into its own straight-alpha target. Same
     *  fullscreen-pipeline shape as VulkanShaderInstance::createFullscreenPipeline().
     *  @param colorFormat  The caller's own swapchain colour format. */
    vk::Pipeline getOrCreateStraightAlphaPipeline (vk::Format colorFormat);

    /** @brief Lazily builds/returns the post-process combine pipeline —
     *  jam_vulkan/shaders/post_process_combine.frag + the shared fullscreen
     *  vertex module, targeting @p compositeRenderPass with
     *  VulkanPipelines::opaqueBlendAttachment(). Built once, eagerly on first
     *  request, shared across every VulkanShaderInstance/VulkanShader.
     *  @param compositeRenderPass  The caller's own identity-composite render pass. */
    vk::Pipeline getOrCreatePostProcessCombinePipeline (vk::RenderPass compositeRenderPass);

private:
    //==========================================================================
    // Private setup helpers
    //==========================================================================

    /** @brief Fixed-function pipeline state shared by every engine-owned
     *  fullscreen-triangle combine/un-premultiply pipeline — straightAlphaPipeline,
     *  backgroundCombinePipeline, and postProcessCombinePipeline alike. */
    struct FullscreenPipelineFixedFunctionState
    {
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo {};
        vk::PipelineViewportStateCreateInfo viewportInfo {};
        vk::PipelineRasterizationStateCreateInfo rasterizerInfo {};
        vk::PipelineMultisampleStateCreateInfo multisampleInfo {};
        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo {};
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo {};
        vk::PipelineColorBlendAttachmentState blendAttachment {};
        std::array<vk::DynamicState, 2> dynamicStates {};
    };

    /** @brief Builds the fixed-function state common to every engine-owned
     *  fullscreen combine/un-premultiply pipeline, returned by value.
     *  The caller (createFullscreenPipeline()) keeps the returned
     *  FullscreenPipelineFixedFunctionState alive in its own stack frame for
     *  as long as vk::GraphicsPipelineCreateInfo's fields point into it
     *  (pVertexInputState, pInputAssemblyState, pViewportState,
     *  pRasterizationState, pMultisampleState, pDepthStencilState) — the
     *  returned value is the lifetime anchor for that pointer chain until
     *  createGraphicsPipelines() consumes it.
     *  @param sampleCount      MSAA sample count for the multisample state.
     *  @param blendAttachment  Blend attachment carried into the returned state. */
    FullscreenPipelineFixedFunctionState
    createFullscreenPipelineFixedFunctionState (vk::SampleCountFlagBits sampleCount,
                                                const vk::PipelineColorBlendAttachmentState& blendAttachment) const;

    /** @brief Builds one engine-owned fullscreen-triangle combine/un-premultiply
     *  pipeline from an already-created @p fragModule — the shared vertex
     *  module (getOrCreateShaderPassVertModule()), VulkanPipelines::noStencilState(),
     *  and getOrCreateShaderInstanceLayout() are constant across every caller.
     *  @param fragModule       The caller's own already-created fragment module.
     *  @param sampleCount      Forwarded to createFullscreenPipelineFixedFunctionState().
     *  @param blendAttachment  Forwarded to createFullscreenPipelineFixedFunctionState().
     *  @param renderPass       The caller's own target render pass. */
    vk::Pipeline createFullscreenPipeline (vk::ShaderModule fragModule, vk::SampleCountFlagBits sampleCount,
                                           const vk::PipelineColorBlendAttachmentState& blendAttachment,
                                           vk::RenderPass renderPass);

    //==========================================================================
    // VulkanMesh-backed material-range draw (private helpers)
    //==========================================================================

    /** @brief Lazily compiles/returns the engine's own default, unanimated
     *  mesh vertex-stage vertex-hook SPIR-V for mesh_default.vert-templated
     *  pipelines — cached once, eagerly, since this content is engine-static. */
    const juce::MemoryBlock& getOrCreateMeshDefaultVertexSpirv();

    /** @brief Lazily compiles/returns the engine's own default, unanimated
     *  mesh vertex-stage vertex-hook SPIR-V for mesh_edge.vert-templated
     *  pipelines. */
    const juce::MemoryBlock& getOrCreateMeshEdgeVertexSpirv();

    /** @brief Lazily builds/returns the engine-owned mesh pipeline, built
     *  against getOrCreateShaderOffscreenRenderPass (@p colorFormat) — the
     *  SAME unified offscreen shader render pass every buffer pass/Image-pass
     *  gather-target framebuffer targets.
     *  @param colorFormat  The caller's own swapchain colour format. */
    vk::Pipeline getOrCreateMeshPipeline (vk::Format colorFormat);

    /** @brief Lazily builds/returns the MTL-less "default material" fill
     *  variant of the mesh pipeline.
     *  @param colorFormat  The caller's own swapchain colour format. */
    vk::Pipeline getOrCreateMeshTransparentFillPipeline (vk::Format colorFormat);

    /** @brief Lazily builds/returns the whole-mesh feature-edge line-art
     *  overlay variant of the mesh pipeline.
     *  @param colorFormat  The caller's own swapchain colour format. */
    vk::Pipeline getOrCreateMeshWireframePipeline (vk::Format colorFormat);

    /** @brief Builds one engine-owned mesh pipeline variant — compiles
     *  @p vertexSpirv/@p fragFile into shader modules, builds the pipeline via
     *  createMeshPipeline() (always vk::PrimitiveTopology::eTriangleList,
     *  vk::PolygonMode::eFill — the whole-mesh feature-edge overlay's own thick-line
     *  quads still rasterize as ordinary triangles, createMeshPipelineFixedFunctionState()'s
     *  own doc comment), then destroys both modules unconditionally.
     *  @param vertexSpirv      Already-compiled vertex-hook SPIR-V (getOrCreateMeshDefaultVertexSpirv()/
     *                          getOrCreateMeshEdgeVertexSpirv()).
     *  @param fragFile         This variant's own fragment shader filename.
     *  @param blendAttachment  Forwarded to createMeshPipeline().
     *  @param colorFormat      Forwarded to createMeshPipeline(). */
    vk::Pipeline buildMeshVariantPipeline (const juce::MemoryBlock& vertexSpirv, const juce::String& fragFile,
                                           const vk::PipelineColorBlendAttachmentState& blendAttachment,
                                           vk::Format colorFormat);

    /** @brief Fixed-function pipeline state shared by every engine-owned mesh
     *  pipeline variant — topology set explicitly by
     *  createMeshPipelineFixedFunctionState(). An instance of this struct is
     *  the lifetime anchor for the vk::GraphicsPipelineCreateInfo pointer
     *  chain built by createMeshPipeline(): pVertexInputState,
     *  pInputAssemblyState, pViewportState, pRasterizationState,
     *  pMultisampleState, and pDepthStencilState all point into this
     *  instance's own fields, so it must outlive the createGraphicsPipelines()
     *  call that reads them — createMeshPipeline() holds it on its own stack
     *  frame for exactly that reason. */
    struct MeshPipelineFixedFunctionState
    {
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo {};
        vk::PipelineViewportStateCreateInfo viewportInfo {};
        vk::PipelineRasterizationStateCreateInfo rasterizerInfo {};
        vk::PipelineMultisampleStateCreateInfo multisampleInfo {};
        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo {};
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo {};
        vk::PipelineColorBlendAttachmentState blendAttachment {};
        std::array<vk::DynamicState, 2> dynamicStates {};
    };

    MeshPipelineFixedFunctionState
    createMeshPipelineFixedFunctionState (vk::PrimitiveTopology topology,
                                          vk::PolygonMode polygonMode,
                                          const vk::PipelineColorBlendAttachmentState& blendAttachment) const;

    /** @brief Builds one engine-owned mesh pipeline variant from already-
     *  created @p vertModule/@p fragModule.
     *  @param vertModule       Already-compiled vertex-stage module.
     *  @param fragModule       Already-compiled fragment-stage module.
     *  @param topology         Forwarded to createMeshPipelineFixedFunctionState().
     *  @param polygonMode      Forwarded to createMeshPipelineFixedFunctionState().
     *  @param blendAttachment  Forwarded to createMeshPipelineFixedFunctionState().
     *  @param colorFormat      The caller's own swapchain colour format. */
    vk::Pipeline createMeshPipeline (vk::ShaderModule vertModule, vk::ShaderModule fragModule,
                                     vk::PrimitiveTopology topology, vk::PolygonMode polygonMode,
                                     const vk::PipelineColorBlendAttachmentState& blendAttachment,
                                     vk::Format colorFormat);

    /** @brief Refreshes execution's mesh UBO (MVP, normal matrix, gather
     *  extent, feature-edge thickness, and imageUniforms' iMouse/iResolution/
     *  iTime/iTimeDelta/iFrame copied verbatim) and binds the state every
     *  MaterialRange draw in recordMeshMaterialRangeDraws() needs already
     *  bound — the mesh descriptor set (getOrCreateMeshPipelineLayout()'s set
     *  0), full viewport/scissor at the gather extent, and execution's own
     *  index buffer. Records no barrier, clear, or render-pass begin/end of
     *  its own — the caller's own already-active render-pass instance (the
     *  ordinary Image-pass fullscreen draw just recorded into the same
     *  framebuffer) serves as this mesh's backdrop, unchanged.
     *  @param execution      shader's built execution — hasMesh() must be true.
     *  @param view           jam::VulkanOrbitCamera::getViewMatrix().
     *  @param projection     jam::VulkanOrbitCamera::getProjectionMatrix (aspectRatio).
     *  @param normalMatrix   jam::VulkanOrbitCamera::getNormalMatrix().
     *  @param imageUniforms  The SAME already-stamped VulkanShaderUniforms bytes the
     *                        caller's own ordinary Image-pass fullscreen draw
     *                        just pushed.
     *  @param commandBuffer  The caller's own active primary command buffer.
     */
    void recordMeshGatherSetup (VulkanShaderInstance& execution, const glm::mat4& view,
                                const glm::mat4& projection, const glm::mat4& normalMatrix,
                                const VulkanShaderUniforms& imageUniforms, vk::CommandBuffer commandBuffer);

    /** @brief Draws every MaterialRange in execution's mesh — the low-alpha
     *  default-material fill for each range, then the whole-mesh
     *  feature-edge line-art overlay once, after every range's own fill draw.
     *  Each draw selects execution's own hooked pipeline
     *  (VulkanShaderInstance::getMeshHookFillPipeline() et al.) when
     *  execution.hasMeshHook() is true, or the engine-default pipeline
     *  (getOrCreateMeshPipeline()/getOrCreateMeshTransparentFillPipeline()/
     *  getOrCreateMeshWireframePipeline()) otherwise — the same rendering
     *  contract either way, only the vertex-stage animation differs.
     *  @param execution      shader's built execution, already set up by
     *                        recordMeshGatherSetup().
     *  @param commandBuffer  The caller's own active primary command buffer.
     *  @param colorFormat    The caller's own swapchain colour format.
     */
    void recordMeshMaterialRangeDraws (VulkanShaderInstance& execution, vk::CommandBuffer commandBuffer,
                                       vk::Format colorFormat);

    //==========================================================================
    // VulkanShader instance cache (private helpers)
    //==========================================================================

    /** @brief Releases every LIVE shaderInstances entry's own pending mesh
     *  staging VulkanBuffer once its build()-time upload command buffer is
     *  proven complete — releaseRetiredInstances()'s own post-fence-wait
     *  release seam. A no-op for every entry that never built a mesh, or
     *  already released one (VulkanShaderInstance::releaseMeshStagingIfPending()'s
     *  own doc comment). */
    void releaseMeshStagingBuffers();

    /** @brief Sweeps shaderInstances for any live entry not re-stamped within
     *  @p swapchainImageCount frames of @p frameCounter (an owner-replaced
     *  VulkanShader's now-unreachable entry), moving each into
     *  previousShaderInstances exactly like a stale-generation rebuild —
     *  releaseRetiredInstances()'s own orphan sweep.
     *  @param frameCounter         The caller's own current (already incremented) frame index.
     *  @param swapchainImageCount  Frames-in-flight bound — the caller's own
     *                              swapchain.getImageCount(), clamped to at least one.
     */
    void retireOrphanedInstances (uint64_t frameCounter, int swapchainImageCount);

    //==========================================================================
    // Members
    //==========================================================================

    VulkanDevice& device;
    VulkanPipelines& pipelines;

    /** @brief Per-VulkanShader GPU execution cache, keyed by VulkanShader instance identity
     *  and validated against VulkanShader::contentHash/scaled extent — see
     *  VulkanGraphics::getOrCreateShaderInstance(). A HashMap since more than
     *  one VulkanShader instance may execute concurrently against one window
     *  (background + post-process). */
    jam::HashMap<const VulkanShader*, std::unique_ptr<VulkanShaderInstance>> shaderInstances {};

    /** @brief Prior-generation ShaderInstances kept alive until the GPU
     *  finishes the frame(s) that referenced them — mirrors
     *  VulkanStagingArena's previousStagingBuffers's exact deferred-destroy
     *  pattern. Drained by releaseRetiredInstances(). */
    jam::Owner<VulkanShaderInstance> previousShaderInstances {};

    /** @brief Shared pipeline layout for every VulkanShader-execution pipeline
     *  (buffer passes and the Image pass alike) — built lazily by
     *  getOrCreateShaderInstanceLayout(). */
    vk::PipelineLayout shaderInstanceLayout {};

    /** @brief Single-sample offscreen render passes every buffer-pass
     *  framebuffer targets, keyed by that render pass's own colour format
     *  (static_cast<int> (vk::Format)) — built lazily, one entry per distinct
     *  format ever requested, by getOrCreateShaderOffscreenRenderPass (vk::Format). */
    jam::HashMap<int, vk::RenderPass> shaderOffscreenRenderPasses {};

    /** @brief Shared fullscreen-triangle vertex module for every VulkanShader-
     *  execution pipeline — built lazily by getOrCreateShaderPassVertModule(). */
    vk::ShaderModule shaderPassVertModule {};

    /** @brief The single, VulkanShaderRegistry-owned, build-once vertex buffer
     *  holding VulkanShaderInstance::slangQuadVertices' 4 vertices — the
     *  static fullscreen quad every slang-format VulkanShaderPass's own real
     *  vertex stage consumes (RetroArch's fixed vertex-input contract).
     *  Built exactly once by createSlangVertexBuffer() and bound — never
     *  rewritten — at every slang-pass draw call site. RAII — self-destructs,
     *  no manual cleanup in ~VulkanShaderRegistry(). */
    VulkanBuffer slangQuadVertexBuffer {};

    /** @brief The straight-alpha un-premultiply pipeline (shader_pass.vert +
     *  straight_alpha.frag, single-sample, VulkanPipelines::opaqueBlendAttachment()),
     *  built lazily by getOrCreateStraightAlphaPipeline (vk::Format). */
    vk::Pipeline straightAlphaPipeline {};

    /** @brief The background mode's combine pipeline (shader_pass.vert +
     *  background_combine.frag, MSAA at the caller's own activeSampleCount,
     *  VulkanPipelines::alphaBlendAttachment()), built lazily by
     *  getOrCreateBackgroundCombinePipeline() — static engine GLSL, never
     *  per-instance-derived, so built once and shared across every
     *  VulkanShaderInstance/VulkanShader for this registry's lifetime. */
    vk::Pipeline backgroundCombinePipeline {};

    /** @brief The post-process mode's combine pipeline (shader_pass.vert +
     *  post_process_combine.frag, single-sample, VulkanPipelines::opaqueBlendAttachment()),
     *  built lazily by getOrCreatePostProcessCombinePipeline() — same
     *  shared-once convention as backgroundCombinePipeline above. */
    vk::Pipeline postProcessCombinePipeline {};

    /** @brief VulkanMesh-backed material-range draw's own descriptor set layout —
     *  see getOrCreateMeshDescriptorSetLayout(). */
    vk::DescriptorSetLayout meshDescriptorSetLayout {};

    /** @brief VulkanMesh-backed material-range draw's own dedicated pipeline
     *  layout — see getOrCreateMeshPipelineLayout(). */
    vk::PipelineLayout meshPipelineLayout {};

    /** @brief The engine-owned mesh pipeline — see getOrCreateMeshPipeline(). */
    vk::Pipeline meshPipeline {};

    /** @brief The engine-owned MTL-less default-material transparent-fill
     *  pipeline — see getOrCreateMeshTransparentFillPipeline(). */
    vk::Pipeline meshTransparentFillPipeline {};

    /** @brief The engine-owned whole-mesh feature-edge line-art overlay
     *  pipeline — see getOrCreateMeshWireframePipeline(). */
    vk::Pipeline meshWireframePipeline {};

    /** @brief The engine's own default, unanimated mesh_default.vert-
     *  templated vertex-hook SPIR-V — see getOrCreateMeshDefaultVertexSpirv().
     *  Cached once, engine-static content (never per-VulkanShader-derived). */
    juce::MemoryBlock meshDefaultVertexSpirv {};

    /** @brief The engine's own default, unanimated mesh_edge.vert-templated
     *  vertex-hook SPIR-V — see getOrCreateMeshEdgeVertexSpirv(). Same
     *  caching contract as meshDefaultVertexSpirv above. */
    juce::MemoryBlock meshEdgeVertexSpirv {};

    /** @brief Per-slang-pass sampler cache, keyed by that pass's own resolved
     *  (filter_linearN, wrap_modeN) composition (jam_VulkanShaderPreset.h) —
     *  up to 2 filter states x 4 wrap modes = 8 distinct samplers, lazily
     *  created by getOrCreateSlangPassSampler() the first time a given
     *  composition is requested, never rebuilt afterward. Destroyed in
     *  ~VulkanShaderRegistry(). */
    jam::HashMap<int, vk::Sampler> slangPassSamplers {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanShaderRegistry)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
