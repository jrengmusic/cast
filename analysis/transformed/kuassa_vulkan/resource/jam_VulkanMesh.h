namespace jam
{
/*____________________________________________________________________________*/
/** @brief GPU-resident static mesh: one device-local vertex-pulling SSBO +
 *  one device-local index buffer, interleaved and staged-uploaded ONCE from
 *  a jam::WavefrontObj parse ("geometry uploaded once into a single SSBO").
 *
 *  @par Feature-edge line art (Blender-style, view-independent)
 *  build() also derives a SECOND, whole-mesh buffer (featureEdgeIndexBuffer,
 *  uint32 endpoint-index pairs, eStorageBuffer — read as an SSBO by
 *  mesh_edge.vert's own screen-space thick-line quad expansion,
 *  jam_vulkan/shader/mesh_edge.vert; NOT a vk::IndexType index buffer — see
 *  getFeatureEdgeIndexBuffer()'s own doc comment for why "Index" still names
 *  this buffer's own CONTENT, not its GPU binding) of "feature" edges only —
 *  boundary edges (touched by exactly one triangle) and crease edges (touched
 *  by exactly two triangles whose face normals differ by more than
 *  classifyFeatureEdges()'s own dihedral-angle threshold) — every smooth
 *  interior edge is dropped. This is a build-time, view-INDEPENDENT
 *  approximation (deliberately NOT silhouette detection, which is camera-
 *  relative and would need per-frame recomputation): the SAME edge set is
 *  correct from every orbit angle, exactly like a CAD/Blender-style line-art
 *  overlay. Edge adjacency is computed by exact object-space position (see
 *  PositionKey's own doc comment for why — jam::WavefrontObj's per-shape
 *  vertex-position-pool index does not survive parse), accumulated across
 *  EVERY shape in interleave()'s existing per-shape walk (accumulateFeatureEdges()),
 *  then classified once, whole-mesh, at the end (classifyFeatureEdges()) —
 *  feature edges are therefore a property of the WHOLE VulkanMesh, not any single
 *  jam::WavefrontObj::Shape/MaterialRange (jam::VulkanGraphics::
 *  recordMeshMaterialRangeDraws()'s own doc comment: drawn once per mesh,
 *  never once per range).
 *
 *  @par Window-agnostic (NOT per-window)
 *  Unlike jam::VulkanBindlessTexture (per-window bindless SLOT registry
 *  over a shared VulkanImage), a VulkanMesh has no per-window concern at all — its two
 *  device-local buffers are consumed by the mesh-backed shader pass via a
 *  descriptor binding, and descriptor-set binding is the per-window concern
 *  (exactly like VulkanBindlessTexture's slot registry), not this type's. A VulkanMesh
 *  built once is valid and shareable across every window's VulkanGraphics, same as
 *  a shared atlas VulkanImage is shareable before any window ever assigns it a
 *  bindless slot.
 *
 *  @par VulkanBuffer usage — SSBO, not vertex-input
 *  The vertex buffer is created with eStorageBuffer | eTransferDst, NOT
 *  eVertexBuffer. This buffer's own design is vertex PULLING: the
 *  mesh-backed shader pass reads {position,normal,uv}
 *  out of this SSBO manually, indexed by gl_VertexIndex, inside the vertex
 *  shader — programmable vertex pulling: geometry in an SSBO pulled via
 *  gl_VertexIndex, the zeux.io / vkguide GPU-driven rendering precedent —
 *  there is no vk::VertexInputBindingDescription
 *  for this buffer and no vkCmdBindVertexBuffers call anywhere in its
 *  consumption path, so eVertexBuffer usage would be a false declaration
 *  (and VMA/validation-layer noise) for a binding that is never made. The
 *  index buffer keeps eIndexBuffer | eTransferDst — it still feeds the
 *  fixed-function index fetch (vkCmdBindIndexBuffer) that DRIVES
 *  gl_VertexIndex itself; that binding genuinely exists.
 *
 *  @par Population — interleave, then one staged upload
 *  build() walks every jam::WavefrontObj::Shape's per-shape SoA VulkanMesh
 *  (vertices/normals/textureCoords, index-aligned per shape) and interleaves
 *  them into one contiguous jam::Array<VulkanVertex> + one jam::Array<uint32_t>
 *  index array — per-shape indices are re-based by that shape's running
 *  vertex-count offset, folding every shape into ONE buffer pair for the
 *  whole object (a single-SSBO contract; no per-shape buffers). The
 *  interleave pass also accumulates the object's AABB (VulkanOrbitCamera's
 *  auto-fit consumer) and one MaterialRange per shape (firstIndex/numIndices
 *  into the now-shared index buffer + that shape's own jam::WavefrontObj::
 *  Material, copied — the mesh draw loop (VulkanGraphics::
 *  recordMeshMaterialRangeDraws()) iterates these to bind/stamp each
 *  shape's material for its index sub-range; the default lit shader samples
 *  diffuse only for now).
 *
 *  @par Staged upload — the buffer-copy analog of the existing image path
 *  This codebase's only existing staged-upload precedent is buffer-TO-IMAGE
 *  (jam::VulkanBindlessTexture::upload(), VulkanGraphics::cacheImageTexture(),
 *  both built on resource/jam_VulkanUploadHelpers.h's createStagingBuffer());
 *  no buffer-to-buffer staged copy exists anywhere in jam_vulkan today (grep-
 *  confirmed). build() mirrors that exact idiom with vk::CommandBuffer::
 *  copyBuffer() standing in for copyBufferToImage() (the direct buffer
 *  analog — no image layout, no vk::BufferImageCopy, just a byte-range
 *  vk::BufferCopy per destination buffer): allocate a CPU_ONLY mapped
 *  staging VulkanBuffer sized for BOTH destinations via createStagingBuffer(),
 *  memcpy the vertex bytes then the index bytes into it back-to-back, record
 *  one copyBuffer() per destination, then one vk::MemoryBarrier
 *  (TRANSFER_WRITE -> SHADER_READ | INDEX_READ, TRANSFER -> VERTEX_SHADER |
 *  VERTEX_INPUT stage) — the buffer-copy equivalent of the image path's
 *  layout-transition barriers, since a plain buffer copy carries no layout
 *  transition to piggy-back synchronisation on. The staging VulkanBuffer is NOT a
 *  per-frame arena allocation (VulkanGraphics::allocateStaging() — that arena is
 *  reset after ITS OWN window's next-frame fence wait, which has no
 *  relationship to a VulkanMesh built once at load time, independent of any
 *  window's frame cadence): it is kept alive as a member past build()'s
 *  return — see releaseStaging()'s doc comment — because the copy command
 *  recorded into the caller's commandBuffer must still reference valid
 *  staging memory until that command buffer's GPU execution actually
 *  completes, which build() itself cannot observe (fence ownership is the
 *  caller's, exactly as it is for every other command-buffer-recording call
 *  in this codebase). No per-frame re-upload — build() runs once per VulkanMesh.
 *
 *  Move-only. Non-copyable.
 */
class VulkanMesh
{
public:
    /** @brief One shape's material binding over a contiguous sub-range of
     *  this VulkanMesh's shared index buffer — the mesh draw loop
     *  (VulkanGraphics::recordMeshMaterialRangeDraws()) iterates these. */
    struct MaterialRange
    {
        uint32_t firstIndex { 0 };
        uint32_t numIndices { 0 };
        jam::WavefrontObj::Material material;
    };

    /** @brief Default constructor — produces an empty, unbuilt VulkanMesh. */
    VulkanMesh() = default;

    /** @brief Destroys the owned buffers via their RAII VulkanBuffer destructors. */
    ~VulkanMesh() = default;

    /** @brief Move constructor — transfers buffer ownership, leaves source empty. */
    VulkanMesh (VulkanMesh&&) noexcept = default;

    /** @brief Move assignment — transfers buffer ownership, leaves source empty. */
    VulkanMesh& operator= (VulkanMesh&&) noexcept = default;

    /** @brief Interleaves every shape's SoA geometry into this VulkanMesh's device-
     *  local vertex SSBO + index buffer and records a ONE-TIME staged upload
     *  into @p commandBuffer — see the class doc comment for the full
     *  interleave/staging contract. VulkanBuffer creation factored into
     *  createGeometryBuffers(), the staged copy into
     *  recordStagedGeometryUpload() — see each's own doc comment.
     *  @param device         Shared Vulkan device — supplies the VMA allocator.
     *  @param commandBuffer  Active command buffer to record the staged
     *                        copy + upload barrier into (the caller's own
     *                        per-frame recording, exactly as every other
     *                        staged-upload call site in this codebase).
     *  @param shapes         Parsed jam::WavefrontObj::Shape list — one
     *                        object's full geometry, parsed ONCE by
     *                        VulkanShaderCompiler::compile() (jam::WavefrontObj::
     *                        extractShapes()) and owned by the caller's own
     *                        jam::VulkanShader (VulkanShader::meshShapes).
     *  @return true if both buffers were created and the copy was recorded.
     */
    bool build (VulkanDevice& device, vk::CommandBuffer commandBuffer,
               const jam::Owner<jam::WavefrontObj::Shape>& shapes)
    {
        jam::Array<VulkanVertex>   interleavedVertices;
        jam::Array<uint32_t> interleavedIndices;
        jam::Array<uint32_t> interleavedFeatureEdgeIndices;

        interleave (shapes, interleavedVertices, interleavedIndices, interleavedFeatureEdgeIndices);

        numVertices           = static_cast<uint32_t> (interleavedVertices.size());
        numIndices            = static_cast<uint32_t> (interleavedIndices.size());
        numFeatureEdgeIndices = static_cast<uint32_t> (interleavedFeatureEdgeIndices.size());

        bool built { false };

        if (numVertices > 0 and numIndices > 0)
        {
            const vk::DeviceSize vertexBytes      { static_cast<vk::DeviceSize> (numVertices) * sizeof (VulkanVertex) };
            const vk::DeviceSize indexBytes       { static_cast<vk::DeviceSize> (numIndices) * sizeof (uint32_t) };
            const vk::DeviceSize featureEdgeBytes { static_cast<vk::DeviceSize> (numFeatureEdgeIndices) * sizeof (uint32_t) };

            built = createGeometryBuffers (device, vertexBytes, indexBytes, featureEdgeBytes);

            if (built)
                built = recordStagedGeometryUpload (device, commandBuffer, interleavedVertices, interleavedIndices,
                                                    interleavedFeatureEdgeIndices, vertexBytes, indexBytes,
                                                    featureEdgeBytes);
        }

        return built;
    }

    /** @brief Releases the temporary staging VulkanBuffer created by build().
     *
     *  Call once the caller's own command-buffer submission has been fenced
     *  (the same "kept alive until the GPU is known to be done" discipline
     *  as VulkanFrameBuffer/VulkanPrimitiveRecordBuffer's previousBuffers sweep, resource/
     *  jam_FrameBuffer.h — applied here to a single one-shot staging VulkanBuffer
     *  rather than a per-frame growth-away list, since build() runs once).
     *  Safe to call on a VulkanMesh whose build() never ran or already released
     *  (stagingBuffer's own move-assignment is safely idempotent).
     */
    void releaseStaging() noexcept { stagingBuffer = VulkanBuffer {}; }

    /** @brief Returns the device-local vertex-pulling SSBO's vk::Buffer handle. */
    vk::Buffer getVertexBuffer() const noexcept { return vertexBuffer.getBuffer(); }

    /** @brief Returns the device-local index buffer's vk::Buffer handle. */
    vk::Buffer getIndexBuffer() const noexcept { return indexBuffer.getBuffer(); }

    /** @brief Returns the device-local feature-edge endpoint-index buffer's
     *  vk::Buffer handle — class doc comment's "Feature-edge line art"
     *  section. eStorageBuffer usage only (NOT eIndexBuffer — see
     *  createGeometryBuffers()'s own doc comment for why): read as an SSBO by
     *  mesh_edge.vert (jam_vulkan/shader/mesh_edge.vert), never bound via
     *  vkCmdBindIndexBuffer. "Index" still names this buffer's own CONTENT
     *  (uint32 vertex-SSBO indices, in endpoint pairs), not its GPU binding
     *  point. Empty handle when getNumFeatureEdgeIndices() is 0 (no feature
     *  edge was classified at all — never bound in that case). */
    vk::Buffer getFeatureEdgeIndexBuffer() const noexcept { return featureEdgeIndexBuffer.getBuffer(); }

    /** @brief Returns the total interleaved vertex count across every shape. */
    uint32_t getNumVertices() const noexcept { return numVertices; }

    /** @brief Returns the total index count across every shape. */
    uint32_t getNumIndices() const noexcept { return numIndices; }

    /** @brief Returns the total feature-edge index count (2 endpoint indices
     *  per classified edge) — VulkanGraphics::recordMeshMaterialRangeDraws()'s own
     *  non-indexed draw derives its own vertex count from this value
     *  (numFeatureEdgeIndices / 2 edges, 6 vertices — one screen-space
     *  quad's own 2 triangles — per edge, mesh_edge.vert's own doc comment).
     *  See getFeatureEdgeIndexBuffer()'s own doc comment. */
    uint32_t getNumFeatureEdgeIndices() const noexcept { return numFeatureEdgeIndices; }

    /** @brief Returns the object-space AABB minimum corner — VulkanOrbitCamera's
     *  auto-fit consumer. */
    const float* getAabbMin() const noexcept { return aabbMin; }

    /** @brief Returns the object-space AABB maximum corner — VulkanOrbitCamera's
     *  auto-fit consumer. */
    const float* getAabbMax() const noexcept { return aabbMax; }

    /** @brief Returns the per-shape material sub-ranges over the shared
     *  index buffer — the mesh draw loop (VulkanGraphics::
     *  recordMeshMaterialRangeDraws()) consumer. */
    const jam::Array<MaterialRange>& getMaterialRanges() const noexcept { return materialRanges; }

    /** @brief Returns true once build() has produced both valid buffers. */
    bool isValid() const noexcept { return vertexBuffer.isValid() and indexBuffer.isValid(); }

private:
    /** @brief Exact object-space position key for feature-edge adjacency —
     *  jam::WavefrontObj::getOrAppendVertex() (jam_WavefrontObj.cpp)
     *  copies a vertex's position verbatim from its shared position pool
     *  (`currentShape->mesh.vertices.push_back (positions.at (static_cast<size_t> (ref.vertexIndex)))`),
     *  so two mesh vertices sharing one OBJ position — split into separate
     *  mesh vertices only because a UV/normal seam gave them different
     *  TripleIndex triples — hold bit-identical float bytes here; exact
     *  float compare is therefore safe, no epsilon needed. jam::WavefrontObj::
     *  TripleIndex::vertexIndex (the OBJ-native position-pool index) is
     *  parse-transient and PRIVATE to jam::WavefrontObj (jam_WavefrontObj.h's
     *  own vertexPositionIndices member, reset every load(), never copied
     *  into Shape::Mesh) — this exact-byte position key is the available
     *  substitute, built fresh in this class's own accumulateFeatureEdges().
     */
    struct PositionKey
    {
        float x, y, z;

        bool operator== (const PositionKey& other) const noexcept
        {
            return x == other.x and y == other.y and z == other.z;
        }

        /** @brief Lexicographic order — accumulateFeatureEdges()'s own
         *  canonical (lower, higher) edge-endpoint ordering, so an edge seen
         *  as (a,b) from one triangle and (b,a) from its neighbour hashes
         *  and compares identically. */
        bool operator< (const PositionKey& other) const noexcept
        {
            return x < other.x
                or (x == other.x and (y < other.y
                or (y == other.y and z < other.z)));
        }

        /** @brief Wyhash of the raw 12-byte buffer — mirrors jam::WavefrontObj::
         *  TripleIndex::Hash's own wyhash-primitive convention
         *  (jam_WavefrontObj.cpp), adapted for this key's 3 contiguous floats
         *  via the same direct-byte-buffer idiom jam::HashMap's own
         *  Hash<std::basic_string<CharT>> specialization uses
         *  (jam_HashMap.h). */
        struct Hash
        {
            size_t operator() (const PositionKey& key) const noexcept
            {
                return static_cast<size_t> (jam::Wyhash::hashBytes (&key, sizeof (key)));
            }
        };
    };

    /** @brief Undirected edge key — two PositionKeys in canonical
     *  (lower, higher) order (PositionKey::operator<), so the SAME edge
     *  shared by two triangles hashes/compares identically regardless of
     *  which triangle's own winding direction it was encountered from. */
    struct EdgeKey
    {
        PositionKey first, second;

        bool operator== (const EdgeKey& other) const noexcept
        {
            return first == other.first and second == other.second;
        }

        struct Hash
        {
            size_t operator() (const EdgeKey& key) const noexcept
            {
                return static_cast<size_t> (jam::Wyhash::mix (
                    PositionKey::Hash {} (key.first), PositionKey::Hash {} (key.second)));
            }
        };
    };

    /** @brief accumulateFeatureEdges()'s own running per-edge accumulator —
     *  the global vertex-index pair the classified edge will be emitted as
     *  (the FIRST occurrence's own indices — every duplicate occurrence
     *  shares the identical object-space position, so any occurrence's own
     *  index pair draws the same line), the adjacent-face count (1 =
     *  boundary, 2 = crease candidate, 3+ = non-manifold, dropped —
     *  classifyFeatureEdges()'s own doc comment), and up to 2 adjacent
     *  triangles' own geometric face normals (dihedral-angle input). */
    struct EdgeInfo
    {
        uint32_t globalVertexA { 0 };
        uint32_t globalVertexB { 0 };
        uint8_t  faceCount { 0 };
        glm::vec3 faceNormals[2] { glm::vec3 { 0.0f }, glm::vec3 { 0.0f } };
    };

    /** @brief Crease-edge dihedral-angle threshold, as the cosine of 30
     *  degrees — classifyFeatureEdges() keeps a 2-face edge as a crease
     *  when its two adjacent face normals' dot product falls BELOW this
     *  value (their dihedral angle exceeds 30 degrees); a smaller dot
     *  product means a LARGER angle between the normals, hence the
     *  below-threshold (not above) test. */
    static constexpr float featureEdgeCreaseCosineThreshold { 0.8660254038f }; // std::cos (glm::radians (30.0f)), evaluated offline

    /** @brief Interleaves every shape's per-shape SoA VulkanMesh into one
     *  contiguous vertex array + one re-based index array, and accumulates
     *  this VulkanMesh's AABB + per-shape MaterialRange list + whole-mesh feature-
     *  edge index list as it goes — see the class doc comment's
     *  "Population"/"Feature-edge line art" sections. AABB accumulation
     *  factored into accumulateAabb(), MaterialRange assembly into
     *  appendMaterialRange(), feature-edge adjacency accumulation into
     *  accumulateFeatureEdges(), final classification into
     *  classifyFeatureEdges() (run once, after every shape's own triangles
     *  have contributed to the shared edgeAccumulator — an edge crossing a
     *  shape boundary still classifies correctly) — see each's own doc
     *  comment.
     *  @param shapes                 Parsed jam::WavefrontObj::Shape list —
     *                                one object's full geometry — see
     *                                build()'s own @p shapes doc comment
     *                                above for its parse-once provenance.
     *  @param outVertices            Interleaved whole-object vertex array —
     *                                build()'s vertex-SSBO staging payload.
     *  @param outIndices             Re-based whole-object triangle indices —
     *                                build()'s index-buffer staging payload.
     *  @param outFeatureEdgeIndices  Whole-mesh feature-edge line-list
     *                                indices (2 per edge) — build()'s own
     *                                second staged-upload payload.
     */
    void interleave (const jam::Owner<jam::WavefrontObj::Shape>& shapes,
                     jam::Array<VulkanVertex>& outVertices, jam::Array<uint32_t>& outIndices,
                     jam::Array<uint32_t>& outFeatureEdgeIndices)
    {
        aabbMin[0] = aabbMin[1] = aabbMin[2] = std::numeric_limits<float>::max();
        aabbMax[0] = aabbMax[1] = aabbMax[2] = std::numeric_limits<float>::lowest();
        materialRanges.clear();

        uint32_t vertexBase { 0 };
        jam::HashMap<EdgeKey, EdgeInfo, EdgeKey::Hash> edgeAccumulator;

        for (const auto& shapePtr : shapes)
        {
            const auto& shape { *shapePtr };
            const auto& shapeMesh { shape.mesh };

            jassert (shapeMesh.vertices.size() == shapeMesh.normals.size());
            jassert (shapeMesh.vertices.size() == shapeMesh.textureCoords.size());

            for (size_t localVertex = 0; localVertex < shapeMesh.vertices.size(); ++localVertex)
            {
                const auto& position { shapeMesh.vertices.at (localVertex) };
                const auto& normal   { shapeMesh.normals.at (localVertex) };
                const auto& uv       { shapeMesh.textureCoords.at (localVertex) };

                outVertices.add (VulkanVertex { { position.x, position.y, position.z },
                                         { normal.x, normal.y, normal.z },
                                         { uv.x, uv.y } });

                accumulateAabb (position);
            }

            appendMaterialRange (shape, vertexBase, outIndices);
            accumulateFeatureEdges (shapeMesh, vertexBase, edgeAccumulator);

            vertexBase += static_cast<uint32_t> (shapeMesh.vertices.size());
        }

        classifyFeatureEdges (edgeAccumulator, outFeatureEdgeIndices);
    }

    /** @brief interleave() step — folds one vertex's own position into this
     *  VulkanMesh's running object-space AABB (aabbMin/aabbMax) — see class doc
     *  comment's "Population" section.
     *  @param position  This vertex's own object-space position.
     */
    void accumulateAabb (const jam::WavefrontObj::Vertex& position) noexcept
    {
        aabbMin[0] = std::min (aabbMin[0], position.x);
        aabbMin[1] = std::min (aabbMin[1], position.y);
        aabbMin[2] = std::min (aabbMin[2], position.z);
        aabbMax[0] = std::max (aabbMax[0], position.x);
        aabbMax[1] = std::max (aabbMax[1], position.y);
        aabbMax[2] = std::max (aabbMax[2], position.z);
    }

    /** @brief interleave() step — re-bases @p shape's own indices by
     *  @p vertexBase (folding this shape into the shared, whole-object index
     *  buffer) into @p outIndices, then appends this shape's own
     *  MaterialRange (firstIndex/numIndices into that now-shared buffer +
     *  @p shape's own Material, copied) to materialRanges — see class doc
     *  comment's "Population" section.
     *  @param shape       This shape's own parsed geometry + material.
     *  @param vertexBase  This shape's own running vertex-count offset into
     *                     the shared vertex buffer (interleave()'s own
     *                     accumulator).
     *  @param outIndices  Shared, whole-object index array — appended to.
     */
    void appendMaterialRange (const jam::WavefrontObj::Shape& shape, uint32_t vertexBase,
                             jam::Array<uint32_t>& outIndices)
    {
        const auto& shapeMesh { shape.mesh };
        const uint32_t firstIndex { static_cast<uint32_t> (outIndices.size()) };

        for (size_t localIndex = 0; localIndex < shapeMesh.indices.size(); ++localIndex)
            outIndices.add (vertexBase + shapeMesh.indices.at (localIndex));

        materialRanges.add ({ firstIndex,
                                   static_cast<uint32_t> (shapeMesh.indices.size()),
                                   shape.material });
    }

    /** @brief interleave() step — walks @p shapeMesh's own indices in
     *  triangle triples (shapeMesh.indices is a flat, per-shape LOCAL-index
     *  triangle list — pre-rebase, mirrors appendMaterialRange()'s own
     *  reading of it), computing each triangle's geometric face normal
     *  (cross product of its own two edge vectors — deliberately the
     *  GEOMETRIC per-triangle normal, not shapeMesh's own possibly-smoothed
     *  per-vertex shading normal: a smooth-shaded crease's shading normals
     *  are blended across the crease and would hide it from the dihedral
     *  test classifyFeatureEdges() runs later) and folding each of its 3
     *  edges into @p edgeAccumulator, keyed by PositionKey (class doc
     *  comment's own "Feature-edge line art" section) so a UV/normal seam
     *  never fractures one geometric edge into two separately-tracked
     *  entries. Global vertex indices (@p vertexBase + the triangle's own
     *  local index) are stored on an edge's FIRST occurrence only — every
     *  later occurrence of that same PositionKey pair shares the identical
     *  object-space position, so any one occurrence's own global index pair
     *  draws the correct line.
     *  @param shapeMesh       This shape's own parsed SoA geometry.
     *  @param vertexBase      This shape's own running vertex-count offset
     *                         (interleave()'s own accumulator).
     *  @param edgeAccumulator Whole-mesh running accumulator — shared across
     *                         every shape's own call, so an edge crossing a
     *                         shape boundary still accumulates both its
     *                         adjacent faces correctly.
     */
    void accumulateFeatureEdges (const jam::WavefrontObj::Mesh& shapeMesh, uint32_t vertexBase,
                                jam::HashMap<EdgeKey, EdgeInfo, EdgeKey::Hash>& edgeAccumulator) const
    {
        const auto accumulateOneEdge = [&edgeAccumulator] (const PositionKey& keyA, const PositionKey& keyB,
                                                            uint32_t globalIndexA, uint32_t globalIndexB,
                                                            const glm::vec3& faceNormal)
        {
            const bool aOrdersFirst { keyA < keyB };
            const EdgeKey edgeKey { aOrdersFirst ? keyA : keyB, aOrdersFirst ? keyB : keyA };

            if (not edgeAccumulator.contains (edgeKey))
            {
                edgeAccumulator.insert ({ edgeKey,
                    EdgeInfo { globalIndexA, globalIndexB, 1, { faceNormal, glm::vec3 { 0.0f } } } });
            }
            else
            {
                auto& existing { edgeAccumulator.at (edgeKey) };

                if (existing.faceCount == 1)
                {
                    existing.faceNormals[1] = faceNormal;
                    existing.faceCount = 2;
                }
                else
                {
                    // Non-manifold (3+ faces touching this edge) — classifyFeatureEdges()
                    // only ever tests faceCount == 1 or == 2, so counting past 2 only
                    // needs to keep this edge OUT of both of those branches.
                    existing.faceCount += 1;
                }
            }
        };

        for (size_t triangleStart = 0; triangleStart + 2 < shapeMesh.indices.size(); triangleStart += 3)
        {
            const uint32_t localIndexA { shapeMesh.indices.at (triangleStart) };
            const uint32_t localIndexB { shapeMesh.indices.at (triangleStart + 1) };
            const uint32_t localIndexC { shapeMesh.indices.at (triangleStart + 2) };

            const auto& positionA { shapeMesh.vertices.at (localIndexA) };
            const auto& positionB { shapeMesh.vertices.at (localIndexB) };
            const auto& positionC { shapeMesh.vertices.at (localIndexC) };

            const glm::vec3 cornerA { positionA.x, positionA.y, positionA.z };
            const glm::vec3 cornerB { positionB.x, positionB.y, positionB.z };
            const glm::vec3 cornerC { positionC.x, positionC.y, positionC.z };
            const glm::vec3 faceNormal { glm::normalize (glm::cross (cornerB - cornerA, cornerC - cornerA)) };

            const PositionKey keyA { positionA.x, positionA.y, positionA.z };
            const PositionKey keyB { positionB.x, positionB.y, positionB.z };
            const PositionKey keyC { positionC.x, positionC.y, positionC.z };

            accumulateOneEdge (keyA, keyB, vertexBase + localIndexA, vertexBase + localIndexB, faceNormal);
            accumulateOneEdge (keyB, keyC, vertexBase + localIndexB, vertexBase + localIndexC, faceNormal);
            accumulateOneEdge (keyC, keyA, vertexBase + localIndexC, vertexBase + localIndexA, faceNormal);
        }
    }

    /** @brief interleave() step — final, whole-mesh pass over
     *  @p edgeAccumulator (run once, after every shape's own triangles have
     *  already accumulated into it): a 1-face edge is a boundary edge, kept
     *  unconditionally; a 2-face edge is kept only when its two faceNormals'
     *  dot product falls below featureEdgeCreaseCosineThreshold (a crease);
     *  a 3+-face (non-manifold) edge is dropped — classifyFeatureEdges()
     *  never attempts a dihedral test against more than 2 face normals. Every
     *  kept edge appends its own globalVertexA/globalVertexB pair to
     *  @p outFeatureEdgeIndices (2 endpoint indices per edge — mesh_edge.vert's
     *  own screen-space quad expansion reads this pair per classified edge,
     *  jam_vulkan/shader/mesh_edge.vert).
     *  @param edgeAccumulator        Whole-mesh accumulator, fully populated
     *                                by every accumulateFeatureEdges() call.
     *  @param outFeatureEdgeIndices  Appended to — build()'s own second
     *                                staged-upload payload.
     */
    void classifyFeatureEdges (const jam::HashMap<EdgeKey, EdgeInfo, EdgeKey::Hash>& edgeAccumulator,
                              jam::Array<uint32_t>& outFeatureEdgeIndices) const
    {
        for (const auto& [edgeKey, info] : edgeAccumulator)
        {
            juce::ignoreUnused (edgeKey);

            const bool isBoundaryEdge { info.faceCount == 1 };
            const bool isCreaseCandidate { info.faceCount == 2 };
            const bool isCreaseEdge { isCreaseCandidate
                and glm::dot (info.faceNormals[0], info.faceNormals[1]) < featureEdgeCreaseCosineThreshold };

            if (isBoundaryEdge or isCreaseEdge)
            {
                outFeatureEdgeIndices.add (info.globalVertexA);
                outFeatureEdgeIndices.add (info.globalVertexB);
            }
        }
    }

    /** @brief build() step — creates vertexBuffer (device-local vertex-
     *  pulling SSBO, eStorageBuffer | eTransferDst — see class doc comment's
     *  "VulkanBuffer usage" section), indexBuffer (device-local,
     *  eIndexBuffer | eTransferDst), both GPU_ONLY, sized from
     *  @p vertexBytes/@p indexBytes, and — only when @p featureEdgeBytes is
     *  non-zero (a mesh may classify zero feature edges, e.g. an
     *  all-smooth-interior closed shape) — featureEdgeIndexBuffer
     *  (device-local, eStorageBuffer | eTransferDst, GPU_ONLY — NOT
     *  eIndexBuffer, since mesh_edge.vert reads it as an SSBO of endpoint
     *  indices, never through vkCmdBindIndexBuffer, see
     *  getFeatureEdgeIndexBuffer()'s own doc comment), sized from
     *  @p featureEdgeBytes.
     *  @param device           Shared Vulkan device — supplies the VMA allocator.
     *  @param vertexBytes      Total interleaved vertex byte count.
     *  @param indexBytes       Total index byte count.
     *  @param featureEdgeBytes Total feature-edge index byte count — may be 0.
     *  @return true if vertexBuffer/indexBuffer (and featureEdgeIndexBuffer,
     *          when @p featureEdgeBytes > 0) were all created successfully.
     */
    bool createGeometryBuffers (VulkanDevice& device, vk::DeviceSize vertexBytes, vk::DeviceSize indexBytes,
                               vk::DeviceSize featureEdgeBytes)
    {
        const vk::BufferCreateInfo vertexBufferInfo { {}, vertexBytes,
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst };
        VmaAllocationCreateInfo vertexAllocInfo {};
        vertexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vertexBuffer = VulkanBuffer (device.getAllocator(), vertexBufferInfo, vertexAllocInfo);

        const vk::BufferCreateInfo indexBufferInfo { {}, indexBytes,
            vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst };
        VmaAllocationCreateInfo indexAllocInfo {};
        indexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        indexBuffer = VulkanBuffer (device.getAllocator(), indexBufferInfo, indexAllocInfo);

        bool geometryBuffersValid { vertexBuffer.isValid() and indexBuffer.isValid() };

        if (geometryBuffersValid and featureEdgeBytes > 0)
        {
            const vk::BufferCreateInfo featureEdgeBufferInfo { {}, featureEdgeBytes,
                vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst };
            VmaAllocationCreateInfo featureEdgeAllocInfo {};
            featureEdgeAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            featureEdgeIndexBuffer = VulkanBuffer (device.getAllocator(), featureEdgeBufferInfo, featureEdgeAllocInfo);

            geometryBuffersValid = featureEdgeIndexBuffer.isValid();
        }

        return geometryBuffersValid;
    }

    /** @brief build() step — allocates stagingBuffer (CPU-mapped, sized
     *  @p vertexBytes + @p indexBytes + @p featureEdgeBytes), memcpys
     *  @p vertices then @p indices then @p featureEdgeIndices into it back-
     *  to-back, records one vk::CommandBuffer::copyBuffer() per destination
     *  (vertexBuffer/indexBuffer always, featureEdgeIndexBuffer only when
     *  @p featureEdgeBytes > 0 — already created by createGeometryBuffers()),
     *  then one vk::MemoryBarrier (TRANSFER_WRITE -> SHADER_READ | INDEX_READ)
     *  covering every destination — the buffer-copy analog of
     *  resource/jam_VulkanUploadHelpers.h's recordUploadBarrier() image-layout
     *  transitions, since a plain buffer copy carries no layout to piggy-
     *  back synchronisation on. See class doc comment's "Staged upload"
     *  section. Only called when createGeometryBuffers() already returned
     *  true (build()'s own gate) — never re-checks vertexBuffer/indexBuffer/
     *  featureEdgeIndexBuffer validity itself.
     *  @param device            Shared Vulkan device — supplies the VMA allocator.
     *  @param commandBuffer     Active command buffer to record the copy +
     *                           barrier into.
     *  @param vertices          Interleaved vertex data — memcpy source.
     *  @param indices           Re-based index data — memcpy source.
     *  @param featureEdgeIndices Classified feature-edge index data — memcpy
     *                           source; may be empty (@p featureEdgeBytes 0).
     *  @param vertexBytes       Total interleaved vertex byte count.
     *  @param indexBytes        Total index byte count.
     *  @param featureEdgeBytes  Total feature-edge index byte count — may be 0.
     *  @return true if stagingBuffer was created successfully.
     */
    bool recordStagedGeometryUpload (VulkanDevice& device, vk::CommandBuffer commandBuffer,
                                     const jam::Array<VulkanVertex>& vertices, const jam::Array<uint32_t>& indices,
                                     const jam::Array<uint32_t>& featureEdgeIndices,
                                     vk::DeviceSize vertexBytes, vk::DeviceSize indexBytes,
                                     vk::DeviceSize featureEdgeBytes)
    {
        stagingBuffer = createStagingBuffer (device, vertexBytes + indexBytes + featureEdgeBytes);

        bool recorded { stagingBuffer.isValid() };

        if (recorded)
        {
            auto* const mappedBytes { static_cast<uint8_t*> (stagingBuffer.getMapped()) };

            std::memcpy (mappedBytes, vertices.data(), static_cast<size_t> (vertexBytes));
            std::memcpy (mappedBytes + vertexBytes, indices.data(), static_cast<size_t> (indexBytes));

            const vk::BufferCopy vertexCopyRegion { 0, 0, vertexBytes };
            commandBuffer.copyBuffer (stagingBuffer.getBuffer(), vertexBuffer.getBuffer(), vertexCopyRegion);

            const vk::BufferCopy indexCopyRegion { vertexBytes, 0, indexBytes };
            commandBuffer.copyBuffer (stagingBuffer.getBuffer(), indexBuffer.getBuffer(), indexCopyRegion);

            if (featureEdgeBytes > 0)
            {
                std::memcpy (mappedBytes + vertexBytes + indexBytes, featureEdgeIndices.data(),
                            static_cast<size_t> (featureEdgeBytes));

                const vk::BufferCopy featureEdgeCopyRegion { vertexBytes + indexBytes, 0, featureEdgeBytes };
                commandBuffer.copyBuffer (stagingBuffer.getBuffer(), featureEdgeIndexBuffer.getBuffer(),
                                          featureEdgeCopyRegion);
            }

            const vk::MemoryBarrier uploadBarrier { vk::AccessFlagBits::eTransferWrite,
                vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eIndexRead };

            commandBuffer.pipelineBarrier (vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eVertexInput,
                {}, uploadBarrier, {}, {});
        }

        return recorded;
    }

    /** @brief VulkanDevice-local vertex-pulling SSBO — see class doc comment's
     *  "VulkanBuffer usage" section for why this is eStorageBuffer, not
     *  eVertexBuffer. */
    VulkanBuffer vertexBuffer;

    /** @brief VulkanDevice-local index buffer feeding the fixed-function index
     *  fetch that drives gl_VertexIndex. */
    VulkanBuffer indexBuffer;

    /** @brief VulkanDevice-local feature-edge endpoint-index SSBO — see class doc
     *  comment's "Feature-edge line art" section. Empty/invalid when
     *  classifyFeatureEdges() classified zero edges (numFeatureEdgeIndices
     *  stays 0) — createGeometryBuffers() never allocates it in that case. */
    VulkanBuffer featureEdgeIndexBuffer;

    /** @brief Temporary CPU_ONLY staging VulkanBuffer created by build() — kept
     *  alive until releaseStaging(), see that method's doc comment. */
    VulkanBuffer stagingBuffer;

    /** @brief Total interleaved vertex count — see getNumVertices(). */
    uint32_t numVertices { 0 };

    /** @brief Total index count — see getNumIndices(). */
    uint32_t numIndices { 0 };

    /** @brief Total feature-edge index count — see getNumFeatureEdgeIndices(). */
    uint32_t numFeatureEdgeIndices { 0 };

    /** @brief Object-space AABB minimum corner — see getAabbMin(). */
    float aabbMin[3] { 0.0f, 0.0f, 0.0f };

    /** @brief Object-space AABB maximum corner — see getAabbMax(). */
    float aabbMax[3] { 0.0f, 0.0f, 0.0f };

    /** @brief Per-shape material sub-ranges — see getMaterialRanges(). */
    jam::Array<MaterialRange> materialRanges;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanMesh)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam