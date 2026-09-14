#version 450
#extension GL_GOOGLE_include_directive : require

#include "mvp.glsl"
#include "push_constants_rect.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 vColor;

void main()
{
    outColor = vColor;
}