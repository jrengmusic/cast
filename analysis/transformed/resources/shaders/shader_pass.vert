#version 450

// Shared vertex stage for every jam::VulkanShader pass (buffer passes and the
// Image pass alike) — fullscreen triangle generated purely from gl_VertexIndex
// (calibration.vert's exact technique: no vertex buffer, no descriptor sets),
// plus a 0-1 uv varying across the target so each pass's fragment stage can
// address itself without any per-draw transform (no MVP needed).

layout(location = 0) out vec2 uv;

void main()
{
    vec2 pos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0 - 1.0;
    gl_Position = vec4(pos, 0.0, 1.0);
    uv = pos * 0.5 + 0.5;
}
