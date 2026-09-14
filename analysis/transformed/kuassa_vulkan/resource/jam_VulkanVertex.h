namespace jam
{
/*____________________________________________________________________________*/
/** @brief GPU-resident interleaved mesh vertex — {position, normal, uv}.
 *
 *  Distinct from the CPU-side jam::WavefrontObj::Vertex (position-only, one
 *  lane of that loader's per-shape SoA pools — jam_graphics/mesh/
 *  jam_WavefrontObj.h). This POD is the GPU vertex-pulling record
 *  jam::VulkanMesh::build() (resource/jam_VulkanMesh.h) interleaves those
 *  SoA pools INTO, per this contract: "geometry uploaded once into a single
 *  SSBO".
 *
 *  @par std430 / vertex-pulling layout contract
 *  32 bytes, tightly packed, no padding — every field below is a scalar
 *  float or a fixed-size array of scalar floats, never a GLSL-alignment-
 *  bearing vec3/vec4 type. This matters because the mesh-backed shader pass
 *  pulls this exact record from an SSBO via gl_VertexIndex: a GLSL struct
 *  declared with a literal `vec3 position;` member has a 16-byte BASE
 *  ALIGNMENT under std430 (not its 12-byte size) — a naive `struct { vec3
 *  position; vec3 normal; vec2 uv; }` SSBO element would silently pad out to
 *  40+ bytes (4 bytes after each vec3), breaking the byte-for-byte
 *  correspondence this type's CPU-side interleave/upload relies on. Its SSBO
 *  declaration therefore indexes a flat `float` array with manual
 *  `gl_VertexIndex * 8 + component` addressing rather than an array of a
 *  vec3-bearing GLSL struct, to preserve this exact 32-byte stride with no
 *  padding surprises — hence the static_assert below.
 */
struct VulkanVertex
{
    /** Object-space vertex position. */
    float position[3];
    /** Object-space vertex normal. */
    float normal[3];
    /** Texture coordinate. */
    float uv[2];
};

static_assert (sizeof (VulkanVertex) == 32,
              "jam::VulkanVertex must stay a tightly packed 32-byte record — "
              "see the class doc comment's std430/vertex-pulling layout contract.");

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam