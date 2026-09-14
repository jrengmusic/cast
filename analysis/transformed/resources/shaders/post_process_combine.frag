#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

// post_process_combine.frag -- the post-process mode's dedicated combine pass:
// samples the Image pass's own offscreen gather target (raw, unmodified
// userColor -- see jam_vulkan/shader/shadertoy_wrapper.frag for
// ShaderFormat::shadertoy, or the pass's own already-complete main()
// compiled as-is with no wrapper for ShaderFormat::slang -- either way
// stripped of this exact formula by the gather/combine split) through channels[0], stamped
// with that gather target's own bindless index (NOT stampChannels()'s
// buffer-pass/scene-fallback semantics -- Graphics::recordPostProcessCompositeDrawCommands()
// stamps channels[0] directly, mirroring straight_alpha.frag's own direct-stamp
// technique), and iScene (straightAlphaImage's own bindless index, carried
// forward unchanged from recordShaderBufferPasses()'s base uniforms -- never
// re-stamped here), then applies the exact effect-intensity mix formula the
// Image pass's compiled shader used to compute itself (jam_VulkanShaderUniforms.h's
// documented opacity formula) over the STRAIGHT-alpha scene -- opacity 0
// reproduces the resolved scene's rgb exactly, opacity 1 is the user shader's
// own unmixed rgb, the result is re-premultiplied by scene.a and scene.a is
// carried through unchanged (glass alpha stays immutable end-to-end). Drawn by
// Graphics::getOrCreatePostProcessCombinePipeline() with
// Pipelines::opaqueBlendAttachment() into compositeRenderPass -- this combine
// shader's own mix() already does the blending math, so the fixed-function
// stage stays a direct overwrite (matches today's existing post-process
// contract). Consumed against Graphics::getOrCreateShaderInstanceLayout()
// (set 0 = the bindless texture array, reused verbatim from
// Pipelines::getLayoutSet1()) -- same push-constant block, same set/binding
// shape as every other Shader-execution pass; see jam_VulkanShaderUniforms.h's
// documented GLSL contract. uv matches shader_pass.vert's shared fullscreen-
// triangle varying.

#include "push_constants_shader.glsl"

layout(set = 0, binding = 0) uniform texture2D textures[];
layout(set = 0, binding = 1) uniform sampler linearSampler;
layout(set = 0, binding = 2) uniform sampler nearestSampler;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

void main()
{
    vec4 userColor = texture(sampler2D(textures[nonuniformEXT(channels[0])], linearSampler), uv);
    vec4 scene = texture(sampler2D(textures[nonuniformEXT(iScene)], linearSampler), uv);
    fragColor = vec4(mix(scene.rgb, userColor.rgb, opacity) * scene.a, scene.a);
}
