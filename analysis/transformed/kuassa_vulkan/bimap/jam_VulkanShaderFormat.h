#pragma once

/** @file jam_VulkanShaderFormat.h
 *  @brief Bimap for shader project source format — dispatches format to its
 *         wrapper-template filename, its manifest file extension (.slangp
 *         only), its own .slangp manifest key/scale-type/wrap-mode
 *         vocabulary, and its own pass-name vocabulary — and reads a shader
 *         project directory into a juce::ValueTree for either format.
 *
 *  Moved from shader/ into this module's own scoped bimap/ directory —
 *  jam_terminal/bimap/jam_DecMode.h is
 *  this codebase's module-bimap canon (scoped bimap/ directory,
 *  jam_-prefixed filename, \#pragma once), adopted here; an unprefixed
 *  module-root filename shadows a project's own same-named header through
 *  the include path, so module files always carry the jam_ prefix. Owns the
 *  pre-existing shadertoy/slang wrapper-filename/extension/pass-name
 *  vocabulary and directory-to-ValueTree reader, plus the .slangp manifest
 *  key/scale-type/wrap-mode vocabulary that jam::VulkanShaderPreset::
 *  parse() reads (jam_VulkanShaderPreset.cpp) — every jam::vulkan Bimap
 *  struct colocated here (mirrors an established project-level precedent of
 *  colocating every format Bimap struct in one file). Declares
 *  out-of-line (jam_VulkanShaderFormat.cpp) — see VulkanShaderFormat()'s own doc
 *  comment for why.
 */

namespace jam
{
/*____________________________________________________________________________*/
/**
 * @brief Bimap for shader project source format.
 *
 * Resolves shadertoy to the engine-owned wrapper template's BinaryData
 * filename every Shadertoy pass is assembled through
 * (jam_VulkanShaderCompiler.cpp's shadertoy_wrapper.frag). slang has no
 * map entry (a .slang pass is compiled completely as-is, zero wrapper text
 * — VulkanShaderCompiler::compile() never
 * calls VulkanShaderFormat::get (slang), see its compilePass lambda's own
 * doc comment) — slang still exists as an enumerator (jam::
 * VulkanShader::format's own dispatch value, VulkanShaderInstance::build()'s top-level
 * branch) even though this Bimap's wrapper-filename lookup is N/A for it.
 * Adding a future wrapper-driven format is one new map entry, zero new
 * branching — mirrors map::ImageResample's own map-entry-driven dispatch
 * precedent (lexicon/jam_vulkan.md).
 *
 * Also owns: the manifest file extension a project's own .slangp is
 * discovered through (extension-only, first match, any filename — shared by
 * both format directory readers via findPresetFile(), and by the host
 * application's own content-based format-detection read — a project's format
 * is resolved from the PARSED
 * result, jam::VulkanShaderPreset::parse()'s own passCount, never from
 * this extension's mere presence/absence), each format's own canon pass-name
 * vocabulary (Common/Image/Preset/BufferA.. for shadertoy — the SAME
 * vocabulary both formats write their properties into; the .slangp manifest
 * key vocabulary — "shaders"/"shaderN"/"#"/"=" plus every per-pass scale/
 * filter/wrap/framebuffer-format/frame-count-modulo/alias directive stem,
 * plus mesh (this manifest's own END-extension key) — for slang; Preset
 * holds the RAW .slangp manifest text for either format: the full RetroArch
 * preset for slang, an optional resource-manifest-only .slangp (textures=/
 * mesh=, no shaders=/passes) for shadertoy, empty when a shadertoy project
 * declares no .slangp of its own at all, jam::VulkanShaderPreset::
 * parse()'s own input), RetroArch's own scale_typeN/wrap_modeN VALUE
 * vocabulary (scaleTypes/wrapModes, getScaleType()/getWrapMode() — the
 * DecMode contains-guarded miss-sentinel shape, jam_terminal/bimap/
 * jam_DecMode.h), and the per-format directory-to-ValueTree reader,
 * dispatched directly through jam::Function::Map (no branching — read
 * (format, ...) calls the exact function registered for that format
 * ordinal).
 */
struct VulkanShaderFormat : public jam::Bimap<int>
{
    /** @brief VulkanShader project source format VulkanShaderCompiler::compile() accepts.
     *  Plain enum (not enum class) — matches file::Shaders' own historic
     *  shape exactly (Source/Bimap.h, commit a48718b) — values convert to
     *  int implicitly, no cast needed anywhere this type is used as a map key. */
    enum
    {
        shadertoy,
        slang
    };

    /** @brief Common/Image/Preset canon pass-slot ordinals — declared to
     *  start immediately after the buffer-ordinal range (0..VulkanShaderUniforms::
     *  maxChannelCount - 1), mirroring file::Shaders' own sequential
     *  buffers-then-common-then-image key layout, generalized from a fixed
     *  4 buffers to maxChannelCount. preset holds the RAW .slangp manifest
     *  text (jam::VulkanShaderPreset::parse()'s own input) for either
     *  format: the project's own full preset for VulkanShaderFormat::slang; an
     *  optional resource-manifest-only .slangp (textures=/mesh=, no
     *  shaders=/passes) for VulkanShaderFormat::shadertoy, empty when that project
     *  declares no .slangp of its own at all (this codebase's established
     *  missing-file-reads-empty tolerance). */
    enum
    {
        common = VulkanShaderUniforms::maxChannelCount,
        image,
        preset
    };

    /** @brief .slangp manifest's own key vocabulary — plain key=value
     *  directive lines: the 4 original top-level keys (passCount/
     *  passSourcePrefix/comment/delimiter), every per-pass directive stem
     *  jam::VulkanShaderPreset::parse() reads, textures (the top-level
     *  "textures=" name-list key, now CARRIED onto jam::
     *  VulkanShaderPreset::textures rather than merely recognized-and-discarded —
     *  see that struct's own doc comment), mesh (an END-extension key, NOT
     *  RetroArch vocabulary — every other key in this enum is verbatim
     *  video_shader_parse.c grammar; carried onto jam::
     *  VulkanShaderPreset::meshPath), feedbackPass (still recognized, not itself
     *  carried onto a Pass), and the 3 PER-TEXTURE suffix keys
     *  (textureLinear/textureWrapMode/textureMipmap) each declared textures=
     *  name composes its own \<name\>_linear/\<name\>_wrap_mode/\<name\>_mipmap
     *  directive spelling from (jam::VulkanShaderPreset::parse(): NAME +
     *  getSlangKey (textureLinear/textureWrapMode/textureMipmap), the SAME
     *  stem-then-suffix concatenation idiom the per-pass stem-then-ordinal
     *  reads above already use, just with a NAME prefix instead of an
     *  ordinal one). meshShader mirrors mesh's exact shape one entry
     *  further — the mesh-backed pass's own AUTHOR .slang connection
     *  (jam::VulkanShaderPreset::meshShaderPath). Verified against every other plain enum this struct
     *  declares (shadertoy/slang; common/image/preset; source/viewport/
     *  absolute; clampToBorder/clampToEdge/repeat/mirroredRepeat) — a plain
     *  enum's enumerators share this struct's own scope, and none of these
     *  enumerator names collide across any pair. */
    enum
    {
        passCount,
        passSourcePrefix,
        comment,
        delimiter,
        filterLinear,
        wrapMode,
        frameCountMod,
        srgbFramebuffer,
        floatFramebuffer,
        mipmapInput,
        alias,
        scaleType,
        scaleTypeX,
        scaleTypeY,
        scale,
        scaleX,
        scaleY,
        textures,
        mesh,
        meshShader,
        feedbackPass,
        textureLinear,
        textureWrapMode,
        textureMipmap
    };

    /** @brief RetroArch's own scale_typeN vocabulary ("source"/"viewport"/
     *  "absolute") — resolves a buffer pass's own render-target extent
     *  relative to the PREVIOUS pass's own extent in the chain (source), this
     *  engine's own scaled viewport extent (viewport), or an exact pixel
     *  count (absolute — jam::VulkanShaderPreset::Pass::scaleX/scaleY then
     *  hold that integer pixel count, stored as a float). source is also
     *  this vocabulary's own documented "key absent" default (RetroArch's
     *  own no-scale_type-key pass-through behavior, FBO_SCALE_FLAG_VALID
     *  left unset, encoded as source-typed at 1x scale — an identical
     *  resulting extent, never a distinct "unset" state). */
    enum
    {
        source,
        viewport,
        absolute
    };

    /** @brief RetroArch's own wrap_modeN vocabulary. Absent or unrecognized
     *  resolves to clampToBorder (RetroArch's own documented RARCH_WRAP_BORDER
     *  default). */
    enum
    {
        clampToBorder,
        clampToEdge,
        repeat,
        mirroredRepeat
    };

    /** @brief Populates every map/vocabulary this Bimap owns — out-of-line
     *  (jam_VulkanShaderFormat.cpp) since the slang directory reader lambda
     *  registered here calls jam::VulkanShaderPreset::parse(), and
     *  jam::VulkanShaderPreset is only a complete type once jam_vulkan.h
     *  has included BOTH this header and shader/jam_VulkanShaderPreset.h —
     *  jam_VulkanShaderPreset.h itself must be included AFTER this header
     *  (VulkanShaderPreset::Pass::scaleTypeX/scaleTypeY/wrapMode default-initialize
     *  from this class's own enumerators above), so this constructor's own
     *  body cannot be defined inline in this header the way every other
     *  Bimap in this codebase is (mirrors jam::VulkanShaderPreset/
     *  VulkanShaderReflection/VulkanShaderCompiler's own established header-declares/
     *  .cpp-implements split, jam_vulkan/shader/ — the ONE other Bimap in
     *  this codebase needing it, for this exact forward-reference reason). */
    VulkanShaderFormat();

    static VulkanShaderFormat* getInstance() noexcept
    {
        return jam::SharedInstance<VulkanShaderFormat>::getInstance();
    }

    /** @brief Returns the wrapper BinaryData filename for a format. */
    static const juce::String& get (int format) noexcept { return getInstance()->jam::Bimap<int>::get (format); }

    /** @brief Returns the format -> manifest-extension map (only slang has
     *  an entry, the SAME .slangp wildcard every project's manifest is
     *  discovered through regardless of its own eventual format) —
     *  Config::Shader::loadFromPath() walks this directly to locate a
     *  project's own .slangp before parsing it (jam::
     *  VulkanShaderPreset::parse()) to content-derive that project's own format,
     *  scales to a future format by adding one more entry here, nothing
     *  else. */
    static const auto& getExtension() noexcept { return getInstance()->extension; }

    /** @brief Returns the canon pass-name vocabulary (Common/Image/BufferX)
     *  every shader project — either format — writes its shaderState
     *  properties into. jam::VulkanShaderCompiler::compile() reads shader
     *  passes by these SAME names (its own Common/Image/buffer-ordinal
     *  discovery loop) — the single source of truth for that vocabulary,
     *  never a duplicated name list at any call site. */
    static const auto& getPresetNames() noexcept { return getInstance()->presets.at (shadertoy); }

    /** @brief Returns one RetroArch .slangp manifest key's own spelling
     *  (e.g. getSlangKey (scaleType) -> "scale_type") — jam::
     *  VulkanShaderPreset::parse()'s own key vocabulary, composed with a per-pass
     *  ordinal suffix at that call site (getSlangKey (scaleType) +
     *  juce::String (ordinal)).
     *  @param key  One of this struct's own key-vocabulary enumerators
     *              (passCount..feedbackPass above).
     */
    static const juce::String& getSlangKey (int key) noexcept { return getInstance()->presets.at (slang).at (key); }

    /** @brief Returns the whole RetroArch .slangp manifest key vocabulary —
     *  jam::VulkanShaderPreset::parse()'s own parameterOverrides sweep
     *  consults this directly (jam::Map::containsValue (getSlangKeys(),
     *  stem)) to test whether a raw manifest key stem is part of this
     *  registry's own recognized vocabulary, rather than re-declaring a
     *  second, locally duplicated key-stem list. */
    static const auto& getSlangKeys() noexcept { return getInstance()->presets.at (slang); }

    /** @brief Resolves a raw scale_typeN/scale_type_xN/scale_type_yN string
     *  value to its scaleTypes ordinal — DecMode's own contains-guarded
     *  miss-sentinel shape (jam_terminal/bimap/jam_DecMode.h): an empty or
     *  unrecognized @p value resolves to source, this vocabulary's own
     *  documented "key absent" default, branchless (no separate presence
     *  check needed at any call site). scaleTypes stores string -> ordinal
     *  directly (the only direction this vocabulary is ever read), so this
     *  is a direct lookup, never a per-call inversion.
     *  @param value  Raw scale_typeN/scale_type_xN/scale_type_yN string, or empty.
     */
    static int getScaleType (const juce::String& value) noexcept
    {
        const auto& scaleTypes { getInstance()->scaleTypes };
        return scaleTypes.contains (value) ? scaleTypes.at (value) : source;
    }

    /** @brief Resolves a raw wrap_modeN string value to its wrapModes
     *  ordinal — same DecMode contains-guarded shape as getScaleType()
     *  above: an empty or unrecognized @p value resolves to clampToBorder,
     *  RetroArch's own documented RARCH_WRAP_BORDER default, branchless.
     *  wrapModes stores string -> ordinal directly (the only direction this
     *  vocabulary is ever read), so this is a direct lookup, never a
     *  per-call inversion.
     *  @param value  Raw wrap_modeN string, or empty.
     */
    static int getWrapMode (const juce::String& value) noexcept
    {
        const auto& wrapModes { getInstance()->wrapModes };
        return wrapModes.contains (value) ? wrapModes.at (value) : clampToBorder;
    }

    /** @brief Reads a shader project directory (shadertoy) or its .slangp
     *  manifest's own directory (slang) into a rootTag-typed juce::ValueTree,
     *  keyed by this class's own canon pass-name vocabulary either way —
     *  direct jam::Function::Map dispatch, no branching on format.
     *  @param format   VulkanShaderFormat::shadertoy or VulkanShaderFormat::slang.
     *  @param rootTag  Type identifier for the returned tree (the caller's
     *                  own state.getType(), background or postProcessing).
     *  @param dir      The shader project directory (file::Shaders::getPath()'s
     *                  result) — shadertoy reads fixed Common/Image/BufferX
     *                  .frag files directly from it, plus its own optional
     *                  resource-manifest .slangp (textures=/mesh=, no
     *                  shaders=/passes) via findPresetFile() when the
     *                  directory declares one; slang locates its own full
     *                  .slangp preset inside it the SAME way.
     */
    static juce::ValueTree load (int format, const juce::Identifier& rootTag, const juce::File& dir)
    {
        return getInstance()->parser.get (format, rootTag, dir);
    }

    /** @brief Textually expands `#include "relative/path"` directives inside a
     *  .slang source file, mirroring RetroArch's own frontend-side preprocessing
     *  (RetroArch gfx/drivers_shader/slang_preprocess.cpp's slang_preprocess) --
     *  glslang/shaderc never perform file I/O of their own, so any \#include
     *  reaching shaderc's raw-string compile call fails outright; expansion has
     *  to happen here, at load time, where the including file's own path is
     *  known. Real-world case: ~/.config/end/shaders/slang-pp-kawase5/shaders/
     *  kawase/kawase0.slang line 53 includes "../../../include/colorspace-tools.h"
     *  -- resolved relative to kawase0.slang's own parent directory, never the
     *  top-level preset directory. Public (not private -- moved out of this
     *  class's own private section) — this class's own slang directory reader
     *  lambda (constructor below) is one consumer; jam::
     *  VulkanShaderCompiler::compile()'s own mesh_shader= (meshShaderPath) load is
     *  the second, cross-class consumer (mesh_shader is authored EXACTLY like
     *  any other .slang pass — SSOT — so it needs this SAME
     *  \#include expansion, not a second, duplicated one).
     *  @param file  The .slang (or included) file to expand.
     *  @return      file's full text, every \#include "..." line replaced by the
     *               RECURSIVE expansion of that quoted path resolved against
     *               file's own parent directory; all other lines pass through
     *               verbatim. A missing include target logs the referencing
     *               file + unresolved path via jam::debug::Log and splices a
     *               GLSL \#error line naming that path, so glslang's own compile
     *               error points at the actual missing file instead of the
     *               cascading type/overload errors an empty splice produces
     *               downstream -- last-good-shader contract preserved (compile
     *               fails cleanly, previous shader retained), never a silent
     *               empty-string swallow.
     */
    static juce::String expandIncludes (const juce::File& file);

private:
    /** @brief Generates the Nth buffer-pass canon name via bijective base-26
     *  (spreadsheet-column) numbering: ordinal 0 -> "BufferA", 25 -> "BufferZ",
     *  26 -> "BufferAA", unbounded. Historic Shadertoy convention only ever names
     *  BufferA-D; this generalizes it to as many ordinals as
     *  VulkanShaderUniforms::maxChannelCount allows, for RetroArch .slangp presets that
     *  declare more than 4 buffer passes. Private member function (moved from a
     *  file-scope static) — its
     *  only consumer is this class's own constructor.
     *  @param ordinal  Zero-based buffer-pass ordinal.
     *  @return         "Buffer" followed by the ordinal's base-26 letter(s).
     */
    static juce::String toBufferName (int ordinal) noexcept;

    /** @brief Locates a shader project directory's own .slangp manifest —
     *  first match, any filename, extension-only discovery (extension's own
     *  slang-keyed wildcard, the SAME entry both format directory readers
     *  below resolve through). Shared by both: the slang reader's own
     *  manifest IS that project's full preset; the shadertoy reader's own
     *  manifest (when present at all) is a host-application extension resource
     *  manifest only (textures=/mesh=, no shaders=/passes — jam::
     *  VulkanShaderPreset::parse() already yields zero passes gracefully for it,
     *  the content-based format-detection contract the host application's
     *  own shader loader relies on). Private member
     *  function (moved out of being duplicated inline in both parser
     *  lambdas) — its only consumers are this class's own shadertoy and
     *  slang parser lambdas, registered in the constructor below.
     *  @param dir  The shader project directory.
     *  @return     The first .slangp match, or juce::File() when the
     *              directory declares none (this codebase's established
     *              missing-file tolerance).
     */
    static juce::File findPresetFile (const juce::File& dir) noexcept;

    /** @brief Builds the complete format -> canon pass-name/manifest-key
     *  vocabulary (presets, below) — shadertoy's own Common/Image/preset
     *  slots plus its base-26 buffer-name run (toBufferName() above), and
     *  slang's own full .slangp manifest key vocabulary. A work buffer
     *  (this function's own local shadertoy name map) assembles the
     *  buffer-name run before the complete result returns — construction,
     *  never a post-construction mutation of the presets member itself.
     *  Private member function — its only consumer is this class's own
     *  constructor's own member-init list.
     *  @return  The complete format -> pass-name/manifest-key vocabulary.
     */
    static jam::HashMap<int, jam::HashMap<int, juce::String>> buildPresets() noexcept;

    /** @brief Format -> manifest file extension/wildcard — the SAME .slangp
     *  wildcard findPresetFile() resolves through for either format; a
     *  project's own format is content-derived from the parsed result
     *  (jam::VulkanShaderPreset::parse()'s own passCount), never from this
     *  extension's mere presence/absence. */
    jam::HashMap<int, juce::String> extension;

    /** @brief Format -> that format's own canon pass-name vocabulary. Both
     *  formats' read() implementations write their resulting ValueTree
     *  properties using presets.at (shadertoy)'s names (Common/Image/BufferX)
     *  — slang's own presets.at (slang) is manifest key vocabulary only,
     *  never a property-name source. */
    jam::HashMap<int, jam::HashMap<int, juce::String>> presets;

    /** @brief RetroArch scale_typeN spelling ("source"/"viewport"/
     *  "absolute") -> its own scaleTypes ordinal — getScaleType()'s own
     *  direct lookup source, string -> ordinal, the only direction this
     *  vocabulary is ever read. */
    jam::HashMap<juce::String, int> scaleTypes;

    /** @brief RetroArch wrap_modeN spelling -> its own wrapModes ordinal —
     *  getWrapMode()'s own direct lookup source, string -> ordinal, the
     *  only direction this vocabulary is ever read. */
    jam::HashMap<juce::String, int> wrapModes;

    /** @brief Format -> its own directory-to-ValueTree reader, direct dispatch. */
    jam::Function::Map<int, juce::ValueTree> parser;
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam