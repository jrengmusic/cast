// Clean-room fork of ankerl::unordered_dense v4.8.1 (MIT License).
// Source: https://github.com/martinus/unordered_dense
// jam owns this fork. Do not merge upstream changes without manual review.
//
// Line-count exception: this file is exempt from CODING.md 300/30/3 line limits.
// All other jam conventions apply.
//
// ============================================================================
// MIT License
//
// Copyright (c) 2022 Martin Leitner-Ankerl <martin.ankerl@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ============================================================================

/**
 * @file jam_HashMap.h
 * @brief Open-addressed hash map and set — clean-room fork of ankerl::unordered_dense v4.8.1.
 */

#pragma once

// see https://semver.org/spec/v2.0.0.html
#define JAM_HASH_MAP_VERSION_MAJOR \
    4// NOLINT(cppcoreguidelines-macro-usage) incompatible API changes
#define JAM_HASH_MAP_VERSION_MINOR \
    8// NOLINT(cppcoreguidelines-macro-usage) backwards compatible functionality
#define JAM_HASH_MAP_VERSION_PATCH \
    1// NOLINT(cppcoreguidelines-macro-usage) backwards compatible bug fixes

#ifdef _MSC_VER
#define JAM_LIKELY(x) (x)
#define JAM_UNLIKELY(x) (x)
#define JAM_NO_SANITIZE_OVERFLOW
#else
#define JAM_LIKELY(x) __builtin_expect (x, 1)
#define JAM_UNLIKELY(x) __builtin_expect (x, 0)
#define JAM_NO_SANITIZE_OVERFLOW __attribute__ ((__no_sanitize__ ("unsigned-integer-overflow")))
#endif

namespace jam
{

/*____________________________________________________________________________*/
// hash ///////////////////////////////////////////////////////////////////////

/**
 * @brief Compact, high-quality 64-bit hash function used internally for keys
 *        whose std::hash is not avalanching, and for hashing integer keys.
 *
 * This is a stripped-down implementation of wyhash:
 * https://github.com/wangyi-fudan/wyhash
 *
 * Notes on this fork:
 * - No big-endian support: the function is allowed to return different values
 *   on different endians; this is irrelevant for hashing purposes.
 * - The seed and the secret are hardcoded constants baked into the algorithm.
 * - The code has been reformatted and clang-tidied; the underlying algorithm
 *   is unchanged.
 *
 * All members are static; Wyhash is never instantiated.
 *
 * @see https://github.com/wangyi-fudan/wyhash
 */
struct Wyhash
{
    /**
     * @brief 64x64 -> 128-bit multiply-and-mix primitive.
     *
     * Multiplies @p *a by @p *b, returning the low 64 bits in @p *a and the
     * high 64 bits in @p *b. Uses __uint128_t on GCC/Clang, _umul128 on
     * MSVC x64, and a portable 32-bit decomposition fallback otherwise.
     *
     * @param a Pointer to the first (and result-low) operand.
     * @param b Pointer to the second (and result-high) operand.
     */
    static inline void multiplyMix (std::uint64_t* a, std::uint64_t* b) noexcept
    {
#if defined(__SIZEOF_INT128__)
        __uint128_t r = *a;
        r *= *b;
        *a = static_cast<std::uint64_t> (r);
        *b = static_cast<std::uint64_t> (r >> 64U);
#elif defined(_MSC_VER) && defined(_M_X64)
        *a = _umul128 (*a, *b, b);
#else
        std::uint64_t ha = *a >> 32U;
        std::uint64_t hb = *b >> 32U;
        std::uint64_t la = static_cast<std::uint32_t> (*a);
        std::uint64_t lb = static_cast<std::uint32_t> (*b);
        std::uint64_t hi {};
        std::uint64_t lo {};
        std::uint64_t rh = ha * hb;
        std::uint64_t rm0 = ha * lb;
        std::uint64_t rm1 = hb * la;
        std::uint64_t rl = la * lb;
        std::uint64_t t = rl + (rm0 << 32U);
        auto c = static_cast<std::uint64_t> (t < rl);
        lo = t + (rm1 << 32U);
        c += static_cast<std::uint64_t> (lo < t);
        hi = rh + (rm0 >> 32U) + (rm1 >> 32U) + c;
        *a = lo;
        *b = hi;
#endif
    }

    /**
     * @brief Multiply-and-xor mix function (a.k.a. MUM).
     *
     * Computes @c (a * b) as a 128-bit product, takes the xor of the low and
     * high 64-bit halves, and returns the result. Avalanches input bits well
     * and is the core of wyhash.
     *
     * @param a First 64-bit operand.
     * @param b Second 64-bit operand.
     * @return The mixed 64-bit hash.
     */
    [[nodiscard]] static inline auto
    mix (std::uint64_t a, std::uint64_t b) noexcept -> std::uint64_t
    {
        multiplyMix (&a, &b);
        return a ^ b;
    }

    /**
     * @brief Sequence-safe fold of one value into a running hash state.
     *
     * mix() alone is unsafe as a chained accumulator: its MUM multiply
     * absorbs zero — @c mix(state,v) returns 0 whenever EITHER operand is 0,
     * so a zero seed (or any zero-valued part, e.g. an empty sub-hash) would
     * silently zero an entire fold chain and every hash after it. fold()
     * adds the value into the state first and multiplies by a fixed odd
     * constant instead — no data-by-data multiply, no absorbing element,
     * order-dependent (the running state evolves per fold).
     *
     * @param state Current folded hash state (zero seed is safe).
     * @param v     64-bit value to fold in.
     * @return Updated hash state.
     */
    [[nodiscard]] static inline auto
    fold (std::uint64_t state, std::uint64_t v) noexcept -> std::uint64_t
    {
        return mix (state + v, std::uint64_t { 0x9ddfea08eb382d69 });
    }

    /**
     * @brief Unaligned little-endian-style read of 8 bytes.
     *
     * @warning Results are not portable across big-endian machines; this is
     *          intentional — the hash need only be consistent within a build.
     *
     * @param p Pointer to at least 8 bytes of readable memory.
     * @return The 64-bit value loaded from @p p.
     */
    [[nodiscard]] static inline auto readU64 (const std::uint8_t* p) noexcept -> std::uint64_t
    {
        std::uint64_t v {};
        std::memcpy (&v, p, 8U);
        return v;
    }

    /**
     * @brief Unaligned little-endian-style read of 4 bytes.
     *
     * @warning Results are not portable across big-endian machines.
     *
     * @param p Pointer to at least 4 bytes of readable memory.
     * @return The 32-bit value loaded from @p p, zero-extended to 64 bits.
     */
    [[nodiscard]] static inline auto readU32 (const std::uint8_t* p) noexcept -> std::uint64_t
    {
        std::uint32_t v {};
        std::memcpy (&v, p, 4);
        return v;
    }

    /**
     * @brief Read 1, 2, or 3 trailing bytes into a 64-bit value.
     *
     * Used to consume the unaligned tail of a short input buffer
     * (length < 4) in hashBytes().
     *
     * @param p Pointer to the start of the tail bytes.
     * @param k Number of valid bytes at @p p (1, 2, or 3).
     * @return A 64-bit value encoding the tail bytes.
     */
    [[nodiscard]] static inline auto
    readTail (const std::uint8_t* p, std::size_t k) noexcept -> std::uint64_t
    {
        return (static_cast<std::uint64_t> (p[0]) << 16U)
               | (static_cast<std::uint64_t> (p[k >> 1U]) << 8U) | p[k - 1];
    }

    /**
     * @brief Hash an arbitrary byte buffer.
     *
     * Implements the wyhash algorithm specialised for short and medium
     * buffers. The hardcoded secret is part of the algorithm definition.
     *
     * @param key Pointer to the first byte of the buffer.
     * @param len Length of the buffer in bytes.
     * @return A 64-bit hash of the buffer contents.
     */
    [[maybe_unused]] [[nodiscard]] static inline auto
    hashBytes (const void* key, std::size_t len) noexcept -> std::uint64_t
    {
        static constexpr auto secret = std::array { UINT64_C (0xa0761d6478bd642f),
                                                    UINT64_C (0xe7037ed1a0b428db),
                                                    UINT64_C (0x8ebc6af09c88c6e3),
                                                    UINT64_C (0x589965cc75374cc3) };

        const auto* p = static_cast<const std::uint8_t*> (key);
        std::uint64_t seed { secret[0] };
        std::uint64_t a {};
        std::uint64_t b {};
        if (JAM_LIKELY (len <= 16))
        {
            if (JAM_LIKELY (len >= 4))
            {
                a = (readU32 (p) << 32U) | readU32 (p + ((len >> 3U) << 2U));
                b = (readU32 (p + len - 4) << 32U) | readU32 (p + len - 4 - ((len >> 3U) << 2U));
            }
            else if (JAM_LIKELY (len > 0))
            {
                a = readTail (p, len);
                b = 0;
            }
            else
            {
                a = 0;
                b = 0;
            }
        }
        else
        {
            std::size_t i { len };
            if (JAM_UNLIKELY (i > 48))
            {
                std::uint64_t see1 = seed;
                std::uint64_t see2 = seed;
                do
                {
                    seed = mix (readU64 (p) ^ secret[1], readU64 (p + 8) ^ seed);
                    see1 = mix (readU64 (p + 16) ^ secret[2], readU64 (p + 24) ^ see1);
                    see2 = mix (readU64 (p + 32) ^ secret[3], readU64 (p + 40) ^ see2);
                    p += 48;
                    i -= 48;
                } while (JAM_LIKELY (i > 48));
                seed ^= see1 ^ see2;
            }
            while (JAM_UNLIKELY (i > 16))
            {
                seed = mix (readU64 (p) ^ secret[1], readU64 (p + 8) ^ seed);
                i -= 16;
                p += 16;
            }
            a = readU64 (p + i - 16);
            b = readU64 (p + i - 8);
        }

        return mix (secret[1] ^ len, mix (a ^ secret[1], b ^ seed));
    }

    /**
     * @brief Hash a single 64-bit integer.
     *
     * Uses the MUM mix with the golden-ratio constant as the second operand
     * to produce a high-quality avalanching hash of an integer.
     *
     * @param x The integer to hash.
     * @return A 64-bit hash of @p x.
     */
    [[nodiscard]] static inline auto hashInt (std::uint64_t x) noexcept -> std::uint64_t
    {
        return mix (x, UINT64_C (0x9e3779b97f4a7c15));
    }
};

/**
 * @brief Primary template: fallback that delegates to @c std::hash<Value>.
 *
 * Used when no specialised Hash<Value> is provided and when @c std::hash<Value>
 * does not declare an @c is_avalanching member type. The result is
 * not guaranteed to avalanche, so HashMap will run it through
 * Wyhash::hashInt to mix the bits before using them.
 *
 * @tparam Value Key type being hashed.
 * @tparam Enable SFINAE hook; defaults to @c void.
 */
template <typename Value, typename Enable = void>
struct Hash
{
    /**
     * @brief Hash a value of type @c Value by forwarding to @c std::hash<Value>.
     *
     * @param obj The value to hash.
     * @return 64-bit hash. Not necessarily avalanching — HashMap will
     *         apply an additional mix step for this case.
     */
    auto operator() (const Value& obj) const noexcept (
        noexcept (std::declval<std::hash<Value>>().operator() (std::declval<const Value&>())))
        -> std::uint64_t
    {
        return std::hash<Value> {}(obj);
    }
};

/**
 * @brief Specialisation selected when @c std::hash<Value> already provides
 *        @c is_avalanching.
 *
 * Indicates to HashMap that the underlying hash is already
 * high quality and that no extra mixing is required.
 *
 * @tparam Value Key type whose @c std::hash is avalanching.
 */
template <typename Value>
struct Hash<Value, typename std::hash<Value>::is_avalanching>
{
    /** Marker type enabling HashMap to detect avalanching hashes. */
    using is_avalanching = void;

    /**
     * @brief Hash a value of type @c Value using its avalanching @c std::hash.
     *
     * @param obj The value to hash.
     * @return 64-bit avalanching hash.
     */
    auto operator() (const Value& obj) const noexcept (
        noexcept (std::declval<std::hash<Value>>().operator() (std::declval<const Value&>())))
        -> std::uint64_t
    {
        return std::hash<Value> {}(obj);
    }
};

/**
 * @brief Avalanching Hash for @c std::basic_string<CharT> using wyhash.
 *
 * @tparam CharT Character type of the string (char, wchar_t, char8_t, ...).
 */
template <typename CharT>
struct Hash<std::basic_string<CharT>>
{
    /** Marker type enabling HashMap to detect avalanching hashes. */
    using is_avalanching = void;

    /**
     * @brief Hash a basic_string by hashing its raw byte buffer with wyhash.
     *
     * @param str The string to hash.
     * @return 64-bit avalanching hash.
     */
    auto operator() (const std::basic_string<CharT>& str) const noexcept -> std::uint64_t
    {
        return Wyhash::hashBytes (str.data(), sizeof (CharT) * str.size());
    }
};

/**
 * @brief Avalanching Hash for @c std::basic_string_view<CharT> using wyhash.
 *
 * @tparam CharT Character type of the string view.
 */
template <typename CharT>
struct Hash<std::basic_string_view<CharT>>
{
    /** Marker type enabling HashMap to detect avalanching hashes. */
    using is_avalanching = void;

    /**
     * @brief Hash a basic_string_view by hashing its raw byte buffer with wyhash.
     *
     * @param sv The string view to hash.
     * @return 64-bit avalanching hash.
     */
    auto operator() (const std::basic_string_view<CharT>& sv) const noexcept -> std::uint64_t
    {
        return Wyhash::hashBytes (sv.data(), sizeof (CharT) * sv.size());
    }
};

/**
 * @brief Avalanching Hash for @c juce::Identifier using wyhash.
 *
 * @c juce::Identifier (`juce_Identifier.h`) wraps a @c juce::String for its
 * name and defines no @c std::hash specialisation of its own — unlike
 * @c juce::String, whose `namespace std` specialisation (`juce_String.h`)
 * forwards to @c juce::String::hash(). Without this specialisation, every
 * existing `jam::HashMap<juce::Identifier, ...>` member (`jam_AnyMap.h`,
 * `jam_Validator.h`, `jam_Model.h`) falls through to the
 * @c Hash<Value,Enable=void> primary template, which calls
 * @c std::hash<juce::Identifier>{}(obj) — ill-formed, since the primary
 * @c std::hash template has no callable @c operator() for an unspecialised
 * type. Hashes the identifier's own text buffer directly with wyhash,
 * matching the @c std::basic_string/@c std::basic_string_view
 * specialisations above rather than delegating to @c juce::String::hash()
 * (unknown avalanche quality).
 */
template <>
struct Hash<juce::Identifier>
{
    /** Marker type enabling HashMap to detect avalanching hashes. */
    using is_avalanching = void;

    /**
     * @brief Hash an Identifier by hashing its name's raw UTF-8 byte buffer.
     *
     * @param identifier The identifier to hash.
     * @return 64-bit avalanching hash.
     */
    auto operator() (const juce::Identifier& identifier) const noexcept -> std::uint64_t
    {
        const auto& name { identifier.toString() };
        return Wyhash::hashBytes (name.toRawUTF8(), name.getNumBytesAsUTF8());
    }
};

/**
 * @brief Avalanching Hash for @c juce::String using wyhash.
 *
 * `namespace std`'s `std::hash<juce::String>` specialisation (`juce_String.h`)
 * forwards to @c juce::String::hash(), of unknown avalanche quality, so every
 * `jam::HashMap<juce::String, ...>` member would otherwise fall through to the
 * @c Hash<Value,Enable=void> primary template's `std::hash` delegation.
 * Hashes the string's raw UTF-8 byte buffer directly with wyhash, matching
 * the @c juce::Identifier specialisation above.
 */
template <>
struct Hash<juce::String>
{
    /** Marker type enabling HashMap to detect avalanching hashes. */
    using is_avalanching = void;

    /**
     * @brief Hash a String by hashing its raw UTF-8 byte buffer.
     *
     * @param str The string to hash.
     * @return 64-bit avalanching hash.
     */
    auto operator() (const juce::String& str) const noexcept -> std::uint64_t
    {
        return Wyhash::hashBytes (str.toRawUTF8(), str.getNumBytesAsUTF8());
    }
};

/**
 * @brief Avalanching Hash for raw pointers; hashes the address as an integer.
 *
 * @tparam Pointee Pointed-to type.
 */
template <typename Pointee>
struct Hash<Pointee*>
{
    /** Marker type enabling HashMap to detect avalanching hashes. */
    using is_avalanching = void;

    /**
     * @brief Hash a raw pointer by hashing its address.
     *
     * @param ptr The pointer to hash.
     * @return 64-bit avalanching hash of the address.
     */
    auto operator() (Pointee* ptr) const noexcept -> std::uint64_t
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return Wyhash::hashInt (reinterpret_cast<std::uintptr_t> (ptr));
    }
};

/**
 * @brief Avalanching Hash for @c std::unique_ptr<Pointee>; hashes the held address.
 *
 * @tparam Pointee Pointed-to type.
 */
template <typename Pointee>
struct Hash<std::unique_ptr<Pointee>>
{
    /** Marker type enabling HashMap to detect avalanching hashes. */
    using is_avalanching = void;

    /**
     * @brief Hash a unique_ptr by hashing the address it holds.
     *
     * @param ptr The unique_ptr to hash.
     * @return 64-bit avalanching hash of the held address.
     */
    auto operator() (const std::unique_ptr<Pointee>& ptr) const noexcept -> std::uint64_t
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return Wyhash::hashInt (reinterpret_cast<std::uintptr_t> (ptr.get()));
    }
};

/**
 * @brief Avalanching Hash for @c std::shared_ptr<Pointee>; hashes the held address.
 *
 * @tparam Pointee Pointed-to type.
 */
template <typename Pointee>
struct Hash<std::shared_ptr<Pointee>>
{
    /** Marker type enabling HashMap to detect avalanching hashes. */
    using is_avalanching = void;

    /**
     * @brief Hash a shared_ptr by hashing the address it holds.
     *
     * @param ptr The shared_ptr to hash.
     * @return 64-bit avalanching hash of the held address.
     */
    auto operator() (const std::shared_ptr<Pointee>& ptr) const noexcept -> std::uint64_t
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return Wyhash::hashInt (reinterpret_cast<std::uintptr_t> (ptr.get()));
    }
};

/**
 * @brief Avalanching Hash for any enum type; hashes the underlying integer.
 *
 * @tparam Enum Enumeration type. Enabled by SFINAE on @c std::is_enum_v.
 */
template <typename Enum>
struct Hash<Enum, typename std::enable_if_t<std::is_enum_v<Enum>>>
{
    /** Marker type enabling HashMap to detect avalanching hashes. */
    using is_avalanching = void;

    /**
     * @brief Hash an enum by casting to its underlying integer and mixing.
     *
     * @param e The enumerator to hash.
     * @return 64-bit avalanching hash of the underlying integer value.
     */
    auto operator() (Enum e) const noexcept -> std::uint64_t
    {
        using underlying = std::underlying_type_t<Enum>;
        return Wyhash::hashInt (static_cast<std::uint64_t> (static_cast<underlying> (e)));
    }
};

/**
 * @brief Helper that mixes a heterogeneous sequence of values into a single
 *        64-bit hash, used by Hash<std::tuple<...>> and Hash<std::pair<...>>.
 *
 * The mixing strategy is a left fold: each element is reduced to a 64-bit
 * value via @c to64, then folded into the running state with @c mix64.
 *
 * @tparam Args The element types of the tuple/pair (unused in the helper
 *             itself, kept for consistency with derived Hash specializations).
 */
template <typename... Args>
struct TupleHashHelper
{
    /**
     * @brief Reduce a tuple element to a 64-bit value.
     *
     * For integral and enum types, performs a plain zero-extending cast;
     * mixing happens later in the fold. For all other types, delegates to
     * @c Hash<Arg>.
     *
     * @tparam Arg Type of the tuple element.
     * @param arg The element value.
     * @return 64-bit reduction of @p arg.
     */
    template <typename Arg>
    [[nodiscard]] constexpr static auto to64 (const Arg& arg) -> std::uint64_t
    {
        if constexpr (std::is_integral_v<Arg> or std::is_enum_v<Arg>)
        {
            return static_cast<std::uint64_t> (arg);
        }
        else
        {
            return Hash<Arg> {}(arg);
        }
    }

    /**
     * @brief Mix one element into a running hash state using the MUM mixer.
     *
     * Unsigned-integer-overflow sanitiser is disabled for this routine
     * because the wraparound is intentional and benign.
     *
     * @param state Current folded hash state.
     * @param v     64-bit value to fold in.
     * @return Updated hash state.
     */
    [[nodiscard]] JAM_NO_SANITIZE_OVERFLOW static auto
    mix64 (std::uint64_t state, std::uint64_t v) -> std::uint64_t
    {
        return Wyhash::fold (state, v);
    }

    /**
     * @brief Compute the hash of a tuple-like value.
     *
     * Performs a left fold of @c to64(std::get<Idx>(t)) over all indices in
     * @p Idx, mixing each into the running state with @c mix64. The
     * index_sequence is fully unrolled at compile time.
     *
     * @tparam Tuple Tuple-like type.
     * @tparam Idx Index sequence spanning the elements of @p Tuple.
     * @param t The tuple/pair to hash.
     * @return 64-bit avalanching hash of @p t.
     */
    template <typename Tuple, std::size_t... Idx>
    [[nodiscard]] static auto
    calcHash (const Tuple& t, std::index_sequence<Idx...> /*unused*/) noexcept -> std::uint64_t
    {
        auto h = std::uint64_t {};
        ((h = mix64 (h, to64 (std::get<Idx> (t)))), ...);
        return h;
    }
};

/**
 * @brief Avalanching Hash for @c std::tuple<Args...>.
 *
 * @tparam Args Element types of the tuple.
 */
template <typename... Args>
struct Hash<std::tuple<Args...>> : TupleHashHelper<Args...>
{
    /** Marker type enabling HashMap to detect avalanching hashes. */
    using is_avalanching = void;

    /**
     * @brief Hash a tuple by fold-mixing its elements.
     *
     * @param t The tuple to hash.
     * @return 64-bit avalanching hash.
     */
    auto operator() (const std::tuple<Args...>& t) const noexcept -> std::uint64_t
    {
        return TupleHashHelper<Args...>::calcHash (t, std::index_sequence_for<Args...> {});
    }
};

/**
 * @brief Avalanching Hash for @c std::pair<First, Second>.
 *
 * @tparam First First element type.
 * @tparam Second Second element type.
 */
template <typename First, typename Second>
struct Hash<std::pair<First, Second>> : TupleHashHelper<First, Second>
{
    /** Marker type enabling HashMap to detect avalanching hashes. */
    using is_avalanching = void;

    /**
     * @brief Hash a pair by fold-mixing its two elements.
     *
     * @param t The pair to hash.
     * @return 64-bit avalanching hash.
     */
    auto operator() (const std::pair<First, Second>& t) const noexcept -> std::uint64_t
    {
        return TupleHashHelper<First, Second>::calcHash (
            t, std::index_sequence_for<First, Second> {});
    }
};

/**
 * @brief Avalanching Hash for all integral types.
 *
 * Uses @c Wyhash::hashInt after casting to @c std::uint64_t.
 * Covers bool, char, all signed/unsigned integer types, wchar_t,
 * char16_t, and char32_t. Mutually exclusive with the enum
 * specialisation via SFINAE (@c std::is_integral_v and @c std::is_enum_v
 * are disjoint for non-bool types; bool is integral and handled here).
 *
 * @tparam Integral Integral or character type.
 */
// see https://en.cppreference.com/w/cpp/utility/hash
template <typename Integral>
struct Hash<Integral, std::enable_if_t<std::is_integral_v<Integral>>>
{
    /** Marker type enabling HashMap to detect avalanching hashes. */
    using is_avalanching = void;

    /**
     * @brief Hash an integral value by zero-extending to 64 bits and mixing.
     *
     * @param obj The value to hash.
     * @return 64-bit avalanching hash.
     */
    auto operator() (const Integral& obj) const noexcept -> std::uint64_t
    {
        return Wyhash::hashInt (static_cast<std::uint64_t> (obj));
    }
};

// BucketType //////////////////////////////////////////////////////////

/**
 * @brief Tag types for the two bucket encodings supported by HashMap.
 *
 * The bucket array stores one entry per slot in the hash table. Each
 * entry encodes (a) the displacement from the key's original bucket
 * index and (b) a 1-byte fingerprint of the key, packed into a single
 * 32-bit word. A second field stores the index into the dense value
 * vector that holds the actual key/value pair.
 *
 * @see HashMap
 */
struct BucketType
{
    /**
     * @brief Default bucket layout: 32-bit displacement/fingerprint plus
     *        32-bit value index.
     *
     * Fields:
     * - @c m_distAndFingerprint — upper 24 bits: probe distance (in
     *   units of @c distInc) from the key's original bucket index.
     *   Lower 8 bits: 1-byte fingerprint of the key's hash. A value of
     *   zero means the slot is empty.
     * - @c m_valueIdx — index into HashMap::m_values where the
     *   actual key/value (or just key, for sets) is stored.
     *
     * @c distInc and @c fingerprintMask encode the layout in use.
     */
    struct Standard
    {
        /** Per-probe displacement unit; lower 8 bits hold the fingerprint. */
        static constexpr std::uint32_t distInc { 1U << 8U };// skip 1 byte fingerprint
        /** Mask isolating the 1-byte fingerprint portion of the packed word. */
        static constexpr std::uint32_t fingerprintMask { distInc
                                                         - 1 };// mask for 1 byte of fingerprint

        /**
         * Packed (distance, fingerprint) word. Upper 24 bits = distance to
         * original bucket, lower 8 bits = fingerprint from hash. Zero means
         * empty slot.
         */
        std::uint32_t
            m_distAndFingerprint;// upper 3 byte: distance to original bucket. lower byte: fingerprint from hash
        /** Index into the dense m_values vector. */
        std::uint32_t m_valueIdx;// index into the m_values vector.
    };

    /**
     * @brief Wide bucket layout used when the value index may exceed 32 bits.
     *
     * Packed (via platform-specific pragma/attribute) to keep the
     * fingerprint/distance word adjacent to the value index. Identical layout to Standard except that
     * @c m_valueIdx is a full @c std::size_t, allowing tables whose value
     * count exceeds @c std::uint32_t.
     *
     * Fields:
     * - @c m_distAndFingerprint — same meaning as in Standard.
     * - @c m_valueIdx — index into HashMap::m_values.
     */
#if JUCE_MSVC
#pragma pack(push, 1)
#endif
    struct Big
    {
        /** Per-probe displacement unit; lower 8 bits hold the fingerprint. */
        static constexpr std::uint32_t distInc { 1U << 8U };// skip 1 byte fingerprint
        /** Mask isolating the 1-byte fingerprint portion of the packed word. */
        static constexpr std::uint32_t fingerprintMask { distInc
                                                         - 1 };// mask for 1 byte of fingerprint

        /**
         * Packed (distance, fingerprint) word. Upper 24 bits = distance to
         * original bucket, lower 8 bits = fingerprint from hash.
         */
        std::uint32_t
            m_distAndFingerprint;// upper 3 byte: distance to original bucket. lower byte: fingerprint from hash
        /** Index into the dense m_values vector (64-bit). */
        std::size_t m_valueIdx;// index into the m_values vector.
    }
#if JUCE_GCC || JUCE_CLANG
    __attribute__ ((__packed__))
#endif
    ;
#if JUCE_MSVC
#pragma pack(pop)
#endif
};

/**
 * @brief Collection of SFINAE detectors, type traits, and tag types used to
 *        specialise HashMap behaviour.
 *
 * These helpers are implementation details of the HashMap machinery; they are
 * exposed at namespace scope so they can be reused by other containers in
 * the same family.
 */
struct HashMapTraits
{
    /** Sentinel type returned by Detector when the probed expression is invalid. */
    struct NoSuch
    {
    };

    /** Tag type meaning "use the default bucket container for this configuration." */
    struct DefaultContainer
    {
    };

    /**
     * @brief SFINAE detector: if @c Operation<Args...> is well-formed, the partial
     *        specialisation kicks in and @c value_t is @c std::true_type.
     *
     * @tparam Default Type used as @c type in the failure branch.
     * @tparam AlwaysVoid Unused tag used to disambiguate the partial
     *                    specialisations; set to @c std::void_t<Operation<Args...>>.
     * @tparam Operation Alias template to instantiate.
     * @tparam Args Arguments to pass to @c Operation.
     */
    template <typename Default,
             typename AlwaysVoid,
             template <typename...> typename Operation,
             typename... Args>
    struct Detector
    {
        using value_t = std::false_type;
        using type = Default;
    };

    /**
     * @brief Partial specialisation of Detector selected when @c Operation<Args...>
     *        is a valid type.
     *
     * @tparam Default Type used as @c type in the failure branch.
     * @tparam Operation Alias template to instantiate.
     * @tparam Args Arguments to pass to @c Operation.
     */
    template <typename Default, template <typename...> typename Operation, typename... Args>
    struct Detector<Default, std::void_t<Operation<Args...>>, Operation, Args...>
    {
        using value_t = std::true_type;
        using type = Operation<Args...>;
    };

    /**
     * @brief Convenience alias yielding @c std::true_type if @c Operation<Args...>
     *        is valid, else @c std::false_type.
     *
     * @tparam Operation Alias template to instantiate.
     * @tparam Args Arguments to pass to @c Operation.
     */
    template <template <typename...> typename Operation, typename... Args>
    using IsDetected = typename Detector<NoSuch, void, Operation, Args...>::value_t;

    /**
     * @brief Convenience variable template yielding the boolean value of
     *        @c IsDetected<Operation, Args...>.
     *
     * @tparam Operation Alias template to instantiate.
     * @tparam Args Arguments to pass to @c Operation.
     */
    template <template <typename...> typename Operation, typename... Args>
    static constexpr bool isDetected { IsDetected<Operation, Args...>::value };

    /** SFINAE probe: looks for a nested @c is_avalanching member. */
    template <typename Type>
    using DetectAvalanching = typename Type::is_avalanching;

    /** SFINAE probe: looks for a nested @c is_transparent member. */
    template <typename Type>
    using DetectIsTransparent = typename Type::is_transparent;

    /** SFINAE probe: looks for a nested @c iterator member. */
    template <typename Type>
    using DetectIterator = typename Type::iterator;

    /** SFINAE probe: looks for a callable @c reserve(std::size_t) member. */
    template <typename Type>
    using DetectReserve = decltype (std::declval<Type&>().reserve (std::size_t {}));

    // enable_if helpers

    /**
     * @brief True when HashMap is being instantiated as a map (T is not void).
     *
     * @tparam Mapped The mapped-type slot of HashMap; @c void means set.
     */
    template <typename Mapped>
    static constexpr bool isMap { not std::is_void_v<Mapped> };

    // clang-format off
    /**
     * @brief True when both @c Hash and @c KeyEqual expose @c is_transparent,
     *        enabling heterogeneous lookup and insertion.
     *
     * @tparam Hash The hasher.
     * @tparam KeyEqual The key-equality functor.
     */
    template <typename Hash, typename KeyEqual>
    static constexpr bool isTransparent { isDetected<DetectIsTransparent, Hash> and isDetected<DetectIsTransparent, KeyEqual> };
    // clang-format on

    /**
     * @brief True when @c From is not implicitly convertible to either @c To1
     *        or @c To2. Used to disambiguate overload sets in the map path.
     *
     * @tparam From Source type.
     * @tparam To1 First target type.
     * @tparam To2 Second target type.
     */
    template <typename From, typename To1, typename To2>
    static constexpr bool isNeitherConvertible =
        not std::is_convertible_v<From, To1> and not std::is_convertible_v<From, To2>;

    /**
     * @brief True when @c Type has a callable @c reserve(std::size_t) method.
     *
     * @tparam Type Type to introspect.
     */
    template <typename Type>
    static constexpr bool hasReserve { isDetected<DetectReserve, Type> };

    /**
     * @brief Base class added to HashMap when instantiated as a map;
     *        contributes the @c mapped_type member alias.
     *
     * @tparam MappedType The mapped value type.
     */
    // base type for map has mapped_type
    template <typename MappedType>
    struct MapBase
    {
        /** The mapped value type. */
        using mapped_type = MappedType;
    };

    /**
     * @brief Base class added to HashMap when instantiated as a set;
     *        intentionally has no @c mapped_type member.
     */
    // base type for set doesn't have mapped_type
    struct SetBase
    {
    };
};

// Very much like std::deque, but faster for indexing (in most cases). As of now this doesn't implement the full std::vector
// API, but merely what's necessary to work as an underlying container for jam::{HashMap, HashSet}.
// It allocates blocks of equal size and puts them into the m_blocks vector. That means it can grow simply by adding a new
// block to the back of m_blocks, and doesn't double its size like an std::vector. The disadvantage is that memory is not
// linear and thus there is one more indirection necessary for indexing.

/**
 * @brief Backing dense container for SegmentedHashMap and SegmentedHashSet.
 *
 * Very much like @c std::deque, but faster for indexing in most cases. It
 * allocates fixed-size blocks (sized so that one block stays under
 * @c MaxSegmentSizeBytes) and stores them in an @c std::vector. Growth
 * appends a new block; there is no doubling of the underlying storage as
 * with @c std::vector. The trade-off is one extra indirection per index
 * (block pointer table, then the block).
 *
 * Only the API surface required by HashMap is implemented. Notably
 * absent: @c push_back, @c insert, @c erase — only @c emplace_back and
 * bulk operations are provided.
 *
 * @tparam Element              Element type.
 * @tparam Allocator            Allocator used for the per-block allocations.
 * @tparam MaxSegmentSizeBytes  Upper bound on the size of a single block in
 *                              bytes. The actual number of elements per
 *                              block is the largest power of two that fits.
 */
template <typename Element,
         typename Allocator = std::allocator<Element>,
         std::size_t MaxSegmentSizeBytes = 4096>
class SegmentedVector
{
    template <bool IsConst>
    class Iter;

public:
    using allocator_type = Allocator;
    using pointer = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;
    using difference_type = typename std::allocator_traits<allocator_type>::difference_type;
    using value_type = Element;
    using size_type = std::size_t;
    using reference = Element&;
    using const_reference = const Element&;
    using iterator = Iter<false>;
    using const_iterator = Iter<true>;

private:
    using VecAlloc = typename std::allocator_traits<Allocator>::template rebind_alloc<pointer>;
    std::vector<pointer, VecAlloc> m_blocks {};
    std::size_t m_size {};

    // Calculates the maximum number for x in  (s << x) <= max_val
    static constexpr auto numBitsClosest (std::size_t max_val, std::size_t s) -> std::size_t
    {
        auto f = std::size_t { 0 };
        while (s << (f + 1) <= max_val)
        {
            ++f;
        }
        return f;
    }

    using Self = SegmentedVector<Element, Allocator, MaxSegmentSizeBytes>;
    static constexpr auto numBits { numBitsClosest (MaxSegmentSizeBytes, sizeof (Element)) };
    static constexpr auto numElementsInBlock { 1U << numBits };
    static constexpr auto mask { numElementsInBlock - 1U };

    /**
     * Iterator class doubles as const_iterator and iterator
     */
    template <bool IsConst>
    class Iter
    {
        using Ptr = std::conditional_t<IsConst,
                                       const SegmentedVector::const_pointer*,
                                       SegmentedVector::pointer*>;
        Ptr m_data {};
        std::size_t m_idx {};

    public:
        using difference_type = SegmentedVector::difference_type;
        using value_type = SegmentedVector::value_type;
        using reference = std::conditional_t<IsConst, const value_type&, value_type&>;
        using pointer =
            std::conditional_t<IsConst, SegmentedVector::const_pointer, SegmentedVector::pointer>;
        using iterator_category = std::forward_iterator_tag;

        Iter() noexcept = default;

        /** @brief Returns the pointer to this iterator's owning block array. */
        [[nodiscard]] constexpr auto getData() const noexcept -> Ptr { return m_data; }

        /** @brief Returns this iterator's linear index into the segmented vector. */
        [[nodiscard]] constexpr auto getIndex() const noexcept -> std::size_t { return m_idx; }

        template <bool OtherIsConst, typename = std::enable_if_t<IsConst and not OtherIsConst>>
        // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
        constexpr Iter (const Iter<OtherIsConst>& other) noexcept
            : m_data (other.getData())
            , m_idx (other.getIndex())
        {
        }

        constexpr Iter (Ptr data, std::size_t idx) noexcept
            : m_data (data)
            , m_idx (idx)
        {
        }

        template <bool OtherIsConst, typename = std::enable_if_t<IsConst and not OtherIsConst>>
        constexpr auto operator= (const Iter<OtherIsConst>& other) noexcept -> Iter&
        {
            m_data = other.getData();
            m_idx = other.getIndex();
            return *this;
        }

        constexpr auto operator++() noexcept -> Iter&
        {
            ++m_idx;
            return *this;
        }

        constexpr auto operator++ (int) noexcept -> Iter
        {
            Iter prev (*this);
            this->operator++();
            return prev;
        }

        constexpr auto operator--() noexcept -> Iter&
        {
            --m_idx;
            return *this;
        }

        constexpr auto operator-- (int) noexcept -> Iter
        {
            Iter prev (*this);
            this->operator--();
            return prev;
        }

        [[nodiscard]] constexpr auto operator+ (difference_type diff) const noexcept -> Iter
        {
            return { m_data,
                     static_cast<std::size_t> (static_cast<difference_type> (m_idx) + diff) };
        }

        constexpr auto operator+= (difference_type diff) noexcept -> Iter&
        {
            m_idx += diff;
            return *this;
        }

        [[nodiscard]] constexpr auto operator- (difference_type diff) const noexcept -> Iter
        {
            return { m_data,
                     static_cast<std::size_t> (static_cast<difference_type> (m_idx) - diff) };
        }

        constexpr auto operator-= (difference_type diff) noexcept -> Iter&
        {
            m_idx -= diff;
            return *this;
        }

        template <bool OtherIsConst>
        [[nodiscard]] constexpr auto
        operator- (const Iter<OtherIsConst>& other) const noexcept -> difference_type
        {
            return static_cast<difference_type> (m_idx)
                   - static_cast<difference_type> (other.getIndex());
        }

        constexpr auto operator*() const noexcept -> reference
        {
            return m_data[m_idx >> numBits][m_idx & mask];
        }

        constexpr auto operator->() const noexcept -> pointer
        {
            return &m_data[m_idx >> numBits][m_idx & mask];
        }

        template <bool OtherConst>
        [[nodiscard]] constexpr auto operator== (const Iter<OtherConst>& o) const noexcept -> bool
        {
            return m_idx == o.getIndex();
        }

        template <bool OtherConst>
        [[nodiscard]] constexpr auto operator!= (const Iter<OtherConst>& o) const noexcept -> bool
        {
            return not(*this == o);
        }

        template <bool OtherConst>
        [[nodiscard]] constexpr auto operator< (const Iter<OtherConst>& o) const noexcept -> bool
        {
            return m_idx < o.getIndex();
        }

        template <bool OtherConst>
        [[nodiscard]] constexpr auto operator> (const Iter<OtherConst>& o) const noexcept -> bool
        {
            return o < *this;
        }

        template <bool OtherConst>
        [[nodiscard]] constexpr auto operator<= (const Iter<OtherConst>& o) const noexcept -> bool
        {
            return not(o < *this);
        }

        template <bool OtherConst>
        [[nodiscard]] constexpr auto operator>= (const Iter<OtherConst>& o) const noexcept -> bool
        {
            return not(*this < o);
        }
    };

    // slow path: need to allocate a new segment every once in a while
    void increaseCapacity()
    {
        auto ba = Allocator (m_blocks.get_allocator());
        pointer block = std::allocator_traits<Allocator>::allocate (ba, numElementsInBlock);
        m_blocks.push_back (block);
    }

    // Moves everything from other
    void appendEverythingFrom (SegmentedVector&& other)
    {// NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
        reserve (size() + other.size());
        for (auto&& o : other)
        {
            emplace_back (std::move (o));
        }
    }

    // Copies everything from other
    void appendEverythingFrom (const SegmentedVector& other)
    {
        reserve (size() + other.size());
        for (const auto& o : other)
        {
            emplace_back (o);
        }
    }

    void dealloc()
    {
        auto ba = Allocator (m_blocks.get_allocator());
        for (auto ptr : m_blocks)
        {
            std::allocator_traits<Allocator>::deallocate (ba, ptr, numElementsInBlock);
        }
    }

    [[nodiscard]] static constexpr auto calcNumBlocksForCapacity (std::size_t capacity)
    {
        return (capacity + numElementsInBlock - 1U) / numElementsInBlock;
    }

    void resizeShrink (std::size_t new_size)
    {
        if constexpr (not std::is_trivially_destructible_v<Element>)
        {
            for (std::size_t ix = new_size; ix < m_size; ++ix)
            {
                operator[] (ix).~Element();
            }
        }
        m_size = new_size;
    }

public:
    SegmentedVector() = default;

    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    SegmentedVector (Allocator alloc)
        : m_blocks (VecAlloc (alloc))
    {
    }

    SegmentedVector (SegmentedVector&& other, Allocator alloc)
        : SegmentedVector (alloc)
    {
        *this = std::move (other);
    }

    SegmentedVector (const SegmentedVector& other, Allocator alloc)
        : m_blocks (VecAlloc (alloc))
    {
        appendEverythingFrom (other);
    }

    SegmentedVector (SegmentedVector&& other) noexcept
        : SegmentedVector (std::move (other), other.get_allocator())
    {
    }

    SegmentedVector (const SegmentedVector& other) { appendEverythingFrom (other); }

    auto operator= (const SegmentedVector& other) -> SegmentedVector&
    {
        if (this == &other)
        {
            return *this;
        }
        clear();
        appendEverythingFrom (other);
        return *this;
    }

    auto operator= (SegmentedVector&& other) noexcept -> SegmentedVector&
    {
        clear();
        dealloc();
        if (other.get_allocator() == get_allocator())
        {
            m_blocks = std::move (other.m_blocks);
            m_size = std::exchange (other.m_size, {});
        }
        else
        {
            // make sure to construct with other's allocator!
            m_blocks = std::vector<pointer, VecAlloc> (VecAlloc (other.get_allocator()));
            appendEverythingFrom (std::move (other));
        }
        return *this;
    }

    ~SegmentedVector()
    {
        clear();
        dealloc();
    }

    [[nodiscard]] constexpr auto size() const -> std::size_t { return m_size; }

    [[nodiscard]] constexpr auto capacity() const -> std::size_t
    {
        return m_blocks.size() * numElementsInBlock;
    }

    // Indexing is highly performance critical
    [[nodiscard]] constexpr auto operator[] (std::size_t i) const noexcept -> const Element&
    {
        return m_blocks[i >> numBits][i & mask];
    }

    [[nodiscard]] constexpr auto operator[] (std::size_t i) noexcept -> Element&
    {
        return m_blocks[i >> numBits][i & mask];
    }

    [[nodiscard]] constexpr auto begin() -> iterator { return { m_blocks.data(), 0U }; }
    [[nodiscard]] constexpr auto begin() const -> const_iterator { return { m_blocks.data(), 0U }; }
    [[nodiscard]] constexpr auto cbegin() const -> const_iterator
    {
        return { m_blocks.data(), 0U };
    }

    [[nodiscard]] constexpr auto end() -> iterator { return { m_blocks.data(), m_size }; }
    [[nodiscard]] constexpr auto end() const -> const_iterator
    {
        return { m_blocks.data(), m_size };
    }
    [[nodiscard]] constexpr auto cend() const -> const_iterator
    {
        return { m_blocks.data(), m_size };
    }

    [[nodiscard]] constexpr auto back() -> reference { return operator[] (m_size - 1); }
    [[nodiscard]] constexpr auto back() const -> const_reference { return operator[] (m_size - 1); }

    void pop_back()
    {
        back().~Element();
        --m_size;
    }

    [[nodiscard]] auto empty() const { return 0 == m_size; }

    void reserve (std::size_t new_capacity)
    {
        m_blocks.reserve (calcNumBlocksForCapacity (new_capacity));
        while (new_capacity > capacity())
        {
            increaseCapacity();
        }
    }

    void resize (const std::size_t count)
    {
        if (count < m_size)
        {
            resizeShrink (count);
        }
        else if (count > m_size)
        {
            const std::size_t new_elems = count - m_size;
            reserve (count);
            for (std::size_t ix = 0; ix < new_elems; ++ix)
            {
                emplace_back();
            }
        }
    }

    void resize (const std::size_t count, const value_type& value)
    {
        if (count < m_size)
        {
            resizeShrink (count);
        }
        else if (count > m_size)
        {
            const std::size_t new_elems = count - m_size;
            reserve (count);
            for (std::size_t ix = 0; ix < new_elems; ++ix)
            {
                emplace_back (value);
            }
        }
    }

    [[nodiscard]] auto get_allocator() const -> allocator_type
    {
        return allocator_type { m_blocks.get_allocator() };
    }

    template <typename... Args>
    auto emplace_back (Args&&... args) -> reference
    {
        if (m_size == capacity())
        {
            increaseCapacity();
        }
        auto* ptr = static_cast<void*> (&operator[] (m_size));
        auto& ref = *new (ptr) Element (std::forward<Args> (args)...);
        ++m_size;
        return ref;
    }

    void clear()
    {
        if constexpr (not std::is_trivially_destructible_v<Element>)
        {
            for (std::size_t i = 0, s = size(); i < s; ++i)
            {
                operator[] (i).~Element();
            }
        }
        m_size = 0;
    }

    void shrink_to_fit()
    {
        auto ba = Allocator (m_blocks.get_allocator());
        auto num_blocks_required = calcNumBlocksForCapacity (m_size);
        while (m_blocks.size() > num_blocks_required)
        {
            std::allocator_traits<Allocator>::deallocate (ba, m_blocks.back(), numElementsInBlock);
            m_blocks.pop_back();
        }
        m_blocks.shrink_to_fit();
    }
};

// This is it, the table. Doubles as map and set, and uses `void` for T when its used as a set.

/**
 * @brief Open-addressed, linear-probing hash table that doubles as both map
 *        and set depending on the @c Mapped template parameter.
 *
 * @details
 * Layout:
 *  - @c m_values is a dense, hole-free vector holding either
 *    @c std::pair<Key, Mapped> (map mode) or just @c Key (set mode, when
 *    @c Mapped is @c void). Insertions append to the back; erasures swap-in
 *    the last element to keep the vector dense.
 *  - @c m_buckets is a sparse index into @c m_values. Each slot encodes
 *    a (probe distance, fingerprint) pair in its upper 24 / lower 8 bits
 *    of @c m_distAndFingerprint, plus a 32-bit (or 64-bit, for
 *    @c BucketType::Big) index into @c m_values.
 *
 * Probe strategy:
 *  - Linear probing with a 1-byte fingerprint per slot. The fingerprint
 *    lets lookups short-circuit empty slots whose stored fingerprint is
 *    higher than the probe's, without consulting the value vector.
 *
 * Hash quality:
 *  - The hasher result is run through an avalanche-mix step
 *    (Wyhash::hashInt) unless the user-supplied @c Hash declares
 *    @c is_avalanching, in which case the hash is used directly.
 *
 * Iterator invalidation:
 *  - @c insert, @c emplace, @c try_emplace, @c addOrReplace,
 *    @c operator[], @c clear, @c rehash, @c reserve: all iterators and
 *    references are invalidated only when a rehash occurs (i.e. when
 *    the bucket array is reallocated).
 *  - @c erase, @c extract: the iterator of the erased element is
 *    invalidated; other iterators remain valid in value but the index
 *    of the moved-from last element changes — only the iterator
 *    pointing to the erased element becomes dangling. The returned
 *    iterator from @c erase is valid.
 *  - @c replace_key: all iterators and references remain valid; only
 *    the key value at the given iterator changes.
 *  - @c replace, @c extract() (rvalue overload): the table is left
 *    empty, and all iterators/references are invalidated.
 *
 * Thread safety:
 *  - Not thread-safe. Concurrent reads and writes on the same instance
 *    are a data race.
 *  - Concurrent reads from multiple threads are safe as long as no
 *    thread is writing.
 *
 * @tparam Key                 Key type.
 * @tparam Mapped              Mapped type. Pass @c void to obtain a set.
 * @tparam HashFn              Hasher functor. Must be default-constructible.
 * @tparam KeyEqual            Key-equality functor. Must be default-constructible.
 * @tparam AllocatorOrContainer Allocator for the values container, or a
 *                             user-supplied values container exposing the
 *                             same interface as @c std::vector.
 * @tparam Bucket              Bucket layout (Standard or Big).
 * @tparam BucketContainer     Container used to hold the bucket array, or
 *                             @c HashMapTraits::DefaultContainer to pick
 *                             a std::vector / SegmentedVector.
 * @tparam IsSegmented         If true, the values and bucket containers
 *                             are @c SegmentedVector; otherwise plain
 *                             @c std::vector.
 */
template <typename Key,
         typename Mapped,// when void, treat it as a set.
         typename HashFn = Hash<Key>,
         typename KeyEqual = std::equal_to<Key>,
         typename AllocatorOrContainer = std::allocator<std::pair<Key, Mapped>>,
         typename Bucket = BucketType::Standard,
         typename BucketContainer = HashMapTraits::DefaultContainer,
         bool IsSegmented = false>
class HashMap
    : public std::conditional_t<HashMapTraits::isMap<Mapped>,
                                HashMapTraits::MapBase<Mapped>,
                                HashMapTraits::SetBase>
{
    using underlying_value_type =
        std::conditional_t<HashMapTraits::isMap<Mapped>, std::pair<Key, Mapped>, Key>;
    using underlying_container_type =
        std::conditional_t<IsSegmented,
                           SegmentedVector<underlying_value_type, AllocatorOrContainer>,
                           std::vector<underlying_value_type, AllocatorOrContainer>>;

public:
    /**
     * @brief The container that holds the dense key/value storage. Either
     *        the user-supplied @c AllocatorOrContainer (if it exposes an
     *        @c iterator member) or the default @c std::vector /
     *        @c SegmentedVector chosen by @c IsSegmented.
     */
    using value_container_type = std::conditional_t<
        HashMapTraits::isDetected<HashMapTraits::DetectIterator, AllocatorOrContainer>,
        AllocatorOrContainer,
        underlying_container_type>;

private:
    using bucket_alloc = typename std::allocator_traits<
        typename value_container_type::allocator_type>::template rebind_alloc<Bucket>;
    using default_bucket_container_type = std::conditional_t<IsSegmented,
                                                             SegmentedVector<Bucket, bucket_alloc>,
                                                             std::vector<Bucket, bucket_alloc>>;

    using bucket_container_type =
        std::conditional_t<std::is_same_v<BucketContainer, HashMapTraits::DefaultContainer>,
                           default_bucket_container_type,
                           BucketContainer>;

    static constexpr std::uint8_t initialShifts { 64 - 2 };// 2^(64-m_shift) number of buckets
    static constexpr float defaultMaxLoadFactor { 0.8F };

public:
    /** Key type stored in the table. */
    using key_type = Key;
    /**
     * @brief Value type stored in the table.
     *
     * Equals @c std::pair<Key, Mapped> for maps and @c Key for sets.
     */
    using value_type = typename value_container_type::value_type;
    /** Unsigned integer type used for sizes and bucket counts. */
    using size_type = typename value_container_type::size_type;
    /** Signed integer type used for iterator differences. */
    using difference_type = typename value_container_type::difference_type;
    /** Hasher functor type. */
    using hasher = HashFn;
    /** Key-equality functor type. */
    using key_equal = KeyEqual;
    /** Allocator type obtained from the value container. */
    using allocator_type = typename value_container_type::allocator_type;
    /** Mutable reference to a stored element. */
    using reference = typename value_container_type::reference;
    /** Const reference to a stored element. */
    using const_reference = typename value_container_type::const_reference;
    /** Mutable pointer to a stored element. */
    using pointer = typename value_container_type::pointer;
    /** Const pointer to a stored element. */
    using const_pointer = typename value_container_type::const_pointer;
    /** Const iterator type. */
    using const_iterator = typename value_container_type::const_iterator;
    /**
     * @brief Iterator type.
     *
     * For maps this is a mutable iterator; for sets the underlying
     * container's iterator is already const-like, so @c iterator is
     * the same type as @c const_iterator.
     */
    using iterator = std::conditional_t<HashMapTraits::isMap<Mapped>,
                                        typename value_container_type::iterator,
                                        const_iterator>;
    /** Bucket layout type (Standard or Big). */
    using bucket_type = Bucket;

private:
    using value_idx_type = decltype (Bucket::m_valueIdx);
    using dist_and_fingerprint_type = decltype (Bucket::m_distAndFingerprint);

    static_assert (std::is_trivially_destructible_v<Bucket>,
                   "assert there's no need to call destructor / std::destroy");
    static_assert (std::is_trivially_copyable_v<Bucket>, "assert we can just memset / memcpy");

    value_container_type
        m_values {};// Contains all the key-value pairs in one densely stored container. No holes.
    bucket_container_type m_buckets {};
    std::size_t m_maxBucketCapacity { 0 };
    float m_maxLoadFactor { defaultMaxLoadFactor };
    HashFn m_hash {};
    KeyEqual m_equal {};
    std::uint8_t m_shifts { initialShifts };

    // make sure this is not inlined as it is slow and dramatically enlarges code, thus making other
    // inlinings more difficult. Throws are also generally the slow path.
/**
 * @brief Terminal miss path for @c at(): the requested key is absent — the
 * caller broke @c at()'s present-key precondition. Asserts, then throws
 * @c std::out_of_range when exceptions are enabled; asserts and aborts
 * otherwise. The exception text names neither key nor call site.
 *
 * The precondition belongs to the caller: a caller holding unvalidated
 * input decides with @c contains(); otherwise the key is established at
 * the writer.
 */
#if ! JUCE_EXCEPTIONS_DISABLED
#if JUCE_MSVC
    [[noreturn]] static __declspec (noinline) void onKeyNotFound()
    {
        jassertfalse;
        throw std::out_of_range ("jam::HashMap::at(): key not found");
    }
    [[noreturn]] static __declspec (noinline) void onBucketOverflow()
    {
        throw std::overflow_error ("jam::HashMap::onBucketOverflow(): reached max bucket size, cannot increase size");
    }
    [[noreturn]] static __declspec (noinline) void onTooManyElements()
    {
        throw std::out_of_range ("jam::HashMap::replace(): too many elements");
    }
#else
    [[noreturn]] static __attribute__ ((noinline)) void onKeyNotFound()
    {
        jassertfalse;
        throw std::out_of_range ("jam::HashMap::at(): key not found");
    }
    [[noreturn]] static __attribute__ ((noinline)) void onBucketOverflow()
    {
        throw std::overflow_error ("jam::HashMap::onBucketOverflow(): reached max bucket size, cannot increase size");
    }
    [[noreturn]] static __attribute__ ((noinline)) void onTooManyElements()
    {
        throw std::out_of_range ("jam::HashMap::replace(): too many elements");
    }
#endif
#else
    [[noreturn]] static void onKeyNotFound() { jassertfalse; abort(); }
    [[noreturn]] static void onBucketOverflow() { abort(); }
    [[noreturn]] static void onTooManyElements() { abort(); }
#endif

    [[nodiscard]] auto next (value_idx_type bucket_idx) const -> value_idx_type
    {
        if (JAM_UNLIKELY (bucket_idx + 1U == bucket_count()))
        {
            return 0;
        }

        return static_cast<value_idx_type> (bucket_idx + 1U);
    }

    // Helper to access bucket through pointer types
    [[nodiscard]] static constexpr auto
    at (bucket_container_type& bucket, std::size_t offset) -> Bucket&
    {
        return bucket[offset];
    }

    [[nodiscard]] static constexpr auto
    at (const bucket_container_type& bucket, std::size_t offset) -> const Bucket&
    {
        return bucket[offset];
    }

    // use the distInc and distDec functions so that std::uint16_t types work without warning
    [[nodiscard]] static constexpr auto
    distInc (dist_and_fingerprint_type x) -> dist_and_fingerprint_type
    {
        return static_cast<dist_and_fingerprint_type> (x + Bucket::distInc);
    }

    [[nodiscard]] static constexpr auto
    distDec (dist_and_fingerprint_type x) -> dist_and_fingerprint_type
    {
        return static_cast<dist_and_fingerprint_type> (x - Bucket::distInc);
    }

    // The goal of mixedHash is to always produce a high quality 64bit hash.
    template <typename KeyArg>
    [[nodiscard]] constexpr auto mixedHash (const KeyArg& key) const -> std::uint64_t
    {
        if constexpr (HashMapTraits::isDetected<HashMapTraits::DetectAvalanching, HashFn>)
        {
            // we know that the hash is good because is_avalanching.
            if constexpr (sizeof (decltype (m_hash (key))) < sizeof (std::uint64_t))
            {
                // 32bit hash and is_avalanching => multiply with a constant to avalanche bits upwards
                return m_hash (key) * UINT64_C (0x9ddfea08eb382d69);
            }
            else
            {
                // 64bit and is_avalanching => only use the hash itself.
                return m_hash (key);
            }
        }
        else
        {
            // not is_avalanching => apply wyhash
            return Wyhash::hashInt (m_hash (key));
        }
    }

    [[nodiscard]] constexpr auto
    distAndFingerprintFromHash (std::uint64_t hash) const -> dist_and_fingerprint_type
    {
        return Bucket::distInc
               | (static_cast<dist_and_fingerprint_type> (hash) & Bucket::fingerprintMask);
    }

    [[nodiscard]] constexpr auto bucketIdxFromHash (std::uint64_t hash) const -> value_idx_type
    {
        return static_cast<value_idx_type> (hash >> m_shifts);
    }

    template <typename KeyArg>
    [[nodiscard]] auto nextWhileLess (const KeyArg& key) const -> Bucket
    {
        auto hash = mixedHash (key);
        auto dist_and_fingerprint = distAndFingerprintFromHash (hash);
        auto bucket_idx = bucketIdxFromHash (hash);

        while (dist_and_fingerprint < at (m_buckets, bucket_idx).m_distAndFingerprint)
        {
            dist_and_fingerprint = distInc (dist_and_fingerprint);
            bucket_idx = next (bucket_idx);
        }
        return { dist_and_fingerprint, bucket_idx };
    }

    void placeAndShiftUp (Bucket bucket, value_idx_type place)
    {
        while (0 != at (m_buckets, place).m_distAndFingerprint)
        {
            bucket = std::exchange (at (m_buckets, place), bucket);
            bucket.m_distAndFingerprint = distInc (bucket.m_distAndFingerprint);
            place = next (place);
        }
        at (m_buckets, place) = bucket;
    }

    void eraseAndShiftDown (value_idx_type bucket_idx)
    {
        // shift down until either empty or an element with correct spot is found
        auto next_bucket_idx = next (bucket_idx);
        while (at (m_buckets, next_bucket_idx).m_distAndFingerprint >= Bucket::distInc * 2)
        {
            auto& next_bucket = at (m_buckets, next_bucket_idx);
            at (m_buckets, bucket_idx) = { distDec (next_bucket.m_distAndFingerprint),
                                           next_bucket.m_valueIdx };
            bucket_idx = std::exchange (next_bucket_idx, next (next_bucket_idx));
        }
        at (m_buckets, bucket_idx) = {};
    }

    [[nodiscard]] static constexpr auto calcNumBuckets (std::uint8_t shifts) -> std::size_t
    {
        return (std::min) (max_bucket_count(), std::size_t { 1 } << (64U - shifts));
    }

    [[nodiscard]] constexpr auto calcShiftsForSize (std::size_t s) const -> std::uint8_t
    {
        auto shifts = initialShifts;
        while (shifts > 0
               and static_cast<std::size_t> (static_cast<float> (calcNumBuckets (shifts))
                                             * max_load_factor())
                       < s)
        {
            --shifts;
        }
        return shifts;
    }

    // assumes m_values has data, m_buckets=m_buckets_end=nullptr, m_shifts is INITIAL_SHIFTS
    void copyBuckets (const HashMap& other)
    {
        // assumes m_values has already the correct data copied over.
        if (empty())
        {
            // when empty, at least allocate an initial buckets and clear them.
            allocateBucketsFromShift();
            clearBuckets();
        }
        else
        {
            m_shifts = other.m_shifts;
            allocateBucketsFromShift();
            if constexpr (IsSegmented
                          or not std::is_same_v<BucketContainer, HashMapTraits::DefaultContainer>)
            {
                for (auto i = 0UL; i < bucket_count(); ++i)
                {
                    at (m_buckets, i) = at (other.m_buckets, i);
                }
            }
            else
            {
                std::memcpy (
                    m_buckets.data(), other.m_buckets.data(), sizeof (Bucket) * bucket_count());
            }
        }
    }

    /**
     * True when no element can be added any more without increasing the size
     */
    [[nodiscard]] auto isFull() const -> bool { return size() > m_maxBucketCapacity; }

    void deallocateBuckets()
    {
        m_buckets.clear();
        m_buckets.shrink_to_fit();
        m_maxBucketCapacity = 0;
    }

    void allocateBucketsFromShift()
    {
        auto num_buckets = calcNumBuckets (m_shifts);
        if constexpr (IsSegmented
                      or not std::is_same_v<BucketContainer, HashMapTraits::DefaultContainer>)
        {
            if constexpr (HashMapTraits::hasReserve<bucket_container_type>)
            {
                m_buckets.reserve (num_buckets);
            }
            for (std::size_t i = m_buckets.size(); i < num_buckets; ++i)
            {
                m_buckets.emplace_back();
            }
        }
        else
        {
            m_buckets.resize (num_buckets);
        }
        if (num_buckets == max_bucket_count())
        {
            // reached the maximum, make sure we can use each bucket
            m_maxBucketCapacity = max_bucket_count();
        }
        else
        {
            m_maxBucketCapacity =
                static_cast<value_idx_type> (static_cast<float> (num_buckets) * max_load_factor());
        }
    }

    void clearBuckets()
    {
        if constexpr (IsSegmented
                      or not std::is_same_v<BucketContainer, HashMapTraits::DefaultContainer>)
        {
            for (auto&& e : m_buckets)
            {
                std::memset (&e, 0, sizeof (e));
            }
        }
        else
        {
            std::memset (m_buckets.data(), 0, sizeof (Bucket) * bucket_count());
        }
    }

    void clearAndFillBucketsFromValues()
    {
        clearBuckets();
        for (value_idx_type value_idx = 0, end_idx = static_cast<value_idx_type> (m_values.size());
             value_idx < end_idx;
             ++value_idx)
        {
            const auto& key = getKey (m_values[value_idx]);
            auto [dist_and_fingerprint, bucket] = nextWhileLess (key);

            // we know for certain that key has not yet been inserted, so no need to check it.
            placeAndShiftUp ({ dist_and_fingerprint, value_idx }, bucket);
        }
    }

    void increaseSize()
    {
        if (m_maxBucketCapacity == max_bucket_count())
        {
            // remove the value again, we can't add it!
            m_values.pop_back();
            onBucketOverflow();
        }
        --m_shifts;
        if constexpr (not IsSegmented
                      or std::is_same_v<BucketContainer, HashMapTraits::DefaultContainer>)
        {
            deallocateBuckets();
        }
        allocateBucketsFromShift();
        clearAndFillBucketsFromValues();
    }

    template <typename EraseHandler>
    void doErase (value_idx_type bucket_idx, EraseHandler handle_erased_value)
    {
        auto const value_idx_to_remove = at (m_buckets, bucket_idx).m_valueIdx;
        eraseAndShiftDown (bucket_idx);
        handle_erased_value (std::move (m_values[value_idx_to_remove]));

        // update m_values
        if (value_idx_to_remove != m_values.size() - 1)
        {
            // no luck, we'll have to replace the value with the last one and update the index accordingly
            auto& val = m_values[value_idx_to_remove];
            val = std::move (m_values.back());

            // update the values_idx of the moved entry. No need to play the info game, just look until we find the values_idx
            bucket_idx = bucketIdxFromHash (mixedHash (getKey (val)));
            auto const values_idx_back = static_cast<value_idx_type> (m_values.size() - 1);
            while (values_idx_back != at (m_buckets, bucket_idx).m_valueIdx)
            {
                bucket_idx = next (bucket_idx);
            }
            at (m_buckets, bucket_idx).m_valueIdx = value_idx_to_remove;
        }
        m_values.pop_back();
    }

    template <typename KeyArg, typename EraseHandler>
    auto doEraseKey (KeyArg&& key, EraseHandler handle_erased_value) -> std::size_t
    {// NOLINT(cppcoreguidelines-missing-std-forward)
        if (empty())
        {
            return 0;
        }

        auto [dist_and_fingerprint, bucket_idx] = nextWhileLess (key);

        while (dist_and_fingerprint == at (m_buckets, bucket_idx).m_distAndFingerprint
               and not m_equal (key, getKey (m_values[at (m_buckets, bucket_idx).m_valueIdx])))
        {
            dist_and_fingerprint = distInc (dist_and_fingerprint);
            bucket_idx = next (bucket_idx);
        }

        if (dist_and_fingerprint != at (m_buckets, bucket_idx).m_distAndFingerprint)
        {
            return 0;
        }
        doErase (bucket_idx, handle_erased_value);
        return 1;
    }

    template <typename KeyArg, typename MappedArg>
    auto doInsertOrAssign (KeyArg&& key, MappedArg&& mapped) -> std::pair<iterator, bool>
    {
        auto [insertedIterator, wasInserted] =
            try_emplace (std::forward<KeyArg> (key), std::forward<MappedArg> (mapped));
        if (not wasInserted)
        {
            auto& [storedKey, storedMapped] = *insertedIterator;
            storedMapped = std::forward<MappedArg> (mapped);
        }
        return { insertedIterator, wasInserted };
    }

    template <typename... Args>
    auto doPlaceElement (dist_and_fingerprint_type dist_and_fingerprint,
                         value_idx_type bucket_idx,
                         Args&&... args) -> std::pair<iterator, bool>
    {
        // emplace the new value. If that throws an exception, no harm done; index is still in a valid state
        m_values.emplace_back (std::forward<Args> (args)...);

        auto value_idx = static_cast<value_idx_type> (m_values.size() - 1);
        if (JAM_UNLIKELY (isFull()))
        {
            increaseSize();
        }
        else
        {
            placeAndShiftUp ({ dist_and_fingerprint, value_idx }, bucket_idx);
        }

        // place element and shift up until we find an empty spot
        return { begin() + static_cast<difference_type> (value_idx), true };
    }

    template <typename KeyArg, typename... Args>
    auto doTryEmplace (KeyArg&& key, Args&&... args) -> std::pair<iterator, bool>
    {
        auto hash = mixedHash (key);
        auto dist_and_fingerprint = distAndFingerprintFromHash (hash);
        auto bucket_idx = bucketIdxFromHash (hash);

        while (true)
        {
            auto* bucket = &at (m_buckets, bucket_idx);
            if (dist_and_fingerprint == bucket->m_distAndFingerprint)
            {
                if (m_equal (key, getKey (m_values[bucket->m_valueIdx])))
                {
                    return { begin() + static_cast<difference_type> (bucket->m_valueIdx), false };
                }
            }
            else if (dist_and_fingerprint > bucket->m_distAndFingerprint)
            {
                return doPlaceElement (dist_and_fingerprint,
                                       bucket_idx,
                                       std::piecewise_construct,
                                       std::forward_as_tuple (std::forward<KeyArg> (key)),
                                       std::forward_as_tuple (std::forward<Args> (args)...));
            }
            dist_and_fingerprint = distInc (dist_and_fingerprint);
            bucket_idx = next (bucket_idx);
        }
    }

    template <typename KeyArg>
    auto doFind (const KeyArg& key) -> iterator
    {
        if (JAM_UNLIKELY (empty()))
        {
            return end();
        }

        auto mh = mixedHash (key);
        auto dist_and_fingerprint = distAndFingerprintFromHash (mh);
        auto bucket_idx = bucketIdxFromHash (mh);
        auto* bucket = &at (m_buckets, bucket_idx);

        // unrolled loop. *Always* check a few directly, then enter the loop. This is faster.
        if (dist_and_fingerprint == bucket->m_distAndFingerprint
            and m_equal (key, getKey (m_values[bucket->m_valueIdx])))
        {
            return begin() + static_cast<difference_type> (bucket->m_valueIdx);
        }
        dist_and_fingerprint = distInc (dist_and_fingerprint);
        bucket_idx = next (bucket_idx);
        bucket = &at (m_buckets, bucket_idx);

        if (dist_and_fingerprint == bucket->m_distAndFingerprint
            and m_equal (key, getKey (m_values[bucket->m_valueIdx])))
        {
            return begin() + static_cast<difference_type> (bucket->m_valueIdx);
        }
        dist_and_fingerprint = distInc (dist_and_fingerprint);
        bucket_idx = next (bucket_idx);
        bucket = &at (m_buckets, bucket_idx);

        while (true)
        {
            if (dist_and_fingerprint == bucket->m_distAndFingerprint)
            {
                if (m_equal (key, getKey (m_values[bucket->m_valueIdx])))
                {
                    return begin() + static_cast<difference_type> (bucket->m_valueIdx);
                }
            }
            else if (dist_and_fingerprint > bucket->m_distAndFingerprint)
            {
                return end();
            }
            dist_and_fingerprint = distInc (dist_and_fingerprint);
            bucket_idx = next (bucket_idx);
            bucket = &at (m_buckets, bucket_idx);
        }
    }

    template <typename KeyArg>
    auto doFind (const KeyArg& key) const -> const_iterator
    {
        return const_cast<HashMap*> (this)->doFind (
            key);// NOLINT(cppcoreguidelines-pro-type-const-cast)
    }

    template <typename KeyArg,
             typename MappedQ = Mapped,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto doAt (const KeyArg& key) -> MappedQ&
    {
        if (auto found { find (key) }; JAM_LIKELY (end() != found))
        {
            auto& [storedKey, storedMapped] = *found;
            return storedMapped;
        }
#if JUCE_DEBUG
        debug::Log::write ("jam::HashMap::at(): key not found:", key);
#endif
        onKeyNotFound();
    }

    template <typename KeyArg,
             typename MappedQ = Mapped,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto doAt (const KeyArg& key) const -> const MappedQ&
    {
        return const_cast<HashMap*> (this)->at (
            key);// NOLINT(cppcoreguidelines-pro-type-const-cast)
    }

public:
    /**
     * @brief Returns the key portion of a stored value_type.
     *
     * For a map (@c Mapped is a real mapped type) this is the first element
     * of the stored pair @p vt; for a set (@c Mapped is @c void) @p vt itself
     * is already the key.
     *
     * @param vt  Stored value to extract the key from.
     * @return    Reference to the key.
     */
    [[nodiscard]] static constexpr auto getKey (const value_type& vt) -> const key_type&
    {
        if constexpr (HashMapTraits::isMap<Mapped>)
        {
            const auto& [storedKey, storedMapped] = vt;
            return storedKey;
        }
        else
        {
            return vt;
        }
    }

    /**
     * @brief Construct with an initial bucket hint and explicit hasher,
     *        key-equality functor, and allocator.
     *
     * @param bucket_count  Minimum bucket count hint. The table rounds up to
     *                      a power of two that satisfies
     *                      @c bucket_count <= load_factor * num_buckets. Pass
     *                      0 to defer allocation until first insertion.
     * @param hash          Hasher to install.
     * @param equal         Key-equality functor to install.
     * @param alloc_or_container Allocator for the value and bucket containers.
     */
    explicit HashMap (std::size_t bucket_count,
                      const HashFn& hash = HashFn(),
                      const KeyEqual& equal = KeyEqual(),
                      const allocator_type& alloc_or_container = allocator_type())
        : m_values (alloc_or_container)
        , m_buckets (alloc_or_container)
        , m_hash (hash)
        , m_equal (equal)
    {
        if (0 != bucket_count)
        {
            reserve (bucket_count);
        }
        else
        {
            allocateBucketsFromShift();
            clearBuckets();
        }
    }

    /**
     * @brief Default-construct an empty table with no preallocated buckets.
     */
    HashMap()
        : HashMap (0)
    {
    }

    /**
     * @brief Construct with a bucket hint and allocator (default hasher/equal).
     *
     * @param bucket_count Minimum bucket count hint.
     * @param alloc        Allocator for the value and bucket containers.
     */
    HashMap (std::size_t bucket_count, const allocator_type& alloc)
        : HashMap (bucket_count, HashFn(), KeyEqual(), alloc)
    {
    }

    /**
     * @brief Construct with a bucket hint, hasher, and allocator.
     *
     * @param bucket_count Minimum bucket count hint.
     * @param hash         Hasher to install.
     * @param alloc        Allocator for the value and bucket containers.
     */
    HashMap (std::size_t bucket_count, const HashFn& hash, const allocator_type& alloc)
        : HashMap (bucket_count, hash, KeyEqual(), alloc)
    {
    }

    /**
     * @brief Construct an empty table with the given allocator.
     *
     * @param alloc Allocator for the value and bucket containers.
     */
    explicit HashMap (const allocator_type& alloc)
        : HashMap (0, HashFn(), KeyEqual(), alloc)
    {
    }

    /**
     * @brief Construct by inserting the elements of an input range.
     *
     * @tparam InputIt Input iterator type.
     * @param first         Beginning of the range to insert.
     * @param last          End of the range to insert.
     * @param bucket_count  Minimum bucket count hint.
     * @param hash          Hasher to install.
     * @param equal         Key-equality functor to install.
     * @param alloc         Allocator for the value and bucket containers.
     */
    template <typename InputIt>
    HashMap (InputIt first,
             InputIt last,
             size_type bucket_count = 0,
             const HashFn& hash = HashFn(),
             const KeyEqual& equal = KeyEqual(),
             const allocator_type& alloc = allocator_type())
        : HashMap (bucket_count, hash, equal, alloc)
    {
        insert (first, last);
    }

    /**
     * @brief Construct by inserting an input range, with bucket hint and allocator.
     *
     * @tparam InputIt Input iterator type.
     * @param first        Beginning of the range to insert.
     * @param last         End of the range to insert.
     * @param bucket_count Minimum bucket count hint.
     * @param alloc        Allocator for the value and bucket containers.
     */
    template <typename InputIt>
    HashMap (InputIt first, InputIt last, size_type bucket_count, const allocator_type& alloc)
        : HashMap (first, last, bucket_count, HashFn(), KeyEqual(), alloc)
    {
    }

    /**
     * @brief Construct by inserting an input range, with bucket hint, hasher, and allocator.
     *
     * @tparam InputIt Input iterator type.
     * @param first        Beginning of the range to insert.
     * @param last         End of the range to insert.
     * @param bucket_count Minimum bucket count hint.
     * @param hash         Hasher to install.
     * @param alloc        Allocator for the value and bucket containers.
     */
    template <typename InputIt>
    HashMap (InputIt first,
             InputIt last,
             size_type bucket_count,
             const HashFn& hash,
             const allocator_type& alloc)
        : HashMap (first, last, bucket_count, hash, KeyEqual(), alloc)
    {
    }

    /**
     * @brief Copy-construct from another table, propagating its allocator.
     *
     * @param other Source table.
     */
    HashMap (const HashMap& other)
        : HashMap (other, other.m_values.get_allocator())
    {
    }

    /**
     * @brief Copy-construct from another table with a different allocator.
     *
     * @param other Source table.
     * @param alloc Allocator for the new table's storage.
     */
    HashMap (const HashMap& other, const allocator_type& alloc)
        : m_values (other.m_values, alloc)
        , m_maxLoadFactor (other.m_maxLoadFactor)
        , m_hash (other.m_hash)
        , m_equal (other.m_equal)
    {
        copyBuckets (other);
    }

    /**
     * @brief Move-construct from another table, propagating its allocator.
     *
     * @param other Source table; left in a valid empty state.
     */
    HashMap (HashMap&& other) noexcept
        : HashMap (std::move (other), other.m_values.get_allocator())
    {
    }

    /**
     * @brief Move-construct from another table with a different allocator.
     *
     * @param other Source table; left in a valid empty state.
     * @param alloc Allocator for the new table's storage.
     */
    HashMap (HashMap&& other, const allocator_type& alloc) noexcept
        : m_values (alloc)
    {
        *this = std::move (other);
    }

    /**
     * @brief Construct from an initializer list of values.
     *
     * @param ilist        Initializer list of values to insert.
     * @param bucket_count Minimum bucket count hint.
     * @param hash         Hasher to install.
     * @param equal        Key-equality functor to install.
     * @param alloc        Allocator for the value and bucket containers.
     */
    HashMap (std::initializer_list<value_type> ilist,
             std::size_t bucket_count = 0,
             const HashFn& hash = HashFn(),
             const KeyEqual& equal = KeyEqual(),
             const allocator_type& alloc = allocator_type())
        : HashMap (bucket_count, hash, equal, alloc)
    {
        insert (ilist);
    }

    /**
     * @brief Construct from an initializer list with bucket hint and allocator.
     *
     * @param ilist        Initializer list of values to insert.
     * @param bucket_count Minimum bucket count hint.
     * @param alloc        Allocator for the value and bucket containers.
     */
    HashMap (std::initializer_list<value_type> ilist,
             size_type bucket_count,
             const allocator_type& alloc)
        : HashMap (ilist, bucket_count, HashFn(), KeyEqual(), alloc)
    {
    }

    /**
     * @brief Construct from an initializer list with bucket hint, hasher, and allocator.
     *
     * @param init         Initializer list of values to insert.
     * @param bucket_count Minimum bucket count hint.
     * @param hash         Hasher to install.
     * @param alloc        Allocator for the value and bucket containers.
     */
    HashMap (std::initializer_list<value_type> init,
             size_type bucket_count,
             const HashFn& hash,
             const allocator_type& alloc)
        : HashMap (init, bucket_count, hash, KeyEqual(), alloc)
    {
    }

    /**
     * @brief Destructor. Trivially destructible; defaults are used.
     */
    ~HashMap() = default;

    /**
     * @brief Copy-assign from another table.
     *
     * Replaces the current contents with a copy of @p other. Allocators
     * are preserved; if @p other uses a different allocator, the
     * bucket array is reallocated before the value container is replaced.
     *
     * @param other Source table.
     * @return @c *this.
     */
    auto operator= (const HashMap& other) -> HashMap&
    {
        if (&other != this)
        {
            deallocateBuckets();// deallocate before m_values is set (might have another allocator)
            m_values = other.m_values;
            m_maxLoadFactor = other.m_maxLoadFactor;
            m_hash = other.m_hash;
            m_equal = other.m_equal;
            m_shifts = initialShifts;
            copyBuckets (other);
        }
        return *this;
    }

    /**
     * @brief Move-assign from another table.
     *
     * Steals the contents of @p other when allocators match; otherwise
     * performs an element-wise move through the value container and
     * rebuilds the bucket array. In both cases @p other is left empty
     * but in a valid state.
     *
     * @param other Source table.
     * @return @c *this.
     */
    auto operator= (HashMap&& other) noexcept (
        noexcept (std::is_nothrow_move_assignable_v<value_container_type>
                  and std::is_nothrow_move_assignable_v<HashFn>
                  and std::is_nothrow_move_assignable_v<KeyEqual>)) -> HashMap&
    {
        if (&other != this)
        {
            deallocateBuckets();// deallocate before m_values is set (might have another allocator)
            m_values = std::move (other.m_values);
            other.m_values.clear();

            // we can only reuse m_buckets when both maps have the same allocator!
            if (get_allocator() == other.get_allocator())
            {
                m_buckets = std::move (other.m_buckets);
                other.m_buckets.clear();
                m_maxBucketCapacity = std::exchange (other.m_maxBucketCapacity, 0);
                m_shifts = std::exchange (other.m_shifts, initialShifts);
                m_maxLoadFactor = std::exchange (other.m_maxLoadFactor, defaultMaxLoadFactor);
                m_hash = std::exchange (other.m_hash, {});
                m_equal = std::exchange (other.m_equal, {});
                other.allocateBucketsFromShift();
                other.clearBuckets();
            }
            else
            {
                // set max_load_factor *before* copying the other's buckets, so we have the same
                // behavior
                m_maxLoadFactor = other.m_maxLoadFactor;

                // copyBuckets sets m_buckets, m_num_buckets, m_maxBucketCapacity, m_shifts
                copyBuckets (other);
                // clear's the other's buckets so other is now already usable.
                other.clearBuckets();
                m_hash = other.m_hash;
                m_equal = other.m_equal;
            }
            // map "other" is now already usable, it's empty.
        }
        return *this;
    }

    /**
     * @brief Replace the contents with the elements of an initializer list.
     *
     * Equivalent to @c clear() followed by @c insert(ilist).
     *
     * @param ilist Initializer list of values to insert.
     * @return @c *this.
     */
    auto operator= (std::initializer_list<value_type> ilist) -> HashMap&
    {
        clear();
        insert (ilist);
        return *this;
    }

    /**
     * @brief Return a copy of the allocator associated with the value container.
     *
     * @return The allocator.
     */
    auto get_allocator() const noexcept -> allocator_type { return m_values.get_allocator(); }

    // iterators //////////////////////////////////////////////////////////////

    /**
     * @brief Mutable iterator to the first element.
     */
    auto begin() noexcept -> iterator { return m_values.begin(); }

    /**
     * @brief Const iterator to the first element.
     */
    auto begin() const noexcept -> const_iterator { return m_values.begin(); }

    /**
     * @brief Const iterator to the first element.
     */
    auto cbegin() const noexcept -> const_iterator { return m_values.cbegin(); }

    /**
     * @brief Mutable past-the-end iterator.
     */
    auto end() noexcept -> iterator { return m_values.end(); }

    /**
     * @brief Const past-the-end iterator.
     */
    auto cend() const noexcept -> const_iterator { return m_values.cend(); }

    /**
     * @brief Const past-the-end iterator.
     */
    auto end() const noexcept -> const_iterator { return m_values.end(); }

    // capacity ///////////////////////////////////////////////////////////////

    /**
     * @brief True if the table contains no elements.
     *
     * @return @c true when @c size() == 0.
     */
    [[nodiscard]] auto empty() const noexcept -> bool { return m_values.empty(); }

    /**
     * @brief Number of elements currently stored.
     *
     * @return The element count.
     */
    [[nodiscard]] auto size() const noexcept -> std::size_t { return m_values.size(); }

    /**
     * @brief Maximum number of elements the table can hold.
     *
     * Bounded by the bucket index type, which is @c std::uint32_t for
     * @c BucketType::Standard and @c std::size_t for @c BucketType::Big.
     *
     * @return The maximum element count.
     */
    [[nodiscard]] static constexpr auto max_size() noexcept -> std::size_t
    {
        if constexpr ((std::numeric_limits<value_idx_type>::max)()
                      == (std::numeric_limits<std::size_t>::max)())
        {
            return std::size_t { 1 } << (sizeof (value_idx_type) * 8 - 1);
        }
        else
        {
            return std::size_t { 1 } << (sizeof (value_idx_type) * 8);
        }
    }

    // modifiers //////////////////////////////////////////////////////////////

    /**
     * @brief Remove all elements. Bucket array is preserved.
     *
     * After this call, @c size() == 0 and all iterators are invalidated.
     */
    void clear() noexcept
    {
        m_values.clear();
        clearBuckets();
    }

    /**
     * @brief Insert a copy of @p value if its key is not already present.
     *
     * @param value The value to insert.
     * @return Pair of (iterator to the inserted or existing element, true
     *         if insertion happened).
     */
    auto insert (const value_type& value) -> std::pair<iterator, bool> { return emplace (value); }

    /**
     * @brief Insert by moving @p value if its key is not already present.
     *
     * @param value The value to insert.
     * @return Pair of (iterator to the inserted or existing element, true
     *         if insertion happened).
     */
    auto insert (value_type&& value) -> std::pair<iterator, bool>
    {
        return emplace (std::move (value));
    }

    /**
     * @brief Insert a value that is convertible to @c value_type but is
     *        not necessarily a @c value_type itself.
     *
     * @tparam ValueArg Type convertible to @c value_type.
     * @param  value The value to insert.
     * @return Pair of (iterator, true if insertion happened).
     */
    template <typename ValueArg,
             std::enable_if_t<std::is_constructible_v<value_type, ValueArg&&>, bool> = true>
    auto insert (ValueArg&& value) -> std::pair<iterator, bool>
    {
        return emplace (std::forward<ValueArg> (value));
    }

    /**
     * @brief Hint-based insert of a copied value. The hint is currently ignored.
     *
     * @param value The value to insert.
     * @return Iterator to the inserted or existing element.
     */
    auto insert (const_iterator /*hint*/, const value_type& value) -> iterator
    {
        const auto [insertedIterator, wasInserted] = insert (value);
        return insertedIterator;
    }

    /**
     * @brief Hint-based insert of a moved value. The hint is currently ignored.
     *
     * @param value The value to insert.
     * @return Iterator to the inserted or existing element.
     */
    auto insert (const_iterator /*hint*/, value_type&& value) -> iterator
    {
        const auto [insertedIterator, wasInserted] = insert (std::move (value));
        return insertedIterator;
    }

    /**
     * @brief Hint-based insert of a value convertible to @c value_type.
     *        The hint is currently ignored.
     *
     * @tparam ValueArg Type convertible to @c value_type.
     * @param  value The value to insert.
     * @return Iterator to the inserted or existing element.
     */
    template <typename ValueArg,
             std::enable_if_t<std::is_constructible_v<value_type, ValueArg&&>, bool> = true>
    auto insert (const_iterator /*hint*/, ValueArg&& value) -> iterator
    {
        const auto [insertedIterator, wasInserted] = insert (std::forward<ValueArg> (value));
        return insertedIterator;
    }

    /**
     * @brief Insert every element of the input range.
     *
     * @tparam InputIt Input iterator type.
     * @param first Beginning of the range.
     * @param last  End of the range.
     */
    template <typename InputIt>
    void insert (InputIt first, InputIt last)
    {
        while (first != last)
        {
            insert (*first);
            ++first;
        }
    }

    /**
     * @brief Insert every element of an initializer list.
     *
     * @param ilist Initializer list of values to insert.
     */
    void insert (std::initializer_list<value_type> ilist) { insert (ilist.begin(), ilist.end()); }

    /**
     * @brief Nonstandard: empty the table and return the underlying value
     *        container by move.
     *
     * After this call @c *this is in a valid empty state. References a
     * caller receives to the returned container are independent of
     * the table; this is the inverse of @c replace.
     *
     * @return The moved-out value container.
     *
     */
    // Also see "A Standard flat_map" https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p0429r9.pdf
    auto extract() && -> value_container_type { return std::move (m_values); }

    /**
     * @brief Nonstandard: discard the internal value container and adopt
     *        the supplied one in its place.
     *
     * Duplicate keys within @p container are removed; the last occurrence
     * of each key wins. The container's allocator must match the table's
     * allocator.
     *
     * @param container The replacement value container; moved in.
     *
     * @throws std::out_of_range if @p container exceeds @c max_size().
     */
    // Discards the internally held container and replaces it with the one passed. Erases non-unique elements.
    auto replace (value_container_type&& container)
    {
        if (JAM_UNLIKELY (container.size() > max_size()))
        {
            onTooManyElements();
        }
        auto shifts = calcShiftsForSize (container.size());
        if (0 == bucket_count() or shifts < m_shifts
            or container.get_allocator() != m_values.get_allocator())
        {
            m_shifts = shifts;
            deallocateBuckets();
            allocateBucketsFromShift();
        }
        clearBuckets();

        m_values = std::move (container);

        // can't use clearAndFillBucketsFromValues() because container elements might not be unique
        auto value_idx = value_idx_type {};

        // loop until we reach the end of the container. duplicated entries will be replaced with back().
        while (value_idx != static_cast<value_idx_type> (m_values.size()))
        {
            const auto& key = getKey (m_values[value_idx]);

            auto hash = mixedHash (key);
            auto dist_and_fingerprint = distAndFingerprintFromHash (hash);
            auto bucket_idx = bucketIdxFromHash (hash);

            bool key_found { false };
            while (true)
            {
                const auto& bucket = at (m_buckets, bucket_idx);
                if (dist_and_fingerprint > bucket.m_distAndFingerprint)
                {
                    break;
                }
                if (dist_and_fingerprint == bucket.m_distAndFingerprint
                    and m_equal (key, getKey (m_values[bucket.m_valueIdx])))
                {
                    key_found = true;
                    break;
                }
                dist_and_fingerprint = distInc (dist_and_fingerprint);
                bucket_idx = next (bucket_idx);
            }

            if (key_found)
            {
                if (value_idx != static_cast<value_idx_type> (m_values.size() - 1))
                {
                    m_values[value_idx] = std::move (m_values.back());
                }
                m_values.pop_back();
            }
            else
            {
                placeAndShiftUp ({ dist_and_fingerprint, value_idx }, bucket_idx);
                ++value_idx;
            }
        }
    }

    /**
     * @brief Map-only: insert or assign a mapped value for @p key.
     *
     * If @p key is not yet present, the pair is inserted; otherwise the
     * existing mapped value is replaced with @p mapped.
     *
     * @tparam MappedArg    Mapped value type.
     * @param  key  The key to insert or look up.
     * @param  mapped The mapped value to assign.
     * @return Pair of (iterator, true if a new element was inserted).
     */
    template <typename MappedArg,
             typename MappedQ = Mapped,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto addOrReplace (const Key& key, MappedArg&& mapped) -> std::pair<iterator, bool>
    {
        return doInsertOrAssign (key, std::forward<MappedArg> (mapped));
    }

    /**
     * @brief Map-only: insert or assign a mapped value, moving @p key if it
     *        is a new element.
     *
     * @tparam MappedArg    Mapped value type.
     * @param  key  The key to insert or look up.
     * @param  mapped The mapped value to assign.
     * @return Pair of (iterator, true if a new element was inserted).
     */
    template <typename MappedArg,
             typename MappedQ = Mapped,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto addOrReplace (Key&& key, MappedArg&& mapped) -> std::pair<iterator, bool>
    {
        return doInsertOrAssign (std::move (key), std::forward<MappedArg> (mapped));
    }

    /**
     * @brief Map-only heterogeneous insert-or-assign, enabled when the
     *        hasher and key-equal are @c is_transparent.
     *
     * @tparam KeyArg   Heterogeneous key type.
     * @tparam MappedArg   Mapped value type.
     * @param  key  The key to insert or look up.
     * @param  mapped The mapped value to assign.
     * @return Pair of (iterator, true if a new element was inserted).
     */
    template <typename KeyArg,
             typename MappedArg,
             typename MappedQ = Mapped,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>
                                  and HashMapTraits::isTransparent<HashArg, KeyEqualArg>,
                              bool> = true>
    auto addOrReplace (KeyArg&& key, MappedArg&& mapped) -> std::pair<iterator, bool>
    {
        return doInsertOrAssign (std::forward<KeyArg> (key), std::forward<MappedArg> (mapped));
    }

    /**
     * @brief Map-only hint-based insert-or-assign. The hint is currently ignored.
     *
     * @tparam MappedArg    Mapped value type.
     * @param  key   The key to insert or look up.
     * @param  mapped The mapped value to assign.
     * @return Iterator to the inserted or updated element.
     */
    template <typename MappedArg,
             typename MappedQ = Mapped,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto addOrReplace (const_iterator /*hint*/, const Key& key, MappedArg&& mapped) -> iterator
    {
        const auto [insertedIterator, wasInserted] =
            doInsertOrAssign (key, std::forward<MappedArg> (mapped));
        return insertedIterator;
    }

    /**
     * @brief Map-only hint-based insert-or-assign with rvalue key. The hint
     *        is currently ignored.
     *
     * @tparam MappedArg    Mapped value type.
     * @param  key   The key to insert or look up.
     * @param  mapped The mapped value to assign.
     * @return Iterator to the inserted or updated element.
     */
    template <typename MappedArg,
             typename MappedQ = Mapped,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto addOrReplace (const_iterator /*hint*/, Key&& key, MappedArg&& mapped) -> iterator
    {
        const auto [insertedIterator, wasInserted] =
            doInsertOrAssign (std::move (key), std::forward<MappedArg> (mapped));
        return insertedIterator;
    }

    /**
     * @brief Map-only heterogeneous hint-based insert-or-assign.
     *
     * @tparam KeyArg   Heterogeneous key type.
     * @tparam MappedArg   Mapped value type.
     * @param  key   The key to insert or look up.
     * @param  mapped The mapped value to assign.
     * @return Iterator to the inserted or updated element.
     */
    template <typename KeyArg,
             typename MappedArg,
             typename MappedQ = Mapped,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>
                                  and HashMapTraits::isTransparent<HashArg, KeyEqualArg>,
                              bool> = true>
    auto addOrReplace (const_iterator /*hint*/, KeyArg&& key, MappedArg&& mapped) -> iterator
    {
        const auto [insertedIterator, wasInserted] =
            doInsertOrAssign (std::forward<KeyArg> (key), std::forward<MappedArg> (mapped));
        return insertedIterator;
    }

    /**
     * @brief Set-only: emplace a single key. Enabled when @c Mapped is @c void
     *        and the hasher / key-equal are @c is_transparent.
     *
     * Allows sets to be used without constructing a full @c value_type.
     *
     * @tparam KeyArg Heterogeneous key type.
     * @param  key The key to insert.
     * @return Pair of (iterator, true if insertion happened).
     */
    // Single arguments for unordered_set can be used without having to construct the value_type
    template <typename KeyArg,
             typename MappedQ = Mapped,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<not HashMapTraits::isMap<MappedQ>
                                  and HashMapTraits::isTransparent<HashArg, KeyEqualArg>,
                              bool> = true>
    auto emplace (KeyArg&& key) -> std::pair<iterator, bool>
    {
        auto hash = mixedHash (key);
        auto dist_and_fingerprint = distAndFingerprintFromHash (hash);
        auto bucket_idx = bucketIdxFromHash (hash);

        while (dist_and_fingerprint <= at (m_buckets, bucket_idx).m_distAndFingerprint)
        {
            if (dist_and_fingerprint == at (m_buckets, bucket_idx).m_distAndFingerprint
                and m_equal (key, m_values[at (m_buckets, bucket_idx).m_valueIdx]))
            {
                // found it, return without ever actually creating anything
                return { begin()
                             + static_cast<difference_type> (at (m_buckets, bucket_idx).m_valueIdx),
                         false };
            }
            dist_and_fingerprint = distInc (dist_and_fingerprint);
            bucket_idx = next (bucket_idx);
        }

        // value is new, insert element first, so when exception happens we are in a valid state
        return doPlaceElement (dist_and_fingerprint, bucket_idx, std::forward<KeyArg> (key));
    }

    /**
     * @brief In-place construct a new element.
     *
     * @c value_type is constructed in place from @p args. If the resulting
     * key is already present, the partially-constructed value is dropped
     * and no insertion takes place.
     *
     * @tparam Args Constructor argument types.
     * @param  args Arguments forwarded to the @c value_type constructor.
     * @return Pair of (iterator, true if insertion happened).
     */
    template <typename... Args>
    auto emplace (Args&&... args) -> std::pair<iterator, bool>
    {
        // we have to instantiate the value_type to be able to access the key.
        // 1. emplace_back the object so it is constructed. 2. If the key is already there, pop it later in the loop.
        auto& key = getKey (m_values.emplace_back (std::forward<Args> (args)...));
        auto hash = mixedHash (key);
        auto dist_and_fingerprint = distAndFingerprintFromHash (hash);
        auto bucket_idx = bucketIdxFromHash (hash);

        while (dist_and_fingerprint <= at (m_buckets, bucket_idx).m_distAndFingerprint)
        {
            if (dist_and_fingerprint == at (m_buckets, bucket_idx).m_distAndFingerprint
                and m_equal (key, getKey (m_values[at (m_buckets, bucket_idx).m_valueIdx])))
            {
                m_values.pop_back();// value was already there, so get rid of it
                return { begin()
                             + static_cast<difference_type> (at (m_buckets, bucket_idx).m_valueIdx),
                         false };
            }
            dist_and_fingerprint = distInc (dist_and_fingerprint);
            bucket_idx = next (bucket_idx);
        }

        // value is new, place the bucket and shift up until we find an empty spot
        auto value_idx = static_cast<value_idx_type> (m_values.size() - 1);
        if (JAM_UNLIKELY (isFull()))
        {
            // increaseSize just rehashes all the data we have in m_values
            increaseSize();
        }
        else
        {
            // place element and shift up until we find an empty spot
            placeAndShiftUp ({ dist_and_fingerprint, value_idx }, bucket_idx);
        }
        return { begin() + static_cast<difference_type> (value_idx), true };
    }

    /**
     * @brief Hint-based emplace. The hint is currently ignored.
     *
     * @tparam Args Constructor argument types.
     * @param  args Arguments forwarded to the @c value_type constructor.
     * @return Iterator to the inserted or existing element.
     */
    template <typename... Args>
    auto emplace_hint (const_iterator /*hint*/, Args&&... args) -> iterator
    {
        const auto [insertedIterator, wasInserted] = emplace (std::forward<Args> (args)...);
        return insertedIterator;
    }

    /**
     * @brief Map-only: insert a key/mapped pair if @p key is not present.
     *
     * If @p key already exists, the mapped value is left untouched and
     * the second member of the returned pair is @c false.
     *
     * @tparam Args Mapped value constructor argument types.
     * @param  key  The key to insert.
     * @param  args Arguments forwarded to the @c mapped_type constructor.
     * @return Pair of (iterator, true if insertion happened).
     */
    template <typename... Args,
             typename MappedQ = Mapped,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto try_emplace (const Key& key, Args&&... args) -> std::pair<iterator, bool>
    {
        return doTryEmplace (key, std::forward<Args> (args)...);
    }

    /**
     * @brief Map-only rvalue-key try_emplace.
     *
     * @tparam Args Mapped value constructor argument types.
     * @param  key  The key to insert (moved if it is a new element).
     * @param  args Arguments forwarded to the @c mapped_type constructor.
     * @return Pair of (iterator, true if insertion happened).
     */
    template <typename... Args,
             typename MappedQ = Mapped,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto try_emplace (Key&& key, Args&&... args) -> std::pair<iterator, bool>
    {
        return doTryEmplace (std::move (key), std::forward<Args> (args)...);
    }

    /**
     * @brief Map-only hint-based try_emplace. The hint is currently ignored.
     *
     * @tparam Args Mapped value constructor argument types.
     * @param  key  The key to insert.
     * @param  args Arguments forwarded to the @c mapped_type constructor.
     * @return Iterator to the inserted or existing element.
     */
    template <typename... Args,
             typename MappedQ = Mapped,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto try_emplace (const_iterator /*hint*/, const Key& key, Args&&... args) -> iterator
    {
        const auto [insertedIterator, wasInserted] = doTryEmplace (key, std::forward<Args> (args)...);
        return insertedIterator;
    }

    /**
     * @brief Map-only hint-based try_emplace with rvalue key. The hint
     *        is currently ignored.
     *
     * @tparam Args Mapped value constructor argument types.
     * @param  key  The key to insert (moved if new).
     * @param  args Arguments forwarded to the @c mapped_type constructor.
     * @return Iterator to the inserted or existing element.
     */
    template <typename... Args,
             typename MappedQ = Mapped,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto try_emplace (const_iterator /*hint*/, Key&& key, Args&&... args) -> iterator
    {
        const auto [insertedIterator, wasInserted] =
            doTryEmplace (std::move (key), std::forward<Args> (args)...);
        return insertedIterator;
    }

    /**
     * @brief Map-only heterogeneous try_emplace, enabled when the hasher
     *        and key-equal are @c is_transparent and @p K is not
     *        convertible to an iterator.
     *
     * @tparam KeyArg    Heterogeneous key type.
     * @tparam Args Mapped value constructor argument types.
     * @param  key  The key to insert.
     * @param  args Arguments forwarded to the @c mapped_type constructor.
     * @return Pair of (iterator, true if insertion happened).
     */
    template <typename KeyArg,
             typename... Args,
             typename MappedQ = Mapped,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<
                 HashMapTraits::isMap<MappedQ> and HashMapTraits::isTransparent<HashArg, KeyEqualArg>
                     and HashMapTraits::isNeitherConvertible<KeyArg&&, iterator, const_iterator>,
                 bool> = true>
    auto try_emplace (KeyArg&& key, Args&&... args) -> std::pair<iterator, bool>
    {
        return doTryEmplace (std::forward<KeyArg> (key), std::forward<Args> (args)...);
    }

    /**
     * @brief Map-only heterogeneous hint-based try_emplace. The hint is
     *        currently ignored.
     *
     * @tparam KeyArg    Heterogeneous key type.
     * @tparam Args Mapped value constructor argument types.
     * @param  key  The key to insert.
     * @param  args Arguments forwarded to the @c mapped_type constructor.
     * @return Iterator to the inserted or existing element.
     */
    template <typename KeyArg,
             typename... Args,
             typename MappedQ = Mapped,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<
                 HashMapTraits::isMap<MappedQ> and HashMapTraits::isTransparent<HashArg, KeyEqualArg>
                     and HashMapTraits::isNeitherConvertible<KeyArg&&, iterator, const_iterator>,
                 bool> = true>
    auto try_emplace (const_iterator /*hint*/, KeyArg&& key, Args&&... args) -> iterator
    {
        const auto [insertedIterator, wasInserted] =
            doTryEmplace (std::forward<KeyArg> (key), std::forward<Args> (args)...);
        return insertedIterator;
    }

    /**
     * @brief Replace the key at the given iterator with @p new_key.
     *
     * @details
     * - The value data is not moved, so all iterators and references
     *   remain valid.
     * - If @p new_key is already present in the table, the operation
     *   fails: no change is made and the function returns
     *   @c {iterator to the existing new_key, false}.
     * - In set mode this is more efficient than erase + insert because
     *   the last element does not need to be repositioned.
     *
     * @tparam KeyArg      Key type (may be the same as @c Key or a heterogeneous type).
     * @param  position     Iterator to the element whose key should be replaced.
     * @param  new_key The new key.
     * @return Pair of (iterator, true if the replacement succeeded).
     */
    // Replaces the key at the given iterator with new_key. This does not change any other data in the underlying table, so
    // all iterators and references remain valid. However, this operation can fail if new_key already exists in the table.
    // In that case, returns {iterator to the already existing new_key, false} and no change is made.
    //
    // In the case of a set, this effectively removes the old key and inserts the new key at the same spot, which is more
    // efficient than removing the old key and inserting the new key because it avoids repositioning the last element.
    template <typename KeyArg>
    auto replace_key (iterator position, KeyArg&& new_key) -> std::pair<iterator, bool>
    {
        auto const new_key_hash = mixedHash (new_key);

        // first, check if new_key already exists and return if so
        auto dist_and_fingerprint = distAndFingerprintFromHash (new_key_hash);
        auto bucket_idx = bucketIdxFromHash (new_key_hash);
        while (dist_and_fingerprint <= at (m_buckets, bucket_idx).m_distAndFingerprint)
        {
            const auto& bucket = at (m_buckets, bucket_idx);
            if (dist_and_fingerprint == bucket.m_distAndFingerprint
                and m_equal (new_key, getKey (m_values[bucket.m_valueIdx])))
            {
                return { begin() + static_cast<difference_type> (bucket.m_valueIdx), false };
            }
            dist_and_fingerprint = distInc (dist_and_fingerprint);
            bucket_idx = next (bucket_idx);
        }

        // const_cast is needed because iterator for the set is always const, so adding another getKey overload is not
        // feasible.
        auto& targetKey = const_cast<key_type&> (getKey (*position));
        auto const old_key_bucket_idx = bucketIdxFromHash (mixedHash (targetKey));

        // Replace the key before doing any bucket changes. If it throws, no harm done, we are still in a valid state as we
        // have not modified any buckets yet.
        targetKey = std::forward<KeyArg> (new_key);

        auto const value_idx = static_cast<value_idx_type> (position - begin());

        // Find the bucket containing our value_idx. It's guaranteed we find it, so no other stopping condition needed.
        bucket_idx = old_key_bucket_idx;
        while (value_idx != at (m_buckets, bucket_idx).m_valueIdx)
        {
            bucket_idx = next (bucket_idx);
        }
        eraseAndShiftDown (bucket_idx);

        // place the new bucket
        dist_and_fingerprint = distAndFingerprintFromHash (new_key_hash);
        bucket_idx = bucketIdxFromHash (new_key_hash);
        while (dist_and_fingerprint < at (m_buckets, bucket_idx).m_distAndFingerprint)
        {
            dist_and_fingerprint = distInc (dist_and_fingerprint);
            bucket_idx = next (bucket_idx);
        }
        placeAndShiftUp ({ dist_and_fingerprint, value_idx }, bucket_idx);

        return { position, true };
    }

    /**
     * @brief Erase the element at @p position.
     *
     * @param position Iterator to the element to erase.
     * @return Iterator to the element that now occupies the erased
     *         position, or @c end() if it was the last element.
     */
    auto erase (iterator position) -> iterator
    {
        auto hash = mixedHash (getKey (*position));
        auto bucket_idx = bucketIdxFromHash (hash);

        auto const value_idx_to_remove = static_cast<value_idx_type> (position - cbegin());
        while (at (m_buckets, bucket_idx).m_valueIdx != value_idx_to_remove)
        {
            bucket_idx = next (bucket_idx);
        }

        doErase (bucket_idx,
                 [] (const value_type& /*unused*/) -> void
                 {
                 });
        return begin() + static_cast<difference_type> (value_idx_to_remove);
    }

    /**
     * @brief Remove the element at @p position and return its value.
     *
     * @param position Iterator to the element to remove.
     * @return The removed value.
     */
    auto extract (iterator position) -> value_type
    {
        auto hash = mixedHash (getKey (*position));
        auto bucket_idx = bucketIdxFromHash (hash);

        auto const value_idx_to_remove = static_cast<value_idx_type> (position - cbegin());
        while (at (m_buckets, bucket_idx).m_valueIdx != value_idx_to_remove)
        {
            bucket_idx = next (bucket_idx);
        }

        auto tmp = std::optional<value_type> {};
        doErase (bucket_idx,
                 [&tmp] (value_type&& val) -> void
                 {
                     tmp = std::move (val);
                 });
        return std::move (tmp).value();
    }

    /**
     * @brief Map-only: erase the element at a const_iterator.
     *
     * @param position Const iterator to the element to erase.
     * @return Iterator to the element that now occupies the erased
     *         position, or @c end() if it was the last element.
     */
    template <typename MappedQ = Mapped, std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto erase (const_iterator position) -> iterator
    {
        return erase (begin() + (position - cbegin()));
    }

    /**
     * @brief Map-only: remove the element at a const_iterator and return its value.
     *
     * @param position Const iterator to the element to remove.
     * @return The removed value.
     */
    template <typename MappedQ = Mapped, std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto extract (const_iterator position) -> value_type
    {
        return extract (begin() + (position - cbegin()));
    }

    /**
     * @brief Erase the elements in the half-open range @p [first, last).
     *
     * The implementation sweeps from both ends of the range to minimise
     * the cost of swap-with-last on each erase.
     *
     * @param first First iterator in the range to erase.
     * @param last  One-past-the-last iterator in the range to erase.
     * @return Iterator to the element that now occupies the position of
     *         @p first, or @c end() if the table is empty.
     */
    auto erase (const_iterator first, const_iterator last) -> iterator
    {
        auto const idx_first = first - cbegin();
        auto const idx_last = last - cbegin();
        auto const first_to_last = std::distance (first, last);
        auto const last_to_end = std::distance (last, cend());

        // remove elements from left to right which moves elements from the end back
        auto const mid = idx_first + (std::min) (first_to_last, last_to_end);
        auto idx = idx_first;
        while (idx != mid)
        {
            erase (begin() + idx);
            ++idx;
        }

        // all elements from the right are moved, now remove the last element until all done
        idx = idx_last;
        while (idx != mid)
        {
            --idx;
            erase (begin() + idx);
        }

        return begin() + idx_first;
    }

    /**
     * @brief Erase the element whose key equals @p key, if any.
     *
     * @param key The key to remove.
     * @return Number of elements removed (0 or 1).
     */
    auto erase (const Key& key) -> std::size_t
    {
        return doEraseKey (key,
                           [] (const value_type& /*unused*/) -> void
                           {
                           });
    }

    /**
     * @brief Remove the element whose key equals @p key, if any, and return it.
     *
     * @param key The key to remove.
     * @return The removed value, or @c std::nullopt if no such key exists.
     */
    auto extract (const Key& key) -> std::optional<value_type>
    {
        auto tmp = std::optional<value_type> {};
        doEraseKey (key,
                    [&tmp] (value_type&& val) -> void
                    {
                        tmp = std::move (val);
                    });
        return tmp;
    }

    /**
     * @brief Heterogeneous erase by key, enabled when the hasher and
     *        key-equal are @c is_transparent.
     *
     * @tparam KeyArg   Heterogeneous key type.
     * @param  key The key to remove.
     * @return Number of elements removed (0 or 1).
     */
    template <typename KeyArg,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<HashMapTraits::isTransparent<HashArg, KeyEqualArg>, bool> = true>
    auto erase (KeyArg&& key) -> std::size_t
    {
        return doEraseKey (std::forward<KeyArg> (key),
                           [] (const value_type& /*unused*/) -> void
                           {
                           });
    }

    /**
     * @brief Heterogeneous extract by key, enabled when the hasher and
     *        key-equal are @c is_transparent.
     *
     * @tparam KeyArg   Heterogeneous key type.
     * @param  key The key to remove.
     * @return The removed value, or @c std::nullopt if no such key exists.
     */
    template <typename KeyArg,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<HashMapTraits::isTransparent<HashArg, KeyEqualArg>, bool> = true>
    auto extract (KeyArg&& key) -> std::optional<value_type>
    {
        auto tmp = std::optional<value_type> {};
        doEraseKey (std::forward<KeyArg> (key),
                    [&tmp] (value_type&& val) -> void
                    {
                        tmp = std::move (val);
                    });
        return tmp;
    }

    /**
     * @brief Swap contents with @p other. No iterators or references
     *        remain valid for either table.
     *
     * @param other Table to swap with.
     */
    void swap (HashMap& other) noexcept (noexcept (std::is_nothrow_swappable_v<value_container_type>
                                                   and std::is_nothrow_swappable_v<HashFn>
                                                   and std::is_nothrow_swappable_v<KeyEqual>))
    {
        using std::swap;
        swap (other, *this);
    }

    // lookup /////////////////////////////////////////////////////////////////

    /**
     * @brief Map-only: access the mapped value for @p key with bounds checking.
     *
     * @param key The key to look up.
     * @return Reference to the mapped value.
     * @throws std::out_of_range if @p key is not present.
     */
    template <typename MappedQ = Mapped, std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto at (const key_type& key) -> MappedQ&
    {
        return doAt (key);
    }

    /**
     * @brief Map-only heterogeneous @c at, enabled when the hasher and
     *        key-equal are @c is_transparent.
     *
     * @tparam KeyArg Heterogeneous key type.
     * @param  key The key to look up.
     * @return Reference to the mapped value.
     * @throws std::out_of_range if @p key is not present.
     */
    template <typename KeyArg,
             typename MappedQ = Mapped,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>
                                  and HashMapTraits::isTransparent<HashArg, KeyEqualArg>,
                              bool> = true>
    auto at (const KeyArg& key) -> MappedQ&
    {
        return doAt (key);
    }

    /**
     * @brief Map-only const @c at.
     *
     * @param key The key to look up.
     * @return Const reference to the mapped value.
     * @throws std::out_of_range if @p key is not present.
     */
    template <typename MappedQ = Mapped, std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto at (const key_type& key) const -> const MappedQ&
    {
        return doAt (key);
    }

    /**
     * @brief Map-only const heterogeneous @c at.
     *
     * @tparam KeyArg Heterogeneous key type.
     * @param  key The key to look up.
     * @return Const reference to the mapped value.
     * @throws std::out_of_range if @p key is not present.
     */
    template <typename KeyArg,
             typename MappedQ = Mapped,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>
                                  and HashMapTraits::isTransparent<HashArg, KeyEqualArg>,
                              bool> = true>
    auto at (const KeyArg& key) const -> const MappedQ&
    {
        return doAt (key);
    }

    /**
     * @brief Map-only: access the mapped value for @p key, defaulting on miss.
     *
     * Unlike @c at, this never throws — a missing @p key yields a
     * default-constructed @c MappedQ instead of @c std::out_of_range.
     *
     * @param key The key to look up.
     * @return Copy of the mapped value, or a default-constructed value if
     *         @p key is not present.
     */
    template <typename MappedQ = Mapped, std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto get (const key_type& key) const noexcept -> MappedQ
    {
        const auto found { find (key) };
        if (end() != found)
        {
            const auto& [storedKey, storedMapped] = *found;
            return storedMapped;
        }
        return MappedQ {};
    }

    /**
     * @brief Map-only heterogeneous: access the mapped value for @p key,
     *        defaulting on miss, enabled when the hasher and key-equal are
     *        @c is_transparent.
     *
     * Unlike @c at, this never throws — a missing @p key yields a
     * default-constructed @c MappedQ instead of @c std::out_of_range.
     *
     * @tparam KeyArg Heterogeneous key type.
     * @param  key The key to look up.
     * @return Copy of the mapped value, or a default-constructed value if
     *         @p key is not present.
     */
    template <typename KeyArg,
             typename MappedQ = Mapped,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>
                                  and HashMapTraits::isTransparent<HashArg, KeyEqualArg>,
                              bool> = true>
    auto get (const KeyArg& key) const noexcept -> MappedQ
    {
        const auto found { find (key) };
        if (end() != found)
        {
            const auto& [storedKey, storedMapped] = *found;
            return storedMapped;
        }
        return MappedQ {};
    }

    /**
     * @brief Map-only: insert-or-access the mapped value for @p key.
     *
     * If @p key is not present, a default-constructed mapped value is
     * inserted. Returns a reference to the mapped value either way.
     *
     * @param key The key to look up.
     * @return Reference to the mapped value.
     */
    template <typename MappedQ = Mapped, std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto operator[] (const Key& key) -> MappedQ&
    {
        const auto [insertedIterator, wasInserted] = try_emplace (key);
        auto& [storedKey, storedMapped] = *insertedIterator;
        return storedMapped;
    }

    /**
     * @brief Map-only rvalue-key operator[].
     *
     * @param key The key to look up (moved if a new element is inserted).
     * @return Reference to the mapped value.
     */
    template <typename MappedQ = Mapped, std::enable_if_t<HashMapTraits::isMap<MappedQ>, bool> = true>
    auto operator[] (Key&& key) -> MappedQ&
    {
        const auto [insertedIterator, wasInserted] = try_emplace (std::move (key));
        auto& [storedKey, storedMapped] = *insertedIterator;
        return storedMapped;
    }

    /**
     * @brief Map-only heterogeneous operator[], enabled when the hasher
     *        and key-equal are @c is_transparent.
     *
     * @tparam KeyArg Heterogeneous key type.
     * @param  key The key to look up.
     * @return Reference to the mapped value.
     */
    template <typename KeyArg,
             typename MappedQ = Mapped,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<HashMapTraits::isMap<MappedQ>
                                  and HashMapTraits::isTransparent<HashArg, KeyEqualArg>,
                              bool> = true>
    auto operator[] (KeyArg&& key) -> MappedQ&
    {
        const auto [insertedIterator, wasInserted] = try_emplace (std::forward<KeyArg> (key));
        auto& [storedKey, storedMapped] = *insertedIterator;
        return storedMapped;
    }

    /**
     * @brief Return the number of elements matching @p key (0 or 1).
     *
     * @param key The key to count.
     * @return 0 or 1.
     */
    auto count (const Key& key) const -> std::size_t { return find (key) == end() ? 0 : 1; }


    /**
     * @brief Heterogeneous @c count, enabled when the hasher and key-equal
     *        are @c is_transparent.
     *
     * @tparam KeyArg   Heterogeneous key type.
     * @param  key The key to count.
     * @return 0 or 1.
     */
    template <typename KeyArg,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<HashMapTraits::isTransparent<HashArg, KeyEqualArg>, bool> = true>
    auto count (const KeyArg& key) const -> std::size_t
    {
        return find (key) == end() ? 0 : 1;
    }

    /**
     * @brief Find the element whose key equals @p key.
     *
     * @param key The key to look up.
     * @return Iterator to the element, or @c end() if not found.
     */
    auto find (const Key& key) -> iterator { return doFind (key); }

    /**
     * @brief Const overload of @c find.
     *
     * @param key The key to look up.
     * @return Const iterator to the element, or @c end() if not found.
     */
    auto find (const Key& key) const -> const_iterator { return doFind (key); }

    /**
     * @brief Heterogeneous @c find, enabled when the hasher and key-equal
     *        are @c is_transparent.
     *
     * @tparam KeyArg   Heterogeneous key type.
     * @param  key The key to look up.
     * @return Iterator to the element, or @c end() if not found.
     */
    template <typename KeyArg,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<HashMapTraits::isTransparent<HashArg, KeyEqualArg>, bool> = true>
    auto find (const KeyArg& key) -> iterator
    {
        return doFind (key);
    }

    /**
     * @brief Heterogeneous const @c find.
     *
     * @tparam KeyArg   Heterogeneous key type.
     * @param  key The key to look up.
     * @return Const iterator to the element, or @c end() if not found.
     */
    template <typename KeyArg,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<HashMapTraits::isTransparent<HashArg, KeyEqualArg>, bool> = true>
    auto find (const KeyArg& key) const -> const_iterator
    {
        return doFind (key);
    }

    /**
     * @brief Test whether the table contains an element with the given key.
     *
     * @param key The key to test for.
     * @return @c true if such an element exists.
     */
    auto contains (const Key& key) const -> bool { return find (key) != end(); }

    /**
     * @brief Heterogeneous @c contains.
     *
     * @tparam KeyArg   Heterogeneous key type.
     * @param  key The key to test for.
     * @return @c true if such an element exists.
     */
    template <typename KeyArg,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<HashMapTraits::isTransparent<HashArg, KeyEqualArg>, bool> = true>
    auto contains (const KeyArg& key) const -> bool
    {
        return find (key) != end();
    }

    /**
     * @brief Range of elements matching @p key.
     *
     * Because keys are unique, the range contains 0 or 1 element.
     *
     * @param key The key to look up.
     * @return Pair of iterators @c [first, second) into the table; empty
     *         if @p key is not present.
     */
    auto equal_range (const Key& key) -> std::pair<iterator, iterator>
    {
        auto found { doFind (key) };
        return { found, found == end() ? end() : found + 1 };
    }

    /**
     * @brief Const overload of @c equal_range.
     *
     * @param key The key to look up.
     * @return Pair of const iterators spanning the (0-or-1) range.
     */
    auto equal_range (const Key& key) const -> std::pair<const_iterator, const_iterator>
    {
        auto found { doFind (key) };
        return { found, found == end() ? end() : found + 1 };
    }

    /**
     * @brief Heterogeneous @c equal_range.
     *
     * @tparam KeyArg   Heterogeneous key type.
     * @param  key The key to look up.
     * @return Pair of iterators spanning the (0-or-1) range.
     */
    template <typename KeyArg,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<HashMapTraits::isTransparent<HashArg, KeyEqualArg>, bool> = true>
    auto equal_range (const KeyArg& key) -> std::pair<iterator, iterator>
    {
        auto found { doFind (key) };
        return { found, found == end() ? end() : found + 1 };
    }

    /**
     * @brief Heterogeneous const @c equal_range.
     *
     * @tparam KeyArg   Heterogeneous key type.
     * @param  key The key to look up.
     * @return Pair of const iterators spanning the (0-or-1) range.
     */
    template <typename KeyArg,
             typename HashArg = HashFn,
             typename KeyEqualArg = KeyEqual,
             std::enable_if_t<HashMapTraits::isTransparent<HashArg, KeyEqualArg>, bool> = true>
    auto equal_range (const KeyArg& key) const -> std::pair<const_iterator, const_iterator>
    {
        auto found { doFind (key) };
        return { found, found == end() ? end() : found + 1 };
    }

    // bucket interface ///////////////////////////////////////////////////////

    /**
     * @brief Current number of buckets in the table.
     *
     * @return Bucket count.
     */
    auto bucket_count() const noexcept -> std::size_t
    {// NOLINT(modernize-use-nodiscard)
        return m_buckets.size();
    }

    /**
     * @brief Maximum possible bucket count, equal to @c max_size().
     *
     * @return Maximum bucket count.
     */
    static constexpr auto max_bucket_count() noexcept -> std::size_t
    {// NOLINT(modernize-use-nodiscard)
        return max_size();
    }

    // hash policy ////////////////////////////////////////////////////////////

    /**
     * @brief Current load factor: @c size() / @c bucket_count().
     *
     * @return Load factor as a float; 0.0 if there are no buckets.
     */
    [[nodiscard]] auto load_factor() const -> float
    {
        return bucket_count() ? static_cast<float> (size()) / static_cast<float> (bucket_count())
                              : 0.0F;
    }

    /**
     * @brief Current maximum load factor setting.
     *
     * @return The maximum load factor as a float.
     */
    [[nodiscard]] auto max_load_factor() const -> float { return m_maxLoadFactor; }

    /**
     * @brief Set a new maximum load factor.
     *
     * @param ml New maximum load factor.
     */
    void max_load_factor (float ml)
    {
        m_maxLoadFactor = ml;
        if (bucket_count() != max_bucket_count())
        {
            m_maxBucketCapacity = static_cast<value_idx_type> (static_cast<float> (bucket_count())
                                                               * max_load_factor());
        }
    }

    /**
     * @brief Rehash so that the bucket count is at least @p count.
     *
     * @param count Minimum desired bucket count. Clamped to @c max_size().
     */
    void rehash (std::size_t count)
    {
        count = (std::min) (count, max_size());
        auto shifts = calcShiftsForSize ((std::max) (count, size()));
        if (shifts != m_shifts)
        {
            m_shifts = shifts;
            deallocateBuckets();
            m_values.shrink_to_fit();
            allocateBucketsFromShift();
            clearAndFillBucketsFromValues();
        }
    }

    /**
     * @brief Reserve space for at least @p capa elements.
     *
     * Pre-allocates both the value container and a sufficient bucket
     * array to hold @p capa elements without rehashing.
     *
     * @param capa Minimum capacity. Clamped to @c max_size().
     */
    void reserve (std::size_t capa)
    {
        capa = (std::min) (capa, max_size());
        if constexpr (HashMapTraits::hasReserve<value_container_type>)
        {
            // std::deque doesn't have reserve(). Make sure we only call when available
            m_values.reserve (capa);
        }
        auto shifts = calcShiftsForSize ((std::max) (capa, size()));
        if (0 == bucket_count() or shifts < m_shifts)
        {
            m_shifts = shifts;
            deallocateBuckets();
            allocateBucketsFromShift();
            clearAndFillBucketsFromValues();
        }
    }

    // observers //////////////////////////////////////////////////////////////

    /**
     * @brief Return a copy of the hasher functor in use.
     *
     * @return The hasher.
     */
    auto hash_function() const -> hasher { return m_hash; }

    /**
     * @brief Return a copy of the key-equality functor in use.
     *
     * @return The key-equality functor.
     */
    auto key_eq() const -> key_equal { return m_equal; }

    /**
     * @brief Nonstandard: read-only access to the underlying dense value container.
     *
     * @return Const reference to the value container.
     */
    [[nodiscard]] auto values() const noexcept -> const value_container_type& { return m_values; }
};

/**
 * @brief Compare two tables for equality of contents.
 *
 * For maps, both keys and mapped values are compared. For sets, only
 * the keys are compared. The order of elements does not matter.
 *
 * @param a Left-hand table.
 * @param b Right-hand table.
 * @return @c true if the tables contain the same key/value pairs.
 */
template <typename Key,
         typename Mapped,
         typename HashFn,
         typename KeyEqual,
         typename AllocatorOrContainer,
         typename Bucket,
         typename BucketContainer,
         bool IsSegmented>
auto operator== (
    const HashMap<Key, Mapped, HashFn, KeyEqual, AllocatorOrContainer, Bucket, BucketContainer, IsSegmented>&
        a,
    const HashMap<Key, Mapped, HashFn, KeyEqual, AllocatorOrContainer, Bucket, BucketContainer, IsSegmented>&
        b) -> bool
{
    if (&a == &b)
    {
        return true;
    }
    if (a.size() != b.size())
    {
        return false;
    }
    for (const auto& b_entry : b)
    {
        auto foundEntry { a.find (
            HashMap<Key, Mapped, HashFn, KeyEqual, AllocatorOrContainer, Bucket, BucketContainer, IsSegmented>::
                getKey (b_entry)) };
        if constexpr (HashMapTraits::isMap<Mapped>)
        {
            // map: check that key is here, then also check that value is the same
            if (a.end() == foundEntry)
            {
                return false;
            }
            const auto& [entryKey, entryMapped] = b_entry;
            const auto& [foundKey, foundMapped] = *foundEntry;
            if (not(entryMapped == foundMapped))
            {
                return false;
            }
        }
        else
        {
            // set: only check that the key is here
            if (a.end() == foundEntry)
            {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief Inverse of @c operator==.
 *
 * @param a Left-hand table.
 * @param b Right-hand table.
 * @return @c true if the tables differ in at least one key/value pair.
 */
template <typename Key,
         typename Mapped,
         typename HashFn,
         typename KeyEqual,
         typename AllocatorOrContainer,
         typename Bucket,
         typename BucketContainer,
         bool IsSegmented>
auto operator!= (
    const HashMap<Key, Mapped, HashFn, KeyEqual, AllocatorOrContainer, Bucket, BucketContainer, IsSegmented>&
        a,
    const HashMap<Key, Mapped, HashFn, KeyEqual, AllocatorOrContainer, Bucket, BucketContainer, IsSegmented>&
        b) -> bool
{
    return not(a == b);
}

/**
 * @brief Map alias backed by a @c SegmentedVector, for large value types
 *        where segment-by-segment allocation beats a single contiguous grow.
 *
 * Prefer this over @c HashMap when @c value_type is large and the table
 * grows in irregular bursts; segment-level allocation avoids the
 * whole-vector reallocations of a plain @c std::vector.
 *
 * @tparam Key                 Key type.
 * @tparam Mapped              Mapped value type.
 * @tparam HashFn              Hasher functor.
 * @tparam KeyEqual            Key-equality functor.
 * @tparam AllocatorOrContainer Allocator for value pairs.
 * @tparam Bucket              Bucket layout (Standard or Big).
 * @tparam BucketContainer     Container used for the bucket array.
 *
 * @par Example
 * @code
 * jam::SegmentedHashMap<int, std::vector<float>> series;
 * series[0] = std::vector<float>(1024, 0.0F);
 * series.reserve(1024); // pre-allocates segmented storage
 * @endcode
 */
template <typename Key,
         typename Mapped,
         typename HashFn = Hash<Key>,
         typename KeyEqual = std::equal_to<Key>,
         typename AllocatorOrContainer = std::allocator<std::pair<Key, Mapped>>,
         typename Bucket = BucketType::Standard,
         typename BucketContainer = HashMapTraits::DefaultContainer>
using SegmentedHashMap =
    HashMap<Key, Mapped, HashFn, KeyEqual, AllocatorOrContainer, Bucket, BucketContainer, true>;

/**
 * @brief Standard set alias: dense @c std::vector-backed HashMap
 *        specialised to hold only keys.
 *
 * @tparam Key                 Key type.
 * @tparam HashFn              Hasher functor.
 * @tparam KeyEqual            Key-equality functor.
 * @tparam AllocatorOrContainer Allocator for keys, or a custom values container.
 * @tparam Bucket              Bucket layout (Standard or Big).
 * @tparam BucketContainer     Container used for the bucket array.
 *
 * @par Example
 * @code
 * jam::HashSet<juce::Identifier> activeTags { juce::Identifier("gain"),
 *                                             juce::Identifier("pan") };
 * if (activeTags.contains(juce::Identifier("gain"))) { ... }
 * @endcode
 */
template <typename Key,
         typename HashFn = Hash<Key>,
         typename KeyEqual = std::equal_to<Key>,
         typename AllocatorOrContainer = std::allocator<Key>,
         typename Bucket = BucketType::Standard,
         typename BucketContainer = HashMapTraits::DefaultContainer>
using HashSet =
    HashMap<Key, void, HashFn, KeyEqual, AllocatorOrContainer, Bucket, BucketContainer, false>;

/**
 * @brief Set alias backed by a @c SegmentedVector. Use for large key types
 *        or for large sets that grow incrementally.
 *
 * @tparam Key                 Key type.
 * @tparam HashFn              Hasher functor.
 * @tparam KeyEqual            Key-equality functor.
 * @tparam AllocatorOrContainer Allocator for keys.
 * @tparam Bucket              Bucket layout (Standard or Big).
 * @tparam BucketContainer     Container used for the bucket array.
 *
 * @par Example
 * @code
 * jam::SegmentedHashSet<std::string> dictionary;
 * dictionary.emplace("apple");
 * dictionary.emplace("banana");
 * @endcode
 */
template <typename Key,
         typename HashFn = Hash<Key>,
         typename KeyEqual = std::equal_to<Key>,
         typename AllocatorOrContainer = std::allocator<Key>,
         typename Bucket = BucketType::Standard,
         typename BucketContainer = HashMapTraits::DefaultContainer>
using SegmentedHashSet =
    HashMap<Key, void, HashFn, KeyEqual, AllocatorOrContainer, Bucket, BucketContainer, true>;

// deduction guides ///////////////////////////////////////////////////////////

// deduction guides for alias templates are only possible since C++20
// see https://en.cppreference.com/w/cpp/language/class_template_argument_deduction

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
