#if JUCE_MAC

namespace jam::StyleWindow
{
/*____________________________________________________________________________*/

bool apply (juce::Component* component, juce::Colour colour)
{
    bool result { false };

    if (auto* peer { component->getPeer() })
    {
        NSView* view { (NSView*) peer->getNativeHandle() };
        NSWindow* window { [view window] };

        if (window != nil)
        {
            [window setTitleVisibility:NSWindowTitleHidden];
            [window setTitlebarAppearsTransparent:YES];
            [window setStyleMask:window.styleMask | NSWindowStyleMaskFullSizeContentView];

            [window setOpaque:NO];
            [window setBackgroundColor:[NSColor colorWithRed:colour.getFloatRed()
                                                       green:colour.getFloatGreen()
                                                        blue:colour.getFloatBlue()
                                                       alpha:colour.getFloatAlpha()]];
            result = true;
        }
    }

    return result;
}

void setButtons (juce::ComponentPeer& peer, bool visible)
{
    NSView* view { (NSView*) peer.getNativeHandle() };
    NSWindow* window { [view window] };

    if (window != nil)
    {
        [[window standardWindowButton:NSWindowCloseButton] setHidden:!visible];
        [[window standardWindowButton:NSWindowMiniaturizeButton] setHidden:!visible];
        [[window standardWindowButton:NSWindowZoomButton] setHidden:!visible];
    }
}

int getTitleBarHeight (const juce::ComponentPeer& peer)
{
    NSView* view { (NSView*) peer.getNativeHandle() };
    NSWindow* window { [view window] };

    int result { 0 };

    if (window != nil)
        result = juce::roundToInt (window.frame.size.height - window.contentLayoutRect.size.height);

    return result;
}

bool setMenu (juce::Component* component, juce::Colour colour)
{
    bool result { false };

    if (auto* peer { component->getPeer() })
    {
        NSView* view { (NSView*) peer->getNativeHandle() };
        NSWindow* window { [view window] };

        if (window != nil)
        {
            // Shape: rounded layer
            const CGFloat effectiveRadius { colour.getFloatAlpha() < 1.0f ? cornerRadius * Math::phi<float>
                                                                          : cornerRadius };
            [[view layer] setCornerRadius:effectiveRadius];

            // Tint via layer.backgroundColor
            CGColorRef tint { CGColorCreateGenericRGB (
                colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha()) };
            [[view layer] setBackgroundColor:tint];
            CGColorRelease (tint);

            result = true;
        }
    }

    return result;
}

/*____________________________END_OF_NAMESPACE________________________________*/
}

#endif// JUCE_MAC
