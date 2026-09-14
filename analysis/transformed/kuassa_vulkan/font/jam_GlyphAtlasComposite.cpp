//
// Composite file: compositeMonoImpl(), compositeEmojiImpl(), computeDestBlend(),
// compositeRows(), and composite() — the software (non-GPU) SIMD glyph-to-target
// blend path, dispatched per Type via the compositors map registered in the
// constructor (jam_GlyphAtlasCore.cpp).

namespace jam
{
/*____________________________________________________________________________*/
void GlyphAtlas::compositeMonoImpl (juce::Image::BitmapData& targetData, const Region& region,
                                    int screenX, int screenY, juce::Colour colour, juce::Rectangle<int> clip) noexcept
{
    using BitmapData = juce::Image::BitmapData;

    BitmapData atlasData (images.at (Type::mono).image, BitmapData::readOnly);

    const int atlasX { juce::roundToInt (region.textureCoordinates.getX() * static_cast<float> (dimension)) };
    const int atlasY { juce::roundToInt (region.textureCoordinates.getY() * static_cast<float> (dimension)) };

    const uint32_t fgPacked {
        (static_cast<uint32_t> (colour.getAlpha()) << 24u)
        | (static_cast<uint32_t> (colour.getRed())   << 16u)
        | (static_cast<uint32_t> (colour.getGreen()) << 8u)
        | static_cast<uint32_t> (colour.getBlue())
    };

    compositeRows (region, screenX, screenY, targetData, clip,
        [&] (uint32_t* dest, int row, int col)
        {
            const auto* srcRow { atlasData.getLinePointer (atlasY + row) + atlasX };
            jam::Graphics::blendMonoTinted (dest, srcRow + col, fgPacked);
        },
        [&] (uint32_t* destRow, int destX, int row, int col)
        {
            const auto* srcRow { atlasData.getLinePointer (atlasY + row) + atlasX };
            const uint32_t a { srcRow[col] };

            if (a > 0)
            {
                const uint32_t fgA { (fgPacked >> 24u) & 0xFFu };
                const uint32_t fgR { (fgPacked >> 16u) & 0xFFu };
                const uint32_t fgG { (fgPacked >> 8u)  & 0xFFu };
                const uint32_t fgB { fgPacked           & 0xFFu };

                const uint32_t sA { (fgA * a + 128u) >> 8u };
                const uint32_t sR { (fgR * a + 128u) >> 8u };
                const uint32_t sG { (fgG * a + 128u) >> 8u };
                const uint32_t sB { (fgB * a + 128u) >> 8u };

                const uint32_t d     { destRow[destX] };
                const uint32_t invA  { 255u - a };
                const auto blend     { computeDestBlend (d, invA) };

                destRow[destX] =
                    ((sA + blend.a) << 24u)
                    | ((sR + blend.r) << 16u)
                    | ((sG + blend.g) << 8u)
                    |  (sB + blend.b);
            }
        });
}

void GlyphAtlas::compositeEmojiImpl (juce::Image::BitmapData& targetData, const Region& region,
                                     int screenX, int screenY, juce::Colour /*colour*/, juce::Rectangle<int> clip) noexcept
{
    using BitmapData = juce::Image::BitmapData;

    BitmapData atlasData (images.at (Type::emoji).image, BitmapData::readOnly);

    const int atlasX { juce::roundToInt (region.textureCoordinates.getX() * static_cast<float> (dimension)) };
    const int atlasY { juce::roundToInt (region.textureCoordinates.getY() * static_cast<float> (dimension)) };

    compositeRows (region, screenX, screenY, targetData, clip,
        [&] (uint32_t* dest, int row, int col)
        {
            const auto* srcRow { reinterpret_cast<const uint32_t*> (atlasData.getLinePointer (atlasY + row)) + atlasX };
            jam::Graphics::blendSourceOver (dest, srcRow + col);
        },
        [&] (uint32_t* destRow, int destX, int row, int col)
        {
            const auto* srcRow { reinterpret_cast<const uint32_t*> (atlasData.getLinePointer (atlasY + row)) + atlasX };
            const uint32_t s        { srcRow[col] };
            const uint32_t srcAlpha { (s >> 24u) & 0xFFu };

            if (srcAlpha > 0)
            {
                const uint32_t d     { destRow[destX] };
                const uint32_t invA  { 255u - srcAlpha };
                const auto blend     { computeDestBlend (d, invA) };

                destRow[destX] =
                    (((s >> 24u) & 0xFFu) + blend.a) << 24u
                    | ((((s >> 16u) & 0xFFu) + blend.r) << 16u)
                    | ((((s >> 8u)  & 0xFFu) + blend.g) << 8u)
                    |  ((s          & 0xFFu) + blend.b);
            }
        });
}

GlyphAtlas::DestBlend GlyphAtlas::computeDestBlend (uint32_t destPixel, uint32_t invAlpha) noexcept
{
    return DestBlend
    {
        (((destPixel >> 24u) & 0xFFu) * invAlpha + 128u) >> 8u,
        (((destPixel >> 16u) & 0xFFu) * invAlpha + 128u) >> 8u,
        (((destPixel >> 8u)  & 0xFFu) * invAlpha + 128u) >> 8u,
        ((destPixel          & 0xFFu) * invAlpha + 128u) >> 8u
    };
}

template <typename SimdBlend, typename ScalarBlend>
void GlyphAtlas::compositeRows (const Region& region, int screenX, int screenY,
                                 juce::Image::BitmapData& targetData, juce::Rectangle<int> clip,
                                 SimdBlend&& simdBlend, ScalarBlend&& scalarBlend) noexcept
{
    const auto [regionWidth, regionHeight] { region.size };

    for (int row { 0 }; row < regionHeight; ++row)
    {
        const int destY { screenY + row };

        if (destY < 0 or destY >= targetData.height or destY < clip.getY() or destY >= clip.getBottom())
            continue;

        auto* destRow { reinterpret_cast<uint32_t*> (targetData.getLinePointer (destY)) };
        const int startCol { juce::jmax (0, clip.getX() - screenX) };
        int col { startCol };
        const int endCol { juce::jmin (regionWidth, targetData.width - screenX, clip.getRight() - screenX) };

        for (; col + 3 < endCol; col += 4)
        {
            const int destX { screenX + col };

            if (destX >= 0)
                simdBlend (destRow + destX, row, col);
        }

        for (; col < endCol; ++col)
        {
            const int destX { screenX + col };

            if (destX >= 0 and destX < targetData.width)
                scalarBlend (destRow, destX, row, col);
        }
    }
}

void GlyphAtlas::composite (Type type, juce::Image::BitmapData& targetData, const Region& region,
                            int screenX, int screenY, juce::Colour colour, juce::Rectangle<int> clip) noexcept
{
    compositors.get (type, targetData, region, screenX, screenY, colour, clip);
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
