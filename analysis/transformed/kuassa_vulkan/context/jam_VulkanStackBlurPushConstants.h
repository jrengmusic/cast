namespace jam
{
/*____________________________________________________________________________*/
/** @brief Push-constant layout read by shaders/stack_blur_texture.comp's and
 *  shaders/stack_blur_buffer.comp's shared `StackBlurPC` block
 *  (VulkanPipelines::ComputeID::stackBlurTexture/stackBlurBuffer).
 *
 *  GLSL push_constant block == `int radius; int lineCount; int lineLength;
 *  uint sourceTextureIndex;` — matches this struct field-for-field, tightly
 *  packed on both sides (std430's base alignment for int/uint is 4 bytes,
 *  matching this struct's natural C++ alignment), sizeof 16 bytes. */
struct VulkanStackBlurPushConstants
{
    /** Blur radius in elements — same value stack_blur.glsl's stackBlurLine() walks with. */
    int radius;

    /** Number of lines this dispatch walks — one invocation per line
     *  (gl_GlobalInvocationID.x), guarded against by both .comp files' `line
     *  < lineCount` check. Pass 1 passes source height; pass 2 passes source width
     *  (VulkanImageEffects::blurImage()'s transpose-on-write). */
    int lineCount;

    /** Number of elements per line — the clamp bound both .comp files'
     *  loadElement() applies before reading. Pass 1 passes source width; pass 2
     *  passes source height. */
    int lineLength;

    /** Bindless sampled-image array index of the source texture
     *  (shaders/bindless_texture.glsl). Consumed only by
     *  shaders/stack_blur_texture.comp's loadElement() — the buffer variant reads
     *  through its own set-2 SSBO instead and ignores this field. */
    uint32_t sourceTextureIndex;
};

static_assert (sizeof (VulkanStackBlurPushConstants) == 16, "VulkanStackBlurPushConstants must be 16 bytes (matches shaders/stack_blur_texture.comp's/stack_blur_buffer.comp's StackBlurPC block)");

/** Invocations per workgroup — paired with both shaders/stack_blur_texture.comp's
 *  and shaders/stack_blur_buffer.comp's `layout (local_size_x = 64, …)`. This
 *  pairing cannot be single-sourced across the C++/GLSL language boundary; the
 *  constant is documented at both sites. */
static constexpr int stackBlurWorkgroupSize { 64 };

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
