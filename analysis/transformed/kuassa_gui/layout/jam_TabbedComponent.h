/**
 * @file jam_TabbedComponent.h
 * @brief One-visible-child owner strategy driven by a jam::ButtonBar.
 */
#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/**
 * @class TabbedComponent
 * @brief Owner strategy where children swap one-visible, driven by an owned
 * `jam::ButtonBar`. Every tab switch routes through the bar (setCurrentTab() →
 * changeCallback() → setFocusedChild()); the focused-child parameter is
 * `Id::focusedTab` on the owner's own row.
 */
class TabbedComponent : public OwnerComponent
{
public:
    /**
     * @brief Build-or-adopt: forwards to OwnerComponent's build-or-adopt constructor.
     * @param model       Owning model.
     * @param parentState Tree to search/append the row under.
     * @param type        Row type identifier to match or create.
     * @param uuid        Identity to match or assign to the row.
     */
    TabbedComponent (jam::Model& model,
                     juce::ValueTree parentState,
                     const juce::Identifier& type,
                     jam::UUID uuid);

    /**
     * @brief Adopt: binds directly to an already-existing row.
     * @param model          Owning model.
     * @param existingState  Row to bind to.
     */
    TabbedComponent (jam::Model& model, juce::ValueTree existingState);
    ~TabbedComponent() override = default;

    /**
     * @brief Sets the tab bar's depth (thickness along its short axis).
     * @param newDepth  New depth, in pixels.
     */
    void setTabBarDepth (int newDepth);

    /** @return The tab bar's current depth, in pixels. */
    int getTabBarDepth() const noexcept { return tabDepth; }

    /**
     * @brief Sets the tab bar's edge position.
     * @param newPosition  Edge to place the tab bar on (map::Position value).
     */
    void setPosition (int newPosition);

    /** @return The tab bar's current edge position (map::Position value). */
    int getPosition() const noexcept;

    /**
     * @brief Sets the outline stroke thickness drawn around the content area.
     * @param newThickness  New thickness, in pixels.
     */
    void setOutline (int newThickness);

    /**
     * @brief Sets the inset between the tab bar and the content area.
     * @param indentThickness  New inset, in pixels.
     */
    void setIndent (int indentThickness);

    /**
     * @brief Renames the tab identified by uuid.
     * @param uuid     Identity of the tab to rename.
     * @param newName  New label for the tab.
     */
    void setTabName (jam::UUID uuid, const juce::String& newName);

    /**
     * @brief Moves a tab within the bar's visual order.
     * @param uuid         Identity of the tab to move.
     * @param newPosition  Destination position in the visual order.
     * @param animate      Whether to animate the indicator after the move.
     */
    void moveTab (jam::UUID uuid, int newPosition, bool animate = false);

    /** @return List of all tab labels, in visual order. */
    juce::StringArray getTabNames() const;

    /** @return Per-tab background fill colour for the tab identified by uuid. */
    juce::Colour getTabBackgroundColour (jam::UUID uuid) const noexcept;

    /**
     * @brief Sets the per-tab background fill colour for the tab identified by uuid.
     * @param uuid       Identity of the tab.
     * @param newColour  New background fill colour.
     */
    void setTabBackgroundColour (jam::UUID uuid, juce::Colour newColour);

    /**
     * @brief Sets the active tab and shows its child, hiding the previously active one.
     * @param uuid  Identity of the tab to make active.
     */
    void setCurrentTab (jam::UUID uuid);

    /** @return The currently active tab's identity. */
    jam::UUID getCurrentTab() const;

    /** @return The currently active tab's label. */
    juce::String getCurrentTabName() const;

    /** Activates the tab following the current one, wrapping to the first. */
    void nextTab();

    /** Activates the tab preceding the current one, wrapping to the last. */
    void prevTab();

    /**
     * @brief Called when the active tab changes.
     * @param newCurrentTab      Identity of the newly active tab.
     * @param newCurrentTabName  Label of the newly active tab.
     */
    virtual void currentTabChanged (jam::UUID newCurrentTab, const juce::String& newCurrentTabName);

    /**
     * @brief Called when the user selects a tab from the bar's overflow popup menu.
     * @param uuid     Identity of the selected tab.
     * @param tabName  Label of the selected tab.
     */
    virtual void popupMenuClickOnTab (jam::UUID uuid, const juce::String& tabName);

    /** @return Reference to the owned tab bar. */
    jam::ButtonBar& getBar() const noexcept { return *bar; }

    /** Fills the content area with the current tab's background colour, then
     *  draws the outline stroke around it when outlineThickness is set.
     */
    void paint (juce::Graphics&) override;

    /** Propagates lookAndFeelChanged() to every owned child. */
    void lookAndFeelChanged() override;

protected:
    /**
     * @brief Factory hook for the tab bar's tab button component.
     * @param tabName  Label for the new tab.
     * @return The newly created tab button, owned by the bar.
     */
    virtual std::unique_ptr<jam::MouseEvents<jam::ButtonTab>>
    createTabButton (const juce::String& tabName);

    /** Adds a tab to the bar for the newly added child and shows it if it is the only child. */
    void childAdded (jam::UUID uuid) override;

    /** Removes the tab from the bar for the removed child. */
    void childRemoved (jam::UUID uuid) override;

    /** Positions the tab bar along its edge; sizes every child to fill the
     *  remaining content area, showing only the focused one.
     */
    void layout() override;

private:
    /** @brief Owned jam::ButtonBar subclass forwarding its virtual callbacks to the owning TabbedComponent. */
    struct ButtonBar final : public jam::ButtonBar
    {
        /** @brief Constructs, recording the owning TabbedComponent to forward callbacks to. */
        explicit ButtonBar (TabbedComponent& tabComp)
            : owner (tabComp)
        {
        }

        /** @brief Forwards to owner.changeCallback(). */
        void currentTabChanged (jam::UUID newCurrentTab, const juce::String& newTabName) override
        {
            owner.changeCallback (newCurrentTab, newTabName);
        }

        /** @brief Forwards to owner.popupMenuClickOnTab(). */
        void popupMenuClickOnTab (jam::UUID uuid, const juce::String& tabName) override
        {
            owner.popupMenuClickOnTab (uuid, tabName);
        }

        /** @brief Forwards to owner.createTabButton(). */
        std::unique_ptr<jam::MouseEvents<jam::ButtonTab>>
        createTabButton (const juce::String& tabName) override
        {
            return owner.createTabButton (tabName);
        }

        TabbedComponent& owner; ///< Owning TabbedComponent, receiving forwarded callbacks.

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonBar)
    };

    std::unique_ptr<jam::ButtonBar> bar; ///< Owned tab bar driving tab switches.
    /** @brief Dispatches, by map::Position edge key, the region-cut function used by paint() and layout(). */
    jam::Function::Map<int, juce::Rectangle<int>> edges {
        []
        {
            jam::Function::Map<int, juce::Rectangle<int>> e;
            e.add<juce::Rectangle<int>&, int&> (map::Position::top,
                                                [] (juce::Rectangle<int>& c, int d)
                                                {
                                                    return c.removeFromTop (d);
                                                });
            e.add<juce::Rectangle<int>&, int&> (map::Position::bottom,
                                                [] (juce::Rectangle<int>& c, int d)
                                                {
                                                    return c.removeFromBottom (d);
                                                });
            e.add<juce::Rectangle<int>&, int&> (map::Position::left,
                                                [] (juce::Rectangle<int>& c, int d)
                                                {
                                                    return c.removeFromLeft (d);
                                                });
            e.add<juce::Rectangle<int>&, int&> (map::Position::right,
                                                [] (juce::Rectangle<int>& c, int d)
                                                {
                                                    return c.removeFromRight (d);
                                                });
            return e;
        }()
    };
    int tabDepth { 30 };        ///< Tab bar depth (thickness along its short axis), in pixels.
    int outlineThickness { 0 }; ///< Outline stroke thickness drawn around the content area, in pixels.
    int edgeIndent { 0 };       ///< Inset between the tab bar and the content area, in pixels.

    /** @brief Publishes the newly focused child, then calls currentTabChanged(). */
    void changeCallback (jam::UUID newCurrentTab, const juce::String& newTabName);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabbedComponent)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
