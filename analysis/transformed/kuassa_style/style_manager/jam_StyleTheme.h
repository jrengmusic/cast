/**
 * @file jam_StyleTheme.h
 * @brief Manager-driven LookAndFeel_V4 theme — fonts, colours, window
 *        backend/opacity/blur, and full component painting overrides.
 */

namespace jam
{
/*____________________________________________________________________________*/

//==============================================================================
/**
 * @class StyleTheme
 * @brief Concrete `LookAndFeel_V4` theme driven by `StyleManager` — resolves
 * fonts, colours, and window backend/opacity/blur from the manager's
 * StyleSheet, and overrides component painting for combo boxes, labels, toggles,
 * buttons, tabs, and popup menus.
 */
class StyleTheme : public StyleCustom
{
public:
    /** @brief Slider text-box label that ignores mouse wheel scrolling. */
    struct SliderLabelComp : public juce::Label
    {
        SliderLabelComp()
            : juce::Label ({}, {})
        {
        }
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override {}
    };

    //==============================================================================
    /**
     * @brief Constructs the theme, applies the default sans-serif typeface, and
     * sets the initial appearance.
     * @param styleManager       Style manager supplying fonts, colours, and StyleSheet queries.
     * @param defaultAppearance  Initial appearance name applied via setAppearance().
     */
    StyleTheme (StyleManager& styleManager, juce::StringRef defaultAppearance = Id::dark);
    ~StyleTheme() override = default;

    /**
     * @brief Delegates to `StyleManager::setAppearance()` to switch light/dark appearance.
     * @param lightOrDark  Appearance name.
     */
    void setAppearance (juce::StringRef lightOrDark);

    /**
     * @brief Applies the glyph atlas's hard rasterizer defaults, then overrides them
     * with any StyleSheet `\<FONTS\>` rasterizer/gamma/contrast attributes present.
     */
    void setFontRasterization();

    /**
     * @brief Applies the StyleSheet `\<FONTS\>` embolden attribute to the glyph atlas,
     * defaulting to `false` when absent.
     */
    void setEmbolden();
    //==============================================================================
    /** Returns the common theme font for combo boxes. */
    juce::Font getComboBoxFont (juce::ComboBox&) override;

    /** Returns the common theme font for popup menus. */
    juce::Font getPopupMenuFont() override;

    /** Enlarges the ideal popup menu item height relative to the popup menu font, unless @p isSeparator. */
    void getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight) override;

    /** Returns the common theme font for text buttons. */
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    /** Returns the label's own font when marked as content; otherwise the common theme font. */
    juce::Font getLabelFont (juce::Label& label) override;

    /** Returns no border when the label is marked as content; otherwise the label's own border size. */
    juce::BorderSize<int> getLabelBorderSize (juce::Label& label) override;

    /** Returns the common theme font for menu bar items. */
    juce::Font getMenuBarFont (juce::MenuBarComponent& menuBar, int itemIndex, const juce::String& itemText) override;

    /** Returns the theme's right-pointing tick/chevron path, scaled to @p height. */
    juce::Path getTickShape (float height) override;
    //==============================================================================

    /** Draws a popup menu item: separator rule, highlight, icon or tick mark, label, sub-menu arrow, and shortcut text. */
    void drawPopupMenuItem (juce::Graphics& g,
                            const juce::Rectangle<int>& area,
                            const bool isSeparator,
                            const bool isActive,
                            const bool isHighlighted,
                            const bool isTicked,
                            const bool hasSubMenu,
                            const juce::String& text,
                            const juce::String& shortcutKeyText,
                            const juce::Drawable* icon,
                            const juce::Colour* const textColourToUse) override;

    /** Draws a popup menu section header using the theme's bold font. */
    void drawPopupMenuSectionHeader (juce::Graphics& g,
                                     const juce::Rectangle<int>& area,
                                     const juce::String& sectionName) override;

    /** No-op — document window title bars are not themed. */
    void
    drawDocumentWindowTitleBar (juce::DocumentWindow&, juce::Graphics&, int, int, int, int, const juce::Image*, bool)
        override;

    juce::Button* createDocumentWindowButton (int buttonType) override;

    void positionDocumentWindowButtons (juce::DocumentWindow&,
                                        int titleBarX,
                                        int titleBarY,
                                        int titleBarW,
                                        int titleBarH,
                                        juce::Button* minimiseButton,
                                        juce::Button* maximiseButton,
                                        juce::Button* closeButton,
                                        bool positionTitleBarButtonsOnLeft) override;

    /** Fills the popup menu background, unless background blur is enabled on Windows. */
    void drawPopupMenuBackgroundWithOptions (juce::Graphics& g,
                                             int width,
                                             int height,
                                             const juce::PopupMenu::Options&) override;

    /** Delegates to drawPopupMenuSectionHeader(), ignoring @p options. */
    void drawPopupMenuSectionHeaderWithOptions (juce::Graphics& g,
                                                const juce::Rectangle<int>& area,
                                                const juce::String& sectionName,
                                                const juce::PopupMenu::Options& options) override;

    /** Makes the popup window transparent and, asynchronously, enables background blur when configured. */
    void preparePopupMenuWindow (juce::Component& newWindow) override;

    /** Returns no extra window flags on Windows (blur handled separately); otherwise the base flags. */
    int getMenuWindowFlags() override;

    /** Draws the combo box's rounded background, outline, and open/closed arrow glyph. */
    void
    drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override;

    /** Draws the combo box placeholder text when nothing is selected. */
    void drawComboBoxTextWhenNothingSelected (juce::Graphics& g, juce::ComboBox& box, juce::Label& label) override;

    /** Draws a label's background, text (wrapped when multi-line), and outline while being edited. */
    void drawLabel (juce::Graphics& g, juce::Label& label) override;

    /** Fills a text editor's rounded background, with a bottom rule when hosted in an `AlertWindow`. */
    void fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override;

    /** Draws a text editor's focus/outline rounded rectangle, when not hosted in an `AlertWindow`. */
    void drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override;

    /** Draws a toggle button's tick box and its text label. */
    void drawToggleButton (juce::Graphics& g,
                           juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    /** Draws a standalone or connected button's gradient-filled, rounded background, unless marked as a group button. */
    void drawButtonBackground (juce::Graphics&,
                               juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    /** Creates the increment/decrement `juce::TextButton` used by IncDecButtons sliders. */
    juce::Button* createSliderButton (juce::Slider&, bool isIncrement) override;

    /** Draws group-button text centred, or IncDecButtons glyphs via `jam::ButtonSVG`, or falls back to the base implementation. */
    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool isMouseOver, bool isButtonDown) override;

    /** Draws a tick box's rounded outline and, when @p ticked, its check-mark stroke. */
    void drawTickBox (juce::Graphics& g,
                      juce::Component& component,
                      float x,
                      float y,
                      float w,
                      float h,
                      const bool ticked,
                      [[maybe_unused]] const bool isEnabled,
                      [[maybe_unused]] const bool shouldDrawButtonAsHighlighted,
                      [[maybe_unused]] const bool shouldDrawButtonAsDown) override;

    /** Draws an angled tab button shape, its outline, and its (possibly rotated) text label. */
    void drawTabButton (juce::TabBarButton& button, juce::Graphics& g, bool isMouseOver, bool isMouseDown) override;

    /** Returns the tab's best width from its measured text plus overlap and any extra component. */
    int getTabButtonBestWidth (juce::TabBarButton& button, int tabDepth) override;

    //==============================================================================
    /** Draws a button group's rounded track fill and outline. */
    void drawButtonGroupTrack (juce::Graphics& g, juce::Component& group) override;

    /** Draws a button group's rounded sliding indicator fill and outline. */
    void drawButtonGroupSlidingIndicator (juce::Graphics& g, juce::Component& indicator) override;

    //==============================================================================
    /** @return The theme's common font, as resolved by the style manager. */
    const juce::FontOptions getCommonFont();

    //==============================================================================
    /** @return The theme's window opacity, from the style manager's `\<WINDOW\>` StyleSheet section. */
    float getWindowOpacity() const noexcept override;

    /** @return The theme's window blur radius, from the style manager's `\<WINDOW\>` StyleSheet section. */
    float getWindowBlur() const noexcept override;

    /** @return The theme's window padding, from the style manager's `\<WINDOW\>` `--padding`. */
    int getWindowPadding() const noexcept override;

    /** @return The theme's window close size, from the style manager's `\<WINDOW\>` `--close`. */
    int getWindowClose() const noexcept override;

    /** @return The platform-specific window backend name (`mac`/`win`), from the style manager. */
    juce::String getWindowBackendString() const noexcept override;

    /** Derives opacity/blur/backend from this theme's own registered colour, then applies
     *  chrome tint, native transparency, opacity, and blur to @p window synchronously. */
    void prepareWindow (juce::Component& window) override;

    //==============================================================================
private:
    StyleManager& style; ///< Style manager backing this theme's font, colour, and StyleSheet queries.

    // Shipped OOTB font-rasterization defaults (freetype backend, gamma 2.2,
    // contrast 0.0) — this atlas's own hard, StyleSheet-independent starting
    // point, applied unconditionally by setFontRasterization().
    static constexpr float defaultFontGamma { 2.2f };    ///< Hard default rasterization gamma.
    static constexpr float defaultFontContrast { 0.0f }; ///< Hard default rasterization contrast.

    /**
     * @brief Resolves the five outline points of an angled tab shape for the given orientation.
     *
     * Dispatches to one of four orientation-specific corner-cut builders. When @p toggled, the
     * front-facing corner is left square; otherwise it is cut by a third of the tab's own depth
     * (height for a horizontal tab bar, width for a vertical one).
     *
     * @param orientation The tab bar's orientation, selecting which edge is angled.
     * @param tab         The tab's active area.
     * @param toggled     True when the tab is the selected/front tab.
     * @return The five points tracing the tab's outline, ready to be closed into a Path.
     */
    static std::array<juce::Point<int>, 5> getTabOutline (juce::TabbedButtonBar::Orientation orientation,
                                                           const juce::Rectangle<int>& tab,
                                                           bool toggled);

    /**
     * @brief Rotates and translates @p g so vertical tab text can be drawn as if horizontal.
     *
     * No-op transform for a top/bottom tab bar (translation only). A left/right tab bar rotates
     * text +/-90 degrees about @p area's near corner so subsequent drawFittedText() calls read
     * along the tab's own long axis.
     *
     * @param g           The graphics context to transform.
     * @param orientation The tab bar's orientation.
     * @param area        The tab's text area, providing the rotation's translation anchor.
     */
    static void applyTabTransform (juce::Graphics& g,
                                   juce::TabbedButtonBar::Orientation orientation,
                                   const juce::Rectangle<float>& area);

    /** @brief Draws a group button's text centred in its bounds, dimmed when the button is disabled. */
    void drawGroupButtonText (juce::Graphics& g, juce::TextButton& button);

    /** @brief Draws an IncDecButtons slider button's +/- glyph via `jam::ButtonSVG`, styled by hover/press state. */
    void drawSliderButtonText (juce::Graphics& g,
                               juce::TextButton& button,
                               bool isMouseOver,
                               bool isButtonDown);

    /** @brief Draws a single horizontal popup-menu separator rule, vertically centred in @p area. */
    void drawMenuSeparator (juce::Graphics& g, const juce::Rectangle<int>& area);

    /**
     * @brief Builds a rounded-rectangle path with corners squared off on the connected edges.
     *
     * Each `flatOn*` flag suppresses rounding on the two corners touching that edge; a corner
     * stays rounded only when neither of its two adjacent edges is flat.
     *
     * @param bounds      The rectangle to trace.
     * @param cornerSize  The corner radius applied to non-flat corners.
     * @param flatOnLeft  True to square the corners on the left edge.
     * @param flatOnRight True to square the corners on the right edge.
     * @param flatOnTop   True to square the corners on the top edge.
     * @param flatOnBottom True to square the corners on the bottom edge.
     * @return The resulting path.
     */
    static juce::Path getConnectedEdgePath (const juce::Rectangle<float>& bounds,
                                            float cornerSize,
                                            bool flatOnLeft,
                                            bool flatOnRight,
                                            bool flatOnTop,
                                            bool flatOnBottom);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleTheme)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
