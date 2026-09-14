/**
 * @file jam_Value.h
 * @brief juce::Value component binding, parameter attachment, and value-mapping helpers.
 */

namespace jam
{
/*____________________________________________________________________________*/
struct Value
{
    //==============================================================
    // Helpers
    //==============================================================

    /**
     * @brief Check if a juce::Value is non-void.
     *
     * @param v The juce::Value to test.
     * @return true if the Value is not void, false otherwise.
     */
    static bool isNonVoid (const juce::Value& v) noexcept
    {
        return not v.getValue().isVoid();
    }

    /**
     * @brief Normalize a floating-point value within a specified range.
     *
     * @tparam FloatType The type of the floating-point values.
     * @param value The value to normalize.
     * @param startValue The start of the range.
     * @param endValue The end of the range.
     * @return The normalized value between 0 and 1.
     */
    template <typename FloatType>
    static FloatType normalise (FloatType value, FloatType startValue, FloatType endValue)
    {
        return (value - startValue) / (endValue - startValue);
    }

    /**
     * @brief Normalize an integer value within a specified range.
     *
     * @tparam FloatType The type of the floating-point values for calculation.
     * @param value The integer value to normalize.
     * @param startValue The start of the range.
     * @param endValue The end of the range.
     * @return The normalized value between 0 and 1.
     */
    template <typename FloatType>
    static FloatType normalise (int value, int startValue, int endValue)
    {
        return normalise (
            static_cast<FloatType> (value), static_cast<FloatType> (startValue), static_cast<FloatType> (endValue));
    }

    /**
     * @brief Normalize a floating-point value within an integer range.
     *
     * @tparam FloatType The type of the floating-point values.
     * @param value The floating-point value to normalize.
     * @param startValue The start of the range as an integer.
     * @param endValue The end of the range as an integer.
     * @return The normalized value between 0 and 1.
     */
    template <typename FloatType>
    static FloatType normalise (FloatType value, int startValue, int endValue)
    {
        return normalise (value, static_cast<FloatType> (startValue), static_cast<FloatType> (endValue));
    }

    /**
     * @brief Clip a value to a minimum threshold.
     *
     * @tparam Type The type of the values.
     * @param value The value to clip.
     * @param minValue The minimum threshold.
     * @return The clipped value, not less than minValue.
     */
    template <typename Type>
    static Type clipMin (Type value, Type minValue)
    {
        if (value < minValue)
            value = minValue;

        return value;
    }

    /**
     * @brief Clip a value to a maximum threshold.
     *
     * @tparam Type The type of the values.
     * @param value The value to clip.
     * @param maxValue The maximum threshold.
     * @return The clipped value, not greater than maxValue.
     */
    template <typename Type>
    static Type clipMax (Type value, Type maxValue)
    {
        if (value > maxValue)
            value = maxValue;

        return value;
    }

    /**
     * @brief Clip a value within a specified range.
     *
     * @tparam Type The type of the values.
     * @param value The value to clip.
     * @param minValue The minimum threshold.
     * @param maxValue The maximum threshold.
     * @return The clipped value, between minValue and maxValue.
     */
    template <typename Type>
    static Type clip (Type value, Type minValue, Type maxValue)
    {
        if (maxValue < minValue)
        {
            value = clipMax (clipMin (value, maxValue), minValue);
        }
        else
        {
            value = clipMax (clipMin (value, minValue), maxValue);
        }

        return value;
    }

    /** @brief Linear integer-to-integer range mapping.
     *
     *  Maps @p value from source range [startValue, endValue] to target range
     *  [targetStart, targetEnd]. No float conversion — pure integer arithmetic.
     *
     *  @param value       The value to map.
     *  @param startValue  Source range start.
     *  @param endValue    Source range end.
     *  @param targetStart Target range start.
     *  @param targetEnd   Target range end.
     *  @return Mapped value in the target range. */
    static int map (int value, int startValue, int endValue, int targetStart, int targetEnd) noexcept
    {
        const int sourceRange { endValue - startValue };
        int result { targetStart };

        if (sourceRange != 0)
        {
            result = targetStart + (value - startValue) * (targetEnd - targetStart) / sourceRange;
        }

        return result;
    }

    /**
     * @brief Map a normalized value to a specified range with optional shaping.
     *
     * @tparam FloatType The type of the floating-point values.
     * @param value The value to map.
     * @param startValue The start of the source range.
     * @param endValue The end of the source range.
     * @param targetStart The start of the target range.
     * @param targetEnd The end of the target range.
     * @param factor A scaling factor for the mapping.
     * @param clamp Whether to clip the result to the target range.
     * @return The mapped value within the target range.
     */
    template <typename FloatType>
    static FloatType map (FloatType value,
                          FloatType startValue,
                          FloatType endValue,
                          FloatType targetStart,
                          FloatType targetEnd,
                          FloatType factor = static_cast<FloatType> (1),
                          bool clamp = true)
    {
        const FloatType valueRange { startValue - endValue };
        const FloatType mapRange { targetEnd - targetStart };

        if (std::abs (valueRange) < Math::flt_epsilon<FloatType>)
        {
            return targetStart;
        }
        else
        {
            FloatType normal { normalise (value, startValue, endValue) };
            FloatType target { targetStart + pow (normal, factor) * mapRange };

            if (clamp)
                target = clip (target, targetStart, targetEnd);

            return target;
        }
    }

    /**
     * @brief Map a normalized value to a specified integer range with optional shaping.
     *
     * @tparam FloatType The type of the floating-point values for calculation.
     * @param value The value to map.
     * @param startValue The start of the source range.
     * @param endValue The end of the source range.
     * @param targetStart The start of the target integer range.
     * @param targetEnd The end of the target integer range.
     * @param factor A scaling factor for the mapping.
     * @param clamp Whether to clip the result to the target range.
     * @return The mapped value as an integer within the target range.
     */
    template <typename FloatType>
    static int map (FloatType value,
                    FloatType startValue,
                    FloatType endValue,
                    int targetStart,
                    int targetEnd,
                    FloatType factor = static_cast<FloatType> (1),
                    bool clamp = true)
    {
        return toInt (map (value,
                           startValue,
                           endValue,
                           static_cast<FloatType> (targetStart),
                           static_cast<FloatType> (targetEnd),
                           factor,
                           clamp));
    }

    /**
     * @brief Map a normalized value to a specified range with optional shaping.
     *
     * This version assumes the source range is between 0 and 1.
     *
     * @tparam FloatType The type of the floating-point values.
     * @param normalisedValue The normalized value (between 0 and 1) to map.
     * @param targetStart The start of the target range.
     * @param targetEnd The end of the target range.
     * @param factor A scaling factor for the mapping.
     * @param clamp Whether to clip the result to the target range.
     * @return The mapped value within the target range.
     */
    template <typename FloatType>
    static FloatType map (FloatType normalisedValue,
                          FloatType targetStart,
                          FloatType targetEnd,
                          FloatType factor = static_cast<FloatType> (1),
                          bool clamp = true)
    {
        return map (normalisedValue,
                    static_cast<FloatType> (0),
                    static_cast<FloatType> (1),
                    targetStart,
                    targetEnd,
                    factor,
                    clamp);
    }

    /**
     * @brief Map a normalized value to a specified integer range with optional shaping.
     *
     * This version assumes the source range is between 0 and 1.
     *
     * @tparam FloatType The type of the floating-point values for calculation.
     * @param normalisedValue The normalized value (between 0 and 1) to map.
     * @param targetStart The start of the target integer range.
     * @param targetEnd The end of the target integer range.
     * @param factor A scaling factor for the mapping.
     * @param clamp Whether to clip the result to the target range.
     * @return The mapped value as an integer within the target range.
     */
    template <typename FloatType>
    static int map (FloatType normalisedValue,
                    int targetStart,
                    int targetEnd,
                    FloatType factor = static_cast<FloatType> (1),
                    bool clamp = true)
    {
        return map (normalisedValue,
                    static_cast<FloatType> (0),
                    static_cast<FloatType> (1),
                    targetStart,
                    targetEnd,
                    factor,
                    clamp);
    }

    /**
     * @brief Convert a percentage value to a mapped value.
     *
     * This function maps the input percentage value from [0, 100] to a specified range.
     *
     * @tparam FloatType The type of the floating-point values for calculation and output.
     * @param percentageValue The percentage value (between 0 and 100) to map.
     * @param targetStart The start of the target range.
     * @param targetEnd The end of the target range.
     * @return The mapped value within the specified range.
     */
    template <typename FloatType>
    static FloatType fromPercent (FloatType percentageValue, FloatType targetStart, FloatType targetEnd)
    {
        return map (percentageValue, static_cast<FloatType> (0), static_cast<FloatType> (100), targetStart, targetEnd);
    }

    /**
     * @brief Convert a percentage value to a mapped value with default start.
     *
     * This function maps the input percentage value from [0, 100] to a specified range starting at 0.
     *
     * @tparam FloatType The type of the floating-point values for calculation and output.
     * @param percentageValue The percentage value (between 0 and 100) to map.
     * @param targetEnd The end of the target range.
     * @return The mapped value within the specified range.
     */
    template <typename FloatType>
    static FloatType fromPercent (FloatType percentageValue, FloatType targetEnd)
    {
        return map (percentageValue,
                    static_cast<FloatType> (0),
                    static_cast<FloatType> (100),
                    static_cast<FloatType> (0),
                    targetEnd);
    }

    /** @brief 8-bit fully-opaque alpha (255) — single source of truth consumed
     *  by jam blending and interpolation primitives. */
    static constexpr int opaque { 255 };

    /** @brief Rounded fixed-point linear blend between two 8-bit channel
     *  values.  Divisor is opaque + 1 (256) so the division folds to the
     *  same right-shift-by-8 every SIMD arm performs directly.
     *  @param a       Value at weight 0.
     *  @param b       Value at weight opaque.
     *  @param weight  Blend weight, [0, opaque].
     *  @return        The rounded blended value. */
    static int lerp (int a, int b, int weight) noexcept
    {
        return (a * (opaque - weight) + b * weight + (opaque + 1) / 2)
               / (opaque + 1);
    }
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
