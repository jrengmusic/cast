#version 450

// mesh_default.vert — engine-owned mesh vertex-stage TEMPLATE, the fill and
// transparent-fill pipelines' own vertex stage (Graphics::getOrCreateMeshPipeline()/
// getOrCreateMeshTransparentFillPipeline() for the default, unanimated look;
// ShaderInstance::buildMeshHookPipelines() for a Shader's own mesh_shader=
// hooked look). Runtime-spliced and compiled by jam::VulkanShaderCompiler::
// compileMeshVertexStage() — the mainMesh placeholder below (the ONLY place
// the at-sign token may appear: replaceholder() substitutes EVERY
// occurrence, so this comment must never spell it out) is spliced with either
// this engine's own no-op mainMesh (default, unanimated look) or a project's own
// mesh_shader= mainMesh snippet (mesh_shader= is a vertex-ANIMATION
// HOOK into this engine's own default mesh look, mirroring Shadertoy's
// mainImage paradigm one level down — never a full-stage replacement). ONE
// template source, never a duplicated hooked/hookless copy of this file.
//
// Vertex-pulling: no vertex-input bindings at all
// (Graphics::getOrCreateMeshPipeline()'s own zero vertex-input state) — every
// {position,normal,uv} is pulled from the descriptor-bound SSBO below via
// gl_VertexIndex, driven by an INDEXED draw (commandBuffer.bindIndexBuffer(),
// Graphics::recordMeshGatherDrawCommands()) so gl_VertexIndex equals the
// fetched INDEX VALUE — correct for SSBO pulling with indexed draws.
//
// Flat float array, not an array of a vec3-bearing struct — matches
// jam::VulkanVertex's own documented std430 layout contract exactly
// (jam_vulkan/resource/jam_VulkanVertex.h): a literal `vec3 position;` GLSL
// struct member has a 16-byte std430 BASE ALIGNMENT (not its 12-byte size),
// which would silently pad this record out to 40+ bytes and break the
// byte-for-byte correspondence jam::VulkanMesh's own CPU-side interleave/
// upload relies on. Manual `gl_VertexIndex * 8 + component` addressing
// preserves the exact 32-byte stride instead.
layout(std430, set = 0, binding = 1) readonly buffer VertexBuffer
{
    float data[];
} vertexBuffer;

// Own dedicated UBO — this pipeline's own set 0 binding 0
// (Graphics::getOrCreateMeshDescriptorSetLayout()). UNNAMED instance — every
// member below is therefore a bare global (mvp, normalMatrix, iMouse,
// iResolution, iTime, iTimeDelta, iFrame), matching shadertoy_wrapper.frag's
// own anonymous ShaderPC convention (jam_VulkanShaderUniforms.h) so an
// author's mainMesh snippet reads the standard iTime/iTimeDelta/iFrame/
// iResolution/iMouse names bare, exactly like any other shader pass — zero
// bespoke uniform vocabulary. This is a PREFIX of the full block
// mesh_edge.vert also declares (that shader's own trailing
// edgeThicknessPixels/viewportSize members are simply never declared here —
// a GLSL uniform block only cares about the members IT declares, both
// shaders bind the SAME underlying buffer). Member order matches
// jam::VulkanShaderInstance::MeshUniforms' own C++ layout exactly — see
// that struct's own doc comment (jam_VulkanShaderInstance.h) for the exact
// std140 byte-offset derivation this declaration order must keep matching.
layout(std140, set = 0, binding = 0) uniform MeshUniforms
{
    mat4  mvp;
    mat4  normalMatrix;
    vec4  iMouse;
    vec2  iResolution;
    float iTime;
    float iTimeDelta;
    int   iFrame;
};

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec2 vTexCoord;

@mainMesh@

void main()
{
    const uint floatsPerVertex = 8u;
    uint base = uint(gl_VertexIndex) * floatsPerVertex;

    vec3 position = vec3(vertexBuffer.data[base + 0u], vertexBuffer.data[base + 1u], vertexBuffer.data[base + 2u]);
    vec3 normal   = vec3(vertexBuffer.data[base + 3u], vertexBuffer.data[base + 4u], vertexBuffer.data[base + 5u]);
    vec2 uv       = vec2(vertexBuffer.data[base + 6u], vertexBuffer.data[base + 7u]);

    // The vertex-animation hook — object space, before the mvp/normal-matrix
    // transforms below (default, unanimated look: the engine's own no-op
    // mainMesh leaves position/normal untouched).
    mainMesh(position, normal);

    gl_Position = mvp * vec4(position, 1.0);

    // normalMatrix already carries the view transform (jam::
    // VulkanOrbitCamera::getNormalMatrix()) — vNormal is therefore a VIEW-space
    // normal, matching mesh_default.frag's own view-space headlight term.
    vNormal = mat3(normalMatrix) * normal;
    vTexCoord = uv;
}
