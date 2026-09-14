/**
 * @file jam_ParameterManager.h
 * @brief Product/preset/settings metadata manager sourced from `ProjectInfo`
 *        statics — presets and user settings paths.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/
using ValueMap = jam::HashMap<juce::String, juce::var>;

class ParameterManager final : public Instance<ParameterManager>
{
public:
    ParameterManager();
    ParameterManager (ValueMap newDefaultValues, ValueMap newUnits);

    ~ParameterManager() = default;
    //==============================================================================
    /**
     * @brief Retrieves the product name.
     *
     * This method returns the name of the product, which is provided by
     * `ProjectInfo`.
     *
     * @return A `juce::String` containing the product name.
     */
    juce::String getProductName() const noexcept;
    //==============================================================================
    /**
     * @brief Retrieves the directory for default user presets.
     *
     * This method constructs the path to the default presets directory by combining
     * the user's directory and the `presets` subdirectory, further appending the
     * product name as a unique identifier.
     *
     * @return A `juce::File` representing the default presets directory for the user.
     */
    juce::File getDefaultPresetsDirectory() const noexcept;

    /**
     * @brief Retrieves the directory for factory default presets.
     *
     * This method determines the directory where factory default presets are stored.
     * If no version string is provided, it returns the base product directory. If a
     * version string is given, the path includes a subdirectory for that version.
     *
     * @param versionString An optional `juce::String` representing the version (defaults to empty).
     * @return A `juce::File` representing the factory default presets directory.
     */
    juce::File getFactoryDefaultPresetsDirectory (const juce::String& versionString = juce::String()) const noexcept;

    /**
     * @brief Retrieves the directory for user presets.
     *
     * This method fetches the user presets directory. If no valid user presets
     * directory is found in the settings, it falls back to the default presets directory.
     *
     * @return A `juce::File` representing the user presets directory.
     */
    juce::File getUserPresetsDirectory() const noexcept;

    /**
     * @brief Retrieves the file extension for presets.
     *
     * This method provides the preset extension, which is provided by
     * `ProjectInfo`.
     *
     * @return A `juce::String` containing the preset extension.
     */
    juce::String getPresetExtension() const noexcept;

    /**
     * @brief Retrieves the file for the default initialization preset.
     *
     * This method constructs the file path for the default initialization preset
     * within the default presets directory.
     *
     * @return A `juce::File` representing the default initialization preset.
     */
    juce::File getDefaultInitPreset() const noexcept;

    /**
     * @brief Determines the index of a specific preset in the provided preset list.
     *
     * This method iterates through the given array of presets to find a file that
     * matches the specified filename with its extension. If no match is found, it
     * returns -1 to indicate the preset was not located.
     *
     * @param presets An array of `juce::File` objects representing the available presets.
     * @param presetFilenameWithExtension The filename of the preset, including its extension.
     * @return An integer representing the index of the preset in the list, or -1 if not found.
     */
    int getPresetIndex (const juce::Array<juce::File>& presets,
                        const juce::String& presetFilenameWithExtension) const noexcept;

    //==============================================================================
    /**
     * @brief Returns the path to the user settings file. Read-only — no file is created or written.
     * @return A `juce::File` representing the user settings file path.
     */
    juce::File getUserSettings() const noexcept;

    /**
     * @brief Returns the user settings file, creating it from defaults when missing or outdated.
     *
     * Builds the path via getUserSettings(). If the existing file is older than the provided
     * defaults it is deleted; if no file exists, the defaults are written. The settings
     * directory is created on demand via getUserSettingsDirectory().
     *
     * @param defaultSettings The default settings XML text to write when the file is absent or stale.
     * @param defaultDocument The default settings text, parsed once by the caller.
     * @return A `juce::File` representing the user settings file.
     */
    juce::File getOrCreateUserSettings (const juce::String& defaultSettings, const Document& defaultDocument) const noexcept;

    /**
     * @brief Validates the user settings file.
     *
     * This method checks if the user settings file is valid by performing the following steps:
     * - Verifies the existence of the settings file.
     * - Parses the settings file as an XML element.
     * - Searches for a `UIScale` child node in the XML.
     * - Confirms that the `UIScale` value exists within the predefined valid values in the UI scale map.
     *
     * @return True if the settings file is valid, false otherwise.
     */
    bool isSettingsValid() const noexcept;

    /**
     * @brief Retrieves the default value for a specific setting.
     *
     * This method checks the user settings file to determine the value of a specific
     * setting, identified by its `identifier`. If no value is found, the method
     * returns the provided fallback value.
     *
     * @param identifier A `juce::StringRef` representing the identifier of the setting.
     * @param defaultFallback A `juce::String` containing the fallback value to return
     *                        if the setting is not found.
     * @return A `juce::String` representing the default value for the specified setting,
     *         or the fallback value if not found.
     */
    juce::String getDefaultValueForSettings (juce::StringRef identifier,
                                            const juce::String& defaultFallback) const noexcept;
    /**
     * @brief Retrieves the default preset filename without its extension.
     *
     * This method fetches the default preset filename by looking up the user settings
     * or falling back to the default initialization preset filename (without extension) if not found.
     *
     * @return A `juce::String` representing the default preset filename without its extension.
     */
    juce::String getDefaultPreset() const noexcept;

    /**
     * @brief Retrieves the default UI scale value.
     *
     * This method fetches the default UI scale value from the user settings file or
     * falls back to the predefined default scale value from the `UIScaleMap` map if not found.
     *
     * @return A `juce::String` representing the default UI scale value.
     */
    juce::String getDefaultScale() const noexcept;

    /**
     * @brief Retrieves the default appearance value.
     *
     * This method fetches the default appearance value from the user settings file or
     * falls back to the predefined default value from the `Appearance` map if not found.
     *
     * @return A `juce::String` representing the default appearance value.
     */
    juce::String getDefaultAppearance() const noexcept;
#if JAM_USING_OVERSAMPLING
    /**
     * @brief Retrieves the default oversampling value.
     *
     * This method fetches the default oversampling setting from the user settings file.
     * If no valid value is found, it falls back to the default value defined in the
     * `Oversampling` map.
     *
     * @return A `juce::String` representing the default oversampling value.
     */
    juce::String getDefaultOversampling() const noexcept;
#endif //JAM_USING_OVERSAMPLING
    /**
     * @brief Retrieves the default value map.
     *
     * This method returns the default value table held in the internal
     * `defaultValues` member.
     *
     * @return A `ValueMap` containing the default values for parameters.
     */

    ValueMap getDefaultValueMap() const noexcept;

    ValueMap getUnitMap() const noexcept;
    juce::String getVersionString() const noexcept;
    juce::String getProductWebsite() const noexcept;
    juce::File getUserManual() const noexcept;
#if JAM_USING_AQUATIC_PRIME
    juce::String getLicenseExtension() const noexcept;
    juce::File getLicenseFile() const noexcept;
    juce::String getPublicKey() const noexcept;
#endif // JAM_USING_AQUATIC_PRIME
    /**
     * @brief Retrieves the default panel height.
     *
     * This method fetches the panel height value from the user settings file. If no valid
     * value is found, it falls back to a default height of 24 pixels.
     *
     * @return An integer representing the panel height.
     */
    int getPanelHeight() const noexcept;

    //==============================================================================
    /**
     * @brief Retrieves the directory for user settings.
     *
     * This method constructs and returns the path to the user settings directory,
     * ensuring the directory exists by creating it if necessary.
     *
     * @return A `juce::File` representing the user settings directory.
     */
    static juce::File getUserSettingsDirectory() noexcept;
private:
    //==============================================================================
    ValueMap defaultValues;
    ValueMap units;
#if JAM_USING_AQUATIC_PRIME
    juce::String publicKey;
    juce::String licenseFileName;
#endif // JAM_USING_AQUATIC_PRIME

    /**
     * @brief Checks whether the user settings file was created by an older version.
     *
     * This method compares the version of the existing user settings file to the version
     * specified in the provided default settings. It returns true if the existing file's
     * version is older than the version in the default settings.
     *
     * @param existingFile A `juce::File` object representing the existing settings file.
     * @param defaultSettings The default settings `Document`, parsed once by the caller,
     *                        to compare against.
     * @return True if the existing settings file's version is older, false otherwise.
     */
    static bool isSettingsOld (juce::File& existingFile,
                              const Document& defaultSettings) noexcept;

    static bool isScaleValid (const Document::Element& uiScale) noexcept;

    static const Document::Element* getScaleElement (const Document& document) noexcept;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterManager)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
