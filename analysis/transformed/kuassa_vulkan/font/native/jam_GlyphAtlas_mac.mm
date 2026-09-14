/**
 * @file jam_GlyphAtlas_mac.mm
 * @brief macOS native code for jam::GlyphAtlas needing actual CoreText/CoreGraphics
 *        syntax, so it cannot live in the class's own plain .cpp:
 *        - findSystemFontFile() — system-font-file resolution for the lazy
 *          FreeType system-typeface tier (see
 *          jam::GlyphAtlas::getOrLoadSystemTypeface()).
 *        - GlyphAtlas::rasterizeNative() — the macOS half of the OS-native
 *          rasterization backend (jam_GlyphAtlasNative.cpp implements the Windows
 *          half under \#if JUCE_WINDOWS; exactly one is compiled per platform).
 *
 *  Compiled only under JUCE_MAC (jam_vulkan.mm), mirroring
 *  jam_LowLevelGraphicsGlyphRenderer_mac.mm's identical split.
 */

/*____________________________________________________________________________*/
// Resolves the on-disk font file backing a system typeface via CoreText — the
// same family+style CTFontDescriptor construction JUCE's own
// CoreTextTypeface::from() uses (juce_Fonts_mac.mm), so the resolved file is
// the exact file JUCE itself would have rasterized from. Declared in
// jam_GlyphAtlas.h.
juce::File jam::findSystemFontFile (const juce::String& fontFamily, const juce::String& fontStyle)
{
    juce::File result;

    CFStringRef familyRef { fontFamily.toCFString() };
    CFStringRef styleRef { fontStyle.toCFString() };

    CFStringRef keys[] { kCTFontFamilyNameAttribute, kCTFontStyleNameAttribute };
    CFTypeRef values[] { familyRef, styleRef };

    CFDictionaryRef attributes { CFDictionaryCreate (nullptr,
                                                      (const void**) keys,
                                                      (const void**) values,
                                                      2,
                                                      &kCFTypeDictionaryKeyCallBacks,
                                                      &kCFTypeDictionaryValueCallBacks) };

    CTFontDescriptorRef descriptor { CTFontDescriptorCreateWithAttributes (attributes) };

    if (descriptor != nullptr)
    {
        CFURLRef fontURL { (CFURLRef) CTFontDescriptorCopyAttribute (descriptor, kCTFontURLAttribute) };

        if (fontURL != nullptr)
        {
            CFStringRef pathRef { CFURLCopyFileSystemPath (fontURL, kCFURLPOSIXPathStyle) };

            if (pathRef != nullptr)
            {
                result = juce::File (juce::String::fromCFString (pathRef));
                CFRelease (pathRef);
            }

            CFRelease (fontURL);
        }

        CFRelease (descriptor);
    }

    CFRelease (attributes);
    CFRelease (styleRef);
    CFRelease (familyRef);

    return result;
}

/*____________________________________________________________________________*/
// rotatedGlyphBounds -- transforms @p ctBounds's four corners by key.rotation
// (radians, CG's own sign convention -- see renderNativeGlyphToScratch's doc
// comment on the JUCE-clockwise/y-down -> CG-counterclockwise/y-up negation)
// and returns their axis-aligned bounding rect. Identity for rotation == 0.0f
// (cos 1 / sin 0 leaves every corner unchanged) -- no special-casing needed.
// Mirrors jam_GlyphAtlasEdgeTable.cpp's extractGlyphBounds() ImageLayer branch,
// which does the identical corner-transform-then-AABB for the same reason
// (CTFontGetBoundingRectsForGlyphs/getLayersForGlyph measure the UNROTATED
// shape; the atlas cell must be sized to the ROTATED footprint instead).
static CGRect rotatedGlyphBounds (CGRect ctBounds, CGFloat rotationForCG) noexcept
{
    const CGAffineTransform rotate { CGAffineTransformMakeRotation (rotationForCG) };

    const CGPoint corners[] {
        CGPointApplyAffineTransform (CGPointMake (ctBounds.origin.x, ctBounds.origin.y), rotate),
        CGPointApplyAffineTransform (CGPointMake (ctBounds.origin.x + ctBounds.size.width, ctBounds.origin.y), rotate),
        CGPointApplyAffineTransform (CGPointMake (ctBounds.origin.x, ctBounds.origin.y + ctBounds.size.height), rotate),
        CGPointApplyAffineTransform (CGPointMake (ctBounds.origin.x + ctBounds.size.width,
                                                  ctBounds.origin.y + ctBounds.size.height), rotate)
    };

    CGFloat minX { corners[0].x }, maxX { corners[0].x };
    CGFloat minY { corners[0].y }, maxY { corners[0].y };

    for (const auto& corner : corners)
    {
        minX = std::min (minX, corner.x);
        maxX = std::max (maxX, corner.x);
        minY = std::min (minY, corner.y);
        maxY = std::max (maxY, corner.y);
    }

    return CGRectMake (minX, minY, maxX - minX, maxY - minY);
}

/*____________________________________________________________________________*/
// renderNativeGlyphToScratch — renders @p cgGlyph via CoreText into a freshly
// allocated @p naturalWidth x @p naturalHeight DeviceGray scratch buffer
// positioned via @p integralBounds — shared tail for rasterizeNative()'s
// unconstrained direct-atlas-write path AND its constrained cell-fit path
// (task B, GlyphConstraint's own documented "required extension")
// below; factored out of what was previously inlined once, since both paths
// now need the identical natural-size render before diverging on how the
// result reaches the atlas.
//
// @p rotation is key.rotation (radians, jam_GlyphAtlas.h's Key doc comment),
// 0.0f for untransformed text -- baked directly into the rasterized bitmap via
// the CGBitmapContext's own CTM rather than any glyph-outline transform, so it
// applies uniformly to CoreText's native rendering path. CG's coordinate
// system is y-up (CGContextRotateCTM's positive angle is counterclockwise);
// key.rotation is measured in JUCE's y-down device pixel space (clockwise-
// positive, AffineTransform::rotation()'s convention, jam_AffineTransform.cpp:110-117)
// -- negating converts between the two conventions so the rasterized bitmap's
// visible rotation matches the composed device transform's. CGContextRotateCTM
// is called AFTER CGContextTranslateCTM here so it is applied to points BEFORE
// the translation (Core Graphics applies the most-recently-concatenated matrix
// first) -- rotation pivots exactly at the pen origin (0, 0), integralBounds
// having already been sized to the ROTATED footprint by rotatedGlyphBounds().
static void renderNativeGlyphToScratch (CTFontRef font, CGGlyph cgGlyph, CGRect integralBounds,
                                        float rotation, int naturalWidth, int naturalHeight,
                                        bool embolden, juce::HeapBlock<uint8_t>& scratch) noexcept
{
    // Stroke width for embolden's fill+stroke drawing mode below, in the same
    // points-space as this CGBitmapContext's CTM. Matches FreeType's own
    // fixed synthetic-embolden strength (1 << 6 in 26.6 fixed-point = 1.0
    // point, jam_GlyphAtlasFreeType.cpp's FT_Outline_Embolden call) so
    // both rasterization backends thicken stems by the same physical amount.
    static constexpr float emboldenStrokeWidth { 1.0f };

    scratch.allocate (static_cast<size_t> (naturalWidth) * static_cast<size_t> (naturalHeight), true);

    CGColorSpaceRef greySpace { CGColorSpaceCreateDeviceGray() };
    CGContextRef ctx { CGBitmapContextCreate (scratch.get(),
                                              static_cast<size_t> (naturalWidth),
                                              static_cast<size_t> (naturalHeight),
                                              8,
                                              static_cast<size_t> (naturalWidth),
                                              greySpace,
                                              kCGImageAlphaNone) };

    if (ctx != nullptr)
    {
        CGContextSetShouldAntialias (ctx, true);
        CGContextSetShouldSmoothFonts (ctx, false);
        CGContextSetGrayFillColor (ctx, 1.0, 1.0);
        CGContextSetTextDrawingMode (ctx, kCGTextFill);

        CGContextTranslateCTM (ctx, -integralBounds.origin.x, -integralBounds.origin.y);
        CGContextRotateCTM (ctx, static_cast<CGFloat> (-rotation));

        const CGPoint position {};

        if (embolden)
        {
            CGContextSetGrayStrokeColor (ctx, 1, 1);
            CGContextSetLineWidth (ctx, emboldenStrokeWidth);
            CGContextSetTextDrawingMode (ctx, kCGTextFillStroke);
        }

        CTFontDrawGlyphs (font, &cgGlyph, &position, 1, ctx);

        CGContextRelease (ctx);
    }

    CGColorSpaceRelease (greySpace);
}

/*____________________________________________________________________________*/
// GlyphAtlas::rasterizeNative — macOS CoreText half (see jam_GlyphAtlas.cpp's
// #if JUCE_WINDOWS block for the DirectWrite half; both implement the same
// declaration in jam_GlyphAtlas.h, exactly one compiled per platform).
//
// CoreText's own font-smoothing boldening rendering path — deliberately
// distinct from FreeType's autofit hinting (see rasterizeNative()'s doc comment).
// Adapted from the pre-Vulkan CoreText glyph Packer
// (jam_graphics/fonts/font/glyph/jam_glyph_packer.mm, recovered from git
// history at commit 1bd4b3593^): same
// CTFontGetBoundingRectsForGlyphs -> antialias-pad -> CGRectIntegral bounds
// pipeline and CGBitmapContext DeviceGray (no alpha channel — the grayscale
// value itself IS the coverage byte) fill-only rendering, rebuilt against
// this atlas's own packGlyph()/BitmapData conventions instead of the deleted
// Packer's pooled-context/writePixels() machinery.
jam::GlyphAtlas::GlyphBounds jam::GlyphAtlas::rasterizeNative (const Key& key, juce::Point<int>& packPos) noexcept
{
    // Anti-aliased CoreText rendering extends slightly beyond
    // CTFontGetBoundingRectsForGlyphs's outline bounds — pad by one physical
    // pixel before integralizing (key.fontSize is already DPI-scaled physical
    // pixels; mirrors the historical Packer's displayScale-sized aaPad).
    static constexpr CGFloat nativeAntiAliasPadding { 1.0 };

    // In native rasterization mode nothing else populates freetypeFaces (only
    // rasterizeFreeType() reaches getOrLoadSystemTypeface() otherwise) — this
    // native backend needs the same registered/resolved FaceEntry lookup below
    // (the freetypeFaces.contains()/.face != nullptr registered-bytes branch),
    // so it resolves it here on its own first miss, mirroring rasterizeFreeType()'s
    // identical idiom.
    if (not freetypeFaces.contains (key.typeface))
        getOrLoadSystemTypeface (key.typeface);

    GlyphBounds bounds {};

    const auto pixelsPerEm {
        key.fontSize * key.typeface->getMetrics (juce::TypefaceMetricsKind::portable).heightToPoints
    };

    // Cached per typeface + point size (NativeFontEntry, jam_GlyphAtlas.h) —
    // this backend used to create and CFRelease a CTFontRef on EVERY glyph
    // miss, a real per-glyph cost for any typeface. Rebuilt only when this
    // typeface is next requested at a different size.
    //
    // isNewNativeFontEntry (map miss, checked BEFORE operator[] inserts the
    // default entry) is also this backend's one-time colour-glyph detection
    // site — see NativeFontEntry::isColourGlyph's doc comment. Checked once
    // per typeface, never per glyph.
    const bool isNewNativeFontEntry { not nativeFonts.contains (key.typeface) };
    auto& cached { nativeFonts[key.typeface] };

    if (isNewNativeFontEntry)
        cached.isColourGlyph = key.typeface->getColourGlyphFormats() != 0;

    if (cached.isColourGlyph)
    {
        // Colour fonts (Apple Color Emoji, or any other bitmap/SVG/COLR
        // typeface) are deliberately excluded from this native CoreText
        // path — CGBitmapContextCreate below fills DeviceGray only, so it can
        // never produce more than a monochrome silhouette of a colour glyph.
        // Reject here so rasterize() falls through to JUCE's
        // getLayersForGlyph() ImageLayer emoji path, which already renders
        // these glyphs in colour — verbatim policy of getOrLoadSystemTypeface()'s
        // FT_HAS_COLOR rejection (jam_GlyphAtlasFreeType.cpp): "Color fonts...
        // deliberately excluded — falls through to JUCE's getLayersForGlyph()
        // ImageLayer emoji path."
        return rasterizeEdgeTable (key, packPos);
    }

    if (cached.fontRef == nullptr or cached.pixelsPerEm != static_cast<float> (pixelsPerEm))
    {
        if (cached.fontRef != nullptr)
            CFRelease (static_cast<CFTypeRef> (cached.fontRef));

        CTFontRef newFont { nullptr };

        // Typefaces whose FaceEntry holds registered/resolved font bytes
        // (registerTypeface()'s embedded defaults, getOrLoadSystemTypeface()'s
        // lazily-resolved system fonts — see FaceEntry's doc comment; a
        // null face there means resolution failed or was never attempted,
        // never real bytes) build the CTFont from those SAME bytes, so this
        // native backend rasterizes the exact registered font file instead
        // of whatever CTFontCreateWithName's family+style lookup would
        // otherwise resolve — which, for an embedded font with no matching
        // installed system font, is a garbled or altogether wrong glyph.
        // Only typefaces with no registered bytes — genuine system fonts —
        // fall back to CTFontCreateWithName, correct there by definition.
        if (freetypeFaces.contains (key.typeface) and freetypeFaces.at (key.typeface).face != nullptr)
        {
            const auto& fontBytes { freetypeFaces.at (key.typeface).fontBytes };

            // No-copy provider over fontBytes — safe because freetypeFaces
            // (and therefore fontBytes) outlives this GlyphAtlas's every
            // cached CTFontRef (both destroyed together, in this order, by
            // ~GlyphAtlas()). nullptr release callback: the provider must
            // never free memory it does not own.
            CGDataProviderRef dataProvider { CGDataProviderCreateWithData (
                nullptr, fontBytes.getData(), fontBytes.getSize(), nullptr) };
            CGFontRef cgFont { CGFontCreateWithDataProvider (dataProvider) };

            if (cgFont != nullptr)
            {
                newFont = CTFontCreateWithGraphicsFont (cgFont, static_cast<CGFloat> (pixelsPerEm), nullptr, nullptr);
                CGFontRelease (cgFont);
            }

            CGDataProviderRelease (dataProvider);
        }
        else
        {
            CFStringRef fontName { key.typeface->getName().toCFString() };
            newFont = CTFontCreateWithName (fontName, static_cast<CGFloat> (pixelsPerEm), nullptr);
            CFRelease (fontName);
        }

        jassert (newFont != nullptr);

        cached.fontRef = (void*) newFont;
        cached.pixelsPerEm = static_cast<float> (pixelsPerEm);
    }

    CTFontRef font { static_cast<CTFontRef> (cached.fontRef) };

    const CGGlyph cgGlyph { static_cast<CGGlyph> (key.glyphIndex) };
    CGRect ctBounds {};
    CTFontGetBoundingRectsForGlyphs (font, kCTFontOrientationHorizontal, &cgGlyph, &ctBounds, 1);

    // key.rotation (radians, 0.0f for untransformed text) resizes the pack cell
    // to the ROTATED footprint before anti-alias padding/integralizing --
    // CTFontGetBoundingRectsForGlyphs measures the UNROTATED glyph, so
    // rotatedGlyphBounds() re-derives the AABB the same way
    // extractGlyphBounds()'s ImageLayer branch does (jam_GlyphAtlasEdgeTable.cpp).
    const CGRect rotatedBounds { rotatedGlyphBounds (ctBounds, static_cast<CGFloat> (-key.rotation)) };
    const CGRect padded { CGRectInset (rotatedBounds, -nativeAntiAliasPadding, -nativeAntiAliasPadding) };
    const CGRect integralBounds { CGRectIntegral (padded) };

    bounds.size = jam::Size<int> (static_cast<int> (integralBounds.size.width),
                                  static_cast<int> (integralBounds.size.height));
    bounds.bearing = juce::Point<int> (static_cast<int> (integralBounds.origin.x),
                                       static_cast<int> (integralBounds.origin.y + integralBounds.size.height));
    const auto [naturalWidth, naturalHeight] { bounds.size };

    if (naturalWidth > 0 and naturalHeight > 0)
    {
        // A real cell-run caller (key.cellWidth > 0, see Key's own doc comment)
        // whose codepoint has an active PUA/Nerd-Font GlyphConstraint renders
        // scaled/aligned/padded into a box-sized bitmap, bearingX = 0 /
        // bearingY = baseline — verbatim old constrained-glyph semantics
        // (this exact file, pre-Vulkan CoreText Packer ancestor).
        // Unconstrained mono text (the `else` branch) is bit-for-bit
        // unchanged from before this wiring.
        const jam::GlyphConstraint constraint { jam::GlyphConstraint::getConstraint (key.codepoint) };

        if (constraint.isActive() and key.cellWidth > 0)
        {
            const int spanCells        { juce::jmax (1, static_cast<int> (key.span)) };
            const int cellBitmapWidth  { key.cellWidth * spanCells };
            const int cellBitmapHeight { key.cellHeight };

            packPos = packGlyph (cellBitmapWidth, cellBitmapHeight, Type::mono);

            if (packPos.x >= 0)
            {
                juce::HeapBlock<uint8_t> scratch;
                renderNativeGlyphToScratch (font, cgGlyph, integralBounds, key.rotation, naturalWidth, naturalHeight, embolden, scratch);

                const auto placement { computeConstraintPlacement (constraint, naturalWidth, naturalHeight,
                                                                   key.cellWidth, key.cellHeight, key.span) };

                renderConstrainedMonoGlyphIntoAtlas (scratch.get(), naturalWidth, naturalWidth, naturalHeight,
                                                     placement, packPos, cellBitmapWidth, cellBitmapHeight);

                bounds.size = jam::Size<int> (cellBitmapWidth, cellBitmapHeight);
                bounds.bearing = juce::Point<int> (0, key.baseline);
            }
        }
        else
        {
            packPos = packGlyph (naturalWidth, naturalHeight, Type::mono);

            if (packPos.x >= 0)
            {
                // Scratch DeviceGray context sized exactly to the glyph (mirrors the
                // historical Packer's pooled-context approach) — kCGImageAlphaNone means
                // the grayscale channel IS the coverage byte, no separate alpha to extract.
                juce::HeapBlock<uint8_t> scratch;
                renderNativeGlyphToScratch (font, cgGlyph, integralBounds, key.rotation, naturalWidth, naturalHeight, embolden, scratch);

                juce::Image::BitmapData atlasData (images.at (Type::mono).image, packPos.x, packPos.y,
                                                   naturalWidth, naturalHeight,
                                                   juce::Image::BitmapData::writeOnly);

                for (int row { 0 }; row < naturalHeight; ++row)
                    writeMonoCoverageRow (atlasData.getLinePointer (row),
                                         scratch.get() + static_cast<ptrdiff_t> (row) * naturalWidth,
                                         naturalWidth);

                images.at (Type::mono).dirty = true;
            }
        }
    }

    // font is owned by nativeFonts (cached, released in ~GlyphAtlas()) — no
    // per-call CFRelease here anymore.

    return bounds;
}
