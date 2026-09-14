namespace jam
{
/*____________________________________________________________________________*/
/** @brief Push-constant layout read by shaders/gradient_fill.frag
 *  (VulkanPipelines::ID::gradientFill).
 *
 *  GLSL push_constant block == `vec2 point1; vec2 point2; int isRadial; uint
 *  lutTextureIndex; float opacity;` — matches this struct field-for-field,
 *  tightly packed on both sides (std430's base alignment for vec2 is 8 bytes,
 *  for int/uint/float is 4 bytes, matching this struct's natural C++
 *  alignment), sizeof 28 bytes, well within jam_VulkanPipelines.cpp's
 *  40-byte sharedPushConstantRangeSize. */
struct VulkanGradientPushConstants
{
    float point1[2];
    float point2[2];

    /** 0 == linear gradient; nonzero == radial gradient. */
    int isRadial;

    /** Bindless texture array index sampled through shaders/bindless_texture.glsl's linearSampler. */
    uint32_t lutTextureIndex;

    float opacity;
};

static_assert (sizeof (VulkanGradientPushConstants) == 28, "VulkanGradientPushConstants must be 28 bytes (matches shaders/gradient_fill.frag's GradientPC block)");

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
