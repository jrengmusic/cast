namespace jam
{
/*____________________________________________________________________________*/

static constexpr float defaultFontHeight { 12.0f };

//==============================================================================
StyleManager::StyleManager (ResourceFetchers newFetchers)
    : style (Css::getOrCreate (juce::Identifier { files::styleSheet }))
    , fetchers (std::move (newFetchers))
{
    registerStyle (style);
    registerColourIds (map::ColourId::getInstance()->get());
}

//==============================================================================
void StyleManager::registerFonts (const Document::Element& fontsSection)
{
    for (auto* font : fontsSection)
    {
        const auto alias { *font->get<juce::String> (Id::id) };
        const auto filename { *font->get<juce::String> (Id::file) };
        const auto height { *font->get<double> (Id::height) };
        const auto kerning { *font->get<double> (Id::kerning) };

        for (auto& fetcher : fetchers)
        {
            if (auto typeface { getFontFromBinary (filename, fetcher) })
            {
                registerFont (alias, typeface, static_cast<float> (height), static_cast<float> (kerning));

#if JUCE_MODULE_AVAILABLE_jam_vulkan
                auto [fontData, fontSize] { fetcher (filename.toRawUTF8()) };

                if (fontData != nullptr and fontSize > 0)
                {
                    jassert (VulkanEngine::getInstance() != nullptr);
                    VulkanEngine::getInstance()->registerTypeface (typeface, fontData, static_cast<size_t> (fontSize));
                }
#endif

                break;// only after success
            }
        }
    }
}

void StyleManager::registerColours (const Document& document)
{
    auto* coloursSection { document.root->getChildByID (Id::colours) };

    coloursSection->applyToProperties (
        [this] (const juce::Identifier& key, const Document::Value& value)
        {
            registerColour (key, juce::Colour (std::get<juce::uint32> (value)));
        });
}

void StyleManager::registerMetrics (const Document& document)
{
    auto* metricsSection { document.root->getChildByID (Id::metrics) };

    metricsSection->applyToProperties (
        [this] (const juce::Identifier& key, const Document::Value& value)
        {
            metrics.addOrReplace (key.toString(), std::get<double> (value));
        });
}

void StyleManager::registerStyle (const Document& document)
{
    if (auto* fontsSection { document.root->getChildByID (Id::fonts) }; fontsSection != nullptr)
        registerFonts (*fontsSection);

    registerColours (document);
    registerMetrics (document);

    auto* appearanceSection { document.root->getChildByID (Id::appearance) };

    appearanceSection->applyToProperties (
        [this] (const juce::Identifier& key, const Document::Value& value)
        {
            appearance.addOrReplace (key.toString(), std::get<juce::Identifier> (value).toString());
        });
}

void StyleManager::registerStyle (juce::StringRef styleSheet)
{
    registerStyle (Css::getOrCreate (juce::Identifier { juce::String { styleSheet } }));
}

void StyleManager::registerFont (const juce::String& fontAlias,
                                 juce::Typeface::Ptr typeface,
                                 float height,
                                 float kerningFactor)
{
    fonts.addOrReplace (fontAlias, juce::FontOptions (typeface).withPointHeight (height).withKerningFactor (kerningFactor));
}

void StyleManager::registerColour (const juce::Identifier& colourAlias, juce::Colour colour)
{
    colours.addOrReplace (colourAlias, colour);
}

void StyleManager::registerColourIds (const jam::HashMap<int, juce::Identifier>& colourIds)
{
    for (const auto& [colourId, lexicon] : colourIds)
        colourScheme.addOrReplace (lexicon, colourId);
}

juce::FontOptions StyleManager::getFont (juce::StringRef fontAlias) const
{
    if (jam::Map::contains (fonts, fontAlias))
    {
        auto& [name, font] { *fonts.find (fontAlias) };
        return font;
    }

    return juce::FontOptions { defaultFontHeight };
}

juce::String StyleManager::getFontFilename (const juce::String& fontAlias) const
{
    if (auto* fontsSection { style.root->getChildByID (Id::fonts) }; fontsSection != nullptr)
    {
        for (auto* font : *fontsSection)
        {
            if (font->get<juce::String> (Id::id)->compare (fontAlias) == 0)
                return *font->get<juce::String> (Id::file);
        }
    }

    return {};
}

juce::Colour StyleManager::getColour (const juce::Identifier& colourAlias) const
{
    if (jam::Map::contains (colours, colourAlias))
    {
        auto& [id, colour] { *colours.find (colourAlias) };
        return colour;
    }

    return juce::Colours::magenta;
}

int StyleManager::getColourId (const juce::Identifier& colourId) const
{
    if (jam::Map::contains (colourScheme, colourId))
    {
        auto& [lexicon, id] { *colourScheme.find (colourId) };
        return id;
    }

    return -1;
}

//==============================================================================
bool StyleManager::isDark (juce::StringRef appearance)
{
    auto* appearanceMap { map::Appearance::getInstance() };

    if (juce::String { appearance }.equalsIgnoreCase (appearanceMap->get (map::Appearance::automatic)))
        return juce::Desktop::getInstance().isDarkModeActive();

    return juce::String { appearance }.equalsIgnoreCase (appearanceMap->get (map::Appearance::dark));
}

void StyleManager::applyDocumentAppearance (const Document& document,
                                            juce::LookAndFeel& laf,
                                            juce::StringRef lightOrDark)
{
    const auto applyAppearance { [this, &laf] (const Document::Element& section)
    {
        section.applyToProperties (
            [this, &laf] (const juce::Identifier& key, const Document::Value& value)
            {
                auto colour { getColour (std::get<juce::Identifier> (value)) };
                auto colourId { getColourId (key) };

                laf.setColour (colourId, colour);
            });
    } };

    const bool isDarkActive { isDark (lightOrDark) };

    auto* appearanceSection { document.root->getChildByID (Id::appearance) };
    jassert (appearanceSection != nullptr);

    map::Appearance::setAppearance (appearanceSection,
                                     appearanceSection->getChildByID (
                                         juce::Identifier { map::Appearance::getInstance()->get (map::Appearance::dark) }),
                                     isDarkActive,
                                     [this, &applyAppearance] (Document::Element* record)
                                     {
                                         applyAppearance (*record);
                                     });
}

void StyleManager::setAppearance (juce::LookAndFeel& laf,
                                    juce::StringRef lightOrDark)
{
    applyDocumentAppearance (style, laf, lightOrDark);
}

void StyleManager::setAppearanceForStyle (juce::StringRef styleSheet, juce::LookAndFeel& laf, juce::StringRef lightOrDark)
{
    applyDocumentAppearance (Css::getOrCreate (juce::Identifier { juce::String { styleSheet } }), laf, lightOrDark);
    applyDocumentAppearance (style, laf, lightOrDark);
}

void StyleManager::applyDocumentStyle (const Document& document,
                                       juce::StringRef styleId,
                                       juce::LookAndFeel* lookAndFeel)
{
    const juce::Identifier widget { juce::String { styleId } };

    if (auto* styleSection { document.root->getChildByID (Id::style) }; styleSection != nullptr)
    {
        if (auto* widgetSection { styleSection->getChildByID (widget) }; widgetSection != nullptr)
            widgetSection->applyToProperties (
                [this, lookAndFeel] (const juce::Identifier& key, const Document::Value& value)
                {
                    auto colour { getColour (std::get<juce::Identifier> (value)) };
                    auto colourId { getColourId (key) };

                    lookAndFeel->setColour (colourId, colour);
                });
    }
}

void StyleManager::setColourForStyle (juce::StringRef styleId,
                                      juce::LookAndFeel* lookAndFeel)
{
    applyDocumentStyle (style, styleId, lookAndFeel);
}

void StyleManager::applyFontsTo (juce::LookAndFeel* laf,
                                 juce::StringRef styleId,
                                 jam::Function::Map<juce::String, void>& setFontMap)
{
    for (const auto& [alias, fontOptions] : fonts)
        setFontMap.get (juce::String (styleId), laf, juce::StringRef (alias), fontOptions);
}

juce::Typeface::Ptr getFontFromBinary (const juce::String& fontFileName,
                                              ResourceFetcher fetcher)
{
    auto [ptr, size] { fetcher (fontFileName.toRawUTF8()) };
    juce::Typeface::Ptr result {};

    if (ptr != nullptr and size > 0)
    {
        result = juce::Typeface::createSystemTypefaceFor (ptr, static_cast<size_t> (size));
    }
    else
    {
#if JUCE_DEBUG
        debug::Log::write ("getFontFromBinary: fallback path -- filename=" + fontFileName);
#endif
        result = juce::LookAndFeel::getDefaultLookAndFeel()
                     .getTypefaceForFont (juce::FontOptions { defaultFontHeight });
    }

    return result;
}

//==============================================================================
#if JUCE_DEBUG
void StyleManager::printRegisteredFonts() const
{
    debug::Log::write ("Registered Fonts:");
    for (const auto& [name, font] : fonts)
    {
        debug::Log::write ("- " + name);
    }
}

void StyleManager::printRegisteredColours() const
{
    debug::Log::write ("Registered Colours:");
    for (const auto& [id, colour] : colours)
    {
        debug::Log::write ("- " + id);
    }
}
#endif// JUCE_DEBUG

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
