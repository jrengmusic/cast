namespace jam
{
/*____________________________________________________________________________*/
/** @brief Push-constant layout read by every pipeline sharing the `instancedImage`/
 *  `instancedClipMask` stage pairs (imageInstanced, imageInstancedStencil, clipMaskInstanced,
 *  the transparency-layer composite, and VulkanGraphics::compositePipeline — all reuse
 *  image.frag verbatim). Exposed here (rather than file-local to
 *  jam_VulkanLowLevelGraphicsContext.cpp, its original home) so
 *  VulkanGraphics::recordSceneCompositeDrawCommands() can also push it — jam_VulkanGraphics.cpp
 *  is included before jam_VulkanLowLevelGraphicsContext.cpp in jam_vulkan.cpp's unity
 *  build, so a file-local definition in the latter is not visible to the former.
 *
 *  The opacity fold: `opacity` is a State-only member — at draw time only
 *  the product `color.a * opacity` reaches the GPU, so this struct carries only
 *  the pre-multiplied colour, never a separate scalar. GLSL push_constant block ==
 *  `vec4 color; ivec4 clip; vec2 textureScale;` — with no intervening scalar,
 *  std430 places `clip` at offset 16 and `textureScale` at offset 32 on both
 *  the C++ and GLSL sides, byte-identical (sizeof == 40).
 *
 *  Pre-fold layout mismatch (verified via
 *  spirv-dis): with the intervening `float opacity` present, std430 forced
 *  `clip` to a padded offset 32 and `textureScale` to offset 48 on the GLSL
 *  side — 4 bytes beyond the 44-byte push-constant range the pipeline layout
 *  declared (color 16 + opacity 4 + clip 16 + textureScale 8, tightly packed on
 *  the C++ side, no padding). `tiled_image.frag`'s `textureScale` read was
 *  therefore a genuine out-of-range push-constant read — dead in practice only
 *  because VulkanPipelines::ID::tiledImage is an unused reserved slot. */
struct VulkanImagePushConstants
{
    /** Pre-multiplied colour; opacity is folded into the alpha channel. */
    VulkanColour color;

    /** @brief Clip rectangle channel — dormant (never written to a non-zero value
     *  by makeImagePushConstants(), never read by image.frag/tiled_image.frag/
     *  image_alpha_mask.frag's `main()`). juce::Rectangle<int> — layout-compatible
     *  with the prior int32_t[4] (a Point<int> pair + two trailing ints, no vtable,
     *  no compiler-inserted padding); offset/size within the struct are unchanged.
     *  Mirrors VulkanPrimitiveRecord::clip's identical dormant-field conversion
     *  (jam_VulkanPrimitiveRecord.h) — same x, y, w, h semantics once wired up. */
    juce::Rectangle<int> clip;

    /** Per-axis texture scale factor. */
    float textureScale[2];
};

static_assert (sizeof (VulkanImagePushConstants) == 40, "VulkanImagePushConstants must be 40 bytes (matches jam_VulkanPipelines.cpp's sharedPushConstantRangeSize)");

/** @brief Packs `opacity` pre-multiplied into the image push-constant layout's
 *  colour alpha channel (colour is always opaque white — the image's own pixels
 *  carry colour). Defined in jam_VulkanLowLevelGraphicsContext.cpp, declared here
 *  for the same cross-TU-ordering reason as VulkanImagePushConstants above.
 *  @param opacity  Multiplier applied to the packed alpha channel.
 */
VulkanImagePushConstants makeImagePushConstants (float opacity) noexcept;

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam