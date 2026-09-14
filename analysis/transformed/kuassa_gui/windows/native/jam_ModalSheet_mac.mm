/**
 * @file jam_ModalSheet_mac.mm
 * @brief macOS NSSheet lifecycle — RAII wrapper.
 *
 * @see jam_ModalSheet.h
 * @see BackgroundBlur
 */

#if JUCE_MAC

// Objective-C helper: content view that forwards Escape (cancelOperation:) to the dismiss callback.
@interface JAMSheetContentView : NSView
@property (nonatomic, copy) void (^onCancel)(void);
@end

@implementation JAMSheetContentView
- (BOOL)acceptsFirstResponder { return YES; }
- (void)cancelOperation:(id)sender
{
    if (self.onCancel)
        self.onCancel();
}
@end


// Objective-C helper: window delegate that dismisses on key-window loss.
@interface JAMSheetDelegate : NSObject <NSWindowDelegate>
+ (instancetype)sharedInstanceWithDismiss:(std::function<void()>)dismiss
                       shouldDismissOnBlur:(bool)dismissOnBlur;
@end

@implementation JAMSheetDelegate
{
    std::function<void()> dismissCallback;
    bool shouldDismissOnBlur;
}

+ (instancetype)sharedInstanceWithDismiss:(std::function<void()>)dismiss
                       shouldDismissOnBlur:(bool)dismissOnBlur
{
    // Safe as singleton: only one sheet is presented at a time.
    static JAMSheetDelegate* inst;
    if (not inst)
        inst = [JAMSheetDelegate new];
    inst->dismissCallback     = std::move (dismiss);
    inst->shouldDismissOnBlur = dismissOnBlur;
    return inst;
}

- (void)windowDidResignKey:(NSNotification*)notification
{
    if (shouldDismissOnBlur and dismissCallback)
    {
        auto cb { dismissCallback };
        juce::MessageManager::callAsync ([cb] { cb(); });
    }
}
@end


namespace jam
{
/*____________________________________________________________________________*/

ModalSheet::ModalSheet (juce::Component& parent,
                        std::unique_ptr<juce::Component> content,
                        int width, int height,
                        juce::Colour colour, float blurRadius,
                        BackgroundBlur::WindowFX backend,
                        bool shouldDismissOnLostFocus,
                        std::function<void()> onDismiss)
    : parentComponent (&parent)
{
    if (auto* peer { parent.getPeer() })
    {
        NSView* parentView { (NSView*) peer->getNativeHandle() };
        NSWindow* parentWindow { [parentView window] };

        if (parentWindow != nil)
        {
            NSRect frame { NSMakeRect (0, 0, (CGFloat) width, (CGFloat) height) };

            NSWindow* sheet { [[NSWindow alloc]
                initWithContentRect:frame
                          styleMask:NSWindowStyleMaskTitled
                                   | NSWindowStyleMaskClosable
                                   | NSWindowStyleMaskFullSizeContentView
                            backing:NSBackingStoreBuffered
                              defer:YES] };

            [sheet setTitleVisibility:NSWindowTitleHidden];
            [sheet setTitlebarAppearsTransparent:YES];
            [[sheet standardWindowButton:NSWindowCloseButton] setHidden:YES];
            [[sheet standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
            [[sheet standardWindowButton:NSWindowZoomButton] setHidden:YES];
            [sheet setOpaque:NO];

            [sheet setBackgroundColor:[NSColor colorWithRed:colour.getFloatRed()
                                                      green:colour.getFloatGreen()
                                                       blue:colour.getFloatBlue()
                                                      alpha:colour.getFloatAlpha()]];

            // Custom content view: intercepts Escape key via cancelOperation:
            JAMSheetContentView* sheetContentView { [[JAMSheetContentView alloc] initWithFrame:frame] };
            sheetContentView.onCancel = ^{
                juce::MessageManager::callAsync ([onDismiss] { onDismiss(); });
            };
            [sheet setContentView:sheetContentView];
            [sheet makeFirstResponder:sheetContentView];

            // Host JUCE content as subview of JAMSheetContentView.
            // ModalSheet retains ownership via contentComponent; dismiss() removes
            // it from the desktop and deletes it to prevent Desktop::desktopComponents leak.
            contentComponent = content.release();
            contentComponent->addToDesktop (0, sheetContentView);
            contentComponent->setBounds (0, 0, width, height);
            contentComponent->setVisible (true);

            // Window delegate: dismiss on key-window loss when requested.
            JAMSheetDelegate* delegate { [JAMSheetDelegate sharedInstanceWithDismiss:onDismiss
                                                            shouldDismissOnBlur:shouldDismissOnLostFocus] };
            [sheet setDelegate:delegate];

            [parentWindow beginSheet:sheet completionHandler:^(NSModalResponse) {}];

            if (blurRadius > 0.0f)
                BackgroundBlur::enable (contentComponent, backend, blurRadius, colour);

            sheetHandle = (void*) sheet;
        }
    }
}

ModalSheet::~ModalSheet()
{
    if (isActive())
        dismiss();
}

void ModalSheet::dismiss()
{
    if (sheetHandle != nullptr)
    {
        NSWindow* sheet { (NSWindow*) sheetHandle };

        [sheet setDelegate:nil];

        if (auto* peer { parentComponent->getPeer() })
        {
            NSView* parentView { (NSView*) peer->getNativeHandle() };
            NSWindow* parentWindow { [parentView window] };

            if (parentWindow != nil)
                [parentWindow endSheet:sheet];
        }

        // Balance the +1 retain from [[NSWindow alloc] initWithContentRect:...].
        // endSheet: releases the parent's sheet-retain; this release balances alloc.
        // Without this, NSWindow leaks on every open/close cycle (MRC — no ARC here).
        [sheet release];
        sheetHandle = nullptr;
    }

    // Content cleanup is outside the sheetHandle guard — the component must be
    // removed from Desktop::desktopComponents regardless of how the sheet ended.
    // Deletion is deferred: dismiss() may be called from a callback originating
    // inside contentComponent (e.g. renderer button group → trigger → close).
    // Immediate delete would be use-after-free.
    if (contentComponent != nullptr)
    {
        vulkanEngine.removePeer (*contentComponent);

        auto owned { std::unique_ptr<juce::Component> (contentComponent) };
        contentComponent = nullptr;
        juce::MessageManager::callAsync ([c = std::move (owned)] {});
    }
}

/*____________________________END_OF_NAMESPACE________________________________*/
} /** namespace jam */

#endif // JUCE_MAC
