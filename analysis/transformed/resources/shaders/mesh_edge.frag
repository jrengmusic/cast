#version 450

// mesh_edge.frag — flat-colour fragment stage for the whole-mesh feature-edge
// line-art overlay (mesh_edge.vert's own screen-space quad expansion) —
// deliberately NOT mesh_default.frag: that shader's own Lambert/headlight
// term needs a per-fragment surface normal (mesh_default.vert's own vNormal
// varying), which a screen-space line quad has no meaningful equivalent of
// (an edge's own two adjacent face normals are a per-EDGE property, not a
// per-fragment one, and this overlay is always drawn as a flat, unlit
// constant colour either way — Graphics::recordMeshMaterialRangeDraws()'s
// own featureEdgeColour push). Premultiplies the SAME MeshMaterialPushConstants
// diffuse colour, matching mesh_default.frag's own premultiplied-alpha
// gather-target contract (Graphics::getOrCreateMeshOffscreenRenderPass()'s own
// colour attachment).

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform MeshMaterialPushConstants
{
    vec4 diffuse;
} material;

void main()
{
    fragColor = vec4 (material.diffuse.rgb * material.diffuse.a, material.diffuse.a);
}
