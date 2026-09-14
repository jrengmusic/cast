#if JUCE_WINDOWS
#include <windows.h>

namespace jam
{
/*____________________________________________________________________________*/

static juce::Colour colourFromSysColor (int index)
{
    auto rgb { GetSysColor (index) };
    return juce::Colour (static_cast<juce::uint8> (rgb & 0xFF),
                         static_cast<juce::uint8> ((rgb >> 8) & 0xFF),
                         static_cast<juce::uint8> ((rgb >> 16) & 0xFF));
}

juce::Colour getSystemColour (const juce::String& identifier)
{
    static const jam::HashMap<juce::String, int> sysColorTable {
        { Id::systemWindow.toString(),          COLOR_WINDOW      },
        { Id::systemText.toString(),            COLOR_WINDOWTEXT  },
        { Id::systemSeparator.toString(),       COLOR_3DSHADOW    },
        { Id::systemHighlightedText.toString(), COLOR_HIGHLIGHTTEXT },
    };

    juce::Colour result { juce::Colours::magenta };

    auto it { sysColorTable.find (identifier) };

    if (it != sysColorTable.end())
    {
        result = colourFromSysColor (it->second);
    }
    else if (identifier.compare (Id::systemAccent.toString()) == 0)
    {
        // TODO: WinRT UISettings::GetColorValue(UIColorType::Accent) for true accent
        // GetSysColor has no accent colour. Fallback for now.
        result = juce::Colours::cornflowerblue;
    }

    return result;
}

/**_____________________________END_OF_NAMESPACE______________________________*/
} // namespace jam

#elif ! JUCE_MAC // Linux / other — stub

namespace jam
{
/*____________________________________________________________________________*/

juce::Colour getSystemColour (const juce::String&)
{
    return juce::Colours::magenta;
}

/**_____________________________END_OF_NAMESPACE______________________________*/
} // namespace jam

#endif
