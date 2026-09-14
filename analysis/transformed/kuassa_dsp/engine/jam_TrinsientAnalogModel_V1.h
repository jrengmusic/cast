/**
 * @file jam_TrinsientAnalogModel_V1.h
 * @brief Three-band time-domain analog hardware emulation model (V1).
 *
 * Signal flow: Input -> Crossover -> [Input Gain] -> [Transient Control]
 * (per band) -> Sum.
 *
 * @see TDAE_v1.md — full methodology
 */

#pragma once

namespace jam::dsp::Engine
{

/**
 * @class TrinsientAnalogModel_V1
 * @brief Three-band analog-modeled transient shaper: crossover splits the signal, then per-band input gain and transient control are summed back together.
 */
class TrinsientAnalogModel_V1
{
public:
    //==============================================================================
    /** @brief Band indices for per-band parameter access. */
    enum Band
    {
        low = 0,
        mid = 1,
        high = 2
    };

    /** @brief Crossover point indices. */
    enum Crossover
    {
        lowMid = 0,
        midHigh = 1
    };

    //==============================================================================
    /**
     * @brief Default constructor for the TrinsientAnalogModel_V1 class.
     */
    TrinsientAnalogModel_V1();

    /**
     * @brief Destructor for the TrinsientAnalogModel_V1 class.
     */
    ~TrinsientAnalogModel_V1();

    //==============================================================================
    /**
     * @brief Resets the crossover filter state.
     */
    void reset();

    /**
     * @brief Sets the sample rate. Must be called before processing.
     * @param newSampleRate The new sample rate in Hz.
     */
    void setSampleRate (double newSampleRate);

    //==============================================================================
    // Crossover Control

    /**
     * @brief Sets a crossover frequency.
     * @param crossoverIndex The crossover point to set (lowMid or midHigh).
     * @param frequencyHz The crossover frequency in Hz.
     */
    void setCrossoverFrequency (Crossover crossoverIndex, double frequencyHz);

    /**
     * @brief Sets the crossover filter order.
     * @param order The filter order (2, 4, 8, etc.).
     */
    void setCrossoverOrder (int order);

    //==============================================================================
    // Per-Band Transient Control

    /**
     * @brief Sets the attack amount for a band.
     * @param band The band to configure.
     * @param value The attack amount (-100 to +100).
     */
    void setAttack (Band band, float value);

    /**
     * @brief Sets the attack strength (envelope follower speed) for a band.
     * @param band The band to configure.
     * @param value The attack strength (0.0 to 1.0).
     */
    void setAttackStrength (Band band, float value);

    /**
     * @brief Sets the decay/sustain amount for a band.
     * @param band The band to configure.
     * @param value The decay amount (-100 to +100).
     */
    void setDecay (Band band, float value);

    /**
     * @brief Sets the decay strength (release curve shape) for a band.
     * @param band The band to configure.
     * @param value The decay strength (0.0 to 1.0).
     */
    void setDecayStrength (Band band, float value);

    /**
     * @brief Sets the input gain (pre-transient processing drive) for a band.
     * @param band The band to configure.
     * @param gainDB The input gain in dB.
     */
    void setInputGain (Band band, float gainDB);

    /**
     * @brief Sets the output gain (post-transient processing makeup) for a band.
     * @param band The band to configure.
     * @param gainDB The output gain in dB.
     */
    void setOutputGain (Band band, float gainDB);

    /**
     * @brief Sets the gain compensation (frequency-dependent EQ trim) for a band.
     * @param band The band to configure.
     * @param gainDB The gain compensation in dB.
     */
    void setGainCompensation (Band band, float gainDB);

    /**
     * @brief Sets the envelope detection mode for a band.
     * @param band The band to configure.
     * @param mode The detection mode (0 = peak, 1 = squared).
     */
    void setDetectionMode (Band band, float mode);

    //==============================================================================
    // Bypass Control

    /**
     * @brief Bypasses the entire engine.
     * @param shouldBeBypassed True to bypass, false to process normally.
     */
    void setBypassed (bool shouldBeBypassed);

    /**
     * @brief Bypasses a specific band.
     * @param band The band to bypass or re-enable.
     * @param shouldBeBypassed True to bypass, false to process normally.
     */
    void setBandBypassed (Band band, bool shouldBeBypassed);

    /**
     * @brief Bypasses the crossover, processing the signal full-band.
     * @param shouldBeBypassed True to bypass, false to process normally.
     */
    void setCrossoverBypassed (bool shouldBeBypassed);

    //==============================================================================
    /**
     * @brief Processes a single sample in place through crossover, input gain, and per-band transient control.
     * @tparam SampleType The sample's numeric type.
     * @param sample The sample to process, replaced with the processed output.
     */
    template <typename SampleType>
    void process (SampleType& sample);

private:
    //==============================================================================
    void calc();

    //==============================================================================
    // State
    double sampleRate;
    bool bypassed;

    // Crossover state
    LinkwitzRiley3BandSplitter<float> crossover;
    int crossoverOrder;
    double crossoverFrequencies[2];// [lowMid, midHigh]

    // Per-band parameters (indexed by Band enum)
    float attack[3];
    float attackStrength[3];
    float decay[3];
    float decayStrength[3];
    float inputGain[3];
    float outputGain[3];
    float gainCompensation[3];
    int detectionMode[3];
    bool bandBypassed[3];

    // Computed values
    double inputGainLinear[3];

    // Transient processors (one per band)
    jam::Owner<TransientControl> transientControls;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrinsientAnalogModel_V1)
};

//==============================================================================
// Template implementation

template <typename SampleType>
void TrinsientAnalogModel_V1::process (SampleType& sample)
{
    if (bypassed)
        return;

    // Split into 3 bands
    SampleType lowBand = sample;
    SampleType midBand = sample;
    SampleType highBand = sample;

    // Apply crossover
    crossover.processLow (lowBand);
    crossover.processMid (midBand);
    crossover.processHigh (highBand);

    // Apply input gain per band
    lowBand *= static_cast<SampleType> (inputGainLinear[low]);
    midBand *= static_cast<SampleType> (inputGainLinear[mid]);
    highBand *= static_cast<SampleType> (inputGainLinear[high]);

    // Apply transient control per band
    if (! bandBypassed[low])
    {
        float lowFloat = static_cast<float> (lowBand);
        transientControls.at (low)->process (lowFloat);
        lowBand = static_cast<SampleType> (lowFloat);
    }

    if (! bandBypassed[mid])
    {
        float midFloat = static_cast<float> (midBand);
        transientControls.at (mid)->process (midFloat);
        midBand = static_cast<SampleType> (midFloat);
    }

    if (! bandBypassed[high])
    {
        float highFloat = static_cast<float> (highBand);
        transientControls.at (high)->process (highFloat);
        highBand = static_cast<SampleType> (highFloat);
    }

    // Sum bands back together
    sample = lowBand + midBand + highBand;
}

}// namespace jam::dsp::Engine
