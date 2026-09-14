/**
 * @file jam_ImageMatte.h
 * @brief juce::ImageEffectFilter that clips a component via a precomputed alpha matte.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/
/** @brief juce::ImageEffectFilter that masks component rendering through a precomputed alpha matte.
 *
 *  The matte is derived from a clip image set via setClipImage() and optionally
 *  feathered via setFeather(). On any change, updateMatte() recomputes the
 *  single matte image consumed by applyEffect() at paint time — computed once,
 *  not per-frame.
 */
class ImageMatte : public juce::ImageEffectFilter
{
public:
    ImageMatte() = default;

    /** @brief Sets the clip image used to derive the alpha matte.
     *
     *  The matte is derived state: it recomputes (updateMatte()) only when
     *  @p image refers to a different underlying shared image than the one
     *  currently held — an identical image (same underlying instance, per
     *  juce::Image::operator!=) is a no-op. The clip image's alpha channel
     *  defines the mask shape; colour channels are ignored.
     *  @param image  New clip image.
     */
    void setClipImage (const juce::Image& image) noexcept
    {
        if (image != clipImage)
        {
            clipImage = image;
            updateMatte();
        }
    }

    /** @brief Sets the feather radius and curve exponent applied to the matte edge.
     *
     *  Triggers a matte recomputation (updateMatte()).
     *  @param radius  Feather softening radius in pixels. 0 disables feathering.
     *  @param curve   Exponent controlling the falloff shape (default 1.0 — linear).
     */
    void setFeather (float radius, float curve = 1.0f) noexcept { featherRadius = radius; featherCurve = curve; updateMatte(); }

    /** @brief Applies the precomputed matte to @p sourceImage and composites the result into @p destContext.
     *
     *  GPU route (VulkanEngine present and isGpuAvailable()): clips @p destContext to the
     *  matte's alpha via juce::Graphics::reduceClipRegion — scaled from matte to
     *  @p sourceImage dimensions — then draws @p sourceImage in-stream at @p alpha opacity;
     *  no intermediate image is written.
     *  CPU route: applies the SIMD matte kernel (Graphics::applyMatte) into the owned
     *  destination image and draws that into @p destContext at @p alpha opacity.
     *
     *  @param sourceImage  Component paint output to mask.
     *  @param destContext  Destination graphics context.
     *  @param alpha        Composite alpha (passed through from JUCE's effect chain).
     */
    void applyEffect (juce::Image& sourceImage, juce::Graphics& destContext, float, float alpha) override;

private:
    void updateMatte();

    juce::Image clipImage;
    juce::Image matte;
    juce::Image destination;
    float featherRadius { 0.0f };
    float featherCurve { 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageMatte)
};

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam
