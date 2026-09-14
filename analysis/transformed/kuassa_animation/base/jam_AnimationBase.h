/**
 * @file jam_AnimationBase.h
 * @brief Common base for timer-driven animated components.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class AnimationBase
 * @brief Base class for animated `juce::Component`s driven by a `juce::Timer`.
 *
 * Derived classes override start()/stop() to control the timer and paint()/timerCallback()
 * to render and advance each animation frame.
 */
class AnimationBase
    : public juce::Component
    , public juce::Timer
{
public:
    AnimationBase() = default;
    virtual ~AnimationBase() = default;

    /** Starts the animation. Base implementation does nothing. */
    virtual void start() {}

    /** Stops the animation. Base implementation does nothing. */
    virtual void stop() {}

    /**
     * @brief Sets the timer interval, in frames per second.
     * @param newValue  New interval, in Hz.
     */
    virtual void setInterval (int newValue)
    {
        if (newValue != timerIntervalHz)
            timerIntervalHz = newValue;
    }

    int timerIntervalHz { 24 }; ///< Timer interval, in Hz.

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationBase)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
