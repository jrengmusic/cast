/**
 * @file jam_Filter.h
 * @brief Common filter types and utilities for the jam DSP framework.
 *
 * This file provides common filter types, coefficients, and utility functions
 * used in the jam DSP framework.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/

/**
 * @struct Filter
 * @brief A structure containing common filter types, coefficients, and utility functions.
 *
 * The Filter struct provides various types, structures, and utility functions
 * commonly used in digital signal processing operations.
 */
struct Filter
{
    /**
     * @enum Type
     * @brief Enumeration of filter types.
     *
     * Defines various types of digital filters that can be used in the DSP framework.
     */
    enum class Type
    {
        highPass,   ///< High-pass filter: passes frequencies above the cutoff frequency
        lowPass,    ///< Low-pass filter: passes frequencies below the cutoff frequency
        highShelf,  ///< High-shelf filter: boosts or cuts frequencies above a certain point
        lowShelf,   ///< Low-shelf filter: boosts or cuts frequencies below a certain point
        peak,       ///< Peaking filter: boosts or cuts a specific frequency band
        allPass,    ///< All-pass filter: changes phase response without affecting magnitude
        notch,      ///< Notch filter: attenuates a narrow band of frequencies
        bandPass,   ///< Band-pass filter: passes frequencies within a specific range
        bandPass2,  ///< Alternative band-pass filter implementation
    };

    /**
     * @enum Pole
     * @brief Enumeration of pole configurations for filters.
     *
     * Defines the number of poles used in filter design, affecting the steepness of the filter response.
     */
    enum class Pole
    {
        one,   ///< Single pole filter (6dB/octave rolloff)
        two    ///< Two pole filter (12dB/octave rolloff)
    };

    /**
     * @enum slope
     * @brief Enumeration of filter slopes in dB per octave.
     *
     * Defines various filter slopes that determine how quickly the filter attenuates frequencies
     * beyond the cutoff point.
     */
    enum slope
    {
        off,    ///< No filtering applied
        dB6 = 6,    ///< 6 dB per octave slope
        dB12 = 12,  ///< 12 dB per octave slope
        dB18 = 18,  ///< 18 dB per octave slope
        dB24 = 24,  ///< 24 dB per octave slope
        dB30 = 30,  ///< 30 dB per octave slope
        dB36 = 36,  ///< 36 dB per octave slope
        dB42 = 42,  ///< 42 dB per octave slope
        dB48 = 48,  ///< 48 dB per octave slope
        dB60 = 60,  ///< 60 dB per octave slope
        dB72 = 72,  ///< 72 dB per octave slope
        dB96 = 96   ///< 96 dB per octave slope
    };

    /**
     * @struct Coefficient
     * @brief Represents the coefficients for a biquad filter.
     *
     * A biquad filter has the form: H(z) = (b0 + b1*z^-1 + b2*z^-2) / (a0 + a1*z^-1 + a2*z^-2)
     * This structure stores the five coefficients (a0, a1, a2, b0, b1, b2) needed for the filter.
     */
    struct Coefficient
    {
        /**
         * @brief Default constructor for Coefficient.
         *
         * Initializes all coefficients to zero, then resets to proper initial values.
         */
        Coefficient()
        {
            reset();
        }

        double a0;   ///< Denominator coefficient for z^0 term
        double a1;   ///< Denominator coefficient for z^-1 term
        double a2;   ///< Denominator coefficient for z^-2 term
        double b0;   ///< Numerator coefficient for z^0 term
        double b1;   ///< Numerator coefficient for z^-1 term
        double b2;   ///< Numerator coefficient for z^-2 term

        /**
         * @brief Reset coefficients to initial values.
         *
         * Sets a0=1.0, a1=0.0, a2=0.0, b0=1.0, b1=0.0, b2=0.0.
         * This represents a neutral filter (unity gain).
         */
        void reset()
        {
            a0 = 1.0;
            a1 = 0.0;
            a2 = 0.0;
            b0 = 1.0;
            b1 = 0.0;
            b2 = 0.0;
        }

        /**
         * @brief Multiply all coefficients by a scalar value.
         *
         * @tparam T The scalar type (should be numeric)
         * @param scalar The scalar value to multiply coefficients by
         * @return Reference to this coefficient object
         */
        template <typename T>
        Coefficient& operator*= (T scalar)
        {
            a0 *= scalar;
            a1 *= scalar;
            a2 *= scalar;
            b0 *= scalar;
            b1 *= scalar;
            b2 *= scalar;
            return *this;
        }

        /**
         * @brief Divide all coefficients by a scalar value.
         *
         * @tparam T The scalar type (should be numeric)
         * @param scalar The scalar value to divide coefficients by
         * @return Reference to this coefficient object
         */
        template <typename T>
        Coefficient& operator/= (T scalar)
        {
            a0 /= scalar;
            a1 /= scalar;
            a2 /= scalar;
            b0 /= scalar;
            b1 /= scalar;
            b2 /= scalar;
            return *this;
        }
    };

    using Coefficients = std::vector<Coefficient>;
    using Magnitudes = std::vector<double>;
    using MagnitudeStages = std::vector<Magnitudes>;

    /**
     * @brief Calculate the frequency response magnitude of a biquad filter
     *
     * Evaluates the transfer function H(z) at evenly spaced points from 0 to π
     * (normalized frequency, where π corresponds to the Nyquist frequency).
     *
     * The magnitude is computed as |H(e^jw)| = |numerator| / |denominator| where:
     * - Numerator: b0 + b1*e^(-jw) + b2*e^(-j2w)
     * - Denominator: a0 + a1*e^(-jw) + a2*e^(-j2w)
     *
     * @param coef        The biquad filter coefficients (a0, a1, a2, b0, b1, b2)
     * @param numPointsIn Number of frequency points to evaluate (default: 1024)
     * @return Vector of magnitude values in linear scale (not dB)
     *
     * @note The frequency points are evenly distributed from 0 (DC) to π (Nyquist)
     * @see getMagnitudesDB() for dB scale output
     */
    inline static Magnitudes getMagnitudes (const Coefficient& coef,
                                            int numPointsIn = 1024)
    {
        Magnitudes magnitude (static_cast<size_t>(numPointsIn));

        // Evaluate at freq_points evenly spaced from 0 to π (normalized frequency)
        for (size_t i = 0; i < static_cast<size_t>(numPointsIn); ++i)
        {
            double w = Math::pi<double> * static_cast<double>(i) / (static_cast<double>(numPointsIn) - 1.0);

            // Evaluate H(e^jw) = H(z) at z = e^jw
            // Numerator: b0 + b1*e^(-jw) + b2*e^(-j2w)
            double num_real = coef.b0 + coef.b1 * std::cos (w) + coef.b2 * std::cos (2 * w);
            double num_imag = -coef.b1 * std::sin (w) - coef.b2 * std::sin (2 * w);

            // Denominator: a0 + a1*e^(-jw) + a2*e^(-j2w)
            double den_real = coef.a0 + coef.a1 * std::cos (w) + coef.a2 * std::cos (2 * w);
            double den_imag = -coef.a1 * std::sin (w) - coef.a2 * std::sin (2 * w);

            // |H(e^jw)| = |numerator| / |denominator|
            double num_mag = std::sqrt (num_real * num_real + num_imag * num_imag);
            double den_mag = std::sqrt (den_real * den_real + den_imag * den_imag);

            magnitude[i] = (den_mag > 1e-10) ? num_mag / den_mag : 0.0;
        }

        return magnitude;
    }

    /**
     * @brief Calculate the frequency response magnitude in decibels
     *
     * Computes the magnitude response and converts to dB scale using the formula:
     * magnitude_dB = 20 * log10(magnitude_linear)
     *
     * @param coef The biquad filter coefficients (a0, a1, a2, b0, b1, b2)
     * @param numPoints Number of frequency points to evaluate (default: 512)
     * @return Vector of magnitude values in decibels (dB)
     *
     * @note Values are clamped to a minimum of 1e-10 before log conversion to avoid log(0)
     * @see getMagnitudes() for linear scale output
     */
    inline static Magnitudes getMagnitudesDB (const Coefficient& coef,
                                              int numPoints = 1024)
    {
        auto mag = getMagnitudes (coef, numPoints);

        for (auto& m : mag)
        {
            m = 20.0 * std::log10 (std::max (m, 1e-10));// Clamp to avoid log(0)
        }

        return mag;
    }

    /**
     * @brief Calculate magnitude at specific normalized frequencies
     *
     * Evaluates the frequency response at user-specified frequency points
     * rather than evenly spaced points across the spectrum.
     *
     * @param coef The biquad filter coefficients (a0, a1, a2, b0, b1, b2)
     * @param norm_freqs Vector of normalized frequencies in range [0, 1]
     *                   where 0 = DC and 1 = Nyquist frequency (Fs/2)
     * @return Vector of magnitude values (linear scale) at the specified frequencies
     *
     * @note The output vector size matches the input norm_freqs size
     * @warning Normalized frequencies outside [0, 1] will produce undefined behavior
     *
     * @par Example:
     * @code
     * std::vector<double> freqs = {0.0, 0.1, 0.5, 1.0}; // DC, 0.1*Nyquist, 0.5*Nyquist, Nyquist
     * auto mags = getMagnitudeAtFreqs(myCoef, freqs);
     * @endcode
     */
    inline static Magnitudes getMagnitudeAtFreqs (const Coefficient& coef,
                                                  const std::vector<double>& norm_freqs)
    {
        Magnitudes magnitude (norm_freqs.size());

        for (size_t i = 0; i < norm_freqs.size(); ++i)
        {
            double w = Math::pi<double> * norm_freqs[i];

            double num_real = coef.b0 + coef.b1 * std::cos (w) + coef.b2 * std::cos (2 * w);
            double num_imag = -coef.b1 * std::sin (w) - coef.b2 * std::sin (2 * w);

            double den_real = coef.a0 + coef.a1 * std::cos (w) + coef.a2 * std::cos (2 * w);
            double den_imag = -coef.a1 * std::sin (w) - coef.a2 * std::sin (2 * w);

            double num_mag = std::sqrt (num_real * num_real + num_imag * num_imag);
            double den_mag = std::sqrt (den_real * den_real + den_imag * den_imag);

            magnitude[i] = (den_mag > 1e-10) ? num_mag / den_mag : 0.0;
        }

        return magnitude;
    }

    /**
     * @brief Multiply magnitude responses from cascaded biquad stages
     *
     * For cascaded (series) filters, the total frequency response is the product
     * of individual stage responses:
     * H_total(z) = H1(z) * H2(z) * ... * Hn(z)
     *
     * Therefore in linear scale: |H_total| = |H1| * |H2| * ... * |Hn|
     *
     * @param stages Vector of magnitude response vectors, one per filter stage
     * @return Combined magnitude response (linear scale)
     *
     * @pre All magnitude vectors in stages must have the same size
     * @warning Asserts if magnitude vectors have different sizes
     *
     * @note In dB scale, you would add instead of multiply: dB_total = dB1 + dB2 + ...
     * @see getCascadedMagnitudeDB_Optimized() for efficient dB domain computation
     *
     * @par Example:
     * @code
     * auto mag1 = getMagnitudes(stage1Coef);
     * auto mag2 = getMagnitudes(stage2Coef);
     * auto totalMag = multiplyCascadedMagnitudes({mag1, mag2});
     * @endcode
     */
    inline static Magnitudes multiplyCascadedMagnitudes (const MagnitudeStages& stages)
    {
        if (stages.empty())
            return {};

        // Verify all stages have the same size
        const size_t numPoints = stages[0].size();
        for (size_t stage = 1; stage < stages.size(); ++stage)
        {
            assert (stages[stage].size() == numPoints && "All magnitude stages must have the same number of points");
        }

        Magnitudes result = stages[0];

        for (size_t stage = 1; stage < stages.size(); ++stage)
        {
            for (size_t i = 0; i < result.size(); ++i)
            {
                result[i] *= stages[stage][i];
            }
        }

        return result;
    }

    /**
     * @brief Calculate combined magnitude response from cascaded biquad filters
     *
     * Convenience function that computes individual magnitude responses and
     * multiplies them together to get the total cascaded response.
     *
     * This is useful for higher-order filters built by cascading multiple
     * biquad sections (e.g., 4th-order Butterworth = 2 biquad stages).
     *
     * @param coeffs Vector of biquad coefficients, one per filter stage
     * @param numPoints Number of frequency points to evaluate (default: 1024)
     * @return Combined magnitude response in linear scale
     *
     * @note Returns empty vector if coeffs is empty
     * @see getCascadedMagnitudesDB() for dB output
     * @see multiplyCascadedMagnitudes() for pre-computed magnitude stages
     *
     * @par Example:
     * @code
     * // 4th-order Butterworth lowpass (2 biquad stages)
     * Coefficients butterworth = {stage1Coef, stage2Coef};
     * auto response = getCascadedMagnitudes(butterworth, 1024);
     * @endcode
     */
    inline static Magnitudes getCascadedMagnitudes (const Coefficients& coeffs,
                                                    int numPoints = 1024)
    {
        if (coeffs.empty())
            return {};

        MagnitudeStages stages;
        stages.reserve (coeffs.size());

        for (const auto& coef : coeffs)
        {
            stages.push_back (getMagnitudes (coef, numPoints));
        }

        return multiplyCascadedMagnitudes (stages);
    }

    /**
     * @brief Calculate combined magnitude response in decibels from cascaded biquads
     *
     * Computes the total magnitude response of cascaded filters and converts
     * to dB scale. This function computes in linear domain then converts to dB.
     *
     * @param coeffs Vector of biquad coefficients, one per filter stage
     * @param numPoints Number of frequency points to evaluate (default: 1024)
     * @return Combined magnitude response in decibels (dB)
     *
     * @note For better efficiency when dB output is needed, consider using
     *       getCascadedMagnitudeDB_Optimized() which operates in dB domain
     * @see getCascadedMagnitudeDB_Optimized() for optimized dB computation
     * @see getCascadedMagnitudes() for linear scale output
     *
     * @par Example:
     * @code
     * Coefficients linkwitzRiley = {lpf1, lpf2, hpf1, hpf2};
     * auto response_dB = getCascadedMagnitudesDB(linkwitzRiley);
     * @endcode
     */
    inline static Magnitudes getCascadedMagnitudesDB (const Coefficients& coeffs,
                                                      int numPoints = 1024)
    {
        auto mag = getCascadedMagnitudes (coeffs, numPoints);

        for (auto& m : mag)
        {
            m = 20.0 * std::log10 (std::max (m, 1e-10));
        }

        return mag;
    }

    /**
     * @brief Calculate combined magnitude response in dB (optimized for dB output)
     *
     * More efficient implementation for dB output that operates directly in the
     * dB domain. Since cascaded filter magnitudes multiply in linear scale, they
     * add in dB scale:
     *
     * 20*log10(|H1| * |H2|) = 20*log10(|H1|) + 20*log10(|H2|)
     *
     * This avoids unnecessary linear multiplication and is more numerically stable.
     *
     * @param coeffs Vector of biquad coefficients, one per filter stage
     * @param numPoints Number of frequency points to evaluate (default: 1024)
     * @return Combined magnitude response in decibels (dB)
     *
     * @note Preferred over getCascadedMagnitudeDB() when only dB output is needed
     * @note More numerically stable for filters with large gain variations
     *
     * @par Performance:
     * - Avoids intermediate linear multiplication
     * - Uses addition instead of multiplication (faster)
     * - Better floating-point precision for extreme values
     *
     * @par Example:
     * @code
     * Coefficients eqChain = {lowShelf, peak1, peak2, highShelf};
     * auto response_dB = getCascadedMagnitudeDB_Optimized(eqChain);
     * @endcode
     */
    inline static Magnitudes getCascadedMagnitudeDB_Optimized (const Coefficients& coeffs,
                                                               int numPoints = 1024)
    {
        if (coeffs.empty())
            return {};

        // Get first stage in dB
        Magnitudes result = getMagnitudesDB (coeffs[0], numPoints);

        // Add subsequent stages in dB domain
        for (size_t stage = 1; stage < coeffs.size(); ++stage)
        {
            auto stage_db = getMagnitudesDB (coeffs[stage], numPoints);
            for (size_t i = 0; i < result.size(); ++i)
            {
                result[i] += stage_db[i];
            }
        }

        return result;
    }
    
    /** @brief Minimum transfer-function denominator magnitude below which
     *  getMagnitude() returns 0.0 instead of dividing. */
    static constexpr double denominatorEpsilon { 1e-10 };

    /**
     * @brief Calculate the transfer function magnitude at one normalized angular frequency.
     *
     * Evaluates |H(e^jw)| at the given @p w, per the same numerator/denominator
     * derivation as getMagnitudes(). Single-point counterpart used by the
     * log-spaced overloads to avoid building an intermediate Magnitudes vector.
     *
     * @param coef The biquad filter coefficients (a0, a1, a2, b0, b1, b2).
     * @param w    Normalized angular frequency, in the range [0, pi].
     * @return Magnitude value in linear scale; 0.0 when the denominator magnitude
     *         is at or below denominatorEpsilon.
     */
    inline static double getMagnitude (const Coefficient& coef, double w) noexcept
    {
        double num_real = coef.b0 + coef.b1 * std::cos (w) + coef.b2 * std::cos (2 * w);
        double num_imag = -coef.b1 * std::sin (w) - coef.b2 * std::sin (2 * w);

        double den_real = coef.a0 + coef.a1 * std::cos (w) + coef.a2 * std::cos (2 * w);
        double den_imag = -coef.a1 * std::sin (w) - coef.a2 * std::sin (2 * w);

        double num_mag = std::sqrt (num_real * num_real + num_imag * num_imag);
        double den_mag = std::sqrt (den_real * den_real + den_imag * den_imag);

        return (den_mag > denominatorEpsilon) ? num_mag / den_mag : 0.0;
    }

    /**
     * @brief Calculate magnitude response at logarithmically-spaced frequencies
     *
     * Unlike getMagnitudes() which uses linear spacing (0 to π), this function
     * evaluates at logarithmically-spaced frequencies matching typical audio
     * visualization (e.g., 20 Hz to 20 kHz).
     *
     * @param coef The biquad filter coefficients
     * @param sampleRate The sample rate in Hz
     * @param minFreq Minimum frequency in Hz (default: 20 Hz)
     * @param maxFreq Maximum frequency in Hz (default: 20000 Hz)
     * @param numPoints Number of frequency points to evaluate (default: 1024)
     * @return Vector of magnitude values in linear scale at log-spaced frequencies
     */
    inline static Magnitudes getMagnitudesLog (const Coefficient& coef,
                                               double sampleRate,
                                               double minFreq = 20.0,
                                               double maxFreq = 20000.0,
                                               int numPoints = 1024)
    {
        Magnitudes magnitude (numPoints);
        
        const double logMin = std::log10(minFreq);
        const double logMax = std::log10(maxFreq);
        const double nyquist = sampleRate / 2.0;

        for (int i = 0; i < numPoints; ++i)
        {
            // Logarithmic frequency spacing
            double logFreq = logMin + (logMax - logMin) * i / (numPoints - 1);
            double freq = std::pow(10.0, logFreq);
            
            // Clamp to Nyquist
            freq = std::min(freq, nyquist);
            
            // Convert frequency to normalized angular frequency (0 to π)
            double w = Math::pi<double> * freq / nyquist;

            // Evaluate H(e^jw)
            double num_real = coef.b0 + coef.b1 * std::cos (w) + coef.b2 * std::cos (2 * w);
            double num_imag = -coef.b1 * std::sin (w) - coef.b2 * std::sin (2 * w);

            double den_real = coef.a0 + coef.a1 * std::cos (w) + coef.a2 * std::cos (2 * w);
            double den_imag = -coef.a1 * std::sin (w) - coef.a2 * std::sin (2 * w);

            double num_mag = std::sqrt (num_real * num_real + num_imag * num_imag);
            double den_mag = std::sqrt (den_real * den_real + den_imag * den_imag);

            magnitude[i] = (den_mag > 1e-10) ? num_mag / den_mag : 0.0;
        }

        return magnitude;
    }

    /**
     * @brief Calculate magnitude response at logarithmically-spaced frequencies into a caller-provided buffer.
     *
     * Allocation-free counterpart of the getMagnitudesLog() overload above that
     * returns a Magnitudes vector — writes each point via getMagnitude() instead.
     *
     * @param coef The biquad filter coefficients.
     * @param sampleRate The sample rate in Hz.
     * @param minFreq Minimum frequency in Hz.
     * @param maxFreq Maximum frequency in Hz.
     * @param output Destination buffer for the linear-scale magnitude values; must hold at least numPoints values.
     * @param numPoints Number of frequency points to evaluate.
     */
    inline static void getMagnitudesLog (const Coefficient& coef,
                                         double sampleRate,
                                         double minFreq,
                                         double maxFreq,
                                         double* output,
                                         int numPoints) noexcept
    {
        const double logMin = std::log10 (minFreq);
        const double logMax = std::log10 (maxFreq);
        const double nyquist = sampleRate / 2.0;

        for (int i = 0; i < numPoints; ++i)
        {
            double logFreq = logMin + (logMax - logMin) * i / (numPoints - 1);
            double freq = std::pow (10.0, logFreq);

            freq = std::min (freq, nyquist);

            double w = Math::pi<double> * freq / nyquist;

            // Raw pointer with caller-guaranteed bounds — [] is safe here
            output[i] = getMagnitude (coef, w);
        }
    }

    /**
     * @brief Calculate combined magnitude response from cascaded biquads with log spacing
     *
     * @param coeffs Vector of biquad coefficients
     * @param sampleRate The sample rate in Hz
     * @param minFreq Minimum frequency in Hz (default: 20 Hz)
     * @param maxFreq Maximum frequency in Hz (default: 20000 Hz)
     * @param numPoints Number of frequency points (default: 1024)
     * @return Combined magnitude response in linear scale at log-spaced frequencies
     */
    inline static Magnitudes getCascadedMagnitudesLog (const Coefficients& coeffs,
                                                       double sampleRate,
                                                       double minFreq = 20.0,
                                                       double maxFreq = 20000.0,
                                                       int numPoints = 1024)
    {
        if (coeffs.empty())
            return {};

        // Get first stage
        Magnitudes result = getMagnitudesLog (coeffs[0], sampleRate, minFreq, maxFreq, numPoints);

        // Multiply subsequent stages
        for (size_t stage = 1; stage < coeffs.size(); ++stage)
        {
            auto stageMag = getMagnitudesLog (coeffs[stage], sampleRate, minFreq, maxFreq, numPoints);
            for (size_t i = 0; i < result.size(); ++i)
            {
                result[i] *= stageMag[i];
            }
        }

        return result;
    }

    /**
     * @brief Calculate combined magnitude response from cascaded biquads with log spacing into a caller-provided buffer.
     *
     * Single-pass, allocation-free counterpart of the getCascadedMagnitudesLog()
     * overload above that returns a Magnitudes vector. For each frequency point,
     * combines all stages via getMagnitude() directly — no intermediate per-stage
     * Magnitudes vectors, no scratch buffer.
     *
     * @param coeffs Vector of biquad coefficients.
     * @param sampleRate The sample rate in Hz.
     * @param minFreq Minimum frequency in Hz.
     * @param maxFreq Maximum frequency in Hz.
     * @param output Destination buffer for the combined linear-scale magnitude values; must hold at least numPoints values.
     * @param numPoints Number of frequency points to evaluate.
     */
    inline static void getCascadedMagnitudesLog (const Coefficients& coeffs,
                                                  double sampleRate,
                                                  double minFreq,
                                                  double maxFreq,
                                                  double* output,
                                                  int numPoints) noexcept
    {
        if (not coeffs.empty())
        {
            const double logMin = std::log10 (minFreq);
            const double logMax = std::log10 (maxFreq);
            const double nyquist = sampleRate / 2.0;

            for (int i = 0; i < numPoints; ++i)
            {
                double logFreq = logMin + (logMax - logMin) * i / (numPoints - 1);
                double freq = std::pow (10.0, logFreq);

                freq = std::min (freq, nyquist);

                double w = Math::pi<double> * freq / nyquist;

                double combinedMagnitude { 1.0 };

                for (size_t stage = 0; stage < coeffs.size(); ++stage)
                    combinedMagnitude *= getMagnitude (coeffs.at (stage), w);

                // Raw pointer with caller-guaranteed bounds — [] is safe here
                output[i] = combinedMagnitude;
            }
        }
    }

    /**
     * @brief Calculate combined magnitude in dB with log spacing (optimized)
     *
     * @param coeffs Vector of biquad coefficients
     * @param sampleRate The sample rate in Hz
     * @param minFreq Minimum frequency in Hz (default: 20 Hz)
     * @param maxFreq Maximum frequency in Hz (default: 20000 Hz)
     * @param numPoints Number of frequency points (default: 1024)
     * @return Combined magnitude response in dB at log-spaced frequencies
     */
    inline static Magnitudes getCascadedMagnitudeDB_LogOptimized (const Coefficients& coeffs,
                                                                  double sampleRate,
                                                                  double minFreq = 20.0,
                                                                  double maxFreq = 20000.0,
                                                                  int numPoints = 1024)
    {
        if (coeffs.empty())
            return {};

        Magnitudes result (numPoints, 0.0);
        
        const double logMin = std::log10(minFreq);
        const double logMax = std::log10(maxFreq);
        const double nyquist = sampleRate / 2.0;

        // For each frequency point
        for (int i = 0; i < numPoints; ++i)
        {
            // Logarithmic frequency spacing
            double logFreq = logMin + (logMax - logMin) * i / (numPoints - 1);
            double freq = std::pow(10.0, logFreq);
            freq = std::min(freq, nyquist);
            
            // Convert to normalized angular frequency
            double w = Math::pi<double> * freq / nyquist;
            
            double totalMag = 1.0;
            
            // Multiply all stages at this frequency
            for (const auto& coef : coeffs)
            {
                double num_real = coef.b0 + coef.b1 * std::cos (w) + coef.b2 * std::cos (2 * w);
                double num_imag = -coef.b1 * std::sin (w) - coef.b2 * std::sin (2 * w);

                double den_real = coef.a0 + coef.a1 * std::cos (w) + coef.a2 * std::cos (2 * w);
                double den_imag = -coef.a1 * std::sin (w) - coef.a2 * std::sin (2 * w);

                double num_mag = std::sqrt (num_real * num_real + num_imag * num_imag);
                double den_mag = std::sqrt (den_real * den_real + den_imag * den_imag);

                double stageMag = (den_mag > 1e-10) ? num_mag / den_mag : 0.0;
                totalMag *= stageMag;
            }
            
            // Convert to dB
            result[i] = 20.0 * std::log10 (std::max (totalMag, 1e-10));
        }

        return result;
    }

    //==============================================================================
    /**
     * @brief Flush denormal values in filter delay lines.
     *
     * Prevents severe CPU degradation in feedback paths caused by denormal floating-point values.
     * Per DSP Architectural Contract Section 6: Denormal Handling.
     *
     * @tparam FloatType Type of delay line values (float or double)
     * @param z0 First delay line element (reference, modified in-place)
     * @param z1 Second delay line element (reference, modified in-place)
     *
     * @note This is a free function (not a member method) following DRY principle.
     *       All biquad filters can use this single implementation.
     *
     * @par Performance:
     * - Inline function (zero call overhead)
     * - Branchless comparison for modern CPUs
     * - Constexpr threshold (compile-time constant)
     *
     * @par Example:
     * @code
     * class MyFilter {
     *     double z[2] {};
     *     template <typename SampleType>
     *     void process(SampleType& sample) {
     *         // ... biquad processing ...
     *         Filter::flushDenormals(z[0], z[1]);  // Clear, explicit, DRY!
     *     }
     * };
     * @endcode
     */
    template <typename FloatType>
    inline static void flushDenormals (FloatType& z0, FloatType& z1) noexcept
    {
        static constexpr FloatType denormalThreshold = FloatType (1.0e-15);
        z0 = (std::abs (z0) < denormalThreshold) ? FloatType (0) : z0;
        z1 = (std::abs (z1) < denormalThreshold) ? FloatType (0) : z1;
    }

    //==============================================================================
    /**
     * @brief Convert resonance parameter to Q factor.
     *
     * Maps resonance (0.0 to 1.0) to Q factor for state variable filters.
     *
     * @param resonance Resonance value in range [0.0, 1.0]
     * @return Q factor (positive value)
     *
     * @note Free function following DRY principle - not tied to any specific filter class.
     */
    inline static double resonanceToQ (double resonance) noexcept
    {
        return 1.0 / (2.0 * (1.0 - resonance));
    }

    /**
     * @brief Convert Q factor to resonance parameter.
     *
     * Maps Q factor to resonance (0.0 to 1.0) for state variable filters.
     *
     * @param q Q factor (positive value)
     * @return Resonance value in range [0.0, 1.0]
     *
     * @note Free function following DRY principle - not tied to any specific filter class.
     */
    inline static double qToResonance (double q) noexcept
    {
        return 1.0 - (1.0 / (2.0 * q));
    }
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::dsp
