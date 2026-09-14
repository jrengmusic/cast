/**
 * @file jam_BackgroundBlur.h
 * @brief Native window background-blur/acrylic effects for macOS and Windows.
 */
#pragma once

#if JUCE_MAC || JUCE_WINDOWS

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct BackgroundBlur
 * @brief Static namespace-like collection of platform-specific native window
 * transparency effects — CoreGraphics/NSVisualEffectView blur styles on macOS,
 * DWM blur/acrylic on Windows.
 */
struct BackgroundBlur
{
#if JUCE_MAC
    /** @return `true` — macOS native glass is always available. */
    static bool isEnabled() noexcept { return true; }
#elif JUCE_WINDOWS
    /** @return `true` when the GPU renderer is available — glass is a GPU
     *  feature on Windows. */
    static bool isEnabled() noexcept;
#endif

#if JUCE_MAC
    /** @brief macOS window transparency styles. */
    enum class WindowFX
    {
        backgroundBlur = 0,
        visualFXWindowBackground = 1,
        glassFXRegular = 2,
        glassFXClear = 3
    };

    /** @return `true` when the glassFX (Liquid Glass) style is available on this OS version. */
    static bool isGlassFXAvailable();

    /**
     * @brief Enables a native transparency style on a component's window.
     *
     * @p blur gates all styles, not only backgroundBlur — when @p blur is
     * 0.0f the function is a no-op returning `true` immediately, regardless
     * of @p style.
     *
     * @param component  Component whose native window is affected.
     * @param style       Transparency style to apply.
     * @param blur        Blur radius; 0.0f skips the style entirely.
     * @param colour      Tint colour (unused on macOS; forwarded for API symmetry).
     * @return `true` on success, or when @p blur is 0.0f.
     */
    static bool
    enable (juce::Component* component, WindowFX style, float blur, juce::Colour colour);

    /**
     * @brief Disables any native transparency style on a component's window.
     *
     * Resets the CGS blur radius to 0, then removes any NSVisualEffectView
     * or NSGlassEffectView subview previously added by enable().
     *
     * @param component  Component whose native window is affected.
     */
    static void disable (juce::Component* component);

#elif JUCE_WINDOWS
    /** @brief Windows window transparency styles. */
    enum class WindowFX
    {
        blurBehind = 0,
        acrylic = 1
    };

    /**
     * @brief Accent policy states for SetWindowCompositionAttribute (undocumented Win32 API).
     *
     * Used by setAccentPolicy to set DWM window composition effects.
     */
    enum class ACCENT_STATE
    {
        accentDisabled = 0,
        accentEnableGradient = 1,
        accentEnableTransparentGradient = 2,
        accentEnableBlurBehind = 3,
        accentEnableAcrylicBlurBehind = 4,
        accentEnableHostBackdrop = 5,
        accentInvalidState = 6
    };

    /**
     * @brief Applies the given accent policy to an HWND via SetWindowCompositionAttribute.
     * @param hwnd   Target window handle.
     * @param state  Accent state to apply.
     * @param tint   Tint colour used by gradient/acrylic accent states.
     */
    static bool setAccentPolicy (HWND hwnd, ACCENT_STATE state, juce::Colour tint);

    /**
     * @brief Enables a native transparency style on a component's window.
     *
     * @p blur gates all styles, not only blurBehind/acrylic — when @p blur
     * is 0.0f the function is a no-op returning `true` immediately,
     * regardless of @p style.
     *
     * @param component  Component whose native window is affected.
     * @param style       Transparency style to apply.
     * @param blur        Blur radius; 0.0f skips the style entirely.
     * @param colour      Tint colour (used by blurBehind and acrylic).
     * @return `true` on success, or when @p blur is 0.0f.
     */
    static bool
    enable (juce::Component* component, WindowFX style, float blur, juce::Colour colour);

    /**
     * @brief Disables any native transparency style on a component's window.
     *
     * Resets the DWM system backdrop type to auto and restores the default
     * (non-extended) DWM frame on the window's root ancestor.
     *
     * @param component  Component whose native window is affected.
     */
    static void disable (juce::Component* component);

    /**
     * @brief Applies or removes the DWM ForceEffectMode registry key.
     *
     * On Windows 11 VMs (detected via software GL renderer), DWM disables
     * rounded corners by default.  Setting ForceEffectMode=2 re-enables them.
     *
     * @param enable  true → set DWORD to 2; false → delete the value.
     *
     * @note Requires elevated privileges (HKEY_LOCAL_MACHINE).
     *       Changes take effect after the application restarts.
     */
    static void setForceEffectRegistry (bool enable) noexcept;

    static constexpr DWORD dwmWindowCornerPreference { 33 };
    static constexpr DWORD dwmCornerRound { 2 };
    static constexpr DWORD dwmCornerRoundSmall { 3 };
    static constexpr DWORD dwmSystemBackdropType { 38 };
    static constexpr DWORD dwmBackdropAuto { 0 };
#endif

    /**
     * @brief Registers a C++ callback invoked on native window close.
     *
     * The callback is stored in a file-scope slot and forwarded from
     * GlassWindowDelegate (macOS) or the Windows close handler.  Only one
     * callback is active at a time; subsequent calls replace the previous one.
     * Pass an empty std::function to clear.
     *
     * @param callback  Callable invoked on window close.  Ownership transferred.
     */
    static void setCloseCallback (std::function<void()> callback);

    /** @brief Returns true when the active WindowFX style requires Component-level tint
     *  (native tint not available for this style).
     *  When true, jam::Window paints backgroundColourId with alpha.
     *  When false, tint is applied at native level (NSWindow/DWM).
     */
    static bool shouldTintComponent (WindowFX style) noexcept
    {
#if JUCE_MAC
        return style == WindowFX::visualFXWindowBackground;
#elif JUCE_WINDOWS
        return false;
#endif
    }

    /** @brief Maps a string identifier to the platform-specific WindowFX style.
     *  Falls back to the platform default if the string is unrecognised.
     */
    static WindowFX fromString (juce::StringRef name) noexcept
    {
        auto* styles { map::WindowFX::getInstance() };
        const juce::String nameString { name };

        return static_cast<WindowFX> (styles->contains (nameString)
                                           ? styles->get (nameString)
                                           : styles->get (styles->getDefault()));
    }

private:
#if JUCE_MAC
    /** @return `true` when the CoreGraphics private blur SPI is available. */
    static bool isCoreGraphicsAvailable();
    /** @brief Applies the CoreGraphics private blur SPI to @p component's native window. */
    static bool setBackgroundBlur (juce::Component* component, float blurRadius);
    /** @brief Applies an NSVisualEffectView background to @p component's native window. */
    static bool setVisualFX (juce::Component* component);
    /** @brief Applies a Liquid Glass effect view to @p component's native window.
     *  @param component Target component whose native peer receives the effect.
     *  @param isRegular  `true` for glassFXRegular, `false` for glassFXClear.
     */
    static bool setGlassFX (juce::Component* component, bool isRegular);
#elif JUCE_WINDOWS
    static constexpr DWORD WCA_ACCENT_POLICY { 19 };        ///< SetWindowCompositionAttribute accent-policy attribute id.
    static constexpr UINT accentFlagUseGradientColor { 2 }; ///< AccentPolicy flag requesting the tint gradient colour.

    /** @brief Prepares an HWND for DWM composition (extends the frame into the client area). */
    static void prepareDwmCompositing (HWND hwnd);
    /** @brief Applies the classic DWM blur-behind accent policy to @p component's window. */
    static bool setBlurBehind (juce::Component* component, juce::Colour tint);
    /** @brief Applies the DWM acrylic accent policy to @p component's window. */
    static bool setAcrylic (juce::Component* component, juce::Colour tint);
#endif
};

/**_____________________________END_OF_NAMESPACE______________________________*/
}// namespace jam

#endif
