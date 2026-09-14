namespace jam
{ /*____________________________________________________________________________*/

//==============================================================================
TabbedComponent::TabbedComponent (jam::Model& model,
                                  juce::ValueTree parentState,
                                  const juce::Identifier& type,
                                  jam::UUID uuid)
    : OwnerComponent (model, parentState, type, uuid, Id::focusedTab)
{
    bar = std::make_unique<ButtonBar> (*this);
    addAndMakeVisible (bar.get());
}

TabbedComponent::TabbedComponent (jam::Model& model, juce::ValueTree existingState)
    : OwnerComponent (model, existingState, Id::focusedTab)
{
    bar = std::make_unique<ButtonBar> (*this);
    addAndMakeVisible (bar.get());
}

//==============================================================================
void TabbedComponent::setPosition (int newPosition)
{
    bar->setPosition (newPosition);
    layout();
}

int TabbedComponent::getPosition() const noexcept { return bar->getPosition(); }

void TabbedComponent::setTabBarDepth (int newDepth)
{
    if (tabDepth != newDepth)
    {
        tabDepth = newDepth;
        layout();
    }
}

std::unique_ptr<jam::MouseEvents<jam::ButtonTab>>
TabbedComponent::createTabButton (const juce::String& tabName)
{
    auto button { std::make_unique<jam::MouseEvents<jam::ButtonTab>>() };
    button->setButtonText (tabName);
    return button;
}

//==============================================================================
void TabbedComponent::childAdded (jam::UUID uuid)
{
    bar->addTab (uuid, juce::String {}, juce::Colours::transparentBlack);
}

void TabbedComponent::childRemoved (jam::UUID uuid)
{
    bar->removeTab (uuid);
}

//==============================================================================
void TabbedComponent::setTabName (jam::UUID uuid, const juce::String& newName)
{
    bar->setTabName (uuid, newName);
}

void TabbedComponent::moveTab (jam::UUID uuid, int newPosition, bool animate)
{
    bar->moveTab (uuid, newPosition, animate);
}

juce::StringArray TabbedComponent::getTabNames() const { return bar->getTabNames(); }

juce::Colour TabbedComponent::getTabBackgroundColour (jam::UUID uuid) const noexcept
{
    return bar->getTabBackgroundColour (uuid);
}

void TabbedComponent::setTabBackgroundColour (jam::UUID uuid, juce::Colour newColour)
{
    bar->setTabBackgroundColour (uuid, newColour);

    if (getCurrentTab() == uuid)
        repaint();
}

//==============================================================================
void TabbedComponent::setCurrentTab (jam::UUID uuid) { bar->setCurrentTab (uuid); }

jam::UUID TabbedComponent::getCurrentTab() const { return bar->getCurrentTab(); }
juce::String TabbedComponent::getCurrentTabName() const { return bar->getCurrentTabName(); }

void TabbedComponent::nextTab() { bar->nextTab(); }
void TabbedComponent::prevTab() { bar->prevTab(); }

void TabbedComponent::setOutline (int thickness)
{
    outlineThickness = thickness;
    layout();
    repaint();
}

void TabbedComponent::setIndent (int indentThickness)
{
    edgeIndent = indentThickness;
    layout();
    repaint();
}

//==============================================================================
void TabbedComponent::paint (juce::Graphics& g)
{
    auto content { getLocalBounds() };
    edges.get (bar->getPosition(), content, tabDepth);

    g.reduceClipRegion (content);
    g.fillAll (bar->getTabBackgroundColour (bar->getCurrentTab()));

    if (outlineThickness > 0)
    {
        juce::RectangleList<int> rl (content);
        rl.subtract (juce::BorderSize<int> (outlineThickness).subtractedFrom (content));
        g.reduceClipRegion (rl);
        g.fillAll (findColour (juce::TabbedComponent::outlineColourId));
    }
}

void TabbedComponent::layout()
{
    auto content { getLocalBounds() };
    bar->setBounds (edges.get (bar->getPosition(), content, tabDepth));

    content =
        juce::BorderSize<int> (edgeIndent)
            .subtractedFrom (juce::BorderSize<int> (outlineThickness).subtractedFrom (content));

    const auto focused { getFocusedChild() };

    for (auto& [uuid, child] : getChildren())
    {
        child->setBounds (content);
        child->setVisible (uuid == focused);
    }

    repaint();
}

void TabbedComponent::lookAndFeelChanged()
{
    for (auto& [uuid, child] : getChildren())
        child->lookAndFeelChanged();
}

void TabbedComponent::changeCallback (jam::UUID newCurrentTab, const juce::String& newTabName)
{
    setFocusedChild (newCurrentTab);
    currentTabChanged (newCurrentTab, newTabName);
}

void TabbedComponent::currentTabChanged (jam::UUID, const juce::String&) {}
void TabbedComponent::popupMenuClickOnTab (jam::UUID, const juce::String&) {}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
