/**
 * @file        jam_BitCast.h
 * @brief       C++17 constexpr polyfill for std::bit_cast.
 *
 * Uses __builtin_bit_cast (available GCC 10+, Clang 9+, MSVC 19.27+).
 * Drop-in replacement for std::bit_cast when moving to C++20.
 */
namespace jam
{
/*____________________________________________________________________________*/

/** @brief Reinterprets the bits of From as To.
 *
 *  Requires sizeof(To) == sizeof(From) and both trivially copyable.
 *  Constexpr. No UB.
 *
 *  @tparam To    Target type.
 *  @tparam From  Source type (deduced).
 *  @param  from  Source value.
 *  @return       Value of type To with identical bit pattern.
 */
template <typename To, typename From>
constexpr To bit_cast (const From& from) noexcept
{
    static_assert (sizeof (To) == sizeof (From));
    static_assert (std::is_trivially_copyable_v<To>);
    static_assert (std::is_trivially_copyable_v<From>);
    return __builtin_bit_cast (To, from);
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
