/**
 * @file jam_Bimap.h
 * @brief Map inversion/lookup helpers and the Bimap CRTP registry base.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Static helpers for inverting and querying map-like containers.
 */
struct Map
{
    /**
     * @brief Get std::map Key from Value.
     *
     * This function inverts a std::map, creating a new map where the keys become the values and the values become the keys.
     *
     * @tparam Key The type of the keys in the original map.
     * @tparam Value The type of the values in the original map.
     * @param mapToBeInverted The original map to be inverted.
     * @return A new map with inverted keys and values.
     */
    template<typename Key, typename Value>
    static std::map<Value, Key> getKey (const std::map<Key, Value>& mapToBeInverted)
    {
        std::map<Value, Key> inverted;
        for (const auto& [key, value] : mapToBeInverted)
            inverted[value] = key;

        return inverted;
    }

    /**
     * @brief Get Key from Value for any map-like container.
     *
     * Inverts any map-like M (std::map, jam::HashMap, std::unordered_map, etc.),
     * creating a new jam::HashMap where the values become the keys and the keys
     * become the values.
     *
     * @tparam M The map-like container type.
     * @param mapToBeInverted The original map to be inverted.
     * @return A new jam::HashMap with inverted keys and values.
     */
    template<class M>
    static auto getKey (const M& mapToBeInverted)
        -> jam::HashMap<typename M::value_type::second_type, typename M::value_type::first_type>
    {
        jam::HashMap<typename M::value_type::second_type, typename M::value_type::first_type>
            inverted;

        for (const auto& [k, v] : mapToBeInverted)
            inverted.emplace (v, k);

        return inverted;
    }

    /**
     * @brief Check if a map contains a key.
     *
     * This function checks if a std::map contains a specific key.
     *
     * @tparam Key The type of the keys in the map.
     * @tparam Value The type of the values in the map.
     * @param map The map to check.
     * @param key The key to look for in the map.
     * @return True if the key is found, false otherwise.
     */
    template<typename Key, typename Value>
    static bool contains (const std::map<Key, Value>& map, const Key& key)
    {
        return map.find (key) != map.end();
    }

    /**
     * @brief Check if any map-like container contains a key.
     *
     * @tparam M The map-like container type.
     * @tparam K The key type.
     * @param map The map to check.
     * @param key The key to look for.
     * @return True if the key is found, false otherwise.
     */
    template<class M, class K>
    static bool contains (const M& map, const K& key)
    {
        return map.find (key) != map.end();
    }

    /**
     * @brief Check if any map-like container contains a specific value.
     *
     * Iterates through the elements of any map-like M and checks whether
     * any of the stored values match the given value.
     *
     * @tparam M The map-like container type.
     * @tparam V The value type to search for.
     * @param map   The map to search.
     * @param value The value to look for.
     * @return True if the value is found in the map, false otherwise.
     *
     * @note This operation has linear complexity O(n), since it must
     *       check each element in the container.
     */
    template<class M, class V>
    static bool containsValue (const M& map, const V& value)
    {
        return std::any_of (map.begin(),
                            map.end(),
                            [&value] (const auto& entry)
                            {
                                const auto& [entryKey, entryValue] = entry;
                                return entryValue == value;
                            });
    }
};

/**
 * @brief Bidirectional lookup registry — key/juce::String/juce::Identifier map with reverse lookup.
 *
 * Derived structs construct through the explicit Bimap (HashMap) constructor
 * and declare their own getDefault(). Lifetime is owned independently by the
 * derived type's own SharedInstance registration — Bimap itself holds no
 * lifecycle.
 *
 * @tparam Key The map's key type. Defaults to int.
 */
template<typename KeyType = int, typename ValueType = juce::String>
class Bimap
{
public:
    /** @brief Constructs both directions complete — @p rows becomes the key-to-value map, inverse derives from it once. */
    explicit Bimap (HashMap<KeyType, ValueType> rows) : map { std::move (rows) }, inverse { Map::getKey (map) } {}

    /** @brief Returns the underlying key-to-value map. */
    const auto& get() const noexcept { return map; }

    /**
   * @brief Looks up the key registered for @p value.
   *
   * @param value  The value to reverse-look-up.
   * @return       The key mapped to @p value.
   */
    KeyType get (const ValueType& value) const { return inverse.at (value); }

    /**
   * @brief Looks up the value registered for @p key.
   *
   * @param key  The key to look up.
   * @return     The value mapped to @p key.
   */
    const ValueType& get (KeyType key) const { return map.at (key); }

    /** @brief Returns true if the map contains the given value. */
    bool contains (const ValueType& value) const noexcept
    {
        return Map::contains (inverse, value);
    }

    /**
   * @brief Returns the registry's default value, used when a lookup has no match.
   *
   * Base implementation returns the first authored entry's value (jam::HashMap
   * iteration is insertion-ordered), or an empty juce::Identifier or juce::String when the map is
   * empty. Derived classes declare their own getDefault() for a different
   * default — resolved by concrete type, never through a Bimap<Key> base
   * pointer.
   */
    const ValueType& getDefault() const noexcept
    {
        static const ValueType none {};
        return map.empty() ? none : map.begin()->second;
    }

protected:
    /** @brief Key-to-value registry. */
    HashMap<KeyType, ValueType> map;

    /** @brief Value-to-key registry. */
    HashMap<ValueType, KeyType> inverse;
};
/**_____________________________END_OF_NAMESPACE______________________________*/
}// namespace jam
