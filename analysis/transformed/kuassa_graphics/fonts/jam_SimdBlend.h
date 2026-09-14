/**
 * @file jam_SimdBlend.h
 * @brief SIMD (NEON/SSE2, scalar fallback) premultiplied-ARGB pixel blending
 *        and bilinear scaled blit primitives.
 */

#pragma once

namespace jam
{

static constexpr uint32_t alphaShift { 24u  };
static constexpr uint32_t redShift   { 16u  };
static constexpr uint32_t greenShift { 8u   };
static constexpr uint32_t byteMask   { 0xFFu };

/** @brief Premultiplied src-over blend of 4 ARGB pixels.
 *  @param dest  Pointer to 4 destination ARGB pixels (read-write).
 *  @param src   Pointer to 4 source premultiplied ARGB pixels (read-only). */
static inline void blendSourceOver (uint32_t* dest, const uint32_t* src) noexcept
{
#if JAM_SIMD_SSE2
    // Load 4 src and 4 dest pixels
    __m128i vSrc  = _mm_loadu_si128 (reinterpret_cast<const __m128i*> (src));
    __m128i vDest = _mm_loadu_si128 (reinterpret_cast<const __m128i*> (dest));
    __m128i zero  = _mm_setzero_si128();

    // Unpack low 2 pixels src
    __m128i srcLo  = _mm_unpacklo_epi8 (vSrc, zero);
    __m128i destLo = _mm_unpacklo_epi8 (vDest, zero);

    // Unpack high 2 pixels src
    __m128i srcHi  = _mm_unpackhi_epi8 (vSrc, zero);
    __m128i destHi = _mm_unpackhi_epi8 (vDest, zero);

    // Extract alpha from src lo (byte 3 of each 16-bit-expanded pixel pair)
    // srcLo layout: [B0 G0 R0 A0 B1 G1 R1 A1] each as 16-bit
    __m128i alphaLo = _mm_shufflelo_epi16 (_mm_shufflehi_epi16 (srcLo, _MM_SHUFFLE (3, 3, 3, 3)),
                                            _MM_SHUFFLE (3, 3, 3, 3));
    __m128i alphaHi = _mm_shufflelo_epi16 (_mm_shufflehi_epi16 (srcHi, _MM_SHUFFLE (3, 3, 3, 3)),
                                            _MM_SHUFFLE (3, 3, 3, 3));

    // invAlpha = 255 - alpha
    __m128i full    = _mm_set1_epi16 (255);
    __m128i invALo  = _mm_sub_epi16 (full, alphaLo);
    __m128i invAHi  = _mm_sub_epi16 (full, alphaHi);

    // dest * invAlpha + 128, then >> 8
    __m128i blendLo = _mm_srli_epi16 (_mm_add_epi16 (_mm_mullo_epi16 (destLo, invALo),
                                                      _mm_set1_epi16 (128)), 8);
    __m128i blendHi = _mm_srli_epi16 (_mm_add_epi16 (_mm_mullo_epi16 (destHi, invAHi),
                                                      _mm_set1_epi16 (128)), 8);

    // result = src + blended_dest, pack back to 8-bit
    __m128i resultLo = _mm_add_epi16 (srcLo, blendLo);
    __m128i resultHi = _mm_add_epi16 (srcHi, blendHi);
    __m128i result   = _mm_packus_epi16 (resultLo, resultHi);

    _mm_storeu_si128 (reinterpret_cast<__m128i*> (dest), result);

#elif JAM_SIMD_NEON
    uint8x16_t vSrc  = vld1q_u8 (reinterpret_cast<const uint8_t*> (src));
    uint8x16_t vDest = vld1q_u8 (reinterpret_cast<const uint8_t*> (dest));

    // Extract alpha channel (byte 3 of each ARGB pixel = index 3,7,11,15)
    uint8x8_t alphaBytes = { vSrc[3], vSrc[3], vSrc[3], vSrc[3],
                              vSrc[7], vSrc[7], vSrc[7], vSrc[7] };
    uint8x8_t alphaBytesHi = { vSrc[11], vSrc[11], vSrc[11], vSrc[11],
                                vSrc[15], vSrc[15], vSrc[15], vSrc[15] };

    uint8x8_t invALo = vsub_u8 (vdup_n_u8 (255), alphaBytes);
    uint8x8_t invAHi = vsub_u8 (vdup_n_u8 (255), alphaBytesHi);

    uint8x8_t destLo = vget_low_u8 (vDest);
    uint8x8_t destHi = vget_high_u8 (vDest);

    // dest * invAlpha >> 8 (using vrshrn for round-shift)
    uint16x8_t mulLo = vmull_u8 (destLo, invALo);
    uint16x8_t mulHi = vmull_u8 (destHi, invAHi);
    uint8x8_t blendLo = vrshrn_n_u16 (mulLo, 8);
    uint8x8_t blendHi = vrshrn_n_u16 (mulHi, 8);

    uint8x8_t srcLo = vget_low_u8 (vSrc);
    uint8x8_t srcHi = vget_high_u8 (vSrc);

    uint8x8_t resLo = vqadd_u8 (srcLo, blendLo);
    uint8x8_t resHi = vqadd_u8 (srcHi, blendHi);

    vst1q_u8 (reinterpret_cast<uint8_t*> (dest), vcombine_u8 (resLo, resHi));

#else
    for (int i { 0 }; i < 4; ++i)
    {
        const uint32_t s     { src[i] };
        const uint32_t sA    { (s >> alphaShift) & byteMask };
        const uint32_t invA  { jam::Value::opaque - sA };
        const uint32_t d     { dest[i] };

        const uint32_t dA { (d >> alphaShift) & byteMask };
        const uint32_t dR { (d >> redShift)   & byteMask };
        const uint32_t dG { (d >> greenShift) & byteMask };
        const uint32_t dB { d                  & byteMask };

        const uint32_t rA { sA + ((dA * invA + 128u) >> 8u) };
        const uint32_t rR { ((s >> redShift) & byteMask) + ((dR * invA + 128u) >> 8u) };
        const uint32_t rG { ((s >> greenShift) & byteMask) + ((dG * invA + 128u) >> 8u) };
        const uint32_t rB { (s & byteMask) + ((dB * invA + 128u) >> 8u) };

        dest[i] = (rA << alphaShift) | (rR << redShift) | (rG << greenShift) | rB;
    }
#endif
}

/** @brief Tint mono atlas alpha with fg color and blend over destination — 4 pixels.
 *  @param dest          Pointer to 4 destination ARGB pixels (read-write).
 *  @param alpha         Pointer to 4 mono coverage bytes (read-only).
 *  @param premulFgColor Premultiplied ARGB foreground colour. */
static inline void blendMonoTinted (uint32_t* dest, const uint8_t* alpha, uint32_t premulFgColor) noexcept
{
#if JAM_SIMD_SSE2
    // Expand premulFgColor to 16-bit lanes: [B G R A B G R A] x2
    __m128i zero  = _mm_setzero_si128();
    __m128i vFg   = _mm_set1_epi32 (static_cast<int> (premulFgColor));
    __m128i fgLo  = _mm_unpacklo_epi8 (vFg, zero); // 2 pixels of fg, 16-bit channels
    __m128i fgHi  = _mm_unpackhi_epi8 (vFg, zero);

    // Load 4 alpha bytes, expand to 16-bit
    __m128i vAlpha = _mm_set_epi16 (0, 0, 0, 0,
                                    static_cast<short> (alpha[3]),
                                    static_cast<short> (alpha[2]),
                                    static_cast<short> (alpha[1]),
                                    static_cast<short> (alpha[0]));

    // Broadcast alpha per pixel into all channels (lo = pixels 0,1)
    __m128i aLo = _mm_shufflelo_epi16 (_mm_shufflehi_epi16 (vAlpha, _MM_SHUFFLE (1, 1, 1, 1)),
                                        _MM_SHUFFLE (0, 0, 0, 0));
    __m128i aHi = _mm_set_epi16 (
        static_cast<short> (alpha[3]), static_cast<short> (alpha[3]),
        static_cast<short> (alpha[3]), static_cast<short> (alpha[3]),
        static_cast<short> (alpha[2]), static_cast<short> (alpha[2]),
        static_cast<short> (alpha[2]), static_cast<short> (alpha[2]));

    // src = fg * coverage / 255  (~fg * coverage >> 8 for speed)
    __m128i srcLo = _mm_srli_epi16 (_mm_add_epi16 (_mm_mullo_epi16 (fgLo, aLo),
                                                    _mm_set1_epi16 (128)), 8);
    __m128i srcHi = _mm_srli_epi16 (_mm_add_epi16 (_mm_mullo_epi16 (fgHi, aHi),
                                                    _mm_set1_epi16 (128)), 8);

    // Load dest, unpack
    __m128i vDest  = _mm_loadu_si128 (reinterpret_cast<const __m128i*> (dest));
    __m128i destLo = _mm_unpacklo_epi8 (vDest, zero);
    __m128i destHi = _mm_unpackhi_epi8 (vDest, zero);

    // invAlpha from coverage
    __m128i full   = _mm_set1_epi16 (255);
    __m128i invALo = _mm_sub_epi16 (full, aLo);
    __m128i invAHi = _mm_sub_epi16 (full, aHi);

    __m128i bLo = _mm_srli_epi16 (_mm_add_epi16 (_mm_mullo_epi16 (destLo, invALo),
                                                  _mm_set1_epi16 (128)), 8);
    __m128i bHi = _mm_srli_epi16 (_mm_add_epi16 (_mm_mullo_epi16 (destHi, invAHi),
                                                  _mm_set1_epi16 (128)), 8);

    __m128i resLo = _mm_add_epi16 (srcLo, bLo);
    __m128i resHi = _mm_add_epi16 (srcHi, bHi);
    __m128i result = _mm_packus_epi16 (resLo, resHi);

    _mm_storeu_si128 (reinterpret_cast<__m128i*> (dest), result);

#elif JAM_SIMD_NEON
    // Expand fg to 4 pixels
    uint8x8_t fgBytes = vreinterpret_u8_u32 (vdup_n_u32 (premulFgColor));

    uint8x8_t aBytes = { alpha[0], alpha[0], alpha[0], alpha[0],
                         alpha[1], alpha[1], alpha[1], alpha[1] };
    uint8x8_t aBytesHi = { alpha[2], alpha[2], alpha[2], alpha[2],
                            alpha[3], alpha[3], alpha[3], alpha[3] };

    // src = fg * alpha >> 8
    uint16x8_t srcMulLo = vmull_u8 (fgBytes, aBytes);
    uint16x8_t srcMulHi = vmull_u8 (fgBytes, aBytesHi);
    uint8x8_t srcLo = vrshrn_n_u16 (srcMulLo, 8);
    uint8x8_t srcHi = vrshrn_n_u16 (srcMulHi, 8);

    // invAlpha = 255 - coverage
    uint8x8_t invALo = vsub_u8 (vdup_n_u8 (255), aBytes);
    uint8x8_t invAHi = vsub_u8 (vdup_n_u8 (255), aBytesHi);

    uint8x16_t vDest  = vld1q_u8 (reinterpret_cast<const uint8_t*> (dest));
    uint8x8_t destLo  = vget_low_u8 (vDest);
    uint8x8_t destHi  = vget_high_u8 (vDest);

    uint16x8_t blendMulLo = vmull_u8 (destLo, invALo);
    uint16x8_t blendMulHi = vmull_u8 (destHi, invAHi);
    uint8x8_t blendLo = vrshrn_n_u16 (blendMulLo, 8);
    uint8x8_t blendHi = vrshrn_n_u16 (blendMulHi, 8);

    uint8x8_t resLo = vqadd_u8 (srcLo, blendLo);
    uint8x8_t resHi = vqadd_u8 (srcHi, blendHi);

    vst1q_u8 (reinterpret_cast<uint8_t*> (dest), vcombine_u8 (resLo, resHi));

#else
    const uint32_t fgR { (premulFgColor >> redShift)   & byteMask };
    const uint32_t fgG { (premulFgColor >> greenShift) & byteMask };
    const uint32_t fgB { premulFgColor                  & byteMask };

    for (int i { 0 }; i < 4; ++i)
    {
        const uint32_t a    { alpha[i] };
        const uint32_t invA { jam::Value::opaque - a };

        const uint32_t sR { (fgR * a + 128u) >> 8u };
        const uint32_t sG { (fgG * a + 128u) >> 8u };
        const uint32_t sB { (fgB * a + 128u) >> 8u };

        const uint32_t d { dest[i] };
        const uint32_t dA { (d >> alphaShift) & byteMask };
        const uint32_t dR { (d >> redShift)   & byteMask };
        const uint32_t dG { (d >> greenShift) & byteMask };
        const uint32_t dB { d                  & byteMask };

        const uint32_t rA { a + ((dA * invA + 128u) >> 8u) };
        const uint32_t rR { sR + ((dR * invA + 128u) >> 8u) };
        const uint32_t rG { sG + ((dG * invA + 128u) >> 8u) };
        const uint32_t rB { sB + ((dB * invA + 128u) >> 8u) };

        dest[i] = (rA << alphaShift) | (rR << redShift) | (rG << greenShift) | rB;
    }
#endif
}

/** @brief Write 4 identical opaque pixels to dest.
 *  @param dest   Pointer to 4 destination ARGB pixels (write-only).
 *  @param color  ARGB color value to fill. */
static inline void fillOpaque (uint32_t* dest, uint32_t color) noexcept
{
#if JAM_SIMD_SSE2
    _mm_storeu_si128 (reinterpret_cast<__m128i*> (dest),
                      _mm_set1_epi32 (static_cast<int> (color)));
#elif JAM_SIMD_NEON
    vst1q_u32 (dest, vdupq_n_u32 (color));
#else
    dest[0] = color;
    dest[1] = color;
    dest[2] = color;
    dest[3] = color;
#endif
}

/** @brief One destination column's horizontal source-sample pair + blend
 *  weight, precomputed by buildColumnSamples() for every column of a scaled
 *  blit's destination width. */
struct ColumnSample
{
    int x0;     ///< Left source column index (clamped to [0, sourceWidth - 1]).
    int x1;     ///< Right source column index (clamped to [0, sourceWidth - 1]).
    int weight; ///< Horizontal blend weight toward x1, [0, 255] (see lerp()).
};

/** @brief One destination row's vertical source-sample pair + blend weight,
 *  computed by buildRowSample() for a single destination row of a scaled
 *  blit. */
struct RowSample
{
    int y0;     ///< Top source row index (clamped to [0, sourceHeight - 1]).
    int y1;     ///< Bottom source row index (clamped to [0, sourceHeight - 1]).
    int weight; ///< Vertical blend weight toward y1, [0, 255] (see lerp()).
};

/** @brief Precomputed per-destination-column sample table plus the source
 *  height and destination dimensions drawScaledImage() iterates against —
 *  built once per blit and reused across every destination row. */
struct ScaledBlitContext
{
    jam::Array<ColumnSample> columns; ///< One entry per destination column, from buildColumnSamples().
    int sourceHeight; ///< Source rectangle height, physical pixels.
    int destWidth;    ///< Destination rectangle width, physical pixels.
    int destHeight;   ///< Destination rectangle height, physical pixels.
};

/** @brief Builds the per-destination-column horizontal sample table for a
 *  scaled blit — one ColumnSample per column in [0, destWidth), each mapping
 *  the destination pixel back to its source pixel pair + blend weight via
 *  jam::Value::map()'s pixel-center convention (destination pixel center
 *  maps into source space, then a -0.5 corner shift locates the left sample
 *  before flooring).
 *  @param sourceWidth  Source rectangle width, physical pixels.
 *  @param destWidth    Destination rectangle width, physical pixels.
 *  @return             One ColumnSample per destination column, [0, destWidth). */
static inline jam::Array<ColumnSample> buildColumnSamples (int sourceWidth, int destWidth)
{
    jam::Array<ColumnSample> samples;
    samples.resize (destWidth);

    const double destWidthF { static_cast<double> (destWidth) };
    const double sourceWidthF { static_cast<double> (sourceWidth) };

    for (int x { 0 }; x < destWidth; ++x)
    {
        const double sourceXf { jam::Value::map (static_cast<double> (x) + 0.5, 0.0, destWidthF, 0.0, sourceWidthF) - 0.5 };
        const int flooredX { static_cast<int> (std::floor (sourceXf)) };
        const double fraction { sourceXf - static_cast<double> (flooredX) };
        const int weight { jam::Value::map (fraction, 0.0, 1.0, 0, 255) };

        // x < destWidth == samples.size() (resized above) — in-bounds by loop bound.
        samples[x] = {
            jam::Value::clip (flooredX, 0, sourceWidth - 1),
            jam::Value::clip (flooredX + 1, 0, sourceWidth - 1),
            weight
        };
    }

    return samples;
}

/** @brief Builds the vertical source-sample pair + blend weight for one
 *  destination row of a scaled blit — same pixel-center mapping convention
 *  as buildColumnSamples(), computed for a single @p destY instead of every
 *  column.
 *  @param sourceHeight  Source rectangle height, physical pixels.
 *  @param destHeight    Destination rectangle height, physical pixels.
 *  @param destY         Destination row index, [0, destHeight).
 *  @return              The row's source sample pair + blend weight. */
static inline RowSample buildRowSample (int sourceHeight, int destHeight, int destY)
{
    const double destHeightF { static_cast<double> (destHeight) };
    const double sourceHeightF { static_cast<double> (sourceHeight) };
    const double sourceYf { jam::Value::map (static_cast<double> (destY) + 0.5, 0.0, destHeightF, 0.0, sourceHeightF) - 0.5 };
    const int flooredY { static_cast<int> (std::floor (sourceYf)) };
    const double fraction { sourceYf - static_cast<double> (flooredY) };
    const int weight { jam::Value::map (fraction, 0.0, 1.0, 0, 255) };

    return {
        jam::Value::clip (flooredY, 0, sourceHeight - 1),
        jam::Value::clip (flooredY + 1, 0, sourceHeight - 1),
        weight
    };
}

/** @brief Horizontal weighted blend of two runs of 4 premultiplied ARGB
 *  pixels, one blend weight per lane.
 *
 *  The SSE2, NEON, and scalar arms are byte-exact with each other and with
 *  lerp() applied per channel: each 8-bit channel blends as
 *  `(a * (255 - weight) + b * weight + 128) / 256`, the same rounded
 *  fixed-point formula lerp() defines (its `opaque + 1` divisor folds to
 *  the same right-shift-by-8 every SIMD arm below performs directly).
 *  @param a       4 packed ARGB pixels at weight 0 (read-only).
 *  @param b       4 packed ARGB pixels at weight 255 (read-only).
 *  @param weight  4 per-lane blend weights, [0, 255], parallel to @p a / @p b.
 *  @param out     4 packed ARGB pixels, blended result (write-only). */
static inline void interpolateColumns (const uint32_t* a, const uint32_t* b, const int* weight, uint32_t* out) noexcept
{
#if JAM_SIMD_SSE2
    const __m128i vA { _mm_loadu_si128 (reinterpret_cast<const __m128i*> (a)) };
    const __m128i vB { _mm_loadu_si128 (reinterpret_cast<const __m128i*> (b)) };
    const __m128i zero { _mm_setzero_si128() };

    const __m128i aLo { _mm_unpacklo_epi8 (vA, zero) };
    const __m128i aHi { _mm_unpackhi_epi8 (vA, zero) };
    const __m128i bLo { _mm_unpacklo_epi8 (vB, zero) };
    const __m128i bHi { _mm_unpackhi_epi8 (vB, zero) };

    const __m128i wLo { _mm_set_epi16 (
        static_cast<short> (weight[1]), static_cast<short> (weight[1]),
        static_cast<short> (weight[1]), static_cast<short> (weight[1]),
        static_cast<short> (weight[0]), static_cast<short> (weight[0]),
        static_cast<short> (weight[0]), static_cast<short> (weight[0])) };
    const __m128i wHi { _mm_set_epi16 (
        static_cast<short> (weight[3]), static_cast<short> (weight[3]),
        static_cast<short> (weight[3]), static_cast<short> (weight[3]),
        static_cast<short> (weight[2]), static_cast<short> (weight[2]),
        static_cast<short> (weight[2]), static_cast<short> (weight[2])) };

    const __m128i full { _mm_set1_epi16 (255) };
    const __m128i invWLo { _mm_sub_epi16 (full, wLo) };
    const __m128i invWHi { _mm_sub_epi16 (full, wHi) };
    const __m128i rounding { _mm_set1_epi16 (128) };

    const __m128i sumLo { _mm_add_epi16 (_mm_add_epi16 (_mm_mullo_epi16 (aLo, invWLo), _mm_mullo_epi16 (bLo, wLo)), rounding) };
    const __m128i sumHi { _mm_add_epi16 (_mm_add_epi16 (_mm_mullo_epi16 (aHi, invWHi), _mm_mullo_epi16 (bHi, wHi)), rounding) };

    const __m128i result { _mm_packus_epi16 (_mm_srli_epi16 (sumLo, 8), _mm_srli_epi16 (sumHi, 8)) };
    _mm_storeu_si128 (reinterpret_cast<__m128i*> (out), result);

#elif JAM_SIMD_NEON
    const uint8x16_t vA { vld1q_u8 (reinterpret_cast<const uint8_t*> (a)) };
    const uint8x16_t vB { vld1q_u8 (reinterpret_cast<const uint8_t*> (b)) };

    const uint8x8_t aLo { vget_low_u8 (vA) };
    const uint8x8_t aHi { vget_high_u8 (vA) };
    const uint8x8_t bLo { vget_low_u8 (vB) };
    const uint8x8_t bHi { vget_high_u8 (vB) };

    const uint8x8_t wLo { static_cast<uint8_t> (weight[0]), static_cast<uint8_t> (weight[0]),
                           static_cast<uint8_t> (weight[0]), static_cast<uint8_t> (weight[0]),
                           static_cast<uint8_t> (weight[1]), static_cast<uint8_t> (weight[1]),
                           static_cast<uint8_t> (weight[1]), static_cast<uint8_t> (weight[1]) };
    const uint8x8_t wHi { static_cast<uint8_t> (weight[2]), static_cast<uint8_t> (weight[2]),
                           static_cast<uint8_t> (weight[2]), static_cast<uint8_t> (weight[2]),
                           static_cast<uint8_t> (weight[3]), static_cast<uint8_t> (weight[3]),
                           static_cast<uint8_t> (weight[3]), static_cast<uint8_t> (weight[3]) };

    const uint8x8_t invWLo { vsub_u8 (vdup_n_u8 (255), wLo) };
    const uint8x8_t invWHi { vsub_u8 (vdup_n_u8 (255), wHi) };

    const uint16x8_t sumLo { vmlal_u8 (vmull_u8 (aLo, invWLo), bLo, wLo) };
    const uint16x8_t sumHi { vmlal_u8 (vmull_u8 (aHi, invWHi), bHi, wHi) };

    const uint8x8_t resultLo { vrshrn_n_u16 (sumLo, 8) };
    const uint8x8_t resultHi { vrshrn_n_u16 (sumHi, 8) };

    vst1q_u8 (reinterpret_cast<uint8_t*> (out), vcombine_u8 (resultLo, resultHi));

#else
    for (int i { 0 }; i < 4; ++i)
    {
        const uint32_t pa { a[i] };
        const uint32_t pb { b[i] };
        const int w { weight[i] };

        const uint32_t rA { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ((pa >> alphaShift) & byteMask), static_cast<int> ((pb >> alphaShift) & byteMask), w)) };
        const uint32_t rR { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ((pa >> redShift)   & byteMask), static_cast<int> ((pb >> redShift)   & byteMask), w)) };
        const uint32_t rG { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ((pa >> greenShift) & byteMask), static_cast<int> ((pb >> greenShift) & byteMask), w)) };
        const uint32_t rB { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ( pa                & byteMask), static_cast<int> ( pb                & byteMask), w)) };

        out[i] = (rA << alphaShift) | (rR << redShift) | (rG << greenShift) | rB;
    }
#endif
}

/** @brief Vertical weighted blend of two runs of 4 premultiplied ARGB
 *  pixels, one shared weight applied to all 4 lanes.
 *
 *  Same byte-exact SSE2/NEON/scalar contract as interpolateColumns() — the only
 *  difference is a single broadcast weight instead of one weight per lane.
 *  @param top     4 packed ARGB pixels at weight 0 (read-only).
 *  @param bottom  4 packed ARGB pixels at weight 255 (read-only).
 *  @param weight  Blend weight applied to all 4 lanes, [0, 255].
 *  @param out     4 packed ARGB pixels, blended result (write-only). */
static inline void interpolateRows (const uint32_t* top, const uint32_t* bottom, int weight, uint32_t* out) noexcept
{
#if JAM_SIMD_SSE2
    const __m128i vTop { _mm_loadu_si128 (reinterpret_cast<const __m128i*> (top)) };
    const __m128i vBottom { _mm_loadu_si128 (reinterpret_cast<const __m128i*> (bottom)) };
    const __m128i zero { _mm_setzero_si128() };

    const __m128i topLo { _mm_unpacklo_epi8 (vTop, zero) };
    const __m128i topHi { _mm_unpackhi_epi8 (vTop, zero) };
    const __m128i bottomLo { _mm_unpacklo_epi8 (vBottom, zero) };
    const __m128i bottomHi { _mm_unpackhi_epi8 (vBottom, zero) };

    const __m128i w { _mm_set1_epi16 (static_cast<short> (weight)) };
    const __m128i invW { _mm_set1_epi16 (static_cast<short> (255 - weight)) };
    const __m128i rounding { _mm_set1_epi16 (128) };

    const __m128i sumLo { _mm_add_epi16 (_mm_add_epi16 (_mm_mullo_epi16 (topLo, invW), _mm_mullo_epi16 (bottomLo, w)), rounding) };
    const __m128i sumHi { _mm_add_epi16 (_mm_add_epi16 (_mm_mullo_epi16 (topHi, invW), _mm_mullo_epi16 (bottomHi, w)), rounding) };

    const __m128i result { _mm_packus_epi16 (_mm_srli_epi16 (sumLo, 8), _mm_srli_epi16 (sumHi, 8)) };
    _mm_storeu_si128 (reinterpret_cast<__m128i*> (out), result);

#elif JAM_SIMD_NEON
    const uint8x16_t vTop { vld1q_u8 (reinterpret_cast<const uint8_t*> (top)) };
    const uint8x16_t vBottom { vld1q_u8 (reinterpret_cast<const uint8_t*> (bottom)) };

    const uint8x8_t topLo { vget_low_u8 (vTop) };
    const uint8x8_t topHi { vget_high_u8 (vTop) };
    const uint8x8_t bottomLo { vget_low_u8 (vBottom) };
    const uint8x8_t bottomHi { vget_high_u8 (vBottom) };

    const uint8x8_t w { vdup_n_u8 (static_cast<uint8_t> (weight)) };
    const uint8x8_t invW { vdup_n_u8 (static_cast<uint8_t> (255 - weight)) };

    const uint16x8_t sumLo { vmlal_u8 (vmull_u8 (topLo, invW), bottomLo, w) };
    const uint16x8_t sumHi { vmlal_u8 (vmull_u8 (topHi, invW), bottomHi, w) };

    const uint8x8_t resultLo { vrshrn_n_u16 (sumLo, 8) };
    const uint8x8_t resultHi { vrshrn_n_u16 (sumHi, 8) };

    vst1q_u8 (reinterpret_cast<uint8_t*> (out), vcombine_u8 (resultLo, resultHi));

#else
    for (int i { 0 }; i < 4; ++i)
    {
        const uint32_t pt { top[i] };
        const uint32_t pb { bottom[i] };

        const uint32_t rA { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ((pt >> alphaShift) & byteMask), static_cast<int> ((pb >> alphaShift) & byteMask), weight)) };
        const uint32_t rR { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ((pt >> redShift)   & byteMask), static_cast<int> ((pb >> redShift)   & byteMask), weight)) };
        const uint32_t rG { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ((pt >> greenShift) & byteMask), static_cast<int> ((pb >> greenShift) & byteMask), weight)) };
        const uint32_t rB { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ( pt                & byteMask), static_cast<int> ( pb                & byteMask), weight)) };

        out[i] = (rA << alphaShift) | (rR << redShift) | (rG << greenShift) | rB;
    }
#endif
}

/** @brief Bilinear-samples 4 consecutive destination pixels from a 2x2
 *  source neighborhood per lane (via @p columns' per-lane x0/x1/weight),
 *  blends horizontally then vertically (interpolateColumns()/interpolateRows()),
 *  and composites the result onto @p destRow with premultiplied src-over
 *  (blendSourceOver()).
 *  @param srcRow0    Source row at rowSample.y0, already offset to the
 *                     source rectangle's own X origin.
 *  @param srcRow1    Source row at rowSample.y1, already offset to the
 *                     source rectangle's own X origin.
 *  @param columns    Per-destination-column sample table (buildColumnSamples()).
 *  @param col        Index of the first of 4 consecutive columns in @p columns.
 *  @param rowSample  This destination row's vertical sample pair + weight.
 *  @param destRow    Destination row pointer, device-space.
 *  @param destX      Destination X of the first of the 4 pixels, device-space. */
static inline void composite (const uint32_t* srcRow0, const uint32_t* srcRow1,
                              const jam::Array<ColumnSample>& columns, int col,
                              const RowSample& rowSample, uint32_t* destRow, int destX) noexcept
{
    uint32_t p00[4];
    uint32_t p01[4];
    uint32_t p10[4];
    uint32_t p11[4];
    int weight[4];

    for (int lane { 0 }; lane < 4; ++lane)
    {
        // col + lane < endCol <= context.destWidth == columns.size() (caller's loop bound).
        const ColumnSample& column { columns[col + lane] };

        p00[lane] = srcRow0[column.x0];
        p01[lane] = srcRow0[column.x1];
        p10[lane] = srcRow1[column.x0];
        p11[lane] = srcRow1[column.x1];
        weight[lane] = column.weight;
    }

    uint32_t top[4];
    uint32_t bottom[4];
    uint32_t interpolated[4];

    interpolateColumns (p00, p01, weight, top);
    interpolateColumns (p10, p11, weight, bottom);
    interpolateRows (top, bottom, rowSample.weight, interpolated);

    blendSourceOver (destRow + destX, interpolated);
}

/** @brief Scalar (single-pixel) counterpart to composite() — same 2x2
 *  bilinear sample plus premultiplied src-over composite, for the
 *  not-a-multiple-of-4 remainder of a scaled blit row.
 *  @param srcRow0    Source row at rowSample.y0, already offset to the
 *                     source rectangle's own X origin.
 *  @param srcRow1    Source row at rowSample.y1, already offset to the
 *                     source rectangle's own X origin.
 *  @param columns    Per-destination-column sample table (buildColumnSamples()).
 *  @param col        Index into @p columns for this single destination pixel.
 *  @param rowSample  This destination row's vertical sample pair + weight.
 *  @param destRow    Destination row pointer, device-space.
 *  @param destX      Destination X of this pixel, device-space. */
static inline void compositePixel (const uint32_t* srcRow0, const uint32_t* srcRow1,
                                   const jam::Array<ColumnSample>& columns, int col,
                                   const RowSample& rowSample, uint32_t* destRow, int destX) noexcept
{
    // col < endCol <= context.destWidth == columns.size() (caller's loop bound).
    const ColumnSample& column { columns[col] };

    const uint32_t p00 { srcRow0[column.x0] };
    const uint32_t p01 { srcRow0[column.x1] };
    const uint32_t p10 { srcRow1[column.x0] };
    const uint32_t p11 { srcRow1[column.x1] };

    const int colWeight { column.weight };
    const int rowWeight { rowSample.weight };

    const uint32_t topA { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ((p00 >> alphaShift) & byteMask), static_cast<int> ((p01 >> alphaShift) & byteMask), colWeight)) };
    const uint32_t topR { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ((p00 >> redShift)   & byteMask), static_cast<int> ((p01 >> redShift)   & byteMask), colWeight)) };
    const uint32_t topG { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ((p00 >> greenShift) & byteMask), static_cast<int> ((p01 >> greenShift) & byteMask), colWeight)) };
    const uint32_t topB { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ( p00                & byteMask), static_cast<int> ( p01                & byteMask), colWeight)) };

    const uint32_t bottomA { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ((p10 >> alphaShift) & byteMask), static_cast<int> ((p11 >> alphaShift) & byteMask), colWeight)) };
    const uint32_t bottomR { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ((p10 >> redShift)   & byteMask), static_cast<int> ((p11 >> redShift)   & byteMask), colWeight)) };
    const uint32_t bottomG { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ((p10 >> greenShift) & byteMask), static_cast<int> ((p11 >> greenShift) & byteMask), colWeight)) };
    const uint32_t bottomB { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> ( p10                & byteMask), static_cast<int> ( p11                & byteMask), colWeight)) };

    const uint32_t srcA { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> (topA), static_cast<int> (bottomA), rowWeight)) };
    const uint32_t srcR { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> (topR), static_cast<int> (bottomR), rowWeight)) };
    const uint32_t srcG { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> (topG), static_cast<int> (bottomG), rowWeight)) };
    const uint32_t srcB { static_cast<uint32_t> (jam::Value::lerp (static_cast<int> (topB), static_cast<int> (bottomB), rowWeight)) };

    const uint32_t destPixel   { destRow[destX] };
    const uint32_t invSrcAlpha { jam::Value::opaque - srcA };

    const uint32_t destA { (destPixel >> alphaShift) & byteMask };
    const uint32_t destR { (destPixel >> redShift)   & byteMask };
    const uint32_t destG { (destPixel >> greenShift) & byteMask };
    const uint32_t destB { destPixel                 & byteMask };

    const uint32_t outA { srcA + ((destA * invSrcAlpha + 128u) >> 8u) };
    const uint32_t outR { srcR + ((destR * invSrcAlpha + 128u) >> 8u) };
    const uint32_t outG { srcG + ((destG * invSrcAlpha + 128u) >> 8u) };
    const uint32_t outB { srcB + ((destB * invSrcAlpha + 128u) >> 8u) };

    destRow[destX] = (outA << alphaShift) | (outR << redShift) | (outG << greenShift) | outB;
}

/** @brief Bilinear scaled premultiplied src-over blit — resamples
 *  @p sourceRect of @p sourceData into @p destRect of @p destData, clipped
 *  to device-space @p clip.
 *
 *  Single-threaded: iterates destination rows, builds each row's RowSample
 *  via buildRowSample(), then dispatches 4-wide SIMD batches
 *  (composite()) across the clipped column range, with the
 *  not-a-multiple-of-4 remainder handled by compositePixel().
 *  @param sourceData  Locked source BitmapData (readOnly).
 *  @param sourceRect  Source sub-rectangle to sample, source-image-space.
 *  @param destData    Locked destination BitmapData (readWrite).
 *  @param destRect    Destination rectangle to fill, device-space.
 *  @param clip        Device-space clip rectangle; rows/columns outside it
 *                     are skipped. */
static inline void drawScaledImage (const juce::Image::BitmapData& sourceData, juce::Rectangle<int> sourceRect,
                              juce::Image::BitmapData& destData, juce::Rectangle<int> destRect,
                              juce::Rectangle<int> clip) noexcept
{
    const ScaledBlitContext context {
        buildColumnSamples (sourceRect.getWidth(), destRect.getWidth()),
        sourceRect.getHeight(),
        destRect.getWidth(),
        destRect.getHeight()
    };

    for (int row { 0 }; row < context.destHeight; ++row)
    {
        const int destY { destRect.getY() + row };

        if (destY >= 0 and destY < destData.height and destY >= clip.getY() and destY < clip.getBottom())
        {
            const RowSample rowSample { buildRowSample (context.sourceHeight, context.destHeight, row) };

            const auto* srcRow0 { reinterpret_cast<const uint32_t*> (sourceData.getLinePointer (sourceRect.getY() + rowSample.y0)) + sourceRect.getX() };
            const auto* srcRow1 { reinterpret_cast<const uint32_t*> (sourceData.getLinePointer (sourceRect.getY() + rowSample.y1)) + sourceRect.getX() };
            auto* destRow { reinterpret_cast<uint32_t*> (destData.getLinePointer (destY)) };

            const int startCol { juce::jmax (0, clip.getX() - destRect.getX()) };
            int col { startCol };
            const int endCol { juce::jmin (context.destWidth, destData.width - destRect.getX(), clip.getRight() - destRect.getX()) };

            for (; col + 3 < endCol; col += 4)
            {
                const int destX { destRect.getX() + col };

                if (destX >= 0)
                {
                    composite (srcRow0, srcRow1, context.columns, col, rowSample, destRow, destX);
                }
            }

            for (; col < endCol; ++col)
            {
                const int destX { destRect.getX() + col };

                if (destX >= 0 and destX < destData.width)
                {
                    compositePixel (srcRow0, srcRow1, context.columns, col, rowSample, destRow, destX);
                }
            }
        }
    }
}

static inline void multiplyAlpha (uint32_t* dest, const uint8_t* maskAlpha) noexcept
{
#if JAM_SIMD_SSE2
    const __m128i vDest { _mm_loadu_si128 (reinterpret_cast<const __m128i*> (dest)) };
    const __m128i zero { _mm_setzero_si128() };

    const __m128i destLo { _mm_unpacklo_epi8 (vDest, zero) };
    const __m128i destHi { _mm_unpackhi_epi8 (vDest, zero) };

    const __m128i aLo { _mm_set_epi16 (
        static_cast<short> (maskAlpha[1]), static_cast<short> (maskAlpha[1]),
        static_cast<short> (maskAlpha[1]), static_cast<short> (maskAlpha[1]),
        static_cast<short> (maskAlpha[0]), static_cast<short> (maskAlpha[0]),
        static_cast<short> (maskAlpha[0]), static_cast<short> (maskAlpha[0])) };
    const __m128i aHi { _mm_set_epi16 (
        static_cast<short> (maskAlpha[3]), static_cast<short> (maskAlpha[3]),
        static_cast<short> (maskAlpha[3]), static_cast<short> (maskAlpha[3]),
        static_cast<short> (maskAlpha[2]), static_cast<short> (maskAlpha[2]),
        static_cast<short> (maskAlpha[2]), static_cast<short> (maskAlpha[2])) };

    const __m128i rounding { _mm_set1_epi16 (128) };
    const __m128i resultLo { _mm_srli_epi16 (_mm_add_epi16 (_mm_mullo_epi16 (destLo, aLo), rounding), 8) };
    const __m128i resultHi { _mm_srli_epi16 (_mm_add_epi16 (_mm_mullo_epi16 (destHi, aHi), rounding), 8) };

    const __m128i result { _mm_packus_epi16 (resultLo, resultHi) };
    _mm_storeu_si128 (reinterpret_cast<__m128i*> (dest), result);

#elif JAM_SIMD_NEON
    const uint8x16_t vDest { vld1q_u8 (reinterpret_cast<const uint8_t*> (dest)) };
    const uint8x8_t destLo { vget_low_u8 (vDest) };
    const uint8x8_t destHi { vget_high_u8 (vDest) };

    const uint8x8_t aLo { maskAlpha[0], maskAlpha[0], maskAlpha[0], maskAlpha[0],
                           maskAlpha[1], maskAlpha[1], maskAlpha[1], maskAlpha[1] };
    const uint8x8_t aHi { maskAlpha[2], maskAlpha[2], maskAlpha[2], maskAlpha[2],
                           maskAlpha[3], maskAlpha[3], maskAlpha[3], maskAlpha[3] };

    const uint16x8_t mulLo { vmull_u8 (destLo, aLo) };
    const uint16x8_t mulHi { vmull_u8 (destHi, aHi) };

    const uint8x8_t resultLo { vrshrn_n_u16 (mulLo, 8) };
    const uint8x8_t resultHi { vrshrn_n_u16 (mulHi, 8) };

    vst1q_u8 (reinterpret_cast<uint8_t*> (dest), vcombine_u8 (resultLo, resultHi));

#else
    for (int i { 0 }; i < 4; ++i)
    {
        const uint32_t d  { dest[i] };
        const uint32_t mA { maskAlpha[i] };

        const uint32_t dA { (d >> alphaShift) & byteMask };
        const uint32_t dR { (d >> redShift)   & byteMask };
        const uint32_t dG { (d >> greenShift) & byteMask };
        const uint32_t dB { d                  & byteMask };

        const uint32_t rA { (dA * mA + 128u) >> 8u };
        const uint32_t rR { (dR * mA + 128u) >> 8u };
        const uint32_t rG { (dG * mA + 128u) >> 8u };
        const uint32_t rB { (dB * mA + 128u) >> 8u };

        dest[i] = (rA << alphaShift) | (rR << redShift) | (rG << greenShift) | rB;
    }
#endif
}

static inline void multiplyAlphaPixel (uint32_t* dest, uint8_t maskAlpha) noexcept
{
    const uint32_t d { *dest };

    const uint32_t dA { (d >> alphaShift) & byteMask };
    const uint32_t dR { (d >> redShift)   & byteMask };
    const uint32_t dG { (d >> greenShift) & byteMask };
    const uint32_t dB { d                  & byteMask };

    const uint32_t rA { (dA * maskAlpha + 128u) >> 8u };
    const uint32_t rR { (dR * maskAlpha + 128u) >> 8u };
    const uint32_t rG { (dG * maskAlpha + 128u) >> 8u };
    const uint32_t rB { (dB * maskAlpha + 128u) >> 8u };

    *dest = (rA << alphaShift) | (rR << redShift) | (rG << greenShift) | rB;
}

static inline void applyAlphaMask (juce::Image::BitmapData& destData, const juce::Image::BitmapData& maskData) noexcept
{
    jam::Array<int> maskColumn;
    maskColumn.resize (destData.width);

    for (int x { 0 }; x < destData.width; ++x)
    {
        // x < destData.width == maskColumn.size() (resized above) — in-bounds by loop bound.
        maskColumn[x] = jam::Value::map<double> (static_cast<double> (x), 0.0, static_cast<double> (destData.width), 0, maskData.width - 1);
    }

    for (int y { 0 }; y < destData.height; ++y)
    {
        const int maskY { jam::Value::map<double> (static_cast<double> (y), 0.0, static_cast<double> (destData.height), 0, maskData.height - 1) };

        auto* destRow { reinterpret_cast<uint32_t*> (destData.getLinePointer (y)) };
        const auto* maskRow { reinterpret_cast<const uint32_t*> (maskData.getLinePointer (maskY)) };

        int x { 0 };

        for (; x + 3 < destData.width; x += 4)
        {
            uint8_t maskAlpha[4];

            for (int lane { 0 }; lane < 4; ++lane)
            {
                // x + lane < destData.width == maskColumn.size() (batch loop bound, lane < 4).
                maskAlpha[lane] = static_cast<uint8_t> ((maskRow[maskColumn[x + lane]] >> alphaShift) & byteMask);
            }

            multiplyAlpha (destRow + x, maskAlpha);
        }

        for (; x < destData.width; ++x)
        {
            // x < destData.width == maskColumn.size() (tail loop bound).
            const uint8_t maskAlpha { static_cast<uint8_t> ((maskRow[maskColumn[x]] >> alphaShift) & byteMask) };

            multiplyAlphaPixel (destRow + x, maskAlpha);
        }
    }
}

static inline void extractAlpha (const juce::Image::BitmapData& data, uint8_t* dest) noexcept
{
    for (int y { 0 }; y < data.height; ++y)
    {
        const auto* row { reinterpret_cast<const uint32_t*> (data.getLinePointer (y)) };
        const int offset { y * data.width };

        for (int x { 0 }; x < data.width; ++x)
            dest[offset + x] = static_cast<uint8_t> ((row[x] >> alphaShift) & byteMask);
    }
}

static inline void writeAlpha (juce::Image::BitmapData& data, const uint8_t* alpha) noexcept
{
    for (int y { 0 }; y < data.height; ++y)
    {
        auto* row { reinterpret_cast<uint32_t*> (data.getLinePointer (y)) };
        const int offset { y * data.width };

        for (int x { 0 }; x < data.width; ++x)
            row[x] = (static_cast<uint32_t> (alpha[offset + x]) << alphaShift) | (row[x] & 0x00FFFFFFu);
    }
}

static inline void chokeHorizontal (const uint8_t* src, uint8_t* dest, int width, int height, int radius) noexcept
{
    const int windowSize { 2 * radius + 1 };
    const int paddedWidth { width + 2 * radius };

    jam::Array<uint8_t> padded;
    padded.resize (paddedWidth);

    for (int y { 0 }; y < height; ++y)
    {
        const int offset { y * width };

        std::memset (padded.data(), 0, static_cast<size_t> (paddedWidth));
        std::memcpy (padded.data() + radius, src + offset, static_cast<size_t> (width));

        int x { 0 };

#if JAM_SIMD_SSE2
        for (; x + 15 < width; x += 16)
        {
            __m128i minVec { _mm_set1_epi8 (static_cast<char> (0xFFu)) };

            for (int k { 0 }; k < windowSize; ++k)
                minVec = _mm_min_epu8 (minVec, _mm_loadu_si128 (reinterpret_cast<const __m128i*> (padded.data() + x + k)));

            _mm_storeu_si128 (reinterpret_cast<__m128i*> (dest + offset + x), minVec);
        }
#elif JAM_SIMD_NEON
        for (; x + 15 < width; x += 16)
        {
            uint8x16_t minVec { vdupq_n_u8 (255) };

            for (int k { 0 }; k < windowSize; ++k)
                minVec = vminq_u8 (minVec, vld1q_u8 (padded.data() + x + k));

            vst1q_u8 (dest + offset + x, minVec);
        }
#endif

        for (; x < width; ++x)
        {
            uint8_t minVal { 255 };

            for (int k { 0 }; k < windowSize; ++k)
                minVal = std::min (minVal, padded[static_cast<size_t> (x + k)]);

            dest[offset + x] = minVal;
        }
    }
}

static inline void chokeVertical (const uint8_t* src, uint8_t* dest, int width, int height, int radius) noexcept
{
    const int windowSize { 2 * radius + 1 };
    const int borderTop { std::min (radius, height) };
    const int borderBottom { std::max (0, height - radius) };

    std::memset (dest, 0, static_cast<size_t> (borderTop * width));

    if (borderBottom < height)
        std::memset (dest + borderBottom * width, 0, static_cast<size_t> ((height - borderBottom) * width));

    for (int y { radius }; y < height - radius; ++y)
    {
        int x { 0 };

#if JAM_SIMD_SSE2
        for (; x + 15 < width; x += 16)
        {
            __m128i minVec { _mm_set1_epi8 (static_cast<char> (0xFFu)) };

            for (int k { 0 }; k < windowSize; ++k)
                minVec = _mm_min_epu8 (minVec, _mm_loadu_si128 (reinterpret_cast<const __m128i*> (src + (y - radius + k) * width + x)));

            _mm_storeu_si128 (reinterpret_cast<__m128i*> (dest + y * width + x), minVec);
        }
#elif JAM_SIMD_NEON
        for (; x + 15 < width; x += 16)
        {
            uint8x16_t minVec { vdupq_n_u8 (255) };

            for (int k { 0 }; k < windowSize; ++k)
                minVec = vminq_u8 (minVec, vld1q_u8 (src + (y - radius + k) * width + x));

            vst1q_u8 (dest + y * width + x, minVec);
        }
#endif

        for (; x < width; ++x)
        {
            uint8_t minVal { 255 };

            for (int k { 0 }; k < windowSize; ++k)
                minVal = std::min (minVal, src[(y - radius + k) * width + x]);

            dest[y * width + x] = minVal;
        }
    }
}

/** @brief Distance-field feathers the alpha channel of @p data in-place — each pixel's
 *  alpha is multiplied by the fraction of its Chebyshev distance to the nearest
 *  transparent or out-of-bounds neighbour within the @p radius window, remapped through
 *  @p curve via jam::Value::map(). No-op when @p radius <= 0.
 *  @param data    Locked BitmapData (readWrite).
 *  @param radius  Feather radius in pixels.
 *  @param curve   Exponent applied to the distance-to-alpha mapping. */
static inline void applyMatteFeather (juce::Image::BitmapData& data, float radius, float curve) noexcept
{
    if (radius > 0.0f)
    {
        const int pixelCount { data.width * data.height };

        jam::Array<uint8_t> alpha;
        alpha.resize (pixelCount);
        extractAlpha (data, alpha.data());

        const int window { static_cast<int> (std::ceil (radius)) };

        for (int y { 0 }; y < data.height; ++y)
        {
            auto* row { reinterpret_cast<uint32_t*> (data.getLinePointer (y)) };

            for (int x { 0 }; x < data.width; ++x)
            {
                float minDist { radius };

                for (int dy { -window }; dy <= window; ++dy)
                {
                    for (int dx { -window }; dx <= window; ++dx)
                    {
                        const int sx { x + dx };
                        const int sy { y + dy };
                        const bool inBounds { sx >= 0 and sx < data.width and sy >= 0 and sy < data.height };

                        if (not inBounds or alpha[sy * data.width + sx] == 0)
                        {
                            minDist = std::min (minDist, static_cast<float> (std::max (std::abs (dx), std::abs (dy))));
                        }
                    }
                }

                const uint8_t factor { static_cast<uint8_t> (jam::Value::map (minDist, 0.0f, radius, 0.0f, 255.0f, curve)) };
                multiplyAlphaPixel (row + x, factor);
            }
        }
    }
}

/** @brief Morphologically erodes the alpha channel of @p data in-place via a two-pass
 *  sliding-window minimum (horizontal then vertical), shrinking the matte edge inward by
 *  @p radius pixels. No-op when @p radius <= 0.
 *  @param data    Locked BitmapData (readWrite).
 *  @param radius  Erosion radius in pixels. */
static inline void applyMatteChoke (juce::Image::BitmapData& data, int radius) noexcept
{
    if (radius > 0)
    {
        const int pixelCount { data.width * data.height };

        jam::Array<uint8_t> alpha;
        alpha.resize (pixelCount);
        extractAlpha (data, alpha.data());

        jam::Array<uint8_t> hPass;
        hPass.resize (pixelCount);
        chokeHorizontal (alpha.data(), hPass.data(), data.width, data.height, radius);

        jam::Array<uint8_t> vPass;
        vPass.resize (pixelCount);
        chokeVertical (hPass.data(), vPass.data(), data.width, data.height, radius);

        writeAlpha (data, vPass.data());
    }
}

namespace simd
{

/** @brief Converts one straight-alpha RGBA pixel to a premultiplied packed
 *  uint32_t (alpha in the high byte via alphaShift/redShift/greenShift),
 *  matching juce::PixelARGB::premultiply() bit-for-bit: alpha 255 bypasses
 *  the multiply, alpha 0 zeroes RGB, otherwise round-half-up (c * a + 127) >> 8.
 *  @param src   Pointer to 1 source pixel, 4 bytes RGBA straight alpha (read-only).
 *  @param dest  Pointer to 1 destination packed pixel (write-only). */
static inline void convertRgbaPremultiplyPixel (const uint8_t* src, uint32_t* dest) noexcept
{
    const uint32_t a { src[3] };

    uint32_t r { src[0] };
    uint32_t g { src[1] };
    uint32_t b { src[2] };

    // Matches juce::PixelARGB::premultiply(): alpha 255 bypasses the
    // multiply entirely (not just an identity result of the formula below),
    // alpha 0 zeroes RGB, otherwise (c * a + 127) >> 8.
    if (a < byteMask)
    {
        if (a == 0u)
        {
            r = 0u;
            g = 0u;
            b = 0u;
        }
        else
        {
            r = (r * a + 127u) >> 8u;
            g = (g * a + 127u) >> 8u;
            b = (b * a + 127u) >> 8u;
        }
    }

    *dest = (a << alphaShift) | (r << redShift) | (g << greenShift) | b;
}

/** @brief SIMD (NEON/SSE2, scalar fallback) counterpart to
 *  convertRgbaPremultiplyPixel() — converts 4 consecutive straight-alpha
 *  RGBA pixels to premultiplied packed pixels in one call.
 *
 *  Each lane runs the same three-arm rule as the scalar version (alpha 255
 *  passthrough, alpha 0 zero, otherwise round-half-up (c * a + 127) >> 8);
 *  the R/G/B byte lanes are swapped before packing so the SIMD store lands
 *  the same byte order in memory as the scalar version's shift-and-OR does.
 *
 *  @param src   Pointer to 4 consecutive source pixels, 16 bytes RGBA straight alpha (read-only).
 *  @param dest  Pointer to 4 consecutive destination packed pixels (write-only). */
static inline void convertRgbaPremultiply4 (const uint8_t* src, uint32_t* dest) noexcept
{
#if JAM_SIMD_SSE2
    static constexpr int channelSwapShuffle { _MM_SHUFFLE (3, 0, 1, 2) };
    static constexpr int alphaBroadcastShuffle { _MM_SHUFFLE (3, 3, 3, 3) };

    const __m128i vSrc { _mm_loadu_si128 (reinterpret_cast<const __m128i*> (src)) };
    const __m128i zero { _mm_setzero_si128() };

    const __m128i rawLo { _mm_unpacklo_epi8 (vSrc, zero) };
    const __m128i rawHi { _mm_unpackhi_epi8 (vSrc, zero) };

    // Swap R and B 16-bit lanes so the result matches this file's BGRA
    // packed-uint32 convention (alphaShift/redShift/greenShift above); A and
    // G lanes are untouched by the swap.
    const __m128i srcLo { _mm_shufflehi_epi16 (_mm_shufflelo_epi16 (rawLo, channelSwapShuffle), channelSwapShuffle) };
    const __m128i srcHi { _mm_shufflehi_epi16 (_mm_shufflelo_epi16 (rawHi, channelSwapShuffle), channelSwapShuffle) };

    const __m128i alphaLo { _mm_shufflehi_epi16 (_mm_shufflelo_epi16 (srcLo, alphaBroadcastShuffle), alphaBroadcastShuffle) };
    const __m128i alphaHi { _mm_shufflehi_epi16 (_mm_shufflelo_epi16 (srcHi, alphaBroadcastShuffle), alphaBroadcastShuffle) };

    const __m128i rounding { _mm_set1_epi16 (127) };
    const __m128i premultLo { _mm_srli_epi16 (_mm_add_epi16 (_mm_mullo_epi16 (srcLo, alphaLo), rounding), 8) };
    const __m128i premultHi { _mm_srli_epi16 (_mm_add_epi16 (_mm_mullo_epi16 (srcHi, alphaHi), rounding), 8) };

    const __m128i maskFullLo { _mm_cmpeq_epi16 (alphaLo, _mm_set1_epi16 (255)) };
    const __m128i maskFullHi { _mm_cmpeq_epi16 (alphaHi, _mm_set1_epi16 (255)) };
    const __m128i maskZeroLo { _mm_cmpeq_epi16 (alphaLo, zero) };
    const __m128i maskZeroHi { _mm_cmpeq_epi16 (alphaHi, zero) };

    const __m128i mixedLo { _mm_or_si128 (_mm_andnot_si128 (maskZeroLo, premultLo), _mm_and_si128 (maskZeroLo, zero)) };
    const __m128i mixedHi { _mm_or_si128 (_mm_andnot_si128 (maskZeroHi, premultHi), _mm_and_si128 (maskZeroHi, zero)) };

    const __m128i selectedLo { _mm_or_si128 (_mm_and_si128 (maskFullLo, srcLo), _mm_andnot_si128 (maskFullLo, mixedLo)) };
    const __m128i selectedHi { _mm_or_si128 (_mm_and_si128 (maskFullHi, srcHi), _mm_andnot_si128 (maskFullHi, mixedHi)) };

    // Alpha channel is always copied through unmodified (never itself
    // premultiplied) — restore it after the generic per-lane select above.
    const __m128i alphaLaneMask { _mm_set_epi16 (static_cast<short> (0xffffu), 0, 0, 0, static_cast<short> (0xffffu), 0, 0, 0) };
    const __m128i finalLo { _mm_or_si128 (_mm_and_si128 (alphaLaneMask, alphaLo), _mm_andnot_si128 (alphaLaneMask, selectedLo)) };
    const __m128i finalHi { _mm_or_si128 (_mm_and_si128 (alphaLaneMask, alphaHi), _mm_andnot_si128 (alphaLaneMask, selectedHi)) };

    _mm_storeu_si128 (reinterpret_cast<__m128i*> (dest), _mm_packus_epi16 (finalLo, finalHi));

#elif JAM_SIMD_NEON
    const uint8x16_t vSrc { vld1q_u8 (src) };

    const uint8x8_t swapIndices { 2, 1, 0, 3, 6, 5, 4, 7 };
    const uint8x8_t srcLo8 { vtbl1_u8 (vget_low_u8 (vSrc), swapIndices) };
    const uint8x8_t srcHi8 { vtbl1_u8 (vget_high_u8 (vSrc), swapIndices) };

    const uint8x8_t alphaIndices { 3, 3, 3, 3, 7, 7, 7, 7 };
    const uint8x8_t alphaLo8 { vtbl1_u8 (srcLo8, alphaIndices) };
    const uint8x8_t alphaHi8 { vtbl1_u8 (srcHi8, alphaIndices) };

    const uint16x8_t srcLo16 { vmovl_u8 (srcLo8) };
    const uint16x8_t srcHi16 { vmovl_u8 (srcHi8) };
    const uint16x8_t alphaLo16 { vmovl_u8 (alphaLo8) };
    const uint16x8_t alphaHi16 { vmovl_u8 (alphaHi8) };

    const uint16x8_t rounding { vdupq_n_u16 (127) };
    const uint16x8_t premultLo16 { vshrq_n_u16 (vaddq_u16 (vmulq_u16 (srcLo16, alphaLo16), rounding), 8) };
    const uint16x8_t premultHi16 { vshrq_n_u16 (vaddq_u16 (vmulq_u16 (srcHi16, alphaHi16), rounding), 8) };

    const uint16x8_t maskFullLo { vceqq_u16 (alphaLo16, vdupq_n_u16 (255)) };
    const uint16x8_t maskFullHi { vceqq_u16 (alphaHi16, vdupq_n_u16 (255)) };
    const uint16x8_t maskZeroLo { vceqq_u16 (alphaLo16, vdupq_n_u16 (0)) };
    const uint16x8_t maskZeroHi { vceqq_u16 (alphaHi16, vdupq_n_u16 (0)) };

    const uint16x8_t mixedLo { vbslq_u16 (maskZeroLo, vdupq_n_u16 (0), premultLo16) };
    const uint16x8_t mixedHi { vbslq_u16 (maskZeroHi, vdupq_n_u16 (0), premultHi16) };

    const uint16x8_t selectedLo { vbslq_u16 (maskFullLo, srcLo16, mixedLo) };
    const uint16x8_t selectedHi { vbslq_u16 (maskFullHi, srcHi16, mixedHi) };

    // Alpha channel is always copied through unmodified (never itself
    // premultiplied) — restore it after the generic per-lane select above.
    const uint16x8_t alphaLaneMask { 0, 0, 0, 0xffffu, 0, 0, 0, 0xffffu };
    const uint16x8_t finalLo16 { vbslq_u16 (alphaLaneMask, alphaLo16, selectedLo) };
    const uint16x8_t finalHi16 { vbslq_u16 (alphaLaneMask, alphaHi16, selectedHi) };

    vst1q_u8 (reinterpret_cast<uint8_t*> (dest), vcombine_u8 (vmovn_u16 (finalLo16), vmovn_u16 (finalHi16)));

#else
    for (int i { 0 }; i < 4; ++i)
    {
        convertRgbaPremultiplyPixel (src + i * 4, dest + i);
    }
#endif
}

/** @brief Converts one full row of straight-alpha RGBA pixels to premultiplied
 *  packed pixels — 4-wide via convertRgbaPremultiply4() with a scalar tail
 *  via convertRgbaPremultiplyPixel() for the remainder.
 *  @param srcRow   Pointer to @p width source pixels, 4 bytes RGBA straight alpha each (read-only).
 *  @param destRow  Pointer to @p width destination packed pixels (write-only).
 *  @param width    Number of pixels in the row. */
static inline void convertRgbaPremultiply (const uint8_t* srcRow, uint32_t* destRow, int width) noexcept
{
    int x { 0 };

    for (; x + 3 < width; x += 4)
    {
        convertRgbaPremultiply4 (srcRow + x * 4, destRow + x);
    }

    for (; x < width; ++x)
    {
        convertRgbaPremultiplyPixel (srcRow + x * 4, destRow + x);
    }
}

} // namespace simd

} // namespace jam
