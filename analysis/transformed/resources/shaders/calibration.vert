#version 450

// MSAA calibration measurement draw. Generates a single fullscreen
// triangle purely from gl_VertexIndex (the standard resource-free technique) —
// no vertex buffer, no UBO, no push constants, no descriptor sets of any kind.
// This keeps calibrateSampleCount()/measureCandidateSampleCount() fully
// self-contained, with zero dependency on Pipelines/Graphics::createDrawState()
// (see Graphics::measureCandidateSampleCount()'s doc comment).

void main()
{
    vec2 pos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0 - 1.0;
    gl_Position = vec4(pos, 0.0, 1.0);
}
