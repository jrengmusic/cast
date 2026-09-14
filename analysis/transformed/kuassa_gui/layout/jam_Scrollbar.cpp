/**
 * @file jam_Scrollbar.cpp
 * @brief Interactive proportional scrollbar — juce::ScrollBar wrapper.
 */

namespace jam
{
/*____________________________________________________________________________*/

Scrollbar::Scrollbar() noexcept
    : Model::ValueComponent<Scrollbar> ("scrollbar")
{
    addAndMakeVisible (scrollBar);
    scrollBar.setAutoHide (false);
    scrollBar.addListener (this);
    scrollValue.addListener (this);
}

void Scrollbar::setRange (int capacity, int visibleRows) noexcept
{
    currentCapacity    = capacity;
    currentVisibleRows = visibleRows;

    if (capacity > 0)
    {
        const double total { static_cast<double> (capacity + visibleRows) };
        scrollBar.setRangeLimits (0.0, total, juce::dontSendNotification);

        const int offset { static_cast<int> (scrollValue.getValue()) };
        const int sbPos  { jam::Value::map (offset, 0, currentCapacity, currentCapacity, 0) };
        scrollBar.setCurrentRange (static_cast<double> (sbPos),
                                   static_cast<double> (visibleRows),
                                   juce::dontSendNotification);
        scrollBar.setVisible (true);
    }
    else
    {
        scrollBar.setVisible (false);
    }
}

juce::Value& Scrollbar::getValueObject() noexcept { return scrollValue; }

void Scrollbar::resized()
{
    scrollBar.setBounds (getLocalBounds());
}

void Scrollbar::scrollBarMoved (juce::ScrollBar*, double newRangeStart)
{
    if (currentCapacity > 0)
    {
        const int sbPos     { static_cast<int> (newRangeStart) };
        const int newOffset { jam::Value::map (sbPos, 0, currentCapacity, currentCapacity, 0) };
        scrollValue.setValue (juce::jlimit (0, currentCapacity, newOffset));
    }
}

void Scrollbar::valueChanged (juce::Value&)
{
    if (currentCapacity > 0)
    {
        const int offset { static_cast<int> (scrollValue.getValue()) };
        const int sbPos  { jam::Value::map (offset, 0, currentCapacity, currentCapacity, 0) };
        scrollBar.setCurrentRange (static_cast<double> (sbPos),
                                   static_cast<double> (currentVisibleRows),
                                   juce::dontSendNotification);
    }
}

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
