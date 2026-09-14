/**
 * @file        jam_StyleCustom.h
 * @brief       Non-template custom virtual base for jam LookAndFeel.
 */
namespace jam
{
/*____________________________________________________________________________*/

/** @brief Non-template base — all custom jam virtuals live here.
 *  Default empty implementations. Projects override what they need.
 *
 *  Each virtual paints a single visual layer of the bar system. The Bar
 *  Component owns the layers (Background, SlidingHighlight, Tab buttons);
 *  the LAF decides how each layer looks. One virtual per layer — no
 *  combined or stateful paint methods.
 */
class StyleCustom : public juce::LookAndFeel_V4
{
public:
    /** @brief Paints the bar's full background area — the strip behind all buttons.
     *  No state. LAF decides fill style, colour source, shape.
     */
    virtual void drawBarBackground (juce::Graphics&, juce::Component& bar) {}

    /** @brief Paints the sliding selection highlight — one component, moves between active tab bounds.
     *  No per-tab state. LAF decides shape, colour source, animation.
     */
    virtual void drawBarHighlight (juce::Graphics&, juce::Component& highlight) {}

    /** @brief Paints a single tab button.
     *  @param button       The tab. button.getToggleState() is the canonical isFront flag.
     *  @param isMouseOver  Hover state.
     *  @param isMouseDown  Pressed state.
     *  LAF decides front vs inactive paint, alpha, shape.
     */
    virtual void
    drawTabButton (juce::Graphics&, juce::Button& button, bool isMouseOver, bool isMouseDown)
    {
    }

    /** @brief Paints a tab button's text label.
     *  @param label  The label owned by jam::ButtonTab.
     *  LAF decides font, colour, text transform, layout.
     */
    virtual void drawTabLabel (juce::Graphics&, juce::Label& label) {}

    /** @brief Returns the font used for tab button text.
     *  Bar layout measures tab widths with this font — measure and draw
     *  share one font source. Config font is the SSOT; bar depth derives
     *  from the font, never the reverse.
     *  Default: JUCE default font.
     */
    virtual juce::Font getTabFont() const { return juce::FontOptions().withPointHeight (12.0f); }

    virtual juce::Font getMonoFont() const
    {
        return juce::FontOptions { juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain };
    }

    /** @brief Horizontal padding in pixels on each side of the tab label,
     *  consumed by jam::ButtonBar::getBestTabLength.
     *  Default: one font height per side.
     */
    virtual int getTabPadding() const { return 0; }

    /** @brief Paints a pane EDGE's seam.
     *  @param bar  The jam::PaneEdge component. Query isMouseOver() / isMouseButtonDown() for state.
     */
    virtual void drawPaneEdge (juce::Graphics&, juce::Component& bar) {}

    /** @brief Paints a pane's outline indicator.
     *  @param pane  The pane component. Query pane.hasKeyboardFocus (true) for the focused state.
     */
    virtual void drawPaneOutline (juce::Graphics&, juce::Component& pane) {}

    /** @brief Pixel thickness of a pane EDGE's seam strip, consumed by jam::PaneEdge::getSeam(). */
    virtual int getPaneEdgeSize() const noexcept { return 8; }

    /** @brief Display transform applied to a tab label before measuring and painting.
     *  Bar::getBestTabLength measures the transformed string; drawTabButton implementations
     *  must render the same call so width and render never diverge.
     *  Default: identity.
     */
    virtual juce::String getTabText (const juce::String& tabName) const { return tabName; }

    /** @brief Component-level padding for the tab bar — space between bar edges
     *  and tab content area. CSS convention: top, right, bottom, left.
     *  Default: no padding.
     */
    virtual juce::BorderSize<int> getTabBarPadding() const { return juce::BorderSize<int> { 0 }; }

    // Untemplated so jam_gui components (ButtonGroup, overlay shadows/markers) can
    // dynamic_cast to StyleCustom* without naming a ManagerType-templated LAF class.

    /** @brief Paints a button group's track (the lane behind its sliding indicator).
     *  @param group  The button group component.
     */
    virtual void drawButtonGroupTrack (juce::Graphics&, juce::Component& group) {}

    /** @brief Paints a button group's sliding selection indicator.
     *  @param indicator  The sliding indicator component.
     */
    virtual void drawButtonGroupSlidingIndicator (juce::Graphics&, juce::Component& indicator) {}

    /** @brief Height, in pixels, of the extrusion collar drawn beneath a knob cap. Default: 0. */
    virtual float getKnobExtrusion() const noexcept { return 0.0f; }

    /** @brief Width, in pixels, reserved for a knob's value label.
     *  @param slider  The knob slider to measure the label for.
     *  Default: 0.
     */
    virtual float getLabelWidth (juce::Slider& slider) noexcept { return 0.0f; }

    /** @brief Height, in pixels, of a toggle's cap extrusion collar. Default: 0. */
    virtual float getCapExtrusion() const noexcept { return 0.0f; }

    /** @brief Area behind a toggle's cap, excluding any extrusion.
     *  @param button  The toggle button to compute the area for.
     *  Default: empty rectangle.
     */
    virtual juce::Rectangle<float> getBackgroundArea (juce::ToggleButton& button) const noexcept { return {}; }

    /** @brief Fill colour for a toggle's cap.
     *  @param button  The toggle button to derive the colour for.
     *  Default: transparent black.
     */
    virtual juce::Colour getCapColour (juce::ToggleButton& button) const noexcept { return {}; }

    /** @brief Bevel inset, in pixels, applied to a toggle's cap. Default: 0. */
    virtual float getBevelSize() const noexcept { return 0.0f; }

    /** @brief Effective visual toggle state, after any bypass inversion.
     *  @param button  The toggle button to read state from.
     *  Default: false.
     */
    virtual bool getState (juce::ToggleButton& button) const noexcept { return false; }

    /** @brief Opacity applied to background-blurred windows. Default: 1.0 (opaque). */
    virtual float getWindowOpacity() const noexcept { return 1.0f; }

    /** @brief Blur radius applied to background-blurred windows. Default: 0.0 (no blur). */
    virtual float getWindowBlur() const noexcept { return 0.0f; }

    /** @brief Padding, in pixels, reserved around a window's content. Default: 0. */
    virtual int getWindowPadding() const noexcept { return 0; }

    /** @brief Size, in pixels, of a window's close control. Default: 0. */
    virtual int getWindowClose() const noexcept { return 0; }

    /** @brief Platform-specific background blur backend name. Default: empty (no backend). */
    virtual juce::String getWindowBackendString() const noexcept { return {}; }

    /** @brief Derives and applies this LookAndFeel's window glass (chrome tint, opacity,
     *  blur backend) onto @p window. Default: no-op — jam_style does not depend on
     *  jam_gui, so the meaningful override lives downstream (see StyleTheme::prepareWindow,
     *  StyleDebug::prepareWindow) where StyleWindow/BackgroundBlur are visible.
     */
    virtual void prepareWindow (juce::Component&) {}

protected:
    jam::ColourScheme colourScheme;
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
