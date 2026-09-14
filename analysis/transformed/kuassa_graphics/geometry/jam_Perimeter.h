/**
 * @file jam_Perimeter.h
 * @brief Progress-along-perimeter path builder — traces a polygon's outline
 *        up to a normalised distance, for animated/partial perimeter strokes.
 */

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @class Perimeter
 * @brief Builds a partial-outline juce::Path tracing a polygon's perimeter up
 *        to a normalised fraction of its total length.
 *
 * Constructed either from a Rotary::Angles point ring (rectangle + division
 * count) or from an explicit point sequence. getPath() walks the accumulated
 * line segments, stopping partway through the segment where the normalised
 * length falls, and records that stopping point as getCurrentPosition().
 */
class Perimeter
{
public:
    /** @brief Builds the perimeter from a Rotary::Angles point ring fitted to @p areaToDraw.
     *  @param areaToDraw       Rectangle the point ring is fitted to.
     *  @param normalisedValue  Fraction of the total perimeter length to trace, in [0, 1].
     *  @param division         Number of points in the ring (see Rotary::Angles::getPoints).
     *  @param lineThickness    Stroke width used by draw().
     *  @param buttLength       Fractional indent applied to the first and last points, relative
     *                          to the first segment's length. Zero disables the indent.
     *  @param rotationDegree   Rotation offset in degrees, forwarded to Rotary::Angles::getPoints.
     */
    Perimeter (const juce::Rectangle<float>& areaToDraw,
               float normalisedValue,
               int division,
               float lineThickness = 2.0f,
               float buttLength = 0.1f,
               float rotationDegree = 0.0f)
        : normal (normalisedValue)
        , stroke (lineThickness)

    {
        if (auto points { Rotary::Angles::getPoints (areaToDraw, division, rotationDegree) };
            points.size())
        {
            if (buttLength)
            {
                auto segmentLength { points.begin()->getDistanceFrom (*(points.begin() + 1)) };
                auto indent { buttLength * segmentLength };

                /** flip the indent direction if it's flipped 180 degree */
                indent = (rotationDegree == 180.0f ? -indent : indent);

                /** add extra two indent point at begin and end */
                auto begin { points.front().withX (points.front().getX() + indent) };
                points.insert (points.begin(), begin);

                auto end { points.back().withX (points.back().getX() - indent) };
                points.push_back (end);
            }

            /** calculate total perimeter length in a single flat line */
            for (auto p = points.begin() + 1; p < points.end(); ++p)
            {
                auto previous { *(p - 1) };
                auto distance { p->getDistanceFrom (previous) };
                perimeter += distance;

                /** and add each lines segment into a vector*/
                lines.push_back ({ previous, *p });
            }
        }
    }

    /** @brief Builds the perimeter directly from an explicit point sequence.
     *  @param points                    Points to connect, in order.
     *  @param normalisedValue           Fraction of the total perimeter length to trace, in [0, 1].
     *  @param shouldBeCounterClockwise  When true, reverses each segment and the overall winding order.
     *  @param lineThickness             Stroke width used by draw().
     *  @param rotationDegree            Unused by this overload; present for signature symmetry
     *                                   with the Rotary-ring constructor.
     */
    Perimeter (const std::vector<juce::Point<float>>& points,
               float normalisedValue = 0.0f,
               bool shouldBeCounterClockwise = false,
               float lineThickness = 2.0f,
               float rotationDegree = 0.0f)
        : normal (normalisedValue)
        , stroke (lineThickness)
    {
        /** calculate total perimeter length in a single flat line */
        for (auto p = points.begin() + 1; p < points.end(); ++p)
        {
            auto previous { *(p - 1) };
            auto distance { p->getDistanceFrom (previous) };
            perimeter += distance;

            /** and add each lines segment into a vector*/
            lines.push_back (shouldBeCounterClockwise ? juce::Line<float> (*p, previous)
                                                      : juce::Line<float> (previous, *p));
        }

        if (shouldBeCounterClockwise)
            std::reverse (lines.begin(), lines.end());
    }

    ~Perimeter()
    {
    }

    /** @brief Builds the traced-so-far outline path, up to the normalised length.
     *  @return Path made of line segments from the start point to the current position.
     *  @note Asserts if no line segments exist (empty or single-point input).
     */
    juce::Path getPath() noexcept
    {
        if (lines.size())
        {
            path.setUsingNonZeroWinding (true);

            /** map 0 to 1 normalised value to get the current position of the end
             of the lines and the current segment */

            float currentLength { Value::map (normal, 0.0f, perimeter) };

            /** currentSegment index start from -1 because the body of do...while loop is executed once before the condition is checked.*/
            int currentSegment { -1 };
            float segmentLength { 0.0f };

            do
            {
                if (currentLength > segmentLength)
                {
                    currentSegment++;
                    segmentLength += lines[currentSegment].getLength();

                    /** we clip minimum value of the distance to 0.0f, to ensure the line
                        don't exceed the segment length */
                    float distance { Value::clipMin (0.0f, segmentLength - currentLength) };
                    auto line { lines[currentSegment].withShortenedEnd (distance) };
                    currentPosition = line.getEnd();
                    path.addLineSegment (line, 1.0f);
                }

            }

            while (segmentLength < currentLength);
        }

        /** If you hit this assertion, there are no lines to draw */

        assert (lines.size() > 0);

        return path;
    }

    /** @brief Wraps getPath() in a juce::DrawablePath.
     *  @return A DrawablePath holding the traced-so-far outline.
     */
    juce::DrawablePath getDrawablePath() noexcept
    {
        juce::DrawablePath drawable;
        drawable.setPath (getPath());
        return drawable;
    }

    /** @brief Strokes the traced-so-far outline directly.
     *  @param g        Destination graphics context.
     *  @param colour   Stroke colour.
     *  @param opacity  Stroke opacity, 0-1.
     */
    void draw (juce::Graphics& g, juce::Colour colour, float opacity = 1.0f)
    {
        Drawable::stroke (g, getPath(), colour, stroke, opacity);
    }

    /** @brief Strokes the traced-so-far outline, scaled to fit @p area.
     *  @param g        Destination graphics context.
     *  @param area     Target area the path is scaled/positioned into.
     *  @param colour   Stroke colour.
     *  @param opacity  Stroke opacity, 0-1.
     */
    void draw (juce::Graphics& g, const juce::Rectangle<float>& area, juce::Colour colour, float opacity = 1.0f)
    {
        Drawable::stroke (g, area, getPath(), colour, stroke, opacity);
    }

    /** @brief Returns the point where the traced outline currently stops.
     *  @return The last computed stopping point (only valid after getPath() has run).
     */
    juce::Point<float> getCurrentPosition()
    {
        return currentPosition;
    }

private:
    juce::Path path;
    float normal;
    float stroke;
    std::vector<juce::Line<float>> lines;
    juce::Point<float> currentPosition;
    float perimeter { 0 };
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Perimeter)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
