#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/** @brief A drop shadow is a path filled with one colour, then blurred, then
 *  composited behind/around the path at an offset.
 */
class DropShadow
{
public:
    /** @brief Constructs a DropShadow with the given colour, blur radius, and
     *  offset.
     *  @param newColour  Fill colour of the blurred shadow.
     *  @param newRadius  Stack-blur radius in pixels, must be > 0.
     *  @param newOffset  Offset of the shadow from the path, in pixels.
     */
    DropShadow (juce::Colour newColour, int newRadius, juce::Point<int> newOffset)
        : colour (newColour), offset (newOffset), mask (newRadius)
    {
    }

    /** @brief Re-rasterizes and re-blurs the mask when @p path has changed,
     *  then draws the blurred, offset shadow with g.
     *  @param g     The destination graphics context.
     *  @param path  The path to cast a shadow from.
     */
    void render (juce::Graphics& g, const juce::Path& path);

private:
    juce::Colour colour;
    juce::Point<int> offset;
    ShadowMask mask;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DropShadow)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
