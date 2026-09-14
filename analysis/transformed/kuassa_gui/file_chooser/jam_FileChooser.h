/**
 * @file jam_FileChooser.h
 * @brief Open/save file dialogs wrapping `juce::FileChooser`, remembering the last used directory.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class FileChooser
 * @brief Wraps `juce::FileChooser` open/save dialogs, sharing one process-wide
 * last-used-directory across all instances.
 */
class FileChooser
{
public:
    /**
     * @brief Constructs, associating the chooser with a parent component.
     * @param associatedComponent  Component the dialog is associated with. Must outlive this object.
     */
    FileChooser (juce::Component* associatedComponent)
        : parent (*associatedComponent) {}

    /**
     * @brief Opens an async file-open dialog.
     * @param file            Receives the chosen file on success.
     * @param fileExtension   File extension filter.
     * @param callback        Called with @p file on success.
     * @param dialogBoxTitle  Dialog window title.
     * @param defaultFlags    Additional `juce::FileBrowserComponent` flags, ORed with `openMode`.
     */
    void open (juce::File& file,
               const juce::String& fileExtension,
               std::function<void (const juce::File&)> callback,
               const juce::String& dialogBoxTitle = "Open",
               int defaultFlags = juce::FileBrowserComponent::canSelectFiles)
    {
        auto startDir { getLastUsedPath().exists() ? getLastUsedPath()
                                                    : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory) };
        auto flags { juce::FileBrowserComponent::openMode | defaultFlags };

        fileChooser = std::make_unique<juce::FileChooser> (dialogBoxTitle, startDir, fileExtension);

        fileChooser->launchAsync (flags, [this, callback, &file] (const juce::FileChooser& c)
                                  {
                                      auto result { c.getResult() };

                                      if (result.exists())
                                      {
                                          file = result;
                                          setLastUsedPath (file.getParentDirectory());
                                          callback (file);
                                      }
                                  });
    }

    /**
     * @brief Opens an async file-save dialog, receiving the raw `juce::FileChooser` result.
     * @param dialogBoxTitle          Dialog window title.
     * @param initialFileOrDirectory  Initial file or directory shown by the dialog.
     * @param fileExtension           File extension filter.
     * @param callback                Called with the underlying `juce::FileChooser` on completion.
     * @param defaultFlags            Additional `juce::FileBrowserComponent` flags, ORed with `saveMode`.
     */
    void save (const juce::String& dialogBoxTitle,
               const juce::File& initialFileOrDirectory,
               const juce::String& fileExtension,
               std::function<void (const juce::FileChooser&)> callback,
               int defaultFlags = juce::FileBrowserComponent::canSelectFiles)
    {
        auto flags { juce::FileBrowserComponent::saveMode | defaultFlags };

        fileChooser = std::make_unique<juce::FileChooser> (dialogBoxTitle, initialFileOrDirectory, fileExtension);

        fileChooser->launchAsync (flags, [this, callback] (const juce::FileChooser& c)
                                  {
                                      if (c.getResult().getFileName().isNotEmpty())
                                          setLastUsedPath (c.getResult().getParentDirectory());

                                      callback (c);
                                  });
    }

    /**
     * @brief Opens an async file-save dialog, receiving the chosen file directly.
     * @param file            Starting file/directory; receives the chosen file on success.
     * @param fileExtension   File extension filter.
     * @param callback        Called with @p file on success.
     * @param dialogBoxTitle  Dialog window title.
     * @param defaultFlags    Additional `juce::FileBrowserComponent` flags, ORed with `saveMode`.
     */
    void save (juce::File& file,
               const juce::String& fileExtension,
               std::function<void (const juce::File&)> callback,
               const juce::String& dialogBoxTitle = "Save",
               int defaultFlags = juce::FileBrowserComponent::canSelectFiles)
    {
        auto flags { juce::FileBrowserComponent::saveMode | defaultFlags };

        auto startDir { file.exists() ? file
                                       : (getLastUsedPath().exists() ? getLastUsedPath()
                                                                      : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)) };

        fileChooser = std::make_unique<juce::FileChooser> (dialogBoxTitle, startDir, fileExtension);

        fileChooser->launchAsync (flags, [this, callback, &file] (const juce::FileChooser& c)
                                  {
                                      auto result { c.getResult() };

                                      if (result.getFileName().isNotEmpty())
                                      {
                                          file = result;
                                          setLastUsedPath (file.getParentDirectory());
                                          callback (file);
                                      }
                                  });
    }

    /**
     * @brief Opens an async "Save preset as" dialog rooted at the parameter manager's user presets directory.
     * @tparam ParameterManagerType  Type providing `getUserPresetsDirectory()` and `getPresetExtension()`.
     * @param param     Parameter manager supplying the presets directory and extension.
     * @param callback  Called on success.
     */
    template <typename ParameterManagerType>
    void savePresetAs (const ParameterManagerType& param, std::function<void()> callback)
    {
        auto flags { juce::FileBrowserComponent::saveMode };

        fileChooser = std::make_unique<juce::FileChooser> ("Save preset as",
                                                            param.getUserPresetsDirectory(),
                                                            param.getPresetExtension());

        fileChooser->launchAsync (flags, [this, callback] (const juce::FileChooser& c)
                                  {
                                      if (c.getResult().getFileName().isNotEmpty())
                                      {
                                          setLastUsedPath (c.getResult().getParentDirectory());
                                          callback();
                                      }
                                  });
    }

#if JAM_USING_AQUATIC_PRIME
    /**
     * @brief Opens a file-open dialog to import an AquaticPrime license file into @p model.
     * @tparam ModelType  Model type accepting the imported license.
     * @param model  Model receiving the imported license.
     */
    template <typename ModelType>
    void copyLicense (ModelType& model);
#endif// JAM_USING_AQUATIC_PRIME

    /** @return The last completed dialog's chosen file, or an invalid `juce::File` when none completed. */
    juce::File getResult() const noexcept
    {
        return fileChooser ? fileChooser->getResult() : juce::File();
    }

    /** @return `true` when the last dialog completed without producing a valid file. */
    bool cancelled() const noexcept
    {
        return fileChooser and not fileChooser->getResult().exists();
    }

    /** @return Reference to the owned `juce::FileChooser`. Asserts one has been created. */
    juce::FileChooser& get() const noexcept
    {
        jassert (fileChooser != nullptr);
        return *fileChooser;
    }

private:
    juce::Component& parent;                          ///< Component the dialog is associated with.
    std::unique_ptr<juce::FileChooser> fileChooser;   ///< Currently active (or last completed) dialog.

    /** @return Reference to the process-wide last-used directory, shared across all instances. */
    static juce::File& getLastUsedPath()
    {
        static juce::File lastPath;
        return lastPath;
    }

    /** @brief Updates the process-wide last-used directory if @p dir is a directory. */
    static void setLastUsedPath (const juce::File& dir)
    {
        if (dir.isDirectory())
            getLastUsedPath() = dir;
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileChooser)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
