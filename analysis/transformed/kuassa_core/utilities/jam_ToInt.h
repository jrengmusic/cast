/**
 * @file jam_ToInt.h
 * @brief Fast floating-point-to-integer and enum/bool conversion helpers.
 */

namespace jam
{
/*____________________________________________________________________________*/

//==============================================================================
/** taken from juce::roundToInt, to reduce dependency with JUCE functions. */

#if JUCE_MSVC
#pragma optimize("t", off)
#ifndef __INTEL_COMPILER
#pragma float_control(precise, on, push)
#endif
#endif

/**
 * @brief Fast floating-point-to-integer conversion.
 *
 * This is faster than using the normal c++ cast to convert a float to an int, and
 * it will round the value to the nearest integer, rather than rounding it down
 * like the normal cast does.
 *
 * Note that this routine gets its speed at the expense of some accuracy, and when
 * rounding values whose floating point component is exactly 0.5, odd numbers and
 * even numbers will be rounded up or down differently.
 *
 * @tparam FloatType  Floating-point type of the input value.
 * @param  value      The value to round.
 * @return            @p value rounded to the nearest int.
 */
template <typename FloatType>
int toInt (const FloatType value) noexcept
{
#ifdef __INTEL_COMPILER
#pragma float_control(precise, on, push)
#endif

    auto combined { static_cast<double> (value) + 6755399441055744.0 };
    return static_cast<int> (jam::bit_cast<int64_t> (combined));
}

/**
 * @brief Fast floating-point-to-integer conversion with optional ceiling.
 *
 * When @p shouldRoundUp is false, rounds to nearest (default).
 * When @p shouldRoundUp is true, rounds toward positive infinity (ceiling).
 *
 * @tparam FloatType     Floating-point type of the input value.
 * @param  value         The value to round.
 * @param  shouldRoundUp When true, rounds toward positive infinity instead of nearest.
 * @return               The rounded int.
 */
template <typename FloatType>
int toInt (const FloatType value, bool shouldRoundUp) noexcept
{
    if (shouldRoundUp)
        return static_cast<int> (std::ceil (value));

    return toInt (value);
}

/**
 * @brief Identity overload — an int passed to toInt() is returned unchanged.
 * @param value  The value to return.
 * @return       @p value, unchanged.
 */
inline int toInt (int value) noexcept
{
    return value;
}

#if JUCE_MSVC
#ifndef __INTEL_COMPILER
#pragma float_control(pop)
#endif
#pragma optimize("", on) // resets optimisations to the project defaults
#endif

/**
 * @brief Fast floating-point-to-integer conversion.
 *
 * This is a slightly slower and slightly more accurate version of toInt(). It works
 * fine for values above zero, but negative numbers are rounded the wrong way.
 *
 * @param value  The value to round.
 * @return       @p value rounded to the nearest int.
 */
inline int toIntIntAccurate (double value) noexcept
{
#ifdef __INTEL_COMPILER
#pragma float_control(pop)
#endif

    return toInt (value + 1.5e-8);
}

/**
 * @brief Converts an enum class to its underlying type.
 *
 * This function extracts the raw integral value from an enum class instance
 * by casting it to its underlying type.
 *
 * @tparam EnumType The enum class type.
 * @param e The enum value to convert.
 * @return The underlying integral representation of the enum.
 */
template <typename EnumType>
constexpr auto fromEnumClass(EnumType e) noexcept
{
    return static_cast<std::underlying_type_t<EnumType>>(e);
}

/**
 * @brief Converts a floating-point value to a boolean representation.
 *
 * This function evaluates whether a floating-point value exceeds 0.5.
 *
 * @tparam FloatType The floating-point type.
 * @param value The value to convert.
 * @return true when the value exceeds the 0.5 threshold.
 */
template <typename FloatType>
bool toBool(const FloatType value) noexcept
{
    return (value > static_cast<FloatType>(0.5));
}

/**
 * @brief Converts an enum value to its underlying integral type.
 *
 * This function extracts the underlying integral type from an enum value.
 *
 * @tparam EnumValueType The enum type.
 * @param e The enum value to convert.
 * @return The integral representation of the enum.
 */
template <typename EnumValueType>
static constexpr typename std::underlying_type<EnumValueType>::type
    to_underlying(EnumValueType e) noexcept
{
    return static_cast<typename std::underlying_type<EnumValueType>::type>(e);
}


/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam
