/**
 * @file jam_SmoothChain.h
 * @brief Chain-level fade-out/callback/fade-in sequencing for discrete state changes.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/

/**
 * @class SmoothChain
 * @brief Chain-level fade for discrete state changes (bypass, oversampling).
 *
 * Fades chain output to silence, executes callback, fades back in.
 * Uses same timing as SmoothStateTransition (12 blocks).
 *
 * Unlike SmoothStateTransition (which crossfades two parallel states),
 * ChainFade uses simple fade-out → callback → fade-in sequence.
 * This avoids artifacts from crossfading nonlinear processors.
 *
 * Usage:
 *   SmoothChain fade;
 *   fade.addTrigger("bypass", [this]() { isBypassed = !isBypassed; });
 *   fade.trigger("bypass");  // Start fade sequence
 *
 *   // In process():
 *   fade.process(sampleL, sampleR);
 */
class SmoothChain
{
public:
    /**
     * @brief Reference block size for fade time calculation.
     * Using 512 samples (power of 2) as the baseline for consistent timing.
     * Fade duration = TRANSITION_BLOCKS × (512 / sampleRate)
     * At 44.1kHz: 18 × 11.6ms = 209ms (empirically validated)
     * Same as SmoothStateTransition for consistency.
     */
    static constexpr int TRANSITION_BLOCKS = 18;
    static constexpr int REFERENCE_BLOCK_SIZE = 512;

    /** @brief Default constructor for the SmoothChain class. */
    SmoothChain() = default;

    /** @brief Destructor for the SmoothChain class. */
    ~SmoothChain() = default;

    /**
     * @brief Prepare fade timing for given sample rate and block size.
     * Uses reference block size for constant fade duration regardless of buffer size.
     * Call once during initialization or when buffer size changes.
     */
    void prepare (double sampleRate, size_t blockSize)
    {
        if (sampleRate > 0.0 && blockSize > 0)
        {
            // Calculate target duration based on reference block size (constant time)
            double referenceBlockDurationSec = static_cast<double> (REFERENCE_BLOCK_SIZE) / sampleRate;
            double targetFadeTimeSec = referenceBlockDurationSec * TRANSITION_BLOCKS;

            // Calculate actual block duration
            double actualBlockDurationSec = static_cast<double> (blockSize) / sampleRate;

            // Calculate blocks needed to reach target duration (minimum 1 block, block-aligned)
            int blocksNeeded = static_cast<int> (std::ceil (targetFadeTimeSec / actualBlockDurationSec));
            if (blocksNeeded < 1) blocksNeeded = 1;

            // Apply block-aligned fade duration
            double fadeTimeSec = actualBlockDurationSec * blocksNeeded;
            gain.reset (sampleRate, fadeTimeSec);
            gain.setCurrentAndTargetValue (1.0);
        }
    }

    /**
     * @brief Register a trigger callback.
     * Callback executes when fade reaches silence (midpoint).
     * @param name Unique identifier for this trigger
     * @param callback Function to execute at silence midpoint
     */
    void addTrigger (const juce::Identifier& name, std::function<void()> callback)
    {
        triggers[name] = std::move (callback);
    }

    /**
     * @brief Start fade-out → callback → fade-in sequence.
     * Ignored if already transitioning (prevents overlapping fades).
     * @param name Identifier of registered trigger to execute
     */
    void trigger (const juce::Identifier& name)
    {
        if (triggers.count (name) > 0 && state == FadeState::Idle)
        {
            pendingTrigger = name;
            state = FadeState::FadingOut;
            gain.setTargetValue (0.0);
        }
    }

    /**
     * @brief Process stereo samples with fade.
     * Call in process() method, after all DSP processing.
     * @param left Left channel sample (modified in-place)
     * @param right Right channel sample (modified in-place)
     */
    template <typename SampleType>
    void process (SampleType& left, SampleType& right)
    {
        auto g = static_cast<SampleType> (gain.getNextValue());
        left *= g;
        right *= g;
        checkFadeState();
    }

    /**
     * @brief Process mono sample with fade.
     * @param channel Channel index (unused, for API compatibility)
     * @param sample Sample to process (modified in-place)
     */
    template <typename SampleType>
    void process (int channel, SampleType& sample)
    {
        juce::ignoreUnused (channel);
        sample *= static_cast<SampleType> (gain.getNextValue());
        checkFadeState();
    }

    /**
     * @brief Check if currently transitioning.
     * @return True if fade is active, false otherwise
     */
    bool isTransitioning() const
    {
        return state != FadeState::Idle;
    }

    /**
     * @brief Force immediate execution of pending trigger callback.
     * Used during initialization when audio thread hasn't started yet.
     * Safe to call anytime, no-op if no pending trigger or already executing.
     */
    void flush()
    {
        // Only flush if we have a pending trigger AND not already executing
        // This prevents recursive callback execution
        if (state == FadeState::FadingOut && pendingTrigger.isValid() && triggers.count (pendingTrigger) > 0)
        {
            triggers.at (pendingTrigger)();
            pendingTrigger = juce::Identifier::null;
            state = FadeState::Idle;
            gain.setCurrentAndTargetValue (1.0);
        }
    }

private:
    // State machine for fade sequence
    enum class FadeState
    {
        Idle,               // No transition active
        FadingOut,          // Fading to silence
        ExecutingCallback,  // At silence, executing callback
        FadingIn            // Fading back to unity gain
    };

    // AUDIO THREAD - Called per-sample from process()
    // Advances state machine: FadeOut → Execute → FadeIn → Idle
    void checkFadeState()
    {
        if (state == FadeState::FadingOut && not gain.isSmoothing() && gain.getCurrentValue() < 0.001)
        {
            // Reached silence - execute callback
            state = FadeState::ExecutingCallback;
            triggers.at (pendingTrigger)();
            pendingTrigger = juce::Identifier::null;

            // Start fade-in
            state = FadeState::FadingIn;
            gain.setTargetValue (1.0);
        }
        else if (state == FadeState::FadingIn && not gain.isSmoothing())
        {
            // Fade-in complete
            state = FadeState::Idle;
        }
    }

    FadeState state { FadeState::Idle };
    juce::SmoothedValue<double> gain { 1.0 };
    juce::Identifier pendingTrigger;
    jam::HashMap<juce::Identifier, std::function<void()>> triggers;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmoothChain)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::dsp
