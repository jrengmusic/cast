#if JUCE_MAC
// Objective‑C helper class

@interface MenuTarget : NSObject<NSMenuDelegate>
+ (instancetype)sharedInstanceWithCallback:(std::function<void (int)>)cb;
+ (instancetype)sharedInstanceWithStringCallback:(std::function<void (juce::String)>)cb;
- (void)menuItemSelected:(id)sender;
- (void)setDismissCallback:(std::function<void()>)cb;
- (void)menuDidClose:(NSMenu*)menu;
@end

@implementation MenuTarget
{
    std::function<void (int)> intCallback;
    std::function<void (juce::String)> stringCallback;
    std::function<void()> dismissCallback;
}

+ (instancetype)sharedInstanceWithCallback:(std::function<void (int)>)cb
{
    // Safe as singleton: only one menu is shown at a time.
    static MenuTarget* inst;
    if (inst == nil)
        inst = [MenuTarget new];
    inst->intCallback = std::move (cb);
    inst->stringCallback = nullptr;
    inst->dismissCallback = nullptr;
    return inst;
}

+ (instancetype)sharedInstanceWithStringCallback:(std::function<void (juce::String)>)cb
{
    // Safe as singleton: only one menu is shown at a time.
    static MenuTarget* inst;
    if (inst == nil)
        inst = [MenuTarget new];
    inst->stringCallback = std::move (cb);
    inst->intCallback = nullptr;
    inst->dismissCallback = nullptr;
    return inst;
}

- (void)menuItemSelected:(id)sender
{
    NSMenuItem* item = (NSMenuItem*) sender;
    dismissCallback = nullptr;
    if (intCallback != nullptr)
        intCallback ((int) item.tag);
    else if (stringCallback != nullptr)
        stringCallback (juce::String ([item.title UTF8String]));
}

- (void)setDismissCallback:(std::function<void()>)cb
{
    dismissCallback = std::move (cb);
}

- (void)menuDidClose:(NSMenu*)menu
{
    if (dismissCallback != nullptr)
    {
        auto cb { dismissCallback };
        dismissCallback = nullptr;
        cb();
    }
    dismissCallback = nullptr;
}

@end

namespace jam::menu
{
/*____________________________________________________________________________*/

static void runMenuWithMonitor (NSView* view, std::function<void()> showMenu)
{
    __block id monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDown
                                                              handler:^NSEvent* (NSEvent* event) {
                                                              [NSEvent removeMonitor:monitor];
                                                              monitor = nil;
                                                              return nil;
                                                              }];

    showMenu();

    if (monitor != nil)
    {
        [NSEvent removeMonitor:monitor];
        monitor = nil;
    }

    [[view window] makeFirstResponder:view];
}

void showNative (void* nsViewHandle,
                 const std::map<int, juce::String>& map,
                 std::function<void (int)> onSelect)
{
    NSView* parentView = (NSView*) nsViewHandle;
    NSMenu* menu = [[NSMenu alloc] initWithTitle:@"Context"];

    // Add the actual items
    for (const auto& [tag, value] : map)
    {
        NSString* title = [NSString stringWithUTF8String:value.toRawUTF8()];
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title
                                                      action:@selector (menuItemSelected:)
                                               keyEquivalent:@""];
        [item setTag:tag];
        [item setTarget:[MenuTarget sharedInstanceWithCallback:onSelect]];
        [menu addItem:item];
    }

    runMenuWithMonitor (parentView, [&] {
        [NSMenu popUpContextMenu:menu withEvent:[NSApp currentEvent] forView:parentView];
    });
}

static NSMenu* buildNSMenu (const juce::PopupMenu& juceMenu, MenuTarget* target)
{
    NSMenu* nsMenu = [[NSMenu alloc] initWithTitle:@""];
    [nsMenu setAutoenablesItems:NO];

    juce::PopupMenu::MenuItemIterator iter (juceMenu, false);

    while (iter.next())
    {
        const auto& item = iter.getItem();

        if (item.isSeparator)
        {
            [nsMenu addItem:[NSMenuItem separatorItem]];
        }
        else if (item.isSectionHeader)
        {
            NSString* title = [NSString stringWithUTF8String:item.text.toRawUTF8()];
            NSMenuItem* headerItem = [[NSMenuItem alloc] initWithTitle:title action:nil keyEquivalent:@""];
            [headerItem setEnabled:NO]; // non-selectable
            [nsMenu addItem:headerItem];
        }
        else if (item.subMenu != nullptr)
        {
            NSString* title = [NSString stringWithUTF8String:item.text.toRawUTF8()];
            NSMenuItem* subMenuItem = [[NSMenuItem alloc] initWithTitle:title action:nil keyEquivalent:@""];
            NSMenu* subMenu = buildNSMenu (*item.subMenu, target);
            [subMenuItem setSubmenu:subMenu];
            [subMenuItem setEnabled:item.isEnabled];
            [nsMenu addItem:subMenuItem];
        }
        else
        {
            juce::String sentenceCase { item.text.substring (0, 1).toUpperCase()
                                        + item.text.substring (1).toLowerCase() };
            NSString* title = [NSString stringWithUTF8String:sentenceCase.toRawUTF8()];
            NSMenuItem* menuItem = [[NSMenuItem alloc] initWithTitle:title
                                                              action:@selector (menuItemSelected:)
                                                       keyEquivalent:@""];
            [menuItem setTag:item.itemID];
            [menuItem setTarget:target];
            [menuItem setEnabled:item.isEnabled];
            [menuItem setState:item.isTicked ? NSControlStateValueOn : NSControlStateValueOff]; // shows a checkmark
            [nsMenu addItem:menuItem];
        }
    }

    return nsMenu;
}

void showNative (void* nsViewHandle, const juce::PopupMenu& menu, std::function<void (int)> onSelect)
{
    NSView* parentView = (NSView*) nsViewHandle;
    MenuTarget* target = [MenuTarget sharedInstanceWithCallback:onSelect];
    NSMenu* nsMenu = buildNSMenu (menu, target);

    [target setDismissCallback:[onSelect]
            {
                if (onSelect != nullptr)
                    onSelect (0);
            }];
    [nsMenu setDelegate:target];

    runMenuWithMonitor (parentView, [&] {
        [NSMenu popUpContextMenu:nsMenu withEvent:[NSApp currentEvent] forView:parentView];
    });
}

void showNativeOptions (void* nsViewHandle,
                        const std::map<int, juce::String>& map,
                        int selectedKey,
                        std::function<void (int)> onSelect,
                        const juce::String& header)
{
    NSView* parentView = (NSView*) nsViewHandle;
    NSMenu* menu = [[NSMenu alloc] initWithTitle:@"Context"];

    // If header is provided, add it first
    if (header.isNotEmpty())
    {
        NSString* headerTitle = [NSString stringWithUTF8String:header.toRawUTF8()];
        NSMenuItem* headerItem = [[NSMenuItem alloc] initWithTitle:headerTitle
                                                            action:nil
                                                     keyEquivalent:@""];
        [headerItem setEnabled:NO]; // non-selectable
        [menu addItem:headerItem];

        // Add a separator below the header
        [menu addItem:[NSMenuItem separatorItem]];
    }

    for (const auto& [tag, value] : map)
    {
        auto rawItem = jam::Format::toTitleCase (value).toRawUTF8();

        NSString* title = [NSString stringWithUTF8String:rawItem];
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title
                                                      action:@selector (menuItemSelected:)
                                               keyEquivalent:@""];

        [item setTag:tag];
        [item setTarget:[MenuTarget sharedInstanceWithCallback:onSelect]];

        // Mark the selected item
        if (tag == selectedKey)
            [item setState:NSControlStateValueOn]; // shows a checkmark

        [menu addItem:item];
    }

    runMenuWithMonitor (parentView, [&] {
        [NSMenu popUpContextMenu:menu withEvent:[NSApp currentEvent] forView:parentView];
    });
}

void addFilesToNativeMenu (NSMenu* parentMenu,
                           const juce::File& parentDirectory,
                           const juce::String& extension,
                           bool addRecursively,
                           const std::function<void (juce::String)>& onSelect)
{
    if (parentDirectory.isDirectory())
    {
        auto findDirs { juce::File::findDirectories | juce::File::ignoreHiddenFiles };
        auto findFiles { juce::File::findFiles | juce::File::ignoreHiddenFiles };

        if (addRecursively)
        {
            auto directories { parentDirectory.findChildFiles (findDirs, false) };
            directories.sort();

            for (auto& sub : directories)
            {
                if (sub.findChildFiles (findFiles, true, extension).size() > 0)
                {
                    NSString* dirTitle = [NSString stringWithUTF8String:sub.getFileNameWithoutExtension().toRawUTF8()];
                    NSMenuItem* dirItem = [[NSMenuItem alloc] initWithTitle:dirTitle action:nil keyEquivalent:@""];
                    NSMenu* subMenu = [[NSMenu alloc] initWithTitle:dirTitle];
                    [dirItem setSubmenu:subMenu];
                    [parentMenu addItem:dirItem];

                    addFilesToNativeMenu (subMenu, sub, extension, true, onSelect);
                }
            }
        }

        auto files { parentDirectory.findChildFiles (findFiles, false, extension) };
        files.sort();

        for (auto& file : files)
        {
            NSString* title = [NSString stringWithUTF8String:file.getFileNameWithoutExtension().toRawUTF8()];
            NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title
                                                          action:@selector (menuItemSelected:)
                                                   keyEquivalent:@""];
            [item setTarget:[MenuTarget sharedInstanceWithStringCallback:onSelect]];
            [parentMenu addItem:item];
        }
    }
}

void addPresetsToNativeMenu (NSMenu* rootMenu,
                             const juce::File& factoryPresetsDirectory,
                             const juce::File& userPresetsDirectory,
                             const juce::String& presetExtension,
                             const std::function<void (juce::String)>& onSelect)
{
    addFilesToNativeMenu (rootMenu, factoryPresetsDirectory, presetExtension, true, onSelect);

    if (userPresetsDirectory.isDirectory())
    {
        NSMenu* userMenu = [[NSMenu alloc] initWithTitle:@"User Presets"];
        addFilesToNativeMenu (userMenu, userPresetsDirectory, presetExtension, true, onSelect);

        if ([userMenu numberOfItems] > 0)
        {
            [rootMenu addItem:[NSMenuItem separatorItem]];
            NSMenuItem* userItem = [[NSMenuItem alloc] initWithTitle:@"User Presets" action:nil keyEquivalent:@""];
            [userItem setSubmenu:userMenu];
            [rootMenu addItem:userItem];
        }
    }
}

void showNativeAt (void* nsViewHandle,
                   const juce::PopupMenu& menu,
                   juce::Rectangle<int> targetBounds,
                   std::function<void (int)> onSelect)
{
    NSView* parentView = (NSView*) nsViewHandle;
    MenuTarget* target = [MenuTarget sharedInstanceWithCallback:onSelect];
    NSMenu* nsMenu = buildNSMenu (menu, target);

    [target setDismissCallback:[onSelect]
            {
                if (onSelect != nullptr)
                    onSelect (0);
            }];
    [nsMenu setDelegate:target];
    [nsMenu setMinimumWidth:(CGFloat) targetBounds.getWidth()];

    NSPoint point;

    if ([parentView isFlipped])
        point = NSMakePoint ((CGFloat) targetBounds.getX(), (CGFloat) targetBounds.getBottom());
    else
        point = NSMakePoint ((CGFloat) targetBounds.getX(),
                             [parentView frame].size.height - (CGFloat) targetBounds.getBottom());

    runMenuWithMonitor (parentView, [&] {
        [nsMenu popUpMenuPositioningItem:nil atLocation:point inView:parentView];
    });
}

void showNativeMenuFromFiles (void* nsViewHandle,
                              const juce::File& factoryDir,
                              const juce::File& userDir,
                              juce::StringRef extension,
                              std::function<void (juce::String)> onSelect)
{
    NSView* parentView = (NSView*) nsViewHandle;
    NSMenu* menu = [[NSMenu alloc] initWithTitle:@"Presets"];

    addPresetsToNativeMenu (menu, factoryDir, userDir, extension, onSelect);

    runMenuWithMonitor (parentView, [&] {
        [NSMenu popUpContextMenu:menu withEvent:[NSApp currentEvent] forView:parentView];
    });
}



#endif // JUCE_MAC


/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::menu

