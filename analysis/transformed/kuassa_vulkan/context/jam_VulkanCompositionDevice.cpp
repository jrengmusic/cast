namespace jam
{
/*____________________________________________________________________________*/
std::unique_ptr<VulkanCompositionDevice> VulkanCompositionDevice::create (LUID adapterLuid)
{
    juce::ComSmartPtr<IDXGIFactory2> dxgiFactory;

    if (FAILED (CreateDXGIFactory2 (0, __uuidof (IDXGIFactory2), reinterpret_cast<void**> (dxgiFactory.resetAndGetPointerAddress()))))
        return nullptr;

    juce::ComSmartPtr<IDXGIAdapter1> matchedAdapter;

    for (UINT adapterIndex = 0; ; ++adapterIndex)
    {
        juce::ComSmartPtr<IDXGIAdapter1> adapter;

        if (dxgiFactory->EnumAdapters1 (adapterIndex, adapter.resetAndGetPointerAddress()) == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_ADAPTER_DESC1 adapterDesc {};

        if (SUCCEEDED (adapter->GetDesc1 (&adapterDesc))
            and adapterDesc.AdapterLuid.LowPart == adapterLuid.LowPart
            and adapterDesc.AdapterLuid.HighPart == adapterLuid.HighPart)
        {
            matchedAdapter = adapter;
            break;
        }
    }

    if (matchedAdapter == nullptr)
        return nullptr;

    juce::ComSmartPtr<ID3D11Device> baseDevice;

    if (FAILED (D3D11CreateDevice (matchedAdapter,
                                   D3D_DRIVER_TYPE_UNKNOWN,
                                   nullptr,
                                   D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                   nullptr,
                                   0,
                                   D3D11_SDK_VERSION,
                                   baseDevice.resetAndGetPointerAddress(),
                                   nullptr,
                                   nullptr)))
        return nullptr;

    auto compositionDevice { std::make_unique<VulkanCompositionDevice>() };
    compositionDevice->dxgiFactory = dxgiFactory;

    if (FAILED (baseDevice.QueryInterface (compositionDevice->direct3DDevice)))
        return nullptr;

    juce::ComSmartPtr<ID3D11DeviceContext> baseContext;
    compositionDevice->direct3DDevice->GetImmediateContext (baseContext.resetAndGetPointerAddress());

    if (baseContext == nullptr)
        return nullptr;

    if (FAILED (baseContext.QueryInterface (compositionDevice->immediateContext)))
        return nullptr;

    if (FAILED (compositionDevice->direct3DDevice.QueryInterface (compositionDevice->dxgiDevice)))
        return nullptr;

    if (FAILED (DCompositionCreateDevice (compositionDevice->dxgiDevice, __uuidof (IDCompositionDevice), reinterpret_cast<void**> (compositionDevice->dcompDevice.resetAndGetPointerAddress()))))
        return nullptr;

    return compositionDevice;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
