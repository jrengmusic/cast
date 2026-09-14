// Pairs with instanced_rect.vert (StagePair::gradientFill). point1/point2 are
// absolute framebuffer pixel coordinates (same space gl_FragCoord.xy reports,
// matching the CPU-CTM-transformed record positions mvp maps to that space).
// vTexCoord/vColor are declared to match instanced_rect.vert's outputs but
// unused here; the gradient position operand is gl_FragCoord.xy.
#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "bindless_texture.glsl"

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 vColor;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform GradientPC { vec2 point1; vec2 point2; int isRadial; uint lutTextureIndex; float opacity; };

void main()
{
    vec2 axis = point2 - point1;
    float t = isRadial != 0
        ? distance (gl_FragCoord.xy, point1) / distance (point2, point1)
        : dot (gl_FragCoord.xy - point1, axis) / dot (axis, axis);

    t = clamp (t, 0.0, 1.0);

    outColor = texture (sampler2D (textures[nonuniformEXT (lutTextureIndex)], linearSampler), vec2 (t, 0.5)) * opacity;
}
