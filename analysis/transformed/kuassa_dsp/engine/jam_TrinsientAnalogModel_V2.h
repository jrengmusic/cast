/**
 * @file jam_TrinsientAnalogModel_V2.h
 * @brief Three-band time-domain analog hardware emulation model with per-band
 *        hysteresis shaping (V2).
 *
 * Signal flow: Input -> Crossover -> [Input Gain] -> [Transient Control] ->
 * [Hysteresis] -> [Output Gain] (per band) -> Sum.
 *
 * @see TDAE_v2.md — full methodology
 */

#pragma once

namespace jam::dsp::Engine
{

/**
 * @class TrinsientAnalogModel_V2
 * @brief Three-band analog-modeled transient shaper with per-band hysteresis: crossover splits the signal, then per-band input gain, transient control, hysteresis shaping, and output gain are summed back together.
 */
class TrinsientAnalogModel_V2
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

    /** @brief Hysteresis model type selection. */
    enum HysteresisType
    {
        saturator = 0,    ///< Jiles-Atherton model (physically accurate)
        stateSpace = 1,   ///< State-space model (efficient)
        preisach = 2      ///< Preisach model (most accurate)
    };

    //==============================================================================
    /**
     * @brief Default constructor for the TrinsientAnalogModel_V2 class.
     */
    TrinsientAnalogModel_V2();

    /**
     * @brief Destructor for the TrinsientAnalogModel_V2 class.
     */
    ~TrinsientAnalogModel_V2();

    //==============================================================================
    /**
     * @brief Resets the hysteresis processors and the crossover filter state.
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
     * @brief Sets the output gain (post-hysteresis makeup) for a band.
     *
     * Applied after hysteresis shaping, not forwarded to `TransientControl`
     * (its own output stays at unity so hysteresis processes the
     * transient-shaped signal before this makeup gain).
     *
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
    // Per-Band Hysteresis Control (NEW in V2)

    /**
     * @brief Sets the hysteresis model type for a band.
     * @param band The band to configure.
     * @param type The hysteresis model to use.
     */
    void setHysteresisType (Band band, HysteresisType type);

    /**
     * @brief Sets the hysteresis drive (input gain into the nonlinearity) for a band.
     * @param band The band to configure.
     * @param value The hysteresis drive (0.0 to 1.0).
     */
    void setHysteresisDrive (Band band, float value);

    /**
     * @brief Sets the hysteresis saturation depth (wet/dry mix) for a band.
     * @param band The band to configure.
     * @param value The saturation amount (0.0 to 1.0).
     */
    void setHysteresisSaturation (Band band, float value);

    /**
     * @brief Sets the width of the hysteresis loop for a band.
     * @param band The band to configure.
     * @param value The hysteresis width (0.0 to 1.0).
     */
    void setHysteresisWidth (Band band, float value);

    /**
     * @brief Sets the hysteresis asymmetry (DC offset for even harmonics) for a band.
     * @param band The band to configure.
     * @param value The asymmetry amount (-1.0 to +1.0).
     */
    void setHysteresisAsymmetry (Band band, float value);

    /**
     * @brief Enables or disables hysteresis for a band.
     * @param band The band to configure.
     * @param enabled True to enable hysteresis, false to disable it.
     */
    void setHysteresisEnabled (Band band, bool enabled);

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

    /**
     * @brief Bypasses all hysteresis processing across every band.
     * @param shouldBeBypassed True to bypass, false to process normally.
     */
    void setHysteresisBypassed (bool shouldBeBypassed);

    //==============================================================================
    /**
     * @brief Processes a single sample in place through crossover, per-band input gain, transient control, hysteresis, and output gain.
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
    bool hysteresisBypassed;

    // Crossover state
    LinkwitzRiley3BandSplitter<float> crossover;
    int crossoverOrder;
    double crossoverFrequencies[2];  // [lowMid, midHigh]

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

    // Hysteresis parameters (NEW in V2)
    HysteresisType hysteresisType[3];
    float hysteresisDrive[3];
    float hysteresisSaturation[3];
    float hysteresisWidth[3];
    float hysteresisAsymmetry[3];
    bool hysteresisEnabled[3];

    // Computed values
    double inputGainLinear[3];
    double outputGainLinear[3];

    // Transient processors (one per band)
    jam::Owner<TransientControl> transientControls;

    // Hysteresis processors (NEW in V2 - one per band, 3 types each)
    jam::Owner<Hysteresis::Saturator> hysteresisSaturators;
    jam::Owner<Hysteresis::StateSpace> hysteresisStateSpace;
    jam::Owner<Hysteresis::Preisach> hysteresisPreisach;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrinsientAnalogModel_V2)
};

//==============================================================================
// Template implementation

template <typename SampleType>
void TrinsientAnalogModel_V2::process (SampleType& sample)
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

    // Process LOW band: Input Gain → TC → Hysteresis → Output Gain
    if (!bandBypassed[low])
    {
        // 1. Input gain
        lowBand *= static_cast<SampleType> (inputGainLinear[low]);

        // 2. Transient control
        float lowFloat = static_cast<float> (lowBand);
        transientControls.at (low)->process (lowFloat);
        lowBand = static_cast<SampleType> (lowFloat);

        // 3. Hysteresis (NEW in V2)
        if (!hysteresisBypassed && hysteresisEnabled[low])
        {
            switch (hysteresisType[low])
            {
                case saturator:
                    hysteresisSaturators.at (low)->process (lowBand);
                    break;
                case stateSpace:
                    hysteresisStateSpace.at (low)->process (lowBand);
                    break;
                case preisach:
                    hysteresisPreisach.at (low)->process (lowBand);
                    break;
            }
        }

        // 4. Output gain
        lowBand *= static_cast<SampleType> (outputGainLinear[low]);
    }

    // Process MID band: Input Gain → TC → Hysteresis → Output Gain
    if (!bandBypassed[mid])
    {
        // 1. Input gain
        midBand *= static_cast<SampleType> (inputGainLinear[mid]);

        // 2. Transient control
        float midFloat = static_cast<float> (midBand);
        transientControls.at (mid)->process (midFloat);
        midBand = static_cast<SampleType> (midFloat);

        // 3. Hysteresis (NEW in V2)
        if (!hysteresisBypassed && hysteresisEnabled[mid])
        {
            switch (hysteresisType[mid])
            {
                case saturator:
                    hysteresisSaturators.at (mid)->process (midBand);
                    break;
                case stateSpace:
                    hysteresisStateSpace.at (mid)->process (midBand);
                    break;
                case preisach:
                    hysteresisPreisach.at (mid)->process (midBand);
                    break;
            }
        }

        // 4. Output gain
        midBand *= static_cast<SampleType> (outputGainLinear[mid]);
    }

    // Process HIGH band: Input Gain → TC → Hysteresis → Output Gain
    if (!bandBypassed[high])
    {
        // 1. Input gain
        highBand *= static_cast<SampleType> (inputGainLinear[high]);

        // 2. Transient control
        float highFloat = static_cast<float> (highBand);
        transientControls.at (high)->process (highFloat);
        highBand = static_cast<SampleType> (highFloat);

        // 3. Hysteresis (NEW in V2)
        if (!hysteresisBypassed && hysteresisEnabled[high])
        {
            switch (hysteresisType[high])
            {
                case saturator:
                    hysteresisSaturators.at (high)->process (highBand);
                    break;
                case stateSpace:
                    hysteresisStateSpace.at (high)->process (highBand);
                    break;
                case preisach:
                    hysteresisPreisach.at (high)->process (highBand);
                    break;
            }
        }

        // 4. Output gain
        highBand *= static_cast<SampleType> (outputGainLinear[high]);
    }

    // Sum bands back together
    sample = lowBand + midBand + highBand;
}

} // namespace jam::dsp::Engine
