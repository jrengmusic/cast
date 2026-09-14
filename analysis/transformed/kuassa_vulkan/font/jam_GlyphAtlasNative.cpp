//
// Native file: rasterizeNative()'s Windows/DirectWrite half (findSystemFontFile(),
// findDirectWriteFontFace(), rasterizeNative()) — entirely #if JUCE_WINDOWS.
// jam_GlyphAtlas_mac.mm implements the macOS/CoreText half of the same
// declarations (jam_GlyphAtlas.h).

namespace jam
{
/*____________________________________________________________________________*/
#if JUCE_WINDOWS
// Bytes per pixel in the ClearType 3x1 alpha texture CreateAlphaTexture()
// produces below (R,G,B subpixel coverage) — shared by createNativeAlphaTexture()
// and GlyphAtlas::writeNativeGlyphIntoAtlas().
static constexpr UINT32 clearTypeSubpixelBytesPerPixel { 3u };

// averageClearTypeToGrayscale — averages ClearType's 3x1 R,G,B subpixel
// coverage down to one grayscale byte per pixel, WITHOUT applying coverageLut
// (the shared renderConstrainedMonoGlyphIntoAtlas()/writeMonoCoverageRow()
// apply it at their own final atlas-write site instead) — the constrained
// cell-fit path's (task B) own natural-coverage extraction, parallel to
// writeNativeGlyphIntoAtlas()'s identical average for the unconstrained
// direct-write path (that function applies coverageLut itself, per its own
// doc comment in jam_GlyphAtlas.h).
static void averageClearTypeToGrayscale (const uint8_t* rgbTexture, int width, int height,
                                         juce::HeapBlock<uint8_t>& grayscaleOut) noexcept
{
    grayscaleOut.allocate (static_cast<size_t> (width) * static_cast<size_t> (height), false);

    for (int row { 0 }; row < height; ++row)
    {
        const auto* srcRow { rgbTexture + static_cast<size_t> (row) * static_cast<size_t> (width)
                                         * clearTypeSubpixelBytesPerPixel };
        auto* destRow { grayscaleOut.getData() + static_cast<ptrdiff_t> (row) * width };

        for (int col { 0 }; col < width; ++col)
        {
            const int r { srcRow[static_cast<size_t> (col) * clearTypeSubpixelBytesPerPixel + 0] };
            const int g { srcRow[static_cast<size_t> (col) * clearTypeSubpixelBytesPerPixel + 1] };
            const int b { srcRow[static_cast<size_t> (col) * clearTypeSubpixelBytesPerPixel + 2] };
            destRow[col] = static_cast<uint8_t> ((r + g + b) / 3);
        }
    }
}

// Windows presentation of getOrLoadSystemTypeface()'s platform resolver — plain
// DirectWrite/COM calls need no separate translation unit, mirroring
// jam_LowLevelGraphicsGlyphRenderer.cpp's own Windows GDI presentation branch
// (that file's own top-of-file comment). macOS needs actual Objective-C syntax
// instead — that half lives in jam_GlyphAtlas_mac.mm.
juce::File findSystemFontFile (const juce::String& fontFamily, const juce::String& fontStyle)
{
    juce::File result;

    IDWriteFactory* factory { nullptr };

    if (SUCCEEDED (DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED, __uuidof (IDWriteFactory),
                                        reinterpret_cast<IUnknown**> (&factory))))
    {
        IDWriteFontCollection* systemFonts { nullptr };

        if (SUCCEEDED (factory->GetSystemFontCollection (&systemFonts)))
        {
            UINT32 familyIndex { 0 };
            BOOL familyFound { FALSE };

            if (SUCCEEDED (systemFonts->FindFamilyName (fontFamily.toWideCharPointer(), &familyIndex, &familyFound))
                and familyFound)
            {
                IDWriteFontFamily* fontFamilyRef { nullptr };

                if (SUCCEEDED (systemFonts->GetFontFamily (familyIndex, &fontFamilyRef)))
                {
                    // Best-effort style mapping — the only two variants the
                    // host application's shipped families and common system fonts distinguish by name.
                    const auto weight { fontStyle.containsIgnoreCase ("Bold")
                                       ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR };
                    const auto style { fontStyle.containsIgnoreCase ("Italic")
                                      ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL };

                    IDWriteFont* font { nullptr };

                    if (SUCCEEDED (fontFamilyRef->GetFirstMatchingFont (weight, DWRITE_FONT_STRETCH_NORMAL, style, &font)))
                    {
                        IDWriteFontFace* fontFace { nullptr };

                        if (SUCCEEDED (font->CreateFontFace (&fontFace)))
                        {
                            UINT32 fileCount { 1 };
                            IDWriteFontFile* fontFile { nullptr };

                            if (SUCCEEDED (fontFace->GetFiles (&fileCount, &fontFile)) and fontFile != nullptr)
                            {
                                const void* referenceKey { nullptr };
                                UINT32 referenceKeySize { 0 };

                                if (SUCCEEDED (fontFile->GetReferenceKey (&referenceKey, &referenceKeySize)))
                                {
                                    IDWriteFontFileLoader* loader { nullptr };

                                    if (SUCCEEDED (fontFile->GetLoader (&loader)))
                                    {
                                        IDWriteLocalFontFileLoader* localLoader { nullptr };

                                        if (SUCCEEDED (loader->QueryInterface (__uuidof (IDWriteLocalFontFileLoader),
                                                                              reinterpret_cast<void**> (&localLoader))))
                                        {
                                            UINT32 pathLength { 0 };

                                            if (SUCCEEDED (localLoader->GetFilePathLengthFromKey (
                                                    referenceKey, referenceKeySize, &pathLength)))
                                            {
                                                // Capacity-only ctor — only .data() is ever read below (DirectWrite
                                                // writes directly into the raw buffer via pointer), so no element
                                                // count needs establishing via resize().
                                                jam::Array<wchar_t> path (static_cast<int> (pathLength) + 1);

                                                if (SUCCEEDED (localLoader->GetFilePathFromKey (
                                                        referenceKey, referenceKeySize, path.data(), pathLength + 1)))
                                                    result = juce::File (juce::String (path.data()));
                                            }

                                            localLoader->Release();
                                        }

                                        loader->Release();
                                    }
                                }

                                fontFile->Release();
                            }

                            fontFace->Release();
                        }

                        font->Release();
                    }

                    fontFamilyRef->Release();
                }
            }

            systemFonts->Release();
        }

        factory->Release();
    }

    return result;
}

// findDirectWriteFontFace — rasterizeNative()'s own font resolution. Mirrors
// findSystemFontFile()'s family/style traversal above (same best-effort
// weight/style mapping), but stops one step earlier: it returns the live
// IDWriteFontFace itself (needed by IDWriteGlyphRunAnalysis below) rather than
// extracting an on-disk file path. Caller owns the returned interface (one
// Release() required on success). Reached only for typefaces with no
// registered/pulled bytes (rasterizeNative()'s own hasRegisteredBytes branch) —
// genuine installed system fonts, correct by GetSystemFontCollection() lookup.
static bool findDirectWriteFontFace (IDWriteFactory* factory, const juce::String& fontFamily,
                                        const juce::String& fontStyle, bool embolden, IDWriteFontFace** outFontFace)
{
    bool resolved { false };

    IDWriteFontCollection* systemFonts { nullptr };

    if (SUCCEEDED (factory->GetSystemFontCollection (&systemFonts)))
    {
        UINT32 familyIndex { 0 };
        BOOL familyFound { FALSE };

        if (SUCCEEDED (systemFonts->FindFamilyName (fontFamily.toWideCharPointer(), &familyIndex, &familyFound))
            and familyFound)
        {
            IDWriteFontFamily* fontFamilyRef { nullptr };

            if (SUCCEEDED (systemFonts->GetFontFamily (familyIndex, &fontFamilyRef)))
            {
                // Best-effort style mapping — same two variants findSystemFontFile() distinguishes.
                const auto weight { fontStyle.containsIgnoreCase ("Bold")
                                   ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_REGULAR };
                const auto style { fontStyle.containsIgnoreCase ("Italic")
                                  ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL };

                IDWriteFont* font { nullptr };

                if (SUCCEEDED (fontFamilyRef->GetFirstMatchingFont (weight, DWRITE_FONT_STRETCH_NORMAL, style, &font)))
                {
                    IDWriteFontFace* baseFontFace { nullptr };

                    if (SUCCEEDED (font->CreateFontFace (&baseFontFace)))
                    {
                        UINT32 fileCount { 1 };
                        IDWriteFontFile* fontFile { nullptr };

                        if (SUCCEEDED (baseFontFace->GetFiles (&fileCount, &fontFile)) and fontFile != nullptr)
                        {
                            const auto faceIndex { baseFontFace->GetIndex() };
                            const auto faceType { baseFontFace->GetType() };
                            const auto simulations { embolden ? DWRITE_FONT_SIMULATIONS_BOLD
                                                               : DWRITE_FONT_SIMULATIONS_NONE };

                            resolved = SUCCEEDED (factory->CreateFontFace (faceType, fileCount, &fontFile, faceIndex,
                                                                           simulations, outFontFace));

                            fontFile->Release();
                        }

                        baseFontFace->Release();
                    }

                    font->Release();
                }

                fontFamilyRef->Release();
            }
        }

        systemFonts->Release();
    }

    return resolved;
}

// rasterizeNative()'s analysis-creation step: builds the DWRITE_GLYPH_RUN
// (one glyph, no advance/offset — this atlas measures+rasterizes single glyphs
// in isolation, never a shaped run) and calls CreateGlyphRunAnalysis
// (DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC, DWRITE_MEASURING_MODE_NATURAL).
// Same JUCE-height-to-pixels-per-em conversion the FreeType backend uses
// (loadFreeTypeGlyphBounds()) — key.fontSize is already DPI-scaled physical
// pixels, so fontEmSize is passed directly with pixelsPerDip = 1.0f. Caller
// owns the returned interface (one Release() required on success).
//
// key.rotation (radians, jam_GlyphAtlas.h's Key doc comment; 0.0f for
// untransformed text) is passed as CreateGlyphRunAnalysis's transform
// parameter — DirectWrite's own coordinate system is top-left-origin/y-down,
// the same handedness as JUCE's AffineTransform::rotation() (clockwise-
// positive, jam_AffineTransform.cpp:110-117), so this DWRITE_MATRIX reproduces
// key.rotation directly with no sign conversion (contrast jam_GlyphAtlas_mac.mm's
// CoreText half, whose y-up CTM negates the angle). GetAlphaTextureBounds()/
// CreateAlphaTexture() below already measure/render the ROTATED glyph, since
// the transform is baked into this analysis.
static IDWriteGlyphRunAnalysis* createGlyphRunAnalysis (IDWriteFactory* factory, IDWriteFontFace* fontFace,
                                                        const GlyphAtlas::Key& key) noexcept
{
    const auto pixelsPerEm {
        key.fontSize * key.typeface->getMetrics (juce::TypefaceMetricsKind::portable).heightToPoints
    };

    const UINT16 glyphIndex { static_cast<UINT16> (key.glyphIndex) };
    const FLOAT glyphAdvance { 0.0f };
    const DWRITE_GLYPH_OFFSET glyphOffset {};

    DWRITE_GLYPH_RUN glyphRun {};
    glyphRun.fontFace = fontFace;
    glyphRun.fontEmSize = static_cast<FLOAT> (pixelsPerEm);
    glyphRun.glyphCount = 1;
    glyphRun.glyphIndices = &glyphIndex;
    glyphRun.glyphAdvances = &glyphAdvance;
    glyphRun.glyphOffsets = &glyphOffset;

    const auto cosRotation { std::cos (key.rotation) };
    const auto sinRotation { std::sin (key.rotation) };
    const DWRITE_MATRIX rotationTransform {
        static_cast<FLOAT> (cosRotation), static_cast<FLOAT> (sinRotation),
        static_cast<FLOAT> (-sinRotation), static_cast<FLOAT> (cosRotation),
        0.0f, 0.0f
    };

    IDWriteGlyphRunAnalysis* analysis { nullptr };

    const bool created {
        SUCCEEDED (factory->CreateGlyphRunAnalysis (&glyphRun, 1.0f, &rotationTransform,
                                                    DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
                                                    DWRITE_MEASURING_MODE_NATURAL,
                                                    0.0f, 0.0f, &analysis))
    };

    return created ? analysis : nullptr;
}

// rasterizeNative()'s texture-readback step: fills outRgbTexture with
// analysis's ClearType 3x1 alpha texture (3-bytes-per-pixel R,G,B subpixel
// coverage) for the already-measured textureBounds/width/height. Caller has
// already confirmed width/height > 0 and reserved packPos via packGlyph().
static bool createNativeAlphaTexture (IDWriteGlyphRunAnalysis* analysis, const RECT& textureBounds,
                                      int width, int height, jam::Array<BYTE>& outRgbTexture) noexcept
{
    const UINT32 rgbTextureSize { static_cast<UINT32> (width) * static_cast<UINT32> (height)
                                  * clearTypeSubpixelBytesPerPixel };
    outRgbTexture.resize (static_cast<int> (rgbTextureSize));

    return SUCCEEDED (analysis->CreateAlphaTexture (DWRITE_TEXTURE_CLEARTYPE_3x1, &textureBounds,
                                                    outRgbTexture.data(), rgbTextureSize));
}

// GlyphAtlas::rasterizeNative — Windows DirectWrite half (see jam_GlyphAtlas_mac.mm
// for the macOS/CoreText half; both implement the same declaration in
// jam_GlyphAtlas.h, exactly one compiled per platform — mirrors
// findSystemFontFile()'s existing per-platform split above).
//
// IDWriteGlyphRunAnalysis measures + rasterizes in one pass:
// GetAlphaTextureBounds() gives the pixel bounds (DirectWrite's baseline-relative
// coordinate space: y increases downward, so a negative `top` is above the
// baseline — bearingY negates it to match Region's "positive = above baseline"
// convention), then createNativeAlphaTexture()/writeNativeGlyphIntoAtlas() below
// fill and apply the ClearType alpha texture to the atlas.
GlyphAtlas::GlyphBounds GlyphAtlas::rasterizeNative (const Key& key, juce::Point<int>& packPos) noexcept
{
    GlyphBounds bounds {};

    // In native rasterization mode nothing else populates freetypeFaces (only
    // rasterizeFreeType() reaches getOrLoadSystemTypeface() otherwise) — this
    // native backend needs the same registered/resolved FaceEntry lookup below
    // (hasRegisteredBytes), so it resolves it here on its own first miss,
    // mirroring rasterizeFreeType()'s identical idiom.
    if (not freetypeFaces.contains (key.typeface))
        getOrLoadSystemTypeface (key.typeface);

    // Cached once per GlyphAtlas — DWRITE_FACTORY_TYPE_SHARED itself already
    // returns a process-wide singleton, so this only does real work on the
    // first call; the in-memory font file loader it creates must stay
    // registered for as long as any cached IDWriteFontFace built through it
    // (below) is still alive.
    if (directWriteFactory == nullptr)
    {
        juce::ComSmartPtr<IDWriteFactory> baseFactory;

        if (SUCCEEDED (DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED, __uuidof (IDWriteFactory),
                                            reinterpret_cast<IUnknown**> (baseFactory.resetAndGetPointerAddress())))
            and SUCCEEDED (baseFactory.QueryInterface (directWriteFactory)))
        {
            if (SUCCEEDED (directWriteFactory->CreateInMemoryFontFileLoader (inMemoryFontFileLoader.resetAndGetPointerAddress())))
                directWriteFactory->RegisterFontFileLoader (inMemoryFontFileLoader);
        }

        jassert (directWriteFactory != nullptr and inMemoryFontFileLoader != nullptr);
    }

    // Colour-glyph detection cached per typeface (NativeFontEntry, jam_GlyphAtlas.h)
    // — mirrors jam_GlyphAtlas_mac.mm's rasterizeNative() identically: checked
    // once, on this NativeFontEntry's first miss, never per glyph.
    const bool isNewNativeFontEntry { not nativeFonts.contains (key.typeface) };
    auto& cached { nativeFonts[key.typeface] };

    if (isNewNativeFontEntry)
        cached.isColourGlyph = key.typeface->getColourGlyphFormats() != 0;

    if (cached.isColourGlyph)
    {
        // Colour fonts (Segoe UI Emoji, or any other bitmap/SVG/COLR
        // typeface) are deliberately excluded from this native DirectWrite
        // path — writeNativeGlyphIntoAtlas() below averages ClearType's R,G,B
        // subpixel coverage down to one grayscale byte, so it can never
        // produce more than a monochrome silhouette of a colour glyph. Reject
        // here so rasterize() falls through to JUCE's getLayersForGlyph()
        // ImageLayer emoji path, which already renders these glyphs in colour
        // — verbatim policy of getOrLoadSystemTypeface()'s FT_HAS_COLOR
        // rejection (jam_GlyphAtlasFreeType.cpp): "Color fonts... deliberately
        // excluded — falls through to JUCE's getLayersForGlyph() ImageLayer
        // emoji path."
        return rasterizeEdgeTable (key, packPos);
    }

    // One-time IDWriteFontFace resolution for this typeface, cached in
    // NativeFontEntry::fontRef exactly like macOS caches its CTFontRef —
    // unlike CTFontRef this face is resolution-independent (fontEmSize is
    // supplied per glyph run below instead), so no per-size rebuild is needed.
    // Registered/pulled font bytes (hasRegisteredBytes) resolve through the
    // in-memory font file loader — the exact registered font file, never a
    // same-named installed system font substituted in its place; typefaces
    // with no registered bytes fall back to GetSystemFontCollection() lookup
    // (findDirectWriteFontFace()), correct there by definition. A failed
    // resolution leaves fontRef null — the negative-cache shape freetypeFaces'
    // FaceEntry already uses — so this typeface's resolution is never
    // retried per glyph.
    if (isNewNativeFontEntry and directWriteFactory != nullptr)
    {
        const bool hasRegisteredBytes { freetypeFaces.contains (key.typeface)
            and freetypeFaces.at (key.typeface).face != nullptr };

        IDWriteFontFace* newFontFace { nullptr };

        if (hasRegisteredBytes and inMemoryFontFileLoader != nullptr)
        {
            const auto& fontBytes { freetypeFaces.at (key.typeface).fontBytes };
            juce::ComSmartPtr<IDWriteFontFile> fontFile;

            if (SUCCEEDED (inMemoryFontFileLoader->CreateInMemoryFontFileReference (
                    directWriteFactory, fontBytes.getData(), static_cast<UINT32> (fontBytes.getSize()),
                    nullptr, fontFile.resetAndGetPointerAddress())))
            {
                BOOL isSupported { FALSE };
                DWRITE_FONT_FILE_TYPE fileType {};
                DWRITE_FONT_FACE_TYPE faceType {};
                UINT32 faceCount { 0 };

                if (SUCCEEDED (fontFile->Analyze (&isSupported, &fileType, &faceType, &faceCount)) and isSupported)
                {
                    IDWriteFontFile* fontFilePtr { fontFile };
                    const auto simulations { embolden ? DWRITE_FONT_SIMULATIONS_BOLD
                                                       : DWRITE_FONT_SIMULATIONS_NONE };

                    directWriteFactory->CreateFontFace (faceType, 1, &fontFilePtr, 0,
                                                        simulations, &newFontFace);
                }
            }
        }
        else if (not hasRegisteredBytes)
        {
            findDirectWriteFontFace (directWriteFactory, key.typeface->getName(), key.typeface->getStyle(),
                                        embolden, &newFontFace);
        }

        cached.fontRef = static_cast<void*> (newFontFace);
    }

    if (cached.fontRef == nullptr)
        return rasterizeEdgeTable (key, packPos);

    // fontFace is owned by nativeFonts (cached, released in ~GlyphAtlas()) —
    // no per-call Release() here.
    IDWriteFontFace* fontFace { static_cast<IDWriteFontFace*> (cached.fontRef) };

    if (auto* analysis { createGlyphRunAnalysis (directWriteFactory, fontFace, key) })
    {
        RECT textureBounds {};

        if (SUCCEEDED (analysis->GetAlphaTextureBounds (DWRITE_TEXTURE_CLEARTYPE_3x1, &textureBounds)))
        {
            bounds.size = jam::Size<int> (static_cast<int> (textureBounds.right - textureBounds.left),
                                          static_cast<int> (textureBounds.bottom - textureBounds.top));
            bounds.bearing = juce::Point<int> (textureBounds.left, -textureBounds.top);
            const auto [naturalWidth, naturalHeight] { bounds.size };

            if (naturalWidth > 0 and naturalHeight > 0)
            {
                // A real cell-run caller (key.cellWidth > 0, see Key's own
                // doc comment) whose codepoint has an active GlyphConstraint
                // renders scaled/aligned/padded into a box-sized bitmap,
                // bearingX = 0 / bearingY = baseline — the PUA/Nerd Font
                // constrained-glyph semantics shared with
                // jam_GlyphAtlas_mac.mm's identical wiring. Unconstrained
                // mono text (the `else` branch) is bit-for-bit unchanged
                // from before this wiring.
                const GlyphConstraint constraint { GlyphConstraint::getConstraint (key.codepoint) };

                if (constraint.isActive() and key.cellWidth > 0)
                {
                    const int spanCells        { juce::jmax (1, static_cast<int> (key.span)) };
                    const int cellBitmapWidth  { key.cellWidth * spanCells };
                    const int cellBitmapHeight { key.cellHeight };

                    packPos = packGlyph (cellBitmapWidth, cellBitmapHeight, Type::mono);

                    if (packPos.x >= 0)
                    {
                        jam::Array<BYTE> rgbTexture;

                        if (createNativeAlphaTexture (analysis, textureBounds, naturalWidth, naturalHeight,
                                                      rgbTexture))
                        {
                            juce::HeapBlock<uint8_t> grayscale;
                            averageClearTypeToGrayscale (rgbTexture.data(), naturalWidth, naturalHeight, grayscale);

                            const auto placement { computeConstraintPlacement (constraint, naturalWidth, naturalHeight,
                                                                               key.cellWidth, key.cellHeight, key.span) };

                            renderConstrainedMonoGlyphIntoAtlas (grayscale.get(), naturalWidth,
                                                                 naturalWidth, naturalHeight, placement,
                                                                 packPos, cellBitmapWidth, cellBitmapHeight);

                            bounds.size = jam::Size<int> (cellBitmapWidth, cellBitmapHeight);
                            bounds.bearing = juce::Point<int> (0, key.baseline);
                        }
                    }
                }
                else
                {
                    packPos = packGlyph (naturalWidth, naturalHeight, Type::mono);

                    if (packPos.x >= 0)
                    {
                        jam::Array<BYTE> rgbTexture;

                        if (createNativeAlphaTexture (analysis, textureBounds, naturalWidth, naturalHeight,
                                                      rgbTexture))
                            writeNativeGlyphIntoAtlas (rgbTexture.data(), packPos, naturalWidth, naturalHeight);
                    }
                }
            }
        }

        analysis->Release();
    }

    return bounds;
}

// GlyphAtlas::writeNativeGlyphIntoAtlas — see jam_GlyphAtlas.h's doc comment.
void GlyphAtlas::writeNativeGlyphIntoAtlas (const uint8_t* rgbTexture, juce::Point<int> packPos,
                                            int width, int height) noexcept
{
    juce::Image::BitmapData atlasData (images.at (Type::mono).image, packPos.x, packPos.y,
                                       width, height, juce::Image::BitmapData::writeOnly);

    for (int row { 0 }; row < height; ++row)
    {
        auto* destRow { atlasData.getLinePointer (row) };
        const auto* srcRow { rgbTexture + static_cast<size_t> (row) * static_cast<size_t> (width)
                                         * clearTypeSubpixelBytesPerPixel };

        for (int col { 0 }; col < width; ++col)
        {
            const int r { srcRow[static_cast<size_t> (col) * clearTypeSubpixelBytesPerPixel + 0] };
            const int g { srcRow[static_cast<size_t> (col) * clearTypeSubpixelBytesPerPixel + 1] };
            const int b { srcRow[static_cast<size_t> (col) * clearTypeSubpixelBytesPerPixel + 2] };
            const uint8_t averaged { static_cast<uint8_t> ((r + g + b) / 3) };

            destRow[col] = coverageLut.at (averaged);
        }
    }

    images.at (Type::mono).dirty = true;
}
#endif

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
