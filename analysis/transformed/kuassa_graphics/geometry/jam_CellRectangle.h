/**
 * @file jam_CellRectangle.h
 * @brief Cell-coordinate rectangle — mirrors juce::Rectangle<int> API.
 *
 * `jam::Cell::Rectangle` represents an axis-aligned region of the terminal
 * cell grid.  Coordinates are in cell units (column, row), not pixels.
 *
 * The API mirrors `juce::Rectangle<int>` so callers already familiar with JUCE
 * layout code face zero learning curve.  All methods are constexpr and noexcept
 * where possible.
 *
 * Included after `jam_CellPoint.h` via `jam_graphics.h`.
 * Do NOT include this file directly — include `jam_graphics.h` instead.
 */

#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/**
 * @struct Cell::Rectangle
 * @brief An axis-aligned cell-grid rectangle.
 *
 * All coordinates are cell units.  x is the column, y is the row.
 * The struct is trivially copyable and passed by value (small type rule).
 */
struct Cell::Rectangle
{
    /** @brief Column of the left edge, in cell units. */
    int x      { 0 };

    /** @brief Row of the top edge, in cell units. */
    int y      { 0 };

    /** @brief Width, in cell units. */
    int width  { 0 };

    /** @brief Height, in cell units. */
    int height { 0 };

    //==========================================================================
    // Construction
    //==========================================================================

    /** Creates a zero-size rectangle at the origin. */
    Rectangle() = default;

    /** Creates a rectangle at (x, y) with the given dimensions. */
    constexpr Rectangle (Cell x, Cell y, Cell w, Cell h) noexcept
        : x { x.value }, y { y.value }, width { w.value }, height { h.value } {}

    /** Creates a rectangle at (0, 0) with the given dimensions. */
    constexpr Rectangle (Cell w, Cell h) noexcept
        : width { w.value }, height { h.value } {}

    //==========================================================================
    // Accessors
    //==========================================================================

    /** Returns the x (column) coordinate. */
    constexpr Cell getX()      const noexcept { return Cell { x }; }

    /** Returns the y (row) coordinate. */
    constexpr Cell getY()      const noexcept { return Cell { y }; }

    /** Returns the width in cells. */
    constexpr Cell getWidth()  const noexcept { return Cell { width }; }

    /** Returns the height in cells. */
    constexpr Cell getHeight() const noexcept { return Cell { height }; }

    /** Returns the column one past the right edge: x + width. */
    constexpr Cell getRight()  const noexcept { return Cell { x + width }; }

    /** Returns the row one past the bottom edge: y + height. */
    constexpr Cell getBottom() const noexcept { return Cell { y + height }; }

    /** Returns the top-left corner as a Point. */
    constexpr Point getPosition()    const noexcept { return { Cell { x }, Cell { y } }; }

    /** Returns the top-left corner. */
    constexpr Point getTopLeft()     const noexcept { return { Cell { x }, Cell { y } }; }

    /** Returns the top-right corner. */
    constexpr Point getTopRight()    const noexcept { return { Cell { x + width }, Cell { y } }; }

    /** Returns the bottom-left corner. */
    constexpr Point getBottomLeft()  const noexcept { return { Cell { x }, Cell { y + height } }; }

    /** Returns the bottom-right corner. */
    constexpr Point getBottomRight() const noexcept { return { Cell { x + width }, Cell { y + height } }; }

    /** Returns the centre point (integer truncation). */
    constexpr Point getCentre()  const noexcept { return { Cell { x + width / 2 }, Cell { y + height / 2 } }; }

    /** Returns the x coordinate of the centre (integer truncation). */
    constexpr Cell getCentreX()  const noexcept { return Cell { x + width / 2 }; }

    /** Returns the y coordinate of the centre (integer truncation). */
    constexpr Cell getCentreY()  const noexcept { return Cell { y + height / 2 }; }

    //==========================================================================
    // State
    //==========================================================================

    /** Returns true when either dimension is zero or negative. */
    constexpr bool isEmpty()  const noexcept { return width <= 0 or height <= 0; }

    /** Returns true when both dimensions are strictly positive. */
    constexpr bool isValid()  const noexcept { return width > 0 and height > 0; }

    /** Returns the area in cells. */
    constexpr int  getArea()  const noexcept { return width * height; }

    constexpr bool operator== (const Rectangle& other) const noexcept
    {
        return x == other.x and y == other.y
           and width == other.width and height == other.height;
    }

    constexpr bool operator!= (const Rectangle& other) const noexcept
    {
        return not (*this == other);
    }

    //==========================================================================
    // Containment
    //==========================================================================

    /** Returns true when the cell at (px, py) is inside this rectangle. */
    constexpr bool contains (Cell px, Cell py) const noexcept
    {
        return px.value >= x and py.value >= y
           and px.value < x + width and py.value < y + height;
    }

    /** Returns true when other is fully contained within this rectangle. */
    constexpr bool contains (const Rectangle& other) const noexcept
    {
        return other.x >= x and other.y >= y
           and other.getRight().value  <= getRight().value
           and other.getBottom().value <= getBottom().value;
    }

    //==========================================================================
    // Set operations
    //==========================================================================

    /** Returns true when this rectangle overlaps with other. */
    constexpr bool intersects (const Rectangle& other) const noexcept
    {
        return x < other.x + other.width
           and y < other.y + other.height
           and x + width  > other.x
           and y + height > other.y;
    }

    /** Returns the overlapping region of this rectangle and other. */
    constexpr Rectangle getIntersection (const Rectangle& other) const noexcept
    {
        const int nx { x > other.x ? x : other.x };
        const int ny { y > other.y ? y : other.y };
        const int nr { (x + width  < other.x + other.width)  ? x + width  : other.x + other.width };
        const int nb { (y + height < other.y + other.height) ? y + height : other.y + other.height };
        const int nw { nr - nx };
        const int nh { nb - ny };
        return { Cell { nx }, Cell { ny }, Cell { nw > 0 ? nw : 0 }, Cell { nh > 0 ? nh : 0 } };
    }

    /** Returns the smallest rectangle containing both this and other. */
    constexpr Rectangle getUnion (const Rectangle& other) const noexcept
    {
        const int nx { x < other.x ? x : other.x };
        const int ny { y < other.y ? y : other.y };
        const int nr { (x + width  > other.x + other.width)  ? x + width  : other.x + other.width };
        const int nb { (y + height > other.y + other.height) ? y + height : other.y + other.height };
        return { Cell { nx }, Cell { ny }, Cell { nr - nx }, Cell { nb - ny } };
    }

    /** Returns a copy clamped so it fits within area. */
    constexpr Rectangle constrainedWithin (const Rectangle& area) const noexcept
    {
        const int cx { x < area.x ? area.x : (x + width  > area.x + area.width  ? area.x + area.width  - width  : x) };
        const int cy { y < area.y ? area.y : (y + height > area.y + area.height ? area.y + area.height - height : y) };
        const int cw { width  < area.width  ? width  : area.width };
        const int ch { height < area.height ? height : area.height };
        return { Cell { cx }, Cell { cy }, Cell { cw }, Cell { ch } };
    }

    //==========================================================================
    // Mutable setters
    //==========================================================================

    /** Sets the x coordinate. */
    constexpr void setX (Cell newX) noexcept { x = newX.value; }

    /** Sets the y coordinate. */
    constexpr void setY (Cell newY) noexcept { y = newY.value; }

    /** Sets both position coordinates. */
    constexpr void setPosition (Cell newX, Cell newY) noexcept { x = newX.value; y = newY.value; }

    /** Sets the position from a Point. */
    constexpr void setPosition (Point p) noexcept { x = p.x; y = p.y; }

    /** Sets the width. */
    constexpr void setWidth (Cell newW) noexcept { width = newW.value; }

    /** Sets the height. */
    constexpr void setHeight (Cell newH) noexcept { height = newH.value; }

    /** Sets both size dimensions. */
    constexpr void setSize (Cell newW, Cell newH) noexcept { width = newW.value; height = newH.value; }

    /** Sets all four fields. */
    constexpr void setBounds (Cell bx, Cell by, Cell bw, Cell bh) noexcept
    {
        x = bx.value; y = by.value; width = bw.value; height = bh.value;
    }

    /** Moves the left edge to newLeft, preserving the right edge. */
    constexpr void setLeft (Cell newLeft) noexcept
    {
        width += x - newLeft.value;
        x      = newLeft.value;
    }

    /** Moves the top edge to newTop, preserving the bottom edge. */
    constexpr void setTop (Cell newTop) noexcept
    {
        height += y - newTop.value;
        y       = newTop.value;
    }

    /** Moves the right edge to newRight, preserving the left edge. */
    constexpr void setRight (Cell newRight) noexcept { width  = newRight.value - x; }

    /** Moves the bottom edge to newBottom, preserving the top edge. */
    constexpr void setBottom (Cell newBottom) noexcept { height = newBottom.value - y; }

    //==========================================================================
    // Fluent builders — each returns a new Rectangle, does not mutate self
    //==========================================================================

    constexpr Rectangle withX      (Cell newX) const noexcept { return { newX,          Cell { y }, Cell { width }, Cell { height } }; }
    constexpr Rectangle withY      (Cell newY) const noexcept { return { Cell { x }, newY,          Cell { width }, Cell { height } }; }
    constexpr Rectangle withWidth  (Cell newW) const noexcept { return { Cell { x }, Cell { y }, newW,              Cell { height } }; }
    constexpr Rectangle withHeight (Cell newH) const noexcept { return { Cell { x }, Cell { y }, Cell { width }, newH              }; }

    constexpr Rectangle withPosition (Cell newX, Cell newY) const noexcept
    {
        return { newX, newY, Cell { width }, Cell { height } };
    }

    constexpr Rectangle withSize (Cell newW, Cell newH) const noexcept
    {
        return { Cell { x }, Cell { y }, newW, newH };
    }

    /** Returns a copy with the position reset to (0, 0). */
    constexpr Rectangle withZeroOrigin() const noexcept
    {
        return { Cell { 0 }, Cell { 0 }, Cell { width }, Cell { height } };
    }

    /** Returns a copy with the left edge moved to newLeft, right edge preserved. */
    constexpr Rectangle withLeft (Cell newLeft) const noexcept
    {
        return { newLeft, Cell { y }, Cell { x + width - newLeft.value }, Cell { height } };
    }

    /** Returns a copy with the top edge moved to newTop, bottom edge preserved. */
    constexpr Rectangle withTop (Cell newTop) const noexcept
    {
        return { Cell { x }, newTop, Cell { width }, Cell { y + height - newTop.value } };
    }

    /** Returns a copy with the right edge moved to newRight, left edge preserved. */
    constexpr Rectangle withRight (Cell newRight) const noexcept
    {
        return { Cell { x }, Cell { y }, Cell { newRight.value - x }, Cell { height } };
    }

    /** Returns a copy with the bottom edge moved to newBottom, top edge preserved. */
    constexpr Rectangle withBottom (Cell newBottom) const noexcept
    {
        return { Cell { x }, Cell { y }, Cell { width }, Cell { newBottom.value - y } };
    }

    /** Returns a copy with the right edge set by adjusting width. */
    constexpr Rectangle withRightX (Cell newRight) const noexcept
    {
        return withRight (newRight);
    }

    /** Returns a copy with the bottom edge set by adjusting height. */
    constexpr Rectangle withBottomY (Cell newBottom) const noexcept
    {
        return withBottom (newBottom);
    }

    /** Returns a copy shifted by (dx, dy). */
    constexpr Rectangle translated (Cell dx, Cell dy) const noexcept
    {
        return { Cell { x + dx.value }, Cell { y + dy.value }, Cell { width }, Cell { height } };
    }

    /**
     * Returns a copy grown by dx cells on each horizontal side and dy cells
     * on each vertical side.
     */
    constexpr Rectangle expanded (Cell dx, Cell dy) const noexcept
    {
        return { Cell { x - dx.value }, Cell { y - dy.value }, Cell { width + dx.value * 2 }, Cell { height + dy.value * 2 } };
    }

    /** Returns a copy uniformly grown by d cells on all sides. */
    constexpr Rectangle expanded (Cell d) const noexcept
    {
        return expanded (d, d);
    }

    /**
     * Returns a copy shrunk by dx cells on each horizontal side and dy cells
     * on each vertical side.  Result may have negative dimensions if the
     * reduction exceeds the rectangle size — callers that need a safe clamp
     * should check isEmpty() on the result.
     */
    constexpr Rectangle reduced (Cell dx, Cell dy) const noexcept
    {
        return { Cell { x + dx.value }, Cell { y + dy.value }, Cell { width - dx.value * 2 }, Cell { height - dy.value * 2 } };
    }

    /** Returns a copy uniformly shrunk by d cells on all sides. */
    constexpr Rectangle reduced (Cell d) const noexcept
    {
        return reduced (d, d);
    }

    /** Returns a copy with the left edge moved right by amount. */
    constexpr Rectangle withTrimmedLeft (Cell amount) const noexcept
    {
        return { Cell { x + amount.value }, Cell { y }, Cell { width - amount.value }, Cell { height } };
    }

    /** Returns a copy with the right edge moved left by amount. */
    constexpr Rectangle withTrimmedRight (Cell amount) const noexcept
    {
        return { Cell { x }, Cell { y }, Cell { width - amount.value }, Cell { height } };
    }

    /** Returns a copy with the top edge moved down by amount. */
    constexpr Rectangle withTrimmedTop (Cell amount) const noexcept
    {
        return { Cell { x }, Cell { y + amount.value }, Cell { width }, Cell { height - amount.value } };
    }

    /** Returns a copy with the bottom edge moved up by amount. */
    constexpr Rectangle withTrimmedBottom (Cell amount) const noexcept
    {
        return { Cell { x }, Cell { y }, Cell { width }, Cell { height - amount.value } };
    }

    //==========================================================================
    // Translation operators
    //==========================================================================

    /** Returns this rectangle translated by a Point offset. */
    constexpr Rectangle operator+ (Point offset) const noexcept
    {
        return { Cell { x + offset.x }, Cell { y + offset.y }, Cell { width }, Cell { height } };
    }

    /** Returns this rectangle translated by the negation of a Point offset. */
    constexpr Rectangle operator- (Point offset) const noexcept
    {
        return { Cell { x - offset.x }, Cell { y - offset.y }, Cell { width }, Cell { height } };
    }

    /** Translates this rectangle in place by a Point offset. */
    constexpr Rectangle& operator+= (Point offset) noexcept
    {
        x += offset.x;
        y += offset.y;
        return *this;
    }

    /** Translates this rectangle in place by the negation of a Point offset. */
    constexpr Rectangle& operator-= (Point offset) noexcept
    {
        x -= offset.x;
        y -= offset.y;
        return *this;
    }

    //==========================================================================
    // Slicing — mutates self, returns the removed strip
    //==========================================================================

    /**
     * Removes a strip of the given height from the top of this rectangle.
     * Self shrinks by amount rows.  Returns the removed strip.
     */
    Rectangle removeFromTop (Cell amount) noexcept
    {
        const Rectangle strip { Cell { x }, Cell { y }, Cell { width }, amount };
        y      += amount.value;
        height -= amount.value;
        return strip;
    }

    /**
     * Removes a strip of the given height from the bottom of this rectangle.
     * Self shrinks by amount rows.  Returns the removed strip.
     */
    Rectangle removeFromBottom (Cell amount) noexcept
    {
        height -= amount.value;
        return { Cell { x }, Cell { y + height }, Cell { width }, amount };
    }

    /**
     * Removes a strip of the given width from the left of this rectangle.
     * Self shrinks by amount columns.  Returns the removed strip.
     */
    Rectangle removeFromLeft (Cell amount) noexcept
    {
        const Rectangle strip { Cell { x }, Cell { y }, amount, Cell { height } };
        x     += amount.value;
        width -= amount.value;
        return strip;
    }

    /**
     * Removes a strip of the given width from the right of this rectangle.
     * Self shrinks by amount columns.  Returns the removed strip.
     */
    Rectangle removeFromRight (Cell amount) noexcept
    {
        width -= amount.value;
        return { Cell { x + width }, Cell { y }, amount, Cell { height } };
    }

    //==========================================================================
    // Pack / unpack
    //==========================================================================

    /**
     * Packs all four fields into a single int64_t.
     * Layout: x [63:48] | y [47:32] | width [31:16] | height [15:0].
     * Fields are treated as 16-bit values — values outside [-32768, 65535] truncate.
     */
    constexpr int64_t pack() const noexcept
    {
        return (int64_t (x) << 48)
             | ((int64_t (y)      & 0xFFFF) << 32)
             | ((int64_t (width)  & 0xFFFF) << 16)
             |  (int64_t (height) & 0xFFFF);
    }

    /** Reconstructs a Rectangle from a packed int64_t produced by pack(). */
    static constexpr Rectangle unpack (int64_t v) noexcept
    {
        return { Cell { static_cast<int> (v >> 48) },
                 Cell { static_cast<int> ((v >> 32) & 0xFFFF) },
                 Cell { static_cast<int> ((v >> 16) & 0xFFFF) },
                 Cell { static_cast<int> (v & 0xFFFF) } };
    }

    //==========================================================================
    // Pixel conversion
    //==========================================================================

    /**
     * Converts a pixel rectangle to a cell Rectangle by floor division.
     * @param pixel       Pixel rectangle to convert.
     * @param cellWidth   Cell width in pixels (must be > 0).
     * @param cellHeight  Cell height in pixels (must be > 0).
     */
    static Rectangle fromPixel (juce::Rectangle<int> pixel, int cellWidth, int cellHeight) noexcept
    {
        Rectangle r;
        if (cellWidth > 0 and cellHeight > 0)
        {
            r.x      = pixel.getX()      / cellWidth;
            r.y      = pixel.getY()      / cellHeight;
            r.width  = pixel.getWidth()  / cellWidth;
            r.height = pixel.getHeight() / cellHeight;
        }
        return r;
    }

    /**
     * Converts a pixel rectangle to a cell Rectangle using ceiling division for
     * width and height, floor division for x and y.
     * @param pixel       Pixel rectangle to convert.
     * @param cellWidth   Cell width in pixels (must be > 0).
     * @param cellHeight  Cell height in pixels (must be > 0).
     */
    static Rectangle fromPixelCeiling (juce::Rectangle<int> pixel, int cellWidth, int cellHeight) noexcept
    {
        Rectangle r;
        if (cellWidth > 0 and cellHeight > 0)
        {
            r.x      = pixel.getX()      / cellWidth;
            r.y      = pixel.getY()      / cellHeight;
            r.width  = (pixel.getWidth()  + cellWidth  - 1) / cellWidth;
            r.height = (pixel.getHeight() + cellHeight - 1) / cellHeight;
        }
        return r;
    }

    /** Converts cell coordinates to a pixel rectangle by multiplication. */
    juce::Rectangle<int> toPixel (int cellWidth, int cellHeight) const noexcept
    {
        return { x * cellWidth, y * cellHeight, width * cellWidth, height * cellHeight };
    }

    //==========================================================================
    // Scale
    //==========================================================================

    /**
     * Computes scale factor to fit inner within outer (pixel domain).
     * Portrait = width <= height.  If orientations match, divide outer's
     * constraining dimension by inner's parallel dimension; if they differ,
     * divide by inner's perpendicular dimension.
     */
    static float getRelativeScale (juce::Rectangle<int> outer, juce::Rectangle<int> inner, float maxScale = 1.0f) noexcept
    {
        const float outWidth  { static_cast<float> (outer.getWidth()) };
        const float outHeight { static_cast<float> (outer.getHeight()) };
        const float inWidth   { static_cast<float> (inner.getWidth()) };
        const float inHeight  { static_cast<float> (inner.getHeight()) };

        const bool outerPortrait { outer.getWidth() <= outer.getHeight() };
        const bool innerPortrait { inner.getWidth() <= inner.getHeight() };
        const bool isSameOrientation { outerPortrait == innerPortrait };

        const float constraint { outerPortrait ? outWidth : outHeight };
        const float parallel   { outerPortrait ? inHeight : inWidth };
        const float perpendicular { outerPortrait ? inWidth : inHeight };

        return (constraint * maxScale) / (isSameOrientation ? parallel : perpendicular);
    }
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
