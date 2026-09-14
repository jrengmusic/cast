//
// macOS's CGContext extraction (NSGraphicsContext, CTM concat) lives in
// jam_LowLevelGraphicsGlyphRenderer_mac.mm — requires Objective-C syntax,
// mirroring how createMetalLayerForView() (jam_VulkanGraphicsSetupSurface.cpp)
// uses the .mm for its own JUCE_MAC branch. Windows presentation (non-mac
// destructor's #elif JUCE_WINDOWS branch) uses plain Win32 GDI calls
// (GetDC/StretchDIBits/ReleaseDC) — <windows.h> is already pulled in by
// vulkan/vulkan.h (VK_USE_PLATFORM_WIN32_KHR, jam_vulkan.h).

namespace jam
{
/*____________________________________________________________________________*/
#if JUCE_MAC && JUCE_GRAPHICS_INCLUDE_COREGRAPHICS_HELPERS

LowLevelGraphicsGlyphRenderer::LowLevelGraphicsGlyphRenderer (CGContextRef nativeContext, float flipHeight,
                                                               jam::GlyphAtlas& glyphAtlas, float scaleFactor)
    : juce::CoreGraphicsContext (nativeContext, flipHeight)
    , atlas (glyphAtlas)
    , scale (scaleFactor)
{
    // One LowLevelGraphicsGlyphRenderer is constructed per paint frame (mirrors
    // jam::VulkanLowLevelGraphicsContext's identical per-frame construct/destroy
    // lifecycle) — advancing the shared atlas's LRU frame counter once here satisfies
    // its "call once per paint cycle" contract without needing an idempotent gate.
    atlas.advanceFrame();
}

/*____________________________________________________________________________*/
void LowLevelGraphicsGlyphRenderer::setOrigin (juce::Point<int> o)
{
    juce::CoreGraphicsContext::setOrigin (o);

    currentTransform = juce::AffineTransform::translation (static_cast<float> (o.x), static_cast<float> (o.y))
                          .followedBy (currentTransform);
}

/*____________________________________________________________________________*/
void LowLevelGraphicsGlyphRenderer::addTransform (const juce::AffineTransform& t)
{
    juce::CoreGraphicsContext::addTransform (t);

    currentTransform = t.followedBy (currentTransform);
}

/*____________________________________________________________________________*/
void LowLevelGraphicsGlyphRenderer::saveState()
{
    juce::CoreGraphicsContext::saveState();

    stateStack.add ({ currentTransform, currentFillColour });
}

/*____________________________________________________________________________*/
void LowLevelGraphicsGlyphRenderer::restoreState()
{
    juce::CoreGraphicsContext::restoreState();

    if (not stateStack.isEmpty())
    {
        const auto& saved { stateStack.last() };
        currentTransform = saved.transform;
        currentFillColour = saved.fillColour;
        stateStack.remove (stateStack.size() - 1);
    }
}

/*____________________________________________________________________________*/
void LowLevelGraphicsGlyphRenderer::setFill (const juce::FillType& f)
{
    juce::CoreGraphicsContext::setFill (f);

    currentFillColour = f.colour;
}

/*____________________________________________________________________________*/
void LowLevelGraphicsGlyphRenderer::setOpacity (float o)
{
    juce::CoreGraphicsContext::setOpacity (o);

    currentFillColour = currentFillColour.withAlpha (o);
}

/*____________________________________________________________________________*/
// drawGlyphs — atlas-backed glyph rendering at ANY angle, mirroring
// jam::VulkanLowLevelGraphicsContext::drawGlyphs()'s atlas resolution exactly: each
// glyph index is resolved through GlyphAtlas::getOrRasterize() (cache hit, or
// rasterize on miss into the shared CPU-side atlas image). Only the final compositing
// primitive differs from the CPU-raster/Vulkan-quad paths — the resolved region is
// composited through the inherited CoreGraphicsContext verbs instead of a direct pixel
// write or a GPU quad.
//
// composedTransform mirrors jam::VulkanLowLevelGraphicsContext::drawGlyphs()'s
// currentState.currentTransform.getTransformWith(t) exactly: t (this call's explicit
// transform) composed with currentTransform (this context's own accumulated
// setOrigin()/addTransform() history, since the base class's own copy is private and
// unreadable here) — used for the atlas Key's rotation identity only. Placement below
// uses t alone: currentTransform is already permanently baked into the live CGContext's
// own CTM by every prior setOrigin()/addTransform() call (forwarded to the base class),
// so composing it again into the placement transform would apply it twice.
//
// Rotation is part of glyph identity (jam_GlyphAtlas.h's Key doc comment) — the
// rotation angle is extracted once below (identical atan2 expression to
// jam::VulkanLowLevelGraphicsContext::drawGlyphs()) and threaded into every atlas
// key, so GlyphAtlas rasterizes the bitmap already rotated.
//
// Each resolved region is rasterized at physical-pixel size (scale baked into the atlas
// key's fontSize); the live CGContext's own CTM already scales logical points up to
// physical pixels for retina, so the glyph's placement transform scales the region back
// down by 1/scale first — otherwise the already-physical-sized bitmap would render at
// scale x its intended size.
void LowLevelGraphicsGlyphRenderer::drawGlyphs (juce::Span<const uint16_t> glyphs,
                                                juce::Span<const juce::Point<float>> positions,
                                                const juce::AffineTransform& t)
{
    const auto composedTransform { t.followedBy (currentTransform) };
    const float rotation { std::atan2 (composedTransform.mat10, composedTransform.mat00) };

    const auto typeface { getFont().getTypefacePtr() };
    jassert (typeface != nullptr);

    // GlyphAtlas::Key::make() composes fontHeight * scale → DPI-correct atlas key
    // (physical pixel size on Retina/high-DPI), the one SSOT site also used by
    // jam::VulkanLowLevelGraphicsContext::packGlyphQuadType().
    const auto fontHeight { getFont().getHeight() };
    const float composedScale { jam::GlyphAtlas::Key::getScale (composedTransform) };
    const auto invScale { 1.0f / composedScale };

    // hasCellRun gates pendingCellRun's use on its count matching this call's
    // own glyph count — see jam::VulkanLowLevelGraphicsContext::packGlyphQuadType()'s
    // identical guard for the rationale (a stale/mismatched setCellRun() call,
    // or none at all, falls back to the no-cell-run-context Key shape).
    const bool hasCellRun { pendingCellRun.count == static_cast<int> (glyphs.size()) };

    for (size_t i { 0 }; i < glyphs.size(); ++i)
    {
        const char32_t codepoint  { hasCellRun ? pendingCellRun.codepoints[i] : char32_t { 0 } };
        const uint8_t  span       { hasCellRun ? pendingCellRun.spans[i]      : uint8_t { 0 } };
        const int      cellWidth  { hasCellRun ? pendingCellRun.cellWidth     : 0 };
        const int      cellHeight { hasCellRun ? pendingCellRun.cellHeight    : 0 };
        const int      baseline   { hasCellRun ? pendingCellRun.baseline      : 0 };

        const auto key { jam::GlyphAtlas::Key::make (typeface.get(), static_cast<uint32_t> (glyphs[i]), fontHeight, composedScale,
                                                      codepoint, cellWidth, cellHeight, baseline, span, rotation) };
        auto* region { atlas.getOrRasterize (key) };

        if (region != nullptr)
        {
            const auto [regionWidth, regionHeight] { region->size };

            if (regionWidth > 0 and regionHeight > 0)
            {
                const auto atlasDimension { static_cast<float> (atlas.getDimension()) };
                const juce::Rectangle<int> sourceRect {
                    juce::roundToInt (region->textureCoordinates.getX() * atlasDimension),
                    juce::roundToInt (region->textureCoordinates.getY() * atlasDimension),
                    regionWidth,
                    regionHeight
                };

                const auto glyphImage { atlas.getAtlas (region->type).getClippedImage (sourceRect) };

                const auto glyphOrigin { juce::Point<float> (
                    positions[i].x + static_cast<float> (region->bearing.x) * invScale,
                    positions[i].y - static_cast<float> (region->bearing.y) * invScale) };

                const auto transformPlacingGlyph {
                    juce::AffineTransform::scale (invScale).translated (glyphOrigin).followedBy (t)
                };

                if (region->type == jam::GlyphAtlas::Type::mono)
                {
                    saveState();
                    clipToImageAlpha (glyphImage, transformPlacingGlyph);
                    setFill (juce::FillType { currentFillColour });
                    fillRect (getClipBounds().toFloat());
                    restoreState();
                }
                else
                {
                    drawImage (glyphImage, transformPlacingGlyph);
                }
            }
        }
    }

    pendingCellRun = {}; // consumed — cleared so a following plain drawGlyphs() call never reuses stale run state
}

#elif not JUCE_MAC

/*____________________________________________________________________________*/
LowLevelGraphicsGlyphRenderer::LowLevelGraphicsGlyphRenderer (const juce::Image& target,
                                                               jam::GlyphAtlas& glyphAtlas,
                                                               juce::ComponentPeer& peer,
                                                               float scaleFactor)
    : juce::LowLevelGraphicsSoftwareRenderer (target)
    , targetImage (target)
    , atlas (glyphAtlas)
    , nativeHandle (peer.getNativeHandle())
    , scale (scaleFactor)
    , currentClip (target.getBounds())
{
    // One LowLevelGraphicsGlyphRenderer is constructed per paint frame (mirrors
    // jam::VulkanLowLevelGraphicsContext's identical per-frame construct/destroy
    // lifecycle) — advancing the shared atlas's LRU frame counter once here satisfies
    // its "call once per paint cycle" contract without needing an idempotent gate.
    atlas.advanceFrame();

    if (scaleFactor != 1.0f)
        addTransform (juce::AffineTransform::scale (scaleFactor));
}

/*____________________________________________________________________________*/
// Presents the rendered target image to the native window — required because
// returning a non-null context from externalContextFactory means JUCE performs zero
// presentation of its own (see file header doc comment). Mirrors
// jam::VulkanLowLevelGraphicsContext's destructor calling Graphics::endFrame() at
// the same point in its lifecycle.
LowLevelGraphicsGlyphRenderer::~LowLevelGraphicsGlyphRenderer()
{
#if JUCE_WINDOWS
    auto* windowHandle { static_cast<HWND> (nativeHandle) };
    auto* deviceContext { GetDC (windowHandle) };

    const juce::Image::BitmapData bitmapData { targetImage, juce::Image::BitmapData::readOnly };

    BITMAPINFO bitmapInfo {};
    bitmapInfo.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = bitmapData.width;
    bitmapInfo.bmiHeader.biHeight = -bitmapData.height; // negative = top-down DIB, matches juce::Image's top-down row layout
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32; // juce::Image::ARGB is 32bpp BGRA in memory — matches Windows' 32bpp DIB byte order exactly, no per-pixel conversion needed
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    StretchDIBits (deviceContext,
                  0, 0, bitmapData.width, bitmapData.height,
                  0, 0, bitmapData.width, bitmapData.height,
                  bitmapData.data,
                  &bitmapInfo,
                  DIB_RGB_COLORS,
                  SRCCOPY);

    ReleaseDC (windowHandle, deviceContext);
#endif
}

/*____________________________________________________________________________*/
void LowLevelGraphicsGlyphRenderer::setOrigin (juce::Point<int> o)
{
    juce::LowLevelGraphicsSoftwareRenderer::setOrigin (o);

    currentTransform = juce::AffineTransform::translation (static_cast<float> (o.x), static_cast<float> (o.y))
                          .followedBy (currentTransform);
}

/*____________________________________________________________________________*/
void LowLevelGraphicsGlyphRenderer::addTransform (const juce::AffineTransform& t)
{
    juce::LowLevelGraphicsSoftwareRenderer::addTransform (t);

    currentTransform = t.followedBy (currentTransform);
}

/*____________________________________________________________________________*/
void LowLevelGraphicsGlyphRenderer::saveState()
{
    juce::LowLevelGraphicsSoftwareRenderer::saveState();

    stateStack.add ({ currentTransform, currentFillColour, currentClip });
}

/*____________________________________________________________________________*/
void LowLevelGraphicsGlyphRenderer::restoreState()
{
    juce::LowLevelGraphicsSoftwareRenderer::restoreState();

    if (not stateStack.isEmpty())
    {
        const auto& saved { stateStack.last() };
        currentTransform = saved.transform;
        currentFillColour = saved.fillColour;
        currentClip = saved.clip;
        stateStack.remove (stateStack.size() - 1);
    }
}

/*____________________________________________________________________________*/
void LowLevelGraphicsGlyphRenderer::setFill (const juce::FillType& f)
{
    juce::LowLevelGraphicsSoftwareRenderer::setFill (f);

    currentFillColour = f.colour;
}

/*____________________________________________________________________________*/
void LowLevelGraphicsGlyphRenderer::setOpacity (float o)
{
    juce::LowLevelGraphicsSoftwareRenderer::setOpacity (o);

    currentFillColour = currentFillColour.withAlpha (o);
}

/*____________________________________________________________________________*/
bool LowLevelGraphicsGlyphRenderer::clipToRectangle (const juce::Rectangle<int>& r)
{
    const bool result { juce::LowLevelGraphicsSoftwareRenderer::clipToRectangle (r) };

    currentClip = currentClip.getIntersection (r.toFloat().transformedBy (currentTransform).getSmallestIntegerContainer());

    return result;
}

/*____________________________________________________________________________*/
bool LowLevelGraphicsGlyphRenderer::clipToRectangleList (const juce::RectangleList<int>& r)
{
    const bool result { juce::LowLevelGraphicsSoftwareRenderer::clipToRectangleList (r) };

    currentClip = currentClip.getIntersection (r.getBounds().toFloat().transformedBy (currentTransform).getSmallestIntegerContainer());

    return result;
}

/*____________________________________________________________________________*/
void LowLevelGraphicsGlyphRenderer::drawImage (const juce::Image& imageToDraw, const juce::AffineTransform& transform)
{
    const auto composedTransform { transform.followedBy (currentTransform) };

    if (composedTransform.isOnlyTranslationOrScale())
    {
        const juce::Image::BitmapData sourceData { imageToDraw, juce::Image::BitmapData::readOnly };
        juce::Image::BitmapData destData { targetImage, juce::Image::BitmapData::readWrite };

        const auto sourceRect { imageToDraw.getBounds() };
        const auto destRect { sourceRect.toFloat().transformedBy (composedTransform).getSmallestIntegerContainer() };

        jam::Graphics::drawScaledImage (sourceData, sourceRect, destData, destRect, currentClip);
    }
    else
    {
        juce::LowLevelGraphicsSoftwareRenderer::drawImage (imageToDraw, transform);
    }
}

/*____________________________________________________________________________*/
// drawGlyphs — atlas-backed glyph rendering at ANY angle, mirroring
// jam::VulkanLowLevelGraphicsContext::drawGlyphs()'s atlas resolution exactly: each
// glyph index is resolved through GlyphAtlas::getOrRasterize() (cache hit, or
// rasterize on miss into the shared CPU-side atlas image), then composited directly
// onto the target image via GlyphAtlas::composite() — no GPU/Vulkan involved, no
// separate upload step (the CPU composite reads straight from the atlas's own
// juce::Image, never its GPU-resident mirror).
//
// composedTransform mirrors jam::VulkanLowLevelGraphicsContext::drawGlyphs()'s
// currentState.currentTransform.getTransformWith(t) exactly: t (this call's explicit
// transform) composed with currentTransform (this context's own accumulated
// setOrigin()/addTransform() history, since the base class's own copy is private and
// unreadable here).
//
// Rotation is part of glyph identity (jam_GlyphAtlas.h's Key doc comment) — rather
// than delegating a rotated composedTransform to JUCE's own native
// juce::LowLevelGraphicsSoftwareRenderer::drawGlyphs(), the rotation angle is
// extracted once below (identical atan2 expression to
// jam::VulkanLowLevelGraphicsContext::drawGlyphs()) and threaded into every atlas
// key, so GlyphAtlas rasterizes the bitmap already rotated.
void LowLevelGraphicsGlyphRenderer::drawGlyphs (juce::Span<const uint16_t> glyphs,
                                                juce::Span<const juce::Point<float>> positions,
                                                const juce::AffineTransform& t)
{
    const auto composedTransform { t.followedBy (currentTransform) };
    const float rotation { std::atan2 (composedTransform.mat10, composedTransform.mat00) };

    const auto typeface { getFont().getTypefacePtr() };
    jassert (typeface != nullptr);

    // GlyphAtlas::Key::make() composes fontHeight * scale → DPI-correct atlas key
    // (physical pixel size on Retina/high-DPI), the one SSOT site also used by
    // jam::VulkanLowLevelGraphicsContext::packGlyphQuadType().
    const auto fontHeight { getFont().getHeight() };
    const float composedScale { jam::GlyphAtlas::Key::getScale (composedTransform) };

    // hasCellRun gates pendingCellRun's use on its count matching this call's
    // own glyph count — see jam::VulkanLowLevelGraphicsContext::packGlyphQuadType()'s
    // identical guard for the rationale (a stale/mismatched setCellRun() call,
    // or none at all, falls back to the no-cell-run-context Key shape).
    const bool hasCellRun { pendingCellRun.count == static_cast<int> (glyphs.size()) };

    juce::Image::BitmapData targetData { targetImage, juce::Image::BitmapData::readWrite };

    for (size_t i { 0 }; i < glyphs.size(); ++i)
    {
        const char32_t codepoint  { hasCellRun ? pendingCellRun.codepoints[i] : char32_t { 0 } };
        const uint8_t  span       { hasCellRun ? pendingCellRun.spans[i]      : uint8_t { 0 } };
        const int      cellWidth  { hasCellRun ? pendingCellRun.cellWidth     : 0 };
        const int      cellHeight { hasCellRun ? pendingCellRun.cellHeight    : 0 };
        const int      baseline   { hasCellRun ? pendingCellRun.baseline      : 0 };

        const auto key { jam::GlyphAtlas::Key::make (typeface.get(), static_cast<uint32_t> (glyphs[i]), fontHeight, composedScale,
                                                      codepoint, cellWidth, cellHeight, baseline, span, rotation) };
        auto* region { atlas.getOrRasterize (key) };

        if (region != nullptr)
        {
            const auto [regionWidth, regionHeight] { region->size };

            if (regionWidth > 0 and regionHeight > 0)
            {
                const auto devicePosition { positions[i].transformedBy (composedTransform) };
                const int screenX { juce::roundToInt (devicePosition.x) + region->bearing.x };
                const int screenY { juce::roundToInt (devicePosition.y) - region->bearing.y };

                atlas.composite (region->type, targetData, *region, screenX, screenY, currentFillColour, currentClip);
            }
        }
    }

    pendingCellRun = {}; // consumed — cleared so a following plain drawGlyphs() call never reuses stale run state
}

#endif

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
