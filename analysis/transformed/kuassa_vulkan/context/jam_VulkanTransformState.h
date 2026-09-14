/** @file jam_VulkanTransformState.h
 *  @brief Namespace-scope user-to-device transform accumulator shared by
 *         Vulkan LLGC state (extracted from
 *         VulkanLowLevelGraphicsContext::TransformState; no dependency on
 *         VulkanLowLevelGraphicsContext itself, so it lives ahead of that header
 *         in the include chain).
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief Accumulates user-space to device-space transform with integer-translation
 *  fast path (mirrors juce::RenderingHelpers::TranslationOrTransform — that type is
 *  internal JUCE, guarded by JUCE_GRAPHICS_INCLUDE_RENDERING_HELPERS; this replicates
 *  the public API subset needed for the D2D-isomorphic coordinate model). */
struct VulkanTransformState
{
    /** @brief Full affine transform used when the mapping is not a pure integer translation. */
    juce::AffineTransform complexTransform;

    /** @brief Integer pixel offset used by the pure-translation fast path. */
    juce::Point<int> offset;

    /** @brief True when the accumulated transform is a pure integer pixel translation. */
    bool isOnlyTranslated { true };

private:
    // ---- addTransform() fast-path constants (8.8 fixed-point sub-pixel detection,
    //      replicated from JUCE's internal RenderingHelpers::TranslationOrTransform) ----

    /** @brief 8.8 fixed-point scale: 2^8 sub-pixel precision. */
    static constexpr float fixedPointScale { 256.0f };

    /** @brief Mask testing whether a translation is within ~3% of an integer pixel. */
    static constexpr int fixedPointFractionMask { 0xf8 };

    /** @brief Shift converting an 8.8 fixed-point value back to integer pixels. */
    static constexpr int fixedPointShift { 8 };

public:
    /** @brief Returns the current transform as an AffineTransform.
     *  @returns  Translation(offset) when isOnlyTranslated; complexTransform otherwise.
     */
    juce::AffineTransform getTransform() const noexcept
    {
        return isOnlyTranslated ? juce::AffineTransform::translation (offset)
                                : complexTransform;
    }

    /** @brief Returns this transform composed with @p t.
     *  @param t  The transform to pre-compose with this one.
     *  @returns  @p t translated by offset, or @p t followed by complexTransform.
     */
    juce::AffineTransform getTransformWith (const juce::AffineTransform& t) const noexcept
    {
        return isOnlyTranslated ? t.translated (offset) : t.followedBy (complexTransform);
    }

    /** @brief Shifts the coordinate origin by @p delta.
     *  @param delta  The pixel offset to apply.
     */
    void setOrigin (juce::Point<int> delta) noexcept
    {
        if (isOnlyTranslated)
            offset += delta;
        else
            complexTransform =
                juce::AffineTransform::translation (delta).followedBy (complexTransform);
    }

    /** @brief Accumulates @p t into the current transform.
     *  @param t  The additional transform to compose.
     */
    void addTransform (const juce::AffineTransform& t) noexcept
    {
        if (isOnlyTranslated and t.isOnlyTranslation())
        {
            const auto tx { static_cast<int> (t.getTranslationX() * fixedPointScale) };
            const auto ty { static_cast<int> (t.getTranslationY() * fixedPointScale) };

            if (((tx | ty) & fixedPointFractionMask) == 0)
            {
                offset += juce::Point<int> (tx >> fixedPointShift, ty >> fixedPointShift);
                return;
            }
        }

        complexTransform = getTransformWith (t);
        isOnlyTranslated = false;
    }

    /** @brief Returns the physical pixel scale factor derived from the current transform. */
    float getPhysicalPixelScaleFactor() const noexcept
    {
        return isOnlyTranslated ? 1.0f
                                : std::sqrt (std::abs (complexTransform.getDeterminant()));
    }

    /** @brief Translates @p r by the integer pixel offset.
     *  @param r  The rectangle to translate; must only be called when isOnlyTranslated is true.
     *  @returns  @p r shifted by offset.
     */
    juce::Rectangle<int> translated (juce::Rectangle<int> r) const noexcept
    {
        jassert (isOnlyTranslated);
        return r + offset;
    }

    /** @brief Transforms @p r by complexTransform.
     *  @param r  The rectangle to transform; must only be called when isOnlyTranslated is false.
     *  @returns  The axis-aligned bounding rect of the transformed rectangle.
     */
    juce::Rectangle<float> boundsAfterTransform (juce::Rectangle<float> r) const noexcept
    {
        jassert (not isOnlyTranslated);
        return r.transformedBy (complexTransform);
    }

    /** @brief Converts @p r from device space back to user space.
     *  @param r  The rectangle in device (physical pixel) space.
     *  @returns  The rectangle in user-space coordinates.
     */
    template <typename Type>
    juce::Rectangle<float> deviceSpaceToUserSpace (juce::Rectangle<Type> r) const noexcept
    {
        return isOnlyTranslated ? r.toFloat() - offset.toFloat()
                                : r.toFloat().transformedBy (complexTransform.inverted());
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam