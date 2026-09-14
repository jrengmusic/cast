// push_constants_glyph.glsl — glslc-include-only, not a standalone shader.
// Declares the GlyphPC push-constant block — different content than RectPC
// (atlasScale instead of color/textureScale) but the same shared 68-byte
// 40-byte pipeline-layout range (jam_VulkanPipelines.cpp's sharedPushConstantRangeSize).
// Stays its own block rather than folding into RectPC — the member types and
// meaning are unrelated to rect/image draws.
//
// Consumed by: glyph_emoji.frag, glyph_mono.frag.

layout(push_constant) uniform GlyphPC { vec4 atlasScale; ivec4 clip; };
