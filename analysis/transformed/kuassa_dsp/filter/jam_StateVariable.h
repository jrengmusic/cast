/**
 * @file jam_StateVariable.h
 * @brief Implementation of the state-variable filter algorithm.
 *
 * This file provides an implementation of the state-variable filter algorithm,
 * which is capable of generating multiple filter responses (low-pass, high-pass,
 * band-pass, etc.) simultaneously from a single processing unit.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/

/**
 * @brief Implements a state-variable filter algorithm.
 *
 * This class implements a state-variable filter which can generate multiple
 * filter responses (low-pass, high-pass, band-pass, etc.) simultaneously.
 * The state-variable filter architecture is efficient and provides good
 * control over the filter characteristics.
 */
class StateVariable
{
public:
    /**
     * @brief Constructor for StateVariable filter.
     *
     * Initializes the filter with the specified type and pole configuration.
     *
     * @param newType The filter type (lowPass, highPass, etc.)
     * @param newPole The pole configuration (one or two poles)
     */
    StateVariable (Filter::Type newType = Filter::Type::highPass,
                   Filter::Pole newPole = Filter::Pole::two) noexcept
        : type (newType)
        , pole (newPole)
    {
    }

    /**
     * @brief Destructor for StateVariable filter.
     */
    ~StateVariable() = default;

    /**
     * @brief Reset the filter state.
     *
     * Clears the internal delay line state variables to zero.
     */
    void reset() noexcept
    {
        z[0] = 0.0;
        z[1] = 0.0;
    }

    /**
     * @brief Set the sample rate for the filter.
     *
     * Updates the internal sample rate and recalculates filter coefficients.
     *
     * @param newSampleRate The new sample rate in Hz.
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
     * @brief Set the filter type.
     *
     * Changes the filter type (lowPass, highPass, etc.) and recalculates coefficients.
     *
     * @param newType The new filter type.
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
     * @brief Set the pole configuration.
     *
     * Changes the pole configuration (first-order or second-order) and recalculates coefficients.
     *
     * @param newPole The new pole configuration.
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
     * @brief Set the center/cutoff frequency.
     *
     * Updates the center or cutoff frequency and recalculates filter coefficients.
     *
     * @param newFrequency The new frequency in Hz.
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
     * @brief Set the resonance parameter.
     *
     * Converts resonance to Q factor and updates the filter.
     *
     * @param newResonance The new resonance value (0.0 to 1.0).
     */
    void setResonance (double newResonance) noexcept
    {
        if (Filter::resonanceToQ (newResonance) != Q)
        {
            Q = Filter::resonanceToQ (newResonance);
            calc();
        }
    }

    /**
     * @brief Set the Q factor.
     *
     * Updates the Q factor and recalculates filter coefficients.
     *
     * @param newQ The new Q factor.
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
     * @brief Set the gain parameter.
     *
     * Updates the gain value and recalculates filter coefficients.
     *
     * @param newGain The new gain value.
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
     * @brief Set the bypass state.
     *
     * When bypassed, the filter will pass samples through unchanged.
     *
     * @param shouldBeBypassed True to bypass the filter, false to process normally.
     */
    void setBypassed (bool shouldBeBypassed) noexcept
    {
        isBypassed = shouldBeBypassed;
    }

    //==============================================================================
    /**
     * @brief Calculate filter coefficients.
     *
     * Recalculates the internal filter coefficients based on the current parameters.
     */
    void calc() noexcept
    {
        const double pi = juce::MathConstants<double>::pi;
        const double alpha { std::tan (frequency * pi / sampleRate) };

        switch (pole)
        {
            case Filter::Pole::one:
                // Calculate g (gain element of integrator)
                a0 = alpha / (1.0 + alpha);
                break;

            case Filter::Pole::two:
                // Calculate g (gain element of integrator)
                a0 = alpha;

                // Calculate Zavalishin's R from Q (referred to as damping parameter)
                a1 = 0.5 * Q;
                break;
        }
    }

    /**
     * @brief Process a single audio sample.
     *
     * Applies the filter to a single audio sample.
     *
     * @tparam SampleType The type of the sample (e.g., float, double)
     * @param sample Reference to the audio sample to be processed.
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
     * @brief Get the filter coefficients.
     *
     * Calculates and returns the biquad filter coefficients for the current settings.
     *
     * @return Filter::Coefficient The calculated coefficients.
     */
    Filter::Coefficient getCoefficients() const noexcept
    {
        Filter::Coefficient coeff;

        switch (pole)
        {
            case Filter::Pole::one:
            {
                // First-order filters
                const double g = a0;// a0 stores the g coefficient

                switch (type)
                {
                    case Filter::Type::lowPass:
                        coeff.b0 = g / (1.0 + g);
                        coeff.b1 = g / (1.0 + g);
                        coeff.b2 = 0.0;
                        coeff.a0 = 1.0;
                        coeff.a1 = -(1.0 - g) / (1.0 + g);
                        coeff.a2 = 0.0;
                        break;

                    case Filter::Type::highPass:
                        coeff.b0 = 1.0 / (1.0 + g);
                        coeff.b1 = -1.0 / (1.0 + g);
                        coeff.b2 = 0.0;
                        coeff.a0 = 1.0;
                        coeff.a1 = -(1.0 - g) / (1.0 + g);
                        coeff.a2 = 0.0;
                        break;

                    default:
                        break;
                }
            }
            break;

            case Filter::Pole::two:
            {
                // Second-order filters (SVF to biquad conversion)
                const double g = a0;
                const double R = 2.0 * a1;// a1 stores R/2
                const double denom = 1.0 + R * g + g * g;

                switch (type)
                {
                    case Filter::Type::lowPass:
                        coeff.b0 = g * g / denom;
                        coeff.b1 = 2.0 * g * g / denom;
                        coeff.b2 = g * g / denom;
                        coeff.a0 = 1.0;
                        coeff.a1 = 2.0 * (g * g - 1.0) / denom;
                        coeff.a2 = (1.0 - R * g + g * g) / denom;
                        break;

                    case Filter::Type::highPass:
                        coeff.b0 = 1.0 / denom;
                        coeff.b1 = -2.0 / denom;
                        coeff.b2 = 1.0 / denom;
                        coeff.a0 = 1.0;
                        coeff.a1 = 2.0 * (g * g - 1.0) / denom;
                        coeff.a2 = (1.0 - R * g + g * g) / denom;
                        break;

                    case Filter::Type::bandPass:
                        coeff.b0 = g / denom;
                        coeff.b1 = 0.0;
                        coeff.b2 = -g / denom;
                        coeff.a0 = 1.0;
                        coeff.a1 = 2.0 * (g * g - 1.0) / denom;
                        coeff.a2 = (1.0 - R * g + g * g) / denom;
                        break;

                    case Filter::Type::notch:
                        coeff.b0 = (1.0 + g * g) / denom;
                        coeff.b1 = 2.0 * (g * g - 1.0) / denom;
                        coeff.b2 = (1.0 + g * g) / denom;
                        coeff.a0 = 1.0;
                        coeff.a1 = 2.0 * (g * g - 1.0) / denom;
                        coeff.a2 = (1.0 - R * g + g * g) / denom;
                        break;

                    case Filter::Type::allPass:
                        coeff.b0 = (1.0 - R * g + g * g) / denom;
                        coeff.b1 = 2.0 * (g * g - 1.0) / denom;
                        coeff.b2 = 1.0;
                        coeff.a0 = 1.0;
                        coeff.a1 = 2.0 * (g * g - 1.0) / denom;
                        coeff.a2 = (1.0 - R * g + g * g) / denom;
                        break;

                    case Filter::Type::peak:
                        coeff.b0 = (1.0 + g * g) / denom;
                        coeff.b1 = 2.0 * (g * g - 1.0) / denom;
                        coeff.b2 = (1.0 - 2.0 * R * g + g * g) / denom;
                        coeff.a0 = 1.0;
                        coeff.a1 = 2.0 * (g * g - 1.0) / denom;
                        coeff.a2 = (1.0 - R * g + g * g) / denom;
                        break;

                    default:
                        // Default to unity gain
                        coeff.b0 = 1.0;
                        coeff.b1 = 0.0;
                        coeff.b2 = 0.0;
                        coeff.a0 = 1.0;
                        coeff.a1 = 0.0;
                        coeff.a2 = 0.0;
                        break;
                }
            }
            break;
        }

        return coeff;
    }

    /**
     * @brief Get the magnitude response at specific frequency points.
     *
     * Calculates the frequency response magnitude of the filter.
     *
     * @param numPoints Number of frequency points to evaluate (default: 1024).
     * @return Filter::Magnitudes Vector of magnitude values.
     */
    Filter::Magnitudes getMagnitudes (int numPoints = 1024) const
    {
        return Filter::getMagnitudes (getCoefficients(), numPoints);
    }

private:
    /**
     * @brief Process a single audio sample through the state-variable filter.
     *
     * Applies the state-variable filter algorithm to the input sample.
     *
     * @param sample Reference to the audio sample to be processed.
     */
    void processSample (double& sample) noexcept
    {
        double output { 0.0 };

        switch (pole)
        {
            case Filter::Pole::one:
            {
                z[1] = (sample - z[0]) * a0;
                const double lowPass { z[1] + z[0] };
                z[0] = lowPass + z[1];
                const double highPass { sample - lowPass };

                switch (type)
                {
                    case Filter::Type::highPass:
                        output = highPass;
                        break;
                    case Filter::Type::lowPass:
                        output = lowPass;
                        break;
                    default:
                        output = sample;
                        break;
                }
            }
            break;

            case Filter::Pole::two:
            {
                const double highPass = (sample - (2.0 * a1 + a0) * z[0] - z[1])
                                        / (1.0 + (2.0 * a1 * a0) + a0 * a0);

                const double bandPass = highPass * a0 + z[0];
                const double lowPass = bandPass * a0 + z[1];
                const double bandPass2 = 2.0 * a1 * bandPass;
                const double highShelf = sample + bandPass2 * gain;
                const double notch = sample - bandPass2;
                const double allPass = sample - (4.0 * a1 * bandPass);
                const double peak = lowPass - highPass;

                z[0] = a0 * highPass + bandPass;
                z[1] = a0 * bandPass + lowPass;

                switch (type)
                {
                    case Filter::Type::lowPass:
                        output = lowPass;
                        break;
                    case Filter::Type::highPass:
                        output = highPass;
                        break;
                    case Filter::Type::peak:
                        output = peak;
                        break;
                    case Filter::Type::highShelf:
                        output = highShelf;
                        break;
                    case Filter::Type::notch:
                        output = notch;
                        break;
                    case Filter::Type::allPass:
                        output = allPass;
                        break;
                    case Filter::Type::bandPass:
                        output = bandPass;
                        break;
                    case Filter::Type::bandPass2:
                        output = bandPass2;
                        break;
                    default:
                        output = sample;
                        break;
                }
            }
            break;
        }

        Filter::flushDenormals (z[0], z[1]);

        sample = output;
    }

    double sampleRate { 48000.0 };///< Current sample rate in Hz
    Filter::Type type { Filter::Type::lowPass };///< Current filter type
    Filter::Pole pole { Filter::Pole::two };///< Current pole configuration
    double frequency { 1000.0 };///< Current center/cutoff frequency in Hz
    double a0 { 1.0 };///< First coefficient (gCoeff in state variable context)
    double a1 { 1.0 };///< Second coefficient (Rcoeff in state variable context)
    double Q { 0.5 };///< Current Q factor
    double gain { 1.0 };///< Current gain factor (KCOeff in state variable context)
    double z[2] { 0.0 };///< Internal state variables (delay line)
    bool isBypassed { false };///< Bypass state flag
    //==============================================================================
};

// Verify DSP contract: trivially copyable (enables lock-free state snapshots)
static_assert (std::is_trivially_copyable_v<StateVariable>,
               "StateVariable must be trivially copyable per DSP contract");

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::dsp
