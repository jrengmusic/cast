/**
 * @file jam_SettingsModel.h
 * @brief Standalone ValueTree wrapper persisting plugin-wide user settings,
 *        independent of AudioModel's APVTS state tree.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class SettingsModel
 * @brief Owns a standalone ValueTree used to persist plugin-wide user settings.
 *
 * Independent of AudioModel's APVTS state tree, SettingsModel wraps its own
 * juce::ValueTree so settings (Settings dialog) can be loaded from and saved
 * to a dedicated file, with juce::Value::Listener callbacks notifying of
 * changes.
 */
class SettingsModel : public juce::Value::Listener
{
public:
    /**
     * @brief Constructs a SettingsModel with an explicit root tree identifier.
     * @param newTreeID The Identifier to use as the root ValueTree's type.
     */
    explicit SettingsModel (const juce::Identifier& newTreeID);

    /**
     * @brief Constructs a SettingsModel using the project name as the root tree identifier.
     */
    SettingsModel();

    /**
     * @brief Destructor for the SettingsModel class.
     */
    ~SettingsModel();
    //==============================================================================
    /**
     * @brief Retrieves the underlying settings ValueTree.
     * @return A reference to the settings juce::ValueTree.
     */
    juce::ValueTree& get() const noexcept;

    /**
     * @brief Replaces the current settings state with the provided ValueTree.
     * @param newState The juce::ValueTree whose properties and children replace the current state.
     */
    void replaceState (const juce::ValueTree& newState);

    /**
     * @brief Writes the current settings state to an XML file.
     * @param destinationFile The juce::File to write the settings XML to.
     * @return True if the file was written successfully, false otherwise.
     */
    bool writeToXml (juce::File& destinationFile);
    //==============================================================================
    /**
     * @brief Callback invoked whenever a listened-to juce::Value changes.
     *
     * Assign a custom lambda or function to react to settings value changes
     * as needed in your application.
     */
    std::function<void()> onValueChanged;

    /**
     * @brief Handles changes to a juce::Value this SettingsModel listens to.
     *
     * Invokes onValueChanged when assigned.
     *
     * @param value The juce::Value that changed.
     */
    void valueChanged (juce::Value& value) override;

private:
    std::unique_ptr<juce::ValueTree> state;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsModel)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
