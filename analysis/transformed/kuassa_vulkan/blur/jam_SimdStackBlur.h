/** @file
    SIMD (NEON/SSE2) and scalar-fallback stack blur for premultiplied ARGB and
    single-channel juce::Image buffers.
*/

#pragma once

namespace jam
{

/** Rounding offset added before truncating the blurred float sum back to a byte. */
static constexpr float stackBlurRoundOffset { 0.5f };
/** Lower clamp bound for a blurred byte channel. */
static constexpr float stackBlurLowestByte  { 0.0f };
/** Upper clamp bound for a blurred byte channel. */
static constexpr float stackBlurHighestByte { 255.0f };

/** Reciprocal of the triangular kernel's total weight, (radius+1)^2.
    @param radius Blur radius in pixels.
    @return Normalization factor to multiply the accumulated weighted sum by.
*/
static inline float stackBlurNormalization (int radius) noexcept
{
    const float radiusPlusOne { static_cast<float> (radius + 1) };
    return 1.0f / (radiusPlusOne * radiusPlusOne);
}

/** Platform-native 4-lane int32 SIMD vector (NEON or SSE2), one lane per group member. */
#if JAM_SIMD_NEON
using StackBlurVector = int32x4_t;
#elif JAM_SIMD_SSE2
using StackBlurVector = __m128i;
#endif

#if JAM_SIMD_NEON or JAM_SIMD_SSE2

/** Number of rows (horizontal pass) or pixel-columns (vertical pass) processed per SIMD group. */
static constexpr int stackBlurGroupWidth     { 4 };
/** Number of contiguous bytes processed per SIMD group in the vertical pass's byte-column walk. */
static constexpr int stackBlurByteGroupWidth { 16 };

/** One stack-recurrence ring slot for a group of stackBlurGroupWidth (rows in the
    horizontal pass, pixel-columns in the vertical pass) -- one SIMD lane vector
    of int32 channel values per group member.
*/
struct StackBlurHistoryGroup
{
    /** Per-group-member int32-lane channel values for this ring slot. */
    std::array<StackBlurVector, stackBlurGroupWidth> lane;
};

/** Widens a single <=4-byte pixel into an int32-lane vector, one channel per
    lane, zero-extended.
    @param pixel Pointer to the first byte of the pixel's channelCount channels.
    @return SIMD vector with each channel widened into its own int32 lane.
*/
template <int channelCount>
static inline StackBlurVector loadPixelChannels (const uint8_t* pixel) noexcept
{
    uint32_t packed { 0 };
    std::memcpy (&packed, pixel, static_cast<size_t> (channelCount));

#if JAM_SIMD_NEON
    const uint8x8_t bytes { vreinterpret_u8_u32 (vdup_n_u32 (packed)) };
    const uint16x8_t widened { vmovl_u8 (bytes) };
    return vreinterpretq_s32_u32 (vmovl_u16 (vget_low_u16 (widened)));
#elif JAM_SIMD_SSE2
    const __m128i zero { _mm_setzero_si128() };
    const __m128i bytes { _mm_cvtsi32_si128 (static_cast<int> (packed)) };
    const __m128i widened { _mm_unpacklo_epi8 (bytes, zero) };
    return _mm_unpacklo_epi16 (widened, zero);
#endif
}

/** Narrows an int32-lane vector back to a <=4-byte pixel, round + saturate.
    @param pixel Pointer to the first byte to write channelCount channels into.
    @param value SIMD vector holding the int32-lane channel values to narrow.
*/
template <int channelCount>
static inline void storePixelChannels (uint8_t* pixel, StackBlurVector value) noexcept
{
    uint32_t packed { 0 };

#if JAM_SIMD_NEON
    const uint16x4_t narrowed16 { vqmovun_s32 (value) };
    const uint8x8_t narrowed8 { vqmovn_u16 (vcombine_u16 (narrowed16, narrowed16)) };
    vst1_lane_u32 (&packed, vreinterpret_u32_u8 (narrowed8), 0);
#elif JAM_SIMD_SSE2
    const __m128i zero { _mm_setzero_si128() };
    const __m128i narrowed16 { _mm_packs_epi32 (value, zero) };
    const __m128i narrowed8 { _mm_packus_epi16 (narrowed16, zero) };
    packed = static_cast<uint32_t> (_mm_cvtsi128_si32 (narrowed8));
#endif

    std::memcpy (pixel, &packed, static_cast<size_t> (channelCount));
}

/** Widens 16 contiguous bytes into four int32-lane vectors, channel-agnostic
    (used by the vertical pass, which walks raw byte columns).
    @param pixel Pointer to 16 contiguous source bytes.
    @param first Widened vector for byte group 0-3.
    @param second Widened vector for byte group 4-7.
    @param third Widened vector for byte group 8-11.
    @param fourth Widened vector for byte group 12-15.
*/
static inline void loadByteGroupOfFour (const uint8_t* pixel, StackBlurVector& first, StackBlurVector& second, StackBlurVector& third, StackBlurVector& fourth) noexcept
{
#if JAM_SIMD_NEON
    const uint8x16_t raw { vld1q_u8 (pixel) };
    const uint16x8_t lowHalf  { vmovl_u8 (vget_low_u8 (raw)) };
    const uint16x8_t highHalf { vmovl_u8 (vget_high_u8 (raw)) };
    first  = vreinterpretq_s32_u32 (vmovl_u16 (vget_low_u16 (lowHalf)));
    second = vreinterpretq_s32_u32 (vmovl_u16 (vget_high_u16 (lowHalf)));
    third  = vreinterpretq_s32_u32 (vmovl_u16 (vget_low_u16 (highHalf)));
    fourth = vreinterpretq_s32_u32 (vmovl_u16 (vget_high_u16 (highHalf)));
#elif JAM_SIMD_SSE2
    const __m128i zero { _mm_setzero_si128() };
    const __m128i raw { _mm_loadu_si128 (reinterpret_cast<const __m128i*> (pixel)) };
    const __m128i lowHalf  { _mm_unpacklo_epi8 (raw, zero) };
    const __m128i highHalf { _mm_unpackhi_epi8 (raw, zero) };
    first  = _mm_unpacklo_epi16 (lowHalf, zero);
    second = _mm_unpackhi_epi16 (lowHalf, zero);
    third  = _mm_unpacklo_epi16 (highHalf, zero);
    fourth = _mm_unpackhi_epi16 (highHalf, zero);
#endif
}

/** Narrows four int32-lane vectors back to 16 contiguous bytes, round +
    saturate, channel-agnostic.
    @param pixel Pointer to 16 contiguous destination bytes.
    @param first Vector to narrow into byte group 0-3.
    @param second Vector to narrow into byte group 4-7.
    @param third Vector to narrow into byte group 8-11.
    @param fourth Vector to narrow into byte group 12-15.
*/
static inline void storeByteGroupOfFour (uint8_t* pixel, StackBlurVector first, StackBlurVector second, StackBlurVector third, StackBlurVector fourth) noexcept
{
#if JAM_SIMD_NEON
    const uint16x4_t narrowedFirst  { vqmovun_s32 (first) };
    const uint16x4_t narrowedSecond { vqmovun_s32 (second) };
    const uint16x4_t narrowedThird  { vqmovun_s32 (third) };
    const uint16x4_t narrowedFourth { vqmovun_s32 (fourth) };
    const uint8x8_t lowBytes  { vqmovn_u16 (vcombine_u16 (narrowedFirst, narrowedSecond)) };
    const uint8x8_t highBytes { vqmovn_u16 (vcombine_u16 (narrowedThird, narrowedFourth)) };
    vst1q_u8 (pixel, vcombine_u8 (lowBytes, highBytes));
#elif JAM_SIMD_SSE2
    const __m128i lowBytes  { _mm_packs_epi32 (first, second) };
    const __m128i highBytes { _mm_packs_epi32 (third, fourth) };
    _mm_storeu_si128 (reinterpret_cast<__m128i*> (pixel), _mm_packus_epi16 (lowBytes, highBytes));
#endif
}

#endif // JAM_SIMD_NEON or JAM_SIMD_SSE2

/** Sliding-window triangular stack blur along one scan line (row for a
    horizontal pass, column for a vertical pass). channelCount channels of one
    element are blurred independently. This is the scalar fallback used
    wholesale when no SIMD gate is active, and the tail handler for the
    remainder rows/columns left over by the SIMD group passes below.
    @param line Pointer to the first element of the scan line.
    @param elementStep Byte stride between consecutive elements along the line.
    @param lineLength Number of elements along the line.
    @param channelCount Number of channels per element (1-4).
    @param radius Blur radius in elements.
*/
static inline void stackBlurLine (uint8_t* line, int elementStep, int lineLength, int channelCount, int radius) noexcept
{
    jassert (lineLength > 0);
    jassert (channelCount > 0 and channelCount <= 4);
    jassert (radius > 0);

    const int div       { radius * 2 + 1 };
    const int lastIndex { lineLength - 1 };
    const float normalization { stackBlurNormalization (radius) };

    for (int channel { 0 }; channel < channelCount; ++channel)
    {
        std::vector<float> stack (static_cast<size_t> (div));
        float sum { 0.0f };
        float sumIn { 0.0f };
        float sumOut { 0.0f };

        const float firstPixel { static_cast<float> (line[channel]) };

        for (int i { 0 }; i <= radius; ++i)
        {
            stack.at (static_cast<size_t> (i)) = firstPixel;
            sum += firstPixel * static_cast<float> (i + 1);
            sumOut += firstPixel;
        }

        int elementIndex { 0 };

        for (int i { 1 }; i <= radius; ++i)
        {
            if (i <= lastIndex)
                elementIndex += 1;

            const float pixel { static_cast<float> (line[elementIndex * elementStep + channel]) };
            stack.at (static_cast<size_t> (i + radius)) = pixel;
            sum += pixel * static_cast<float> (radius + 1 - i);
            sumIn += pixel;
        }

        int stackPointer { radius };
        int readIndex { std::min (radius, lastIndex) };
        elementIndex = readIndex;

        for (int x { 0 }; x < lineLength; ++x)
        {
            line[x * elementStep + channel] = static_cast<uint8_t> (juce::jlimit (stackBlurLowestByte, stackBlurHighestByte, sum * normalization + stackBlurRoundOffset));

            sum -= sumOut;

            int stackStart { stackPointer + div - radius };
            if (stackStart >= div)
                stackStart -= div;

            sumOut -= stack.at (static_cast<size_t> (stackStart));

            if (readIndex < lastIndex)
            {
                elementIndex += 1;
                readIndex += 1;
            }

            const float pixel { static_cast<float> (line[elementIndex * elementStep + channel]) };
            stack.at (static_cast<size_t> (stackStart)) = pixel;
            sumIn += pixel;
            sum += sumIn;

            stackPointer += 1;
            if (stackPointer >= div)
                stackPointer = 0;

            sumOut += stack.at (static_cast<size_t> (stackPointer));
            sumIn -= stack.at (static_cast<size_t> (stackPointer));
        }
    }
}

/** Blurs data in place along rows, SIMD-grouped stackBlurGroupWidth rows at a
    time with a scalar tail for the remainder.
    @param data Bitmap data to blur in place.
    @param radius Blur radius in pixels.
*/
template <int channelCount>
static inline void stackBlurHorizontalPass (juce::Image::BitmapData& data, int radius) noexcept
{
#if JAM_SIMD_NEON or JAM_SIMD_SSE2
    const int width       { data.width };
    const int height      { data.height };
    const int pixelStride { data.pixelStride };
    const int div         { radius * 2 + 1 };
    const int lastColumn  { width - 1 };

  #if JAM_SIMD_NEON
    const float32x4_t normalization { vdupq_n_f32 (stackBlurNormalization (radius)) };
  #elif JAM_SIMD_SSE2
    const __m128 normalization { _mm_set1_ps (stackBlurNormalization (radius)) };
  #endif

    std::vector<StackBlurHistoryGroup> history (static_cast<size_t> (div));

    int rowGroupStart { 0 };

    for (; rowGroupStart + stackBlurGroupWidth <= height; rowGroupStart += stackBlurGroupWidth)
    {
        std::array<StackBlurVector, stackBlurGroupWidth> sum;
        std::array<StackBlurVector, stackBlurGroupWidth> sumIn;
        std::array<StackBlurVector, stackBlurGroupWidth> sumOut;
        std::array<StackBlurVector, stackBlurGroupWidth> pixel;
        std::array<uint8_t*, stackBlurGroupWidth> row;
        std::array<int, stackBlurGroupWidth> sourceOffset;
        std::array<int, stackBlurGroupWidth> destOffset;

        for (int lane { 0 }; lane < stackBlurGroupWidth; ++lane)
        {
            row.at (lane) = data.getLinePointer (rowGroupStart + lane);
            sourceOffset.at (lane) = 0;

          #if JAM_SIMD_NEON
            sum.at (lane) = vdupq_n_s32 (0);
            sumIn.at (lane) = vdupq_n_s32 (0);
            sumOut.at (lane) = vdupq_n_s32 (0);
          #elif JAM_SIMD_SSE2
            sum.at (lane) = _mm_setzero_si128();
            sumIn.at (lane) = _mm_setzero_si128();
            sumOut.at (lane) = _mm_setzero_si128();
          #endif

            pixel.at (lane) = loadPixelChannels<channelCount> (row.at (lane) + sourceOffset.at (lane));
        }

        for (int i { 0 }; i <= radius; ++i)
        {
            for (int lane { 0 }; lane < stackBlurGroupWidth; ++lane)
            {
                history.at (static_cast<size_t> (i)).lane.at (lane) = pixel.at (lane);

              #if JAM_SIMD_NEON
                const StackBlurVector weight { vdupq_n_s32 (i + 1) };
                sum.at (lane) = vmlaq_s32 (sum.at (lane), pixel.at (lane), weight);
                sumOut.at (lane) = vaddq_s32 (sumOut.at (lane), pixel.at (lane));
              #elif JAM_SIMD_SSE2
                const StackBlurVector weight { _mm_set1_epi32 (i + 1) };
                sum.at (lane) = _mm_add_epi32 (sum.at (lane), _mm_madd_epi16 (pixel.at (lane), weight));
                sumOut.at (lane) = _mm_add_epi32 (sumOut.at (lane), pixel.at (lane));
              #endif
            }
        }

        for (int i { 1 }; i <= radius; ++i)
        {
            for (int lane { 0 }; lane < stackBlurGroupWidth; ++lane)
            {
                if (i <= lastColumn)
                    sourceOffset.at (lane) += pixelStride;

                pixel.at (lane) = loadPixelChannels<channelCount> (row.at (lane) + sourceOffset.at (lane));
                history.at (static_cast<size_t> (i + radius)).lane.at (lane) = pixel.at (lane);

              #if JAM_SIMD_NEON
                const StackBlurVector weight { vdupq_n_s32 (radius + 1 - i) };
                sum.at (lane) = vmlaq_s32 (sum.at (lane), pixel.at (lane), weight);
                sumIn.at (lane) = vaddq_s32 (sumIn.at (lane), pixel.at (lane));
              #elif JAM_SIMD_SSE2
                const StackBlurVector weight { _mm_set1_epi32 (radius + 1 - i) };
                sum.at (lane) = _mm_add_epi32 (sum.at (lane), _mm_madd_epi16 (pixel.at (lane), weight));
                sumIn.at (lane) = _mm_add_epi32 (sumIn.at (lane), pixel.at (lane));
              #endif
            }
        }

        int stackPointer { radius };
        int readIndex { std::min (radius, lastColumn) };

        for (int lane { 0 }; lane < stackBlurGroupWidth; ++lane)
        {
            sourceOffset.at (lane) = pixelStride * readIndex;
            destOffset.at (lane) = 0;
        }

        for (int x { 0 }; x < width; ++x)
        {
            for (int lane { 0 }; lane < stackBlurGroupWidth; ++lane)
            {
              #if JAM_SIMD_NEON
                const float32x4_t scaled { vmulq_f32 (vcvtq_f32_s32 (sum.at (lane)), normalization) };
                storePixelChannels<channelCount> (row.at (lane) + destOffset.at (lane), vcvtaq_s32_f32 (scaled));
              #elif JAM_SIMD_SSE2
                const __m128 scaled { _mm_mul_ps (_mm_cvtepi32_ps (sum.at (lane)), normalization) };
                storePixelChannels<channelCount> (row.at (lane) + destOffset.at (lane), _mm_cvtps_epi32 (scaled));
              #endif

                destOffset.at (lane) += pixelStride;

              #if JAM_SIMD_NEON
                sum.at (lane) = vsubq_s32 (sum.at (lane), sumOut.at (lane));
              #elif JAM_SIMD_SSE2
                sum.at (lane) = _mm_sub_epi32 (sum.at (lane), sumOut.at (lane));
              #endif
            }

            int stackStart { stackPointer + div - radius };
            if (stackStart >= div)
                stackStart -= div;

            if (readIndex < lastColumn)
            {
                for (int lane { 0 }; lane < stackBlurGroupWidth; ++lane)
                    sourceOffset.at (lane) += pixelStride;
                readIndex += 1;
            }

            for (int lane { 0 }; lane < stackBlurGroupWidth; ++lane)
            {
              #if JAM_SIMD_NEON
                sumOut.at (lane) = vsubq_s32 (sumOut.at (lane), history.at (static_cast<size_t> (stackStart)).lane.at (lane));
              #elif JAM_SIMD_SSE2
                sumOut.at (lane) = _mm_sub_epi32 (sumOut.at (lane), history.at (static_cast<size_t> (stackStart)).lane.at (lane));
              #endif

                pixel.at (lane) = loadPixelChannels<channelCount> (row.at (lane) + sourceOffset.at (lane));
                history.at (static_cast<size_t> (stackStart)).lane.at (lane) = pixel.at (lane);

              #if JAM_SIMD_NEON
                sumIn.at (lane) = vaddq_s32 (sumIn.at (lane), pixel.at (lane));
                sum.at (lane) = vaddq_s32 (sum.at (lane), sumIn.at (lane));
              #elif JAM_SIMD_SSE2
                sumIn.at (lane) = _mm_add_epi32 (sumIn.at (lane), pixel.at (lane));
                sum.at (lane) = _mm_add_epi32 (sum.at (lane), sumIn.at (lane));
              #endif
            }

            stackPointer += 1;
            if (stackPointer >= div)
                stackPointer = 0;

            for (int lane { 0 }; lane < stackBlurGroupWidth; ++lane)
            {
                const StackBlurVector atPointer { history.at (static_cast<size_t> (stackPointer)).lane.at (lane) };

              #if JAM_SIMD_NEON
                sumOut.at (lane) = vaddq_s32 (sumOut.at (lane), atPointer);
                sumIn.at (lane) = vsubq_s32 (sumIn.at (lane), atPointer);
              #elif JAM_SIMD_SSE2
                sumOut.at (lane) = _mm_add_epi32 (sumOut.at (lane), atPointer);
                sumIn.at (lane) = _mm_sub_epi32 (sumIn.at (lane), atPointer);
              #endif
            }
        }
    }

    for (int y { rowGroupStart }; y < height; ++y)
        stackBlurLine (data.getLinePointer (y), pixelStride, width, channelCount, radius);

#else
    for (int y { 0 }; y < data.height; ++y)
        stackBlurLine (data.getLinePointer (y), data.pixelStride, data.width, channelCount, radius);
#endif
}

/** Blurs data in place along columns, SIMD-grouped stackBlurByteGroupWidth
    bytes at a time with a scalar tail for the remainder.
    @param data Bitmap data to blur in place.
    @param radius Blur radius in pixels.
*/
template <int channelCount>
static inline void stackBlurVerticalPass (juce::Image::BitmapData& data, int radius) noexcept
{
#if JAM_SIMD_NEON or JAM_SIMD_SSE2
    const int height      { data.height };
    const int lineStride  { data.lineStride };
    const int rowByteWidth { data.width * data.pixelStride };
    const int div         { radius * 2 + 1 };
    const int lastRow     { height - 1 };

  #if JAM_SIMD_NEON
    const float32x4_t normalization { vdupq_n_f32 (stackBlurNormalization (radius)) };
  #elif JAM_SIMD_SSE2
    const __m128 normalization { _mm_set1_ps (stackBlurNormalization (radius)) };
  #endif

    std::vector<StackBlurHistoryGroup> history (static_cast<size_t> (div));

    uint8_t* base { data.getLinePointer (0) };
    int columnGroupStart { 0 };

    for (; columnGroupStart + stackBlurByteGroupWidth <= rowByteWidth; columnGroupStart += stackBlurByteGroupWidth)
    {
        std::array<StackBlurVector, stackBlurGroupWidth> sum;
        std::array<StackBlurVector, stackBlurGroupWidth> sumIn;
        std::array<StackBlurVector, stackBlurGroupWidth> sumOut;
        std::array<StackBlurVector, stackBlurGroupWidth> pixel;

        for (int lane { 0 }; lane < stackBlurGroupWidth; ++lane)
        {
          #if JAM_SIMD_NEON
            sum.at (lane) = vdupq_n_s32 (0);
            sumIn.at (lane) = vdupq_n_s32 (0);
            sumOut.at (lane) = vdupq_n_s32 (0);
          #elif JAM_SIMD_SSE2
            sum.at (lane) = _mm_setzero_si128();
            sumIn.at (lane) = _mm_setzero_si128();
            sumOut.at (lane) = _mm_setzero_si128();
          #endif
        }

        int sourceOffset { columnGroupStart };
        loadByteGroupOfFour (base + sourceOffset, pixel.at (0), pixel.at (1), pixel.at (2), pixel.at (3));

        for (int i { 0 }; i <= radius; ++i)
        {
            for (int lane { 0 }; lane < stackBlurGroupWidth; ++lane)
            {
                history.at (static_cast<size_t> (i)).lane.at (lane) = pixel.at (lane);

              #if JAM_SIMD_NEON
                const StackBlurVector weight { vdupq_n_s32 (i + 1) };
                sum.at (lane) = vmlaq_s32 (sum.at (lane), pixel.at (lane), weight);
                sumOut.at (lane) = vaddq_s32 (sumOut.at (lane), pixel.at (lane));
              #elif JAM_SIMD_SSE2
                const StackBlurVector weight { _mm_set1_epi32 (i + 1) };
                sum.at (lane) = _mm_add_epi32 (sum.at (lane), _mm_madd_epi16 (pixel.at (lane), weight));
                sumOut.at (lane) = _mm_add_epi32 (sumOut.at (lane), pixel.at (lane));
              #endif
            }
        }

        for (int i { 1 }; i <= radius; ++i)
        {
            if (i <= lastRow)
                sourceOffset += lineStride;

            loadByteGroupOfFour (base + sourceOffset, pixel.at (0), pixel.at (1), pixel.at (2), pixel.at (3));

            for (int lane { 0 }; lane < stackBlurGroupWidth; ++lane)
            {
                history.at (static_cast<size_t> (i + radius)).lane.at (lane) = pixel.at (lane);

              #if JAM_SIMD_NEON
                const StackBlurVector weight { vdupq_n_s32 (radius + 1 - i) };
                sum.at (lane) = vmlaq_s32 (sum.at (lane), pixel.at (lane), weight);
                sumIn.at (lane) = vaddq_s32 (sumIn.at (lane), pixel.at (lane));
              #elif JAM_SIMD_SSE2
                const StackBlurVector weight { _mm_set1_epi32 (radius + 1 - i) };
                sum.at (lane) = _mm_add_epi32 (sum.at (lane), _mm_madd_epi16 (pixel.at (lane), weight));
                sumIn.at (lane) = _mm_add_epi32 (sumIn.at (lane), pixel.at (lane));
              #endif
            }
        }

        int stackPointer { radius };
        int readRow { std::min (radius, lastRow) };
        sourceOffset = columnGroupStart + readRow * lineStride;
        int destOffset { columnGroupStart };

        for (int y { 0 }; y < height; ++y)
        {
          #if JAM_SIMD_NEON
            storeByteGroupOfFour (base + destOffset,
                                   vcvtaq_s32_f32 (vmulq_f32 (vcvtq_f32_s32 (sum.at (0)), normalization)),
                                   vcvtaq_s32_f32 (vmulq_f32 (vcvtq_f32_s32 (sum.at (1)), normalization)),
                                   vcvtaq_s32_f32 (vmulq_f32 (vcvtq_f32_s32 (sum.at (2)), normalization)),
                                   vcvtaq_s32_f32 (vmulq_f32 (vcvtq_f32_s32 (sum.at (3)), normalization)));
          #elif JAM_SIMD_SSE2
            storeByteGroupOfFour (base + destOffset,
                                   _mm_cvtps_epi32 (_mm_mul_ps (_mm_cvtepi32_ps (sum.at (0)), normalization)),
                                   _mm_cvtps_epi32 (_mm_mul_ps (_mm_cvtepi32_ps (sum.at (1)), normalization)),
                                   _mm_cvtps_epi32 (_mm_mul_ps (_mm_cvtepi32_ps (sum.at (2)), normalization)),
                                   _mm_cvtps_epi32 (_mm_mul_ps (_mm_cvtepi32_ps (sum.at (3)), normalization)));
          #endif

            destOffset += lineStride;

            for (int lane { 0 }; lane < stackBlurGroupWidth; ++lane)
            {
              #if JAM_SIMD_NEON
                sum.at (lane) = vsubq_s32 (sum.at (lane), sumOut.at (lane));
              #elif JAM_SIMD_SSE2
                sum.at (lane) = _mm_sub_epi32 (sum.at (lane), sumOut.at (lane));
              #endif
            }

            int stackStart { stackPointer + div - radius };
            if (stackStart >= div)
                stackStart -= div;

            if (readRow < lastRow)
            {
                sourceOffset += lineStride;
                readRow += 1;
            }

            loadByteGroupOfFour (base + sourceOffset, pixel.at (0), pixel.at (1), pixel.at (2), pixel.at (3));

            for (int lane { 0 }; lane < stackBlurGroupWidth; ++lane)
            {
              #if JAM_SIMD_NEON
                sumOut.at (lane) = vsubq_s32 (sumOut.at (lane), history.at (static_cast<size_t> (stackStart)).lane.at (lane));
              #elif JAM_SIMD_SSE2
                sumOut.at (lane) = _mm_sub_epi32 (sumOut.at (lane), history.at (static_cast<size_t> (stackStart)).lane.at (lane));
              #endif

                history.at (static_cast<size_t> (stackStart)).lane.at (lane) = pixel.at (lane);

              #if JAM_SIMD_NEON
                sumIn.at (lane) = vaddq_s32 (sumIn.at (lane), pixel.at (lane));
                sum.at (lane) = vaddq_s32 (sum.at (lane), sumIn.at (lane));
              #elif JAM_SIMD_SSE2
                sumIn.at (lane) = _mm_add_epi32 (sumIn.at (lane), pixel.at (lane));
                sum.at (lane) = _mm_add_epi32 (sum.at (lane), sumIn.at (lane));
              #endif
            }

            stackPointer += 1;
            if (stackPointer >= div)
                stackPointer = 0;

            for (int lane { 0 }; lane < stackBlurGroupWidth; ++lane)
            {
                const StackBlurVector atPointer { history.at (static_cast<size_t> (stackPointer)).lane.at (lane) };

              #if JAM_SIMD_NEON
                sumOut.at (lane) = vaddq_s32 (sumOut.at (lane), atPointer);
                sumIn.at (lane) = vsubq_s32 (sumIn.at (lane), atPointer);
              #elif JAM_SIMD_SSE2
                sumOut.at (lane) = _mm_add_epi32 (sumOut.at (lane), atPointer);
                sumIn.at (lane) = _mm_sub_epi32 (sumIn.at (lane), atPointer);
              #endif
            }
        }
    }

    for (int x { columnGroupStart }; x < rowByteWidth; ++x)
        stackBlurLine (base + x, lineStride, height, 1, radius);

#else
    for (int x { 0 }; x < data.width; ++x)
        stackBlurLine (data.getLinePointer (0) + x * data.pixelStride, data.lineStride, data.height, channelCount, radius);
#endif
}

/** In-place stack blur of a premultiplied ARGB image.
    @param data Bitmap data to blur in place; must be juce::Image::ARGB.
    @param radius Blur radius in pixels.
*/
static inline void stackBlurArgb (juce::Image::BitmapData& data, int radius) noexcept
{
    jassert (radius > 0);
    jassert (data.pixelFormat == juce::Image::ARGB);

    stackBlurHorizontalPass<4> (data, radius);
    stackBlurVerticalPass<4> (data, radius);
}

/** In-place stack blur of a single-channel coverage image.
    @param data Bitmap data to blur in place; must be juce::Image::SingleChannel.
    @param radius Blur radius in pixels.
*/
static inline void stackBlurSingleChannel (juce::Image::BitmapData& data, int radius) noexcept
{
    jassert (radius > 0);
    jassert (data.pixelFormat == juce::Image::SingleChannel);

    stackBlurHorizontalPass<1> (data, radius);
    stackBlurVerticalPass<1> (data, radius);
}

} // namespace jam
