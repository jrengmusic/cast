/**
 * @file jam_Charset.cpp
 * @brief DEC VT100 line-drawing table + AttributedChar::fromCodepoint single-cell producer.
 *
 * decLineDrawing maps bytes 0x60-0x7E to box-drawing glyphs when line-drawing
 * mode is active. AttributedChar::fromCodepoint is the single-cell producer: translates
 * charset, looks up width, sets widthClass hint, packs the cell.
 *
 * Data source: Unicode Standard 17.0.0, extracted from the terminal renderer's
 * character-property tables.
 * decLineDrawing: verbatim from endless/Source/terminal/Charset.h.
 */

namespace jam
{

// =============================================================================
// DEC Special Character and Line Drawing Set (VT100)
// =============================================================================

static constexpr uint32_t decLineDrawing[32]
{
    0x25C6, 0x2592, 0x2409, 0x240C, 0x240D, 0x240A, 0x00B0, 0x00B1,
    0x2424, 0x240B, 0x2518, 0x2510, 0x250C, 0x2514, 0x253C, 0x23BA,
    0x23BB, 0x2500, 0x23BC, 0x23BD, 0x251C, 0x2524, 0x2534, 0x252C,
    0x2502, 0x2264, 0x2265, 0x03C0, 0x2260, 0x00A3, 0x00B7, 0x0020
};

// =============================================================================
// jam::AttributedChar static method bodies
// =============================================================================

AttributedChar AttributedChar::fromCodepoint (uint32_t codepoint, uint16_t styleId, bool lineDrawing) noexcept
{
    // Apply DEC line-drawing translation (caller owns the active state flag).
    uint32_t cp { codepoint };

    if (lineDrawing and cp >= 0x60 and cp <= 0x7E)
        cp = decLineDrawing[cp - 0x60];

    // Look up packed properties; recover signed width via AttributedChar::widthShift.
    const uint32_t props { charPropsFor (cp) };
    const int      rawWidth { static_cast<int> ((props >> AttributedChar::bitWidthShifted) & ((1u << AttributedChar::widthFieldBits) - 1u)) - AttributedChar::widthShift };
    const int      charWidth { rawWidth < 1 ? 1 : rawWidth };
    const uint8_t  widthClass { charWidth == 2 ? wide : narrow };

    return make (cp, contentCodepoint, widthClass, styleId);
}

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
