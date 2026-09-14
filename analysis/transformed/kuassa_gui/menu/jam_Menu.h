/**
 * @file jam_Menu.h
 * @brief Popup menu builders — key/label option menus, file/directory browsers, native menu bridges.
 */

namespace jam::menu
{
/*____________________________________________________________________________*/

/**
 * @brief Builds a section-headed popup menu from a key/label map, ticking the selected entry.
 * @param map          Map from item key to display label.
 * @param header       Section header text; underscores are stripped.
 * @param editorWidth  Unused.
 * @param selected     Value string matched against @p map's keys to determine the ticked item.
 * @return Populated popup menu.
 */
static juce::PopupMenu createOptions (const jam::HashMap<int, juce::String>& map,
                                      const juce::String& header,
                                      [[maybe_unused]] int editorWidth,
                                      const juce::String& selected) noexcept
{
    juce::PopupMenu menu;

    menu.addSectionHeader (Format::removeUnderscore (header));
    menu.addSeparator();

    const auto inverse { Map::getKey (map) };

    for (const auto& [key, value] : map)
    {
        menu.addItem (key, value, true, key == inverse.at (selected));
    }

    return menu;
}

/**
 * @brief Builds a section-headed popup menu from a key/label map, without a ticked entry.
 * @param map          Map from item key to display label.
 * @param header       Section header text; underscores are stripped.
 * @param editorWidth  Unused.
 * @return Populated popup menu.
 */
static juce::PopupMenu
create (const std::map<int, juce::String>& map, const juce::String& header, [[maybe_unused]] int editorWidth) noexcept
{
    juce::PopupMenu menu;

    menu.addSectionHeader (Format::removeUnderscore (header));
    menu.addSeparator();

    for (const auto& [key, value] : map)
    {
        menu.addItem (key, value);
    }

    return menu;
}

/**
 * @brief Populates a popup menu with files (and, recursively, sub-menus per directory) matching an extension.
 * @param itemIdOffset     Running item ID counter; incremented as items are added.
 * @param menu             Menu to populate; cleared before rebuilding.
 * @param parentDirectory  Directory to search.
 * @param extension        File extension filter.
 * @param addRecursively   `true` to add sub-directories as nested sub-menus.
 * @return Flat list of all matching files found, recursively.
 */
static juce::Array<juce::File> addFiles (int& itemIdOffset,
                                         juce::PopupMenu* menu,
                                         const juce::File& parentDirectory,
                                         const juce::String& extension,
                                         bool addRecursively = true)
{
    juce::Array<juce::File> filesToBeIndexed;

    /** ensure menu is empty before rebuilding */
    menu->clear();

    if (parentDirectory.isDirectory())
    {
        auto findDirectories { juce::File::findDirectories | juce::File::ignoreHiddenFiles };
        auto findFiles { juce::File::findFiles | juce::File::ignoreHiddenFiles };

        /** If recursive, get sub directories from parent and apply this function
              for each directory */

        if (addRecursively)
        {
            /** recursive flag for findChildFiles must be set to false */
            auto directories { parentDirectory.findChildFiles (findDirectories, false) };

            directories.sort();

            if (directories.size() > 0)
            {
                for (auto& sub : directories)
                {
                    if (sub.findChildFiles (findFiles, true, extension).size() > 0)
                    {
                        juce::PopupMenu subMenu;
                        menu::addFiles (++itemIdOffset, &subMenu, sub, extension, true);
                        menu->addSubMenu (sub.getFileNameWithoutExtension(), subMenu);
                    }
                }
            }
        }

        //==============================================================================
        /** don't add recursively, thus only add found files in the current directory */

        auto files { parentDirectory.findChildFiles (findFiles, false, extension) };

        files.sort();

        if (files.size() > 0)
        {
            for (auto& file : files)
            {
                menu->addItem (itemIdOffset++, file.getFileNameWithoutExtension());
            }
        }

        //==============================================================================

        /** Finally we add found files to the array. Here recursive flag set to true, so it will include all found files from the parent directory */
        filesToBeIndexed = parentDirectory.findChildFiles (findFiles, true, extension);
        filesToBeIndexed.sort();
    }

    return filesToBeIndexed;
}

/**
 * @brief Populates a popup menu with directories (and, recursively, sub-menus per sub-directory).
 * @param itemIdOffset      Running item ID counter; incremented as items are added.
 * @param menu              Menu to populate; cleared before rebuilding.
 * @param parentDirectory   Directory to search.
 * @param addRecursively    `true` to add sub-directories as nested sub-menus.
 * @param shouldBeFormatted `true` to title-case directory names for display.
 * @param wildCardPattern   Directory name filter pattern.
 * @return Flat list of all matching directories found, recursively.
 */
static juce::Array<juce::File> addDirectories (int& itemIdOffset,
                                               juce::PopupMenu* menu,
                                               const juce::File& parentDirectory,
                                               bool addRecursively,
                                               bool shouldBeFormatted = true,
                                               const juce::String& wildCardPattern = "*")
{
    juce::Array<juce::File> filesToBeIndexed;

    /** ensure menu is empty before rebuilding */
    menu->clear();

    auto findDirectories { juce::File::findDirectories | juce::File::ignoreHiddenFiles };

    /** If recursive, get sub directories from parent and apply this function
          for each directory */

    if (addRecursively)
    {
        /** recursive flag for findChildFiles must be set to false */
        auto directories { parentDirectory.findChildFiles (findDirectories, false, wildCardPattern) };

        directories.sort();

        if (directories.size() > 0)
        {
            for (auto& sub : directories)
            {
                if (sub.findChildFiles (findDirectories, true, wildCardPattern).size() > 0)
                {
                    juce::PopupMenu subMenu;
                    menu::addDirectories (++itemIdOffset, &subMenu, sub, true, shouldBeFormatted, wildCardPattern);

                    juce::String name { sub.getFileNameWithoutExtension() };

                    if (shouldBeFormatted)
                        name = Format::toTitleCase (name);

                    menu->addSubMenu (name, subMenu);
                }
            }
        }
    }

    //==============================================================================
    /** don't add recursively, thus only add found files in the current directory */

    auto files { parentDirectory.findChildFiles (findDirectories, false, wildCardPattern) };

    files.sort();

    if (files.size() > 0)
    {
        for (auto& file : files)
        {
            juce::String name { file.getFileNameWithoutExtension() };

            if (shouldBeFormatted)
                name = Format::toTitleCase (name);

            menu->addItem (++itemIdOffset, name);
        }
    }

    //==============================================================================

    /** Finally we add found files to the array. Here recursive flag set to true, so it will include all found files from the parent directory */
    filesToBeIndexed = parentDirectory.findChildFiles (findDirectories, true, wildCardPattern);
    filesToBeIndexed.sort();

    return filesToBeIndexed;
}

/**
 * @brief Populates a popup menu with factory presets, plus a "User Presets" sub-menu when any exist.
 * @param menu                     Menu to populate.
 * @param factoryPresetsDirectory  Directory searched for factory presets.
 * @param userPresetsDirectory     Directory searched for user presets.
 * @param presetExtension          Preset file extension filter.
 * @return Combined list of factory and user preset files found.
 */
static juce::Array<juce::File> addPresets (juce::PopupMenu& menu,
                                           const juce::File& factoryPresetsDirectory,
                                           const juce::File& userPresetsDirectory,
                                           juce::StringRef presetExtension)
{
    int itemIdOffset { 1 };
    auto factoryPresets { addFiles (itemIdOffset, &menu, factoryPresetsDirectory, presetExtension, true) };

    juce::PopupMenu userPresetsMenu;
    auto userPresets { addFiles (itemIdOffset, &userPresetsMenu, userPresetsDirectory, presetExtension, true) };

    if (userPresets.size() > 0)
    {
        menu.addSeparator();
        menu.addSubMenu (Id::userPresets.toString(), userPresetsMenu);
    }

    factoryPresets.addArray (userPresets);
    return factoryPresets;
}

#if JUCE_MAC
//==============================================================================
/**
 * @brief Shows a native NSMenu built from a key/label map, anchored to a view.
 * @param nsViewHandle  Native `NSView*` handle to anchor the menu to.
 * @param map           Map from item key to display label.
 * @param onSelect      Called with the selected item's key.
 */
void showNative (void* nsViewHandle, const std::map<int, juce::String>& map, std::function<void (int)> onSelect);

/**
 * @brief Shows a native NSMenu built from a `juce::PopupMenu`, anchored to a view.
 * @param nsViewHandle  Native `NSView*` handle to anchor the menu to.
 * @param menu          Menu item data model to translate to a native menu.
 * @param onSelect      Called with the selected item's ID; called with 0 when the menu closes without a selection.
 */
void showNative (void* nsViewHandle, const juce::PopupMenu& menu, std::function<void (int)> onSelect);

/**
 * @brief Shows a native NSMenu built from a `juce::PopupMenu`, anchored to explicit screen bounds.
 * @param nsViewHandle  Native `NSView*` handle to anchor the menu to.
 * @param menu          Menu item data model to translate to a native menu.
 * @param targetBounds  Screen bounds the menu is positioned relative to.
 * @param onSelect      Called with the selected item's ID; called with 0 when the menu closes without a selection.
 */
void showNativeAt (void* nsViewHandle,
                   const juce::PopupMenu& menu,
                   juce::Rectangle<int> targetBounds,
                   std::function<void (int)> onSelect);

/**
 * @brief Shows a native NSMenu of selectable key/label options, ticking the currently selected key.
 * @param nsViewHandle  Native `NSView*` handle to anchor the menu to.
 * @param map           Map from item key to display label.
 * @param selectedKey   Currently selected key, ticked in the menu.
 * @param onSelect      Called with the selected item's key.
 * @param header        Optional section header text.
 */
void showNativeOptions (void* nsViewHandle,
                        const std::map<int, juce::String>& map,
                        int selectedKey,
                        std::function<void (int)> onSelect,
                        const juce::String& header = juce::String());

/**
 * @brief Shows a native NSMenu built from factory/user preset files, mirroring addPresets().
 * @param nsViewHandle  Native `NSView*` handle to anchor the menu to.
 * @param factoryDir    Directory searched for factory presets.
 * @param userDir       Directory searched for user presets.
 * @param extension     Preset file extension filter.
 * @param onSelect      Called with the selected preset's file name.
 */
void showNativeMenuFromFiles (void* nsViewHandle,
                              const juce::File& factoryDir,
                              const juce::File& userDir,
                              juce::StringRef extension,
                              std::function<void (juce::String)> onSelect);

#endif// JUCE_MAC

#if JUCE_WINDOWS
//==============================================================================
/**
 * @brief Shows a native Win32 menu built from a key/label map, anchored to a window.
 * @param windowHandle  Native `HWND` handle to anchor the menu to.
 * @param map           Map from item key to display label.
 * @param onSelect      Called with the selected item's key.
 */
void showNative (void* windowHandle, const std::map<int, juce::String>& map, std::function<void (int)> onSelect);

/**
 * @brief Shows a native Win32 menu built from a `juce::PopupMenu`, anchored to a window.
 * @param windowHandle  Native `HWND` handle to anchor the menu to.
 * @param menu          Menu item data model to translate to a native menu.
 * @param onSelect      Called with the selected item's ID.
 */
void showNative (void* windowHandle, const juce::PopupMenu& menu, std::function<void (int)> onSelect);

/**
 * @brief Shows a native Win32 menu built from a `juce::PopupMenu`, anchored to explicit screen bounds.
 * @param windowHandle  Native `HWND` handle to anchor the menu to.
 * @param menu          Menu item data model to translate to a native menu.
 * @param targetBounds  Screen bounds the menu is positioned relative to.
 * @param onSelect      Called with the selected item's ID.
 */
void showNativeAt (void* windowHandle,
                   const juce::PopupMenu& menu,
                   juce::Rectangle<int> targetBounds,
                   std::function<void (int)> onSelect);

/**
 * @brief Shows a native Win32 menu of selectable key/label options, ticking the currently selected key.
 * @param windowHandle  Native `HWND` handle to anchor the menu to.
 * @param map           Map from item key to display label.
 * @param selectedKey   Currently selected key, ticked in the menu.
 * @param onSelect      Called with the selected item's key.
 * @param header        Optional section header text.
 */
void showNativeOptions (void* windowHandle,
                        const std::map<int, juce::String>& map,
                        int selectedKey,
                        std::function<void (int)> onSelect,
                        const juce::String& header = juce::String());

#endif// JUCE_WINDOWS

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::menu
