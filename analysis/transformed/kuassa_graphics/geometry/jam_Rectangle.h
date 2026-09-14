/**
 * @file jam_Rectangle.h
 * @brief Pixel-domain rectangle utilities — perimeter decomposition into line segments.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/** @brief A sequence of juce::Point<ValueType>. */
template <typename ValueType>
using Points = std::vector<juce::Point<ValueType>>;

/** @brief A sequence of juce::Line<ValueType>. */
template <typename ValueType>
using Lines = std::vector<juce::Line<ValueType>>;

/**
 * @struct Rectangle
 * @brief Pixel-domain rectangle perimeter utilities.
 * @tparam ValueType  Coordinate scalar type (matches juce::Rectangle<ValueType>).
 */
template <typename ValueType>
struct Rectangle
{
    /** @brief Decomposes @p r's perimeter into its four edges as line segments.
     *  @param r                    Rectangle to decompose.
     *  @param isCounterClockwise   When true, reverses winding order (default clockwise
     *                              starting at the top edge).
     *  @return Four line segments: top, right, bottom, left (or reversed order).
     */
    static Lines<ValueType> getLines (const juce::Rectangle<ValueType>& r, bool isCounterClockwise = false)
    {
        Lines<ValueType> lines {
            { r.getTopLeft(), r.getTopRight() },
            { r.getTopRight(), r.getBottomRight() },
            { r.getBottomRight(), r.getBottomLeft() },
            { r.getBottomLeft(), r.getTopLeft() },
        };

        if (isCounterClockwise)
            std::reverse (lines.begin(), lines.end());

        return lines;
    }
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
