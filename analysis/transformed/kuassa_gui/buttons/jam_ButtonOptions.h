/**
 * @file        jam_ButtonOptions.h
 * @brief       Options button — owned trigger that opens a popup menu
 *              populated from a jam::HashMap<int, juce::String>.
 */
namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Button that opens a popup menu of selectable string options.
 *
 * Owns a trigger `juce::Button` and a `juce::Value` representing the currently
 * selected key (encoded as a string). Clicking the trigger displays a
 * `juce::PopupMenu` built from the supplied `jam::HashMap<int, juce::String>`;
 * selection updates `selectedItem`.
 *
 * The trigger is positioned by `resized()` to fill local bounds.
 *
 * @note MESSAGE THREAD.
 */
class ButtonOptions
    : public juce::Component
    , public Model::ValueComponent<ButtonOptions>
{
public:
    ButtonOptions()
        : Model::ValueComponent<ButtonOptions> (juce::String())
    {
    }

    /**
     * @brief Constructs a ButtonOptions with a trigger, a key->label map,
     *        an initial value, and a preferred popup direction.
     *
     * Takes ownership of @p newButton, makes it visible, and wires its click
     * handler to display the popup menu. The componentID is also used as the
     * default menu section header.
     *
     * @param componentID    Identifier propagated to `juce::Component::setComponentID`
     *                       and used as the default menu header.
     * @param newOptions     Accessor returning the key->label map for the popup items.
     * @param newButton      Trigger button. Ownership is transferred.
     * @param initValue      Initial value for `selectedItem`.
     * @param popupDirection Preferred direction the popup opens; default `upwards`.
     */
    ButtonOptions (const juce::String& componentID,
             std::function<const jam::HashMap<int, juce::String>&()> newOptions,
             std::unique_ptr<juce::Button> newButton,
             juce::var initValue,
             PopupDirection popupDirection = PopupDirection::upwards)
        : Model::ValueComponent<ButtonOptions> (componentID)
        , options (std::move (newOptions))
        , selectedItem (initValue)
        , menuHeader (componentID)
    {
        button = std::move (newButton);
        juce::Component::addAndMakeVisible (button.get());
        wireButton (popupDirection);
    }

    ~ButtonOptions() = default;

    /**
     * @brief Binds the trigger button and selected value to a panel model.
     *
     * Takes the registered button via `Registry::toButton`, attaches `selectedItem`
     * to the model parameter named by the element's `Id::dataParameter`, and wires
     * the click handler. No guards — the button and parameter are trusted to exist.
     *
     * @param reg      Registry supplying the registered button.
     * @param element  Component descriptor providing the button and parameter keys.
     * @param model    Audio model owning the bound parameter.
     */
    void bindToPanel (Registry& reg, const Document::Element& element, AudioModel& model)
    {
        button = Registry::toButton (reg.make.get (Registry::getButton (element)));
        juce::Component::addAndMakeVisible (button.get());
        menuHeader = Registry::getParameter (element);
        Model::attach (model.state, selectedItem, juce::Identifier { menuHeader }, Id::value);
        wireButton (popupDirection);
    }

    /**
     * @brief Sets the option accessor and default value.
     *
     * @param newOptions   Accessor returning the key->label map for popup items.
     * @param defaultValue Default value for `selectedItem`.
     */
    void setOptions (std::function<const jam::HashMap<int, juce::String>&()> newOptions, const juce::String& defaultValue)
    {
        options = std::move (newOptions);
        selectedItem = defaultValue;
    }

    /**
     * @brief Sets the section header text shown at the top of the popup menu.
     *
     * @param newText Header text. Underscores are stripped by `jam::menu::createOptions`.
     */
    void setMenuHeader (const juce::String& newText) { menuHeader = newText; }

    /**
     * @brief Sets the preferred direction the popup opens.
     *
     * @param direction New popup direction.
     */
    void setPopupDirection (PopupDirection direction) { popupDirection = direction; }

    /** @brief Lays the trigger button out to fill local bounds. */
    void resized() override
    {
        button->setBounds (getLocalBounds());
    }

    /**
     * @brief Returns the underlying value object driving selection.
     *
     * @return Reference to the internal `juce::Value`.
     */
    juce::Value& getValueObject() noexcept override { return selectedItem; }

    //==============================================================================
private:
    std::function<const jam::HashMap<int, juce::String>&()> options; ///< Accessor returning the key->label map for the popup items.
    juce::Value selectedItem;
    std::unique_ptr<juce::Button> button;
    juce::String menuHeader;
    PopupDirection popupDirection { PopupDirection::upwards };

    /**
     * @brief Wires the trigger button's click handler to display the popup menu.
     *
     * Captures @p direction by value for use when the menu is shown asynchronously.
     *
     * @param direction Preferred popup direction.
     */
    void wireButton (PopupDirection direction)
    {
        button->onClick = [this, direction]
        {
            if (auto* topLevelComponent { getTopLevelComponent() }; topLevelComponent != nullptr)
            {
                juce::PopupMenu menu { jam::menu::createOptions (
                    options(), menuHeader, topLevelComponent->getWidth(), selectedItem.getValue().toString()) };
                menu.setLookAndFeel (&getLookAndFeel());

                auto popupOptions { juce::PopupMenu::Options()
                                   .withPreferredPopupDirection (direction)
                                   .withDeletionCheck (*this)
                                   .withTargetComponent (this) };

                jam::showAsync (menu, popupOptions,
                                    [this] (int result)
                                    {
                                        if (result > 0)
                                        {
                                            juce::Value newValue { options().at (result) };
                                            if (not (selectedItem.getValue() == newValue))
                                                selectedItem.setValue (newValue);
                                        }
                                        repaint();
                                    });
            }
        };
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonOptions)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
