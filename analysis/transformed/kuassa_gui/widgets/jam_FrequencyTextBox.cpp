namespace jam
{
/*____________________________________________________________________________*/

FrequencyTextBox::FrequencyTextBox()
    : juce::Slider()
{
    setBufferedToImage (true);

    textModes.add<const juce::String&> (map::VariDisplayMode::value::note,
                                        [] (const juce::String& t) -> std::optional<double>
                                        {
                                            if (Format::isValidNoteName (t))
                                                return Format::getFrequencyInHzFromNoteName<double> (t);
                                            return {};
                                        });
    textModes.add<const juce::String&> (map::VariDisplayMode::value::khz,
                                        [] (const juce::String& t) -> std::optional<double>
                                        {
                                            if (Format::isKilo (t))
                                                return t.getDoubleValue() * 1000.0;
                                            return {};
                                        });
    textModes.add<const juce::String&> (map::VariDisplayMode::value::hz,
                                        [] (const juce::String& t) -> std::optional<double>
                                        {
                                            if (not Format::isValidNoteName (t) and not Format::isKilo (t))
                                                return t.getDoubleValue();
                                            return {};
                                        });
}

//==============================================================================

double FrequencyTextBox::getValueFromText (const juce::String& text)
{
    for (const auto& [mode, element] : textModes)
    {
        juce::ignoreUnused (element);

        if (const auto frequency { textModes.get (mode, text) })
        {
            if (auto* m { map::VariDisplayMode::getInstance() })
                modeValue.setValue (m->get (mode));

            return *frequency;
        }
    }

    return 0.0;
}

//==============================================================================

juce::String FrequencyTextBox::getTextFromValue (double value)
{
    return juce::String (value);
}

//==============================================================================

void FrequencyTextBox::mouseDown (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
    showTextBox();
}

//==============================================================================

void FrequencyTextBox::lookAndFeelChanged()
{
    juce::Slider::lookAndFeelChanged();

    // juce::ListenerList dedup is implicit — safe under repeated LAF changes.
    // The base call above recreates the valueBox Label; we re-attach here so
    // the Label::Listener registration survives the swap.
    for (auto& child : getChildren())
    {
        if (auto* label { dynamic_cast<juce::Label*> (child) })
            label->addListener (this);
    }
}

//==============================================================================

void FrequencyTextBox::editorShown (juce::Label* label, juce::TextEditor& editor)
{
    juce::ignoreUnused (label);
    editor.setPopupMenuEnabled (false);
    editor.setInputRestrictions (0, "0123456789.ABCDEFGabcdefg#kK");
}

//==============================================================================

void FrequencyTextBox::labelTextChanged (juce::Label*) {}

//==============================================================================

void FrequencyTextBox::mouseDrag (const juce::MouseEvent&) {}

//==============================================================================

void FrequencyTextBox::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) {}

//==============================================================================

void FrequencyTextBox::bindToModel (const juce::String& parameterID, AudioModel& state)
{
    if (parameterID.isNotEmpty())
    {
        if (auto* m { map::VariDisplayMode::getInstance() })
        {
            if (modeValue.getValue().toString().isEmpty())
                modeValue.setValue (m->getDefault());
        }

        state.attach (modeValue, parameterID, Id::mode);
        setTextValueSuffix ({});
    }
}

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
