namespace jam
{
/*__________________________________________________________________________________________*/

AnimationScrollingText::AnimationScrollingText (const juce::StringArray& lineOfText)
    : text (lineOfText)
{
}

AnimationScrollingText::AnimationScrollingText()
{
}

AnimationScrollingText::~AnimationScrollingText()
{
    stopTimer();
}

void AnimationScrollingText::resized()
{
}

//==============================================================================
void AnimationScrollingText::setColour (const juce::Colour& newColour)
{
    colour = newColour;
}

void AnimationScrollingText::setFont (const juce::FontOptions& newFont)
{
    font = newFont;
}

void AnimationScrollingText::setText (const juce::StringArray& lineOfText)
{
    if (not lineOfText.isEmpty())
    {
        text = lineOfText;

        textLinesHeight = text.size() * jam::toInt (font.getHeight());
    }
}

//==============================================================================
void AnimationScrollingText::timerCallback()
{
    if (not text.isEmpty())
    {
        if (yPosition > -textLinesHeight)
            --yPosition;
        else
            yPosition = getHeight();

        repaint();
    }
}

void AnimationScrollingText::paint (juce::Graphics& g)
{
    g.setFont (font);
    g.setColour (colour);

    juce::Rectangle<int> newTextArea { 0, yPosition, getWidth(), textLinesHeight };

    for (auto& t : text)
    {
        g.drawText (t, newTextArea.removeFromTop (jam::toInt (font.getHeight())), juce::Justification::centred);
    }
}

//==============================================================================

void AnimationScrollingText::start()
{
    if (isTimerRunning())
        stop();

    startTimerHz (timerIntervalHz);
}

void AnimationScrollingText::stop()
{
    stopTimer();
}

/**____________________________________END OF NAMESPACE____________________________________*/
} // namespace jam
