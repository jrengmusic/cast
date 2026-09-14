namespace jam
{
/*____________________________________________________________________________*/

CollapsibleList::CollapsibleList (const juce::String& componentID)
{
    setComponentID (componentID);

    addAndMakeVisible (viewport);
    viewport.setViewedComponent (&content, false);
    viewport.setComponentID (componentID);

    for (auto& child : viewport.getChildren())
    {
        if (dynamic_cast<juce::ScrollBar*> (child) == nullptr)
            child->setComponentID (componentID);
    }

    content.setComponentID (componentID);
}

void CollapsibleList::addSection (std::unique_ptr<CollapsibleSectionBase> section)
{
    auto* raw { section.get() };

    raw->onToggle = [this]
    {
        updateLayout();
    };

    content.addAndMakeVisible (raw);
    sections.add (std::move (section));

    updateLayout();
}

void CollapsibleList::clearSections()
{
    sections.clear();
    content.removeAllChildren();
    updateLayout();
}

void CollapsibleList::resized()
{
    viewport.setBounds (getLocalBounds());
    updateLayout();
}

void CollapsibleList::updateLayout()
{
    auto& animator { juce::Desktop::getInstance().getAnimator() };

    const int viewportWidth { viewport.getWidth() };
    int y { 0 };

    for (auto& section : sections)
        y += section->getTotalHeight();

    content.setSize (viewportWidth, juce::jmax (y, 1));

    int availableWidth { viewportWidth };

    if (viewport.getVerticalScrollBar().isVisible())
        availableWidth -= viewport.getScrollBarThickness();

    int currentY { 0 };

    for (auto& section : sections)
    {
        const int h { section->getTotalHeight() };
        juce::Rectangle<int> targetBounds { 0, currentY, availableWidth, h };

        float finalAlpha { 1.0f };
        int durationMilliseconds { 100 };
        double startSpeed { 1.0 };
        double endSpeed { 1.0 };
        animator.animateComponent (section.get(),
                                   targetBounds,
                                   finalAlpha,
                                   durationMilliseconds,
                                   false,
                                   startSpeed,
                                   endSpeed);

        currentY += h;
    }

    content.setSize (availableWidth, juce::jmax (currentY, 1));
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
