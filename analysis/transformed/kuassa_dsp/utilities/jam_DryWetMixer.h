/**
 * @file jam_DryWetMixer.h
 * @brief Dual-precision (float/double) dry/wet mixer wrapper.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/

/**
 * @class DryWetMixer
 * @brief Dual-precision dry/wet mixer wrapping parallel float and double juce::dsp::DryWetMixer instances.
 *
 * Maintains separate single- and double-precision juce::dsp::DryWetMixer
 * instances behind a single API, dispatching each call to whichever
 * precision the overload's sample type selects.
 */
class DryWetMixer
{
public:
    /**
     * @brief Default constructor for the DryWetMixer class.
     */
    DryWetMixer() = default;

    /**
     * @brief Destructor for the DryWetMixer class.
     */
    ~DryWetMixer() = default;

    //==============================================================================

    /**
     * @brief Sets the dry/wet mixing rule for both precisions.
     * @param newRule The mixing rule to apply.
     */
    void setMixingRule (juce::dsp::DryWetMixingRule newRule)
    {
        singlePrecision.setMixingRule (newRule);
        doublePrecision.setMixingRule (newRule);
    }

    /**
     * @brief Sets the wet mix proportion for both precisions.
     * @param newValue The wet proportion, typically in [0, 1].
     */
    void setWetMixProportion (float newValue)
    {
        if (newValue != wetMixProportion)
        {
            wetMixProportion = newValue;
            singlePrecision.setWetMixProportion (wetMixProportion);
            doublePrecision.setWetMixProportion (wetMixProportion);
        }
    }

    /**
     * @brief Sets the wet-path latency compensation for both precisions.
     * @param wetLatencyInSamples The wet path's latency in samples.
     */
    void setWetLatency (int wetLatencyInSamples)
    {
        singlePrecision.setWetLatency (static_cast<float> (wetLatencyInSamples));
        doublePrecision.setWetLatency (static_cast<double> (wetLatencyInSamples));
    }

    /**
     * @brief Resets and prepares both precisions with the given process spec.
     * @param spec The process spec supplying sample rate, block size, and channel count.
     */
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        singlePrecision.reset();
        doublePrecision.reset();
        singlePrecision.prepare (spec);
        doublePrecision.prepare (spec);
    }

    /**
     * @brief Pushes float-precision dry samples for later mixing.
     * @param drySamples The dry audio block to store.
     */
    void pushDrySamples (const juce::dsp::AudioBlock<const float> drySamples)
    {
        singlePrecision.pushDrySamples (drySamples);
    }

    /**
     * @brief Pushes double-precision dry samples for later mixing.
     * @param drySamples The dry audio block to store.
     */
    void pushDrySamples (const juce::dsp::AudioBlock<const double> drySamples)
    {
        doublePrecision.pushDrySamples (drySamples);
    }

    /**
     * @brief Mixes previously pushed float-precision dry samples into wetSamples in place.
     * @param wetSamples The wet audio block to mix the dry signal into.
     */
    void mixWetSamples (juce::dsp::AudioBlock<float> wetSamples)
    {
        singlePrecision.mixWetSamples (wetSamples);
    }

    /**
     * @brief Mixes previously pushed double-precision dry samples into wetSamples in place.
     * @param wetSamples The wet audio block to mix the dry signal into.
     */
    void mixWetSamples (juce::dsp::AudioBlock<double> wetSamples)
    {
        doublePrecision.mixWetSamples (wetSamples);
    }

private:
    float wetMixProportion { 1.0f };
    juce::dsp::DryWetMixer<float> singlePrecision { 128 };
    juce::dsp::DryWetMixer<double> doublePrecision { 128 };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DryWetMixer)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::dsp
