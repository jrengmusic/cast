/**
 * @file        jam_Union.h
 * @brief       Variadic trivially copyable packed transport type.
 */
namespace jam
{
/*____________________________________________________________________________*/

/** @brief Variadic trivially copyable packed transport type.
 *
 *  Packs 2-4 values of mixed trivially copyable types into a single
 *  uint32_t or uint64_t. Transport only — not storage. Pack on write,
 *  unpack on read.
 *
 *  Backing auto-selected: sum of sizeof(Ts) <= 4 → uint32_t,
 *  <= 8 → uint64_t, > 8 → static_assert fail.
 *
 *  @code
 *  // Pack width + height
 *  auto size = jam::Union<int32_t, int32_t>::pack (1920, 1080);
 *  auto [w, h] = size;  // structured binding
 *  auto w = size.unpack<0>();
 *  auto h = size.unpack<1>();
 *
 *  // Pack colour + blur radius
 *  auto glass = jam::Union<uint32_t, float>::pack (colour.getARGB(), 12.0f);
 *  auto argb  = glass.unpack<0>();
 *  auto blur  = glass.unpack<1>();
 *
 *  // Transport via ValueTree (int64)
 *  state.setProperty (Id::size, static_cast<juce::int64> (size.bits), nullptr);
 *  auto restored = jam::Union<int32_t, int32_t> { static_cast<uint64_t> (state[Id::size]) };
 *  @endcode
 *
 *  @tparam Ts  2–4 trivially copyable types whose total sizeof <= 8.
 */
template <typename... Ts>
struct Union
{
    // --- Constraints ---
    static_assert (sizeof...(Ts) >= 2, "Union requires at least 2 types");
    static_assert (sizeof...(Ts) <= 4, "Union supports at most 4 types");
    static_assert ((sizeof (Ts) + ...) <= 8, "Total size exceeds 8 bytes (uint64_t)");
    static_assert ((std::is_trivially_copyable_v<Ts> and ...),
                   "All types must be trivially copyable");

    // --- Backing type ---

    /** @brief Backing integer type: uint32_t when total size <= 4, uint64_t otherwise. */
    using Backing = std::conditional_t<(sizeof (Ts) + ...) <= 4, uint32_t, uint64_t>;

    // --- Public nested helpers (required by std::tuple_element specialisation) ---

    /** @brief Maps index I to the type at that position in Ts. */
    template <size_t I>
    struct TypeAt
    {
        template <size_t Remaining, typename Head, typename... Tail>
        struct Impl
        {
            using type = typename Impl<Remaining - 1, Tail...>::type;
        };

        template <typename Head, typename... Tail>
        struct Impl<0, Head, Tail...>
        {
            using type = Head;
        };

        using type = typename Impl<I, Ts...>::type;
    };

    // --- Data ---

    /** @brief Packed backing word. */
    Backing bits { 0 };

    // --- Construction ---

    /** @brief Pack values into a Union.
     *
     *  Each value is reinterpreted as its same-sized unsigned integer and
     *  shifted into position by byte offset.
     *
     *  @param values  One value per type in Ts, in declaration order.
     *  @return        Packed Union.
     */
    static constexpr Union pack (Ts... values) noexcept
    {
        return packImpl (std::index_sequence_for<Ts...> {}, values...);
    }

    /** @brief Construct from raw backing word (ValueTree round-trip).
     *
     *  @param raw  Backing word previously obtained from bits.
     */
    constexpr explicit Union (Backing raw) noexcept
        : bits (raw)
    {
    }

    /** @brief Default-constructed Union with all bits zero. */
    constexpr Union() noexcept = default;

    // --- Unpack ---

    /** @brief Extract the value at index I.
     *
     *  Shifts the backing word right by the byte offset of I and
     *  reinterprets the low bytes as the type at I.
     *
     *  @tparam I  Zero-based index into Ts.
     *  @return    Value of type Ts[I] extracted from bits.
     */
    template <size_t I>
    constexpr auto unpack() const noexcept
    {
        using ValueType = typename TypeAt<I>::type;
        using UintType = UintForSize<ValueType>;

        constexpr size_t byteOffset { ByteOffset<I>::value };
        auto raw { static_cast<UintType> (bits >> (byteOffset * 8)) };

        return jam::bit_cast<ValueType> (raw);
    }

private:
    // --- Private helpers ---

    /** @brief Compile-time byte offset of type at index I in the backing word. */
    template <size_t I>
    struct ByteOffset
    {
        template <size_t Remaining, typename Head, typename... Tail>
        struct Impl
        {
            static constexpr size_t value { sizeof (Head) + Impl<Remaining - 1, Tail...>::value };
        };

        template <typename Head, typename... Tail>
        struct Impl<0, Head, Tail...>
        {
            static constexpr size_t value { 0 };
        };

        static constexpr size_t value { Impl<I, Ts...>::value };
    };

    /** @brief Maps a type to the unsigned integer type of the same byte width. */
    template <typename ValueType>
    using UintForSize = std::conditional_t<
        sizeof (ValueType) == 1,
        uint8_t,
        std::conditional_t<sizeof (ValueType) == 2,
                           uint16_t,
                           std::conditional_t<sizeof (ValueType) == 4, uint32_t, uint64_t>>>;

    /** @brief Index-sequence-driven pack implementation.
     *
     *  Packs each value at its compile-time byte offset using a fold expression.
     *
     *  @tparam Is       Index pack matching Ts.
     *  @param  values   Values to pack, one per type in Ts.
     *  @return          Packed Union.
     */
    template <size_t... Is>
    static constexpr Union packImpl (std::index_sequence<Is...>, Ts... values) noexcept
    {
        return Union { static_cast<Backing> (
            ((static_cast<Backing> (jam::bit_cast<UintForSize<Ts>> (values))
                  << (ByteOffset<Is>::value * 8))
             | ...)) };
    }
};

static_assert (sizeof (Union<int16_t, int16_t>) == 4);
static_assert (sizeof (Union<int32_t, int32_t>) == 8);
static_assert (sizeof (Union<uint32_t, float>) == 8);
static_assert (std::is_trivially_copyable_v<Union<int32_t, int32_t>>);

/** @brief ADL get() for structured binding support.
     *
     *  @tparam I  Zero-based index into the Union's type list.
     *  @param  u  Union to extract from.
     *  @return    Value at index I.
     */
template <size_t I, typename... Ts>
constexpr auto get (const Union<Ts...>& u) noexcept
{
    return u.template unpack<I>();
}
/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam

//==============================================================================
// --- Structured binding support ---
namespace std
{
/*____________________________________________________________________________*/
/** @brief Specialisation of tuple_size for jam::Union. */
template <typename... Ts>
struct tuple_size<jam::Union<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)>
{
};

/** @brief Specialisation of tuple_element for jam::Union. */
template <size_t I, typename... Ts>
struct tuple_element<I, jam::Union<Ts...>>
{
    using type = typename jam::Union<Ts...>::template TypeAt<I>::type;
};
/**______________________________END OF NAMESPACE______________________________*/
}// namespace std
