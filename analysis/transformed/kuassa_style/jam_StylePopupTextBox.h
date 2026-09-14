/**
 * @file jam_StylePopupTextBox.h
 * @brief LookAndFeel for PopupTextBox — Label-only rounded numeric field with
 *        an optional unit-suffix chip.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief LookAndFeel for PopupTextBox — suppresses the slider track/thumb and
 *        renders only its editable Label, as a rounded numeric field with an
 *        optional unit-suffix chip.
 */
class StylePopupTextBox : public juce::LookAndFeel_V4
{
public:
    StylePopupTextBox()=default;
    ~StylePopupTextBox()=default;
    //==============================================================================
    /** @brief Sets the number of decimal places shown when the label is not being edited.
     *  @param newValue The number of decimal places to display.
     */
    void setDecimalPlaces (int newValue)
    {
        numOfDecimalPlaces = newValue;
    }
    //==============================================================================
    /** @brief Returns a layout whose text box occupies the slider's full local bounds.
     *  @param slider The slider being laid out.
     */
    juce::Slider::SliderLayout getSliderLayout (juce::Slider& slider) override
    {
        auto area { slider.getLocalBounds() };

        juce::Slider::SliderLayout layout;

        layout.textBoxBounds = area;

        return layout;
    }

    /** @brief Receives fonts pushed from StyleManager via Registry setFont dispatch; stores matching alias on instance for thread-agnostic paint. */
    void setFont (juce::StringRef alias, const juce::FontOptions& font)
    {
        juce::String aliasName { alias };

        if (aliasName.compare (Id::bold.toString()) == 0)
            boldFont = font;
    }

    /** @brief Returns the bold font scaled to fit the label's inset height.
     *  @param label The label to derive the font size from.
     */
    juce::Font getLabelFont (juce::Label& label) override
    {
        auto area { label.getLocalBounds().reduced (cornerSize) };
        juce::FontOptions font { boldFont };
        font = font.withHeight (0.6f * area.getHeight());
        font = font.withKerningFactor (0.01f);

        return font;
    }

    /**
     * @brief No-op — PopupTextBox renders only its Label, never the slider track/thumb.
     * @param g          Unused.
     * @param x          Unused.
     * @param y          Unused.
     * @param width      Unused.
     * @param height     Unused.
     * @param sliderPos  Unused.
     * @param minSliderPos Unused.
     * @param maxSliderPos Unused.
     * @param style      Unused.
     * @param slider     Unused.
     */
    void drawLinearSlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPos,
                           float minSliderPos,
                           float maxSliderPos,
                           const juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        /** don't draw the slider */
    }

    /**
     * @brief Draws the rounded-rect background, optional unit-suffix chip, and
     *        the current value (or the live text editor, while being edited).
     * @param g     The graphics context to paint into.
     * @param label The label being drawn — its parent Slider supplies the unit suffix.
     */
    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        auto area { label.getLocalBounds().reduced (cornerSize) };

        /** Rounded rectangle clipping mask */
        juce::Path clip;

        clip.addRoundedRectangle (area, cornerSize);
        g.reduceClipRegion (clip);

        g.setColour (findColour (juce::Label::backgroundColourId));
        g.fillRect (area);

        auto colour { findColour (juce::Label::textColourId) };
        auto font { getLabelFont (label) };
        g.setFont (font);

        if (juce::String suffix { getTextValueSuffix (label) };
            suffix.isNotEmpty())
        {
            int width { jam::toInt (juce::TextLayout::getStringWidth (font, suffix)) };
            int horizontalPad { 2 * (area.getHeight() - jam::toInt (font.getHeight())) };

            auto unitArea { area.removeFromRight (width + horizontalPad) };

            g.setColour (colour);
            g.fillRect (unitArea);

            g.setColour (colour.contrasting (1.0f));
            g.drawText (suffix, unitArea, juce::Justification::centred);
        }

        if (label.isBeingEdited())
        {
            if (auto textEditor { label.getCurrentTextEditor() })
                textEditor->setBounds (area);
        }
        else
        {
            g.setColour (colour);
            g.drawText ({ label.getText().getDoubleValue(), numOfDecimalPlaces }, area, juce::Justification::centred);
        }
    }

    //==============================================================================
private:
    int numOfDecimalPlaces { 2 };
    static const juce::String getTextValueSuffix (const juce::Label& label)
    {
        if (auto* slider { dynamic_cast<juce::Slider*> (label.getParentComponent()) })
            return slider->getTextValueSuffix().trim();

        return {};
    }

    float cornerSize { 4.0f };
    juce::FontOptions boldFont;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StylePopupTextBox)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
