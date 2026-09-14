//
// EdgeTable file: extractGlyphBounds(), renderMonoGlyphIntoAtlas(),
// renderEmojiGlyphIntoAtlas(), and rasterizeEdgeTable() — the tier-3
// juce::Typeface::getLayersForGlyph() coverage backend (class doc's three-tier
// model), also reached directly whenever FontRasterizerBackend::edgeTable is active, and as
// rasterizeFreeType()'s (jam_GlyphAtlasFreeType.cpp) internal no-FT_Face
// fallback. The only backend that ever produces Type::emoji glyphs.

namespace jam
{
/*____________________________________________________________________________*/

/** @brief Scans an ARGB bitmap's alpha channel for the tight ink rectangle, in
 *  image pixel coordinates. Color-emoji strike bitmaps (sbix, e.g. Apple Color
 *  Emoji's 160px cells) carry transparent padding around the glyph's actual
 *  ink; the pre-2e37f6d CoreText path measured ink bounds directly via
 *  CTFontGetBoundingRectsForGlyphs (endless GlyphAtlas.mm) instead of the raw
 *  strike rect. extractGlyphBounds()'s ImageLayer branch and
 *  renderEmojiGlyphIntoAtlas() both call this on the same juce::ImageLayer's
 *  image to restore that ink-bounds convention — each computes it
 *  independently (deterministic, same result for the same image) rather than
 *  threading it through as a parameter, since these two methods' signatures
 *  are declared in jam_GlyphAtlas.h, off-limits this sprint; consolidating
 *  into one call site is a follow-up once that header is free.
 *  @param image  Source ARGB bitmap (a juce::ImageLayer's strike image).
 *  @return       Tight ink rectangle in image pixel coordinates, or an empty
 *                rectangle if the image is fully transparent. */
static juce::Rectangle<int> computeInkBounds (const juce::Image& image) noexcept
{
    jassert (image.getFormat() == juce::Image::ARGB);

    juce::Image::BitmapData imageData (image, juce::Image::BitmapData::readOnly);

    int minRow { imageData.height };
    int maxRow { -1 };
    int minCol { imageData.width };
    int maxCol { -1 };

    for (int row { 0 }; row < imageData.height; ++row)
    {
        for (int col { 0 }; col < imageData.width; ++col)
        {
            if (imageData.getPixelColour (col, row).getAlpha() > 0)
            {
                minRow = std::min (minRow, row);
                maxRow = std::max (maxRow, row);
                minCol = std::min (minCol, col);
                maxCol = std::max (maxCol, col);
            }
        }
    }

    return maxRow >= 0
               ? juce::Rectangle<int> { minCol, minRow, maxCol - minCol + 1, maxRow - minRow + 1 }
               : juce::Rectangle<int> {};
}

// Fits natural (emoji ink already sized/positioned at font-size scale by
// extractGlyphBounds()'s ImageLayer branch) aspect-preserving into the
// span-aware cell box (key's cellWidth * span wide, cellHeight tall),
// centered on both axes — unconditional, no GlyphConstraint table
// consultation (that table has no entries for arbitrary emoji codepoints).
//
// Repositions bounds.bearing so the LLGC's existing bearing-based composite
// (devicePos + (bearing.x, -bearing.y), jam_VulkanLowLevelGraphicsContextGlyph.cpp
// / jam_LowLevelGraphicsGlyphRenderer.cpp) centers the FITTED (packed, not
// box-sized) ink rect within the box relative to the baseline-anchored draw
// position: box left = devicePos.x, box top = devicePos.y - key.baseline.
GlyphAtlas::GlyphBounds GlyphAtlas::fitEmojiToCellBox (const GlyphBounds& natural,
                                                       const Key& key) noexcept
{
    GlyphBounds fitted { natural };

    const auto [naturalWidth, naturalHeight] { natural.size };

    if (naturalWidth > 0 and naturalHeight > 0)
    {
        const int   spanCells  { juce::jmax (1, static_cast<int> (key.span)) };
        const float boxWidth   { static_cast<float> (key.cellWidth * spanCells) };
        const float boxHeight  { static_cast<float> (key.cellHeight) };
        const float fitScale   { std::min (boxWidth / static_cast<float> (naturalWidth),
                                           boxHeight / static_cast<float> (naturalHeight)) };

        const int fittedWidth  { juce::jmax (1, juce::roundToInt (static_cast<float> (naturalWidth)  * fitScale)) };
        const int fittedHeight { juce::jmax (1, juce::roundToInt (static_cast<float> (naturalHeight) * fitScale)) };

        const float horizontalInset { (boxWidth - static_cast<float> (fittedWidth)) * 0.5f };
        const float verticalInset   { (boxHeight - static_cast<float> (fittedHeight)) * 0.5f };

        fitted.size = jam::Size<int> (fittedWidth, fittedHeight);
        fitted.bearing = juce::Point<int> (juce::roundToInt (horizontalInset),
                                           key.baseline - juce::roundToInt (verticalInset));
    }

    return fitted;
}

GlyphAtlas::GlyphBounds GlyphAtlas::extractGlyphBounds (const juce::GlyphLayer& firstLayer) const noexcept
{
    GlyphBounds bounds {};

    if (auto* colourLayer = std::get_if<juce::ColourLayer> (&firstLayer.layer))
    {
        // Monochrome outline — EdgeTable contains coverage data
        auto clipBounds { colourLayer->clip.getMaximumBounds() };
        bounds.size = jam::Size<int> (clipBounds.getWidth(), clipBounds.getHeight());
        bounds.bearing = juce::Point<int> (clipBounds.getX(), -clipBounds.getY());
    }
    else if (auto* imageLayer = std::get_if<juce::ImageLayer> (&firstLayer.layer))
    {
        // Bitmap emoji — imageLayer->transform composes the native-strike-to-
        // extents scale with the glyph's bearing translation, then the caller's
        // key.fontSize scale (juce_Typeface.cpp:452-460:
        // AffineTransform::scale (extents->width / imageW, extents->height / imageH)
        //     .translated (x_bearing, y_bearing).followedBy (t)).
        // Transforming the source image's own rect by that composed transform
        // yields the glyph's actual on-atlas footprint (e.g. Apple Color Emoji's
        // 160px strike scaled down to the requested key.fontSize extents) —
        // PROVIDED that rect is the strike's ink, not its full padded bitmap.
        //
        // Color-emoji strike bitmaps (sbix, e.g. Apple Color Emoji's 160px
        // cells) carry transparent padding around the glyph's actual ink —
        // transforming the FULL image rect (as opposed to the pre-2e37f6d
        // CoreText path, which measured ink bounds directly via
        // CTFontGetBoundingRectsForGlyphs, endless GlyphAtlas.mm)
        // over-reports bounds.bearing.y (LLGC draws the quad at
        // `baselineY - bearing.y`, too high) and bounds.size (the visible ink
        // ends up smaller than its packed box). computeInkBounds() trims the
        // source image to its ink rect first, restoring that convention.
        bounds.isEmoji = true;

        const auto inkRect { computeInkBounds (imageLayer->image) };

        if (not inkRect.isEmpty())
        {
            const juce::Rectangle<float> inkRectFloat { (float) inkRect.getX(), (float) inkRect.getY(),
                                                         (float) inkRect.getWidth(), (float) inkRect.getHeight() };
            const auto transformedRect { inkRectFloat.transformedBy (imageLayer->transform) };

            bounds.size = jam::Size<int> (static_cast<int> (std::ceil (transformedRect.getWidth())),
                                          static_cast<int> (std::ceil (transformedRect.getHeight())));
            // bearing from the transformed rect's origin — same convention as the mono
            // branch above (bearing.y negated from device-space Y).
            bounds.bearing = juce::Point<int> (
                juce::roundToInt (transformedRect.getX()),
                -juce::roundToInt (transformedRect.getY()));
        }
    }

    return bounds;
}

void GlyphAtlas::renderMonoGlyphIntoAtlas (const juce::GlyphLayer& firstLayer, juce::Point<int> packPos,
                                           int glyphWidth, int glyphHeight) noexcept
{
    if (auto* colourLayer = std::get_if<juce::ColourLayer> (&firstLayer.layer))
    {
        auto edgeBounds { colourLayer->clip.getMaximumBounds() };
        const juce::Point<int> offset { edgeBounds.getX(), edgeBounds.getY() };

        juce::Image::BitmapData atlasData (images.at (Type::mono).image, packPos.x, packPos.y,
                                           glyphWidth, glyphHeight,
                                           juce::Image::BitmapData::writeOnly);

        // Iterate the EdgeTable directly into the atlas — no scratch image needed.
        // getLinePointer(row) is relative to the BitmapData subregion (packPos).
        // coverageLut applies gamma/contrast per byte at this same write site.
        MonoCoverageWriter writer { atlasData, offset, coverageLut.data() };
        colourLayer->clip.iterate (writer);
    }

    images.at (Type::mono).dirty = true;
}

void GlyphAtlas::renderEmojiGlyphIntoAtlas (const juce::GlyphLayer& firstLayer, juce::Point<int> packPos,
                                            int glyphWidth, int glyphHeight) noexcept
{
    if (auto* imageLayer = std::get_if<juce::ImageLayer> (&firstLayer.layer))
    {
        // extractGlyphBounds() sized glyphWidth/glyphHeight from imageLayer->transform
        // applied to the INK-TRIMMED strike bitmap (computeInkBounds() — see its
        // doc comment on sbix strike padding, endless GlyphAtlas.mm's
        // CTFontGetBoundingRectsForGlyphs precedent) — rescale that same ink
        // subsection to that footprint before copying, rather than blitting the
        // raw native strike (padding included) into the atlas cell it was packed
        // into. getClippedImage() shares the source image's pixel data rather
        // than copying it (juce_Image.h); Image::rescaled() reads through that
        // view via its own drawImageTransformed() call sized from the clipped
        // image's own (ink-sized) width/height (juce_Image.cpp:746-759), so no
        // separate copy of the ink subsection is needed before rescaling.
        const auto inkRect { computeInkBounds (imageLayer->image) };
        // rasterizeEdgeTable() only reaches this render call when
        // extractGlyphBounds()'s ink-derived bounds.size was already > 0 for this
        // same layer's image — computeInkBounds() is deterministic, so it cannot
        // report empty here.
        jassert (not inkRect.isEmpty());

        // Image::rescaled() preserves the source pixelFormat (juce_Image.cpp:746-759),
        // so an ARGB strike (hb_ot_color_glyph_reference_png via PNGImageFormat)
        // stays ARGB — the per-row * 4 stride below still applies.
        const auto rescaledImage { imageLayer->image.getClippedImage (inkRect)
                                       .rescaled (glyphWidth, glyphHeight,
                                                  juce::Graphics::highResamplingQuality) };
        jassert (rescaledImage.getFormat() == juce::Image::ARGB);

        juce::Image::BitmapData srcData (rescaledImage, juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData atlasData (images.at (Type::emoji).image, packPos.x, packPos.y,
                                           glyphWidth, glyphHeight,
                                           juce::Image::BitmapData::writeOnly);

        for (int row { 0 }; row < glyphHeight; ++row)
        {
            std::memcpy (atlasData.getLinePointer (row),
                         srcData.getLinePointer (row),
                         static_cast<size_t> (glyphWidth * 4));
        }
    }

    images.at (Type::emoji).dirty = true;
}

GlyphAtlas::GlyphBounds GlyphAtlas::rasterizeEdgeTable (const Key& key, juce::Point<int>& packPos) noexcept
{
    // JUCE's own rasterization handles every font format (outline, bitmap,
    // COLRv0, COLRv1 fallback) cross-platform — the only backend that ever
    // produces Type::emoji glyphs.
    //
    // key.rotation (radians, 0.0f for untransformed text) is composed AFTER the
    // pixel-size scale, about the pen origin (0, 0) — AffineTransform::rotated()
    // is scale.followedBy (AffineTransform::rotation (key.rotation)), so this is
    // exactly "apply rotation to the glyph path, composed with the existing
    // scale transform". getLayersForGlyph() bakes this transform into whatever
    // it returns for BOTH layer kinds: a ColourLayer's EdgeTable coverage is
    // shaped by it directly, and an ImageLayer's own .transform (extractGlyphBounds()/
    // renderEmojiGlyphIntoAtlas() below) composes it via `.followedBy (t)`
    // (juce_Typeface.cpp:452-460) — so the emoji/colour-layer path needs no
    // separate rotation application; it already inherits it from this one site.
    const auto scale { juce::AffineTransform::scale (key.fontSize).rotated (key.rotation) };
    auto edgeTableLayers { key.typeface->getLayersForGlyph (static_cast<int> (key.glyphIndex), scale) };

    GlyphBounds bounds {};

    if (not edgeTableLayers.empty())
    {
        bounds = extractGlyphBounds (edgeTableLayers.front());

        // Task A (EMOJI CELL-FIT) — a real cell-run caller (key.cellWidth > 0,
        // see Key's own doc comment) aspect-fits color-emoji ink into the
        // span-aware cell box, centered. Packed size becomes the FITTED size
        // (below), not the natural size — see fitEmojiToCellBox()'s doc comment.
        if (bounds.isEmoji and key.cellWidth > 0)
            bounds = fitEmojiToCellBox (bounds, key);

        const auto [boundsWidth, boundsHeight] { bounds.size };

        if (boundsWidth > 0 and boundsHeight > 0)
        {
            packPos = packGlyph (boundsWidth, boundsHeight, bounds.isEmoji ? Type::emoji : Type::mono);

            if (packPos.x >= 0)
            {
                if (bounds.isEmoji)
                    renderEmojiGlyphIntoAtlas (edgeTableLayers.front(), packPos, boundsWidth, boundsHeight);
                else
                    renderMonoGlyphIntoAtlas (edgeTableLayers.front(), packPos, boundsWidth, boundsHeight);
            }
        }
    }

    return bounds;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
