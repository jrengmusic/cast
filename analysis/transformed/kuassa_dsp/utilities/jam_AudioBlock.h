/**
 * @file jam_AudioBlock.h
 * @brief Gain/dither free functions and AudioBlock fade/dual-single processing helpers.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/

//==============================================================================

/**
 * @brief Converts a decibel gain value to a linear amplitude ratio.
 * @tparam FloatType The numeric type of the gain value.
 * @param gainInDecibel The gain in decibels.
 * @return The equivalent linear amplitude ratio.
 */
template <typename FloatType>
FloatType amp (FloatType gainInDecibel)
{
    return std::pow (10, gainInDecibel * 0.05);
}

//==============================================================================

/**
 * @brief Converts a decibel gain value to a linear power-domain ratio (half-scale exponent).
 * @tparam FloatType The numeric type of the gain value.
 * @param gainInDecibel The gain in decibels.
 * @return The equivalent linear power-domain ratio.
 */
template <typename FloatType>
FloatType amp2 (FloatType gainInDecibel)
{
    return std::pow (10, gainInDecibel * 0.025);
}

//==============================================================================
/**
 * @brief Converts a linear amplitude ratio to decibels.
 * @tparam FloatType The numeric type of the amplitude value.
 * @param amp The linear amplitude ratio.
 * @return The equivalent gain in decibels.
 */
template <typename FloatType>
static FloatType dB (FloatType amp)
{
    return static_cast<FloatType> (20) * std::log10 (amp);
}

//==============================================================================

/**
 * @brief Applies 32-bit floating-point dither to a double-precision sample on its way down to float.
 * @param input The sample to dither in place.
 * @param fpd The PRNG state (XORshift), advanced in place.
 * @return The dithered sample value.
 */
inline static float ditherToFloat (double& input, uint32_t& fpd)
{
    //begin 32 bit floating point dither
    int expon;
    frexpf ((float) input, &expon);
    fpd ^= fpd << 13;
    fpd ^= fpd >> 17;
    fpd ^= fpd << 5;
    input += ((double (fpd) - uint32_t (0x7fffffff)) * 5.5e-36l * std::pow (2, expon + 62));
    //end 32 bit floating point dither
    return input;
}

/**
 * @brief Applies double-precision dither to a sample, flooring near-silent values to the PRNG noise floor.
 * @param input The sample to dither in place.
 * @param fpd The PRNG state (XORshift), advanced in place.
 * @return The dithered sample value.
 */
inline static double ditherToDouble (double& input, uint32_t& fpd)
{
    if (std::abs (input) < 1.18e-23)
        return input = fpd * 1.18e-17;

    fpd ^= fpd << 13;
    fpd ^= fpd >> 17;
    fpd ^= fpd << 5;

    return input;
}

/**
 * @brief Seeds a dither PRNG state with a non-zero random value.
 * @param fpd The PRNG state to seed, set in place.
 */
inline static void getDitherSeed (uint32_t& fpd)
{
    while (fpd < 16386)
        fpd = static_cast<uint32_t>(std::rand()) * UINT32_MAX;
}

//==============================================================================
/**
 * @brief Creates a new AudioBuffer sized to match a block and allocates its storage.
 * @tparam SampleType The sample type of the block (float or double).
 * @param block The audio block whose channel and sample counts size the new buffer.
 * @return A newly allocated AudioBuffer with matching channel and sample counts.
 */
template <typename SampleType>
inline static juce::AudioBuffer<SampleType> createBufferCopyFrom (juce::dsp::AudioBlock<SampleType>& block)
{
    juce::dsp::ProcessContextReplacing<SampleType> context (block);

    const int numChannels { jam::toInt (context.getOutputBlock().getNumChannels()) };
    const int numSamples { jam::toInt (context.getOutputBlock().getNumSamples()) };

    /** Allocate storage */
    juce::AudioBuffer<SampleType> buffer (numChannels, numSamples);

    //        juce::AudioBuffer<SampleType> buffer { K::toInt (block.getNumChannels()), K::toInt (block.getNumSamples()) };// Allocate storage
    //        juce::dsp::AudioBlock<SampleType> copy { buffer };
    //        copy.copyFrom (buffer);// Copy the data

    return buffer;
}

//template <typename SampleType>
//inline static void addDrySampleToBlock (juce::dsp::AudioBlock<SampleType>& block, int channel, bool shouldFlipPolarity = false)
//{
//    juce::dsp::ProcessContextReplacing<SampleType> context (block);
//
//    for (auto spl = 0; spl < block.getNumSamples(); ++spl)
//    {
//        /** cast to double if SampleType is float */
//        double dry { static_cast<double> (context.getInputBlock().getSample (channel, spl)) };
//        double wet { static_cast<double> (block.getSample (channel, spl)) };
//
//        double mix { wet + (shouldFlipPolarity ? -1 : 1) * dry };
//
//        block.setSample (channel, spl, mix);
//    }
//}

/**
 * @brief Applies a fade-in ramp (0 to 1) to a single channel of a block.
 * @tparam SampleType The sample type of the block (float or double).
 * @tparam Type The numeric type of the smoothed value.
 * @param block The audio block to fade in place.
 * @param channel The channel index to fade.
 * @param fadeIn The smoothed value driving the fade ramp; reset and retargeted to 1.0 by this call.
 */
template <typename SampleType, typename Type>
inline static void addFadeInToBlock (juce::dsp::AudioBlock<SampleType>& block,
                                     int channel,
                                     juce::SmoothedValue<Type>& fadeIn)
{
    fadeIn.reset (jam::toInt (block.getNumSamples()));
    fadeIn.setTargetValue (1.0);

    for (auto input = 0; input < block.getNumSamples(); ++input)
    {
        /** cast to double if SampleType is float */
        double output { static_cast<double> (block.getSample (channel, input)) * fadeIn.getNextValue() };

        block.setSample (channel, input, output);
    }
}

/**
 * @brief Applies a fade-in ramp (0 to 1) to every channel of a block.
 * @tparam SampleType The sample type of the block (float or double).
 * @tparam Type The numeric type of the smoothed value.
 * @param block The audio block to fade in place.
 * @param fadeIn The smoothed value driving the fade ramp, shared and reset across all channels.
 */
template <typename SampleType, typename Type>
inline static void addFadeInToBlock (juce::dsp::AudioBlock<SampleType>& block,
                                     juce::SmoothedValue<Type>& fadeIn)
{
    for (auto channel { 0 }; channel < block.getNumChannels(); ++channel)
        addFadeInToBlock (block, channel, fadeIn);
}

/**
 * @brief Applies a fade-out ramp (1 to 0) to a single channel of a block.
 * @tparam SampleType The sample type of the block (float or double).
 * @tparam Type The numeric type of the smoothed value.
 * @param block The audio block to fade in place.
 * @param channel The channel index to fade.
 * @param fadeOut The smoothed value driving the fade ramp; reset and retargeted to 0.0 by this call.
 */
template <typename SampleType, typename Type>
inline static void addFadeOutToBlock (juce::dsp::AudioBlock<SampleType>& block,
                                      int channel,
                                      juce::SmoothedValue<Type>& fadeOut)
{
    fadeOut.reset (jam::toInt (block.getNumSamples()));
    fadeOut.setTargetValue (0.0);

    for (auto input = 0; input < block.getNumSamples(); ++input)
    {
        /** cast to double if SampleType is float */
        double output { static_cast<double> (block.getSample (channel, input)) * fadeOut.getNextValue() };

        block.setSample (channel, input, output);
    }
}

/**
 * @brief Applies a fade-out ramp (1 to 0) to every channel of a block.
 * @tparam SampleType The sample type of the block (float or double).
 * @tparam Type The numeric type of the smoothed value.
 * @param block The audio block to fade in place.
 * @param fadeIn The smoothed value driving the fade-out ramp, shared and reset across all channels.
 */
template <typename SampleType, typename Type>
inline static void addFadeOutToBlock (juce::dsp::AudioBlock<SampleType>& block,
                                      juce::SmoothedValue<Type>& fadeIn)
{
    for (auto channel { 0 }; channel < block.getNumChannels(); ++channel)
        addFadeOutToBlock (block, channel, fadeIn);
}

//==============================================================================

/**
 * @struct AudioBlock
 * @brief Left/right channel index constants and dual/single-channel per-sample processing helpers.
 */
struct AudioBlock
{
    enum
    {
        left,
        right
    };

    /**
     * @brief Calls function with each sample-index's left and right samples, writing back the (possibly modified) results.
     * @tparam SampleType The sample type of the block (float or double).
     * @tparam FunctionType A callable taking (SampleType& left, SampleType& right).
     * @param block The stereo audio block to process in place.
     * @param function The callable invoked once per sample index with the left and right samples.
     */
    template <typename SampleType, typename FunctionType>
    static void processDual (juce::dsp::AudioBlock<SampleType>& block,
                             const FunctionType& function)
    {
        for (auto spl { 0 }; spl < block.getNumSamples(); ++spl)
        {
            SampleType sampleL { block.getSample (left, spl) };
            SampleType sampleR { block.getSample (right, spl) };
            //==============================================================================
            function (sampleL, sampleR);
            //==============================================================================
            block.setSample (left, spl, sampleL);
            block.setSample (right, spl, sampleR);
        }
    }

    /**
     * @brief Calls function with each channel/sample-index's sample, writing back the (possibly modified) result.
     * @tparam SampleType The sample type of the block (float or double).
     * @tparam FunctionType A callable taking (int channel, SampleType& sample).
     * @param block The audio block to process in place.
     * @param function The callable invoked once per channel and sample index.
     */
    template <typename SampleType, typename FunctionType>
    static void processSingle (juce::dsp::AudioBlock<SampleType>& block,
                               const FunctionType& function)
    {
        for (auto channel { 0 }; channel < block.getNumChannels(); ++channel)
        {
            for (auto spl { 0 }; spl < block.getNumSamples(); ++spl)
            {
                SampleType sample { block.getSample (channel, spl) };
                //==============================================================================
                function (channel, sample);
                //==============================================================================
                block.setSample (channel, spl, sample);
            }
        }
    }
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::dsp
