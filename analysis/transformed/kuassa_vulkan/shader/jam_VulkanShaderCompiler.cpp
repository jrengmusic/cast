// exception (external, vendored under jam_vulkan/shaderc/ — deliberately kept out of
// jam_VulkanShaderCompiler.h so no shaderc symbol reaches any consumer of jam_vulkan).
#include <shaderc/shaderc.hpp>

namespace jam
{
/*____________________________________________________________________________*/
// Channel/scene macro text, moved out of hand-rolled juce::String concatenation
// into BinaryData-backed templates (mirrors the deleted graphics::Compositor's
// wrapper.frag + jam::Format::replaceholder precedent, commit 4dbc2ee) —
// substituted per compile via the @token@ placeholder tokens named below,
// never a duplicated GLSL literal in C++ source. The wrapper template itself
// (shadertoy_wrapper.frag — the ONLY format assemblePass() is still reached
// for, since VulkanShaderFormat::slang now compiles its own source completely
// as-is, see compile()'s compilePass lambda below) is resolved per compile via
// VulkanShaderFormat — see assemblePass() — rather than cached here, since which
// wrapper applies is a per-pass format choice, not a process-wide constant.
static const juce::String& getChannelMacroTemplate()
{
    static const juce::String macroTemplate { BinaryData::getString (files::shaderToyChannelMacro) };
    return macroTemplate;
}

static const juce::String& getSceneMacroTemplate()
{
    static const juce::String macroTemplate { BinaryData::getString (files::shaderToySceneMacro) };
    return macroTemplate;
}

// Set-0 bindless sampler2D[] array identifiers -- MUST match shadertoy_wrapper.frag's own
// binding declarations (layout(set = 0, binding = 3) uniform sampler2D texturesLinear[];
// binding 4 = texturesNearest[]).
//
// Per-filter sampler-array-name dispatch -- jam::LookupTable direct-indexed by
// map::ImageResample::value cast to int (LookupTable's own Key contract requires
// an integral type, jam_LookupTable.h's is_integral_v static_assert -- an enum class
// key, even one with an int underlying type, does not satisfy it, same reason
// optimizationLevelLut below keys VulkanShaderFormat's own plain-enum ordinal directly but
// this table cannot key value directly). Consumed by both channelMacros() and
// sceneMacro(), replacing their identically duplicated ternary. jam::LookupTable's own
// Value contract keeps juce::Identifier out of a table slot (jam_LookupTable.h's own
// "ints in, ints out" doc comment) and requires a constexpr-constructible Value, so this
// table sources its rows from Id::texturesLinear/Id::texturesNearest's own process-lifetime
// backing juce::String at runtime instead of at file-scope static-init time.
static const jam::LookupTable<int, const char*, 2>& getSamplerArrayLut()
{
    static const jam::LookupTable<int, const char*, 2> samplerArrayLut
    {
        {
            { static_cast<int> (map::ImageResample::linear),  Id::texturesLinear.toString().toRawUTF8() },
            { static_cast<int> (map::ImageResample::nearest), Id::texturesNearest.toString().toRawUTF8() },
        }
    };
    return samplerArrayLut;
}

/*____________________________________________________________________________*/
// Shadertoy's own channel-uniform prefix (iChannel0-3, shadertoy_wrapper.frag's own
// paste-compat contract). channelMacros()/sceneMacro() below are only
// ever reached for VulkanShaderFormat::shadertoy -- compile()'s own compilePass lambda skips
// assemblePass() (and everything it calls) entirely for VulkanShaderFormat::slang,
// per the slang zero-injection contract (see compile()'s own doc comment) --
// so no per-format alias
// dispatch is needed here.
static juce::StringArray shadertoyChannelAliases (int ordinal)
{
    static constexpr int shadertoyAliasCount { 4 };// iChannel0-3 paste-compat ceiling

    juce::StringArray aliases;

    if (ordinal < shadertoyAliasCount)
        aliases.add (Id::iChannel.toString() + juce::String (ordinal));

    return aliases;
}

// Per-format optimization-level dispatch, same idiom -- VulkanShaderFormat::slang
// compiles unoptimized because jam::VulkanShaderReflection's SPIR-V
// reflection matches UBO/push_constant members and texture resources BY NAME
// via OpName/OpMemberName, which shaderc's performance optimization strips;
// VulkanShaderFormat::shadertoy keeps performance optimization since its SPIR-V is
// never reflected.
static constexpr jam::LookupTable<int, shaderc_optimization_level, 2> optimizationLevelLut
{
    {
        { jam::VulkanShaderFormat::shadertoy, shaderc_optimization_level_performance },
        { jam::VulkanShaderFormat::slang,     shaderc_optimization_level_zero },
    }
};

juce::String VulkanShaderCompiler::channelMacros (const juce::StringArray& bufferPassNames,
                                             const jam::Array<VulkanShaderPreset::Texture>& textures,
                                             map::ImageResample::value filter)
{
    const juce::String samplerArray { getSamplerArrayLut()[static_cast<int> (filter)] };
    juce::String macros;

    // bufferPassNames is already ordinal-ordered by construction (compile()'s
    // VulkanShaderFormat-driven discovery loop) — a direct indexed loop is this
    // ordering's own canon reader, no transient name-resolving map needed.
    for (int ordinal = 0; ordinal < bufferPassNames.size(); ++ordinal)
    {
        const auto& name { bufferPassNames.getReference (ordinal) };
        const juce::String expression {
            samplerArray + "[nonuniformEXT(channels[" + juce::String (ordinal) + "])]"
        };

        macros << jam::Format::replaceholder (
            jam::Format::replaceholder (getChannelMacroTemplate(), Id::name, name),
            Id::expression, expression);

        for (const auto& alias : shadertoyChannelAliases (ordinal))
            macros << jam::Format::replaceholder (
                jam::Format::replaceholder (getChannelMacroTemplate(), Id::name, alias),
                Id::expression, expression);
    }

    // Named external textures (preset.textures, the project's own .slangp
    // textures= list) are assigned channel slots on the SAME push-constant
    // channels[] array every buffer pass above already indexes — never a
    // separate array, and pure declaration order (texture K's own slot is
    // bufferPassNames.size() + K, RetroArch's own textures= directive
    // quartet carries no channel key at all). Channel BINDING is name-based
    // instead: an entry literally named iChannel2 binds that reference
    // directly (this loop's own \#define emits exactly that name), so no
    // separate iChannelN alias is emitted here the way the buffer-pass loop
    // above does. compile()'s own bound assert guarantees every slot
    // reaching this loop stays inside the push-constant channels[] array's
    // own bound (VulkanShaderUniforms::maxChannelCount) before a single pass is
    // ever compiled against it — order-assigned slots can never collide, so
    // no collision check applies either.
    for (int index = 0; index < textures.size(); ++index)
    {
        const auto& texture { textures.at (index) };
        const int ordinal { bufferPassNames.size() + index };

        // Per-texture filter choice, independent of every buffer pass above
        // (which always samples through the SAME filter argument) — an
        // entry's own explicit filterLinear wins; absent falls back to this
        // SAME call's own global filter (mirrors compile()'s own
        // UNSPEC-falls-back-to-global-filter resolution for preset.passes,
        // just resolved lazily here instead of eagerly mutated onto the
        // preset, since VulkanShaderPreset::Texture::filterLinear is left
        // deliberately unresolved at parse time — see its own doc comment).
        const bool textureLinear {
            texture.filterLinear.value_or (filter == map::ImageResample::linear)
        };
        const juce::String textureSamplerArray {
            getSamplerArrayLut()[static_cast<int> (textureLinear ? map::ImageResample::linear
                                                                   : map::ImageResample::nearest)]
        };
        const juce::String expression {
            textureSamplerArray + "[nonuniformEXT(channels[" + juce::String (ordinal) + "])]"
        };

        macros << jam::Format::replaceholder (
            jam::Format::replaceholder (getChannelMacroTemplate(), Id::name, texture.name),
            Id::expression, expression);
    }

    return macros;
}

juce::String VulkanShaderCompiler::sceneMacro (map::ImageResample::value filter)
{
    const juce::String samplerArray { getSamplerArrayLut()[static_cast<int> (filter)] };
    const juce::String expression { samplerArray + "[nonuniformEXT(" + Id::iScene.toString() + ")]" };

    return jam::Format::replaceholder (getSceneMacroTemplate(), Id::expression, expression);
}

juce::String VulkanShaderCompiler::assemblePass (const juce::String& passSource, const juce::String& commonSource,
                                            const juce::StringArray& bufferPassNames,
                                            const jam::Array<VulkanShaderPreset::Texture>& textures,
                                            map::ImageResample::value filter, bool isBackground, int format)
{
    juce::String prelude { jam::Format::replaceholder (
        BinaryData::getString (VulkanShaderFormat::get (format)), Id::maxChannelCount,
        juce::String (jam::VulkanShaderUniforms::maxChannelCount)) };

    prelude = jam::Format::replaceholder (prelude, Id::channelMacros,
                                           channelMacros (bufferPassNames, textures, filter));
    prelude = jam::Format::replaceholder (prelude, Id::sceneMacro,
                                           isBackground ? juce::String() : sceneMacro (filter));
    prelude = jam::Format::replaceholder (prelude, Id::commonSource, commonSource);
    prelude = jam::Format::replaceholder (prelude, Id::passSource, passSource);

    return prelude;
}

static const juce::String parameterTokenBreakCharacters { " \t" };
static const juce::String parameterTokenQuoteCharacters { "\"" };
static constexpr int parameterIdentifierTokenIndex { 2 };
static constexpr int parameterDefaultTokenIndex { 4 };

jam::HashMap<juce::String, float> VulkanShaderCompiler::parseParameterDefaults (const juce::String& source)
{
    // RetroArch's #pragma parameter declaration --
    // "#pragma parameter <identifier> \"<label>\" <default> <min> <max> [<step>]"
    // -- named once here, never a repeated magic string/index at a call site.
    // Composed from Id::pragma ("#pragma", lexicon/jam_vulkan.md's own semantic category)
    // and Id::parameter ("parameter", jam_data_structures/lexicon.md's own parameters category) via
    // jam::Format::appendWithSpace() -- never a locally duplicated string literal.
    static const juce::String parameterPragma { jam::Format::appendWithSpace (Id::pragma, Id::parameter) };

    jam::HashMap<juce::String, float> parameterDefaults;

    for (const auto& rawLine : juce::StringArray::fromLines (source))
    {
        const auto line { rawLine.trim() };

        if (line.startsWith (parameterPragma))
        {
            // Quote-aware tokenizing keeps the quoted "<label>" field (which
            // may itself contain spaces) as ONE token, so the identifier and
            // default fields always land at the same fixed positions.
            const auto tokens { juce::StringArray::fromTokens (
                line, parameterTokenBreakCharacters, parameterTokenQuoteCharacters) };

            // A malformed "#pragma parameter" line (fewer tokens than the
            // identifier/default fields require) contributes no entry --
            // juce::StringArray::operator[] returns a silent default String
            // out of range, which would otherwise emplace a bogus empty-
            // identifier/zero-default entry instead of simply skipping the
            // line (this loader's own existing tolerance for absent
            // directives elsewhere in this file).
            if (tokens.size() > parameterDefaultTokenIndex)
                parameterDefaults.emplace (tokens.getReference (parameterIdentifierTokenIndex),
                                            tokens.getReference (parameterDefaultTokenIndex).getFloatValue());
        }
    }

    return parameterDefaults;
}

juce::String VulkanShaderCompiler::parsePassName (const juce::String& source)
{
    // RetroArch's own #pragma name declaration (slang_process.cpp:941-942's own
    // fallback read of it) -- named once here, never a repeated magic string at
    // a call site. Carries a single bare identifier token (no quoted label field
    // the way #pragma parameter does), so this function needs no
    // tokenizing -- just the line's own remainder past this prefix, trimmed.
    // Composed the same way parameterPragma is (Id::pragma +
    // Id::name, jam::Format::appendWithSpace()).
    static const juce::String namePragma { jam::Format::appendWithSpace (Id::pragma, Id::name) };

    juce::String name;

    for (const auto& rawLine : juce::StringArray::fromLines (source))
    {
        const auto line { rawLine.trim() };

        if (line.startsWith (namePragma))
            name = line.fromFirstOccurrenceOf (namePragma, false, false).trim();
    }

    return name;
}

VulkanShaderCompiler::SlangStages VulkanShaderCompiler::splitSlangStages (const juce::String& source)
{
    // Composed from Id::pragma/stage/vertex/fragment
    // (lexicon/jam_vulkan.md's own semantic category) via nested
    // jam::Format::appendWithSpace() calls -- never a locally duplicated string literal.
    static const juce::String vertexPragma   { jam::Format::appendWithSpace (jam::Format::appendWithSpace (Id::pragma, Id::stage), Id::vertex) };
    static const juce::String fragmentPragma { jam::Format::appendWithSpace (jam::Format::appendWithSpace (Id::pragma, Id::stage), Id::fragment) };

    const int vertexMarker   { source.indexOf (vertexPragma) };
    const int fragmentMarker { source.indexOf (fragmentPragma) };

    const juce::String sharedSource { source.substring (0, vertexMarker) };
    const juce::String vertexBody   { source.substring (vertexMarker + vertexPragma.length(), fragmentMarker) };
    const juce::String fragmentBody { source.substring (fragmentMarker + fragmentPragma.length()) };

    return { sharedSource + vertexBody, sharedSource + fragmentBody };
}

juce::MemoryBlock VulkanShaderCompiler::compileSpirv (const juce::String& source, const juce::String& passName, Stage stage,
                                                 int format)
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    options.SetTargetEnvironment (shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);
    options.SetOptimizationLevel (optimizationLevelLut[format]);

    const auto shaderKind {
        stage == Stage::vertex ? shaderc_glsl_vertex_shader : shaderc_glsl_fragment_shader
    };

    const auto result { compiler.CompileGlslToSpv (source.toRawUTF8(), source.getNumBytesAsUTF8(),
                                                    shaderKind, passName.toRawUTF8(), options) };

    juce::MemoryBlock spirv;

    if (result.GetCompilationStatus() == shaderc_compilation_status_success)
        spirv.append (result.cbegin(), static_cast<size_t> (result.cend() - result.cbegin()) * sizeof (uint32_t));
    else
    {
#if JUCE_DEBUG
        jam::debug::Log::write ("jam::VulkanShaderCompiler: " + passName + " -- " + result.GetErrorMessage());
#endif
    }

    return spirv;
}

juce::MemoryBlock VulkanShaderCompiler::compileMeshVertexStage (const juce::String& templateFilename,
                                                          const juce::String& meshHookSource)
{
    const juce::String assembledSource { jam::Format::replaceholder (
        BinaryData::getString (templateFilename), Id::mainMesh,
        meshHookSource.isNotEmpty() ? meshHookSource : text::English::mainMeshNoOp) };

    return compileSpirv (assembledSource, templateFilename, Stage::vertex, VulkanShaderFormat::shadertoy);
}

std::unique_ptr<jam::VulkanShader> VulkanShaderCompiler::compile (const juce::ValueTree& shaderState,
                                                               bool isBackground,
                                                               int format,
                                                               map::ImageResample::value filter)
{
    const auto& presetNames { VulkanShaderFormat::getPresetNames() };

    const juce::String commonSource { shaderState.getProperty (juce::Identifier (presetNames.at (VulkanShaderFormat::common))).toString() };
    const juce::String imageSource { shaderState.getProperty (juce::Identifier (presetNames.at (VulkanShaderFormat::image))).toString() };
    const juce::String imagePassName { presetNames.at (VulkanShaderFormat::image) };

    // Raw .slangp manifest text -- the project's own full preset for
    // VulkanShaderFormat::slang; an optional resource-manifest-only .slangp
    // (textures=/mesh=, no shaders=/passes) for VulkanShaderFormat::shadertoy when
    // the project declares one, empty otherwise. VulkanShaderPreset::parse() of an
    // empty string yields zero passes, zero textures, empty meshPath, empty
    // overrides (jam_VulkanShaderPreset.h's own doc comment).
    const juce::String presetText { shaderState.getProperty (juce::Identifier (presetNames.at (VulkanShaderFormat::preset))).toString() };
    VulkanShaderPreset preset { VulkanShaderPreset::parse (presetText) };

    // Project directory -- the host application's own shader loader stamps
    // Id::path at shaderState's own root level -- absolutizes
    // every parsed texture's own path and preset.meshPath itself: both are
    // AS-WRITTEN, relative to that SAME directory (VulkanShaderPreset::Texture::path
    // and VulkanShaderPreset::meshPath's own doc comments), so VulkanShader::preset.textures
    // and VulkanShader::meshPath both always carry loadable absolute paths downstream
    // (VulkanShaderInstance/VulkanGraphics::buildExternalTexture(), VulkanShaderInstance::build()).
    const juce::String projectDirPath { shaderState.getProperty (Id::path).toString() };

    for (auto& texture : preset.textures)
        texture.path = juce::File (projectDirPath).getChildFile (texture.path).getFullPathName();

    const juce::String meshPath { preset.meshPath.isNotEmpty()
        ? juce::File (projectDirPath).getChildFile (preset.meshPath).getFullPathName()
        : juce::String() };

    // mesh_shader= — mirrors meshPath's own absolutize step one entry
    // further (VulkanShaderPreset::meshShaderPath's own doc comment): the
    // mesh-backed pass's own AUTHOR .slang connection.
    const juce::String meshShaderPath { preset.meshShaderPath.isNotEmpty()
        ? juce::File (projectDirPath).getChildFile (preset.meshShaderPath).getFullPathName()
        : juce::String() };

    // mesh_shader= is a vertex-ANIMATION HOOK into the engine's own
    // default mesh look (mirroring Shadertoy's mainImage paradigm one level
    // down), never a full-stage replacement — SSOT. The author file is a
    // plain GLSL snippet defining exactly `void mainMesh (inout vec3
    // position, inout vec3 normal)`, no uniform blocks/descriptor sets/
    // #pragma stages/engine tokens of its own — read and #include-expanded
    // here (VulkanShaderFormat::expandIncludes(), the SAME expansion every other
    // .slang source gets), never compiled to SPIR-V at THIS call: compiling
    // happens per-VulkanShaderInstance instead (VulkanShaderCompiler::
    // compileMeshVertexStage(), spliced into the engine's own mesh vertex-
    // stage templates), since the SAME snippet is compiled TWICE — once per
    // engine mesh vertex-stage template (mesh_default.vert/mesh_edge.vert) —
    // never once here in isolation. Read unconditionally alongside the pass
    // chain below, never gated on @p imageSource — a mesh_shader= connection
    // is orthogonal to whether this VulkanShader's own background/post-process
    // pass chain even exists. Empty for every mesh connection using only the
    // engine's own default, unanimated look (VulkanShader::meshShaderSource's own
    // doc comment).
    juce::String meshShaderSource;

    if (meshShaderPath.isNotEmpty())
        meshShaderSource = VulkanShaderFormat::expandIncludes (juce::File (meshShaderPath));

    // Parsed exactly ONCE here, at compile time, never per VulkanShaderInstance
    // rebuild — jam::VulkanShaderInstance::buildMeshResources() previously
    // re-ran jam::WavefrontObj::load() (and re-logged its own identical
    // mtllib/usemtl warnings) on EVERY per-extent rebuild, including every
    // window-resize extent change. The CPU parse moves here; the GPU upload
    // (jam::VulkanMesh::build()) still runs per VulkanShaderInstance, since no
    // vk::Device/vk::CommandBuffer exists at this call. A parse failure
    // (unreadable/malformed OBJ) logs via jam::debug::Log and leaves
    // meshShapes empty — graceful last-good, exactly the same "renders
    // without a mesh" outcome buildMeshResources() itself used to produce
    // on a parse failure (jam::VulkanMesh::build() already treats an
    // empty jam::Owner<Shape> as "nothing to upload").
    jam::Owner<jam::WavefrontObj::Shape> meshShapes;

    if (meshPath.isNotEmpty())
    {
        jam::WavefrontObj meshSource;
        juce::StringArray meshWarnings;
        const juce::Result meshParseResult { meshSource.load (juce::File (meshPath), &meshWarnings) };

        for (auto& warning : meshWarnings)
        {
#if JUCE_DEBUG
            jam::debug::Log::write ("jam::VulkanShaderCompiler::compile: " + meshPath + ": " + warning);
#endif
        }

        if (meshParseResult.wasOk())
            meshShapes = std::move (meshSource).extractShapes();
        else
        {
#if JUCE_DEBUG
            jam::debug::Log::write ("jam::VulkanShaderCompiler::compile: failed to load \""
                                    + meshPath + "\" (" + meshParseResult.getErrorMessage()
                                    + ") -- every VulkanShaderInstance renders without a mesh");
#endif
        }
    }

    // RetroArch's own UNSPEC-falls-back-to-global-filter contract
    // (shader_vulkan.cpp:1959, verified this session) — resolved HERE, at
    // compile time, because the render path (jam::VulkanGraphics/
    // VulkanShaderInstance) never sees this compile()'s own global @p filter
    // argument. Every preset pass whose filter_linearN directive was absent
    // (empty optional, VulkanShaderPreset::Pass::filterLinear's own doc comment) is
    // made concrete against the SAME global filter this pass's own
    // VulkanShaderFormat::shadertoy sampler macro already resolves against
    // (channelMacros()/sceneMacro() above) — after this loop, filterLinear is
    // ALWAYS concrete for every pass this preset declares; a global filter
    // config change still recompiles (VulkanShaderPreset::hash() folds filterLinear's
    // now-resolved state, so a different global filter yields a different
    // contentHash). A no-op for VulkanShaderFormat::shadertoy (preset.passes is
    // always empty for it) and for any pass whose filter_linearN was already
    // explicit in the manifest.
    for (auto& pass : preset.passes)
        if (not pass.filterLinear.has_value())
            pass.filterLinear = filter == map::ImageResample::linear;

    std::unique_ptr<jam::VulkanShader> shader;

    if (imageSource.isNotEmpty())
    {
        juce::StringArray bufferPassNames;

        for (int ordinal = 0; ordinal < jam::VulkanShaderUniforms::maxChannelCount; ++ordinal)
        {
            const auto& passName { presetNames.at (ordinal) };

            if (shaderState.getProperty (juce::Identifier (passName)).toString().isNotEmpty())
                bufferPassNames.add (passName);
        }

        // Named-texture channel slots (preset.textures, the project's own
        // .slangp textures= list) are pure declaration order — texture K's
        // own slot is always bufferPassNames.size() + K, computed where
        // consumed (channelMacros() above, VulkanShaderInstance::build()), never a
        // carried field. Order-assigned slots can never collide with each
        // other or with a buffer pass's own ordinal, so only the upper bound
        // needs asserting here, once, before a single pass is compiled
        // against it (mirrors jam::VulkanShader's own ctor pass-count
        // ceiling assert against the SAME maxChannelCount floor,
        // jam_VulkanShader.h).
        jassert (bufferPassNames.size() + preset.textures.size()
                 <= jam::VulkanShaderUniforms::maxChannelCount);

        jam::Owner<jam::VulkanShaderPass> passes;
        bool everyPassCompiled { true };
        const bool isSlang { format == VulkanShaderFormat::slang };

        const auto compilePass = [&] (const juce::String& passName, const juce::String& rawSource)
        {
            // passes.size() BEFORE this call's own passes.add() below IS this
            // pass's own ordinal -- buffer passes 0..bufferPassNames.size()-1
            // as each is compiled in order, then the mandatory Image pass at
            // bufferPassNames.size() -- the SAME ordinal space preset.passes
            // is itself aligned to (jam_VulkanShaderPreset.h).
            const int ordinal { passes.size() };

            const auto stages { isSlang ? splitSlangStages (rawSource) : SlangStages {} };
            const juce::String fragmentSource { isSlang ? stages.fragment : rawSource };

            juce::MemoryBlock vertexSpirv;

            if (isSlang)
            {
                vertexSpirv = compileSpirv (stages.vertex, passName, Stage::vertex, format);
                everyPassCompiled = everyPassCompiled and not vertexSpirv.isEmpty();
            }

            // A .slang pass is compiled completely as-is -- zero wrapper/macro
            // text injection (real .slang shaders declare their own
            // layout(set=0, binding=N)
            // resources, which this compiler's fixed-set-0 bindless macro
            // contract would collide with; jam::VulkanShaderReflection's
            // SPIR-V reflection resolves those bindings downstream instead,
            // never this wrapper). VulkanShaderFormat::shadertoy's own path
            // (assemblePass() below) is reached exactly as before this
            // change -- same call, same arguments, only ever skipped for
            // isSlang == true.
            const juce::String assembledFragmentSource { isSlang
                ? fragmentSource
                : assemblePass (fragmentSource, commonSource, bufferPassNames, preset.textures, filter, isBackground,
                                 format) };

            auto fragmentSpirv { compileSpirv (assembledFragmentSource, passName, Stage::fragment, format) };

            everyPassCompiled = everyPassCompiled and not fragmentSpirv.isEmpty();

            // #pragma parameter declarations are conventionally declared in
            // the shared prelude (before either #pragma stage marker), so
            // parsing rawSource (never the already-split fragmentSource)
            // finds them regardless of which stage's main() actually reads
            // the reflected member. Shadertoy passes have no #pragma
            // parameter convention at all -- empty map.
            auto parameterDefaults { isSlang ? parseParameterDefaults (rawSource)
                                              : jam::HashMap<juce::String, float> {} };

            // The author's preset value outranks the shader file's own
            // #pragma default (RetroArch's own documented override-wins
            // semantics, jam_VulkanShaderPreset.h's own parameterOverrides
            // doc comment) -- only a preset override whose key matches a
            // parameter identifier THIS pass's own #pragma parameter
            // declarations actually reflect is ever looked up; every other
            // preset.parameterOverrides entry is simply never queried,
            // exactly RetroArch's own behavior.
            // Compile-time path (once per shader compile, never per frame) —
            // contains()+.at() over find()+->second per the coding standard;
            // the double lookup costs nothing here.
            for (auto& [parameterName, parameterDefault] : parameterDefaults)
                if (preset.parameterOverrides.contains (parameterName))
                    parameterDefault = preset.parameterOverrides.at (parameterName).getFloatValue();

            // RetroArch's own aliasN-outranks-#pragma-name precedence
            // (slang_process.cpp:941-942) -- only a preset entry that
            // exists for this ordinal AND declares no aliasN of its own is
            // ever overlaid; every other ordinal (every VulkanShaderFormat::
            // shadertoy shader, whose preset is always empty; a slang pass
            // whose preset already carries a real aliasN) is left
            // untouched. Read from rawSource (never the already-split
            // fragmentSource), the same shared-prelude convention
            // parseParameterDefaults() already relies on.
            if (isSlang and ordinal < preset.passes.size()
                and preset.passes.at (ordinal).alias.isEmpty())
            {
                const auto pragmaName { parsePassName (rawSource) };

                if (pragmaName.isNotEmpty())
                    preset.passes.at (ordinal).alias = pragmaName;
            }

            passes.add (std::make_unique<jam::VulkanShaderPass> (
                jam::VulkanShaderPass { passName, std::move (fragmentSpirv), std::move (vertexSpirv),
                                          std::move (parameterDefaults) }));
        };

        for (auto& bufferPassName : bufferPassNames)
            compilePass (bufferPassName, shaderState.getProperty (juce::Identifier (bufferPassName)).toString());

        compilePass (imagePassName, imageSource);

        if (everyPassCompiled)
            shader = std::make_unique<jam::VulkanShader> (std::move (passes), format, std::move (preset), meshPath,
                                                             meshShaderPath, std::move (meshShaderSource),
                                                             std::move (meshShapes));
    }

    return shader;
}

std::unique_ptr<jam::VulkanShader> VulkanShaderCompiler::compile (const juce::String& imageSource,
                                                               bool isBackground,
                                                               int format,
                                                               map::ImageResample::value filter)
{
    juce::ValueTree shaderState { "VulkanShader" };
    shaderState.setProperty (juce::Identifier { VulkanShaderFormat::getPresetNames().at (VulkanShaderFormat::image) },
                             imageSource,
                             nullptr);

    return compile (shaderState, isBackground, format, filter);
}

std::unique_ptr<jam::VulkanShader> VulkanShaderCompiler::compile (const juce::File& shaderDir,
                                                               bool isBackground,
                                                               int format,
                                                               map::ImageResample::value filter)
{
    auto shaderState { VulkanShaderFormat::load (format, Id::shader, shaderDir) };

    return compile (shaderState, isBackground, format, filter);
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam