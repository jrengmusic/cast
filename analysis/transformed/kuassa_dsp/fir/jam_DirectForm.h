/**
 * @file jam_DirectForm.h
 * @brief Direct form FIR filter with Kaiser-windowed sinc coefficient generation.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/

/**
 * @struct DirectForm
 * @brief Direct form FIR filter implementation.
 *
 * This template struct implements a direct form FIR filter with configurable tap count.
 * It supports various filter types (low-pass, high-pass, band-pass, band-stop) and
 * provides both 1:1 processing and 2× to 1× decimation capabilities.
 *
 * The filter uses the windowed sinc method with Kaiser window for coefficient generation:
 *
 *     h(n) = w(n) * sinc(2πfc(n - N/2))
 *
 * where w(n) is the Kaiser window and fc is the normalized cutoff frequency.
 *
 * The filter follows the DSP contract and is trivially copyable for lock-free
 * state management in real-time audio applications.
 *
 * @tparam TapCount Number of FIR filter taps (default: 63)
 *
 * @note This filter uses a circular delay line for efficient FIR convolution.
 * @note Coefficient generation is expensive and should only be done on the UI thread.
 */
template <int TapCount = 63>
struct DirectForm
{
    //==============================================================================
    // Compile-time invariants
    //==============================================================================
    /** @brief Number of FIR filter taps */
    static constexpr int NumTaps = TapCount;

    /** @brief Center index of the FIR filter (used for symmetric windowing) */
    static constexpr int centerIndex = NumTaps / 2;

    /** @brief Size of the circular delay line (next power of 2 for efficient masking) */
    static constexpr int delayLineSize = Math::nextPowerOf2 (NumTaps);

    /** @brief Bitmask for circular delay line indexing */
    static constexpr int delayMask = delayLineSize - 1;
    
    /** @brief Latency introduced by the filter in samples ((NumTaps - 1) / 2) */
    static constexpr int latency = (NumTaps - 1) / 2;

    //==============================================================================
    // Coefficient storage (external ownership, immutable)
    //==============================================================================
    /**
     * @struct Coefficients
     * @brief Container for FIR filter coefficients.
     *
     * The coefficients are stored as an array of doubles and are designed to be
     * shared across multiple filter instances for memory efficiency.
     */
    struct Coefficients
    {
        /** @brief Array of FIR filter coefficients */
        std::array<double, NumTaps> taps;
    };

    /** @brief Pointer to the filter coefficients (externally owned) */
    const Coefficients* coefficients { nullptr };

    /** @brief Mathematical constant π (pi) */
    static constexpr double PI { Math::pi<double> };

    //==============================================================================
    // Parameters (for external coefficient generation)
    //==============================================================================
    /** @brief Current sample rate in Hz (default: 48000.0) */
    double sampleRate { 48000.0 };

    /** @brief Current cutoff frequency in Hz (default: 1000.0) */
    double cutoffHz { 1000.0 };

    /** @brief Current filter type (default: LowPass) */
    FIR::FilterType filterType { FIR::FilterType::LowPass };

    /** @brief Current quality setting (default: High) */
    FIR::Quality quality { FIR::Quality::High };

    //==============================================================================
    // Stateless design - all state is externally managed
    // No internal delay line or write index
    //==============================================================================

    /**
     * @brief Sets the sample rate for the filter.
     *
     * @param sr The new sample rate in Hz
     *
     * @note This method is real-time safe and can be called from the audio thread.
     * @note This only updates the sample rate parameter; it does not regenerate coefficients.
     */
    void setSampleRate (double sr) noexcept
    {
        if (sr > 0.0 && sr != sampleRate)
        {
            sampleRate = sr;
        }
    }

    //==============================================================================
    // Parameter setters
    // WARNING: These call generateCoefficients() internally — EXPENSIVE operation.
    // Must ONLY be called from ProcessorChain::parameterChanged() (UI thread).
    // NEVER call from audio thread.
    //
    // Enforced by requiring std::shared_ptr<Coefficients> owned by ProcessorChain.
    //==============================================================================
    /**
     * @brief Sets the cutoff frequency of the filter.
     *
     * This method updates the filter's cutoff frequency and regenerates the coefficients.
     * The frequency is automatically clamped to the valid range [20Hz, 0.499*sampleRate].
     *
     * @param coeffs Shared pointer to coefficients storage (owned by ProcessorChain)
     * @param freqHz The new cutoff frequency in Hz
     *
     * @note This method is EXPENSIVE and must ONLY be called from UI threads.
     * @note The coefficients are regenerated and the filter state is updated.
     */
    void setFrequency (std::shared_ptr<Coefficients>& coeffs, double freqHz) noexcept
    {
        const double clampedFreq = std::clamp (freqHz, 20.0, sampleRate * 0.499);

        if (clampedFreq != cutoffHz)
        {
            cutoffHz = clampedFreq;
            const double normalized = cutoffHz / sampleRate;
            *coeffs = generateCoefficients (normalized, filterType, quality);
            coefficients = coeffs.get();
        }
    }

    /**
     * @brief Sets the filter type.
     *
     * This method changes the filter type (LowPass, HighPass, BandPass, BandStop)
     * and regenerates the coefficients accordingly.
     *
     * @param coeffs Shared pointer to coefficients storage (owned by ProcessorChain)
     * @param type The new filter type
     *
     * @note This method is EXPENSIVE and must ONLY be called from UI threads.
     * @note The coefficients are regenerated and the filter state is updated.
     */
    void setFilterType (std::shared_ptr<Coefficients>& coeffs, FIR::FilterType type) noexcept
    {
        if (type != filterType)
        {
            filterType = type;
            const double normalized = cutoffHz / sampleRate;
            *coeffs = generateCoefficients (normalized, filterType, quality);
            coefficients = coeffs.get();
        }
    }

    /**
     * @brief Sets the filter quality.
     *
     * This method changes the filter quality (Default, High, Maximum) which affects
     * the Kaiser window beta parameter and thus the stopband attenuation.
     *
     * @param coeffs Shared pointer to coefficients storage (owned by ProcessorChain)
     * @param q The new quality setting
     *
     * @note This method is EXPENSIVE and must ONLY be called from UI threads.
     * @note The coefficients are regenerated and the filter state is updated.
     */
    void setQuality (std::shared_ptr<Coefficients>& coeffs, FIR::Quality q) noexcept
    {
        if (q != quality)
        {
            quality = q;
            const double normalized = cutoffHz / sampleRate;
            *coeffs = generateCoefficients (normalized, filterType, quality);
            coefficients = coeffs.get();
        }
    }

    //==============================================================================
    // Processing (1:1, no decimation)
    //==============================================================================
    /**
     * @brief Processes a single sample through the FIR filter (1:1 processing).
     *
     * This method performs FIR convolution on a single sample using the current coefficients:
     *
     *     y(n) = Σₖ₌₀^N⁻¹ h(k) * x(n - k)
     *
     * @tparam SampleType The sample type (float, double, etc.)
     * @param delayBuffer External circular delay line buffer (must be at least delayLineSize)
     * @param writeHead Current write position in the circular buffer (will be updated)
     * @param sample The input sample (modified in-place with filtered output)
     *
     * @note This method is real-time safe and designed for audio callback threads.
     * @note The method uses circular buffering for efficient delay line management.
     * @note The delay buffer and write head are externally managed for better cache locality.
     */
    template <typename SampleType>
    void process (double* delayBuffer,
                  int& writeHead,
                  SampleType& sample) noexcept
    {
        delayBuffer[writeHead] = static_cast<double> (sample);
        writeHead = (writeHead + 1) & delayMask;

        // FIR convolution
        double accumulator = 0.0;
        int readIndex = writeHead;

        for (int tapIndex = 0; tapIndex < NumTaps; ++tapIndex)
        {
            readIndex = (readIndex - 1) & delayMask;
            accumulator += delayBuffer[readIndex] * coefficients->taps.at (tapIndex);
        }

        sample = static_cast<SampleType> (accumulator);
    }

    //==============================================================================
    // 2× → 1× decimation
    //==============================================================================
    /**
     * @brief Performs 2× to 1× decimation on a pair of samples.
     *
     * This method takes two consecutive samples from the 2× oversampled domain
     * and produces one output sample at the original sample rate using FIR filtering.
     *
     * @tparam SampleType The sample type (float, double, etc.)
     * @param delayBuffer External circular delay line buffer (must be at least delayLineSize)
     * @param writeHead Current write position in the circular buffer (will be updated)
     * @param evenSample The even-indexed sample from the 2× domain
     * @param oddSample The odd-indexed sample from the 2× domain
     * @return The decimated output sample at the original sample rate
     *
     * @note This method is real-time safe and designed for audio callback threads.
     * @note The method maintains proper time ordering of samples in the circular buffer.
     * @note The delay buffer and write head are externally managed for better cache locality.
     */
    template <typename SampleType>
    SampleType decimate (double* delayBuffer,
                         int& writeHead,
                         SampleType evenSample,
                         SampleType oddSample) noexcept
    {
        // Store samples in correct time order
        double even = static_cast<double> (evenSample);
        double odd = static_cast<double> (oddSample);

        // Flush denormals from input samples
        Filter::flushDenormals (even, odd);

        delayBuffer[writeHead] = even;
        writeHead = (writeHead + 1) & delayMask;

        delayBuffer[writeHead] = odd;
        writeHead = (writeHead + 1) & delayMask;

        // FIR convolution
        double accumulator = 0.0;
        int readIndex = writeHead;

        for (int tapIndex = 0; tapIndex < NumTaps; ++tapIndex)
        {
            readIndex = (readIndex - 1) & delayMask;
            accumulator += delayBuffer[readIndex] * coefficients->taps.at (tapIndex);
        }

        return static_cast<SampleType> (accumulator);
    }
    

    /**
     * @brief Generates FIR filter coefficients using the windowed sinc method.
     *
     * This static method generates FIR filter coefficients using a Kaiser-windowed
     * sinc function. It supports various filter types through spectral transformation.
     *
     * The coefficient generation follows these steps:
     * 1. Generate windowed sinc prototype (low-pass)
     * 2. Apply spectral transformation for desired filter type
     * 3. Normalize for unity gain
     *
     * @param cutoffNormalized Normalized cutoff frequency (fc / fs, where fs is the sample rate)
     * @param filterType The type of filter to generate (LowPass, HighPass, BandPass, BandStop)
     * @param quality The quality setting for stopband attenuation
     * @return Generated coefficients structure
     *
     * @note This method is computationally expensive and should only be called from UI threads.
     * @note For bandpass/bandstop filters, the cutoffNormalized parameter is used as the center frequency.
     * @note The generated coefficients are normalized for unity gain at DC (or Nyquist for high-pass).
     */
    static Coefficients generateCoefficients (double cutoffNormalized,
                                              FIR::FilterType filterType,
                                              FIR::Quality quality) noexcept
    {
        Coefficients coefficients;

        constexpr int center { NumTaps / 2 };
        const double beta { FIR::kaiserBetaForQuality (quality) };
        const double i0Beta { FIR::besselI0 (beta) };

        // Generate windowed sinc prototype (lowpass)
        for (int n = 0; n < NumTaps; ++n)
        {
            const int k = n - center;

            double ideal;
            if (k == 0)
            {
                ideal = 2.0 * cutoffNormalized;
            }
            else
            {
                ideal = std::sin (2.0 * PI * cutoffNormalized * k) / (PI * k);
            }

            const double ratio = static_cast<double> (k) / center;
            const double window = FIR::besselI0 (beta * std::sqrt (1.0 - ratio * ratio)) / i0Beta;

            coefficients.taps.at (n) = ideal * window;
        }

        // Apply spectral transformation
        switch (filterType)
        {
            case FIR::FilterType::LowPass:
                // Already lowpass, no transformation needed
                break;

            case FIR::FilterType::HighPass:
            {
                // Spectral inversion: negate all taps, add impulse at center
                for (int i = 0; i < NumTaps; ++i)
                    coefficients.taps.at (i) = -coefficients.taps.at (i);
                coefficients.taps.at (center) += 1.0;
            }
            break;

            case FIR::FilterType::BandPass:
            {
                // Modulate by cosine (frequency shift)
                for (int n = 0; n < NumTaps; ++n)
                {
                    const int k = n - center;
                    coefficients.taps.at (n) *= 2.0 * std::cos (2.0 * PI * cutoffNormalized * k);
                }
            }
            break;

            case FIR::FilterType::BandStop:
            {
                std::array<double, NumTaps> bpTaps;
                for (int n = 0; n < NumTaps; ++n)
                {
                    const int k = n - center;
                    bpTaps.at (n) = coefficients.taps.at (n) * 2.0 * std::cos (2.0 * PI * cutoffNormalized * k);
                }

                for (int i = 0; i < NumTaps; ++i)
                    coefficients.taps.at (i) = -bpTaps.at (i);
                coefficients.taps.at (center) += 1.0;
            }
            break;
        }

        // Normalize gain
        if (filterType == FIR::FilterType::HighPass)
        {
            // Normalize for unity gain at Nyquist
            double sum = 0.0;
            for (int i = 0; i < NumTaps; ++i)
                sum += coefficients.taps.at (i) * ((i & 1) ? -1.0 : 1.0);

            for (int i = 0; i < NumTaps; ++i)
                coefficients.taps.at (i) /= sum;
        }
        else
        {
            // Normalize for unity DC gain
            double sum = 0.0;
            for (int i = 0; i < NumTaps; ++i)
                sum += coefficients.taps.at (i);

            for (int i = 0; i < NumTaps; ++i)
                coefficients.taps.at (i) /= sum;
        }

        return coefficients;
    }
};

//==============================================================================
// DSP contract
//==============================================================================
static_assert (std::is_trivially_copyable_v<DirectForm<63>>,
               "DirectForm must be trivially copyable per DSP contract");

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::dsp
