namespace jam
{
/*____________________________________________________________________________*/
// RetroArch's own config_get_bool() accepts both "true" and "1" for a bool-
// valued key (real-world presets use both spellings) — named once here,
// never a repeated two-literal equalsIgnoreCase pair at each of this file's
// four bool-valued preset directive reads below.
static bool parsePresetBool (const juce::String& value) noexcept
{
    return value.equalsIgnoreCase (jam::Format::fromBoolean (true)) or value.getIntValue() == 1;
}

/*____________________________________________________________________________*/
VulkanShaderPreset VulkanShaderPreset::parse (const juce::String& presetText)
{
    // Same trim/#-comment/=-delimiter scan VulkanShaderFormat's own slang reader
    // lambda used to duplicate independently (jam_vulkan/bimap/jam_VulkanShaderFormat.h)
    // — that reader now calls THIS parse() exclusively instead, one lex, never two.
    // Every manifest key/delimiter/comment spelling read through
    // VulkanShaderFormat::getSlangKey() — the registry (lexicon/jam_vulkan.md's
    // own preset category) is the one
    // source of truth, never a locally duplicated string literal.
    juce::StringPairArray presetValues;
    const auto& commentPrefix { VulkanShaderFormat::getSlangKey (VulkanShaderFormat::comment) };
    const auto& delimiter { VulkanShaderFormat::getSlangKey (VulkanShaderFormat::delimiter) };

    for (const auto& rawLine : juce::StringArray::fromLines (presetText))
    {
        const auto line { rawLine.trim() };

        if (line.isNotEmpty() and not line.startsWith (commentPrefix))
            presetValues.set (line.upToFirstOccurrenceOf (delimiter, false, false).trim(),
                              line.fromFirstOccurrenceOf (delimiter, false, false).trim());
    }

    VulkanShaderPreset preset;
    const int passCount { presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::passCount), {}).getIntValue() };

    for (int ordinal = 0; ordinal < passCount; ++ordinal)
    {
        const juce::String ordinalText { juce::String (ordinal) };
        Pass pass;

        // scale_typeN wins over the per-axis scale_type_x/yN keys when
        // present (RetroArch's own documented precedence, verified this
        // session) — nested getValue fallback replaces the two-level
        // precedence if/else; VulkanShaderFormat::getScaleType() resolves an
        // empty/unrecognized result to its own documented source default,
        // branchless (jam_vulkan/bimap/jam_VulkanShaderFormat.h's own getScaleType() doc comment).
        pass.scaleTypeX = VulkanShaderFormat::getScaleType (
            presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::scaleType) + ordinalText,
                presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::scaleTypeX) + ordinalText, {})));

        pass.scaleTypeY = VulkanShaderFormat::getScaleType (
            presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::scaleType) + ordinalText,
                presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::scaleTypeY) + ordinalText, {})));

        // scaleN wins over the per-axis scale_x/yN keys when present — same
        // precedence as scale_typeN above. Guarded (unlike scale_typeN/
        // wrapMode above/below) — an absent scaleN AND scale_xN/scale_yN
        // resolves to Pass::scaleX/scaleY's own documented 1.0f default;
        // juce::String{}.getFloatValue() is 0.0f, not 1.0f, so an
        // unconditional assignment from the nested getValue result alone
        // would silently zero-size an unscoped pass's own render target —
        // this guard is the one deviation from a fully branchless read.
        if (const auto scaleXValue { presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::scale) + ordinalText,
                presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::scaleX) + ordinalText, {})) };
            scaleXValue.isNotEmpty())
            pass.scaleX = scaleXValue.getFloatValue();

        if (const auto scaleYValue { presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::scale) + ordinalText,
                presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::scaleY) + ordinalText, {})) };
            scaleYValue.isNotEmpty())
            pass.scaleY = scaleYValue.getFloatValue();

        if (const auto filterLinearValue { presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::filterLinear) + ordinalText, {}) };
            filterLinearValue.isNotEmpty())
            pass.filterLinear = parsePresetBool (filterLinearValue);

        pass.wrapMode = VulkanShaderFormat::getWrapMode (
            presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::wrapMode) + ordinalText, {}));

        // FBO_SCALE_FLAG_VALID (RetroArch shader_vulkan.cpp:1984 +
        // video_shader_parse.c:761-764, verified this session) — RetroArch
        // only honors srgb_framebufferN/float_framebufferN when this SAME
        // ordinal also carries a scale_typeN/scale_type_xN/scale_type_yN
        // directive of its own; either format flag with no scale_type key at
        // all is a silent no-op in the reference. This struct carries no
        // separate validity bit (Pass::srgbFramebuffer/floatFramebuffer are
        // plain bools), so that gate is encoded HERE, at parse time, by
        // simply never reading the two format keys at all for an ordinal
        // that declares no scale_type directive — Pass::srgbFramebuffer/
        // floatFramebuffer then stay at their own default-constructed false,
        // identical to RetroArch's own unhonored-flag behavior. containsKey
        // is the OOTB presence check (RetroArch's own gate reads presence,
        // never a resolved value).
        const bool hasScaleTypeDirective {
            presetValues.containsKey (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::scaleType) + ordinalText)
            or presetValues.containsKey (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::scaleTypeX) + ordinalText)
            or presetValues.containsKey (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::scaleTypeY) + ordinalText)
        };

        if (hasScaleTypeDirective)
        {
            pass.srgbFramebuffer = parsePresetBool (
                presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::srgbFramebuffer) + ordinalText, {}));
            pass.floatFramebuffer = parsePresetBool (
                presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::floatFramebuffer) + ordinalText, {}));
        }

        // Bool/uint/string fields read unconditionally — an absent key's
        // getValue (key, {}) read returns an empty juce::String, which
        // parsePresetBool() is false and getIntValue() is 0 for, so each
        // already lands on this Pass field's own documented "absent" default
        // with no separate presence branch needed. No FBO_SCALE_FLAG_VALID-
        // style gate applies to any of these four.
        pass.mipmapInput = parsePresetBool (
            presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::mipmapInput) + ordinalText, {}));
        pass.frameCountMod = static_cast<uint32_t> (
            presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::frameCountMod) + ordinalText, {}).getIntValue());
        pass.alias = presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::alias) + ordinalText, {});
        pass.sourcePath = presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::passSourcePrefix) + ordinalText, {});

        preset.passes.add (pass);
    }

    // textures= — RetroArch's own semicolon-delimited external-texture name
    // list (gfx/video_shader_parse.c, textures="a;b" convention, verified
    // this session). Each declared name then reads its own <name>/
    // <name>_linear/<name>_wrap_mode/<name>_mipmap directive quartet — the
    // SAME per-field vocabulary/default shape Pass::filterLinear/wrapMode/
    // mipmapInput already established above, just keyed by NAME
    // (name + getSlangKey (textureLinear/textureWrapMode/textureMipmap))
    // rather than by ordinal (getSlangKey (...) + ordinalText).
    //
    // Every manifest key this loop consumes for a given name — the name
    // itself plus its own _linear/_wrap_mode/_mipmap suffix trio — is
    // recorded into textureKeyOrdinals (keyed to that texture's own ordinal
    // in preset.textures), the parameterOverrides sweep below's own
    // per-texture mirror of consumedAsIndexedKey's per-pass stem+ordinal
    // check, so no textures= entry is ever double-carried forward as a
    // bogus top-level parameter override.
    // RetroArch's own textures="name1;name2" list-separator
    // (gfx/video_shader_parse.c) — Chars::semicolon, the same
    // Chars-constant home hash/equals already use for this grammar.
    const auto textureNames { juce::StringArray::fromTokens (
        presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::textures), {}),
        juce::String::charToString (Chars::semicolon), {}) };
    jam::HashMap<juce::String, int> textureKeyOrdinals;

    for (int ordinal = 0; ordinal < textureNames.size(); ++ordinal)
    {
        const auto name { textureNames.getReference (ordinal).trim() };

        if (name.isNotEmpty())
        {
            const auto linearKey { name + VulkanShaderFormat::getSlangKey (VulkanShaderFormat::textureLinear) };
            const auto wrapModeKey { name + VulkanShaderFormat::getSlangKey (VulkanShaderFormat::textureWrapMode) };
            const auto mipmapKey { name + VulkanShaderFormat::getSlangKey (VulkanShaderFormat::textureMipmap) };

            textureKeyOrdinals.emplace (name, ordinal);
            textureKeyOrdinals.emplace (linearKey, ordinal);
            textureKeyOrdinals.emplace (wrapModeKey, ordinal);
            textureKeyOrdinals.emplace (mipmapKey, ordinal);

            // Graceful last-good tolerance (this struct's own established
            // shape, mirrors an empty presetText's own zero-passes default):
            // a declared name whose own <name> key is absent or empty never
            // reaches preset.textures, even though its manifest keys are
            // still excluded from parameterOverrides above — RetroArch
            // itself never treats a textures= name as a generic parameter
            // identifier, resolved or not.
            if (const auto path { presetValues.getValue (name, {}) };
                path.isNotEmpty())
            {
                Texture texture;
                texture.name = name;
                texture.path = path;

                if (const auto linearValue { presetValues.getValue (linearKey, {}) };
                    linearValue.isNotEmpty())
                    texture.filterLinear = parsePresetBool (linearValue);

                texture.wrapMode = VulkanShaderFormat::getWrapMode (presetValues.getValue (wrapModeKey, {}));
                texture.mipmapInput = parsePresetBool (presetValues.getValue (mipmapKey, {}));

                preset.textures.add (texture);
            }
        }
    }

    // mesh= — an END-extension manifest key (not RetroArch vocabulary,
    // lexicon/jam_vulkan.md's own preset category doc), carried in the SAME .slangp manifest as every directive
    // above, for either format. As-written, relative to the owning .slangp
    // preset's own parent directory — absolutized by jam::
    // VulkanShaderCompiler::compile() into jam::VulkanShader::meshPath.
    preset.meshPath = presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::mesh), {});

    // mesh_shader= — mirrors mesh='s exact read one entry further (this
    // struct's own meshShaderPath doc comment) — the mesh-backed pass's own
    // AUTHOR .slang connection, absolutized by jam::VulkanShaderCompiler::
    // compile() into jam::VulkanShader::meshShaderPath the SAME way.
    preset.meshShaderPath = presetValues.getValue (VulkanShaderFormat::getSlangKey (VulkanShaderFormat::meshShader), {});

    // Every top-level key this parse's own field reads above did not consume
    // becomes a parameterOverrides candidate — an indexed per-pass key
    // (stem + in-range ordinal) present in VulkanShaderFormat's own slang key
    // registry (jam::Map::containsValue (VulkanShaderFormat::getSlangKeys(),
    // stem)), one of the fixed non-indexed keys (passCount/textures/mesh/
    // feedbackPass), or one of textureKeyOrdinals's own per-name keys built
    // above, is consumed; everything else carries forward verbatim. No local
    // HashSet — the registry (and, for textures, textureKeyOrdinals itself)
    // is the one source of truth for which keys this parse recognizes.
    for (const auto& key : presetValues.getAllKeys())
    {
        int stemLength { key.length() };
        while (stemLength > 0 and Chars::isNumeric (key[stemLength - 1]))
            --stemLength;

        const auto stem { key.substring (0, stemLength) };
        const auto ordinalText { key.substring (stem.length()) };

        const bool consumedAsIndexedKey { ordinalText.isNotEmpty()
                                          and ordinalText.getIntValue() < passCount
                                          and jam::Map::containsValue (VulkanShaderFormat::getSlangKeys(), stem) };
        const bool consumedAsFixedKey { key == VulkanShaderFormat::getSlangKey (VulkanShaderFormat::passCount)
                                        or key == VulkanShaderFormat::getSlangKey (VulkanShaderFormat::textures)
                                        or key == VulkanShaderFormat::getSlangKey (VulkanShaderFormat::mesh)
                                        or key == VulkanShaderFormat::getSlangKey (VulkanShaderFormat::meshShader)
                                        or key == VulkanShaderFormat::getSlangKey (VulkanShaderFormat::feedbackPass) };
        const bool consumedAsTextureKey { jam::Map::contains (textureKeyOrdinals, key) };

        if (not consumedAsIndexedKey and not consumedAsFixedKey and not consumedAsTextureKey)
            preset.parameterOverrides.emplace (key, presetValues.getValue (key, {}));
    }

    return preset;
}

/*____________________________________________________________________________*/
uint64_t VulkanShaderPreset::hash() const noexcept
{
    // jam::Wyhash::fold, never mix, as the chained accumulator — mix()'s MUM
    // multiply absorbs zero (a zero seed, or any zero part such as an empty
    // parameterOverrides accumulator, would zero the whole chain and every
    // preset would hash identically — jam::Wyhash::fold's own doc comment,
    // jam_lexicon/utils/jam_HashMap.h).
    uint64_t combined { 0 };

    for (const auto& pass : passes)
    {
        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashInt (static_cast<uint64_t> (pass.scaleTypeX)));
        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashInt (static_cast<uint64_t> (pass.scaleTypeY)));

        uint32_t scaleXBits { 0 };
        std::memcpy (&scaleXBits, &pass.scaleX, sizeof (scaleXBits));
        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashInt (scaleXBits));

        uint32_t scaleYBits { 0 };
        std::memcpy (&scaleYBits, &pass.scaleY, sizeof (scaleYBits));
        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashInt (scaleYBits));

        const uint64_t filterLinearState { pass.filterLinear.has_value()
            ? (pass.filterLinear.value() ? 2ULL : 1ULL)
            : 0ULL };
        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashInt (filterLinearState));

        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashInt (static_cast<uint64_t> (pass.wrapMode)));
        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashInt (pass.srgbFramebuffer ? 1ULL : 0ULL));
        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashInt (pass.floatFramebuffer ? 1ULL : 0ULL));
        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashInt (pass.mipmapInput ? 1ULL : 0ULL));
        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashInt (pass.frameCountMod));
        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashBytes (
            pass.alias.toRawUTF8(), pass.alias.getNumBytesAsUTF8()));
        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashBytes (
            pass.sourcePath.toRawUTF8(), pass.sourcePath.getNumBytesAsUTF8()));
    }

    // textures folded the SAME ordered way passes are, in textures=
    // declaration order — never order-independently, since that order is
    // itself part of this struct's own identity (textures's own doc
    // comment, jam_VulkanShaderPreset.h).
    for (const auto& texture : textures)
    {
        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashBytes (
            texture.name.toRawUTF8(), texture.name.getNumBytesAsUTF8()));
        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashBytes (
            texture.path.toRawUTF8(), texture.path.getNumBytesAsUTF8()));

        const uint64_t textureFilterLinearState { texture.filterLinear.has_value()
            ? (texture.filterLinear.value() ? 2ULL : 1ULL)
            : 0ULL };
        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashInt (textureFilterLinearState));

        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashInt (static_cast<uint64_t> (texture.wrapMode)));
        combined = jam::Wyhash::fold (combined, jam::Wyhash::hashInt (texture.mipmapInput ? 1ULL : 0ULL));
    }

    // meshPath — folded once, directly, so an edit toggling/changing the
    // mesh connection still rebuilds this VulkanShader's own GPU execution.
    combined = jam::Wyhash::fold (combined, jam::Wyhash::hashBytes (
        meshPath.toRawUTF8(), meshPath.getNumBytesAsUTF8()));

    // meshShaderPath — folded the SAME way, immediately after meshPath, so an
    // edit toggling/changing the mesh-backed pass's own AUTHOR .slang
    // connection also still rebuilds this VulkanShader's own GPU execution.
    combined = jam::Wyhash::fold (combined, jam::Wyhash::hashBytes (
        meshShaderPath.toRawUTF8(), meshShaderPath.getNumBytesAsUTF8()));

    // parameterOverrides folded ORDER-INDEPENDENTLY via XOR-accumulate — same
    // technique jam::VulkanShader::computeContentHash() already uses for
    // VulkanShaderPass::parameterDefaults (jam::HashMap's own iteration order is
    // unspecified, jam_HashMap.h).
    uint64_t parameterOverridesHash { 0 };

    for (const auto& [parameterName, parameterValue] : parameterOverrides)
    {
        const auto nameHash { jam::Wyhash::hashBytes (
            parameterName.toRawUTF8(), parameterName.getNumBytesAsUTF8()) };
        const auto valueHash { jam::Wyhash::hashBytes (
            parameterValue.toRawUTF8(), parameterValue.getNumBytesAsUTF8()) };

        parameterOverridesHash ^= jam::Wyhash::fold (nameHash, valueHash);
    }

    return jam::Wyhash::fold (combined, parameterOverridesHash);
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam