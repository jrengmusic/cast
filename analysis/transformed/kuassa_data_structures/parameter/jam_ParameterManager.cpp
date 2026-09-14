namespace jam
{
/*____________________________________________________________________________*/
ParameterManager::ParameterManager() = default;

ParameterManager::ParameterManager (ValueMap newDefaultValues, ValueMap newUnits)
    : defaultValues { std::move (newDefaultValues) }, units { std::move (newUnits) }
#if JAM_USING_AQUATIC_PRIME
    , publicKey (ProjectInfo::publicKey)
    , licenseFileName (Format::toFileName (ProjectInfo::projectName, ProjectInfo::licenseExtension))
#endif// JAM_USING_AQUATIC_PRIME
{
}

//==============================================================================
juce::String ParameterManager::getProductName() const noexcept { return ProjectInfo::projectName; }
//==============================================================================
juce::File ParameterManager::getDefaultPresetsDirectory() const noexcept
{
    return File::getOrCreateDirectory (
        File::getOrCreateDirectory (File::getUserDirectory(), Id::presets.toString()), getProductName());
}

juce::File ParameterManager::getFactoryDefaultPresetsDirectory (const juce::String& versionString) const noexcept
{
    if (versionString.isEmpty())
        return File::getCompanyCommonApplicationDataDirectory (Id::defaultPresets).getChildFile (getProductName());

    return File::getCompanyCommonApplicationDataDirectory (Id::defaultPresets)
        .getChildFile (getProductName())
        .getChildFile (versionString);
}

juce::File ParameterManager::getUserPresetsDirectory() const noexcept
{
    auto fallback { getDefaultPresetsDirectory() };

    return juce::File (
        getDefaultValueForSettings (Format::toValidID (Id::toTag (Id::userPresets), true), fallback.getFullPathName()));
}

juce::String ParameterManager::getPresetExtension() const noexcept { return Format::toFileExtension (ProjectInfo::presetExtension); }

juce::File ParameterManager::getDefaultInitPreset() const noexcept
{
    return getDefaultPresetsDirectory().getChildFile (Format::toFileName (ProjectInfo::presetDefault, ProjectInfo::presetExtension));
}

int ParameterManager::getPresetIndex (const juce::Array<juce::File>& presets,
                                   const juce::String& presetFilenameWithExtension) const noexcept
{
    // Iterate through the presets to compare filenames
    for (int index { 0 }; index < presets.size(); ++index)
        if (presets[index].getFileName() == presetFilenameWithExtension)
            return index;

    // If the preset is not found, return -1 or any other value that indicates not found
    return -1;
}

//==============================================================================
juce::File ParameterManager::getUserSettings() const noexcept
{
    return getUserSettingsDirectory().getChildFile (getProductName() + files::settingExtension);
}

juce::File ParameterManager::getOrCreateUserSettings (const juce::String& defaultSettings, const Document& defaultDocument) const noexcept
{
    auto file { getUserSettings() };

    /** create-with-defaults requires a valid default tree */
    jassert (not defaultDocument.root->id.isNull());

    if (isSettingsOld (file, defaultDocument))
        if (file.existsAsFile())
            file.deleteFile();

    if (not file.existsAsFile())
        file.replaceWithText (defaultSettings);

    return file;
}

bool ParameterManager::isScaleValid (const Document::Element& uiScale) noexcept
{
    if (uiScale.contains (Id::value))
    {
        const auto& scale { *map::UIScaleMap::getInstance() };
        const auto value { scale.get (*uiScale.get<juce::String> (Id::value)) };

        return jam::Map::contains (scale.get(), value);
    }
#if JUCE_DEBUG
    else
    {
        debug::Log::write ("isScaleValid: UI scale value not set");
    }
#endif

    return false;
}

const Document::Element* ParameterManager::getScaleElement (const Document& document) noexcept
{
    if (const auto* settingsElement { document.root->getChildByID (Id::toTag (Id::settings)) })
    {
        if (const auto* uiScale { settingsElement->getChildByID (Id::toTag (Id::UIScale)) })
        {
            return uiScale;
        }
#if JUCE_DEBUG
        else
        {
            debug::Log::write ("getScaleElement: UI scale element not found");
        }
#endif
    }
#if JUCE_DEBUG
    else
    {
        debug::Log::write ("getScaleElement: settings element not found");
    }
#endif

    return nullptr;
}

bool ParameterManager::isSettingsValid() const noexcept
{
    static const XmlValidator validator;

    if (auto file { getUserSettings() }; file.existsAsFile())
    {
        const auto document { Xml::parse (file.loadFileAsString()) };

        if (const auto validation { validator.isValid (document) }; validation.wasOk())
        {
            if (const auto* uiScale { getScaleElement (document) })
            {
                return isScaleValid (*uiScale);
            }
        }
#if JUCE_DEBUG
        else
        {
            debug::Log::write ("isSettingsValid: " + validation.getErrorMessage());
        }
#endif
    }
#if JUCE_DEBUG
    else
    {
        debug::Log::write ("isSettingsValid: settings file not found");
    }
#endif

    return false;
}

juce::String
ParameterManager::getDefaultValueForSettings (juce::StringRef identifier, const juce::String& defaultFallback) const noexcept
{
    if (auto file { getUserSettings() }; file.existsAsFile())
        if (const auto document { Xml::parse (file.loadFileAsString()) }; not document.root->id.isNull())
            if (const auto value { Xml::get<juce::String> (document, Id::toTag (Id::settings), identifier, Id::value) };
                value.isNotEmpty())
                return value;

    return defaultFallback;
}

juce::String ParameterManager::getDefaultPreset() const noexcept
{
    return getDefaultValueForSettings (Id::toTag (Id::preset), getDefaultInitPreset().getFileNameWithoutExtension());
}

juce::String ParameterManager::getDefaultScale() const noexcept
{
    return getDefaultValueForSettings (Id::toTag (Id::UIScale), map::UIScaleMap::getInstance()->getDefault());
}

juce::String ParameterManager::getDefaultAppearance() const noexcept
{
    return getDefaultValueForSettings (Id::toTag (Id::appearance), map::Appearance::getInstance()->getDefault());
}

#if JAM_USING_OVERSAMPLING
juce::String ParameterManager::getDefaultOversampling() const noexcept
{
    return getDefaultValueForSettings (Id::toTag (Id::oversampling), map::Oversampling::getInstance()->getDefault());
}
#endif//JAM_USING_OVERSAMPLING
ValueMap ParameterManager::getDefaultValueMap() const noexcept { return defaultValues; }

ValueMap ParameterManager::getUnitMap() const noexcept { return units; }
juce::String ParameterManager::getVersionString() const noexcept { return ProjectInfo::versionString; }
juce::String ParameterManager::getProductWebsite() const noexcept { return ProjectInfo::productWebsite; }

juce::File ParameterManager::getUserManual() const noexcept
{
    return File::getCompanyCommonApplicationDataDirectory (Id::userManuals).getChildFile (getProductName() + files::manualSuffix);
}

int ParameterManager::getPanelHeight() const noexcept
{
    int fallbackHeight { 24 };
    return getDefaultValueForSettings (Id::toTag (Id::panelHeight), juce::String (fallbackHeight)).getIntValue();
}

/** @cond */
bool ParameterManager::isSettingsOld (juce::File& existingFile, const Document& defaultSettings) noexcept
{
    if (existingFile.existsAsFile())
    {
        if (const auto existing { Xml::parse (existingFile.loadFileAsString()) }; not existing.root->id.isNull())
        {
            const auto versionToCheck { existing.root->contains (Id::version)
                                             ? *existing.root->get<juce::String> (Id::version)
                                             : juce::String() };

            const auto versionToCompare { defaultSettings.root->contains (Id::version)
                                               ? *defaultSettings.root->get<juce::String> (Id::version)
                                               : juce::String() };

            return Format::isVersionOld (versionToCheck, versionToCompare);
        }
    }

    return false;
}
/** @endcond */

juce::File ParameterManager::getUserSettingsDirectory() noexcept
{
    return File::getOrCreateDirectory (File::getUserDirectory(), files::settingsDirectory);
}

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
