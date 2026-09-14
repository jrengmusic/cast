namespace jam
{

/** @brief Shared per-adapter Direct3D11/DirectComposition device, one per
 *  jam::VulkanDevice, not one per window.
 */
struct VulkanCompositionDevice
{
    VulkanCompositionDevice() = default;

    /** Creates a composition device bound to the given DXGI adapter.
        @param adapterLuid Locally unique identifier of the DXGI adapter to bind to.
        @return Newly constructed composition device, or nullptr on failure.
    */
    static std::unique_ptr<VulkanCompositionDevice> create (LUID adapterLuid);

    /** DXGI factory used to enumerate/bind the adapter. */
    juce::ComSmartPtr<IDXGIFactory2> dxgiFactory;
    /** Direct3D11 device backing the composition device. */
    juce::ComSmartPtr<ID3D11Device5> direct3DDevice;
    /** Immediate device context of direct3DDevice. */
    juce::ComSmartPtr<ID3D11DeviceContext4> immediateContext;
    /** DXGI device interface of direct3DDevice, used to create the composition device. */
    juce::ComSmartPtr<IDXGIDevice> dxgiDevice;

    /** @brief Shared DirectComposition device for this adapter — one per
     *  jam::VulkanDevice, not one per window. Every window's own
     *  VulkanComposition::create() builds its per-window dcompTarget/dcompVisual
     *  from this single shared device instead of creating its own. */
    juce::ComSmartPtr<IDCompositionDevice> dcompDevice;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanCompositionDevice)
};

}// namespace jam
