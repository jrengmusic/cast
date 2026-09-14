/** @file jam_VulkanPrimitiveRecord.h
 *  @brief Unified quad primitive instance record (VulkanPrimitiveRecord), the SSBO layout
 *         pulled by the instanced vertex shader for every rect fill, image draw,
 *         glyph quad, and clip-mask quad.
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief One instance record for a unified quad primitive (rect fill, image draw,
 *         glyph quad, or clip-mask quad), pulled by the instanced vertex shader via
 *         gl_InstanceIndex from the per-frame VulkanPrimitiveRecordBuffer SSBO (set 2,
 *         binding 0, readonly, vertex stage).
 *
 *  Layout is std430-compatible by construction — every member's natural C++ offset
 *  already satisfies its GLSL std430 base-alignment requirement (vec2 members at
 *  8-byte-aligned offsets, vec4/ivec4 members at 16-byte-aligned offsets), so no
 *  compiler-inserted padding exists; the trailing `maskTextureIndex` member occupies
 *  the final 4 bytes that bring the struct to the 16-byte std430 array stride of 80.
 *  Total size: 80 bytes.
 *
 *  `stencilRef` is baked from `currentState.stencilClipDepth` at each record's emit
 *  time — read by the GPU stencil test via the stencil-gated draw path.
 *  `clip` remains dormant until the winding-rule work wires encoding. `textureIndex`
 *  indexes the bindless `texture2D[]` array at set 1 binding 0 — a stable
 *  per-texture slot assigned once at upload, read by every image/glyph fragment
 *  shader via `nonuniformEXT(textureIndex)`.
 *  `flags` is reserved for the winding-rule encoding and unused (0) until then.
 */
struct VulkanPrimitiveRecord
{
    /** @brief Top-left corner of the quad, in device (physical pixel) space.
     *  juce::Point<float> — layout-compatible with a raw float[2] (two
     *  trivially-copyable floats, no vtable, no compiler-inserted padding);
     *  offset/size within the struct are unchanged. */
    juce::Point<float> position;

    /** @brief Width/height of the quad, in device (physical pixel) space.
     *  jam::Size<float> — backed by jam::Union\<float, float\>'s uint64_t
     *  (jam_core/utilities/jam_Size.h), bit-identical to a raw float[2];
     *  `get<0>()`/`get<1>()` or structured bindings address width/height. */
    jam::Size<float> size;

    /** @brief UV rectangle sampled by textured primitives, extent semantics —
     *  x, y = UV origin, w, h = UV extent (matches `clip` below's own
     *  Rectangle layout). juce::Rectangle<float> — layout-compatible with a
     *  raw float[4] (four trivially-copyable floats, no vtable, no
     *  compiler-inserted padding); instanced.vert derives each corner's UV as
     *  `uvRect.xy + corner * uvRect.zw`. */
    juce::Rectangle<float> uvRect;

    /** @brief Fill/tint colour (RGBA), opacity pre-multiplied into the alpha channel.
     *  jam::VulkanColour — layout-compatible with a raw float[4] (jam_VulkanColour.h). */
    VulkanColour color;

    /** @brief Clip rectangle channel — dormant until the winding-rule work wires encoding.
     *  juce::Rectangle<int> — layout-compatible with the prior int32_t[4] (a
     *  Point<int> pair + two trailing ints, no vtable, no compiler-inserted
     *  padding); offset/size within the struct are unchanged. */
    juce::Rectangle<int> clip;

    /** @brief Bindless texture array index — resolved via VulkanGraphics::getBindlessIndex()
     *  (cached-image textures, per-window bookkeeping on VulkanGraphics) or
     *  jam::VulkanBindlessTexture::getBindlessIndex() (the glyph atlas's
     *  GPU-mirror textures, per-window bookkeeping on the texture itself,
     *  keyed by VulkanGraphics::getNativeHandle()) at record-emit time — never on
     *  the VulkanEngine-wide GlyphAtlas instance itself. */
    uint32_t textureIndex;

    /** @brief Stencil reference value for this primitive's active clip depth. */
    uint32_t stencilRef;

    /** @brief Primitive-type/winding/mono-emoji flags — reserved, unused until the
     *  winding-rule work wires them up. */
    uint32_t flags;

    /** @brief Bindless texture array index of the active alpha mask, or noMaskIndex
     *  when no mask is bound. Sampled only by masked_image.frag (the
     *  maskedImageInstancedStencil pipeline, selected when this differs from
     *  noMaskIndex); index semantics mirror `textureIndex` — a stable per-texture
     *  slot assigned once at upload, addressed via `nonuniformEXT(maskTextureIndex)`. */
    uint32_t maskTextureIndex;
};

static_assert (sizeof (VulkanPrimitiveRecord) == 80, "VulkanPrimitiveRecord must be 80 bytes (std430 SSBO stride)");
static_assert (sizeof (VulkanPrimitiveRecord) % 16 == 0, "VulkanPrimitiveRecord size must be a multiple of 16 for std430 array stride");

/** @brief Sentinel value for VulkanPrimitiveRecord::maskTextureIndex meaning no mask
 *  is bound for this primitive. recordImageDrawCommands() never selects the
 *  maskedImageInstancedStencil pipeline for a record carrying this value, so the
 *  maskTextureIndex slot is never dereferenced in the bindless array. The value
 *  (0xFFFFFFFF) lies outside any real bindless slot range — hardware-clamped
 *  VulkanGraphics::maxBindlessTextures is far below this ceiling. */
static constexpr uint32_t noMaskIndex { 0xFFFFFFFF };

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam