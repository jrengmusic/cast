/**
 * @file        jam_ButtonMenu.h
 * @brief       Button-triggered popup menu component.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

using PopupDirection = juce::PopupMenu::Options::PopupDirection;

/**
 * @brief Wraps an owned juce::Button that opens a juce::PopupMenu on click.
 *
 * ButtonMenu owns no menu state of its own — the menu content is built on demand
 * by the factory passed to setMenuFactory(), and the chosen result is
 * forwarded to the action set via setAction(). The wrapped button fills the
 * component's local bounds.
 */
class ButtonMenu : public juce::Component
{
public:
    /** @brief Constructs an empty ButtonMenu with no button, action, or menu factory set. */
    ButtonMenu() = default;
    /** @brief Default destructor. */
    ~ButtonMenu() override = default;

    /**
     * @brief Takes ownership of the button and wires its click to showMenu().
     *
     * @param newButton The button to display; becomes a child component.
     */
    void setButton (std::unique_ptr<juce::Button> newButton)
    {
        button = std::move (newButton);
        juce::Component::addAndMakeVisible (button.get());

        button->onClick = [this]
        {
            showMenu();
        };
    }

    /**
     * @brief Binds the registered trigger button to this menu for a panel model.
     *
     * Takes the registered button via `Registry::toButton` and passes it to
     * setButton(). No guards — the button is trusted to exist.
     *
     * @param reg      Registry supplying the registered button.
     * @param element  Component descriptor providing the button key.
     * @param model    Unused — no model binding is performed.
     */
    void bindToPanel (Registry& reg, const Document::Element& element, AudioModel& model)
    {
        setButton (Registry::toButton (reg.make.get (Registry::getButton (element))));
    }

    /** @brief Sets the callback invoked with the chosen item ID when the menu closes with a result > 0. */
    void setAction (std::function<void (int)> newAction) { onAction = std::move (newAction); }

    /** @brief Sets the factory that builds the juce::PopupMenu shown by showMenu(). */
    void setMenuFactory (std::function<juce::PopupMenu()> factory) { menuFactory = std::move (factory); }

    /** @brief Sets the preferred direction the popup opens relative to the button. */
    void setPopupDirection (PopupDirection direction) { popupDirection = direction; }

    /** @brief Forces the wrapped button's visual state, e.g. for external hover control. */
    void setButtonState (juce::Button::ButtonState newState)
    {
        button->setState (newState);
    }

    /** @brief Stretches the wrapped button to fill this component's local bounds. */
    void resized() override
    {
        button->setBounds (getLocalBounds());
    }

    /**
     * @brief Builds and shows the popup menu asynchronously against this component.
     *
     * Fires onButtonClick before building the menu. Deletion of this component
     * while the menu is open is checked via withDeletionCheck. onAction is only
     * invoked when the menu closes with a result greater than zero.
     */
    void showMenu()
    {
        if (auto* topLevelComponent { getTopLevelComponent() })
        {
            if (onButtonClick != nullptr)
                onButtonClick();

            if (menuFactory != nullptr)
            {
                auto menu { menuFactory() };

                menu.setLookAndFeel (&getLookAndFeel());

                auto options { juce::PopupMenu::Options()
                                   .withMinimumWidth (menuMinimumWidth)
                                   .withPreferredPopupDirection (popupDirection)
                                   .withDeletionCheck (*this)
                                   .withTargetComponent (this) };

                jam::showAsync (menu, options, [this] (int result)
                {
                    if (result > 0 and onAction != nullptr)
                        onAction (result);
                });
            }
        }
    }

    //==============================================================================
    /** @brief Fired immediately before the popup menu is built and shown. */
    std::function<void()> onButtonClick;
    //==============================================================================

private:
    std::unique_ptr<juce::Button> button;
    std::function<void (int)> onAction;
    std::function<juce::PopupMenu()> menuFactory;
    PopupDirection popupDirection { PopupDirection::downwards };
    static constexpr int menuMinimumWidth { 170 };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonMenu)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
