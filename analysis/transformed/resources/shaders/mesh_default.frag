#version 450

// mesh_default.frag — the default engine-owned lit fragment stage:
// Lambert/headlight (light at the camera, computed
// in VIEW space — mesh_default.vert's own vNormal is already view-space,
// jam::VulkanOrbitCamera::getNormalMatrix()'s own doc comment), sampling
// the material's diffuse colour from a per-MaterialRange push constant
// (Graphics::recordMeshGatherDrawCommands()'s own per-drawIndexed-range
// push, jam::VulkanShaderInstance::MeshMaterialPushConstants). Output
// premultiplied alpha (rgb scaled by the material's own alpha) — the
// real-material fill variant's push always carries alpha 1.0
// (Graphics::recordMeshMaterialRangeDraws()'s own materialPushConstants),
// so premultiplication is a no-op there; the default-material transparent-fill
// variant's low-alpha push (same method's fillPushConstants) composites
// correctly against the gather target via Pipelines::alphaBlendAttachment()'s
// premultiplied-over equation (Graphics::getOrCreateMeshOffscreenRenderPass()'s
// own colour attachment).

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform MeshMaterialPushConstants
{
    vec4 diffuse;
} material;

void main()
{
    // Headlight direction, view space: GLM_FORCE_LEFT_HANDED is NOT defined
    // (jam_vulkan.h's own doc comment — right-handed is glm's default, the
    // correct chirality here, fixing the mirrored-render bug the previous
    // left-handed configuration caused) — glm::lookAtRH()'s own convention
    // maps the eye-to-target "forward" direction onto view-space -Z (the
    // standard OpenGL/RH camera looks down its own view space's negative Z
    // axis), so the direction FROM a surface point TOWARD the camera (the
    // headlight's own light-to-surface vector, reversed) is +Z, not -Z.
    vec3 normal = normalize(vNormal);
    float headlightTerm = max(dot(normal, vec3(0.0, 0.0, 1.0)), 0.0);

    fragColor = vec4(material.diffuse.rgb * headlightTerm * material.diffuse.a, material.diffuse.a);
}
