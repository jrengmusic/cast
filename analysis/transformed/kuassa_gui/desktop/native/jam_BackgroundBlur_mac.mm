/**
 * @file jam_BackgroundBlur_mac.mm
 * @brief Objective-C++ implementation of BackgroundBlur — macOS native blur.
 *
 * @par Implementation Notes
 * This file is compiled as Objective-C++ (.mm) because it interacts directly
 * with AppKit (NSWindow, NSVisualEffectView, NSGlassEffectView) and the
 * CoreGraphics private SPI.
 *
 * @par CoreGraphics Private SPI
 * @c CGSSetWindowBackgroundBlurRadius is a private CoreGraphics function not
 * declared in any public header.  It is loaded at runtime via @c dlsym() to
 * avoid link-time dependency on a private framework symbol.  The function
 * signature used here matches the observed ABI on macOS 10.15–14.x:
 * @code
 * int32_t CGSSetWindowBackgroundBlurRadius(CGSConnectionID, int32_t windowNumber, int64_t radius);
 * @endcode
 *
 * @par WindowFX dispatch
 * Each WindowFX value (backgroundBlur, visualFXWindowBackground, glassFXRegular,
 * glassFXClear) maps to a private method that is 1:1 with one native API.
 * No implicit fallback; caller supplies the explicit WindowFX.
 *
 * @par NSVisualEffectView Fallback
 * When the private SPI is unavailable, an @c NSVisualEffectView is inserted
 * behind the window's content view.  The system controls blur intensity;
 * the @p blurRadius parameter is ignored in this path.
 *
 * @par Window Delegate
 * @c GlassWindowDelegate is an Objective-C class defined at file scope
 * (required by the Objective-C runtime).  A single instance is stored in
 * @c g_windowDelegate and a single close callback in @c windowCloseCallback.
 *
 * @note macOS only — the entire file is conditionally compiled under
 *       @c JUCE_MAC.
 *
 * @see jam_background_blur.h
 * @see Window
 */

#if JUCE_MAC
#include <dlfcn.h>
#import <Cocoa/Cocoa.h>

/*____________________________________________________________________________*/

// Declared at global scope because Objective-C @implementation blocks cannot
// capture C++ lambdas directly; replaced by BackgroundBlur::setCloseCallback().
static std::function<void()> windowCloseCallback;

@interface GlassWindowDelegate : NSObject<NSWindowDelegate>
@end

@implementation GlassWindowDelegate

- (BOOL)windowShouldClose:(NSWindow*)sender
{
    if (windowCloseCallback)
        windowCloseCallback();
    return YES;
}

@end

static GlassWindowDelegate* g_windowDelegate = nil;

/*____________________________________________________________________________*/

namespace jam
{

// Matches the private typedef used internally by CoreGraphics on macOS.
typedef intptr_t CGSConnectionID;

using CGSMainConnectionID_Func = CGSConnectionID (*)();
using CGSSetWindowBackgroundBlurRadius_Func = int32_t (*) (CGSConnectionID, int32_t, int64_t);

/**____________________________________________________________________________*/

bool BackgroundBlur::isCoreGraphicsAvailable()
{
    static const bool result { dlsym (RTLD_DEFAULT, "CGSMainConnectionID") != nullptr
                               and dlsym (RTLD_DEFAULT, "CGSSetWindowBackgroundBlurRadius") != nullptr };
    return result;
}

bool BackgroundBlur::isGlassFXAvailable()
{
    static const bool result { [] () -> bool
    {
        if (@available (macOS 26.0, *))
            return true;

        return false;
    }() };

    return result;
}

bool BackgroundBlur::enable (juce::Component* component, BackgroundBlur::WindowFX style, float blur, juce::Colour colour)
{
    bool result { false };

    if (blur > 0.0f)
    {
        switch (style)
        {
            case BackgroundBlur::WindowFX::backgroundBlur:           result = setBackgroundBlur (component, blur); break;
            case BackgroundBlur::WindowFX::visualFXWindowBackground: result = setVisualFX (component);            break;
            case BackgroundBlur::WindowFX::glassFXRegular:           result = setGlassFX (component, true);      break;
            case BackgroundBlur::WindowFX::glassFXClear:             result = setGlassFX (component, false);     break;
        }
    }
    else
    {
        result = true;
    }

    return result;
}

// Window chrome and tint are applied by the caller (StyleWindow::apply) before
// this is invoked; the window is styled in-place with no swap or view relocation.
bool BackgroundBlur::setBackgroundBlur (juce::Component* component, float blurRadius)
{
    bool result { false };

    if (auto* peer { component->getPeer() })
    {
        NSView* view { (NSView*) peer->getNativeHandle() };
        NSWindow* window { [view window] };

        if (window != nil)
        {
            auto CGSMainConnectionID { (CGSMainConnectionID_Func) dlsym (RTLD_DEFAULT, "CGSMainConnectionID") };
            auto CGSSetWindowBackgroundBlurRadius { (CGSSetWindowBackgroundBlurRadius_Func) dlsym (
                RTLD_DEFAULT, "CGSSetWindowBackgroundBlurRadius") };

            auto connection { CGSMainConnectionID() };
            CGSSetWindowBackgroundBlurRadius (connection, [window windowNumber], (int64_t) blurRadius);

            result = true;
        }
    }

    return result;
}

bool BackgroundBlur::setVisualFX (juce::Component* component)
{
    bool result { false };

    if (auto* peer { component->getPeer() })
    {
        NSView* view { (NSView*) peer->getNativeHandle() };
        NSWindow* window { [view window] };

        if (window != nil)
        {
            for (NSView* subview in [[[window contentView] subviews] copy])
                if ([subview isKindOfClass:[NSVisualEffectView class]])
                    [subview removeFromSuperview];

            NSRect frame { [[window contentView] frame] };
            NSVisualEffectView* visualEffect { [[NSVisualEffectView alloc] initWithFrame:frame] };
            [visualEffect setMaterial:NSVisualEffectMaterialWindowBackground];
            [visualEffect setState:NSVisualEffectStateActive];
            [visualEffect setBlendingMode:NSVisualEffectBlendingModeBehindWindow];
            [visualEffect setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

            // Insert visual effect behind existing content
            [[window contentView] addSubview:visualEffect positioned:NSWindowBelow relativeTo:nil];

            result = true;
        }
    }

    return result;
}

bool BackgroundBlur::setGlassFX (juce::Component* component, bool isRegular)
{
    bool result { false };

    if (@available (macOS 26.0, *))
    {
        if (auto* peer { component->getPeer() })
        {
            NSView* view { (NSView*) peer->getNativeHandle() };
            NSWindow* window { [view window] };

            if (window != nil)
            {
                for (NSView* subview in [[[window contentView] subviews] copy])
                    if ([subview isKindOfClass:[NSGlassEffectView class]])
                        [subview removeFromSuperview];

                NSRect frame { [[window contentView] frame] };
                NSGlassEffectView* glass { [[NSGlassEffectView alloc] initWithFrame:frame] };
                [glass setStyle:isRegular ? NSGlassEffectViewStyleRegular : NSGlassEffectViewStyleClear];
                [glass setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

                [[window contentView] addSubview:glass positioned:NSWindowBelow relativeTo:nil];

                result = true;
            }
        }
    }

    return result;
}

// Blur-only removal; chrome/style is managed by StyleWindow. Order: CGS blur
// radius reset, then NSVisualEffectView teardown, then NSGlassEffectView teardown.
void BackgroundBlur::disable (juce::Component* component)
{
    NSWindow* window { nil };

    if (auto* peer { component->getPeer() })
    {
        NSView* view { (NSView*) peer->getNativeHandle() };
        window = [view window];
    }

    if (window != nil)
    {
        if (isCoreGraphicsAvailable())
        {
            auto CGSMainConnectionID { (CGSMainConnectionID_Func) dlsym (RTLD_DEFAULT, "CGSMainConnectionID") };
            auto CGSSetWindowBackgroundBlurRadius { (CGSSetWindowBackgroundBlurRadius_Func) dlsym (
                RTLD_DEFAULT, "CGSSetWindowBackgroundBlurRadius") };

            if (CGSMainConnectionID != nullptr and CGSSetWindowBackgroundBlurRadius != nullptr)
            {
                auto connection { CGSMainConnectionID() };
                CGSSetWindowBackgroundBlurRadius (connection, [window windowNumber], 0);
            }
        }

        for (NSView* subview in [[[window contentView] subviews] copy])
        {
            if ([subview isKindOfClass:[NSVisualEffectView class]])
                [subview removeFromSuperview];

            if (@available (macOS 26.0, *))
            {
                if ([subview isKindOfClass:[NSGlassEffectView class]])
                    [subview removeFromSuperview];
            }
        }
    }
}

void BackgroundBlur::setCloseCallback (std::function<void()> callback)
{
    windowCloseCallback = std::move (callback);
}

/**_____________________________END_OF_NAMESPACE______________________________*/
} // namespace jam

#endif
