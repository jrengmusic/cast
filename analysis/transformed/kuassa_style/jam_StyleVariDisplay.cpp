namespace jam
{
/*____________________________________________________________________________*/

void StyleVariDisplay::setImage (const juce::Image& newImage) { image = newImage; }

void StyleVariDisplay::setDisplayMode (int mode)
{
    static const jam::HashMap<int, int> modeDigitLengths {
        { map::VariDisplayMode::value::hz, 5 },
        { map::VariDisplayMode::value::khz, 2 },
        { map::VariDisplayMode::value::note, 5 }
    };

    static const jam::HashMap<int, int> modeDecimalPlaces {
        { map::VariDisplayMode::value::hz, 0 },
        { map::VariDisplayMode::value::khz, 2 },
        { map::VariDisplayMode::value::note, 0 }
    };

    displayMode = mode;
    digitLength = modeDigitLengths.at (mode);
    decimalPlaces = modeDecimalPlaces.at (mode);
}

void StyleVariDisplay::drawLinearSlider (juce::Graphics& g,
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
        int sourceY { slider.isEnabled() * slider.getHeight() };
        g.drawImage (image, 0, 0, width, height, 0, sourceY, width, height);
    }
}

juce::Font StyleVariDisplay::getLabelFont (juce::Label& label)
{
    return segmentFont.withHeight (label.getHeight());
}

juce::Font StyleVariDisplay::getTextButtonFont (juce::TextButton& button, int newButtonHeight)
{
    return boldFont.withHeight (0.6f * newButtonHeight);
}

juce::Label* StyleVariDisplay::createSliderTextBox (juce::Slider& slider)
{
    auto textBox { new SliderLabelComp() };
    textBox->setJustificationType (juce::Justification::right);
    textBox->setKeyboardType (juce::TextInputTarget::textKeyboard);
    textBox->setMinimumHorizontalScale (1.0f);

    return textBox;
}

void StyleVariDisplay::drawLabel (juce::Graphics& g, juce::Label& label)
{
    auto alpha { label.isEnabled() ? 1.0f : dimFactor };

    if (not label.isBeingEdited())
    {
        juce::String toDisplay;
        int numOctaveIndex { -1 };
        bool isShowNoteName { false };
        juce::String text;

        const float value { label.getText().getFloatValue() };

        switch (displayMode)
        {
            case map::VariDisplayMode::value::hz:
                text = juce::String { value, decimalPlaces };
                break;
            case map::VariDisplayMode::value::khz:
                text = juce::String { value * 0.001f, decimalPlaces };
                break;
            case map::VariDisplayMode::value::note:
                isShowNoteName = true;
                break;
            default:
                break;
        }

        if (isShowNoteName)
        {
            label.setJustificationType (juce::Justification::left);

            /** If text drawn as note digit length always 4,
                 [0] key center [1] # symbol if any and last two digit for octave number;
                 */

            juce::String note { jam::Format::getMIDINoteNameFromHz (value) };

            /** determine where the octave number start index*/
            numOctaveIndex = note.indexOfAnyOf ("0123456789-");

            /** use # symbol as pad character for pitch, because we want fix width
                 string whether it's natural or sharps */
            juce::juce_wchar sharp { *juce::CharPointer_UTF8 ("\x23") };
            juce::String pitch { note.substring (0, numOctaveIndex).paddedRight (sharp, 2) };
            juce::String octaveNumber { note.substring (numOctaveIndex).paddedRight (padCharacter, 2) };

            toDisplay = pitch + octaveNumber;
        }
        else
        {
            label.setJustificationType (juce::Justification::right);

            juce::String dot { "." };
            juce::String wholeNumber { text.upToFirstOccurrenceOf (dot, false, true) };

            toDisplay = wholeNumber.paddedLeft (padCharacter, digitLength);

            if (decimalPlaces)
            {
                juce::juce_wchar zero { '0' };
                juce::String fractions { text.fromFirstOccurrenceOf (dot, false, true) };
                toDisplay += dot + fractions.paddedRight (zero, decimalPlaces);
            }
        }

        juce::AttributedString formatted { toDisplay };
        juce::Font font { getLabelFont (label) };
        float stringWidth { juce::TextLayout::getStringWidth (font, toDisplay) };
        auto area { label.getLocalBounds().toFloat() };

        /** the following will try its best to draw the digit at available space */
        font.setHeight (juce::jmin (area.getHeight(), area.getHeight() * (area.getWidth() / stringWidth)));

        formatted.setFont (font);
        formatted.setJustification (label.getJustificationType());
        formatted.setColour (findColour (juce::Label::textColourId).withMultipliedAlpha (alpha));

        /** if key is natural, don't draw sharps*/
        if (isShowNoteName and (numOctaveIndex == 1))
        {
            formatted.setColour ({ numOctaveIndex, numOctaveIndex + 1 }, juce::Colour());
            formatted.setJustification (juce::Justification::left);
        }

        formatted.draw (g, area.withSizeKeepingCentre (label.getWidth(), font.getHeight()));
    }
}

juce::Slider::SliderLayout StyleVariDisplay::getSliderLayout (juce::Slider& slider)
{
    juce::Slider::SliderLayout layout;

    auto area { slider.getLocalBounds().reduced (edgeIndent) };

    layout.sliderBounds = area;
    layout.textBoxBounds = area.removeFromBottom (area.getHeight() - buttonHeight).reduced (edgeIndent);

    return layout;
}

void StyleVariDisplay::drawButtonBackground (juce::Graphics&,
                                             juce::Button&,
                                             const juce::Colour& backgroundColour,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
}

void StyleVariDisplay::drawButtonText (juce::Graphics& g,
                                       juce::TextButton& button,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown)
{
    const auto alpha { button.isEnabled() ? 1.0f : dimFactor };
    const bool state { button.getToggleState() };
    juce::Colour colour { findColour (state ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId) };

    if (shouldDrawButtonAsHighlighted)
    {
        colour = colour.brighter (0.5f);
    }

    g.setColour (colour.withMultipliedAlpha (alpha));

    auto area { button.getLocalBounds() };
    juce::Font font (getTextButtonFont (button, area.getHeight()));

    g.setFont (font);
    g.drawFittedText (button.getButtonText(), area, juce::Justification::right, 1, 1.0f);
}
/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
