#version 450

// MSAA calibration measurement draw. Solid output — this measures
// MSAA resolve/fill-rate cost, not shading cost, so the exact color is
// irrelevant. No inputs of any kind (no UBO, no push constants, no textures).

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
}
