/**
 * @file jam_CaretComponent.h
 * @brief Terminal-aware caret that renders DECSCUSR cursor shapes.
 *
 * Purely geometric — no glyph rasterization, no `jam::Font`/`jam::Typeface`
 * dependency. Positioned entirely via `juce::Component::setBounds()`; the
 * owning `jam::CodeView` performs cell-to-pixel conversion and calls
 * `setBounds()` directly.
 *
 * Blink is driven by an internal Timer that flips visibility and calls
 * `repaint()` on self only. Visibility is additionally gated by keyboard
 * focus, pushed in by the owning CodeView via `setFocused()` — CaretComponent
 * never queries focus itself (no stored owner pointer, no reaching out).
 *
 * Blink is configurable via `setBlink()`/`setBlinkInterval()` (theme.lua
 * cursor.blink/cursor.blink_interval, plumbed by
 * `jam::CodeView::setCaretBlink()`/`setCaretBlinkInterval()`).
 * `setBlink (false)` stops the timer and forces
 * the caret solid, rather than possibly freezing mid-blink on the off phase.
 */
#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/**
 * @brief DECSCUSR cursor shape vocabulary.
 *
 * `block` = DECSCUSR 0/1/2, `underline` = DECSCUSR 3/4, `bar` = DECSCUSR 5/6.
 */
enum class CaretShape
{
    block,
    underline,
    bar
};

/**
 * @brief Terminal caret — block/underline/bar geometry, blink, focus-gated.
 *
 * Colour resolves via `juce::CaretComponent::caretColourId` (0x1000204) —
 * the standard JUCE caret colour key, shared with any other JUCE
 * text-editing component in the same LookAndFeel scope.
 */
class CaretComponent
    : public juce::Component
    , private juce::Timer
{
public:
    /** @brief Constructs the caret. Does not intercept mouse clicks. Blink
     *  timer policy is centralized in updateTimer() — not started here, since
     *  the default state (unfocused) never runs it (see updateTimer()). */
    CaretComponent() noexcept
    {
        setInterceptsMouseClicks (false, false);
    }

    /** @brief Sets the DECSCUSR cursor shape. Repaints if changed. */
    void setShape (CaretShape newShape) noexcept
    {
        if (shape != newShape)
        {
            shape = newShape;
            repaint();
        }
    }

    /** @brief Sets whether the owning CodeView currently holds keyboard focus.
     *  Gates blink visibility — the caret never draws while unfocused. Applies
     *  the updated timer policy via updateTimer(). */
    void setFocused (bool focused) noexcept
    {
        hasFocus = focused;
        updateTimer();
        repaint();
    }

    /** @brief Enables or disables blink. Applies the updated timer policy via
     *  updateTimer() — disabling forces the caret solid (`shouldDraw = true`)
     *  so it never freezes on the off phase; enabling restarts the timer (iff
     *  focused) at the current blinkIntervalMs. Repaints if changed. */
    void setBlink (bool enabled) noexcept
    {
        if (blinkEnabled != enabled)
        {
            blinkEnabled = enabled;
            updateTimer();
            repaint();
        }
    }

    /** @brief Sets the blink interval in milliseconds (full cycle = 2x this
     *  value). Applies the updated timer policy via updateTimer() — restarts
     *  the timer at the new interval when blink is currently running. */
    void setBlinkInterval (int newIntervalMs) noexcept
    {
        if (blinkIntervalMs != newIntervalMs)
        {
            blinkIntervalMs = newIntervalMs;
            updateTimer();
        }
    }

    /** @internal */
    void paint (juce::Graphics& g) override
    {
        if (shouldDraw and hasFocus)
        {
            g.setColour (findColour (juce::CaretComponent::caretColourId, true));

            auto area { getLocalBounds() };

            if (shape == CaretShape::underline)
                g.fillRect (area.removeFromBottom (2));
            else if (shape == CaretShape::bar)
                g.fillRect (area.removeFromLeft (2));
            else
                g.fillRect (area);
        }
    }

private:
    void timerCallback() override
    {
        shouldDraw = not shouldDraw;
        repaint();
    }

    /** @brief Centralizes blink-timer policy — the single decision point for
     *  whether the timer runs. Running iff focused and blink is enabled;
     *  otherwise the timer is stopped and the caret is forced solid
     *  (`shouldDraw = true`) so it never freezes on the off phase. Called
     *  from setFocused(), setBlink(), setBlinkInterval(). */
    void updateTimer() noexcept
    {
        if (hasFocus and blinkEnabled)
        {
            startTimer (blinkIntervalMs);
        }
        else
        {
            stopTimer();
            shouldDraw = true;
        }
    }

    /** @brief Blink half-cycle duration in milliseconds (full cycle = 2x this
     *  value) — default matches the prior hardcoded constant; overridden by
     *  setBlinkInterval() from theme.lua cursor.blink_interval. */
    int blinkIntervalMs { 530 };

    /** @brief Whether the timer is currently running — set by setBlink(). */
    bool blinkEnabled { true };

    CaretShape shape      { CaretShape::block };
    bool       hasFocus   { false };
    bool       shouldDraw { true };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CaretComponent)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
