/** @file jam_VulkanShader.h
 *  @brief Compiled multi-pass shader description — SPIR-V fragment blobs,
 *         the pure-data unit consumed by the engine's shader execution
 *         machinery.
 */

namespace jam
{
/*____________________________________________________________________________*/
/** @brief Compiled, immutable, multi-pass shader description.
 *
 *  Pure data — no Vulkan handles, no presentation parameters. Opacity and
 *  resolution scale are supplied by the caller at render/build time (see
 *  render(), VulkanShaderComponent::setShader()/setParams(),
 *  VulkanEngine::setPostProcess()/setPostProcessParams()), never stored here —
 *  a VulkanShader's identity (contentHash below) is therefore unaffected by either
 *  value changing. Sampling resample mode is likewise never stored — resolved
 *  by jam::VulkanShaderCompiler at GLSL-generation time from
 *  map::ImageResample::value (lexicon/jam_vulkan.md),
 *  baked into the generated sampler macro per compiled pass, not a VulkanShader field.
 *
 *  passes holds the ordered pass chain: every buffer pass unconditionally
 *  ping-pong-feedback-capable (VulkanShaderPass's own doc comment) followed by the
 *  mandatory final Image pass, always the last entry.
 *
 *  contentHash is a content-derived identity computed once at construction —
 *  the single source of truth a consumer (VulkanShaderInstance) compares against
 *  its own cached value to detect that the compiled content actually changed
 *  and its GPU-side resources must be rebuilt. Identical recompile output
 *  (same pass names, same SPIR-V bytes) yields an identical hash, so a
 *  downstream cache hit survives a no-op recompile.
 *
 *  Every field is const, set once at construction — public, no getters,
 *  consumers read fields directly. Non-copyable, non-movable — every
 *  instance is held by the caller through indirection (e.g.
 *  std::unique_ptr<VulkanShader>).
 */
struct VulkanShader
{
    /** @brief Constructs a compiled shader from its pass chain.
     *  @param newPasses     Ordered pass chain — buffer passes (lexicographic
     *                       name order) followed by the mandatory final VulkanImage
     *                       pass, the last entry.
     *  @param sourceFormat  This VulkanShader's source format — VulkanShaderCompiler::
     *                       compile()'s own @p format parameter, carried
     *                       forward rather than re-derived: every pass this
     *                       VulkanShader owns was compiled through the SAME format
     *                       (a per-compile-call choice, never a per-pass
     *                       one), so ONE field here (not a VulkanShaderPass-level
     *                       field, which would duplicate the identical value
     *                       across every pass) is this VulkanShader's SSOT for
     *                       whether VulkanShaderInstance builds/draws this
     *                       execution's passes through the shared bindless
     *                       pipeline layout (VulkanShaderFormat::shadertoy) or each
     *                       pass's own SPIR-V-reflected dedicated layout/
     *                       descriptor set (VulkanShaderFormat::slang) — see
     *                       VulkanShaderInstance::build()'s
     *                       own top-level branch on this field.
     *  @param newPreset     This VulkanShader's own parsed .slangp preset
     *                       directives (jam::VulkanShaderPreset::parse()'s
     *                       result) — empty passes/overrides for a
     *                       VulkanShaderFormat::shadertoy project declaring no
     *                       .slangp resource manifest of its own at all
     *                       (that format's own optional textures=/mesh=
     *                       manifest, when present, still carries zero
     *                       passes). VulkanShaderInstance's own per-pass extent
     *                       computation reads this field (see
     *                       VulkanShaderInstance::build()'s own doc comment).
     *  @param newMeshPath   This VulkanShader's own OBJ mesh connection — an
     *                       absolute path (already absolutized by the
     *                       caller, VulkanShaderCompiler::compile(), against the
     *                       shader project directory, from the parsed
     *                       .slangp manifest's own mesh= key,
     *                       VulkanShaderPreset::meshPath — an END-extension key,
     *                       not RetroArch vocabulary, lexicon/jam_vulkan.md's
     *                       own preset category doc), or empty
     *                       for every shader with no mesh connection (every
     *                       project before mesh support was added). Folded
     *                       into contentHash below so a manifest
     *                       edit toggling/changing the mesh path still
     *                       rebuilds this VulkanShader's VulkanShaderInstance.
     *  @param newMeshShaderPath      This VulkanShader's own mesh-backed pass AUTHOR
     *                       .slang connection — mirrors @p newMeshPath's exact
     *                       shape one entry further (already absolutized by
     *                       the caller, VulkanShaderCompiler::compile(), from
     *                       VulkanShaderPreset::meshShaderPath), or empty for every
     *                       mesh connection using only the engine's own
     *                       default, unanimated mesh look.
     *                       mesh_shader= is a vertex-ANIMATION HOOK into the
     *                       engine's own default mesh look, mirroring
     *                       Shadertoy's mainImage paradigm one level down —
     *                       never a full-stage replacement.
     *  @param newMeshShaderSource    This VulkanShader's own raw mainMesh snippet
     *                       source — @p newMeshShaderPath's file content,
     *                       already \#include-expanded (VulkanShaderFormat::
     *                       expandIncludes()) by the caller, VulkanShaderCompiler::
     *                       compile() — a plain GLSL snippet defining exactly
     *                       `void mainMesh (inout vec3 position, inout vec3
     *                       normal)`, the author's object-space vertex
     *                       transform. Declares no uniform blocks, no
     *                       descriptor sets, no \#pragma stages, no engine
     *                       tokens — the engine's own mesh vertex-stage
     *                       templates (jam_vulkan/shader/mesh_default.vert,
     *                       mesh_edge.vert) declare everything and splice
     *                       this snippet in at their own \%\%mainMesh\%\%
     *                       placeholder (jam::VulkanShaderCompiler::
     *                       compileMeshVertexStage()), compiled fresh per
     *                       VulkanShaderInstance (jam::VulkanShaderInstance::
     *                       buildMeshHookPipelines()). Empty for every mesh
     *                       connection using only the engine's own default,
     *                       unanimated look, or a mesh_shader= file that
     *                       failed to read.
     *  @param newMeshShapes This VulkanShader's own parsed OBJ mesh geometry —
     *                       jam::WavefrontObj::load() run ONCE by the
     *                       caller, VulkanShaderCompiler::compile(), against
     *                       @p newMeshPath, then moved out via
     *                       jam::WavefrontObj::extractShapes(). Every
     *                       per-extent VulkanShaderInstance rebuild
     *                       (VulkanShaderInstance::buildMeshResources()) reads
     *                       this field directly instead of re-parsing the
     *                       OBJ file — no vk::Device/vk::CommandBuffer
     *                       exists at compile time, so only the CPU parse
     *                       moves here; the GPU upload (jam::
     *                       VulkanMesh::build()) still runs per VulkanShaderInstance.
     *                       Empty for every shader with no mesh connection
     *                       (@p newMeshPath empty) or whose mesh= file
     *                       failed to load (@p newMeshPath non-empty,
     *                       parse failed) — jam::VulkanMesh::build()
     *                       already treats an empty jam::Owner<Shape> as
     *                       "nothing to upload," so this field's own
     *                       emptiness is the SSOT "no mesh geometry" signal,
     *                       mirroring every other empty-signal field in this
     *                       struct.
     */
    explicit VulkanShader (jam::Owner<VulkanShaderPass> newPasses, int sourceFormat, VulkanShaderPreset newPreset,
                     juce::String newMeshPath, juce::String newMeshShaderPath,
                     juce::String newMeshShaderSource, jam::Owner<jam::WavefrontObj::Shape> newMeshShapes)
        : passes (std::move (newPasses)), format (sourceFormat), preset (std::move (newPreset)),
          meshPath (std::move (newMeshPath)), meshShaderPath (std::move (newMeshShaderPath)),
          meshShaderSource (std::move (newMeshShaderSource)), meshShapes (std::move (newMeshShapes)),
          contentHash (computeContentHash (passes, preset, meshPath, meshShaderPath, meshShaderSource))
    {
        jassert (not passes.isEmpty());                                    // mandatory Image pass present
        jassert (passes.size() <= static_cast<size_t> (VulkanShaderUniforms::maxChannelCount) + 1);
    }

    /** @brief Ordered pass chain — buffer passes (lexicographic name order)
     *  then the mandatory Image pass, the last entry. */
    const jam::Owner<VulkanShaderPass> passes;

    /** @brief This VulkanShader's source format — see the constructor's own
     *  @p sourceFormat doc comment above. Field addition (slang-reflection
     *  work) to this already-built struct. */
    const int format;

    /** @brief This VulkanShader's own parsed .slangp preset directives — see the
     *  constructor's own @p newPreset doc comment above. Field addition
     *  (per-pass-extent/preset work) to this already-built struct, same
     *  precedent as format above. */
    const VulkanShaderPreset preset;

    /** @brief This VulkanShader's own OBJ mesh connection — see the constructor's
     *  own @p newMeshPath doc comment above. Empty for every shader with no
     *  mesh connection (jam::VulkanShaderInstance::build() branches on
     *  meshPath.isNotEmpty() to build the mesh-backed gather target). Field
     *  addition (the mesh-backed shader pass) to this already-built struct,
     *  same precedent as format/preset above. */
    const juce::String meshPath;

    /** @brief This VulkanShader's own mesh-backed pass AUTHOR .slang connection —
     *  see the constructor's own @p newMeshShaderPath doc comment above.
     *  Empty for every mesh connection using only the engine's own default,
     *  unanimated mesh look (jam::VulkanShaderInstance::hasMeshHook()
     *  branches on meshShaderSource below, not this path directly — this
     *  field exists for diagnostics/documentation parity with meshPath). */
    const juce::String meshShaderPath;

    /** @brief This VulkanShader's own raw mainMesh snippet source — see the
     *  constructor's own @p newMeshShaderSource doc comment above. Empty
     *  exactly when this VulkanShader declares no mesh_shader= connection at all,
     *  or one that failed to read — jam::VulkanShaderInstance::
     *  buildMeshHookPipelines() branches on this being non-empty to compile
     *  and build its own hooked mesh pipelines, falling back to the engine's
     *  default, unanimated mesh pipelines otherwise. */
    const juce::String meshShaderSource;

    /** @brief This VulkanShader's own parsed OBJ mesh geometry — see the
     *  constructor's own @p newMeshShapes doc comment above. Parsed exactly
     *  ONCE per compile (VulkanShaderCompiler::compile()), consumed by every
     *  per-extent VulkanShaderInstance rebuild (VulkanShaderInstance::
     *  buildMeshResources()) without any re-parse — replaces the old
     *  per-rebuild jam::WavefrontObj::load() call, which re-ran the CPU
     *  parse (and re-logged its own identical warnings) on every window-
     *  resize extent change. NOT folded into contentHash below — the
     *  parsed OBJ file's own CONTENT was never hashed before this field
     *  existed either (meshPath's own path string is what contentHash
     *  folds), so this field introduces no new content-hash gap. */
    const jam::Owner<jam::WavefrontObj::Shape> meshShapes;

    /** @brief Content-derived identity — one wyhash-family combine
     *  (jam::Wyhash::mix, jam_HashMap.h) over every pass's name, spirv, and
     *  vertexSpirv bytes, in pass order, plus preset's own content hash
     *  (VulkanShaderPreset::hash()). REPLACES a monotonic generation counter
     *  entirely: identity is derived from compiled content, so an unchanged
     *  recompile produces an identical hash — SSOT for "this VulkanShader's
     *  compiled content changed" (VulkanShaderInstance::build(),
     *  VulkanGraphics::getOrCreateShaderInstance()). */
    const uint64_t contentHash;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanShader)

private:
    /** @brief Computes contentHash — folds every pass's name, spirv,
     *  vertexSpirv bytes, and parameterDefaults, in pass order, through
     *  jam::Wyhash::mix/hashBytes (jam_HashMap.h), then folds @p newPreset's
     *  own content hash (VulkanShaderPreset::hash()) last. Hashing an empty
     *  vertexSpirv (today, every Shadertoy-compiled pass) is deterministic/
     *  stable, no special-casing needed.
     *
     *  parameterDefaults is folded so that hot-reload editing ONLY an
     *  author's \#pragma parameter default (never a spirv/vertexSpirv byte —
     *  glslang silently drops the pragma line, so it never reaches SPIR-V)
     *  still rebuilds the VulkanShaderInstance — the hot-reload-tuning contract.
     *  jam::HashMap's own iteration order is unspecified (jam_HashMap.h), so
     *  each entry's own combined hash (its name bytes mixed with its float's
     *  bit pattern) is XOR-accumulated first — XOR is commutative, so the
     *  accumulator is identical regardless of iteration order — before that
     *  one order-independent accumulator is mixed into the running,
     *  pass-order-dependent hash above. @p newPreset is folded the same
     *  way — a preset-only edit (scale/filter/wrap/alias/override) still
     *  changes contentHash, the same hot-reload-tuning contract
     *  parameterDefaults already established. @p newMeshPath is folded next —
     *  a manifest edit toggling/changing the mesh connection alone (no
     *  pass/preset byte changed) still rebuilds this VulkanShader's VulkanShaderInstance.
     *  @p newMeshShaderPath/@p newMeshShaderSource are folded last, mirroring
     *  @p newMeshPath's own fold — an edit to the mesh_shader= mainMesh
     *  snippet (path OR source text — editing ONLY the snippet hot-reloads,
     *  since it is compiled fresh per VulkanShaderInstance, never cached
     *  elsewhere) still rebuilds this VulkanShader's VulkanShaderInstance. */
    static uint64_t computeContentHash (const jam::Owner<VulkanShaderPass>& passes, const VulkanShaderPreset& newPreset,
                                       const juce::String& newMeshPath, const juce::String& newMeshShaderPath,
                                       const juce::String& newMeshShaderSource) noexcept
    {
        // jam::Wyhash::fold, never mix, as the chained accumulator — mix()'s
        // MUM multiply absorbs zero (mix(state, v) is 0 whenever EITHER
        // operand is 0), so the zero seed below plus any zero-valued part
        // (an empty parameterDefaults accumulator — every Shadertoy pass —
        // or a 0.0f default's bit pattern) would zero the ENTIRE chain and
        // every VulkanShader would hash identically (jam::Wyhash::fold's own doc
        // comment, jam_lexicon/utils/jam_HashMap.h).
        uint64_t hash { 0 };

        for (auto& pass : passes)
        {
            hash = jam::Wyhash::fold (hash, jam::Wyhash::hashBytes (
                pass->name.toRawUTF8(), pass->name.getNumBytesAsUTF8()));
            hash = jam::Wyhash::fold (hash, jam::Wyhash::hashBytes (
                pass->spirv.getData(), pass->spirv.getSize()));
            hash = jam::Wyhash::fold (hash, jam::Wyhash::hashBytes (
                pass->vertexSpirv.getData(), pass->vertexSpirv.getSize()));

            uint64_t parameterDefaultsHash { 0 };

            for (const auto& [parameterName, parameterDefault] : pass->parameterDefaults)
            {
                const auto nameHash { jam::Wyhash::hashBytes (
                    parameterName.toRawUTF8(), parameterName.getNumBytesAsUTF8()) };

                uint32_t defaultBits { 0 };
                std::memcpy (&defaultBits, &parameterDefault, sizeof (defaultBits));

                parameterDefaultsHash ^= jam::Wyhash::fold (nameHash, defaultBits);
            }

            hash = jam::Wyhash::fold (hash, parameterDefaultsHash);
        }

        hash = jam::Wyhash::fold (hash, newPreset.hash());

        hash = jam::Wyhash::fold (hash, jam::Wyhash::hashBytes (
            newMeshPath.toRawUTF8(), newMeshPath.getNumBytesAsUTF8()));

        hash = jam::Wyhash::fold (hash, jam::Wyhash::hashBytes (
            newMeshShaderPath.toRawUTF8(), newMeshShaderPath.getNumBytesAsUTF8()));

        return jam::Wyhash::fold (hash, jam::Wyhash::hashBytes (
            newMeshShaderSource.toRawUTF8(), newMeshShaderSource.getNumBytesAsUTF8()));
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam