/**
 * @file jam_BoxDrawing.h
 * @brief Procedural rasterizer for box drawing, block elements, and braille.
 *
 * Ported from the deleted jam::glyph::Atlas::Box (commit 1bd4b3593^,
 * jam_graphics/fonts/font/glyph/jam_box_drawing.h), flattened to a standalone
 * jam::BoxDrawing type (current jam::GlyphAtlas has no nested Packer/Atlas::Box
 * hierarchy to hang this off).
 *
 * BoxDrawing provides a fully self-contained, allocation-free rasterizer for
 * three Unicode character ranges:
 *
 * | Range           | Description             | Method                                        |
 * |-----------------|--------------------------|-----------------------------------------------|
 * | U+2500-U+257F   | Box drawing characters  | drawLines(), drawDashedLine(), drawDoubleLines(), drawRoundedCorner(), drawDiagonal() |
 * | U+2580-U+259F   | Block elements          | drawBlockElement()                             |
 * | U+2800-U+28FF   | Braille patterns        | drawBraille()                                  |
 *
 * ### Design principles
 * - **No allocations** for straight/block/braille paths: all methods operate
 *   on a caller-supplied `uint8_t*` buffer of `w x h` bytes. drawRoundedCorner()
 *   allocates one scratch supersample buffer internally (juce::HeapBlock).
 * - **Integer arithmetic** for straight lines and block fills (fillRect()).
 * - **Anti-aliased Bresenham** for diagonal lines (drawDiagonal()): distance to
 *   the line is computed analytically and mapped to alpha.
 * - **4x supersampled SDF, box-averaged down** for rounded corners
 *   (drawRoundedCorner()).
 * - **Cell-relative metrics**: line thickness is derived from cellWidth via
 *   lightThickness()/heavyThickness(), so the output scales correctly at any
 *   font size.
 *
 * ### Coordinate system
 * The output buffer uses a top-left origin with Y increasing downward. Pixel
 * (x, y) maps to `buf[y * w + x]`.
 *
 * @par Integration status
 * This rasterizer is ported and available (isProcedural()/rasterize() are a
 * complete, working, drop-in port), but NOT YET wired as a first-check
 * dispatch path inside GlyphAtlas::rasterize(). Doing so needs the target
 * codepoint at that call site (GlyphAtlas::Key only carries typeface +
 * glyphIndex + fontSize -- glyphIndex is a font-internal id, not the Unicode
 * codepoint JUCE's drawGlyphs() call sites never pass down) and the target
 * terminal cell's exact pixel width/height (needed so the rasterized box
 * character tiles seamlessly against its neighbours -- not derivable from
 * juce::Typeface alone, only from jam::Font/juce::Font, which GlyphAtlas does
 * not currently reference). An unresolved design decision, not implemented here.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/
/**
 * @struct BoxDrawing
 * @brief Procedural rasterizer for box-drawing, block-element, and braille codepoints.
 *
 * All methods are `static` and `noexcept`. BoxDrawing has no instance state
 * and is never instantiated.
 */
struct BoxDrawing
{
    /** @brief Line weight for box-drawing strokes. */
    enum class Weight : uint8_t
    {
        none,   ///< No line on this side.
        light,  ///< Standard thin line (~1/8 of cell width).
        heavy,  ///< Bold line (~2x light thickness).
        double_ ///< Two parallel light lines (handled by drawDoubleLines()).
    };

    /**
     * @struct Lines
     * @brief Four-directional line weight descriptor for a box-drawing character.
     *
     * Each field specifies the weight of the line segment extending from the
     * cell centre toward the named edge. Used by drawLines() to render
     * standard box-drawing characters.
     */
    struct Lines
    {
        /** @brief Weight of the line segment extending upward from the cell centre. */
        Weight up { Weight::none };

        /** @brief Weight of the line segment extending rightward from the cell centre. */
        Weight right { Weight::none };

        /** @brief Weight of the line segment extending downward from the cell centre. */
        Weight down { Weight::none };

        /** @brief Weight of the line segment extending leftward from the cell centre. */
        Weight left { Weight::none };
    };

    static_assert (std::is_trivially_copyable_v<Lines>);

    /**
     * @brief Returns `true` if the codepoint is handled by this rasterizer.
     *
     * Covers box drawing (U+2500-U+257F), block elements (U+2580-U+259F), and
     * braille patterns (U+2800-U+28FF).
     */
    static bool isProcedural (uint32_t codepoint) noexcept
    {
        return (codepoint >= 0x2500 and codepoint <= 0x257F) or (codepoint >= 0x2580 and codepoint <= 0x259F)
               or (codepoint >= 0x2800 and codepoint <= 0x28FF);
    }

    /**
     * @brief Rasterize a box-drawing, block-element, or braille codepoint.
     *
     * Clears `buf` to zero, then dispatches to the appropriate sub-renderer
     * based on the codepoint range.
     *
     * @param codepoint Unicode codepoint to rasterize.
     * @param w         Output bitmap width in pixels (= cell width).
     * @param h         Output bitmap height in pixels (= cell height).
     * @param buf       Output buffer of `w x h` bytes (R8 greyscale). Must be pre-allocated by the caller.
     * @param embolden  When `true`, doubles effective line thickness for both light and heavy weight strokes.
     * @see isProcedural()
     */
    static void rasterize (uint32_t codepoint, int w, int h, uint8_t* buf, bool embolden) noexcept
    {
        std::memset (buf, 0, static_cast<size_t> (w) * static_cast<size_t> (h));

        if (codepoint >= 0x2500 and codepoint <= 0x257F)
        {
            const uint32_t idx { codepoint - 0x2500 };

            if (idx >= 0x6D and idx <= 0x70)
            {
                drawRoundedCorner (idx, w, h, buf, embolden);
            }
            else if (idx >= 0x04 and idx <= 0x0B)
            {
                drawDashedLine (idx, w, h, buf, embolden);
            }
            else if (idx >= 0x71 and idx <= 0x73)
            {
                drawDiagonal (idx, w, h, buf, embolden);
            }
            else if (idx <= 0x4B)
            {
                const Lines& lines { table.at (idx) };
                drawLines (lines, w, h, buf, embolden);
            }
            else if (idx >= 0x50 and idx <= 0x6C)
            {
                drawDoubleLines (idx, w, h, buf, embolden);
            }
            else if (idx >= 0x74 and idx <= 0x7F)
            {
                const Lines& lines { halfTable.at (idx - 0x74) };
                drawLines (lines, w, h, buf, embolden);
            }
            else
            {
                const Lines fallback { Weight::light, Weight::light, Weight::light, Weight::light };
                drawLines (fallback, w, h, buf, embolden);
            }
        }

        if (codepoint >= 0x2580 and codepoint <= 0x259F)
        {
            drawBlockElement (codepoint, w, h, buf);
        }

        if (codepoint >= 0x2800 and codepoint <= 0x28FF)
        {
            drawBraille (codepoint, w, h, buf);
        }
    }

private:
    /** @brief Fill an axis-aligned rectangle in the bitmap with a constant value. */
    static void fillRect (uint8_t* buf, int stride, int x0, int y0, int x1, int y1, uint8_t value) noexcept
    {
        x0 = std::max (x0, 0);
        y0 = std::max (y0, 0);
        x1 = std::min (x1, stride);

        for (int y { y0 }; y < y1; ++y)
        {
            for (int x { x0 }; x < x1; ++x)
            {
                buf[y * stride + x] = value;
            }
        }
    }

    /** @brief Computes the light (thin) line thickness for the given cell width. */
    static int lightThickness (int cellWidth, bool embolden) noexcept
    {
        const int base { std::max (1, cellWidth / 10) };
        return embolden ? base * 2 : base;
    }

    /** @brief Computes the heavy (bold) line thickness for the given cell width. */
    static int heavyThickness (int cellWidth, bool embolden) noexcept
    {
        return std::max (2, lightThickness (cellWidth, embolden) * 2);
    }

    /** @brief Renders the four directional line segments described by `lines`. */
    static void drawLines (const Lines& lines, int w, int h, uint8_t* buf, bool embolden) noexcept
    {
        const int cx { w / 2 };
        const int cy { h / 2 };
        const int lt { lightThickness (w, embolden) };
        const int ht { heavyThickness (w, embolden) };

        if (lines.up != Weight::none)
        {
            const int t { lines.up == Weight::heavy ? ht : lt };
            fillRect (buf, w, cx - t / 2, 0, cx - t / 2 + t, cy + t / 2, 255);
        }

        if (lines.down != Weight::none)
        {
            const int t { lines.down == Weight::heavy ? ht : lt };
            fillRect (buf, w, cx - t / 2, cy - t / 2, cx - t / 2 + t, h, 255);
        }

        if (lines.left != Weight::none)
        {
            const int t { lines.left == Weight::heavy ? ht : lt };
            fillRect (buf, w, 0, cy - t / 2, cx + t / 2, cy - t / 2 + t, 255);
        }

        if (lines.right != Weight::none)
        {
            const int t { lines.right == Weight::heavy ? ht : lt };
            fillRect (buf, w, cx - t / 2, cy - t / 2, w, cy - t / 2 + t, 255);
        }
    }

    /** @brief Renders a dashed horizontal or vertical line (U+2504-U+250B). */
    static void drawDashedLine (uint32_t idx, int w, int h, uint8_t* buf, bool embolden) noexcept
    {
        const int cx { w / 2 };
        const int cy { h / 2 };
        const bool horizontal { idx == 0x04 or idx == 0x05 or idx == 0x08 or idx == 0x09 };
        const bool heavy { idx == 0x05 or idx == 0x07 or idx == 0x09 or idx == 0x0B };
        const int numDashes { (idx == 0x04 or idx == 0x05 or idx == 0x06 or idx == 0x07) ? 3 : 4 };
        const int t { heavy ? heavyThickness (w, embolden) : lightThickness (w, embolden) };

        if (horizontal)
        {
            const int dashLen { w / (numDashes * 2) };
            const int gapLen { dashLen };
            const int y0 { cy - t / 2 };
            const int y1 { y0 + t };
            int x { dashLen / 2 };

            for (int d { 0 }; d < numDashes and x < w; ++d)
            {
                const int end { std::min (x + dashLen, w) };
                fillRect (buf, w, x, y0, end, y1, 255);
                x = end + gapLen;
            }
        }
        else
        {
            const int dashLen { h / (numDashes * 2) };
            const int gapLen { dashLen };
            const int x0 { cx - t / 2 };
            const int x1 { x0 + t };
            int y { dashLen / 2 };

            for (int d { 0 }; d < numDashes and y < h; ++d)
            {
                const int end { std::min (y + dashLen, h) };
                fillRect (buf, w, x0, y, x1, end, 255);
                y = end + gapLen;
            }
        }
    }

    /** @brief Renders anti-aliased diagonal lines (U+2571-U+2573). */
    static void drawDiagonal (uint32_t idx, int w, int h, uint8_t* buf, bool embolden) noexcept
    {
        const int t { lightThickness (w, embolden) };
        const float halfT { static_cast<float> (t) * 0.5f };
        const float fw { static_cast<float> (w) };
        const float fh { static_cast<float> (h) };

        for (int py { 0 }; py < h; ++py)
        {
            for (int px { 0 }; px < w; ++px)
            {
                const float x { static_cast<float> (px) + 0.5f };
                const float y { static_cast<float> (py) + 0.5f };
                float alpha { 0.0f };

                if (idx == 0x71 or idx == 0x73)
                {
                    const float dist { std::abs ((x / fw + y / fh - 1.0f) * fw * fh / std::sqrt (fw * fw + fh * fh)) };

                    if (dist < halfT + 0.5f)
                    {
                        alpha = std::max (alpha, std::min (1.0f, halfT + 0.5f - dist));
                    }
                }

                if (idx == 0x72 or idx == 0x73)
                {
                    const float dist { std::abs ((x / fw - y / fh) * fw * fh / std::sqrt (fw * fw + fh * fh)) };

                    if (dist < halfT + 0.5f)
                    {
                        alpha = std::max (alpha, std::min (1.0f, halfT + 0.5f - dist));
                    }
                }

                if (alpha > 0.0f)
                {
                    const int existing { buf[py * w + px] };
                    buf[py * w + px] = static_cast<uint8_t> (std::max (existing, static_cast<int> (alpha * 255.0f)));
                }
            }
        }
    }

    /** @brief Renders double-line box-drawing characters (U+2550-U+256C). */
    static void drawDoubleLines (uint32_t idx, int w, int h, uint8_t* buf, bool embolden) noexcept
    {
        const int cx { w / 2 };
        const int cy { h / 2 };
        const int lt { lightThickness (w, embolden) };
        const int gap { std::max (1, lt) };

        const int outerH { cy - gap };
        const int innerH { cy + gap };
        const int outerV { cx - gap };
        const int innerV { cx + gap };

        switch (idx)
        {
            case 0x50:// =
                fillRect (buf, w, 0, outerH - lt / 2, w, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, 0, innerH - lt / 2, w, innerH - lt / 2 + lt, 255);
                break;
            case 0x51:// ||
                fillRect (buf, w, outerV - lt / 2, 0, outerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, innerV - lt / 2, 0, innerV - lt / 2 + lt, h, 255);
                break;
            case 0x52:// down=light, right=double
                fillRect (buf, w, cx - lt / 2, cy, cx - lt / 2 + lt, h, 255);
                fillRect (buf, w, cx, outerH - lt / 2, w, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, cx, innerH - lt / 2, w, innerH - lt / 2 + lt, 255);
                break;
            case 0x53:// down=double, right=light
                fillRect (buf, w, outerV - lt / 2, cy, outerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, innerV - lt / 2, cy, innerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, cx, cy - lt / 2, w, cy - lt / 2 + lt, 255);
                break;
            case 0x54:// down=double, right=double
                fillRect (buf, w, outerV - lt / 2, cy, outerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, innerV - lt / 2, cy, innerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, cx, outerH - lt / 2, w, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, innerV, innerH - lt / 2, w, innerH - lt / 2 + lt, 255);
                break;
            case 0x55:// down=light, left=double
                fillRect (buf, w, cx - lt / 2, cy, cx - lt / 2 + lt, h, 255);
                fillRect (buf, w, 0, outerH - lt / 2, cx, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, 0, innerH - lt / 2, cx, innerH - lt / 2 + lt, 255);
                break;
            case 0x56:// down=double, left=light
                fillRect (buf, w, outerV - lt / 2, cy, outerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, innerV - lt / 2, cy, innerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, 0, cy - lt / 2, cx, cy - lt / 2 + lt, 255);
                break;
            case 0x57:// down=double, left=double
                fillRect (buf, w, outerV - lt / 2, cy, outerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, innerV - lt / 2, cy, innerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, 0, outerH - lt / 2, cx, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, 0, innerH - lt / 2, outerV, innerH - lt / 2 + lt, 255);
                break;
            case 0x58:// up=light, right=double
                fillRect (buf, w, cx - lt / 2, 0, cx - lt / 2 + lt, cy, 255);
                fillRect (buf, w, cx, outerH - lt / 2, w, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, cx, innerH - lt / 2, w, innerH - lt / 2 + lt, 255);
                break;
            case 0x59:// up=double, right=light
                fillRect (buf, w, outerV - lt / 2, 0, outerV - lt / 2 + lt, cy, 255);
                fillRect (buf, w, innerV - lt / 2, 0, innerV - lt / 2 + lt, cy, 255);
                fillRect (buf, w, cx, cy - lt / 2, w, cy - lt / 2 + lt, 255);
                break;
            case 0x5A:// up=double, right=double
                fillRect (buf, w, outerV - lt / 2, 0, outerV - lt / 2 + lt, cy, 255);
                fillRect (buf, w, innerV - lt / 2, 0, innerV - lt / 2 + lt, cy, 255);
                fillRect (buf, w, cx, outerH - lt / 2, w, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, innerV, innerH - lt / 2, w, innerH - lt / 2 + lt, 255);
                break;
            case 0x5B:// up=light, left=double
                fillRect (buf, w, cx - lt / 2, 0, cx - lt / 2 + lt, cy, 255);
                fillRect (buf, w, 0, outerH - lt / 2, cx, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, 0, innerH - lt / 2, cx, innerH - lt / 2 + lt, 255);
                break;
            case 0x5C:// up=double, left=light
                fillRect (buf, w, outerV - lt / 2, 0, outerV - lt / 2 + lt, cy, 255);
                fillRect (buf, w, innerV - lt / 2, 0, innerV - lt / 2 + lt, cy, 255);
                fillRect (buf, w, 0, cy - lt / 2, cx, cy - lt / 2 + lt, 255);
                break;
            case 0x5D:// up=double, left=double
                fillRect (buf, w, outerV - lt / 2, 0, outerV - lt / 2 + lt, cy, 255);
                fillRect (buf, w, innerV - lt / 2, 0, innerV - lt / 2 + lt, cy, 255);
                fillRect (buf, w, 0, outerH - lt / 2, cx, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, 0, innerH - lt / 2, outerV, innerH - lt / 2 + lt, 255);
                break;
            case 0x5E:// up=light, down=light, right=double
                fillRect (buf, w, cx - lt / 2, 0, cx - lt / 2 + lt, h, 255);
                fillRect (buf, w, cx, outerH - lt / 2, w, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, cx, innerH - lt / 2, w, innerH - lt / 2 + lt, 255);
                break;
            case 0x5F:// up=double, down=double, right=light
                fillRect (buf, w, outerV - lt / 2, 0, outerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, innerV - lt / 2, 0, innerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, cx, cy - lt / 2, w, cy - lt / 2 + lt, 255);
                break;
            case 0x60:// up=double, down=double, right=double
                fillRect (buf, w, outerV - lt / 2, 0, outerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, innerV - lt / 2, 0, innerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, innerV, outerH - lt / 2, w, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, innerV, innerH - lt / 2, w, innerH - lt / 2 + lt, 255);
                break;
            case 0x61:// up=light, down=light, left=double
                fillRect (buf, w, cx - lt / 2, 0, cx - lt / 2 + lt, h, 255);
                fillRect (buf, w, 0, outerH - lt / 2, cx, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, 0, innerH - lt / 2, cx, innerH - lt / 2 + lt, 255);
                break;
            case 0x62:// up=double, down=double, left=light
                fillRect (buf, w, outerV - lt / 2, 0, outerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, innerV - lt / 2, 0, innerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, 0, cy - lt / 2, cx, cy - lt / 2 + lt, 255);
                break;
            case 0x63:// up=double, down=double, left=double
                fillRect (buf, w, outerV - lt / 2, 0, outerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, innerV - lt / 2, 0, innerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, 0, outerH - lt / 2, outerV, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, 0, innerH - lt / 2, outerV, innerH - lt / 2 + lt, 255);
                break;
            case 0x64:// down=light, left=double, right=double
                fillRect (buf, w, cx - lt / 2, cy, cx - lt / 2 + lt, h, 255);
                fillRect (buf, w, 0, outerH - lt / 2, w, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, 0, innerH - lt / 2, w, innerH - lt / 2 + lt, 255);
                break;
            case 0x65:// down=double, left=light, right=light
                fillRect (buf, w, outerV - lt / 2, cy, outerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, innerV - lt / 2, cy, innerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, 0, cy - lt / 2, w, cy - lt / 2 + lt, 255);
                break;
            case 0x66:// down=double, left=double, right=double
                fillRect (buf, w, outerV - lt / 2, cy, outerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, innerV - lt / 2, cy, innerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, 0, outerH - lt / 2, w, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, outerV, innerH - lt / 2, innerV, innerH - lt / 2 + lt, 255);
                break;
            case 0x67:// up=light, left=double, right=double
                fillRect (buf, w, cx - lt / 2, 0, cx - lt / 2 + lt, cy, 255);
                fillRect (buf, w, 0, outerH - lt / 2, w, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, 0, innerH - lt / 2, w, innerH - lt / 2 + lt, 255);
                break;
            case 0x68:// up=double, left=light, right=light
                fillRect (buf, w, outerV - lt / 2, 0, outerV - lt / 2 + lt, cy, 255);
                fillRect (buf, w, innerV - lt / 2, 0, innerV - lt / 2 + lt, cy, 255);
                fillRect (buf, w, 0, cy - lt / 2, w, cy - lt / 2 + lt, 255);
                break;
            case 0x69:// up=double, left=double, right=double
                fillRect (buf, w, outerV - lt / 2, 0, outerV - lt / 2 + lt, cy, 255);
                fillRect (buf, w, innerV - lt / 2, 0, innerV - lt / 2 + lt, cy, 255);
                fillRect (buf, w, 0, outerH - lt / 2, w, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, outerV, innerH - lt / 2, innerV, innerH - lt / 2 + lt, 255);
                break;
            case 0x6A:// up=light, down=light, left=double, right=double
                fillRect (buf, w, cx - lt / 2, 0, cx - lt / 2 + lt, h, 255);
                fillRect (buf, w, 0, outerH - lt / 2, w, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, 0, innerH - lt / 2, w, innerH - lt / 2 + lt, 255);
                break;
            case 0x6B:// up=double, down=double, left=light, right=light
                fillRect (buf, w, outerV - lt / 2, 0, outerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, innerV - lt / 2, 0, innerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, 0, cy - lt / 2, w, cy - lt / 2 + lt, 255);
                break;
            case 0x6C:// up=double, down=double, left=double, right=double
                fillRect (buf, w, outerV - lt / 2, 0, outerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, innerV - lt / 2, 0, innerV - lt / 2 + lt, h, 255);
                fillRect (buf, w, 0, outerH - lt / 2, outerV, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, innerV, outerH - lt / 2, w, outerH - lt / 2 + lt, 255);
                fillRect (buf, w, 0, innerH - lt / 2, outerV, innerH - lt / 2 + lt, 255);
                fillRect (buf, w, innerV, innerH - lt / 2, w, innerH - lt / 2 + lt, 255);
                break;
            default:
                break;
        }
    }

    /** @brief Smoothstep anti-aliasing kernel: Hermite interpolation. */
    static float smoothstep (float edge0, float edge1, float x) noexcept
    {
        const float t { std::clamp ((x - edge0) / (edge1 - edge0), 0.0f, 1.0f) };
        return t * t * (3.0f - 2.0f * t);
    }

    /**
     * @brief Renders an SDF-based rounded corner (U+256D-U+2570).
     *
     * 4x supersampled SDF, box-averaged down to native resolution. SDF
     * geometry parameters are derived from the actual pixel range that
     * drawLines() uses for the straight strokes, so the arc tangent point
     * aligns with those strokes regardless of cell size.
     */
    static void drawRoundedCorner (uint32_t idx, int w, int h, uint8_t* buf, bool embolden) noexcept
    {
        static constexpr int supersampleFactor { 4 };

        const int scratchWidth { w * supersampleFactor };
        const int scratchHeight { h * supersampleFactor };
        juce::HeapBlock<uint8_t> scratch (
            static_cast<size_t> (scratchWidth) * static_cast<size_t> (scratchHeight), true);

        // Derive SDF centre from the actual pixel range drawLines() uses for the vertical/horizontal
        // strokes: drawLines() places the vertical stroke at [cx - t/2, cx - t/2 + t), so its
        // geometric centre is (cx - t/2) + t/2.0f.
        const int lightT { lightThickness (w, embolden) };
        const int cxNative { w / 2 };
        const int cyNative { h / 2 };
        const float adjustedHxSs { (static_cast<float> (cxNative - lightT / 2) + static_cast<float> (lightT) * 0.5f)
                                   * static_cast<float> (supersampleFactor) };
        const float adjustedHySs { (static_cast<float> (cyNative - lightT / 2) + static_cast<float> (lightT) * 0.5f)
                                   * static_cast<float> (supersampleFactor) };

        const float strokeSs { static_cast<float> (lightT * supersampleFactor) };
        const float halfStrokeSs { strokeSs * 0.5f };
        const float radiusSs { std::min (adjustedHxSs, adjustedHySs) };
        const float bxSs { adjustedHxSs - radiusSs };
        const float bySs { adjustedHySs - radiusSs };
        const float aaSs { static_cast<float> (supersampleFactor) * 0.5f };

        float xShiftSs { 0.0f };
        float yShiftSs { 0.0f };

        switch (idx)
        {
            case 0x6D:
                xShiftSs = -adjustedHxSs;
                yShiftSs = -adjustedHySs;
                break;
            case 0x6E:
                xShiftSs = +adjustedHxSs;
                yShiftSs = -adjustedHySs;
                break;
            case 0x6F:
                xShiftSs = +adjustedHxSs;
                yShiftSs = +adjustedHySs;
                break;
            case 0x70:
                xShiftSs = -adjustedHxSs;
                yShiftSs = +adjustedHySs;
                break;
            default:
                break;
        }

        for (int sy { 0 }; sy < scratchHeight; ++sy)
        {
            for (int sx { 0 }; sx < scratchWidth; ++sx)
            {
                const float sampleX { static_cast<float> (sx) + 0.5f + xShiftSs };
                const float sampleY { static_cast<float> (sy) + 0.5f + yShiftSs };
                const float posX { sampleX - adjustedHxSs };
                const float posY { sampleY - adjustedHySs };
                const float qx { std::abs (posX) - bxSs };
                const float qy { std::abs (posY) - bySs };
                const float dx { std::max (qx, 0.0f) };
                const float dy { std::max (qy, 0.0f) };
                const float dist { std::sqrt (dx * dx + dy * dy) + std::min (std::max (qx, qy), 0.0f) - radiusSs };
                const float aa { (qx > 1e-7f and qy > 1e-7f) ? aaSs : 0.0f };
                const float outer { halfStrokeSs - dist };
                const float inner { -halfStrokeSs - dist };
                const float alpha { smoothstep (-aa, aa, outer) - smoothstep (-aa, aa, inner) };

                if (alpha > 0.0f)
                    scratch[sy * scratchWidth + sx] = static_cast<uint8_t> (alpha * 255.0f);
            }
        }

        const int samplesPerPixel { supersampleFactor * supersampleFactor };

        for (int py { 0 }; py < h; ++py)
        {
            for (int px { 0 }; px < w; ++px)
            {
                int accumulator { 0 };

                const int ssBaseX { px * supersampleFactor };
                const int ssBaseY { py * supersampleFactor };

                for (int dy { 0 }; dy < supersampleFactor; ++dy)
                {
                    for (int dx { 0 }; dx < supersampleFactor; ++dx)
                    {
                        accumulator += scratch[(ssBaseY + dy) * scratchWidth + (ssBaseX + dx)];
                    }
                }

                const int averaged { accumulator / samplesPerPixel };

                if (averaged > 0)
                {
                    const int existing { buf[py * w + px] };
                    buf[py * w + px] = static_cast<uint8_t> (std::max (existing, averaged));
                }
            }
        }
    }

    /** @brief Fills the entire bitmap with a uniform shade value (U+2591-U+2593). */
    static void drawShade (int w, int h, uint8_t* buf, uint8_t alpha) noexcept
    {
        std::memset (buf, alpha, static_cast<size_t> (w) * static_cast<size_t> (h));
    }

    /** @brief Renders a block element character (U+2580-U+259F). */
    static void drawBlockElement (uint32_t codepoint, int w, int h, uint8_t* buf) noexcept
    {
        const int halfW { w / 2 };
        const int halfH { h / 2 };

        switch (codepoint)
        {
            case 0x2580:
                fillRect (buf, w, 0, 0, w, halfH, 255);
                break;
            case 0x2581:
                fillRect (buf, w, 0, h * 7 / 8, w, h, 255);
                break;
            case 0x2582:
                fillRect (buf, w, 0, h * 6 / 8, w, h, 255);
                break;
            case 0x2583:
                fillRect (buf, w, 0, h * 5 / 8, w, h, 255);
                break;
            case 0x2584:
                fillRect (buf, w, 0, h * 4 / 8, w, h, 255);
                break;
            case 0x2585:
                fillRect (buf, w, 0, h * 3 / 8, w, h, 255);
                break;
            case 0x2586:
                fillRect (buf, w, 0, h * 2 / 8, w, h, 255);
                break;
            case 0x2587:
                fillRect (buf, w, 0, h * 1 / 8, w, h, 255);
                break;
            case 0x2588:
                fillRect (buf, w, 0, 0, w, h, 255);
                break;
            case 0x2589:
                fillRect (buf, w, 0, 0, w * 7 / 8, h, 255);
                break;
            case 0x258A:
                fillRect (buf, w, 0, 0, w * 6 / 8, h, 255);
                break;
            case 0x258B:
                fillRect (buf, w, 0, 0, w * 5 / 8, h, 255);
                break;
            case 0x258C:
                fillRect (buf, w, 0, 0, w * 4 / 8, h, 255);
                break;
            case 0x258D:
                fillRect (buf, w, 0, 0, w * 3 / 8, h, 255);
                break;
            case 0x258E:
                fillRect (buf, w, 0, 0, w * 2 / 8, h, 255);
                break;
            case 0x258F:
                fillRect (buf, w, 0, 0, w * 1 / 8, h, 255);
                break;
            case 0x2590:
                fillRect (buf, w, halfW, 0, w, h, 255);
                break;
            case 0x2591:
                drawShade (w, h, buf, 64);
                break;
            case 0x2592:
                drawShade (w, h, buf, 128);
                break;
            case 0x2593:
                drawShade (w, h, buf, 191);
                break;
            case 0x2594:
                fillRect (buf, w, 0, 0, w, h * 1 / 8, 255);
                break;
            case 0x2595:
                fillRect (buf, w, w * 7 / 8, 0, w, h, 255);
                break;
            case 0x2596:
                fillRect (buf, w, 0, halfH, halfW, h, 255);
                break;
            case 0x2597:
                fillRect (buf, w, halfW, halfH, w, h, 255);
                break;
            case 0x2598:
                fillRect (buf, w, 0, 0, halfW, halfH, 255);
                break;
            case 0x2599:
                fillRect (buf, w, 0, 0, halfW, halfH, 255);
                fillRect (buf, w, 0, halfH, halfW, h, 255);
                fillRect (buf, w, halfW, halfH, w, h, 255);
                break;
            case 0x259A:
                fillRect (buf, w, 0, 0, halfW, halfH, 255);
                fillRect (buf, w, halfW, halfH, w, h, 255);
                break;
            case 0x259B:
                fillRect (buf, w, 0, 0, halfW, halfH, 255);
                fillRect (buf, w, halfW, 0, w, halfH, 255);
                fillRect (buf, w, 0, halfH, halfW, h, 255);
                break;
            case 0x259C:
                fillRect (buf, w, 0, 0, halfW, halfH, 255);
                fillRect (buf, w, halfW, 0, w, halfH, 255);
                fillRect (buf, w, halfW, halfH, w, h, 255);
                break;
            case 0x259D:
                fillRect (buf, w, halfW, 0, w, halfH, 255);
                break;
            case 0x259E:
                fillRect (buf, w, halfW, 0, w, halfH, 255);
                fillRect (buf, w, 0, halfH, halfW, h, 255);
                break;
            case 0x259F:
                fillRect (buf, w, halfW, 0, w, halfH, 255);
                fillRect (buf, w, 0, halfH, halfW, h, 255);
                fillRect (buf, w, halfW, halfH, w, h, 255);
                break;
            default:
                break;
        }
    }

    /** @brief Renders a braille pattern character (U+2800-U+28FF). */
    static void drawBraille (uint32_t codepoint, int w, int h, uint8_t* buf) noexcept
    {
        const uint8_t pattern { static_cast<uint8_t> (codepoint - 0x2800) };
        const int cellW { w / 2 };
        const int cellH { h / 4 };
        const int dotW { std::max (1, cellW / 2) };
        const int dotH { std::max (1, cellH / 2) };

        static constexpr std::array<int, 8> dotCol {
            { 0, 0, 0, 1, 1, 1, 0, 1 }
        };
        static constexpr std::array<int, 8> dotRow {
            { 0, 1, 2, 0, 1, 2, 3, 3 }
        };

        for (int i { 0 }; i < 8; ++i)
        {
            if (pattern & static_cast<uint8_t> (1 << i))
            {
                const int col { dotCol.at (i) };
                const int row { dotRow.at (i) };
                const int x0 { col * cellW + (cellW - dotW) / 2 };
                const int y0 { row * cellH + (cellH - dotH) / 2 };
                fillRect (buf, w, x0, y0, x0 + dotW, y0 + dotH, 255);
            }
        }
    }

    /**
     * @brief Lookup table mapping box-drawing indices 0x00-0x4B to Lines descriptors.
     *
     * Index `i` corresponds to Unicode codepoint U+2500 + i. Entries for
     * dashed characters (0x04-0x0B) are present but ignored by rasterize(),
     * which dispatches those to drawDashedLine() instead.
     */
    static constexpr std::array<Lines, 76> table {
        {
         { Weight::none, Weight::light, Weight::none, Weight::light },// 0x2500
            { Weight::none, Weight::heavy, Weight::none, Weight::heavy },// 0x2501
            { Weight::light, Weight::none, Weight::light, Weight::none },// 0x2502
            { Weight::heavy, Weight::none, Weight::heavy, Weight::none },// 0x2503
            { Weight::none, Weight::light, Weight::none, Weight::light },// 0x2504
            { Weight::none, Weight::heavy, Weight::none, Weight::heavy },// 0x2505
            { Weight::light, Weight::none, Weight::light, Weight::none },// 0x2506
            { Weight::heavy, Weight::none, Weight::heavy, Weight::none },// 0x2507
            { Weight::none, Weight::light, Weight::none, Weight::light },// 0x2508
            { Weight::none, Weight::heavy, Weight::none, Weight::heavy },// 0x2509
            { Weight::light, Weight::none, Weight::light, Weight::none },// 0x250A
            { Weight::heavy, Weight::none, Weight::heavy, Weight::none },// 0x250B
            { Weight::none, Weight::light, Weight::light, Weight::none },// 0x250C
            { Weight::none, Weight::heavy, Weight::light, Weight::none },// 0x250D
            { Weight::none, Weight::light, Weight::heavy, Weight::none },// 0x250E
            { Weight::none, Weight::heavy, Weight::heavy, Weight::none },// 0x250F
            { Weight::none, Weight::none, Weight::light, Weight::light },// 0x2510
            { Weight::none, Weight::none, Weight::light, Weight::heavy },// 0x2511
            { Weight::none, Weight::none, Weight::heavy, Weight::light },// 0x2512
            { Weight::none, Weight::none, Weight::heavy, Weight::heavy },// 0x2513
            { Weight::light, Weight::light, Weight::none, Weight::none },// 0x2514
            { Weight::light, Weight::heavy, Weight::none, Weight::none },// 0x2515
            { Weight::heavy, Weight::light, Weight::none, Weight::none },// 0x2516
            { Weight::heavy, Weight::heavy, Weight::none, Weight::none },// 0x2517
            { Weight::light, Weight::none, Weight::none, Weight::light },// 0x2518
            { Weight::light, Weight::none, Weight::none, Weight::heavy },// 0x2519
            { Weight::heavy, Weight::none, Weight::none, Weight::light },// 0x251A
            { Weight::heavy, Weight::none, Weight::none, Weight::heavy },// 0x251B
            { Weight::light, Weight::light, Weight::light, Weight::none },// 0x251C
            { Weight::light, Weight::heavy, Weight::light, Weight::none },// 0x251D
            { Weight::heavy, Weight::light, Weight::light, Weight::none },// 0x251E
            { Weight::light, Weight::light, Weight::heavy, Weight::none },// 0x251F
            { Weight::heavy, Weight::light, Weight::heavy, Weight::none },// 0x2520
            { Weight::heavy, Weight::heavy, Weight::light, Weight::none },// 0x2521
            { Weight::light, Weight::heavy, Weight::heavy, Weight::none },// 0x2522
            { Weight::heavy, Weight::heavy, Weight::heavy, Weight::none },// 0x2523
            { Weight::light, Weight::none, Weight::light, Weight::light },// 0x2524
            { Weight::light, Weight::none, Weight::light, Weight::heavy },// 0x2525
            { Weight::heavy, Weight::none, Weight::light, Weight::light },// 0x2526
            { Weight::light, Weight::none, Weight::heavy, Weight::light },// 0x2527
            { Weight::heavy, Weight::none, Weight::heavy, Weight::light },// 0x2528
            { Weight::heavy, Weight::none, Weight::light, Weight::heavy },// 0x2529
            { Weight::light, Weight::none, Weight::heavy, Weight::heavy },// 0x252A
            { Weight::heavy, Weight::none, Weight::heavy, Weight::heavy },// 0x252B
            { Weight::none, Weight::light, Weight::light, Weight::light },// 0x252C
            { Weight::none, Weight::light, Weight::light, Weight::heavy },// 0x252D
            { Weight::none, Weight::heavy, Weight::light, Weight::light },// 0x252E
            { Weight::none, Weight::heavy, Weight::light, Weight::heavy },// 0x252F
            { Weight::none, Weight::light, Weight::heavy, Weight::light },// 0x2530
            { Weight::none, Weight::light, Weight::heavy, Weight::heavy },// 0x2531
            { Weight::none, Weight::heavy, Weight::heavy, Weight::light },// 0x2532
            { Weight::none, Weight::heavy, Weight::heavy, Weight::heavy },// 0x2533
            { Weight::light, Weight::light, Weight::none, Weight::light },// 0x2534
            { Weight::light, Weight::light, Weight::none, Weight::heavy },// 0x2535
            { Weight::light, Weight::heavy, Weight::none, Weight::light },// 0x2536
            { Weight::light, Weight::heavy, Weight::none, Weight::heavy },// 0x2537
            { Weight::heavy, Weight::light, Weight::none, Weight::light },// 0x2538
            { Weight::heavy, Weight::light, Weight::none, Weight::heavy },// 0x2539
            { Weight::heavy, Weight::heavy, Weight::none, Weight::light },// 0x253A
            { Weight::heavy, Weight::heavy, Weight::none, Weight::heavy },// 0x253B
            { Weight::light, Weight::light, Weight::light, Weight::light },// 0x253C
            { Weight::light, Weight::light, Weight::light, Weight::heavy },// 0x253D
            { Weight::light, Weight::heavy, Weight::light, Weight::light },// 0x253E
            { Weight::light, Weight::heavy, Weight::light, Weight::heavy },// 0x253F
            { Weight::heavy, Weight::light, Weight::light, Weight::light },// 0x2540
            { Weight::light, Weight::light, Weight::heavy, Weight::light },// 0x2541
            { Weight::heavy, Weight::light, Weight::heavy, Weight::light },// 0x2542
            { Weight::heavy, Weight::light, Weight::light, Weight::heavy },// 0x2543
            { Weight::heavy, Weight::heavy, Weight::light, Weight::light },// 0x2544
            { Weight::light, Weight::light, Weight::heavy, Weight::heavy },// 0x2545
            { Weight::light, Weight::heavy, Weight::heavy, Weight::light },// 0x2546
            { Weight::light, Weight::heavy, Weight::light, Weight::heavy },// 0x2547
            { Weight::heavy, Weight::light, Weight::heavy, Weight::heavy },// 0x2548
            { Weight::heavy, Weight::heavy, Weight::heavy, Weight::light },// 0x2549
            { Weight::heavy, Weight::heavy, Weight::light, Weight::heavy },// 0x254A
            { Weight::heavy, Weight::heavy, Weight::heavy, Weight::heavy },// 0x254B
        }
    };

    /**
     * @brief Lookup table for half-line characters U+2574-U+257F.
     *
     * Index `i` corresponds to Unicode codepoint U+2574 + i. These characters
     * have a line segment on only one side of the cell centre.
     */
    static constexpr std::array<Lines, 12> halfTable {
        {
         { Weight::none, Weight::none, Weight::none, Weight::light },// 0x2574
            { Weight::light, Weight::none, Weight::none, Weight::none },// 0x2575
            { Weight::none, Weight::light, Weight::none, Weight::none },// 0x2576
            { Weight::none, Weight::none, Weight::light, Weight::none },// 0x2577
            { Weight::none, Weight::none, Weight::none, Weight::heavy },// 0x2578
            { Weight::heavy, Weight::none, Weight::none, Weight::none },// 0x2579
            { Weight::none, Weight::heavy, Weight::none, Weight::none },// 0x257A
            { Weight::none, Weight::none, Weight::heavy, Weight::none },// 0x257B
            { Weight::none, Weight::heavy, Weight::none, Weight::light },// 0x257C
            { Weight::light, Weight::none, Weight::heavy, Weight::none },// 0x257D
            { Weight::none, Weight::light, Weight::none, Weight::heavy },// 0x257E
            { Weight::heavy, Weight::none, Weight::light, Weight::none },// 0x257F
        }
    };
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
