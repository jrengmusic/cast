/**
 * @file jam_Oversampling.h
 * @brief Dual-precision wrapper around juce::dsp::Oversampling (halfband FIR-equiripple / polyphase-IIR).
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/

/**
 * @class Oversampling
 * @brief Dual-precision wrapper around juce::dsp::Oversampling, switching between halfband FIR-equiripple and polyphase-IIR filter families.
 *
 * Maintains parallel float and double juce::dsp::Oversampling instances,
 * dispatching each templated call to whichever precision the sample type
 * selects.
 */
class Oversampling
{
public:
    /** @brief Selects the halfband filter family used for oversampling. */
    enum class FilterType
    {
        IIR,
        FIR
    };

    /**
     * @brief Constructs an Oversampling wrapper using the given filter family.
     * @param filter The filter family to use for both precisions.
     */
    Oversampling (FilterType filter = FilterType::IIR)
        : type (filter) {}

    /**
     * @brief Destructor for the Oversampling class.
     */
    ~Oversampling() {}

    //==============================================================================

    /**
     * @brief Reinitializes processing when the process spec has changed.
     * @param newSpec The process spec to compare against and adopt if changed.
     */
    void prepareToPlay (const juce::dsp::ProcessSpec& newSpec)
    {
        if (isSpecChanged (newSpec))
        {
            spec = newSpec;
            initProcessing (spec.maximumBlockSize);
        }
    }

    /**
     * @brief Resets both precisions' internal filter state.
     */
    void reset()
    {
        singlePrecision.reset();
        doublePrecision.reset();
    }

    /**
     * @brief Initializes both precisions' internal buffers for the given block size.
     * @param samplesPerBlock The maximum number of samples per processing block.
     */
    void initProcessing (size_t samplesPerBlock)
    {
        doublePrecision.initProcessing (samplesPerBlock);
        singlePrecision.initProcessing (samplesPerBlock);
    }

    /**
     * @brief Rebuilds both precisions' oversampling stages for a new oversampling factor.
     * @param newOversamplingFactor The new oversampling factor (as consumed by juce::dsp::Oversampling's constructor).
     */
    void setOversamplingFactor (int newOversamplingFactor) noexcept
    {
        switch (type)
        {
            case FilterType::FIR:
                doublePrecision = Double (spec.numChannels, newOversamplingFactor, Double::FilterType::filterHalfBandFIREquiripple, true, false);
                singlePrecision = Float (spec.numChannels, newOversamplingFactor, Float::FilterType::filterHalfBandFIREquiripple, true, false);
                break;

            case FilterType::IIR:
                doublePrecision = Double (spec.numChannels, newOversamplingFactor, Double::FilterType::filterHalfBandPolyphaseIIR, true, false);
                singlePrecision = Float (spec.numChannels, newOversamplingFactor, Float::FilterType::filterHalfBandPolyphaseIIR, true, false);
                break;
        }

        initProcessing (spec.maximumBlockSize);
    }

    //==============================================================================

    /**
     * @brief Checks whether the given process spec differs from the currently prepared spec.
     * @param newSpec The process spec to compare against the currently prepared spec.
     * @return True if sample rate, maximum block size, or channel count differ.
     */
    bool isSpecChanged (const juce::dsp::ProcessSpec& newSpec) noexcept
    {
        return (newSpec.sampleRate != spec.sampleRate
                or newSpec.maximumBlockSize != spec.maximumBlockSize
                or newSpec.numChannels != spec.numChannels);
    }

    //==============================================================================

    /**
     * @brief Retrieves the oversampler's latency for the given sample precision.
     * @tparam SampleType The sample type selecting which precision to query (float or double).
     * @return The latency in samples for that precision.
     */
    template <typename SampleType>
    SampleType getLatencyInSamples() const noexcept
    {
        if constexpr (std::is_same<double, SampleType>::value)
            return doublePrecision.getLatencyInSamples();
        else
            return singlePrecision.getLatencyInSamples();
    }

    /**
     * @brief Upsamples an audio block using the precision matching SampleType.
     * @tparam SampleType The sample type selecting which precision processes the block (float or double).
     * @param inputBlock The audio block at the base sample rate.
     * @return An AudioBlock view of the upsampled data.
     */
    template <typename SampleType>
    juce::dsp::AudioBlock<SampleType> processSamplesUp (const juce::dsp::AudioBlock<SampleType>& inputBlock)
    {
        if constexpr (std::is_same<double, SampleType>::value)
            return doublePrecision.processSamplesUp (inputBlock);
        else
            return singlePrecision.processSamplesUp (inputBlock);
    }

    /**
     * @brief Downsamples an audio block in place using the precision matching SampleType.
     * @tparam SampleType The sample type selecting which precision processes the block (float or double).
     * @param outputBlock The audio block containing upsampled data, decimated in place; no-op if it holds no samples.
     */
    template <typename SampleType>
    void processSamplesDown (juce::dsp::AudioBlock<SampleType>& outputBlock)
    {
        if (outputBlock.getNumSamples())
        {
            if constexpr (std::is_same<double, SampleType>::value)
                doublePrecision.processSamplesDown (outputBlock);
            else
                singlePrecision.processSamplesDown (outputBlock);
        }
    }

private:
    const FilterType type;
    juce::dsp::ProcessSpec spec { 44100.0, 1024, 2 };

    using Float = juce::dsp::Oversampling<float>;
    using Double = juce::dsp::Oversampling<double>;

    Float singlePrecision;
    Double doublePrecision;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Oversampling)
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::dsp
