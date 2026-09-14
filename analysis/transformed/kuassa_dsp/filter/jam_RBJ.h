/**
 * @file jam_RBJ.h
 * @brief Implementation of Robert Bristow-Johnson (RBJ) filter algorithms.
 *
 * This file provides implementations of the Robert Bristow-Johnson filter algorithms,
 * which are commonly used in digital signal processing for various filter types.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/

/**
 * @brief Implements the Robert Bristow-Johnson (RBJ) filter algorithms.
 *
 * This class provides various filter types based on the RBJ cookbook formulas,
 * including low-pass, high-pass, band-pass, notch, peak, and shelving filters.
 */
class RBJ
{
public:
    /**
     * @brief Constructor for RBJ filter
     *
     * Initializes the filter with a given type and pole configuration.
     *
     * @param newType The filter type (highPass, lowPass, etc.)
     * @param newPole The pole configuration (one or two poles)
     */
    RBJ (Filter::Type newType = Filter::Type::highPass, Filter::Pole newPole = Filter::Pole::two) noexcept
        : type (newType)
        , pole (newPole)
    {
    }

    /**
     * @brief Destructor for RBJ filter
     */
    ~RBJ() = default;
    //==============================================================================

    /**
     * @brief Reset the filter state
     *
     * Clears the delay line state variables.
     */
    void reset() noexcept
    {
        z[0] = z[1] = 0.0;
    }

    /**
     * @brief Set the bypass state
     *
     * When bypassed, the filter will pass samples through unchanged.
     *
     * @param shouldBeBypassed True to bypass the filter, false to process normally
     */
    void setBypassed (bool shouldBeBypassed) noexcept
    {
        isBypassed = shouldBeBypassed;
    }

    /**
     * @brief Set the sample rate
     *
     * Updates the internal sample rate and recalculates filter coefficients.
     *
     * @param newSampleRate The new sample rate in Hz
     */
    void setSampleRate (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        calc();
    }

    /**
     * @brief Set the gain
     *
     * Updates the gain value in dB and recalculates filter coefficients.
     *
     * @param newGain The new gain value in dB
     */
    void setGain (double newGain) noexcept
    {
        if (newGain != gain)
        {
            gain = newGain;
            calc();
        }
    }

    /**
     * @brief Set the filter type
     *
     * Changes the filter type (lowPass, highPass, peak, etc.) and recalculates coefficients.
     *
     * @param newType The new filter type
     */
    void setType (Filter::Type newType) noexcept
    {
        if (newType != type)
        {
            type = newType;
            calc();
        }
    }

    /**
     * @brief Set the pole configuration
     *
     * Changes the pole configuration (one or two poles) and recalculates coefficients.
     *
     * @param newPole The new pole configuration
     */
    void setPole (Filter::Pole newPole) noexcept
    {
        if (newPole != pole)
        {
            pole = newPole;
            calc();
        }
    }

    /**
     * @brief Set the center frequency
     *
     * Updates the center frequency and recalculates filter coefficients.
     *
     * @param newFrequency The new center frequency in Hz
     */
    void setFrequency (double newFrequency) noexcept
    {
        if (newFrequency != frequency)
        {
            frequency = newFrequency;
            calc();
        }
    }

    /**
     * @brief Set the Q factor
     *
     * Updates the Q factor and recalculates filter coefficients.
     *
     * @param newQ The new Q factor
     */
    void setQ (double newQ) noexcept
    {
        if (newQ != Q)
        {
            Q = newQ;
            calc();
        }
    }

    /**
     * @brief Calculate filter coefficients
     *
     * Recalculates the filter coefficients based on current parameters.
     */
    void calc() noexcept
    {
        w = (2.0 * pi * frequency) / sampleRate; /** center frequency in rads/sample */
        alpha = std::sin (w) * 0.5 / Q;

        switch (type)
        {
            case Filter::Type::highShelf:
            case Filter::Type::lowShelf:
                G = std::pow(10.0, gain / 40.0); /** boost/cut gain */
                break;

            default:
                G = std::pow(10.0, gain / 20.0); /** boost/cut gain */
                break;
        }

        calculateCoefficients();
    }

    /**
     * @brief Process a single audio sample
     *
     * Applies the filter to a single audio sample.
     *
     * @tparam SampleType The type of the sample (e.g., float, double)
     * @param sample Reference to the audio sample to be processed
     */
    template <typename SampleType>
    void process (SampleType& sample) noexcept
    {
        if (not isBypassed)
        {
            double xd = static_cast<double> (sample);
            processSample (xd);
            sample = static_cast<SampleType> (xd);
        }
    }

    /**
     * @brief Get the current filter coefficients
     *
     * @return The current filter coefficients
     */
    Filter::Coefficient getCoefficients() const noexcept
    {
        return c;
    }

    /**
     * @brief Get the magnitude response
     *
     * Calculates the frequency response magnitude of the filter.
     *
     * @param numPoints Number of frequency points to evaluate (default: 1024)
     * @return Vector of magnitude values
     */
    Filter::Magnitudes getMagnitudes (int numPoints = 1024) const noexcept
    {
        return Filter::getMagnitudes (c, numPoints);
    }

    /**
     * @brief Get the current frequency setting
     *
     * @return The current frequency in Hz
     */
    double getFrequency() const noexcept { return frequency; }
    //==============================================================================

private:
    /**
     * @brief Process a single audio sample through the filter
     *
     * Applies the biquad filter to the input sample using the direct form II transposed structure.
     *
     * @param sample Reference to the audio sample to be processed
     */
    void processSample (double& sample) noexcept
    {
        double output = sample * c.b0 + z[0];

        z[0] = sample * c.b1 + z[1] - c.a1 * output;
        z[1] = sample * c.b2 - c.a2 * output;

        Filter::flushDenormals (z[0], z[1]);

        sample = output;
    }

    /**
     * @brief Calculate filter coefficients for the current type
     *
     * Calls the appropriate coefficient calculation method based on the current filter type.
     */
    void calculateCoefficients() noexcept
    {
        switch (type)
        {
            case Filter::Type::highPass: calculateHighPass(); break;
            case Filter::Type::lowPass: calculateLowPass(); break;
            case Filter::Type::highShelf: calculateHighShelf(); break;
            case Filter::Type::lowShelf: calculateLowShelf(); break;
            case Filter::Type::peak: calculatePeak(); break;
            case Filter::Type::allPass: calculateAllPass(); break;
            case Filter::Type::notch: calculateNotch(); break;
            case Filter::Type::bandPass: calculateBandPass(); break;
            case Filter::Type::bandPass2: calculateBandPass2(); break;
            default: break;
        }
    }

    /**
     * @brief Calculate coefficients for high-pass filter
     *
     * Computes the filter coefficients for a high-pass filter using either
     * first-order or second-order design.
     */
    void calculateHighPass() noexcept
    {
        switch (pole)
        {
            case Filter::Pole::one:
            {
                double gamma = std::cos (w) / (1.0 + std::sin (w));

                c.a0 = 1.0;
                c.b0 = (1.0 + gamma) / 2.0;
                c.b1 = -c.b0;
                c.b2 = 0.0;
                c.a1 = -gamma;
                c.a2 = 0.0;
            }
            break;
            case Filter::Pole::two:
            {
                c.a0 = 1.0 + alpha;
                c.b0 = ((1.0 + std::cos (w)) * 0.5) / c.a0;
                c.b1 = -(1.0 + std::cos (w)) / c.a0;
                c.b2 = ((1.0 + std::cos (w)) * 0.5) / c.a0;
                c.a1 = (-2.0 * std::cos (w)) / c.a0;
                c.a2 = (1.0 - alpha) / c.a0;
            }
            break;
        }
    }

    /**
     * @brief Calculate coefficients for low-pass filter
     *
     * Computes the filter coefficients for a low-pass filter using either
     * first-order or second-order design.
     */
    void calculateLowPass() noexcept
    {
        switch (pole)
        {
            case Filter::Pole::one:
            {
                // Using bilinear transform
                double gamma = std::cos (w) / (1.0 + std::sin (w));

                c.a0 = 1.0;
                c.b0 = (1.0 - gamma) / 2.0;
                c.b1 = c.b0;
                c.b2 = 0.0;
                c.a1 = -gamma;
                c.a2 = 0.0;
            }
            break;

            case Filter::Pole::two:
            {
                c.a0 = 1.0 + alpha;
                c.b0 = ((1.0 - std::cos (w)) * 0.5) / c.a0;
                c.b1 = (1.0 - std::cos (w)) / c.a0;
                c.b2 = ((1.0 - std::cos (w)) * 0.5) / c.a0;
                c.a1 = (-2.0 * std::cos (w)) / c.a0;
                c.a2 = (1.0 - alpha) / c.a0;
            }
            break;
        }
    }

    /**
     * @brief Calculate coefficients for high-shelf filter
     *
     * Computes the filter coefficients for a high-shelf filter.
     */
    void calculateHighShelf() noexcept
    {
        c.a0 = (G + 1.0) - (G - 1.0) * std::cos (w) + 2.0 * std::sqrt (G) * alpha;
        c.b0 = (G * ((G + 1.0) + (G - 1.0) * std::cos (w) + 2.0 * std::sqrt (G) * alpha)) / c.a0;
        c.b1 = (-2.0 * G * ((G - 1.0) + (G + 1.0) * std::cos (w))) / c.a0;
        c.b2 = (G * ((G + 1.0) + (G - 1.0) * std::cos (w) - 2.0 * std::sqrt (G) * alpha)) / c.a0;
        c.a1 = (2.0 * ((G - 1.0) - (G + 1.0) * std::cos (w))) / c.a0;
        c.a2 = ((G + 1.0) - (G - 1.0) * std::cos (w) - 2.0 * std::sqrt (G) * alpha) / c.a0;
    }

    /**
     * @brief Calculate coefficients for low-shelf filter
     *
     * Computes the filter coefficients for a low-shelf filter.
     */
    void calculateLowShelf() noexcept
    {
        c.a0 = (G + 1.0) + (G - 1.0) * std::cos (w) + 2.0 * std::sqrt (G) * alpha;
        c.b0 = (G * ((G + 1.0) - (G - 1.0) * std::cos (w) + 2.0 * std::sqrt (G) * alpha)) / c.a0;
        c.b1 = (2.0 * G * ((G - 1.0) - (G + 1.0) * std::cos (w))) / c.a0;
        c.b2 = (G * ((G + 1.0) - (G - 1.0) * std::cos (w) - 2.0 * sqrt (G) * alpha)) / c.a0;
        c.a1 = (-2.0 * ((G - 1.0) + (G + 1.0) * std::cos (w))) / c.a0;
        c.a2 = ((G + 1.0) + (G - 1.0) * std::cos (w) - 2.0 * std::sqrt (G) * alpha) / c.a0;
    }

    /**
     * @brief Calculate coefficients for peak filter
     *
     * Computes the filter coefficients for a peak (parametric) filter.
     */
    void calculatePeak() noexcept
    {
        c.a0 = 1.0 + alpha / G;
        c.b0 = (1.0 + alpha * G) / c.a0;
        c.b1 = (-2.0 * std::cos (w)) / c.a0;
        c.b2 = (1.0 - alpha * G) / c.a0;
        c.a1 = (-2.0 * std::cos (w)) / c.a0;
        c.a2 = (1.0 - alpha / G) / c.a0;
    }

    /**
     * @brief Calculate coefficients for all-pass filter
     *
     * Computes the filter coefficients for an all-pass filter.
     */
    void calculateAllPass() noexcept
    {
        c.a0 = 1.0 + alpha;
        c.b0 = (1.0 - alpha) / c.a0;
        c.b1 = -(2.0 * std::cos (w)) / c.a0;
        c.b2 = (1.0 + alpha) / c.a0;
        c.a1 = -(2.0 * std::cos (w)) / c.a0;
        c.a2 = (1.0 - alpha) / c.a0;
    }

    /**
     * @brief Calculate coefficients for notch filter
     *
     * Computes the filter coefficients for a notch filter.
     */
    void calculateNotch() noexcept
    {
        c.a0 = 1.0 + alpha;
        c.b0 = 1.0 / c.a0;
        c.b1 = -(2.0 * std::cos (w)) / c.a0;
        c.b2 = 1.0 / c.a0;
        c.a1 = -(2.0 * std::cos (w)) / c.a0;
        c.a2 = (1.0 - alpha) / c.a0;
    }

    /**
     * @brief Calculate coefficients for band-pass filter (constant skirt gain)
     *
     * Computes the filter coefficients for a band-pass filter with constant skirt gain.
     */
    void calculateBandPass() noexcept
    {
        c.a0 = 1.0 + alpha * 0.5;
        c.b0 = alpha * 0.5 / c.a0;
        c.b1 = 0.0;
        c.b2 = -alpha * 0.5 / c.a0;
        c.a1 = -(2.0 * std::cos (w)) / c.a0;
        c.a2 = (1.0 - alpha * 0.5) / c.a0;
    }

    /**
     * @brief Calculate coefficients for band-pass filter (constant peak gain)
     *
     * Computes the filter coefficients for a band-pass filter with constant peak gain.
     */
    void calculateBandPass2() noexcept
    {
        c.a0 = 1.0 + alpha * 0.5;
        c.b0 = (std::sin (w) * 0.5) / c.a0;
        c.b1 = 0.0;
        c.b2 = (-std::sin (w) * 0.5) / c.a0;
        c.a1 = (-2.0 * std::cos (w)) / c.a0;
        c.a2 = (1.0 - alpha * 0.5) / c.a0;
    }

    Filter::Type type;                  ///< Current filter type
    Filter::Pole pole;                  ///< Current pole configuration
    Filter::Coefficient c;              ///< Current filter coefficients

    double frequency { 1000.0 };        ///< Current center frequency in Hz
    double gain { 0.0 };                ///< Current gain in dB
    double Q { juce::MathConstants<double>::sqrt2 }; ///< Current Q factor

    double sampleRate { 44100.0 };      ///< Current sample rate in Hz
    double w {};                        ///< Normalized angular frequency
    double alpha {};                    ///< Bandwidth factor
    double G {};                        ///< Linear gain factor
    double z[2] {};                     ///< Delay line state variables
    bool isBypassed { false };          ///< Bypass state flag

    static constexpr double pi { Math::pi<double> }; ///< Constant pi value
};

// Verify DSP contract: trivially copyable (enables lock-free state snapshots)
static_assert (std::is_trivially_copyable_v<RBJ>,
               "RBJ must be trivially copyable per DSP contract");

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam::dsp
