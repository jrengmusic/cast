namespace jam
{
/*____________________________________________________________________________*/
void ButtonSVG::paintSingleImage (juce::Graphics& g,
                                  bool shouldDrawButtonAsHighlighted,
                                  bool shouldDrawButtonAsDown,
                                  const juce::Button& button,
                                  const char* const image,
                                  ColourMode colourMode,
                                  jam::Svg::PathStyle style,
                                  juce::PathStrokeType strokeType,
                                  int edgeIndent)
{
    auto area { button.getLocalBounds().reduced (edgeIndent).toFloat() };
    auto path { jam::Svg::getPath (image, area) };

    juce::Colour colour;
    constexpr float dimFactor { 0.25f };
    float alpha { button.isEnabled() ? 1.0f : dimFactor };

    if (colourMode == ColourMode::forceOff)
    {
        colour = button.findColour (juce::TextButton::textColourOffId);
    }
    else if (colourMode == ColourMode::forceOn)
    {
        colour = button.findColour (juce::TextButton::textColourOnId);
    }
    else
    {
        colour = button.findColour (button.getToggleState() ? juce::TextButton::textColourOnId
                                                            : juce::TextButton::textColourOffId);

        if (button.isToggleable())
            alpha = button.getToggleState() ? alpha : dimFactor;
    }

    if (shouldDrawButtonAsHighlighted)
        colour = colour.brighter (1.0f);
    if (shouldDrawButtonAsDown)
        colour = colour.darker (0.75f);

    g.setColour (colour.withAlpha (alpha));

    switch (style)
    {
        case jam::Svg::PathStyle::alternate:
            path.setUsingNonZeroWinding (false);
            [[fallthrough]];
        case jam::Svg::PathStyle::fill:
            g.fillPath (path);
            break;
        case jam::Svg::PathStyle::stroke:
            g.strokePath (path, strokeType);
            break;
    }
}

/*____________________________________________________________________________*/
int ButtonSVG::getState (const juce::Button& button, bool isOver, bool isDown, size_t count)
{
    jassert (count == 1 or count == 3 or count == 4 or count == 6 or count == 8);

    int index { map::ButtonState::normal };

    if (count >= 8)
    {
        if (not button.isEnabled())
            index = map::ButtonState::disabled;
        else if (isDown)
            index = map::ButtonState::down;
        else if (isOver)
            index = map::ButtonState::over;
        if (button.getToggleState())
            index += map::ButtonState::normalOn;
    }
    else if (count >= 6)
    {
        if (isDown)
            index = map::ButtonState::down;
        else if (isOver)
            index = map::ButtonState::over;
        if (button.getToggleState())
            index += map::ButtonState::normalOn;
    }
    else if (count >= 4)
    {
        if (not button.isEnabled())
            index = map::ButtonState::disabled;
        else if (isDown)
            index = map::ButtonState::down;
        else if (isOver)
            index = map::ButtonState::over;
    }
    else if (count >= 3)
    {
        if (isDown)
            index = map::ButtonState::down;
        else if (isOver)
            index = map::ButtonState::over;
    }
    else
    {
        index = map::ButtonState::normal;
    }

    return index;
}

void ButtonSVG::paint (juce::Graphics& g,
                       bool shouldDrawButtonAsHighlighted,
                       bool shouldDrawButtonAsDown,
                       const juce::Button& button,
                       const char* const* images,
                       size_t count,
                       ColourMode colourMode,
                       jam::Svg::PathStyle style,
                       juce::PathStrokeType strokeType,
                       int edgeIndent)
{
    int index { getState (button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown, count) };

    if (images[index] == nullptr or images[index][0] == '\0')
        index = map::ButtonState::normal;

    paintSingleImage (g,
                      shouldDrawButtonAsHighlighted,
                      shouldDrawButtonAsDown,
                      button,
                      images[index],
                      colourMode,
                      style,
                      strokeType,
                      edgeIndent);
}
/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
