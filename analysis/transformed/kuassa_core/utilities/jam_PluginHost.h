/**
 * @file jam_PluginHost.h
 * @brief Host-detection utilities for AU sandbox hosts, Pro Tools, and host DPI scale.
 *
 * Provides `isAUSandboxHost()`, `isProToolsHost()`, and `getHostScale()` as
 * cached, cross-platform-safe probes of the hosting DAW. Consumed by menu
 * and popup native-dispatch code to decide between JUCE-rendered and
 * OS-native menu paths.
 *
 * @par Availability
 * Each probe degrades to a fixed stub when the required JUCE module or
 * platform is not present, so this header may be included unconditionally.
 *
 * @note Every result is cached in a `static const` — computed once per
 *       process lifetime.
 */

#pragma once

/**
 * @brief Returns true if the current host is an AU sandbox host (Logic or GarageBand).
 *
 * Result is cached in a `static const` — computed once per process lifetime.
 *
 * @return @c true  on macOS when hosted by Logic or GarageBand.
 * @return @c false on any other platform or host, or if `juce_audio_processors`
 *         is unavailable.
 */
#if JUCE_MAC && JUCE_MODULE_AVAILABLE_juce_audio_processors
inline bool isAUSandboxHost() noexcept
{
    static const bool cached { [] () noexcept -> bool {
        const juce::PluginHostType host;
        return host.isLogic() or host.isGarageBand();
    }() };

    return cached;
}
#else
inline bool isAUSandboxHost() noexcept { return false; }
#endif

/**
 * @brief Returns true if the current host is Pro Tools.
 *
 * Result is cached in a `static const` — computed once per process lifetime.
 *
 * @return @c true  when hosted by Pro Tools.
 * @return @c false on any other host, or if `juce_audio_processors` is unavailable.
 */
#if JUCE_MODULE_AVAILABLE_juce_audio_processors
inline bool isProToolsHost() noexcept
{
    static const bool cached { [] () noexcept -> bool {
        const juce::PluginHostType host;
        return host.isProTools ();
    }() };

    return cached;
}
#else
inline bool isProToolsHost() noexcept { return false; }
#endif

/**
 * @brief Returns the host's primary display DPI scale, as required by Pro Tools.
 *
 * Non-Pro-Tools hosts return 1.0f unconditionally. Result is cached in a
 * `static const` — computed once per process lifetime.
 *
 * @return Primary display scale factor when hosted by Pro Tools on Windows;
 *         @c 1.0f otherwise, or if `juce_gui_basics` is unavailable.
 */
#if JUCE_WINDOWS && JUCE_MODULE_AVAILABLE_juce_gui_basics
inline float getHostScale() noexcept
{
    static const float cached { [] () noexcept -> float {
        if (not isProToolsHost())
            return 1.0f;

        const auto& displays { juce::Desktop::getInstance().getDisplays() };

        if (auto* primary = displays.getPrimaryDisplay())
            return static_cast<float> (primary->scale);

        return 1.0f;
    }() };

    return cached;
}
#else
inline float getHostScale() noexcept { return 1.0f; }
#endif
