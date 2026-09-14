/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/**
 * @file jam_PopupMenu.h
 * @brief Fork of juce::PopupMenu internals for native-glass menu window lifecycle.
 *
 * Verbatim fork of JUCE 8.0.13 juce_PopupMenu.cpp HelperClasses.
 * The fork exists to control the MenuWindow lifecycle so native compositor
 * blur (BackgroundBlur) can be applied on menu presentation.
 *
 * Changes from the JUCE original:
 *   - namespace juce -> namespace jam
 *   - PopupMenu::HelperClasses -> jam::PopupMenu (top-level struct)
 *   - All unforked JUCE types carry explicit juce:: prefix
 *   - menu.items (private) replaced with juce::PopupMenu::MenuItemIterator
 *   - menu.lookAndFeel (private) replaced with nullptr-returning fallback
 *     (see findLookAndFeel comment)
 *   - showWithOptionalCallback is a free function, not a member of juce::PopupMenu
 *   - showAsync convenience wrapper added
 */

#pragma once

// Some things to keep in mind when modifying this file:
// - Popup menus may be free-floating or parented. Make sure to test both!
// - Menus may open while the mouse button is down, in which case the following mouse-up may
//   trigger a hovered menu item if the mouse has moved since the menu was displayed.
// - Consider a long menu attached to a button. It's possible for a such a menu to open underneath
//   the mouse cursor. In this case, the menu item underneath the mouse should *not* be initially
//   selected or clickable. Instead, wait until the mouse cursor is moved, which we interpret as the
//   user signalling intent to trigger a menu item.
// - Menu items may be navigated with the cursor keys. The most recent input mechanism should
//   generally win, so pressing a cursor key should cause the mouse state to be ignored until
//   the mouse is next moved.
// - It's possible for menus to overlap, especially in the case of nested submenus. Of course,
//   clicking an overlapping menu should only trigger the topmost menu item.
// - Long menus must update properly when the mouse is completely stationary inside the scroll area
//   at the end of the menu. This means it's not sufficient to drive all menu updates from mouse
//   and keyboard input callbacks. Scrolling must be driven by some other periodic update mechanism
//   such as a timer.

namespace jam
{

namespace PopupMenuSettings
{
    const int scrollZone { 24 };
    const int dismissCommandId { 0x6287345f };

    static bool menuWasHiddenBecauseOfAppChange { false };
}

//==============================================================================
struct PopupMenu
{

// ItemComponent, MouseSourceState, and MenuWindow form a genuine three-way
// mutual reference: ItemComponent and MouseSourceState each hold a MenuWindow&,
// while MenuWindow owns ItemComponent and MouseSourceState instances. Defining
// ItemComponent and MouseSourceState first — each using MenuWindow only as a
// forward-declared reference type, never dereferenced until their own method
// bodies run — and defining MenuWindow last (by which point ItemComponent and
// MouseSourceState are already complete) resolves the cycle with a single
// forward declaration, the minimum a three-way reference cycle admits.
struct MenuWindow;

static bool canBeTriggered (const juce::PopupMenu::Item& item) noexcept
{
    return item.isEnabled
        and item.itemID != 0
        and not item.isSectionHeader
        and (item.customComponent == nullptr or item.customComponent->isTriggeredAutomatically());
}

static bool hasActiveSubMenu (const juce::PopupMenu::Item& item) noexcept
{
    // NOTE: original accessed item.subMenu->items.size() > 0 (private field).
    // juce::PopupMenu::getNumItems() is the nearest public equivalent; it skips
    // separators, but a menu containing only separators would not qualify as
    // "active" in any meaningful sense.
    return item.isEnabled
        and item.subMenu != nullptr
        and item.subMenu->getNumItems() > 0;
}

//==============================================================================
struct HeaderItemComponent final : public juce::PopupMenu::CustomComponent
{
    HeaderItemComponent (const juce::String& name, const juce::PopupMenu::Options& opts)
        : CustomComponent (false), options (opts)
    {
        setName (name);
    }

    void paint (juce::Graphics& g) override
    {
        getLookAndFeel().drawPopupMenuSectionHeaderWithOptions (g,
                                                                getLocalBounds(),
                                                                getName(),
                                                                options);
    }

    void getIdealSize (int& idealWidth, int& idealHeight) override
    {
        getLookAndFeel().getIdealPopupMenuSectionHeaderSizeWithOptions (getName(),
                                                                        -1,
                                                                        idealWidth,
                                                                        idealHeight,
                                                                        options);
    }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return createIgnoredAccessibilityHandler (*this);
    }

    const juce::PopupMenu::Options& options;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderItemComponent)
};

//==============================================================================
struct ItemComponent final : public juce::Component
{
    ItemComponent (const juce::PopupMenu::Item& i, const juce::PopupMenu::Options& o, MenuWindow& parent)
        : item (i), parentWindow (parent), options (o), customComp (i.customComponent)
    {
        if (item.isSectionHeader)
        {
            customComp = *new HeaderItemComponent (item.text, options);
            setEnabled (false);
        }

        if (customComp != nullptr)
        {
            // NOTE: juce::PopupMenu::setItem is a private static method (friend PopupMenu only).
            // It sets CustomComponent::item (private) then calls repaint(). Since we are not a
            // friend of juce::PopupMenu, we cannot set the item pointer. CustomComponent::getItem()
            // will return nullptr for components hosted in this fork — a documented fork limitation.
            // The repaint() is called below to match the original's side-effect.
            customComp->repaint();
            addAndMakeVisible (*customComp);
        }

        parent.addAndMakeVisible (this);

        updateShortcutKeyDescription();

        int itemW { 80 };
        int itemH { 16 };
        getIdealSize (itemW, itemH, options.getStandardItemHeight());
        setSize (itemW, juce::jlimit (1, 600, itemH));

        addMouseListener (&parent, false);
    }

    // juce::Component::~Component() already removes/detaches all child components,
    // so no explicit removeChildComponent() call is required here.
    ~ItemComponent() override = default;

    void getIdealSize (int& idealWidth, int& idealHeight, const int standardItemHeight)
    {
        if (customComp != nullptr)
            customComp->getIdealSize (idealWidth, idealHeight);
        else
            getLookAndFeel().getIdealPopupMenuItemSizeWithOptions (getTextForMeasurement(),
                                                                   item.isSeparator,
                                                                   standardItemHeight,
                                                                   idealWidth, idealHeight,
                                                                   options);
    }

    void paint (juce::Graphics& g) override
    {
        if (customComp == nullptr)
            getLookAndFeel().drawPopupMenuItemWithOptions (g, getLocalBounds(),
                                                           isHighlighted,
                                                           item,
                                                           options);
    }

    void resized() override
    {
        if (auto* child = getChildComponent (0))
        {
            const auto border { getLookAndFeel().getPopupMenuBorderSizeWithOptions (options) };
            child->setBounds (getLocalBounds().reduced (border, 0));
        }
    }

    void setHighlighted (bool shouldBeHighlighted)
    {
        shouldBeHighlighted = shouldBeHighlighted and item.isEnabled;

        if (isHighlighted != shouldBeHighlighted)
        {
            isHighlighted = shouldBeHighlighted;

            if (customComp != nullptr)
                customComp->setHighlighted (shouldBeHighlighted);

            if (isHighlighted)
                if (auto* handler = getAccessibilityHandler())
                    handler->grabFocus();

            repaint();
        }
    }

    static bool isAccessibilityHandlerRequired (const juce::PopupMenu::Item& item)
    {
        return item.isSectionHeader or hasActiveSubMenu (item) or canBeTriggered (item);
    }

    juce::PopupMenu::Item item;

private:
    //==============================================================================
    class ItemAccessibilityHandler final : public juce::AccessibilityHandler
    {
    public:
        explicit ItemAccessibilityHandler (ItemComponent& itemComponentToWrap)
            : AccessibilityHandler (itemComponentToWrap,
                                    isAccessibilityHandlerRequired (itemComponentToWrap.item) ? juce::AccessibilityRole::menuItem
                                                                                              : juce::AccessibilityRole::ignored,
                                    getAccessibilityActions (*this, itemComponentToWrap)),
              itemComponent (itemComponentToWrap)
        {
        }

        juce::String getTitle() const override
        {
            return itemComponent.item.text;
        }

        juce::AccessibleState getCurrentState() const override
        {
            auto state { AccessibilityHandler::getCurrentState().withSelectable()
                                                                .withAccessibleOffscreen() };

            if (hasActiveSubMenu (itemComponent.item))
            {
                state = itemComponent.parentWindow.isSubMenuVisible() ? state.withExpandable().withExpanded()
                                                                      : state.withExpandable().withCollapsed();
            }

            if (itemComponent.item.isTicked)
                state = state.withCheckable().withChecked();

            return state.isFocused() ? state.withSelected() : state;
        }

    private:
        static juce::AccessibilityActions getAccessibilityActions (ItemAccessibilityHandler&,
                                                                   ItemComponent& item)
        {
            auto onFocus = [&item]
            {
                item.parentWindow.disableMouseMovesOnMenuAndAncestors();
                item.parentWindow.ensureItemComponentIsVisible (item, std::nullopt);
                item.parentWindow.setCurrentlyHighlightedChild (&item);
            };

            auto actions { juce::AccessibilityActions().addAction (juce::AccessibilityActionType::focus, std::move (onFocus)) };

            if (canBeTriggered (item.item))
            {
                actions.addAction (juce::AccessibilityActionType::press, [&item]
                {
                    item.parentWindow.setCurrentlyHighlightedChild (&item);
                    item.parentWindow.triggerCurrentlyHighlightedItem();
                });
            }

            if (hasActiveSubMenu (item.item))
            {
                auto showSubMenu = [&item]
                {
                    item.parentWindow.showSubMenuFor (&item);

                    if (auto* subMenu = item.parentWindow.activeSubMenu.get())
                        subMenu->setCurrentlyHighlightedChild (subMenu->items.isEmpty() ? nullptr : subMenu->items.begin()->get());
                };

                actions.addAction (juce::AccessibilityActionType::press,    showSubMenu);
                actions.addAction (juce::AccessibilityActionType::showMenu, showSubMenu);
            }

            return actions;
        }

        ItemComponent& itemComponent;
    };

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return item.isSeparator ? createIgnoredAccessibilityHandler (*this)
                                : std::make_unique<ItemAccessibilityHandler> (*this);
    }

    //==============================================================================
    MenuWindow& parentWindow;
    const juce::PopupMenu::Options& options;
    // NB: we use a copy of the one from the item info in case we're using our own section comp
    juce::ReferenceCountedObjectPtr<juce::PopupMenu::CustomComponent> customComp;
    bool isHighlighted { false };

    void updateShortcutKeyDescription()
    {
        if (item.commandManager != nullptr
             and item.itemID != 0
             and item.shortcutKeyDescription.isEmpty())
        {
            juce::String shortcutKey;

            for (auto& keypress : item.commandManager->getKeyMappings()
                                    ->getKeyPressesAssignedToCommand (item.itemID))
            {
                auto key { keypress.getTextDescriptionWithIcons() };

                if (shortcutKey.isNotEmpty())
                    shortcutKey << ", ";

                if (key.length() == 1 and key[0] < 128)
                    shortcutKey << "shortcut: '" << key << '\'';
                else
                    shortcutKey << key;
            }

            item.shortcutKeyDescription = shortcutKey.trim();
        }
    }

    juce::String getTextForMeasurement() const
    {
        return item.shortcutKeyDescription.isNotEmpty() ? item.text + "   " + item.shortcutKeyDescription
                                                        : item.text;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ItemComponent)
};

//==============================================================================
class MouseSourceState final : private juce::Timer
{
public:
    MouseSourceState (MenuWindow& w, juce::MouseInputSource s)
        : window (w), source (s), lastScrollTime (juce::Time::getMillisecondCounter())
    {
        startTimerHz (20);
    }

    // juce::Timer::~Timer() already calls stopTimer(), so no explicit call is required here.
    ~MouseSourceState() override = default;

    void handleMouseEventWithPosition (const juce::Point<int>& e)
    {
        if (window.windowIsStillValid())
        {
            startTimerHz (20);
            handleMousePosition (e);
        }
    }

    bool isOver() const
    {
        return window.reallyContains (window.getLocalPoint (nullptr, source.getScreenPosition()).roundToInt(), true);
    }

    using juce::Timer::stopTimer;

    MenuWindow& window;
    juce::MouseInputSource source;

private:
    juce::Point<int> lastMousePos;
    double scrollAcceleration { 0.0 };
    juce::uint32 lastScrollTime { 0 };
    juce::uint32 lastMoveTime { 0 };
    bool isDown { false };

    // Although most mouse movements can be handled inside mouse event callbacks, scrolling of menus
    // may happen while the mouse is not moving, so periodic timer callbacks are required in this
    // scenario.
    void timerCallback() override
    {
       #if JUCE_WINDOWS
        // touch and pen devices on Windows send an offscreen mouse move after mouse up events
        // but we don't want to forward these on as they will dismiss the menu
        if ((not source.isTouch() and not source.isPen()) or isValidMousePosition())
            handleMouseEventWithPosition (source.getScreenPosition().roundToInt());
       #else
        handleMouseEventWithPosition (source.getScreenPosition().roundToInt());
       #endif
    }

    void handleMousePosition (juce::Point<int> globalMousePos)
    {
        auto localMousePos { window.getLocalPoint (nullptr, globalMousePos) };
        auto timeNow { juce::Time::getMillisecondCounter() };

        if (timeNow > window.timeEnteredCurrentChildComp + 100
             and window.reallyContains (localMousePos, true)
             and window.currentChild != nullptr
             and not (window.disableMouseMoves or window.isSubMenuVisible()))
        {
            window.showSubMenuFor (window.currentChild);
        }

        highlightItemUnderMouse (globalMousePos, localMousePos, timeNow);

        const bool overScrollArea { scrollIfNecessary (localMousePos, timeNow) };
        const bool isOverAny { window.isOverAnyMenu() };

        if (window.hideOnExit and window.mouseHasBeenOver() and not isOverAny)
            window.hide (nullptr, true);
        else
            checkButtonState (localMousePos, timeNow, isDown, overScrollArea, isOverAny);
    }

    void checkButtonState (juce::Point<int> localMousePos, const juce::uint32 timeNow,
                           const bool wasDown, const bool overScrollArea, const bool isOverAny)
    {
        isDown = window.mouseHasBeenOver()
                    and (juce::ModifierKeys::getCurrentModifiers().isAnyMouseButtonDown()
                         or juce::ComponentPeer::getCurrentModifiersRealtime().isAnyMouseButtonDown());

        const auto reallyContained { window.reallyContains (localMousePos, true) };

        if (not window.doesAnyJuceCompHaveFocus() and not reallyContained)
        {
            if (timeNow > window.lastFocusedTime + 10)
            {
                PopupMenuSettings::menuWasHiddenBecauseOfAppChange = true;
                window.dismissMenu (nullptr);
                // Note: This object may have been deleted by the previous call.
            }
        }
        else if (wasDown and timeNow > window.windowCreationTime + 250 and not isDown and not overScrollArea)
        {
            if (reallyContained and window.allowMouseUpToTriggerItem())
                window.triggerCurrentlyHighlightedItem();
            else if ((window.mouseHasBeenOver() or not window.allowMouseUpToTriggerItem()) and not isOverAny)
                window.dismissMenu (nullptr);

            // Note: This object may have been deleted by the previous call.
        }
        else
        {
            window.lastFocusedTime = timeNow;
        }
    }

    void highlightItemUnderMouse (juce::Point<int> globalMousePos, juce::Point<int> localMousePos, juce::uint32 timeNow)
    {
        const auto mouseTimedOut { lastMoveTime != 0 and 350 < (timeNow - lastMoveTime) };
        const auto mouseHasMoved { 2 < lastMousePos.getDistanceFrom (globalMousePos) };
        const auto isMouseOver { window.reallyContains (localMousePos, true) };

        if (mouseHasMoved and isMouseOver)
        {
            window.disableMouseMoves = false;
            lastMoveTime = timeNow;
        }

        if (not (not mouseHasMoved and not mouseTimedOut))
        {
            if (not window.disableMouseMoves)
            {
                if (not (window.activeSubMenu != nullptr and window.activeSubMenu->isOverChildren()))
                {
                    const auto isMovingTowardsMenu { isMouseOver
                                                  and globalMousePos != lastMousePos
                                                  and isMovingTowardsSubmenu (globalMousePos) };

                    lastMousePos = globalMousePos;

                    if (not isMovingTowardsMenu)
                    {
                        auto* componentUnderMouse { window.getComponentAt (localMousePos) };
                        auto* childComponentUnderMouse { componentUnderMouse != &window ? componentUnderMouse : nullptr };

                        auto* itemUnderMouse { std::invoke ([&]() -> ItemComponent*
                        {
                            if (auto* candidate = dynamic_cast<ItemComponent*> (childComponentUnderMouse))
                                return candidate;

                            if (childComponentUnderMouse != nullptr)
                                return childComponentUnderMouse->findParentComponentOfClass<ItemComponent>();

                            return nullptr;
                        }) };

                        if (itemUnderMouse != window.currentChild)
                        {
                            if (not (not isMouseOver and window.activeSubMenu != nullptr and window.activeSubMenu->isVisible()))
                            {
                                if (isMouseOver and childComponentUnderMouse != nullptr and window.activeSubMenu != nullptr)
                                    window.activeSubMenu->hide (nullptr, true);

                                if (not (not isMouseOver and not window.mouseHasBeenOver()))
                                {
                                    window.setCurrentlyHighlightedChild (isMouseOver ? itemUnderMouse : nullptr);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    bool isMovingTowardsSubmenu (juce::Point<int> newGlobalPos) const
    {
        bool result { false };

        if (window.activeSubMenu != nullptr)
        {
            // try to intelligently guess whether the user is moving the mouse towards a currently-open
            // submenu. To do this, look at whether the mouse stays inside a triangular region that
            // extends from the last mouse pos to the submenu's rectangle

            auto itemScreenBounds { window.activeSubMenu->getScreenBounds() };
            auto subX { static_cast<float> (itemScreenBounds.getX()) };

            auto oldGlobalPos { lastMousePos };

            if (itemScreenBounds.getX() > window.getX())
            {
                oldGlobalPos -= juce::Point<int> (2, 0);  // to enlarge the triangle a bit, in case the mouse only moves a couple of pixels
            }
            else
            {
                oldGlobalPos += juce::Point<int> (2, 0);
                subX += static_cast<float> (itemScreenBounds.getWidth());
            }

            juce::Path areaTowardsSubMenu;
            areaTowardsSubMenu.addTriangle (static_cast<float> (oldGlobalPos.x), static_cast<float> (oldGlobalPos.y),
                                            subX, static_cast<float> (itemScreenBounds.getY()),
                                            subX, static_cast<float> (itemScreenBounds.getBottom()));

            result = areaTowardsSubMenu.contains (newGlobalPos.toFloat());
        }

        return result;
    }

    bool scrollIfNecessary (juce::Point<int> localMousePos, const juce::uint32 timeNow)
    {
        if (window.canScroll()
             and juce::isPositiveAndBelow (localMousePos.x, window.getWidth())
             and (juce::isPositiveAndBelow (localMousePos.y, window.getHeight()) or source.isDragging()))
        {
            if (window.isTopScrollZoneActive() and localMousePos.y < PopupMenuSettings::scrollZone)
                return scroll (timeNow, -1);

            if (window.isBottomScrollZoneActive() and localMousePos.y > window.getHeight() - PopupMenuSettings::scrollZone)
                return scroll (timeNow, 1);
        }

        scrollAcceleration = 1.0;
        return false;
    }

    bool scroll (const juce::uint32 timeNow, const int direction)
    {
        if (timeNow > lastScrollTime + 20)
        {
            scrollAcceleration = juce::jmin (4.0, scrollAcceleration * 1.04);
            int amount { 0 };

            for (int i { 0 }; i < window.items.size() and amount == 0; ++i)
                amount = (static_cast<int> (scrollAcceleration)) * window.items.at (static_cast<size_t> (i))->getHeight();

            window.alterChildYPos (amount * direction);
            lastScrollTime = timeNow;
        }

        return true;
    }

   #if JUCE_WINDOWS
    bool isValidMousePosition()
    {
        auto screenPos { source.getScreenPosition() };
        auto localPos { (window.activeSubMenu == nullptr) ? window.getLocalPoint (nullptr, screenPos)
                                                          : window.activeSubMenu->getLocalPoint (nullptr, screenPos) };

        return not (localPos.x < 0 and localPos.y < 0);
    }
   #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MouseSourceState)
};

//==============================================================================
struct MenuWindow final : public juce::Component
{
    MenuWindow (const juce::PopupMenu& menu,
                MenuWindow* parentWindow,
                juce::PopupMenu::Options opts,
                bool alignToRectangle,
                juce::ApplicationCommandManager*& manager,
                float parentScaleFactor = 1.0f)
        : Component ("menu"),
          parent (parentWindow),
          options (opts.withParentComponent (findNonNullLookAndFeel (menu, parentWindow).getParentComponentForMenuOptions (opts))),
          managerOfChosenCommand (manager),
          componentAttachedTo (options.getTargetComponent()),
          windowCreationTime (juce::Time::getMillisecondCounter()),
          lastFocusedTime (windowCreationTime),
          timeEnteredCurrentChildComp (windowCreationTime),
          scaleFactor (parentWindow != nullptr ? parentScaleFactor : 1.0f)
    {
        setWantsKeyboardFocus (false);
        setMouseClickGrabsKeyboardFocus (false);
        setAlwaysOnTop (true);
        setFocusContainerType (juce::Component::FocusContainerType::focusContainer);

        setLookAndFeel (findLookAndFeel (menu, parentWindow));

        auto& lf { getLookAndFeel() };

        if (auto* pc = options.getParentComponent())
        {
            pc->addChildComponent (this);
        }
        else
        {
            const auto shouldDisableAccessibility { std::invoke ([this]
            {
                const auto* compToCheck { parent != nullptr ? parent
                                                            : options.getTargetComponent() };

                return compToCheck != nullptr and not compToCheck->isAccessible();
            }) };

            if (shouldDisableAccessibility)
                setAccessible (false);

            addToDesktop (juce::ComponentPeer::windowIsTemporary
                          | juce::ComponentPeer::windowIgnoresKeyPresses
                          | lf.getMenuWindowFlags());
        }

        // Using a global mouse listener means that we get notifications about all mouse events.
        // Without this, drags that are started on a button that displays a menu won't reach the
        // menu, because they *only* target the component that initiated the drag interaction.
        juce::Desktop::getInstance().addGlobalMouseListener (this);

        scaleFactor = std::invoke ([&]
        {
            if (options.getParentComponent() != nullptr)
                return scaleFactor;

            if (parentWindow != nullptr)
                return scaleFactor;

            if (not lf.shouldPopupMenuScaleWithTargetComponent (options))
                return scaleFactor;

            auto* targetComponent { options.getTargetComponent() };

            if (targetComponent == nullptr)
                return scaleFactor;

            const auto baseScale { getApproximateScaleFactorForComponent (targetComponent) };
            const auto targetScale { std::invoke ([&]
            {
                if (auto* targetPeer = targetComponent->getPeer())
                    return targetPeer->getPlatformScaleFactor();

                return 1.0;
            }) };

            // Move the menu window's peer to the screen where it will display so that we can
            // retrieve the peer's native scale there.
            // The final position will be computed and applied later on.

            const juce::ScopeGuard scope { [this, pos = getPosition()] { setTopLeftPosition (pos.x, pos.y); } };
            setTopLeftPosition (options.getTargetScreenArea().getCentre());

            const auto selfScale { std::invoke ([&]
            {
                if (auto* selfPeer = getPeer())
                    return static_cast<float> (selfPeer->getPlatformScaleFactor());

                return 1.0f;
            }) };

            return baseScale * static_cast<float> (targetScale) / static_cast<float> (selfScale);
        });

        setOpaque (lf.findColour (juce::PopupMenu::backgroundColourId).isOpaque()
                     or not juce::Desktop::canUseSemiTransparentWindows());

        // NOTE: original iterated menu.items directly (private Array<Item>).
        // We collect items via MenuItemIterator, respecting the original logic:
        // skip the item only if it is a trailing separator (last-item sentinel).
        {
            std::vector<juce::PopupMenu::Item> collectedItems;
            {
                juce::PopupMenu::MenuItemIterator iter (menu, false);
                while (iter.next())
                    collectedItems.push_back (iter.getItem());
            }

            const auto initialSelectedId { options.getInitiallySelectedItemId() };
            const int totalItems { static_cast<int> (collectedItems.size()) };

            for (int i { 0 }; i < totalItems; ++i)
            {
                auto& menuItem { collectedItems.at (static_cast<size_t> (i)) };

                if (i + 1 < totalItems or not menuItem.isSeparator)
                {
                    auto* child { items.add (std::make_unique<ItemComponent> (menuItem, options, *this)).get() };
                    child->setExplicitFocusOrder (1 + i);

                    if (initialSelectedId != 0 and menuItem.itemID == initialSelectedId)
                        setCurrentlyHighlightedChild (child);
                }
            }
        }

        auto targetArea { options.getTargetScreenArea() / scaleFactor };

        calculateWindowPos (targetArea, alignToRectangle);
        setTopLeftPosition (windowPos.getPosition());

        if (auto visibleID = options.getItemThatMustBeVisible())
        {
            const auto iter { std::find_if (items.begin(), items.end(), [&] (const auto& item)
            {
                return item->item.itemID == visibleID;
            }) };

            if (iter != items.end())
            {
                const auto targetPosition { std::invoke ([&]
                {
                    if (auto* pc = options.getParentComponent())
                        return pc->getLocalPoint (nullptr, targetArea.getTopLeft());

                    return targetArea.getTopLeft();
                }) };

                ensureItemComponentIsVisible (**iter, targetPosition.getY() - windowPos.getY());
            }
        }

        resizeToBestWindowPos();

        getActiveWindows().add (this);

        lf.preparePopupMenuWindow (*this);

        getMouseState (juce::Desktop::getInstance().getMainMouseSource()); // forces creation of a mouse source watcher for the main mouse
    }

    ~MenuWindow() override
    {
        vulkanEngine.removePeer (*this);

        getActiveWindows().removeFirstMatchingValue (this);
        juce::Desktop::getInstance().removeGlobalMouseListener (this);
    }

    //==============================================================================
    void paint (juce::Graphics& g) override
    {
        if (isOpaque())
            g.fillAll (juce::Colours::white);

        auto& theme { getLookAndFeel() };
        theme.drawPopupMenuBackgroundWithOptions (g, getWidth(), getHeight(), options);

        if (not columnWidths.isEmpty())
        {
            const auto separatorWidth { theme.getPopupMenuColumnSeparatorWidthWithOptions (options) };
            const auto border { theme.getPopupMenuBorderSizeWithOptions (options) };

            auto currentX { 0 };

            std::for_each (columnWidths.begin(), std::prev (columnWidths.end()), [&] (int width)
            {
                const juce::Rectangle<int> separator (currentX + width,
                                                border,
                                                separatorWidth,
                                                getHeight() - border * 2);
                theme.drawPopupMenuColumnSeparatorWithOptions (g, separator, options);
                currentX += width + separatorWidth;
            });
        }
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        auto& lf { getLookAndFeel() };

        if (options.getParentComponent() != nullptr)
            lf.drawResizableFrame (g, getWidth(), getHeight(),
                                   juce::BorderSize<int> (getLookAndFeel().getPopupMenuBorderSizeWithOptions (options)));

        if (canScroll())
        {
            if (isTopScrollZoneActive())
            {
                lf.drawPopupMenuUpDownArrowWithOptions (g,
                                                        getWidth(),
                                                        PopupMenuSettings::scrollZone,
                                                        true,
                                                        options);
            }

            if (isBottomScrollZoneActive())
            {
                g.setOrigin (0, getHeight() - PopupMenuSettings::scrollZone);
                lf.drawPopupMenuUpDownArrowWithOptions (g,
                                                        getWidth(),
                                                        PopupMenuSettings::scrollZone,
                                                        false,
                                                        options);
            }
        }
    }

    //==============================================================================
    // hide this and all sub-comps
    void hide (const juce::PopupMenu::Item* item, bool makeInvisible)
    {
        if (isVisible())
        {
            juce::WeakReference<juce::Component> deletionChecker { this };

            activeSubMenu.reset();
            currentChild = nullptr;

            if (item != nullptr
                 and item->commandManager != nullptr
                 and item->itemID != 0)
            {
                managerOfChosenCommand = item->commandManager;
            }

            auto resultID { options.hasWatchedComponentBeenDeleted() ? 0 : getResultItemID (item) };

            exitModalState (resultID);

            if (deletionChecker != nullptr)
            {
                exitingModalState = true;

                if (makeInvisible)
                    setVisible (false);
            }

            if (resultID != 0
                 and item != nullptr
                 and item->action != nullptr)
                juce::MessageManager::callAsync (item->action);
        }
    }

    static int getResultItemID (const juce::PopupMenu::Item* item)
    {
        int result { 0 };

        if (item != nullptr)
        {
            result = item->itemID;

            if (auto* cc = item->customCallback.get())
            {
                if (not cc->menuItemTriggered())
                    result = 0;
            }
        }

        return result;
    }

    void dismissMenu (const juce::PopupMenu::Item* item)
    {
        if (parent != nullptr)
        {
            parent->dismissMenu (item);
        }
        else
        {
            if (item != nullptr)
            {
                // need a copy of this on the stack as the one passed in will get deleted during this call
                auto mi (*item);
                hide (&mi, false);
            }
            else
            {
                hide (nullptr, true);
            }
        }
    }

    float getDesktopScaleFactor() const override    { return scaleFactor * juce::Desktop::getInstance().getGlobalScaleFactor(); }

    void visibilityChanged() override
    {
        if (isShowing())
        {
            auto* accessibleFocus { [this]
            {
              if (currentChild != nullptr)
                  if (auto* childHandler = currentChild->getAccessibilityHandler())
                      return childHandler;

                return getAccessibilityHandler();
            }() };

            if (accessibleFocus != nullptr)
                accessibleFocus->grabFocus();
        }
    }

    //==============================================================================
    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key.isKeyCode (juce::KeyPress::downKey))
        {
            selectNextItem (MenuSelectionDirection::forwards);
        }
        else if (key.isKeyCode (juce::KeyPress::upKey))
        {
            selectNextItem (MenuSelectionDirection::backwards);
        }
        else if (key.isKeyCode (juce::KeyPress::leftKey))
        {
            if (parent != nullptr)
            {
                juce::Component::SafePointer<MenuWindow> parentWindow { parent };
                ItemComponent* currentChildOfParent { parentWindow->currentChild };

                hide (nullptr, true);

                if (parentWindow != nullptr)
                    parentWindow->setCurrentlyHighlightedChild (currentChildOfParent);

                disableMouseMovesOnMenuAndAncestors();
            }
            else if (componentAttachedTo != nullptr)
            {
                componentAttachedTo->keyPressed (key);
            }
        }
        else if (key.isKeyCode (juce::KeyPress::rightKey))
        {
            disableMouseMovesOnMenuAndAncestors();

            if (showSubMenuFor (currentChild))
            {
                if (isSubMenuVisible())
                    activeSubMenu->selectNextItem (MenuSelectionDirection::current);
            }
            else if (componentAttachedTo != nullptr)
            {
                componentAttachedTo->keyPressed (key);
            }
        }
        else if (key.isKeyCode (juce::KeyPress::returnKey) or key.isKeyCode (juce::KeyPress::spaceKey))
        {
            triggerCurrentlyHighlightedItem();
        }
        else if (key.isKeyCode (juce::KeyPress::escapeKey))
        {
            dismissMenu (nullptr);
        }
        else
        {
            return false;
        }

        return true;
    }

    void inputAttemptWhenModal() override
    {
        juce::WeakReference<juce::Component> deletionChecker { this };

        const auto deletedDuringNotification { std::any_of (mouseSourceStates.begin(), mouseSourceStates.end(),
            [&deletionChecker] (const std::unique_ptr<MouseSourceState>& ms)
            {
                ms->handleMouseEventWithPosition (ms->source.getScreenPosition().roundToInt());
                return deletionChecker == nullptr;
            }) };

        if (not deletedDuringNotification)
        {
            if (not isOverAnyMenu())
            {
                if (componentAttachedTo != nullptr)
                {
                    // we want to dismiss the menu, but if we do it synchronously, then
                    // the mouse-click will be allowed to pass through. That's good, except
                    // when the user clicks on the button that originally popped the menu up,
                    // as they'll expect the menu to go away, and in fact it'll just
                    // come back. So only dismiss synchronously if they're not on the original
                    // comp that we're attached to.
                    auto mousePos { componentAttachedTo->getMouseXYRelative() };

                    if (not componentAttachedTo->reallyContains (mousePos, true))
                    {
                        dismissMenu (nullptr);
                    }
                    else
                    {
                        postCommandMessage (PopupMenuSettings::dismissCommandId); // dismiss asynchronously
                    }
                }
                else
                {
                    dismissMenu (nullptr);
                }
            }
        }
    }

    void handleCommandMessage (int commandId) override
    {
        juce::Component::handleCommandMessage (commandId);

        if (commandId == PopupMenuSettings::dismissCommandId)
            dismissMenu (nullptr);
    }

    //==============================================================================
    void mouseDown  (const juce::MouseEvent& e) override    { handleMouseEvent (e); }

    void mouseUp (const juce::MouseEvent& e) override
    {
        juce::Component::SafePointer<MenuWindow> self { this };

        handleMouseEvent (e);

        // Check whether this menu was deleted as a result of the mouse being released.
        if (self != nullptr)
        {
            // If the mouse was down when the menu was created, releasing the mouse should
            // not trigger the item under the mouse, because we might still be handling the click
            // that caused the menu to show in the first place. Once the mouse has been released once,
            // then the user must have clicked the mouse again, so they are attempting to trigger or
            // dismiss the menu.
            mouseUpCanTrigger |= true;
        }
    }

    // Any move/drag after the menu is created will allow the mouse to trigger a highlighted item
    void mouseDrag  (const juce::MouseEvent& e) override    { mouseUpCanTrigger |= true; handleMouseEvent (e); }
    void mouseMove  (const juce::MouseEvent& e) override    { mouseUpCanTrigger |= true; handleMouseEvent (e); }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        alterChildYPos (juce::roundToInt (-10.0f * wheel.deltaY * PopupMenuSettings::scrollZone));
    }

    bool windowIsStillValid()
    {
        bool result { false };

        if (isVisible())
        {
            if (componentAttachedTo == options.getTargetComponent())
            {
                auto* currentlyModalWindow { dynamic_cast<MenuWindow*> (juce::Component::getCurrentlyModalComponent()) };

                if (not (currentlyModalWindow != nullptr and not treeContains (currentlyModalWindow)))
                {
                    if (not exitingModalState)
                    {
                        result = true;
                    }
                }
            }
            else
            {
                dismissMenu (nullptr);
            }
        }

        return result;
    }

    static juce::Array<MenuWindow*>& getActiveWindows()
    {
        static juce::Array<MenuWindow*> activeMenuWindows;
        return activeMenuWindows;
    }

    MouseSourceState& getMouseState (juce::MouseInputSource source)
    {
        MouseSourceState* mouseState { nullptr };

        for (const auto& ms : mouseSourceStates)
        {
            if (ms->source == source)
                mouseState = ms.get();
            else if (ms->source.getType() != source.getType())
                ms->stopTimer();
        }

        if (mouseState == nullptr)
            mouseState = mouseSourceStates.add (std::make_unique<MouseSourceState> (*this, source)).get();

        return *mouseState;
    }

    //==============================================================================
    bool isOverAnyMenu() const
    {
        return parent != nullptr ? parent->isOverAnyMenu()
                                 : isOverChildren();
    }

    bool isOverChildren() const
    {
        return isVisible()
                and (isAnyMouseOver() or (activeSubMenu != nullptr and activeSubMenu->isOverChildren()));
    }

    bool isAnyMouseOver() const
    {
        for (const auto& ms : mouseSourceStates)
            if (ms->isOver())
                return true;

        return false;
    }

    bool treeContains (const MenuWindow* const window) const noexcept
    {
        auto* mw { this };

        while (mw->parent != nullptr)
            mw = mw->parent;

        while (mw != nullptr)
        {
            if (mw == window)
                return true;

            mw = mw->activeSubMenu.get();
        }

        return false;
    }

    bool doesAnyJuceCompHaveFocus()
    {
        // NOTE: juce::detail::WindowingHelpers::isForegroundOrEmbeddedProcess is JUCE-internal
        // (juce_WindowingHelpers.h). Its body is:
        //   return Process::isForegroundProcess() || isEmbeddedInForegroundProcess (viewComponent);
        // isEmbeddedInForegroundProcess is also internal. We use the public isForegroundProcess()
        // half only. The embedded-process path (plugins hosted in a DAW peer) is lost; that case
        // would incorrectly dismiss the menu when the DAW peer owns focus. Acceptable for now —
        // the original behaviour is unachievable without the internal API.
        if (juce::Process::isForegroundProcess())
        {
            if (juce::Component::getCurrentlyFocusedComponent() != nullptr)
                return true;

            for (int i { juce::ComponentPeer::getNumPeers() }; --i >= 0;)
            {
                if (juce::ComponentPeer::getPeer (i)->isFocused())
                {
                    hasAnyJuceCompHadFocus = true;
                    return true;
                }
            }

            return not hasAnyJuceCompHadFocus;
        }

        return false;
    }

    //==============================================================================
    juce::Rectangle<int> getParentArea (juce::Point<int> targetPoint, juce::Component* relativeTo = nullptr)
    {
        if (relativeTo != nullptr)
            targetPoint = relativeTo->localPointToGlobal (targetPoint);

        auto* display { juce::Desktop::getInstance().getDisplays().getDisplayForPoint (targetPoint.toFloat() * scaleFactor) };
        const auto intBorder { display->safeAreaInsets };
        const juce::BorderSize<float> floatBorder (static_cast<float> (intBorder.getTop()),
                                      static_cast<float> (intBorder.getLeft()),
                                      static_cast<float> (intBorder.getBottom()),
                                      static_cast<float> (intBorder.getRight()));
        auto parentArea { display->userBounds.getIntersection (floatBorder.subtractedFrom (display->logicalBounds)) };

        if (auto* pc = options.getParentComponent())
        {
            return pc->getLocalArea (nullptr,
                                     pc->getScreenBounds().toFloat()
                                           .reduced (static_cast<float> (getLookAndFeel().getPopupMenuBorderSizeWithOptions (options)))
                                           .getIntersection (parentArea)).getLargestIntegerWithin();
        }

        return parentArea.toNearestInt();
    }

    void calculateWindowPos (juce::Rectangle<int> target, const bool alignToRectangle)
    {
        auto parentArea { getParentArea (target.getCentre()) / scaleFactor };

        if (auto* pc = options.getParentComponent())
            target = pc->getLocalArea (nullptr, target).constrainedWithin (parentArea);

        auto maxMenuHeight { parentArea.getHeight() - 24 };

        int x { 0 };
        int y { 0 };
        int widthToUse { 0 };
        int heightToUse { 0 };
        layoutMenuItems (parentArea.getWidth() - 24, maxMenuHeight, widthToUse, heightToUse);

        if (alignToRectangle)
        {
            x = target.getX();

            auto spaceUnder { parentArea.getBottom() - target.getBottom() };
            auto spaceOver { target.getY() - parentArea.getY() };
            auto bufferHeight { 30 };

            if (options.getPreferredPopupDirection() == juce::PopupMenu::Options::PopupDirection::upwards)
                y = (heightToUse < spaceOver - bufferHeight  or spaceOver >= spaceUnder) ? target.getY() - heightToUse
                                                                                         : target.getBottom();
            else
                y = (heightToUse < spaceUnder - bufferHeight or spaceUnder >= spaceOver) ? target.getBottom()
                                                                                         : target.getY() - heightToUse;
        }
        else
        {
            bool tendTowardsRight { target.getCentreX() < parentArea.getCentreX() };

            if (parent != nullptr)
            {
                if (parent->parent != nullptr)
                {
                    const bool parentGoingRight { (parent->getX() + parent->getWidth() / 2
                                                    > parent->parent->getX() + parent->parent->getWidth() / 2) };

                    if (parentGoingRight and target.getRight() + widthToUse < parentArea.getRight() - 4)
                        tendTowardsRight = true;
                    else if ((not parentGoingRight) and target.getX() > widthToUse + 4)
                        tendTowardsRight = false;
                }
                else if (target.getRight() + widthToUse < parentArea.getRight() - 32)
                {
                    tendTowardsRight = true;
                }
            }

            auto biggestSpace { juce::jmax (parentArea.getRight() - target.getRight(),
                                      target.getX() - parentArea.getX()) - 32 };

            if (biggestSpace < widthToUse)
            {
                layoutMenuItems (biggestSpace + target.getWidth() / 3, maxMenuHeight, widthToUse, heightToUse);

                if (numColumns > 1)
                    layoutMenuItems (biggestSpace - 4, maxMenuHeight, widthToUse, heightToUse);

                tendTowardsRight = (parentArea.getRight() - target.getRight()) >= (target.getX() - parentArea.getX());
            }

            x = tendTowardsRight ? juce::jmin (parentArea.getRight() - widthToUse - 4, target.getRight())
                                 : juce::jmax (parentArea.getX() + 4, target.getX() - widthToUse);

            if (getLookAndFeel().getPopupMenuBorderSizeWithOptions (options) == 0) // workaround for dismissing the window on mouse up when border size is 0
                x += tendTowardsRight ? 1 : -1;

            const auto border { getLookAndFeel().getPopupMenuBorderSizeWithOptions (options) };
            y = target.getCentreY() > parentArea.getCentreY() ? juce::jmax (parentArea.getY(), target.getBottom() - heightToUse) + border
                                                              : target.getY() - border;
        }

        x = juce::jmax (parentArea.getX() + 1, juce::jmin (parentArea.getRight()  - (widthToUse  + 6), x));
        y = juce::jmax (parentArea.getY() + 1, juce::jmin (parentArea.getBottom() - (heightToUse + 6), y));

        windowPos.setBounds (x, y, widthToUse, heightToUse);

        // sets this flag if it's big enough to obscure any of its parent menus
        hideOnExit = parent != nullptr
                      and parent->windowPos.intersects (windowPos.expanded (-4, -4));
    }

    void layoutMenuItems (const int maxMenuW, const int maxMenuH, int& width, int& height)
    {
        // Ensure we don't try to add an empty column after the final item
        if (not items.isEmpty())
        {
            auto* last { items.back().get() };
            last->item.shouldBreakAfter = false;
        }

        const auto isBreak = [] (const std::unique_ptr<ItemComponent>& item) { return item->item.shouldBreakAfter; };
        const auto numBreaks { static_cast<int> (std::count_if (items.begin(), items.end(), isBreak)) };
        numColumns = numBreaks + 1;

        if (numBreaks == 0)
            insertColumnBreaks (maxMenuW, maxMenuH);

        workOutManualSize (maxMenuW);
        height = juce::jmin (contentHeight, maxMenuH);

        needsToScroll = contentHeight > height;

        width = updateYPositions();
    }

    void insertColumnBreaks (const int maxMenuW, const int maxMenuH)
    {
        numColumns = options.getMinimumNumColumns();
        contentHeight = 0;

        auto maximumNumColumns { options.getMaximumNumColumns() > 0 ? options.getMaximumNumColumns() : 7 };

        for (;;)
        {
            auto totalW { workOutBestSize (maxMenuW) };

            if (totalW > maxMenuW)
            {
                numColumns = juce::jmax (1, numColumns - 1);
                workOutBestSize (maxMenuW); // to update col widths
                break;
            }

            if (totalW > maxMenuW / 2
                or contentHeight < maxMenuH
                or numColumns >= maximumNumColumns)
                break;

            ++numColumns;
        }

        const auto itemsPerColumn { (items.size() + numColumns - 1) / numColumns };

        for (auto i { 0 };; i += itemsPerColumn)
        {
            const auto breakIndex { i + itemsPerColumn - 1 };

            if (breakIndex >= items.size())
                break;

            items.at (static_cast<size_t> (breakIndex))->item.shouldBreakAfter = true;
        }

        if (not items.isEmpty())
            (*std::prev (items.end()))->item.shouldBreakAfter = false;
    }

    int correctColumnWidths (const int maxMenuW)
    {
        auto totalW { std::accumulate (columnWidths.begin(), columnWidths.end(), 0) };
        const auto minWidth { juce::jmin (maxMenuW, options.getMinimumWidth()) };

        if (totalW < minWidth)
        {
            totalW = minWidth;

            for (auto& column : columnWidths)
                column = totalW / numColumns;
        }

        return totalW;
    }

    void workOutManualSize (const int maxMenuW)
    {
        contentHeight = 0;
        columnWidths.clear();

        for (auto it = items.begin(), end = items.end(); it != end;)
        {
            const auto isBreak = [] (const std::unique_ptr<ItemComponent>& item) { return item->item.shouldBreakAfter; };
            const auto nextBreak { std::find_if (it, end, isBreak) };
            const auto columnEnd { nextBreak == end ? end : std::next (nextBreak) };

            const auto getMaxWidth = [] (int acc, const std::unique_ptr<ItemComponent>& item) { return juce::jmax (acc, item->getWidth()); };
            const auto colW { std::accumulate (it, columnEnd, options.getStandardItemHeight(), getMaxWidth) };
            const auto adjustedColW { juce::jmin (maxMenuW / juce::jmax (1, numColumns - 2),
                                            colW + getLookAndFeel().getPopupMenuBorderSizeWithOptions (options) * 2) };

            const auto sumHeight = [] (int acc, const std::unique_ptr<ItemComponent>& item) { return acc + item->getHeight(); };
            const auto colH { std::accumulate (it, columnEnd, 0, sumHeight) };

            contentHeight = juce::jmax (contentHeight, colH);
            columnWidths.add (adjustedColW);
            it = columnEnd;
        }

        contentHeight += getLookAndFeel().getPopupMenuBorderSizeWithOptions (options) * 2;

        correctColumnWidths (maxMenuW);
    }

    int workOutBestSize (const int maxMenuW)
    {
        contentHeight = 0;
        int childNum { 0 };

        for (int col { 0 }; col < numColumns; ++col)
        {
            int colW { options.getStandardItemHeight() };
            int colH { 0 };

            auto numChildren { juce::jmin (items.size() - childNum,
                                     (items.size() + numColumns - 1) / numColumns) };

            for (int i { numChildren }; --i >= 0;)
            {
                colW = juce::jmax (colW, items.at (static_cast<size_t> (childNum + i))->getWidth());
                colH += items.at (static_cast<size_t> (childNum + i))->getHeight();
            }

            colW = juce::jmin (maxMenuW / juce::jmax (1, numColumns - 2),
                         colW + getLookAndFeel().getPopupMenuBorderSizeWithOptions (options) * 2);

            columnWidths.set (col, colW);
            contentHeight = juce::jmax (contentHeight, colH);

            childNum += numChildren;
        }

        return correctColumnWidths (maxMenuW);
    }

    void ensureItemComponentIsVisible (const ItemComponent& itemComp, std::optional<int> wantedY)
    {
        const auto parentArea { getParentArea (windowPos.getPosition(), options.getParentComponent()) / scaleFactor };

        if (const auto posAndOffset = computePosAndOffsetToEnsureVisibility (windowPos, parentArea, itemComp.getBounds(), contentHeight, wantedY))
        {
            std::tie (windowPos, childYOffset) = std::tie (posAndOffset->windowPos, posAndOffset->childYOffset);
            updateYPositions();
        }
    }

    struct PosAndOffset
    {
        juce::Rectangle<int> windowPos;
        int childYOffset { 0 };
    };

    static std::optional<PosAndOffset> computePosAndOffsetToEnsureVisibility (juce::Rectangle<int> windowPos,
                                                                              const juce::Rectangle<int>& parentArea,
                                                                              const juce::Rectangle<int>& itemCompBounds,
                                                                              int contentHeight,
                                                                              std::optional<int> wantedY)
    {
        std::optional<PosAndOffset> result;

        // If there's no specific wantedY, and the item component is already visible, then we don't
        // need to make any adjustments.
        if (not (not wantedY.has_value() and 0 <= itemCompBounds.getY() and itemCompBounds.getBottom() <= windowPos.getHeight()))
        {
            const auto spaceNeededAboveItem { juce::jmin (PopupMenuSettings::scrollZone, itemCompBounds.getY()) };
            const auto spaceNeededBelowItem { juce::jmin (PopupMenuSettings::scrollZone, contentHeight - itemCompBounds.getBottom()) };
            const auto parentSpaceTargetY { windowPos.getY() + wantedY.value_or (itemCompBounds.getY()) };

            // In order to display the visible item over the target area, we need to make sure that
            // there's enough space above and below to hold the scroll areas if they're showing.
            // Ideally, we want to avoid the case where the menu opens with the scroll area over the
            // target area.
            const auto isSpaceToOverlay { spaceNeededAboveItem <= (parentSpaceTargetY - parentArea.getY())
                                       and spaceNeededBelowItem <= (parentArea.getBottom() - (parentSpaceTargetY + itemCompBounds.getHeight())) };

            if (wantedY.has_value() and isSpaceToOverlay)
            {
                windowPos = windowPos.withY (parentSpaceTargetY - itemCompBounds.getY())
                                     .withHeight (contentHeight)
                                     .constrainedWithin (parentArea);

                const auto menuSpaceTargetY { parentSpaceTargetY - windowPos.getY() };
                const auto offset { itemCompBounds.getY() - menuSpaceTargetY };

                result = PosAndOffset { windowPos, offset };
            }
            else
            {
                // If there's not enough space to overlay the menu, then just use the provided menu
                // bounds but try to position the visible item as close to the target area as possible,
                // while avoiding the scroll areas.
                const auto menuSpaceTargetY { juce::jlimit (spaceNeededAboveItem,
                                                      windowPos.getHeight() - spaceNeededBelowItem - itemCompBounds.getHeight(),
                                                      parentSpaceTargetY - windowPos.getY()) };
                const auto offset { itemCompBounds.getY() - menuSpaceTargetY };

                result = PosAndOffset { windowPos, offset };
            }
        }

        return result;
    }

    void resizeToBestWindowPos()
    {
        auto r { windowPos };

        if (childYOffset < 0)
        {
            r = r.withTop (r.getY() - childYOffset);
        }
        else if (childYOffset > 0)
        {
            auto spaceAtBottom { r.getHeight() - (contentHeight - childYOffset) };

            if (spaceAtBottom > 0)
                r.setSize (r.getWidth(), r.getHeight() - spaceAtBottom);
        }

        setBounds (r);
        updateYPositions();
    }

    void alterChildYPos (int delta)
    {
        if (canScroll())
        {
            childYOffset += delta;

            childYOffset = [&]
            {
                if (delta < 0)
                    return juce::jmax (childYOffset, 0);

                if (delta > 0)
                {
                    const auto limit { contentHeight
                                        - windowPos.getHeight()
                                        + getLookAndFeel().getPopupMenuBorderSizeWithOptions (options) };
                    return juce::jmin (childYOffset, limit);
                }

                return childYOffset;
            }();

            updateYPositions();
        }
        else
        {
            childYOffset = 0;
        }

        resizeToBestWindowPos();
        repaint();
    }

    int updateYPositions()
    {
        const auto separatorWidth { getLookAndFeel().getPopupMenuColumnSeparatorWidthWithOptions (options) };
        const auto initialY { getLookAndFeel().getPopupMenuBorderSizeWithOptions (options)
                              - (childYOffset + (getY() - windowPos.getY())) };

        auto col { 0 };
        auto x { 0 };
        auto y { initialY };

        for (const auto& item : items)
        {
            jassert (col < columnWidths.size());
            const auto columnWidth { columnWidths[col] };
            item->setBounds (x, y, columnWidth, item->getHeight());
            y += item->getHeight();

            if (item->item.shouldBreakAfter)
            {
                col += 1;
                x += columnWidth + separatorWidth;
                y = initialY;
            }
        }

        return std::accumulate (columnWidths.begin(), columnWidths.end(), 0)
               + (separatorWidth * (columnWidths.size() - 1));
    }

    void setCurrentlyHighlightedChild (ItemComponent* child)
    {
        if (currentChild != nullptr)
            currentChild->setHighlighted (false);

        currentChild = child;

        if (currentChild != nullptr)
        {
            currentChild->setHighlighted (true);
            timeEnteredCurrentChildComp = juce::Time::getApproximateMillisecondCounter();
        }

        if (auto* handler = getAccessibilityHandler())
            handler->notifyAccessibilityEvent (juce::AccessibilityEvent::rowSelectionChanged);
    }

    bool isSubMenuVisible() const noexcept          { return activeSubMenu != nullptr and activeSubMenu->isVisible(); }

    bool showSubMenuFor (ItemComponent* childComp)
    {
        // Defer old submenu destruction — its GL render thread calls
        // thread.join() which blocks the message thread.  During a mouse
        // event handler the render thread may need the message pump to
        // finish GL cleanup, causing a deadlock / mutex crash.  Moving
        // the old submenu to a callAsync lambda lets it destruct on the
        // next message pump iteration outside the event handler context.
        if (activeSubMenu != nullptr)
            juce::MessageManager::callAsync ([old = std::move (activeSubMenu)] {});

        bool result { false };

        if (not (childComp == nullptr or not hasActiveSubMenu (childComp->item)))
        {
            activeSubMenu = std::make_unique<MenuWindow> (*(childComp->item.subMenu), this,
                                                          options.forSubmenu()
                                                                 .withTargetScreenArea (childComp->getScreenBounds())
                                                                 .withMinimumWidth (0),
                                                          false, managerOfChosenCommand, scaleFactor);

            activeSubMenu->setVisible (true); // (must be called before enterModalState on Windows to avoid DropShadower confusion)
            activeSubMenu->enterModalState (false);
            activeSubMenu->toFront (false);
            result = true;
        }

        return result;
    }

    void triggerCurrentlyHighlightedItem()
    {
        if (currentChild != nullptr and canBeTriggered (currentChild->item))
            dismissMenu (&currentChild->item);
    }

    enum class MenuSelectionDirection
    {
        forwards,
        backwards,
        current
    };

    void selectNextItem (MenuSelectionDirection direction)
    {
        disableMouseMovesOnMenuAndAncestors();

        auto start { [&]
        {
            auto index { items.indexOf (currentChild) };

            if (index >= 0)
                return index;

            return direction == MenuSelectionDirection::backwards ? items.size() - 1
                                                                  : 0;
        }() };

        auto preIncrement { (direction != MenuSelectionDirection::current and currentChild != nullptr) };

        for (int i { items.size() }; --i >= 0;)
        {
            if (preIncrement)
                start += (direction == MenuSelectionDirection::backwards ? -1 : 1);

            if (auto* mic = items.at (static_cast<size_t> ((start + items.size()) % items.size())).get())
            {
                if (canBeTriggered (mic->item) or hasActiveSubMenu (mic->item))
                {
                    setCurrentlyHighlightedChild (mic);
                    return;
                }
            }

            if (not preIncrement)
                preIncrement = true;
        }
    }

    void disableMouseMovesOnMenuAndAncestors()
    {
        disableMouseMoves = true;

        if (parent != nullptr)
            parent->disableMouseMovesOnMenuAndAncestors();
    }

    bool canScroll() const noexcept                 { return childYOffset != 0 or needsToScroll; }
    bool isTopScrollZoneActive() const noexcept     { return canScroll() and childYOffset > 0; }
    bool isBottomScrollZoneActive() const noexcept  { return canScroll() and childYOffset < contentHeight - windowPos.getHeight(); }

    //==============================================================================
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return std::make_unique<juce::AccessibilityHandler> (*this,
                                                       juce::AccessibilityRole::popupMenu,
                                                       juce::AccessibilityActions().addAction (juce::AccessibilityActionType::focus, [this]
                                                       {
                                                           if (currentChild != nullptr)
                                                           {
                                                               if (auto* handler = currentChild->getAccessibilityHandler())
                                                                   handler->grabFocus();
                                                           }
                                                           else
                                                           {
                                                               selectNextItem (MenuSelectionDirection::forwards);
                                                           }
                                                       }));
    }

    juce::LookAndFeel* findLookAndFeel (const juce::PopupMenu& menu, MenuWindow* parentWindow) const
    {
        // NOTE: menu.lookAndFeel is private in juce::PopupMenu. There is no public
        // accessor. When parentWindow is null (top-level menu), the JUCE original
        // returns the LookAndFeel set on the PopupMenu object. We cannot access that
        // field from outside the class, so we return nullptr here. The caller
        // (findNonNullLookAndFeel) falls through to getLookAndFeel() in that case,
        // which resolves the ambient LookAndFeel from the component hierarchy — correct
        // for most hosts. If explicit per-menu LookAndFeel is required, this fork must
        // be extended to accept a juce::LookAndFeel* at construction time.
        juce::ignoreUnused (menu);
        return parentWindow != nullptr ? &(parentWindow->getLookAndFeel())
                                       : nullptr;
    }

    juce::LookAndFeel& findNonNullLookAndFeel (const juce::PopupMenu& menu, MenuWindow* parentWindow) const
    {
        if (auto* result = findLookAndFeel (menu, parentWindow))
            return *result;

        return getLookAndFeel();
    }

    bool mouseHasBeenOver() const
    {
        return mouseWasOver;
    }

    bool allowMouseUpToTriggerItem() const
    {
        return mouseUpCanTrigger;
    }

    //==============================================================================
    juce::Component::SafePointer<MenuWindow> parent;
    const juce::PopupMenu::Options options;
    jam::Owner<ItemComponent> items;
    juce::ApplicationCommandManager*& managerOfChosenCommand;
    juce::WeakReference<juce::Component> componentAttachedTo;
    juce::Rectangle<int> windowPos;
    bool needsToScroll { false };
    bool hideOnExit { false };
    bool disableMouseMoves { false };
    bool hasAnyJuceCompHadFocus { false };
    int numColumns { 0 };
    int contentHeight { 0 };
    int childYOffset { 0 };
    juce::Component::SafePointer<ItemComponent> currentChild;
    std::unique_ptr<MenuWindow> activeSubMenu;
    juce::Array<int> columnWidths;
    juce::uint32 windowCreationTime;
    juce::uint32 lastFocusedTime;
    juce::uint32 timeEnteredCurrentChildComp;
    jam::Owner<MouseSourceState> mouseSourceStates;
    float scaleFactor;
    bool exitingModalState { false };

private:
    static jam::VulkanEngine& getVulkanEngine() noexcept
    {
        auto* engine { jam::VulkanEngine::getInstance() };
        jassert (engine != nullptr
                 and "jam::PopupMenu::MenuWindow requires a jam::VulkanEngine instance to already exist");
        return *engine;
    }

    jam::VulkanEngine& vulkanEngine { getVulkanEngine() };

    void handleMouseEvent (const juce::MouseEvent& e)
    {
        mouseWasOver |= reallyContains (getLocalPoint (nullptr, e.getScreenPosition()), true);
        getMouseState (e.source).handleMouseEventWithPosition (e.getScreenPosition());
    }

    bool mouseWasOver { false };
    bool mouseUpCanTrigger { not juce::ModifierKeys::getCurrentModifiers().isAnyMouseButtonDown() };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenuWindow)
};

//==============================================================================
struct NormalComponentWrapper final : public juce::PopupMenu::CustomComponent
{
    NormalComponentWrapper (juce::Component& comp, int w, int h, bool triggerMenuItemAutomaticallyWhenClicked)
        : CustomComponent (triggerMenuItemAutomaticallyWhenClicked),
          width (w), height (h)
    {
        addAndMakeVisible (comp);
    }

    void getIdealSize (int& idealWidth, int& idealHeight) override
    {
        idealWidth = width;
        idealHeight = height;
    }

    void resized() override
    {
        if (auto* child = getChildComponent (0))
            child->setBounds (getLocalBounds());
    }

    const int width, height;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NormalComponentWrapper)
};

}; // struct PopupMenu

//==============================================================================
// This invokes any command manager commands and deletes the menu window when it is dismissed
struct PopupMenuCompletionCallback final : public juce::ModalComponentManager::Callback
{
    PopupMenuCompletionCallback() = default;

    void modalStateFinished (int result) override
    {
        if (managerOfChosenCommand != nullptr and result != 0)
        {
            juce::ApplicationCommandTarget::InvocationInfo info (result);
            info.invocationMethod = juce::ApplicationCommandTarget::InvocationInfo::fromMenu;

            managerOfChosenCommand->invoke (info, true);
        }

        // (this would be the place to fade out the component, if that's what's required)
        component.reset();

        if (not PopupMenuSettings::menuWasHiddenBecauseOfAppChange)
        {
            if (auto* focusComponent = juce::Component::getCurrentlyFocusedComponent())
            {
                const auto focusedIsNotMinimised = [focusComponent]
                {
                    if (auto* peer = focusComponent->getPeer())
                        return not peer->isMinimised();

                    return false;
                }();

                if (focusedIsNotMinimised)
                {
                    if (auto* topLevel = focusComponent->getTopLevelComponent())
                        topLevel->toFront (true);

                    if (focusComponent->isShowing() and not focusComponent->hasKeyboardFocus (true))
                        focusComponent->grabKeyboardFocus();
                }
            }
        }
    }

    juce::ApplicationCommandManager* managerOfChosenCommand { nullptr };
    std::unique_ptr<juce::Component> component;

    JUCE_DECLARE_NON_COPYABLE (PopupMenuCompletionCallback)
};

/** @brief Shows a juce::PopupMenu using jam::PopupMenu::MenuWindow (GL-aware lifecycle).
 *  Drop-in replacement for juce::PopupMenu::showWithOptionalCallback, cross-platform;
 *  applies a DPI-awareness scope around window creation on Windows only.
 *
 *  @param menu         The JUCE PopupMenu holding the item data model (not forked).
 *  @param options      Display options.
 *  @param userCallback Optional modal callback; ownership is transferred.
 *  @param canBeModal   If true and userCallback is null, runs a modal loop (JUCE_MODAL_LOOPS_PERMITTED only).
 *  @return             The selected item ID, or 0 when the menu is empty, dismissed, or shown asynchronously.
 */
inline int showWithOptionalCallback (juce::PopupMenu& menu,
                                     const juce::PopupMenu::Options& options,
                                     juce::ModalComponentManager::Callback* userCallback,
                                     [[maybe_unused]] bool canBeModal)
{
    std::unique_ptr<juce::ModalComponentManager::Callback> userCallbackDeleter { userCallback };
    auto callback { std::make_unique<PopupMenuCompletionCallback>() };

   #if JUCE_WINDOWS
    const auto handle { std::invoke ([&]() -> void*
    {
        if (auto* target = options.getTargetComponent())
            return target->getWindowHandle();

        return nullptr;
    }) };
    const juce::ScopedThreadDPIAwarenessSetter dpiScope { handle };
   #endif

    if (menu.getNumItems() > 0)
    {
        callback->component = std::make_unique<jam::PopupMenu::MenuWindow> (menu, nullptr, options,
                                                          not options.getTargetScreenArea().isEmpty(),
                                                          callback->managerOfChosenCommand);
        auto* window { static_cast<jam::PopupMenu::MenuWindow*> (callback->component.get()) };

        PopupMenuSettings::menuWasHiddenBecauseOfAppChange = false;

        window->setVisible (true); // (must be called before enterModalState on Windows to avoid DropShadower confusion)
        window->enterModalState (false, userCallbackDeleter.release());
        juce::ModalComponentManager::getInstance()->attachCallback (window, callback.release());

        window->toFront (false);  // need to do this after making it modal, or it could
                                  // be stuck behind other comps that are already modal

       #if JUCE_MODAL_LOOPS_PERMITTED
        if (userCallback == nullptr and canBeModal)
            return window->runModalLoop();
       #else
        jassert (not (userCallback == nullptr and canBeModal));
       #endif
    }

    return 0;
}

/** @brief Async show — the primary entry point for consumers.
 *
 *  Equivalent to juce::PopupMenu::showMenuAsync (options, callback). In an
 *  AU-sandboxed or Pro Tools host with a resolvable native peer, routes
 *  through jam::menu::showNativeAt() (native OS menu) instead; otherwise
 *  routes through jam::PopupMenu::MenuWindow for GL-aware lifecycle control.
 *
 *  @param menu      The JUCE PopupMenu holding the item data model.
 *  @param options   Display options.
 *  @param callback  Called with the selected item's ID (0 if dismissed).
 */
inline void showAsync (juce::PopupMenu& menu,
                       const juce::PopupMenu::Options& options,
                       std::function<void (int)> callback)
{
    auto* target { (isAUSandboxHost() or isProToolsHost()) ? options.getTargetComponent() : nullptr };
    auto* top { target != nullptr ? target->getTopLevelComponent() : nullptr };
    auto* peer { top != nullptr ? top->getPeer() : nullptr };

    if (peer != nullptr)
    {
        jam::menu::showNativeAt (peer->getNativeHandle(),
                                    menu,
                                    top->getLocalArea (target, target->getLocalBounds()),
                                    std::move (callback));
    }
    else
    {
        showWithOptionalCallback (menu,
                                  options,
                                  juce::ModalCallbackFunction::create (callback),
                                  false);
    }
}

} // namespace jam
