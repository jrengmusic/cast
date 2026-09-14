/**
 * @file        jam_ButtonBar.h
 * @brief       Tab bar with per-button natural width and overflow collapse.
 *
 * Each tab's width is getBestTabLength (text + 2×pad) — exact natural width,
 * never scaled. Tabs that overflow the bar's available length collapse into
 * the extraTabsButton popup; visible tabs keep their exact natural widths.
 * Forked from juce::TabbedButtonBar. Paint pipeline uses
 * jam::StyleCustom instead of juce LAF methods.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/** @brief Tab bar with per-button natural width and overflow collapse.
 *  Each tab's length is getBestTabLength (text + 2×pad) — exact natural width,
 *  never scaled. Tabs that overflow collapse into the extraTabsButton popup;
 *  visible tabs keep their exact natural widths.
 *  Forked from juce::TabbedButtonBar. Supports all four orientations via
 *  setPosition(). Inherits ChangeListener to receive arrival notification
 *  from the indicator animator — toggle states are applied on arrival, not on
 *  departure.
 */
class ButtonBar : public juce::Component,
            public juce::ChangeListener
{
public:
    /**
     * @brief Colour identifiers for jam::ButtonBar.
     *
     * Used by StyleCustom implementations to resolve bar and highlight
     * colours via findColour(). Tab button text uses the JUCE-native
     * juce::TextButton colour ids (textColourOnId, textColourOffId) —
     * ButtonBar has no involvement in those. Tab fill instead uses the
     * per-tab colour passed to addTab()/setTabBackgroundColour() and
     * retrieved via getTabBackgroundColour().
     *
     * Range 0x4200001: bar strip background.
     * Range 0x4200004: bar outline stroke.
     * Range 0x4200100: sliding selection highlight.
     */
    enum ColourIds
    {
        /** @brief Bar strip background fill colour. */
        backgroundColourId = map::ColourId::buttonBarBackgroundColourId,
        /** @brief Tab outline stroke colour. */
        outlineColourId    = map::ColourId::buttonBarOutlineColourId,
        /** @brief Sliding selection highlight fill colour. */
        highlightColourId  = map::ColourId::buttonBarHighlightColourId,
    };

    /** @brief Full-bar background layer. Painted first (behind everything).
     *  Delegated to jam::StyleCustom::drawBarBackground.
     */
    class Background : public juce::Component
    {
    public:
        /** @brief Paint the bar background via the LAF. */
        void paint (juce::Graphics& g) override
        {
            static_cast<jam::StyleCustom&> (getLookAndFeel())
                .drawBarBackground (g, *this);
        }
    };

    /** @brief Sliding selection highlight drawn between background and tab buttons.
     *  Delegated to jam::StyleCustom::drawBarHighlight.
     */
    class SlidingHighlight : public juce::Component
    {
    public:
        /** @brief Paint the sliding selection highlight via the LAF. */
        void paint (juce::Graphics& g) override
        {
            static_cast<jam::StyleCustom&> (getLookAndFeel())
                .drawBarHighlight (g, *this);
        }
    };

    /** @brief Construct: add Background + SlidingHighlight as children,
     *  attach a juce::ComponentAnimator to the highlight. */
    ButtonBar();
    /** @brief Default destructor. */
    ~ButtonBar() override;

    /** @brief Sets the bar position. Triggers layout update.
     *  @param newPosition  Edge to place the tab bar on (Position value).
     */
    void setPosition (int newPosition);
    /** @brief The current Position value. */
    int getPosition() const noexcept { return position; }

    /** @brief True when orientation is left or right. */
    bool isVertical() const noexcept;

    /** @brief Remove all tabs from the bar. */
    void clearTabs();
    /**
     * @brief Add a tab to the bar.
     *
     * @param uuid                 Identity of the new tab; caller-supplied.
     * @param tabName              Label for the new tab; drives variable width.
     * @param tabBackgroundColour  Per-tab background fill colour, retrieved via
     *                             getTabBackgroundColour() and consumed by the LAF
     *                             when painting this tab's fill.
     */
    void addTab (jam::UUID uuid, const juce::String& tabName, juce::Colour tabBackgroundColour);
    /** @brief Rename the tab identified by uuid.
     *  @param uuid     Identity of the tab to rename.
     *  @param newName  New label for the tab.
     */
    void setTabName (jam::UUID uuid, const juce::String& newName);
    /**
     * @brief Remove the tab identified by uuid.
     *
     * Selects the next adjacent tab (or none, if the bar becomes empty) via
     * setCurrentTab(), then repositions the remaining tab buttons.
     *
     * @param uuid     Identity of the tab to remove.
     * @param animate  Whether to animate the remaining tabs' repositioning
     *                 (and the sliding highlight, when it moves as a result).
     */
    void removeTab (jam::UUID uuid, bool animate = false);
    /**
     * @brief Move a tab within the bar's visual order.
     * @param uuid         Identity of the tab to move.
     * @param newPosition  Destination position in the visual order.
     * @param animate      Whether to animate the tab buttons' repositioning
     *                     (and the sliding highlight, when it moves as a result).
     */
    void moveTab (jam::UUID uuid, int newPosition, bool animate = false);
    /** @brief Number of tabs currently in the bar. */
    int getNumTabs() const;
    /** @brief List of all tab labels, in visual order. */
    juce::StringArray getTabNames() const;

    /**
     * @brief Set the active tab and broadcast the change (currentTabChanged()).
     *
     * When there was a previously active tab the indicator animates to the new
     * tab and toggle states are applied on arrival (changeListenerCallback).
     * When there was no previous tab (first add, or after clearTabs) the
     * indicator snaps immediately and toggle states are applied inline.
     *
     * @param uuid  Identity of the tab to make active.
     */
    void setCurrentTab (jam::UUID uuid);
    /** @brief The currently active tab identity, or UUID::none() if the bar is empty. */
    jam::UUID getCurrentTab() const noexcept { return currentTab; }
    /** @brief The label of the currently active tab; empty if the bar is empty. */
    juce::String getCurrentTabName() const;

    void nextTab();
    void prevTab();

    /** @brief Raw pointer to the tab button identified by uuid (non-owning). */
    jam::MouseEvents<ButtonTab>* getTabButton (jam::UUID uuid) const;

    /** @brief Per-tab background fill colour for the tab identified by uuid. */
    juce::Colour getTabBackgroundColour (jam::UUID uuid) const;
    /** @brief Set the per-tab background fill colour for the tab identified by uuid. */
    void setTabBackgroundColour (jam::UUID uuid, juce::Colour newColour);

    /**
     * @brief Returns preferred tab length based on shaped text width.
     *
     * Measures shaped-text bounding-box width via juce::TextLayout::getStringWidth
     * of the getTabText-transformed name, using the LAF tab font (getTabFont).
     * Adds getTabPadding per side.  Result is exact — no minimum clamp, no
     * GlyphArrangement.
     *
     * @param tabName  Tab label to measure; trimmed then passed through getTabText.
     * @return Exact preferred length in pixels: roundToInt(width) + pad * 2.
     */
    int getBestTabLength (const juce::String& tabName) const;

    /**
     * @brief Called when the active tab changes.
     *
     * Override in subclasses to react to user-driven tab switches.
     *
     * @param newCurrentTab      Identity of the newly active tab.
     * @param newCurrentTabName  Label of the newly active tab.
     */
    virtual void currentTabChanged (jam::UUID newCurrentTab, const juce::String& newCurrentTabName);
    /**
     * @brief Called when the user selects a tab from the overflow popup menu.
     *
     * @param uuid     Identity of the selected tab.
     * @param tabName  Label of the selected tab.
     */
    virtual void popupMenuClickOnTab (jam::UUID uuid, const juce::String& tabName);

    /** @brief Fired after a tab has been moved via drag-reorder.
     *  @param uuid  Identity of the moved tab.
     */
    std::function<void (jam::UUID)> onTabMoved;

    /** @brief Paint the bar's three layers (background, highlight, tabs). */
    void paint (juce::Graphics&) override;
    /** @brief Lay out background, highlight, and visible tab buttons for the current size. */
    void resized() override;
    /** @brief Repaint all children when the LAF changes. */
    void lookAndFeelChanged() override;

protected:
    /**
     * @brief Factory hook for the tab button component.
     *
     * Override in subclasses to provide a custom tab button.
     * Default creates a jam::MouseEvents<ButtonTab> and sets its button text.
     *
     * @param tabName  Label for the new tab.
     * @return The newly created tab button, owned by the bar.
     */
    virtual std::unique_ptr<jam::MouseEvents<ButtonTab>> createTabButton (const juce::String& tabName);

private:
    /** @brief Per-tab data: the button itself, its label, and its background colour. */
    struct TabInfo
    {
        std::unique_ptr<jam::MouseEvents<ButtonTab>> button; ///< The tab button component.
        juce::String name;                                ///< Tab label.
        juce::Colour colour;                              ///< Per-tab background fill.
    };

    /** @brief All tab info records, keyed by identity. */
    jam::HashMap<jam::UUID, TabInfo> tabs;
    /** @brief Visual sequence of tab identities. */
    jam::Array<jam::UUID> order;
    /** @brief Identity of the active tab, or UUID::none() if the bar is empty. */
    jam::UUID currentTab { jam::UUID::none() };
    /** @brief Current bar position (map::Position). */
    int position { map::Position::top };

    /** @brief Tab identity captured at mouseDown, used as the source for drag-to-reorder. */
    jam::UUID dragStartTab { jam::UUID::none() };
    /** @brief Component-relative position of the button at mouseDown, used to compute drag distance. */
    juce::Point<int> dragStartPos;
    /** @brief Pixels of mouse movement past which a drag is treated as a reorder gesture. */
    static constexpr int dragThreshold { 5 };

    /** @brief Optional overflow popup trigger button (shown when tabs exceed the bar width). */
    std::unique_ptr<juce::Button> extraTabsButton;

    /** @brief Immediately places the sliding highlight over the current tab (cancels any running
     *  animation) and applies toggle states — snap is an instant arrival.
     */
    void snapHighlight();

    /** @brief Animates the sliding highlight to slide to the current tab over
     *  durationMs (jam_ButtonBar.cpp's file-scope animation-duration constant).
     */
    void animateHighlight();

    /** @brief Applies toggle state to every tab button from currentTab.
     *  Called on indicator arrival — animated (changeListenerCallback) or
     *  snapped (snapHighlight). Toggle state drives the active text colour.
     */
    void setToggleStates();

    /** @brief Receives animator change notifications; applies toggle states on highlight arrival.
     *  The start broadcast is filtered out via isAnimating(&highlight) — only the finish
     *  broadcast (after the task is removed) passes through to setToggleStates().
     */
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    /** @brief Show the overflow popup with all hidden tabs. */
    void showExtraItemsMenu();
    /** @brief Recompute tab positions and (optionally) animate to them.
     *  @param animate     When true, animate the tab buttons (and, if shouldSnap
     *                     is also true, the sliding highlight) to their new bounds.
     *                     When false, reposition immediately.
     *  @param shouldSnap  When true, also move the sliding highlight onto the
     *                     current tab's new bounds (applying toggle states).
     *                     When false, leave the highlight untouched — the caller
     *                     retargets it explicitly (see setCurrentTab()).
     */
    void updateTabPositions (bool animate, bool shouldSnap = true);

    /** @brief Sliding selection highlight child component. */
    SlidingHighlight highlight;
    /** @brief Full-bar background child component. */
    Background background;
    /** @brief Drives the sliding highlight animation. */
    juce::ComponentAnimator animator;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonBar)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
