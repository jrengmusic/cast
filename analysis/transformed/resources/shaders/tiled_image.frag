#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "mvp.glsl"
#include "push_constants_rect.glsl"
#include "bindless_texture.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 vTexCoord;
layout(location = 2) flat in uint vTextureIndex;

void main()
{
    outColor = texture(sampler2D(textures[nonuniformEXT(vTextureIndex)], linearSampler), fract(vTexCoord * textureScale)) * color.a;
}
