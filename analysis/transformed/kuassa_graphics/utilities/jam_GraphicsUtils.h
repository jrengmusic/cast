/**
 * @file jam_GraphicsUtils.h
 * @brief Domain surface for cross-module image operations.
 *        Implementations are the three-arm SIMD kernels
 *        (jam_SimdBlend.h) — SSE2/NEON/scalar, byte-exact.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/** @brief Domain surface for cross-module image operations.
 *  Thin forwarding wrappers over the three-arm SIMD kernels in jam_SimdBlend.h
 *  (SSE2/NEON/scalar, byte-exact). */
struct Graphics
{
    Graphics() = delete;

    /** @brief Multiplies every pixel's alpha in @p destData by the corresponding
     *  (bilinearly-mapped) alpha of @p maskData.
     *  @param destData  Locked destination BitmapData (readWrite).
     *  @param maskData  Locked mask BitmapData (readOnly); scaled to destData dimensions. */
    static void applyAlphaMask (juce::Image::BitmapData& destData, const juce::Image::BitmapData& maskData) noexcept
    {
        jam::applyAlphaMask (destData, maskData);
    }

    /** @brief Morphologically erodes the alpha channel of @p data by @p radius pixels
     *  (two-pass sliding-window minimum, horizontal then vertical).
     *  @param data    Locked BitmapData (readWrite).
     *  @param radius  Erosion radius in pixels — no-op when zero. */
    static void applyMatteChoke (juce::Image::BitmapData& data, int radius) noexcept { jam::applyMatteChoke (data, radius); }

    /** @brief Distance-field feathers the alpha channel of @p data by @p radius pixels —
     *  each pixel's alpha is scaled by its Chebyshev distance to the nearest transparent
     *  neighbour within the window, mapped through @p curve.
     *  @param data    Locked BitmapData (readWrite).
     *  @param radius  Feather radius in pixels — no-op when zero.
     *  @param curve   Exponent applied to the distance-to-alpha mapping (jam::Value::map()). */
    static void applyMatteFeather (juce::Image::BitmapData& data, float radius, float curve) noexcept { jam::applyMatteFeather (data, radius, curve); }

    /** @brief Copies @p source into @p destination (reallocating if needed), then
     *  multiplies each pixel's alpha by the bilinearly-mapped alpha of @p mask.
     *  @param destination  Output image; reallocated when invalid or mismatched in size/format.
     *  @param source       Source image to composite.
     *  @param mask         Alpha-mask image; its alpha channel is scaled to source dimensions. */
    static void applyMatte (juce::Image& destination, const juce::Image& source, const juce::Image& mask) noexcept
    {
        const int w { source.getWidth() };
        const int h { source.getHeight() };

        if (not destination.isValid() or destination.getWidth() != w or destination.getHeight() != h
            or destination.getFormat() != source.getFormat())
            destination = juce::Image (source.getFormat(), w, h, true, juce::SoftwareImageType());

        {
            const juce::Image::BitmapData srcData (source, juce::Image::BitmapData::readOnly);
            juce::Image::BitmapData dstData (destination, juce::Image::BitmapData::writeOnly);

            for (int y { 0 }; y < h; ++y)
                std::memcpy (dstData.getLinePointer (y),
                             srcData.getLinePointer (y),
                             static_cast<size_t> (w) * static_cast<size_t> (srcData.pixelStride));
        }

        {
            juce::Image::BitmapData dstData (destination, juce::Image::BitmapData::readWrite);
            const juce::Image::BitmapData maskData (mask, juce::Image::BitmapData::readOnly);
            applyAlphaMask (dstData, maskData);
        }
    }

    /** @brief Tints 4 mono coverage bytes with @p premulFgColor and blends the result
     *  over @p dest (premultiplied src-over, 4-pixel wide).
     *  @param dest          Pointer to 4 destination ARGB pixels (read-write).
     *  @param alpha         Pointer to 4 mono coverage bytes (read-only).
     *  @param premulFgColor Premultiplied ARGB foreground colour. */
    static void blendMonoTinted (uint32_t* dest, const uint8_t* alpha, uint32_t premulFgColor) noexcept
    {
        jam::blendMonoTinted (dest, alpha, premulFgColor);
    }

    /** @brief Premultiplied src-over blend of 4 ARGB pixels.
     *  @param dest  Pointer to 4 destination ARGB pixels (read-write).
     *  @param src   Pointer to 4 source premultiplied ARGB pixels (read-only). */
    static void blendSourceOver (uint32_t* dest, const uint32_t* src) noexcept { jam::blendSourceOver (dest, src); }

    /** @brief Bilinear scaled premultiplied src-over blit — resamples @p sourceRect of
     *  @p sourceData into @p destRect of @p destData, clipped to device-space @p clip.
     *  @param sourceData  Locked source BitmapData (readOnly).
     *  @param sourceRect  Source sub-rectangle in source-image space.
     *  @param destData    Locked destination BitmapData (readWrite).
     *  @param destRect    Destination rectangle in device space.
     *  @param clip        Device-space clip rectangle; rows/columns outside it are skipped. */
    static void drawScaledImage (const juce::Image::BitmapData& sourceData,
                                 juce::Rectangle<int> sourceRect,
                                 juce::Image::BitmapData& destData,
                                 juce::Rectangle<int> destRect,
                                 juce::Rectangle<int> clip) noexcept
    {
        jam::drawScaledImage (sourceData, sourceRect, destData, destRect, clip);
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
