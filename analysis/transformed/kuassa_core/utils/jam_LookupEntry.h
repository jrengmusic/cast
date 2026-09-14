/**
 * @file        jam_LookupEntry.h
 * @brief       Row type + capacity derivation for the `LookupTable` canon SSOT LUT pattern.
 */
namespace jam
{ /*____________________________________________________________________________*/

/**
 * @struct LookupEntry
 * @brief One key/value row — the constructor argument shape for `LookupTable`.
 *
 * @tparam Key    Integral key type. Direct-indexed into the backing array.
 * @tparam Value  Trivially copyable value type.
 */
template <typename Key, typename Value>
struct LookupEntry
{
    Key key;
    Value value;
};

/**
 * @brief Derives a `LookupTable` capacity from a fixed `LookupEntry` array —
 *        one past the highest key present, so every entry's key indexes a
 *        valid slot.
 *
 * @tparam Key      Integral key type.
 * @tparam Value    Row value type.
 * @tparam N        Number of entries in @p entries.
 * @param entries   Fixed array of key/value rows to scan.
 * @return          The highest `entries[i].key` value plus one.
 */
template <typename Key, typename Value, size_t N>
constexpr int getCapacity (const LookupEntry<Key, Value> (&entries)[N]) noexcept
{
    size_t capacity { 0 };

    for (size_t i = 0; i < N; ++i)
        if (static_cast<size_t> (entries[i].key) + 1 > capacity)
            capacity = static_cast<size_t> (entries[i].key) + 1;

    return static_cast<int> (capacity);
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
