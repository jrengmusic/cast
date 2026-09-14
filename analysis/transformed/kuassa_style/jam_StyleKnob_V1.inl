namespace jam
{
/*____________________________________________________________________________*/

template <typename ManagerType>
StyleKnob_V1<ManagerType>::StyleKnob_V1()
{
}

template <typename ManagerType>
StyleKnob_V1<ManagerType>::~StyleKnob_V1() {}

//==============================================================================
template <typename ManagerType>
void StyleKnob_V1<ManagerType>::setTextPosition (int newPosition)
{
    textPosition = newPosition;
}

template <typename ManagerType>
void StyleKnob_V1<ManagerType>::setTextHeight (int newHeight)
{
    textHeight = newHeight;
}

template <typename ManagerType>
juce::Colour StyleKnob_V1<ManagerType>::getKnobColour (juce::Slider& slider) const noexcept
{
    const float amount { 0.16f };
    auto colour { slider.findColour (juce::Slider::backgroundColourId) };

    return colour.getLightness() < 0.5f ? colour.brighter (amount) : colour.darker (amount);
}

template <typename ManagerType>
void StyleKnob_V1<ManagerType>::setShouldDrawKnobLabel (bool shouldDraw)
{
    shouldDrawKnobLabel = shouldDraw;
}

template <typename ManagerType>
float StyleKnob_V1<ManagerType>::getKnobExtrusion() const noexcept
{
    return knob_extrusion;
}

template <typename ManagerType>
float StyleKnob_V1<ManagerType>::getLabelWidth (juce::Slider& slider) noexcept
{

    if (auto* editor { slider.findParentComponentOfClass<juce::AudioProcessorEditor>() })
    {
        if (auto* laf { dynamic_cast<StyleTheme*> (&editor->getLookAndFeel()) })
        {
            auto area { getSliderLayout (slider).textBoxBounds };
            auto font { laf->getCommonFont().withHeight (area.getHeight()).withKerningFactor (0.025f) };
            return juce::TextLayout::getStringWidth (font, labelText);
        }
    }

    return 0.0f;
}

//==============================================================================
template <typename ManagerType>
juce::Slider::SliderLayout StyleKnob_V1<ManagerType>::getSliderLayout (juce::Slider& slider)
{
    auto area { slider.getLocalBounds() };

    juce::Slider::SliderLayout layout;
    int space { 10 };

    switch (textPosition)
    {
        case top:
            layout.textBoxBounds = area.removeFromTop (textHeight);

            if (shouldDrawKnobLabel)
                area.removeFromTop (space);
            break;

        case bottom:
            layout.textBoxBounds = area.removeFromBottom (textHeight);

            if (shouldDrawKnobLabel)
                area.removeFromBottom (space);
            break;
    }

    layout.sliderBounds = area;

    return layout;
}

//==============================================================================
template <typename ManagerType>
void StyleKnob_V1<ManagerType>::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider)
{
    drawKnobExtrusion (g, slider);
    drawKnobCap (g, slider);
    drawKnobPointer (g, slider, sliderPos);

    if (shouldDrawKnobLabel)
        drawKnobLabel (g, slider);
}

//==============================================================================
template <typename ManagerType>
juce::Rectangle<float> StyleKnob_V1<ManagerType>::getKnobCapArea (juce::Slider& slider) noexcept
{
    auto area { getSliderLayout (slider).sliderBounds };
    return area.withSizeKeepingCentre (area.getWidth(), area.getWidth()).reduced (knob_bevelSize).toFloat();
}

template <typename ManagerType>
void StyleKnob_V1<ManagerType>::drawKnobExtrusion (juce::Graphics& g, juce::Slider& slider)
{
    juce::Path p;
    float rectX { getKnobCapArea (slider).getX() };
    float rectY { getKnobCapArea (slider).getCentreY() };
    float width { getKnobCapArea (slider).getWidth() };

    const float extrusion { jam::Value::clipMin (width * 0.125f, knob_extrusion) };

    juce::Rectangle<float> rect { rectX, rectY, width, extrusion };
    p.addRectangle (rect);

    float fromRadians { juce::degreesToRadians (90.0f) };
    float toRadians { juce::degreesToRadians (270.0f) };
    float innerCircleProportionalSize { 0.0f };
    p.addPieSegment ({ rectX, extrusion, width, getKnobCapArea (slider).getHeight() }, fromRadians, toRadians, innerCircleProportionalSize);

    g.setColour (getKnobColour (slider).darker (0.1f));
    g.fillPath (p);
}

template <typename ManagerType>
void StyleKnob_V1<ManagerType>::drawKnobCap (juce::Graphics& g, juce::Slider& slider)
{
    juce::Path bevel;
    bevel.addEllipse (getKnobCapArea (slider));

    auto shapeBounds { getSliderLayout (slider).sliderBounds.toFloat() };
    g.setGradientFill (jam::colours::getVerticalGradient (shapeBounds, getKnobColour (slider)));
    g.fillPath (bevel);

    juce::Path cap;
    cap.addEllipse (getKnobCapArea (slider).reduced (knob_bevelSize));
    g.setColour (getKnobColour (slider));
    g.fillPath (cap);
}

template <typename ManagerType>
void StyleKnob_V1<ManagerType>::drawKnobPointer (juce::Graphics& g, juce::Slider& slider, float sliderPos)
{
    const auto cap { getKnobCapArea (slider).reduced (knob_bevelSize) };
    const float centreX { cap.getCentreX() };
    const float centreY { cap.getCentreY() };
    const float radius { cap.getWidth() / 2 };
    const float angle { jam::Value::map (sliderPos, 0.0f, 1.0f, juce::degreesToRadians (-knob_angleLength), juce::degreesToRadians (knob_angleLength)) };
    const float pointerInset { 6.0f };

    juce::Path p;
    p.addRoundedRectangle (-knob_pointerThickness * 0.5f, -radius + pointerInset, knob_pointerThickness, (radius - pointerInset) * knob_pointerLength, knob_pointerRoundness);
    p.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
    g.setColour (slider.findColour (juce::Slider::thumbColourId));
    g.fillPath (p);

    pointerShadow.render (g, p);
}

template <typename ManagerType>
void StyleKnob_V1<ManagerType>::drawKnobLabel (juce::Graphics& g,
                                                juce::Slider& slider)
{
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
