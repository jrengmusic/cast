#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/** @brief An inner shadow is a drop shadow with inverted coverage, composited
 *  clipped to the original path so the blur only shows up inside it.
 */
class InnerShadow
{
public:
    /** @brief Constructs an InnerShadow with the given colour, blur radius,
     *  and offset.
     *  @param newColour  Fill colour of the blurred shadow.
     *  @param newRadius  Stack-blur radius in pixels, must be > 0.
     *  @param newOffset  Offset of the shadow from the path, in pixels.
     */
    InnerShadow (juce::Colour newColour, int newRadius, juce::Point<int> newOffset)
        : colour (newColour), offset (newOffset), mask (newRadius)
    {
    }

    /** @brief Re-rasterizes, inverts, and re-blurs the mask when @p path has
     *  changed, then draws the blurred, offset shadow with g, clipped to
     *  @p path so it only shows up inside it.
     *  @param g     The destination graphics context.
     *  @param path  The path to cast an inner shadow inside.
     */
    void render (juce::Graphics& g, const juce::Path& path);

private:
    static void invertCoverage (juce::Image& image);

    juce::Colour colour;
    juce::Point<int> offset;
    ShadowMask mask;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InnerShadow)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
