/**
 * @file jam_Cell.h
 * @brief Strongly-typed grid-coordinate scalar — one dimension of a Point.
 *
 * `jam::Cell` is the coordinate scalar for the terminal cell grid.  It
 * prevents accidental mixing of pixel and cell coordinates by being a
 * distinct type from plain `int`.
 *
 * Use `jam::Cell::Point` and `jam::Cell::Rectangle` for two-dimensional
 * grid positions and regions.  The character atom (`jam::AttributedChar`) lives
 * in `jam_AttributedChar.h`.
 */

#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/**
 * @struct Cell
 * @brief Strongly-typed cell-grid coordinate scalar — one dimension of a Point.
 *
 * Prevents accidental mixing of pixel and cell coordinates.  All arithmetic
 * and comparison operators are constexpr and noexcept.
 *
 * `Cell::Point` and `Cell::Rectangle` are nested types defined in
 * `jam_CellPoint.h` and `jam_CellRectangle.h` respectively.
 */
struct Cell
{
    /** @brief The underlying grid-coordinate scalar. */
    int value { 0 };

    /** @brief Constructs a Cell wrapping @p v.
     *  @param v  The grid-coordinate scalar value.
     */
    constexpr explicit Cell (int v) noexcept : value { v } {}

    // Forward declarations — defined in jam_CellPoint.h and jam_CellRectangle.h
    struct Point;
    struct Rectangle;

    //==========================================================================
    // Arithmetic

    constexpr Cell operator+ (Cell other)  const noexcept { return Cell { value + other.value }; }
    constexpr Cell operator- (Cell other)  const noexcept { return Cell { value - other.value }; }
    constexpr Cell operator* (int factor)  const noexcept { return Cell { value * factor }; }
    constexpr Cell operator/ (int divisor) const noexcept { return Cell { value / divisor }; }

    constexpr Cell& operator+= (Cell other) noexcept { value += other.value; return *this; }
    constexpr Cell& operator-= (Cell other) noexcept { value -= other.value; return *this; }

    constexpr Cell& operator++ ()    noexcept { ++value; return *this; }
    constexpr Cell  operator++ (int) noexcept { Cell tmp { *this }; ++value; return tmp; }
    constexpr Cell& operator-- ()    noexcept { --value; return *this; }
    constexpr Cell  operator-- (int) noexcept { Cell tmp { *this }; --value; return tmp; }

    constexpr Cell operator% (Cell other)  const noexcept { return Cell { value % other.value }; }
    constexpr Cell operator% (int divisor) const noexcept { return Cell { value % divisor }; }

    //==========================================================================
    // Comparison

    constexpr bool operator== (Cell other) const noexcept { return value == other.value; }
    constexpr bool operator!= (Cell other) const noexcept { return value != other.value; }
    constexpr bool operator<  (Cell other) const noexcept { return value <  other.value; }
    constexpr bool operator<= (Cell other) const noexcept { return value <= other.value; }
    constexpr bool operator>  (Cell other) const noexcept { return value >  other.value; }
    constexpr bool operator>= (Cell other) const noexcept { return value >= other.value; }
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam

namespace jam::literals
{
/** @brief User-defined literal constructing a jam::Cell, e.g. @c 4_cell.
 *  @param v  The grid-coordinate scalar value.
 *  @return   A Cell wrapping @p v.
 */
constexpr jam::Cell operator"" _cell (unsigned long long v) noexcept
{
    return jam::Cell { static_cast<int> (v) };
}
} // namespace jam::literals

using namespace jam::literals;

// cell = jam::Cell — strongly-typed coordinate scalar.
// Use jam::Cell::Point and jam::Cell::Rectangle
// for grid-coordinate types.
using cell = jam::Cell;
