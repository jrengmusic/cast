/**
 * @file jam_Grapheme.h
 * @brief Grapheme cluster interning table and UAX #29 segmentation result.
 *
 * `jam::Grapheme` is a `SharedResources<Grapheme>` — identical grapheme
 * cluster entries are deduplicated.  `Grapheme::Entry` holds up to 8
 * codepoints.  `Grapheme::SegmentationResult` is the packed 16-bit output
 * of one UAX #29 state machine step.
 */
#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/**
 * @struct Grapheme
 * @brief Shared grapheme cluster table — dedupes multi-codepoint clusters.
 *
 * Inherits `addIfNotAlreadyThere(entry) -> int` from SharedResources; the
 * returned index is stored in a `jam::AttributedChar` codepoint field when
 * `contentTag == contentGrapheme`.
 *
 * ### Nested types
 * - `Grapheme::Entry`              — the interned cluster descriptor (up to 8 codepoints).
 * - `Grapheme::SegmentationResult` — packed 16-bit output of the UAX #29 state machine step.
 */
struct Grapheme : SharedResources<Grapheme>
{
    /**
     * @struct Entry
     * @brief Grapheme cluster descriptor: up to 8 codepoints.
     *
     * The atom interned in `jam::Grapheme` and referenced by `jam::AttributedChar::codepoint()`
     * when `contentTag == contentGrapheme`.  Inherits `SharedResource` so it can
     * be stored polymorphically by the SharedResources interning container and
     * compared/hashed via virtual dispatch.
     */
    struct Entry : SharedResource
    {
        /** @brief Up to 8 codepoints forming the cluster (base + combining marks). */
        std::array<char32_t, 8> codepoints {};

        /** @brief Number of occupied entries in codepoints. */
        uint8_t count { 0 };

        /** @brief Element-wise equality up to `count` codepoints. */
        bool operator== (const SharedResource& other) const noexcept override
        {
            const auto& o { static_cast<const Entry&> (other) };
            bool equal { count == o.count };

            for (uint8_t i { 0 }; i < count and equal; ++i)
                equal = (codepoints.at (i) == o.codepoints.at (i));

            return equal;
        }

        /**
         * @brief Polynomial hash over the occupied codepoint bytes.
         *
         * Runs `jam::hashBytes` over `count * sizeof(char32_t)` bytes of the
         * codepoints array (multiplier 31).
         */
        size_t hash() const noexcept override
        {
            return hashBytes (codepoints.data(), static_cast<size_t> (count) * sizeof (char32_t));
        }
    };

    /**
     * @struct SegmentationResult
     * @brief Packed 16-bit state machine output for UAX #29 cluster segmentation.
     *
     * Holds the new state plus a single-bit `addToCurrentCell` flag indicating
     * whether the codepoint should extend the current grapheme cluster or start
     * a new one. Trivially copyable — safe to store in registers and pass by value.
     *
     * @par Bit Layout
     * @code
     *  Bit    | Field
     *  -------|---------
     *  [9:0]  | state (10-bit state machine index)
     *  [10]   | addToCurrentCell
     *  [15:11]| unused
     * @endcode
     */
    struct SegmentationResult
    {
        /** @brief Packed state and boundary flag. */
        uint16_t val { 0 };

        /**
         * @brief True if the codepoint should be added to the current cluster.
         * @return true to extend current cluster; false to start a new cluster.
         */
        bool addToCurrentCell() const noexcept { return static_cast<bool> ((val >> 10) & 1); }

        /**
         * @brief State machine state after processing the codepoint.
         * @return 10-bit state value to feed into the next step.
         */
        uint16_t state() const noexcept { return static_cast<uint16_t> (val & 0x3FF); }
    };
};

static_assert (sizeof (Grapheme::SegmentationResult) == sizeof (uint16_t),
               "Grapheme::SegmentationResult must be 16 bits");
static_assert (std::is_trivially_copyable_v<Grapheme::SegmentationResult>,
               "Grapheme::SegmentationResult must be trivially copyable");

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
