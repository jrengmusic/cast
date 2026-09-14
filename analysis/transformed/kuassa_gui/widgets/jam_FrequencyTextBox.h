#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class FrequencyTextBox
 * @brief A juce::Slider subclass that understands Hz, kHz, and note-name text entry.
 *
 * FrequencyTextBox overrides juce::Slider's virtual text-conversion methods so
 * that user-typed text is parsed according to three modes driven by
 * map::VariDisplayMode (the framework SSOT — no duplicate Mode struct
 * lives here). When the user types a note name ("A4"), a kHz value ("2k"), or a
 * plain Hz number ("440"), the override detects the format, writes the resolved
 * mode to the AudioModel ValueTree via modeValue, and returns the corresponding Hz double.
 *
 * @par Attachment contract
 * juce::Slider's std::function members (valueFromTextFunction,
 * textFromValueFunction) are left unset in the constructor; SliderAttachment may
 * assign them freely.  Because getValueFromText() and getTextFromValue() are
 * declared virtual, our overrides are called by every internal JUCE call site
 * (label commit, popup display, accessibility) regardless of what
 * SliderAttachment stores in those std::function members — virtual dispatch
 * never reaches the base-class body that would inspect them.
 *
 * @par AudioModel wiring
 * Call bindToModel() during the framework binding pass (View::Manager::bindToModel
 * dispatches this automatically via the HasBindToModel trait).  bindToModel() seeds
 * modeValue with map::VariDisplayMode::getDefault() only when modeValue is empty,
 * then attaches modeValue to the AudioModel ValueTree node for this parameter under
 * Id::mode, mirroring VariComponent's established pattern exactly — no cached
 * pointer or ID.
 */
class FrequencyTextBox : public juce::Slider,
                         private juce::Label::Listener
{
public:
    /** @brief Default constructor. Leaves std::function members unassigned. */
    FrequencyTextBox();

    /** @brief Default destructor. */
    ~FrequencyTextBox() override = default;

    //==============================================================================
    /**
     * @brief Parses user-typed text to a Hz double and writes the resolved mode.
     *
     * Walks textModes — a Function::Map of self-classifying entries keyed by
     * map::VariDisplayMode::value (note, khz, hz) — passing text to each entry
     * in turn. Each entry independently decides whether it recognizes the
     * text (note-name via jam::Format::isValidNoteName, kilo-suffixed via
     * jam::Format::isKilo, or plain numeric as the fallback) and returns
     * an std::optional<double> Hz value when it does. Whichever entry
     * returns a value writes its mode key to modeValue via
     * map::VariDisplayMode::getInstance()->get(mode) — propagating
     * persistently to the AudioModel ValueTree with zero stored pointers or
     * IDs — and that value becomes the result. If no entry recognizes the
     * text, the result falls back to 0.0.
     *
     * Called by juce::Slider for every text-box commit. The virtual override
     * makes this authoritative regardless of valueFromTextFunction.
     *
     * @param text Raw text from the slider text box (suffix already stripped by base).
     * @return Frequency in Hz as a double.
     */
    double getValueFromText (const juce::String& text) override;

    //==============================================================================
    /**
     * @brief Returns the raw numeric string for a Hz value, bypassing any suffix.
     *
     * Overrides the virtual to prevent SliderAttachment's textFromValueFunction
     * (which appends " Hz" from the parameter label) from reaching JUCE's internal
     * call sites.  VariDisplay::drawLabel reads this string and applies mode-aware
     * formatting independently.
     *
     * @param value Frequency in Hz.
     * @return Pure numeric string with no suffix.
     */
    juce::String getTextFromValue (double value) override;

    //==============================================================================
    /**
     * @brief Opens the text editor on click; suppresses drag-start.
     *
     * Does NOT call the base juce::Slider::mouseDown — that would initiate a
     * drag session and allow value changes via mouse position.  showTextBox()
     * is called directly so the user can type a value.
     *
     * @param e Mouse event (position/modifiers forwarded by JUCE).
     */
    void mouseDown (const juce::MouseEvent& e) override;

    /**
     * @brief No-op drag handler — prevents value change via mouse drag.
     *
     * juce::Slider::mouseDrag is NOT called so dragging the component
     * produces no value mutation.
     *
     * @param e Mouse event (ignored).
     */
    void mouseDrag (const juce::MouseEvent& e) override;

    /**
     * @brief No-op wheel handler — prevents value change via scroll wheel.
     *
     * juce::Slider::mouseWheelMove is NOT called so the scroll wheel
     * produces no value mutation.
     *
     * @param e       Mouse event (ignored).
     * @param details Wheel delta details (ignored).
     */
    void mouseWheelMove (const juce::MouseEvent& e,
                         const juce::MouseWheelDetails& details) override;

    //==============================================================================
    /**
     * @brief Re-attaches this listener to the slider's valueBox Label after a LAF change.
     *
     * juce::Slider::lookAndFeelChanged() recreates the valueBox Label via
     * LookAndFeel::createSliderTextBox.  Overriding here allows FrequencyTextBox
     * to call addListener(this) on the new Label immediately after the base
     * recreates it, keeping the Label::Listener registration live across LAF swaps.
     *
     * Calls the base juce::Slider::lookAndFeelChanged() first, then scans child
     * components and attaches this as a listener to every juce::Label* found.
     */
    void lookAndFeelChanged() override;

    //==============================================================================
    /** Repaints when the component's enabled state changes. */
    void enablementChanged() override { repaint(); }

    /**
     * @brief Seeds modeValue with the display-mode default when unset, then binds it to the model.
     *
     * When modeValue is empty, seeds it from map::VariDisplayMode::getDefault();
     * a value already stored on the AudioModel ValueTree node wins once attach()
     * restores it. Attaches modeValue to the parameter's Id::mode property and
     * clears the text-box suffix.
     *
     * @param parameterID The parameter ID whose ValueTree node owns the mode property.
     * @param state       The AudioModel to attach modeValue to.
     */
    void bindToModel (const juce::String& parameterID, AudioModel& state);

private:
    //==============================================================================
    // juce::Label::Listener — private base; these must not be called directly.

    /**
     * @brief Restricts text editor input to frequency-relevant characters.
     *
     * Called by the slider's valueBox Label when its inline text editor is created.
     * Allows digits, decimal point, note letters (A–G, a–g), sharp (#), and kilo
     * suffix (k, K).
     *
     * @param label  The Label that opened the editor (ignored).
     * @param editor The text editor to configure.
     */
    void editorShown (juce::Label* label, juce::TextEditor& editor) override;

    /** @brief No-op — satisfies the Label::Listener pure-virtual contract. */
    void labelTextChanged (juce::Label*) override;

    //==============================================================================
    juce::Value modeValue;
    jam::Function::Map<int, std::optional<double>> textModes;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyTextBox)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
