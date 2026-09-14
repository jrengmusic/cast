/**
 * @file jam_ImageTransition.h
 * @brief Timer-driven fade-out overlay for smoothing snapshot-to-live-content transitions.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct ImageTransition
 * @brief Fades out a snapshot image over the current component, smoothing visual transitions.
 * Caller paints current content first, then calls paint() to overlay the fading snapshot.
 */
struct ImageTransition : private juce::Timer
{
    /** @param newOnRepaint  Callback invoked each timer tick to trigger a repaint. */
    explicit ImageTransition (std::function<void()> newOnRepaint)
        : onRepaint (std::move (newOnRepaint))
    {
    }

    /** Starts the fade using the default 150 ms duration.
        @param snapshot  Image captured before the content change.
    */
    void start (juce::Image snapshot) { start (std::move (snapshot), defaultDurationMs); }

    /** Starts the fade with a custom duration.
        @param snapshot    Image captured before the content change.
        @param durationMs  Fade duration in milliseconds.
    */
    void start (juce::Image snapshot, int durationMs)
    {
        image = std::move (snapshot);
        alpha = 1.0f;
        fadeStep = 1000.0f / (static_cast<float> (frameRate) * static_cast<float> (durationMs));
        startTimerHz (frameRate);
    }

    /** Draws the fading snapshot if the transition is active.
        @param g          Graphics context supplied by the component's paint method.
        @param bounds     Area to draw the image into.
        @param placement  How the snapshot is fitted into @p bounds (default: stretch to fit).
    */
    void paint (juce::Graphics& g,
                juce::Rectangle<int> bounds,
                juce::RectanglePlacement placement = juce::RectanglePlacement::stretchToFit)
    {
        if (isActive())
        {
            g.setOpacity (alpha);
            g.drawImage (image, bounds.toFloat(), placement);
        }
    }

    /** Returns true while the fade is in progress. */
    bool isActive() const noexcept { return alpha > 0.0f; }

    ImageTransition() = default;
    ~ImageTransition() = default;

private:
    static constexpr int defaultDurationMs { 300 };
    static constexpr int frameRate { 60 };

    void timerCallback() override
    {
        alpha -= fadeStep;

        if (alpha <= 0.0f)
        {
            alpha = 0.0f;
            image = {};
            stopTimer();
        }

        onRepaint();
    }

    std::function<void()> onRepaint;
    juce::Image image;
    float alpha { 0.0f };
    float fadeStep { 0.0f };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageTransition)
};

/**____________________________________END OF NAMESPACE____________________________________*/
} /** namespace jam */
