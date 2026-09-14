//
// FreeType file: registerTypeface() (tier 1, embedded defaults),
// getOrLoadSystemTypeface() (tier 2, lazily-resolved system fonts), and the
// FreeType rasterization backend itself (loadFreeTypeGlyphBounds(),
// renderFreeTypeGlyphIntoAtlas(), writeMonoCoverageRow(), rasterizeFreeType()).
// rasterizeFreeType()'s internal EdgeTable fallback (tier 3) calls
// rasterizeEdgeTable(), defined in jam_GlyphAtlasEdgeTable.cpp — the class body
// (jam_GlyphAtlas.h) makes every private member visible across sibling TUs.

namespace jam
{
/*____________________________________________________________________________*/

/** @brief FreeType 26.6 fixed-point scale factor — the lower 6 bits of every
 *  FT_Pos metric are the fractional pixel part. Named per endless's own
 *  `ftFixedScale` (deleted commit 2e37f6d, Fonts.h), restored here for
 *  calcMetrics()'s ceiling conversion. */
static constexpr FT_Pos ftFixedScale { 64 };

/** @brief Ceiling-converts a 26.6 fixed-point FreeType metric to whole
 *  pixels — verbatim endless `ceil26_6ToPx()` (deleted commit 2e37f6d,
 *  Fonts.h), restored here for calcMetrics(). */
static int ceil26_6ToPx (FT_Pos value26_6) noexcept
{
    return static_cast<int> (std::ceil (static_cast<float> (value26_6) / static_cast<float> (ftFixedScale)));
}

/** @brief FreeType 16.16 fixed-point scale factor -- FT_Matrix's four entries
 *  (xx/xy/yx/yy) are FT_Fixed values, a different fixed-point format from
 *  FT_Pos's 26.6 (ftFixedScale above). Used by loadFreeTypeGlyphBounds() to
 *  build the rotation matrix FT_Set_Transform() bakes into the rasterized
 *  bitmap for key.rotation. */
static constexpr double ftFixed16Dot16Scale { 65536.0 };

/** @brief Typographic points per inch. Passed as FT_Set_Char_Size()'s device
 *  dpi so 1 point equals exactly 1 pixel — char size in 26.6 points then IS
 *  fractional pixels-per-em (calcMetrics(), loadFreeTypeGlyphBounds()). */
static constexpr FT_UInt ftPointsPerInch { 72 };

/** @brief Scans printable ASCII glyphs (U+0020-U+007F inclusive) on @p face
 *  for the maximum horizontal advance — verbatim endless
 *  `measureMaxCellWidth()` semantics (deleted commit 2e37f6d,
 *  FontsMetrics.cpp), restored here for calcMetrics()'s cell-width
 *  calculation. @p face must already be sized (FT_Set_Char_Size).
 *  @return Maximum horiAdvance in 26.6 fixed-point, or 0 if no ASCII glyph
 *          was found — calcMetrics() falls back to
 *          face->size->metrics.max_advance in that case. */
static FT_Pos measureMaxCellWidth26_6 (FT_Face face) noexcept
{
    FT_Pos maxAdvance26_6 { 0 };

    for (int code { 32 }; code <= 127; ++code)
    {
        const FT_UInt glyphIndex { FT_Get_Char_Index (face, static_cast<FT_UInt> (code)) };

        if (glyphIndex != 0 and FT_Load_Glyph (face, glyphIndex, FT_LOAD_DEFAULT) == 0)
        {
            const FT_Pos horiAdvance26_6 { face->glyph->metrics.horiAdvance };

            if (horiAdvance26_6 > maxAdvance26_6)
                maxAdvance26_6 = horiAdvance26_6;
        }
    }

    return maxAdvance26_6;
}

GlyphAtlas::Metrics GlyphAtlas::calcMetrics (const juce::Typeface::Ptr& typeface, float fontHeightJuce) const noexcept
{
    Metrics metrics;

    if (freetypeFaces.contains (typeface.get()))
    {
        auto* face { freetypeFaces.at (typeface.get()).face };

        if (face != nullptr)
        {
            // SSOT conversion — identical to loadFreeTypeGlyphBounds()'s
            // pixelsPerEm formula, so cell metrics are measured at the exact
            // FT size glyphs rasterize at (the whole conformance point).
            const auto pixelsPerEm {
                fontHeightJuce * typeface->getMetrics (juce::TypefaceMetricsKind::portable).heightToPoints
            };

            // 26.6 fixed-point at ftPointsPerInch dpi (1 pt == 1 px) — preserves
            // the fractional em size FT_Set_Pixel_Sizes' integer parameter would
            // truncate, matching the DirectWrite backend's float fontEmSize.
            FT_Set_Char_Size (face, 0,
                              static_cast<FT_F26Dot6> (juce::roundToInt (pixelsPerEm * static_cast<float> (ftFixedScale))),
                              ftPointsPerInch, ftPointsPerInch);

            const FT_Pos maxAdvance26_6 { measureMaxCellWidth26_6 (face) };
            const FT_Pos cellWidth26_6 { maxAdvance26_6 > 0 ? maxAdvance26_6 : face->size->metrics.max_advance };

            metrics.cellWidth  = ceil26_6ToPx (cellWidth26_6);
            metrics.cellHeight = ceil26_6ToPx (face->size->metrics.height);
            metrics.baseline   = ceil26_6ToPx (face->size->metrics.ascender);
        }
    }

    return metrics;
}

void GlyphAtlas::registerTypeface (const juce::Typeface::Ptr& typeface, const void* fontData, size_t sizeBytes)
{
    jassert (typeface != nullptr);
    jassert (fontData != nullptr and sizeBytes > 0);

    // A real (non-null) FaceEntry already registered for this typeface identity
    // is idempotent — no new FT_Face is created at all. A missing entry or a
    // null-face entry (getOrLoadSystemTypeface()'s negative-resolution cache)
    // both need the real face built below; replacing a null-face entry frees
    // nothing, since it never held an FT resource.
    const bool needsRegistration { not freetypeFaces.contains (typeface.get())
        or freetypeFaces.at (typeface.get()).face == nullptr };

    if (needsRegistration)
    {
        // GlyphAtlas owns a copy of the font bytes (fontBytes) rather than reading
        // the caller's buffer directly — FT_New_Memory_Face does not copy its input,
        // so without this copy the caller's buffer would need static storage
        // duration; embedded fonts are a few hundred KB each, so copying is cheap
        // and removes that lifetime constraint entirely (see FaceEntry doc).
        juce::MemoryBlock fontBytes (fontData, sizeBytes);
        FT_Face face { nullptr };

        // faceIndex 0 — every registered typeface here is a single-face TTF/OTF, never
        // a font collection.
        const FT_Error ftResult { FT_New_Memory_Face (freetypeLibrary, static_cast<const FT_Byte*> (fontBytes.getData()),
                                static_cast<FT_Long> (fontBytes.getSize()), 0, &face) };

        if (ftResult == 0)
            freetypeFaces.addOrReplace (typeface.get(), FaceEntry { face, std::move (fontBytes) });
    }
}

GlyphAtlas::GlyphBounds GlyphAtlas::loadFreeTypeGlyphBounds (FT_Face face, const Key& key) const noexcept
{
    GlyphBounds bounds {};

    // SSOT conversion — key.fontSize holds JUCE's own "height" units (physical
    // pixels, already DPI-scaled by the caller); heightToPoints converts that to
    // points-per-em, which at 1 physical pixel per point is exactly pixels-per-em.
    // TypefaceMetricsKind::portable matches this codebase's
    // Fonts — every Font here is constructed via FontOptions, whose metricsKind
    // defaults to portable (juce_FontOptions.h).
    const auto pixelsPerEm {
        key.fontSize * key.typeface->getMetrics (juce::TypefaceMetricsKind::portable).heightToPoints
    };

    // 26.6 fixed-point at ftPointsPerInch dpi (1 pt == 1 px) — preserves the
    // fractional em size FT_Set_Pixel_Sizes' integer parameter would truncate,
    // matching the DirectWrite backend's float fontEmSize (calcMetrics() uses
    // the identical call, keeping measured cell metrics on the exact rasterized size).
    FT_Set_Char_Size (face, 0,
                      static_cast<FT_F26Dot6> (juce::roundToInt (pixelsPerEm * static_cast<float> (ftFixedScale))),
                      ftPointsPerInch, ftPointsPerInch);

    // key.rotation (radians, 0.0f for untransformed text) participates in glyph
    // identity (jam_GlyphAtlas.h's Key doc comment) -- baked directly into the
    // rasterized bitmap via FT_Set_Transform's 16.16 fixed-point rotation
    // matrix, applied before glyph load so FT_Load_Glyph/FT_Render_Glyph below
    // produce the ALREADY-ROTATED bitmap, with bitmap_left/bitmap_top
    // (bounds.bearing) reported relative to the rotated shape --
    // Region::bearing semantics stay unchanged.
    //
    // Sign: key.rotation is the JUCE screen-space (y-DOWN) angle
    // (atan2 (mat10, mat00) at the drawGlyphs call sites); FT outline space is
    // y-UP, so the screen rotation conjugates through the y-flip to R(-theta):
    // {cos, sin; -sin, cos} -- the same negation rasterizeNative()'s CoreText
    // (y-up) half applies, while DirectWrite (y-down) stays unnegated.
    //
    // No special-casing for rotation == 0.0f: the matrix is the identity
    // there. FT_Face is shared state (freetypeFaces) -- reset to identity
    // below before returning so a following unrelated glyph load never
    // inherits this rotation.
    const auto cosRotation { std::cos (key.rotation) };
    const auto sinRotation { std::sin (key.rotation) };
    FT_Matrix rotationMatrix {
        static_cast<FT_Fixed> (juce::roundToInt (cosRotation * ftFixed16Dot16Scale)),
        static_cast<FT_Fixed> (juce::roundToInt (sinRotation * ftFixed16Dot16Scale)),
        static_cast<FT_Fixed> (juce::roundToInt (-sinRotation * ftFixed16Dot16Scale)),
        static_cast<FT_Fixed> (juce::roundToInt (cosRotation * ftFixed16Dot16Scale))
    };
    FT_Set_Transform (face, &rotationMatrix, nullptr);

    if (FT_Load_Glyph (face, key.glyphIndex, FT_LOAD_NO_HINTING) == 0)
    {
        // Synthetic embolden — verbatim old chain (deleted jam::glyph::Atlas's
        // FreeType path), gated by setEmbolden(). Applied to the just-loaded
        // outline before rendering, so the heavier stems rasterize directly
        // rather than requiring a post-render bitmap operation.
        if (embolden)
            FT_Outline_Embolden (&face->glyph->outline, 1 << 6);

        if (FT_Render_Glyph (face->glyph, FT_RENDER_MODE_NORMAL) == 0)
        {
            const auto& bitmap { face->glyph->bitmap };
            bounds.size = jam::Size<int> (static_cast<int> (bitmap.width), static_cast<int> (bitmap.rows));
            bounds.bearing = juce::Point<int> (face->glyph->bitmap_left, face->glyph->bitmap_top);
        }
    }

    FT_Set_Transform (face, nullptr, nullptr);

    return bounds;
}

void GlyphAtlas::renderFreeTypeGlyphIntoAtlas (FT_Face face, juce::Point<int> packPos,
                                               int glyphWidth, int glyphHeight) noexcept
{
    const auto& bitmap { face->glyph->bitmap };
    const size_t sourceStride { static_cast<size_t> (std::abs (bitmap.pitch)) };

    // Positive pitch (top-down) is FreeType's normal convention for FT_RENDER_MODE_NORMAL;
    // negative pitch (bottom-up) is handled by starting from the last physical row —
    // mirrors the deleted jam::glyph::Atlas::Packer's copyBitmapRows() row-copy convention.
    const unsigned char* sourceTop { bitmap.pitch < 0
        ? bitmap.buffer + static_cast<ptrdiff_t> ((1 - static_cast<ptrdiff_t> (bitmap.rows)) * bitmap.pitch)
        : bitmap.buffer };

    juce::Image::BitmapData atlasData (images.at (Type::mono).image, packPos.x, packPos.y,
                                       glyphWidth, glyphHeight,
                                       juce::Image::BitmapData::writeOnly);

    for (int row { 0 }; row < glyphHeight; ++row)
    {
        writeMonoCoverageRow (atlasData.getLinePointer (row),
                             sourceTop + static_cast<ptrdiff_t> (row) * static_cast<ptrdiff_t> (sourceStride),
                             glyphWidth);
    }

    images.at (Type::mono).dirty = true;
}

void GlyphAtlas::writeMonoCoverageRow (uint8_t* dest, const uint8_t* source, int count) const noexcept
{
    for (int i { 0 }; i < count; ++i)
        dest[i] = coverageLut.at (source[i]);
}

GlyphAtlas::GlyphBounds GlyphAtlas::rasterizeFreeType (const Key& key, juce::Point<int>& packPos) noexcept
{
    // First miss for this typeface identity — attempt lazy system-font
    // resolution exactly once before falling back to EdgeTable (tier 2 of the
    // class doc's three-tier model). Inserts a FaceEntry either way (real face
    // on success, null face on failure), so this branch is never re-entered
    // for the same typeface again.
    if (not freetypeFaces.contains (key.typeface))
        getOrLoadSystemTypeface (key.typeface);

    GlyphBounds bounds {};
    auto* face { freetypeFaces.at (key.typeface).face };

    if (face != nullptr)
    {
        // Registered or lazily-resolved typeface — unhinted FreeType rasterization.
        // Always mono: only text typefaces ever reach a real FT_Face here
        // (registerTypeface(), getOrLoadSystemTypeface()).
        bounds = loadFreeTypeGlyphBounds (face, key);
        const auto [naturalWidth, naturalHeight] { bounds.size };

        if (naturalWidth > 0 and naturalHeight > 0)
        {
            // A real cell-run caller (key.cellWidth > 0, see Key's own
            // doc comment) whose codepoint has an active GlyphConstraint
            // renders scaled/aligned/padded into a box-sized bitmap,
            // bearingX = 0 / bearingY = baseline — the same
            // constrained-glyph semantics shared with
            // jam_GlyphAtlas_mac.mm and jam_GlyphAtlasNative.cpp's
            // identical wiring, applied here to FreeType's
            // unhinted mono glyph bitmap. Unconstrained mono text
            // (the `else` branch) is bit-for-bit unchanged from before
            // this wiring.
            const GlyphConstraint constraint { GlyphConstraint::getConstraint (key.codepoint) };

            if (constraint.isActive() and key.cellWidth > 0)
            {
                const int spanCells        { juce::jmax (1, static_cast<int> (key.span)) };
                const int cellBitmapWidth  { key.cellWidth * spanCells };
                const int cellBitmapHeight { key.cellHeight };

                packPos = packGlyph (cellBitmapWidth, cellBitmapHeight, Type::mono);

                if (packPos.x >= 0)
                {
                    const auto& bitmap { face->glyph->bitmap };
                    const size_t sourceStride { static_cast<size_t> (std::abs (bitmap.pitch)) };

                    // Positive pitch (top-down) is FreeType's normal convention for
                    // FT_RENDER_MODE_NORMAL; negative pitch (bottom-up) starts from the
                    // last physical row — same convention renderFreeTypeGlyphIntoAtlas()
                    // uses for the unconstrained path below.
                    const unsigned char* sourceTop { bitmap.pitch < 0
                        ? bitmap.buffer + static_cast<ptrdiff_t> ((1 - static_cast<ptrdiff_t> (bitmap.rows)) * bitmap.pitch)
                        : bitmap.buffer };

                    const auto placement { computeConstraintPlacement (constraint, naturalWidth, naturalHeight,
                                                                       key.cellWidth, key.cellHeight, key.span) };

                    renderConstrainedMonoGlyphIntoAtlas (sourceTop, static_cast<int> (sourceStride),
                                                         naturalWidth, naturalHeight, placement,
                                                         packPos, cellBitmapWidth, cellBitmapHeight);

                    bounds.size = jam::Size<int> (cellBitmapWidth, cellBitmapHeight);
                    bounds.bearing = juce::Point<int> (0, key.baseline);
                }
            }
            else
            {
                packPos = packGlyph (naturalWidth, naturalHeight, Type::mono);

                if (packPos.x >= 0)
                    renderFreeTypeGlyphIntoAtlas (face, packPos, naturalWidth, naturalHeight);
            }
        }
    }
    else
    {
        // No usable FT_Face for this typeface — either resolution failed (a
        // null-face FaceEntry, tier 2 miss) or it is a JUCE-internal fallback
        // typeface this atlas never owns bytes for (see registerTypeface() doc
        // comment). FreeType backend's own defined fallback — an internal call,
        // never through rasterizeByBackend (see this method's header doc).
        bounds = rasterizeEdgeTable (key, packPos);
    }

    return bounds;
}

void GlyphAtlas::getOrLoadSystemTypeface (const juce::Typeface* typeface) noexcept
{
    FT_Face face { nullptr };
    juce::MemoryBlock fontBytes;

    const auto fontFile { findSystemFontFile (typeface->getName(), typeface->getStyle()) };

    if (fontFile.existsAsFile() and fontFile.loadFileAsData (fontBytes))
    {
        FT_Face loadedFace { nullptr };

        // faceIndex 0 — same single-face assumption as registerTypeface(); own
        // bytes copy for the same reason (FT_New_Memory_Face does not copy).
        const FT_Error ftResult { FT_New_Memory_Face (freetypeLibrary, static_cast<const FT_Byte*> (fontBytes.getData()),
                                static_cast<FT_Long> (fontBytes.getSize()), 0, &loadedFace) };

        if (ftResult == 0)
        {
            // Color fonts (Apple Color Emoji, Segoe UI Emoji, Noto Color Emoji)
            // are deliberately excluded from this FreeType path — it rasterizes
            // grayscale coverage only. Reject the face immediately so rasterize()
            // falls through to JUCE's getLayersForGlyph() ImageLayer emoji path,
            // which already renders these glyphs in colour.
            if (FT_HAS_COLOR (loadedFace))
                FT_Done_Face (loadedFace);
            else
                face = loadedFace;
        }

        if (face == nullptr)
            fontBytes.reset();
    }

    // Inserted unconditionally — a null face doubles as the negative-resolution
    // cache (see FaceEntry doc), so this attempt is never repeated per glyph.
    freetypeFaces.emplace (typeface, FaceEntry { face, std::move (fontBytes) });
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
