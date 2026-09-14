/**
 * @file        jam_ButtonGroup.h
 * @brief       Radio-style group of toggle buttons with a sliding selection indicator.
 */
namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class ButtonGroup
 * @brief Radio-style group of toggle buttons sharing one `juce::Value`, with a
 * sliding indicator animated to the currently selected button.
 *
 * Buttons added via addButton() (with `isFreeButton` false) join a shared radio
 * group and toggle the group's value; the sliding indicator follows the toggled
 * button, snapping on layout and animating on selection change.
 */
class ButtonGroup
    : public juce::Component
    , public Model::ValueComponent<ButtonGroup>
    , public juce::Value::Listener
{
public:
    /** @brief Colour identifiers for `jam::ButtonGroup`. */
    enum ColourIds
    {
        trackFillColourId        = map::ColourId::buttonGroupTrackFillColourId,
        trackOutlineColourId     = map::ColourId::buttonGroupTrackOutlineColourId,
        indicatorFillColourId    = map::ColourId::buttonGroupIndicatorFillColourId,
        indicatorOutlineColourId = map::ColourId::buttonGroupIndicatorOutlineColourId,
    };

    /** @brief Sliding selection indicator painted between the track and the buttons. */
    class SlidingIndicator : public juce::Component
    {
    public:
        /** @brief Paint via `jam::StyleCustom::drawButtonGroupSlidingIndicator`. */
        void paint (juce::Graphics& g) override
        {
            if (auto* laf = dynamic_cast<jam::StyleCustom*> (&getLookAndFeel()))
                laf->drawButtonGroupSlidingIndicator (g, *this);
        }
    };

    /** @brief Constructs an empty group, listening to its own shared value. */
    ButtonGroup();

    /** @brief Default destructor. */
    ~ButtonGroup() = default;

    /** Overrides the default horizontal-row layout when set. */
    std::function<void()> resizeFunction;

    /** Draws the group's track via `jam::StyleCustom::drawButtonGroupTrack`. */
    void paint (juce::Graphics& g) override;

    /**
     * @brief Lays out buttons as a horizontal row of the given height, inset by the provided margin values.
     * @param rowHeight Height of the row strip to consume from the top of local bounds.
     * @param insetX    Horizontal inset applied via juce::Rectangle::reduced (default 2).
     * @param insetY    Vertical inset applied via juce::Rectangle::reduced (default 2).
     */
    void makeRow (int rowHeight, int insetX = 0, int insetY = 0);

    /** @brief Lays out buttons via resizeFunction, or makeRow() by default, then snaps the indicator. */
    void resized() override;

    /**
     * @brief Sets the group's shared value.
     * @param newValue  New value; matched against toggle button text.
     */
    void setValue (juce::var newValue) { value.setValue (newValue); }

    /** @return The group's shared value. */
    juce::var getValue() const noexcept { return value.getValue(); }

    /** @return The group's shared `juce::Value` object. */
    juce::Value& getValueObject() noexcept override { return value; }

    /** Called after the group's value changes and the indicator has been re-targeted. */
    std::function<void()> onValueChanged;

    /**
     * @brief Reconciles a changed button toggle state or the shared value with all group buttons.
     * @param valueThatChanged  The `juce::Value` that changed — either a button's toggle state or the group's own value.
     */
    void valueChanged (juce::Value& valueThatChanged) override;

    /**
     * @brief Attaches the group's value to an `AudioModel` parameter property.
     * @param model           Audio model owning the parameter.
     * @param parameterID     Parameter identifier.
     * @param valueProperty   Property within the parameter to bind to.
     */
    void attachTo (AudioModel& model, const juce::Identifier& parameterID, const juce::Identifier& valueProperty)
    {
        model.attach (value, parameterID, valueProperty);
    }

    /**
     * @brief Binder attaching a `ButtonGroup`'s shared value to an audio model
     * parameter property named by the group's `Id::id` component property.
     *
     * Not RAII — the constructor performs the binding and there are no members
     * to release; the binding itself lives in the model's `juce::Value`, not
     * in this object.
     *
     * @par Ordering contract
     * The group's `Id::id` property must already be set before construction —
     * the component config lane runs before the bind lane. An unset `Id::id`
     * yields an empty property identifier and the attachment binds to nothing.
     */
    class Attachment
    {
    public:
        /**
         * @brief Constructs the attachment and binds @p group to the given parameter.
         * @param state        Audio model owning the parameter.
         * @param parameterID  Parameter identifier.
         * @param group        Button group to bind.
         */
        Attachment (AudioModel& state, const juce::String& parameterID, ButtonGroup& group)
        {
            group.attachTo (state, parameterID, group.getProperties()[Id::id].toString());
        }
    };

    Owner<juce::Button> buttons; ///< Owned buttons in the group, in add order.

    /**
     * @brief Adds a button to the group, wiring it into the shared radio group unless free.
     * @param newButton    Button to add; ownership transfers to the group.
     * @param isFreeButton  `true` to add without joining the radio group or toggle-value binding.
     */
    void addButton (std::unique_ptr<juce::Button> newButton, bool isFreeButton = false);

    /**
     * @brief Enables or disables a button by its one-based index.
     * @param itemIndex  One-based button index.
     * @param enabled    `true` to enable the button.
     */
    void setItemEnabled (int itemIndex, bool enabled) { buttons.at (itemIndex - 1)->setEnabled (enabled); }

protected:
    juce::Value value; ///< Shared value backing the group's selection.

private:
    /** @brief Immediately places the indicator over the currently toggled button, cancelling any running animation. */
    void snapIndicator();

    /** @brief Animates the indicator to slide over the currently toggled button. */
    void animateIndicator();

    SlidingIndicator indicator; ///< Sliding selection indicator child component.
    juce::ComponentAnimator animator; ///< Drives the sliding indicator animation.

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonGroup)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
