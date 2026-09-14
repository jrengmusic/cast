/**
 * @file jam_StyleKnob_V1.h
 * @brief Bevelled circular knob LookAndFeel with an inner-shadowed pointer and optional label.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class StyleKnob_V1
 * @brief LookAndFeel drawing a rotary knob as an extruded, bevelled circular cap with
 * a rounded pointer, and an optional value label above or below the cap.
 *
 * @tparam ManagerType  Style manager type; used to resolve the common font via
 * `StyleTheme` when computing getLabelWidth().
 */
template <typename ManagerType>
class StyleKnob_V1 : public StyleCustom
{
public:
    StyleKnob_V1();
    ~StyleKnob_V1();

    //==============================================================================

    /** Returns a layout reserving textHeight for the label above or below the knob, per textPosition. */
    juce::Slider::SliderLayout getSliderLayout (juce::Slider& slider) override;

    /** Draws the extrusion, cap, pointer, and — when enabled — the value label. */
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override;

    /** No-op — StyleKnob_V1 does not draw the slider's text-box label. */
    void drawLabel (juce::Graphics& g, juce::Label& label) override {}
    //==============================================================================
    /** @brief Position of the value label relative to the knob. */
    enum TextPosition
    {
        top,
        bottom
    };

    /**
     * @brief Sets the value label's position relative to the knob.
     * @param newPosition  One of the TextPosition values.
     */
    void setTextPosition (int newPosition);

    /**
     * @brief Sets the height reserved for the value label.
     * @param newHeight  Label height, in pixels.
     */
    void setTextHeight (int newHeight);

    /**
     * @brief Sets whether the value label is drawn.
     * @param shouldDraw  `true` to draw the label.
     */
    void setShouldDrawKnobLabel (bool shouldDraw);

    /** @return The knob cap's extrusion collar height, in pixels. */
    float getKnobExtrusion() const noexcept override;

    /** Returns the width needed to render labelText in the common theme font, or 0 outside an `AudioProcessorEditor`. */
    float getLabelWidth (juce::Slider&) noexcept override;
    //==============================================================================
private:
    int textHeight { 28 };
    int textPosition { bottom };
    bool shouldFlipToggle { false };
    bool shouldDrawKnobLabel { true };
    juce::Colour labelColour { juce::Colours::magenta };
    juce::String labelText;
    //==============================================================================
    float knob_bevelSize { 5.0f };
    float knob_pointerThickness { 8.0f };
    float knob_pointerRoundness { 4.0f };
    float knob_pointerLength { 0.75f };// proportion
    float knob_angleLength { 140.0f };
    float knob_extrusion { 20.0f };

    const juce::Colour shadowColour { juce::Colours::black };
    const float pointershadowOpacity { 0.5f };
    const int pointerShadowRadius { 4 };
    const juce::Point<int> pointerShadowOffset { 0, 2 };
    jam::InnerShadow pointerShadow { shadowColour.withAlpha (pointershadowOpacity), pointerShadowRadius, pointerShadowOffset };

    /** functions to draw knob */
    /**
     * @brief Square area of the knob cap, centred within the slider bounds and inset by the bevel.
     * @param slider  Slider to compute the cap area for.
     */
    juce::Rectangle<float> getKnobCapArea (juce::Slider& slider) noexcept;

    /**
     * @brief Area behind a toggle's cap.
     * @param button  The toggle button to compute the area for.
     */
    juce::Rectangle<float> getToggleArea (juce::ToggleButton& button) const noexcept;

    /**
     * @brief Draws the pie-segment extrusion collar beneath the knob cap.
     * @param g       Graphics context to draw into.
     * @param slider  Slider supplying the cap area and colour.
     */
    void drawKnobExtrusion (juce::Graphics& g, juce::Slider& slider);

    /**
     * @brief Draws the knob cap's bevel gradient and flat face.
     * @param g       Graphics context to draw into.
     * @param slider  Slider supplying the cap area and colour.
     */
    void drawKnobCap (juce::Graphics& g, juce::Slider& slider);

    /**
     * @brief Draws the rounded pointer, rotated to the slider's normalised position.
     * @param g          Graphics context to draw into.
     * @param slider     Slider supplying the cap area and thumb colour.
     * @param sliderPos  Normalised slider position in the range [0, 1].
     */
    void drawKnobPointer (juce::Graphics& g, juce::Slider& slider, float sliderPos);

    /**
     * @brief Draws the knob's value label. Currently a no-op.
     * @param g       Graphics context to draw into.
     * @param slider  Slider the label describes.
     */
    void drawKnobLabel (juce::Graphics& g, juce::Slider& slider);

    /**
     * @brief Derives the knob cap colour from the slider's background colour.
     * @param slider  Slider to derive the colour from.
     */
    juce::Colour getKnobColour (juce::Slider& slider) const noexcept;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleKnob_V1)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
