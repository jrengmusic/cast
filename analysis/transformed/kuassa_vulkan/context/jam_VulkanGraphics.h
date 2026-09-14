namespace jam
{
/*____________________________________________________________________________*/

#if JUCE_MAC
/** @brief Creates a CAMetalLayer for the given NSView and returns it as void*.
 *  Implemented in jam_vulkan.mm.
 *  @param nsView  The NSView pointer (as void*).
 *  @return Opaque CAMetalLayer pointer.
 */
void* createMetalLayerForView (void* nsView);

/** @brief Returns the backing scale factor for the given NSView's window.
 *  Implemented in jam_vulkan.mm.
 *  @param nsView  The NSView pointer (as void*).
 *  @return Scale factor (2.0 on Retina, 1.0 otherwise).
 */
float getScaleFactor (void* nsView);

/** @brief Updates the CAMetalLayer frame and drawable size for resize.
 *  Implemented in jam_vulkan.mm.
 *  @param layer   The CAMetalLayer pointer (as void*).
 *  @param width   Logical width in points.
 *  @param height  Logical height in points.
 *  @param scale   Display scale factor (backingScaleFactor).
 */
void updateMetalLayerFrame (void* layer, int width, int height, float scale);

// Current CAMetalLayer frame size in points — tracks the backing NSView's
// bounds automatically via CALayer autoresizing (kCALayerWidthSizable |
// kCALayerHeightSizable, set at layer creation), independent of the last
// width/height passed to updateMetalLayerFrame(). Implemented in jam_vulkan.mm.
jam::Size<int> getMetalLayerSize (void* layer);
#endif

/*____________________________________________________________________________*/
/** @brief Owns the complete Vulkan rendering pipeline for a single window surface.
 *
 *  Created via the static create() factory — constructor is public but
 *  construction-by-factory is convention, not access-enforced. Owns the
 *  WSI presentation cluster (VulkanSwapchain), render passes, command pool,
 *  sync objects, all draw state buffers, glyph atlas images, the per-frame
 *  path VulkanFrameBuffer, the per-frame VulkanPrimitiveRecordBuffer SSBO,
 *  and the winding scratch target.
 *
 *  One VulkanGraphics per juce::ComponentPeer. Lifecycle: create() → beginFrame() /
 *  endFrame() per paint cycle → destructor. resize() recreates the swapchain.
 *
 *  Multiple LLGCs share a single command buffer per frame; the active count is
 *  tracked via incrementContextCount() and decremented internally by endFrame().
 *
 *  Owns bindlessRegistry (jam::VulkanBindlessRegistry) for per-window
 *  bindless array slot allocation and juce::ImagePixelData::Listener-driven
 *  slot recycling — registered per cached root at slot-assign time
 *  (cacheImageTexture(), via bindlessRegistry.registerBindlessIndex()); see
 *  that class's own doc comment. This class remains the orchestrator: it
 *  assigns/releases through bindlessRegistry and writes every descriptor
 *  itself (writeBindlessTextureDescriptor()).
 */
class VulkanGraphics
{
public:
    //==========================================================================
    // Named constants
    //==========================================================================

    /** @brief Maximum nesting depth of reentrant offscreen recordings —
     *  the ceiling on activeRecordings.size() at any instant. */
    static constexpr uint32_t maxRecordingDepth { 4 };

    /** @brief Per-epoch target-instance capacity — the maximum number of
     *  distinct beginOffscreenFrame calls within a single depth-0 epoch
     *  (between consecutive depth-0 begins). Each call takes one Recording
     *  entry from the pool by append order (recordingsUsed), never sharing
     *  an entry with a sibling at the same depth. */
    static constexpr uint32_t maxRecordingsPerFrame { 16 };

    /** @brief Maximum number of descriptor sets in the shared descriptor pool.
     *  Per recording: 1 projection storage-buffer set (set 3) + 1 VulkanPrimitiveRecord
     *  storage-buffer set (set 2) at begin, plus 1 fresh set 2 at resume
     *  when returning to an outer recording. Total per depth-0 epoch:
     *  maxRecordingsPerFrame set3s + 2 * maxRecordingsPerFrame set2s
     *  = 3 * maxRecordingsPerFrame. Set 1 (bindless texture array) is never
     *  allocated from this pool — it lives in its own persistent
     *  bindlessDescriptorPool. */
    static constexpr uint32_t maxDescriptorSets { maxRecordingsPerFrame * 3 };

    /** @brief Byte size of the projection storage buffer (one mat4). */
    static constexpr vk::DeviceSize projectionBufferSize { sizeof (float) * 16 };

    /** @brief Number of floats per path vertex (position only). */
    static constexpr int pathVertexStride { 2 };

    /** @brief Initial element capacity for the path per-frame VulkanFrameBuffer. */
    static constexpr int initialPathCapacity { 4096 };

    /** @brief Initial element capacity for the per-frame VulkanPrimitiveRecordBuffer SSBO. */
    static constexpr int initialPrimitiveRecordCapacity { 1024 };

    /** @brief Aspirational bindless sampled-image array bound — 2 glyph
     *  atlases (mono + emoji) plus generous headroom for a multi-tab image cache.
     *  Slots recycle via bindlessRegistry.releaseBindlessIndex() — every
     *  currently-live, distinct source image holds at most one slot at a time. Exhaustion asserts
     *  (design bug — capacity is sized for the full static image set plus
     *  dynamic roots). Clamped at init
     *  against the hardware-queried ceiling
     *  (VulkanDevice::getMaxDescriptorSetUpdateAfterBindSampledImages()) into
     *  bindlessTextureCapacity — see VulkanGraphics::create(). */
    static constexpr uint32_t maxBindlessTextures { 128 };

    /** @brief Count of sampled-image-typed bindless arrays sharing set 1's layout —
     *  binding 0 (`eSampledImage`), binding 3, and binding 4 (both `eCombinedImageSampler`,
     *  VulkanPipelines::createDescriptorSetLayouts()'s doc comment). Per the Vulkan spec,
     *  `eCombinedImageSampler` counts against `maxDescriptorSetUpdateAfterBindSampledImages`
     *  the same as `eSampledImage` — all three arrays draw from the same hardware-queried
     *  budget, so bindlessTextureCapacity below divides that budget by this count rather
     *  than assuming binding 0 alone consumes it. */
    static constexpr uint32_t bindlessSampledImageArrayCount { 3 };

    //==========================================================================
    // Destructor
    //==========================================================================

    /** @brief Waits the device idle, then destroys all Vulkan objects —
     *  swapchain's own destructor orders its WSI teardown per spec (views,
     *  then semaphores, then swapchain, then surface), while the native
     *  window is still alive. */
    ~VulkanGraphics();

    //==========================================================================
    // Constructor
    //==========================================================================

    /** @brief Stores the device reference, constructs bindlessInstance against
     *  @p nativeHandle, constructs bindlessRegistry against
     *  bindlessInstance.getHandle() and @p bindlessCapacity, and
     *  zero-initialises all remaining Vulkan handles. Called only
     *  by create() — construction-by-factory is convention, not access-enforced.
     *  @param vulkanDevice     Shared Vulkan device.
     *  @param nativeHandle     Platform window handle (NSView* on macOS, HWND on
     *                          Windows) — threaded straight into bindlessInstance's
     *                          own ctor rather than assigned post-construction.
     *  @param bindlessCapacity  Session-locked bindless array bound
     *                          (maxBindlessTextures clamped against
     *                          VulkanDevice::getMaxDescriptorSetUpdateAfterBindSampledImages(),
     *                          computed by create()/createOffscreen() before
     *                          this object exists) — threaded straight into
     *                          bindlessRegistry's own ctor and stored as
     *                          bindlessTextureCapacity.
     */
    explicit VulkanGraphics (VulkanDevice& vulkanDevice, void* nativeHandle, uint32_t bindlessCapacity);

    //==========================================================================
    // Static factory
    //==========================================================================

    /** @brief Creates a fully initialised VulkanGraphics for the given native window handle.
     *
     *  Runs every createXxx() helper in sequence; returns nullptr if any step fails.
     *  calibrateSampleCount() runs, by deliberate ordering, right
     *  after swapchain.create()/createCommandPool()/createTimestampPool(), BEFORE
     *  createRenderPass()/pipelines.load(): its measurement draw is fully self-
     *  contained (own render pass, framebuffer, empty pipeline layout, calibration.
     *  vert/frag pipeline — see measureCandidateSampleCount()'s doc comment), so it
     *  has zero dependency on the real VulkanPipelines/createDrawState() objects.
     *  createRenderPass() then builds the MAIN render pass ONCE, correctly, at the
     *  now-known activeSampleCount — no rebuild, no double-build. createRenderPass()
     *  now also builds compositeRenderPass (the identity-composite pass targeting the
     *  swapchain directly); createSceneTarget() (scene MSAA color + resolve images)
     *  runs before it so createFramebuffers() can build sceneFramebuffer;
     *  createCompositePipeline() runs after pipelines.load()/createDrawState() (needs
     *  the real pipeline layout + bindless/SSBO descriptor sets to exist).
     *  createStraightAlphaTarget() (straightAlphaImage + straightAlphaFramebuffer,
     *  the post-process path's un-premultiplied scene target) runs right after
     *  createSceneTarget(), for the same reason and at the same extent.
     *
     *  @param device               Shared Vulkan device — not owned, must outlive this VulkanGraphics.
     *  @param nativeHandle         Platform window handle (NSView* on macOS, HWND on Windows).
     *  @param width                Initial surface width in pixels.
     *  @param height               Initial surface height in pixels.
     *  @param targetFrameBudgetMs  Per-frame time budget calibrateSampleCount() measures
     *                              MSAA candidates against — caller-supplied explicitly
     *                              (host application, refresh-rate-driven; no hidden default in KANJUT).
     *  @param pipelineCache        Shared vk::PipelineCache handle owned by VulkanEngine —
     *                              not owned by this object. Threaded into pipelines.load().
     *  @return Unique pointer to a ready-to-use VulkanGraphics, or nullptr on failure.
     */
    static std::unique_ptr<VulkanGraphics> create (VulkanDevice& device,
                                                   void* nativeHandle,
                                                   int width,
                                                   int height,
                                                   double targetFrameBudgetMs,
                                                   vk::PipelineCache pipelineCache
#if JUCE_WINDOWS
                                                   ,
                                                   bool opaquePeer
#endif
    );

    /** @brief Creates an offscreen VulkanGraphics for synchronous one-shot
     *  rendering into caller-owned framebuffers (VulkanImagePixelData targets).
     *  The create() flow minus surface/swapchain/composite/present/timestamp-
     *  per-frame-present machinery. The constructed instance's own address
     *  serves as the identity key for bindless registration (no window handle
     *  exists). Render pass uses fixed eB8G8R8A8Unorm format. Sample count
     *  calibrated identically to the window path. Pipelines loaded against the
     *  offscreen render pass. Depth-0 submit via offscreenFence.
     *  @param device               Shared Vulkan device.
     *  @param targetFrameBudgetMs  Per-frame time budget for MSAA calibration.
     *  @param pipelineCache        Shared vk::PipelineCache handle owned by VulkanEngine —
     *                              not owned by this object.
     *  @param maxImageExtent       Largest offscreen render-target extent this
     *                              engine's VulkanImageRenderer will serve (the
     *                              layout-declared full design editor size); seeds
     *                              calibrateSampleCount's calibration probe and
     *                              transparency-stack extent. Explicit — no hidden
     *                              default; the caller owns the layout-declared value.
     *  @return Unique pointer to a ready-to-use offscreen VulkanGraphics, or
     *          nullptr on failure.
     */
    static std::unique_ptr<VulkanGraphics> createOffscreen (VulkanDevice& device,
                                                            double targetFrameBudgetMs,
                                                            vk::PipelineCache pipelineCache,
                                                            vk::Extent2D maxImageExtent);

    //==========================================================================
    // Frame lifecycle
    //==========================================================================

    /** @brief Acquires a swapchain image and begins recording the command buffer.
     *  Idempotent — no-op if a frame is already active (multiple LLGCs per paint cycle).
     *  @return true if the frame is active (already begun or just begun successfully).
     */
    bool beginFrame();

    /** @brief Decrements the active LLGC count.
     *  Ends the render pass, submits, and presents only when the last LLGC for this
     *  frame is done (count reaches zero). All LLGCs in a paint cycle share one command
     *  buffer; only the last destruction triggers submit + present.
     */
    void endFrame();

    /** @brief Begins a render pass using LOAD_OP_CLEAR against @p framebuffer, sized to
     *  @p extent. SSOT for "begin a fresh CLEAR-op render pass against a given
     *  framebuffer/extent, mark the render pass active" — shared by the main-scene
     *  overload below and recordWindingAccumulateAndCoverDrawCommands() (the isolated
     *  winding-scratch pass, sized to VulkanWindingScratch's own capacity extent rather
     *  than swapchainExtent — mirrors resumeRenderPass(vk::Framebuffer)'s exact
     *  generalization shape). Idempotent — no-op if a render pass is already active.
     *  @param framebuffer  Target to clear-begin — sceneFramebuffer, or
     *                      VulkanWindingScratch's own framebuffer.
     *  @param extent       Render area extent — swapchainExtent for the main scene, or
     *                      the scratch target's own capacity extent.
     */
    void beginRenderPass (vk::Framebuffer framebuffer, vk::Extent2D extent);

    /** @brief Begins the main render pass (CLEAR_OP) on the current swapchain framebuffer.
     *  Idempotent — no-op if the render pass is already active. Called once by the
     *  factory before the first LLGC is constructed; subsequent calls for the same
     *  frame are no-ops. Thin call into the beginRenderPass (vk::Framebuffer, vk::Extent2D)
     *  overload above.
     */
    void beginRenderPass();

    /** @brief Ends the current render pass.
     *  Idempotent — no-op if no render pass is active. Called internally by endFrame()
     *  on the last LLGC destruction.
     */
    void endRenderPass();

    /** @brief Resumes a render pass using LOAD_OP_LOAD against @p framebuffer,
     *  sized to @p extent — continues drawing onto existing pixels rather than
     *  clearing them (LOAD_OP_LOAD is the mechanism; "resume" is the semantic).
     *  SSOT for "resume a render pass against a given framebuffer/extent, mark
     *  the render pass active" — shared by the main-scene overload below,
     *  beginOffscreenTransparencyRenderPass() (entering an offscreen transparency
     *  target fresh), and endTransparencyLayer() (resuming a parent transparency
     *  level's own framebuffer after closing a nested child level). Idempotent —
     *  no-op if a render pass is already active.
     *  @param framebuffer  Target to resume — sceneFramebuffer, or a
     *                      VulkanTransparencyStack::TransparencyLayer's own framebuffer.
     *  @param extent       Render area extent — swapchainExtent for the main scene, or
     *                      the resumed target's own extent.
     */
    void resumeRenderPass (vk::Framebuffer framebuffer, vk::Extent2D extent);

    /** @brief Resumes the render pass against the context-appropriate target:
     *  offscreen (offscreenInstance) resumes against the active recording's
     *  own framebuffer/extent (Recording::framebuffer, Recording::extent,
     *  written at beginOffscreenFrame push time); windowed resumes against
     *  sceneFramebuffer/swapchainExtent. Thin call into the
     *  resumeRenderPass (vk::Framebuffer, vk::Extent2D) overload above.
     *  Idempotent — no-op if the render pass is already active.
     */
    void resumeRenderPass();

    /** @brief Begins the scene render pass — resumes with LOAD and a one-time stencil clear when persisted scene content is valid, otherwise begins with CLEAR. */
    void beginSceneRenderPass();

    /** @brief Clears the color aspect of the active scene render pass to
     *  transparent black ({0,0,0,0}), restricted to @p area — the LOAD path's
     *  own counterpart to beginRenderPass()'s LOAD_OP_CLEAR: a persisted-scene
     *  LOAD frame otherwise never clears color, so translucent content re-drawn
     *  over @p area would blend onto whatever pixels the prior frame left there
     *  instead of starting from transparent, mirroring how AppKit itself clears
     *  a non-opaque window's dirty rect before drawRect:. @p area is clamped to
     *  the scene extent (swapchainExtent) — a degenerate or out-of-bounds rect
     *  clamps to zero area and records a no-op clear, the same
     *  compute-then-report shape clipToRectangle() uses for a degenerate clip
     *  (jam_VulkanLowLevelGraphicsContext.h).
     *  @pre Must be called between beginFrame() and endFrame(), with the scene
     *       render pass active (renderPassActive).
     *  @param area  VulkanDevice-space (physical pixel) rectangle to clear.
     */
    void clearSceneColorRegion (juce::Rectangle<int> area);

    /** @brief Recreates the swapchain for new dimensions.
     *  @param width   Logical width in points.
     *  @param height  Logical height in points.
     *  @param scale   Display scale factor (backingScaleFactor).
     */
    void resize (int width, int height, float scale = 1.0f);

    /** @brief Increments the active context count.
     *  Called by the factory each time a new LLGC is constructed for this frame.
     */
    void incrementContextCount();

    /** @brief Begins an offscreen frame against a caller-owned framebuffer.
     *  Reentrant: depth-0 resets+begins THE commandBuffer, resets the
     *  descriptor pool, resets per-frame arenas; depth > 0 suspends the
     *  outer's render pass (endRenderPass). All depths: get-or-create
     *  a Recording for this level, store framebuffer/extent/resolveImage,
     *  allocate set3 + set2 from the pool (written to this recording's own
     *  projection storage buffer and the current primitiveRecordBuffer handle
     *  respectively), write
     *  the projection ortho, point mirrors, and begin a CLEAR render
     *  pass against @p framebuffer at @p extent.
     *  Called only by VulkanImageRenderer::renderTarget, which must not
     *  record against @p framebuffer when this returns false.
     *  @param framebuffer   The target's own framebuffer (owned by VulkanImagePixelData).
     *  @param extent        The target's pixel extent.
     *  @param resolveImage  The target's resolve image, stored in the Recording so
     *                       endFrame()'s nested-unwind path can barrier it to
     *                       eShaderReadOnlyOptimal for the outer pass's sampled read.
     *  @return true once the render pass against @p framebuffer is active;
     *          false if commandBuffer.reset/begin, resetDescriptorPool, or
     *          either allocateDescriptorSets call failed — no render pass
     *          is active and the caller must not record draws.
     */
    bool beginOffscreenFrame (vk::Framebuffer framebuffer, vk::Extent2D extent, vk::Image resolveImage);

    //==========================================================================
    // Accessors
    //==========================================================================

    /** @brief Returns the logical vk::Device handle from the shared VulkanDevice. */
    vk::Device getDevice() const noexcept;

    /** @brief Returns the vk::PhysicalDevice handle from the shared VulkanDevice. */
    vk::PhysicalDevice getPhysicalDevice() const noexcept;

    /** @brief Returns the active primary command buffer for the current frame. */
    vk::CommandBuffer getCommandBuffer() const noexcept;

    /** @brief Returns the main vk::RenderPass handle (CLEAR_OP variant). */
    vk::RenderPass getRenderPass() const noexcept;

    /** @brief Returns the current swapchain pixel extent. */
    vk::Extent2D getSwapchainExtent() const noexcept;

    /** @brief Returns the swapchain colour format — threaded into
     *  VulkanShaderRegistry's public getOrCreate and get surface at call sites
     *  needing the caller's own colour format (that registry stores no
     *  swapchain reference of its own). */
    vk::Format getSwapchainFormat() const noexcept { return swapchain.getFormat().format; }

    /** @brief Returns the native surface's current extent — on Windows, the
     *  DirectComposition target's own client extent when composition interop
     *  is active (VulkanComposition::getClientExtent()); otherwise forwards to
     *  swapchain.getSurfaceExtent(). */
    vk::Extent2D getSurfaceExtent() const;

    /** @brief Returns the graphics queue family index from the shared VulkanDevice. */
    uint32_t getGraphicsQueueFamily() const noexcept;

    /** @brief Returns the graphics vk::Queue handle from the shared VulkanDevice. */
    vk::Queue getGraphicsQueue() const noexcept;

    /** @brief Returns the VmaAllocator handle from the shared VulkanDevice. */
    VmaAllocator getAllocator() const noexcept;

    /** @brief Returns the pipeline collection for this surface. */
    VulkanPipelines& getPipelines() noexcept;

    /** @brief Returns the stencil vk::ImageView for path and alpha clipping. */
    vk::ImageView getStencilView() const noexcept;

    /** @brief Returns the vk::Image handle for the main scene's clip-depth stencil.
     *  Exposed for beginOffscreenTransparencyRenderPass(), which copies
     *  this image's content into a transparency layer's own isolated stencil when
     *  an outer clip is active (currentState.stencilClipDepth > 0). */
    vk::Image getStencilImage() const noexcept;

    //==========================================================================
    // Rendering partner interface
    //==========================================================================

    /** @brief Allocates sizeBytes from this VulkanGraphics's per-frame staging arena
     *  (stagingArena). Thin call into VulkanStagingArena::getOrCreateAllocation() —
     *  see its own doc comment for the growth and retire-on-grow/drain contract.
     *  Every two allocations within the same frame land at disjoint offsets — never overlapping.
     *  @param sizeBytes  Required allocation size in bytes.
     *  @return The reserved buffer/mapped-pointer/offset, or an invalid result if
     *          VMA buffer creation failed on grow.
     */
    VulkanStagingArena::StagingAllocation allocateStaging (vk::DeviceSize sizeBytes);

    /** @brief Resolved identity of a juce::Image's ultimate backing pixel
     *  store. Walks every ImagePixelData::getSourcePixelData()/getSubsection()
     *  hop from the image's own pixel data up to the fixed point (a store
     *  that is its own source — ImagePixelData::getSourcePixelData()'s own
     *  default-implementation contract, juce_Image.h). `area` accumulates
     *  each hop's own getSubsection() offset, expressed in `root`'s
     *  coordinate space — the region of `root`'s pixels this specific
     *  juce::Image ultimately samples. Supports subsection-of-subsection
     *  (juce::Image::getClippedImage() called again on an already-clipped
     *  image). See getImageSourceRoot() below. */
    struct ImageSourceRoot
    {
        juce::ImagePixelData* root { nullptr };
        juce::Rectangle<int> area {};
    };

    /** @brief Walks image's own pixel-data chain to its root — see
     *  ImageSourceRoot's own doc comment. This is what makes every
     *  juce::Image::getClippedImage() subsection of the SAME source image a
     *  texture-cache HIT: each getClippedImage() call constructs a fresh
     *  SubsectionPixelData instance (a fresh key under a naive per-VulkanImage
     *  ImagePixelData* keying scheme), but every one of them resolves to the
     *  SAME root here, not image's own immediate ImagePixelData*.
     *  @param image  The juce::Image passed to drawImage()/clipToImageAlpha()
     *                (via cacheImageTexture()/getCachedImageView()/
     *                getBindlessIndex()/getImageUVRect()), guaranteed non-null
     *                by every one of those callers' own preconditions.
     */
    static ImageSourceRoot getImageSourceRoot (const juce::Image& image) noexcept;

    /** @brief Drives cache-owned idempotent upload (static roots no-op by
     *  bootstrap invariant, dynamic roots created/uploaded on first call,
     *  dirty roots re-uploaded) and assigns a per-window bindless slot for
     *  this context if not yet assigned.
     *  @param src  Source JUCE image, possibly a subsection (or subsection-of-
     *              subsection) of the image actually uploaded.
     */
    void cacheImageTexture (const juce::Image& src);

    /** @brief Returns the vk::ImageView for src's resolved root texture, or an
     *  empty handle if not yet resident. Engine-backed images (VulkanImagePixelData)
     *  return their resolve texture's view directly; foreign (software) images
     *  return the cache entry's view when clean. Resolves src to its root pixel
     *  store first (getImageSourceRoot()) so every subsection of the same source
     *  is a hit.
     *  @param src  Source JUCE image, possibly a subsection (or subsection-of-
     *              subsection) of the image actually cached.
     */
    vk::ImageView getCachedImageView (const juce::Image& src) const noexcept;

    /** @brief Lazily creates and returns a linear, clamp-to-edge sampler for shader sampling. */
    vk::Sampler getOrCreateLinearSampler();

    /** @brief Returns the persistent set 1 bindless texture array descriptor set — bound
     *  once per draw call, same handle every time, distinguished per-primitive by
     *  VulkanPrimitiveRecord::textureIndex. */
    vk::DescriptorSet getBindlessTextureDescriptorSet() const noexcept { return bindlessTextureDescriptorSet; }

    /** @brief Writes @p view into the persistent bindless texture array's slot @p index
     *  across all three sampled-image-typed bindings sharing that slot index — set 1
     *  binding 0 (plain `texture2D`), binding 3 (combined image-sampler, paired with the
     *  linear sampler), and binding 4 (combined image-sampler, paired with the nearest
     *  sampler) — in a single vkUpdateDescriptorSets call. Called once per unique
     *  texture/atlas/transparency-target at upload/(re)creation time, never per draw.
     *  @param index  Bindless array slot, must be < bindlessTextureCapacity.
     *  @param view   vk::ImageView to bind at that slot.
     */
    void writeBindlessTextureDescriptor (uint32_t index, vk::ImageView view);

    /** @brief Returns the bindless array slot assigned to src's resolved root
     *  texture for this window, or -1 if not yet resident. Engine-backed images
     *  (VulkanImagePixelData) return their resolve texture's per-window slot
     *  directly; foreign (software) images return the cache entry's slot.
     *  Resolves src to its root pixel store first — every subsection of one
     *  root shares this slot.
     *  @param src  Source JUCE image, possibly a subsection (or subsection-of-
     *              subsection) of the image actually cached.
     */
    int getBindlessIndex (const juce::Image& src) const noexcept;

    /** @brief Returns the normalized [0,1] UV sub-rect within the resolved root
     *  texture that src samples — {0,0,1,1} when src IS the root (the common,
     *  non-subsection case), or the proportional sub-rect for a subsection.
     *  Engine-backed images compute UV proportional to the resolve texture's
     *  extent; foreign (software) images use the cache entry's size. Falls back
     *  to {0,0,1,1} if the root is not yet resident.
     *  @param src  Source JUCE image, possibly a subsection (or subsection-of-
     *              subsection) of the image actually cached.
     */
    juce::Rectangle<float> getImageUVRect (const juce::Image& src) const noexcept;

    /** @brief Assigns + writes each atlas type's per-window bindless descriptor
     *  slot. Called once at create() time — atlas GPU images are guaranteed to
     *  exist by construction (jam::GlyphAtlas creates them eagerly). The
     *  assigned index is recorded into the atlas's own jam::VulkanBindlessTexture
     *  (atlas.getTexture (type)), keyed by this VulkanGraphics's own
     *  getNativeHandle().
     *  @param atlas  The shared CPU+GPU glyph atlas (jam::GlyphAtlas::getInstance()).
     */
    void registerGlyphAtlasSlots (jam::GlyphAtlas& atlas);

    /** @brief Returns the key jam::VulkanBindlessTexture's registry uses to
     *  identify this instance — the native window handle for windowed
     *  instances, exactly the value passed to create(), matching
     *  jam::VulkanEngine::contexts's identical void* key idiom
     *  (juce::ComponentPeer::getNativeHandle()); this instance's own address
     *  for offscreen instances. */
    void* getNativeHandle() const noexcept { return bindlessInstance.getHandle(); }

    //==========================================================================
    // Frame buffer accessors
    //==========================================================================

    /** @brief Returns the per-frame path vertex/index VulkanFrameBuffer. */
    VulkanFrameBuffer& getPathFrameBuffer();

    /** @brief Returns the per-frame VulkanPrimitiveRecordBuffer SSBO (rect/image/glyph/clip-mask
     *  quad records, pulled by the instanced vertex shader via gl_InstanceIndex). */
    VulkanPrimitiveRecordBuffer& getPrimitiveRecordBuffer();

    /** @brief Reserves capacity in the VulkanPrimitiveRecordBuffer for at least
     *  @p requiredRecords. Re-writes the set-2 storage-buffer descriptor if growth
     *  changed the underlying vk::Buffer handle. Asserts on allocation failure.
     *  @param requiredRecords  Minimum element capacity required.
     */
    void reservePrimitiveRecords (int requiredRecords);

    /** @brief Returns the projection mat4 descriptor set — bound as set 3 at draw time. */
    vk::DescriptorSet getProjectionDescriptorSet() const noexcept;

    /** @brief Returns the persistently-mapped pointer for the projection storage buffer.
     *  Callers memcpy a mat4 before each draw call.
     */
    void* getProjectionMappedPtr() const noexcept;

    /** @brief Returns the VulkanPrimitiveRecord storage-buffer descriptor set — bound as
     *  set 2 at draw time by every instanced (rect/image/glyph/clip-mask) pipeline. */
    vk::DescriptorSet getRecordDescriptorSet() const noexcept;

    //==========================================================================
    // Transparency
    //==========================================================================

    /** @brief Returns or creates the TransparencyLayer at the given nesting level.
     *  Creates at the stack's current extent (set by resize/create seed) if the
     *  level does not yet exist. Assigns a bindless slot on creation. Asserts on
     *  creation failure (VRAM exhaustion).
     *  @param level  Zero-based stack index.
     */
    void getOrCreateTransparencyLayer (int level);

    /** @brief Returns the TransparencyLayer at the given nesting level.
     *  @param level  Stack index — must be within the range created by getOrCreateTransparencyLayer().
     */
    VulkanTransparencyStack::TransparencyLayer& getTransparencyLayer (int level);

    //==========================================================================
    // Winding (non-zero/even-odd stencil-then-cover fill)
    //==========================================================================

    /** @brief Reserves winding-accumulate scratch capacity for at least requiredExtent
     *  and assigns a bindless slot on first use or after growth. Asserts on
     *  allocation failure (VRAM exhaustion).
     *  @param requiredExtent  Minimum pixel extent needed for the current fillPath() call.
     *  @return The winding-accumulate scratch target owned by this VulkanGraphics.
     */
    VulkanWindingScratch& getOrCreateWindingScratch (vk::Extent2D requiredExtent);

    /** @brief Returns the winding-accumulate scratch target owned by this VulkanGraphics. */
    VulkanWindingScratch& getWindingScratch() noexcept { return windingScratch; }

    //==========================================================================
    // Image effects
    //==========================================================================

    /** @brief Returns the compute-shader image-effect processor (blur, matte
     *  choke, matte feather) owned by this VulkanGraphics — this class
     *  remains the orchestrator: it threads its own descriptor pool and
     *  bindless texture descriptor set into VulkanImageEffects::createDescriptorSets()
     *  and never stores a back-reference the other way. */
    VulkanImageEffects& getImageEffects() noexcept { return imageEffects; }

#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
    //==========================================================================
    // VulkanShader registry
    //==========================================================================

    /** @brief Returns the runtime-shader-compiler lane's own GPU execution
     *  registry — cache, shared setup helpers (pipeline layout, offscreen
     *  render pass, shared vertex module, straight-alpha/background/
     *  post-process combine pipelines), and the mesh-backed material-range
     *  draw. This class remains the orchestrator: it threads its own frame
     *  fabric (swapchain extent/format, render passes, activeSampleCount,
     *  commandBuffer, bindlessRegistry, frameCounter) into the registry's
     *  public getOrCreate*/get* surface at each call site, never storing a
     *  back-reference the other way. */
    VulkanShaderRegistry& getShaderRegistry() noexcept { return shaderRegistry; }
#endif

    //==========================================================================
    // MSAA calibration
    //==========================================================================

    /** @brief Returns the MSAA sample count set once by calibrateSampleCount()
     *  — read by every pipeline's multisampleState() call (SSOT, no duplicated
     *  sample-count literal anywhere else). */
    vk::SampleCountFlagBits getActiveSampleCount() const noexcept { return device.getActiveSampleCount(); }

#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
    //==========================================================================
    // VulkanShader execution
    //==========================================================================

    /** @brief Ensures shader's per-content-hash GPU execution resources (buffer-
     *  pass images/pipelines, the Image-pass pipeline cache, the channel
     *  sampler) are built and current, rebuilding whenever shader.contentHash
     *  or the scaled extent (swapchainExtent × resolutionScale) changed since
     *  the last build — the resize path this way needs no explicit hook of
     *  its own, since the next call after a resize naturally sees a changed
     *  scaled extent. A stale build is moved to previousShaderInstances
     *  (deferred destroy), never destroyed mid-frame.
     *  @param shader             The compiled shader to prepare for execution.
     *  @param resolutionScale    Intermediate-pass extent fraction, [0, 1] —
     *                            caller-supplied (not a VulkanShader field, see
     *                            jam_VulkanShader.h's VulkanShader doc comment).
     *  @param chainInputExtent   Forwarded verbatim to VulkanShaderInstance::build()'s
     *                            own @p chainInputExtent — the basis extent for
     *                            pass 0's own "source"-typed scale directive
     *                            (see VulkanShaderInstance::build()'s own doc
     *                            comment, jam_VulkanShaderInstance.h). This
     *                            method's own two callers each supply their
     *                            own basis: VulkanLowLevelGraphicsContext::
     *                            renderShader() (the in-scene background
     *                            path, no resolved scene) passes this
     *                            shader's own scaled extent
     *                            (VulkanShaderInstance::computeScaledExtent
     *                            (getSwapchainExtent(), resolutionScale));
     *                            VulkanGraphics::endFrame() (the post-process
     *                            composite) passes swapchainExtent, the
     *                            actual straight-alpha scene extent.
     *  @return Pointer to shader's built execution if its resources are ready
     *          for use, nullptr otherwise.
     *  @pre None — self-managed. On a (re)build this method itself records
     *       transfer commands — buildExternalTexture()'s LUT uploads
     *       (VulkanBindlessTexture::upload: copyBufferToImage + barriers, plus
     *       recordMipChainGeneration()'s blits) and the mesh path's
     *       VulkanMesh::build() staging copy (jam_VulkanShaderInstance.cpp,
     *       reached via VulkanShaderInstance::build()) — all illegal inside an
     *       active render pass instance, so this method itself suspends the
     *       currently active render pass (renderPassActive, captured before
     *       mutation) before recording them and resumes it after, entirely
     *       within the (re)build branch (jam_VulkanGraphicsSlangPass.cpp).
     *       A cache-hit call (no (re)build needed) records zero pass
     *       boundaries — no suspend, no resume. Neither caller brackets this
     *       call any longer: VulkanLowLevelGraphicsContext::renderShader()
     *       (jam_VulkanLowLevelGraphicsContextRender.cpp) calls this with
     *       the scene pass active; VulkanGraphics::endFrame()
     *       (jam_VulkanGraphics.cpp) calls this with no pass active (already
     *       ended the scene render pass beforehand) — this method observes
     *       renderPassActive itself and reacts correctly either way.
     */
    VulkanShaderInstance* getOrCreateShaderInstance (const VulkanShader& shader, float resolutionScale, vk::Extent2D chainInputExtent);

    /** @brief Records shader's buffer passes, in declared order, into their
     *  own offscreen single-sample targets: barriers the previous frame's
     *  ping-pong-current image to shader-read, begins the offscreen pass,
     *  binds that pass's own pipeline, pushes this call's stamped
     *  VulkanShaderUniforms (channels[] = the other ping-pong half for the pass's
     *  own record or an earlier pass's fresh output, per Shadertoy channel
     *  semantics — VulkanShaderInstance::stampChannels()), draws the fullscreen
     *  triangle, ends the pass, barriers the written image to shader-read.
     *  Suspends the currently active render pass first (if any); resumes it
     *  afterward only when resumeAfter is true — the resume decision is
     *  caller-controlled (explicit parameter, not auto-inferred): the
     *  in-scene render() draw passes true (a scene pass was active), the
     *  endFrame() post-process composite passes false (no scene pass is
     *  active at that point).
     *  @param shader              The compiled shader whose buffer passes are recorded.
     *  @param execution           shader's built execution (getOrCreateShaderInstance()/getShaderInstance()).
     *  @param resumeAfter         Whether to resume the render pass active before this
     *                             call once recording ends.
     *  @param sceneBindlessIndex  Bindless index of the resolved scene texture,
     *                             stamped into the returned VulkanShaderUniforms' iScene
     *                             (SSOT — every pass sharing these base uniforms
     *                             copies this value forward, never re-stamping
     *                             it) and used as every buffer pass's own
     *                             channels[]' fallback (VulkanShaderInstance::stampChannels()).
     *                             The in-scene render() call site
     *                             (VulkanLowLevelGraphicsContext::renderShader()) has
     *                             no scene to offer and passes
     *                             VulkanShaderUniforms::noScene; the post-process
     *                             composite (recordPostProcessCompositeDrawCommands())
     *                             passes sceneColorBindlessIndex, the same index
     *                             its own channels[] fallback reads.
     *  @param resolutionScale     Intermediate-pass extent fraction, [0, 1] —
     *                             caller-supplied (not a VulkanShader field).
     *  @param mouse               This call's iMouse value, forwarded verbatim
     *                             to VulkanShaderInstance::stampUniforms() (SSOT for
     *                             the sign-encoding contract, see its own doc
     *                             comment) — a pure per-call parameter, never
     *                             stored by this VulkanGraphics or by execution.
     *  @return The VulkanShaderUniforms stamped for this call — reuse these exact
     *          values (iTime/iTimeDelta/iFrame/iResolution/iMouse/iScene) when
     *          pushing the Image pass's own draw.
     */
    VulkanShaderUniforms recordShaderBufferPasses (const VulkanShader& shader,
                                                   VulkanShaderInstance& execution,
                                                   bool resumeAfter,
                                                   int32_t sceneBindlessIndex,
                                                   float resolutionScale,
                                                   const std::array<float, 4>& mouse);

    /** @brief Returns straightAlphaImage's own vk::ImageView — the slang
     *  texture-name resolution (getSlangTextureBindings() below) needs
     *  the ACTUAL view/extent
     *  behind straightAlphaBindlessIndex for a slang pass's own dedicated
     *  descriptor set, not just the bindless array slot every
     *  VulkanShaderFormat::shadertoy pass samples through instead. Public
     *  (unlike straightAlphaImage itself): getSlangTextureBindings() is
     *  this accessor's only consumer, but it is declared here as a small,
     *  named, single-purpose accessor rather than widening straightAlphaImage
     *  itself to public, per this class's own "no excessive getters" contract
     *  — this is the one legitimate external read straightAlphaImage's own
     *  identity needs. */
    vk::ImageView getStraightAlphaImageView() const noexcept { return straightAlphaImage.getView(); }

    /** @brief Resolves a slang pass's own reflected texture names
     *  (VulkanShaderReflection::textures, jam_VulkanShaderReflection.h) to their
     *  actual GPU resources for THIS draw — the slang texture-name
     *  resolution, plus RetroArch's own aliasN preset directive
     *  (jam_VulkanShaderPreset.h) and its \#pragma name source-level fallback
     *  (jam::VulkanShaderCompiler::parsePassName()) resolving through the
     *  SAME ordinal space below (VulkanShaderInstance::getPassAliases()). A name
     *  matching one of execution.getExternalTextures()'s own declared LUT/
     *  texture names (jam::VulkanShaderPreset::Texture::name) resolves
     *  FIRST, via that direct
     *  name-map lookup — a manifest author's own LUT name is a first-class
     *  identifier the SAME way a buffer pass's own alias is, never
     *  colliding with the engine-fixed vocabulary below since this lookup
     *  runs before it; this is table/map dispatch, not a fourth top-level
     *  if-chain arm (MANIFESTO's 3-branch max, same shape OriginalHistoryN
     *  below already folds into an existing branch rather than adding one):
     *  - "PassOutputN"/"PassFeedbackN" (N = buffer-pass ordinal), or an
     *    author's own alias spelling / "<alias>Feedback" resolving to that
     *    SAME ordinal N via VulkanShaderInstance::getPassAliases(), resolve to
     *    execution.getBufferPassTarget (N)'s CURRENT readable half
     *    (VulkanRenderResources::images.at (currentReadHalf).getView()) and that
     *    SAME target's own VulkanRenderResources::extent — every buffer pass is
     *    unconditionally self-feedback-capable (VulkanShaderPass's own doc
     *    comment), so a feedback-spelled name resolves identically to its
     *    output-spelled sibling, a name alias only, never a distinct lookup.
     *  - "Original" always resolves to getStraightAlphaImageView() above and
     *    getSwapchainExtent() — the SAME resolved-scene image this engine's
     *    existing straightAlphaBindlessIndex/iScene mechanism already
     *    feeds — ONLY when @p isBackground is false (background mode has no
     *    resolved scene to offer, matching the existing isBackground gate
     *    already used for iScene/sceneMacro() at GLSL-generation time,
     *    jam_VulkanShaderCompiler.cpp). This engine has no distinct
     *    "unprocessed input frame" concept, but Original's OWN semantics
     *    never vary by @p passIndex, unlike Source below.
     *  - "Source" resolves to the PREVIOUS pass's own current output within
     *    THIS shader's own chain — execution.getBufferPassTarget
     *    (@p passIndex - 1)'s current readable half and its own
     *    VulkanRenderResources::extent — when @p passIndex > 0. @p passIndex == 0
     *    (no previous pass exists in the chain) falls back to the exact same
     *    resolved-scene resolution as Original, still only when
     *    @p isBackground is false (background mode's pass 0 has no scene to
     *    fall back to either — stays unresolved, same graceful degradation
     *    as every other unresolved name here).
     *  - The mandatory Image pass's own self-feedback read — "PassFeedback"
     *    plus its own final-pass ordinal, or an author's own
     *    "<alias>Feedback" resolving to that SAME final ordinal — resolves
     *    to execution.getImagePassGatherTarget()'s OTHER half (last
     *    completed frame's gather output), gated on that target being
     *    history-capable (VulkanShaderInstance::build()'s own conditional
     *    gatherImageCount == 2, RetroArch's real-world 1-pass feedback
     *    preset shape). Reading the Image pass's CURRENT-frame output (its
     *    own "PassOutputN"/exact-alias spelling) is undefined (this pass
     *    hasn't drawn yet this frame) and so is never resolved here.
     *  - "OriginalHistoryN" folds into the SAME Original/Source branch above
     *    as this function's own implementation (never a fourth top-level
     *    resolution branch — MANIFESTO's 3-branch max) — RetroArch's
     *    OriginalHistory0 resolves identically to Original (the straight-
     *    alpha scene, pp only, same as above); OriginalHistoryN (N >= 1, N <=
     *    execution.getOriginalHistoryDepth()) resolves to a computed slot in
     *    execution.getOriginalHistoryImages()'s own ring buffer, written once
     *    per frame by VulkanGraphics::recordOriginalHistoryCopy() (called BEFORE
     *    this resolution ever runs, recordPostProcessCompositeDrawCommands()'s
     *    own ordering) — see this function's own .cpp implementation for the
     *    exact cursor-arithmetic derivation. N beyond
     *    execution.getOriginalHistoryDepth(), or background mode (no
     *    original input to offer there either, same gate as Original/Source
     *    above), stays unresolved.
     *  - Any other name is left unresolved (absent from both maps) —
     *    graceful degradation, mirrors VulkanShaderReflection::populateMemberBuffer()'s
     *    own "unknown member stays zero, no failure" contract; a slang pass's
     *    own refreshSlangPass() call already tolerates a missing key this
     *    same way.
     *
     *  Called fresh every frame a slang pass draws (never cached) — see
     *  VulkanShaderTextureBindings's own doc comment for why a cached result would
     *  go stale the moment a referenced buffer pass's ping-pong toggles.
     *  @param textures     This pass's own reflected texture list
     *                      (execution.getSlangPassTextures (passIndex)).
     *  @param execution    shader's built execution — supplies buffer-pass
     *                      targets.
     *  @param isBackground True for a background-mode draw (no resolved
     *                      scene — see above), false for post-process.
     *  @param passIndex    Zero-based ordinal of THIS resolving pass into
     *                      shader.passes (buffer passes then the mandatory
     *                      Image pass, execution.getImagePassIndex()) —
     *                      Source's own ordinal-1 arithmetic above needs to
     *                      know which pass is resolving. recordSingleBufferPass()
     *                      supplies its own passIndex; the Image-pass call
     *                      sites (recordPostProcessCompositeDrawCommands(),
     *                      VulkanLowLevelGraphicsContext::
     *                      recordShaderImagePassDrawCommands()) supply
     *                      execution.getImagePassIndex().
     *  @return This call's resolved name -> view/extent maps.
     */
    VulkanShaderTextureBindings
    getSlangTextureBindings (const jam::Array<VulkanShaderReflection::TextureResource>& textures,
                             VulkanShaderInstance& execution,
                             bool isBackground,
                             int passIndex) const;

#endif

    //==========================================================================
    // Scene persistence
    //==========================================================================

    bool isSceneContentValid() const noexcept { return sceneContentValid; }

private:
    //==========================================================================
    // Frame lifecycle (private types)
    //==========================================================================

    /** @brief Outcome of dispatching a per-frame Vulkan result (acquireNextImageKHR
     *  in beginFrame(), presentKHR in endFrame()) through getDisposition()'s
     *  shared result-to-disposition table. */
    enum class FrameDisposition
    {
        /** @brief The result carries no swapchain problem — beginFrame() records
         *  and endFrame() presents normally. */
        proceed,

        /** @brief No image is available yet and the swapchain itself is still
         *  valid — beginFrame() returns false without recording; the caller
         *  retries next paint cycle. */
        skip,

        /** @brief The swapchain no longer matches the surface (out-of-date or
         *  surface-lost) — sets isSwapchainStale so the next beginFrame()
         *  heals via resize() before acquiring again. */
        stale,

        /** @brief The logical device itself was lost — the faulting frame
         *  aborts (beginFrame() returns false; endFrame() skips presenting)
         *  and a VulkanEngine::reinitialiseDevice() call is deferred to the
         *  next message-loop tick via juce::MessageManager::callAsync(). */
        reinitialise
    };

    /** @brief Looks up result in dispositions, logging and falling back to
     *  fallback on a miss — the shared find/end/else/log idiom beginFrame()'s
     *  acquireNextImageKHR dispatch and endFrame()'s presentKHR dispatch both
     *  need against their own distinct result tables.
     *  @param dispositions  The caller's own result-to-disposition table.
     *  @param result        The vk::Result just returned by the caller's own Vulkan call.
     *  @param fallback      Disposition returned when result has no entry in dispositions.
     *  @param context       Prefix identifying the caller for the unhandled-result log line.
     */
    static FrameDisposition getDisposition (const jam::HashMap<vk::Result, FrameDisposition>& dispositions,
                                            vk::Result result,
                                            FrameDisposition fallback,
                                            juce::StringRef context);

    /** @brief Result table for a Vulkan call whose failure may legitimately be
     *  device loss — eSuccess maps to proceed, eErrorOutOfHostMemory/
     *  eErrorOutOfDeviceMemory map to skip, eErrorDeviceLost maps to
     *  reinitialise. Used by waitForFences/vkQueueSubmit-class dispatches.
     *  @return The static table, by const reference. */
    static const jam::HashMap<vk::Result, FrameDisposition>& getDeviceLostTolerantDispositions();

    /** @brief Result table for a Vulkan call with exactly one acceptable
     *  outcome — eSuccess maps to proceed; every other result is unhandled
     *  and falls back to the caller's own fallback via getDisposition().
     *  @return The static table, by const reference. */
    static const jam::HashMap<vk::Result, FrameDisposition>& getSuccessOnlyDispositions();

    /** @brief Defers VulkanEngine::reinitialiseDevice() to the next
     *  message-loop tick via juce::MessageManager::callAsync() — called from
     *  the message thread mid-frame, where reinitialising the device
     *  synchronously would destroy handles this call's own stack frame still
     *  references. */
    static void deferDeviceReinitialise();

    //==========================================================================
    // Per-recording state (offscreen nesting)
    //==========================================================================

    /** @brief Per-target state for reentrant offscreen rendering.
     *  Each level owns its own projection storage buffer and the framebuffer/extent
     *  of the target it is recording into. Created on demand (grow-only
     *  pool); reused forever after. All levels share the context's single
     *  commandBuffer — no per-recording command stream. */
    struct Recording
    {
        vk::Framebuffer framebuffer {};
        vk::Extent2D extent {};
        vk::Image resolveImage {}; ///< Resolve target; barriered to eShaderReadOnlyOptimal on endFrame() nested-unwind.
        VulkanBuffer projectionBuffer {}; ///< Same CPU_ONLY + MAPPED_BIT shape as VulkanGraphics::projectionBuffer — see its allocation-site coherency rationale.
        vk::DescriptorSet projectionDescriptorSet {};
        vk::DescriptorSet recordDescriptorSet {};
    };

    //==========================================================================
    // Per-frame reset (shared by beginFrame / beginOffscreenFrame)
    //==========================================================================

    /** @brief Resets the class-level command buffer, begins one-time-submit
     *  recording, resets the descriptor pool, re-allocates the projection
     *  (set 3) and record (set 2) descriptor sets with their storage-buffer/SSBO writes,
     *  and resets pathFrameBuffer/primitiveRecordBuffer usage counters. Used
     *  by the windowed path's beginFrame(). Returns false if any underlying
     *  Vulkan call in this sequence failed. */
    bool beginRecording();

    //==========================================================================
    // Private setup helpers (called by create())
    //==========================================================================

#if JUCE_WINDOWS
    bool importCompositionImage();
#endif

    /** @brief Index-enum sweep — the scene render pass's 3 attachment slots (renderPass/
     *  renderPassLoad, and the framebuffers built against either), named so
     *  beginRenderPass()'s clearValues writes reference the slot's meaning rather than
     *  a raw index. SSOT for the same 0/1/2 shape createRenderPass() documents inline. */
    enum class RenderPassAttachment : uint8_t
    {
        msaaColor = 0,
        stencil = 1,
        resolve = 2
    };

    /** @brief Attachment count backing RenderPassAttachment — clearValueCount/
     *  attachmentCount SSOT for the scene render pass shape. */
    static constexpr size_t renderPassAttachmentCount { 3 };

    /** @brief Creates the scene render pass (CLEAR variant, renderPass), its LOAD
     *  variant (renderPassLoad), and compositeRenderPass (the identity composite
     *  — the one render pass that legitimately still targets the swapchain directly).
     *  renderPass/renderPassLoad describe 3 attachments (MSAA color, MSAA
     *  stencil, single-sample resolve) instead of the earlier 2 (swapchain
     *  color, stencil) — TransparencyLayer/VulkanWindingScratch's own framebuffers
     *  reuse this same renderPass object and must therefore also supply 3
     *  matching attachments. Render pass objects are extent-
     *  independent — never rebuilt by resize(). */
    bool createRenderPass();

    /** @brief Creates sceneFramebuffer (1 instance — VulkanGraphics-owned persistent
     *  images, independent of currentImageIndex) against renderPass, and
     *  swapchainFramebuffers (1 per swapchain image) against compositeRenderPass —
     *  the identity-composite target. Called by create() and resize(). */
    bool createFramebuffers();

    /** @brief Creates the vk::CommandPool and allocates one primary command buffer. */
    bool createCommandPool();

    /** @brief Creates per-frame semaphores and the in-flight fence. */
    bool createSyncObjects();

    /** @brief Creates the projection storage buffer, descriptor pool (projection +
     *  VulkanPrimitiveRecord storage-buffer descriptor sets), linear + nearest samplers, and the
     *  persistent bindless texture descriptor pool/set (unconditional — the queried
     *  descriptor-indexing capabilities are hard requirements, asserted at device creation). */
    bool createProjectionBuffer();
    bool createDescriptorSets();
    bool createSamplers();
    bool createBindlessTexturePool();

    bool createDrawState();

    /** @brief Creates the stencil image + view for the current swapchain extent.
     *  MSAA (activeSampleCount-sampled), matching the scene render
     *  pass's attachment 1. Deliberately NOT vk::ImageUsageFlagBits::eTransientAttachment
     *  (see createSceneTarget()'s doc comment for why). */
    bool createStencilImage();

    /** @brief Creates (or recreates, at a new extent) sceneMsaaColorImage +
     *  sceneColorImage — the scene render target. Destroys the prior
     *  sceneFramebuffer first (its attachments are about to be replaced); the
     *  RAII VulkanImage move-assigns destroy their own prior images.
     *
     *  Deliberately NOT vk::ImageUsageFlagBits::eTransientAttachment / lazily-allocated
     *  memory for sceneMsaaColorImage: the scene render pass is paused and resumed multiple times within
     *  a single frame (resumeRenderPass(), used by endTransparencyLayer() and
     *  recordWindingCompositeDrawCommands() to resume scene rendering after an
     *  offscreen composite) using LOAD_OP_LOAD on this exact image —
     *  the Vulkan spec (vk::ImageCreateInfo usage rules for
     *  vk::ImageUsageFlagBits::eTransientAttachment) forbids LOAD_OP_LOAD/STORE_OP_STORE
     *  on a transient-usage image. stencilImage (createStencilImage()) carries the
     *  same constraint for the identical reason (clip-depth must survive the same
     *  resumes) and is likewise non-transient. Only sceneColorImage (the resolve
     *  target, read-only after resolve, no LOAD ever used against it) would be a
     *  transient-safe candidate, but it must be vk::ImageUsageFlagBits::eSampled for
     *  bindless composite sampling — the two usages combine safely without the
     *  transient bit, so plain GPU_ONLY memory is used throughout for consistency.
     *  @param targetExtent  Pixel extent for the scene target — the swapchain extent.
     */
    bool createSceneTarget (vk::Extent2D targetExtent);

#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
    /** @brief Creates (or recreates, at a new extent) straightAlphaImage +
     *  straightAlphaDepthImage + straightAlphaFramebuffer — the post-process
     *  path's un-premultiplied scene target, sized and recreated identically
     *  to sceneColorImage (same swapchainFormat.format, single-sample, full
     *  swapchain extent) since it holds the SAME frame's content, merely
     *  un-premultiplied by recordStraightAlphaPass(). Built against the
     *  swapchainFormat.format single-sample offscreen render pass (the
     *  zero-arg getOrCreateShaderOffscreenRenderPass()) — straightAlphaImage
     *  always mirrors sceneColorImage's own fixed format, never a per-pass
     *  srgb_framebufferN/float_framebufferN-resolved one, so this target
     *  never needs its own dedicated render pass beyond that shared entry;
     *  straightAlphaDepthImage is that SAME shared render pass's own
     *  unconditional second attachment (straightAlphaDepthImage's own doc
     *  comment) — this VulkanGraphics-owned framebuffer supplies one exactly like
     *  every VulkanRenderResources' own depthImage, since it is built against the
     *  same 2-attachment render pass shape. Mirrors createSceneTarget()'s
     *  destroy-old-framebuffer-first / reset-bindless-index-to--1 shape
     *  exactly. eTransferSrc alongside eColorAttachment/eSampled on
     *  straightAlphaImage — see its own doc comment for why
     *  (recordOriginalHistoryCopy()'s vk::CommandBuffer::copyImage source).
     *  @param targetExtent  Pixel extent for the straight-alpha target — the swapchain extent.
     */
    bool createStraightAlphaTarget (vk::Extent2D targetExtent);
#endif

    /** @brief Assigns + writes the bindless array slot for sceneColorImage,
     *  once its view identity is (re)created — mirrors getOrCreateTransparencyLayer()/
     *  getOrCreateWindingScratch()'s exact "assign once, re-assign only on identity
     *  change" convention. Split out from createSceneTarget() because the bindless
     *  descriptor set/pool do not exist yet the first time createSceneTarget() runs
     *  during create() (createDrawState() creates them later in the sequence) —
     *  called once after createDrawState() at create() time, and again after
     *  createFramebuffers() at resize() time (by which point the descriptor set
     *  already exists). */
    void updateSceneColorBindlessDescriptor();

#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
    /** @brief Assigns + writes the bindless array slot for straightAlphaImage —
     *  mirrors updateSceneColorBindlessDescriptor()'s exact convention and call
     *  sites (once after createDrawState() at create() time, again after
     *  createFramebuffers() at resize() time). */
    void updateStraightAlphaBindlessDescriptor();
#endif

    /** @brief Builds the identity-composite pipeline (compositePipeline) —
     *  reuses instanced.vert + image.frag (same shader pair as ID::imageInstanced)
     *  and the real VulkanPipelines::getLayout(), but as a DISTINCT vk::Pipeline built with
     *  rasterizationSamples = vk::SampleCountFlagBits::e1 against compositeRenderPass.
     *  Not literally the same vk::Pipeline object as ID::imageInstanced: Vulkan
     *  requires a pipeline's rasterizationSamples to equal its target subpass's
     *  attachment sample count, and every one of the 21 real pipelines is built at
     *  activeSampleCount — none of them is valid against compositeRenderPass's
     *  single-sample swapchain attachment. VulkanShader modules are created via VulkanPipelines::createShaderModule()
     *  (shared with measureCandidateSampleCount() and VulkanPipelines::loadShaderModules() —
     *  one implementation, three call sites) and destroyed locally — only the resulting
     *  vk::Pipeline is persisted. Runs once, after pipelines.load()/
     *  createDrawState() (needs the real layout + descriptor sets to exist);
     *  resolution-independent (viewport/scissor are dynamic state) — never rebuilt
     *  by resize(). */
    bool createCompositePipeline (vk::PipelineCache pipelineCache);

    /** @brief Ends the context's commandBuffer and performs a synchronous
     *  one-shot submit via offscreenFence: submit, wait, reset fence.
     *  Called from endFrame()'s offscreen branch when activeRecordings
     *  is empty (depth-0 final submit). */
    void submitOffscreenAndWait();

    /** @brief Re-writes the set-2 storage-buffer descriptor to point at
     *  primitiveRecordBuffer's CURRENT vk::Buffer handle. Called once per frame
     *  (beginFrame(), mirroring the projection descriptor's per-frame re-write)
     *  and again whenever reservePrimitiveRecords() detects mid-frame growth.
     */
    void updateRecordDescriptorSet();

    /** @brief Reclaims every resource moved into a previous* container since
     *  the last drain: releases VulkanWindingScratch's previous-generation bindless
     *  slots (windingScratch.getPreviousBindlessIndices()) then calls
     *  windingScratch.resetPrevious(); drains bindlessRegistry's own retired
     *  index list (bindlessRegistry.releaseRetiredIndices()); drains
     *  stagingArena (stagingArena.resetResources()); releases every
     *  previousShaderInstances entry's buffer-pass ping-pong bindless slots
     *  before clearing the list; increments frameCounter. Precondition:
     *  called only from beginFrame(), after its fence wait has returned —
     *  every previous* entry's GPU work is guaranteed complete at that point,
     *  never before.
     */
    void resetResources();

    /** @brief The minimal identity composite. Called by endFrame() right
     *  after the scene render pass ends (its resolve has just written
     *  sceneColorImage). Barriers sceneColorImage for sampling, appends one
     *  full-screen VulkanPrimitiveRecord addressing its bindless slot, and draws it via
     *  compositePipeline into compositeRenderPass targeting
     *  swapchainFramebuffers[currentImageIndex] — a structurally identical copy of
     *  what reached the swapchain directly before the scene/composite split, routed
     *  through one extra resolve + composite hop. No-op if the bindless array was
     *  at capacity when
     *  sceneColorImage was created (capacity bound, not an architecture branch —
     *  same convention as compositeTransparencyLayer()/
     *  recordWindingCompositeDrawCommands()). Sets renderPassActive true/false around
     *  its own begin/end for invariant symmetry — this call is always terminal
     *  (endFrame()'s last step before commandBuffer.end()/present), so nothing reads
     *  the flag afterward this frame, but the flag must stay accurate everywhere it
     *  is set, not only at sites something later depends on. */
    void recordSceneCompositeDrawCommands();

#if JUCE_WINDOWS
    void recordCompositionAcquireBarrier();
    void recordCompositionReleaseBarrier();
#endif

#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
    //==========================================================================
    // VulkanShader execution — shared setup helpers (jam_VulkanGraphicsSlangPass.cpp)
    //==========================================================================

    /** @brief Records one buffer pass's full draw sequence — write-target
     *  resolution, discard-transition barrier, offscreen render pass begin,
     *  descriptor/uniform/pipeline bind, fullscreen-triangle draw, render
     *  pass end, shader-read barrier, and (for a feedback pass) the
     *  ping-pong read-half flip. Extracted from
     *  recordShaderBufferPasses()'s per-pass loop body — a genuine
     *  sub-responsibility (one buffer pass's own recording), repeated once
     *  per BufferA-D entry.
     *  @param execution           shader's built execution — supplies passIndex's
     *                             VulkanRenderResources and per-channel bindless lookups.
     *  @param passIndex           Zero-based BufferA-D index into execution.
     *  @param baseUniforms        This call's stamped base uniforms (recordShaderBufferPasses()'s
     *                             SSOT stamp) — copied per pass, then this pass's own
     *                             channels[]/opacity overwritten before pushing.
     *  @param layout              Shared VulkanShader-execution pipeline layout (getOrCreateShaderInstanceLayout()).
     *  @param bindlessSet         The bindless texture array descriptor set (set 0).
     *  @param sceneBindlessIndex  Forwarded to VulkanShaderInstance::stampChannels() as this pass's own
     *                             channels[] fallback — see recordShaderBufferPasses()'s doc comment.
     *
     *  This pass's own render target extent AND render pass are read
     *  directly from execution.getBufferPassTarget (passIndex).extent/
     *  renderPass (the per-pass render-target extent AND format contract:
     *  each pass independently resolves its own scaled extent and colour
     *  format) — no longer shared
     *  parameters, since different buffer passes on the same execution may
     *  now legitimately be built at different extents (jam::
     *  VulkanShaderPreset::Pass's own per-pass scale directive) AND different
     *  formats (that same struct's own srgb_framebufferN/float_framebufferN
     *  directive), each requiring its own dedicated render pass
     *  (VulkanGraphics::getOrCreateShaderOffscreenRenderPass (vk::Format)).
     */
    void recordSingleBufferPass (VulkanShaderInstance& execution,
                                 int passIndex,
                                 const VulkanShaderUniforms& baseUniforms,
                                 vk::PipelineLayout layout,
                                 vk::DescriptorSet bindlessSet,
                                 int32_t sceneBindlessIndex);

    /** @brief Records the straight-alpha un-premultiply pass — the FIRST read
     *  of sceneColorImage on the post-process composite path (moved here from
     *  recordPostProcessCompositeDrawCommands(), which used to barrier and
     *  sample sceneColorImage directly before this contract existed).
     *  Barriers sceneColorImage for sampling (same-layout memory-visibility
     *  barrier, mirroring recordSceneCompositeDrawCommands()'s own identical,
     *  entirely separate, read of the same image), barriers straightAlphaImage
     *  UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL (discard-transition — this pass
     *  fully overwrites the whole image every call, same WAR-hazard shape as
     *  recordSingleBufferPass()'s own write-half barrier), draws
     *  straight_alpha.frag (getOrCreateStraightAlphaPipeline()) into
     *  straightAlphaFramebuffer with every channels[] slot and iScene pointed
     *  at sceneColorBindlessIndex (no VulkanShaderInstance executes this pass, so
     *  VulkanShaderUniforms is stamped directly here rather than through
     *  VulkanShaderInstance::stampUniforms()/stampChannels()), then barriers
     *  straightAlphaImage for sampling (same-layout, mirrors
     *  recordSingleBufferPass()'s own post-pass barrier — the offscreen render
     *  pass's finalLayout is already eShaderReadOnlyOptimal, but its implicit
     *  vk::SubpassExternal dependency alone does not guarantee cross-stage
     *  read visibility). Every subsequent read on this composite path
     *  (recordShaderBufferPasses()' sceneBindlessIndex parameter and
     *  VulkanShaderInstance::stampChannels()'s fallback for the Image pass) is
     *  pointed at straightAlphaBindlessIndex, never sceneColorBindlessIndex,
     *  by recordPostProcessCompositeDrawCommands() — see its own doc comment. */
    void recordStraightAlphaPass();

    /** @brief Records RetroArch's OriginalHistoryN ring-buffer snapshot —
     *  copies THIS frame's straight-alpha scene (straightAlphaImage,
     *  already un-premultiplied by recordStraightAlphaPass(), called
     *  immediately before this in recordPostProcessCompositeDrawCommands())
     *  into execution's own OriginalHistoryN ring buffer
     *  (VulkanShaderInstance::getOriginalHistoryImages()) at its current write
     *  cursor (VulkanShaderInstance::getOriginalHistoryCursor()), then advances
     *  that cursor. Gated by the caller on
     *  execution.getOriginalHistoryDepth() > 0 — a chain reflecting no
     *  OriginalHistoryN texture at all has no ring to write, so this is
     *  never called for it.
     *
     *  Barriers straightAlphaImage eShaderReadOnlyOptimal ->
     *  eTransferSrcOptimal (srcAccess combines eShaderRead, the layout this
     *  image already carries per recordStraightAlphaPass()'s own final
     *  barrier, with eColorAttachmentWrite, that same barrier's own
     *  predecessor access — conservative, mirrors this file's other
     *  same-layout-chain barriers), barriers the ring's own write slot
     *  eUndefined -> eTransferDstOptimal (discard-transition, same WAR-
     *  hazard shape as recordSingleBufferPass()'s own ping-pong write
     *  barrier — only reachable once the ring has wrapped at least once),
     *  vkCmdCopyImage (single region, full swapchainExtent, matching both
     *  images' own extent), barriers the write slot -> eShaderReadOnlyOptimal
     *  (for a LATER frame's OriginalHistoryK resolution to sample), barriers
     *  straightAlphaImage back -> eShaderReadOnlyOptimal (unchanged from
     *  before this call, for every downstream read on this same composite
     *  path), then advances the cursor.
     *
     *  ORDERING SEMANTICS (SSOT — see also VulkanGraphics::getSlangTextureBindings()'s
     *  own OriginalHistory resolution, which derives its read-slot index
     *  from the SAME cursor value, read AFTER this call's own advance
     *  below): the copy this frame lands at the cursor's PRE-advance value —
     *  frame n's own snapshot, resolved directly as Original/OriginalHistory0
     *  elsewhere, never through this ring. After this call's advance,
     *  (cursor - 1 + count) % count is that same just-written slot; frame
     *  n - N (N >= 1, N <= execution.getOriginalHistoryDepth()) sits at
     *  (cursor - 1 - N + count) % count — with ring count ==
     *  getOriginalHistoryDepth() + 1, the slot this call overwrites THIS
     *  frame is never one the deepest read (N == getOriginalHistoryDepth())
     *  still needs THIS SAME frame (see VulkanShaderInstance::originalHistoryImages'
     *  own doc comment for why count carries that +1).
     *  @param execution  shader's built execution — supplies the ring buffer
     *                    and cursor this call reads/writes.
     */
    void recordOriginalHistoryCopy (VulkanShaderInstance& execution);

    /** @brief Runs shader's buffer passes (offscreen, at its own
     *  resolutionScale-scaled extent), draws its Image pass into its own
     *  offscreen gather target (execution.getImagePassGatherTarget(),
     *  VulkanShaderInstance::imagePassGatherTarget's framebuffer, against that
     *  target's own stored gatherTarget.renderPass — always this VulkanGraphics's
     *  colorFormat entry, unconditionally, VulkanShaderInstance::build()'s own doc
     *  comment — raw, unmodified colour, no opacity math: see
     *  jam_vulkan/shader/shadertoy_wrapper.frag for VulkanShaderFormat::shadertoy,
     *  or the pass's own already-complete main() compiled as-is for
     *  VulkanShaderFormat::slang (no wrapper at all — the zero-injection contract:
     *  real .slang sources declare their own layout(set,binding) resources,
     *  which the fixed-set-0 bindless macro wrapper would collide with;
     *  bindings resolved by SPIR-V reflection downstream instead)),
     *  then draws the post-process combine pass
     *  (getOrCreatePostProcessCombinePipeline(), post_process_combine.frag)
     *  into a fresh compositeRenderPass/swapchainFramebuffers[currentImageIndex]
     *  begin/end pair — the real swapchain composite this VulkanShader's
     *  post-process chain produces, replacing recordSceneCompositeDrawCommands()'s
     *  identity composite for this frame. Called by endFrame() between the
     *  scene render pass ending and the command buffer's submit; no scene
     *  render pass is active at that point, so shader's buffer passes record
     *  with resumeAfter = false (mirrors recordShaderBufferPasses()'s
     *  documented resumeAfter contract) and this method opens/closes
     *  gatherTarget's own offscreen render pass itself for its own
     *  Image-pass gather draw (raw begin/end, same as
     *  recordSingleBufferPass() — this offscreen render pass never touches
     *  renderPassActive, see its own doc comment).
     *
     *  Calls recordStraightAlphaPass() FIRST, un-premultiplying sceneColorImage
     *  into straightAlphaImage — every user post-process shader sees straight
     *  alpha, never the premultiplied scene directly (locked contract: glass
     *  alpha is immutable, so a user shader must be able to read/replace RGB
     *  without the alpha channel corrupting it; the post-process combine
     *  pass's own post_process_combine.frag re-premultiplies by scene.a
     *  before this composite's opaque write — see
     *  jam_vulkan/shaders/post_process_combine.frag).
     *
     *  Calls recordOriginalHistoryCopy() SECOND, immediately after
     *  recordStraightAlphaPass() and BEFORE recordShaderBufferPasses() below
     *  — gated on execution.getOriginalHistoryDepth() > 0 — snapshotting
     *  THIS frame's straight-alpha scene into execution's own
     *  OriginalHistoryN ring buffer before any pass this same frame can read
     *  it (see that method's own doc comment for the full ordering/cursor-
     *  arithmetic contract).
     *
     *  Channel mapping (SSOT, the one rule this engine applies wherever a
     *  resolved scene texture actually exists to fall back to —
     *  VulkanShaderInstance::stampChannels()): channels[] reads
     *  execution.getChannelBindlessIndex() for a buffer pass declared at that
     *  ordinal, or — since this composite is the one call site where a fully
     *  resolved scene texture is available — straightAlphaImage's own bindless
     *  slot (NOT sceneColorImage's — see recordStraightAlphaPass()'s doc
     *  comment) for every ordinal beyond it. A post-process VulkanShader with no
     *  buffer passes at all (the common case) therefore reads the straight-
     *  alpha scene through whichever channel(s) its Image pass samples. No-op
     *  (matches recordSceneCompositeDrawCommands()'s own graceful degradation)
     *  when sceneColorImage has no bindless slot (bindless array was at
     *  capacity when it was created).
     *  @param shader           The installed post-process chain (VulkanEngine::getPostProcess()).
     *  @param execution        shader's built execution (already confirmed ready by
     *                          the caller's getOrCreateShaderInstance() call).
     *  @param opacity          VulkanEngine::getPostProcess()'s own opacity value,
     *                          stamped into the Image pass's push constant
     *                          (VulkanShader carries no opacity field, see
     *                          jam_VulkanShader.h's VulkanShader doc comment).
     *  @param resolutionScale  VulkanEngine::getPostProcess()'s own resolution
     *                          scale, forwarded to recordShaderBufferPasses().
     *  @param mouse            This call's iMouse value, forwarded verbatim to
     *                          recordShaderBufferPasses() (see its own doc
     *                          comment) — endFrame()'s own call site. */
    void recordPostProcessCompositeDrawCommands (const VulkanShader& shader,
                                                 VulkanShaderInstance& execution,
                                                 float opacity,
                                                 float resolutionScale,
                                                 const std::array<float, 4>& mouse);

    /** @brief recordPostProcessCompositeDrawCommands() step — records the
     *  Image pass's own offscreen gather draw (barrier, render-pass begin,
     *  bind/push, pipeline bind, viewport/scissor, draw, render-pass end,
     *  barrier, ping-pong currentReadHalf advance) into
     *  execution.getImagePassGatherTarget() — see
     *  recordPostProcessCompositeDrawCommands()'s own doc comment for the
     *  Gather-stage contract this orchestrates via
     *  recordPostProcessGatherPassBegin()/recordPostProcessGatherBindAndPush()/
     *  recordPostProcessGatherDrawAndEnd() below.
     *  @param execution     shader's built execution.
     *  @param baseUniforms  recordShaderBufferPasses()'s own returned base uniforms.
     *  @param opacity       See recordPostProcessCompositeDrawCommands()'s own doc comment.
     *  @return The stamped imageUniforms — reused verbatim by
     *          recordPostProcessCombineDraw()'s own channels[0] overwrite. */
    VulkanShaderUniforms recordPostProcessGatherDraw (VulkanShaderInstance& execution,
                                                      const VulkanShaderUniforms& baseUniforms,
                                                      float opacity);

    /** @brief recordPostProcessGatherDraw() step — resolves the write half
     *  (ping-pong toggle, degenerates to always 0 for a non-feedback,
     *  single-image gather target), barriers it into
     *  eColorAttachmentOptimal (discard-transition — same WAR-hazard shape as
     *  recordSingleBufferPass()'s own ping-pong write barrier), and begins
     *  execution.getImagePassGatherTarget()'s own offscreen render pass
     *  against that write half's framebuffer.
     *  @param execution  shader's built execution.
     *  @return The resolved write half — threaded into
     *          recordPostProcessGatherDrawAndEnd()'s own post-pass barrier/
     *          currentReadHalf advance. */
    int recordPostProcessGatherPassBegin (VulkanShaderInstance& execution);

    /** @brief recordPostProcessGatherDraw() step — binds this draw's
     *  descriptor set(s)/push constants: VulkanShaderFormat::slang resolves this
     *  pass's own reflected texture bindings (getSlangTextureBindings())
     *  and refreshes its per-pass descriptor set/UBO/push constant
     *  (VulkanShaderInstance::refreshSlangPass()) before binding its OWN dedicated
     *  descriptor set/pipeline layout; VulkanShaderFormat::shadertoy binds the
     *  shared bindless set and pushes @p imageUniforms directly — mirrors
     *  recordSingleBufferPass()'s own identical two-branch bind/push shape.
     *  @param execution      shader's built execution.
     *  @param imageUniforms  This draw's already-stamped uniforms (recordPostProcessGatherDraw()'s own local).
     *  @param gatherExtent   execution.getImagePassGatherTarget().extent — this
     *                        draw's own render target extent. */
    void recordPostProcessGatherBindAndPush (VulkanShaderInstance& execution,
                                             const VulkanShaderUniforms& imageUniforms,
                                             vk::Extent2D gatherExtent);

    /** @brief recordPostProcessGatherDraw() step — binds the gather target's
     *  own pipeline, sets viewport/scissor, issues the fullscreen draw (slang
     *  quad strip or Shadertoy triangle, selected by
     *  execution.usesSlangPipeline()), ends the render pass, barriers the
     *  written image for sampling (same-layout memory-visibility barrier,
     *  mirrors recordSingleBufferPass()'s own post-pass barrier shape), then
     *  advances execution.getImagePassGatherTarget()'s own currentReadHalf to
     *  @p writeHalf.
     *  @param execution      shader's built execution.
     *  @param imageUniforms  See recordPostProcessGatherBindAndPush()'s own doc comment.
     *  @param writeHalf      recordPostProcessGatherPassBegin()'s own resolved write half. */
    void recordPostProcessGatherDrawAndEnd (VulkanShaderInstance& execution,
                                            const VulkanShaderUniforms& imageUniforms,
                                            int writeHalf);

    /** @brief recordPostProcessCompositeDrawCommands() step — the real
     *  swapchain composite this VulkanShader's post-process chain produces,
     *  replacing recordSceneCompositeDrawCommands()'s identity composite for
     *  this frame: samples the just-written gather target back through
     *  channels[0] (execution.getImagePassGatherTarget()'s own currentReadHalf,
     *  already advanced by recordPostProcessGatherDrawAndEnd() before this
     *  runs), opens/closes compositeRenderPass raw (no scene render pass is
     *  active at this call site), and draws getOrCreatePostProcessCombinePipeline().
     *  Sets renderPassActive true/false around its own begin/end for
     *  invariant symmetry — this call is always terminal (endFrame()'s last
     *  composite step before commandBuffer.end()/present).
     *  @param execution      shader's built execution.
     *  @param imageUniforms  recordPostProcessGatherDraw()'s own returned, already-stamped uniforms. */
    void recordPostProcessCombineDraw (VulkanShaderInstance& execution, const VulkanShaderUniforms& imageUniforms);

#endif

    //==========================================================================
    // Members
    //==========================================================================

    /** @brief Shared Vulkan device — not owned, must outlive this VulkanGraphics. */
    VulkanDevice& device;

    /** @brief WSI presentation cluster (surface, swapchain, per-image views,
     *  imageAvailableSemaphore, renderFinishedSemaphores) for this window. */
    VulkanSwapchain swapchain;

    VulkanMsaaCalibration msaaCalibration;

    /** @brief True once beginFrame()'s acquireNextImageKHR or endFrame()'s
     *  presentKHR reports FrameDisposition::stale (out-of-date or
     *  surface-lost) — consumed and cleared by the next beginFrame(), which
     *  heals through resize() before acquiring again. */
    bool isSwapchainStale { false };

    // True once endFrame() has completed at least one submitted frame since
    // the scene target was last (re)created. createSceneTarget() resets this
    // to false. Consulted by beginFrame() itself to choose resumeRenderPass()
    // (LOAD, preserving persisted scene content, followed by a one-time
    // stencil clear before any clip is recorded) over the CLEAR path in
    // beginRenderPass(); also consulted via isSceneContentValid() by
    // VulkanEngine::createContext to decide whether the peer's recovered
    // dirty rect may be applied as the frame's initial clip — skipped when
    // false, since that frame's CLEAR path requires a full-window record
    // regardless of what the peer reports.
    bool sceneContentValid { false };

#if JUCE_WINDOWS
    std::unique_ptr<VulkanComposition> composition {};
    VulkanImage compositionImage {};
    vk::Framebuffer compositionFramebuffer {};
#endif

    /** @brief Per-swapchain-image vk::Framebuffers — owned. Built against
     *  compositeRenderPass; holds only the identity-composite draw, not scene
     *  content (see sceneFramebuffer). */
    jam::Array<vk::Framebuffer> swapchainFramebuffers {};

    /** @brief The one scene framebuffer (VulkanGraphics-owned persistent
     *  images, independent of currentImageIndex), built against renderPass/
     *  renderPassLoad. Combines sceneMsaaColorImage + stencilImage + sceneColorImage. */
    vk::Framebuffer sceneFramebuffer {};

    /** @brief Scene render pass — CLEAR_OP on load. 3 attachments (MSAA
     *  color, MSAA stencil, single-sample resolve); no longer targets the swapchain
     *  directly. TransparencyLayer/VulkanWindingScratch's own framebuffers are also
     *  built against this same vk::RenderPass object (an established pipeline-
     *  compatibility precedent). */
    vk::RenderPass renderPass {};

    /** @brief Scene render pass, LOAD_OP_LOAD variant — resumes scene rendering
     *  (endTransparencyLayer(), recordWindingCompositeDrawCommands()) after an
     *  offscreen composite interrupts it. Same 3-attachment shape as renderPass. */
    vk::RenderPass renderPassLoad {};

    /** @brief The identity-composite render pass. Single-sample, single
     *  color attachment that IS the swapchain image — the one render pass that
     *  legitimately still targets the swapchain directly (the final composite, not
     *  UI content). Extent-independent — never rebuilt by resize(). */
    vk::RenderPass compositeRenderPass {};

    /** @brief Command pool backing commandBuffer. */
    vk::CommandPool commandPool {};

    /** @brief Primary command buffer for the current frame. */
    vk::CommandBuffer commandBuffer {};

    /** @brief CPU-side fence — waited before beginning a new frame. */
    vk::Fence inFlightFence {};

    /** @brief True for an offscreen VulkanGraphics instance (self-keyed by its
     *  own address — getNativeHandle() returns `this`, not nullptr). Cached at
     *  createOffscreen() time — endFrame() uses this to branch between the
     *  offscreen recording path and the windowed swapchain path. */
    bool offscreenInstance { false };

    /** @brief CPU-side fence for the single offscreen command stream —
     *  used ONLY by the depth-0 submitOffscreenAndWait(). Created once
     *  in createOffscreen(). */
    vk::Fence offscreenFence {};

    /** @brief Pipeline collection for this surface. */
    VulkanPipelines pipelines {};

    VulkanImageEffects imageEffects;

    /** @brief RAII projection storage buffer (mat4, persistently mapped,
     *  VMA_MEMORY_USAGE_CPU_ONLY + VMA_ALLOCATION_CREATE_MAPPED_BIT — see the
     *  coherency rationale at createDrawState()'s allocation site,
     *  jam_VulkanGraphicsSetupDrawState.cpp). */
    VulkanBuffer projectionBuffer {};

    /** @brief Shared descriptor pool for the projection storage buffer and VulkanPrimitiveRecord
     *  storage-buffer descriptors (set 3 and set 2). Reset at depth-0 of each
     *  offscreen epoch; each beginOffscreenFrame allocates sets monotonically.
     *  Resume allocates a fresh set2 per return to an outer recording. Sized for
     *  maxDescriptorSets total (3 * maxRecordingsPerFrame). */
    vk::DescriptorPool descriptorPool {};

    /** @brief Projection mat4 descriptor set — bound as set 3 each frame. */
    vk::DescriptorSet projectionDescriptorSet {};

    /** @brief VulkanPrimitiveRecord storage-buffer descriptor set — bound as set 2 each frame. */
    vk::DescriptorSet recordDescriptorSet {};

    /** @brief Effective sampled-image array bound for set 1 binding 0:
     *  maxBindlessTextures clamped against the hardware-queried ceiling
     *  (VulkanDevice::getMaxDescriptorSetUpdateAfterBindSampledImages()). */
    uint32_t bindlessTextureCapacity { 0 };

    /** @brief Persistent descriptor pool for the bindless texture array set.
     *  Flagged UPDATE_AFTER_BIND, never reset per frame (unlike descriptorPool) —
     *  outlives every individual texture upload. */
    vk::DescriptorPool bindlessDescriptorPool {};

    /** @brief The one persistent set 1 bound every frame, written
     *  incrementally (once per unique texture/atlas/transparency-target) at
     *  upload/(re)creation time. */
    vk::DescriptorSet bindlessTextureDescriptorSet {};

    /** @brief RAII stencil image + view for path and alpha clipping (main scene's
     *  clip-depth stencil). MSAA (activeSampleCount-sampled); shared as
     *  the scene render pass's attachment 1. No longer shared with
     *  TransparencyLayer, which owns its own per-level stencil. */
    VulkanImage stencilImage {};

    /** @brief Transient-eligible-by-usage-shape but deliberately NOT
     *  vk::ImageUsageFlagBits::eTransientAttachment (see createSceneTarget()'s doc
     *  comment) MSAA color attachment for the scene render pass. Scene attachment 0. */
    VulkanImage sceneMsaaColorImage {};

    /** @brief The single-sample 8-bit UNORM scene texture that
     *  sceneMsaaColorImage resolves into every time the scene render pass ends.
     *  Persistent for this VulkanGraphics instance's lifetime (recreated on resize by
     *  createSceneTarget()); read by the identity composite via the
     *  bindless texture array (sceneColorBindlessIndex). Scene attachment 2
     *  (pResolveAttachments). */
    VulkanImage sceneColorImage {};

    /** @brief sceneColorImage's stable bindless array slot — assigned once by
     *  updateSceneColorBindlessDescriptor() and held for this VulkanGraphics
     *  instance's entire lifetime (unlike TransparencyLayer::bindlessIndex,
     *  never reset to -1 on resize): createSceneTarget() recreates
     *  sceneColorImage's view identity every resize, but
     *  updateSceneColorBindlessDescriptor() re-writes the descriptor at the
     *  SAME slot rather than reassigning one, avoiding a leaked slot per
     *  resize (resize()'s waitIdle() already guarantees no in-flight frame
     *  still samples the old view by the time the rewrite happens). */
    int sceneColorBindlessIndex { -1 };

#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
    /** @brief The post-process path's un-premultiplied scene target — same
     *  format/sample-count/extent as sceneColorImage, holding the SAME frame's
     *  content merely un-premultiplied by recordStraightAlphaPass(). Recreated
     *  alongside sceneColorImage by createStraightAlphaTarget(). Built against
     *  the shared single-sample offscreen render pass
     *  (getOrCreateShaderOffscreenRenderPass()), not a dedicated render pass.
     *  eTransferSrc (alongside eColorAttachment/eSampled, createStraightAlphaTarget()'s
     *  own creation site) — recordOriginalHistoryCopy() copies this image's
     *  content into a VulkanShaderInstance's own OriginalHistoryN ring buffer via
     *  vk::CommandBuffer::copyImage, which requires the source image to have
     *  been created with this usage bit (mirrors stencilImage's own identical
     *  eTransferSrc addition, createStencilImage()'s doc comment). */
    VulkanImage straightAlphaImage {};

    /** @brief straightAlphaFramebuffer's own SECOND (depth) attachment —
     *  getOrCreateShaderOffscreenRenderPass()'s shared render-pass shape now
     *  carries an unconditional depth attachment (that method's own doc
     *  comment), so this VulkanGraphics-owned framebuffer needs one too, exactly
     *  like every VulkanRenderResources' own depthImage (that struct's own doc
     *  comment) — inert scratch space, re-cleared per pass, never sampled;
     *  straightAlphaFramebuffer's own draw (VulkanGraphics::recordStraightAlphaPass())
     *  never tests or writes it. Recreated alongside straightAlphaImage by
     *  createStraightAlphaTarget(). */
    VulkanImage straightAlphaDepthImage {};

    /** @brief Framebuffer for straightAlphaImage + straightAlphaDepthImage,
     *  built against getOrCreateShaderOffscreenRenderPass() — recreated (old
     *  instance destroyed first) whenever createStraightAlphaTarget() runs. */
    vk::Framebuffer straightAlphaFramebuffer {};

    /** @brief straightAlphaImage's stable bindless array slot — assigned once
     *  by updateStraightAlphaBindlessDescriptor() and held for this VulkanGraphics
     *  instance's entire lifetime, mirroring sceneColorBindlessIndex's exact
     *  lifetime-stable convention (see its doc comment). */
    int straightAlphaBindlessIndex { -1 };
#endif

    /** @brief The identity-composite pipeline (instanced.vert +
     *  image.frag, single-sample, Blend::opaque-equivalent), built once by
     *  createCompositePipeline() against compositeRenderPass. See its doc comment
     *  for why this is a distinct vk::Pipeline rather than ID::imageInstanced. */
    vk::Pipeline compositePipeline {};

    /** @brief Per-frame path vertex/index buffer (host-visible, grows on demand). */
    VulkanFrameBuffer pathFrameBuffer {};

    /** @brief Per-frame VulkanPrimitiveRecord storage buffer (host-visible, grows on demand),
     *  shared by every rect/image/glyph/clip-mask instanced draw. */
    VulkanPrimitiveRecordBuffer primitiveRecordBuffer {};

    /** @brief Linear clamp-to-edge sampler shared by all drawImage calls. */
    vk::Sampler linearSampler {};

    /** @brief Owns the per-frame growable staging buffer arena backing
     *  allocateStaging() — see VulkanStagingArena's own class doc comment. */
    VulkanStagingArena stagingArena;

    /** @brief This window's RAII registration — constructed against the
     *  native platform handle passed to create() (NSView* on macOS, HWND on
     *  Windows), so registerGlyphAtlasSlots() can key jam::VulkanBindlessTexture's
     *  registry by this window's own identity (see getNativeHandle()), and
     *  its destructor withdraws every registered slot assignment this window
     *  was ever assigned. Replaces the former raw nativeHandle member plus
     *  ~VulkanGraphics()'s own manual clearBindlessIndex() cleanup block — see
     *  VulkanBindlessInstance's own class doc comment. */
    VulkanBindlessInstance bindlessInstance;

    /** @brief Owns this window's bindless array slot allocator and
     *  juce::ImagePixelData::Listener-driven per-root slot recycling — see
     *  jam::VulkanBindlessRegistry's own class doc comment. Constructed
     *  against bindlessInstance.getHandle() (the resolved native handle),
     *  after bindlessInstance so that value already exists. */
    VulkanBindlessRegistry bindlessRegistry;

    /** @brief Owns the stack of offscreen transparency-layer render targets. */
    VulkanTransparencyStack transparencyStack;

    /** @brief Owns the isolated scratch stencil+color target for fillPath()'s
     *  complex-path (multi-subpath / even-odd) winding-accumulate-then-cover
     *  technique. Never shares the main stencil attachment's
     *  clip-depth bits (jam_VulkanPipelinesState.cpp's stencilWriteState()). */
    VulkanWindingScratch windingScratch;

    /** @brief Index of the acquired swapchain image for the current frame. */
    uint32_t currentImageIndex { 0 };

    /** @brief Number of active rendering contexts for this frame. */
    int activeContextCount { 0 };

    /** @brief Epoch append cursor into the recordings pool — each
     *  beginOffscreenFrame takes entry recordingsUsed++ (get-or-create).
     *  Reset to 0 at depth-0 begin (alongside pool/arena resets).
     *  Rule-5 sibling of VulkanStagingArena's stagingUsed. */
    int recordingsUsed { 0 };

    /** @brief Stack of indices into recordings for currently-active
     *  recording levels. Pushed at beginOffscreenFrame, popped at
     *  endFrame's offscreen branch. activeRecordings.size() IS the
     *  current nesting depth — no shadow counter.
     *  Rule-5 sibling of activeContextCount (active* naming). */
    jam::Array<int> activeRecordings {};

    /** @brief Grow-only pool of per-recording-level state for reentrant
     *  offscreen rendering. Indexed by epoch append order (recordingsUsed),
     *  not by depth. Entries are created on demand and reused across frames
     *  by index, never within an epoch — growth mirrors the arena canon. */
    jam::Owner<Recording> recordings {};

    /** @brief True while a render pass is being recorded into commandBuffer. */
    bool renderPassActive { false };

    /** @brief Monotonically increasing frame index, incremented once per real
     *  frame by resetResources() (called from beginFrame(), guarded by the
     *  same activeContextCount idempotency check as the rest of that
     *  per-frame reset). SSOT clock for VulkanShaderInstance::lastUsedFrame —
     *  stamped at getOrCreateShaderInstance() bind time and swept by
     *  resetResources() to move an owner-replaced (destroyed) VulkanShader's
     *  now-unreachable shaderInstances entry into previousShaderInstances. */
    uint64_t frameCounter { 0 };

    /** @brief Clamp-to-edge nearest-filter sampler — ImageResample::nearest's
     *  counterpart to linearSampler, same owner, created eagerly by
     *  createDrawState() alongside linearSampler and written once into the
     *  persistent bindless set's binding 2 (see VulkanPipelines::getLayoutSet1()'s
     *  doc comment). */
    vk::Sampler nearestSampler {};

#if KUASSA_VULKAN_RUNTIME_SHADER_COMPILER
    /** @brief The runtime-shader-compiler lane's own GPU execution registry —
     *  VulkanShaderInstance cache, shared setup helpers, and mesh-backed
     *  material-range draw. This class threads its own frame fabric into the
     *  registry's public surface at each call site — no back-reference the
     *  other way. */
    VulkanShaderRegistry shaderRegistry;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanGraphics)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
