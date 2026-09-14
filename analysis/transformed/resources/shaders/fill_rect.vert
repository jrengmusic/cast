#version 450
#extension GL_GOOGLE_include_directive : require

#include "mvp.glsl"
#include "push_constants_rect.glsl"

layout(location = 0) in vec2 position;

void main()
{
    gl_Position = mvp.mvp * vec4(position, 0.0, 1.0);
}