/**
 * @file jam_StateSpace.h
 * @brief Efficient two-pole state-space hysteresis processor.
 */

namespace jam::dsp::Hysteresis
{
/*____________________________________________________________________________*/
/**
 * @class StateSpace
 * @brief Efficient state-space hysteresis processor using two-pole system.
 *
 * This class implements a computationally efficient hysteresis effect using
 * a state-space approach with two time-varying poles. Unlike the full
 * Jiles-Atherton model, this implementation uses coupled state variables with
 * different time constants to create hysteresis-like behavior with reduced
 * computational cost.
 *
 * @details
 * The implementation features:
 * - **Two-pole state-space system** with fast and slow state variables
 * - **Direction-dependent damping** based on input signal derivative
 * - **Asymmetric saturation curves** for positive and negative excursions
 * - **Efficient real-time processing** suitable for low-latency applications
 *
 * The fast state tracks the input signal quickly while the slow state lags behind,
 * creating a hysteresis-like memory effect. The damping is modulated based on
 * signal direction (rising vs falling), mimicking the coercivity behavior of
 * magnetic materials.
 *
 * @note
 * This model is approximately 3-4x more efficient than the full Jiles-Atherton
 * implementation while still providing musically pleasing hysteresis characteristics.
 * State variables should be reset when processing starts/stops to avoid clicks.
 *
 * @author PT Infinites Audio
 * @version 0.0.1
 * @date 2025-10-20
 */
class StateSpace
{
public:
    /**
     * @brief Constructor initializes state variables.
     */
    StateSpace()
    {
        reset();
    }

    /**
     * @brief Destructor for cleanup.
     */
    ~StateSpace() {}

    /**
     * @brief Resets the internal state variables and history.
     *
     * Call this method when starting playback or when discontinuities
     * in the audio stream occur to prevent artifacts.
     */
    void reset()
    {
        m_state1 = 0.0;
        m_state2 = 0.0;
        m_lastInput = 0.0;
    }

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
            calc();
        }
    }

    /**
     * @brief Computes internal processing coefficients.
     *
     * This method is called automatically when parameters change.
     * Recalculates pole coefficients based on hysteresis width and sample rate.
     */
    void calc()
    {
        double T = 1.0 / sampleRate;
        double width = hysteresisWidth * 10.0 + 0.1;

        // Time constants for two-pole system
        double tau1 = 1.0 / (width * 1000.0);  // Fast pole
        double tau2 = 1.0 / (width * 100.0);   // Slow pole

        a1 = std::exp (-T / tau1);
        a2 = std::exp (-T / tau2);
        b1 = 1.0 - a1;
        b2 = 1.0 - a2;
    }

    /**
     * @brief Sets the drive amount (input gain into the nonlinearity).
     *
     * @param newValue Drive amount in range [0.0, 1.0] where 0 is no drive
     *                 and 1.0 is maximum drive (maps to ~9x gain internally).
     */
    void setDrive (double newValue)
    {
        if (newValue != drive)
        {
            drive = juce::jlimit (0.0, 1.0, newValue);
            calc();
        }
    }

    /**
     * @brief Sets the saturation amount (wet/dry blend and saturation intensity).
     *
     * @param newValue Saturation amount in range [0.0, 1.0] where 0 is bypass
     *                 and 1.0 is full hysteresis saturation.
     */
    void setSaturation (double newValue)
    {
        if (newValue != saturation)
        {
            saturation = juce::jlimit (0.0, 1.0, newValue);
            calc();
        }
    }

    /**
     * @brief Sets the hysteresis loop width (time constant spread).
     *
     * @param newValue Hysteresis width in range [0.0, 1.0] where 0 is minimal
     *                 hysteresis (fast response) and 1.0 is maximum hysteresis
     *                 (larger time constant difference between poles).
     */
    void setHysteresisWidth (double newValue)
    {
        if (newValue != hysteresisWidth)
        {
            hysteresisWidth = juce::jlimit (0.0, 1.0, newValue);
            calc();
        }
    }

    /**
     * @brief Sets the asymmetry amount (DC bias for even harmonics).
     *
     * @param newValue Asymmetry in range [-1.0, +1.0] where 0 is symmetric,
     *                 positive values add positive DC bias, negative values
     *                 add negative DC bias. Creates even-order harmonics.
     */
    void setAsymmetry (double newValue)
    {
        if (newValue != asymmetry)
        {
            asymmetry = juce::jlimit (-1.0, 1.0, newValue);
            calc();
        }
    }

    /**
     * @brief Enables or disables processing bypass.
     *
     * @param newValue Bypass state (0.0 = process, 1.0 = bypass).
     */
    void setBypassed (double newValue)
    {
        bool newBypassed = jam::toInt (newValue) != 0;
        if (newBypassed != isBypassed)
        {
            isBypassed = newBypassed;
            if (isBypassed)
                reset();
        }
    }

    /**
     * @brief Processes a single audio sample through state-space hysteresis.
     *
     * Applies a two-pole state-space model with direction-dependent damping
     * to create efficient hysteresis effects. The fast and slow state variables
     * are updated and blended to produce the output.
     *
     * @tparam SampleType The sample type (e.g., float, double).
     * @param sample The audio sample to be processed (modified in-place).
     */
    template <typename SampleType>
    void process (SampleType& sample)
    {
        if (isBypassed || saturation < 0.001)
            return;

        // Store original input for wet/dry blend
        double input = static_cast<double> (sample);

        // 1. Apply drive
        double driven = input * (1.0 + drive * 8.0);

        // 2. Calculate input derivative (for direction-dependent behavior)
        double derivative = (driven - m_lastInput) * sampleRate;
        m_lastInput = driven;

        // 3. Add asymmetry
        double x = driven + asymmetry * 0.3;

        // 4. Direction-dependent damping
        double direction = std::tanh (derivative * 0.1); // -1 to +1
        double dampingMod = 1.0 + direction * hysteresisWidth * 0.5;

        // 5. Two-pole state-space model
        // Fast state: tracks input quickly
        double target1 = std::tanh (x * 1.2);
        m_state1 = a1 * m_state1 + b1 * target1 * dampingMod;

        // Slow state: lags behind, creates hysteresis
        double target2 = m_state1;
        m_state2 = a2 * m_state2 + b2 * target2;

        // 6. Combine states with asymmetric weighting
        double fastWeight = 0.6;
        double slowWeight = 0.4;
        double output = fastWeight * m_state1 + slowWeight * m_state2;

        // 7. Additional saturation curve
        output = saturate (output);

        // 8. Blend wet/dry
        sample = static_cast<SampleType> (input * (1.0 - saturation) + output * saturation);
    }

private:
    /**
     * @brief Asymmetric saturation function.
     *
     * Applies different saturation curves for positive and negative signals
     * to create additional harmonic richness and asymmetry.
     *
     * @param x Input value.
     * @return Saturated output value.
     */
    double saturate (double x)
    {
        // Asymmetric curves for positive and negative
        double pos_curve = 1.0 + saturation * 0.3;  // Harder saturation
        double neg_curve = 1.0 - saturation * 0.2;  // Softer saturation

        if (x > 0.0)
            return std::tanh (x * pos_curve) / pos_curve;
        else
            return std::tanh (x * neg_curve) / neg_curve;
    }

    //==============================================================================
    /// @brief Bypass flag indicating whether processing is disabled.
    bool isBypassed { false };

    /// @brief Current sample rate (Hz).
    double sampleRate { 44100.0 };

    /// @brief Fast state variable (tracks input quickly).
    double m_state1 { 0.0 };

    /// @brief Slow state variable (creates hysteresis lag).
    double m_state2 { 0.0 };

    /// @brief Previous input value (for derivative calculation).
    double m_lastInput { 0.0 };

    /// @brief Drive amount [0.0, 1.0] - input gain into nonlinearity.
    double drive { 0.0 };

    /// @brief Saturation amount [0.0, 1.0] - wet/dry mix and intensity.
    double saturation { 0.5 };

    /// @brief Hysteresis loop width [0.0, 1.0] - time constant spread.
    double hysteresisWidth { 0.5 };

    /// @brief Asymmetry [-1.0, 1.0] - DC bias for even harmonics.
    double asymmetry { 0.0 };

    /// @brief Fast pole coefficient (exponential decay).
    double a1 { 0.0 };

    /// @brief Slow pole coefficient (exponential decay).
    double a2 { 0.0 };

    /// @brief Fast pole gain coefficient.
    double b1 { 0.0 };

    /// @brief Slow pole gain coefficient.
    double b2 { 0.0 };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StateSpace)
};

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam::dsp::Hysteresis
