//
// Implementation of jam::GlyphArrangement — shape dispatch (cmap + tryLigature),
// draw-run construction (arrange), and LLGC dispatch (draw). See
// jam_GlyphArrangement.h for the old->new conformance provenance table.

namespace jam
{ /*____________________________________________________________________________*/

// ============================================================================
// Lifecycle
// ============================================================================

GlyphArrangement::GlyphArrangement() noexcept
    : ligatureBuffer (hb_buffer_create()), clusterBuffer (hb_buffer_create())
{
    static const juce::String platformEmojiFamilyName {
    #if JUCE_MAC
        "Apple Color Emoji"
    #elif JUCE_WINDOWS
        "Segoe UI Emoji"
    #else
        "Noto Color Emoji"
    #endif
    };

    const juce::Font emojiFont { juce::FontOptions{}.withName (platformEmojiFamilyName) };
    auto              typefacePtr { emojiFont.getTypefacePtr() };
    auto*             typefaceTable { jam::Typeface::getInstance() };

    if (typefacePtr != nullptr and typefaceTable != nullptr)
    {
    #if JUCE_MAC || JUCE_WINDOWS
        const auto        fontFile  { jam::findSystemFontFile (platformEmojiFamilyName, typefacePtr->getStyle()) };
        juce::MemoryBlock fontBytes;

        if (fontFile.existsAsFile() and fontFile.loadFileAsData (fontBytes))
            typefaceTable->registerTypeface (typefacePtr, std::move (fontBytes));
        else
            typefaceTable->registerTypeface (typefacePtr);
    #else
        typefaceTable->registerTypeface (typefacePtr);
    #endif

        emojiTypefacePtr = std::move (typefacePtr);
    }
}

GlyphArrangement::~GlyphArrangement() noexcept
{
    // entries is a flat juce::HeapBlock<Entry> with no nested heap
    // allocations of its own — its own member destructor (std::free on the
    // flat block) is sufficient; no manual entries.free() needed here.
    //
    // drawRuns' Run elements each own further juce::HeapBlock buffers
    // (glyphCodes/positions/codepoints/spans/styles/bgColours). juce::
    // HeapBlock<T>::malloc/calloc/free are raw std::malloc/calloc/free
    // wrappers (juce_HeapBlock.h) — they never construct or destruct T, so
    // drawRuns' own destructor cannot reach those nested buffers. freeDrawRuns()
    // is the only path that releases them; kept here (not "manual RAII" —
    // legitimate non-RAII cleanup, same category as hb_buffer_destroy below).
    freeDrawRuns();
    hb_buffer_destroy (ligatureBuffer);
    hb_buffer_destroy (clusterBuffer);
}

// ============================================================================
// Configuration
// ============================================================================

void GlyphArrangement::setTypefaces (const juce::Typeface* newRegularTypeface, const juce::Typeface* newBoldTypeface) noexcept
{
    regularTypeface = newRegularTypeface;
    boldTypeface    = newBoldTypeface;
}

void GlyphArrangement::setCellSize (int newCellWidth, int newCellHeight) noexcept
{
    cellWidth  = newCellWidth;
    cellHeight = newCellHeight;
}

void GlyphArrangement::setBaseline (int newBaseline) noexcept
{
    baseline = newBaseline;
}

void GlyphArrangement::setFontSize (float newFontSize) noexcept
{
    fontSize = newFontSize;
}

void GlyphArrangement::setLigatures (bool enabled) noexcept
{
    ligaturesEnabled = enabled;
}

void GlyphArrangement::setDefaultTextColour (juce::Colour newColour) noexcept
{
    defaultTextColour = newColour;
}

// ============================================================================
// GlyphArrangement::getCodepoint
// ============================================================================

char32_t GlyphArrangement::getCodepoint (jam::AttributedChar pen) const noexcept
{
    char32_t codepoint { static_cast<char32_t> (pen.codepoint()) };

    if (pen.contentTag() == jam::AttributedChar::contentGrapheme
        and pen.codepoint() < static_cast<uint32_t> (jam::Grapheme::getInstance()->size()))
    {
        const auto& cluster { jam::Grapheme::getInstance()->get (static_cast<int> (pen.codepoint())) };

        if (cluster.count > 0)
            codepoint = cluster.codepoints.at (0);
    }

    return codepoint;
}

// ============================================================================
// GlyphArrangement::findGlyph
// ============================================================================

GlyphArrangement::GlyphLookup GlyphArrangement::findGlyph (jam::AttributedChar pen, const jam::Typeface::Entry& typefaceEntry) const noexcept
{
    // Step 0 — grapheme clusters only: full-cluster hb_shape() against the
    // emoji face, the god object's own shape-first order (file doc's
    // "Grapheme-cluster shaping" note). A miss here (plain cell, no real
    // emoji hb_font_t, or HarfBuzz did not collapse the cluster) falls through
    // to the pre-existing base-codepoint steps (1)-(3) below unchanged.
    GlyphLookup result { findClusterGlyph (pen) };

    if (not result.found)
    {
        const char32_t codepoint { getCodepoint (pen) };
        result.codepoint = codepoint;

        if (codepoint != 0)
        {
            result = findMonoGlyph (codepoint, typefaceEntry);

            if (not result.found and codepoint > emojiCandidateFloor)
                result = findEmojiGlyph (codepoint);

            if (not result.found)
                result = findFallbackGlyph (codepoint);
        }
    }

    return result;
}

// ============================================================================
// GlyphArrangement::findClusterGlyph  (resolution step 0: full-cluster
// HarfBuzz shaping against the platform emoji face)
// ============================================================================

GlyphArrangement::GlyphLookup GlyphArrangement::findClusterGlyph (jam::AttributedChar pen) const noexcept
{
    GlyphLookup result;

    if (pen.contentTag() == jam::AttributedChar::contentGrapheme
        and pen.codepoint() < static_cast<uint32_t> (jam::Grapheme::getInstance()->size()))
    {
        const auto& cluster       { jam::Grapheme::getInstance()->get (static_cast<int> (pen.codepoint())) };
        const auto* emojiTypeface { emojiTypefacePtr.get() };
        auto*       typefaceTable { jam::Typeface::getInstance() };
        const int   emojiIndex    { typefaceTable != nullptr and emojiTypeface != nullptr
                                        ? typefaceTable->find (emojiTypeface) : -1 };

        if (cluster.count > 0 and emojiIndex >= 0)
        {
            const auto& emojiEntry { typefaceTable->get (emojiIndex) };

            // Only a real hb_font_t (the owned-bytes route, jam_Typeface.h)
            // can hb_shape() a multi-codepoint cluster — the bytes-less route
            // (findSystemFontFile() failed, or JUCE_LINUX) leaves hbFont
            // null, and this step simply never fires (positive check, no
            // error path); findGlyph() falls through to the base-codepoint
            // steps below.
            if (emojiEntry.hbFont != nullptr)
            {
                std::array<uint32_t, std::tuple_size_v<decltype (cluster.codepoints)>> codepoints {};

                for (uint8_t offset { 0 }; offset < cluster.count; ++offset)
                    codepoints.at (offset) = jam::toU32 (cluster.codepoints.at (offset));

                hb_buffer_reset (clusterBuffer);
                hb_buffer_add_utf32 (clusterBuffer, codepoints.data(), static_cast<int> (cluster.count),
                                     0, static_cast<int> (cluster.count));
                hb_buffer_guess_segment_properties (clusterBuffer);
                hb_shape (emojiEntry.hbFont, clusterBuffer, nullptr, 0);

                unsigned int     glyphCount { 0 };
                hb_glyph_info_t* glyphInfo  { hb_buffer_get_glyph_infos (clusterBuffer, &glyphCount) };

                if (glyphCount >= 1 and glyphInfo[0].codepoint != 0)
                {
                    result.codepoint  = cluster.codepoints.at (0);
                    result.glyphIndex = static_cast<uint16_t> (glyphInfo[0].codepoint);
                    result.typeface   = emojiTypeface;
                    result.isEmoji    = true;
                    result.found   = true;
                }
            }
        }
    }

    return result;
}

// ============================================================================
// GlyphArrangement::findMonoGlyph  (resolution step 1: run typeface cmap)
// ============================================================================

GlyphArrangement::GlyphLookup GlyphArrangement::findMonoGlyph (char32_t codepoint, const jam::Typeface::Entry& typefaceEntry) const noexcept
{
    GlyphLookup result;
    result.codepoint = codepoint;

    hb_codepoint_t hbGlyph { 0 };

    if (typefaceEntry.hbFont != nullptr
        and hb_font_get_nominal_glyph (typefaceEntry.hbFont, jam::toU32 (codepoint), &hbGlyph) != 0)
    {
        result.glyphIndex = static_cast<uint16_t> (hbGlyph);
        result.typeface   = typefaceEntry.juceTypeface.get();
        result.found   = true;
    }
    else if (typefaceEntry.juceTypeface != nullptr)
    {
        const auto nominal { typefaceEntry.juceTypeface->getNominalGlyphForCodepoint (
            jam::toChar (codepoint)) };

        if (nominal.has_value())
        {
            result.glyphIndex = static_cast<uint16_t> (*nominal);
            result.typeface   = typefaceEntry.juceTypeface.get();
            result.found   = true;
        }
    }

    return result;
}

// ============================================================================
// GlyphArrangement::findEmojiGlyph  (resolution step 2: platform emoji font)
// ============================================================================

GlyphArrangement::GlyphLookup GlyphArrangement::findEmojiGlyph (char32_t codepoint) const noexcept
{
    GlyphLookup result;
    result.codepoint = codepoint;

    const auto* emojiTypeface { emojiTypefacePtr.get() };

    if (emojiTypeface != nullptr)
    {
        const auto nominal { emojiTypeface->getNominalGlyphForCodepoint (jam::toChar (codepoint)) };

        if (nominal.has_value())
        {
            result.glyphIndex = static_cast<uint16_t> (*nominal);
            result.typeface   = emojiTypeface;
            result.isEmoji    = true;
            result.found   = true;
        }
    }

    return result;
}

// ============================================================================
// GlyphArrangement::findFallbackGlyph  (resolution step 3: generic fallback)
// ============================================================================

GlyphArrangement::GlyphLookup GlyphArrangement::findFallbackGlyph (char32_t codepoint) const noexcept
{
    GlyphLookup result;
    result.codepoint = codepoint;

    const auto* fallbackTypeface { getOrLoadFallbackTypeface (codepoint) };

    if (fallbackTypeface != nullptr)
    {
        const auto nominal { fallbackTypeface->getNominalGlyphForCodepoint (jam::toChar (codepoint)) };

        if (nominal.has_value())
        {
            result.glyphIndex = static_cast<uint16_t> (*nominal);
            result.typeface   = fallbackTypeface;
            result.found   = true;
        }
    }

    return result;
}

// ============================================================================
// GlyphArrangement::getOrLoadFallbackTypeface
// ============================================================================

const juce::Typeface* GlyphArrangement::getOrLoadFallbackTypeface (char32_t codepoint) const noexcept
{
    const juce::Typeface* result { nullptr };

    if (fallbackTypefaceCache.contains (codepoint))
    {
        result = fallbackTypefaceCache.at (codepoint);
    }
    else
    {
        auto*     typefaceTable { jam::Typeface::getInstance() };
        const int regularIndex  { typefaceTable != nullptr ? typefaceTable->find (regularTypeface) : -1 };

        if (regularIndex >= 0)
        {
            // Candidate derived FROM the run's own regular typeface (its already-
            // interned Ptr, no new reference-counted handle needed) — mirrors the
            // god object's own `CTFontCreateForString (mainFont, ...)`: the
            // fallback candidate is found FROM the primary font, not from scratch.
            const auto&        regularEntry  { typefaceTable->get (regularIndex) };
            const juce::Font   baseFont      { juce::FontOptions { regularEntry.juceTypeface }.withHeight (fontSize) };
            const juce::String text          { juce::String::charToString (jam::toChar (codepoint)) };
            const juce::Font   candidateFont { baseFont.findSuitableFontForText (text) };
            auto               candidatePtr  { candidateFont.getTypefacePtr() };

            // "LastResort" rejection — see getOrLoadFallbackTypeface()'s doc comment.
            const bool isUsable { candidatePtr != nullptr and candidatePtr->getName().compare ("LastResort") != 0 };

            if (isUsable and candidatePtr->getNominalGlyphForCodepoint (jam::toChar (codepoint)).has_value())
            {
                typefaceTable->registerTypeface (candidatePtr);
                result = candidatePtr.get();
            }

            // Only cache once a real query ran — regularIndex < 0 means
            // "not yet registered", not "queried, no font covers this
            // codepoint"; caching that would poison the codepoint permanently
            // (see the god object's own fallbackFontCache semantics, which
            // never observed this transient state).
            fallbackTypefaceCache.emplace (codepoint, result);
        }
    }

    return result;
}

// ============================================================================
// GlyphArrangement::shapeCell
// ============================================================================

bool GlyphArrangement::shapeCell (jam::AttributedChar pen, const jam::Stamp::Entry& style, int typefaceIndex,
                                  int col, int row, Entry& out) const noexcept
{
    bool wrote { false };
    if (typefaceIndex >= 0)
    {
        auto* typefaceTable { jam::Typeface::getInstance() };

        if (typefaceTable != nullptr)
        {
            const auto&           typefaceEntry { typefaceTable->get (typefaceIndex) };
            const GlyphLookup resolution     { findGlyph (pen, typefaceEntry) };
            if (resolution.found)
            {
                out.glyphIndex = resolution.glyphIndex;
                out.span       = static_cast<uint8_t> (pen.widthClass() == jam::AttributedChar::wide ? 2 : 1);
                out.codepoint  = resolution.codepoint;
                out.col        = col;
                out.row        = row;
                // jam_Stamp.h's default-Entry() sentinel — an unstyled cell's
                // fg is transparent (alpha == 0); substitute the caller's
                // resolved default text colour instead of drawing invisibly.
                out.colour     = style.fg.getAlpha() == 0 ? defaultTextColour : style.fg;
                out.bg         = style.bg;
                out.typeface   = resolution.typeface;
                out.isEmoji    = resolution.isEmoji;
                out.style      = style.flags;

                const float monospaceAdvance { static_cast<float> (pen.widthClass() == jam::AttributedChar::wide ? 2 * cellWidth : cellWidth) };

                if (pen.widthClass() == jam::AttributedChar::proportional)
                {
                    const int foundTypefaceIndex { typefaceTable->find (resolution.typeface) };

                    out.advance = monospaceAdvance; // positive-path fallback default

                    if (foundTypefaceIndex >= 0)
                    {
                        const auto& foundEntry { typefaceTable->get (foundTypefaceIndex) };

                        if (foundEntry.hbFont != nullptr)
                        {
                            const hb_position_t rawAdvance { hb_font_get_glyph_h_advance (foundEntry.hbFont, resolution.glyphIndex) };
                            const unsigned int  upem       { hb_face_get_upem (hb_font_get_face (foundEntry.hbFont)) };

                            if (upem > 0)
                            {
                                const float pixelsPerEm { fontSize
                                                        * resolution.typeface->getMetrics (juce::TypefaceMetricsKind::portable).heightToPoints };

                                out.advance = (static_cast<float> (rawAdvance) / static_cast<float> (upem)) * pixelsPerEm;
                            }
                        }
                    }
                }
                else
                {
                    out.advance = monospaceAdvance;
                }

                wrote          = true;
            }
        }
    }
    return wrote;
}

// ============================================================================
// GlyphArrangement::tryLigature
// ============================================================================

int GlyphArrangement::tryLigature (const jam::AttributedChar* chars, int startIndex, int cellEnd, int typefaceIndex,
                                   int col, int row, const jam::Stamp::Entry& style, Entry& out) noexcept
{
    int result { 0 };

    if (typefaceIndex >= 0)
    {
        auto* typefaceTable { jam::Typeface::getInstance() };

        if (typefaceTable != nullptr)
        {
            const auto& typefaceEntry { typefaceTable->get (typefaceIndex) };

            if (typefaceEntry.hbFont != nullptr)
            {
                const uint16_t firstStyleId { chars[startIndex].styleId() };

                for (int tryLength { maxLigatureLength }; tryLength >= minLigatureLength and result == 0; --tryLength)
                {
                    if (startIndex + tryLength <= cellEnd)
                    {
                        std::array<uint32_t, static_cast<size_t> (maxLigatureLength)> codepoints {};
                        bool eligible { true };

                        for (int offset { 0 }; offset < tryLength and eligible; ++offset)
                        {
                            const jam::AttributedChar cell      { chars[startIndex + offset] };
                            const uint32_t  codepoint { cell.codepoint() };

                            eligible = codepoint > 0 and codepoint < static_cast<uint32_t> (asciiCeiling)
                                   and cell.contentTag() == jam::AttributedChar::contentCodepoint
                                   and cell.widthClass() == jam::AttributedChar::narrow
                                   and cell.styleId() == firstStyleId;

                            codepoints.at (static_cast<size_t> (offset)) = codepoint;
                        }

                        if (eligible)
                        {
                            hb_buffer_reset (ligatureBuffer);
                            hb_buffer_add_utf32 (ligatureBuffer, codepoints.data(), tryLength, 0, tryLength);
                            hb_buffer_guess_segment_properties (ligatureBuffer);
                            hb_shape (typefaceEntry.hbFont, ligatureBuffer, nullptr, 0);

                            unsigned int     glyphCount { 0 };
                            hb_glyph_info_t* glyphInfo  { hb_buffer_get_glyph_infos (ligatureBuffer, &glyphCount) };

                            if (glyphCount == 1)
                            {
                                out.glyphIndex = static_cast<uint16_t> (glyphInfo[0].codepoint);
                                out.span       = static_cast<uint8_t> (tryLength);
                                out.codepoint  = static_cast<char32_t> (codepoints.at (0));
                                out.col        = col;
                                out.row        = row;
                                // Same alpha == 0 sentinel substitution as
                                // shapeCell() — see its comment.
                                out.colour     = style.fg.getAlpha() == 0 ? defaultTextColour : style.fg;
                                out.bg         = style.bg;
                                out.typeface   = typefaceEntry.juceTypeface.get();
                                out.isEmoji    = false;
                                out.style      = style.flags;
                                out.advance    = static_cast<float> (tryLength * cellWidth);

                                result = tryLength;
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

// ============================================================================
// GlyphArrangement::arrange (second pass)
// ============================================================================

int GlyphArrangement::findRunKeyIndex (const RunKey* keys, int numKeys, const Entry& entry) noexcept
{
    int matchIndex { numKeys };

    for (int keyIndex { 0 }; keyIndex < numKeys and matchIndex == numKeys; ++keyIndex)
        if (keys[keyIndex].matches (entry))
            matchIndex = keyIndex;

    return matchIndex;
}

void GlyphArrangement::freeDrawRuns() noexcept
{
    for (int runIndex { 0 }; runIndex < numDrawRuns; ++runIndex)
    {
        drawRuns[runIndex].glyphCodes.free();
        drawRuns[runIndex].positions.free();
        drawRuns[runIndex].codepoints.free();
        drawRuns[runIndex].spans.free();
        drawRuns[runIndex].styles.free();
        drawRuns[runIndex].bgColours.free();
    }

    drawRuns.free();
    numDrawRuns = 0;
}

void GlyphArrangement::arrange() noexcept
{
    freeDrawRuns();

    if (numEntries > 0)
    {
        juce::HeapBlock<RunKey> keys;
        juce::HeapBlock<int>    runCounts;
        keys.malloc (numEntries);
        runCounts.calloc (numEntries);

        int numKeys { 0 };

        for (int entryIndex { 0 }; entryIndex < numEntries; ++entryIndex)
        {
            const Entry& entry      { entries[entryIndex] };
            const int    matchIndex { findRunKeyIndex (keys.getData(), numKeys, entry) };

            if (matchIndex < numKeys)
            {
                ++runCounts[matchIndex];
            }
            else
            {
                keys[numKeys]      = { entry.typeface, entry.colour, entry.isEmoji };
                runCounts[numKeys] = 1;
                ++numKeys;
            }
        }

        drawRuns.calloc (numKeys);
        numDrawRuns = numKeys;

        for (int keyIndex { 0 }; keyIndex < numKeys; ++keyIndex)
        {
            Run& run { drawRuns[keyIndex] };
            run.count    = runCounts[keyIndex];
            run.typeface = keys[keyIndex].typeface;
            run.colour   = keys[keyIndex].colour;
            run.isEmoji  = keys[keyIndex].isEmoji;
            run.glyphCodes.malloc (runCounts[keyIndex]);
            run.positions.calloc (runCounts[keyIndex]);
            run.codepoints.malloc (runCounts[keyIndex]);
            run.spans.calloc (runCounts[keyIndex]);
            run.styles.calloc (runCounts[keyIndex]);
            run.bgColours.calloc (runCounts[keyIndex]);
        }

        juce::HeapBlock<int> fillIndex;
        fillIndex.calloc (numKeys);

        for (int entryIndex { 0 }; entryIndex < numEntries; ++entryIndex)
        {
            const Entry& entry      { entries[entryIndex] };
            const int    matchIndex { findRunKeyIndex (keys.getData(), numKeys, entry) };
            const int    fill       { fillIndex[matchIndex] };
            Run&         run        { drawRuns[matchIndex] };

            run.glyphCodes[fill] = entry.glyphIndex;
            run.positions[fill]  = { entry.originX,
                                    static_cast<float> (entry.row * cellHeight + baseline) };
            run.codepoints[fill] = entry.codepoint;
            run.spans[fill]      = entry.span;
            run.styles[fill]     = entry.style;
            run.bgColours[fill]  = entry.bg;
            ++fillIndex[matchIndex];
        }
    }
}

// ============================================================================
// GlyphArrangement::draw
// ============================================================================

void GlyphArrangement::draw (juce::Graphics& g) const
{
    auto* typefaceTable { jam::Typeface::getInstance() };

    if (typefaceTable != nullptr)
    {
        for (int runIndex { 0 }; runIndex < numDrawRuns; ++runIndex)
        {
            const Run& run           { drawRuns[runIndex] };
            const int  typefaceIndex { typefaceTable->find (run.typeface) };

            if (typefaceIndex >= 0 and run.count > 0)
            {
                const auto&       typefaceEntry { typefaceTable->get (typefaceIndex) };
                // FontOptions' Typeface::Ptr constructor is the sanctioned typeface-first
                // path — withTypeface() on a default-constructed FontOptions asserts,
                // because the default options already carry a non-empty typeface NAME
                // (juce_FontOptions.h:125-126 requires name/style empty).
                const juce::Font  runFont { juce::FontOptions { typefaceEntry.juceTypeface }.withHeight (fontSize) };

                g.setColour (run.colour);

                auto& context { g.getInternalContext() };

                // Threads this run's per-glyph codepoint/span + this arrangement's
                // own cell-box metrics through to GlyphAtlas::Key
                // (GlyphConstraint's own documented "required extension":
                // emoji cell-fit + PUA/NF GlyphConstraint application, both keyed
                // off the codepoint + span-aware cell box GlyphAtlas::rasterize()
                // could not previously see) — mirrors this same call site's own
                // context.setFont() immediately below. g.getInternalContext()
                // returns the base juce::LowLevelGraphicsContext&; the jam
                // LLGCs share no common jam base (checked: jam::
                // VulkanLowLevelGraphicsContext derives juce::LowLevelGraphicsContext
                // directly, jam::LowLevelGraphicsGlyphRenderer derives
                // juce::CoreGraphicsContext on JUCE_MAC or juce::
                // LowLevelGraphicsSoftwareRenderer otherwise) — a dynamic_cast
                // to each concrete type is the cheapest correct dispatch
                // available; all types are already complete here (this TU
                // compiles after the full jam_vulkan.h aggregation,
                // jam_vulkan.mm/jam_vulkan.cpp).
                if (auto* vulkanContext { dynamic_cast<jam::VulkanLowLevelGraphicsContext*> (&context) })
                {
                    vulkanContext->setCellRun (run.codepoints.getData(), run.spans.getData(), run.count,
                                               cellWidth, cellHeight, baseline);
                }
                else if (auto* cpuContext { dynamic_cast<jam::LowLevelGraphicsGlyphRenderer*> (&context) })
                {
                    cpuContext->setCellRun (run.codepoints.getData(), run.spans.getData(), run.count,
                                            cellWidth, cellHeight, baseline);
                }

                context.setFont (runFont);
                context.drawGlyphs (juce::Span<const uint16_t> (run.glyphCodes.getData(), static_cast<size_t> (run.count)),
                                    juce::Span<const juce::Point<float>> (run.positions.getData(), static_cast<size_t> (run.count)),
                                    {});
            }
        }
    }
}

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
