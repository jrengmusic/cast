/**
 * @file jam_FIR.h
 * @brief Base FIR filter utilities and common definitions — quality settings,
 *        filter types, and Kaiser-window mathematical utilities.
 */

namespace jam::dsp
{
/*____________________________________________________________________________*/

/**
 * @struct FIR
 * @brief Base FIR filter utilities and common definitions.
 *
 * This struct provides canonical definitions and utility functions used by all FIR filter
 * implementations in the jam DSP framework. It includes quality settings, filter types,
 * and mathematical utilities for FIR filter design.
 * 
 * The FIR filter design uses the windowed sinc method with Kaiser window for optimal
 * stopband attenuation. The Kaiser window is defined by:
 * 
 *     w(n) = I₀(β * √(1 - (n/N)²)) / I₀(β)
 * 
 * where I₀ is the modified Bessel function of the first kind, order 0.
 * 
 * @note This is a utility struct containing only static methods and type definitions.
 *       It is not intended to be instantiated.
 */
struct FIR
{
    /**
     * @enum Quality
     * @brief FIR filter quality settings that determine stopband attenuation.
     * 
     * These quality settings control the Kaiser window beta parameter, which affects
     * the trade-off between stopband attenuation and transition bandwidth.
     */
    enum class Quality
    {
        Default,///< ~ -90 dB stopband attenuation (β = 8.96)
        High,   ///< ~ -100 dB stopband attenuation (β = 10.06)
        Maximum ///< ~ -120 dB stopband attenuation (β = 12.27)
    };

    /**
     * @enum FilterType
     * @brief Supported FIR filter types.
     * 
     * These filter types are used by the DirectForm FIR implementation for different
     * frequency response characteristics.
     */
    enum class FilterType
    {
        LowPass,  ///< Low-pass filter (passes frequencies below cutoff)
        HighPass, ///< High-pass filter (passes frequencies above cutoff)
        BandPass, ///< Band-pass filter (passes frequencies around center frequency)
        BandStop  ///< Band-stop filter (attenuates frequencies around center frequency)
    };

    /**
     * @brief Gets the Kaiser window beta parameter for the specified quality setting.
     * 
     * The Kaiser window beta parameter controls the trade-off between main lobe width
     * and side lobe attenuation. Higher beta values provide better stopband attenuation
     * but wider transition bands.
     * 
     * @param quality The desired filter quality
     * @return The beta parameter for the Kaiser window
     * 
     * @note Higher beta values result in better stopband attenuation but wider transition bands.
     */
    static const double kaiserBetaForQuality (Quality quality)
    {
        switch (quality)
        {
            case Quality::Default:
                return 8.96;// ~ -90 dB
            case Quality::High:
                return 10.06;// ~ -100 dB
            case Quality::Maximum:
                return 12.27;// ~ -120 dB
        }

        return 8.96;
    }

    /**
     * @brief Computes the modified Bessel function of the first kind, order 0.
     * 
     * This function implements a polynomial approximation of the Bessel function I₀(x)
     * used for generating the Kaiser window coefficients:
     * 
     *     I₀(x) = Σₖ₌₀^∞ (x²/4)ᵏ / (k! k!)
     * 
     * The implementation uses a 12-term approximation for efficiency.
     * 
     * @param x The input value
     * @return The Bessel function result I₀(x)
     * 
     * @note This implementation uses a polynomial approximation for efficiency.
     */
    static const double besselI0 (double x)
    {
        double sum = 1.0;
        double y = x * x / 4.0;
        double t = y;

        for (int k = 1; k < 12; ++k)
        {
            sum += t;
            t *= y / (k * k);
        }

        return sum;
    }

  
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::dsp
