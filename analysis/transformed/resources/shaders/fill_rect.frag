#version 450
#extension GL_GOOGLE_include_directive : require

#include "mvp.glsl"
#include "push_constants_rect.glsl"

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(color.rgb * color.a, color.a);
}
