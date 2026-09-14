/**
 * @file jam_StyleToggleSlide.h
 * @brief LookAndFeel for a sliding-cap toggle button — cap slides between the
 *        two ends of a rounded lane, oriented by the button's aspect ratio.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief LookAndFeel for a sliding-cap toggle button — a circular cap that
 *        slides between the two ends of a rounded lane, oriented by the
 *        button's own aspect ratio (portrait slides vertically, landscape
 *        slides horizontally).
 */
class StyleToggleSlide : public juce::LookAndFeel_V4
{
public:
    StyleToggleSlide() = default;
    ~StyleToggleSlide() = default;

    //==============================================================================

    /**
     * @brief Draws the lane background, then the sliding cap, on top of it.
     * @param g                              The graphics context to paint into.
     * @param button                         The toggle button being drawn.
     * @param shouldDrawButtonAsHighlighted  Unused — hover state is not distinctly styled.
     * @param shouldDrawButtonAsDown         Unused — pressed state is not distinctly styled.
     */
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    //==============================================================================
    /**
     * @brief Sets whether the toggle's visual "on" state is inverted relative
     *        to its underlying toggle state — used when the button represents
     *        bypass (off = active) rather than direct enable.
     * @param isUsingToggleForBypass True to flip the visual state.
     */
    void setUsingToggleForBypass (bool isUsingToggleForBypass);
    //==============================================================================
private:
    /** typically used for bypass toggle, hence should be flipped */
    bool shouldFlipToggle { true };
    const juce::Colour shadowColour { juce::Colours::black };
    const float shadowBleed { 10.0f };
    float bevelSize { 2.0f };
    const int shadowRadius { 8 };
    const float shadowOpacity { 0.4f };
    const juce::Point<int> shadowOffset { 0, 6 };
    jam::InnerShadow backgroundShadow { shadowColour.withAlpha (shadowOpacity), shadowRadius, shadowOffset };
    jam::DropShadow capShadow { shadowColour.withAlpha (shadowOpacity), shadowRadius, shadowOffset };

    /** functions to draw toggle button */
    bool isPortrait (juce::ToggleButton& button) const noexcept;
    bool isLandscape (juce::ToggleButton& button) const noexcept;
    float getToggleCornerSize (juce::ToggleButton& button, float bevelSize = 0.0f) const noexcept;
    juce::Rectangle<float> getToggleArea (juce::ToggleButton& button) const noexcept;
    juce::Colour getCapColour (juce::ToggleButton& button) const noexcept;
    juce::Colour getLaneColour (juce::ToggleButton& button) const noexcept;
    void drawToggleBackground (juce::Graphics& g, juce::ToggleButton& button);
    void drawToggleCap (juce::Graphics& g, juce::ToggleButton& button);
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleToggleSlide)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
