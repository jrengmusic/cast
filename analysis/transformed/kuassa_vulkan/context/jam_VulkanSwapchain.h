namespace jam
{
/*____________________________________________________________________________*/
/** @brief Owns the WSI presentation cluster for one native window surface:
 *  the platform vk::SurfaceKHR, the vk::SwapchainKHR and its per-image
 *  views, and the two semaphore roles WSI acquisition/presentation need
 *  (imageAvailableSemaphore, renderFinishedSemaphores).
 *
 *  Created via create() — the windowed lane (real surface + real swapchain)
 *  — or via setFormatAndExtent() for an instance with no real surface
 *  (offscreen instances, Windows composition interop). recreate() rebuilds
 *  every surface-dependent handle for a new size, returning the outgoing
 *  vk::SwapchainKHR and its outgoing renderFinishedSemaphores so the caller
 *  can destroy them once its own device-idle wait has confirmed nothing can
 *  still reference them — this class does not know VulkanEngine.
 *
 *  Sole consumer: VulkanGraphics::swapchain. Framebuffers, render passes,
 *  and every FrameDisposition policy decision stay on VulkanGraphics — this
 *  class is mechanism only (acquire/present/recreate), never policy.
 */
class VulkanSwapchain
{
public:
    //==========================================================================
    // Constructor / Destructor
    //==========================================================================

    /** @brief Stores the device reference. Owns no Vulkan handles until create().
     *  @param device  Shared Vulkan device — not owned, must outlive this VulkanSwapchain.
     */
    explicit VulkanSwapchain (VulkanDevice& device);

    /** @brief WSI teardown ordered per spec — image views, then semaphores,
     *  then swapchain, then surface (VUID-vkDestroySurfaceKHR-surface-01266:
     *  every swapchain created for a surface must be destroyed before that
     *  surface). Every destroy is unconditional — vk.xml marks the handle
     *  parameter of vkDestroyImageView/vkDestroySemaphore/vkDestroySwapchainKHR/
     *  vkDestroySurfaceKHR optional="true", so an empty handle is a documented
     *  no-op. Called only after the owner's own device-idle wait. */
    ~VulkanSwapchain();

    //==========================================================================
    // Create
    //==========================================================================

    /** @brief Windowed create lane — creates the real vk::SurfaceKHR +
     *  vk::SwapchainKHR + per-image views/semaphores and the
     *  imageAvailableSemaphore (VUID-vkAcquireNextImageKHR-semaphore-01286:
     *  must be unsignaled — a fresh semaphore is always unsignaled). On
     *  Windows, the caller only takes this lane for the real-surface peer;
     *  DirectComposition interop (VulkanGraphics::composition) instead calls
     *  setFormatAndExtent() directly, since no real surface/swapchain exists
     *  for that lane.
     *  @param nativeHandle  Platform window handle (HWND on Windows, NSView*
     *                       on macOS, Window on Linux).
     *  @param width         Initial surface width in pixels.
     *  @param height        Initial surface height in pixels.
     *  @return true if every step succeeded.
     */
    bool create (void* nativeHandle, int width, int height);

    /** @brief Adopts @p format/@p extent directly, with no real surface or
     *  swapchain — VulkanGraphics::createOffscreen()'s one-time seed,
     *  beginOffscreenFrame()'s re-seed when a differently-sized target is
     *  bound, and resize()'s post-recreate refresh for Windows composition
     *  interop. No Vulkan call is made — always succeeds.
     *  @param format  Format to adopt.
     *  @param extent  Extent to adopt.
     */
    void setFormatAndExtent (vk::SurfaceFormatKHR format, vk::Extent2D extent);

    //==========================================================================
    // Recreate
    //==========================================================================

    /** @brief Rebuilds every surface-dependent handle for a new size: destroys
     *  the current image views immediately, captures and nulls the swapchain
     *  and imageAvailableSemaphore members before touching either — every
     *  member this function reassigns is left internally consistent on any
     *  failure path, never a stale or double-owned handle — then destroys
     *  and recreates imageAvailableSemaphore (same VUID-01286 rationale as
     *  create()), then calls createSwapchain() with the captured outgoing
     *  swapchain threaded through explicitly as
     *  vk::SwapchainCreateInfoKHR::oldSwapchain. The outgoing vk::SwapchainKHR
     *  and its outgoing renderFinishedSemaphores are NOT destroyed here —
     *  they are returned to the caller, which destroys them once its own
     *  device-idle wait (already completed before this call, by contract with
     *  VulkanGraphics::resize()) has confirmed nothing can still reference
     *  them.
     *  @param width   New surface width in pixels.
     *  @param height  New surface height in pixels.
     *  @return Whether the imageAvailableSemaphore recreate and
     *          createSwapchain() both succeeded, the outgoing
     *          vk::SwapchainKHR (empty if none existed), and the outgoing
     *          renderFinishedSemaphores (moved out — this instance's own
     *          array is cleared and rebuilt by createSwapchain()).
     */
    std::tuple<bool, vk::SwapchainKHR, jam::Array<vk::Semaphore>> recreate (int width, int height);

    //==========================================================================
    // Frame lifecycle
    //==========================================================================

    /** @brief Acquires the next presentable image, waiting on
     *  imageAvailableSemaphore with an infinite timeout (UINT64_MAX) — a
     *  device-lost swapchain returns eErrorDeviceLost rather than hanging
     *  (the OS watchdog converts a hung device to a lost one), so no finite
     *  bound is needed here. Mechanism only — the caller
     *  (VulkanGraphics::beginFrame()) classifies the returned vk::Result
     *  through its own FrameDisposition dispositions.
     *  @return The raw vk::Result and the acquired image index (index is
     *          only meaningful when the result indicates an image was acquired).
     */
    std::pair<vk::Result, uint32_t> acquireNextImage();

    /** @brief Presents @p imageIndex on @p queue, waiting on @p waitSemaphore.
     *  Mechanism only — the caller (VulkanGraphics::endFrame()) classifies the
     *  returned vk::Result through its own FrameDisposition dispositions.
     *  @param queue          Presentation-capable queue.
     *  @param imageIndex     Index acquired by acquireNextImage().
     *  @param waitSemaphore  Semaphore signalled once rendering into
     *                        @p imageIndex is complete — the same
     *                        renderFinishedSemaphore the caller's own
     *                        vkQueueSubmit signalled for this image
     *                        (getRenderFinishedSemaphore()).
     *  @return The raw vk::Result from vkQueuePresentKHR.
     */
    vk::Result present (vk::Queue queue, uint32_t imageIndex, vk::Semaphore waitSemaphore);

    //==========================================================================
    // Accessors
    //==========================================================================

    /** @brief Returns the number of swapchain images/views — the framebuffer
     *  count VulkanGraphics::createFramebuffers() builds against. */
    int getImageCount() const noexcept { return swapchainImageViews.size(); }

    /** @brief Returns the per-image vk::ImageView at @p index.
     *  @param index  Zero-based swapchain image index, < getImageCount(). */
    vk::ImageView getImageView (int index) const { return swapchainImageViews.at (index); }

    /** @brief Returns the current swapchain pixel extent. */
    vk::Extent2D getExtent() const noexcept { return swapchainExtent; }

    /** @brief Returns the selected swapchain surface format. */
    vk::SurfaceFormatKHR getFormat() const noexcept { return swapchainFormat; }

    /** @brief Returns the renderFinishedSemaphore at @p index — the signal
     *  semaphore VulkanGraphics::endFrame()'s own vkQueueSubmit signals for
     *  this image, and the same handle it then passes to present() as
     *  waitSemaphore. The submit that signals this semaphore is Graphics-owned
     *  command-buffer submission, entirely outside this class's WSI mechanism.
     *  @param index  Zero-based swapchain image index, < getImageCount(). */
    vk::Semaphore getRenderFinishedSemaphore (int index) const { return renderFinishedSemaphores.at (index); }

    /** @brief Returns imageAvailableSemaphore — the wait semaphore
     *  VulkanGraphics::endFrame()'s own vkQueueSubmit must wait on before the
     *  colour-attachment-output stage, mirroring getRenderFinishedSemaphore()'s
     *  rationale: that submit is Graphics-owned command-buffer submission,
     *  entirely outside this class's own acquire/present mechanism. */
    vk::Semaphore getImageAvailableSemaphore() const noexcept { return imageAvailableSemaphore; }

    /** @brief Returns the native surface's current extent — the queried
     *  vk::SurfaceCapabilitiesKHR::currentExtent for surface.
     *  VulkanGraphics::getSurfaceExtent() forwards here on every platform
     *  except the Windows composition-active branch, which reads
     *  VulkanComposition::getClientExtent() instead (composition is not
     *  owned by this class). */
    vk::Extent2D getSurfaceExtent() const;

#if JUCE_MAC
    /** @brief Returns the opaque CAMetalLayer pointer backing surface —
     *  VulkanGraphics::beginFrame()/resize() track this layer's own frame
     *  size independently of swapchainExtent for macOS resize healing.
     *  metalLayerRef is a surface-creation artifact: createSurface() wraps
     *  the NSView in a CAMetalLayer before creating the vk::SurfaceKHR
     *  from it. */
    void* getMetalLayerRef() const noexcept { return metalLayerRef; }
#endif

private:
    //==========================================================================
    // Private setup helpers
    //==========================================================================

    /** @brief Creates the platform-native vk::SurfaceKHR from the native window handle. */
    bool createSurface (void* nativeHandle);

    /** @brief Returns surface's current vk::SurfaceCapabilitiesKHR, and logs
     *  on failure (JUCE_DEBUG only) — shared by getSurfaceExtent() and
     *  createSwapchain(), the class's only two callers of
     *  getSurfaceCapabilitiesKHR.
     *  @return The raw vk::Result and the queried capabilities (default-
     *          constructed when the result is not vk::Result::eSuccess). */
    std::pair<vk::Result, vk::SurfaceCapabilitiesKHR> getSurfaceCapabilities() const;

    /** @brief Creates the vk::SwapchainKHR and retrieves swapchain image handles.
     *  @param width          Requested surface width in pixels, used only when
     *                         the surface does not dictate currentExtent.
     *  @param height         Requested surface height in pixels, used only when
     *                         the surface does not dictate currentExtent.
     *  @param oldSwapchain   The outgoing vk::SwapchainKHR to thread through as
     *                        vk::SwapchainCreateInfoKHR::oldSwapchain — empty on
     *                        first creation, the just-captured outgoing handle
     *                        on recreate().
     */
    bool createSwapchain (int width, int height, vk::SwapchainKHR oldSwapchain);

    /** @brief Selects the swapchain present mode: enumerates physicalDevice's supported
     *  modes for surface and prefers MAILBOX (low-latency triple buffering) when present,
     *  falling back to FIFO otherwise — FIFO is guaranteed supported by every Vulkan
     *  implementation that supports VK_KHR_surface at all (spec-mandated), so this
     *  fallback is always total, never a failure path.
     *  @return vk::PresentModeKHR::eMailbox if supported, otherwise vk::PresentModeKHR::eFifo.
     */
    vk::PresentModeKHR selectPresentMode() const;

    //==========================================================================
    // Members
    //==========================================================================

    /** @brief Shared Vulkan device — not owned, must outlive this VulkanSwapchain. */
    VulkanDevice& device;

    /** @brief Platform surface — owned. */
    vk::SurfaceKHR surface {};

    /** @brief Presentation swapchain — owned. */
    vk::SwapchainKHR swapchain {};

    /** @brief Current swapchain pixel dimensions. */
    vk::Extent2D swapchainExtent {};

    /** @brief Selected swapchain surface format. */
    vk::SurfaceFormatKHR swapchainFormat {};

    /** @brief Per-swapchain-image vk::ImageViews — owned. */
    jam::Array<vk::ImageView> swapchainImageViews {};

    /** @brief Signalled when a swapchain image is available for rendering. */
    vk::Semaphore imageAvailableSemaphore {};

    /** @brief Signalled when rendering is complete and the image is ready to
     *  present — one per swapchain image, indexed by the caller's own
     *  currentImageIndex. */
    jam::Array<vk::Semaphore> renderFinishedSemaphores {};

#if JUCE_MAC
    /** @brief Opaque CAMetalLayer pointer — createSurface()'s own surface-
     *  creation artifact on macOS; read back via getMetalLayerRef(). */
    void* metalLayerRef { nullptr };
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanSwapchain)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
