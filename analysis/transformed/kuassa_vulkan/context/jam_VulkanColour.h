/** @file jam_VulkanColour.h
 *  @brief GPU-facing normalised RGBA colour — the canonical unpacked form of
 *         juce::Colour's packed 32-bit ARGB, used wherever a shader push
 *         constant or SSBO record needs four float channels.
 */

namespace jam
{
/*____________________________________________________________________________*/

/** @brief Normalised RGBA colour (each channel in [0, 1]). Trivially copyable,
 *  standard layout, 16 bytes — a valid SSBO/push-constant field type.
 *
 *  Opacity is brush state, not a colour property — fromHex() never applies it.
 *  Callers compose opacity explicitly at emit time (`colour.a *= currentState.opacity`
 *  or equivalent visible multiply on the unpacked VulkanColour), immediately before the
 *  value reaches a push constant or SSBO record. See every fromHex() call site.
 */
struct VulkanColour
{
    /** @brief Red, green, blue, alpha channels, each normalised to [0, 1]. */
    float r, g, b, a;

    /** @brief Unpacks @p colour's channels to normalised floats.
     *  @param colour  Source colour.
     *  @returns         The unpacked colour — alpha is @p colour's own alpha channel
     *                   only; the caller composes opacity separately (see class doc).
     */
    static VulkanColour fromHex (juce::Colour colour) noexcept
    {
        return { colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha() };
    }

    /** @brief Packs this colour's normalised float channels back into a juce::Colour. */
    juce::Colour toHex() const noexcept
    {
        return juce::Colour::fromFloatRGBA (r, g, b, a);
    }
};

static_assert (sizeof (VulkanColour) == 16, "VulkanColour must be 16 bytes (4 floats, no padding)");
static_assert (std::is_trivially_copyable_v<VulkanColour>);
static_assert (std::is_standard_layout_v<VulkanColour>);

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam