#if JUCE_MAC
#import <Cocoa/Cocoa.h>

namespace jam
{
/*____________________________________________________________________________*/

juce::Colour getSystemColour (const juce::String& identifier)
{
    static const jam::Function::Map<juce::String, NSColor*> systemColours {
        []
        {
            jam::Function::Map<juce::String, NSColor*> colours;

            colours.add<> (Id::systemWindow, [] { return [NSColor windowBackgroundColor]; });
            colours.add<> (Id::systemText, [] { return [NSColor labelColor]; });
            colours.add<> (Id::systemAccent, [] { return [NSColor controlAccentColor]; });
            colours.add<> (Id::systemSeparator, [] { return [NSColor separatorColor]; });
            colours.add<> (Id::systemHighlightedText, [] { return [NSColor selectedTextColor]; });

            return colours;
        }()
    };

    NSColor* nsColour { nil };

    if (systemColours.contains (identifier))
        nsColour = systemColours.get (identifier);

    if (nsColour != nil)
    {
        NSColor* srgb { [nsColour colorUsingColorSpace:NSColorSpace.sRGBColorSpace] };

        if (srgb != nil)
        {
            CGFloat r, g, b, a;
            [srgb getRed:&r green:&g blue:&b alpha:&a];
            return juce::Colour (static_cast<uint8> (r * 255.0),
                                 static_cast<uint8> (g * 255.0),
                                 static_cast<uint8> (b * 255.0),
                                 static_cast<uint8> (a * 255.0));
        }
    }

    return juce::Colours::magenta;
}

/**_____________________________END_OF_NAMESPACE______________________________*/
} // namespace jam

#endif // JUCE_MAC
