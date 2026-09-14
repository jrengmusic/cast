/**
 * @file jam_AudioModel.h
 * @brief APVTS-derived plugin state model — UI scale, appearance, oversampling,
 *        preset load/save, and parameter-page tree management.
 */

namespace jam
{
/*____________________________________________________________________________*/
//==============================================================================
/**
 * @class AudioModel
 * @brief Represents an audio model handling user interface and parameter management.
 *        Integrates functionality for value tree properties, dark mode settings,
 *        preset handling, and license evaluation.
 */
class AudioModel : public juce::AudioProcessorValueTreeState
{
public:
    /**
     * @brief Constructor for the AudioModel class.
     * @param parameterManager The parameter manager to utilize.
     * @param processorToConnectTo The audio processor to connect.
     * @param parameterLayout The parameter layout to hand to the APVTS base.
     * @param undoManagerToUse Optional undo manager for state changes.
     */
    AudioModel (jam::ParameterManager& parameterManager,
                juce::AudioProcessor& processorToConnectTo,
                juce::AudioProcessorValueTreeState::ParameterLayout parameterLayout,
                juce::UndoManager* undoManagerToUse = nullptr);

    /**
     * @brief Destructor for the AudioModel class.
     *
     * The destructor removes all parameter listeners to prevent callbacks after
     * objects are destroyed, ensuring safe cleanup of resources.
     */
    ~AudioModel();
    //==============================================================================
    /**
     * @brief Handles changes to properties in the ValueTree.
     *
     * This method is invoked when a property in the ValueTree changes. Depending
     * on the type of the ValueTree, it triggers specific actions such as notifying
     * user tree changes or refreshing and marking preset values as dirty.
     *
     * @param treeWhosePropertyHasChanged The ValueTree whose property has changed.
     * @param property The Identifier of the changed property.
     */
    void
    valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;

    /**
     * @brief Callback function to handle changes in the user ValueTree.
     *
     * This function is invoked when the user-related portion of the ValueTree changes.
     * It can be assigned a custom lambda or function to handle user-specific updates
     * as needed in your application.
     */
    std::function<void()> onUserTreeChanged;

    /**
     * @brief Retrieves the bypass state value from the ValueTree.
     *
     * This method fetches the value associated with the bypass state by searching
     * the child node in the ValueTree with the specified bypass ID and value ID.
     *
     * @return A juce::Value object representing the bypass state.
     */
    juce::Value getBypassStateValue() const noexcept;

    //==============================================================================
    /**
     * @brief Updates the UI scale value.
     *
     * This method sets the UI scale value to a new scale provided as a string.
     *
     * @param newScale The new scale value as a juce::String.
     */
    void setUIScale (const juce::String& newScale);

    /**
     * @brief Sets the desktop scaling factor in the ValueTree.
     *
     * This method updates the desktop scale property within the UIScale child
     * of the ValueTree, if the child node exists and is valid.
     *
     * @param newScale The new desktop scale factor as a float.
     */
    void setDesktopScale (float newScale);

    /**
     * @brief Retrieves the desktop scaling factor from the ValueTree.
     *
     * This method fetches the desktop scale property value from the UIScale child
     * in the ValueTree. If the property is unavailable, it returns a default value
     * of 1.0f.
     *
     * @return The desktop scale factor as a float.
     */
    float getDesktopScale() const noexcept;

    /**
     * @brief Retrieves the UI scale value as an integer.
     *
     * This method fetches the UI scale value and converts it to an integer using
     * the mapping function provided by map::UIScaleMap.
     *
     * @return The UI scale value as an integer.
     */
    int getUIScale() const noexcept;

    /**
     * @brief Retrieves the UI scale value from the ValueTree.
     *
     * This method fetches the value associated with the UI scale by searching
     * the child node in the ValueTree with the specified UIScale ID.
     *
     * @return A juce::Value object representing the UI scale.
     */
    juce::Value getUIScaleValue() const noexcept;

    /**
     * @brief Retrieves the height of the UI panel.
     *
     * This method fetches the UI panel height from the ParameterManager.
     *
     * @return The UI panel height as an integer.
     */
    int getUIPanelHeight() const noexcept;

    //==============================================================================
    /**
     * @brief Retrieves the appearance as a juce::String.
     *
     * This method fetches the value associated with the appearance ID from the
     * ValueTree and converts it to a string.
     *
     * @return A juce::String representing the appearance.
     */
    juce::String getAppearance() const noexcept;

    /**
     * @brief Retrieves the appearance value from the ValueTree.
     *
     * This method fetches the value associated with the appearance ID from the
     * ValueTree.
     *
     * @return A juce::Value object representing the appearance.
     */
    juce::Value getAppearanceValue() const noexcept;

    /**
     * @brief Sets the appearance value in the ValueTree.
     *
     * This method updates the value associated with the appearance ID by
     * setting the appearance ValueTree's value property.
     *
     * @param appearance The new appearance value as a juce::String.
     */
    void setAppearance (const juce::String& appearance);

    /**
     * @brief Retrieves the orientation value from the ValueTree.
     *
     * This method fetches the value associated with the orientation ID from the
     * ValueTree.
     *
     * @return A juce::Value object representing the orientation.
     */
    juce::Value getOrientationValue() const noexcept;

    /**
     * @brief Retrieves the orientation as a juce::String.
     *
     * This method fetches the orientation value from the ValueTree and converts it
     * to a string.
     *
     * @return A juce::String representing the orientation.
     */
    juce::String getOrientation() const noexcept;
    //==============================================================================
#if JAM_USING_OVERSAMPLING
    /**
     * @brief Retrieves the oversampling value from the ValueTree.
     *
     * This method fetches the value associated with the oversampling ID from the
     * ValueTree.
     *
     * @return A juce::Value object representing the oversampling value.
     */
    juce::Value getOversamplingValue() const noexcept;

    /**
     * @brief Calculates the oversampling factor.
     *
     * This method computes the oversampling factor by retrieving the oversampling
     * value and mapping it using Map::Oversampling. If no valid value is found, it
     * uses the default oversampling value provided by the ParameterManager.
     *
     * @return The oversampling factor as an integer.
     */
    int getOversamplingFactor() const noexcept;

    /**
     * @brief Determines if oversampling is active.
     *
     * This method checks whether the oversampling factor is non-zero, indicating
     * that oversampling is enabled.
     *
     * @return True if oversampling is active, false otherwise.
     */
    bool isOversampled() const noexcept;
#endif// JAM_USING_OVERSAMPLING

    //==============================================================================
    /**
     * @brief Sets the state of the audio model using data from the DAW.
     *
     * This method replaces the current state with the provided ValueTree from
     * the DAW, if it is valid.
     *
     * @param stateFromDAW The juce::ValueTree representing the state received from the DAW.
     */
    void setState (const juce::ValueTree& stateFromDAW);

    /**
     * @brief Generates an XML representation of the current preset, excluding certain elements.
     *
     * This method creates an XML representation of the state ValueTree while
     * filtering out specific elements such as user information, bypass parameters,
     * A-B trees, settings, and non-automated parameters. The resulting XML is
     * tailored for preset file storage.
     *
     * @return A `std::unique_ptr<juce::XmlElement>` representing the filtered preset,
     *         or nullptr if the XML creation fails.
     */
    std::unique_ptr<juce::XmlElement> getPresetXml() const noexcept;

    /**
     * @brief Creates the default initialization preset file if it does not already exist.
     *
     * This method checks whether the default initialization preset file exists. If not,
     * it generates an XML representation of the current preset, updates the parameter
     * values with their respective default values from the ParameterManager, and writes
     * the resulting XML to the preset file.
     *
     * @return True if the default initialization preset file already exists or was
     *         successfully created, false otherwise.
     */
    bool createDefaultInitPreset() const noexcept;

    /**
     * @brief Retrieves the current preset file.
     *
     * This method fetches the file path of the current preset by accessing the
     * ValueTree and extracting the associated file attribute.
     *
     * @return A juce::File object representing the current preset file.
     */
    juce::File getCurrentPresetFile() const noexcept;

    /**
     * @brief Retrieves the name of the current preset.
     *
     * This method fetches the name of the current preset by accessing the
     * ValueTree and extracting the associated value attribute.
     *
     * @return A juce::String representing the current preset name.
     */
    juce::String getCurrentPresetName() const noexcept;

    /**
     * @brief Constructs the current preset's file name.
     *
     * This method combines the current preset name with the preset extension
     * obtained from the ParameterManager to construct the preset's file name.
     *
     * @return A juce::String representing the current preset file name.
     */
    juce::String getCurrentPresetFileName() const noexcept;

    /**
     * @brief Loads a preset from a file.
     *
     * This method checks if the specified file exists and attempts to parse its XML content.
     * It updates the preset file path and dirty state, loads the preset content, and repopulates
     * the parameter page tree.
     *
     * @param file The juce::File object representing the preset file to load.
     */
    void loadPreset (const juce::File& file);

    /**
     * @brief Loads preset values from an XML element.
     *
     * This method iterates through the child elements of the XML and updates the parameters
     * with their respective values. It ensures smooth transitions by beginning and ending
     * change gestures for each parameter.
     *
     * @param presetXml The juce::XmlElement containing preset data.
     */
    void loadPreset (juce::XmlElement* presetXml);

    /**
     * @brief Saves the current preset to the specified file.
     *
     * This method writes the current preset's XML representation to the provided file.
     *
     * @param file The juce::File object representing the file where the preset will be saved.
     * @return True if the preset was successfully saved, false otherwise.
     */
    bool savePreset (const juce::File& file) const noexcept;

    /**
     * @brief Saves the current preset to its associated file.
     *
     * This method saves the current preset to the file specified by the preset's file path.
     *
     * @return True if the preset was successfully saved, false otherwise.
     */
    bool saveCurrentPreset() const noexcept;

    /**
     * @brief Checks if the current preset is marked as dirty.
     *
     * A preset is considered "dirty" when the user has tweaked parameters on the
     * last loaded preset but has not yet saved those changes. This method retrieves
     * the dirty state of the current preset from the ValueTree to indicate whether
     * the preset needs saving.
     *
     * @return True if the preset is dirty (modified but not saved), false otherwise.
     */
    bool isPresetDirty() const noexcept;

    /**
     * @brief Checks whether license evaluation is currently active.
     */
    bool isEvaluating() const noexcept;
    //==============================================================================
    /**
     * @brief Retrieves the current page value from the ValueTree.
     *
     * This method fetches the value associated with the current page from the
     * ValueTree by accessing the child node with the page ID.
     *
     * @return A juce::Value object representing the current page.
     */
    juce::Value getCurrentPageValue() const noexcept;

    /**
     * @brief Retrieves the preset XML for a specific page.
     *
     * This method locates the child element corresponding to the given page type
     * within the page ValueTree and generates an XML representation of the preset.
     *
     * @param type The juce::Identifier representing the page type to retrieve.
     * @return A `std::unique_ptr<juce::XmlElement>` containing the preset data for the
     *         specified page type, or nullptr if not found.
     */
    std::unique_ptr<juce::XmlElement> getPagePreset (const juce::Identifier& type) const noexcept;

    /**
     * @brief Refreshes the value of a parameter on the current page.
     *
     * This method updates the value of the specified parameter by retrieving it
     * from the current page in the ValueTree. It sets the parameter's value property
     * using its current value after conversion from the normalized 0-to-1 range.
     *
     * @param parameterID The juce::String representing the ID of the parameter to refresh.
     */
    void refreshCurrentPageValue (const juce::String& parameterID);

    /**
     * @brief Copies parameter values to a specified page in the ValueTree.
     *
     * This method iterates through the parameters within the specified page in the
     * ValueTree. For each parameter, it retrieves the parameter ID and updates the
     * parameter's value property using the raw value loaded from the associated parameter.
     *
     * @param pageDestination The juce::String representing the ID of the page
     *        where the parameter values will be copied.
     */
    void copyParameterValuesToPage (const juce::String& pageDestination);
    //==============================================================================
#if JAM_USING_AQUATIC_PRIME
    juce::Value getEvaluationValue() const noexcept;
    void refreshEvaluationStatus (bool shouldAlsoPrompt = false);
    void setEvaluating (bool isEvaluating);
    void setShouldPromptLicense (bool shouldCheck);
    bool shouldPromptLicense() const noexcept;
    juce::String getUserName() const noexcept;
#endif// JAM_USING_AQUATIC_PRIME
    //==============================================================================
    /**
     * @brief Retrieves a description of the current plugin host (DAW).
     *
     * This function returns the host (DAW) name in which the plugin is running.
     *
     * @return juce::String The name of the plugin host.
     */
    juce::String getHostDescription() const noexcept;

    /**
     * @brief Retrieves the plugin format name.
     *
     * This function returns a string representation of the current plugin format,
     * such as "VST3", "AAX", or "AU", based on the processor's wrapper type.
     *
     * @return juce::String The plugin format name.
     */
    juce::String getPluginFormatName() const noexcept;

    juce::AudioProcessor::WrapperType getWrapperType() const noexcept;
    //==============================================================================
    /**
     * @brief Checks whether a parameter ID names a host parameter.
     *
     * @param parameterID The juce::String representing the parameter ID to check.
     * @return True if the ID names a host parameter, false otherwise.
     */
    bool isValidParameterID (const juce::String& parameterID) const noexcept;

    /**
     * @brief Attaches a juce::Value to a parameter's property.
     *
     * Checks whether the ID names a host parameter. If it does, attaches
     * against the state tree. Otherwise, attaches against the non-automated
     * parameter child.
     *
     * @param value The juce::Value to be attached.
     * @param parameterId The juce::Identifier representing the ID of the parameter to attach to.
     * @param propertyId The juce::Identifier representing the property ID to associate with the value.
     */
    void attach (juce::Value& value, const juce::Identifier& parameterId, const juce::Identifier& propertyId);

    /**
     * @brief Retrieves a value associated with a specific parameter and property from the state tree.
     *
     * This method fetches a juce::Value from the state tree based on the provided parameter
     * and property identifiers. It searches within the state tree for a child node with
     * the given parameter ID and returns the associated property value.
     *
     * @param parameterId The juce::Identifier representing the parameter ID to search for.
     * @param propertyId The juce::Identifier representing the property ID to retrieve.
     * @return A juce::Value object associated with the parameter and property, or an invalid value if not found.
     */
    juce::Value getValue (const juce::Identifier& parameterId, const juce::Identifier& propertyId) const noexcept;

    /**
     * @brief Retrieves a preset value from the state tree.
     *
     * This method fetches a preset value from the state tree based on the provided
     * property identifier. It searches within the "preset" child node of the state tree
     * for the specified property.
     *
     * @param propertyId The juce::Identifier representing the property ID to retrieve from the preset.
     * @return A juce::Value object associated with the preset property, or an invalid value if not found.
     */
    juce::Value getPresetValue (const juce::Identifier& propertyId) const noexcept;

    /**
     * @brief Marks a preset as dirty when a parameter value changes.
     *
     * This method compares the current parameter value with its original value in the preset file.
     * If the values differ, the preset is marked as "dirty" indicating that changes have been made
     * that should be saved. This is important for tracking when preset needs to be saved.
     *
     * @param parameterID The juce::String representing the ID of the parameter that changed.
     */
    void markPresetDirty (const juce::String& parameterID);

    /**
     * @brief Adds parameter listeners to the audio processor.
     *
     * This method registers the processor chain as a listener for all automatable parameters.
     * This enables the system to receive callbacks when parameter values change, allowing
     * for real-time updates and processing of parameter changes.
     *
     * @param processorChainAsListener The processor chain to register as a parameter listener.
     */
    void addListener (juce::AudioProcessorValueTreeState::Listener& processorChainAsListener);

    /**
     * @brief Removes parameter listeners from the audio processor.
     *
     * This method unregisters the processor chain as a listener for all automatable parameters.
     * It's typically called during destruction to prevent callbacks after objects are destroyed.
     */
    void removeListener();

    /**
     * @brief Gets or creates a component state in the interface tree.
     *
     * This method retrieves an existing component state from the interface ValueTree,
     * or creates a new one if it doesn't exist. This is used for storing state specific
     * to UI components.
     *
     * @param componentId The juce::String representing the unique ID of the component.
     * @return A juce::ValueTree representing the component state.
     */
    juce::ValueTree getOrCreateComponentState (const juce::String& componentId);

    /**
     * @brief Gets a component property value from the interface tree.
     *
     * This method retrieves a property value for a specific component from the interface
     * ValueTree. It first gets or creates the component state, then returns the value
     * associated with the specified property.
     *
     * @param componentId The juce::String representing the unique ID of the component.
     * @param propertyId The juce::Identifier representing the property to retrieve.
     * @return A juce::Value object representing the component property value.
     */
    juce::Value getComponentPropertyValue (const juce::String& componentId, const juce::Identifier& propertyId);

    /**
     * @brief Callback function executed when the audio model's state is set from DAW.
     *
     * This function is invoked when the setState method is called with a ValueTree
     * from the DAW. It can be assigned a custom lambda or function to handle
     * state-setting events as needed in your application.
     */
    std::function<void()> onSetState;

private:
    jam::ParameterManager& manager;
    juce::AudioProcessorValueTreeState::Listener* listener { nullptr };

    /**
     * @brief Rebuilds the parameter page tree in the ValueTree.
     *
     * Collects every `param` child of the state tree, then builds one page per
     * entry in the `map::ParameterPage` bimap by copying the collected parameters
     * under each page. The state's `page` child is fetched or created, its existing
     * children are removed, and the newly built pages are appended in their place.
     * Each page's parameter values are then refreshed from the processor.
     */
    void populateParameterPageTree();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioModel)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
