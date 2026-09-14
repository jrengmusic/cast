/**
 * @file jam_Butterworth.h
 * @brief Implementation of a variable-order Butterworth filter.
 *
 * This file provides a variable-order Butterworth filter with configurable slope (up to 96dB/oct).
 * The filter is composed of cascaded biquad sections, each contributing 6dB of attenuation per octave.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/
/**
 * @brief Butterworth filter class template
 *
 * Implements a variable-order Butterworth filter with configurable slope (up to 96dB/oct).
 * The filter is composed of cascaded biquad sections, each contributing 6dB of attenuation per octave.
 *
 * @tparam FilterType The type of filter to cascade (e.g., StateVariable, RBJ)
 */
template <typename FilterType>
class Butterworth
{
public:
    /**
     * @brief Constructor for Butterworth filter
     *
     * Initializes the filter with a given type and maximum slope.
     *
     * @param newType The filter type (highPass, lowPass, etc.)
     * @param dbPerOctMaxSlope The maximum slope in dB per octave (default is 96dB/oct)
     */
    Butterworth (Filter::Type newType = Filter::Type::lowPass, int dbPerOctMaxSlope = Filter::dB96)
        : type (newType)
        , maxSlope (dbPerOctMaxSlope)
    {
        activeCount = 1 + getNthOrder (maxSlope);

        for (size_t i = 0; i < activeCount; ++i)
        {
            filters.at(i).setType (newType);
        }

        /** first iteration for odd order is one pole (6dB/oct) filter */
        filters.at(to_underlying (Filter::Pole::one)).setPole (Filter::Pole::one);
        filters.at(to_underlying (Filter::Pole::one)).setQ (2.0);
    }

    /**
     * @brief Destructor for Butterworth filter
     */
    ~Butterworth() = default;
    //==============================================================================

    /**
     * @brief Process a single audio sample
     *
     * Applies the filter cascade to a single audio sample.
     *
     * @tparam SampleType The type of the sample (e.g., float, double)
     * @param sample Reference to the audio sample to be processed
     */
    template <typename SampleType>
    void process (SampleType& sample)
    {
        if (not isBypassed)
        {
            if (slope > Filter::off)
            {
                /** process replace samples for higher slopes (above 18dB/oct)
                    sounds better (less clicky, when adjusting parameter knob)
                    when filtering done starts from latest iteration ¯\_(ツ)_/¯
               */
                if ((type == Filter::Type::lowPass) and (slope > Filter::dB18))
                {
                    for (int index = range.second; index >= range.first; --index)
                        filters.at(index).process (sample);
                }
                else
                {
                    for (int index = range.first; index <= range.second; ++index)
                        filters.at(index).process (sample);
                }
            }
        }
    }

    //==============================================================================
    /**
     * @brief Reset all filter stages
     *
     * Clears the state of all cascaded filter stages.
     */
    void reset()
    {
        for (size_t i = 0; i < activeCount; ++i)
            filters.at(i).reset();
    }

    /**
     * @brief Set the filter type
     *
     * Updates the type of all cascaded filter stages.
     *
     * @param filterType The new filter type
     */
    void setType (Filter::Type filterType)
    {
        if (filterType != type)
        {
            type = filterType;

            for (size_t i = 0; i < activeCount; ++i)
                filters.at(i).setType (type);
        }
    }

    /**
     * @brief Set the sample rate
     *
     * Updates the sample rate for all cascaded filter stages.
     *
     * @param newSampleRate The new sample rate in Hz
     */
    void setSampleRate (double newSampleRate)
    {
        if (newSampleRate != sampleRate)
        {
            sampleRate = newSampleRate;
            
            for (size_t i = 0; i < activeCount; ++i)
                filters.at(i).setSampleRate (sampleRate);
        }
    }

    /**
      * @brief Set the filter slope
      *
      * Updates the filter slope (steepness) in dB per octave.
      *
      * @param newValue The new slope value in dB per octave
      */
    void setSlope (double newValue)
    {
        if (newValue != slope)
        {
            slope = jam::toInt (newValue);
            range = getRange (slope);

            for (int index = to_underlying (Filter::Pole::two); index <= getNthOrder (slope); ++index)
                filters.at(index).setQ (getQ (slope, index));
        }
    }

    /**
     * @brief Set the cutoff frequency
     *
     * Updates the cutoff frequency for all cascaded filter stages.
     *
     * @param newValue The new cutoff frequency in Hz
     */
    void setFrequency (double newValue)
    {
        if (newValue != frequency)
        {
            frequency = newValue;

            for (size_t i = 0; i < activeCount; ++i)
                filters.at(i).setFrequency (frequency);
        }
    }

    /**
     * @brief Set the bypass state
     *
     * When bypassed, the filter will pass samples through unchanged.
     *
     * @param shouldBeBypassed True to bypass the filter, false to process normally
     */
    void setBypassed (bool shouldBeBypassed)
    {
        isBypassed = shouldBeBypassed;
    }

    /**
     * @brief Set the inverted state
     *
     * When inverted, the filter type is swapped (e.g., high-pass becomes low-pass).
     *
     * @param shouldBeInverted True to invert the filter type, false to use the original type
     */
    void setInverted (bool shouldBeInverted)
    {
        if (shouldBeInverted != isInverted)
        {
            isInverted = shouldBeInverted;

            if (isInverted)
            {
                previousType = type;
                if (type == Filter::Type::highPass)
                    setType (Filter::Type::lowPass);
                else if (type == Filter::Type::lowPass)
                    setType (Filter::Type::highPass);
            }
            else
            {
                setType (previousType);
            }
        }
    }

    /**
      * @brief Get the magnitude response of the filter
      *
      * Calculates the frequency response of the filter cascade.
      *
      * @param numPoints The number of frequency points to calculate (default is 1024)
      * @return A vector of magnitude values
      */
    Filter::Magnitudes getMagnitudes (int numPoints = 1024) const
    {
        return Filter::getCascadedMagnitudes (getCoefficients(), numPoints);
    }

    /**
     * @brief Get the coefficients of the filter cascade
     *
     * Returns the coefficients of all active filter stages.
     *
     * @return A vector of coefficient sets for each active filter stage
     */
    Filter::Coefficients getCoefficients() const
    {
        Filter::Coefficients coeffs;

        for (int index = range.first; index <= range.second; ++index)
            coeffs.emplace_back (filters.at(index).getCoefficients());

        return coeffs;
    }

    /**
     * @brief Get the current filter type
     *
     * @return The current filter type
     */
    Filter::Type getType() const noexcept { return type; }

    //==============================================================================

private:
    Filter::Type type;                                    ///< Current filter type
    Filter::Type previousType;                            ///< Previous filter type (for inversion)

    int maxSlope;                                         ///< Maximum allowed slope in dB/octave
    int slope { Filter::off };                           ///< Current slope setting
    double frequency { 1000.0 };                         ///< Cutoff frequency in Hz
    double sampleRate { 44100.0 };                       ///< Sample rate in Hz
    bool isBypassed { false };                           ///< Bypass state flag
    bool isInverted { false };                           ///< Inversion state flag

    static constexpr size_t maxFilters = 17;            ///< Maximum number of filter stages (96dB = 16 poles + 1)
    std::array<FilterType, maxFilters> filters;          ///< Array of cascaded filter stages
    size_t activeCount { 0 };                            ///< Number of active filter stages

    //==============================================================================
    /**
     * @struct IntRange
     * @brief A simple structure to hold a range of integers
     *
     * Contains a first and second value to define a range.
     */
    struct IntRange
    {
        int first;   ///< First value of the range
        int second;  ///< Second value of the range
    };
    IntRange range { 0, 0 };                             ///< Current range of active filter stages

    /**
     * @brief Check if the filter order is odd
     *
     * Determines if the given dB slope corresponds to an odd-order filter.
     *
     * @param dBSlope The slope in dB per octave
     * @return True if the order is odd, false otherwise
     */
    static bool isOrderOdd (int dBSlope)
    {
        return dBSlope % 12 == 6;
    }

    /**
     * @brief Get the order of the filter
     *
     * Calculates the order of the filter based on the dB slope.
     *
     * @param dBSlope The slope in dB per octave (must be a multiple of 6)
     * @return The filter order, or 0 if the slope is not a multiple of 6
     */
    static int getNthOrder (int dBSlope)
    {
        if (dBSlope % 6 == 0)
            return dBSlope / 6;

        /** dB slope value must be division of 6dB */
        return 0;
    }

    /**
     * @brief Calculate the range of active filter stages
     *
     * Determines which filter stages are active based on the current slope.
     *
     * @param dBSlope The slope in dB per octave
     * @return An IntRange representing the active filter stage indices
     */
    static IntRange getRange (int dBSlope)
    {
        int first { isOrderOdd (dBSlope) ? 0 : 1 };
        int div { dBSlope / 6 };
        int second { (isOrderOdd (dBSlope) ? (div - 1) : div) / 2 };

        return { first, second };
    }

    /**
     * @brief Calculate the Q factor for a filter stage
     *
     * Calculates the Q factor for a specific filter stage in the cascade.
     *
     * @param dBSlope The slope in dB per octave
     * @param index The index of the filter stage
     * @return The Q factor for the specified stage
     */
    static double getQ (int dBSlope, int index) noexcept
    {
        const auto& pi = Math::pi<double>;

        double q { 2 * std::cos (((2 * index) - 1) * pi / (2 * getNthOrder (dBSlope))) };

        if (isOrderOdd (dBSlope))
            q = 2 * std::cos (index * pi / getNthOrder (dBSlope));

        return std::abs (q);
    }

    //==============================================================================
};

// Verify DSP contract: trivially copyable (enables lock-free state snapshots)
static_assert (std::is_trivially_copyable_v<Butterworth<StateVariable>>,
               "Butterworth must be trivially copyable per DSP contract");

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::dsp
