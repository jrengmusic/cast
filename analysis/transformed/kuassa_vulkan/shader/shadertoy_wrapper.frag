#version 450
#extension GL_EXT_nonuniform_qualifier : require

// Bindings 0/1/2 stay declared even though the generated channel/scene macros below
// no longer use their constructor form (glslang rejects sampler2D(texture2D, sampler)
// as a user-function argument — "sampler constructor must appear at point of use").
// They remain harmless, unused declarations, consistent with the shared set-1 layout
// object (jam::VulkanPipelines::createDescriptorSetLayouts()) that every one of the
// 21 main pipelines and every Shader-execution pass binds regardless of which bindings
// its own GLSL actually references. Bindings 3/4 are the first-class sampler2D arrays
// the generated macros index into directly — a legal function-argument value.
layout(set = 0, binding = 0) uniform texture2D textures[];
layout(set = 0, binding = 1) uniform sampler linearSampler;
layout(set = 0, binding = 2) uniform sampler nearestSampler;
layout(set = 0, binding = 3) uniform sampler2D texturesLinear[];
layout(set = 0, binding = 4) uniform sampler2D texturesNearest[];

layout(push_constant) uniform ShaderPC
{
    vec4  iMouse;
    vec2  iResolution;
    float iTime;
    float iTimeDelta;
    int   iFrame;
    int   channels[@maxChannelCount@];
    int   iScene;
    float opacity;
};

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

@channelMacros@@sceneMacro@
@commonSource@
@passSource@
void main()
{
    mainImage (fragColor, uv * iResolution);
}
