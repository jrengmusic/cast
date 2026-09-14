namespace jam
{
/*____________________________________________________________________________*/
#if JUCE_MAC
static constexpr const char* portabilitySubsetExtensionName { "VK_KHR_portability_subset" };
#endif

#if JUCE_DEBUG
static constexpr const char* validationLayerName { "VK_LAYER_KHRONOS_validation" };
#endif

void VulkanDevice::initialise (const juce::String& applicationName)
{
    if (instance == nullptr)
    {
        valid = createInstance (applicationName)
                and selectPhysicalDevice()
                and findQueueFamily()
                and createLogicalDevice()
                and createAllocator();
    }
    else
    {
        valid = createLogicalDevice()
                and createAllocator();
    }
}

void VulkanDevice::shutdown()
{
    if (allocator != VK_NULL_HANDLE)
        vmaDestroyAllocator (allocator);

    if (device != nullptr)
        device.destroy (nullptr);

    allocator = VK_NULL_HANDLE;
    device = nullptr;
    graphicsQueue = nullptr;
    valid = false;
}

VulkanDevice::~VulkanDevice()
{
    if (allocator != VK_NULL_HANDLE)// createAllocator never ran, or ran on a device that never reached validity
        vmaDestroyAllocator (allocator);

   #if JUCE_DEBUG
    if (debugMessenger != nullptr)// createDebugMessenger never ran — validation unsupported, or the instance itself was never created
    {
        const vk::detail::DispatchLoaderDynamic dispatcher { instance, vkGetInstanceProcAddr };
        instance.destroyDebugUtilsMessengerEXT (debugMessenger, nullptr, dispatcher);
    }
   #endif

    if (device != nullptr)// createLogicalDevice never ran, or failed
        device.destroy (nullptr);

    if (instance != nullptr)// createInstance never ran, or failed
        instance.destroy (nullptr);
}

bool VulkanDevice::createInstance (const juce::String& applicationName)
{
   #if JUCE_MAC || JUCE_DEBUG
    const auto instanceExtensionsResult { vk::enumerateInstanceExtensionProperties (nullptr) };
    const bool instanceExtensionsQueried { instanceExtensionsResult.result == vk::Result::eSuccess };
   #endif

    jam::Array<const char*> extensions
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
       #if JUCE_MAC
        VK_EXT_METAL_SURFACE_EXTENSION_NAME,
       #elif JUCE_WINDOWS
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
       #elif JUCE_LINUX
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
       #endif
    };

    vk::InstanceCreateFlags instanceFlags {};

   #if JUCE_MAC
    // Apple/MoltenVK path — VK_KHR_portability_enumeration is required by the
    // Vulkan loader to list portability-only (MoltenVK) physical devices from
    // Vulkan 1.3.216 onward; only requested when the loader actually reports it.
    bool portabilityEnumerationSupported { false };

    if (instanceExtensionsQueried)
    {
        for (const auto& extension : instanceExtensionsResult.value)
        {
            if (juce::CharPointer_UTF8 (extension.extensionName.data())
                    .compare (juce::CharPointer_UTF8 (VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) == 0)
            {
                portabilityEnumerationSupported = true;
                break;
            }
        }
    }

    if (portabilityEnumerationSupported)
    {
        extensions.add (VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        instanceFlags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    }
   #endif

    jam::Array<const char*> layers {};

   #if JUCE_DEBUG
    const auto instanceLayersResult { vk::enumerateInstanceLayerProperties() };

    bool validationLayerSupported { false };

    if (instanceLayersResult.result == vk::Result::eSuccess)
    {
        for (const auto& layer : instanceLayersResult.value)
        {
            if (juce::CharPointer_UTF8 (layer.layerName.data())
                    .compare (juce::CharPointer_UTF8 (validationLayerName)) == 0)
            {
                validationLayerSupported = true;
                break;
            }
        }
    }

    bool debugUtilsSupported { false };

    if (instanceExtensionsQueried)
    {
        for (const auto& extension : instanceExtensionsResult.value)
        {
            if (juce::CharPointer_UTF8 (extension.extensionName.data())
                    .compare (juce::CharPointer_UTF8 (VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) == 0)
            {
                debugUtilsSupported = true;
                break;
            }
        }
    }

    if (validationLayerSupported and debugUtilsSupported)
    {
        layers.add (validationLayerName);
        extensions.add (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
   #endif

    const vk::ApplicationInfo appInfo { applicationName.toRawUTF8(), VK_MAKE_VERSION (0, 0, 1),
                                        "JAM", VK_MAKE_VERSION (0, 0, 1), vulkanApiVersion };

    const vk::InstanceCreateInfo instanceInfo { instanceFlags, &appInfo, layers, extensions };

    const bool created { vk::createInstance (&instanceInfo, nullptr, &instance) == vk::Result::eSuccess };

   #if JUCE_DEBUG
    if (created and validationLayerSupported and debugUtilsSupported)
        createDebugMessenger();
   #endif

    return created;
}

bool VulkanDevice::selectPhysicalDevice()
{
    const auto physicalDevicesResult { instance.enumeratePhysicalDevices() };

    const bool hasDevices { physicalDevicesResult.result == vk::Result::eSuccess
                             and not physicalDevicesResult.value.empty() };

    if (hasDevices)
        physicalDevice = physicalDevicesResult.value.at (0);

   #if JUCE_WINDOWS
    if (hasDevices)
    {
        vk::PhysicalDeviceIDProperties idProperties {};
        vk::PhysicalDeviceProperties2 properties2 { {}, &idProperties };
        physicalDevice.getProperties2 (&properties2);

        jassert (idProperties.deviceLUIDValid == vk::True);

        LUID adapterLuid {};
        std::memcpy (&adapterLuid, idProperties.deviceLUID.data(), VK_LUID_SIZE);

        compositionDevice = VulkanCompositionDevice::create (adapterLuid);
    }
   #endif

    return hasDevices;
}

bool VulkanDevice::findQueueFamily()
{
    const auto queueFamilies { physicalDevice.getQueueFamilyProperties() };

    bool found { false };

    for (uint32_t i = 0; i < static_cast<uint32_t> (queueFamilies.size()); ++i)
    {
        if (queueFamilies.at (i).queueFlags & vk::QueueFlagBits::eGraphics)
        {
            graphicsQueueFamily = i;
            timestampValidBits = queueFamilies.at (i).timestampValidBits;
            computeCapable = static_cast<bool> (queueFamilies.at (i).queueFlags & vk::QueueFlagBits::eCompute);
            found = true;
            break;
        }
    }

    return found;
}

void VulkanDevice::queryDeviceCapabilities()
{
    vk::PhysicalDeviceVulkan12Features queriedFeatures12 {};

    vk::PhysicalDeviceFeatures2 queriedFeatures2 { {}, &queriedFeatures12 };

    physicalDevice.getFeatures2 (&queriedFeatures2);

    vk::PhysicalDeviceVulkan12Properties queriedProperties12 {};

    vk::PhysicalDeviceProperties2 queriedProperties2 { {}, &queriedProperties12 };

    physicalDevice.getProperties2 (&queriedProperties2);

    descriptorIndexing = queriedFeatures12.descriptorIndexing == vk::True;
    runtimeDescriptorArray = queriedFeatures12.runtimeDescriptorArray == vk::True;
    descriptorBindingPartiallyBound = queriedFeatures12.descriptorBindingPartiallyBound == vk::True;
    descriptorBindingSampledImageUpdateAfterBind = queriedFeatures12.descriptorBindingSampledImageUpdateAfterBind == vk::True;
    shaderSampledImageArrayNonUniformIndexing = queriedFeatures12.shaderSampledImageArrayNonUniformIndexing == vk::True;
    timelineSemaphore = queriedFeatures12.timelineSemaphore == vk::True;

    maxDescriptorSetUpdateAfterBindSampledImages = queriedProperties12.maxDescriptorSetUpdateAfterBindSampledImages;
    timestampPeriod = queriedProperties2.properties.limits.timestampPeriod;
    timestampComputeAndGraphics = queriedProperties2.properties.limits.timestampComputeAndGraphics == vk::True;
}

bool VulkanDevice::createLogicalDevice()
{
    queryDeviceCapabilities();

    // Hard requirements for this engine's bindless architecture — not optional
    // features gating a runtime branch. Loud, fail-fast: invalid state is never
    // silently swallowed (CODING.md loud-failure policy).
    jassert (descriptorIndexing);
    jassert (runtimeDescriptorArray);
    jassert (descriptorBindingPartiallyBound);
    jassert (descriptorBindingSampledImageUpdateAfterBind);
    jassert (shaderSampledImageArrayNonUniformIndexing);
    jassert (timelineSemaphore);

    vk::PhysicalDeviceVulkan12Features enabledFeatures12 {};
    enabledFeatures12.descriptorIndexing = vk::True;
    enabledFeatures12.runtimeDescriptorArray = vk::True;
    enabledFeatures12.descriptorBindingPartiallyBound = vk::True;
    enabledFeatures12.descriptorBindingSampledImageUpdateAfterBind = vk::True;
    enabledFeatures12.shaderSampledImageArrayNonUniformIndexing = vk::True;
    enabledFeatures12.timelineSemaphore = vk::True;

    // No core vk::PhysicalDeviceFeatures bit is required by this engine — an
    // all-default (every field false) struct, still passed as pEnabledFeatures
    // below (a null pEnabledFeatures is legal too, but every other
    // enabled-features struct in this function is passed explicitly, so this
    // one stays explicit for the same reason).
    vk::PhysicalDeviceFeatures enabledFeatures {};

    const float queuePriority { 1.0f };

    const vk::DeviceQueueCreateInfo queueInfo { {}, graphicsQueueFamily, 1, &queuePriority };

    jam::Array<const char*> deviceExtensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

   #if JUCE_WINDOWS
    // External memory interop is required-if-present for the VulkanComposition
    // (DComp glass) leg — presence-checked so the WSI leg keeps working
    // when a driver lacks it.
    const auto interopExtensionsResult { physicalDevice.enumerateDeviceExtensionProperties (nullptr) };

    bool externalMemoryWin32Supported { false };

    if (interopExtensionsResult.result == vk::Result::eSuccess)
    {
        for (const auto& extension : interopExtensionsResult.value)
        {
            if (juce::CharPointer_UTF8 (extension.extensionName.data())
                    .compare (juce::CharPointer_UTF8 (VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME)) == 0)
            {
                externalMemoryWin32Supported = true;
                break;
            }
        }
    }

    compositionInteropSupported = externalMemoryWin32Supported and compositionDevice != nullptr;

    if (compositionInteropSupported)
        deviceExtensions.add (VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
   #endif

   #if JUCE_MAC
    // VK_KHR_portability_subset is required-if-present by spec (a portability
    // implementation that supports it must have it enabled) — presence-checked
    // rather than pushed unconditionally.
    const auto deviceExtensionsResult { physicalDevice.enumerateDeviceExtensionProperties (nullptr) };

    bool portabilitySubsetSupported { false };

    if (deviceExtensionsResult.result == vk::Result::eSuccess)
    {
        for (const auto& extension : deviceExtensionsResult.value)
        {
            if (juce::CharPointer_UTF8 (extension.extensionName.data())
                    .compare (juce::CharPointer_UTF8 (portabilitySubsetExtensionName)) == 0)
            {
                portabilitySubsetSupported = true;
                break;
            }
        }
    }

    if (portabilitySubsetSupported)
        deviceExtensions.add (portabilitySubsetExtensionName);
   #endif

    const vk::DeviceCreateInfo deviceInfo { {}, queueInfo, {}, deviceExtensions, &enabledFeatures, &enabledFeatures12 };

    const bool created { physicalDevice.createDevice (&deviceInfo, nullptr, &device) == vk::Result::eSuccess };

    if (created)
        device.getQueue (graphicsQueueFamily, 0, &graphicsQueue);

    return created;
}

bool VulkanDevice::createAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo {};
    allocatorInfo.vulkanApiVersion = vulkanApiVersion;
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;

    return vmaCreateAllocator (&allocatorInfo, &allocator) == VK_SUCCESS;
}

#if JUCE_DEBUG
VKAPI_ATTR vk::Bool32 VKAPI_CALL VulkanDevice::debugCallback (vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        vk::DebugUtilsMessageTypeFlagsEXT messageType,
                                                        const vk::DebugUtilsMessengerCallbackDataEXT* callbackData,
                                                        void* userData)
{
    juce::ignoreUnused (userData);

    const juce::String severityText { vk::to_string (messageSeverity) };
    const juce::String typeText { vk::to_string (messageType) };

    jam::debug::Log::write ("[vulkan " + severityText + " " + typeText + "] " + juce::String (callbackData->pMessage));

    if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        jassertfalse;

    return vk::False;
}

void VulkanDevice::createDebugMessenger()
{
    const vk::DebugUtilsMessengerCreateInfoEXT messengerInfo
    {
        {},
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        &VulkanDevice::debugCallback
    };

    // VK_EXT_debug_utils entry points are not exported by vulkan-1.lib on Windows —
    // resolved at runtime through vkGetInstanceProcAddr via the dynamic dispatcher.
    const vk::detail::DispatchLoaderDynamic dispatcher { instance, vkGetInstanceProcAddr };
    const vk::Result messengerResult { instance.createDebugUtilsMessengerEXT (&messengerInfo, nullptr, &debugMessenger, dispatcher) };

    if (messengerResult != vk::Result::eSuccess)
    {
        jam::debug::Log::write ("createDebugUtilsMessengerEXT failed: " + juce::String (vk::to_string (messengerResult)));
        debugMessenger = vk::DebugUtilsMessengerEXT {};
    }
}
#endif

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam