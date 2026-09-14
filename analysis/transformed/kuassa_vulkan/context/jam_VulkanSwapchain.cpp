namespace jam
{
/*____________________________________________________________________________*/
//==============================================================================
// Constructor / Destructor
//==============================================================================

VulkanSwapchain::VulkanSwapchain (VulkanDevice& vulkanDevice) : device (vulkanDevice) {}

VulkanSwapchain::~VulkanSwapchain()
{
    for (auto& imageView : swapchainImageViews)
        device.getDevice().destroyImageView (imageView, nullptr);

    device.getDevice().destroySemaphore (imageAvailableSemaphore, nullptr);

    for (auto& semaphore : renderFinishedSemaphores)
        device.getDevice().destroySemaphore (semaphore, nullptr);

    device.getDevice().destroySwapchainKHR (swapchain, nullptr);

    device.getInstance().destroySurfaceKHR (surface, nullptr);
}

//==============================================================================
// Create
//==============================================================================

bool VulkanSwapchain::create (void* nativeHandle, int width, int height)
{
    bool ready { createSurface (nativeHandle) };
    ready = ready and createSwapchain (width, height, swapchain);

    const vk::SemaphoreCreateInfo semInfo {};
    ready = ready
            and device.getDevice().createSemaphore (&semInfo, nullptr, &imageAvailableSemaphore) == vk::Result::eSuccess;

    return ready;
}

void VulkanSwapchain::setFormatAndExtent (vk::SurfaceFormatKHR format, vk::Extent2D extent)
{
    swapchainFormat = format;
    swapchainExtent = extent;
}

//==============================================================================
// Recreate
//==============================================================================

std::tuple<bool, vk::SwapchainKHR, jam::Array<vk::Semaphore>> VulkanSwapchain::recreate (int width, int height)
{
    for (auto& imageView : swapchainImageViews)
        device.getDevice().destroyImageView (imageView, nullptr);
    swapchainImageViews.clear();

    const vk::SwapchainKHR previousSwapchain { swapchain };
    swapchain = nullptr;

    jam::Array<vk::Semaphore> previousRenderFinishedSemaphores { std::move (renderFinishedSemaphores) };

    device.getDevice().destroySemaphore (imageAvailableSemaphore, nullptr);
    imageAvailableSemaphore = nullptr;

    const vk::SemaphoreCreateInfo imageAvailableSemaphoreInfo {};
    bool ready { device.getDevice().createSemaphore (&imageAvailableSemaphoreInfo, nullptr, &imageAvailableSemaphore)
                 == vk::Result::eSuccess };

    ready = ready and createSwapchain (width, height, previousSwapchain);

    return { ready, previousSwapchain, std::move (previousRenderFinishedSemaphores) };
}

//==============================================================================
// Frame lifecycle
//==============================================================================

std::pair<vk::Result, uint32_t> VulkanSwapchain::acquireNextImage()
{
    uint32_t imageIndex { 0 };
    const vk::Result result { device.getDevice().acquireNextImageKHR (
        swapchain, UINT64_MAX, imageAvailableSemaphore, nullptr, &imageIndex) };

    return { result, imageIndex };
}

vk::Result VulkanSwapchain::present (vk::Queue queue, uint32_t imageIndex, vk::Semaphore waitSemaphore)
{
    const vk::PresentInfoKHR presentInfo { waitSemaphore, swapchain, imageIndex };
    return queue.presentKHR (&presentInfo);
}

//==============================================================================
// Accessors
//==============================================================================

vk::Extent2D VulkanSwapchain::getSurfaceExtent() const
{
    const auto [capabilitiesResult, capabilities] { getSurfaceCapabilities() };

    if (capabilitiesResult == vk::Result::eSuccess)
        return capabilities.currentExtent;

    return vk::Extent2D {};
}

//==============================================================================
// Private setup helpers
//==============================================================================

#if JUCE_MAC
bool VulkanSwapchain::createSurface (void* nativeHandle)
{
    metalLayerRef = createMetalLayerForView (nativeHandle);
    const vk::MetalSurfaceCreateInfoEXT metalInfo { {}, static_cast<CAMetalLayer*> (metalLayerRef) };
    return device.getInstance().createMetalSurfaceEXT (&metalInfo, nullptr, &surface)
           == vk::Result::eSuccess;
}
#elif JUCE_WINDOWS
bool VulkanSwapchain::createSurface (void* nativeHandle)
{
    const vk::Win32SurfaceCreateInfoKHR win32Info { {}, GetModuleHandle (nullptr), static_cast<HWND> (nativeHandle) };
    return device.getInstance().createWin32SurfaceKHR (&win32Info, nullptr, &surface)
           == vk::Result::eSuccess;
}
#elif JUCE_LINUX
bool VulkanSwapchain::createSurface (void* nativeHandle)
{
    const vk::XlibSurfaceCreateInfoKHR xlibInfo { {}, XOpenDisplay (nullptr), reinterpret_cast<Window> (nativeHandle) };
    return device.getInstance().createXlibSurfaceKHR (&xlibInfo, nullptr, &surface)
           == vk::Result::eSuccess;
}
#else
bool VulkanSwapchain::createSurface (void* nativeHandle)
{
    juce::ignoreUnused (nativeHandle);
    return false;
}
#endif

std::pair<vk::Result, vk::SurfaceCapabilitiesKHR> VulkanSwapchain::getSurfaceCapabilities() const
{
    const auto capabilitiesResult { device.getPhysicalDevice().getSurfaceCapabilitiesKHR (surface) };

    if (capabilitiesResult.result == vk::Result::eSuccess)
        return { capabilitiesResult.result, capabilitiesResult.value };

#if JUCE_DEBUG
    jam::debug::Log::write ("getSurfaceCapabilitiesKHR failed: " + juce::String (vk::to_string (capabilitiesResult.result)));
#endif
    return { capabilitiesResult.result, vk::SurfaceCapabilitiesKHR {} };
}

vk::PresentModeKHR VulkanSwapchain::selectPresentMode() const
{
    const auto modes { device.getPhysicalDevice().getSurfacePresentModesKHR (surface) };

    if (modes.result == vk::Result::eSuccess)
    {
        const bool hasMailbox { std::find (modes.value.begin(), modes.value.end(), vk::PresentModeKHR::eMailbox) != modes.value.end() };
        return hasMailbox ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
    }

#if JUCE_DEBUG
    jam::debug::Log::write ("getSurfacePresentModesKHR failed: " + juce::String (vk::to_string (modes.result)));
#endif
    return vk::PresentModeKHR::eFifo;
}

bool VulkanSwapchain::createSwapchain (int width, int height, vk::SwapchainKHR oldSwapchain)
{
    const auto [capabilitiesResult, capabilities] { getSurfaceCapabilities() };

    if (capabilitiesResult != vk::Result::eSuccess)
        return false;

    // Use currentExtent if the surface dictates a specific size (Vulkan spec requirement)
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        swapchainExtent = capabilities.currentExtent;
    }
    else
    {
        swapchainExtent = vk::Extent2D { static_cast<uint32_t> (width), static_cast<uint32_t> (height) };
        swapchainExtent.width = std::clamp (swapchainExtent.width,
                                            capabilities.minImageExtent.width,
                                            capabilities.maxImageExtent.width);
        swapchainExtent.height = std::clamp (swapchainExtent.height,
                                             capabilities.minImageExtent.height,
                                             capabilities.maxImageExtent.height);
    }

    uint32_t imageCount { capabilities.minImageCount + 1 };
    if (capabilities.maxImageCount > 0 and imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;

    // Find a suitable surface format (prefer BGRA8 UNORM — software renderer
    // produces sRGB-encoded data; _SRGB format would double-encode gamma)
    const auto formatsResult { device.getPhysicalDevice().getSurfaceFormatsKHR (surface) };

    if (formatsResult.result != vk::Result::eSuccess or formatsResult.value.empty())
    {
#if JUCE_DEBUG
        jam::debug::Log::write ("getSurfaceFormatsKHR failed: " + juce::String (vk::to_string (formatsResult.result)));
#endif
        return false;
    }

    const std::vector<vk::SurfaceFormatKHR>& formats { formatsResult.value };

    swapchainFormat = formats.at (0);// fallback
    for (const auto& fmt : formats)
    {
        if (fmt.format == vk::Format::eB8G8R8A8Unorm
            and fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            swapchainFormat = fmt;
            break;
        }
    }

    // VUID-VkSwapchainCreateInfoKHR-compositeAlpha-01280: compositeAlpha must be a bit
    // present in capabilities.supportedCompositeAlpha. MoltenVK reports eInherit; Win32
    // reports eOpaque only.
    const vk::CompositeAlphaFlagBitsKHR compositeAlpha {
        (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
            ? vk::CompositeAlphaFlagBitsKHR::eInherit
            : vk::CompositeAlphaFlagBitsKHR::eOpaque
    };

    const vk::SwapchainCreateInfoKHR swapInfo { {}, surface, imageCount, swapchainFormat.format,
                                                swapchainFormat.colorSpace, swapchainExtent, 1,
                                                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
                                                vk::SharingMode::eExclusive, {}, {}, capabilities.currentTransform,
                                                compositeAlpha,
                                                selectPresentMode(), vk::True, oldSwapchain };

    if (device.getDevice().createSwapchainKHR (&swapInfo, nullptr, &swapchain) != vk::Result::eSuccess)
        return false;

    // Get swapchain images
    const auto swapchainImagesResult { device.getDevice().getSwapchainImagesKHR (swapchain) };

    if (swapchainImagesResult.result != vk::Result::eSuccess)
    {
#if JUCE_DEBUG
        jam::debug::Log::write ("getSwapchainImagesKHR failed: " + juce::String (vk::to_string (swapchainImagesResult.result)));
#endif
        return false;
    }

    const std::vector<vk::Image>& swapchainImages { swapchainImagesResult.value };

    // Create image views
    swapchainImageViews.resize (static_cast<int> (swapchainImages.size()));
    for (size_t i = 0; i < swapchainImages.size(); ++i)
    {
        const vk::ImageViewCreateInfo viewInfo { {}, swapchainImages.at (i), vk::ImageViewType::e2D, swapchainFormat.format,
                                                 {}, vk::ImageSubresourceRange { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };

        if (device.getDevice().createImageView (&viewInfo, nullptr, &swapchainImageViews.at (static_cast<int> (i)))
            != vk::Result::eSuccess)
            return false;
    }

    const vk::SemaphoreCreateInfo semInfo {};
    renderFinishedSemaphores.resize (static_cast<int> (swapchainImages.size()));
    for (int i = 0; i < renderFinishedSemaphores.size(); ++i)
    {
        if (device.getDevice().createSemaphore (&semInfo, nullptr, &renderFinishedSemaphores.at (i))
            != vk::Result::eSuccess)
            return false;
    }

    return true;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
