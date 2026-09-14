/**
 * @file jam_SliderIncDec.h
 * @brief Slider flanked by increment/decrement buttons.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class SliderIncDec
 * @brief Composes a text-box-less `juce::Slider` with leading decrement and
 * trailing increment buttons, each stepping the slider's value by one interval.
 */
class SliderIncDec
    : public juce::Component
    , public Model::ValueComponent<SliderIncDec>
    , juce::Slider::Listener
{
public:
    /** @brief Constructs with an empty component ID and a [0, 1] slider range. */
    SliderIncDec()
        : SliderIncDec (juce::String(), 0.0, 1.0)
    {
    }

    /**
     * @brief Constructs, wiring the increment/decrement buttons to step the slider.
     * @param componentID  Component ID.
     * @param minimum      Slider minimum value.
     * @param maximum      Slider maximum value.
     * @param interval     Slider step interval, also the increment/decrement step.
     * @param newButtons   Two buttons, in `{ increment, decrement }` order; ownership transfers.
     * @param style        Slider style.
     */
    SliderIncDec (juce::StringRef componentID,
                  double minimum,
                  double maximum,
                  double interval = 1.0,
                  std::initializer_list<std::unique_ptr<juce::Button>> newButtons = { std::make_unique<ButtonSVG> (Id::plus), std::make_unique<ButtonSVG> (Id::minus) },
                  juce::Slider::SliderStyle style = juce::Slider::LinearHorizontal)
        : Model::ValueComponent<SliderIncDec> (componentID)
        , buttons (newButtons)
    {
        slider = std::make_unique<juce::Slider> (style, juce::Slider::NoTextBox);
        slider->setComponentID (Id::value.toString());
        slider->setRange (minimum, maximum, interval);
        slider->addListener (this);
        addAndMakeVisible (slider.get());

        for (auto& button : buttons)
            addAndMakeVisible (button.get());

        //==============================================================================
        buttons.at (inc)->onClick = [this]
        {
            auto value { slider->getValue() };
            slider->setValue (++value, juce::sendNotification);
        };

        buttons.at (dec)->onClick = [this]
        {
            auto value { slider->getValue() };
            slider->setValue (--value, juce::sendNotification);
        };
    }

    /** @brief Default destructor. */
    ~SliderIncDec() {}

    /** Lays the decrement button, slider, and increment button out left to right. */
    void resized() override
    {
        auto area { getLocalBounds() };
        buttons.at (inc)->setBounds (area.removeFromRight (getHeight()));
        buttons.at (dec)->setBounds (area.removeFromLeft (getHeight()));
        slider->setBounds (area);
    }

    /** Fires onValueChange after the slider's value changes. */
    void sliderValueChanged (juce::Slider*) override
    {
        if (onValueChange != nullptr)
            onValueChange();
    }

    /** @return Reference to the underlying slider's `juce::Value`. */
    juce::Value& getValueObject() noexcept override
    {
        return slider->getValueObject();
    }

    /**
     * @brief Sets the slider's value, sending notification.
     * @param newValue  New value.
     */
    void setValue (double newValue)
    {
        slider->setValue (newValue, juce::sendNotification);
    }

    /**
     * @brief Sets the slider's range and step interval.
     * @param minimum   Minimum value.
     * @param maximum   Maximum value.
     * @param interval  Step interval.
     */
    void setRange (double minimum, double maximum, double interval = 1.0)
    {
        slider->setRange (minimum, maximum, interval);
    }
    //==============================================================================
    /** Called after the slider's value changes. */
    std::function<void()> onValueChange;
    //==============================================================================
private:
    /** @brief Button indices within the constructed buttons list. */
    enum
    {
        inc,
        dec,
    };

    std::unique_ptr<juce::Slider> slider;
    Owner<juce::Button> buttons;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SliderIncDec)
};

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam
