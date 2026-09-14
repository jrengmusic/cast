#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/** @brief Blurs a source image into one owned, stably-mutated destination.
 *
 *  Delegates the actual blur to jam::VulkanEngine::blurImage(), which picks
 *  the GPU two-dispatch compute stack-blur route when the engine is
 *  GPU-enabled and available, falling back to the CPU stack-blur
 *  (jam::stackBlurArgb/stackBlurSingleChannel) otherwise. destination is
 *  (re)allocated to match source's dimensions and juce::Image::PixelFormat on
 *  either path.
 *
 *  Rebinds itself as a juce::ImagePixelData::Listener on the resolved root of
 *  whichever source it is last asked to render, so re-rendering the same
 *  source repeatedly only pays for the blur once per actual mutation.
 */
class Blur : private juce::ImagePixelData::Listener
{
public:
    /** @brief Constructs a Blur with the given radius.
     *  @param newRadius  Blur radius in pixels, must be > 0.
     */
    explicit Blur (int newRadius)
        : radius (newRadius)
    {
    }

    /** Unregisters this listener from any still-bound source root. */
    ~Blur()
    {
        if (boundSourceRoot != nullptr)
            boundSourceRoot->listeners.remove (this);
    }

    /** @brief Sets the blur radius and marks the destination dirty for the
     *  next render() call.
     *  @param newRadius  Blur radius in pixels, must be > 0.
     */
    void setRadius (int newRadius) noexcept
    {
        jassert (newRadius > 0);

        radius = newRadius;
        dirty = true;
    }

    /** @brief Returns the blur radius in pixels. */
    int getRadius() const noexcept { return radius; }

    /** @brief Returns the blurred version of @p source, re-blurring only when
     *  the source's resolved root has changed or been mutated since the last
     *  call.
     *  @param source  The image to blur, must be valid.
     *  @return         Reference to the owned destination image, valid until
     *                   the next render() call.
     */
    const juce::Image& render (const juce::Image& source);

private:
    void imageDataChanged (juce::ImagePixelData* changedSource) override
    {
        if (changedSource == boundSourceRoot)
            dirty = true;
    }

    void imageDataBeingDeleted (juce::ImagePixelData* deletedSource) override
    {
        if (deletedSource == boundSourceRoot)
            boundSourceRoot = nullptr;
    }

    void bindSource (juce::ImagePixelData* newRoot)
    {
        jassert (newRoot != nullptr);

        if (boundSourceRoot != nullptr)
            boundSourceRoot->listeners.remove (this);

        boundSourceRoot = newRoot;
        boundSourceRoot->listeners.add (this);
    }

    //==============================================================================
    int radius;
    juce::Image destination;
    juce::ImagePixelData* boundSourceRoot { nullptr };
    bool dirty { true };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Blur)
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
