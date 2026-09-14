// push_constants_shader.glsl — glslc-include-only, not a standalone shader.
// Declares the ShaderPC push-constant block — Shadertoy-compatible uniforms
// (iMouse, iResolution, iTime, iTimeDelta, iFrame, channels[]) plus iScene
// and opacity for the combine passes that mix a Shader execution's own
// output against the resolved scene.
//
// Consumed by: post_process_combine.frag, background_combine.frag,
// straight_alpha.frag.

layout(push_constant) uniform ShaderPC
{
    vec4  iMouse;
    vec2  iResolution;
    float iTime;
    float iTimeDelta;
    int   iFrame;
    int   channels[21];// literal = jam::VulkanShaderUniforms::maxChannelCount
    int   iScene;
    float opacity;
};
