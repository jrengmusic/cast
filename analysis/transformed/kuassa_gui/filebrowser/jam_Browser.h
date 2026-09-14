/**
 * @file jam_Browser.h
 * @brief Icon button that opens a native directory chooser.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class Browser
 * @brief Wraps an owned trigger button that opens the native directory chooser
 * via `jam::NativeFileChooser::openDirectory()`.
 */
class Browser : public juce::Component
{
public:
    /** @brief Default constructor. No trigger button until setButton() is called. */
    Browser() = default;
    /** @brief Constructs with a component ID; retained for legacy API compatibility. */
    Browser (juce::StringRef) {}
    /** @brief Constructs with a component ID and an unused legacy parameter. */
    Browser (juce::StringRef, int) {}

    /**
     * @brief Takes ownership of the trigger button.
     * @param newButton  Button to display; becomes a child component.
     */
    void setButton (std::unique_ptr<juce::Button> newButton)
    {
        button = std::move (newButton);
        addAndMakeVisible (button.get());
    }

    /** Stretches the trigger button to fill this component's local bounds. */
    void resized() override
    {
        if (button != nullptr)
            button->setBounds (getLocalBounds());
    }

    /**
     * @brief Opens the native directory chooser.
     * @param startingPath  Initial directory shown by the chooser.
     */
    void openBrowser (const juce::String& startingPath = {})
    {
        NativeFileChooser::openDirectory (getTopLevelComponent(), startingPath, onPathSelected);
    }

    /** Called with the chosen path when the directory chooser completes. */
    std::function<void (const juce::String&)> onPathSelected;

    // legacy stubs - remove after SelectorBrowser/PathBrowser rebuilt
    /** Unused — retained for SelectorBrowser/PathBrowser API compatibility. */
    std::function<void()> onValueChanged;
    /** Unused — retained for SelectorBrowser/PathBrowser API compatibility. */
    std::function<void()> onBrowserCallback;
    /** No-op — retained for SelectorBrowser/PathBrowser API compatibility. */
    void setPath (juce::StringRef) {}
    /** @return An empty string — retained for SelectorBrowser/PathBrowser API compatibility. */
    juce::String getPath() const noexcept { return {}; }
    /** @return An unused, always-empty `juce::Value`. */
    juce::Value& getValueObject() noexcept { return legacyValue; }

private:
    std::unique_ptr<juce::Button> button; ///< Owned trigger button.
    juce::Value legacyValue;              ///< Unused — backs getValueObject() for legacy API compatibility.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Browser)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
