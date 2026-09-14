namespace jam
{
/*____________________________________________________________________________*/
/** @brief Owns one isolated, growable scratch stencil+color render target.
 *
 *  Sole consumer: VulkanGraphics::windingScratch backs fillPath()'s complex-path
 *  (multi-subpath / even-odd) winding-accumulate-then-cover technique. This
 *  class itself has no fillPath()-specific state; only its caller
 *  (VulkanGraphics::getOrCreateWindingScratch()) and the render pass/framebuffer
 *  it is reserve()'d against give it a role.
 *
 *  Completely decoupled from the main stencil attachment that carries the
 *  clip-nesting depth (jam_VulkanPipelinesState.cpp's stencilWriteState()) — this
 *  target's stencil bits mean nothing outside a single owning call and are
 *  re-cleared to 0 (LOAD_OP_CLEAR) every use. A bit-partition scheme cannot
 *  share the main stencil buffer between the two concerns: Vulkan stencil ops
 *  compute against the FULL stored byte before writeMask filters what gets
 *  written back, so this isolated target is required.
 *
 *  Sized to the CURRENT call's required extent, growing monotonically
 *  (power-of-2 doubling per axis — mirrors VulkanFrameBuffer::reserve()'s exact
 *  growth shape) rather than recreating on every differently-sized call the way
 *  TransparencyLayer does. TransparencyLayer's recreate-on-mismatch is only safe
 *  because it is driven by whole-window resize events between frames; this target
 *  may need to grow multiple times within a single frame across multiple calls,
 *  so the previous images/framebuffers must survive (not be destroyed) until the
 *  next safe per-frame reset point — exactly like VulkanFrameBuffer's previousBuffers.
 *
 *  Bound into the persistent bindless texture array once ready, so the finished
 *  scratch-color output can be sampled back — windingScratch's output composites
 *  onto the active render target via the imageInstanced/imageInstancedStencil
 *  pipeline.
 *
 *  Single instance per role (not a stack) — unlike transparency layers, calls
 *  against one role never nest within themselves, so one growable target per
 *  role suffices.
 */
class VulkanWindingScratch
{
public:
    //==========================================================================
    // Constructor / Destructor
    //==========================================================================

    /** @brief Stores the device reference. Owns no Vulkan handles until first use.
     *  @param device  Shared Vulkan device — not owned, must outlive this VulkanWindingScratch.
     */
    explicit VulkanWindingScratch (VulkanDevice& device);

    /** @brief Destroys the current framebuffer and any framebuffers still in
     *  previousFramebuffers; RAII Images self-destruct on member/vector destruction. */
    ~VulkanWindingScratch();

    //==========================================================================
    // Capacity
    //==========================================================================

    /** @brief Reserves scratch capacity for at least requiredExtent. Grows
     *  (power-of-2 doubling per axis) if exceeded; moves — never immediately
     *  destroys — the previous images/framebuffer into their previous* containers,
     *  so command buffer references recorded against them earlier this frame remain
     *  valid until resetPrevious().
     *  @param renderPass    Render pass the scratch framebuffer is created against.
     *  @param colorFormat   Swapchain surface format.
     *  @param sampleCount   Session-locked MSAA sample count.
     *  @param requiredExtent  Minimum pixel extent needed for the current call
     *                         (fillPath()'s device-space bounding box).
     */
    void reserve (vk::RenderPass renderPass, vk::SurfaceFormatKHR colorFormat,
                  vk::SampleCountFlagBits sampleCount, vk::Extent2D requiredExtent);

    /** @brief Releases the previous images/framebuffers kept alive only until the
     *  GPU finished the frame(s) that referenced them. windingScratch is drained
     *  once per frame at beginFrame() — mirrors VulkanFrameBuffer::resetUsage()'s
     *  per-frame previous-drain timing. Call only once the GPU work that
     *  referenced the drained capacity has been confirmed complete. */
    void resetPrevious();

    /** @brief Returns every bindlessIndex reserve() invalidated since the
     *  last resetPrevious() call, for the caller (VulkanGraphics::beginFrame())
     *  to release back to the bindless pool via VulkanGraphics::bindlessRegistry.
     *  releaseBindlessIndex() BEFORE calling resetPrevious() (drain order mirrors previousColorImages/
     *  previousFramebuffers — read, then clear).
     *  Deferred rather than released immediately at growth time (unlike
     *  VulkanTransparencyStack, whose recreation is resize-only): reserve() may
     *  grow multiple times within a single owning call's lifetime (this class's
     *  own doc comment), so an old slot may still be sampled by an earlier draw
     *  already recorded into the same command buffer — only safe to reissue once
     *  that work's own completion (fence wait or submitAndWait(), whichever gates
     *  resetPrevious() for that role) has confirmed the GPU is done. */
    const jam::Array<int>& getPreviousBindlessIndices() const noexcept { return previousBindlessIndices; }

    //==========================================================================
    // Accessors
    //==========================================================================

    /** @brief Returns the scratch framebuffer (color + owned stencil), sized to
     *  the current capacity extent. */
    vk::Framebuffer getFramebuffer() const noexcept { return framebuffer; }

    /** @brief Returns the scratch RESOLVE image's vk::Image handle, for layout-transition
     *  barriers. Named getResolveImage (not getColorImage): colorImage is
     *  now the MSAA attachment and can never be sampled directly (a multisample image
     *  cannot be a sampler2D source); resolveImage is the actual composited output. */
    vk::Image getResolveImage() const noexcept { return resolveImage.getImage(); }

    /** @brief Returns the scratch RESOLVE image's view, for bindless descriptor writes. */
    vk::ImageView getResolveView() const noexcept { return resolveImage.getView(); }

    /** @brief Returns the current allocated capacity extent (may exceed the
     *  current call's required extent — monotonic growth never shrinks). */
    vk::Extent2D getCapacityExtent() const noexcept { return capacityExtent; }

    /** @brief Returns the stable bindless array slot, or -1 if not yet
     *  assigned or over-capacity (graceful degradation, no LRU eviction). */
    int getBindlessIndex() const noexcept { return bindlessIndex; }

    /** @brief Sets the bindless array slot assigned by VulkanGraphics::getOrCreateWindingScratch().
     *  @param index  The assigned slot, or -1 if the bindless array was at capacity.
     */
    void setBindlessIndex (int index) noexcept { bindlessIndex = index; }

private:
    //==========================================================================
    // Members
    //==========================================================================

    /** @brief Shared Vulkan device — not owned, must outlive this VulkanWindingScratch. */
    VulkanDevice& device;

    /** @brief Owned scratch MSAA color image (swapchain format, activeSampleCount-
     *  sampled). Never sampled directly — resolveImage below is. */
    VulkanImage colorImage {};

    /** @brief Owned scratch MSAA stencil image (S8_UINT, activeSampleCount-sampled)
     *  — never shared with the main stencil attachment's clip-depth bits. */
    VulkanImage stencilImage {};

    /** @brief Owned single-sample resolve target — the actual sampled/
     *  bindless-bound composite output; colorImage resolves into this every time the
     *  scratch render pass ends. */
    VulkanImage resolveImage {};

    /** @brief Framebuffer combining colorImage + stencilImage + resolveImage,
     *  created against the caller-supplied render pass. */
    vk::Framebuffer framebuffer {};

    /** @brief Current allocated pixel extent of colorImage/stencilImage/framebuffer. */
    vk::Extent2D capacityExtent {};

    /** @brief Stable bindless array slot — reset to -1 whenever growth
     *  recreates colorImage (new vk::ImageView identity), mirroring TransparencyLayer's
     *  exact invalidation convention. The slot this replaces is pushed onto
     *  previousBindlessIndices below rather than dropped. */
    int bindlessIndex { -1 };

    /** @brief Bindless slots invalidated by growth since the last
     *  resetPrevious() call — see getPreviousBindlessIndices()'s doc comment for
     *  why release is deferred rather than immediate. Owner-sweep-free (plain
     *  ints, no Vulkan handles) — a jam::Array suffices, unlike
     *  previousColorImages/previousFramebuffers. */
    jam::Array<int> previousBindlessIndices {};

    /** @brief Prior-capacity color images kept alive until resetPrevious(). Owner
     *  sweep — vector-of-unique_ptr, pointer-stable, auto-deleting
     *  (jam_core/utilities/jam_Owner.h). */
    jam::Owner<VulkanImage> previousColorImages {};

    /** @brief Prior-capacity stencil images kept alive until resetPrevious(). Owner
     *  sweep — see previousColorImages. */
    jam::Owner<VulkanImage> previousStencilImages {};

    /** @brief Prior-capacity resolve images kept alive until resetPrevious().
     *  Owner sweep — see previousColorImages. */
    jam::Owner<VulkanImage> previousResolveImages {};

    /** @brief Prior-capacity framebuffers kept alive until resetPrevious() — raw
     *  handles (vk::Framebuffer has no RAII wrapper in this codebase; mirrors
     *  TransparencyLayer's own raw-handle framebuffer field). */
    jam::Array<vk::Framebuffer> previousFramebuffers {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanWindingScratch)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam