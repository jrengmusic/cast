#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

// background_combine.frag -- the background mode's dedicated combine pass:
// samples the Image pass's own offscreen gather target (raw, unmodified
// userColor -- see jam_vulkan/shader/shadertoy_wrapper.frag for
// ShaderFormat::shadertoy, or the pass's own already-complete main()
// compiled as-is with no wrapper for ShaderFormat::slang -- either way
// stripped of this exact formula by the gather/combine split) through channels[0], stamped
// with that gather target's own bindless index (NOT stampChannels()'s
// buffer-pass/scene-fallback semantics -- Graphics::recordShaderImagePassDrawCommands()
// stamps channels[0] directly, mirroring straight_alpha.frag's own direct-stamp
// technique), then applies the exact component-transparency formula the Image
// pass's compiled shader used to compute itself (jam_VulkanShaderUniforms.h's
// documented opacity formula) -- opacity 0 is fully transparent, opacity 1 is
// the user shader's own alpha -- and premultiplies rgb by that combined alpha
// itself, since Pipelines::alphaBlendAttachment()'s srcRgbFactor is eOne
// (colour arriving already premultiplied). Drawn by
// Graphics::getOrCreateBackgroundCombinePipeline() with
// Pipelines::alphaBlendAttachment() into the scene's own render pass (renderPass) --
// this fixed-function blend composites the already-premultiplied result over
// whatever the scene render pass already painted beneath it, without
// re-scaling rgb. Consumed against
// Graphics::getOrCreateShaderInstanceLayout() (set 0 = the bindless texture
// array, reused verbatim from Pipelines::getLayoutSet1()) -- same push-constant
// block, same set/binding shape as every other Shader-execution pass; see
// jam_VulkanShaderUniforms.h's documented GLSL contract. uv matches
// shader_pass.vert's shared fullscreen-triangle varying.

#include "push_constants_shader.glsl"

layout(set = 0, binding = 0) uniform texture2D textures[];
layout(set = 0, binding = 1) uniform sampler linearSampler;
layout(set = 0, binding = 2) uniform sampler nearestSampler;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

void main()
{
    vec4 userColor = texture(sampler2D(textures[nonuniformEXT(channels[0])], linearSampler), uv);
    fragColor = vec4(userColor.rgb * (userColor.a * opacity), userColor.a * opacity);
}
