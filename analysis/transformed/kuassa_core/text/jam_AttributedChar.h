/**
 * @file jam_AttributedChar.h
 * @brief Universal 8-byte attributed character atom — the shaping input.
 *
 * `jam::AttributedChar` is an 8-byte, trivially-copyable packed u64 that carries one
 * display column's worth of resolved visual state: codepoint or grapheme index,
 * widthClass/layout hint, and a styleId into the jam::Stamp table.
 *
 * It is the canonical attributed character atom for the terminal and TUI render
 * pipelines.  Colours and SGR flags live in jam::Stamp keyed by styleId.
 * Grapheme cluster codepoints live in jam::Grapheme keyed by the 21-bit
 * codepoint field when contentTag == contentGrapheme.
 *
 * ### Packed u64 layout (8 bytes, little-endian bit numbering)
 * ```
 * Bit 63                                                              Bit 0
 * [ pad(7) | hyperlinkId (16) | styleId (16) | widthClass (2) | contentTag (2) | codepoint (21) ]
 * ```
 *
 * | Bits  | Width | Field       | Meaning                                                       |
 * |-------|-------|-------------|---------------------------------------------------------------|
 * |  0-20 |  21   | codepoint   | Unicode scalar U+0000–U+10FFFF; or Grapheme idx               |
 * | 21-22 |   2   | contentTag  | 0 = codepoint, 1 = grapheme index, 2-3 reserved               |
 * | 23-24 |   2   | widthClass  | 0 = narrow, 1 = wide, 2 = spacerTail, 3 = proportional        |
 * | 25-40 |  16   | styleId     | Index into jam::Stamp table                                   |
 * | 41-56 |  16   | hyperlinkId | jam::Hyperlink table index + 1; 0 = no link (zero-init default) |
 * | 57-63 |   7   | (padding)  | Reserved, always 0                                            |
 *
 * ### Wide-character encoding
 * - A **wide** (CJK / fullwidth) character occupies two columns.  The left
 *   column stores the codepoint with `widthClass == wide`; the right column stores
 *   `codepoint == 0` with `widthClass == spacerTail`.  The renderer skips tail cells.
 *
 * ### Grapheme cluster encoding
 * - When `contentTag == contentGrapheme`, the 21-bit codepoint field holds an
 *   index into jam::Grapheme.  The table entry contains the full
 *   cluster (base + combining marks).
 */

#pragma once
namespace jam
{ /*____________________________________________________________________________*/

/**
 * @struct AttributedChar
 * @brief Universal 8-byte attributed character atom — the shaping input.
 *
 * AttributedChar is the atomic unit of the terminal and TUI render pipelines.  Its fixed
 * size and trivial-copyability allow dense storage in Buffer<String> and
 * memcpy-based screen diffing.
 *
 * All fields are packed into a single `uint64_t packed` member.  Use the named
 * accessors and factory methods — never access `packed` directly in callers.
 *
 * - `codepoint()`: 21-bit Unicode scalar (or Grapheme index when contentTag == contentGrapheme).
 * - `contentTag()`: 2-bit selector — contentCodepoint or contentGrapheme.
 * - `widthClass()`: 2-bit geometry hint — narrow, wide, spacerTail, or proportional.
 * - `styleId()`:    16-bit index into jam::Stamp for fg/bg/flags.
 * - `hyperlinkId()`: 16-bit jam::Hyperlink table index + 1 for the OSC 8 hyperlink;
 *                   0 = no link. Consumers resolve the table entry via
 *                   `jam::Hyperlink::get(hyperlinkId() - 1)` — the +1 offset avoids
 *                   colliding with the 0 = "no link" sentinel, since
 *                   jam::Hyperlink's own table is 0-based.
 *
 * @note `sizeof(AttributedChar) == 8` and `std::is_trivially_copyable_v` for `AttributedChar`
 *       are enforced by static_assert immediately after the struct definition.
 */
struct AttributedChar
{
    //==========================================================================
    // contentTag values

    /** @brief contentTag — char holds a single Unicode scalar in the codepoint field. */
    static constexpr uint8_t contentCodepoint { 0 };

    /** @brief contentTag — codepoint field is an index into jam::Grapheme. */
    static constexpr uint8_t contentGrapheme  { 1 };

    /** @brief contentTag — elastic whitespace cell (flex-grow). */
    static constexpr uint8_t flexGap          { 2 };

    //==========================================================================
    // widthClass values

    /** @brief Narrow cell — occupies one display column. */
    static constexpr uint8_t narrow      { 0 };

    /** @brief Wide cell — occupies two display columns (left/head column). */
    static constexpr uint8_t wide        { 1 };

    /** @brief Spacer tail — right-hand continuation column of a wide character. */
    static constexpr uint8_t spacerTail { 2 };

    /** @brief Proportional — derive advance from HarfBuzz glyph metric. */
    static constexpr uint8_t proportional { 3 };

    //==========================================================================
    // Bit-field layout constants (private geometry — used only inside accessors and make())

private:

    static constexpr uint64_t codepointMask   { 0x1FFFFF };
    static constexpr int      contentTagShift { 21 };
    static constexpr uint64_t contentTagMask  { uint64_t (0x3) << 21 };
    static constexpr int      widthClassShift { 23 };
    static constexpr uint64_t widthClassMask  { uint64_t (0x3) << 23 };
    static constexpr int      styleIdShift    { 25 };
    static constexpr uint64_t styleIdMask     { uint64_t (0xFFFF) << 25 };
    static constexpr int      hyperlinkIdShift { 41 };
    static constexpr uint64_t hyperlinkIdMask  { uint64_t (0xFFFF) << 41 };

    // -------------------------------------------------------------------------
    // Packed property value layout (returned by charPropsFor()).
    // Positions match the bit layout in endless/Source/terminal/CharProps.h.

    /** @brief Bit-offset of the 3-bit display width field in the packed property value. */
    static constexpr uint32_t bitWidthShifted            { 9  };
    /** @brief Bit-offset of the isCombining flag in the packed property value. */
    static constexpr uint32_t bitIsCombining             { 22 };
    /** @brief Bit-offset of the isWordChar flag in the packed property value. */
    static constexpr uint32_t bitIsWordChar             { 23 };
    /** @brief Bit-offset of the 7-bit grapheme break property field in the packed property value. */
    static constexpr uint32_t bitGraphemeSegProperty    { 25 };

    /** @brief Bit-width of the display width field. Mask = (1u << 3) - 1u = 0x7. */
    static constexpr uint32_t widthFieldBits             { 3  };
    /** @brief Bias subtracted from the stored 3-bit width field to recover the signed display width.
     *  Stored value = actual_width + widthShift; recovery: rawWidth - widthShift. */
    static constexpr int      widthShift                  { 4  };
    /** @brief Bit-width of the grapheme break property field. Mask = (1u << 7) - 1u = 0x7F. */
    static constexpr uint32_t graphemeSegPropertyBits   { 7  };
    /** @brief Bit-width of single-bit boolean fields. Mask = (1u << 1) - 1u = 0x1. */
    static constexpr uint32_t boolFieldBits              { 1  };

    /**
     * @brief 3-level multistage lookup for the packed codepoint property value.
     *
     * Returns the packed uint32_t for a codepoint. Branchless clamp + T1→T2→T3
     * table walk. Caller is responsible for shift/mask extraction using the
     * `bit*` and `*Bits` constants above. Defined in jam_CharProps.cpp.
     */
    static uint32_t charPropsFor (uint32_t codepoint) noexcept;

public:

    //==========================================================================
    // Storage

    /** @brief All fields packed into a single 64-bit word.  Always use accessors. */
    uint64_t packed { 0 };

    //==========================================================================
    // Accessors

    /** @brief Returns the 21-bit codepoint or Grapheme index (bits 0–20). */
    uint32_t codepoint () const noexcept
    {
        return static_cast<uint32_t> (packed & codepointMask);
    }

    /** @brief Returns the 2-bit content tag (bits 21–22). */
    uint8_t contentTag () const noexcept
    {
        return static_cast<uint8_t> ((packed & contentTagMask) >> contentTagShift);
    }

    /** @brief Returns the 2-bit widthClass geometry hint (bits 23–24). */
    uint8_t widthClass () const noexcept
    {
        return static_cast<uint8_t> ((packed & widthClassMask) >> widthClassShift);
    }

    /** @brief Returns the 16-bit jam::Stamp index (bits 25–40). */
    uint16_t styleId () const noexcept
    {
        return static_cast<uint16_t> ((packed & styleIdMask) >> styleIdShift);
    }

    /** @brief Returns the 16-bit jam::Hyperlink table index + 1 (bits 41–56);
     *  0 = no link. Resolve via `jam::Hyperlink::get(hyperlinkId() - 1)`. */
    uint16_t hyperlinkId () const noexcept
    {
        return static_cast<uint16_t> ((packed & hyperlinkIdMask) >> hyperlinkIdShift);
    }

    //==========================================================================
    // Factory methods

    /**
     * @brief Packs four fields into an AttributedChar.
     *
     * @param cp         21-bit Unicode scalar or Grapheme index.
     * @param tag        contentTag — contentCodepoint or contentGrapheme.
     * @param widthClass width-class hint — narrow, wide, spacerTail, or proportional.
     * @param sid        jam::Stamp styleId.
     * @return           Fully packed AttributedChar with padding bits zero.
     */
    static AttributedChar make (uint32_t cp, uint8_t tag, uint8_t widthClass, uint16_t sid) noexcept
    {
        AttributedChar c;
        c.packed = (static_cast<uint64_t> (cp)         & codepointMask)
                 | (static_cast<uint64_t> (tag)         << contentTagShift)
                 | (static_cast<uint64_t> (widthClass)  << widthClassShift)
                 | (static_cast<uint64_t> (sid)          << styleIdShift);
        return c;
    }

    /**
     * @brief Returns a blank erase char — zero codepoint, no content tag, narrow, specified styleId.
     *
     * Used by Video for all erase/clear/scroll fill operations.
     *
     * @param sid  jam::Stamp styleId carrying the background fill colour.
     * @return     Erase char with codepoint == 0, contentTag == contentCodepoint,
     *             widthClass == narrow, styleId == sid.
     */
    static AttributedChar erase (uint16_t sid) noexcept
    {
        return make (0, contentCodepoint, narrow, sid);
    }

    /**
     * @brief Returns a copy of @p c with its hyperlinkId field replaced.
     *
     * Pen-side pack helper — `Video::stampHyperlink()` applies this to every
     * written cell (head and spacerTail) while the pen's `activeHyperlinkId`
     * is non-zero. All other fields are preserved unchanged.
     *
     * @param c   Source AttributedChar to stamp.
     * @param id  jam::Hyperlink table index + 1 (see `hyperlinkId()`), or 0 to clear the link.
     * @return    Copy of @p c with hyperlinkId bits (41–56) replaced by @p id.
     */
    static AttributedChar withHyperlinkId (AttributedChar c, uint16_t id) noexcept
    {
        c.packed = (c.packed & ~hyperlinkIdMask) | (static_cast<uint64_t> (id) << hyperlinkIdShift);
        return c;
    }

    //==========================================================================
    // Codepoint property queries (TU-static lookup data in jam_CharProps.cpp)
    //
    // These methods are stateless. The grapheme segmentation state machine
    // is caller-owned (Video holds the running state); AttributedChar provides pure
    // step functions and never retains per-call state.
    //==========================================================================

    /**
     * @brief Single-cell producer: applies DEC line-drawing translation,
     *        looks up codepoint width, sets widthClass hint, packs the cell.
     *
     * Caller (Video) owns the `useLineDrawing` state; AttributedChar stays stateless.
     * Replaces the sequence: `translateCharset(cp, useLineDrawing)` →
     * `charPropsFor(cp).width()` → `AttributedChar::make(cp, contentCodepoint, hint, sid)`.
     *
     * @param codepoint   21-bit Unicode scalar (0–0x10FFFF).
     * @param styleId     jam::Stamp index carrying fg/bg/flags.
     * @param lineDrawing DEC line-drawing mode active (affects 0x60–0x7E only).
     * @return            Fully packed AttributedChar ready to write to a cell.
     */
    static AttributedChar fromCodepoint (uint32_t codepoint, uint16_t styleId, bool lineDrawing) noexcept;

    /**
     * @brief Per-codepoint monospace display width.
     *
     * Returns 0 for combining marks (zero advance), 1 for regular characters,
     * 2 for wide / fullwidth / CJK characters. Follows the Unicode Width
     * property (East Asian Width).
     *
     * @param codepoint  21-bit Unicode scalar.
     * @return           Signed width: 0 (combining), 1 (regular), 2 (wide).
     */
    static int width (uint32_t codepoint) noexcept;

    static bool isWide (uint32_t codepoint, uint8_t widthClass) noexcept
    {
        return widthClass != proportional and width (codepoint) == 2;
    }

    /**
     * @brief Word-boundary classification for selection / word-wise navigation.
     *
     * True for letters, digits, and underscore across all scripts.
     *
     * @param codepoint  21-bit Unicode scalar.
     * @return           true if the codepoint participates in word boundaries.
     */
    static bool isWordChar (uint32_t codepoint) noexcept;

    /**
     * @brief Combining mark detection.
     *
     * True for characters in the Mark categories (Mn / Mc / Me) — modifiers
     * that attach to a base character and have zero advance.
     *
     * @param codepoint  21-bit Unicode scalar.
     * @return           true if the codepoint is a combining mark.
     */
    static bool isCombining (uint32_t codepoint) noexcept;

    /**
     * @brief One step of the UAX #29 grapheme cluster state machine.
     *
     * AttributedChar stays stateless — the result is the new state plus an
     * `addToCurrentCell` flag indicating whether the codepoint extends the
     * current cluster or starts a new one.
     *
     * @param state     Previous grapheme segmentation state (caller-owned).
     * @param codepoint 21-bit Unicode scalar to fold into the cluster.
     * @return          New `Grapheme::SegmentationResult` with updated state and boundary flag.
     */
    static Grapheme::SegmentationResult
        graphemeSegmentationStep (Grapheme::SegmentationResult state, uint32_t codepoint) noexcept;

    /**
     * @brief Initial state for a new text run (UAX #29 "AtStart" state).
     *
     * The initial state is the synthetic start-of-string sentinel — the first
     * codepoint processed is guaranteed to start a new cluster.
     *
     * @return  `Grapheme::SegmentationResult` at the AtStart state.
     */
    static Grapheme::SegmentationResult graphemeSegmentationInit() noexcept
    {
        return Grapheme::SegmentationResult { 0 };
    }
};

static_assert (sizeof (AttributedChar) == 8, "AttributedChar must be exactly 8 bytes");
static_assert (std::is_trivially_copyable_v<AttributedChar>, "AttributedChar must be trivially copyable");

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
