/**
 * @file jam_Drawable.h
 * @brief One-shot fill/stroke helpers over juce::DrawablePath — curved-joint,
 *        rounded-cap path rendering without a persistent Drawable owner.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct Drawable
 * @brief Stateless fill/stroke rendering helpers built on a transient juce::DrawablePath.
 */
struct Drawable
{
    /** @brief Fills and strokes @p path directly into @p g, with curved joints and rounded caps.
     *  @param g              Destination graphics context.
     *  @param path           Path to draw.
     *  @param fillColour     Fill colour.
     *  @param strokeColour   Stroke colour.
     *  @param lineThickness  Stroke width.
     *  @param opacity        Overall draw opacity, 0-1.
     */
    static void draw (juce::Graphics& g,
                      const juce::Path& path,
                      juce::Colour fillColour,
                      juce::Colour strokeColour,
                      float lineThickness = 1.0f,
                      float opacity = 1.0f)
    {
        auto joint { juce::PathStrokeType::curved };
        auto endCap { juce::PathStrokeType::rounded };
        auto strokeType { juce::PathStrokeType (lineThickness, joint, endCap) };

        juce::DrawablePath d;
        d.setPath (path);
        d.setFill (fillColour);
        d.setStrokeType (strokeType);
        d.setStrokeFill (strokeColour);
        d.draw (g, opacity);
    }

    /** @brief Fills and strokes @p path scaled and centred within @p area.
     *  @param g              Destination graphics context.
     *  @param area           Area the path is scaled/centred into.
     *  @param path           Path to draw.
     *  @param fillColour     Fill colour.
     *  @param strokeColour   Stroke colour.
     *  @param lineThickness  Stroke width.
     *  @param opacity        Overall draw opacity, 0-1.
     */
    static void drawWithin (juce::Graphics& g,
                            const juce::Rectangle<float>& area,
                            const juce::Path& path,
                            juce::Colour fillColour,
                            juce::Colour strokeColour,
                            float lineThickness = 1.0f,
                            float opacity = 1.0f)
    {
        auto joint { juce::PathStrokeType::curved };
        auto endCap { juce::PathStrokeType::rounded };
        auto strokeType { juce::PathStrokeType (lineThickness, joint, endCap) };

        juce::DrawablePath d;
        d.setPath (path);
        d.setFill (fillColour);
        d.setStrokeType (strokeType);
        d.setStrokeFill (strokeColour);
        d.drawWithin (g, area, juce::RectanglePlacement::centred, opacity);
    }

    /** @brief Strokes @p path only (no fill), directly into @p g.
     *  @param g              Destination graphics context.
     *  @param path           Path to stroke.
     *  @param strokeColour   Stroke colour.
     *  @param lineThickness  Stroke width.
     *  @param opacity        Overall draw opacity, 0-1.
     */
    static void stroke (juce::Graphics& g,
                        const juce::Path& path,
                        juce::Colour strokeColour,
                        float lineThickness = 1.0f,
                        float opacity = 1.0f)
    {
        draw (g, path, juce::Colour(), strokeColour, lineThickness, opacity);
    }

    /** @brief Strokes @p path only (no fill), scaled and centred within @p area.
     *  @param g              Destination graphics context.
     *  @param area           Area the path is scaled/centred into.
     *  @param path           Path to stroke.
     *  @param strokeColour   Stroke colour.
     *  @param lineThickness  Stroke width.
     *  @param opacity        Overall draw opacity, 0-1.
     */
    static void stroke (juce::Graphics& g,
                        const juce::Rectangle<float>& area,
                        const juce::Path& path,
                        juce::Colour strokeColour,
                        float lineThickness = 1.0f,
                        float opacity = 1.0f)
    {
        drawWithin (g, area, path, juce::Colour(), strokeColour, lineThickness, opacity);
    }

    /** @brief Fills @p path only (no stroke), directly into @p g.
     *  @param g              Destination graphics context.
     *  @param path           Path to fill.
     *  @param fillColour     Fill colour.
     *  @param lineThickness  Unused stroke width parameter, kept for signature
     *                        symmetry with draw(); no stroke is drawn.
     *  @param opacity        Overall draw opacity, 0-1.
     */
    static void fill (juce::Graphics& g,
                      const juce::Path& path,
                      juce::Colour fillColour,
                      float lineThickness = 1.0f,
                      float opacity = 1.0f)
    {
        draw (g, path, fillColour, juce::Colour(), lineThickness, opacity);
    }
};

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
