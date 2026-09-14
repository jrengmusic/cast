namespace jam
{
/*____________________________________________________________________________*/
/** @brief Shared Vulkan device — owns instance, physical device, logical device,
 *         graphics queue, and VMA allocator.
 *
 *  One VulkanDevice is created per Vulkan context family. Surfaces and swapchains are
 *  managed separately by VulkanGraphics.
 *
 *  Construction is cheap and does no Vulkan work; initialise() runs the
 *  single-shot chain. If any step fails, isValid() returns false and
 *  the destructor safely cleans up whatever was created.
 */
class VulkanDevice : public jam::Instance<VulkanDevice>
{
public:
    /** @brief Constructs an empty, not-yet-valid device — no Vulkan work runs
     *  until initialise() is called. */
    VulkanDevice() noexcept = default;

    /** @brief Destroys every Vulkan handle actually created by initialise(),
     *  in reverse creation order — safe to call whether initialise() ran
     *  to completion, partway, or never at all. */
    ~VulkanDevice();

    /** @brief Create-when-absent: runs the full instance-through-allocator
     *  chain the first time it is called. Once instance already exists (a
     *  prior initialise() succeeded and shutdown() has not run since), only
     *  the logical-device-scoped stages — createLogicalDevice(),
     *  createAllocator() — are re-run against the surviving instance and
     *  physicalDevice; the instance-scoped stages are skipped. isValid()
     *  reflects the run chain's outcome once this returns.
     *  @param applicationName  Name reported to the Vulkan driver via vk::ApplicationInfo.
     */
    void initialise (const juce::String& applicationName);

    /** @brief Destroys the logical-device-scoped handles only — VMA allocator,
     *  then the logical device — and nulls both. instance, physicalDevice,
     *  the queue family index, queried capability flags, and debugMessenger
     *  are untouched and remain reusable. Safe to call on a lost device.
     *  Call initialise() afterward to re-create the logical device and
     *  allocator against the surviving instance and physicalDevice. */
    void shutdown();

    //==================================================
    // Constants
    //==================================================

    /** @brief Vulkan API version targeted by this device. */
    static constexpr uint32_t vulkanApiVersion { VK_API_VERSION_1_2 };

    //==================================================
    // Accessors
    //==================================================

    /** @brief Returns the vk::Instance handle. */
    vk::Instance getInstance() const noexcept { return instance; }

    /** @brief Returns the selected vk::PhysicalDevice handle. */
    vk::PhysicalDevice getPhysicalDevice() const noexcept { return physicalDevice; }

    /** @brief Returns the vk::Device handle. */
    vk::Device getDevice() const noexcept { return device; }

    /** @brief Returns the graphics vk::Queue handle. */
    vk::Queue getQueue() const noexcept { return graphicsQueue; }

    /** @brief Returns the graphics queue family index. */
    uint32_t getQueueFamily() const noexcept { return graphicsQueueFamily; }

    /** @brief Returns the VmaAllocator handle. */
    VmaAllocator getAllocator() const noexcept { return allocator; }

   #if JUCE_WINDOWS
    VulkanCompositionDevice& getCompositionDevice() const noexcept { jassert (compositionDevice != nullptr); return *compositionDevice; }
    bool isCompositionInteropSupported() const noexcept { return compositionInteropSupported; }
   #endif

    /** @brief Returns true if all creation steps succeeded. */
    bool isValid() const noexcept { return valid.load(); }

    /** @brief Returns vk::PhysicalDeviceVulkan12Properties::maxDescriptorSetUpdateAfterBindSampledImages — the bindless sampled-image array ceiling. */
    uint32_t getMaxDescriptorSetUpdateAfterBindSampledImages() const noexcept { return maxDescriptorSetUpdateAfterBindSampledImages; }

    /** @brief Returns vk::PhysicalDeviceLimits::timestampPeriod — nanoseconds represented by one timestamp query tick. */
    float getTimestampPeriod() const noexcept { return timestampPeriod; }

    /** @brief Returns vk::PhysicalDeviceLimits::timestampComputeAndGraphics — true if all graphics/compute queues support timestamps. */
    bool isTimestampComputeAndGraphics() const noexcept { return timestampComputeAndGraphics; }

    /** @brief Returns true if the selected graphics queue family also carries vk::QueueFlagBits::eCompute. */
    bool isComputeCapable() const noexcept { return computeCapable; }

    /** @brief Returns vk::QueueFamilyProperties::timestampValidBits for the selected graphics queue family. */
    uint32_t getTimestampValidBits() const noexcept { return timestampValidBits; }

    /** @brief Returns the device-wide MSAA sample count calibrated once by the
     *  first VulkanMsaaCalibration::calibrateSampleCount() call, or
     *  vk::SampleCountFlagBits {} if calibration has not run yet. */
    vk::SampleCountFlagBits getActiveSampleCount() const noexcept { return activeSampleCount; }

    /** @brief Publishes the device-wide calibrated MSAA sample count. Called
     *  exactly once, by the first VulkanMsaaCalibration::calibrateSampleCount()
     *  to run on this device. */
    void setActiveSampleCount (vk::SampleCountFlagBits newSampleCount) noexcept { activeSampleCount = newSampleCount; }

private:
    bool createInstance (const juce::String& applicationName);
    bool selectPhysicalDevice();
    bool findQueueFamily();
    void queryDeviceCapabilities();
    bool createLogicalDevice();
    bool createAllocator();

   #if JUCE_DEBUG
    /** @brief Creates the VK_EXT_debug_utils messenger routing validation
     *  messages to jam::debug::Log. Only called once createInstance() has
     *  confirmed both the validation layer and VK_EXT_debug_utils are present. */
    void createDebugMessenger();

    /** @brief Routes a validation message to jam::debug::Log; asserts on
     *  eError severity (KUASSA-CODING-STANDARD.md loud-failure policy). */
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback (vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                           vk::DebugUtilsMessageTypeFlagsEXT messageType,
                                                           const vk::DebugUtilsMessengerCallbackDataEXT* callbackData,
                                                           void* userData);
   #endif

    vk::Instance instance {};
    vk::PhysicalDevice physicalDevice {};
    vk::Device device {};
    vk::Queue graphicsQueue {};
    uint32_t graphicsQueueFamily { 0 };
    VmaAllocator allocator { VK_NULL_HANDLE };

   #if JUCE_WINDOWS
    std::unique_ptr<VulkanCompositionDevice> compositionDevice {};
    bool compositionInteropSupported { false };
   #endif

    std::atomic<bool> valid { false };

   #if JUCE_DEBUG
    /** @brief VK_EXT_debug_utils messenger — created after the instance,
     *  destroyed before it. Empty (null) when validation is unavailable. */
    vk::DebugUtilsMessengerEXT debugMessenger {};
   #endif

    bool descriptorIndexing { false };
    bool runtimeDescriptorArray { false };
    bool descriptorBindingPartiallyBound { false };
    bool descriptorBindingSampledImageUpdateAfterBind { false };
    bool shaderSampledImageArrayNonUniformIndexing { false };
    bool timelineSemaphore { false };
    uint32_t maxDescriptorSetUpdateAfterBindSampledImages { 0 };
    float timestampPeriod { 0.0f };
    bool timestampComputeAndGraphics { false };
    uint32_t timestampValidBits { 0 };
    bool computeCapable { false };

    vk::SampleCountFlagBits activeSampleCount {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanDevice)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam