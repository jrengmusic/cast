#if JUCE_MAC

namespace jam
{
/*____________________________________________________________________________*/

void NativeFileChooser::openDirectory (juce::Component* parentWindow,
                                       const juce::String& startingPath,
                                       std::function<void (const juce::String&)> onSelected)
{
    if (parentWindow != nullptr)
    {
        if (auto* peer = parentWindow->getPeer())
        {
            NSView* view = (NSView*) peer->getNativeHandle();
            NSWindow* window = [view window];

            if (window != nil)
            {
                NSOpenPanel* panel = [NSOpenPanel openPanel];
                [panel setCanChooseFiles:NO];
                [panel setCanChooseDirectories:YES];
                [panel setAllowsMultipleSelection:NO];
                [panel setCanCreateDirectories:YES];

                if (startingPath.isNotEmpty())
                {
                    NSString* nsPath = [NSString stringWithUTF8String:startingPath.toRawUTF8()];
                    [panel setDirectoryURL:[NSURL fileURLWithPath:nsPath isDirectory:YES]];
                }

                [panel beginSheetModalForWindow:window
                              completionHandler:^(NSModalResponse result)
                {
                    if (result == NSModalResponseOK)
                    {
                        NSURL* url = panel.URLs.firstObject;

                        if (url != nil)
                        {
                            juce::String path = juce::String::fromUTF8 ([[url path] UTF8String]);

                            juce::MessageManager::callAsync ([onSelected, path]()
                            {
                                if (onSelected != nullptr)
                                    onSelected (path);
                            });
                        }
                    }
                }];
            }
        }
    }
}

/**_____________________________END_OF_NAMESPACE______________________________*/
}// namespace jam

#endif
