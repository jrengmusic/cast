/**
 * @file        jam_LookupTable.h
 * @brief       Generic constexpr direct-indexed lookup table — canon SSOT LUT pattern.
 */
namespace jam
{ /*____________________________________________________________________________*/

/**
 * @struct LookupTable
 * @brief Generic constexpr direct-indexed lookup table — int-keyed, int/enum/bits-valued.
 *
 * Canon SSOT LUT pattern (ARCHITECT-ratified): one shape for every static
 * value->value conversion table, hot-path or cold.
 * Identity is the hash — small-integer protocol keys (SGR params, OSC subcmds,
 * VT modes, UTF-8 lead bytes, palette indices, packed `jam::Union` composites)
 * index directly into a `std::array<Value, Capacity>`. No custom hash function,
 * nothing to regenerate, no collision handling.
 *
 * `static constexpr` instances of this type live in `.rodata` — zero
 * initialisation cost, hot-path safe by construction. There is one rule for
 * hot tables (per-cell SGR/CSI dispatch) and cold tables (VT340 default
 * palette) alike.
 *
 * @par Contract — ints in, ints out
 * `LookupTable` carries only int-keyed / int-or-enum-or-bits-valued rows.
 * Objects (`juce::Identifier`, `juce::Colour`, `juce::String`) never live in
 * a LookupTable slot:
 *  - `juce::Identifier` resolves only at the DecMode-Bimap / Model boundary
 *    (zero `constexpr` in `juce_Identifier.h` — an Identifier cannot be a
 *    `constexpr` table row).
 *  - `juce::Colour` is stored as a raw `uint32_t` ARGB word and wrapped into
 *    `juce::Colour` at the read site (`juce::Colour`'s constructors are not
 *    `constexpr` — `juce_Colour.h:66`).
 * Composite keys pack homogeneous via `jam::Union` (the `CursorState` /
 * `Winsize` / `CellFifo`-header precedent) — e.g. a (CSI intermediate, CSI
 * final) pair collapses to one integral key via `Union<...>::pack(...).bits`.
 *
 * @par Every slot holds a value — the table is a total function
 * Every slot not named in the constructor's entry list holds @p defaultValue —
 * the "unknown code" / "no-op" value for that table. The default value is a
 * first-class row (row zero of the source table), not an exception path:
 * `operator[]` never throws, an out-of-range key returns @p defaultValue
 * exactly like an unset in-range slot. A constructor's own entry list is
 * held to a stricter contract than `operator[]`: every key in @p entries
 * must already be in range — an out-of-range entry key is an authoring
 * error in the source table, not a runtime lookup, and asserts via
 * `failKeyOutOfRange()` instead of resolving to @p defaultValue.
 *
 * @par Whole-machinery performance rule (ARCHITECT)
 * Microscopic "slower" is void where wall-clock is unmeasurable; the
 * measurable tiers are per-byte / per-cell (~10^8/s flood). `LookupTable`'s
 * direct-index `operator[]` is safe even at that tier — no branch misprediction
 * risk from a hash probe, no allocation, no indirection beyond the array.
 *
 * @code
 * struct Sequence { enum SGR { bold = 1, dim = 2 }; };
 *
 * static constexpr jam::LookupTable<uint16_t, uint16_t, 10> attributeSetLut
 * {
 *     0,   // default: unknown SGR code -> no-op
 *     {
 *         { Sequence::bold, 0x01 },
 *         { Sequence::dim,  0x40 },
 *     }
 * };
 *
 * flags |= attributeSetLut[code];   // direct index, no branch on membership
 * @endcode
 *
 * @tparam Key       Integral key type. Direct-indexed — no hashing, no probing.
 * @tparam Value      Trivially copyable value type (int/enum/bit-flag width).
 * @tparam Capacity   Number of slots — one past the highest key ever indexed.
 */
template <typename Key, typename Value, int Capacity>
struct LookupTable
{
    static_assert (std::is_integral_v<Key>, "LookupTable requires an integral Key");
    static_assert (std::is_trivially_copyable_v<Value>,
                   "LookupTable requires a trivially copyable Value");

    /** @brief Row type accepted by both constructors — `LookupEntry<Key, Value>`. */
    using Entry = LookupEntry<Key, Value>;

    static constexpr int capacity { Capacity };

    /**
     * @brief Fills every slot with the first entry's value, then overwrites
     *        the slots named in @p entries.
     *
     * @param entries        Key/value rows; each key indexes directly into
     *                       the backing array. The first row's value is the
     *                       table's default.
     */
    constexpr LookupTable (std::initializer_list<Entry> entries) noexcept
        : defaultValue (entries.begin()->value), slots (fillSlots (std::make_index_sequence<static_cast<size_t> (Capacity)> {}, defaultValue))
    {
        for (const auto& entry : entries)
        {
            if (static_cast<size_t> (entry.key) < static_cast<size_t> (Capacity))
                slots.at (static_cast<size_t> (entry.key)) = entry.value;
            else
                failKeyOutOfRange();
        }
    }

    /**
     * @brief Fills every slot with the first entry's value, then overwrites
     *        the slots named in @p entries — fixed-array overload, for tables
     *        built from a `static constexpr Entry[]` alongside `getCapacity()`.
     *
     * @param entries        Key/value rows; each key indexes directly into
     *                       the backing array. The first row's value is the
     *                       table's default.
     */
    template <size_t N>
    constexpr LookupTable (const Entry (&entries)[N]) noexcept
        : defaultValue (entries[0].value), slots (fillSlots (std::make_index_sequence<static_cast<size_t> (Capacity)> {}, defaultValue))
    {
        for (const auto& entry : entries)
        {
            if (static_cast<size_t> (entry.key) < static_cast<size_t> (Capacity))
                slots.at (static_cast<size_t> (entry.key)) = entry.value;
            else
                failKeyOutOfRange();
        }
    }

    /**
     * @brief Direct-indexed lookup.
     * @param key  Table key. Negative or out-of-range keys are a positive
     *             check away from @p defaultValue — never a throw, never UB.
     * @return The row's value, or @p defaultValue when @p key names no row.
     */
    constexpr Value operator[] (Key key) const noexcept
    {
        return (static_cast<size_t> (key) < static_cast<size_t> (Capacity)) ? slots[static_cast<size_t> (key)] : defaultValue;
    }

    constexpr auto begin() const noexcept { return slots.begin(); }

    constexpr auto end() const noexcept { return slots.end(); }

private:
    /** @brief Asserts — a constructor's own entry key fell outside [0, Capacity). */
    static void failKeyOutOfRange() noexcept
    {
        jassertfalse;
    }

    /** @brief Index-sequence-driven fill of the backing array with @p defaultValue.
     *
     *  @tparam Is            Index pack spanning [0, Capacity).
     *  @param  defaultValue  Value written into every slot.
     *  @return               Filled array of size Capacity.
     */
    template <size_t... Is>
    static constexpr std::array<Value, static_cast<size_t> (Capacity)> fillSlots (std::index_sequence<Is...>, Value defaultValue) noexcept
    {
        return { ((void) Is, defaultValue)... };
    }

    Value defaultValue;
    std::array<Value, static_cast<size_t> (Capacity)> slots;
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
