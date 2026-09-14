/**
 * @file jam_OverlayImage.h
 * @brief CRTP mixin binding a component as a z-ordered image overlay behind a primary component.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class OverlayImage
 * @brief CRTP mixin adding bindTo(), which — when the primary is a
 * juce::Button — attaches this component as a private juce::Button::Listener
 * so it mirrors the primary's `ButtonState` onto itself on every
 * hover/press/toggle change, without a paint-scheduling-paint hack.
 *
 * Z-order is not managed by this mixin. It follows component.md row order:
 * an overlay's row precedes its primary's row, so the component tree already
 * places the overlay behind the primary it shadows.
 *
 * The pattern is consumed by `jam::StyleShadow::drawToggleButton()`/
 * `drawLinearSlider()`, which read the overlay's own mirrored state.
 *
 * No removeListener in the destructor: the overlay and its primary share the
 * same parent component and are torn down together as that parent's children.
 *
 * @tparam ComponentType  The JUCE Component type to extend.
 */
template <typename ComponentType>
class OverlayImage : public ComponentType
                    , private juce::Button::Listener
{
public:
    using UnderlyingType = ComponentType;///< breadcrumb must be declared for traits.
    using ComponentType::ComponentType;///< Inherit constructors from the base type.
    static constexpr bool isWrapper { true };///< Marker for Traits::IsWrapper

    ~OverlayImage() = default;

    /**
     * @brief Binds this overlay to a primary component — when @p primary is a
     * juce::Button, attaches this overlay as a Button::Listener so it mirrors
     * the primary's hover/press/toggle state onto itself.
     * @param primary  Primary component this overlay shadows.
     */
    void bindTo (juce::Component* primary)
    {
        if (primary != nullptr)
        {
            if (auto* primaryButton { dynamic_cast<juce::Button*> (primary) })
                primaryButton->addListener (this);
        }
    }

private:
    /** @brief Repaints the overlay when the primary button is clicked. */
    void buttonClicked (juce::Button*) override { this->repaint(); }
    /** @brief Repaints the overlay when the primary button's hover/press state changes. */
    void buttonStateChanged (juce::Button* primary) override
    {
        if constexpr (std::is_base_of_v<juce::Button, ComponentType>)
            this->setState (primary->getState());
        else
            this->repaint();
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OverlayImage)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
