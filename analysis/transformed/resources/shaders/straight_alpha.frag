#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

// straight_alpha.frag -- un-premultiplies the resolved scene texture (straight-
// alpha component/scene draws composited over a zero-cleared target read back
// as premultiplied colour) into straight alpha before any post-process user
// shader ever samples it -- Graphics::recordStraightAlphaPass() stamps
// channels[0] with the premultiplied scene's own bindless index
// (jam_VulkanGraphicsSlangPass.cpp) and draws this pass first, so every
// post-process channel/iScene fallback resolves to THIS pass's straight-alpha
// output instead. Consumed against ShaderRegistry::getOrCreateShaderInstanceLayout()
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
    vec4 s = texture(sampler2D(textures[nonuniformEXT(channels[0])], linearSampler), uv);
    fragColor = vec4(s.a > 0.0 ? s.rgb / s.a : vec3(0.0), s.a);
}
