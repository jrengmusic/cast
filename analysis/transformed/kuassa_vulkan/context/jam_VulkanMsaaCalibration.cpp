//
// Calibration file: createTimestampPool(), calibrateSampleCount() — the
// timestamp query pool and the per-candidate MSAA sample-count selection loop.
// calibrateSampleCount() runs its measurement loop once per device: the first
// context to calibrate publishes the result onto VulkanDevice, and every later
// context on that device adopts it without re-measuring.
// measureCandidateSampleCount() (the actual GPU timing probe) lives in its own
// sibling TU (jam_VulkanMsaaCalibrationMeasurement.cpp) — see jam_vulkan.cpp
// for the full list of sibling translation units making up this one class's
// setup helpers.

namespace jam
{
/*____________________________________________________________________________*/
//==============================================================================
// Constructor / Destructor
//==============================================================================

VulkanMsaaCalibration::VulkanMsaaCalibration (VulkanDevice& vulkanDevice) : device (vulkanDevice) {}

VulkanMsaaCalibration::~VulkanMsaaCalibration()
{
    device.getDevice().destroyQueryPool (timestampPool, nullptr);
}

//==============================================================================
// MSAA calibration
//==============================================================================

bool VulkanMsaaCalibration::createTimestampPool()
{
    const vk::QueryPoolCreateInfo poolInfo { {}, vk::QueryType::eTimestamp, timestampPoolQueryCount };

    return device.getDevice().createQueryPool (&poolInfo, nullptr, &timestampPool) == vk::Result::eSuccess;
}

void VulkanMsaaCalibration::calibrateSampleCount (double targetFrameBudgetMs,
                                                  vk::SurfaceFormatKHR swapchainFormat,
                                                  vk::Extent2D swapchainExtent,
                                                  vk::CommandPool commandPool)
{
    if (device.getActiveSampleCount() != vk::SampleCountFlagBits {})
        return;

    if (not device.isTimestampComputeAndGraphics())
    {
        // Documented deterministic fallback (not silent) — timestamp queries are
        // unsupported on this device/queue family, so no measurement is possible.
        // Apple GPUs (TBDR) resolve memoryless transient MSAA attachments at
        // near-zero bandwidth cost; non-Apple targets default one step lower.
#if JUCE_MAC
        static constexpr vk::SampleCountFlagBits noTimestampFallbackSampleCount { vk::SampleCountFlagBits::e4 };
#else
        static constexpr vk::SampleCountFlagBits noTimestampFallbackSampleCount { vk::SampleCountFlagBits::e2 };
#endif
        device.setActiveSampleCount (noTimestampFallbackSampleCount);
        return;
    }

    const vk::PhysicalDeviceProperties physicalProperties { device.getPhysicalDevice().getProperties() };

    static constexpr std::array<vk::SampleCountFlagBits, 3> candidates
    {
        vk::SampleCountFlagBits::e4, vk::SampleCountFlagBits::e2, vk::SampleCountFlagBits::e1
    };

    for (auto candidate : candidates)
    {
        // Intel/AMD Windows MSAA sample-count ceilings vary;
        // this hardware query (not an assumption) skips any candidate the physical
        // device cannot back with a real color-attachment sample count, regardless
        // of the actual ceiling.
        if ((physicalProperties.limits.framebufferColorSampleCounts & candidate) != vk::SampleCountFlags {})
        {
            const auto measuredMs { measureCandidateSampleCount (candidate, swapchainFormat, swapchainExtent, commandPool) };

            if (measuredMs.has_value() and *measuredMs <= targetFrameBudgetMs)
            {
                device.setActiveSampleCount (candidate);
                return;
            }
        }
    }

    // vk::SampleCountFlagBits::e1 is guaranteed supported by every Vulkan implementation
    // (spec-mandated) — deterministic final fallback if the loop above never locked
    // a candidate in (e.g. every measurement itself failed to allocate resources).
    device.setActiveSampleCount (vk::SampleCountFlagBits::e1);
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
