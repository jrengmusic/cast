/**
 * @file jam_GlyphArrangement.h
 * @brief Monospace layout engine: shapes a `TextModel` document into positioned glyph draw runs.
 *
 * Conformance restoration (ARCHITECT-directed): the CodeView rewrite
 * delegated text layout to `juce::GlyphArrangement`
 * (`addLineOfText`/`draw`), bypassing the measured CPU half of the glyph
 * pipeline (shaping+layout). This restores the OLD `jam::glyph::Arrangement`
 * semantics (deleted commit e8543bf1a^, `jam_vulkan/fonts/font/glyph/jam_Run.h`
 * / `jam_Run.cpp` / `jam_GlyphArrangementShape.cpp`) — Entry/Run shape,
 * two-pass shape()/arrange(), cell-arithmetic draw-run grouping — with
 * today's types underneath:
 *
 * | Old (dead)                                    | New (this file)                                    |
 * |------------------------------------------------|-----------------------------------------------------|
 * | `void* fontHandle` (platform font handle)       | `const juce::Typeface*` (identity into `jam::Typeface`) |
 * | `jam::Typeface` god object (CoreText/DirectWrite shaping, per-style sub-fonts, emoji fallback) | `jam::Typeface` — flat `hb_font_t` interning table (`jam_Typeface.h`); cmap lookup only, no general shaping |
 * | `typeface.shapeText()`/`shapeEmoji()` (full shape per glyph) | `hb_font_get_nominal_glyph()` cmap lookup (fallback `juce::Typeface::getNominalGlyphForCodepoint()`); HarfBuzz shaping (`hb_shape`) reserved for `tryLigature()` only |
 * | flexGap-based wrap-point backtracking inside `buildArrangements` | none — wrap ownership already moved to `CodeView`/`jam::TextLine::getWrappedLines()` (naive ceiling-division, landed prior sprint); reproducing flexGap backtracking here would silently diverge from that already-shipped row count |
 * | Coordinate query methods (`indexAtPosition`, `positionForIndex`, `boundsForRange`, `getLineForIndex`, `getIndexForLine`) | not restored — `CodeView` already owns document/pixel coordinate mapping (`pixelToDocumentPosition`, `locateRow`) |
 *
 * `tryLigature()`'s HarfBuzz flow (ASCII/same-style window, `hb_buffer` +
 * `hb_shape`) is recovered from END's own history
 * (`Source/terminal/rendering/ScreenRender.cpp` @ 40d2163, `Screen::tryLigature`)
 * rather than from the deleted god object.
 *
 * @par Emoji / CJK-fallback / grapheme conformance restoration
 * `findGlyph()` restores the god object's `shapeCodepoint()` resolution
 * order (`jam_GlyphArrangementShape.cpp`, e8543bf1a^): (0) grapheme clusters
 * only — `hb_shape()` the full cluster against the platform emoji face first
 * (`findClusterGlyph()`, see "Grapheme-cluster shaping" below); then, on a
 * cluster-shape miss or a plain (non-cluster) cell: (1) run typeface cmap,
 * (2) miss + codepoint > `emojiCandidateFloor` + platform emoji typeface has
 * the glyph → emoji, (3) still missing → per-codepoint generic-fallback
 * discovery (`juce::Font::findSuitableFontForText()`, cached, failures cached
 * too), each step decomposed into `findMonoGlyph()`/`findEmojiGlyph()`/
 * `findFallbackGlyph()`. Both the emoji typeface (resolved at construction)
 * and each fallback typeface (`getOrLoadFallbackTypeface()`) are interned into
 * the SAME `jam::Typeface` table `shapeCell()`/`draw()` already consult (via
 * `jam::Typeface::registerTypeface()` — the platform emoji typeface via the
 * owned-bytes overload, generic fallback typefaces via the bytes-less overload
 * — `jam_Typeface.h`) — `draw()` and `RunKey`'s existing `isEmoji` batching
 * need no special case.
 *
 * @par Grapheme-cluster shaping (restored — was "Known gap")
 * The god object's `shapeCodepoint()` reshaped a full grapheme cluster
 * (`jam::Grapheme::Entry`) against `shapeEmoji()` first (VS16/ZWJ sequences —
 * family emoji, flags — collapsing to fewer glyphs), falling back to the
 * cluster's base codepoint only when that failed. That `hb_shape()` call needs
 * a real `hb_font_t` for the emoji face; the constructor now builds
 * one from the emoji family's own on-disk font-file bytes
 * (`jam::findSystemFontFile()`, the same route `GlyphAtlas::
 * getOrLoadSystemTypeface()`'s tier-2 uses) instead of the `@internal`
 * `Typeface::getNativeDetails()` route the god object used — see
 * `jam_Typeface.h`'s "Owned-bytes construction" doc comment for the full
 * mechanism. `findClusterGlyph()` restores the god object's own shape-first
 * order: on `contentGrapheme` cells, `hb_shape()` the full cluster against
 * that `hb_font_t`; a single resulting glyph with a non-zero glyph id resolves
 * the ENTIRE cluster to one `Entry` (VS16/ZWJ family/flag sequences collapse to
 * one glyph, same as the god object). On a miss (no emoji `hb_font_t` — e.g.
 * `jam::findSystemFontFile()` could not resolve the platform emoji family's
 * file — or HarfBuzz did not collapse the cluster), the god object's own
 * last-resort path is reached: `getCodepoint()`'s base-codepoint
 * extraction, then steps (1)-(3) above, same as a plain cell.
 *
 * @par Thread contract
 * All methods must be called on the **MESSAGE THREAD**.
 *
 * @see jam::Typeface
 * @see jam::TextModel
 * @see jam::Stamp
 * @see jam::Grapheme
 */
#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/**
 * @class GlyphArrangement
 * @brief Monospace layout engine that shapes a `TextModel` document into `Run` objects.
 *
 * Call `shape()` once per paint over the visible document line range, then
 * `arrange()`, then `draw()` — three explicit steps (not chained
 * internally) so a caller may inspect/patch `getEntry()` between the first and
 * second pass if ever needed (mirrors the old `rebuildDrawRuns()` seam).
 *
 * @par Thread contract
 * **MESSAGE THREAD** — all methods.
 */
class GlyphArrangement
{
public:
    /**
     * @struct Entry
     * @brief A single shaped glyph with its resolved cell position, colour, and typeface.
     *
     * Intermediate result of `shape()`'s first pass. Positions are cell
     * coordinates (col, row) — pixel conversion happens in `arrange()`.
     */
    struct Entry
    {
        uint16_t               glyphIndex { 0 };       ///< Font-internal glyph ID (cmap or ligature-shaped).
        uint8_t                span       { 1 };        ///< Display width in cells (1 = narrow, 2 = wide, >2 = ligature).
        float                  advance    { 0.0f };     ///< This glyph's own pixel advance width — cellWidth (or 2*cellWidth for wide) for monospace cells; HarfBuzz-glyph-metric-derived for proportional cells (see file doc's proportional restoration note).
        char32_t               codepoint  { 0 };        ///< Resolved codepoint (grapheme clusters: base codepoint).
        int                    col        { 0 };        ///< Cell column.
        int                    row        { 0 };        ///< Cell row.
        int                    cell       { 0 };        ///< Source cell index within the shaped line (jam::TextLine::chars) — skipped cells (spacerTail, unresolved shapeCell) leave gaps, so this is never simply the Entry's own array index.
        float                  originX    { 0.0f };     ///< Resolved pixel x-position (pen origin) for this glyph — set at shape() time; arrange() reads this directly instead of recomputing from `col`.
        juce::Colour           colour     {};           ///< Resolved foreground colour.
        juce::Colour           bg         {};           ///< Resolved background colour (alpha=0 sentinel = transparent).
        const juce::Typeface*  typeface   { nullptr };  ///< Non-owning typeface identity — resolved back via `jam::Typeface::find()`; lifetime guaranteed by setTypefaces()'s contract. May be the run typeface, the platform emoji typeface, or a resolved generic-fallback typeface — see findGlyph().
        bool                   isEmoji    { false };    ///< `true` when resolved via the platform emoji typeface (findEmojiGlyph()) — see file doc's conformance restoration note.
        uint16_t               style      { 0 };        ///< SGR flag bitmask copied from `Stamp::Entry::flags`.
    };

    /**
     * @struct Run
     * @brief A run of glyphs sharing the same typeface, `isEmoji` state, and colour.
     *
     * Each `Run` corresponds to exactly one `LowLevelGraphicsContext::drawGlyphs`
     * call — grouping by typeface is mandatory because `drawGlyphs` draws through
     * a single `setFont()` call per invocation.
     */
    struct Run
    {
        juce::HeapBlock<uint16_t>           glyphCodes {};  ///< Font-internal glyph indices.
        juce::HeapBlock<juce::Point<float>> positions  {};  ///< Pixel positions per glyph (from cell coords).
        juce::HeapBlock<char32_t>           codepoints {};  ///< Original codepoints per glyph.
        juce::HeapBlock<uint8_t>            spans      {};  ///< Display width in cells per glyph.
        juce::HeapBlock<uint16_t>           styles     {};  ///< SGR flag bitmask per glyph.
        juce::HeapBlock<juce::Colour>       bgColours  {};  ///< Background colour per glyph.
        int                    count      { 0 };            ///< Number of valid entries in all arrays.
        const juce::Typeface*  typeface   { nullptr };      ///< Non-owning typeface identity for this run; lifetime guaranteed by setTypefaces()'s contract.
        juce::Colour           colour     {};                ///< Uniform foreground colour for this run.
        bool                   isEmoji    { false };          ///< True when glyphs must be rasterised via the emoji atlas.
    };

    GlyphArrangement() noexcept;
    ~GlyphArrangement() noexcept;

    // =========================================================================
    // Configuration — TOLD by the caller (CodeView), never internally derived
    // =========================================================================

    /**
     * @brief Sets the regular and bold typeface identities used for cmap lookup.
     *
     * Raw pointers — resolved back to their `jam::Typeface` table entry via
     * `jam::Typeface::find()` at `shape()` time. Registration (interning the
     * `hb_font_t`) is the caller's responsibility (`jam::Typeface::
     * registerTypeface()`), mirroring how `GlyphAtlas::registerTypeface()` is
     * already called with the same embedded font bytes elsewhere.
     *
     * Italic carries no separate face — `jam::Stamp::italic` never changes
     * which typeface is selected (matches the pre-restoration `CodeView::
     * styledFont()`'s own italic handling: a style flag, not a font
     * substitution).
     *
     * @par Lifetime contract
     * Non-owning — this class stores raw pointer identity only. The caller
     * must keep both typefaces alive for as long as this arrangement (and any
     * `Entry`/`Run` objects built from it via `shape()`/`arrange()`) may
     * reference them. In END the anchors are `CodeView`'s retained
     * `jam::Typeface::Ptr`s plus the `jam::Typeface` table itself.
     *
     * @param newRegularTypeface  Typeface identity for plain/italic text.
     * @param newBoldTypeface     Typeface identity for `Stamp::bold` text.
     */
    void setTypefaces (const juce::Typeface* newRegularTypeface, const juce::Typeface* newBoldTypeface) noexcept;

    /** @brief Sets the cell width/height in pixels. TOLD, never font-derived. */
    void setCellSize (int newCellWidth, int newCellHeight) noexcept;

    /** @brief Sets the baseline offset in pixels (cell top to glyph baseline). TOLD, never font-derived. */
    void setBaseline (int newBaseline) noexcept;

    /** @brief Sets the font size (`juce::Font::getHeight()`'s JUCE height unit —
     *  same convention as `GlyphAtlas::Key::make()`'s `fontHeight` parameter)
     *  used to build the draw `juce::Font` via `FontOptions::withHeight()`.
     *  TOLD, never derived. */
    void setFontSize (float newFontSize) noexcept;

    /**
     * @brief Enables or disables ASCII ligature detection in `shape()`.
     * @see tryLigature
     */
    void setLigatures (bool enabled) noexcept;

    /**
     * @brief Sets the colour `shapeCell()`/`tryLigature()` substitute for a
     * cell whose resolved `jam::Stamp::Entry::fg` is the transparent
     * (alpha == 0) default — the default-constructed `Stamp::Entry{}`'s own
     * value (styleId 0, `jam_Stamp.h`'s `Entry()` default). Without this
     * substitution such a cell's glyph draws with a fully transparent
     * foreground colour — invisible. The owning text view is expected to
     * call this once per paint with its current theme text colour so
     * LookAndFeel/theme changes are picked up immediately. TOLD, never
     * derived — mirrors every other `set*()` in this "Configuration" group.
     * @param newColour  Foreground colour substituted for alpha == 0 `fg`.
     */
    void setDefaultTextColour (juce::Colour newColour) noexcept;

    // =========================================================================
    // Layout — two explicit passes
    // =========================================================================

    /**
     * @brief Second pass: groups the entries from the last `shape()` call into `Run`s.
     *
     * Groups by (typeface, isEmoji, colour) — mandatory since `drawGlyphs`
     * resolves a single typeface per call. Converts cell coordinates to pixel
     * positions using the metrics set via `setCellSize()`/`setBaseline()`.
     *
     * @note **MESSAGE THREAD**.
     */
    void arrange() noexcept;

    /**
     * @brief Dispatches every draw run from the last `arrange()` call.
     *
     * Per run: resolves the run's typeface back to its `jam::Typeface::Ptr`
     * (`jam::Typeface::find()`), builds a `juce::Font` at `setFontSize()`'s
     * value via `juce::FontOptions{}.withTypeface(...).withHeight(...)`, sets
     * the run's colour (`juce::Graphics::setColour`), then dispatches through
     * the LowLevelGraphicsContext instanced path (`setFont`/`drawGlyphs`) —
     * GPU (Vulkan LLGC) or CPU (`LowLevelGraphicsGlyphRenderer`) dispatch is
     * the VulkanEngine's business; this method never names an engine.
     *
     * @param g  Graphics context to draw into.
     *
     * @note **MESSAGE THREAD**.
     */
    void draw (juce::Graphics& g) const;

    // =========================================================================
    // Accessors
    // =========================================================================

    /** @brief Returns the number of draw runs produced by the last `arrange()` call. */
    int getNumDrawRuns() const noexcept { return numDrawRuns; }

    /**
     * @brief Returns the draw run at the given index.
     * @param index  Zero-based run index. Asserted in [0, getNumDrawRuns()).
     */
    const Run& getDrawRun (int index) const noexcept
    {
        jassert (index >= 0 and index < numDrawRuns);
        return drawRuns[index];
    }

    /** @brief Returns the number of shaped entries produced by the last `shape()` call. */
    int getNumEntries() const noexcept { return numEntries; }

    /**
     * @brief Returns the shaped entry at the given index.
     * @param index  Zero-based entry index. Asserted in [0, getNumEntries()).
     */
    const Entry& getEntry (int index) const noexcept
    {
        jassert (index >= 0 and index < numEntries);
        return entries[index];
    }

private:
    // =========================================================================
    // Internal shaping helpers
    // =========================================================================

    /**
     * @struct GlyphLookup
     * @brief Result of findGlyph(): codepoint, glyph index, typeface identity,
     *        emoji flag, and found flag.
     *
     * Result-return style (CODING.md-sanctioned) — findGlyph() and its
     * findMonoGlyph()/findEmojiGlyph()/findFallbackGlyph() steps each
     * return this by value instead of writing through nested out-parameters,
     * keeping shapeCell()'s nesting flat.
     */
    struct GlyphLookup
    {
        char32_t               codepoint  { 0 };        ///< Resolved codepoint (grapheme cluster base, or the cell's own codepoint).
        uint16_t               glyphIndex { 0 };         ///< Font-internal glyph ID. Only meaningful when `found`.
        const juce::Typeface*  typeface   { nullptr };  ///< Resolved glyph's owning typeface identity (run typeface, emoji, or fallback). Only meaningful when `found`.
        bool                   isEmoji    { false };     ///< `true` iff resolved via the platform emoji typeface.
        bool                   found      { false };     ///< `true` iff a glyph was found for `codepoint`.
    };

    /**
     * @brief Resolves a cell's codepoint: grapheme cluster base, or its own codepoint.
     *
     * Grapheme clusters (`contentTag() == contentGrapheme`) resolve against
     * the cluster's base (first) codepoint; plain cells resolve against their
     * own codepoint directly.
     *
     * @param pen  Source cell (may carry a grapheme-cluster index as its codepoint).
     * @return     Resolved codepoint (0 if a grapheme-cluster index was out of range).
     */
    char32_t getCodepoint (jam::AttributedChar pen) const noexcept;

    /**
     * @brief Resolves one cell to a codepoint and font-internal glyph index.
     *
     * Grapheme clusters try findClusterGlyph() FIRST (the god object's own
     * shape-first order — see file doc's "Grapheme-cluster shaping" note); on a
     * miss (plain cell, or a cluster the emoji face's `hb_shape()` did not
     * collapse), falls through to getCodepoint() then the god object's
     * `shapeCodepoint()` resolution order (see file doc's conformance
     * restoration note): findMonoGlyph() on @p typefaceEntry's cmap; on miss,
     * if the codepoint qualifies (exceeds `emojiCandidateFloor`), findEmojiGlyph();
     * still missing, findFallbackGlyph().
     *
     * @param pen            Source cell (may carry a grapheme-cluster index as its codepoint).
     * @param typefaceEntry  Resolved `jam::Typeface` table entry providing the cmap.
     * @return               `GlyphLookup` with `found == true` and `typeface`/`glyphIndex`/`isEmoji` set on success.
     */
    GlyphLookup findGlyph (jam::AttributedChar pen, const jam::Typeface::Entry& typefaceEntry) const noexcept;

    /**
     * @brief Resolution step 0: full grapheme-cluster HarfBuzz shaping against the emoji face.
     *
     * Restores the god object's `shapeCodepoint()` shape-first order (see file
     * doc's "Grapheme-cluster shaping" note): for `contentGrapheme` cells only,
     * fetches the full cluster (`jam::Grapheme::Entry`, up to 8 codepoints),
     * reads the platform emoji typeface resolved at construction, and — only
     * if that typeface's interned `Entry`
     * carries a real `hb_font_t` (the owned-bytes route,
     * `jam_Typeface.h`) — `hb_shape()`s the whole cluster against it
     * (`hb_buffer_add_utf32` + `hb_buffer_guess_segment_properties`, same idiom
     * as tryLigature()). A single resulting glyph with a non-zero glyph id
     * resolves the entire cluster (VS16/ZWJ family/flag sequences collapsing to
     * one glyph, same as the god object): `codepoint` is the cluster's base
     * codepoint, `typeface` is the emoji typeface, `isEmoji` is `true`. Returns
     * an unresolved `GlyphLookup` for plain cells, when no real emoji
     * `hb_font_t` is available, or when HarfBuzz did not collapse the cluster —
     * findGlyph() then falls through to getCodepoint()'s base-codepoint
     * extraction, the god object's own last-resort path.
     *
     * @param pen  Source cell (may carry a grapheme-cluster index as its codepoint).
     * @return     `GlyphLookup` with `found == true` on a successful cluster shape.
     */
    GlyphLookup findClusterGlyph (jam::AttributedChar pen) const noexcept;

    /**
     * @brief Resolution step 1: cmap lookup on the cell's own run typeface.
     *
     * `hb_font_get_nominal_glyph()` on @p typefaceEntry's `hb_font_t`, falling
     * back to `juce::Typeface::getNominalGlyphForCodepoint()` when the HarfBuzz
     * lookup is unavailable or misses. On success, `typeface` is @p
     * typefaceEntry's own `juceTypeface`, `isEmoji` stays `false`.
     *
     * @param codepoint      Resolved codepoint (from getCodepoint()).
     * @param typefaceEntry  Resolved `jam::Typeface` table entry providing the cmap.
     * @return               `GlyphLookup` with `found == true` on a cmap hit.
     */
    GlyphLookup findMonoGlyph (char32_t codepoint, const jam::Typeface::Entry& typefaceEntry) const noexcept;

    /**
     * @brief Resolution step 2: cmap lookup on the platform emoji typeface.
     *
     * Reads the platform emoji typeface resolved at construction, then
     * `juce::Typeface::getNominalGlyphForCodepoint()`
     * on it (the O(1) cmap-equivalent check the god object's `hasEmojiGlyph()`
     * performed via `FT_Get_Char_Index`/`CTFontGetGlyphsForCharacters`) —
     * this single-codepoint cmap check stays on the `juce::Typeface` API even
     * though a real `hb_font_t` may now be available for the emoji face (see
     * findClusterGlyph(), resolution step 0): that `hb_font_t` exists to
     * `hb_shape()` a full multi-codepoint cluster, not to answer a one-codepoint
     * cmap query this API already answers directly. On success, `typeface` is
     * the emoji typeface, `isEmoji` is `true`.
     *
     * @param codepoint  Resolved codepoint. Caller gates this call on
     *                   `codepoint` exceeding `emojiCandidateFloor`, matching
     *                   the god object's own `shapeCodepoint()` gate.
     * @return           `GlyphLookup` with `found == true` on a cmap hit.
     */
    GlyphLookup findEmojiGlyph (char32_t codepoint) const noexcept;

    /**
     * @brief Resolution step 3: per-codepoint generic-fallback discovery.
     *
     * Resolves/interns a per-codepoint system fallback typeface via
     * getOrLoadFallbackTypeface(), then `juce::Typeface::
     * getNominalGlyphForCodepoint()` on it. On success, `typeface` is the
     * fallback typeface, `isEmoji` stays `false` — restores the god object's
     * `shapeFallback()` semantics (CTFontCreateForString/
     * IDWriteFontFallback::MapCharacters) via `juce::Font::
     * findSuitableFontForText()`, the cross-platform public equivalent.
     *
     * @param codepoint  Resolved codepoint.
     * @return           `GlyphLookup` with `found == true` on success.
     */
    GlyphLookup findFallbackGlyph (char32_t codepoint) const noexcept;

    /**
     * @brief Resolves (and interns) a per-codepoint system fallback typeface.
     *
     * Restores the god object's `shapeFallback()` per-codepoint discovery and
     * caching semantics: `juce::Font::findSuitableFontForText()` queried
     * against a `juce::Font` built from `regularTypeface` (the god object's
     * `CTFontCreateForString (mainFont, ...)` — the candidate is derived FROM
     * the primary font for style/script consistency, not created from
     * scratch). Rejects a candidate typeface named "LastResort" — the same
     * sentinel the god object's `shapeFallback()` rejected by PostScript name;
     * on macOS `Typeface::getName()` returns that same family name for
     * CoreText's last-resort font, so the public `getName()` check is
     * equivalent without needing the (inaccessible) PostScript name API.
     * Successful and failed resolutions are both cached in
     * `fallbackTypefaceCache`, keyed by @p codepoint — a failure is never
     * re-queried, matching the god object's own `fallbackFontCache` semantics.
     * Requires `regularTypeface` to already be registered in the `jam::Typeface`
     * table (`shapeRow()`'s own precondition for `regularIndex`); returns
     * `nullptr` without querying if it is not yet registered.
     *
     * @param codepoint  Unicode scalar to resolve a fallback typeface for.
     * @return           Non-owning identity of the interned fallback typeface,
     *                   or `nullptr` if no system font covers @p codepoint.
     */
    const juce::Typeface* getOrLoadFallbackTypeface (char32_t codepoint) const noexcept;

    /**
     * @brief Resolves one `jam::AttributedChar` cell to a single `Entry` via findGlyph().
     *
     * Grapheme clusters (`contentTag() == contentGrapheme`) try full-cluster
     * `hb_shape()` against the emoji face first (findGlyph()'s step 0,
     * findClusterGlyph()) — VS16/ZWJ family/flag sequences collapse to the
     * ONE glyph HarfBuzz's shaper produces, filling this same one-glyph-per-cell
     * `Entry` (no intra-cell glyph offset needed, since the whole cluster IS one
     * glyph). On a cluster-shape miss, resolution falls through to the cluster's
     * base (first) codepoint only, same as before this restoration — full
     * multi-glyph cluster composition (independently positioned combining marks
     * within one cell) is still out of scope for this Entry model and is not
     * restored here.
     *
     * @param pen            Source cell.
     * @param style          Resolved `Stamp::Entry` for this cell's styleId.
     * @param typefaceIndex  `jam::Typeface` table index selected for this cell's style (-1 = unregistered).
     * @param col            Cell column for the written Entry.
     * @param row            Cell row for the written Entry.
     * @param out            Entry to fill.
     * @return               `true` if a glyph was resolved and @p out was written.
     */
    bool shapeCell (jam::AttributedChar pen, const jam::Stamp::Entry& style, int typefaceIndex,
                    int col, int row, Entry& out) const noexcept;

    /**
     * @brief Attempts to shape a 2- or 3-character ASCII ligature starting at @p startIndex.
     *
     * Tries sequence lengths 3 then 2 (longest match first). For each length,
     * every cell in the run must be ASCII — `codepoint` greater than 0 and
     * less than `0x80` — plain codepoint content (not grapheme/wide-continuation), and share the first
     * cell's styleId. Eligible runs are shaped via `hb_shape` on the selected
     * typeface's `hb_font_t`; the run is a ligature iff HarfBuzz collapses it to
     * exactly one output glyph. The resulting `Entry.span` covers the full
     * matched length (`tryLen`) so the caller advances its cursor by `tryLen`
     * cells total — equivalent to the recovered END history's `ligatureSkip =
     * tryLen - 1` convention (that plus the one cell the outer loop would have
     * advanced anyway).
     *
     * @param chars           Cell array (same array `startIndex` indexes into).
     * @param startIndex      First cell index to try.
     * @param cellEnd         Exclusive end of the available cell range.
     * @param typefaceIndex   `jam::Typeface` table index for this run's style (-1 = unregistered).
     * @param col             Cell column for the written Entry.
     * @param row             Cell row for the written Entry.
     * @param style           Resolved `Stamp::Entry` for this run's styleId.
     * @param out             Entry to fill on a ligature match.
     * @return                Total cells consumed (`tryLen`) on a match; 0 if no ligature found.
     */
    int tryLigature (const jam::AttributedChar* chars, int startIndex, int cellEnd, int typefaceIndex,
                     int col, int row, const jam::Stamp::Entry& style, Entry& out) noexcept;

    /** @brief Frees `drawRuns`' inner HeapBlocks and resets the draw-run count. */
    void freeDrawRuns() noexcept;

    /**
     * @struct RunKey
     * @brief Key identifying one unique draw run: typeface, colour, and emoji flag.
     *
     * `arrange()`'s grouping key — declared at class scope (not inside the
     * method body) per struct discipline.
     */
    struct RunKey
    {
        const juce::Typeface* typeface { nullptr }; ///< Non-owning typeface identity; lifetime guaranteed by setTypefaces()'s contract.
        juce::Colour          colour   {};
        bool                  isEmoji  { false };

        /** @brief True when @p entry belongs to the run identified by this key. */
        bool matches (const Entry& entry) const noexcept
        {
            return typeface == entry.typeface
               and colour   == entry.colour
               and isEmoji  == entry.isEmoji;
        }
    };

    /**
     * @brief Linear scan for @p entry's matching RunKey index.
     * @param keys     Key array accumulated so far.
     * @param numKeys  Number of valid entries in @p keys.
     * @param entry    Entry to match.
     * @return         Matching index, or @p numKeys if no existing key matches (a new key is needed).
     */
    static int findRunKeyIndex (const RunKey* keys, int numKeys, const Entry& entry) noexcept;

    // =========================================================================
    // Constants
    // =========================================================================

    /** @brief Exclusive upper bound of the ASCII range tryLigature() operates in. */
    static constexpr char32_t asciiCeiling { 0x80 };

    /** @brief Longest ligature sequence length tryLigature() attempts (tried first). */
    static constexpr int maxLigatureLength { 3 };

    /** @brief Shortest ligature sequence length tryLigature() attempts (tried last). */
    static constexpr int minLigatureLength { 2 };

    /** @brief Exclusive lower bound a resolved codepoint must exceed before
     *  findGlyph() considers the emoji path on a run-typeface cmap miss —
     *  the god object's own `shapeCodepoint()` gate — `codepoint` exceeding
     *  `0x7eu` — verbatim. */
    static constexpr char32_t emojiCandidateFloor { 0x7Eu };

    // =========================================================================
    // Data
    // =========================================================================

    juce::HeapBlock<Entry> entries    {};    ///< Shaped entries from the last shape() call.
    int                     numEntries { 0 }; ///< Number of valid entries in `entries`.

    juce::HeapBlock<Run>   drawRuns    {};    ///< Draw runs from the last arrange() call.
    int                    numDrawRuns { 0 }; ///< Number of valid entries in `drawRuns`.

    const juce::Typeface* regularTypeface { nullptr }; ///< Non-owning plain/italic typeface identity — set via setTypefaces(), lifetime per its contract.
    const juce::Typeface* boldTypeface    { nullptr }; ///< Non-owning `Stamp::bold` typeface identity — set via setTypefaces(), lifetime per its contract.

    int   cellWidth  { 0 };     ///< Cell width in pixels — TOLD via setCellSize().
    int   cellHeight { 0 };     ///< Cell height in pixels — TOLD via setCellSize().
    int   baseline   { 0 };     ///< Baseline offset in pixels — TOLD via setBaseline().
    float fontSize   { 0.0f };  ///< JUCE height unit for the draw-time juce::Font — TOLD via setFontSize().

    bool ligaturesEnabled { false }; ///< Gate for tryLigature() — TOLD via setLigatures().

    /** @brief Substituted for a transparent (alpha == 0) `Stamp::Entry::fg` —
     *  TOLD via setDefaultTextColour(). Defaults transparent itself (matches
     *  `juce::Colour()`'s own default) until the caller's first paint sets
     *  it — see setDefaultTextColour()'s doc comment. */
    juce::Colour defaultTextColour {};

    /** @brief Reusable HarfBuzz buffer for tryLigature()'s hb_shape calls — allocated once, reset per call. */
    hb_buffer_t* ligatureBuffer { nullptr };

    /** @brief Reusable HarfBuzz buffer for findClusterGlyph()'s hb_shape
     *  calls against the emoji face — allocated once (alongside `ligatureBuffer`
     *  in the constructor), reset per call, destroyed alongside `ligatureBuffer`
     *  in the destructor. `mutable`: findGlyph() (and findClusterGlyph(),
     *  which it calls) is `const` — resetting/filling this scratch buffer per
     *  call is not a caller-visible state change, same category as
     *  `fallbackTypefaceCache` below (caching/scratch on a
     *  `const` resolution path). Not the same buffer as `ligatureBuffer`
     *  (non-`mutable`, owned by the non-`const` tryLigature()) — a shared buffer
     *  would need `const_cast` on this `const` path, which this codebase forbids. */
    mutable hb_buffer_t* clusterBuffer { nullptr };

    /** @brief Platform emoji typeface identity, resolved once at construction.
     *  `nullptr` when the platform emoji family is not installed or
     *  `jam::Typeface` was not yet wired at construction time. */
    juce::Typeface::Ptr emojiTypefacePtr {};

    /** @brief Per-codepoint system fallback typeface cache — see
     *  getOrLoadFallbackTypeface(). A cached `nullptr` means "queried, no system
     *  font covers this codepoint", never re-queried — mirrors the god
     *  object's own `fallbackFontCache` semantics exactly. */
    mutable jam::HashMap<char32_t, const juce::Typeface*> fallbackTypefaceCache {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlyphArrangement)
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
