/**
 * @file jam_TransientControl.h
 * @brief Single-band transient shaper driven by dual attack/decay envelope followers.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/

/**
 * @class TransientControl
 * @brief Single-band transient shaper driven by dual attack/decay envelope followers.
 *
 * Detects the input envelope (peak or squared) via a fast follower, compares
 * it against separately-timed desired-attack and desired-decay followers,
 * and derives a dB gain correction from the difference. The result reshapes
 * the transient character (sharpening or softening attacks and sustains)
 * before applying output gain and compensation.
 */
class TransientControl
{
public:
    /**
     * @brief Default constructor for the TransientControl class.
     */
    TransientControl()=default;

    /**
     * @brief Destructor for the TransientControl class.
     */
    ~TransientControl()=default;

    /**
     * @brief Sets the sample rate for the main processor and its internal envelope followers.
     * @param newSampleRate The new sample rate in Hz.
     */
    void setSampleRate (double newSampleRate)
    {
        if (newSampleRate != sampleRate)
        {
            sampleRate = newSampleRate;
            calc();

            follower.setSampleRate (sampleRate);
            desiredAttack.setSampleRate (sampleRate);
            desiredDecay.setSampleRate (sampleRate);
        }
    }

    /**
     * @brief Sets the attack amount.
     * @param newValue The attack amount (-100 to +100), scaled internally to a gain-correction weight.
     */
    void setAttack (float newValue)
    {
        if (newValue != attack)
        {
            attack = newValue * 0.01;
            calc();
        }
    }

    /**
     * @brief Sets the attack strength, controlling the desired-attack envelope follower's speed.
     * @param newValue The attack strength (0.0 to 1.0).
     */
    void setAttackStrength (float newValue)
    {
        if (newValue != attackStrength)
        {
            attackStrength = newValue;
            calc();
        }
    }

    /**
     * @brief Sets the decay/sustain amount.
     * @param newValue The decay amount (-100 to +100), scaled internally to a gain-correction weight.
     */
    void setDecay (float newValue)
    {
        if (newValue != decay)
        {
            decay = newValue * 0.01;
            calc();
        }
    }

    /**
     * @brief Sets the decay strength, controlling the desired-decay envelope follower's release curve.
     * @param newValue The decay strength (0.0 to 1.0).
     */
    void setDecayStrength (float newValue)
    {
        if (newValue != decayStrength)
        {
            decayStrength = newValue;
            calc();
        }
    }

    /**
     * @brief Sets the output makeup gain.
     * @param newValueDecibel The output gain in dB.
     */
    void setOutput (float newValueDecibel)
    {
        if (amp (newValueDecibel) != output)
        {
            output = amp (newValueDecibel);
            calc();
        }
    }

    /**
     * @brief Sets the envelope detection mode.
     * @param newValue The detection mode (0 = peak, 1 = squared).
     */
    void setDetectionMode (float newValue)
    {
        if (static_cast<int> (newValue) != mode)
        {
            mode = static_cast<int> (newValue);
            calc();
        }
    }

    /**
     * @brief Sets the gain compensation applied alongside the derived attack/decay correction.
     * @param newValue The gain compensation in dB.
     */
    void setGainCompensation (float newValue)
    {
        if (newValue != compensate)
        {
            compensate = newValue;
            calc();
        }
    }

    //==============================================================================
    /**
     * @brief Recalculates internal envelope follower coefficients from the current attack/decay strengths.
     */
    void calc()
    {
        double beta { 0.0 };
        double alpha { 0.0 };

        beta = std::log (minAttack);
        alpha = std::log (maxAttack) - beta;
        a = std::exp (alpha * attackStrength + beta) - 1.0;

        beta = std::log (minDecay);
        alpha = std::log (maxDecay) - beta;
        d = std::exp (alpha * decayStrength + beta);

        // Max gain smoothing is 15 ms
        //        alphaGain = std::exp (-1.0 / (0.5 * 0.015 * gainSmoothing * sampleRate));

        // 20 Hz => ~50 ms period
        follower.init (followerAttack, followerRelease);
        desiredAttack.init (a, desiredAttackRelese);
        desiredDecay.init (followerAttack, d);
    }

    /**
     * @brief Processes a single sample, applying the derived transient-shaping gain correction and output makeup.
     * @param input The audio sample to process in place.
     */
    void process (float& input)
    {
        double spl { static_cast<double> (input) };

        double inputGain { 0.0 };

        switch (mode)
        {
            case peak:
                inputGain = dB (std::max (0.001, std::abs (spl)));
                break;

            case squared:
                inputGain = dB (std::max (0.001, spl * spl));
                break;
        }

        double envelopeFollower { follower.getEnvelope (inputGain) };
        double targetAttack { desiredAttack.getEnvelope (inputGain) };
        double targetDecay { desiredDecay.getEnvelope (inputGain) };

        /* Gain changes in dB space */
        double dBAttack { attack * (targetAttack - envelopeFollower) };
        double dBDecay { decay * (targetDecay - envelopeFollower) };
        double dbGainCurrent { -dBAttack + dBDecay + compensate };

        //        dBgain = alphaGain * dBgain + (1.0 - alphaGain) * dbGainCurrent;
        dBgain = dbGainCurrent;

        /* Convert to linear */
        input *= amp (dBgain) * output;
    }
    //==============================================================================
private:
    /** parameters */
    double sampleRate { 44100.0 };
    // attack range -100.0 to 100.0
    double attack { 0.0 };
    // sustain range -100.0 to 100.0
    double decay { 0.0 };
    double output { 1.0 };
    double attackStrength { 0.5 };
    double decayStrength { 1.0 };
    //    double gainSmoothing { 0.0 };

    enum
    {
        peak,
        squared,
    };

    int mode { squared };

    /** constants */
    static constexpr double followerAttack { 1.0 };
    static constexpr double followerRelease { 120.0 };
    static constexpr double desiredAttackRelese { 150.0 };
    static constexpr double minAttack { 2.0 };
    static constexpr double maxAttack { 120.0 };
    static constexpr double minDecay { 130.0 };
    static constexpr double maxDecay { 1000.0 };

    /** variables */
    double a { 0.0 };
    double d { 0.0 };
    double alphaGain { 1.0 };
    double dBgain { 0.0 };
    double compensate { 0.0 };

    //==============================================================================
    class Envelope
    {
    public:
        Envelope() {}
        ~Envelope() {}

        void setSampleRate (double newValue)
        {
            if (newValue != sampleRate)
                sampleRate = newValue;
        }

        void init (double attack, double release)
        {
            a = (attack > 0.0) ? std::exp (-1.0 / (0.5 * 0.001 * attack * sampleRate)) : 0.0;
            r = std::exp (-1.0 / (0.5 * 0.001 * release * sampleRate));
        }

        double getEnvelope (double spl)
        {
            if (spl > current)
                current = a * current + (1.0 - a) * spl;
            else
                current = r * current + (1.0 - r) * spl;

            return current;
        }

        //    private:
        double sampleRate;
        double a;
        double r;
        double current { -60.0 };
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Envelope)
    };

    Envelope follower;
    Envelope desiredAttack;
    Envelope desiredDecay;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransientControl)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam::dsp */
