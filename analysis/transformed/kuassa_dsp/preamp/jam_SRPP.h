/**
 * @file jam_SRPP.h
 * @brief Shunt Regulated Push-Pull preamp emulation — waveshaping, bias
 *        compensation, high-pass filtering, and perceptual gain mapping.
 */

namespace jam::dsp::Preamp
{
/*____________________________________________________________________________*/
/**
 * @class SRPP
 * @brief A signal processing unit integrating waveshaping and high-pass filtering
 * modeled after Shunt Regulated Push-Pull Preamp;
 *
 * The SRPP class applies a nonlinear waveshaping technique with bias compensation,
 * followed by high-pass filtering to maintain spectral integrity. Gain normalization
 * is dynamically mapped across a specified decibel range.
 *
 * @details
 * This processor combines:
 * - **Nonlinear waveshaping** via an Atan-based distortion model.
 * - **Bias compensation** to correct spectral tilt effects from waveshaping.
 * - **High-pass filtering** to attenuate unwanted low-frequency artifacts.
 * - **Gain normalization** using a perceptual mapping function.
 *
 * The internal gain-mapping mechanism leverages the golden ratio (`phi`) and `pi`
 * to provide smooth perceptual transitions.
 */
class SRPP
{
public:
    /**
     * @brief Constructor initializes filter frequency.
     */
    SRPP() = default;

    /**
     * @brief Destructor for cleanup.
     */
    ~SRPP() = default;

    /**
     * @brief Sets the sample rate and updates processing coefficients.
     *
     * @param newSampleRate The new sample rate in Hz.
     */
    void setSampleRate (double newSampleRate)
    {
        if (newSampleRate != sampleRate)
        {
            sampleRate = newSampleRate;
            shaper.setSampleRate (sampleRate);
            highPass.setSampleRate (sampleRate);
            calc();
        }
    }

    /**
     * @brief Computes filter and waveshaper adjustments.
     *
     * Ensures bias compensation and gain normalization remain consistent with
     * changes in gain range, bias settings, and sample rate.
     */
    void calc()
    {
        shaper.setPreGain (preGain);
        shaper.setPostBias (postBias);
        shaper.setPreBias (preBias);

        const double postGainFactor { 1 + (sampleRate <= 48000.0 ? Math::sqrt2<double> : Math::halfSqrt2<double>) };
        postGain = Value::map (gain, minGain, maxGain, 1.0, postGainFactor);

        shaper.setPostGain (postGain);

        if (highPass.getFrequency() != highPassFrequency)
            highPass.setFrequency (highPassFrequency);

        highPass.calc();
    }

    /**
     * @brief Processes a single audio sample.
     *
     * Applies gain scaling, waveshaping, and high-pass filtering in sequence.
     *
     * @tparam SampleType The sample type (e.g., float, double).
     * @param sample The audio sample to be processed.
     */
    template <typename SampleType>
    void process (SampleType& sample)
    {
        if (not isBypassed)
        {
            sample *= gain;
            shaper.process (sample);
            highPass.process (sample);
        }
    }

    //==============================================================================
    /// @brief Enables or disables processing bypass.
    /// @param newValue A boolean-like indicator (converted internally).
    void setBypassed (double newValue)
    {
        const bool newBypass { jam::toInt (newValue) != 0 };
        if (newBypass != isBypassed)
        {
            isBypassed = newBypass;
            calc();
        }
    }

    /// @brief Sets the gain range in decibels.
    /// @param newMindB Minimum gain in dB.
    /// @param newMaxdB Maximum gain in dB.
    void setRange (double newMindB, double newMaxdB)
    {
        minGain = juce::Decibels::decibelsToGain (newMindB);
        maxGain = juce::Decibels::decibelsToGain (newMaxdB);
        calc();
    }

    /// @brief Adjusts the gain level.
    /// @param newValueDB New gain in decibels.
    void setGain (double newValueDB)
    {
        if (newValueDB != juce::Decibels::gainToDecibels (gain))
        {
            gain = juce::Decibels::decibelsToGain (newValueDB);
            calc();
        }
    }

    /// @brief Resets internal state (delay lines in shaper and highPass).
    void reset() noexcept
    {
        shaper.reset();
        highPass.reset();
    }

private:
    /// @brief Bypass flag indicating whether processing is disabled.
    bool isBypassed { false };

    /// @brief Current sample rate (Hz).
    double sampleRate { 44100.0 };

    /// @brief Waveshaping processor using atan-based saturation.
    Waveshaper::Atan shaper;

    /// @brief High-pass filter for post-processing.
    RBJ highPass { Filter::Type::highPass };

    /// @brief Gain values in amplitude.
    double gain { 1.0 };///< Gain scaling in amplitude.

    /// @brief min Gain values in amplitude.
    double minGain { juce::Decibels::decibelsToGain (-60.0) };

    /// @brief max Gain values in amplitude.
    double maxGain { juce::Decibels::decibelsToGain (20.0) };

    /// @brief Pre-bias and post-bias parameters for nonlinear compensation.
    double preBias { 0.28540401387201614 };
    double postBias { 0.0f };

    /// @brief Pre-gain level before waveshaping.
    double preGain { 1.0759646651078483 };

    /// @brief Post-gain level after waveshaping.
    double postGain { 1.0 };

    /// @brief High-pass filter frequency for post-processing.
    ///
    /// This frequency attenuates unwanted low-frequency content introduced
    /// by waveshaping and bias compensation.
    static constexpr double highPassFrequency { 9.944591886601053 };
};

// Verify DSP contract: trivially copyable (enables lock-free state snapshots)
static_assert (std::is_trivially_copyable_v<SRPP>,
               "SRPP must be trivially copyable per DSP contract");

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::dsp::Preamp
