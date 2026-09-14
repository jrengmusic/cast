/**
 * @file jam_CellPoint.h
 * @brief Cell-coordinate point — mirrors juce::Point<int> API.
 *
 * `jam::Cell::Point` represents a position on the terminal cell grid.
 * Coordinates are in cell units (column, row), not pixels.
 *
 * The API mirrors `juce::Point<int>` so callers already familiar with JUCE
 * layout code face zero learning curve.  All methods are constexpr and noexcept.
 *
 * Included after `jam_Cell.h` via `jam_graphics.h`.
 * Do NOT include this file directly — include `jam_graphics.h` instead.
 */

#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/**
 * @struct Cell::Point
 * @brief A cell-grid coordinate.
 *
 * All coordinates are cell units.  x is the column, y is the row.
 * The struct is trivially copyable and passed by value (small type rule).
 */
struct Cell::Point
{
    /** @brief Column coordinate, in cell units. */
    int x { 0 };

    /** @brief Row coordinate, in cell units. */
    int y { 0 };

    //==========================================================================
    // Construction
    //==========================================================================

    /** Creates a point at the origin. */
    Point() = default;

    /** Creates a point at (col, row). */
    constexpr Point (Cell col, Cell row) noexcept
        : x { col.value }, y { row.value } {}

    //==========================================================================
    // Accessors
    //==========================================================================

    /** Returns the x coordinate as a Cell. */
    constexpr Cell getX() const noexcept { return Cell { x }; }

    /** Returns the y coordinate as a Cell. */
    constexpr Cell getY() const noexcept { return Cell { y }; }

    /** Returns true if this point is at the origin (0, 0). */
    constexpr bool isOrigin() const noexcept { return x == 0 and y == 0; }

    //==========================================================================
    // Mutable setters
    //==========================================================================

    /** Sets the x coordinate. */
    constexpr void setX (Cell col) noexcept { x = col.value; }

    /** Sets the y coordinate. */
    constexpr void setY (Cell row) noexcept { y = row.value; }

    /** Sets both coordinates. */
    constexpr void setXY (Cell col, Cell row) noexcept { x = col.value; y = row.value; }

    /** Adds dx to x and dy to y in place. */
    constexpr void addXY (Cell dx, Cell dy) noexcept { x += dx.value; y += dy.value; }

    //==========================================================================
    // Immutable builders
    //==========================================================================

    /** Returns a copy with x replaced by col. */
    constexpr Point withX (Cell col) const noexcept { return { col, Cell { y } }; }

    /** Returns a copy with y replaced by row. */
    constexpr Point withY (Cell row) const noexcept { return { Cell { x }, row }; }

    /** Returns a copy offset by (dx, dy). */
    constexpr Point translated (Cell dx, Cell dy) const noexcept
    {
        return { Cell { x + dx.value }, Cell { y + dy.value } };
    }

    //==========================================================================
    // Comparison
    //==========================================================================

    /** Returns true if both coordinates are equal. */
    constexpr bool operator== (Point other) const noexcept { return x == other.x and y == other.y; }

    /** Returns true if any coordinate differs. */
    constexpr bool operator!= (Point other) const noexcept { return not (*this == other); }

    //==========================================================================
    // Arithmetic
    //==========================================================================

    /** Returns the sum of two points. */
    constexpr Point operator+ (Point other) const noexcept { return { Cell { x + other.x }, Cell { y + other.y } }; }

    /** Returns the difference of two points. */
    constexpr Point operator- (Point other) const noexcept { return { Cell { x - other.x }, Cell { y - other.y } }; }

    /** Adds other to this point in place. */
    constexpr Point& operator+= (Point other) noexcept { x += other.x; y += other.y; return *this; }

    /** Subtracts other from this point in place. */
    constexpr Point& operator-= (Point other) noexcept { x -= other.x; y -= other.y; return *this; }

    /** Returns the negation of this point. */
    constexpr Point operator-() const noexcept { return { Cell { -x }, Cell { -y } }; }

    //==========================================================================
    // Pack / unpack
    //==========================================================================

    /** Packs both coordinates into a single int: high 16 bits = x, low 16 bits = y. */
    constexpr int pack() const noexcept { return (x << 16) | (y & 0xFFFF); }

    /** Reconstructs a Point from a packed int produced by pack(). */
    static constexpr Point unpack (int v) noexcept
    {
        return { Cell { v >> 16 }, Cell { v & 0xFFFF } };
    }

    //==========================================================================
    // Pixel conversion
    //==========================================================================

    /** Converts a pixel position to a cell Point by integer division.
     *  @param pixel       Pixel position to convert.
     *  @param cellWidth   Cell width in pixels (must be > 0).
     *  @param cellHeight  Cell height in pixels (must be > 0).
     */
    static Point fromPixel (juce::Point<int> pixel, int cellWidth, int cellHeight) noexcept
    {
        return { Cell { (cellWidth  > 0) ? pixel.x / cellWidth  : 0 },
                 Cell { (cellHeight > 0) ? pixel.y / cellHeight : 0 } };
    }

    /** Converts cell coordinates to a pixel position by multiplication.
     *  @param cellWidth   Cell width in pixels.
     *  @param cellHeight  Cell height in pixels.
     */
    juce::Point<int> toPixel (int cellWidth, int cellHeight) const noexcept
    {
        return { x * cellWidth, y * cellHeight };
    }
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
