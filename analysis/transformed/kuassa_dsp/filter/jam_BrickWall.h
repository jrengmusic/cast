/**
 * @file jam_BrickWall.h
 * @brief Implementation of a brick-wall FIR filter using windowed sinc function.
 *
 * This file provides a linear-phase FIR filter with configurable length and cutoff.
 * Uses circular buffer for efficient real-time processing.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/

/**
 * @brief Brick-wall FIR filter using windowed sinc function
 *
 * Implements linear-phase FIR filter with configurable length and cutoff.
 * Uses circular buffer for efficient real-time processing.
 */
class BrickWall
{
public:
    /**
     * @brief Constructor for BrickWall filter
     *
     * Initializes the filter with a given type and length.
     *
     * @param newType The filter type (highPass, lowPass, etc.)
     * @param newLength The length of the filter (must be odd)
     */
    BrickWall (Filter::Type newType = Filter::Type::lowPass, int newLength = 51)
        : type (newType)
        , filterLength (newLength)
    {
        jassert (filterLength > 0 && filterLength % 2 == 1); // Must be odd
        coefficients.resize (filterLength);
        calculateCoefficients();
    }

    /**
     * @brief Destructor for BrickWall filter
     */
    ~BrickWall() = default;

    //==============================================================================
    /**
     * @brief Process a single audio sample
     *
     * Applies the filter to a single audio sample using convolution with the
     * filter coefficients stored in the circular buffer.
     *
     * @tparam SampleType The type of the sample (e.g., float, double)
     * @param sample Reference to the audio sample to be processed
     */
    template <typename SampleType>
    void process (SampleType& sample)
    {
        if (isBypassed)
            return;

        // Write input to circular buffer
        delayBuffer[writePosition] = static_cast<double> (sample);

        // Convolve with filter coefficients
        double output = 0.0;
        int readPos = writePosition;

        for (int i = 0; i < filterLength; ++i)
        {
            output += delayBuffer[readPos] * coefficients[i];

            // Circular buffer wrap
            if (--readPos < 0)
                readPos = filterLength - 1;
        }

        // Advance write position (circular)
        if (++writePosition >= filterLength)
            writePosition = 0;

        sample = static_cast<SampleType> (output);
    }

    //==============================================================================
    /**
     * @brief Reset the filter state
     *
     * Clears the delay buffer and resets the write position to zero,
     * effectively resetting the filter to its initial state.
     */
    void reset()
    {
        std::fill (delayBuffer.begin(), delayBuffer.end(), 0.0);
        writePosition = 0;
    }

    /**
     * @brief Set the filter type
     *
     * Updates the filter type (e.g., low-pass, high-pass) and recalculates
     * the filter coefficients if the type has changed.
     *
     * @param newType The new filter type
     */
    void setType (Filter::Type newType)
    {
        if (newType != type)
        {
            type = newType;
            calculateCoefficients();
        }
    }

    /**
     * @brief Set the sample rate
     *
     * Updates the filter's sample rate and recalculates the filter coefficients
     * if the sample rate has changed.
     *
     * @param newSampleRate The new sample rate in Hz
     */
    void setSampleRate (double newSampleRate)
    {
        if (newSampleRate != sampleRate)
        {
            sampleRate = newSampleRate;
            calculateCoefficients();
        }
    }

    /**
     * @brief Set the cutoff frequency
     *
     * Updates the filter's cutoff frequency and recalculates the filter coefficients
     * if the frequency has changed.
     *
     * @param newFrequency The new cutoff frequency in Hz
     */
    void setFrequency (double newFrequency)
    {
        if (newFrequency != cutoffFrequency)
        {
            cutoffFrequency = newFrequency;
            calculateCoefficients();
        }
    }

    /**
     * @brief Set the filter length
     *
     * Updates the length of the filter and recalculates the filter coefficients
     * if the length has changed. The length must be odd.
     *
     * @param newLength The new filter length (must be odd)
     */
    void setFilterLength (int newLength)
    {
        jassert (newLength > 0 && newLength % 2 == 1);

        if (newLength != filterLength)
        {
            filterLength = newLength;
            coefficients.resize (filterLength);
            delayBuffer.resize (filterLength, 0.0);
            writePosition = 0;
            calculateCoefficients();
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
     * @brief Get the filter coefficients
     *
     * Returns a copy of the current filter coefficients vector.
     *
     * @return A vector containing the filter coefficients
     */
    std::vector<double> getCoefficients() const
    {
        return coefficients;
    }

private:
    //==============================================================================
    /**
     * @brief Calculate filter coefficients using windowed sinc function
     *
     * Generates the filter coefficients by creating a windowed sinc function
     * based on the current filter parameters (type, length, frequency, sample rate).
     * The coefficients are generated using a Blackman window to reduce sidelobes.
     */
    void calculateCoefficients()
    {
        const auto& pi = Math::pi<double>;
        const double nyquist = sampleRate * 0.5;
        const double normalizedCutoff = cutoffFrequency / nyquist;
        const int center = filterLength / 2;

        // Generate windowed sinc function
        for (int n = 0; n < filterLength; ++n)
        {
            const int offset = n - center;

            // Sinc function
            double sinc = (offset == 0) ? 1.0
                                        : std::sin (pi * normalizedCutoff * offset)
                                          / (pi * normalizedCutoff * offset);

            // Blackman window
            double window = 0.42
                          - 0.5 * std::cos (2.0 * pi * n / (filterLength - 1))
                          + 0.08 * std::cos (4.0 * pi * n / (filterLength - 1));

            coefficients[n] = sinc * window;
        }

        // Normalize to unity gain
        double sum = 0.0;
        for (auto coeff : coefficients)
            sum += coeff;

        if (sum != 0.0)
        {
            for (auto& coeff : coefficients)
                coeff /= sum;
        }

        // Initialize delay buffer on first calculation
        if (delayBuffer.empty())
            delayBuffer.resize (filterLength, 0.0);
    }

    //==============================================================================
    Filter::Type type { Filter::Type::lowPass };      ///< The filter type (lowPass, highPass, etc.)
    int filterLength { 51 };                          ///< Length of the filter (must be odd)
    double cutoffFrequency { 1000.0 };                ///< Cutoff frequency in Hz
    double sampleRate { 48000.0 };                    ///< Sample rate in Hz
    bool isBypassed { false };                        ///< Bypass state flag

    std::vector<double> coefficients;                 ///< Filter coefficients
    std::vector<double> delayBuffer;                  ///< Delay buffer for the circular buffer implementation
    int writePosition { 0 };                          ///< Write position in the circular buffer
};

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam::dsp
