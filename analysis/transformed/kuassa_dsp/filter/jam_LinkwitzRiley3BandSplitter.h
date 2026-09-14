/**
 * @file jam_LinkwitzRiley3BandSplitter.h
 * @brief Implementation of a Linkwitz-Riley 3-band crossover filter.
 *
 * This file provides a 3-band Linkwitz-Riley crossover implementation using cascaded
 * Butterworth filters to achieve perfect reconstruction when the bands are summed.
 */

/*____________________________________________________________________________*/
/*
    Corrected Linkwitz-Riley 3-Band Crossover Implementation

    ARCHITECTURE:
    =============
    Linkwitz-Riley crossovers use cascaded Butterworth filters to achieve:
    1. Flat magnitude response when bands are summed (perfect reconstruction)
    2. Zero phase difference at crossover points
    3. -6dB at crossover frequencies for each adjacent band pair

    For a 3-band LR splitter:

    LOW BAND:    LPF @ lowCrossover
    MID BAND:    HPF @ lowCrossover → LPF @ highCrossover (cascaded!)
    HIGH BAND:   HPF @ highCrossover

    Common orders:
    - LR2: 2nd order Butterworth (12 dB/octave)
    - LR4: 4th order Butterworth (24 dB/octave) [most common]
    - LR8: 8th order Butterworth (48 dB/octave)

    USAGE:
    ======
    LinkwitzRiley3BandSplitter<float> splitter(4); // LR4 (24 dB/octave)
    splitter.setSampleRate(48000.0);
    splitter.setLowCrossover(200.0);   // Low/Mid split at 200 Hz
    splitter.setHighCrossover(3000.0); // Mid/High split at 3 kHz

    // In processBlock:
    float low = sample, mid = sample, high = sample;
    splitter.processLow(low);
    splitter.processMid(mid);
    splitter.processHigh(high);
    // low + mid + high = original (perfect reconstruction)
*/
/*____________________________________________________________________________*/

namespace jam::dsp
{

/**
 * @brief A template class implementing a Linkwitz-Riley 3-band crossover filter.
 *
 * This class splits an audio signal into three frequency bands (low, mid, high) using
 * Linkwitz-Riley crossover filters. When recombined, the three bands perfectly
 * reconstruct the original signal with flat frequency response.
 *
 * @tparam SampleType The type used for audio samples (typically float or double)
 */
template <typename SampleType>
class LinkwitzRiley3BandSplitter
{
public:
    //==============================================================================
    /**
     * @brief Constructor for LinkwitzRiley3BandSplitter
     *
     * Creates a Linkwitz-Riley 3-band crossover with the specified filter order.
     *
     * @param filterOrder The Butterworth filter order (must be even: 2, 4, 8, etc.)
     *                    LR2 = 2nd order (12 dB/oct)
     *                    LR4 = 4th order (24 dB/oct) [recommended]
     *                    LR8 = 8th order (48 dB/oct)
     */
    LinkwitzRiley3BandSplitter (int filterOrder = 4)
        : order (filterOrder)
        , lowFilter (filterOrder)
        , midLowFilter (filterOrder)
        , midHighFilter (filterOrder)
        , highFilter (filterOrder)
    {
        jassert (filterOrder > 0 && filterOrder % 2 == 0);// Must be even order

        // Initialize with sensible defaults
        setSampleRate (44100.0);
        setLowCrossover (200.0);
        setHighCrossover (3000.0);
    }

    /**
     * @brief Destructor for LinkwitzRiley3BandSplitter
     */
    ~LinkwitzRiley3BandSplitter() = default;

    //==============================================================================
    /**
     * @brief Process the low frequency band
     *
     * Applies a low-pass filter to the input sample to extract the low frequency band.
     *
     * @param sample Reference to the audio sample to be processed
     */
    void processLow (SampleType& sample)
    {
        if (! bypassed)
            lowFilter.process (sample);
    }

    /**
     * @brief Process the mid frequency band
     *
     * Applies a high-pass filter followed by a low-pass filter to extract the mid frequency band.
     * The first filter removes frequencies below the low crossover, and the second removes
     * frequencies above the high crossover.
     *
     * @param sample Reference to the audio sample to be processed
     */
    void processMid (SampleType& sample)
    {
        if (! bypassed)
        {
            midLowFilter.process (sample);// HPF at low crossover
            midHighFilter.process (sample);// LPF at high crossover
        }
    }

    /**
     * @brief Process the high frequency band
     *
     * Applies a high-pass filter to the input sample to extract the high frequency band.
     *
     * @param sample Reference to the audio sample to be processed
     */
    void processHigh (SampleType& sample)
    {
        if (! bypassed)
            highFilter.process (sample);
    }

    //==============================================================================
    /**
     * @brief Reset all filter states
     *
     * Clears the internal state of all filter components. This should be called
     * when playback stops or when there's a discontinuity in the audio stream.
     */
    void reset()
    {
        lowFilter.reset();
        midLowFilter.reset();
        midHighFilter.reset();
        highFilter.reset();
    }

    /**
     * @brief Update the sample rate
     *
     * Sets the sample rate for all internal filters and recalculates their coefficients.
     * This must be called before processing begins.
     *
     * @param newSampleRate The new sample rate in Hz
     */
    void setSampleRate (double newSampleRate)
    {
        if (newSampleRate != sampleRate && newSampleRate > 0.0)
        {
            sampleRate = newSampleRate;
            lowFilter.setSampleRate (newSampleRate);
            midLowFilter.setSampleRate (newSampleRate);
            midHighFilter.setSampleRate (newSampleRate);
            highFilter.setSampleRate (newSampleRate);
            updateFilters();// Recalculate coefficients
        }
    }

    /**
     * @brief Set the low/mid crossover frequency
     *
     * Sets the frequency that separates the low and mid bands, in Hz.
     *
     * @param frequencyHz The crossover frequency in Hz (must be > 0 and < highCrossover)
     */
    void setLowCrossover (double frequencyHz)
    {
        if (frequencyHz != lowCrossover && frequencyHz > 0.0 && frequencyHz < highCrossover)
        {
            lowCrossover = frequencyHz;
            updateFilters();
        }
    }

    /**
     * @brief Set the mid/high crossover frequency
     *
     * Sets the frequency that separates the mid and high bands, in Hz.
     *
     * @param frequencyHz The crossover frequency in Hz (must be > lowCrossover and < Nyquist)
     */
    void setHighCrossover (double frequencyHz)
    {
        if (frequencyHz != highCrossover && frequencyHz > lowCrossover && frequencyHz < sampleRate * 0.5)
        {
            highCrossover = frequencyHz;
            updateFilters();
        }
    }

    /**
     * @brief Change the filter order
     *
     * Updates the filter order (which affects the rolloff steepness) and reinitializes
     * all internal filters with the new order.
     *
     * @param newOrder The new filter order (must be even: 2, 4, 8, etc.)
     */
    void setOrder (int newOrder)
    {
        jassert (newOrder > 0 && newOrder % 2 == 0);// Must be even

        if (newOrder != order && newOrder > 0 && newOrder % 2 == 0)
        {
            order = newOrder;

            // Recreate filters with new order
            lowFilter = Butterworth<SampleType> (order);
            midLowFilter = Butterworth<SampleType> (order);
            midHighFilter = Butterworth<SampleType> (order);
            highFilter = Butterworth<SampleType> (order);

            setSampleRate (sampleRate);// Reinitialize
            updateFilters();
        }
    }

    /**
     * @brief Bypass the crossover processing
     *
     * When bypassed, the crossover passes audio through unchanged.
     *
     * @param shouldBeBypassed True to bypass processing, false to process normally
     */
    void setBypassed (bool shouldBeBypassed)
    {
        bypassed = shouldBeBypassed;
    }

    //==============================================================================
    // Getters

    /**
     * @brief Get the current sample rate
     *
     * @return The current sample rate in Hz
     */
    double getSampleRate() const { return sampleRate; }

    /**
     * @brief Get the low/mid crossover frequency
     *
     * @return The low/mid crossover frequency in Hz
     */
    double getLowCrossover() const { return lowCrossover; }

    /**
     * @brief Get the mid/high crossover frequency
     *
     * @return The mid/high crossover frequency in Hz
     */
    double getHighCrossover() const { return highCrossover; }

    /**
     * @brief Get the current filter order
     *
     * @return The current filter order (e.g., 2 for LR2, 4 for LR4)
     */
    int getOrder() const { return order; }

    /**
     * @brief Check if the crossover is currently bypassed
     *
     * @return True if bypassed, false if processing normally
     */
    bool isBypassed() const { return bypassed; }


private:
    //==============================================================================
    /**
     * @brief Update all filter coefficients based on current crossover settings
     *
     * Recalculates the coefficients for all internal filters based on the current
     * crossover frequencies and filter type settings.
     */
    void updateFilters()
    {
        // Low band: Simple lowpass at low crossover
        lowFilter.setType (Butterworth<SampleType>::Type::lowPass);
        lowFilter.setFrequency (lowCrossover);

        // Mid band LOW side: Highpass at low crossover
        midLowFilter.setType (Butterworth<SampleType>::Type::highPass);
        midLowFilter.setFrequency (lowCrossover);

        // Mid band HIGH side: Lowpass at high crossover
        midHighFilter.setType (Butterworth<SampleType>::Type::lowPass);
        midHighFilter.setFrequency (highCrossover);

        // High band: Simple highpass at high crossover
        highFilter.setType (Butterworth<SampleType>::Type::highPass);
        highFilter.setFrequency (highCrossover);
    }

    //==============================================================================
    Butterworth<SampleType> lowFilter;    ///< Low-pass filter for the low band (LPF @ lowCrossover)
    Butterworth<SampleType> midLowFilter; ///< High-pass filter for the mid band part 1 (HPF @ lowCrossover)
    Butterworth<SampleType> midHighFilter;///< Low-pass filter for the mid band part 2 (LPF @ highCrossover)
    Butterworth<SampleType> highFilter;   ///< High-pass filter for the high band (HPF @ highCrossover)

    double sampleRate { 44100.0 };       ///< Current sample rate in Hz
    double lowCrossover { 200.0 };       ///< Low/mid crossover frequency in Hz
    double highCrossover { 3000.0 };     ///< Mid/high crossover frequency in Hz
    int order { 4 };                     ///< Filter order (default to LR4 = 24 dB/octave)
    bool bypassed { false };             ///< Bypass state flag

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LinkwitzRiley3BandSplitter)
};

}// namespace jam::dsp
