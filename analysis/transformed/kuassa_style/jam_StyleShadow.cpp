namespace jam
{
/*____________________________________________________________________________*/
void StyleShadow::setImage (const juce::Image& newImage) { image = newImage; }

/** @cond */
#if JUCE_MODULE_AVAILABLE_jam_gui
void StyleShadow::drawToggleButton (juce::Graphics& g,
                                    juce::ToggleButton& button,
                                    bool shouldDrawButtonAsHighlighted,
                                    bool shouldDrawButtonAsDown)
{
    if (image.isValid())
    {
        auto bounds { button.getLocalBounds() };
        const int width { bounds.getWidth() };
        const int height { bounds.getHeight() };

        const int frame { button.isDown() ? juce::Button::buttonDown : jam::toInt (button.getToggleState()) };
        g.drawImage (image, 0, 0, width, height, 0, frame * height, width, height);
    }
}

void StyleShadow::drawLinearSlider (juce::Graphics& g,
                                    int x,
                                    int y,
                                    int width,
                                    int height,
                                    float sliderPos,
                                    float minSliderPos,
                                    float maxSliderPos,
                                    const juce::Slider::SliderStyle style,
                                    juce::Slider& slider)
{
    if (image.isValid())
    {
        auto minY { slider.valueToProportionOfLength (slider.getMinimum())
                    * height };
        auto maxY { slider.valueToProportionOfLength (slider.getMaximum())
                    * height };
        int yPosition { jam::Value::map (
            jam::Value::normalise (static_cast<double> (sliderPos), minY, maxY),
            minY,
            (height - image.getHeight())) };

        g.drawImageAt (image, x, jam::toInt (yPosition));
    }
}
#endif
/** @endcond */
/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
