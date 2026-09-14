/**
 * @file jam_Stamp.h
 * @brief Style interning table for the rendering pipeline.
 *
 * `jam::Stamp::Entry` is the style descriptor carried by every `jam::AttributedChar`.
 * All visual attributes of a single cell (foreground, background, underline
 * colour, SGR bit flags) live in one Entry; the AttributedChar atom holds only a
 * 16-bit styleId index into the `jam::Stamp` interning table.
 *
 * `jam::Stamp` is a `SharedResources<Stamp>` — identical stamp
 * entries (fg/bg/underline/flags) are deduplicated.  Video SGR writes
 * resolve a `Stamp::Entry` once and stamp the same styleId on every cell in
 * a styled run.
 *
 * `Stamp::Entry` inherits `SharedResource` so it can be stored polymorphically
 * by the SharedResources interning container and compared/hashed via virtual
 * dispatch.
 *
 * ### Stamp::Entry layout
 * ```
 * [ juce::Colour fg (4) | juce::Colour bg (4) | juce::Colour underline (4) | uint16_t flags (2) | padding | ]
 * ```
 *
 * `underline` is the underline colour.  An underline colour of
 * `juce::Colour()` (alpha == 0) is the sentinel meaning "follow the
 * foreground" — the renderer substitutes `fg` when alpha is 0.
 *
 * ### flags bit layout (uint16_t)
 * @code
 *  Bit     | Name                | Meaning
 *  --------|---------------------|--------------------------------------------
 *  0       | bold                | Bold weight
 *  1       | italic              | Italic slant
 *  3       | strike              | Strikethrough
 *  4       | blink               | Slow blink
 *  5       | inverse             | Inverse video (swap fg/bg)
 *  6       | dim                 | Faint / dim
 *  9-11    | underline*          | Underline style (3-bit field, mask 0xe00)
 *  12      | overline            | Overline
 *  13      | superscript         | Superscript baseline shift
 *  14      | subscript           | Subscript baseline shift
 *  15      | code                | Markdown inline code span (spec §6.1)
 *  Bits 2, 7, 8 are reserved for future use.
 * @endcode
 *
 * The underline style field is 3 bits wide and uses the same values as
 * xterm's `SGR 4:n` parameter: 0=none, 1=single, 2=double, 3=curly,
 * 4=dotted, 5=dashed.  The `Stamp::underline*` constants carry the field
 * at bit position 9 so they can be OR'd directly into `flags`.  Use
 * `Stamp::underlineStyleMask` to extract the field from a flags value;
 * `(flags & Stamp::underlineStyleMask) != 0` is the "any underline set"
 * test (replaces the legacy single-bit `Stamp::underline` check, which has
 * been removed — the single-bit version is no longer addressable).
 */
#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/**
 * @struct Stamp
 * @brief Shared style table — dedupes `Stamp::Entry` values.
 *
 * Interned styleId source for the SGR pen state.  Inherits
 * `addIfNotAlreadyThere(entry) -> int` from SharedResources; the returned
 * index is the `AttributedChar::styleId()` value.
 */
struct Stamp : SharedResources<Stamp>
{
    /**
     * @struct Entry
     * @brief Style descriptor: foreground, background, underline colour, and SGR bit flags.
     *
     * The atom interned in `jam::Stamp` and referenced by `jam::AttributedChar::styleId()`.
     * Inherits `SharedResource` so it can be stored polymorphically by the
     * SharedResources interning container and compared/hashed via virtual dispatch.
     */
    struct Entry : SharedResource
    {
        juce::Colour fg;                   ///< Foreground colour.
        juce::Colour bg;                   ///< Background colour.
        juce::Colour underline;            ///< Underline colour (alpha == 0 sentinel = follow fg).
        uint16_t    flags { 0 };           ///< SGR bit flags — see bit layout in jam_Stamp.h.

        /** @brief Construct from fg/bg/underline/flags. */
        Entry (juce::Colour fgColour, juce::Colour bgColour,
               juce::Colour underlineColour, uint16_t flagBits) noexcept
            : fg (fgColour), bg (bgColour), underline (underlineColour), flags (flagBits) {}

        /** @brief Default-construct (all zero / transparent). */
        Entry() noexcept = default;

        /** @brief Field-by-field equality. Colour comparison via juce::Colour::operator==. */
        bool operator== (const SharedResource& other) const noexcept override
        {
            const auto& o { static_cast<const Entry&> (other) };
            return fg        == o.fg
               and bg        == o.bg
               and underline == o.underline
               and flags     == o.flags;
        }

        /**
         * @brief Byte-packed polynomial hash over all Entry fields.
         *
         * Packs fg, bg, and underline as big-endian ARGB bytes (4 each) plus
         * the uint16_t flags as 2 bytes, total 14 bytes, then runs
         * `jam::hashBytes` (polynomial rolling, multiplier 31).
         */
        size_t hash() const noexcept override
        {
            const auto fgArgb        { fg.getARGB() };
            const auto bgArgb        { bg.getARGB() };
            const auto underlineArgb { underline.getARGB() };
            std::array<uint8_t, 14> packed {};
            packed.at (0)  = static_cast<uint8_t> (fgArgb >> 24);
            packed.at (1)  = static_cast<uint8_t> (fgArgb >> 16);
            packed.at (2)  = static_cast<uint8_t> (fgArgb >> 8);
            packed.at (3)  = static_cast<uint8_t> (fgArgb);
            packed.at (4)  = static_cast<uint8_t> (bgArgb >> 24);
            packed.at (5)  = static_cast<uint8_t> (bgArgb >> 16);
            packed.at (6)  = static_cast<uint8_t> (bgArgb >> 8);
            packed.at (7)  = static_cast<uint8_t> (bgArgb);
            packed.at (8)  = static_cast<uint8_t> (underlineArgb >> 24);
            packed.at (9)  = static_cast<uint8_t> (underlineArgb >> 16);
            packed.at (10) = static_cast<uint8_t> (underlineArgb >> 8);
            packed.at (11) = static_cast<uint8_t> (underlineArgb);
            packed.at (12) = static_cast<uint8_t> (flags >> 8);
            packed.at (13) = static_cast<uint8_t> (flags);
            return hashBytes (packed.data(), packed.size());
        }
    };

    // --- Single-bit SGR flags (bits 0-7) -------------------------------------

    static constexpr uint16_t bold      { 0x01 };      ///< SGR 1: bold weight.
    static constexpr uint16_t italic    { 0x02 };      ///< SGR 3: italic slant.
    static constexpr uint16_t strike    { 0x08 };      ///< SGR 9: strikethrough.
    static constexpr uint16_t blink     { 0x10 };      ///< SGR 5: slow blink.
    static constexpr uint16_t inverse   { 0x20 };      ///< SGR 7: inverse video.
    static constexpr uint16_t dim       { 0x40 };      ///< SGR 2: faint / dim.

    // --- Underline style field (3 bits at shift 9) ---------------------------

    /** @brief Underline style field mask — 3 bits at bit position 9 (0xe00). */
    static constexpr uint16_t underlineStyleMask { 0xe00 };

    /** @brief Underline style: no underline (0b000 in style field). */
    static constexpr uint16_t underlineNone    { 0     };
    /** @brief Underline style: single straight line (0b001 in style field). */
    static constexpr uint16_t underlineSingle  { 1 << 9 };
    /** @brief Underline style: double straight line (0b010 in style field). */
    static constexpr uint16_t underlineDouble  { 2 << 9 };
    /** @brief Underline style: curly / wavy line (0b011 in style field). */
    static constexpr uint16_t underlineCurly   { 3 << 9 };
    /** @brief Underline style: dotted line (0b100 in style field). */
    static constexpr uint16_t underlineDotted  { 4 << 9 };
    /** @brief Underline style: dashed line (0b101 in style field). */
    static constexpr uint16_t underlineDashed  { 5 << 9 };

    // --- New single-bit flags (bits 12-15) -----------------------------------

    static constexpr uint16_t overline     { 1 << 12 }; ///< SGR 53: overline.
    static constexpr uint16_t superscript  { 1 << 13 }; ///< SGR 73: superscript baseline shift.
    static constexpr uint16_t subscript    { 1 << 14 }; ///< SGR 74: subscript baseline shift.

    /**
     * @brief Markdown inline code span cell (spec §6.1) — never an SGR/
     *        terminal concept, caps off this "new single-bit flags"
     *        section (12-15) the same way `overline`/`superscript`/
     *        `subscript` already extended `Stamp` beyond the base terminal
     *        SGR set; bit 8 stays the sole isolated reserved bit (no
     *        adjacent section to belong to). Consumed by
     *        `jam::MarkdownLayout` (code-span background tint decoration)
     *        and `jam::MarkdownDocument::drawRows` (draw-time fg resolves
     *        to `codeTextColourId`) — `jam_markdown/parser/
     *        jam_MarkdownInline.cpp` is the sole producer.
     */
    static constexpr uint16_t code         { 1 << 15 }; ///< Markdown inline code span (spec §6.1).
    // Bit 8 reserved.
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
