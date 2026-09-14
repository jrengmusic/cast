namespace jam
{
/*____________________________________________________________________________*/
std::unique_ptr<VulkanComposition> VulkanComposition::create (VulkanCompositionDevice& compositionDevice, HWND hwnd)
{
    auto composition { std::make_unique<VulkanComposition>() };

    const auto clientExtent { composition->getClientExtent (hwnd) };

    if (not composition->createSharedTexture (compositionDevice, clientExtent.width, clientExtent.height))
        return nullptr;

    DXGI_SWAP_CHAIN_DESC1 swapchainDesc {};
    swapchainDesc.Width = clientExtent.width;
    swapchainDesc.Height = clientExtent.height;
    swapchainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapchainDesc.SampleDesc.Count = 1;
    swapchainDesc.SampleDesc.Quality = 0;
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.BufferCount = swapchainBufferCount;
    swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    if (FAILED (compositionDevice.dxgiFactory->CreateSwapChainForComposition (compositionDevice.direct3DDevice, &swapchainDesc, nullptr, composition->swapchain.resetAndGetPointerAddress())))
        return nullptr;

    if (FAILED (compositionDevice.dcompDevice->CreateTargetForHwnd (hwnd, FALSE, composition->dcompTarget.resetAndGetPointerAddress())))
        return nullptr;

    if (FAILED (compositionDevice.dcompDevice->CreateVisual (composition->dcompVisual.resetAndGetPointerAddress())))
        return nullptr;

    if (FAILED (composition->dcompTarget->SetRoot (composition->dcompVisual)))
        return nullptr;

    if (FAILED (composition->dcompVisual->SetContent (composition->swapchain)))
        return nullptr;

    if (FAILED (compositionDevice.dcompDevice->Commit()))
        return nullptr;

    return composition;
}

bool VulkanComposition::createSharedTexture (VulkanCompositionDevice& compositionDevice, uint32_t width, uint32_t height)
{
    D3D11_TEXTURE2D_DESC textureDesc {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;

    if (FAILED (compositionDevice.direct3DDevice->CreateTexture2D (&textureDesc, nullptr, sharedTexture.resetAndGetPointerAddress())))
        return false;

    juce::ComSmartPtr<IDXGIResource1> dxgiResource;

    if (FAILED (sharedTexture.QueryInterface (dxgiResource)))
        return false;

    VulkanSharedTextureExport newTextureExport;

    if (FAILED (dxgiResource->CreateSharedHandle (nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr, newTextureExport.resetAndGetPointerAddress())))
        return false;

    textureExport = std::move (newTextureExport);

    return true;
}

HANDLE VulkanComposition::releaseTextureExport() noexcept
{
    return textureExport.releaseTextureExport();
}

vk::Extent2D VulkanComposition::getClientExtent (HWND hwnd) const
{
    RECT clientRect {};
    GetClientRect (hwnd, &clientRect);

    return vk::Extent2D { static_cast<uint32_t> (clientRect.right - clientRect.left),
                          static_cast<uint32_t> (clientRect.bottom - clientRect.top) };
}

void VulkanComposition::present (VulkanCompositionDevice& compositionDevice)
{
    juce::ComSmartPtr<ID3D11Texture2D> backBuffer;
    const HRESULT getBufferResult { swapchain->GetBuffer (
        0, __uuidof (ID3D11Texture2D), reinterpret_cast<void**> (backBuffer.resetAndGetPointerAddress())) };

    if (SUCCEEDED (getBufferResult))
    {
        compositionDevice.immediateContext->CopyResource (backBuffer, sharedTexture);

        const HRESULT presentResult { swapchain->Present (presentSyncInterval, 0) };

        if (FAILED (presentResult))
        {
            jam::debug::Log::write ("jam::VulkanComposition::present: Present failed 0x"
                                       + juce::String::toHexString (static_cast<int32_t> (presentResult)));

            if (presentResult == DXGI_ERROR_DEVICE_REMOVED or presentResult == DXGI_ERROR_DEVICE_RESET
                or FAILED (compositionDevice.direct3DDevice->GetDeviceRemovedReason()))
            {
                juce::MessageManager::callAsync ([]
                {
                    if (auto* reinitialiseEngine { VulkanEngine::getInstance() })
                        reinitialiseEngine->reinitialiseDevice();
                });
            }
        }
    }
    else
    {
        jam::debug::Log::write ("jam::VulkanComposition::present: GetBuffer failed 0x"
                                   + juce::String::toHexString (static_cast<int32_t> (getBufferResult)));
    }
}

bool VulkanComposition::resize (VulkanCompositionDevice& compositionDevice, uint32_t width, uint32_t height)
{
    if (not createSharedTexture (compositionDevice, width, height))
        return false;

    if (FAILED (swapchain->ResizeBuffers (swapchainBufferCount, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, 0)))
        return false;

    return true;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
