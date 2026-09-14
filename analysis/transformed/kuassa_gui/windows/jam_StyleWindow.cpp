#if JUCE_WINDOWS
#include <dwmapi.h>
#include <jam_core/utilities/jam_Platform.h>

namespace jam::StyleWindow
{
/*____________________________________________________________________________*/

bool apply (juce::Component* component, juce::Colour colour)
{
    bool result { false };

    if (auto* peer { component->getPeer() })
    {
        auto hwnd { static_cast<HWND> (peer->getNativeHandle()) };

        if (hwnd != nullptr)
        {
            DWORD cornerPref { BackgroundBlur::dwmCornerRound };
            DwmSetWindowAttribute (hwnd, BackgroundBlur::dwmWindowCornerPreference, &cornerPref, sizeof (cornerPref));
            result = true;
        }
    }

    return result;
}

bool setMenu (juce::Component* component, juce::Colour colour)
{
    bool result { false };

    if (auto* peer { component->getPeer() })
    {
        auto hwnd { static_cast<HWND> (peer->getNativeHandle()) };

        if (hwnd != nullptr)
        {
            DWORD cornerPref { BackgroundBlur::dwmCornerRound };
            DwmSetWindowAttribute (hwnd, BackgroundBlur::dwmWindowCornerPreference, &cornerPref, sizeof (cornerPref));

            result = BackgroundBlur::setAccentPolicy (hwnd, BackgroundBlur::ACCENT_STATE::accentEnableGradient, colour);
        }
    }

    return result;
}

/*____________________________END_OF_NAMESPACE________________________________*/
}

#endif// JUCE_WINDOWS
