// push_constants_rect.glsl — glslc-include-only, not a standalone shader.
// Declares the full RectPC push-constant block. The pipeline layout's single
// VkPushConstantRange is a shared 40-byte window across every pipeline
// (jam_VulkanPipelines.cpp's sharedPushConstantRangeSize, VERTEX | FRAGMENT
// stage flags) — every RectPC consumer declares the FULL block uniformly,
// even where a shader's own main() only reads a subset (fill_rect.vert/frag,
// background.frag never read textureScale; image.frag/tiled_image.frag do).
// Layout matches jam_VulkanGraphics.h's ImagePushConstants exactly:
// color@0 (vec4, 16 bytes), clip@16 (ivec4, 16 bytes), textureScale@32
// (vec2, 8 bytes) — sizeof 40, byte-identical offsets on both sides (see
// ImagePushConstants's own doc comment for the std430 offset history).
//
// Consumed by: fill_rect.vert, fill_rect.frag, background.frag, image.frag,
// tiled_image.frag.

layout(push_constant) uniform RectPC { vec4 color; ivec4 clip; vec2 textureScale; };
