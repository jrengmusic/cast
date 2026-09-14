/**
 * @file jam_SmoothStateTransition.h
 * @brief A template class that adds smooth parameter transitions to any class.
 *
 * This file provides a generic implementation for adding smooth parameter transitions
 * to any class, using crossfading between old and new parameter states to prevent
 * audible artifacts during parameter changes.
 */

#pragma once

#include <functional>
#include <map>
#include <vector>

namespace jam::dsp
{

/**
 * @brief A template class that adds smooth parameter transitions to any class.
 *
 * This class is a generic version of SmoothCascade. It can wrap any class
 * and provide smooth transitions for any parameter, keyed by juce::Identifier.
 *
 * Setters for parameters that should be smoothed must be registered.
 *
 * The wrapped ObjectClass is expected to have the following methods:
 * - void process(SampleType& sample) OR
 * - void process(int channel, SampleType& sample)
 * - void process(SampleType& sampleL, SampleType& sampleR)
 * - void reset()
 * - void setSampleRate(double newSampleRate)
 *
 * CRITICAL: ObjectClass MUST be trivially copyable per DSP Architectural Contract.
 * This enables lock-free state snapshotting: previous = current (memcpy-fast).
 *
 * Usage:
 *   SmoothStateTransition<MyEffect> smoother;
 *   smoother.addTrigger("gain", [](MyEffect& e, double v) { e.setGain(v); });
 *   smoother.set("gain", 0.5); // This will be a smooth change
 *
 *   smoother.getCurrent().setOtherParameter(123); // This is an immediate change
 *
 * @tparam ObjectClass The type of the class to be wrapped (must be trivially copyable).
 */
template <typename ObjectClass>
class SmoothStateTransition
{
public:
    // Verify DSP contract: ObjectClass must be trivially copyable for lock-free snapshots
    static_assert (std::is_trivially_copyable_v<ObjectClass>,
                   "SmoothStateTransition requires trivially copyable ObjectClass per DSP contract");

    /**
     * @brief Reference block size for crossfade time calculation.
     * Using 512 samples (power of 2) as the baseline for consistent timing.
     * Crossfade duration = TRANSITION_BLOCKS × (512 / sampleRate)
     * At 44.1kHz: 18 × 11.6ms = 209ms (empirically validated)
     *
     * This ensures crossfade timing scales with actual buffer size while
     * remaining block-aligned for artifact-free transitions.
     */
    static constexpr int SMOOTH_POLE_TRANSITION_BLOCKS = 18;
    static constexpr int REFERENCE_BLOCK_SIZE = 512;

    /**
     * @brief Constructor for SmoothStateTransition.
     *
     * Initializes both current and previous objects with the provided arguments.
     *
     * @param args Arguments to pass to the constructor of the ObjectClass.
     */
    template <typename... Args>
    SmoothStateTransition (Args&&... args)
        : current (std::forward<Args> (args)...)
        , previous (std::forward<Args> (args)...)
        , target (std::forward<Args> (args)...)
    {
        updateCrossfadeIncrement();
    }

    /**
     * @brief Destructor for SmoothStateTransition.
     */
    ~SmoothStateTransition() = default;

    //==============================================================================
    /**
     * @brief Processes a single sample with automatic crossfading during transitions.
     *
     * During transitions, this method processes the sample through both the
     * current and previous filters and crossfades between them based on the
     * current crossfade position.
     *
     * @tparam SampleType The type of the sample (e.g., float, double)
     * @param sample The input and output audio sample.
     */
    template <typename SampleType>
    void process (SampleType& sample)
    {
        if (not isReady)
            isReady = true;

        if (isTransitioning)
        {
            SampleType sampleOutputCurrent = sample;
            SampleType sampleOutputPrevious = sample;

            current.process (sampleOutputCurrent);
            previous.process (sampleOutputPrevious);

            // Crossfade using jam::Value::map (no JUCE dependency)
            SampleType blendFactor = static_cast<SampleType> (crossfadePosition);
            sample = jam::Value::map (blendFactor, sampleOutputPrevious, sampleOutputCurrent);

            advanceCrossfade();
        }
        else
        {
            current.process (sample);
        }
    }

    /**
     * @brief Processes a single channel of a sample with automatic crossfading.
     *
     * During transitions, this method processes the sample through both the
     * current and previous filters and crossfades between them based on the
     * current crossfade position.
     *
     * @tparam SampleType The type of the sample (e.g., float, double)
     * @param channel The channel index to process
     * @param sample The input and output audio sample for the specified channel.
     */
    template <typename SampleType>
    void process (int channel, SampleType& sample)
    {
        if (not isReady)
            isReady = true;

        if (isTransitioning)
        {
            SampleType sampleOutputCurrent = sample;
            SampleType sampleOutputPrevious = sample;

            current.process (channel, sampleOutputCurrent);
            previous.process (channel, sampleOutputPrevious);

            SampleType blendFactor = static_cast<SampleType> (crossfadePosition);
            sample = jam::Value::map (blendFactor, sampleOutputPrevious, sampleOutputCurrent);

            advanceCrossfade();
        }
        else
        {
            current.process (channel, sample);
        }
    }

    /**
     * @brief Processes two channels of a sample with automatic crossfading.
     *
     * During transitions, this method processes both left and right samples through both the
     * current and previous filters and crossfades between them based on the
     * current crossfade position.
     *
     * @tparam SampleType The type of the sample (e.g., float, double)
     * @param leftSample The input and output audio sample for the left channel.
     * @param rightSample The input and output audio sample for the right channel.
     */
    template <typename SampleType>
    void process (SampleType& leftSample, SampleType& rightSample)
    {
        if (not isReady)
            isReady = true;

        if (isTransitioning)
        {
            SampleType leftOutputCurrent = leftSample;
            SampleType rightOutputCurrent = rightSample;
            SampleType leftOutputPrevious = leftSample;
            SampleType rightOutputPrevious = rightSample;

            current.process (leftOutputCurrent, rightOutputCurrent);
            previous.process (leftOutputPrevious, rightOutputPrevious);

            SampleType blendFactor = static_cast<SampleType> (crossfadePosition);
            leftSample = jam::Value::map (blendFactor, leftOutputPrevious, leftOutputCurrent);
            rightSample = jam::Value::map (blendFactor, rightOutputPrevious, rightOutputCurrent);

            advanceCrossfade();// Only ONCE per sample pair!
        }
        else
        {
            current.process (leftSample, rightSample);
        }
    }

    //==============================================================================
    /**
     * @brief Resets both objects and stops any active crossfade.
     *
     * This method calls reset on both internal objects and resets the
     * crossfade state to its initial condition.
     */
    void reset()
    {
        current.reset();
        previous.reset();
        target.reset();
        isTransitioning = false;
        crossfadePosition = 1.0;
        pendingChanges.clear();
    }

    /**
      * @brief Sets the sample rate for both objects.
      *
      * Updates the sample rate on both current and previous objects.
      * Recalculates the crossfade increment based on the new sample rate.
      *
      * @param newSampleRate The new sample rate in Hz.
      */
    void setSampleRate (double newSampleRate)
    {
        if (newSampleRate != sampleRate)
        {
            sampleRate = newSampleRate;
            current.setSampleRate (newSampleRate);
            previous.setSampleRate (newSampleRate);
            target.setSampleRate (newSampleRate);
            updateCrossfadeIncrement();
        }
    }

    /**
      * @brief Prepare transition parameters for given sample rate and block size.
      * Similar to juce::dsp::Processor::prepare() - call once at initialization
      * or when DAW changes buffer size / oversampling.
      * Resets isReady to false — parameter changes after prepare() configure current
      * directly without creating transitions until the first process() call.
      * @param newSampleRate Sample rate (accounts for oversampling if passed pre-computed)
      * @param blockSize Maximum block size (accounts for oversampling if passed pre-computed)
      */
    void prepare (double newSampleRate, size_t blockSize)
    {
        setSampleRate (newSampleRate);
        setCrossfadeTimeForBlockSize (blockSize);
        isReady = false;
    }

    /**
      * @brief Set crossfade time aligned to block boundaries.
      * Calculates duration based on reference block size, then rounds to actual block boundaries.
      * Called internally by prepare(). Can also be called directly if needed.
      * @param blockSize The effective block size (accounting for oversampling if applicable)
      */
    void setCrossfadeTimeForBlockSize (size_t blockSize)
    {
        if (sampleRate > 0.0 && blockSize > 0)
        {
            // Calculate target duration based on reference block size (constant time)
            double referenceBlockDurationMs = (static_cast<double> (REFERENCE_BLOCK_SIZE) / sampleRate) * 1000.0;
            double targetCrossfadeDurationMs = referenceBlockDurationMs * SMOOTH_POLE_TRANSITION_BLOCKS;

            // Calculate actual block duration
            double actualBlockDurationMs = (static_cast<double> (blockSize) / sampleRate) * 1000.0;

            // Calculate blocks needed to reach target duration (minimum 1 block, block-aligned)
            int blocksNeeded = static_cast<int> (std::ceil (targetCrossfadeDurationMs / actualBlockDurationMs));
            if (blocksNeeded < 1) blocksNeeded = 1;

            // Apply block-aligned crossfade duration
            double crossfadeDurationMs = actualBlockDurationMs * blocksNeeded;
            setCrossfadeTimeMs (crossfadeDurationMs);
        }
    }

    //==============================================================================
    /**
     * @brief Adds a trigger function for a parameter.
     *
     * This method allows you to add a trigger function that will be executed
     * when the parameter value is set during smooth transitions.
     *
     * @tparam ValueType The type of the value to set (auto-deduced).
     * @param name The name of the parameter to trigger.
     * @param setter The function that will be used to set the parameter value.
     */
    template <typename ValueType, typename FunctionType>
    void addTrigger (const juce::Identifier& name, FunctionType&& setter)
    {
        triggers.add<ObjectClass&, ValueType&> (name, std::forward<FunctionType> (setter));
    }

    /**
      * @brief Sets a parameter value smoothly.
      * If transitioning, replaces pending change (don't queue all).
      * Keeps only the latest value - no lag, no zippering.
      *
      * @tparam ValueType The type of the value to set (auto-deduced).
      * @param name The name of the parameter to set.
      * @param value The new value for the parameter.
      */
    template <typename ValueType>
    void set (const juce::Identifier& name, ValueType value)
    {
        if (triggers.contains (name))
        {
            triggers.get (name, target, value);

            if (isTransitioning)
            {
                // Replace last pending change, don't queue all
                pendingChanges.clear();
                pendingChanges.add<> ([this, name, value]()
                {
                    applyChange (name, value);
                });
            }
            else
            {
                applyChange (name, value);
            }
        }
    }

    /**
      * @brief Applies all pending changes immediately without transitions.
      * Stops any active transition and flushes the pending changes queue.
      * Used during initialization (prepare/preset load) when audio is suspended.
      */
    void flush()
    {
        for (int i = 0; i < static_cast<int> (pendingChanges.size()); ++i)
            pendingChanges.get (i);  // Executes lambda which calls applyChange

        // Force transition state off (applyChange sets it on)
        isTransitioning = false;
        crossfadePosition = 1.0;
        pendingChanges.clear();
        target = current;
    }

    //==============================================================================
    /**
     * @brief Provides access to the current object for immediate (non-smoothed) operations.
     *
     * @return A reference to the current wrapped object.
     */
    ObjectClass& getCurrent() { return current; }
    const ObjectClass& getCurrent() const { return current; }

    /**
     * @brief Provides access to the target object — the state that all pending and
     *        in-flight parameter changes will ultimately produce.
     *
     * This is the SSOT for "what the user requested." It is updated immediately on
     * every set() call, regardless of whether a crossfade transition is active.
     * Use this in UI getter paths (magnitude curves, display state) to reflect the
     * latest requested parameter values without waiting for the crossfade to complete.
     *
     * Do NOT use this in the audio processing path — the audio thread uses getCurrent()
     * and getPrevious() exclusively.
     *
     * @return A reference to the target wrapped object.
     */
    ObjectClass& getTarget() noexcept { return target; }
    const ObjectClass& getTarget() const noexcept { return target; }

    //==============================================================================
    /**
     * @brief Sets the crossfade duration in milliseconds.
     *
     * @param timeMs The new crossfade duration in milliseconds.
     */
    void setCrossfadeTimeMs (double timeMs)
    {
        crossfadeTimeMs = timeMs;
        updateCrossfadeIncrement();
    }

    /**
     * @brief Checks if a transition is currently active.
     *
     * @return True if a transition is active, false otherwise.
     */
    bool isInTransition() const { return isTransitioning; }

    /**
     * @brief Gets the current crossfade duration in milliseconds.
     *
     * @return The current crossfade duration in milliseconds.
     */
    double getCrossfadeTimeMs() const { return crossfadeTimeMs; }

private:
    ObjectClass current;                           ///< The current object instance (audio processing SSOT)
    ObjectClass previous;                          ///< The previous object instance (for crossfading)
    ObjectClass target;                            ///< The target object instance — latest requested state (UI SSOT)

    jam::Function::Map<juce::String, void> triggers; ///< Map of registered setter functions that will trigger crossfade
    jam::Function::Array<void> pendingChanges;      ///< Queue of pending parameter changes (zero-arg lambdas)

    double sampleRate { 44100.0 };                ///< The current sample rate in Hz
    double crossfadeTimeMs { 17.0 };              ///< Crossfade duration in milliseconds

    bool isReady { false };                          ///< Cold until first process() — suppresses transitions during init
    bool isTransitioning { false };               ///< Flag indicating if a crossfade is active
    double crossfadePosition { 1.0 };             ///< Current position in the crossfade (0.0-1.0)
    double crossfadeIncrement { 0.0 };            ///< Increment value for crossfade position

    /**
     * @brief Applies a parameter change, creating a transition only when ready.
     *
     * This method saves the current state to the previous object and applies
     * the new value to the current object. When isReady is true (after the first
     * process() call), a crossfade transition is started. When isReady is false
     * (cold — before the first process() call), current is configured directly
     * without creating a transition, preventing audible artifacts at init.
     *
     * @tparam ValueType The type of the value to set (auto-deduced).
     * @param name The name of the parameter to change.
     * @param value The new value for the parameter.
     */
    template <typename ValueType>
    void applyChange (const juce::Identifier& name, ValueType value)
    {
        previous = current;
        triggers.get (name, current, value);

        if (isReady)
        {
            crossfadePosition = 0.0;
            isTransitioning = true;
        }
    }

    /**
     * @brief Advances the crossfade position and handles completion.
     *
     * This method increments the crossfade position and checks if the
     * transition is complete. If the transition is complete and there are
     * pending changes, it applies the next change.
     */
    void advanceCrossfade()
    {
        crossfadePosition += crossfadeIncrement;

        if (crossfadePosition >= 1.0)
        {
            crossfadePosition = 1.0;
            isTransitioning = false;

            if (pendingChanges.size() > 0)
            {
                // Execute first pending change, then clear all pending
                // (new implementation: only keep latest change, not queue all)
                pendingChanges.get (0);
                pendingChanges.clear();
            }
        }
    }

    /**
     * @brief Updates the crossfade increment based on the sample rate and crossfade time.
     *
     * Calculates the amount to increment the crossfade position each sample to
     * achieve the desired crossfade duration.
     */
    void updateCrossfadeIncrement()
    {
        if (sampleRate > 0.0 && crossfadeTimeMs > 0.0)
        {
            double crossfadeSamples = (crossfadeTimeMs / 1000.0) * sampleRate;
            crossfadeIncrement = 1.0 / crossfadeSamples;
        }
    }

    //==============================================================================
};

}// namespace jam::dsp
