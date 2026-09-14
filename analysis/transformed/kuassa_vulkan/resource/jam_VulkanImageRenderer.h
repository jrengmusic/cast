namespace jam
{
/*____________________________________________________________________________*/
/** @brief One VulkanImagePixelData target's complete GPU residency —
 *  MSAA colour + stencil attachments, the bindless-registered resolve
 *  texture callers sample/readback, and the framebuffer binding all three
 *  together. Owned by VulkanImageRenderer::renderTargets, keyed by the
 *  owning VulkanImagePixelData's own address. */
struct VulkanRenderTargetResources
{
    /** @brief MSAA colour attachment — resolved into resolveTexture at
     *  render-pass end. */
    VulkanImage msaaColorImage {};

    /** @brief MSAA stencil attachment for path and alpha clipping. */
    VulkanImage stencilImage {};

    /** @brief Single-sample resolve target — the bindless-registered,
     *  sampled/readback-capable image this render target exposes to callers. */
    VulkanBindlessTexture resolveTexture {};

    /** @brief Framebuffer binding msaaColorImage/stencilImage/resolveTexture
     *  together against the offscreen VulkanGraphics's own render pass. */
    vk::Framebuffer framebuffer {};
};

/*____________________________________________________________________________*/
/** @brief Engine-owned shared offscreen renderer for VulkanImagePixelData
 *  targets. Owns one offscreen-mode VulkanGraphics (no surface/swapchain/
 *  composite) and a dedicated one-shot command pool/fence for readback/
 *  writeback operations (staging transfers outside the render frame).
 *  Constructed cheaply at engine birth; initialise() runs the GPU-side setup
 *  once the device is valid. Renders one target at a time via
 *  renderTarget — synchronous, one-shot, message-thread-serialized.
 *
 *  Device-lifetime: declared after VulkanDevice in VulkanEngine, constructs
 *  after device, destructs before device.
 */
class VulkanImageRenderer
{
public:
    /** @brief Stores the device reference. Owns no Vulkan handles until
     *  initialise().
     *  @param gpuDevice  Shared Vulkan device — not owned, must outlive this VulkanImageRenderer.
     */
    explicit VulkanImageRenderer (VulkanDevice& gpuDevice) noexcept
        : device (gpuDevice)
    {
    }

    /** @brief Destroys the one-shot command pool/fence directly — the offscreen
     *  VulkanGraphics (graphics) and every VulkanRenderTargetResources framebuffer
     *  self-destruct via RAII/renderTargets' own destructor. */
    ~VulkanImageRenderer()
    {
        device.getDevice().destroyFence (oneShotFence, nullptr);
        device.getDevice().destroyCommandPool (oneShotPool, nullptr);
    }

    /** @brief Creates the offscreen VulkanGraphics plus the one-shot command
     *  pool/fence used by beginCommands()/submitAndWait() — deferred out of the
     *  constructor so the engine can call this only once its device has
     *  become valid.
     *  @param targetFrameBudgetMs  Per-frame time budget the offscreen
     *                              VulkanGraphics's own MSAA calibration measures against.
     *  @param pipelineCache        Shared vk::PipelineCache handle owned by
     *                              VulkanEngine — not owned by this object.
     *  @param maxImageExtent       Largest offscreen render-target extent this
     *                              renderer will serve.
     */
    void initialise (double targetFrameBudgetMs, vk::PipelineCache pipelineCache, vk::Extent2D maxImageExtent);

    /** @brief Destroys every device-derived handle this renderer owns (the
     *  offscreen VulkanGraphics, the one-shot command pool/fence) — the
     *  object remains reinitialisable via initialise(). Safe to call against
     *  a lost device: every step below is a destroy call, never a wait. */
    void shutdown();

    /** @brief Returns the offscreen VulkanGraphics owned by this renderer.
     *  @pre initialise() must have already succeeded.
     */
    VulkanGraphics& getGraphics() noexcept
    {
        jassert (graphics != nullptr);
        return *graphics;
    }

    /** @brief Returns the shared Vulkan device — not owned by this VulkanImageRenderer. */
    VulkanDevice& getDevice() noexcept { return device; }

    /** @brief Begins recording the one-shot command buffer with
     *  eOneTimeSubmit usage, ready for the caller to record into before
     *  submitAndWait().
     *  @return The one-shot command buffer, already in the recording state.
     */
    vk::CommandBuffer beginCommands();

    /** @brief Submits the one-shot command buffer and waits on this job's own
     *  fence only — not the whole queue. Message-thread only.
     *  @return end()'s Result immediately when it is not eSuccess (nothing was
     *          submitted); otherwise the submit's Result immediately when it
     *          is not eSuccess; otherwise the waitForFences Result — eSuccess
     *          once the fence is signalled. resetFences/reset run only after
     *          a successful wait and cannot revert that outcome — their own
     *          failure is logged, never folded into the return.
     */
    vk::Result submitAndWait();

    /** @brief Begins an offscreen frame, returns an LLGC whose destruction
     *  ends the frame (submit + fence wait).
     *  @param framebuffer   The target's own framebuffer.
     *  @param extent        The target's pixel extent.
     *  @param resolveImage  The target's resolve image — sampled by the outer pass after unwind.
     *  @param scale         Display scale factor for the LLGC's coordinate transform.
     *  @return nullptr if VulkanGraphics::beginOffscreenFrame fails — no render
     *          pass is active against @p framebuffer and no LLGC is constructed.
     */
    std::unique_ptr<VulkanLowLevelGraphicsContext> renderTarget (vk::Framebuffer framebuffer,
                                                                 vk::Extent2D extent,
                                                                 vk::Image resolveImage,
                                                                 float scale);

    /** @brief Returns the VulkanRenderTargetResources already keyed by
     *  @p key, building one via buildRenderTarget() when absent — the side
     *  effect is in the name.
     *  @param key     Identity key — the owning VulkanImagePixelData's own address.
     *  @param extent  Target pixel extent, used only when building a new entry.
     *  @return Pointer to the stored resources, or nullptr when absent and
     *          buildRenderTarget() failed (no entry is stored on failure).
     */
    VulkanRenderTargetResources* getOrCreateRenderTarget (void* key, vk::Extent2D extent);

    /** @brief Destroys the framebuffer and erases the VulkanRenderTargetResources
     *  entry keyed by @p key. No-op if absent.
     *  @param key  Identity key previously passed to getOrCreateRenderTarget().
     */
    void removeRenderTarget (void* key);

private:
    /** @brief Builds one VulkanRenderTargetResources at @p extent: MSAA colour +
     *  stencil images, the bindless-registered resolve texture, the framebuffer
     *  binding all three, then clears the resolve image (clearResolveImage())
     *  so readback before the first render pass returns defined content.
     *  @param extent  Target pixel extent.
     *  @return The built resources — framebuffer is empty on any step's failure.
     */
    VulkanRenderTargetResources buildRenderTarget (vk::Extent2D extent);

    /** @brief Clears @p resolveImage to transparent black via a one-shot
     *  command (beginCommands()/submitAndWait()) so readback before the
     *  first render pass returns defined content — MSAA colour is CLEAR-op'd
     *  by the render pass, but the resolve target is undefined until first resolve.
     *  @param resolveImage  The resolve target to clear.
     *  @return submitAndWait()'s Result.
     */
    vk::Result clearResolveImage (vk::Image resolveImage);

    /** @brief Shared Vulkan device — not owned, must outlive this VulkanImageRenderer. */
    VulkanDevice& device;

    /** @brief Offscreen VulkanGraphics owned by this renderer — no surface,
     *  swapchain, or composite; initialise()'s own construction. */
    std::unique_ptr<VulkanGraphics> graphics;

    /** @brief Per-target GPU residency, keyed by the owning VulkanImagePixelData's
     *  own address — see getOrCreateRenderTarget()/removeRenderTarget(). */
    jam::HashMap<void*, VulkanRenderTargetResources> renderTargets;

    /** @brief Command pool backing oneShotCommandBuffer. */
    vk::CommandPool oneShotPool {};

    /** @brief Single primary command buffer for beginCommands()/submitAndWait(). */
    vk::CommandBuffer oneShotCommandBuffer {};

    /** @brief CPU-side fence waited by submitAndWait() — this job's own fence,
     *  never the whole queue. */
    vk::Fence oneShotFence {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanImageRenderer)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
