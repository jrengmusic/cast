// mvp.glsl — glslc-include-only, not a standalone shader (no #version, no
// entry point). Declares the projection storage buffer shared by every draw
// except calibration.vert/frag (the self-contained MSAA measurement
// pass, which uses no descriptor sets of any kind — see calibration.vert's
// own doc comment).
//
// Consumed by: fill_rect.vert, fill_rect.frag, background.frag, instanced.vert,
// glyph_emoji.frag, glyph_mono.frag, image.frag, image_alpha_mask.frag,
// tiled_image.frag, instanced_rect.vert, instanced_masked.vert, masked_image.frag.

layout(set = 3, binding = 0, std430) readonly buffer MVP { mat4 mvp; } mvp;
