#if JUCE_WINDOWS

#include <dwmapi.h>
#include <jam_core/utilities/jam_Platform.h>

/*____________________________________________________________________________*/

// Undocumented SetWindowCompositionAttribute API (user32.dll, Windows 10+)

struct ACCENT_POLICY
{
    DWORD    AccentState;
    UINT     AccentFlags;
    COLORREF GradientColor;  // ABGR format
    LONG     AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA
{
    DWORD  Attrib;
    LPVOID pvData;
    UINT   cbData;
};

using SetWindowCompositionAttribute_t = BOOL (WINAPI*) (HWND, WINDOWCOMPOSITIONATTRIBDATA*);

/*____________________________________________________________________________*/

static std::function<void()> windowCloseCallback;

/*____________________________________________________________________________*/

namespace jam
{

static constexpr const wchar_t* const user32LibraryName { L"user32.dll" };
static constexpr const char* const setWindowCompositionAttributeSymbol { "SetWindowCompositionAttribute" };

bool BackgroundBlur::isEnabled() noexcept
{
    jassert (jam::VulkanEngine::getInstance() != nullptr);
    return jam::VulkanEngine::getInstance()->isGpuAvailable();
}

/**____________________________________________________________________________*/

bool BackgroundBlur::setAccentPolicy (HWND hwnd, ACCENT_STATE state, juce::Colour tint)
{
    bool result { false };

    auto SetWindowCompositionAttribute { reinterpret_cast<SetWindowCompositionAttribute_t> (
        GetProcAddress (GetModuleHandleW (user32LibraryName), setWindowCompositionAttributeSymbol)) };

    if (SetWindowCompositionAttribute != nullptr)
    {
        COLORREF abgr { (static_cast<COLORREF> (tint.getAlpha())  << 24)
                      | (static_cast<COLORREF> (tint.getBlue())   << 16)
                      | (static_cast<COLORREF> (tint.getGreen())  <<  8)
                      |  static_cast<COLORREF> (tint.getRed()) };

        ACCENT_POLICY accent {};
        accent.AccentState   = static_cast<DWORD> (state);
        accent.AccentFlags   = accentFlagUseGradientColor;
        accent.GradientColor = abgr;
        accent.AnimationId   = 0;

        WINDOWCOMPOSITIONATTRIBDATA data {};
        data.Attrib  = WCA_ACCENT_POLICY;
        data.pvData  = &accent;
        data.cbData  = sizeof (accent);

        result = SetWindowCompositionAttribute (hwnd, &data) != FALSE;

        if (result)
            DwmFlush();
    }

    return result;
}

// Idempotent — safe to call multiple times. Called before applying any DWM
// backdrop type (acrylic).
void BackgroundBlur::prepareDwmCompositing (HWND hwnd)
{
    LONG_PTR exStyle { GetWindowLongPtrW (hwnd, GWL_EXSTYLE) };

    if ((exStyle & WS_EX_LAYERED) != 0)
        SetWindowLongPtrW (hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_LAYERED);

    MARGINS margins { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea (hwnd, &margins);
}

bool BackgroundBlur::enable (juce::Component* component, BackgroundBlur::WindowFX style, float blur, juce::Colour colour)
{
    bool result { false };
    bool isWindows11 { false };

    if (blur > 0.0f)
    {
        static const bool windows11 { getWindowsBuildNumber() >= 22000 };
        isWindows11 = windows11;

        result = isWindows11 ? setAcrylic (component, colour) : setBlurBehind (component, colour);
    }
    else
    {
        result = true;
    }

    return result;
}

bool BackgroundBlur::setBlurBehind (juce::Component* component, juce::Colour tint)
{
    bool result { false };

    if (auto* peer { component->getPeer() })
    {
        auto hwnd { static_cast<HWND> (peer->getNativeHandle()) };

        if (hwnd != nullptr)
        {
            prepareDwmCompositing (hwnd);
            result = setAccentPolicy (hwnd, ACCENT_STATE::accentEnableBlurBehind, tint);
        }
    }

    return result;
}

bool BackgroundBlur::setAcrylic (juce::Component* component, juce::Colour tint)
{
    bool result { false };

    if (auto* peer { component->getPeer() })
    {
        auto hwnd { static_cast<HWND> (peer->getNativeHandle()) };

        if (hwnd != nullptr)
        {
            prepareDwmCompositing (hwnd);
            result = setAccentPolicy (hwnd, ACCENT_STATE::accentEnableAcrylicBlurBehind, tint);
        }
    }

    return result;
}

// Called from the message thread when switching from the GPU renderer to the CPU renderer.
void BackgroundBlur::disable (juce::Component* component)
{
    if (auto* peer { component->getPeer() })
    {
        auto hwnd { static_cast<HWND> (peer->getNativeHandle()) };

        if (hwnd != nullptr)
        {
            HWND root { GetAncestor (hwnd, GA_ROOT) };

            if (root != nullptr)
            {
                DWORD backdropType { dwmBackdropAuto };
                DwmSetWindowAttribute (root, dwmSystemBackdropType, &backdropType, sizeof (backdropType));

                MARGINS margins { 0, 0, 0, 0 };
                DwmExtendFrameIntoClientArea (root, &margins);
            }
        }
    }
}

void BackgroundBlur::setCloseCallback (std::function<void()> callback)
{
    windowCloseCallback = std::move (callback);
}

// On Windows 11 VMs (detected via software GL renderer), DWM disables rounded
// corners by default; ForceEffectMode=2 re-enables them. Requires elevated
// privileges and takes effect after the application restarts.
void BackgroundBlur::setForceEffectRegistry (bool enable) noexcept
{
    HKEY hKey { nullptr };
    const auto opened { RegOpenKeyExW (
        HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\Dwm", 0, KEY_SET_VALUE, &hKey) };

    if (opened == ERROR_SUCCESS)
    {
        if (enable)
        {
            DWORD value { dwmCornerRound };
            RegSetValueExW (
                hKey, L"ForceEffectMode", 0, REG_DWORD, reinterpret_cast<const BYTE*> (&value), sizeof (value));
        }
        else
        {
            RegDeleteValueW (hKey, L"ForceEffectMode");
        }

        RegCloseKey (hKey);
    }
}

/**_____________________________END_OF_NAMESPACE______________________________*/
} // namespace jam

#endif
