/**
 * @file jam_MessageBox.h
 * @brief Preset library of async modal alert/confirm dialogs for common plugin situations.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct MessageBox
 * @brief Static namespace-like collection of async alert/confirm dialog presets,
 * each wrapping a `juce::AlertWindow` show call with fixed title/message wording.
 */
struct MessageBox
{
    /*____________________________________________________________________________*/

    /**
     * @brief Shows a generic alert dialog.
     * @param title                Dialog title.
     * @param message              Dialog message body.
     * @param associatedComponent  Component the dialog is associated with.
     * @param callback             Called with the dialog result on close.
     * @param iconType             Icon shown in the dialog.
     */
    static void showAlert (const juce::String& title,
                           const juce::String& message,
                           juce::Component* associatedComponent = nullptr,
                           std::function<void (int)> callback = nullptr,
                           juce::MessageBoxIconType iconType = juce::AlertWindow::WarningIcon);

    /**
     * @brief Shows a Yes/No confirmation dialog.
     * @param title                Dialog title.
     * @param message              Dialog message body.
     * @param associatedComponent  Component the dialog is associated with.
     * @param callback             Called with the dialog result (nonzero = Yes) on close.
     */
    static void showYesNoBox (const juce::String& title,
                              const juce::String& message,
                              juce::Component* associatedComponent = nullptr,
                              std::function<void (int)> callback = nullptr);

    /**
     * @brief Warns that a preset was saved by a newer plugin version.
     * @param presetFileName       Name of the affected preset file.
     * @param associatedComponent  Component the dialog is associated with.
     * @param callback             Called with the dialog result on close.
     */
    static void newerVersionPreset (const juce::String& presetFileName,
                                    juce::Component* associatedComponent = nullptr,
                                    std::function<void (int)> callback = nullptr);

    /**
     * @brief Asks whether to replace an existing preset.
     * @param presetFileName       Name of the preset that would be replaced.
     * @param associatedComponent  Component the dialog is associated with.
     * @param callback             Called with the dialog result (nonzero = confirmed) on close.
     */
    static void askReplacePreset (juce::StringRef presetFileName,
                                  juce::Component* associatedComponent = nullptr,
                                  std::function<void (int)> callback = nullptr);

    /**
     * @brief Asks whether to replace an existing file.
     * @param fileName             Name of the file that would be replaced.
     * @param associatedComponent  Component the dialog is associated with.
     * @param callback             Called with the dialog result (nonzero = confirmed) on close.
     */
    static void askReplaceFile (juce::StringRef fileName,
                                juce::Component* associatedComponent = nullptr,
                                std::function<void (int)> callback = nullptr);

    /**
     * @brief Confirms a preset was saved successfully.
     * @param presetFileName       Name of the saved preset file.
     * @param associatedComponent  Component the dialog is associated with.
     */
    static void presetSaved (const juce::String& presetFileName, juce::Component* associatedComponent = nullptr);

    /**
     * @brief Confirms a file was saved successfully.
     * @param file                 The saved file.
     * @param associatedComponent  Component the dialog is associated with.
     */
    static void fileSaved (const juce::File& file,
                           juce::Component* associatedComponent = nullptr);

    /**
     * @brief Warns that the manual document could not be found.
     * @param associatedComponent  Component the dialog is associated with.
     */
    static void manualNotFound (juce::Component* associatedComponent = nullptr);

    /**
     * @brief Warns that an impulse response file's path exceeds the supported length.
     * @param associatedComponent  Component the dialog is associated with.
     */
    static void impulsePathTooLong (juce::Component* associatedComponent = nullptr);

    /**
     * @brief Warns that an impulse response file was not recognised.
     * @param associatedComponent  Component the dialog is associated with.
     */
    static void impulseNotRecognised (juce::Component* associatedComponent = nullptr);

    /**
     * @brief Shows a generic failure alert.
     * @param message              Dialog message body.
     * @param associatedComponent  Component the dialog is associated with.
     */
    static void noCigar (const juce::String& message, juce::Component* associatedComponent = nullptr);

    /**
     * @brief Shows a generic retry alert.
     * @param message              Dialog message body.
     * @param associatedComponent  Component the dialog is associated with.
     */
    static void tryAgain (const juce::String& message, juce::Component* associatedComponent = nullptr);

    /**
     * @brief Asks whether to replace an already-existing file, with an explicit filename.
     * @param filename             Name of the file that would be replaced.
     * @param callback             Called with the dialog result (nonzero = confirmed) on close.
     * @param associatedComponent  Component the dialog is associated with.
     */
    static void fileAlreadyExists (juce::StringRef filename, std::function<void (int)> callback, juce::Component* associatedComponent = nullptr);

#if JAM_USING_AQUATIC_PRIME
    /**
     * @brief Confirms an AquaticPrime license authorized successfully.
     * @tparam ParameterManagerType  Parameter manager type owning license state.
     * @param param                Parameter manager owning license state.
     * @param associatedComponent  Component the dialog is associated with.
     */
    template <typename ParameterManagerType>
    static void authorizationSuccess (const ParameterManagerType& param, juce::Component* associatedComponent = nullptr);

    /**
     * @brief Warns that AquaticPrime license authorization failed.
     * @tparam ParameterManagerType  Parameter manager type owning license state.
     * @param param                Parameter manager owning license state.
     * @param associatedComponent  Component the dialog is associated with.
     */
    template <typename ParameterManagerType>
    static void authorizationFail (const ParameterManagerType& param, juce::Component* associatedComponent = nullptr);

    /**
     * @brief Warns that no AquaticPrime license file was found, offering to locate one.
     * @tparam ParameterManagerType  Parameter manager type owning license state.
     * @param param                Parameter manager owning license state.
     * @param associatedComponent  Component the dialog is associated with.
     * @param callback             Called with the dialog result on close.
     */
    template <typename ParameterManagerType>
    static void licenseNotFound (const ParameterManagerType& param,
                                 juce::Component* associatedComponent,
                                 std::function<void (int)> callback);
#endif// JAM_USING_AQUATIC_PRIME
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
