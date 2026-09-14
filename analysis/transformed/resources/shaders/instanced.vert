// Shared vertex stage for every instanced stage pair whose fragment stage
// bindless-samples a texture (instancedImage, tiled, instancedGlyphMono,
// instancedGlyphEmoji, instancedClipMask) — outputs vTextureIndex for them.
// StagePair::instancedRect and StagePair::background use instanced_rect.vert
// instead (this file minus vTextureIndex): fill_rect.frag and background.frag
// never sample a texture. StagePair::maskedImage uses instanced_masked.vert
// instead (this file plus the mask index output).
#version 450
#extension GL_GOOGLE_include_directive : require

#include "mvp.glsl"

struct PrimitiveRecord
{
    vec2 position;
    vec2 size;
    vec4 uvRect;
    vec4 color;
    ivec4 clip;
    uint textureIndex;
    uint stencilRef;
    uint flags;
    uint maskTextureIndex;
};

layout(set = 2, binding = 0, std430) readonly buffer PrimitiveRecords
{
    PrimitiveRecord records[];
};

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 vColor;
layout(location = 2) flat out uint vTextureIndex;

void main()
{
    PrimitiveRecord record = records[gl_InstanceIndex];

    // Quad corners (triangle-strip order): 0=TL, 1=TR, 2=BL, 3=BR — matches
    // fill_rect.vert's corner convention exactly.
    const vec2 cornerOffsets[4] = vec2[4](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0)
    );
    int vi = gl_VertexIndex % 4;
    vec2 pixelPos = record.position + cornerOffsets[vi] * record.size;

    // Derive UV from record.uvRect: xy = UV origin, zw = UV extent.
    vec2 uv = record.uvRect.xy + cornerOffsets[vi] * record.uvRect.zw;

    gl_Position = mvp.mvp * vec4(pixelPos, 0.0, 1.0);
    vTexCoord = uv;
    vColor = record.color;
    vTextureIndex = record.textureIndex;
}
