//
// OBJ-geometry file: parse-state lifecycle (resetParseState/
// finalizeCurrentShape), v/vn/vt/f/g/o/s/usemtl keyword parsing, face
// triangulation/dedup, and generateNormals() — everything read/written by
// dispatchLines()'s OBJ-side parser table. The MTL-material concern
// (materialParser registration, mtllib loading, newmtl/Ka/Kd/... property
// parsing) lives in jam_WavefrontObjMaterial.cpp; dispatchLines() itself
// stays here since both tables share its one dispatch loop.

namespace jam
{
/*____________________________________________________________________________*/
bool WavefrontObj::TripleIndex::operator== (const TripleIndex& other) const noexcept
{
    return vertexIndex == other.vertexIndex
       and textureIndex == other.textureIndex
       and normalIndex == other.normalIndex;
}

/*____________________________________________________________________________*/
// TripleIndex::Hash — wyhash of the 3 signed indices via jam_core's
// Wyhash::hashInt/mix primitives (jam_lexicon/utils/jam_HashMap.h:73-263), folded
// left-to-right the same way TupleHashHelper::mix64 folds tuple elements
// (jam_HashMap.h:513-517) — TripleIndex is 3 plain ints rather than a
// std::tuple, so the fold is reproduced directly against this type instead of
// routing through Hash<std::tuple<int,int,int>>.
size_t WavefrontObj::TripleIndex::Hash::operator() (const TripleIndex& key) const noexcept
{
    const auto state { jam::Wyhash::mix (jam::Wyhash::hashInt (static_cast<uint64_t> (key.vertexIndex)),
                                         jam::Wyhash::hashInt (static_cast<uint64_t> (key.textureIndex))) };

    return jam::Wyhash::mix (state, jam::Wyhash::hashInt (static_cast<uint64_t> (key.normalIndex)));
}

/*____________________________________________________________________________*/
// toZeroBasedIndex — OBJ face-vertex indices are 1-based; a NEGATIVE index is
// relative to the END of the current pool (tinyobj fixIndex behavior: -1 is
// "the last element added so far"). Returns a 0-based index; callers bounds-
// check the result (this function does not — 0 and out-of-range positive or
// negative inputs both fall outside the valid 0 to poolCount-1 span and are
// caught there).
static int toZeroBasedIndex (int rawIndex, int poolCount) noexcept
{
    return rawIndex < 0 ? poolCount + rawIndex : rawIndex - 1;
}

/*____________________________________________________________________________*/
// computeFaceNormal — unnormalized cross product of a triangle's two edge
// vectors. Magnitude is proportional to triangle area, giving natural area-
// weighting when several face normals are summed by accumulateFaceNormals().
static WavefrontObj::Vertex computeFaceNormal (const WavefrontObj::Vertex& a, const WavefrontObj::Vertex& b,
                                               const WavefrontObj::Vertex& c) noexcept
{
    const WavefrontObj::Vertex edge1 { b.x - a.x, b.y - a.y, b.z - a.z };
    const WavefrontObj::Vertex edge2 { c.x - a.x, c.y - a.y, c.z - a.z };

    return { edge1.y * edge2.z - edge1.z * edge2.y,
             edge1.z * edge2.x - edge1.x * edge2.z,
             edge1.x * edge2.y - edge1.y * edge2.x };
}

/*____________________________________________________________________________*/
// computeNewellNormal — Newell's method: a polygon normal that stays well-
// defined even for a near-planar, non-convex n-gon where a single 3-point
// cross product would be numerically unreliable. Used only to pick the
// n-gon's dominant projection axis ahead of jam::Earcut::triangulate().
static WavefrontObj::Vertex computeNewellNormal (const std::vector<WavefrontObj::Vertex>& polygon) noexcept
{
    WavefrontObj::Vertex normal { 0.0f, 0.0f, 0.0f };
    const auto count { polygon.size() };

    for (size_t i = 0; i < count; ++i)
    {
        const auto& current { polygon[i] };
        const auto& next { polygon[(i + 1) % count] };

        normal.x += (current.y - next.y) * (current.z + next.z);
        normal.y += (current.z - next.z) * (current.x + next.x);
        normal.z += (current.x - next.x) * (current.y + next.y);
    }

    return normal;
}

/*____________________________________________________________________________*/
// projectToDominantPlane — drops the coordinate axis the face normal points
// most strongly along, projecting the 3D polygon into the remaining 2D plane
// for jam::Earcut::triangulate(). Winding is irrelevant here — Earcut detects
// orientation itself via its own shoelace sum (jam_Earcut.h's class doc).
static std::vector<juce::Point<float>> projectToDominantPlane (const std::vector<WavefrontObj::Vertex>& polygon,
                                                                const WavefrontObj::Vertex& normal) noexcept
{
    const float absX { std::abs (normal.x) };
    const float absY { std::abs (normal.y) };
    const float absZ { std::abs (normal.z) };

    std::vector<juce::Point<float>> ring;
    ring.reserve (polygon.size());

    if (absX >= absY and absX >= absZ)
        for (const auto& vertex : polygon) ring.push_back ({ vertex.y, vertex.z });
    else if (absY >= absZ)
        for (const auto& vertex : polygon) ring.push_back ({ vertex.x, vertex.z });
    else
        for (const auto& vertex : polygon) ring.push_back ({ vertex.x, vertex.y });

    return ring;
}

/*____________________________________________________________________________*/
// triangulatePolygon — n-gon (4+ vertex) triangulation: dominant-plane
// projection followed by jam::Earcut::triangulate(). One triangulator for
// every n-gon size, including quads — no shortest-diagonal special case.
static std::vector<uint32_t> triangulatePolygon (const std::vector<WavefrontObj::Vertex>& polygon)
{
    const auto faceNormal { computeNewellNormal (polygon) };
    const auto ring { projectToDominantPlane (polygon, faceNormal) };

    // jam::Earcut::triangulate() takes jam::Array — this polygon's own SoA
    // vectors stay std::vector (consumption-only seam), so the ring is copied
    // across the boundary here rather than upstream.
    jam::Array<juce::Point<float>> earcutRing (static_cast<int> (ring.size()));

    for (const auto& point : ring)
        earcutRing.add (point);

    const auto triangleIndices { jam::Earcut::triangulate (earcutRing) };

    return std::vector<uint32_t> (triangleIndices.begin(), triangleIndices.end());
}

/*____________________________________________________________________________*/
static WavefrontObj::Vertex normaliseVertex (const WavefrontObj::Vertex& v) noexcept
{
    const float lengthSquared { v.x * v.x + v.y * v.y + v.z * v.z };
    WavefrontObj::Vertex result { v };

    if (lengthSquared > 0.0f)
    {
        const float inverseLength { 1.0f / std::sqrt (lengthSquared) };

        result.x *= inverseLength;
        result.y *= inverseLength;
        result.z *= inverseLength;
    }

    return result;
}

/*____________________________________________________________________________*/
// addToSmoothedNormal — accumulates a face normal into the running sum for
// one (original position-pool index, smoothing-group) key. See
// generateNormals()'s doc (jam_WavefrontObj.h) for why position (not mesh
// vertex slot) is the key.
static void addToSmoothedNormal (jam::HashMap<std::pair<int, int>, WavefrontObj::Vertex>& smoothedNormals,
                                 std::pair<int, int> key, const WavefrontObj::Vertex& faceNormal)
{
    auto& sum { smoothedNormals[key] };

    sum.x += faceNormal.x;
    sum.y += faceNormal.y;
    sum.z += faceNormal.z;
}

/*____________________________________________________________________________*/
// accumulateFaceNormals — pass 1 of generateNormals(): writes flat (group 0)
// face normals directly; sums smooth (group N>0) face normals per (position,
// group) key for applySmoothedNormals() to read back once every triangle
// has contributed.
static void accumulateFaceNormals (WavefrontObj::Mesh& mesh, const std::vector<int>& triangleSmoothingGroups,
                                   const std::vector<int>& vertexPositionIndices,
                                   jam::HashMap<std::pair<int, int>, WavefrontObj::Vertex>& smoothedNormals)
{
    for (size_t triangle = 0; triangle < triangleSmoothingGroups.size(); ++triangle)
    {
        const auto i0 { mesh.indices.at (triangle * 3 + 0) };
        const auto i1 { mesh.indices.at (triangle * 3 + 1) };
        const auto i2 { mesh.indices.at (triangle * 3 + 2) };

        const auto faceNormal { computeFaceNormal (mesh.vertices.at (i0), mesh.vertices.at (i1),
                                                    mesh.vertices.at (i2)) };
        const auto group { triangleSmoothingGroups.at (triangle) };

        if (group == 0)
        {
            mesh.normals.at (i0) = faceNormal;
            mesh.normals.at (i1) = faceNormal;
            mesh.normals.at (i2) = faceNormal;
        }
        else
        {
            addToSmoothedNormal (smoothedNormals, { vertexPositionIndices.at (i0), group }, faceNormal);
            addToSmoothedNormal (smoothedNormals, { vertexPositionIndices.at (i1), group }, faceNormal);
            addToSmoothedNormal (smoothedNormals, { vertexPositionIndices.at (i2), group }, faceNormal);
        }
    }
}

/*____________________________________________________________________________*/
// applySmoothedNormals — pass 2 of generateNormals(): for every group N>0
// triangle, overwrites its three corners with the fully-summed (position,
// group) normal (final normalize happens once, back in generateNormals()).
static void applySmoothedNormals (WavefrontObj::Mesh& mesh, const std::vector<int>& triangleSmoothingGroups,
                                    const std::vector<int>& vertexPositionIndices,
                                    const jam::HashMap<std::pair<int, int>, WavefrontObj::Vertex>& smoothedNormals)
{
    for (size_t triangle = 0; triangle < triangleSmoothingGroups.size(); ++triangle)
    {
        const auto group { triangleSmoothingGroups.at (triangle) };

        if (group != 0)
            for (size_t corner = 0; corner < 3; ++corner)
            {
                const auto vertex { mesh.indices.at (triangle * 3 + corner) };
                const auto key { std::pair<int, int> { vertexPositionIndices.at (vertex), group } };

                mesh.normals.at (vertex) = smoothedNormals.at (key);
            }
    }
}

/*____________________________________________________________________________*/
// parseFaceVertexRef — custom '/'-splitting per face-vertex reference: v,
// v/vt, v//vn, or v/vt/vn. Empty vt/vn fields resolve to -1 (absent). Returns
// false if the vertex index or any PRESENT vt/vn index falls outside its
// pool after relative/negative resolution — caller warns and skips the whole
// face (never a partial/garbage face).
bool WavefrontObj::parseFaceVertexRef (const juce::String& ref, int positionCount, int textureCount,
                                         int normalCount, TripleIndex& outRef) noexcept
{
    const auto firstSlash { ref.indexOfChar ('/') };
    const auto secondSlash { firstSlash < 0 ? -1 : ref.indexOfChar (firstSlash + 1, '/') };

    const auto vToken { firstSlash < 0 ? ref : ref.substring (0, firstSlash) };
    const auto vtToken { firstSlash < 0 ? juce::String()
                        : (secondSlash < 0 ? ref.substring (firstSlash + 1) : ref.substring (firstSlash + 1, secondSlash)) };
    const auto vnToken { secondSlash < 0 ? juce::String() : ref.substring (secondSlash + 1) };

    outRef.vertexIndex  = toZeroBasedIndex (vToken.getIntValue(), positionCount);
    outRef.textureIndex = vtToken.isEmpty() ? -1 : toZeroBasedIndex (vtToken.getIntValue(), textureCount);
    outRef.normalIndex  = vnToken.isEmpty() ? -1 : toZeroBasedIndex (vnToken.getIntValue(), normalCount);

    return outRef.vertexIndex >= 0 and outRef.vertexIndex < positionCount
       and (vtToken.isEmpty() or (outRef.textureIndex >= 0 and outRef.textureIndex < textureCount))
       and (vnToken.isEmpty() or (outRef.normalIndex >= 0 and outRef.normalIndex < normalCount));
}

/*____________________________________________________________________________*/
// registerParser — OBJ line-keyword dispatch. Each entry is a one-line
// forward to its own member function — this table IS the per-keyword
// function-length decomposition.
void WavefrontObj::registerParser()
{
    parser.add<const juce::StringArray&, juce::StringArray&> ("v", [this] (const juce::StringArray& t, juce::StringArray& w) { parseVertexPosition (t, w); });
    parser.add<const juce::StringArray&, juce::StringArray&> ("vn", [this] (const juce::StringArray& t, juce::StringArray& w) { parseVertexNormal (t, w); });
    parser.add<const juce::StringArray&, juce::StringArray&> ("vt", [this] (const juce::StringArray& t, juce::StringArray& w) { parseTextureCoord (t, w); });
    parser.add<const juce::StringArray&, juce::StringArray&> ("f", [this] (const juce::StringArray& t, juce::StringArray& w) { parseFace (t, w); });
    parser.add<const juce::StringArray&, juce::StringArray&> ("g", [this] (const juce::StringArray& t, juce::StringArray& w) { startShape (t, w); });
    parser.add<const juce::StringArray&, juce::StringArray&> ("o", [this] (const juce::StringArray& t, juce::StringArray& w) { startShape (t, w); });
    parser.add<const juce::StringArray&, juce::StringArray&> ("s", [this] (const juce::StringArray& t, juce::StringArray& w) { setSmoothingGroup (t, w); });
    parser.add<const juce::StringArray&, juce::StringArray&> ("usemtl", [this] (const juce::StringArray& t, juce::StringArray& w) { useMaterial (t, w); });
    parser.add<const juce::StringArray&, juce::StringArray&> ("mtllib", [this] (const juce::StringArray& t, juce::StringArray& w) { loadMaterialLibrary (t, w); });
}

/*____________________________________________________________________________*/
// dispatchLines — shared line-splitting + keyword-dispatch loop, reused for
// both the top-level OBJ text (parser) and each mtllib file's text
// (materialParser — see jam_WavefrontObjMaterial.cpp). Comments/blank lines
// carry no keyword and are simply never found in table (positive check, no
// bail-out guard). Every diagnostic is warn-and-continue — nothing inside a
// parse ever aborts the remaining lines.
void WavefrontObj::dispatchLines (const juce::String& text, const jam::Function::Map<juce::String, void>& table,
                                  juce::StringArray& warnings)
{
    const auto lines { juce::StringArray::fromLines (text) };

    for (const auto& rawLine : lines)
    {
        const auto line { rawLine.trim() };

        if (line.isNotEmpty() and not line.startsWith ("#"))
        {
            const auto tokens { juce::StringArray::fromTokens (line, true) };
            const auto keyword { tokens[0] };

            if (table.contains (keyword))
                table.get (keyword, tokens, warnings);
        }
    }
}

/*____________________________________________________________________________*/
// resetParseState — clears every parse-transient buffer and starts the
// unnamed default shape (content before any g/o belongs to it). Called at
// the top of every load() so a WavefrontObj instance is safely reusable
// across repeated load() calls (BLESSED Deterministic: same input always
// produces the same output, never a residue of a prior parse).
void WavefrontObj::resetParseState (const juce::File& objSourceFile)
{
    shapes.clear();
    positions.clear();
    normals.clear();
    texCoords.clear();
    materials.clear();

    currentMaterialName.clear();
    currentSmoothingGroup = 0;
    warnedExtraVertexFields = false;
    sourceFile = objSourceFile;

    currentShape = std::make_unique<Shape>();
    tripleIndices.clear();
    vertexPositionIndices.clear();
    triangleSmoothingGroups.clear();
    shapeHasExplicitNormals = false;
}

/*____________________________________________________________________________*/
// finalizeCurrentShape — called when a g/o line starts a new shape and once
// more at end-of-parse. A shape with no faces is not emitted (silently
// discarded, per spec — not a warning case). generateNormals() runs only
// when this shape's faces never referenced vn data at all.
void WavefrontObj::finalizeCurrentShape()
{
    if (currentShape->mesh.indices.size() > 0)
    {
        if (not shapeHasExplicitNormals)
            generateNormals (currentShape->mesh);

        shapes.add (std::move (currentShape));
    }

    tripleIndices.clear();
    vertexPositionIndices.clear();
    triangleSmoothingGroups.clear();
    shapeHasExplicitNormals = false;
}

/*____________________________________________________________________________*/
// parseVertexPosition — "v x y z [w] [r g b]". Optional w/colour fields are
// ignored; their presence is warned once per file, not once per line.
void WavefrontObj::parseVertexPosition (const juce::StringArray& tokens, juce::StringArray& warnings)
{
    positions.push_back ({ tokens[1].getFloatValue(), tokens[2].getFloatValue(), tokens[3].getFloatValue() });

    if (tokens.size() > 4 and not warnedExtraVertexFields)
    {
        warnings.add ("v: optional w/colour fields present, ignored");
        warnedExtraVertexFields = true;
    }
}

/*____________________________________________________________________________*/
void WavefrontObj::parseVertexNormal (const juce::StringArray& tokens, juce::StringArray& warnings)
{
    juce::ignoreUnused (warnings);
    normals.push_back ({ tokens[1].getFloatValue(), tokens[2].getFloatValue(), tokens[3].getFloatValue() });
}

/*____________________________________________________________________________*/
// parseTextureCoord — "vt u v [w]"; the optional w is silently ignored (no
// warn-once — unlike v's w/colour, a vt w is common and not a spec anomaly).
void WavefrontObj::parseTextureCoord (const juce::StringArray& tokens, juce::StringArray& warnings)
{
    juce::ignoreUnused (warnings);
    texCoords.push_back ({ tokens[1].getFloatValue(), tokens[2].getFloatValue() });
}

/*____________________________________________________________________________*/
// parseFace — resolves every vertex reference, dedups identical triples
// within this one face, then routes to emitFace(). Out-of-bounds anywhere
// skips the whole face; fewer than 3 distinct triples after dedup is a
// degenerate face — both warn-and-continue, never a partial face.
void WavefrontObj::parseFace (const juce::StringArray& tokens, juce::StringArray& warnings)
{
    std::vector<TripleIndex> faceRefs;
    bool allParsed { true };

    for (int i = 1; i < tokens.size() and allParsed; ++i)
    {
        TripleIndex ref;
        allParsed = parseFaceVertexRef (tokens[i], static_cast<int> (positions.size()),
                                            static_cast<int> (texCoords.size()), static_cast<int> (normals.size()), ref);

        if (allParsed and std::find (faceRefs.begin(), faceRefs.end(), ref) == faceRefs.end())
            faceRefs.push_back (ref);
    }

    if (not allParsed)
        warnings.add ("f: out-of-bounds vertex reference, face skipped");
    else if (faceRefs.size() < 3)
        warnings.add ("f: degenerate face (fewer than 3 distinct vertices), face skipped");
    else
        emitFace (faceRefs);
}

/*____________________________________________________________________________*/
// emitFace — a 3-vertex face passes through as one triangle; 4+ triangulates
// via triangulatePolygon() and maps the resulting ring-local indices back to
// this face's own vertex references.
void WavefrontObj::emitFace (const std::vector<TripleIndex>& faceRefs)
{
    if (faceRefs.size() == 3)
    {
        emitTriangle (faceRefs[0], faceRefs[1], faceRefs[2]);
    }
    else
    {
        std::vector<Vertex> polygon;

        for (const auto& ref : faceRefs)
            polygon.push_back (positions.at (static_cast<size_t> (ref.vertexIndex)));

        const auto ringTriangles { triangulatePolygon (polygon) };

        for (size_t t = 0; t < ringTriangles.size(); t += 3)
            emitTriangle (faceRefs[ringTriangles[t]], faceRefs[ringTriangles[t + 1]], faceRefs[ringTriangles[t + 2]]);
    }
}

/*____________________________________________________________________________*/
void WavefrontObj::emitTriangle (const TripleIndex& a, const TripleIndex& b, const TripleIndex& c)
{
    currentShape->mesh.indices.push_back (getOrAppendVertex (a));
    currentShape->mesh.indices.push_back (getOrAppendVertex (b));
    currentShape->mesh.indices.push_back (getOrAppendVertex (c));

    triangleSmoothingGroups.push_back (currentSmoothingGroup);
}

/*____________________________________________________________________________*/
// getOrAppendVertex — the per-shape TripleIndex dedup: an already-emitted
// triple reuses its output index; a new triple appends position/texcoord/
// normal (default-filled when vt/vn are absent) to the shape's Mesh SoA and
// records its position-pool origin for generateNormals().
WavefrontObj::Index WavefrontObj::getOrAppendVertex (const TripleIndex& ref)
{
    const auto existing { tripleIndices.find (ref) };
    Index index { 0 };

    if (existing != tripleIndices.end())
    {
        index = existing->second;
    }
    else
    {
        index = static_cast<Index> (currentShape->mesh.vertices.size());

        currentShape->mesh.vertices.push_back (positions.at (static_cast<size_t> (ref.vertexIndex)));
        currentShape->mesh.textureCoords.push_back (ref.textureIndex >= 0 ? texCoords.at (static_cast<size_t> (ref.textureIndex)) : TextureCoord { 0.0f, 0.0f });
        currentShape->mesh.normals.push_back (ref.normalIndex >= 0 ? normals.at (static_cast<size_t> (ref.normalIndex)) : Vertex { 0.0f, 0.0f, 0.0f });

        vertexPositionIndices.push_back (ref.vertexIndex);
        shapeHasExplicitNormals = shapeHasExplicitNormals or ref.normalIndex >= 0;

        tripleIndices.insert ({ ref, index });
    }

    return index;
}

/*____________________________________________________________________________*/
// startShape — g/o both start a new Shape; content before the first one
// belongs to the unnamed default shape resetParseState() already opened.
void WavefrontObj::startShape (const juce::StringArray& tokens, juce::StringArray& warnings)
{
    juce::ignoreUnused (warnings);
    finalizeCurrentShape();

    currentShape = std::make_unique<Shape>();
    currentShape->name = tokens.size() > 1 ? tokens[1] : juce::String();
}

/*____________________________________________________________________________*/
void WavefrontObj::setSmoothingGroup (const juce::StringArray& tokens, juce::StringArray& warnings)
{
    juce::ignoreUnused (warnings);
    currentSmoothingGroup = tokens[1].equalsIgnoreCase ("off") ? 0 : tokens[1].getIntValue();
}

/*____________________________________________________________________________*/
void WavefrontObj::useMaterial (const juce::StringArray& tokens, juce::StringArray& warnings)
{
    const auto name { tokens[1] };

    if (materials.contains (name))
    {
        currentShape->material = materials.at (name);
    }
    else
    {
        warnings.add ("usemtl: unknown material '" + name + "', using default");
        currentShape->material = Material {};
    }
}

/*____________________________________________________________________________*/
void WavefrontObj::generateNormals (Mesh& mesh) const
{
    mesh.normals.clear();

    for (size_t vertex = 0; vertex < mesh.vertices.size(); ++vertex)
        mesh.normals.push_back (Vertex { 0.0f, 0.0f, 0.0f });

    jam::HashMap<std::pair<int, int>, Vertex> smoothedNormals;

    accumulateFaceNormals (mesh, triangleSmoothingGroups, vertexPositionIndices, smoothedNormals);
    applySmoothedNormals (mesh, triangleSmoothingGroups, vertexPositionIndices, smoothedNormals);

    for (size_t vertex = 0; vertex < mesh.vertices.size(); ++vertex)
        mesh.normals.at (vertex) = normaliseVertex (mesh.normals.at (vertex));
}

/*____________________________________________________________________________*/
// load — reads objFile, then BOM-stripped, keyword-dispatched line parse;
// mtllib's relative paths resolve against objFile's parent directory
// (resetParseState() records it as sourceFile). The last open shape is
// finalized once dispatch completes (g/o finalize the ones before them).
// The unreadable-file check is the ONE hard failure; the parse itself is
// warn-and-continue throughout.
juce::Result WavefrontObj::load (const juce::File& objFile, juce::StringArray* optionalWarnings)
{
    juce::Result result { juce::Result::fail ("Could not open " + objFile.getFullPathName()) };

    if (objFile.existsAsFile())
    {
        resetParseState (objFile);

        if (parser.size() == 0)
            registerParser();

        if (materialParser.size() == 0)
            registerMaterialParser();

        // Every parse routine appends into a concrete juce::StringArray — a
        // caller discarding diagnostics (optionalWarnings == nullptr, the
        // default) gets this local array instead, dropped at return; the
        // hard-failure message rides the returned juce::Result either way.
        juce::StringArray discardedWarnings;
        auto& warnings { optionalWarnings != nullptr ? *optionalWarnings : discardedWarnings };

        const auto objText { objFile.loadFileAsString() };
        const auto byteOrderMark { static_cast<juce::juce_wchar> (0xfeffu) };
        const auto text { objText.startsWithChar (byteOrderMark) ? objText.substring (1) : objText };

        dispatchLines (text, parser, warnings);
        finalizeCurrentShape();

        result = juce::Result::ok();
    }

    return result;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
