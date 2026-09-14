//
// Core file: constructor, destructor, advanceFrame(), getOrRasterize(),
// setRasterization(), and rasterize()'s branchless backend dispatch. FreeType/
// EdgeTable/Native rasterization backends, the software composite helper, the
// LRU cache/packer, and the GPU-resident mirror all live in their own sibling
// TUs — see jam_vulkan.cpp for the full list making up this one class.

#if JUCE_MAC
// ~GlyphAtlas()'s nativeFonts release below (plain C API, no Objective-C syntax
// needed, unlike jam_GlyphAtlas_mac.mm's rasterizeNative() which creates these
// CTFontRef entries). Must be included at global scope (outside namespace jam).
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace jam
{
/*____________________________________________________________________________*/
GlyphAtlas::GlyphAtlas (jam::VulkanDevice& gpuDevice) noexcept
    : device (gpuDevice)
{
    // Slot holds a jam::Array (move-only), so images' own std::initializer_list
    // constructor (which copies every element out of the list) is no longer
    // usable here — each Slot is built locally and moved into place instead.
    Slot monoSlot { juce::Image (juce::Image::SingleChannel, dimension, dimension, true, juce::SoftwareImageType()) };
    Slot emojiSlot { juce::Image (juce::Image::ARGB, dimension, dimension, true, juce::SoftwareImageType()) };

    images.try_emplace (Type::mono, std::move (monoSlot));
    images.try_emplace (Type::emoji, std::move (emojiSlot));

    using BitmapData = juce::Image::BitmapData;

    compositors.add<BitmapData&, const Region&, int&, int&, juce::Colour&, juce::Rectangle<int>&> (Type::mono,
        [this] (BitmapData& targetData, const Region& region, int screenX, int screenY, juce::Colour colour, juce::Rectangle<int> clip)
        {
            compositeMonoImpl (targetData, region, screenX, screenY, colour, clip);
        });

    compositors.add<BitmapData&, const Region&, int&, int&, juce::Colour&, juce::Rectangle<int>&> (Type::emoji,
        [this] (BitmapData& targetData, const Region& region, int screenX, int screenY, juce::Colour colour, juce::Rectangle<int> clip)
        {
            compositeEmojiImpl (targetData, region, screenX, screenY, colour, clip);
        });

    FT_Init_FreeType (&freetypeLibrary);

    // coverageLut starts at the identity mapping — valid immediately, even if
    // setRasterization() is never called (the CPU-fallback path; see backend's
    // own doc comment).
    rebuildCoverageLut (gamma, contrast);
}

void GlyphAtlas::initialise()
{
    // GPU-resident mirror images — device-lifetime invariant once created.
    // Per-window bindless slots are registered separately at
    // VulkanGraphics::create() time (registerGlyphAtlasSlots).
    const bool monoCreated { createAtlasImage (Type::mono, vk::Format::eR8Unorm) };
    jassert (monoCreated);
    juce::ignoreUnused (monoCreated);

    const bool emojiCreated { createAtlasImage (Type::emoji, vk::Format::eB8G8R8A8Unorm) };
    jassert (emojiCreated);
    juce::ignoreUnused (emojiCreated);
}

void GlyphAtlas::shutdown()
{
    gpuImages.clear();

    for (auto& [type, slot] : images)
        slot.dirty = true;
}

GlyphAtlas::~GlyphAtlas()
{
    for (const auto& [typeface, entry] : freetypeFaces)
    {
        if (entry.face != nullptr)
            FT_Done_Face (entry.face);
    }

    FT_Done_FreeType (freetypeLibrary);

#if JUCE_MAC
    // Releases rasterizeNative()'s cached CTFontRef entries (jam_GlyphAtlas_mac.mm) —
    // see NativeFontEntry's doc comment (jam_GlyphAtlas.h).
    for (const auto& [typeface, entry] : nativeFonts)
    {
        if (entry.fontRef != nullptr)
            CFRelease (static_cast<CFTypeRef> (entry.fontRef));
    }
#endif

#if JUCE_WINDOWS
    // Releases rasterizeNative()'s cached IDWriteFontFace entries
    // (jam_GlyphAtlasNative.cpp) before the loader they were built through
    // is unregistered — mirrors the JUCE_MAC block's CTFontRef release above.
    for (const auto& [typeface, entry] : nativeFonts)
    {
        if (entry.fontRef != nullptr)
            static_cast<IDWriteFontFace*> (entry.fontRef)->Release();
    }

    if (directWriteFactory != nullptr and inMemoryFontFileLoader != nullptr)
        directWriteFactory->UnregisterFontFileLoader (inMemoryFontFileLoader);
#endif
}

void GlyphAtlas::advanceFrame() noexcept
{
    ++frameCounter;
}

GlyphAtlas::Region* GlyphAtlas::getOrRasterize (const Key& key) noexcept
{
    if (cache.contains (key))
    {
        auto& entry { cache.at (key) };
        entry.lastUsedFrame = frameCounter;
        return &entry.region;
    }

    return rasterize (key);
}

void GlyphAtlas::setRasterization (map::FontRasterizerBackend::value newBackend, float newGamma, float newContrast) noexcept
{
    const bool changed { newBackend != backend or newGamma != gamma or newContrast != contrast };

    if (changed)
    {
        backend = newBackend;
        gamma = newGamma;
        contrast = newContrast;

        rebuildCoverageLut (gamma, contrast);
        flushCache();
    }
}

void GlyphAtlas::setEmbolden (bool newEmbolden) noexcept
{
    const bool changed { newEmbolden != embolden };

    if (changed)
    {
        embolden = newEmbolden;

#if JUCE_WINDOWS
        // rasterizeNative()'s cached IDWriteFontFace entries (jam_GlyphAtlasNative.cpp)
        // bake DWRITE_FONT_SIMULATIONS_BOLD/NONE in at creation — a stale cached
        // face would keep rendering under the previous embolden state even after
        // flushCache() clears the glyph cache below, so every cached face is
        // released and the cache cleared too, forcing rasterizeNative() to rebuild
        // each face under the new embolden state on next miss.
        for (const auto& [typeface, entry] : nativeFonts)
        {
            if (entry.fontRef != nullptr)
                static_cast<IDWriteFontFace*> (entry.fontRef)->Release();
        }

        nativeFonts.clear();
#endif

        // macOS applies embolden at draw time (jam_GlyphAtlas_mac.mm) — its
        // cached CTFontRef carries no baked simulation, so nativeFonts needs no
        // release/clear here.

        flushCache();
    }
}

GlyphAtlas::Region* GlyphAtlas::rasterize (const Key& key) noexcept
{
    jassert (key.typeface != nullptr);

    juce::Point<int> packPos { -1, -1 };
    const auto bounds { (this->*rasterizeByBackend.at (static_cast<uint8_t> (backend))) (key, packPos) };
    const auto [boundsWidth, boundsHeight] { bounds.size };

    Region* result { nullptr };

    if (boundsWidth > 0 and boundsHeight > 0 and packPos.x >= 0)
        result = insertRasterizedGlyph (key, bounds, packPos, bounds.isEmoji ? Type::emoji : Type::mono);

    return result;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
