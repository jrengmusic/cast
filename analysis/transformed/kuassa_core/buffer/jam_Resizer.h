/**
 * @file jam_Resizer.h
 * @brief Timer-coalesced resize coordinator built on Function::Map triggers.
 */

#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/**
 * @brief Timer-driven resize coordinator — coalesces rapid dimension changes.
 *
 * 16ms coalescing timer with optional start trigger and mandatory stop trigger
 * via Function::Map.  set() fires the named start trigger only if it has been
 * registered via addTrigger(); if no trigger is registered under that name the
 * call is a no-op for the trigger and the timer starts unconditionally.
 * When the timer fires, it fires the "stop" trigger.
 */
class Resizer : private juce::Timer
{
public:
    Resizer() noexcept = default;
    ~Resizer() override = default;

    /**
     * @brief Registers a callback under @p name, invoked with (width, height) on set().
     *
     * @tparam Args         Unused; callback signature is fixed to (int&, int&).
     * @tparam FunctionType Callback type, forwarded into Function::Map.
     * @param name      Identifier the callback is registered under.
     * @param callback  Callback invoked with the pending width and height.
     */
    template <typename... Args, typename FunctionType>
    void addTrigger (const juce::Identifier& name, FunctionType&& callback)
    {
        triggers.add<int&, int&> (name, std::forward<FunctionType> (callback));
    }

    /**
     * @brief Records a pending resize target and (re)starts the coalescing timer.
     *
     * Fires the trigger registered under @p name, if any, then arms the timer;
     * the "stop" trigger fires once the timer elapses without a further set() call.
     *
     * @param name    Identifier of the start trigger to fire, if registered.
     * @param width   Pending target width.
     * @param height  Pending target height.
     */
    void set (const juce::Identifier& name, int width, int height)
    {
        pendingWidth  = width;
        pendingHeight = height;

        if (triggers.contains (name))
            triggers.get (name, width, height);

        isTransitioning = true;
        startTimer (tickIntervalMs);
    }

    /** @brief Returns true while a resize is pending (timer armed, "stop" not yet fired). */
    bool isInTransition()  const noexcept { return isTransitioning; }
    /** @brief Returns the most recently set pending target width. */
    int  getTargetWidth()  const noexcept { return pendingWidth; }
    /** @brief Returns the most recently set pending target height. */
    int  getTargetHeight() const noexcept { return pendingHeight; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Resizer)

private:
    void timerCallback() override
    {
        isTransitioning = false;
        stopTimer();
        triggers.get (juce::Identifier { "stop" }, pendingWidth, pendingHeight);
    }

    jam::Function::Map<juce::Identifier, void> triggers;
    int  pendingWidth     { 0 };
    int  pendingHeight    { 0 };
    bool isTransitioning  { false };
    static constexpr int tickIntervalMs { 16 };
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
