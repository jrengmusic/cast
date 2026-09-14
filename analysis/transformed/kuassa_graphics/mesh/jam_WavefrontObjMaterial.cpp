//
// MTL-material file: materialParser registration (newmtl/Ka/Kd/Ks/Ke/Ns/Ni/
// Pr/Pm/Ps/map_Kd/map_Pr/map_Pm/map_Ke/norm/map_bump), loadMaterialLibrary()
// (mtllib file resolution/read), startMaterial() (newmtl), and
// getCurrentMaterial() (the newmtl-in-progress lookup every property entry
// reads through). The OBJ-geometry concern (parser registration, face
// parsing, generateNormals()) lives in jam_WavefrontObj.cpp; dispatchLines()
// itself stays there since both tables share its one dispatch loop.

namespace jam
{
/*____________________________________________________________________________*/
static void assignColour (WavefrontObj::Vertex& target, const juce::StringArray& tokens) noexcept
{
    target.x = tokens[1].getFloatValue();
    target.y = tokens[2].getFloatValue();
    target.z = tokens[3].getFloatValue();
}

/*____________________________________________________________________________*/
static void assignScalar (float& target, const juce::StringArray& tokens) noexcept
{
    target = tokens[1].getFloatValue();
}

/*____________________________________________________________________________*/
// registerMaterialParser — MTL line-keyword dispatch (same idiom as
// registerParser()). Split across 4 sub-
// registrars purely to keep each function under the 30-line Lean bound —
// they populate the same one materialParser table.
void WavefrontObj::registerMaterialParser()
{
    materialParser.add<const juce::StringArray&, juce::StringArray&> ("newmtl", [this] (const juce::StringArray& t, juce::StringArray& w) { startMaterial (t, w); });

    registerColourMaterialParser();
    registerScalarMaterialParser();
    registerSimpleTextureMaterialParser();
    registerNormalTextureMaterialParser();
}

/*____________________________________________________________________________*/
void WavefrontObj::registerColourMaterialParser()
{
    materialParser.add<const juce::StringArray&, juce::StringArray&> ("Ka", [this] (const juce::StringArray& t, juce::StringArray& w)
    {
        if (auto* material = getCurrentMaterial (w)) assignColour (material->ambient, t);
    });

    materialParser.add<const juce::StringArray&, juce::StringArray&> ("Kd", [this] (const juce::StringArray& t, juce::StringArray& w)
    {
        if (auto* material = getCurrentMaterial (w)) assignColour (material->diffuse, t);
    });

    materialParser.add<const juce::StringArray&, juce::StringArray&> ("Ks", [this] (const juce::StringArray& t, juce::StringArray& w)
    {
        if (auto* material = getCurrentMaterial (w)) assignColour (material->specular, t);
    });

    materialParser.add<const juce::StringArray&, juce::StringArray&> ("Ke", [this] (const juce::StringArray& t, juce::StringArray& w)
    {
        if (auto* material = getCurrentMaterial (w)) assignColour (material->emission, t);
    });
}

/*____________________________________________________________________________*/
void WavefrontObj::registerScalarMaterialParser()
{
    materialParser.add<const juce::StringArray&, juce::StringArray&> ("Ns", [this] (const juce::StringArray& t, juce::StringArray& w)
    {
        if (auto* material = getCurrentMaterial (w)) assignScalar (material->shininess, t);
    });

    materialParser.add<const juce::StringArray&, juce::StringArray&> ("Ni", [this] (const juce::StringArray& t, juce::StringArray& w)
    {
        if (auto* material = getCurrentMaterial (w)) assignScalar (material->refractiveIndex, t);
    });

    materialParser.add<const juce::StringArray&, juce::StringArray&> ("Pr", [this] (const juce::StringArray& t, juce::StringArray& w)
    {
        if (auto* material = getCurrentMaterial (w)) assignScalar (material->roughness, t);
    });

    materialParser.add<const juce::StringArray&, juce::StringArray&> ("Pm", [this] (const juce::StringArray& t, juce::StringArray& w)
    {
        if (auto* material = getCurrentMaterial (w)) assignScalar (material->metallic, t);
    });

    materialParser.add<const juce::StringArray&, juce::StringArray&> ("Ps", [this] (const juce::StringArray& t, juce::StringArray& w)
    {
        if (auto* material = getCurrentMaterial (w)) assignScalar (material->sheen, t);
    });
}

/*____________________________________________________________________________*/
// registerSimpleTextureMaterialParser — map_Kd/map_Pr/map_Pm/map_Ke each
// assign their texture-name field unconditionally.
void WavefrontObj::registerSimpleTextureMaterialParser()
{
    materialParser.add<const juce::StringArray&, juce::StringArray&> ("map_Kd", [this] (const juce::StringArray& t, juce::StringArray& w)
    {
        if (auto* material = getCurrentMaterial (w)) material->diffuseTextureName = t[1];
    });

    materialParser.add<const juce::StringArray&, juce::StringArray&> ("map_Pr", [this] (const juce::StringArray& t, juce::StringArray& w)
    {
        if (auto* material = getCurrentMaterial (w)) material->roughnessTextureName = t[1];
    });

    materialParser.add<const juce::StringArray&, juce::StringArray&> ("map_Pm", [this] (const juce::StringArray& t, juce::StringArray& w)
    {
        if (auto* material = getCurrentMaterial (w)) material->metallicTextureName = t[1];
    });

    materialParser.add<const juce::StringArray&, juce::StringArray&> ("map_Ke", [this] (const juce::StringArray& t, juce::StringArray& w)
    {
        if (auto* material = getCurrentMaterial (w)) material->emissiveTextureName = t[1];
    });
}

/*____________________________________________________________________________*/
// registerNormalTextureMaterialParser — norm/map_bump both target
// normalTextureName, with norm always winning (map_bump only fills it when
// still empty, so a norm on either side of a map_bump line ends up as the
// final value — see Material::normalTextureName's own doc).
void WavefrontObj::registerNormalTextureMaterialParser()
{
    materialParser.add<const juce::StringArray&, juce::StringArray&> ("norm", [this] (const juce::StringArray& t, juce::StringArray& w)
    {
        if (auto* material = getCurrentMaterial (w)) material->normalTextureName = t[1];
    });

    materialParser.add<const juce::StringArray&, juce::StringArray&> ("map_bump", [this] (const juce::StringArray& t, juce::StringArray& w)
    {
        if (auto* material = getCurrentMaterial (w))
            if (material->normalTextureName.isEmpty())
                material->normalTextureName = t[1];
    });
}

/*____________________________________________________________________________*/
// loadMaterialLibrary — mtllib may list multiple filenames on one line (OBJ
// standard); each is resolved relative to sourceFile's parent directory and
// dispatched through the shared materialParser table. An unreadable mtllib
// is a WARNING, not fatal — real-world OBJs shipped without their sibling
// .mtl are commonplace (the geometry is complete without it); affected
// shapes simply render with the default Material, the same fallback an
// unknown usemtl name already takes.
void WavefrontObj::loadMaterialLibrary (const juce::StringArray& tokens, juce::StringArray& warnings)
{
    for (int i = 1; i < tokens.size(); ++i)
    {
        const auto mtlFile { sourceFile.getParentDirectory().getChildFile (tokens[i]) };

        if (mtlFile.existsAsFile())
            dispatchLines (mtlFile.loadFileAsString(), materialParser, warnings);
        else
            warnings.add ("mtllib: could not open " + mtlFile.getFullPathName() + ", using default material");
    }
}

/*____________________________________________________________________________*/
// startMaterial — newmtl always starts a fresh Material (a redefinition
// fully replaces the prior fields under that name, warned, later wins).
void WavefrontObj::startMaterial (const juce::StringArray& tokens, juce::StringArray& warnings)
{
    const auto name { tokens[1] };

    if (materials.contains (name))
        warnings.add ("newmtl: material '" + name + "' redefined");

    auto& material { materials[name] };
    material = Material {};
    material.name = name;
    currentMaterialName = name;
}

/*____________________________________________________________________________*/
WavefrontObj::Material* WavefrontObj::getCurrentMaterial (juce::StringArray& warnings)
{
    Material* material { nullptr };

    if (materials.contains (currentMaterialName))
        material = &materials.at (currentMaterialName);
    else
        warnings.add ("mtl: property before newmtl, ignored");

    return material;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
