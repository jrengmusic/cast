/**
 * @file jam_Saturator.h
 * @brief Magnetic tape-style hysteresis saturation processor (simplified Jiles-Atherton model).
 */

namespace jam::dsp::Hysteresis
{
/*____________________________________________________________________________*/
/**
 * @class Saturator
 * @brief Magnetic tape-style hysteresis saturation processor using simplified
 * Jiles-Atherton model.
 *
 * This class implements a nonlinear hysteresis effect that emulates the behavior
 * of magnetic recording media. The hysteresis loop creates a memory effect where
 * the output depends not only on the current input but also on the signal history,
 * producing characteristic harmonic distortion and dynamic saturation.
 *
 * @details
 * The implementation uses a simplified Jiles-Atherton model which captures:
 * - **Anhysteretic magnetization** via Langevin function (ideal magnetization curve)
 * - **Hysteresis loop behavior** through differential equations modeling coercivity
 * - **Asymmetric saturation** for even-harmonic generation
 * - **Dynamic state-dependent processing** where magnetization state evolves over time
 *
 * The processor maintains an internal magnetization state (M) that changes based on
 * the input signal (H - magnetic field), creating the characteristic hysteresis loop.
 * This results in a warm, analog-style saturation with frequency-dependent behavior.
 *
 * @note
 * The magnetization state should be reset when processing is stopped and restarted
 * to avoid discontinuities. The model is suitable for real-time processing and
 * provides musically pleasing saturation characteristics.
 *
 * @author PT Infinites Audio
 * @version 0.0.1
 * @date 2025-10-20
 */
class Saturator
{
public:
    /**
     * @brief Constructor initializes magnetization state.
     */
    Saturator()
    {
        reset();
    }

    /**
     * @brief Destructor for cleanup.
     */
    ~Saturator() {}

    /**
     * @brief Resets the internal magnetization state and history.
     *
     * Call this method when starting playback or when discontinuities
     * in the audio stream occur to prevent artifacts.
     */
    void reset()
    {
        m_M = 0.0;
        m_H_prev = 0.0;
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
     * Recalculates time-step dependent values and integration factors.
     */
    void calc()
    {
        dt = 1.0 / sampleRate;
    }

    /**
     * @brief Sets the drive amount (input gain into the nonlinearity).
     *
     * @param newValue Drive amount in range [0.0, 1.0] where 0 is no drive
     *                 and 1.0 is maximum drive (maps to ~6x gain internally).
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
     * @brief Sets the hysteresis loop width (coercivity).
     *
     * @param newValue Hysteresis width in range [0.0, 1.0] where 0 is minimal
     *                 hysteresis (closer to soft clipping) and 1.0 is maximum
     *                 hysteresis loop width (more memory/lag effect).
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
     * @brief Processes a single audio sample through hysteresis saturation.
     *
     * Applies the Jiles-Atherton hysteresis model to create nonlinear saturation
     * with memory effects. The magnetization state is updated based on the input
     * signal and previous state.
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

        // 1. Apply drive (input gain)
        double driven = input * (1.0 + drive * 5.0);

        // 2. Add asymmetry (DC offset for even harmonics)
        double H = driven + asymmetry * 0.5;

        // 3. Calculate anhysteretic magnetization (ideal, no hysteresis)
        double M_an = langevin (H);

        // 4. Calculate derivative (rate of change)
        double dH = H - m_H_prev;
        double dH_dt = dH * sampleRate;

        // 5. Hysteresis: magnetization lags behind ideal curve
        double delta = M_an - m_M;

        // 6. Hysteresis parameters
        double k = hysteresisWidth * 0.5; // Coercivity
        double alpha = hysteresisWidth;   // Inter-domain coupling

        double dM_an_dH = langevinDerivative (H);

        // Sign-dependent hysteresis coefficient
        double sign_dH = (dH > 0.0) ? 1.0 : -1.0;

        // Differential equation for magnetization
        double c = (1.0 - alpha) * delta;
        double denominator = (1.0 - alpha) * k - alpha * dM_an_dH;

        if (std::abs (denominator) > 1e-6)
        {
            double dM = c / denominator;

            // Integrate magnetization state
            m_M += dM * sign_dH * std::abs (dH_dt) * dt;

            // Clamp to physical limits
            m_M = juce::jlimit (-1.0, 1.0, m_M);
        }

        // Store for next sample
        m_H_prev = H;

        // 7. Output magnetization with wet/dry blend
        double wet = m_M;

        // Blend: preserve input at saturation=0, full hysteresis at saturation=1
        sample = static_cast<SampleType> (input * (1.0 - saturation) + wet * saturation);
    }

private:
    /**
     * @brief Langevin function - anhysteretic magnetization curve.
     *
     * Computes the ideal (no hysteresis) magnetization response to magnetic field.
     * Uses Taylor series approximation for small values to avoid numerical issues.
     *
     * @param H Magnetic field strength.
     * @return Anhysteretic magnetization value.
     */
    double langevin (double H)
    {
        const double Ms = 1.0;  // Saturation magnetization
        const double a = 1.0 / (saturation + 0.01); // Shape parameter

        double x = a * H;

        if (std::abs (x) < 0.001)
        {
            // Taylor series for small x (avoid division by zero)
            return Ms * (x / 3.0 - x * x * x / 45.0);
        }

        double coth_x = 1.0 / std::tanh (x);
        return Ms * (coth_x - 1.0 / x);
    }

    /**
     * @brief Derivative of Langevin function with respect to H.
     *
     * Required for solving the differential equations in the hysteresis model.
     *
     * @param H Magnetic field strength.
     * @return Derivative of anhysteretic magnetization.
     */
    double langevinDerivative (double H)
    {
        const double a = 1.0 / (saturation + 0.01);
        double x = a * H;

        if (std::abs (x) < 0.001)
        {
            return a / 3.0;
        }

        double sinh_x = std::sinh (x);
        double csch_x = 1.0 / sinh_x;

        return a * (csch_x * csch_x - 1.0 / (x * x));
    }

    //==============================================================================
    /// @brief Bypass flag indicating whether processing is disabled.
    bool isBypassed { false };

    /// @brief Current sample rate (Hz).
    double sampleRate { 44100.0 };

    /// @brief Time step for integration (1.0 / sampleRate).
    double dt { 1.0 / 44100.0 };

    /// @brief Current magnetization state [-1.0, 1.0].
    double m_M { 0.0 };

    /// @brief Previous magnetic field value (for derivative calculation).
    double m_H_prev { 0.0 };

    /// @brief Drive amount [0.0, 1.0] - input gain into nonlinearity.
    double drive { 0.0 };

    /// @brief Saturation amount [0.0, 1.0] - wet/dry mix and intensity.
    double saturation { 0.5 };

    /// @brief Hysteresis loop width [0.0, 1.0] - coercivity parameter.
    double hysteresisWidth { 0.5 };

    /// @brief Asymmetry [-1.0, 1.0] - DC bias for even harmonics.
    double asymmetry { 0.0 };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Saturator)
};

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam::dsp::Hysteresis
