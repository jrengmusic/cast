/**
 * @file jam_MouseEvents.h
 * @brief CRTP mixin exposing JUCE mouse events as lambda callbacks.
 */
namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief CRTP mixin that exposes JUCE mouse events as lambda callbacks.
 *
 * Inherit from jam::MouseEvents<Base> instead of directly from Base
 * to gain a set of std::function hooks for all raw mouse events.
 *
 * This pattern makes any Component "bind-ready": auxiliaries can attach
 * themselves declaratively by assigning lambdas to onMouseEnter, onMouseExit,
 * etc., without subclassing again.
 *
 * @tparam Base The JUCE Component type to extend (e.g. juce::Slider, juce::Label).
 *
 * @code
 * using SliderEvents = jam::MouseEvents<juce::Slider>;
 *
 * SliderEvents slider;
 * slider.onMouseEnter = [] (const juce::MouseEvent& e) { DBG("Entered"); };
 * slider.onMouseExit  = [] (const juce::MouseEvent& e) { DBG("Exited"); };
 * @endcode
 */
template <typename BaseType>
struct MouseEvents : public BaseType
{
    using UnderlyingType = BaseType;///< breadcrumb must be declared for traits.
    using BaseType::BaseType;///< Inherit constructors from the base type.
    static constexpr bool isWrapper = true;///< Marker for Traits::IsWrapper

    /** Called when the mouse enters this component. */
    std::function<void()> onMouseEnter;

    /** Called when the mouse exits this component. */
    std::function<void()> onMouseExit;

    /** Called when a mouse button is pressed on this component. */
    std::function<void()> onMouseDown;

    /** Called when the right mouse button is clicked on this component. */
    std::function<void()> onRightClick;

    /** Called when a mouse button is released on this component. */
    std::function<void()> onMouseUp;

    /** Called when the mouse moves within this component. */
    std::function<void()> onMouseMove;

    /** Called when the mouse is dragged within this component. */
    std::function<void()> onMouseDrag;

    /** Called when the mouse is double-clicked on this component. */
    std::function<void()> onMouseDoubleClick;

    /** Called when the mouse wheel is moved over this component. */
    std::function<void()> onMouseWheelMove;

    /** Called when a drag gesture starts (mouseDown or mouseWheel). */
    std::function<void()> onDragStart;

    /** Called when a drag gesture ends (mouseUp or mouseWheel). */
    std::function<void()> onDragEnd;

    /** Forwarded from juce::Component::mouseEnter. */
    void mouseEnter (const juce::MouseEvent& e) override
    {
        BaseType::mouseEnter (e);
        if (onMouseEnter)
            onMouseEnter();
    }

    /** Forwarded from juce::Component::mouseExit. */
    void mouseExit (const juce::MouseEvent& e) override
    {
        BaseType::mouseExit (e);
        if (onMouseExit)
            onMouseExit();
    }

    /** Forwarded from juce::Component::mouseDown. */
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) // cross platfrom check
        {
            if (onRightClick)
                onRightClick();

            return;
        }

        BaseType::mouseDown (e);

        if (onMouseDown)
            onMouseDown();

        if (not isDragging)
        {
            isDragging = true;
            if (onDragStart)
                onDragStart();
        }
    }

    /** Forwarded from juce::Component::mouseUp. */
    void mouseUp (const juce::MouseEvent& e) override
    {
        BaseType::mouseUp (e);
        if (onMouseUp)
            onMouseUp();

        if (isDragging)
        {
            isDragging = false;
            if (onDragEnd)
                onDragEnd();
        }
    }

    /** Forwarded from juce::Component::mouseMove. */
    void mouseMove (const juce::MouseEvent& e) override
    {
        BaseType::mouseMove (e);
        if (onMouseMove)
            onMouseMove();
    }

    /** Forwarded from juce::Component::mouseDrag. */
    void mouseDrag (const juce::MouseEvent& e) override
    {
        BaseType::mouseDrag (e);
        if (onMouseDrag)
            onMouseDrag();
    }

    /** Forwarded from juce::Component::mouseDoubleClick. */
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        BaseType::mouseDoubleClick (e);
        if (onMouseDoubleClick)
            onMouseDoubleClick();
    }

    /** Forwarded from juce::Component::mouseWheelMove. */
    void mouseWheelMove (const juce::MouseEvent& e,
                         const juce::MouseWheelDetails& d) override
    {
        BaseType::mouseWheelMove (e, d);
        if (onMouseWheelMove)
            onMouseWheelMove();

        // Mousewheel is an atomic drag gesture (start + end in same tick)
        if (onDragStart)
            onDragStart();
        if (onDragEnd)
            onDragEnd();
    }

private:
    bool isDragging { false }; ///< Tracks whether a drag gesture is in progress, for onDragStart/onDragEnd pairing.
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
