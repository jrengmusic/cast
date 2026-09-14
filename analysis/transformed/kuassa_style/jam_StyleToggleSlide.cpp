namespace jam
{
/*____________________________________________________________________________*/

void StyleToggleSlide::setUsingToggleForBypass (bool isUsingToggleForBypass)
{
    shouldFlipToggle = isUsingToggleForBypass;
}

juce::Colour StyleToggleSlide::getCapColour (juce::ToggleButton& button) const noexcept
{
    const float amount { 0.15f };
    auto colour { button.findColour (juce::TextButton::buttonColourId) };
    return colour.getLightness() < 0.5f ? colour.brighter (amount) : colour.darker (amount);
}

juce::Colour StyleToggleSlide::getLaneColour (juce::ToggleButton& button) const
    noexcept
{
    return button.findColour (juce::TextButton::buttonOnColourId);
}
//==============================================================================
void StyleToggleSlide::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    drawToggleBackground (g, button);
    drawToggleCap (g, button);
}

//==============================================================================
juce::Rectangle<float> StyleToggleSlide::getToggleArea (juce::ToggleButton& button) const noexcept
{
    auto area { button.getLocalBounds().reduced (shadowBleed).toFloat() };
    return area;
}

bool StyleToggleSlide::isPortrait (juce::ToggleButton& button) const noexcept
{
    return jam::Size<int>::isPortrait ({ button.getWidth(), button.getHeight() });
}

bool StyleToggleSlide::isLandscape (juce::ToggleButton& button) const noexcept
{
    return jam::Size<int>::isLandscape ({ button.getWidth(), button.getHeight() });
}

float StyleToggleSlide::getToggleCornerSize (juce::ToggleButton& button, float bevelSize) const noexcept
{
    auto area { getToggleArea (button) };

    return 0.5f * ((isPortrait (button) ? area.getWidth() : area.getHeight()) - bevelSize);
}

void StyleToggleSlide::drawToggleBackground (juce::Graphics& g, juce::ToggleButton& button)
{
    auto area { getToggleArea (button) };

    juce::Path bevel;
    bevel.addRoundedRectangle (area, getToggleCornerSize (button));
    g.setGradientFill (jam::colours::getVerticalGradient (area, getCapColour (button), 1.5f, 0.5f, 5.0f, false));
    g.fillPath (bevel);

    area.reduce (bevelSize, bevelSize);

    bool buttonState { shouldFlipToggle ? not button.getToggleState() : button.getToggleState() };

    juce::Path p;
    p.addRoundedRectangle (area, getToggleCornerSize (button, bevelSize));
    auto backgroundColour { buttonState ? getLaneColour (button) : getCapColour (button).darker (1.0f) };
    g.setColour (backgroundColour);
    g.fillPath (p);

    backgroundShadow.render (g, p);
}

void StyleToggleSlide::drawToggleCap (juce::Graphics& g, juce::ToggleButton& button)
{
    auto area { getToggleArea (button) };

    bool buttonState { shouldFlipToggle ? not button.getToggleState() : button.getToggleState() };

    float capBevelRadius { static_cast<float> ((isPortrait (button) ? area.getWidth() : area.getHeight())) };
    float delta { (buttonState ? -0.5f : 0.5f) * bevelSize };
    float dontChange { 0.0f };

    juce::Rectangle<float> capArea;

    if (isPortrait (button))
    {
        capArea = buttonState ? area.removeFromTop (capBevelRadius) : area.removeFromBottom (capBevelRadius);
        capArea.translate (dontChange, -delta);
    }
    else
    {
        capArea = buttonState ? area.removeFromRight (capBevelRadius) : area.removeFromLeft (capBevelRadius);
        capArea.translate (delta, dontChange);
    }

    capArea.reduce (bevelSize, bevelSize);

    juce::Path bevel;
    bevel.addEllipse (capArea);

    capShadow.render (g, bevel);

    g.setGradientFill (jam::colours::getVerticalGradient (capArea, getCapColour (button), 2.0f));
    g.fillPath (bevel);

    juce::Path cap;
    cap.addEllipse (capArea.reduced (bevelSize));
    g.setColour (getCapColour (button).brighter (0.1f));
    g.fillPath (cap);
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
