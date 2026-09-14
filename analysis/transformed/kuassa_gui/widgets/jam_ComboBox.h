/**
 * @file jam_ComboBox.h
 * @brief Static helpers for populating and querying `juce::ComboBox` item menus.
 */

namespace jam
{
/*____________________________________________________________________________*/

/** @brief Static namespace-like collection of `juce::ComboBox` population/lookup helpers. */
struct ComboBox
{
    /**
     * @brief Populates a combo box's root menu with files matching an extension.
     * @param comboBox         Combo box to populate.
     * @param parentDirectory  Directory to search.
     * @param extension        File extension filter.
     * @param addRecursively   `true` to add sub-directories as nested sub-menus.
     * @return Flat list of all matching files found, recursively.
     */
    static juce::Array<juce::File> addFiles (juce::ComboBox* comboBox,
                                             const juce::File& parentDirectory,
                                             juce::StringRef extension,
                                             bool addRecursively = true) noexcept
    {
        int itemIdOffset { 1 };
        return menu::addFiles (itemIdOffset,
                               comboBox->getRootMenu(),
                               parentDirectory,
                               Format::toFileExtension (extension),
                               addRecursively);
    }

    /**
     * @brief Populates a combo box's root menu with directories.
     * @param comboBox          Combo box to populate.
     * @param parentDirectory   Directory to search.
     * @param addRecursively    `true` to add sub-directories as nested sub-menus.
     * @param shouldBeFormatted `true` to title-case directory names for display.
     * @param wildCardPattern   Directory name filter pattern.
     * @return Flat list of all matching directories found, recursively.
     */
    static juce::Array<juce::File> addDirectories (juce::ComboBox* comboBox,
                                                   const juce::File& parentDirectory,
                                                   bool addRecursively,
                                                   bool shouldBeFormatted = true,
                                                   const juce::String& wildCardPattern = "*") noexcept
    {
        int itemIdOffset { 1 };
        return menu::addDirectories (itemIdOffset,
                                     comboBox->getRootMenu(),
                                     parentDirectory,
                                     addRecursively,
                                     shouldBeFormatted,
                                     wildCardPattern);
    }

    /**
     * @brief Enables/disables a paged combo box's previous/next navigation buttons
     * and clips the selection index to the valid range.
     * @tparam ComboBoxType  Combo box type exposing `getButton()`, `navigation`,
     * `getSelectedItemIndex()`, and `getNumItems()`.
     * @param comboBox  Combo box whose navigation buttons are refreshed.
     */
    template <typename ComboBoxType>
    static void refreshNavigationButtons (ComboBoxType* comboBox)
    {
        comboBox->getButton (ComboBoxType::navigation::previous)
            ->setEnabled (not (comboBox->getSelectedItemIndex() <= 0));

        comboBox->getButton (ComboBoxType::navigation::next)
            ->setEnabled (not (comboBox->getSelectedItemIndex() >= comboBox->getNumItems() - 1));

        Value::clip (comboBox->getSelectedItemIndex(), 0, comboBox->getNumItems() - 1);
    }

    /**
     * @brief Finds an item's index by matching its display text, case-insensitively.
     * @param comboBox    Combo box to search.
     * @param itemToFind  Display text to match.
     * @return Zero-based item index, or -1 when not found.
     */
    static int getItemIndex (juce::ComboBox* comboBox, const juce::String& itemToFind) noexcept
    {
        for (int index { 0 }; index < comboBox->getNumItems(); ++index)
        {
            if (itemToFind.equalsIgnoreCase (comboBox->getItemText (index)))
                return index;
        }

        return -1;
    }

    /**
     * @brief Finds an item's ID by matching its display text.
     * @param comboBox    Combo box to search.
     * @param itemToFind  Display text to match.
     * @return Item ID, or an implementation-defined result when not found (see getItemIndex()).
     */
    static int getItemId (juce::ComboBox* comboBox,
                          const juce::String& itemToFind) noexcept
    {
        return comboBox->getItemId (getItemIndex (comboBox, itemToFind));
    }

    /**
     * @brief Populates a combo box's root menu with files matching an extension, recursively.
     * @param comboBox       Combo box to populate.
     * @param directory      Directory to search.
     * @param fileExtension  File extension filter.
     * @return Flat list of all matching files found, recursively.
     */
    static juce::Array<juce::File> addFilesToComboBox (juce::ComboBox* comboBox,
                                                       const juce::File& directory,
                                                       juce::StringRef fileExtension)
    {
        int itemIdOffet { 1 };
        return menu::addFiles (itemIdOffet, comboBox->getRootMenu(), directory, fileExtension, true);
    }

    //==============================================================================
    /**
     * @brief Adds factory and user presets to a combo box.
     *
     * @param comboBox The combo box to which presets will be added.
     * @param factoryPresetsDirectory Directory containing factory presets.
     * @param userPresetsDirectory Directory containing user presets.
     * @param presetExtension File extension of the preset files.
     * @return An array of combined factory and user presets.
     */
    static juce::Array<juce::File> addPresetsToComboBox (juce::ComboBox* comboBox,
                                                         const juce::File& factoryPresetsDirectory,
                                                         const juce::File& userPresetsDirectory,
                                                         juce::StringRef presetExtension)
    {
        if (comboBox == nullptr)
        {
            jassertfalse;// ComboBox pointer is null, add proper error handling here
            return {};
        }

        int itemIdOffset = 1;
        auto factoryPresets = menu::addFiles (itemIdOffset,
                                              comboBox->getRootMenu(),
                                              factoryPresetsDirectory,
                                              presetExtension,
                                              true);

        juce::PopupMenu userPresetsMenu;
        auto userPresets = menu::addFiles (itemIdOffset,
                                           &userPresetsMenu,
                                           userPresetsDirectory,
                                           presetExtension,
                                           true);

        if (userPresets.size() > 0)
        {
            comboBox->getRootMenu()->addSeparator();// Add separator before User Presets
            comboBox->getRootMenu()->addSubMenu (Id::userPresets.toString(), userPresetsMenu);
        }

        // Combine both factory and user presets into a single array and return
        factoryPresets.addArray (userPresets);
        return factoryPresets;
    }

    //==============================================================================
    /**
     * @brief Finds an item's ID by matching its display text exactly.
     * @param comboBox      Combo box to search.
     * @param searchString  Display text to match.
     * @return Item ID, or -1 when no matching item is found.
     */
    static int findComboBoxIDFromString (juce::ComboBox& comboBox,
                                         const juce::String& searchString)
    {
        for (int i = 0; i < comboBox.getNumItems(); ++i)
        {
            if (comboBox.getItemText (i) == searchString)
            {
                return comboBox.getItemId (i);
            }
        }
        // Return -1 if no matching item is found
        return -1;
    }

    /**
     * @brief Selects the item whose display text matches @p searchString.
     * @param comboBox      Combo box to update.
     * @param searchString  Display text to match.
     * @return `true` when a matching item was found and selected.
     */
    static bool setComboBoxSelectedIdFromString (juce::ComboBox& comboBox,
                                                 const juce::String& searchString)
    {
        int id = findComboBoxIDFromString (comboBox, searchString);
        if (id != -1)
        {
            comboBox.setSelectedId (id);
            return true;
        }
        return false;
    }
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
