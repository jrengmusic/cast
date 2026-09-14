/**
 * @file jam_StyleTogglePush.h
 * @brief LookAndFeel for a push-cap toggle button — bevelled cap over an
 *        extrusion collar, with an inner-shadowed standby glyph.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief LookAndFeel for a push-cap toggle button — a raised, bevelled circular
 *        cap over an extrusion collar, with an inner-shadowed "standby" glyph
 *        that glows when active.
 */
class StyleTogglePush : public StyleCustom
{
public:
    StyleTogglePush()=default;
    ~StyleTogglePush()=default;
    //==============================================================================

    /**
     * @brief Draws the cap extrusion, the cap, and the standby symbol, in that order.
     * @param g                              The graphics context to paint into.
     * @param button                         The toggle button being drawn.
     * @param shouldDrawButtonAsHighlighted  Unused — hover state is not distinctly styled.
     * @param shouldDrawButtonAsDown         Unused — down state is read from button.isDown() internally.
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

    /** @brief Returns the cap fill colour derived from buttonColourId.
     *  @param button The toggle button to derive the colour for.
     */
    juce::Colour getCapColour (juce::ToggleButton& button) const noexcept override;

    /** @brief Returns the standby-symbol colour from buttonOnColourId.
     *  @param button The toggle button to derive the colour for.
     */
    juce::Colour getSymbolColour (juce::ToggleButton& button) const noexcept;

    /** @brief Returns the area behind the cap, excluding the top extrusion half.
     *  @param button The toggle button to compute the area for.
     */
    juce::Rectangle<float> getBackgroundArea (juce::ToggleButton& button) const noexcept override;

    /** @brief Returns the effective visual toggle state, applying the
     *  shouldFlipToggle inversion set by setUsingToggleForBypass().
     *  @param button The toggle button to read state from.
     */
    bool getState (juce::ToggleButton& button) const noexcept override;

    /** @brief Returns the cap's bevel inset in pixels. */
    float getBevelSize() const noexcept override;

    /** @brief Returns the cap extrusion collar's height in pixels. */
    float getCapExtrusion() const noexcept override;
    //==============================================================================
private:
    bool shouldFlipToggle { true };
    /** @brief Height, in pixels, of the extrusion collar drawn beneath the cap. */
    const float capExtrusion { 10.0f };
    const float bevelSize { 2.0f };
    const int shadowRadius { 8 };
    const float shadowOpacity { 0.4f };
    const juce::Point<int> shadowOffset { 0, 6 };

    const float symbolShadowOpacity { 0.4f };
    const int symbolShadowRadius { 3 };
    const juce::Point<int> symbolShadowOffset { 0, 2 };

    const juce::Colour shadowColour { juce::Colours::black };

    jam::InnerShadow capInnerShadow { shadowColour.withAlpha (shadowOpacity), shadowRadius, shadowOffset };
    jam::InnerShadow symbolShadow { shadowColour.withAlpha (symbolShadowOpacity), symbolShadowRadius, symbolShadowOffset };
    jam::Blur glowBlur { 16 };

    //==============================================================================
    juce::Rectangle<float> getCapArea (juce::ToggleButton& button) const noexcept;

    void drawBackground (juce::Graphics& g, juce::ToggleButton& button);
    void drawCap (juce::Graphics& g, juce::ToggleButton& button);
    void drawCapExtrusion (juce::Graphics& g, juce::ToggleButton& button);
    void drawStandbySymbol (juce::Graphics& g, juce::ToggleButton& button);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleTogglePush)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
