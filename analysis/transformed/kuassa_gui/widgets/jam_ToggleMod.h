/**
 * @file jam_ToggleMod.h
 * @brief Slider-backed multi-state toggle button (click to toggle, right-click to cycle on-states).
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class ToggleMod
 * @brief A `juce::Slider` styled and driven as a button: left-click toggles
 * between 0 and the current on-state value; right-click (when
 * setClickingTogglesState() is enabled) cycles the on-state forward. Non-toggle
 * mode simply cycles the value forward on any click.
 */
class ToggleMod : public juce::Slider
{
public:
    /** @brief Constructs, styling the slider as a vertical linear bar with no editable text box. */
    ToggleMod()
    {
        /** Our slider is actually linear bar, because we want the textBox
            at the center */
        setSliderStyle (juce::Slider::LinearBarVertical);

        /** This must be set as false, so it behave like button */
        setSliderSnapsToMousePosition (false);
        setTextBoxIsEditable (false);
        setChangeNotificationOnlyOnRelease (true);
    }

    /** No-op — double-click does not reset the value. */
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
    }

    /** No-op — mouse wheel does not change the value. */
    void mouseWheelMove (const juce::MouseEvent&,
                         const juce::MouseWheelDetails&) override
    {
    }

    /** Toggles or cycles the value per the current click button and toggle mode, then forwards to the base slider. */
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (not isEnabled())
            return;

        const auto modifiers { juce::ModifierKeys::getCurrentModifiers() };
        const bool isLeftClick { modifiers.isLeftButtonDown() };
        const bool isRightClick { modifiers.isRightButtonDown() };

        if (! isLeftClick && ! isRightClick)
            return;

        int currentValue { toInt (getValue()) };
        int maxValue { toInt (getMaximum()) + 1 };
        int onStateValue { std::max (1, currentState) };
        int newValue { currentValue };

        if (isToggle)
        {
            if (isLeftClick)
            {
                // Toggle between 0 and valid onState (never 0)
                newValue = (currentValue == 0) ? onStateValue : 0;
            }
            else if (isRightClick)
            {
                // Cycle onState forward, skipping 0
                onStateValue = ((onStateValue + 1) % maxValue);
                if (onStateValue == 0)
                    onStateValue = 1;

                newValue = onStateValue;

                currentState = onStateValue;
            }
        }
        else
        {
            // Non-toggle: cycle current value, skipping 0
            newValue = ((currentValue + 1) % maxValue);

            if (newValue == 0)
                newValue = 1;

            currentState = newValue;
        }

        setValue (newValue);

        /** required for certain DAW to trigger parameter automation changes*/
        juce::Slider::mouseDown (e);
    }

    /**
     * @brief Sets whether left-click toggles between 0 and the on-state (vs. always cycling forward).
     * @param shouldAutoToggleOnClick  `true` for toggle mode.
     */
    void setClickingTogglesState (bool shouldAutoToggleOnClick)
    {
        isToggle = shouldAutoToggleOnClick;
    }

    /** @return `true` when the current value is greater than 0. */
    bool getToggleState() const noexcept
    {
        int value { static_cast<int> (getValue()) };
        return value > 0.0;
    }
    //==============================================================================

private:
    bool isToggle { false };
    int currentState { true };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToggleMod)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
