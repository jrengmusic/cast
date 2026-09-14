/**
 * @file jam_HalfbandPolyphase.h
 * @brief Halfband polyphase FIR filter for efficient 2x oversampling
 *        interpolation/decimation.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/

/**
 * @struct HalfbandPolyphase
 * @brief Halfband polyphase FIR filter for efficient 2× oversampling.
 *
 * This template struct implements a halfband polyphase FIR filter specifically designed
 * for 2× oversampling applications. It provides efficient 2× interpolation and decimation
 * using polyphase decomposition, which reduces computational complexity by ~50% compared
 * to standard FIR filters.
 *
 * Halfband filters have the property that every other tap (except the center tap) is zero,
 * which allows for efficient polyphase implementation. The filter is defined by:
 *
 *     H(z) = 0.5 + 0.5 z^(-N/2)
 *
 * This filter is particularly useful in oversampling applications where computational
 * efficiency is critical.
 *
 * The filter follows the DSP contract and is trivially copyable for lock-free
 * state management in real-time audio applications.
 *
 * @tparam TapCount Number of FIR filter taps (must be odd, default: 63)
 *
 * @note This filter is optimized for halfband applications (cutoff at fs/4).
 * @note The tap count must be odd and at least 7 for proper halfband characteristics.
 */
template <int NumTaps = 63>
struct HalfbandPolyphase
{
    //==============================================================================
    // Compile-time invariants
    //==============================================================================
    /** @brief Ensures tap count is odd (required for halfband filters) */
    static_assert (NumTaps % 2 == 1, "Halfband filter must have odd tap count");

    /** @brief Ensures minimum tap count for proper filtering */
    static_assert (NumTaps >= 7, "Halfband filter too short");

    /** @brief Center index of the FIR filter */
    static constexpr int centerIndex = NumTaps / 2;

    /** @brief Size of the circular delay line (next power of 2 for efficient masking) */
    static constexpr int delayLineSize = Math::nextPowerOf2 (NumTaps);

    /** @brief Bitmask for circular delay line indexing */
    static constexpr int delayMask = delayLineSize - 1;

    /** @brief Latency introduced by the filter in samples ((NumTaps - 1) / 2) */
    static constexpr int latency = (NumTaps - 1) / 2;

    /** @brief Mathematical constant π (pi) */
    static constexpr double PI { Math::pi<double> };

    //==============================================================================
    // Coefficient storage (external ownership)
    //==============================================================================
    /**
     * @struct Coefficients
     * @brief Container for halfband polyphase filter coefficients.
     *
     * This struct stores the full FIR coefficients and the split polyphase coefficients
     * for efficient processing. The coefficients are designed to be shared across
     * multiple filter instances for memory efficiency.
     */
    struct Coefficients
    {
        /** @brief Full FIR filter coefficients (for reference) */
        std::array<double, NumTaps> taps;

        /** @brief Polyphase coefficients for even-indexed taps (includes center tap) */
        std::array<double, (NumTaps + 1) / 2> phaseEven;

        /** @brief Polyphase coefficients for odd-indexed taps */
        std::array<double, (NumTaps - 1) / 2> phaseOdd;
    };

    /** @brief Pointer to the filter coefficients (externally owned) */
    const Coefficients* coefficients { nullptr };

    //==============================================================================
    // Stateless design - all state is externally managed
    // No internal delay lines or write indices
    //==============================================================================

    //==============================================================================
    // -------------------- 2× INTERPOLATION --------------------
    //==============================================================================
    /**
     * @brief Performs 2× interpolation on an input sample.
     *
     * This method takes a single input sample and produces two output samples
     * (even and odd) at 2× the input sample rate using polyphase interpolation.
     * The output samples are multiplied by 2.0 for proper gain compensation.
     *
     * The polyphase interpolation uses two separate filter branches:
     * - Even branch: processes even-indexed taps
     * - Odd branch: processes odd-indexed taps
     *
     * @tparam SampleType The sample type (float, double, etc.)
     * @param delayBuffer External circular delay line buffer (must be at least delayLineSize)
     * @param writeHead Current write position in the circular buffer (will be updated)
     * @param inputSample The input sample at the original sample rate
     * @param outputEven The even-indexed output sample (first sample in 2× domain)
     * @param outputOdd The odd-indexed output sample (second sample in 2× domain)
     *
     * @note This method is real-time safe and designed for audio callback threads.
     * @note The method uses separate polyphase branches for even and odd outputs.
     * @note The delay buffer and write head are externally managed for better cache locality.
     */
    template <typename SampleType>
    void interpolate (double* delayBuffer,
                      int& writeHead,
                      SampleType inputSample,
                      SampleType& outputEven,
                      SampleType& outputOdd)
    {
        delayBuffer[writeHead] = static_cast<double> (inputSample);

        // -------------------------
        // Phase 0 (even output): even-indexed taps
        // Multiply by 2.0 for interpolation gain compensation
        // (Coefficients sum to 1.0, but each independent output needs unity gain)
        // -------------------------
        {
            double accumulator { 0.0 };
            int readIndex { writeHead };

            for (int i = 0; i < coefficients->phaseEven.size(); ++i)
            {
                accumulator += delayBuffer[readIndex] * coefficients->phaseEven.at (i);
                readIndex = (readIndex - 1) & delayMask;
            }

            outputEven = static_cast<SampleType> (accumulator * 2.0);
        }

        // -------------------------
        // Phase 1 (odd output): odd-indexed taps (center tap + zeros)
        // Multiply by 2.0 for interpolation gain compensation
        // -------------------------
        {
            double accumulator { 0.0 };
            int readIndex { writeHead };

            for (int i = 0; i < coefficients->phaseOdd.size(); ++i)
            {
                accumulator += delayBuffer[readIndex] * coefficients->phaseOdd.at (i);
                readIndex = (readIndex - 1) & delayMask;
            }

            outputOdd = static_cast<SampleType> (accumulator * 2.0);
        }

        writeHead = (writeHead + 1) & delayMask;
    }

    //==============================================================================
    // -------------------- 2× DECIMATION --------------------
    //==============================================================================
    /**
     * @brief Performs 2× to 1× decimation on a pair of samples.
     *
     * This method takes two consecutive samples from the 2× oversampled domain
     * and produces one output sample at the original sample rate using polyphase
     * decimation. It filters both even and odd branches separately and sums the results.
     *
     * The polyphase decimation process:
     * 1. Store even and odd samples in time order
     * 2. Filter even branch with even-indexed coefficients
     * 3. Filter odd branch with odd-indexed coefficients
     * 4. Sum the results
     *
     * @tparam SampleType The sample type (float, double, etc.)
     * @param delayBuffer External circular delay line buffer (must be at least delayLineSize)
     * @param writeHead Current write position in the circular buffer (will be updated)
     * @param inputEven The even-indexed sample from the 2× domain
     * @param inputOdd The odd-indexed sample from the 2× domain
     * @return The decimated output sample at the original sample rate
     *
     * @note This method is real-time safe and designed for audio callback threads.
     * @note The method maintains proper time ordering and uses separate delay lines for interpolation/decimation.
     * @note The delay buffer and write head are externally managed for better cache locality.
     */
    template <typename SampleType>
    SampleType decimate (double* delayBuffer,
                         int& writeHead,
                         SampleType inputEven,
                         SampleType inputOdd)
    {
        // -------------------------------------------------
        // Polyphase decimation: filter both even and odd branches, then sum
        // -------------------------------------------------

        // Store in time order: even first, then odd
        // write both samples
        delayBuffer[writeHead] = static_cast<double> (inputEven);
        int evenIndex = writeHead;
        writeHead = (writeHead + 1) & delayMask;

        delayBuffer[writeHead] = static_cast<double> (inputOdd);
        int oddIndex = writeHead;
        writeHead = (writeHead + 1) & delayMask;

        // ---- even phase
        double evenAccumulator { 0.0 };
        int read { evenIndex };
        for (int i = 0; i < coefficients->phaseEven.size(); ++i)
        {
            evenAccumulator += delayBuffer[read] * coefficients->phaseEven.at (i);
            read = (read - 2) & delayMask;
        }

        // ---- odd phase
        double oddAccumulator { 0.0 };
        read = (oddIndex - 2) & delayMask;
        for (int i = 0; i < coefficients->phaseOdd.size(); ++i)
        {
            oddAccumulator += delayBuffer[read] * coefficients->phaseOdd.at (i);
            read = (read - 2) & delayMask;
        }

        return static_cast<SampleType> (evenAccumulator + oddAccumulator);
    }

    //------------------------------------------------------------------------------
    /**
     * @brief Generates halfband polyphase FIR filter coefficients.
     *
     * This static method generates halfband FIR filter coefficients using a Kaiser-windowed
     * sinc function, then splits them into polyphase branches for efficient processing.
     *
     * The halfband filter has these key properties:
     * - Every other tap is zero (except center tap)
     * - Cutoff frequency is fs/4
     * - Provides 2× oversampling with minimal computation
     *
     * @param quality The quality setting for stopband attenuation
     * @return Generated coefficients structure with full and polyphase coefficients
     *
     * @note This method is computationally expensive and should only be called from UI threads.
     * @note The generated coefficients have unity gain and enforce halfband constraints.
     * @note Interpolation applies 2× gain compensation, while decimation uses coefficients as-is.
     */
    //------------------------------------------------------------------------------
    static Coefficients generateCoefficients (FIR::Quality quality)
    {
        Coefficients coefficients;

        constexpr int center { NumTaps / 2 };
        const double beta { FIR::kaiserBetaForQuality (quality) };
        const double i0Beta { FIR::besselI0 (beta) };

        for (int n = 0; n < NumTaps; ++n)
        {
            const int k { n - center };

            double ideal;
            if (k == 0)
                ideal = 0.5;
            else
                ideal = std::sin (0.5 * PI * k) / (PI * k);

            const double ratio { static_cast<double> (k) / center };
            const double window {
                FIR::besselI0 (beta * std::sqrt (1.0 - ratio * ratio)) / i0Beta
            };

            coefficients.taps.at (n) = ideal * window;
        }

        // Enforce halfband constraints BEFORE normalization
        for (int i = 0; i < NumTaps; ++i)
        {
            if ((i & 1) && i != center)
                coefficients.taps.at (i) = 0.0;
        }

        coefficients.taps.at (center) = 0.5;

        // Normalize to unity gain (sum = 1.0)
        // Interpolation will apply 2× gain for proper upsampling
        // Decimation will use coefficients as-is
        double sum { 0.0 };
        for (int i = 0; i < NumTaps; ++i)
            sum += coefficients.taps.at (i);

        for (int i = 0; i < NumTaps; ++i)
            coefficients.taps.at (i) /= sum;

        int evenIndex { 0 };
        int oddIndex { 0 };

        //------------------------------------------------------------------------------
        // Split full FIR into polyphase tables
        //------------------------------------------------------------------------------
        for (int i = 0; i < NumTaps; ++i)
        {
            if ((i & 1) == 0)
                coefficients.phaseEven.at (evenIndex++) = coefficients.taps.at (i);
            else
                coefficients.phaseOdd.at (oddIndex++) = coefficients.taps.at (i);
        }

        return coefficients;
    }
};

//==============================================================================
// DSP Contract
//==============================================================================
static_assert (std::is_trivially_copyable_v<HalfbandPolyphase<63>>,
               "HalfbandPolyphase must be trivially copyable");

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::dsp
