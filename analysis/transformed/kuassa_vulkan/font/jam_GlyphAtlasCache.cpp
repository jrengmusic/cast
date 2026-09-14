//
// Cache file: insertRasterizedGlyph(), rebuildCoverageLut(), flushCache(),
// buildCacheAgeList(), evictLeastRecentlyUsed(), packGlyph(), and writePixels() —
// the LRU cache/eviction bookkeeping and the shelf/strip bin-packer shared by
// every rasterization backend.

namespace jam
{
/*____________________________________________________________________________*/
GlyphAtlas::Region* GlyphAtlas::insertRasterizedGlyph (const Key& key, const GlyphBounds& bounds,
                                                        juce::Point<int> packPos, Type type) noexcept
{
    const float invDim { 1.0f / static_cast<float> (dimension) };
    const auto [boundsWidth, boundsHeight] { bounds.size };
    Region region
    {
        juce::Rectangle<float> (
            static_cast<float> (packPos.x) * invDim,
            static_cast<float> (packPos.y) * invDim,
            static_cast<float> (boundsWidth) * invDim,
            static_cast<float> (boundsHeight) * invDim),
        bounds.size,
        bounds.bearing,
        type
    };

    if (cache.size() >= maxCachedGlyphs)
        evictLeastRecentlyUsed();

    cache.emplace (key, CacheEntry { region, frameCounter });
    return &cache.at (key).region;
}

void GlyphAtlas::rebuildCoverageLut (float newGamma, float newContrast) noexcept
{
    constexpr float maxCoverage { 255.0f };
    constexpr int coverageLevels { 256 };
    constexpr int midCoverage { 128 };

    for (int coverage { 0 }; coverage < coverageLevels; ++coverage)
    {
        const double normalized { static_cast<double> (coverage) / static_cast<double> (maxCoverage) };
        const double gammaCorrected { std::pow (normalized, 1.0 / static_cast<double> (newGamma)) };
        const double contrasted { gammaCorrected
                                  + static_cast<double> (newContrast) * gammaCorrected * (1.0 - gammaCorrected) };

        coverageLut.at (coverage) = static_cast<uint8_t> (juce::roundToInt (static_cast<double> (maxCoverage) * contrasted));
    }

    if (newGamma == identityGamma and newContrast == identityContrast)
        jassert (coverageLut.at (midCoverage) == midCoverage);
}

void GlyphAtlas::flushCache() noexcept
{
    cache.clear();

    images.at (Type::mono).shelves.clear();
    images.at (Type::emoji).shelves.clear();

    // Cleared CPU packing state no longer matches the GPU mirror's stale
    // pixels — mark both slots dirty so uploadDirtyAtlasSlots() re-uploads
    // (precedent: destroying the GPU-resident mirror images forces the same
    // dirty-both-slots transition, jam_GlyphAtlas.h's shutdown doc comment).
    images.at (Type::mono).dirty = true;
    images.at (Type::emoji).dirty = true;
}

int GlyphAtlas::buildCacheAgeList (juce::HeapBlock<AgeEntry>& ageList) const noexcept
{
    const int count { static_cast<int> (cache.size()) };
    ageList.allocate (static_cast<size_t> (count), false);

    int idx { 0 };
    for (const auto& [key, entry] : cache)
    {
        jassert (idx >= 0 and idx < count);
        ageList[idx].key = key;
        ageList[idx].age = frameCounter - entry.lastUsedFrame;
        ++idx;
    }

    return count;
}

void GlyphAtlas::evictLeastRecentlyUsed() noexcept
{
    if (not cache.empty())
    {
        constexpr size_t evictionFraction { 10 };
        const size_t targetRemove { std::max (static_cast<size_t> (1), cache.size() / evictionFraction) };
        juce::HeapBlock<AgeEntry> ageList;
        const int ageCount { buildCacheAgeList (ageList) };
        const size_t sortedCount { std::min (static_cast<size_t> (ageCount), targetRemove) };

        std::partial_sort (
            ageList.get(),
            ageList.get() + sortedCount,
            ageList.get() + ageCount,
            [] (const AgeEntry& a, const AgeEntry& b) { return a.age > b.age; });

        const int removeCount { static_cast<int> (sortedCount) };

        for (int i { 0 }; i < removeCount; ++i)
        {
            jassert (i >= 0 and i < removeCount);
            cache.erase (ageList[i].key);
        }
    }
}

juce::Point<int> GlyphAtlas::packGlyph (int width, int height, Type type) noexcept
{
    auto& slot { images.at (type) };

    // Shelf-packing: reuse the first existing shelf tall enough and with enough
    // horizontal room, before opening a new shelf below the last one. Ported
    // from the deleted jam::glyph::AtlasPacker::allocate().
    int fittingShelf { -1 };

    for (int i { 0 }; i < slot.shelves.size(); ++i)
    {
        const auto& shelf { slot.shelves[i] };

        if (fittingShelf == -1 and height <= shelf.height and shelf.currentX + width <= dimension)
            fittingShelf = i;
    }

    juce::Point<int> pos { -1, -1 };

    if (fittingShelf != -1)
    {
        auto& shelf { slot.shelves[fittingShelf] };
        pos = { shelf.currentX, shelf.y };
        shelf.currentX += width + slotGutter;
    }
    else
    {
        int shelfY { 0 };

        if (not slot.shelves.isEmpty())
        {
            const auto& lastShelf { slot.shelves.last() };
            shelfY = lastShelf.y + lastShelf.height + slotGutter;
        }

        if (shelfY + height <= dimension)
        {
            slot.shelves.add (Shelf { shelfY, height, width + slotGutter });
            pos = { 0, shelfY };
        }
    }

    return pos;
}

void GlyphAtlas::writePixels (const uint8_t* pixelData, int width, int height,
                               juce::Rectangle<int> destRect, Type type) noexcept
{
    auto& slot { images.at (type) };
    const int pixelStride { type == Type::emoji ? 4 : 1 };

    juce::Image::BitmapData destData (slot.image,
                                      destRect.getX(), destRect.getY(),
                                      destRect.getWidth(), destRect.getHeight(),
                                      juce::Image::BitmapData::writeOnly);

    const int srcStride { width * pixelStride };

    for (int row { 0 }; row < height; ++row)
    {
        std::memcpy (destData.getLinePointer (row),
                     pixelData + row * srcStride,
                     static_cast<size_t> (srcStride));
    }

    slot.dirty = true;
}

// ============================================================================
// Constraint application (task B) — shared by rasterizeFreeType() and
// rasterizeNative() (mac + Windows halves). See jam_GlyphAtlas.h's
// ConstraintPlacement/computeConstraintPlacement/renderConstrainedMonoGlyphIntoAtlas
// declarations for the full doc comment.
// ============================================================================

GlyphAtlas::ConstraintPlacement GlyphAtlas::computeConstraintPlacement (const GlyphConstraint& constraint,
                                                                        int naturalWidth, int naturalHeight,
                                                                        int cellWidth, int cellHeight,
                                                                        uint8_t span) noexcept
{
    const int   spanCells { juce::jmax (1, static_cast<int> (span)) };
    const float effCellW  { static_cast<float> (cellWidth * spanCells) };
    const float effCellH  { static_cast<float> (cellHeight) };
    const float inset     { GlyphConstraint::iconInset };

    const float targetW { effCellW * (1.0f - constraint.padLeft - constraint.padRight - inset * 2.0f) };
    const float targetH { effCellH * (1.0f - constraint.padTop - constraint.padBottom - inset * 2.0f) };

    const float naturalW { static_cast<float> (naturalWidth) };
    const float naturalH { static_cast<float> (naturalHeight) };

    float scaleX { 1.0f };
    float scaleY { 1.0f };

    if (naturalW > 0.0f and naturalH > 0.0f)
    {
        scaleX = targetW / naturalW;
        scaleY = targetH / naturalH;

        if (constraint.scaleMode == GlyphConstraint::ScaleMode::fit
            or constraint.scaleMode == GlyphConstraint::ScaleMode::adaptiveScale)
        {
            const float uniform { std::min (scaleX, scaleY) };
            scaleX = std::min (1.0f, uniform);
            scaleY = scaleX;
        }
        else if (constraint.scaleMode == GlyphConstraint::ScaleMode::cover)
        {
            const float uniform { std::min (scaleX, scaleY) };
            scaleX = uniform;
            scaleY = uniform;
        }
        // ScaleMode::stretch — scaleX/scaleY stay at their independent
        // targetW/naturalW, targetH/naturalH ratio (no aspect preservation),
        // verbatim old fall-through (no explicit branch needed).
    }

    if (constraint.maxAspectRatio > 0.0f)
    {
        const float scaledW { naturalW * scaleX };
        const float scaledH { naturalH * scaleY };

        if (scaledH > 0.0f and scaledW / scaledH > constraint.maxAspectRatio)
            scaleX = scaledH * constraint.maxAspectRatio / naturalW;
    }

    const float scaledW { naturalW * scaleX };
    const float scaledH { naturalH * scaleY };

    float posX { constraint.padLeft * effCellW };

    if (constraint.alignH == GlyphConstraint::Align::center)
        posX = (effCellW - scaledW) * 0.5f;
    else if (constraint.alignH == GlyphConstraint::Align::end)
        posX = effCellW - scaledW - constraint.padRight * effCellW;

    // Vertical placement — verbatim old scale/align/pad formula (endless
    // GlyphAtlas.mm @ 2e37f6d) computed bottom-up (CoreText's Y-up convention:
    // posY there measured distance from the box's BOTTOM edge). This atlas's
    // coverage canvas is top-down (Y increases downward, matches every other
    // mono/emoji write path in this class), so each branch below is the
    // top-down equivalent of that same formula: Align::center is symmetric
    // under the flip (unchanged); the old default/"start" (bottom-anchored)
    // case becomes `effCellH - scaledH - padBottom*effCellH` measured from the
    // top; the old "end" (top-anchored) case becomes `padTop*effCellH` directly.
    float posY { effCellH - scaledH - constraint.padBottom * effCellH };

    if (constraint.alignV == GlyphConstraint::Align::center)
        posY = (effCellH - scaledH) * 0.5f;
    else if (constraint.alignV == GlyphConstraint::Align::end)
        posY = constraint.padTop * effCellH;

    return ConstraintPlacement { scaleX, scaleY, posX, posY };
}

void GlyphAtlas::renderConstrainedMonoGlyphIntoAtlas (const uint8_t* naturalCoverage, int naturalStride,
                                                      int naturalWidth, int naturalHeight,
                                                      const ConstraintPlacement& placement,
                                                      juce::Point<int> packPos,
                                                      int cellBitmapWidth, int cellBitmapHeight) noexcept
{
    const int scaledWidth  { juce::jmax (1, juce::roundToInt (static_cast<float> (naturalWidth)  * placement.scaleX)) };
    const int scaledHeight { juce::jmax (1, juce::roundToInt (static_cast<float> (naturalHeight) * placement.scaleY)) };

    juce::Image naturalImage (juce::Image::SingleChannel, naturalWidth, naturalHeight, false, juce::SoftwareImageType());

    {
        juce::Image::BitmapData naturalData (naturalImage, juce::Image::BitmapData::writeOnly);

        for (int row { 0 }; row < naturalHeight; ++row)
        {
            std::memcpy (naturalData.getLinePointer (row),
                        naturalCoverage + static_cast<ptrdiff_t> (row) * naturalStride,
                        static_cast<size_t> (naturalWidth));
        }
    }

    // Image::rescaled() preserves the source pixelFormat (juce_Image.cpp:746-759),
    // so this SingleChannel coverage image stays SingleChannel through the
    // resample — same technique renderEmojiGlyphIntoAtlas() already uses for
    // its own ARGB ink rescale (jam_GlyphAtlasEdgeTable.cpp).
    const auto scaledImage { naturalImage.rescaled (scaledWidth, scaledHeight, juce::Graphics::highResamplingQuality) };
    jassert (scaledImage.getFormat() == juce::Image::SingleChannel);

    juce::HeapBlock<uint8_t> canvas (
        static_cast<size_t> (cellBitmapWidth) * static_cast<size_t> (cellBitmapHeight), true);

    juce::Image::BitmapData scaledData (scaledImage, juce::Image::BitmapData::readOnly);

    const int destX { juce::roundToInt (placement.posX) };
    const int destY { juce::roundToInt (placement.posY) };

    for (int row { 0 }; row < scaledHeight; ++row)
    {
        const int canvasY { destY + row };

        if (canvasY >= 0 and canvasY < cellBitmapHeight)
        {
            const auto* srcRow { scaledData.getLinePointer (row) };
            auto* destRow { canvas.getData() + static_cast<ptrdiff_t> (canvasY) * cellBitmapWidth };

            for (int col { 0 }; col < scaledWidth; ++col)
            {
                const int canvasX { destX + col };

                if (canvasX >= 0 and canvasX < cellBitmapWidth)
                    destRow[canvasX] = srcRow[col];
            }
        }
    }

    juce::Image::BitmapData atlasData (images.at (Type::mono).image, packPos.x, packPos.y,
                                       cellBitmapWidth, cellBitmapHeight,
                                       juce::Image::BitmapData::writeOnly);

    for (int row { 0 }; row < cellBitmapHeight; ++row)
    {
        writeMonoCoverageRow (atlasData.getLinePointer (row),
                             canvas.getData() + static_cast<ptrdiff_t> (row) * cellBitmapWidth,
                             cellBitmapWidth);
    }

    images.at (Type::mono).dirty = true;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
