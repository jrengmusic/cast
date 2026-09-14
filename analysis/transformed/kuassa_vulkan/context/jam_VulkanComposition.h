namespace jam
{

/** @brief Per-window Windows DirectComposition present leg — the shared
 *  D3D11 texture Vulkan renders into, its DXGI swapchain-for-composition,
 *  and the DirectComposition target/visual that hosts it on the window.
 *
 *  Used in place of the WSI swapchain path whenever
 *  jam::VulkanDevice::isCompositionInteropSupported() is true — see
 *  jam::VulkanGraphics's own composition member and its present()/
 *  resize() call sites. */
struct VulkanComposition
{
    /** Number of buffers in the composition swapchain. */
    static constexpr UINT swapchainBufferCount { 2 };
    /** Swapchain present sync interval (0 = no vsync wait). */
    static constexpr UINT presentSyncInterval { 0 };

    VulkanComposition() = default;
    ~VulkanComposition() = default;

    /** @brief Builds a VulkanComposition for @p hwnd against the shared
     *  @p compositionDevice: creates the shared D3D11 texture
     *  (createSharedTexture()), the DXGI swapchain-for-composition, and the
     *  DirectComposition target/visual, then commits them.
     *  @param compositionDevice  Shared per-adapter DirectComposition device.
     *  @param hwnd                Window this VulkanComposition presents into.
     *  @return                    The constructed VulkanComposition, or nullptr if
     *                             any creation step failed. */
    static std::unique_ptr<VulkanComposition> create (VulkanCompositionDevice& compositionDevice, HWND hwnd);

    /** @brief Transfers ownership of the shared texture's NT export handle
     *  to the caller — see VulkanSharedTextureExport::releaseTextureExport().
     *  @return The handle previously held by textureExport, or nullptr if
     *          none was held. */
    HANDLE releaseTextureExport() noexcept;

    /** @brief Returns @p hwnd's current client-area size via GetClientRect().
     *  @param hwnd  Window to query.
     *  @return       Client-area width/height, physical pixels. */
    vk::Extent2D getClientExtent (HWND hwnd) const;

    /** @brief Presents this window's frame: copies sharedTexture into the
     *  swapchain's current back buffer (GetBuffer -> CopyResource), then
     *  presents the swapchain.
     *
     *  The caller has already host-waited on the frame's in-flight fence
     *  (queue submit + vk::Device::waitForFences()) before calling this —
     *  Vulkan's rendering into sharedTexture is therefore guaranteed
     *  complete before CopyResource reads from it on @p compositionDevice's
     *  own immediate context.
     *  @param compositionDevice  Shared per-adapter DirectComposition device
     *                            whose immediate context performs the copy. */
    void present (VulkanCompositionDevice& compositionDevice);

    /** @brief Recreates the shared texture at @p width x @p height and
     *  resizes the swapchain's buffers to match.
     *  @param compositionDevice  Shared per-adapter DirectComposition device.
     *  @param width               New client-area width, physical pixels.
     *  @param height              New client-area height, physical pixels.
     *  @return                    true if both steps succeeded. */
    bool resize (VulkanCompositionDevice& compositionDevice, uint32_t width, uint32_t height);

    /** D3D11 texture Vulkan renders into, shared into DirectComposition via textureExport. */
    juce::ComSmartPtr<ID3D11Texture2D> sharedTexture;
    /** DXGI swapchain-for-composition sharedTexture is copied into on present(). */
    juce::ComSmartPtr<IDXGISwapChain1> swapchain;
    /** DirectComposition target bound to the presenting window. */
    juce::ComSmartPtr<IDCompositionTarget> dcompTarget;
    /** DirectComposition visual hosting swapchain on the window. */
    juce::ComSmartPtr<IDCompositionVisual> dcompVisual;

    /** @brief NT shared-handle export of sharedTexture — created alongside
     *  sharedTexture by createSharedTexture(), consumed once by the caller
     *  via releaseTextureExport() to import sharedTexture into Vulkan
     *  (jam::VulkanGraphics::importCompositionImage()). */
    VulkanSharedTextureExport textureExport;

private:
    /** @brief Creates sharedTexture at @p width x @p height
     *  (D3D11_RESOURCE_MISC_SHARED_NTHANDLE) and its NT export handle
     *  (textureExport) — called by create() and by resize() on every resize.
     *  @param compositionDevice  Shared per-adapter DirectComposition device.
     *  @param width               Texture width, physical pixels.
     *  @param height              Texture height, physical pixels.
     *  @return                    true if the texture and its export handle
     *                             were both created successfully. */
    bool createSharedTexture (VulkanCompositionDevice& compositionDevice, uint32_t width, uint32_t height);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanComposition)
};

}// namespace jam
