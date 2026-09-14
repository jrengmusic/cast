#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/** @brief Owns the rasterized SingleChannel coverage mask shared by
 *  DropShadow and InnerShadow.
 *
 *  Re-rasterizes only when the path differs from the cached one; blurring is
 *  the caller's responsibility (InnerShadow needs to invert coverage between
 *  rasterize and blur, DropShadow does not).
 */
class ShadowMask
{
public:
    /** @brief Constructs a ShadowMask with the given blur radius.
     *  @param newRadius  Stack-blur radius in pixels, must be > 0. Also
     *                     controls how far rasterize() expands the mask
     *                     image's bounds beyond the path.
     */
    explicit ShadowMask (int newRadius) : radius (newRadius)
    {
        jassert (radius > 0);
    }

    /** @brief Re-rasterizes image/maskOrigin from @p path when it differs
     *  from the cached path, leaving them untouched otherwise.
     *  @param path  The path to rasterize coverage from.
     *  @return       True when the mask was re-rasterized (path changed).
     */
    bool update (const juce::Path& path)
    {
        const bool pathChanged { not (path == cachedPath) };

        if (pathChanged)
        {
            rasterize (path);
            cachedPath = path;
        }

        return pathChanged;
    }

    /** @brief Rasterized SingleChannel coverage bitmap of the cached path,
     *  expanded by radius, white fill against a transparent background. */
    juce::Image image;

    /** @brief Top-left of image's bounds, in the cached path's own coordinate
     *  space (path.getBounds() expanded by radius). */
    juce::Point<int> maskOrigin;

    /** @brief Stack-blur radius in pixels — also the fixed expansion applied
     *  to the path's bounds when rasterizing image. */
    const int radius;

private:
    void rasterize (const juce::Path& path)
    {
        const juce::Rectangle<int> bounds { path.getBounds().getSmallestIntegerContainer().expanded (radius) };

        maskOrigin = bounds.getPosition();
        image = juce::Image (juce::Image::SingleChannel, bounds.getWidth(), bounds.getHeight(), true, juce::SoftwareImageType());

        juce::Graphics maskGraphics { image };
        maskGraphics.setColour (juce::Colours::white);
        maskGraphics.fillPath (path, juce::AffineTransform::translation (static_cast<float> (-maskOrigin.x), static_cast<float> (-maskOrigin.y)));
    }

    juce::Path cachedPath;
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
