/**
 * @file jam_Orfanidis.h
 * @brief Implementation of the Orfanidis EQ filter algorithm.
 *
 * This file provides an implementation of the Orfanidis EQ filter algorithm, which
 * is used for creating parametric equalizers with complex frequency responses.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/

/**
 * @brief Implements the Orfanidis EQ filter algorithm for parametric equalization.
 *
 * The Orfanidis filter is a sophisticated parametric EQ implementation that allows
 * for precise control of gain, frequency, and bandwidth with complex mathematical
 * relationships to achieve desired frequency responses.
 */
class Orfanidis
{
public:
    /**
     * @brief Constructor for Orfanidis filter
     *
     * Initializes the filter with a given type and width factor.
     *
     * @param newType The filter type (peak, lowShelf, highShelf)
     * @param widthFactor A factor affecting the filter width response
     */
    Orfanidis (Filter::Type newType = Filter::Type::peak, double widthFactor = 1.0) noexcept
        : type (newType)
        , width (widthFactor)
    {
    }

    /**
     * @brief Destructor for Orfanidis filter
     */
    ~Orfanidis() = default;
    //==============================================================================

    /**
     * @brief Reset the filter state
     *
     * Clears the internal state variables (z[0] and z[1]) to zero.
     */
    void reset() noexcept
    {
        z[0] = 0.0;
        z[1] = 0.0;
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
     * @brief Set the inverted state
     *
     * Inversion is folded into the numerator coefficients at calculation time
     * (calculateCoefficients() applies b <- a - b when inverted), so
     * processSample() stays branch-free regardless of this flag.
     *
     * @param shouldBeInverted True to invert the filter response, false otherwise
     */
    void setInverted (bool shouldBeInverted) noexcept
    {
        if (isInverted != shouldBeInverted)
        {
            isInverted = shouldBeInverted;
            calc();
        }
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
        if (newSampleRate != sampleRate)
        {
            sampleRate = newSampleRate;
            calc();
        }
    }

    /**
     * @brief Set the filter gain
     *
     * Updates the gain value in dB and recalculates filter coefficients.
     *
     * @param newValue The new gain value in dB
     */
    void setGain (double newValue) noexcept
    {
        if (newValue != dBgain)
        {
            dBgain = newValue;
            calc();
        }
    }

    /**
     * @brief Set the filter type
     *
     * Changes the filter type (peak, lowShelf, highShelf) and recalculates coefficients.
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
     * @brief Set the center frequency
     *
     * Updates the center frequency and recalculates filter coefficients.
     *
     * @param newValue The new center frequency in Hz
     */
    void setFrequency (double newValue) noexcept
    {
        if (newValue != frequency)
        {
            frequency = newValue;
            calc();
        }
    }

    /**
     * @brief Set the bandwidth
     *
     * Updates the bandwidth and recalculates filter coefficients.
     *
     * @param newValue The new bandwidth value
     */
    void setBandwidth (double newValue) noexcept
    {
        if (newValue != bandwidth)
        {
            bandwidth = newValue;
            calc();
        }
    }

    /**
     * @brief Set the Q factor
     *
     * Converts the Q factor to bandwidth and updates the filter.
     * BW = 2 * asinh(1/(2Q)) / ln(2)
     *
     * @param newQ The new Q factor
     */
    void setQ (double newQ) noexcept
    {
        // Convert Q to bandwidth in octaves: BW = 2 * asinh(1/(2Q)) / ln(2)
        const double bandwidthOctaves { 2.0 * std::asinh (1.0 / (2.0 * newQ)) / std::log (2.0) };
        setBandwidth (bandwidthOctaves);
    }

    /**
     * @brief Calculate filter coefficients
     *
     * Recalculates the filter coefficients based on current parameters.
     */
    void calc() noexcept
    {
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
            double sampleValue { static_cast<double> (sample) };
            processSample (sampleValue);
            sample = static_cast<SampleType> (sampleValue);
        }
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
     * @brief Get the current filter coefficients
     *
     * @return The current filter coefficients
     */
    Filter::Coefficient getCoefficient() const noexcept
    {
        return c;
    }

    /**
     * @brief Get the current gain in dB
     *
     * @return The current gain value in dB
     */
    double getGainDecibel() const noexcept
    {
        return dBgain;
    }

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
        double output { sample * c.b0 + z[0] };

        z[0] = sample * c.b1 + z[1] - c.a1 * output;
        z[1] = sample * c.b2 - c.a2 * output;

        Filter::flushDenormals (z[0], z[1]);

        sample = output;
    }

    /**
     * @brief Calculate filter coefficients based on current parameters
     *
     * Recalculates the filter coefficients based on the current type and parameters.
     * If dBgain is 0, sets coefficients to identity (no change). When isInverted is
     * true, folds the inversion into the numerator afterward via b <- a - b, so
     * processSample() never branches on the inverted state.
     */
    void calculateCoefficients() noexcept
    {
        if (dBgain == 0.0)
        {
            c.reset();
        }
        else
        {
            switch (type)
            {
                case Filter::Type::peak:      calculatePeak();      break;
                case Filter::Type::lowShelf:  calculateLowShelf();  break;
                case Filter::Type::highShelf: calculateHighShelf(); break;
                default: break;
            }
        }

        if (isInverted)
        {
            c.b0 = c.a0 - c.b0;
            c.b1 = c.a1 - c.b1;
            c.b2 = c.a2 - c.b2;
        }
    }

    /**
     * @brief Calculate coefficients for peak filter
     *
     * Computes the filter coefficients for a peak (parametric) EQ filter
     * using the Orfanidis algorithm.
     */
    void calculatePeak() noexcept
    {
        const auto& pow2 = jam::Math::pow2<double>;
        const auto& square = jam::Math::square<double>;

        gain = isInverted ? 1.0 + amp (dBgain) : amp (dBgain);

        double GB { amp2 (dBgain) };
        double G0 { 1.0 };
        double f { (frequency * pow2 (bandwidth) - frequency) / sampleRate * 2 };
        double dw { width * pi * (std::exp (f) - std::exp (-f)) / (std::exp (f) + std::exp (-f)) / 2 };
        double F { std::abs (square (gain) - square (GB)) };
        double G00 { std::abs (square (gain) - square (G0)) };
        double F00 { std::abs (square (GB) - square (G0)) };
        double w { 2.0 * pi * frequency / sampleRate };
        double num { square (G0) * square (square (w) - square (pi)) + square (gain) * F00 * square (pi) * square (dw) / F };
        double den { square (square (w) - square (pi)) + F00 * square (pi) * square (dw) / F };
        double G1 { std::sqrt (num / den) };
        double G01 { std::abs (square (gain) - G0 * G1) };
        double G11 { std::abs (square (gain) - square (G1)) };
        double F01 { std::abs (square (GB) - G0 * G1) };
        double F11 { std::abs (square (GB) - square (G1)) };
        double W2 { (sqrt (G11 / G00) * square (std::tan (w / 2))) };
        double DW { (1.0 + std::sqrt (F00 / F11) * W2) * std::tan (dw / 2) };
        double C { F11 * square (DW) - 2.0 * W2 * (F01 - std::sqrt (F00 * F11)) };
        double D { 2.0 * W2 * (G01 - std::sqrt (G00 * G11)) };
        double A { std::sqrt ((C + D) / F) };
        double B { std::sqrt ((square (gain) * C + square (GB) * D) / F) };

        c.a0 = 1.0;
        double a0 = (1.0 + W2 + A);

        c.b0 = (G1 + G0 * W2 + B) / a0;
        c.b1 = -2.0 * (G1 - G0 * W2) / a0;
        c.b2 = (G1 - B + G0 * W2) / a0;
        c.a1 = -2.0 * (1.0 - W2) / a0;
        c.a2 = (1.0 + W2 - A) / a0;
    }

    /**
     * @brief Calculate coefficients for low-shelf filter
     *
     * Computes the filter coefficients for a low-shelf EQ filter
     * using the Orfanidis algorithm.
     */
    void calculateLowShelf() noexcept
    {
        const auto& square = jam::Math::square<double>;

        gain = amp (dBgain);
        double G0 { 1.0 };
        double GB { amp2 (dBgain) };

        double w0 { 2.0 * pi * frequency / sampleRate };

        double epsilon_squared { (square (gain) - square (GB)) / (square (GB) - square (G0)) };
        double epsilon { std::sqrt (epsilon_squared) };

        int N = 2;
        double g { std::pow (gain, 1.0 / N) };
        double g0 { std::pow (G0, 1.0 / N) };

        double dww { bandwidth * w0 };
        double WB { w0 * std::sqrt ((1.0 + dww * dww) / (1.0 - dww * dww)) };
        double beta { std::pow (epsilon, -1.0 / N) * std::tan (WB / 2.0) };

        double si { std::sin (pi / 4.0) };

        double D { square (beta) + 2.0 * si * beta + 1.0 };

        c.a0 = 1.0;
        c.b0 = (square (g * beta) + 2.0 * g * g0 * si * beta + square (g0)) / D;
        c.b1 = 2.0 * (square (g * beta) - square (g0)) / D;
        c.b2 = (square (g * beta) - 2.0 * g * g0 * si * beta + square (g0)) / D;
        c.a1 = 2.0 * (square (beta) - 1.0) / D;
        c.a2 = (square (beta) - 2.0 * si * beta + 1.0) / D;
    }

    /**
     * @brief Calculate coefficients for high-shelf filter
     *
     * Computes the filter coefficients for a high-shelf EQ filter
     * using the Orfanidis algorithm.
     */
    void calculateHighShelf() noexcept
    {
        const auto& square = jam::Math::square<double>;

        gain = amp (dBgain);
        double G0 { 1.0 };
        double GB { amp2 (dBgain) };

        double w0 { 2.0 * pi * frequency / sampleRate };

        double epsilon_squared { (square (gain) - square (GB)) / (square (GB) - square (G0)) };
        double epsilon { std::sqrt (epsilon_squared) };

        int N = 2;
        double g { std::pow (gain, 1.0 / N) };
        double g0 { std::pow (G0, 1.0 / N) };

        double dww { bandwidth * w0 };
        double WB { w0 * std::sqrt ((1.0 + dww * dww) / (1.0 - dww * dww)) };
        double beta { std::pow (epsilon, -1.0 / N) * std::tan (WB / 2.0) };

        double si { std::sin (pi / 4.0) };

        double D { square (beta) + 2.0 * si * beta + 1.0 };

        c.a0 = 1.0;
        c.b0 = (square (g) + 2.0 * g * g0 * si * beta + square (g0 * beta)) / D;
        c.b1 = 2.0 * (square (g) - square (g0 * beta)) / D;
        c.b2 = (square (g) - 2.0 * g * g0 * si * beta + square (g0 * beta)) / D;
        c.a1 = 2.0 * (square (beta) - 1.0) / D;
        c.a2 = (square (beta) - 2.0 * si * beta + 1.0) / D;
    }

    Filter::Type type;                  ///< Current filter type
    Filter::Coefficient c;              ///< Current filter coefficients

    double width;                       ///< Width factor for the filter response
    double bandwidth { std::sqrt (2.0) };///< Current bandwidth value
    double dBgain { 0.0 };              ///< Current gain in dB
    double frequency { 1000.0 };        ///< Current center frequency in Hz

    double gain { 1.0 };                ///< Linear gain value
    double sampleRate { 44100.0 };      ///< Current sample rate in Hz
    double z[2] {};                     ///< Delay line state variables
    bool isBypassed { false };          ///< Bypass state flag
    bool isInverted { false };          ///< Inversion state flag

    static constexpr double pi { Math::pi<double> }; ///< Constant pi value
};

// Verify DSP contract: trivially copyable (enables lock-free state snapshots)
static_assert (std::is_trivially_copyable_v<Orfanidis>,
               "Orfanidis must be trivially copyable per DSP contract");

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam::dsp
