/**
 * @file jam_ButtonToggleValue.h
 * @brief Two-state toggle button bound to a value drawn from a key/label map.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class ButtonToggleValue
 * @brief Wraps an owned two-state `juce::Button` whose toggle state is mapped
 * to and from a `juce::Value`, via the injected `BimapType`'s one-based keys
 * (1 = off, 2 = on) to value strings.
 */
template <typename BimapType>
class ButtonToggleValue
    : public juce::Component
    , public Model::ValueComponent<ButtonToggleValue<BimapType>>
    , juce::Value::Listener
{
public:
    ButtonToggleValue()
        : Model::ValueComponent<ButtonToggleValue<BimapType>> (juce::String())
    {
        if (auto* bimap { BimapType::getInstance() })
            value = bimap->getDefault();
    }

    ~ButtonToggleValue() = default;

    /** Stretches the toggle button to fill this component's local bounds. */
    void resized() override
    {
        button->setBounds (getLocalBounds());
    }

    /** @return Reference to the underlying `juce::Value` driving toggle state. */
    juce::Value& getValueObject() noexcept override
    {
        return value;
    }

    /**
     * @brief Binds the toggle button and its value to a panel model.
     *
     * Takes the registered button via `Registry::toButton`, configures it as a
     * click-toggled, trigger-on-mouse-down toggle, attaches `value` to the model
     * parameter named by the element's `Id::dataParameter`, and wires the click
     * handler to flip between the bimap's two keys. No guards — the button and
     * parameter are trusted to exist.
     *
     * @param reg      Registry supplying the registered button.
     * @param element  Component descriptor providing the button and parameter keys.
     * @param model    Audio model owning the bound parameter.
     */
    void bindToPanel (Registry& reg, const Document::Element& element, AudioModel& model)
    {
        button = Registry::toButton (reg.make.get (Registry::getButton (element)));
        juce::Component::addAndMakeVisible (button.get());
        button->setClickingTogglesState (true);
        button->setTriggeredOnMouseDown (true);

        const auto descParam { Registry::getParameter (element) };
        Model::attach (model.state, value, juce::Identifier { descParam }, Id::value);
        value.addListener (this);

        button->onClick = [this]
        {
            if (auto* bimap { BimapType::getInstance() })
            {
                const auto key { bimap->get (value.toString()) };
                value.setValue (bimap->get (key == 1 ? 2 : 1));
            }
        };
    }

    //==============================================================================
    /**
     * @brief Reflects a value change onto the toggle button's state.
     * @param newValue  The `juce::Value` that changed.
     */
    void valueChanged (juce::Value& newValue) override
    {
        if (auto* bimap { BimapType::getInstance() })
            button->setToggleState (bimap->get (newValue.toString()) - 1, juce::dontSendNotification);
    }
    //==============================================================================
protected:
    juce::Value value;                                      ///< Value bound to the toggle button's state.
    std::unique_ptr<juce::Button> button;                    ///< Owned toggle button.

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonToggleValue)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
