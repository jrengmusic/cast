/**
 * @file jam_WavefrontObj.h
 * @brief Wavefront OBJ/MTL mesh loader — plain-float SoA output, full-standard
 *        MTL (Phong + PBR) parsing, n-gon triangulation via jam::Earcut.
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief Wavefront OBJ/MTL loader (clean-room; JUCE-precedent PODs, CODING.md idiom).
 *
 *  Format-agnostic mesh output (plain-float SoA) consumable by the native
 *  Vulkan renderer and a future runtime-JS asset seam alike — deps stay
 *  juce_core-family only (no vk::, no 3D-math), matching the
 *  enforced jam_vulkan -> jam_graphics module direction (jam_vulkan already
 *  depends on jam_graphics; the reverse dependency is forbidden — see
 *  jam_graphics.h's Fonts section for the identical GlyphAtlas-relocation
 *  precedent).
 *
 *  @par Robustness beyond the JUCE example (examples/Assets/WavefrontObjParser.h)
 *  - Relative/negative face-vertex indices resolved against the current pool
 *    size (tinyobj fixIndex behavior), 1-based positive indices resolved to
 *    0-based.
 *  - Out-of-bounds references and degenerate faces (fewer than 3 distinct
 *    vertex references after within-face dedup) are warned and the offending
 *    face is skipped — never fatal.
 *  - Faces of 4+ vertices are triangulated by projecting onto the face's
 *    dominant plane (Newell's method for the face normal, dropping the
 *    dominant axis) and running jam::Earcut::triangulate() on the resulting
 *    2D ring; triangles pass through untouched. One triangulator, no
 *    quad-specific shortest-diagonal special case.
 *  - Per-shape (v, vt, vn) triple dedup via jam::HashMap<TripleIndex, Index,
 *    TripleIndex::Hash> — a triple already emitted reuses its output index;
 *    a missing vt/vn in a triple default-fills (0,0)/(0,0,0) so the SoA
 *    arrays stay index-aligned (JUCE demo behavior).
 *  - Smoothing-group-aware normal generation (generateNormals(), see its own
 *    doc) when a shape's faces carried no vn data at all.
 *  - Full-standard MTL parse (Phong + PBR fields), multiple mtllib files,
 *    material redefinition warned (later definition wins), unknown MTL keys
 *    silently skipped (one vocabulary, no per-unknown-key noise).
 *
 *  Diagnostics are accumulated (warn-and-continue) into the caller's
 *  juce::StringArray; the only fatal condition is an unreadable OBJ file
 *  itself (load(File)'s own juce::Result failure). A missing/unreadable
 *  mtllib is a warning — real-world OBJs shipped without their sibling .mtl
 *  are commonplace; affected shapes render with the default Material.
 */
class WavefrontObj
{
public:
    /** @brief Index type into a Mesh's attribute arrays. */
    using Index = juce::uint32;

    /** @brief A 3-component float vector — position or normal. */
    struct Vertex        { float x, y, z; };

    /** @brief A 2-component float UV coordinate. */
    struct TextureCoord  { float x, y;    };

    /** @brief One shape's geometry. SoA: contiguous, memcpy-safe (std::vector —
     *  the same contiguity/memcpy guarantee juce::Array gave this SoA layout
     *  holds identically for std::vector; .at() additionally gains fail-fast
     *  bounds checking that juce::Array's getReference()/getUnchecked() never
     *  had). */
    struct Mesh
    {
        /** @brief Vertex positions. */
        std::vector<Vertex>       vertices;

        /** @brief Vertex normals, parallel to vertices. */
        std::vector<Vertex>       normals;

        /** @brief Vertex UV coordinates, parallel to vertices. */
        std::vector<TextureCoord> textureCoords;

        /** @brief Triangle indices into the above arrays, in groups of 3. */
        std::vector<Index>        indices;
    };

    /** @brief Full standard MTL (Phong + exocortex PBR). Renderer consumes a subset.
     *  @par name — the default-material signal
     *  name is only ever assigned by startMaterial() (newmtl), always to a
     *  non-empty token; every default-constructed Material (no mtllib loaded,
     *  or an unknown usemtl name — see useMaterial()'s own fallback) keeps
     *  name empty. name.isEmpty() is therefore the SSOT signal a shape has no
     *  real MTL material — read by the renderer's own wireframe/transparent-
     *  fill path (jam::VulkanGraphics::recordMeshMaterialRangeDraws()). */
    struct Material
    {
        /** @brief MTL newmtl name; empty signals the default (no-material) state — see class doc. */
        juce::String name;

        /** @brief Ambient colour (Ka). */
        Vertex ambient { 0.0f, 0.0f, 0.0f };

        /** @brief Diffuse colour (Kd).
         *
         *  OBJ-spec-less default: real-world OBJs without Kd (or without any
         *  mtllib at all) rendered fully black under the old {0,0,0} default —
         *  tinyobj-parity mid-gray instead (tinyobj_loader's own default_diffuse).
         *  Superseded entirely for a genuinely default (unnamed) Material by the
         *  wireframe/transparent-fill path above; this default only matters for a
         *  partial MTL that defines newmtl/name but never sets Kd.
         */
        Vertex diffuse { 0.8f, 0.8f, 0.8f };

        /** @brief Specular colour (Ks). */
        Vertex specular { 0.0f, 0.0f, 0.0f };

        /** @brief Emissive colour (Ke). */
        Vertex emission { 0.0f, 0.0f, 0.0f };

        /** @brief Specular exponent (Ns). */
        float shininess { 1.0f };

        /** @brief Index of refraction (Ni). */
        float refractiveIndex { 0.0f };

        /** @brief PBR roughness (Pr). */
        float roughness { 0.0f };

        /** @brief PBR metallic (Pm). */
        float metallic { 0.0f };

        /** @brief PBR sheen (Ps). */
        float sheen { 0.0f };

        /** @brief Diffuse texture map filename (map_Kd). */
        juce::String diffuseTextureName;

        /** @brief Normal/bump map filename (norm / map_bump; norm wins if both present). */
        juce::String normalTextureName;

        /** @brief Roughness texture map filename (map_Pr). */
        juce::String roughnessTextureName;

        /** @brief Metallic texture map filename (map_Pm). */
        juce::String metallicTextureName;

        /** @brief Emissive texture map filename (map_Ke). */
        juce::String emissiveTextureName;
    };

    struct Shape
    {
        /** @brief Shape name from an OBJ `o`/`g` line. */
        juce::String name;

        /** @brief The shape's parsed geometry. */
        Mesh mesh;

        /** @brief The shape's resolved material (default-constructed when no usemtl matched). */
        Material material;
    };

    WavefrontObj() = default;

    /** @brief Reads and parses @p objFile. Every in-parse diagnostic is
     *  warn-and-continue (missing mtllib included; see class doc) — the ONE
     *  hard failure is an unreadable @p objFile itself. mtllib's relative
     *  paths resolve against @p objFile's parent directory.
     *  @param objFile           The .obj file to read and parse.
     *  @param optionalWarnings  Non-fatal diagnostics appended here — pass
     *                           nullptr (the default) to discard them; the
     *                           hard-failure message always rides the
     *                           returned juce::Result regardless.
     *  @return ok, or fail ("Could not open ...") — juce::Result carries
     *          both the boolean (its own operator bool) and the message,
     *          so a caller may simply branch on it. */
    juce::Result load (const juce::File& objFile, juce::StringArray* optionalWarnings = nullptr);

    /** @brief Moves this WavefrontObj's own parsed shape list out — the
     *  caller becomes the sole owner, this WavefrontObj is left with an
     *  empty shapes (jam::Owner's own moved-from state). Ref-qualified to
     *  an rvalue (mirrors jam::HashMap::extract()'s identical move-out
     *  idiom, jam_lexicon/utils/jam_HashMap.h) so a caller must say std::move()
     *  explicitly at the call site — never an accidental read of a
     *  WavefrontObj a caller still believes is intact. The one caller
     *  today (jam::VulkanShaderCompiler::compile()) parses once at
     *  compile time then extracts immediately; the local WavefrontObj goes
     *  out of scope right after, so no further read of shapes through this
     *  instance is ever attempted.
     *  @return This WavefrontObj's own parsed shapes, moved. */
    jam::Owner<Shape> extractShapes() && noexcept { return std::move (shapes); }

private:
    /** @brief Dedup key into the source OBJ attribute pools. Also used as the
     *  resolved (relative/negative-adjusted, 0-based) representation of one
     *  face-vertex reference before its dedup lookup. */
    struct TripleIndex
    {
        int vertexIndex  { -1 };
        int textureIndex { -1 };
        int normalIndex  { -1 };
        bool operator== (const TripleIndex& other) const noexcept;

        /** @brief Hash functor for TripleIndex — used as jam::HashMap's explicit
         *  HashFn (the HashMap<TripleIndex, Index, TripleIndex::Hash> dedup
         *  table), same nested-functor shape as jam::GlyphAtlas::Key::Hash
         *  (jam_GlyphAtlas.h:110), consumed identically to GlyphAtlas's own
         *  jam::HashMap<Key, CacheEntry, Key::Hash> (jam_GlyphAtlas.h:917). */
        struct Hash
        {
            size_t operator() (const TripleIndex& key) const noexcept;
        };
    };

    jam::Owner<Shape> shapes;

    // Keyword dispatch — v/vn/vt/f/g/o/s/usemtl/mtllib (OBJ lines) and
    // newmtl/Ka/Kd/Ks/Ke/Ns/Ni/Pr/Pm/Ps/map_Kd/norm/map_bump/map_Pr/map_Pm/
    // map_Ke (MTL lines). Each keyword's registered lambda is a one-line
    // dispatch to its own member function below — the table IS the per-
    // keyword function-length decomposition (MANIFESTO Lean 30-line rule),
    // replacing the JUCE 8-branch if/continue chain. Built once, keyed by
    // first token.
    jam::Function::Map<juce::String, void> parser;
    jam::Function::Map<juce::String, void> materialParser;

    // Parse-transient source attribute pools (file-scoped v/vn/vt pools that
    // TripleIndex indexes into) plus the shape currently being built and its
    // per-shape dedup/smoothing bookkeeping. Reset at the top of every load()
    // call — calculation buffers for one parse operation (BLESSED Stateless),
    // never read once that operation's shapes have been produced.
    std::vector<Vertex>       positions;
    std::vector<Vertex>       normals;
    std::vector<TextureCoord> texCoords;

    std::unique_ptr<Shape> currentShape;
    jam::HashMap<TripleIndex, Index, TripleIndex::Hash> tripleIndices;
    std::vector<int> vertexPositionIndices;    // original position-pool index per emitted mesh vertex — generateNormals()'s per-position accumulation key.
    std::vector<int> triangleSmoothingGroups;  // smoothing-group id per emitted triangle (parallel to mesh.indices in groups of 3) — generateNormals()'s per-triangle group key.
    bool shapeHasExplicitNormals { false };
    int  currentSmoothingGroup { 0 };
    bool warnedExtraVertexFields { false };

    // MTL material registry (built by mtllib, looked up by usemtl) and the
    // name of the newmtl block currently being parsed.
    jam::HashMap<juce::String, Material> materials;
    juce::String currentMaterialName;

    juce::File sourceFile;

    void registerParser();
    void registerMaterialParser();
    void registerColourMaterialParser();
    void registerScalarMaterialParser();
    void registerSimpleTextureMaterialParser();
    void registerNormalTextureMaterialParser();

    void dispatchLines (const juce::String& text, const jam::Function::Map<juce::String, void>& table,
                        juce::StringArray& warnings);
    void resetParseState (const juce::File& objSourceFile);
    void finalizeCurrentShape();

    void parseVertexPosition (const juce::StringArray& tokens, juce::StringArray& warnings);
    void parseVertexNormal   (const juce::StringArray& tokens, juce::StringArray& warnings);
    void parseTextureCoord   (const juce::StringArray& tokens, juce::StringArray& warnings);
    void parseFace           (const juce::StringArray& tokens, juce::StringArray& warnings);
    void startShape          (const juce::StringArray& tokens, juce::StringArray& warnings);
    void setSmoothingGroup   (const juce::StringArray& tokens, juce::StringArray& warnings);
    void useMaterial         (const juce::StringArray& tokens, juce::StringArray& warnings);
    void loadMaterialLibrary (const juce::StringArray& tokens, juce::StringArray& warnings);
    void startMaterial       (const juce::StringArray& tokens, juce::StringArray& warnings);

    Material* getCurrentMaterial (juce::StringArray& warnings);

    void emitFace (const std::vector<TripleIndex>& faceRefs);
    void emitTriangle (const TripleIndex& a, const TripleIndex& b, const TripleIndex& c);
    Index getOrAppendVertex (const TripleIndex& ref);

    static bool parseFaceVertexRef (const juce::String& ref, int positionCount, int textureCount,
                                      int normalCount, TripleIndex& outRef) noexcept;

    /** @brief Smoothing-group-aware normal generation, called once per shape
     *  after parse ONLY when that shape's faces carried no vn data at all
     *  (WavefrontObj::shapeHasExplicitNormals stayed false through the whole
     *  shape).
     *
     *  @par Algorithm
     *  Per triangle (three consecutive mesh.indices), the geometric face
     *  normal is the cross product of its two edge vectors. Group 0 ("off")
     *  is flat shading: each triangle's own face normal is written directly
     *  to its three corners, no sharing with any other triangle. Group N>0
     *  is smooth shading: face normals are summed per (original position-pool
     *  index, group) key across every triangle in that group touching that
     *  position — keying on the ORIGINAL position index (not the per-shape
     *  mesh vertex slot) so a hard UV seam, which duplicates mesh vertices,
     *  still shares one smooth normal across the seam. Every mesh vertex is
     *  then normalized once at the end (idempotent for the already-normalized
     *  group-0 case). A mesh vertex slot touched by both a group-0 and a
     *  group-N triangle resolves to the group-N (smoothed) value, since the
     *  smoothed pass runs after the flat pass, in triangle-emission order —
     *  deterministic, not a case real OBJ content is expected to produce.
     */
    void generateNormals (Mesh&) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavefrontObj)
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
