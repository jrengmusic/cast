/**
 * @file        jam_PaneComponent.h
 * @brief       Owned pane leaf with four corner overflow menus and a
 *              focus-aware outline.
 */
#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/** @brief Base pane leaf for a MatrixComponent binary-space graph.
 *
 *  Adds four corner hit zones, each backed by a jam::ButtonMenu sharing
 *  one caller-supplied factory/action pair (setCornerMenuFactory(),
 *  setCornerMenuAction()), and a focus-aware outline paint via
 *  StyleCustom::drawPaneOutline().
 */
class PaneComponent : public OwnedComponent
{
public:
    /**
     * @brief Colour identifiers for jam::PaneComponent.
     *
     * Used by StyleCustom implementations to resolve pane colours
     * via findColour() — base 0x4500000 — next free block after markdown
     * (0x4400000).
     */
    enum ColourIds
    {
        /** @brief Pane background fill colour. */
        backgroundColourId = map::ColourId::paneComponentBackgroundColourId,
        /** @brief Pane outline stroke colour. */
        outlineColourId = map::ColourId::paneComponentOutlineColourId,
        /** @brief Focused pane outline stroke colour. */
        focusedOutlineColourId = map::ColourId::paneComponentFocusedOutlineColourId,
    };

    //==============================================================================
    /** @brief Build-or-adopt a pane row and configure its four corner menus.
     *  @param model       Owning model; forwarded to OwnedComponent.
     *  @param parentState Tree to search/append the row under.
     *  @param type        Row type identifier to match or create.
     *  @param uuid        Identity to match or assign to the row.
     */
    PaneComponent (jam::Model& model,
                   juce::ValueTree parentState,
                   const juce::Identifier& type,
                   jam::UUID uuid)
        : OwnedComponent (model, parentState, type, uuid)
    {
        configureCornerButtons();
    }

    /** @brief Adopt an already-existing pane row and configure its four corner menus.
     *  @param model         Owning model; forwarded to OwnedComponent.
     *  @param existingState Row to bind to.
     */
    PaneComponent (jam::Model& model, juce::ValueTree existingState)
        : OwnedComponent (model, existingState)
    {
        configureCornerButtons();
    }

    /** @brief Default destructor. */
    ~PaneComponent() override = default;

    //==============================================================================
    /** @brief Paints the pane's focus-aware outline via StyleCustom::drawPaneOutline(). */
    void paint (juce::Graphics& g) override
    {
        static_cast<jam::StyleCustom&> (getLookAndFeel()).drawPaneOutline (g, *this);
    }

    /** @brief Lays out the base pane plus the four corner menu hit zones. */
    void resized() override
    {
        OwnedComponent::resized();

        const auto bounds { getLocalBounds() };

        topLeftMenu.setBounds (0, 0, cornerZoneExtent, cornerZoneExtent);
        topRightMenu.setBounds (bounds.getWidth() - cornerZoneExtent, 0, cornerZoneExtent, cornerZoneExtent);
        bottomLeftMenu.setBounds (0, bounds.getHeight() - cornerZoneExtent, cornerZoneExtent, cornerZoneExtent);
        bottomRightMenu.setBounds (bounds.getWidth() - cornerZoneExtent,
                                   bounds.getHeight() - cornerZoneExtent,
                                   cornerZoneExtent,
                                   cornerZoneExtent);
    }

    /** @brief Updates corner hover state as the pointer enters the pane. */
    void mouseEnter (const juce::MouseEvent& event) override { updateCornerHover (getCorner (event.getPosition())); }
    /** @brief Updates corner hover state as the pointer moves within the pane. */
    void mouseMove (const juce::MouseEvent& event) override { updateCornerHover (getCorner (event.getPosition())); }
    /** @brief Clears corner hover state once the pointer leaves the pane. */
    void mouseExit (const juce::MouseEvent&) override { updateCornerHover ({}); }

    /** @brief Right/context-clicking inside a corner zone opens that corner's menu. */
    void mouseDown (const juce::MouseEvent& event) override
    {
        if (event.mods.isPopupMenu())
        {
            const auto corner { getCorner (event.getPosition()) };

            if (not corner.isEmpty())
                getCornerMenu (corner).showMenu();
        }
    }

    /** @brief Resolves which corner zone, if any, contains position.
     *  @param position Point in this component's local coordinates.
     *  @return The zone's bounds when position falls within one of the four
     *          corner zones; an empty rectangle otherwise.
     */
    juce::Rectangle<int> getCorner (juce::Point<int> position) const noexcept
    {
        const auto bounds { getLocalBounds() };
        const bool left { position.getX() < cornerZoneExtent };
        const bool top { position.getY() < cornerZoneExtent };
        const bool right { position.getX() > bounds.getWidth() - cornerZoneExtent };
        const bool bottom { position.getY() > bounds.getHeight() - cornerZoneExtent };

        if ((left or right) and (top or bottom))
            return { left ? 0 : bounds.getWidth() - cornerZoneExtent,
                     top ? 0 : bounds.getHeight() - cornerZoneExtent,
                     cornerZoneExtent,
                     cornerZoneExtent };

        return {};
    }

    /** @brief Sets the menu-building factory shared by all four corner menus. */
    void setCornerMenuFactory (std::function<juce::PopupMenu()> factory)
    {
        topLeftMenu.setMenuFactory (factory);
        topRightMenu.setMenuFactory (factory);
        bottomLeftMenu.setMenuFactory (factory);
        bottomRightMenu.setMenuFactory (factory);
    }

    /** @brief Sets the result-handling action shared by all four corner menus. */
    void setCornerMenuAction (std::function<void (int)> action)
    {
        topLeftMenu.setAction (action);
        topRightMenu.setAction (action);
        bottomLeftMenu.setAction (action);
        bottomRightMenu.setAction (action);
    }

    /** @brief Minimum pane extent in pixels along either axis, enforced by
     *  MatrixComponent::shiftSeam() when a seam is dragged or a pane is
     *  reduced/expanded.
     */
    static constexpr int minimumPaneExtent { 80 };
    /** @brief Side length in pixels of each of the four corner hit zones. */
    static constexpr int cornerZoneExtent { 24 };
    /** @brief Inset in pixels between the pane's bounds and its outline
     *  stroke, consumed by StyleCustom::drawPaneOutline() alongside
     *  lineThickness.
     */
    static constexpr int edgePadding { 8 };
    /** @brief Corner radius in pixels of the outline stroke drawn by
     *  StyleCustom::drawPaneOutline().
     */
    static constexpr float cornerSize { 4.0f };
    /** @brief Stroke thickness in pixels of the outline drawn by
     *  StyleCustom::drawPaneOutline().
     */
    static constexpr float lineThickness { 1.0f };

private:
    /** @return The shared overflow-menu dots icon SVG source, loaded once. */
    static const juce::String& cornerMenuIconSVG()
    {
        static const juce::String svg { BinaryData::getString (files::paneCornerMenu) };
        return svg;
    }

    /** @brief Corner-zone button painting the overflow-menu dots icon on hover/press. */
    struct CornerButton : public juce::Button
    {
        /** @brief Constructs, assigning the button's component name. */
        explicit CornerButton (const juce::String& name) : juce::Button (name) {}

        /** @brief Paints the overflow-menu dots icon when hovered or pressed; otherwise draws nothing. */
        void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
        {
            if (shouldDrawButtonAsHighlighted or shouldDrawButtonAsDown)
                jam::ButtonSVG::paintSingleImage (g,
                                                  shouldDrawButtonAsHighlighted,
                                                  shouldDrawButtonAsDown,
                                                  *this,
                                                  cornerMenuIconSVG().toRawUTF8(),
                                                  jam::ButtonSVG::ColourMode::forceOff,
                                                  jam::Svg::PathStyle::fill);
        }
    };

    /** @brief Constructs and mounts all four corner menu buttons. */
    void configureCornerButtons()
    {
        configureCornerButton (topLeftMenu, "paneCornerTopLeft");
        configureCornerButton (topRightMenu, "paneCornerTopRight");
        configureCornerButton (bottomLeftMenu, "paneCornerBottomLeft");
        configureCornerButton (bottomRightMenu, "paneCornerBottomRight");
    }

    /** @brief Constructs and mounts a single corner menu's CornerButton. */
    void configureCornerButton (jam::ButtonMenu& menu, const juce::String& name)
    {
        menu.setButton (std::make_unique<CornerButton> (name));
        menu.setInterceptsMouseClicks (false, false);

        addAndMakeVisible (menu);
    }

    /** @return The corner menu whose bounds match corner; bottomRightMenu when none match. */
    jam::ButtonMenu& getCornerMenu (juce::Rectangle<int> corner) noexcept
    {
        if (corner == topLeftMenu.getBounds())
            return topLeftMenu;
        if (corner == topRightMenu.getBounds())
            return topRightMenu;
        if (corner == bottomLeftMenu.getBounds())
            return bottomLeftMenu;

        return bottomRightMenu;
    }

    /** @brief Sets each corner button's state to buttonOver when it matches corner, buttonNormal otherwise. */
    void updateCornerHover (juce::Rectangle<int> corner) noexcept
    {
        topLeftMenu.setButtonState (corner == topLeftMenu.getBounds() ? juce::Button::buttonOver : juce::Button::buttonNormal);
        topRightMenu.setButtonState (corner == topRightMenu.getBounds() ? juce::Button::buttonOver : juce::Button::buttonNormal);
        bottomLeftMenu.setButtonState (corner == bottomLeftMenu.getBounds() ? juce::Button::buttonOver : juce::Button::buttonNormal);
        bottomRightMenu.setButtonState (corner == bottomRightMenu.getBounds() ? juce::Button::buttonOver : juce::Button::buttonNormal);
    }

    jam::ButtonMenu topLeftMenu;     ///< Top-left corner overflow menu.
    jam::ButtonMenu topRightMenu;    ///< Top-right corner overflow menu.
    jam::ButtonMenu bottomLeftMenu;  ///< Bottom-left corner overflow menu.
    jam::ButtonMenu bottomRightMenu; ///< Bottom-right corner overflow menu.

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaneComponent)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
