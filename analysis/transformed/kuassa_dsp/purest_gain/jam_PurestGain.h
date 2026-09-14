/**
 * @file jam_PurestGain.h
 * @brief Smoothed, dithered gain stage with independent fader-chase and fade-chase envelopes.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/

/**
 * @class PurestGain
 * @brief Smoothed, dithered gain stage with independent fader-chase and fade-chase envelopes.
 *
 * Applies gain in linear domain by chasing two independent targets — the
 * user-set gain (in dB, converted to linear) and a straight-multiply fade
 * factor — each with its own adaptive chase speed that accelerates on fresh
 * fader movement and decays back toward a floor. Float-precision processing
 * additionally applies PRNG-seeded dither on the way back to float.
 */
class PurestGain
{
public:
    /**
     * @brief Constructs a PurestGain stage, seeding the dither PRNG.
     */
    PurestGain()
    {
        getDitherSeed (fpd);
    }

    //==============================================================================
    /**
     * @brief Processes a single float sample: applies gain in double precision, then dithers back to float.
     * @param sample The audio sample to process in place.
     */
    void process (float& sample)
    {
        double spl { static_cast<double> (sample) };

        process (spl);

        sample = ditherToFloat (spl, fpd);
    }

    /**
     * @brief Processes a single double sample by chasing the gain and fade targets and applying their product.
     * @param sample The audio sample to process in place.
     */
    void process (double& sample)
    {
        if (not isBypassed)
        {
            double overallscale { 1.0 };
            overallscale /= 44100.0;
            overallscale *= sampleRate;

            if (settingchase != gain)
            {
                chasespeed *= 2.0;
                settingchase = gain;
                //increment the slowness for each fader movement
                //continuous alteration makes it react smoother
                //sudden jump to setting, not so much
            }

            if (chasespeed > 2500.0)
                chasespeed = 2500.0;
            //bail out if it's too extreme

            if (gainchase < -60.0)
            {
                gainchase = amp (gain);
                //shouldn't even be a negative number
                //this is about starting at whatever's set, when
                //plugin is instantiated.
                //Otherwise it's the target, in dB.
            }

            //done with top controller

            if (gainBchase < 0.0)
                gainBchase = fade;
            //this one is not a dB value, but straight multiplication
            //done with slow fade controller

            double targetgain { amp (settingchase) };
            //now we have the target in our temp variable

            chasespeed *= 0.9999;
            chasespeed -= 0.01;

            if (chasespeed < 350.0)
                chasespeed = 350.0;
            //we have our chase speed compensated for recent fader activity

            gainchase = (((gainchase * chasespeed) + targetgain) / (chasespeed + 1.0));
            //gainchase is chasing the target, as a simple multiply gain factor

            gainBchase = (((gainBchase * 4000) + fade) / 4001);
            //gainchase is chasing the target, as a simple multiply gain factor

            double outGain { gainchase * gainBchase };
            //directly multiply the dB gain by the straight multiply gain

            sample *= outGain;
        }
    }

    //==============================================================================
    /**
     * @brief Sets the bypass state.
     * @param shouldBeBypassed True to bypass the gain stage, false to process normally.
     */
    void setBypassed (bool shouldBeBypassed)
    {
        if (shouldBeBypassed != isBypassed)
            isBypassed = shouldBeBypassed;
    }

    /**
     * @brief Sets the target gain in decibels.
     * @param newValue The new target gain in dB.
     */
    void setGain (float newValue)
    {
        if (newValue != gain)
            gain = newValue;
    }

    /**
     * @brief Sets the sample rate used to scale the chase speed.
     * @param newSampleRate The new sample rate in Hz.
     */
    void setSampleRate (double newSampleRate)
    {
        if (newSampleRate != sampleRate)
            sampleRate = newSampleRate;
    }

    /// @brief Resets internal state variables (chase parameters, NOT dither seed).
    /// @note Dither seed (fpd) is NOT reset - it should persist for continuous PRNG sequence.
    void reset() noexcept
    {
        gainchase = -90.0;
        settingchase = -90.0;
        gainBchase = -90.0;
        chasespeed = 350.0;
        // fpd is intentionally NOT reset - it's a PRNG seed, not signal state
    }

    //==============================================================================

private:
    double gain { 0.0 };
    double fade { 1.0 };
    double gainchase { -90.0 };
    double settingchase { -90.0 };
    double gainBchase { -90.0 };
    double chasespeed { 350.0 };
    double sampleRate { 44100.0 };
    bool isBypassed { false };
    uint32_t fpd;  // Dither PRNG seed (XORshift state)
};

// Verify DSP contract: trivially copyable (enables lock-free state snapshots)
static_assert (std::is_trivially_copyable_v<PurestGain>,
               "PurestGain must be trivially copyable per DSP contract");

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam::dsp
