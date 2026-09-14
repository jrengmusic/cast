namespace jam
{
/*____________________________________________________________________________*/

void StyleTogglePush::setUsingToggleForBypass (bool isUsingToggleForBypass)
{
    shouldFlipToggle = isUsingToggleForBypass;
}

//==============================================================================
juce::Rectangle<float> StyleTogglePush::getCapArea (juce::ToggleButton& button) const noexcept
{
    const auto& area = [this, &button]
    {
        auto a { button.getLocalBounds().toFloat() };

        if (button.isDown() or getState (button))
            a.removeFromTop (capExtrusion * 0.5f);
        else
            a.removeFromBottom (capExtrusion * 0.5f);

        return a.reduced (bevelSize * (button.isDown() ? 2.0f : 1.0f));
    }();

    return area;
}

juce::Colour StyleTogglePush::getCapColour (juce::ToggleButton& button) const noexcept
{
    const float amount { 0.15f };
    auto colour { button.findColour (juce::TextButton::buttonColourId) };
    return colour.getLightness() < 0.5f ? colour.brighter (amount) : colour.darker (amount);
}

juce::Colour StyleTogglePush::getSymbolColour (juce::ToggleButton& button) const noexcept
{
    return button.findColour (juce::TextButton::buttonOnColourId);
}

juce::Rectangle<float> StyleTogglePush::getBackgroundArea (juce::ToggleButton& button) const noexcept
{
    const auto& area = [this, &button]
    {
        auto a { button.getLocalBounds().toFloat() };
        a.removeFromTop (capExtrusion * 0.5f);
        return a;
    }();

    return area;
}

bool StyleTogglePush::getState (juce::ToggleButton& button) const noexcept
{
    return not (shouldFlipToggle == button.getToggleState());
}

float StyleTogglePush::getBevelSize() const noexcept
{
    return bevelSize;
}

float StyleTogglePush::getCapExtrusion() const noexcept
{
    return capExtrusion;
}

//==============================================================================

void StyleTogglePush::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    drawCapExtrusion (g, button);
    drawCap (g, button);
    drawStandbySymbol (g, button);
}

void StyleTogglePush::drawCap (juce::Graphics& g, juce::ToggleButton& button)
{
    auto area { getCapArea (button) };

    if (not button.isDown())
    {
        juce::Path bevel;
        bevel.addEllipse (area);
        g.setGradientFill (jam::colours::getVerticalGradient (area, getCapColour (button), 2.0f));
        g.fillPath (bevel);
    }

    juce::Path cap;
    cap.addEllipse (button.isDown() ? area : area.reduced (bevelSize));
    const float amount { 0.1f };
    g.setColour (getCapColour (button).brighter (amount));
    g.fillPath (cap);

    if (button.isDown())
        capInnerShadow.render (g, cap);
}

void StyleTogglePush::drawCapExtrusion (juce::Graphics& g, juce::ToggleButton& button)
{
    if (not button.isDown() and not getState (button))
    {
        juce::Path p;
        const auto capArea { getCapArea (button) };
        const float rectX { capArea.getX() };
        const float rectY { capArea.getCentreY() };
        const float width { capArea.getWidth() };
        const float height { capArea.getHeight() };
        juce::Rectangle<float> rect { rectX, rectY, width, capExtrusion };
        p.addRectangle (rect);

        const float fromRadians { juce::degreesToRadians (90.0f) };
        const float toRadians { juce::degreesToRadians (270.0f) };
        const float innerCircleProportionalSize { 0.0f };
        p.addPieSegment ({ rectX, capExtrusion - 1.0f, width, height }, fromRadians, toRadians, innerCircleProportionalSize);

        const float amount { 0.1f };
        g.setColour (getCapColour (button).darker (amount));
        g.fillPath (p);
    }
}

void StyleTogglePush::drawStandbySymbol (juce::Graphics& g, juce::ToggleButton& button)
{
    const float size { 0.5f };

    auto area = [this, &button] (float size)
    {
        const auto capArea { getCapArea (button) };
        auto a { capArea * size };
        a.setCentre (capArea.getCentre());
        return a;
    };

    const auto& fillColour = [this, &button]
    {
        auto c { getState (button) ? getSymbolColour (button) : getCapColour (button).contrasting (0.5f) };
        const float amount { 0.1414f };
        return button.isDown() ? getSymbolColour (button).darker (amount).withMultipliedSaturation (amount * 4.0f) : c.brighter (amount);
    }();

    const auto& document { jam::Xml::getOrCreate (juce::Identifier { files::standby }) };
    const auto path { jam::Svg::getPath (*document.root, area (size)) };

    g.setColour (fillColour);
    g.fillPath (path);

    symbolShadow.render (g, path);

    if (getState (button))
    {
        juce::Path glow;
        glow.addEllipse (area (size * 2.0f));

        auto image { juce::Image (juce::Image::ARGB, button.getWidth(), button.getHeight(), true, juce::SoftwareImageType()) };
        auto graphics { juce::Graphics (image) };
        graphics.setColour (fillColour.withAlpha (0.075f));
        graphics.fillPath (glow);

        g.drawImageAt (glowBlur.render (image), 0, 0);
    }
}
/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
