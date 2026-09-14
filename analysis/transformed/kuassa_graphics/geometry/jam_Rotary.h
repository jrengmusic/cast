/**
 * @file jam_Rotary.h
 * @brief Rotary-knob marking geometry — arcs, ticks, dots, and polygon marks
 *        arranged around a centre, with SVG export.
 *
 * `jam::Rotary::Base` establishes the centre/radius/arc/rotation from either
 * an explicit centre+diameter or a rectangle fitted to a normalised value.
 * The `Arc`, `Dash`, `Line`, `Dot`, `Polygon` subclasses (each with a `Sub`
 * variant carrying secondary marks) draw a specific mark style; all can
 * render an accompanying number ring via `Base::drawNumbers`.
 */

#pragma once
namespace jam
{
/*____________________________________________________________________________*/

/** @brief Returns the line segment from @p inRadius to @p outRadius at @p angle
 *         around @p centre, in radians.
 *  @tparam ValueType  Coordinate scalar type.
 *  @param centre     Centre of the circle.
 *  @param inRadius   Radius of the segment's start point.
 *  @param outRadius  Radius of the segment's end point.
 *  @param angle      Angle in radians, measured per juce::Point::getPointOnCircumference.
 *  @return The line from the inner to the outer point.
 */
template <typename ValueType>
juce::Line<ValueType> centredLine (const juce::Point<ValueType>& centre,
                                   ValueType inRadius,
                                   ValueType outRadius,
                                   ValueType angle)
{
    return juce::Line<ValueType> (centre.getPointOnCircumference (inRadius, angle),
                                  centre.getPointOnCircumference (outRadius, angle));
}

/**
 * @struct Rotary
 * @brief Namespace-like container for rotary-knob mark geometry and drawing classes.
 */
struct Rotary
{
    /**
     * @struct Angles
     * @brief Sequence of evenly spaced angles (radians) spanning [-arc, arc] about
     *        a rotation offset, plus static helpers converting angle sets to
     *        drawable geometry.
     */
    struct Angles : public std::vector<float>
    {
        /** @brief Fills the vector with @p div angles evenly spaced across [-arc, arc], offset by @p rotation.
         *  @param div       Number of angles to generate.
         *  @param arc       Half-span in radians.
         *  @param rotation  Rotation offset in radians, added to every angle.
         */
        Angles (int div, float arc, float rotation);

        /** @brief Returns @p div points on @p area's inscribed circle (or the 4 corners of @p area when div == 4).
         *  @param area            Rectangle whose centre and radius the points are placed relative to.
         *  @param div             Number of points (4 is treated as the rectangle's corners).
         *  @param rotationDegree  Rotation offset in degrees.
         *  @return The computed points.
         */
        static Points<float> getPoints (const juce::Rectangle<float>& area, int div, float rotationDegree = 0.0f);

        /** @brief Connects consecutive getPoints() results into line segments.
         *  @param area            Rectangle whose centre and radius the points are placed relative to.
         *  @param div             Number of points.
         *  @param shouldBeClosed  When true, adds a final segment closing the loop back to the first point.
         *  @param rotationDegree  Rotation offset in degrees.
         *  @return The connecting line segments.
         */
        static Lines<float> getLines (const juce::Rectangle<float>& area, int div, bool shouldBeClosed = true, float rotationDegree = 0.0f);

        /** @brief Builds a stroked-line juce::Path from getLines().
         *  @param area            Rectangle whose centre and radius the points are placed relative to.
         *  @param div             Number of points.
         *  @param shouldBeClosed  When true, closes the loop.
         *  @param rotationDegree  Rotation offset in degrees.
         *  @param lineThickness   Thickness applied to each segment via juce::Path::addLineSegment.
         *  @return The constructed path.
         */
        static juce::Path getLinePath (const juce::Rectangle<float>& area, int div, bool shouldBeClosed = true, float rotationDegree = 0.0f, float lineThickness = 2.0f);

        /** @brief Builds and fills the getLinePath() result.
         *  @param g               Destination graphics context.
         *  @param colour          Fill colour.
         *  @param area            Rectangle whose centre and radius the points are placed relative to.
         *  @param div             Number of points.
         *  @param shouldBeClosed  When true, closes the loop.
         *  @param rotationDegree  Rotation offset in degrees.
         *  @param lineThickness   Thickness applied to each segment.
         *  @param opacity         Fill opacity, 0-1.
         */
        static void drawLines (juce::Graphics& g, juce::Colour colour, const juce::Rectangle<float>& area, int div, bool shouldBeClosed = true, float rotationDegree = 0.0f, float lineThickness = 2.0f, float opacity = 1.0f);

        /** @brief Lays out @p numbers as glyphs around @p area's rim and returns the combined outline path.
         *
         *  Each entry of @p numbers is a quoted CSV token
         *  `"number,isShowing,isUsingCustomText,customText"`; entries with
         *  `isShowing` false are skipped.
         *
         *  @param numbers          CSV-encoded per-mark number descriptors (see above).
         *  @param font             Font used to lay out each number's glyphs.
         *  @param length           Outer radius of the number ring, measured from the mark geometry.
         *  @param space            Additional radial spacing between the mark ring and the numbers.
         *  @param baseline         Baseline offset as a multiple of font height.
         *  @param shouldDrawInside When true, numbers are placed toward the centre rather than outward.
         *  @param area             Rectangle whose centre the numbers are placed around.
         *  @param div              Number of angle slots (matches the mark ring's division).
         *  @param arc              Half-span in radians.
         *  @param rotation         Rotation offset in radians.
         *  @return Combined glyph outline path for all shown numbers.
         */
        static juce::Path getNumbersPath (const juce::StringArray& numbers,
                                          const juce::Font& font,
                                          float length,
                                          float space,
                                          float baseline,
                                          bool shouldDrawInside,
                                          const juce::Rectangle<float>& area,
                                          int div,
                                          float arc,
                                          float rotation = 0.0f);

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Angles)
    };

    /**
     * @struct SubAngles
     * @brief Sequence of secondary (between-tick) angles for the `Sub`-suffixed
     *        mark classes, derived the same way as Angles but at @p sub subdivisions
     *        per main division, excluding angles that coincide with a main tick.
     */
    struct SubAngles : public std::vector<float>
    {
        /** @brief Fills the vector with sub-division angles between each of @p div main angles.
         *  @param div       Number of main divisions.
         *  @param sub       Number of sub-divisions between each pair of main divisions.
         *  @param arc       Half-span in radians.
         *  @param rotation  Rotation offset in radians.
         */
        SubAngles (int div, int sub, float arc, float rotation);

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubAngles)
    };

    /*____________________________________________________________________________*/

    /** @brief Selects whether Base::getSVG renders marks as filled shapes or stroked outlines. */
    enum class SVGtype
    {
        fill,
        stroke,
    };

    /**
     * @class Base
     * @brief Common centre/radius/arc/rotation state and SVG export for all rotary mark classes.
     */
    class Base
    {
    public:
        /** @brief Constructs from an explicit centre and diameter.
         *  @param centrePoint         Centre of the rotary.
         *  @param diameter            Overall diameter; radius is half of this.
         *  @param arcDegree           Half-span of the arc, in degrees.
         *  @param rotationDegree      Rotation offset, in degrees.
         *  @param shouldWriteSVGAs    Whether getSVG() renders marks filled or stroked.
         */
        Base (const juce::Point<float>& centrePoint,
              float diameter,
              float arcDegree = 150.0f,
              float rotationDegree = 0.0f,
              const SVGtype shouldWriteSVGAs = SVGtype::stroke);

        /** @brief Constructs centred within @p areaToDraw, with arc and rotation scaled by @p normalisedValue.
         *  @param areaToDraw          Rectangle the rotary is centred within.
         *  @param normalisedValue     Value in [0, 1] mapping to arc span [0, arcDegree] and rotation [-arcDegree, 0].
         *  @param arcDegree           Maximum half-span of the arc, in degrees.
         *  @param rotationDegree      Unused by this overload; present for signature symmetry with the other constructor.
         *  @param shouldWriteSVGAs    Whether getSVG() renders marks filled or stroked.
         */
        Base (const juce::Rectangle<float>& areaToDraw,
              float normalisedValue = 1.0f,
              float arcDegree = 150.0f,
              float rotationDegree = 0.0f,
              const SVGtype shouldWriteSVGAs = SVGtype::stroke);

        virtual ~Base() {}

        /** @brief Rebuilds and paints this rotary's mark geometry.
         *  @param g  Destination graphics context.
         */
        virtual void draw (juce::Graphics& g) = 0;

        /** @brief Lays out and fills a number ring around the current mark geometry.
         *  @param g                 Destination graphics context.
         *  @param numbers           CSV-encoded per-mark number descriptors (see Angles::getNumbersPath).
         *  @param div               Number of angle slots.
         *  @param baseline          Baseline offset as a multiple of font height.
         *  @param position          Radial spacing between the mark ring and the numbers.
         *  @param shouldDrawInside  When true, numbers are placed toward the centre rather than outward.
         */
        void drawNumbers (juce::Graphics& g,
                          const juce::StringArray& numbers,
                          int div,
                          float baseline,
                          float position,
                          bool shouldDrawInside);

        /** @brief Serializes the current mark, sub-mark, number, and centre-marker paths as an SVG group.
         *  @param colour        Colour applied to the main mark path.
         *  @param numberColour  Colour applied to the number path.
         *  @param stroke        Stroke width for the main mark path (stroke SVGtype only).
         *  @param subStroke     Stroke width for the secondary mark path (stroke SVGtype only).
         *  @return SVG markup for the marks, numbers, and a magenta centre-cross guide.
         */
        juce::String getSVG (const juce::Colour& colour,
                             const juce::Colour& numberColour,
                             float stroke,
                             float subStroke) const noexcept;

    protected:
        /** @brief Returns half of @p area's constraining dimension (height if portrait, width if landscape), minus @p offset.
         *  @param area    Rectangle to measure.
         *  @param offset  Amount subtracted from the computed radius.
         *  @return The computed radius.
         */
        float getRadius (const juce::Rectangle<float>& area, float offset = 0) const noexcept;

        /** @brief Centre of the rotary. */
        juce::Point<float> centre;

        /** @brief Radius of the primary mark ring. */
        float radius;

        /** @brief Half-span of the arc, in radians. */
        float arc;

        /** @brief Rotation offset, in radians. */
        float rotation;

        /** @brief Outer radial extent of the primary marks, measured from centre. */
        float length;

        /** @brief Primary mark geometry, rebuilt each draw(). */
        juce::Path path;

        /** @brief Secondary (sub-division) mark geometry, rebuilt each draw() by Sub subclasses. */
        juce::Path subPath;

        /** @brief Number-ring glyph outline, built by drawNumbers(). */
        juce::Path numbersPath;

        /** @brief Radial distance from centre to the corner guide marks, set by drawNumbers(). */
        float cornerDistance;

    private:
        const SVGtype svgType;

        /** @brief Builds the centre-cross and four corner guide marks used by getSVG(). */
        std::tuple<juce::Path, double> getMarker() const noexcept;
    };

    /** @brief Selects whether Line-family marks are measured from the arc's inner or outer radius. */
    enum class Mode
    {
        Arc = 1,
        Line,
        ArcLine,
        Dot,
        InvLine,
        InvArcLine,
        Poly,
        ArcDiv,
        LineSub,
        ArcLineSub,
        DotSub,
        InvLineSub,
        InvArcLineSub,
        PolySub,
        InvPolySub = 17,
    };

    /*____________________________________________________________________________*/

    /**
     * @class Arc
     * @brief A single filled pie-segment mark spanning the rotary's arc.
     */
    class Arc : public Base
    {
    public:
        /** @brief Constructs from an explicit centre and diameter.
         *  @param centrePoint     Centre of the rotary.
         *  @param diameter        Overall diameter.
         *  @param lineWidth       Thickness of the arc band.
         *  @param arcDegree       Half-span of the arc, in degrees.
         *  @param rotationDegree  Rotation offset, in degrees.
         */
        Arc (const juce::Point<float>& centrePoint,
             float diameter,
             float lineWidth = 2.0f,
             float arcDegree = 150.f,
             float rotationDegree = 0.0f);

        /** @brief Constructs centred within @p areaToDraw, scaled by @p normalisedValue.
         *  @param areaToDraw       Rectangle the rotary is centred within.
         *  @param normalisedValue  Value in [0, 1] mapping to arc span and rotation.
         *  @param lineWidth        Thickness of the arc band.
         *  @param arcDegree        Maximum half-span of the arc, in degrees.
         *  @param rotationDegree   Rotation offset, in degrees.
         */
        Arc (const juce::Rectangle<float>& areaToDraw,
             float normalisedValue = 1.0f,
             float lineWidth = 2.0f,
             float arcDegree = 150.f,
             float rotationDegree = 0.0f);

        void draw (juce::Graphics& g) override;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Arc)
    };

    /*____________________________________________________________________________*/

    /**
     * @class Dash
     * @brief An arc band divided into evenly spaced pie-segment dashes.
     */
    class Dash : public Arc
    {
    public:
        /** @brief Constructs from an explicit centre and diameter.
         *  @param centrePoint     Centre of the rotary.
         *  @param diameter        Overall diameter.
         *  @param lineWidth       Thickness of the arc band.
         *  @param division        Number of dashes.
         *  @param gap             Fractional gap between dashes, relative to one dash's angular width.
         *  @param arcDegree       Half-span of the arc, in degrees.
         *  @param rotationDegree  Rotation offset, in degrees.
         */
        Dash (const juce::Point<float>& centrePoint,
              float diameter,
              float lineWidth = 2.0f,
              int division = 9,
              float gap = 0.25f,
              float arcDegree = 150.0f,
              float rotationDegree = 0.0f);

        /** @brief Constructs centred within @p areaToDraw, scaled by @p normalisedValue.
         *  @param areaToDraw       Rectangle the rotary is centred within.
         *  @param normalisedValue  Value in [0, 1] mapping to arc span and rotation.
         *  @param lineWidth        Thickness of the arc band.
         *  @param division         Number of dashes.
         *  @param gap              Fractional gap between dashes.
         *  @param arcDegree        Maximum half-span of the arc, in degrees.
         *  @param rotationDegree   Rotation offset, in degrees.
         */
        Dash (const juce::Rectangle<float>& areaToDraw,
              float normalisedValue = 1.0f,
              float lineWidth = 2.0f,
              int division = 9,
              float gap = 0.25f,
              float arcDegree = 150.0f,
              float rotationDegree = 0.0f);

        void draw (juce::Graphics& g) override;

    private:
        int div;

        /** @brief Recomputes the dash boundary angles for @p division dashes. */
        void calculateAngles (int division);

        float offset;
        std::vector<float> dashes;
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Dash)
    };

    /*____________________________________________________________________________*/
    /** @brief Selects whether Line-family marks also draw a connecting arc. */
    enum class ShouldDraw
    {
        withoutArc,
        withArc
    };

    /**
     * @class Line
     * @brief Radial tick-mark lines at evenly spaced angles, with an optional connecting arc.
     */
    class Line : public Base
    {
    public:
        /** @brief Constructs from an explicit centre and diameter.
         *  @param centrePoint     Centre of the rotary.
         *  @param diameter        Overall diameter.
         *  @param division        Number of tick marks.
         *  @param lineLength      Length each tick extends beyond the base radius.
         *  @param strokeWidth     Stroke width of each tick.
         *  @param shouldDrawArc   Whether to also stroke a connecting arc.
         *  @param isInside        When true, ticks are measured from the inner radius; ticks point outward otherwise.
         *  @param arcDegree       Half-span of the arc, in degrees.
         *  @param rotationDegree  Rotation offset, in degrees.
         */
        Line (const juce::Point<float>& centrePoint,
              float diameter,
              int division = 9,
              float lineLength = 10.0f,
              float strokeWidth = 2.0f,
              ShouldDraw shouldDrawArc = ShouldDraw::withoutArc,
              bool isInside = true,
              float arcDegree = 150.0f,
              float rotationDegree = 0.0f);

        /** @brief Constructs centred within @p areaToDraw, scaled by @p normalisedValue.
         *  @param areaToDraw       Rectangle the rotary is centred within.
         *  @param normalisedValue  Value in [0, 1] mapping to arc span and rotation.
         *  @param division         Number of tick marks.
         *  @param lineLength       Length each tick extends beyond the base radius.
         *  @param strokeWidth      Stroke width of each tick.
         *  @param shouldDrawArc    Whether to also stroke a connecting arc.
         *  @param isInside         When true, ticks are measured from the inner radius; ticks point outward otherwise.
         *  @param arcDegree        Maximum half-span of the arc, in degrees.
         *  @param rotationDegree   Rotation offset, in degrees.
         */
        Line (const juce::Rectangle<float>& areaToDraw,
              float normalisedValue = 1.0f,
              int division = 9,
              float lineLength = 10.0f,
              float strokeWidth = 2.0f,
              ShouldDraw shouldDrawArc = ShouldDraw::withoutArc,
              bool isInside = true,
              float arcDegree = 150.0f,
              float rotationDegree = 0.0f);

        void draw (juce::Graphics& g) override;

    protected:
        /** @brief Number of tick marks. */
        int div;

        /** @brief Whether a connecting arc is also stroked. */
        ShouldDraw drawArc;

        /** @brief When true, ticks are measured from the inner radius rather than pointing outward. */
        bool arcPosition;

        /** @brief Stroke width applied to each tick. */
        float stroke;
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Line)
    };

    /*____________________________________________________________________________*/
    /**
     * @class LineSub
     * @brief Line marks with an additional set of shorter sub-division ticks between each main tick.
     */
    class LineSub : public Line
    {
    public:
        /** @brief Constructs from an explicit centre and diameter.
         *  @param centrePoint      Centre of the rotary.
         *  @param diameter         Overall diameter.
         *  @param division         Number of main tick marks.
         *  @param subDivision      Number of sub-ticks between each pair of main ticks.
         *  @param lineLength       Length each main tick extends beyond the base radius.
         *  @param strokeWidth      Stroke width of each main tick.
         *  @param subLineLength    Sub-tick length as a fraction of @p lineLength.
         *  @param subStrokeWidth   Sub-tick stroke width as a fraction of @p strokeWidth.
         *  @param shouldDrawArc    Whether to also stroke a connecting arc.
         *  @param isInside         When true, ticks are measured from the inner radius.
         *  @param arcDegree        Half-span of the arc, in degrees.
         *  @param rotationDegree   Rotation offset, in degrees.
         */
        LineSub (const juce::Point<float>& centrePoint,
                 float diameter,
                 int division = 9,
                 int subDivision = 1,
                 float lineLength = 10.0f,
                 float strokeWidth = 2.0f,
                 float subLineLength = 0.75f,
                 float subStrokeWidth = 0.75f,
                 ShouldDraw shouldDrawArc = ShouldDraw::withoutArc,
                 bool isInside = true,
                 float arcDegree = 150.0f,
                 float rotationDegree = 0.0f);

        /** @brief Constructs centred within @p areaToDraw, scaled by @p normalisedValue.
         *  @param areaToDraw       Rectangle the rotary is centred within.
         *  @param normalisedValue  Value in [0, 1] mapping to arc span and rotation.
         *  @param division         Number of main tick marks.
         *  @param subDivision      Number of sub-ticks between each pair of main ticks.
         *  @param lineLength       Length each main tick extends beyond the base radius.
         *  @param strokeWidth      Stroke width of each main tick.
         *  @param subLineLength    Sub-tick length as a fraction of @p lineLength.
         *  @param subStrokeWidth   Sub-tick stroke width as a fraction of @p strokeWidth.
         *  @param shouldDrawArc    Whether to also stroke a connecting arc.
         *  @param isInside         When true, ticks are measured from the inner radius.
         *  @param arcDegree        Maximum half-span of the arc, in degrees.
         *  @param rotationDegree   Rotation offset, in degrees.
         */
        LineSub (const juce::Rectangle<float>& areaToDraw,
                 float normalisedValue = 1.0f,
                 int division = 9,
                 int subDivision = 1,
                 float lineLength = 10.0f,
                 float strokeWidth = 2.0f,
                 float subLineLength = 0.75f,
                 float subStrokeWidth = 0.75f,
                 ShouldDraw shouldDrawArc = ShouldDraw::withoutArc,
                 bool isInside = true,
                 float arcDegree = 150.0f,
                 float rotationDegree = 0.0f);

        void draw (juce::Graphics& g) override;

    private:
        int sub;
        float subLength;
        float subStroke;
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LineSub)
    };

    /*____________________________________________________________________________*/
    /**
     * @class Dot
     * @brief Circular dot marks at evenly spaced angles.
     */
    class Dot : public Base
    {
    public:
        /** @brief Constructs from an explicit centre and diameter.
         *  @param centrePoint     Centre of the rotary.
         *  @param diameter        Overall diameter.
         *  @param division        Number of dots.
         *  @param lineLength      Radial offset added to the dot ring's radius.
         *  @param dotSize         Dot diameter.
         *  @param arcDegree       Half-span of the arc, in degrees.
         *  @param rotationDegree  Rotation offset, in degrees.
         */
        Dot (const juce::Point<float>& centrePoint,
             float diameter = 1.0f,
             int division = 9,
             float lineLength = 10.0f,
             float dotSize = 4.0f,
             float arcDegree = 150.0f,
             float rotationDegree = 0.0f);

        /** @brief Constructs centred within @p areaToDraw, scaled by @p normalisedValue.
         *  @param areaToDraw       Rectangle the rotary is centred within.
         *  @param normalisedValue  Value in [0, 1] mapping to arc span and rotation.
         *  @param division         Number of dots.
         *  @param lineLength       Radial offset added to the dot ring's radius.
         *  @param dotSize          Dot diameter.
         *  @param arcDegree        Maximum half-span of the arc, in degrees.
         *  @param rotationDegree   Rotation offset, in degrees.
         */
        Dot (const juce::Rectangle<float>& areaToDraw,
             float normalisedValue = 1.0f,
             int division = 9,
             float lineLength = 10.0f,
             float dotSize = 4.0f,
             float arcDegree = 150.0f,
             float rotationDegree = 0.0f);

        void draw (juce::Graphics& g) override;

    protected:
        /** @brief Number of dots. */
        int div;

        /** @brief Dot radius (half of dotSize). */
        float size;
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Dot)
    };

    /*____________________________________________________________________________*/
    /**
     * @class DotSub
     * @brief Dot marks with an additional set of smaller sub-division dots between each main dot.
     */
    class DotSub : public Dot
    {
    public:
        /** @brief Constructs from an explicit centre and diameter.
         *  @param centrePoint     Centre of the rotary.
         *  @param diameter        Overall diameter.
         *  @param division        Number of main dots.
         *  @param subDivision     Number of sub-dots between each pair of main dots.
         *  @param lineLength      Radial offset added to the dot ring's radius.
         *  @param dotSize         Main dot diameter.
         *  @param subDotSize      Sub-dot diameter as a fraction of @p dotSize.
         *  @param arcDegree       Half-span of the arc, in degrees.
         *  @param rotationDegree  Rotation offset, in degrees.
         */
        DotSub (const juce::Point<float>& centrePoint,
                float diameter,
                int division = 9,
                int subDivision = 1,
                float lineLength = 10.0f,
                float dotSize = 4.0f,
                float subDotSize = 0.75f,
                float arcDegree = 150.0f,
                float rotationDegree = 0.0f);

        /** @brief Constructs centred within @p areaToDraw, scaled by @p normalisedValue.
         *  @param areaToDraw       Rectangle the rotary is centred within.
         *  @param normalisedValue  Value in [0, 1] mapping to arc span and rotation.
         *  @param division         Number of main dots.
         *  @param subDivision      Number of sub-dots between each pair of main dots.
         *  @param lineLength       Radial offset added to the dot ring's radius.
         *  @param dotSize          Main dot diameter.
         *  @param subDotSize       Sub-dot diameter as a fraction of @p dotSize.
         *  @param arcDegree        Maximum half-span of the arc, in degrees.
         *  @param rotationDegree   Rotation offset, in degrees.
         */
        DotSub (const juce::Rectangle<float>& areaToDraw,
                float normalisedValue = 1.0f,
                int division = 9,
                int subDivision = 1,
                float lineLength = 10.0f,
                float dotSize = 4.0f,
                float subDotSize = 0.75f,
                float arcDegree = 150.0f,
                float rotationDegree = 0.0f);

        void draw (juce::Graphics& g) override;

    private:
        int sub;
        float subSize;
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DotSub)
    };

    /*____________________________________________________________________________*/
    /**
     * @class Polygon
     * @brief Regular-polygon marks at evenly spaced angles.
     */
    class Polygon : public Base
    {
    public:
        /** @brief Constructs from an explicit centre and diameter.
         *  @param centrePoint       Centre of the rotary.
         *  @param diameter          Overall diameter.
         *  @param division          Number of polygon marks.
         *  @param lineLength        Radial offset added to the mark ring's radius.
         *  @param polySide          Number of sides per polygon mark.
         *  @param dotSize           Polygon circumscribed diameter.
         *  @param polyAngleDegree   Rotation applied to each individual polygon, in degrees.
         *  @param arcDegree         Half-span of the arc, in degrees.
         *  @param rotationDegree    Rotation offset, in degrees.
         */
        Polygon (const juce::Point<float>& centrePoint,
                 float diameter,
                 int division = 9,
                 float lineLength = 10.0f,
                 int polySide = 3,
                 float dotSize = 4.0f,
                 float polyAngleDegree = 0.0f,
                 float arcDegree = 150.0f,
                 float rotationDegree = 0.0f);

        /** @brief Constructs centred within @p areaToDraw, scaled by @p normalisedValue.
         *  @param areaToDraw        Rectangle the rotary is centred within.
         *  @param normalisedValue   Value in [0, 1] mapping to arc span and rotation.
         *  @param division          Number of polygon marks.
         *  @param lineLength        Radial offset added to the mark ring's radius.
         *  @param polySide          Number of sides per polygon mark.
         *  @param dotSize           Polygon circumscribed diameter.
         *  @param polyAngleDegree   Rotation applied to each individual polygon, in degrees.
         *  @param arcDegree         Maximum half-span of the arc, in degrees.
         *  @param rotationDegree    Rotation offset, in degrees.
         */
        Polygon (const juce::Rectangle<float>& areaToDraw,
                 float normalisedValue = 1.0f,
                 int division = 9,
                 float lineLength = 10.0f,
                 int polySide = 3,
                 float dotSize = 4.0f,
                 float polyAngleDegree = 0.0f,
                 float arcDegree = 150.0f,
                 float rotationDegree = 0.0f);

        void draw (juce::Graphics& g) override;

    protected:
        /** @brief Number of polygon marks. */
        int div;

        /** @brief Number of sides per polygon mark. */
        int side;

        /** @brief Polygon circumscribed diameter. */
        float size;

        /** @brief Per-polygon rotation, in radians. */
        float angle;
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Polygon)
    };

    /*____________________________________________________________________________*/
    /**
     * @class PolygonSub
     * @brief Polygon marks with an additional set of smaller sub-division polygons between each main mark.
     */
    class PolygonSub : public Polygon
    {
    public:
        /** @brief Constructs from an explicit centre and diameter.
         *  @param centrePoint       Centre of the rotary.
         *  @param diameter          Overall diameter.
         *  @param division          Number of main polygon marks.
         *  @param subDivision       Number of sub-marks between each pair of main marks.
         *  @param lineLength        Radial offset added to the mark ring's radius.
         *  @param polySide          Number of sides per polygon mark.
         *  @param dotSize           Main polygon circumscribed diameter.
         *  @param subDotSize        Sub-polygon circumscribed diameter as a fraction of @p dotSize.
         *  @param isInside          When true, sub-marks are placed toward the centre rather than outward.
         *  @param polyAngleDegree   Rotation applied to each individual polygon, in degrees.
         *  @param arcDegree         Half-span of the arc, in degrees.
         *  @param rotationDegree    Rotation offset, in degrees.
         */
        PolygonSub (const juce::Point<float>& centrePoint,
                    float diameter,
                    int division = 9,
                    int subDivision = 1,
                    float lineLength = 10.0f,
                    int polySide = 3,
                    float dotSize = 4.0f,
                    float subDotSize = 0.75f,
                    bool isInside = true,
                    float polyAngleDegree = 0.0f,
                    float arcDegree = 150.0f,
                    float rotationDegree = 0.0f);

        /** @brief Constructs centred within @p areaToDraw, scaled by @p normalisedValue.
         *  @param areaToDraw        Rectangle the rotary is centred within.
         *  @param normalisedValue   Value in [0, 1] mapping to arc span and rotation.
         *  @param division          Number of main polygon marks.
         *  @param subDivision       Number of sub-marks between each pair of main marks.
         *  @param lineLength        Radial offset added to the mark ring's radius.
         *  @param polySide          Number of sides per polygon mark.
         *  @param dotSize           Main polygon circumscribed diameter.
         *  @param subDotSize        Sub-polygon circumscribed diameter as a fraction of @p dotSize.
         *  @param isInside          When true, sub-marks are placed toward the centre rather than outward.
         *  @param polyAngleDegree   Rotation applied to each individual polygon, in degrees.
         *  @param arcDegree         Maximum half-span of the arc, in degrees.
         *  @param rotationDegree    Rotation offset, in degrees.
         */
        PolygonSub (const juce::Rectangle<float>& areaToDraw,
                    float normalisedValue = 1.0f,
                    int division = 9,
                    int subDivision = 1,
                    float lineLength = 10.0f,
                    int polySide = 3,
                    float dotSize = 4.0f,
                    float subDotSize = 0.75f,
                    bool isInside = true,
                    float polyAngleDegree = 0.0f,
                    float arcDegree = 150.0f,
                    float rotationDegree = 0.0f);

        void draw (juce::Graphics& g) override;

    private:
        int sub;
        float subSize;
        bool subPosition;
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolygonSub)
    };
};
/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
