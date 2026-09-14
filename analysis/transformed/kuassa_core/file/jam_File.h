/**
 * @file jam_File.h
 * @brief Cross-platform application directory/settings-file resolution helpers.
 */

namespace jam
{
/*____________________________________________________________________________*/
struct File
{
    /**
     * TODO: Doxygen     */
    static juce::File getOrCreate (const juce::File& parent, juce::StringRef filename) noexcept
    {
        if (not parent.isDirectory())
            parent.createDirectory();

        juce::File child { parent.getChildFile (filename) };

        if (not child.existsAsFile())
            child.create();

        return child;
    }

    /**
     * @brief Get or create a directory.
     *
     * This function gets the specified child directory from the parent directory.
     * If the child directory does not exist, it creates it.
     *
     * @param parent The parent directory.
     * @param child The name of the child directory to get or create.
     * @return The child directory.
     */
    static juce::File
    getOrCreateDirectory (const juce::File& parent, juce::StringRef child) noexcept
    {
        /** cannot create or get child without name */
        assert (not child.isEmpty());

        auto directory { parent.getChildFile (child) };

        if (not directory.isDirectory())
            directory.createDirectory();

        return directory;
    }

    /**
     * @brief Get the index of a child file in the parent directory.
     *
     * This function searches for a child file with the specified name in the parent directory
     * and returns its index. The search can be recursive and can use a wildcard pattern.
     *
     * @param parent The parent directory.
     * @param childName The name of the child file to search for.
     * @param whatToLook The type of files to look for (e.g., files, directories).
     * @param searchRecursively Whether to search recursively in subdirectories.
     * @param wildCardPattern The wildcard pattern to use for the search.
     * @return The index of the child file if found, or -1 if not found.
     */
    static int getIndex (const juce::File& parent,
                         const juce::String& childName,
                         int whatToLook,
                         bool searchRecursively = true,
                         const juce::String& wildCardPattern = "*") noexcept
    {
        auto childFiles = parent.findChildFiles (
            whatToLook | juce::File::ignoreHiddenFiles, searchRecursively, wildCardPattern);

        childFiles.sort();

        for (int index { 0 }; index < childFiles.size(); ++index)
        {
            if (childFiles[index].getFileName().compare (childName) == 0)
                return index;
        }

        return -1;
    }

    /**
     * @brief Get the [Downloads] directory.
     *
     * This function retrieves the path to the user's Downloads directory.
     *
     * @return The path to the user's Downloads directory.
     */
    static juce::File getDownloadsDirectory() noexcept
    {
        return juce::File::getSpecialLocation (juce::File::SpecialLocationType::userHomeDirectory)
            .getChildFile (Id::downloads);
    }

    /**
     * @brief Get the [Desktop] directory.
     *
     * This function retrieves the path to the user's Desktop directory.
     *
     * @return The path to the user's Desktop directory.
     */
    static juce::File getDesktopDirectory() noexcept
    {
        return juce::File::getSpecialLocation (
            juce::File::SpecialLocationType::userDesktopDirectory);
    }

    /**
     * @brief Get the user application data directory.
     *
     * This function retrieves the path to the user application data directory.
     * On Windows, this is the AppData directory. On macOS, this is the Application Support directory.
     * If a product or company name is provided, it gets or creates a subdirectory with that name.
     *
     * @param productOrCompanyName The name of the product or company to get or create a subdirectory for.
     * @return The path to the user application data directory or the specified subdirectory.
     */
    static juce::File getUserApplicationDataDirectory (
        const juce::String& productOrCompanyName = juce::String()) noexcept
    {
        const auto& userApp = []
        {
#if JUCE_WINDOWS /** appData on Windows */
            return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);

#elif JUCE_MAC /** ~Users/username/Library/Application Support on macOS */
            return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                .getChildFile (Id::applicationSupport);
#endif
        }();

        if (productOrCompanyName.isEmpty())
            return userApp;

        return File::getOrCreateDirectory (userApp, productOrCompanyName);
    }

    /**
     * @brief Get the common application data directory.
     *
     * This function retrieves the path to the common application data directory.
     * On Windows, this is the AppData directory. On macOS, this is the Application Support directory.
     * If a product or company name is provided, it gets or creates a subdirectory with that name.
     *
     * @param productOrCompanyName The name of the product or company to get or create a subdirectory for.
     * @return The path to the common application data directory or the specified subdirectory.
     */
    static juce::File getCommonApplicationDataDirectory (
        const juce::String& productOrCompanyName = juce::String()) noexcept
    {
        const auto& userApp = []
        {
#if JUCE_WINDOWS /** appData on Windows */
            return juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory);

#elif JUCE_MAC /** ~Library/Application Support on macOS */
            return juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory)
                .getChildFile (Id::applicationSupport);
#endif
        }();

        if (productOrCompanyName.isEmpty())
            return userApp;

        return File::getOrCreateDirectory (userApp, productOrCompanyName);
    }

    /**
     * @brief Get the company directory inside the common application data directory.
     *
     * This function retrieves the path to the company directory inside the common application data directory.
     * On Windows, this is the common AppData directory. On macOS, this is the Application Support directory.
     * If a child directory is provided, it gets or creates a subdirectory with that name.
     *
     * @param childDirectory The name of the child directory to get or create.
     * @return The path to the company directory or the specified subdirectory.
     */
    static juce::File
    getCompanyCommonApplicationDataDirectory (const juce::String& childDirectory = juce::String())
    {
        if (childDirectory.isEmpty())
            return File::getCommonApplicationDataDirectory (ProjectInfo::companyName);

        return File::getOrCreateDirectory (
            File::getCommonApplicationDataDirectory (ProjectInfo::companyName), childDirectory);
    }

    static juce::File
    getCompanyCommonApplicationDataDirectory (const juce::Identifier& childDirectory)
    {
        return getCompanyCommonApplicationDataDirectory (childDirectory.toString());
    }

    /**
     * @brief Get the company directory inside the user's Documents or Music directory.
     *
     * This function retrieves the path to the company directory inside the user's Documents directory on Windows
     * or the user's Music directory on macOS.
     *
     * @return The path to the company directory.
     */
    static juce::File getUserDirectory() noexcept
    {
#if JUCE_MAC
        return File::getOrCreateDirectory (
            juce::File::getSpecialLocation (juce::File::userMusicDirectory), ProjectInfo::companyName);
#elif JUCE_WINDOWS
        return File::getOrCreateDirectory (
            juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), ProjectInfo::companyName);
#endif
    }

    /**
     * @brief Get the user application settings file.
     *
     * This function retrieves the path to the user application settings file. If the file does not exist,
     * it creates a new file with the provided default initial settings.
     *
     * @param defaultInitSettings The default initial settings to use if the file does not exist.
     * @param productOrCompanyName The name of the product or company for which to get the settings file.
     * @param settingsExtension The extension for the settings file.
     * @return The path to the user application settings file.
     */
    static juce::File
    getUserApplicationSettings (const juce::ValueTree& defaultInitSettings = juce::ValueTree(),
                                const juce::String& productOrCompanyName = ProjectInfo::projectName,
                                juce::StringRef settingsExtension = Id::settings) noexcept
    {
        auto file { File::getUserApplicationDataDirectory (productOrCompanyName)
                        .getChildFile (
                            Format::toFileName (productOrCompanyName, settingsExtension)) };

        if (not file.existsAsFile())
        {
            assert (defaultInitSettings.isValid());

            if (defaultInitSettings.isValid())
                if (auto xml { juce::parseXML (defaultInitSettings.toXmlString()) })
                    xml->writeTo (file);
        }

        return file;
    }

    /**
     * @brief Get the debug log file.
     *
     * This function retrieves the path to the project's debug log file on the user's Desktop.
     *
     * @return The path to the debug log file.
     */
    static juce::File getDebugLog() noexcept
    {
        return juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
            .getChildFile (Format::toFileName (ProjectInfo::projectName, Id::ode));
    }

    /**
     * @brief Get the common application settings file.
     *
     * This function retrieves the path to the common application settings file.
     * If the file does not exist, it creates a new file with the provided default initial settings.
     *
     * @param defaultInitSettings The default initial settings to use if the file does not exist.
     * @return The path to the common application settings file.
     */
    static juce::File
    getCommonApplicationSettings (const juce::ValueTree& defaultInitSettings) noexcept
    {
        auto file { File::getCommonApplicationDataDirectory (ProjectInfo::projectName)
                        .getChildFile (Format::toFileName (
                            ProjectInfo::projectName, Id::toTag (Id::settings).toLowerCase())) };

        if (not file.existsAsFile())
        {
            assert (defaultInitSettings.isValid());

            if (defaultInitSettings.isValid())
                if (auto xml { juce::parseXML (defaultInitSettings.toXmlString()) })
                    xml->writeTo (file);
        }

        return file;
    }

    /**
     * @brief Get the application settings file at an explicit path.
     *
     * Retrieves (creating if absent, seeded from @p defaultInitSettings) the
     * settings file at @p settingsPath, with the given extension applied.
     *
     * @param settingsPath The directory in which the settings file lives.
     * @param defaultInitSettings The default initial settings to use if the file does not exist.
     * @param settingsExtension The extension for the settings file.
     * @return The path to the application settings file.
     */
    static juce::File getApplicationSettings (
        const juce::File& settingsPath,
        const juce::ValueTree& defaultInitSettings = juce::ValueTree(),
        juce::StringRef settingsExtension = Id::toTag (Id::settings).toLowerCase()) noexcept
    {
        auto file { settingsPath.getChildFile (
            Format::toFileName (ProjectInfo::projectName, settingsExtension)) };

        if (not file.existsAsFile())
        {
            assert (defaultInitSettings.isValid());

            if (defaultInitSettings.isValid())
                if (auto xml { juce::parseXML (defaultInitSettings.toXmlString()) })
                    xml->writeTo (file);
        }

        return file;
    }

    /**
     * @brief Copy missing files from the default directory to the user directory.
     *
     * This function copies files and directories from the default directory to the user directory
     * if they are missing. It can also replace files with newer versions based on the provided version.
     *
     * @param defaultDir The default directory to copy files from.
     * @param userDir The user directory to copy files to.
     * @param currentVersion The current version to compare against for replacing files.
     * @param shouldReplaceWithNewerVersion Whether to replace files with newer versions.
     */
    static void copyMissingFiles (const juce::File& defaultDir,
                                  const juce::File& userDir,
                                  const juce::String& currentVersion,
                                  bool shouldReplaceWithNewerVersion = true)
    {
        jassert (defaultDir.isDirectory() and userDir.isDirectory());

        for (const juce::File& defaultFile :
             defaultDir.findChildFiles (juce::File::findFilesAndDirectories, false, "*"))
        {
            auto userFile { userDir.getChildFile (defaultFile.getFileName()) };

            if (defaultFile.isDirectory())
            {
                if (not userFile.exists())
                {
                    userFile.createDirectory();
                }

                copyMissingFiles (
                    defaultFile, userFile, currentVersion, shouldReplaceWithNewerVersion);
            }
            else
            {
                bool shouldCopy { not userFile.exists() };

                if (shouldReplaceWithNewerVersion)
                {
                    const juce::String defaultVersion {
                        defaultDir.getFileName().fromLastOccurrenceOf ("Ver.", false, true)
                    };
                    shouldCopy =
                        shouldCopy or Format::isVersionOld (currentVersion, defaultVersion);
                }

                if (shouldCopy)
                {
                    defaultFile.copyFileTo (userFile);
                }
            }
        }
    }

    /**
     * @brief Clear the recent files list.
     *
     * This function clears the recent files list by deleting all child elements with the tag name "recent"
     * from the user application data settings file.
     */
    static void clearRecentFilesList()
    {
        if (auto settings { File::getUserApplicationDataDirectory() }; settings.existsAsFile())
        {
            if (auto xml { juce::parseXML (settings) })
            {
                xml->deleteAllChildElementsWithTagName (Id::toTag (Id::recent));

                xml->writeTo (settings);
            }
        }
    }

    //==============================================================================
    /** Native filesystem watcher — full definition in jam_file_watcher.h */
    struct Watcher;
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
