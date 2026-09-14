namespace jam
{
/*____________________________________________________________________________*/
/** @brief Per-glyph codepoint/span + span-aware cell-box metrics, set by
 *  setCellRun() for the immediately following drawGlyphs() call — see that
 *  method's own doc comment. Cleared by drawGlyphs() once consumed;
 *  default-constructed (count == 0) means "no cell-run context". */
struct PendingCellRun
{
    /** @brief Per-glyph original Unicode codepoints, parallel to the glyph
     *  indices the following drawGlyphs() call will receive. Null when no
     *  cell-run context is pending. */
    const char32_t* codepoints { nullptr };

    /** @brief Per-glyph display width in cells (1 = narrow, 2 = wide),
     *  parallel to codepoints. Null when no cell-run context is pending. */
    const uint8_t*  spans      { nullptr };

    /** @brief Element count of both codepoints and spans — must equal the
     *  following drawGlyphs() call's glyph count, or this state is ignored.
     *  Zero means "no cell-run context". */
    int             count      { 0 };

    /** @brief Terminal cell width, physical pixels. */
    int             cellWidth  { 0 };

    /** @brief Terminal cell height, physical pixels. */
    int             cellHeight { 0 };

    /** @brief Cell-top-to-baseline offset, physical pixels. */
    int             baseline   { 0 };
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
