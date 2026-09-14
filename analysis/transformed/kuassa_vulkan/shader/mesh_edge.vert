#version 450

// mesh_edge.vert — engine-owned mesh vertex-stage TEMPLATE, screen-space
// thick-line quad expansion for the whole-mesh feature-edge overlay
// (Graphics::getOrCreateMeshWireframePipeline() for the default, unanimated
// look; ShaderInstance::buildMeshHookPipelines() for a Shader's own
// mesh_shader= hooked look). Runtime-spliced and compiled by jam::
// VulkanShaderCompiler::compileMeshVertexStage() — the mainMesh placeholder below
// is spliced the SAME way mesh_default.vert's own placeholder is (that
// file's own doc comment, incl. why no comment may spell the token out) —
// ONE template source, never a duplicated hooked/hookless copy.
// vk::PipelineRasterizationStateCreateInfo::lineWidth greater than 1.0
// requires the wideLines device feature, unsupported on MoltenVK/Metal —
// this shader expands each feature edge into a screen-space QUAD (2
// triangles, vk::PrimitiveTopology::eTriangleList) instead of relying on a
// native wide GPU line, so the overlay's own pixel thickness never touches
// lineWidth at all.
//
// Non-indexed draw: Graphics::recordMeshMaterialRangeDraws() issues
// vkCmdDraw(numFeatureEdges * 6, 1, 0, 0) — no bound index buffer for this
// draw (jam::VulkanMesh::getFeatureEdgeIndexBuffer()'s own doc comment:
// its own uint32 endpoint-index pairs are read here as an SSBO, never fed
// through the fixed-function index fetch) — gl_VertexIndex alone decodes
// which edge and which of that edge's own 6 quad-corner vertices this
// invocation is:
//   edgeIndex = gl_VertexIndex / 6
//   corner    = gl_VertexIndex % 6
//
// Reads the SAME vertex-pulling SSBO (binding 1) mesh_default.vert reads,
// plus the feature-edge endpoint-index SSBO (binding 2) to fetch this edge's
// own two endpoint vertex indices, then pulls both endpoints' own
// position+normal out of the vertex SSBO exactly as mesh_default.vert does.
layout(std430, set = 0, binding = 1) readonly buffer VertexBuffer
{
    float data[];
} vertexBuffer;

layout(std430, set = 0, binding = 2) readonly buffer FeatureEdgeBuffer
{
    uint data[];
} featureEdgeBuffer;

// The FULL MeshUniforms block — mesh_default.vert's own doc comment for the
// shared-prefix/unnamed-instance convention (mvp, normalMatrix, iMouse,
// iResolution, iTime, iTimeDelta, iFrame), extended here with this shader's
// OWN trailing tail (edgeThicknessPixels, viewportSize) needed ONLY by its
// own screen-space quad expansion below — both stamped once per frame
// alongside the rest (ShaderInstance::refreshMeshUniforms()). Member order
// matches jam::VulkanShaderInstance::MeshUniforms' own C++ layout exactly —
// see that struct's own doc comment (jam_VulkanShaderInstance.h) for the
// exact std140 byte-offset derivation this declaration order must keep
// matching.
layout(std140, set = 0, binding = 0) uniform MeshUniforms
{
    mat4  mvp;
    mat4  normalMatrix;
    vec4  iMouse;
    vec2  iResolution;
    float iTime;
    float iTimeDelta;
    int   iFrame;
    float edgeThicknessPixels;
    vec2  viewportSize;
};

// Corner -> (endpoint, offset sign) table for the quad's 2 triangles:
//   triangle 0 = corners 0,1,2 = (A + perp), (A - perp), (B + perp)
//   triangle 1 = corners 3,4,5 = (A - perp), (B - perp), (B + perp)
// endpoint 0 = A, 1 = B; sign +1.0 = +perpendicular, -1.0 = -perpendicular.
// Winding is irrelevant — this pipeline's own cullMode is eNone
// (Graphics::createMeshPipelineFixedFunctionState()'s own doc comment).
const int cornerEndpoint[6] = int[](0, 0, 1, 0, 1, 1);
const float cornerOffsetSign[6] = float[](1.0, -1.0, 1.0, -1.0, -1.0, 1.0);

@mainMesh@

void main()
{
    const uint floatsPerVertex = 8u;
    uint edgeIndex = uint(gl_VertexIndex) / 6u;
    uint corner    = uint(gl_VertexIndex) % 6u;

    uint indexA = featureEdgeBuffer.data[edgeIndex * 2u];
    uint indexB = featureEdgeBuffer.data[edgeIndex * 2u + 1u];

    uint baseA = indexA * floatsPerVertex;
    uint baseB = indexB * floatsPerVertex;

    vec3 positionA = vec3(vertexBuffer.data[baseA + 0u], vertexBuffer.data[baseA + 1u], vertexBuffer.data[baseA + 2u]);
    vec3 normalA   = vec3(vertexBuffer.data[baseA + 3u], vertexBuffer.data[baseA + 4u], vertexBuffer.data[baseA + 5u]);
    vec3 positionB = vec3(vertexBuffer.data[baseB + 0u], vertexBuffer.data[baseB + 1u], vertexBuffer.data[baseB + 2u]);
    vec3 normalB   = vec3(vertexBuffer.data[baseB + 3u], vertexBuffer.data[baseB + 4u], vertexBuffer.data[baseB + 5u]);

    // The vertex-animation hook — object space, per endpoint, before the mvp
    // transform below (default, unanimated look: the engine's own no-op
    // mainMesh leaves both endpoints untouched) — so the feature-edge overlay
    // animates in lock-step with the fill draws' own mesh_default.vert hook.
    mainMesh(positionA, normalA);
    mainMesh(positionB, normalB);

    vec4 clipA = mvp * vec4(positionA, 1.0);
    vec4 clipB = mvp * vec4(positionB, 1.0);

    // Perspective divide into screen-PIXEL space (NOT clamped NDC [-1, 1] —
    // multiplied straight through by viewportSize below), so the
    // perpendicular direction/offset computed from screenA/screenB is a
    // genuine PIXEL-space quantity, independent of either endpoint's own
    // clip-space w.
    vec2 ndcA = clipA.xy / clipA.w;
    vec2 ndcB = clipB.xy / clipB.w;
    vec2 screenA = (ndcA * 0.5 + 0.5) * viewportSize;
    vec2 screenB = (ndcB * 0.5 + 0.5) * viewportSize;

    vec2 direction = normalize(screenB - screenA);
    vec2 perpendicular = vec2(-direction.y, direction.x) * (edgeThicknessPixels * 0.5);

    int endpoint = cornerEndpoint[corner];
    float offsetSign = cornerOffsetSign[corner];

    vec2 sourceScreen = endpoint == 0 ? screenA : screenB;
    vec4 sourceClip = endpoint == 0 ? clipA : clipB;
    vec2 expandedScreen = sourceScreen + perpendicular * offsetSign;

    // Back to clip space: NDC recovered from the expanded pixel-space
    // position, then re-multiplied by the SAME source endpoint's own
    // clip-space w (the standard "expand after the perspective divide, then
    // un-divide" thick-line technique) — sourceClip.z/w carry that
    // endpoint's own depth/perspective term through unchanged, only .xy is
    // replaced by the expanded position.
    vec2 expandedNdc = (expandedScreen / viewportSize) * 2.0 - 1.0;
    gl_Position = vec4(expandedNdc * sourceClip.w, sourceClip.z, sourceClip.w);
}
