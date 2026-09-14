/**
 * @file jam_PathBrowser.h
 * @brief Read-only path text box with a directory-browse button and optional reset button.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class PathBrowser
 * @brief Composes a `Browser` trigger, a read-only path text box, and an
 * optional reset-to-default button into a single directory-path field.
 */
class PathBrowser
    : public juce::Component
    , public Model::ValueComponent<PathBrowser>
{
public:
    /** @brief Constructs, wiring the browser trigger to setPath() and the value attachment to the text box. */
    PathBrowser()
        : Model::ValueComponent<PathBrowser> (juce::String())
    {
        addAndMakeVisible (browser);

        browser.onPathSelected = [this] (const juce::String& path)
        {
            setPath (path);
        };

        textBox.setReadOnly (true);
        addAndMakeVisible (textBox);

        onAttachment = [this]()
        {
            textBox.setText (value.getValue().toString(), juce::dontSendNotification);
            textBox.setCaretPosition (0);
        };
    }

    /**
     * @brief Takes ownership of the folder-browse trigger button.
     * @param folderButton  Button that opens the directory chooser at the current path.
     */
    void setButton (std::unique_ptr<juce::Button> folderButton)
    {
        folderButton->onClick = [this]
        {
            browser.openBrowser (getPath());
        };
        browser.setButton (std::move (folderButton));
    }

    /**
     * @brief Takes ownership of the reset-to-default button.
     * @param newResetButton  Button that resets the path to setDefaultPath()'s value.
     */
    void setResetButton (std::unique_ptr<juce::Button> newResetButton)
    {
        resetButton = std::move (newResetButton);
        addAndMakeVisible (resetButton.get());

        resetButton->onClick = [this]
        {
            reset();
        };
    }

    /**
     * @brief Sets the default path used by reset(), and applies it immediately.
     * @param path  Default directory path.
     */
    void setDefaultPath (const juce::String& path)
    {
        defaultPath = path;
        setPath (path);
    }

    /**
     * @brief Sets the current path and updates the text box.
     * @param path  New directory path.
     */
    void setPath (const juce::String& path)
    {
        value.setValue (path);
        textBox.setText (path, juce::dontSendNotification);
        textBox.setCaretPosition (0);
    }

    /** @return The current path. */
    juce::String getPath() const noexcept { return value.getValue().toString(); }

    /** Resets the path to the value set via setDefaultPath(). */
    void reset() { setPath (defaultPath); }

    /** @return Reference to the underlying `juce::Value` holding the path. */
    juce::Value& getValueObject() noexcept override { return value; }

    /** Lays out the browse button, path text box, and (when present) reset button in a row. */
    void resized() override
    {
        auto area { getLocalBounds() };
        const int gap { 4 };

        if (resetButton != nullptr)
        {
            resetButton->setBounds (area.removeFromRight (area.getHeight()));
            area.removeFromRight (gap);
        }

        browser.setBounds (area.removeFromLeft (area.getHeight()));
        area.removeFromLeft (gap);
        textBox.setBounds (area);

        jam::TextEditorUtils::setTextCentered (textBox);
    }

    /** Applies the LAF's popup menu font to the path text box. */
    void lookAndFeelChanged() override
    {
        const auto font { getLookAndFeel().getPopupMenuFont() };
        textBox.setFont (font);
        textBox.applyFontToAllText (font);
    }

    /** Applies the current text colour to the path text box. */
    void colourChanged() override
    {
        textBox.applyColourToAllText (findColour (juce::TextEditor::textColourId));
    }

private:
    Browser browser;                              ///< Directory-browse trigger child component.
    juce::TextEditor textBox;                      ///< Read-only display of the current path.
    juce::Value value;                             ///< Current directory path.
    juce::String defaultPath;                      ///< Path applied by reset().
    std::unique_ptr<juce::Button> resetButton;     ///< Optional reset-to-default trigger.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PathBrowser)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
