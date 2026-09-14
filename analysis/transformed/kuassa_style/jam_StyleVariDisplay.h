/**
 * @file jam_StyleVariDisplay.h
 * @brief LookAndFeel for a seven-segment-style variable display slider.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class StyleVariDisplay
 * @brief LookAndFeel for a seven-segment-style variable display slider — draws
 * a frame-strip enabled/disabled background and a padded numeric or note-name
 * label sized to fill the available space.
 */
class StyleVariDisplay : public juce::LookAndFeel_V4
{
public:
    // Constructor/Destructor
    StyleVariDisplay() = default;
    ~StyleVariDisplay() = default;

    //==============================================================================
    /**
     * @brief Sets the source frame strip image (enabled/disabled frames).
     * @param newImage  Frame strip image.
     */
    void setImage (const juce::Image& newImage);

    /**
     * @brief Sets the value display mode, and the digit length and decimal
     * places that go with it.
     * @param mode  One of `map::VariDisplayMode::value` (hz, khz, note).
     */
    void setDisplayMode (int mode);

    /** @brief Receives fonts pushed from StyleManager via Registry setFont dispatch; stores matching alias on instance for thread-agnostic paint. */
    void setFont (juce::StringRef alias, const juce::FontOptions& font)
    {
        juce::String aliasName { alias };

        if (aliasName.compare (Id::bold.toString()) == 0)
            boldFont = font;
        else if (aliasName.compare (Id::sevenSegment.toString()) == 0)
            segmentFont = font;
    }

    // Look and feel overrides for vari display components
    /** Draws the frame strip's enabled/disabled background frame. */
    void drawLinearSlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPos,
                           float minSliderPos,
                           float maxSliderPos,
                           const juce::Slider::SliderStyle style,
                           juce::Slider& slider) override;

    /** Returns the seven-segment font, scaled to the label's height. */
    juce::Font getLabelFont (juce::Label& label) override;

    /** Returns the bold font, scaled to 60% of the button's height. */
    juce::Font getTextButtonFont (juce::TextButton&, int newButtonHeight) override;

    /** Creates the right-justified, keyboard-editable SliderLabelComp used as the slider's text box. */
    juce::Label* createSliderTextBox (juce::Slider& slider) override;

    /** Draws the current value or note name, sized and padded per the display mode, when not being edited. */
    void drawLabel (juce::Graphics& g, juce::Label& label) override;

    /** Draws right-justified button text in the bold font, brightened when highlighted. */
    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    /** Returns a layout with the text box inset from the bottom of the slider bounds. */
    juce::Slider::SliderLayout getSliderLayout (juce::Slider& slider) override;

    /** No-op — StyleVariDisplay draws no button background. */
    void drawButtonBackground (juce::Graphics&,
                               juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

private:
    juce::Image image;
    juce::FontOptions boldFont;
    juce::FontOptions segmentFont;
    int digitLength { 5 };
    int decimalPlaces { 0 };
    int buttonHeight { 30 };
    float cornerSize { 10.0f };
    float dimFactor { 0.25f };
    int edgeIndent { 4 };

    int displayMode { map::VariDisplayMode::value::hz };

    /** whitespace */
    inline static juce::juce_wchar padCharacter { *juce::CharPointer_UTF8 ("\x20") };

    //==============================================================================
    /** @brief Right-justified, keyboard-editable slider text box with an ignored accessibility handler. */
    class SliderLabelComp : public juce::Label
    {
    public:
        SliderLabelComp()
            : juce::Label ({}, {}) {}

        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override {}

        std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
        {
            return createIgnoredAccessibilityHandler (*this);
        }
    };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleVariDisplay)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
