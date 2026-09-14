/**
 * @file jam_Taper.h
 * @brief Logarithmic/anti-logarithmic taper transforms and NormalisableRange builders.
 */

namespace jam
{
/*____________________________________________________________________________*/
/**
 * @brief A structure for managing taper types and associated mapping/logarithmic calculations.
 *
 * The `Taper` struct includes a variety of logarithmic tapering options, mapping functionality,
 * and utilities for value transformations with different log scales.
 */
struct Taper
{
    /**
     * @brief Enumeration of taper types and their corresponding values.
     */
    enum
    {
        linear = 0, /**< Linear taper. */
        log1 = 1, /**< Logarithmic taper with factor 1. */
        log2 = 2, /**< Logarithmic taper with factor 2. */
        log3 = 3, /**< Logarithmic taper with factor 3. */
        log4 = 4, /**< Logarithmic taper with factor 4. */
        log5 = 5, /**< Logarithmic taper with factor 5. */
        log10 = 10, /**< Logarithmic taper with factor 10. */
        log15 = 15, /**< Logarithmic taper with factor 15. */
        log20 = 20, /**< Logarithmic taper with factor 20. */
        log25 = 25, /**< Logarithmic taper with factor 25. */
        log30 = 30, /**< Logarithmic taper with factor 30. */
        log35 = 35, /**< Logarithmic taper with factor 35. */
        log40 = 40, /**< Logarithmic taper with factor 40. */
        log45 = 45, /**< Logarithmic taper with factor 45. */
        alog1 = -1, /**< Anti-logarithmic taper with factor 1. */
        alog2 = -2, /**< Anti-logarithmic taper with factor 2. */
        alog3 = -3, /**< Anti-logarithmic taper with factor 3. */
        alog4 = -4, /**< Anti-logarithmic taper with factor 4. */
        alog5 = -5, /**< Anti-logarithmic taper with factor 5. */
        alog10 = -10, /**< Anti-logarithmic taper with factor 10. */
        alog15 = -15, /**< Anti-logarithmic taper with factor 15. */
        alog20 = -20, /**< Anti-logarithmic taper with factor 20. */
        alog25 = -25, /**< Anti-logarithmic taper with factor 25. */
        alog30 = -30, /**< Anti-logarithmic taper with factor 30. */
        alog35 = -35, /**< Anti-logarithmic taper with factor 35. */
        alog40 = -40, /**< Anti-logarithmic taper with factor 40. */
        alog45 = -45, /**< Anti-logarithmic taper with factor 45. */
        skew = 1000 /**< Skew taper. */
    };

    /**
     * @brief Logarithmic factors for different taper types.
     *
     * @tparam NormaliseFloatType Type for normalization (e.g., float, double).
     */
    template <typename NormaliseFloatType>
    inline static const jam::HashMap<int, NormaliseFloatType> logFactors {
        { log1, static_cast<NormaliseFloatType> (9800) },
        { log2, static_cast<NormaliseFloatType> (2400) },
        { log3, static_cast<NormaliseFloatType> (1044.444) },
        { log4, static_cast<NormaliseFloatType> (575) },
        { log5, static_cast<NormaliseFloatType> (360) },
        { log10, static_cast<NormaliseFloatType> (80) },
        { log15, static_cast<NormaliseFloatType> (31) },
        { log20, static_cast<NormaliseFloatType> (15) },
        { log25, static_cast<NormaliseFloatType> (8) },
        { log30, static_cast<NormaliseFloatType> (4.445) },
        { log35, static_cast<NormaliseFloatType> (2.448979592) },
        { log40, static_cast<NormaliseFloatType> (1.231335626) },
        { log45, static_cast<NormaliseFloatType> (0.4938271605) },
    };

    /**
     * @brief Applies a logarithmic taper transformation.
     *
     * @tparam NormaliseFloatType Type of the input value.
     * @param value The input value to be transformed.
     * @param logTaper The logarithmic taper type (default is log10).
     * @return NormaliseFloatType Transformed value.
     * @throws std::invalid_argument If the taper is invalid.
     */
    template <typename NormaliseFloatType>
    static constexpr NormaliseFloatType log (NormaliseFloatType value, int logTaper = log10)
    {
        if (isTaperLog (logTaper))
        {
            auto factor = logFactors<NormaliseFloatType>.at (logTaper);
            return (std::pow (static_cast<NormaliseFloatType> (factor + 1), value) - static_cast<NormaliseFloatType> (1)) / factor;
        }
        throw std::invalid_argument ("Percentage value should be a multiple of 5, ranged from 5 to 45.");
    }

    /**
     * @brief Applies an anti-logarithmic taper transformation.
     *
     * @tparam NormaliseFloatType Type of the input value (e.g., float, double).
     * @param value The input value to be transformed.
     * @param logTaper The logarithmic taper type (default is log10).
     * @return NormaliseFloatType Transformed value.
     */
    template <typename NormaliseFloatType>
    static constexpr NormaliseFloatType antiLog (NormaliseFloatType value, int logTaper = log10)
    {
        return static_cast<NormaliseFloatType> (1) - log (static_cast<NormaliseFloatType> (1) - value, logTaper);
    }

    /**
     * @brief Applies an inverse logarithmic taper transformation.
     *
     * @tparam NormaliseFloatType Type of the input value (e.g., float, double).
     * @param value The input value to be transformed.
     * @param logTaper The logarithmic taper type (default is log10).
     * @return NormaliseFloatType Transformed value.
     * @throws std::invalid_argument If the taper type is invalid.
     */
    template <typename NormaliseFloatType>
    static constexpr NormaliseFloatType inverseLog (NormaliseFloatType value, int logTaper = log10)
    {
        if (isTaperLog (logTaper))
        {
            auto factor { logFactors<NormaliseFloatType>.at (logTaper) };
            auto factorPlusOne { static_cast<NormaliseFloatType> (factor + 1) };

            return std::log (factor * value + static_cast<NormaliseFloatType> (1)) / std::log (factorPlusOne);
        }

        throw std::invalid_argument ("Percentage value should be a multiple of 5, ranged from 5 to 45.");
    }

    /**
     * @brief Applies an inverse anti-logarithmic taper transformation.
     *
     * @tparam NormaliseFloatType Type of the input value (e.g., float, double).
     * @param value The input value to be transformed.
     * @param logTaper The logarithmic taper type (default is log10).
     * @return NormaliseFloatType Transformed value.
     * @throws std::invalid_argument If the taper type is invalid.
     */
    template <typename NormaliseFloatType>
    static constexpr NormaliseFloatType inverseAnti (NormaliseFloatType value, int logTaper = log10)
    {
        return static_cast<NormaliseFloatType> (1) - inverseLog (static_cast<NormaliseFloatType> (1) - value, logTaper);
    }

    /**
     * @brief Creates a normalisable range for decibel values with custom tapering.
     *
     * @tparam FloatType Type of the decibel values (e.g., float, double).
     * @param mindB Minimum decibel value of the range.
     * @param maxdB Maximum decibel value of the range.
     * @param taper Logarithmic taper type (default is log10).
     * @return juce::NormalisableRange<FloatType> A normalisable range for decibel values.
     * @details This method applies logarithmic tapering if specified, allowing non-linear scaling
     *          of the decibel range.
     */
    template <typename FloatType>
    static constexpr juce::NormalisableRange<FloatType> getDecibelNormalisableRange (FloatType mindB, FloatType maxdB, int taper)
    {
        if (isTaperLog (taper))
        {
            auto convertFrom0To1Function = [=] (FloatType mindB, FloatType maxdB, FloatType normal)
            {
                FloatType min { Decibels::toAmp (mindB) };
                FloatType max { Decibels::toAmp (maxdB) };
                FloatType gain { Value::map (log (normal, taper), min, max) };

                return Decibels::fromAmp (gain);
            };

            auto convertTo0To1Function = [=] (FloatType mindB, FloatType maxdB, FloatType dB)
            {
                FloatType gain { Decibels::toAmp (dB) };
                FloatType min { Decibels::toAmp (mindB) };
                FloatType max { Decibels::toAmp (maxdB) };

                return inverseLog (Value::normalise (gain, min, max), taper);
            };

            return juce::NormalisableRange<FloatType> (mindB, maxdB, convertFrom0To1Function, convertTo0To1Function, {});
        }

        return juce::NormalisableRange<FloatType> (mindB, maxdB);
    }

    /**
     * @brief Creates a normalisable range for general values with custom tapering.
     *
     * @tparam FloatType Type of the values (e.g., float, double).
     * @param min Minimum value of the range.
     * @param max Maximum value of the range.
     * @param taper Logarithmic taper type (default is log10).
     * @return juce::NormalisableRange<FloatType> A normalisable range for general values.
     * @details This method applies logarithmic tapering if specified, enabling non-linear scaling
     *          within the range.
     */
    template <typename FloatType>
    static constexpr juce::NormalisableRange<FloatType> getNormalisableRange (FloatType min, FloatType max, int taper)
    {
        if (isTaperLog (taper))
        {
            auto convertFrom0To1Function = [=] (double min, double max, double normal)
            {
                return Value::map (log (normal, taper), min, max);
            };

            auto convertTo0To1Function = [=] (double min, double max, double value)
            {
                return inverseLog (Value::normalise (value, min, max), taper);
            };

            return juce::NormalisableRange<FloatType> (min, max, convertFrom0To1Function, convertTo0To1Function, {});
        }

        return juce::NormalisableRange<FloatType> (min, max);
    }

    /**
     * @brief Creates a normalisable range for general values with anti-logarithmic tapering.
     *
     * @tparam FloatType Type of the values (e.g., float, double).
     * @param min Minimum value of the range.
     * @param max Maximum value of the range.
     * @param taper Anti-logarithmic taper type (negative value, e.g. alog10).
     * @return juce::NormalisableRange<FloatType> A normalisable range for general values.
     */
    template <typename FloatType>
    static constexpr juce::NormalisableRange<FloatType> getAntiLogNormalisableRange (FloatType min, FloatType max, int taper)
    {
        if (isTaperAntiLog (taper))
        {
            auto convertFrom0To1Function = [=] (double min, double max, double normal)
            {
                return Value::map (antiLog (normal, -taper), min, max);
            };

            auto convertTo0To1Function = [=] (double min, double max, double value)
            {
                return inverseAnti (Value::normalise (value, min, max), -taper);
            };

            return juce::NormalisableRange<FloatType> (min, max, convertFrom0To1Function, convertTo0To1Function, {});
        }

        return juce::NormalisableRange<FloatType> (min, max);
    }

    /**
     * @brief Creates a normalisable range for decibel values with anti-logarithmic tapering.
     *
     * @tparam FloatType Type of the decibel values (e.g., float, double).
     * @param mindB Minimum decibel value of the range.
     * @param maxdB Maximum decibel value of the range.
     * @param taper Anti-logarithmic taper type (negative value, e.g. alog10).
     * @return juce::NormalisableRange<FloatType> A normalisable range for decibel values.
     */
    template <typename FloatType>
    static constexpr juce::NormalisableRange<FloatType> getDecibelAntiLogNormalisableRange (FloatType mindB, FloatType maxdB, int taper)
    {
        if (isTaperAntiLog (taper))
        {
            auto convertFrom0To1Function = [=] (FloatType mindB, FloatType maxdB, FloatType normal)
            {
                FloatType min { Decibels::toAmp (mindB) };
                FloatType max { Decibels::toAmp (maxdB) };
                FloatType gain { Value::map (antiLog (normal, -taper), min, max) };

                return Decibels::fromAmp (gain);
            };

            auto convertTo0To1Function = [=] (FloatType mindB, FloatType maxdB, FloatType dB)
            {
                FloatType gain { Decibels::toAmp (dB) };
                FloatType min { Decibels::toAmp (mindB) };
                FloatType max { Decibels::toAmp (maxdB) };

                return inverseAnti (Value::normalise (gain, min, max), -taper);
            };

            return juce::NormalisableRange<FloatType> (mindB, maxdB, convertFrom0To1Function, convertTo0To1Function, {});
        }

        return juce::NormalisableRange<FloatType> (mindB, maxdB);
    }

    /**
     * @brief Determines whether the given taper value corresponds to a logarithmic taper.
     *
     * @param taper The taper value to check.
     * @return bool Returns true if the taper value is a valid logarithmic taper; otherwise false.
     * @note Valid taper values are:
     *       - Between 1 and 4 (inclusive).
     *       - Multiples of 5 between 5 and 45 (inclusive).
     */
    static bool isTaperLog (int taper) noexcept
    {
        return ((taper >= 1) and (taper <= 4)) or ((taper >= 5) and (taper <= 45) and (taper % 5 == 0));
    }

    /**
     * @brief Determines whether the given taper value corresponds to an anti-logarithmic taper.
     *
     * @param taper The taper value to check.
     * @return bool Returns true if the taper value is a valid anti-logarithmic taper; otherwise false.
     */
    static bool isTaperAntiLog (int taper) noexcept
    {
        return isTaperLog (-taper);
    }
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
