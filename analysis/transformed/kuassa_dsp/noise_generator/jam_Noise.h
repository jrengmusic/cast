/**
 * @file jam_Noise.h
 * @brief Periodic white/pink/brown noise burst injector timed by an internal minute-cycle clock.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/
/**
 * @class Noise
 * @brief Injects a periodic burst of white, pink, or brown noise into the signal, timed by an internal minute-cycle clock.
 *
 * A juce::Timer ticks every 10ms, advancing an internal millisecond counter
 * that wraps every minute. Whenever that counter falls within the configured
 * noise duration window, process() writes generated noise samples in place
 * of the input signal at the configured gain.
 */
class Noise : public juce::Timer
{
public:
    /** @brief Noise colour selecting which internal generator produces samples. */
    enum class Type
    {
        white,
        pink,
        brown
    };

    //==============================================================================
    /**
     * @brief Constructs a Noise generator of the given type, seeding the noise burst timing at a random offset.
     * @param newType The initial noise type to generate.
     */
    Noise (Type newType = Type::white);

    /**
     * @brief Destructor for the Noise class.
     */
    ~Noise() override;

    //==============================================================================
    /**
     * @brief Starts or stops the internal noise-burst timer.
     * @param shouldBeBypassed True to start the timer, false to stop it.
     */
    void setNoiseEnabled (bool shouldBeBypassed);

    /**
     * @brief Sets the duration of each noise burst.
     * @param newValueMiliseconds The burst duration in milliseconds.
     */
    void setNoiseDuration (int newValueMiliseconds);

    /**
     * @brief Advances the internal millisecond clock and updates whether a noise burst is currently active.
     */
    void timerCallback() override;

    /**
     * @brief Replaces sample with a generated noise value while a noise burst is active.
     * @tparam SampleType The sample's numeric type.
     * @param sample The sample to process, replaced with noise when a burst is active.
     */
    template <typename SampleType>
    void process (SampleType& sample);

    /**
     * @brief Replaces sample with a generated noise value while a noise burst is active.
     * @tparam SampleType The sample's numeric type.
     * @param channel The channel index (currently unused; forwards to the single-argument overload).
     * @param sample The sample to process, replaced with noise when a burst is active.
     */
    template <typename SampleType>
    void process (int channel, SampleType& sample);
    //==============================================================================

    /**
     * @brief Switches the active noise generator to a new type.
     * @param newType The noise type to generate from this point on.
     */
    void setNoiseType (Type newType);

#if JUCE_DEBUG
    /**
     * @brief Debug-only diagnostic print of the elapsed seconds and noise-burst start events.
     */
    void log()
    {
        if (milliseconds % 1000 == 0)
        {
            ++seconds;
            seconds %= 60;
            std::cout << seconds << "/n";
        }

        if (isNoiseStarting)
            std::cout << "Start noise..." << "/n";

    }
#endif // JUCE_DEBUG

private:
    const int interval { 10 }; // ms
    const int oneMinuteInMs { 60000 }; // 1 minute = 60 seconds = 60000 ms;
    int milliseconds { 0 };
//    int noiseDuration { 4440 }; // 4.44 seconds
    int noiseDuration { 4440 }; // 4.44 seconds

    double gain { -44.4 }; // output gain
    bool isNoiseStarting { false };
#if JUCE_DEBUG
    int seconds { 0 };
#endif // JUCE_DEBUG

    //==============================================================================
    class Generator
    {
    public:
        Generator() {}
        virtual ~Generator() {}
        virtual double generate() = 0;
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Generator)
    };

    std::unique_ptr<Generator> noise;

    //==============================================================================
    /**
        NoiseGenerator forked from https://github.com/johnfmcrae/NoiseGenerator
     */
    class Pink : public Generator
    {
    public:
        // constructor, overload to initialize with 12 rows, which worked out to be a
        // good number when testing in Octave
        Pink (int numRows = 12);
        ~Pink() override {}
        // generates pink noise one sample at a time
        double generate() override;

        // Changes the number of noise generating rows
        // Note that this overrides the initialization found in the constructor
        // AS WELL AS the pinkRows vector. Therefore, it is advised that this
        // function only be called on initialization
        void setRows (int newRows);

    private:
        // random noise generator from the JUCE library
        juce::Random noiseSrc;
        // each row effectively holds an independent random number generator
        std::vector<double> pinkRows;
        // running sum for noise output
        double pinkRunSum;
        // the column index, incremented each sample
        int pinkIndex;
        // the row mask, which ensures that the index of the pinkRows vector is never exceeded
        int pinkIndexMask;
        // used to normalize the noise at the output
        double pinkNorm;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pink)
    };

    //==============================================================================
    class Brown : public Generator
    {
    public:
        // constructor
        Brown (int bL = 20000);
        ~Brown() override {}
        // input is the first sample, or seed sample
        void fillBuffer (double input);
        double generate() override;

    private:
        // random noise generator from the JUCE library
        juce::Random noiseSrc;
        // buffer vector of unormalized brown noise
        // (vector instead of queue so that we can use the .begin() and .end() functions)
        std::vector<double> nB, nBn;
        // iterator for the brown noise vectors
        std::vector<double>::iterator itB;
        // max and min values of the brown noise buffer, used in noramlization
        double maxB, minB;
        // op: raw output sample of brown noise gen, ip: input to sample buffer function
        double op;
        int bLength;
        // leaky integrator constant http://sepwww.stanford.edu/sep/prof/pvi/zp/paper_html/node2.html
        double a = 0.95;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Brown)
    };

    //==============================================================================
    class White : public Generator
    {
    public:
        White() {}
        ~White() override {}
        double generate() override;

    private:
        int range { 256 };
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (White)
    };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Noise)
};

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam::dsp
