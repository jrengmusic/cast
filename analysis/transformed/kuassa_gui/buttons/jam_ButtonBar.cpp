namespace jam
{
/*____________________________________________________________________________*/

// ButtonBar animation duration (SSOT) — highlight slide (animateHighlight(), the
// updateTabPositions() re-target path) and tab-button reposition
// (updateTabPositions()'s desktopAnimator call).
static constexpr int durationMs { 200 };

//==============================================================================
// ButtonBar
//==============================================================================

ButtonBar::ButtonBar()
{
    setInterceptsMouseClicks (false, true);
    addAndMakeVisible (background);
    background.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (highlight);
    highlight.setInterceptsMouseClicks (false, false);
    setFocusContainerType (FocusContainerType::keyboardFocusContainer);
    animator.addChangeListener (this);
}

ButtonBar::~ButtonBar()
{
    animator.removeChangeListener (this);
}

std::unique_ptr<jam::MouseEvents<ButtonTab>> ButtonBar::createTabButton (const juce::String& name)
{
    auto button { std::make_unique<jam::MouseEvents<ButtonTab>>() };
    button->setButtonText (name);
    return button;
}

void ButtonBar::setPosition (int newPosition)
{
    if (position != newPosition)
    {
        position = newPosition;
        resized();
    }
}

bool ButtonBar::isVertical() const noexcept
{
    return position == map::Position::left or position == map::Position::right;
}

//==============================================================================

void ButtonBar::clearTabs()
{
    currentTab = jam::UUID::none();
    tabs.clear();
    order.clear();
    extraTabsButton.reset();
}

void ButtonBar::addTab (jam::UUID uuid, const juce::String& tabName, juce::Colour tabBackgroundColour)
{
    auto [tabEntry, inserted] { tabs.try_emplace (uuid, TabInfo{}) };
    assert (inserted and "tab uuid must be unique");

    auto& [tabKey, tabInfo] { *tabEntry };
    tabInfo.name = tabName;
    tabInfo.colour = tabBackgroundColour;
    tabInfo.button = createTabButton (tabName);
    assert (tabInfo.button != nullptr);

    auto* rawButton { tabInfo.button.get() };

    // Wire mouseDown: capture drag-start identity and component-relative position.
    rawButton->onMouseDown = [this, uuid]
    {
        dragStartTab = uuid;
        dragStartPos = getTabButton (uuid)->getMouseXYRelative();
    };

    // Wire onMouseDrag: drag-to-reorder when movement exceeds dragThreshold.
    // Reproduces original ButtonTab::mouseDrag logic — distance derived from stored
    // dragStartPos vs current getMouseXYRelative() to avoid MouseEvent argument.
    rawButton->onMouseDrag = [this, uuid]
    {
        auto* draggedButton { getTabButton (uuid) };
        const auto currentPos { draggedButton->getMouseXYRelative() };
        const auto dragDistance { isVertical() ? currentPos.y - dragStartPos.y
                                               : currentPos.x - dragStartPos.x };

        if (std::abs (dragDistance) > dragThreshold)
        {
            // Convert button-local current position to bar-local coordinates.
            const auto barPos { draggedButton->localPointToGlobal (currentPos.toFloat()).toInt()
                                - getScreenPosition() };

            for (int i { 0 }; i < order.size(); ++i)
            {
                const auto candidate { order.at (i) };

                if (candidate != uuid and tabs.at (candidate).button->getBounds().contains (barPos))
                {
                    moveTab (uuid, i, true);
                    break;
                }
            }
        }
    };

    // Wire onClick (Button::clicked equivalent): select tab or invoke popup.
    // MouseEvents routes right-click to onRightClick; left click fires onClick.
    rawButton->onClick = [this, uuid]
    {
        if (dragStartTab == jam::UUID::none() or dragStartTab == uuid)
            setCurrentTab (uuid);
    };

    rawButton->onRightClick = [this, uuid]
    {
        getTabButton (uuid)->showEditor();
    };

    addAndMakeVisible (rawButton);
    order.add (uuid);

    if (currentTab == jam::UUID::none())
        setCurrentTab (uuid);
    else
        resized();
}

void ButtonBar::setTabName (jam::UUID uuid, const juce::String& newName)
{
    if (tabs.contains (uuid))
    {
        auto& tabInfo { tabs.at (uuid) };

        if (tabInfo.name != newName)
        {
            tabInfo.name = newName;
            tabInfo.button->setButtonText (newName);
            resized();
        }
    }
}

void ButtonBar::removeTab (jam::UUID uuid, bool animate)
{
    const int orderIndex { order.indexOf (uuid) };
    assert (orderIndex >= 0);

    auto nextSelectedTab { currentTab };

    if (uuid == currentTab)
    {
        if (orderIndex + 1 < order.size())
            nextSelectedTab = order.at (orderIndex + 1);
        else if (orderIndex > 0)
            nextSelectedTab = order.at (orderIndex - 1);
        else
            nextSelectedTab = jam::UUID::none();
    }

    tabs.erase (uuid);
    order.remove (orderIndex);

    setCurrentTab (nextSelectedTab);
    updateTabPositions (animate);
}

void ButtonBar::moveTab (jam::UUID uuid, int newPosition, bool animate)
{
    const int currentPosition { order.indexOf (uuid) };
    assert (currentPosition >= 0);

    order.remove (currentPosition);

    const int insertPos { juce::jmin (newPosition, order.size()) };
    order.insert (insertPos, uuid);

    updateTabPositions (animate);

    if (onTabMoved != nullptr)
        onTabMoved (uuid);
}

int ButtonBar::getNumTabs() const { return order.size(); }

juce::String ButtonBar::getCurrentTabName() const
{
    if (tabs.contains (currentTab))
        return tabs.at (currentTab).name;

    return {};
}

juce::StringArray ButtonBar::getTabNames() const
{
    juce::StringArray names;

    for (const auto& uuid : order)
        names.add (tabs.at (uuid).name);

    return names;
}

void ButtonBar::setCurrentTab (jam::UUID uuid)
{
    if (currentTab != uuid)
    {
        const bool hadPreviousTab { currentTab != jam::UUID::none() };
        currentTab = uuid;

        updateTabPositions (false, false);

        if (hadPreviousTab)
            animateHighlight();
        else
            snapHighlight();

        currentTabChanged (uuid, getCurrentTabName());
    }
}

void ButtonBar::nextTab()
{
    const int orderIndex { order.indexOf (currentTab) };

    if (orderIndex >= 0 and orderIndex + 1 < order.size())
        setCurrentTab (order.at (orderIndex + 1));
}

void ButtonBar::prevTab()
{
    const int orderIndex { order.indexOf (currentTab) };

    if (orderIndex > 0)
        setCurrentTab (order.at (orderIndex - 1));
}

jam::MouseEvents<ButtonTab>* ButtonBar::getTabButton (jam::UUID uuid) const
{
    if (tabs.contains (uuid))
        return tabs.at (uuid).button.get();

    return nullptr;
}

juce::Colour ButtonBar::getTabBackgroundColour (jam::UUID uuid) const
{
    if (tabs.contains (uuid))
        return tabs.at (uuid).colour;

    return juce::Colours::transparentBlack;
}

void ButtonBar::setTabBackgroundColour (jam::UUID uuid, juce::Colour newColour)
{
    if (tabs.contains (uuid))
    {
        auto& tabInfo { tabs.at (uuid) };

        if (tabInfo.colour != newColour)
        {
            tabInfo.colour = newColour;
            repaint();
        }
    }
}

int ButtonBar::getBestTabLength (const juce::String& tabName) const
{
    const auto& custom { static_cast<jam::StyleCustom&> (getLookAndFeel()) };
    const auto font { custom.getTabFont() };
    const auto pad { custom.getTabPadding() };
    const auto width { juce::TextLayout::getStringWidth (
        font, custom.getTabText (tabName.trim())) };

    return juce::roundToInt (width) + pad * 2;
}

void ButtonBar::lookAndFeelChanged()
{
    extraTabsButton.reset();
    resized();
}

void ButtonBar::paint (juce::Graphics&) {}

void ButtonBar::resized() { updateTabPositions (false); }

//==============================================================================

void ButtonBar::updateTabPositions (bool animate, bool shouldSnap)
{
    // V3 defaults: overlap = -1, spaceAroundImage = 0
    constexpr int overlapValue { -1 };
    constexpr int spaceAroundImage { 0 };

    const auto& custom { static_cast<jam::StyleCustom&> (getLookAndFeel()) };
    const auto barPadding { custom.getTabBarPadding() };
    const auto contentArea { barPadding.subtractedFrom (getLocalBounds()) };

    auto length { isVertical() ? contentArea.getHeight() : contentArea.getWidth() };

    auto overlap { overlapValue + spaceAroundImage * 2 };

    auto totalLength { juce::jmax (0, overlap) };
    auto numVisibleButtons { order.size() };

    for (int i { 0 }; i < order.size(); ++i)
        totalLength += getBestTabLength (tabs.at (order.at (i)).name) - overlap;

    const bool isTooBig { totalLength > length };
    int tabsButtonPos { 0 };

    if (isTooBig)
    {
        if (extraTabsButton == nullptr)
        {
            extraTabsButton = std::make_unique<juce::TextButton> ("...");
            addAndMakeVisible (extraTabsButton.get());
            extraTabsButton->setAlwaysOnTop (true);
            extraTabsButton->setTriggeredOnMouseDown (true);
            extraTabsButton->onClick = [this]
            {
                showExtraItemsMenu();
            };
        }

        auto buttonSize { juce::jmin (
            juce::roundToInt (getWidth() * 0.7f), juce::roundToInt (getHeight() * 0.7f)) };
        extraTabsButton->setSize (buttonSize, buttonSize);

        tabsButtonPos =
            (isVertical() ? contentArea.getBottom() : contentArea.getRight()) - buttonSize / 2 - 1;

        if (isVertical())
            extraTabsButton->setCentrePosition (contentArea.getCentreX(), tabsButtonPos);
        else
            extraTabsButton->setCentrePosition (tabsButtonPos, contentArea.getCentreY());

        totalLength = 0;

        for (int i { 0 }; i < order.size(); ++i)
        {
            auto newLength { totalLength + getBestTabLength (tabs.at (order.at (i)).name) };

            if (i > 0 and newLength > tabsButtonPos)
            {
                totalLength += overlap;
                break;
            }

            numVisibleButtons = i + 1;
            totalLength = newLength - overlap;
        }
    }
    else
    {
        extraTabsButton.reset();
    }

    int pos { isVertical() ? contentArea.getY() : contentArea.getX() };
    jam::MouseEvents<ButtonTab>* frontTab { nullptr };
    juce::Rectangle<int> currentTabBounds;
    auto& desktopAnimator { juce::Desktop::getInstance().getAnimator() };

    for (int i { 0 }; i < order.size(); ++i)
    {
        const auto uuid { order.at (i) };
        auto& tabInfo { tabs.at (uuid) };
        auto* tb { tabInfo.button.get() };

        auto bestLength { getBestTabLength (tabInfo.name) };

        if (i < numVisibleButtons)
        {
            auto newBounds {
                isVertical() ? juce::Rectangle<int> (
                                   contentArea.getX(), pos, contentArea.getWidth(), bestLength)
                             : juce::Rectangle<int> (
                                   pos, contentArea.getY(), bestLength, contentArea.getHeight())
            };

            if (animate)
            {
                desktopAnimator.animateComponent (tb, newBounds, 1.0f, durationMs, false, 3.0, 0.0);
            }
            else
            {
                desktopAnimator.cancelAnimation (tb, false);
                tb->setBounds (newBounds);
            }

            if (uuid == currentTab)
            {
                frontTab = tb;
                currentTabBounds = newBounds;
            }

            tb->setVisible (true);

            // Propagate label layout — ButtonTab no longer reads ButtonBar internals.
            if (isVertical())
            {
                const auto w { static_cast<float> (newBounds.getWidth()) };
                const auto h { static_cast<float> (newBounds.getHeight()) };

                auto transform { (position == map::Position::left)
                    ? juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi)
                          .translated (0.0f, h)
                    : juce::AffineTransform::rotation (juce::MathConstants<float>::halfPi)
                          .translated (w, 0.0f) };

                tb->setLabelLayout (transform,
                    { 0, 0, static_cast<int> (h), static_cast<int> (w) });
            }
            else
            {
                tb->setLabelLayout ({}, newBounds.withZeroOrigin());
            }
        }
        else
        {
            tb->setVisible (false);
        }

        pos += bestLength - overlap;
    }

    background.setBounds (contentArea);
    background.toBack();

    if (frontTab != nullptr)
        frontTab->toFront (false);

    highlight.toFront (false);

    if (shouldSnap and not currentTabBounds.isEmpty())
    {
        // Use target bounds directly — button->getBounds() returns pre-animation position
        // when animate=true (desktopAnimator hasn't moved buttons yet at this point).
        if (animate or animator.isAnimating (&highlight))
        {
            animator.animateComponent (&highlight, currentTabBounds, 1.0f, durationMs, false, 1.0, 0.0);
        }
        else
        {
            animator.cancelAnimation (&highlight, false);
            highlight.setBounds (currentTabBounds);
        }

        setToggleStates();
    }
}

void ButtonBar::snapHighlight()
{
    animator.cancelAnimation (&highlight, false);

    if (currentTab != jam::UUID::none() and tabs.contains (currentTab))
    {
        highlight.setBounds (tabs.at (currentTab).button->getBounds());
        setToggleStates();
    }
}

void ButtonBar::setToggleStates()
{
    for (const auto& uuid : order)
        tabs.at (uuid).button->setToggleState (uuid == currentTab, juce::dontSendNotification);
}

void ButtonBar::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source == &animator and not animator.isAnimating (&highlight))
        setToggleStates();
}

void ButtonBar::animateHighlight()
{
    if (tabs.contains (currentTab))
    {
        animator.animateComponent (
            &highlight,
            tabs.at (currentTab).button->getBounds(),
            1.0f,
            durationMs,
            false,
            1.0,
            0.0);
    }
}

//==============================================================================

void ButtonBar::showExtraItemsMenu()
{
    juce::PopupMenu m;

    for (const auto& uuid : order)
    {
        auto& tabInfo { tabs.at (uuid) };

        if (not tabInfo.button->isVisible())
            m.addItem (juce::PopupMenu::Item (tabInfo.name)
                           .setTicked (uuid == currentTab)
                           .setAction (
                               [this, uuid]
                               {
                                   setCurrentTab (uuid);
                               }));
    }

    m.showMenuAsync (juce::PopupMenu::Options().withDeletionCheck (*this).withTargetComponent (
        extraTabsButton.get()));
}

void ButtonBar::currentTabChanged (jam::UUID, const juce::String&) {}
void ButtonBar::popupMenuClickOnTab (jam::UUID, const juce::String&) {}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
