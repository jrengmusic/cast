namespace jam
{
/*____________________________________________________________________________*/

ButtonGroup::ButtonGroup()
    : Model::ValueComponent<ButtonGroup> (juce::StringRef {})
    , value (juce::String {})
{
    value.addListener (this);
    addAndMakeVisible (indicator);
    indicator.setInterceptsMouseClicks (false, false);
}

void ButtonGroup::paint (juce::Graphics& g)
{
    if (auto* laf = dynamic_cast<jam::StyleCustom*> (&getLookAndFeel()))
        laf->drawButtonGroupTrack (g, *this);
}

void ButtonGroup::makeRow (int rowHeight, int insetX, int insetY)
{
    auto area { getLocalBounds().removeFromTop (rowHeight).reduced (insetX, insetY) };
    FlexBox::makeRow (buttons, area);
}

void ButtonGroup::resized()
{
    if ((not getLocalBounds().isEmpty()) and not buttons.isEmpty())
    {
        if (resizeFunction != nullptr)
            resizeFunction();
        else
            makeRow (getHeight());

        snapIndicator();
    }
}

void ButtonGroup::valueChanged (juce::Value& valueThatChanged)
{
    for (auto& b : buttons)
    {
        if (b->getProperties().contains (Id::groupButton))
        {
            if (valueThatChanged.refersToSameSourceAs (b->getToggleStateValue()))
            {
                const auto label { b->getButtonText() };

                if (label.compare (value.toString()) != 0)
                    value.setValue (label);
            }
        }
    }

    if (valueThatChanged.refersToSameSourceAs (value))
    {
        for (auto& b : buttons)
        {
            if (b->getProperties().contains (Id::groupButton))
            {
                if (b->getButtonText().compare (value.toString()) == 0)
                    b->setToggleState (true, juce::dontSendNotification);
            }
        }

        animateIndicator();

        if (onValueChanged != nullptr)
            onValueChanged();
    }
}

void ButtonGroup::addButton (std::unique_ptr<juce::Button> newButton, bool isFreeButton)
{
    newButton->setTriggeredOnMouseDown (true);

    if (not isFreeButton)
    {
        jassert (getComponentID().isNotEmpty());

        newButton->setClickingTogglesState (true);
        newButton->getProperties().set (Id::groupButton, true);
        newButton->setRadioGroupId (getComponentID().hashCode());
        newButton->getToggleStateValue().addListener (this);
    }

    juce::Component::addAndMakeVisible (newButton.get());
    buttons.add (std::move (newButton));

    resized();
}

void ButtonGroup::snapIndicator()
{
    animator.cancelAnimation (&indicator, false);

    for (const auto& b : buttons)
    {
        if (b->getProperties().contains (Id::groupButton))
        {
            if (b->getToggleState())
                indicator.setBounds (b->getBounds());
        }
    }
}

void ButtonGroup::animateIndicator()
{
    for (const auto& b : buttons)
    {
        if (b->getProperties().contains (Id::groupButton))
        {
            if (b->getToggleState())
            {
                constexpr int durationMs { 120 };

                animator.animateComponent (&indicator, b->getBounds(), 1.0f, durationMs, false, 1.0, 0.0);
            }
        }
    }
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
