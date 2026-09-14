/**
 * @file jam_Preisach.h
 * @brief High-accuracy hysteresis processor using the classical Preisach relay-array model.
 */

namespace jam::dsp::Hysteresis
{
/*____________________________________________________________________________*/
/**
 * @class Preisach
 * @brief High-accuracy hysteresis processor using the Preisach model.
 *
 * This class implements the classical Preisach model for magnetic hysteresis,
 * which is considered one of the most accurate phenomenological models for
 * capturing real transformer and magnetic tape behavior. The model represents
 * the magnetic material as a collection of independent relay operators (elementary
 * hysteresis operators) with distributed switching thresholds.
 *
 * @details
 * The Preisach model features:
 * - **Array of relay operators** each with upper (alpha) and lower (beta) thresholds
 * - **Distributed threshold distribution** across the relay array
 * - **State memory** where each relay maintains its on/off state
 * - **Weighted superposition** of all relay outputs for final magnetization
 *
 * The model creates true hysteresis behavior where the output depends on the
 * entire history of the input signal, not just recent samples. Each relay switches
 * on when input exceeds its upper threshold and switches off when input falls
 * below its lower threshold, maintaining state between these values.
 *
 * @note
 * The accuracy and computational cost scale with the number of relays. Default
 * of 16 relays provides good balance between quality and efficiency. For higher
 * accuracy (e.g., offline processing), use 32-64 relays. The model should be
 * reset when playback starts to ensure consistent initial state.
 *
 * @author PT Infinites Audio
 * @version 0.0.1
 * @date 2025-10-20
 */
class Preisach
{
public:
    /**
     * @brief Constructor initializes relay operators with distributed thresholds.
     *
     * @param numRelays Number of relay operators to use (default: 16).
     *                  Higher values provide more accuracy but cost more CPU.
     *                  Typical range: 8-64 relays.
     */
    Preisach (int numRelays = 16) : m_numRelays (numRelays)
    {
        // Initialize relay operators with distributed thresholds
        m_relays.resize (m_numRelays);

        for (int i = 0; i < m_numRelays; ++i)
        {
            double alpha = -1.0 + 2.0 * i / (m_numRelays - 1);
            double beta = alpha * 0.9; // Hysteresis width

            m_relays[i] = { alpha, beta, false };
        }

        reset();
    }

    /**
     * @brief Destructor for cleanup.
     */
    ~Preisach() {}

    /**
     * @brief Resets all relay operator states.
     *
     * Call this method when starting playback or when discontinuities
     * in the audio stream occur to ensure consistent behavior.
     */
    void reset()
    {
        for (auto& relay : m_relays)
        {
            relay.state = false;
        }
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
     */
    void calc()
    {
        // Currently no sample-rate dependent calculations
        // Reserved for future enhancements
    }

    /**
     * @brief Sets the drive amount (input gain into the nonlinearity).
     *
     * @param newValue Drive amount in range [0.0, 1.0] where 0 is no drive
     *                 and 1.0 is maximum drive (maps to ~7x gain internally).
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
     * @brief Sets the hysteresis loop width (threshold scaling).
     *
     * @param newValue Hysteresis width in range [0.0, 1.0] where 0 is minimal
     *                 hysteresis (narrow loop) and 1.0 is maximum hysteresis
     *                 (wide loop with more pronounced memory effect).
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
     * @brief Processes a single audio sample through Preisach hysteresis model.
     *
     * Updates all relay operators based on input signal and computes weighted
     * sum of relay states to produce hysteresis output. Each relay maintains
     * its state between threshold crossings, creating true memory effect.
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
        double driven = input * (1.0 + drive * 6.0);

        // 2. Add asymmetry
        double H = driven + asymmetry * 0.4;

        // 3. Scale thresholds by hysteresis width
        double widthScale = 0.1 + hysteresisWidth * 2.0;

        // 4. Update all relay operators
        double output = 0.0;
        for (auto& relay : m_relays)
        {
            double alpha = relay.alpha * widthScale;
            double beta = relay.beta * widthScale;

            // Update relay state based on input
            if (H > alpha)
            {
                relay.state = true;
            }
            else if (H < beta)
            {
                relay.state = false;
            }
            // Else: maintain previous state (hysteresis!)

            // Accumulate output
            output += relay.state ? 1.0 : -1.0;
        }

        // 5. Normalize output
        output /= static_cast<double> (m_numRelays);

        // 6. Apply saturation curve
        output = std::tanh (output * (1.0 + saturation));

        // 7. Blend wet/dry
        sample = static_cast<SampleType> (input * (1.0 - saturation) + output * saturation);
    }

private:
    /**
     * @struct RelayOperator
     * @brief Elementary hysteresis operator with upper and lower thresholds.
     *
     * Each relay represents a bistable element that switches on when input
     * exceeds alpha (upper threshold) and switches off when input falls below
     * beta (lower threshold). The state is maintained between these values.
     */
    struct RelayOperator
    {
        double alpha;  ///< Upper switching threshold
        double beta;   ///< Lower switching threshold
        bool state;    ///< Current relay state (true = on, false = off)
    };

    //==============================================================================
    /// @brief Bypass flag indicating whether processing is disabled.
    bool isBypassed { false };

    /// @brief Current sample rate (Hz).
    double sampleRate { 44100.0 };

    /// @brief Array of relay operators forming the Preisach model.
    std::vector<RelayOperator> m_relays;

    /// @brief Number of relay operators in the model.
    int m_numRelays { 16 };

    /// @brief Drive amount [0.0, 1.0] - input gain into nonlinearity.
    double drive { 0.0 };

    /// @brief Saturation amount [0.0, 1.0] - wet/dry mix and intensity.
    double saturation { 0.5 };

    /// @brief Hysteresis loop width [0.0, 1.0] - threshold scaling.
    double hysteresisWidth { 0.5 };

    /// @brief Asymmetry [-1.0, 1.0] - DC bias for even harmonics.
    double asymmetry { 0.0 };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Preisach)
};

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam::dsp::Hysteresis
