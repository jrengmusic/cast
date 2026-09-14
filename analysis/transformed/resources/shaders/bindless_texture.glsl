// bindless_texture.glsl — glslc-include-only, not a standalone shader.
// Declares the Step A3 bindless sampled-image array (set 1) shared by every
// fragment shader that samples through Graphics::bindlessTextureDescriptorSet.
// `#extension GL_EXT_nonuniform_qualifier : require` stays in each including
// file (extension pragmas are not relocated into includes) since it governs
// how each shader's own main() indexes textures[], not this declaration.
//
// binding 2 (nearestSampler) — written once at init alongside linearSampler
// (Graphics::createDrawState()). glyph_mono.frag/glyph_emoji.frag sample their
// own atlas texture through it so bilinear filtering never blends a packed
// glyph's edge texels with its neighbor (jam_GlyphAtlas.h's slotGutter
// reserves the padding this depends on). A shader-execution fragment pass
// (map::ImageResample::value, lexicon/jam_vulkan.md, resolved at
// GLSL-generation time by jam::VulkanShaderCompiler) samples through
// binding 1 or binding 2 directly instead, with no per-execution descriptor
// set of its own (see jam_VulkanShaderUniforms.h's documented GLSL contract).
//
// Consumed by: glyph_emoji.frag, glyph_mono.frag, image.frag,
// tiled_image.frag, fill_rect.frag, gradient_fill.frag, stack_blur_texture.comp.

layout(set = 1, binding = 0) uniform texture2D textures[];
layout(set = 1, binding = 1) uniform sampler linearSampler;
layout(set = 1, binding = 2) uniform sampler nearestSampler;
