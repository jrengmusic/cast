namespace jam
{
/*____________________________________________________________________________*/
juce::String VulkanShaderFormat::toBufferName (int ordinal) noexcept
{
    juce::String letters;
    int n { ordinal };

    do
    {
        letters =
            juce::String::charToString (static_cast<juce::juce_wchar> ('A' + (n % 26))) + letters;
        n = n / 26 - 1;
    } while (n >= 0);

    return Id::buffer.toString() + letters;
}

juce::String VulkanShaderFormat::expandIncludes (const juce::File& file)
{
    juce::StringArray expandedLines;

    for (const auto& rawLine : juce::StringArray::fromLines (file.loadFileAsString()))
    {
        const auto line { rawLine.trim() };

        if (line.startsWith (Id::includeDirective))
        {
            const auto includeTarget { file.getParentDirectory().getChildFile (
                line.fromFirstOccurrenceOf ("\"", false, false)
                    .upToLastOccurrenceOf ("\"", false, false)) };

            if (includeTarget.existsAsFile())
                expandedLines.add (expandIncludes (includeTarget));
            else
            {
#if JUCE_DEBUG
                jam::debug::Log::write ("jam::VulkanShaderFormat::expandIncludes: \""
                                        + file.getFullPathName() + "\" references missing include \""
                                        + includeTarget.getFullPathName() + "\"");
#endif
                expandedLines.add (Id::errorDirective.toString() + " missing include: " + includeTarget.getFullPathName());
            }
        }
        else
            expandedLines.add (rawLine);
    }

    return expandedLines.joinIntoString ("\n");
}

/*____________________________________________________________________________*/
juce::File VulkanShaderFormat::findPresetFile (const juce::File& dir) noexcept
{
    const auto presetFiles { dir.findChildFiles (
        juce::File::findFiles, false, getInstance()->extension.at (slang)) };
    return not presetFiles.isEmpty() ? presetFiles.getReference (0) : juce::File();
}

/*____________________________________________________________________________*/
jam::HashMap<int, jam::HashMap<int, juce::String>> VulkanShaderFormat::buildPresets() noexcept
{
    jam::HashMap<int, juce::String> shadertoyNames {
        { common, jam::Format::upperFirstChar (Id::common) },
        { image, jam::Format::upperFirstChar (Id::image) },
        { preset, jam::Format::upperFirstChar (Id::preset) },
    };

    for (int ordinal = 0; ordinal < VulkanShaderUniforms::maxChannelCount; ++ordinal)
        shadertoyNames.emplace (ordinal, toBufferName (ordinal));

    return {
        { shadertoy, std::move (shadertoyNames) },

        // RetroArch .slangp manifest's own key vocabulary (lexicon/jam_vulkan.md's
        // own preset category) — passCount/
        // passSourcePrefix/comment/delimiter are the 4 original top-level keys
        // this Bimap always carried; every entry below is the per-pass directive
        // stem/non-indexed key jam::VulkanShaderPreset::parse() now reads
        // exclusively through getSlangKey(), never a locally duplicated string
        // literal.
        { slang, {
            { passCount, Id::shaders.toString() },
            { passSourcePrefix, Id::shader.toString() },
            { comment, juce::String::charToString (Chars::hash) },
            { delimiter, juce::String::charToString (Chars::equals) },
            { filterLinear, Id::filterLinear.toString() },
            { wrapMode, Id::wrapMode.toString() },
            { frameCountMod, Id::frameCountMod.toString() },
            { srgbFramebuffer, Id::srgbFramebuffer.toString() },
            { floatFramebuffer, Id::floatFramebuffer.toString() },
            { mipmapInput, Id::mipmapInput.toString() },
            { alias, Id::alias.toString() },
            { scaleType, Id::scaleType.toString() },
            { scaleTypeX, Id::scaleTypeX.toString() },
            { scaleTypeY, Id::scaleTypeY.toString() },
            { scale, Id::scale.toString() },
            { scaleX, Id::scaleX.toString() },
            { scaleY, Id::scaleY.toString() },
            { textures, Id::textures.toString() },
            { mesh, Id::mesh.toString() },
            { meshShader, Id::meshShader.toString() },
            { feedbackPass, Id::feedbackPass.toString() },

            // Per-texture suffix keys jam::VulkanShaderPreset::parse() composes
            // a NAME onto (name + getSlangKey (textureLinear/textureWrapMode/
            // textureMipmap) -> "<name>_linear"/"<name>_wrap_mode"/"<name>_mipmap")
            // -- a DIFFERENT composition idiom from every per-pass stem above
            // (stem + ordinal, no joiner needed since a digit already reads
            // unambiguously after a stem); a NAME prefix needs the underscore
            // joiner RetroArch's own <name>_linear spelling carries, so each
            // entry here is PRE-COMPOSED via jam::Format::appendWithUnderscore
            // (jam_core/format/jam_Format.h's own OOTB joiner -- inventoried before
            // reaching for a hand-rolled "_" + stem concatenation) with an empty
            // leading text, yielding just the suffix itself ("_linear" etc.);
            // the parse-time NAME + suffix concatenation then mirrors the
            // existing stem + ordinalText idiom exactly. linear/wrapMode/mipmap
            // are Id::linear (lexicon/jam_vulkan.md's own resample category,
            // reused verbatim), Id::wrapMode (reused unchanged from
            // this same block's own per-pass wrap_modeN stem), and Id::mipmap
            // (lexicon/jam_vulkan.md's own preset category).
            { textureLinear, jam::Format::appendWithUnderscore ({}, Id::linear) },
            { textureWrapMode, jam::Format::appendWithUnderscore ({}, Id::wrapMode) },
            { textureMipmap, jam::Format::appendWithUnderscore ({}, Id::mipmap) },
        } },
    };
}

/*____________________________________________________________________________*/
VulkanShaderFormat::VulkanShaderFormat()
    : jam::Bimap<int> ({ { shadertoy, files::shadertoyWrapper } }),
      extension { { slang, jam::Format::toFileExtension (Id::slangp) } },
      presets { buildPresets() },

      // RetroArch's own scale_typeN VALUE vocabulary, string -> ordinal —
      // source is also this vocabulary's own "key absent" default
      // (getScaleType()'s own doc comment); Id::source is lexicon/
      // jam_vulkan.md's own EXISTING spelling, reused unchanged rather than
      // re-declared as a second, duplicated Id:: member.
      scaleTypes {
          { Id::source.toString(),   source   },
          { Id::viewport.toString(), viewport },
          { Id::absolute.toString(), absolute },
      },

      // RetroArch's own wrap_modeN VALUE vocabulary, string -> ordinal.
      wrapModes {
          { Id::clampToBorder.toString(),  clampToBorder  },
          { Id::clampToEdge.toString(),    clampToEdge    },
          { Id::repeat.toString(),         repeat         },
          { Id::mirroredRepeat.toString(), mirroredRepeat },
      }
{
    parser.add<const juce::Identifier&, const juce::File&> (
        shadertoy,
        [this] (const juce::Identifier& rootTag, const juce::File& dir)
        {
            const auto& names { presets.at (shadertoy) };

            // A shadertoy project may carry its own resource-manifest-only
            // .slangp (textures=/mesh=, no shaders=/passes — the SAME
            // findPresetFile() discovery the slang reader below uses) —
            // its raw text lands on this SAME preset canon slot VulkanShaderCompiler::
            // compile() already parses for every format, so textures/mesh then
            // flow through with zero further plumbing; absent is empty (this
            // codebase's established missing-file tolerance).
            const auto presetFile { findPresetFile (dir) };

            return jam::Model::fromFiles (
                rootTag,
                names,
                [&names, dir, presetFile] (int key) -> juce::String
                {
                    if (key == preset)
                        return presetFile.loadFileAsString();

                    return dir.getChildFile (names.at (key)).loadFileAsString();
                });
        });

    parser.add<const juce::Identifier&, const juce::File&> (
        slang,
        [this] (const juce::Identifier& rootTag, const juce::File& dir)
        {
            // findPresetFile() can legitimately return juce::File() here -- the
            // .slangp Config::Shader::loadFromPath() detected can vanish between
            // that detection and this read (a hot-reload transient, not
            // programmer error) -- resolved the SAME graceful last-good way
            // hasPasses below resolves a missing/zero shaders= key: presetFile
            // stays empty, so loadFileAsString()/parse() below produce the same
            // all-slots-empty tree the empty-preset path already produces,
            // never a std::out_of_range/UB read into an empty array.
            const auto presetFile { findPresetFile (dir) };

            // Single lex — jam::VulkanShaderPreset::parse() already performs
            // the exact trim/#-comment/=-delimiter scan this reader needs (via
            // getSlangKey (comment)/getSlangKey (delimiter), the SAME registry
            // entries this class owns); a second, independent StringPairArray
            // scan duplicating that same parse is never performed here anymore.
            // parsedPreset (not "preset" — this class's own preset canon-slot
            // enumerator would otherwise be shadowed by a same-named local).
            const auto parsedPreset { VulkanShaderPreset::parse (presetFile.loadFileAsString()) };
            const int lastPass { parsedPreset.passes.size() - 1 };

            // Graceful-degradation contract (this loader's own, shared with
            // the shadertoy reader above): a transient hot-reload save can
            // yield a .slangp with a missing/zero shaders= key, parsing to a
            // ZERO-pass VulkanShaderPreset. hasPasses folds that shape into the
            // existing isImageSlot/pass-ordinal condition below rather than a
            // separate bail-out guard, so every slot falls through to the
            // same empty-string branch the old code produced — last-good
            // shader retained by compile(), never a std::out_of_range throw.
            const bool hasPasses { not parsedPreset.passes.isEmpty() };

            // Same jam::Model::fromFiles() call shadertoy uses above -- every
            // canon slot always gets a property, real content for this
            // preset's own passes, empty string for every unused slot, the
            // one working method, not a second one.
            return jam::Model::fromFiles (
                rootTag,
                presets.at (shadertoy),
                [this, presetFile, &parsedPreset, lastPass, hasPasses] (int key) -> juce::String
                {
                    // The RAW .slangp manifest text itself (jam::
                    // VulkanShaderFormat::preset's own canon slot) — checked before
                    // the buffer/Image dispatch below, since preset's own
                    // ordinal value (declared past every buffer ordinal
                    // and image/common, this class's own canon-slot enum)
                    // would otherwise fall through to that dispatch's own
                    // "no source at this ordinal" empty-string branch.
                    if (key == preset)
                        return presetFile.loadFileAsString();

                    const bool isImageSlot { key == image };
                    const int pass { isImageSlot ? lastPass : key };

                    if (hasPasses and (isImageSlot or pass < lastPass))
                    {
                        // sourcePath — jam::VulkanShaderPreset::Pass's own
                        // per-pass shaderN directive value, already resolved by
                        // parse() through getSlangKey (passSourcePrefix); relative
                        // to presetFile's own parent directory, includes expanded
                        // exactly as before.
                        const auto& sourcePath {
                            parsedPreset.passes.at (pass).sourcePath
                        };
                        return expandIncludes (
                            presetFile.getParentDirectory().getChildFile (sourcePath));
                    }

                    return juce::String();
                });
        });
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam