namespace jam
{
/*____________________________________________________________________________*/
class VulkanMsaaCalibration
{
public:
    //==========================================================================
    // Constructor
    //==========================================================================

    /** @brief Stores the device reference. Owns no Vulkan handles until createTimestampPool().
     *  @param device  Shared Vulkan device — not owned, must outlive this VulkanMsaaCalibration.
     */
    explicit VulkanMsaaCalibration (VulkanDevice& device);

    //==========================================================================
    // Destructor
    //==========================================================================

    /** @brief Destroys timestampPool — unconditional (an empty handle is a
     *  no-op per Vulkan spec). */
    ~VulkanMsaaCalibration();

    //==========================================================================
    // MSAA calibration
    //==========================================================================

    /** @brief Creates the 2-slot timestampPool used exclusively by calibrateSampleCount(). */
    bool createTimestampPool();

    /** @brief Runs during create(), right after swapchain.create()/
     *  createCommandPool()/createTimestampPool(), BEFORE createRenderPass()/
     *  pipelines.load() (a deliberate ordering fix — see create()'s doc
     *  comment). Adopts VulkanDevice's already-calibrated sample count
     *  directly if one exists. Otherwise skips measurement and defaults
     *  deterministically (documented fallback, not silent) if
     *  VulkanDevice::isTimestampComputeAndGraphics() reports unsupported:
     *  vk::SampleCountFlagBits::e4 on Apple platforms (memoryless transient
     *  resolve is near-free on TBDR GPUs), vk::SampleCountFlagBits::e2 elsewhere.
     *  Otherwise measures each descending candidate via
     *  measureCandidateSampleCount() and locks activeSampleCount to the first
     *  candidate whose measured time fits targetFrameBudgetMs, publishing the
     *  result onto the device — VulkanDevice::getActiveSampleCount() is the
     *  one truth every caller reads afterward.
     *  @param targetFrameBudgetMs  Caller-supplied per-frame budget (see create()).
     *  @param swapchainFormat      The caller's own swapchain surface format.
     *  @param swapchainExtent      The caller's own swapchain pixel extent.
     *  @param commandPool          The caller's own command pool for the throwaway
     *                              calibration command buffers.
     */
    void calibrateSampleCount (double targetFrameBudgetMs,
                               vk::SurfaceFormatKHR swapchainFormat,
                               vk::Extent2D swapchainExtent,
                               vk::CommandPool commandPool);

private:
    //==========================================================================
    // Named constants
    //==========================================================================

    /** @brief timestampPool query slot count: one begin + one end
     *  timestamp per calibrateSampleCount() measurement draw. */
    static constexpr uint32_t timestampPoolQueryCount { 2 };

    /** @brief Index-enum sweep — names measureCandidateSampleCount()'s begin/end
     *  timestamp query slots (vk::CommandBuffer::writeTimestamp's query index, and the matching
     *  timestamps[] result array) instead of raw 0/1 literals. Cardinality is
     *  timestampPoolQueryCount (SSOT). */
    enum class Timestamp : uint32_t
    {
        begin = 0,
        end = 1
    };

    /** @brief Iterations discarded as GPU/driver warm-up before averaging
     *  calibrateSampleCount()'s per-candidate timing samples (pipeline/descriptor
     *  state is not yet warm on the first submissions). */
    static constexpr int calibrationWarmupIterations { 2 };

    /** @brief Iterations averaged into calibrateSampleCount()'s measured
     *  time per MSAA sample-count candidate, after calibrationWarmupIterations
     *  is discarded. */
    static constexpr int calibrationMeasuredIterations { 3 };

    //==========================================================================
    // Private setup helpers
    //==========================================================================

    /** @brief Measures one MSAA sample-count candidate's realistic
     *  resolve cost. FULLY self-contained by deliberate design: builds
     *  its own throwaway transient MSAA-color + single-sample-resolve render pass/
     *  framebuffer, its own EMPTY vk::PipelineLayout (zero descriptor set layouts,
     *  zero push-constant ranges), and its own throwaway vk::Pipeline using the new
     *  calibration.vert/frag shader pair — the vertex shader generates a single
     *  fullscreen triangle purely from gl_VertexIndex, so the draw needs no vertex
     *  buffer, no UBO, no push constants, no descriptor sets. Zero dependency on
     *  VulkanPipelines/VulkanGraphics::createDrawState() (mirrors the device-capability query's
     *  own precedent — a capability-discovery step that runs fully before anything
     *  commits to using the answer). Submits calibrationWarmupIterations + calibrationMeasuredIterations
     *  synchronous one-draw command buffers (cmd.draw (VulkanPipelines::fullscreenTriangleVertexCount, 1, 0, 0)) bracketed by
     *  vk::CommandBuffer::writeTimestamp, and averages the post-warm-up samples into averageMs via
     *  VulkanDevice::getTimestampPeriod(). All throwaway Vulkan objects (pipeline, empty
     *  layout, render pass, framebuffer, images) are destroyed before returning —
     *  nothing here is retained on the VulkanGraphics instance. Orchestrates the private
     *  createCalibrationTargets()/createCalibrationPipelineSetup()/
     *  measureCalibrationIterations()/destroyCalibrationResources() steps below —
     *  see each step's own doc comment for its slice of the above.
     *  @param candidate        MSAA sample count to measure.
     *  @param swapchainFormat  The caller's own swapchain surface format.
     *  @param swapchainExtent  The caller's own swapchain pixel extent.
     *  @param commandPool      The caller's own command pool for the throwaway
     *                          calibration command buffers.
     *  @return The average measured milliseconds if every throwaway resource
     *          was created and the measurement submitted/read back
     *          successfully, otherwise an empty optional.
     */
    std::optional<double> measureCandidateSampleCount (vk::SampleCountFlagBits candidate,
                                                        vk::SurfaceFormatKHR swapchainFormat,
                                                        vk::Extent2D swapchainExtent,
                                                        vk::CommandPool commandPool);

    /** @brief measureCandidateSampleCount() step 1 — builds the throwaway transient
     *  MSAA color image and its single-sample resolve image. Lazily-allocated
     *  (memoryless) transient memory exists only on tile-based GPUs (Apple Silicon
     *  qualifies) — probes support via vmaFindMemoryTypeIndexForImageInfo() rather
     *  than assuming it; desktop GPUs with no such memory type fail the allocation
     *  outright (VMA doc), so the probe result selects VMA_MEMORY_USAGE_GPU_ONLY as
     *  a fallback. The returned MSAA color image stays hand-filled rather than
     *  VulkanImage::create2D(): the probe needs direct access to the built
     *  vk::ImageCreateInfo/VmaAllocationCreateInfo structs BEFORE construction, and
     *  VulkanImage::create2D() hides those intermediate structs. The returned resolve
     *  image has no such probe requirement and uses VulkanImage::create2D() directly.
     *  @param candidate        MSAA sample count under measurement.
     *  @param swapchainFormat  The caller's own swapchain surface format.
     *  @param swapchainExtent  The caller's own swapchain pixel extent.
     *  @return The created transient MSAA color image paired with the
     *          created single-sample resolve image — check isValid() on
     *          each; either may be empty on failure.
     */
    std::pair<VulkanImage, VulkanImage> createCalibrationTargets (vk::SampleCountFlagBits candidate,
                                                                   vk::SurfaceFormatKHR swapchainFormat,
                                                                   vk::Extent2D swapchainExtent) const;

    /** @brief measureCandidateSampleCount() step 2 — builds every throwaway Vulkan
     *  object the measurement draw needs: the 2-attachment render pass (MSAA color
     *  attachment 0 auto-resolved into the single-sample resolve image attachment 1
     *  via pResolveAttachments), the matching framebuffer, an EMPTY pipeline layout
     *  (zero descriptor set layouts, zero push-constant ranges — calibration.vert/
     *  frag reads nothing but gl_VertexIndex, so this measurement has no dependency
     *  on VulkanPipelines' real bindless/SSBO-backed layout — the structural fix for the
     *  ordering conflict described above: calibrateSampleCount() can now run before
     *  VulkanGraphics::createRenderPass()/pipelines.load()), and the throwaway pipeline
     *  itself (calibration.vert/frag, no vertex input state, no pDepthStencilState —
     *  this subpass references no depth/stencil attachment, so the field is ignored
     *  per Vulkan spec §9.2). Does NOT clean up on partial failure — every created
     *  handle is returned at its real value (or empty if never reached);
     *  the caller's unconditional destroyCalibrationResources() call handles
     *  cleanup for exactly the subset that was actually created.
     *  @param candidate        MSAA sample count under measurement.
     *  @param msaaColorImage   The transient MSAA color image (createCalibrationTargets()).
     *  @param resolveImage     The single-sample resolve image (createCalibrationTargets()).
     *  @param swapchainFormat  The caller's own swapchain surface format.
     *  @param swapchainExtent  The caller's own swapchain pixel extent.
     *  @return Success flag, the created vk::RenderPass, vk::Framebuffer, empty
     *          vk::PipelineLayout, vertex shader module, fragment shader module,
     *          and vk::Pipeline, in that order. On a false success flag, every
     *          handle reached before the failing step is still populated (or
     *          empty if never reached) — the caller's unconditional
     *          destroyCalibrationResources() call handles cleanup for exactly
     *          the subset that was actually created.
     */
    std::tuple<bool, vk::RenderPass, vk::Framebuffer, vk::PipelineLayout, vk::ShaderModule, vk::ShaderModule, vk::Pipeline>
        createCalibrationPipelineSetup (vk::SampleCountFlagBits candidate,
                                        const VulkanImage& msaaColorImage,
                                        const VulkanImage& resolveImage,
                                        vk::SurfaceFormatKHR swapchainFormat,
                                        vk::Extent2D swapchainExtent) const;

    /** @brief measureCandidateSampleCount() step 3 — records, submits, and reads
     *  back ONE calibration draw: a single fullscreen triangle generated entirely
     *  in calibration.vert from gl_VertexIndex (no vertex buffer, no descriptor
     *  sets, no push constants bound at all), bracketed by vk::CommandBuffer::writeTimestamp at
     *  vk::PipelineStageFlagBits::eTopOfPipe/eBottomOfPipe. Synchronous — blocks on
     *  a per-draw vk::Fence (one-time init cost, acceptable at calibration time) before reading
     *  timestampPool back.
     *  @param targetRenderPass The throwaway calibration render pass.
     *  @param framebuffer     The throwaway calibration framebuffer.
     *  @param pipeline        The throwaway calibration pipeline.
     *  @param swapchainExtent The caller's own swapchain pixel extent.
     *  @param commandPool     The caller's own command pool for the throwaway
     *                         calibration command buffer.
     *  @return The begin/end timestamp query results if the command buffer was
     *          allocated, submitted, and read back successfully; an empty
     *          optional (caller breaks its loop) if allocation failed.
     */
    std::optional<std::array<uint64_t, timestampPoolQueryCount>> recordAndMeasureCalibrationDraw (
        vk::RenderPass targetRenderPass, vk::Framebuffer framebuffer, vk::Pipeline pipeline,
        vk::Extent2D swapchainExtent, vk::CommandPool commandPool) const;

    /** @brief measureCandidateSampleCount() step 4 — runs calibrationWarmupIterations
     *  + calibrationMeasuredIterations calls to recordAndMeasureCalibrationDraw(),
     *  discarding the warm-up iterations (pipeline/descriptor state is not yet warm
     *  on the first submissions) and accumulating the rest, then converts the
     *  averaged tick delta to milliseconds via VulkanDevice::getTimestampPeriod().
     *  @param targetRenderPass The throwaway calibration render pass.
     *  @param framebuffer  The throwaway calibration framebuffer.
     *  @param pipeline     The throwaway calibration pipeline.
     *  @param swapchainExtent The caller's own swapchain pixel extent.
     *  @param commandPool     The caller's own command pool for the throwaway
     *                         calibration command buffers.
     *  @return The average measured milliseconds if calibrationMeasuredIterations
     *          iterations all completed, otherwise an empty optional.
     */
    std::optional<double> measureCalibrationIterations (vk::RenderPass targetRenderPass,
                                                         vk::Framebuffer framebuffer,
                                                         vk::Pipeline pipeline,
                                                         vk::Extent2D swapchainExtent,
                                                         vk::CommandPool commandPool) const;

    /** @brief measureCandidateSampleCount() step 5 — destroys every throwaway
     *  Vulkan object it may have created, in reverse creation order. Safe to call
     *  with any subset still empty — every destroy call on device is a
     *  documented no-op against an empty handle, so the caller passes exactly the
     *  handles createCalibrationPipelineSetup() actually populated.
     *  @param targetRenderPass The throwaway calibration render pass, or empty.
     *  @param framebuffer     The throwaway calibration framebuffer, or empty.
     *  @param pipelineLayout  The throwaway empty pipeline layout, or empty.
     *  @param pipeline        The throwaway calibration pipeline, or empty.
     *  @param vertModule      The throwaway vertex shader module, or empty.
     *  @param fragModule      The throwaway fragment shader module, or empty.
     */
    void destroyCalibrationResources (vk::RenderPass targetRenderPass,
                                      vk::Framebuffer framebuffer,
                                      vk::PipelineLayout pipelineLayout,
                                      vk::Pipeline pipeline,
                                      vk::ShaderModule vertModule,
                                      vk::ShaderModule fragModule) const;

    //==========================================================================
    // Members
    //==========================================================================

    /** @brief Shared Vulkan device — not owned, must outlive this VulkanMsaaCalibration. */
    VulkanDevice& device;

    /** @brief GPU timestamp query pool used exclusively by
     *  calibrateSampleCount()'s measurement draws (2 slots: begin/end),
     *  reset and reused once per measured iteration. */
    vk::QueryPool timestampPool {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanMsaaCalibration)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
