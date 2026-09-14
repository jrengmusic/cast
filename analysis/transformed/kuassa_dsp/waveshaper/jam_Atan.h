/**
 * @file jam_Atan.h
 * @brief Arctan-based waveshaping processor with sample-rate-adaptive bias compensation.
 */

namespace jam::dsp::Waveshaper
{
/*____________________________________________________________________________*/

/**
 * @class Atan
 * @brief A waveshaping processor using atan-based distortion with bias compensation
 * adaptive to higher samplerate (oversampled).
 *
 * This class applies nonlinear distortion via an arctan waveshaper while compensating
 * for spectral tilt effects due to varying sample rates. It includes bias filtering
 * and normalization to maintain consistent gain across different conditions.
 */
class Atan
{
public:
    /**
     * @brief Constructor initializes low-pass filter frequency.
     */
    Atan() = default;

    /**
     * @brief Sets the sample rate and updates compensation factors.
     *
     * @param newValue New sample rate in Hz.
     */
    void setSampleRate (double newValue)
    {
        if (newValue != sampleRate)
        {
            sampleRate = newValue;
            lowPass.setSampleRate (sampleRate);
            
            const double nyquist { sampleRate * 0.5 };
            lowFreqCompensation = centreFrequency / nyquist;

            calc();
        }
    }

    /**
     * @brief Computes low-pass filter coefficients.
     */
    void calc()
    {
        if (lowPass.getFrequency() != biasLowPassFrequency)
            lowPass.setFrequency(biasLowPassFrequency);
        lowPass.calc();
    }

    /**
     * @brief Processes a single sample with nonlinear waveshaping.
     *
     * @tparam SampleType Type of sample (e.g., float, double).
     * @param sample Audio sample to be processed.
     */
    template <typename SampleType>
    void process (SampleType& sample)
    {
        // Compute bias and apply compensation.
        SampleType bias = std::abs (sample);
        lowPass.process (bias);
        bias *= preBias * lowFreqCompensation;

        // Subtract bias before waveshaping.
        double wet = sample - bias;

        // Apply atan-based waveshaper normalization.
        wet = std::atan (wet * preGain) / std::atan (preGain);

        // Apply post-processing gain and overall normalization.
        sample = (wet * postGain + postBias) * overallNormalization;
    }

    /// @brief Sets the pre-gain factor.
    /// @param newNormalisedValue Normalized gain value.
    void setPreGain (double newNormalisedValue)
    {
        preGain = newNormalisedValue;
    }

    /// @brief Sets the pre-bias value.
    /// @param newNormalisedValue Normalized bias level.
    void setPreBias (double newNormalisedValue)
    {
        preBias = newNormalisedValue;
    }

    /// @brief Sets the post-gain factor.
    /// @param newNormalisedValue Normalized gain value.
    void setPostGain (double newNormalisedValue)
    {
        postGain = newNormalisedValue;
    }

    /// @brief Sets the post-bias value.
    /// @param newNormalisedValue Normalized bias level.
    void setPostBias (double newNormalisedValue)
    {
        postBias = newNormalisedValue;
    }

    /// @brief Resets internal state (delay lines in lowPass filter).
    void reset() noexcept
    {
        lowPass.reset();
    }

private:
    /// @brief One-pole low-pass filter for bias processing.
    RBJ lowPass { Filter::Type::lowPass, Filter::Pole::one };

    /// @brief Pre-bias scaling factor.
    double preBias { 1.0 };

    /// @brief Pre-gain scaling factor before waveshaping.
    double preGain { 1.0 };

    /// @brief Post-gain scaling factor after waveshaping.
    double postGain { 1.0 };

    /// @brief Bias offset applied after waveshaping.
    double postBias { 0.0 };

    /// @brief Current sample rate (Hz).
    double sampleRate { 44100.0 };

    /// @brief Compensation factor for low-frequency attenuation.
    double lowFreqCompensation { 1.0 };

    /// @brief Overall gain normalization factor.
    static constexpr double overallNormalization { Math::halfSqrt2<double> };

    /// @brief Cutoff frequency for bias low-pass filtering.
    static constexpr double biasLowPassFrequency { 10.0 };

    /// @brief Logarithmically centered reference frequency.
    ///
    /// This constant represents the geometric midpoint of the standard audio range (20 Hz – 20,000 Hz).
    /// It is used as a basis for frequency-dependent compensation, particularly in nonlinear processing.
    ///
    /// @details
    /// The value is computed using the geometric mean formula:
    /// @f$ f_c = \sqrt{f_{\text{low}} \times f_{\text{high}}} @f$
    /// which ensures perceptual balance in logarithmic space.
    ///
    /// In this context, `centreFrequency` is leveraged to normalize spectral tilt effects that
    /// arise due to varying sample rates, particularly affecting low-frequency bias compensation.
    /// Adjustments based on Nyquist frequency scaling allow consistent energy distribution in
    /// waveshaping applications.
    ///
    /// @note
    /// While this is a static constant, alternative perceptual weightings (e.g., equal-loudness contours)
    /// may further refine compensation models.
    ///
    static constexpr double centreFrequency { 632.456 };
};

// Verify DSP contract: trivially copyable (enables lock-free state snapshots)
static_assert (std::is_trivially_copyable_v<Atan>,
               "Atan must be trivially copyable per DSP contract");

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::dsp::Waveshaper
