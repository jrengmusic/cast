namespace jam
{
/*____________________________________________________________________________*/

CssDocument::StyleDeclaration::StyleDeclaration (std::string&& sourceBytes,
                                                 jam::Array<Document::Token>&& sourceTokens,
                                                 jam::Array<Declaration>&& sourceDeclarations)
    : bytes (std::move (sourceBytes))
    , tokens (std::move (sourceTokens))
    , declarations (std::move (sourceDeclarations))
{
}

juce::String CssDocument::StyleDeclaration::item (int index) const
{
    if (index >= 0 and index < declarations.size())
        return declarations.at (index).property;

    return {};
}

juce::String CssDocument::StyleDeclaration::getPropertyValue (juce::StringRef property) const
{
    return CssDocument::getPropertyValue (bytes, tokens, declarations, property);
}

juce::String CssDocument::StyleDeclaration::getVariable (juce::StringRef attributeName) const
{
    return CssDocument::getVariable (bytes, tokens, declarations, attributeName);
}

bool CssDocument::StyleDeclaration::hasAttribute (juce::StringRef attributeName) const noexcept
{
    return findDeclaration (attributeName) != nullptr;
}

juce::String CssDocument::StyleDeclaration::getStringAttribute (juce::StringRef attributeName) const
{
    return CssDocument::getStringAttribute (bytes, tokens, declarations, attributeName);
}

juce::String CssDocument::StyleDeclaration::getUnit (juce::StringRef attributeName) const
{
    return CssDocument::getUnit (bytes, tokens, declarations, attributeName);
}

double CssDocument::StyleDeclaration::getDoubleAttribute (juce::StringRef attributeName) const
{
    return CssDocument::getDoubleAttribute (tokens, declarations, attributeName);
}

int CssDocument::StyleDeclaration::getPropertyType (juce::StringRef attributeName) const
{
    return CssDocument::getPropertyType (tokens, declarations, attributeName);
}

const CssDocument::Declaration* CssDocument::StyleDeclaration::findDeclaration (juce::StringRef property) const
{
    return CssDocument::findDeclaration (declarations, property);
}

CssDocument::StyleDeclaration CssDocument::getStyle (const juce::String& input)
{
    CssDocument document;
    document.entryProduction = declarationsProduction;
    document.Document::parse (input.toRawUTF8(), static_cast<int> (input.getNumBytesAsUTF8()));

    return StyleDeclaration (std::move (document.source),
                             std::move (*document.root->get<jam::Array<Document::Token>> (Id::tokens)),
                             std::move (document.declarations));
}

const Document& CssDocument::getOrCreate (const juce::Identifier& name)
{
    auto* registry { SharedDocuments::getInstance() };
    jassert (registry != nullptr);

    if (registry->contains (name))
        return *registry->at (name);

    using namespace BinaryData;
    Raw binary (name.toString());

    auto document { std::make_unique<CssDocument>() };
    document->Document::parse (binary.data, binary.size);

    auto& ref { *document };
    registry->try_emplace (name, std::move (document));
    return ref;
}

int CssDocument::getToken (Cursor& cursor, int segmentType)
{
    static const auto segments {
        []
        {
            jam::Function::Map<int, Document::Token> segments;

            segments.add<Cursor&> (map::DocumentTokenType::region,
                [] (Cursor& walkCursor) { return getRegionToken (walkCursor); });

            segments.add<Cursor&> (map::DocumentTokenType::operators,
                [] (Cursor& walkCursor) { return getOperatorToken (walkCursor); });

            segments.add<Cursor&> (map::DocumentTokenType::text,
                [] (Cursor& walkCursor) { return getTextToken (walkCursor); });

            segments.add<Cursor&> (map::DocumentTokenType::comment,
                [] (Cursor& walkCursor)
                {
                    const auto& close { getCssVocabulary().commentClose };

                    while (not walkCursor.isEmpty() and not walkCursor.startsWith (close))
                        ++walkCursor;

                    if (not walkCursor.isEmpty())
                        walkCursor += static_cast<int> (close.size());

                    Document::Token token;
                    token.type = map::CssTokenType::whitespace;
                    return token;
                });

            return segments;
        }()
    };

    jassert (not cursor.isEmpty());

    Cursor walkCursor { cursor.source, cursor.getOffset() };
    const auto start { walkCursor.getOffset() };
    Document::Token token { segments.get (segmentType, walkCursor) };
    token.setSpan (start, walkCursor.getOffset());

    const auto length { static_cast<int> (walkCursor.getOffset() - start) };
    tokens.add (std::move (token));

    return length;
}

void CssDocument::build()
{
    Document::Token endOfFile;
    endOfFile.type = map::CssTokenType::endOfFile;
    tokens.add (std::move (endOfFile));

    if (entryProduction == declarationsProduction)
    {
        declarations = getDeclarations (getSource(), tokens, 0, tokens.size());
    }
    else
    {
        const auto sheet { parseStyleSheet (getSource(), tokens) };

        addChild (*root, Id::colours);
        addChild (*root, Id::appearance);
        addChild (*root, Id::metrics);
        addChild (*root, Id::unit);
        addChild (*root, Id::window);

        Document::Element* fonts { nullptr };
        Document::Element* style { nullptr };

        addRules (*this, fonts, style, tokens, sheet, getSelectors());
    }

    root->add<jam::Array<Document::Token>> (Id::tokens, std::move (tokens));
}

const CssDocument::Declaration* CssDocument::findDeclaration (const jam::Array<Declaration>& declarations,
                                                               juce::StringRef property)
{
    for (int index { 0 }; index < declarations.size(); ++index)
        if (declarations.at (index).property == property)
            return &declarations.at (index);

    return nullptr;
}

juce::String CssDocument::getVariable (const std::string& source,
                                       const jam::Array<Document::Token>& tokens,
                                       const jam::Array<Declaration>& declarations,
                                       juce::StringRef attributeName)
{
    if (auto* declaration { findDeclaration (declarations, attributeName) };
        declaration and declaration->value.size() >= 2
        and tokens.at (declaration->value.at (0)).type == map::CssTokenType::function
        and getText (source, *tokens.at (declaration->value.at (0)).get<Document::Token> (Id::name))
                .equalsIgnoreCase (Chars::varFunction)
        and tokens.at (declaration->value.at (1)).type == map::CssTokenType::ident)
        return getText (source, *tokens.at (declaration->value.at (1)).get<Document::Token> (Id::name));

    return {};
}

juce::String CssDocument::getStringAttribute (const std::string& source,
                                              const jam::Array<Document::Token>& tokens,
                                              const jam::Array<Declaration>& declarations,
                                              juce::StringRef attributeName)
{
    static const auto attributeSerializers {
        []
        {
            jam::Function::Map<int, juce::String> attributeSerializers;

            auto nameSerializer { [] (const std::string& source, const Document::Token& token)
                                  {
                                      return getText (source, *token.get<Document::Token> (Id::name));
                                  } };

            for (const auto type : textTokenTypes)
                attributeSerializers.add<const std::string&, const Document::Token&> (type, nameSerializer);

            attributeSerializers.add<const std::string&, const Document::Token&> (map::CssTokenType::number,
                                             [] (const std::string&, const Document::Token& token)
                                             {
                                                 return juce::String (*token.get<double> (Id::numeric));
                                             });

            attributeSerializers.add<const std::string&, const Document::Token&> (map::CssTokenType::dimension,
                                             [] (const std::string& source, const Document::Token& token)
                                             {
                                                 return juce::String (*token.get<double> (Id::numeric))
                                                        + getText (source, *token.get<Document::Token> (Id::unit));
                                             });

            attributeSerializers.add<const std::string&, const Document::Token&> (map::CssTokenType::percentage,
                                             [] (const std::string&, const Document::Token& token)
                                             {
                                                 return juce::String (*token.get<double> (Id::numeric))
                                                        + juce::String::charToString (Chars::percent);
                                             });

            return attributeSerializers;
        }()
    };

    if (auto* declaration { findDeclaration (declarations, attributeName) };
        declaration and declaration->value.size() > 0)
    {
        const auto& token { tokens.at (declaration->value.at (0)) };

        if (attributeSerializers.contains (token.type))
            return attributeSerializers.get (token.type, source, token);

        return {};
    }

    return {};
}

juce::String CssDocument::getUnit (const std::string& source,
                                   const jam::Array<Document::Token>& tokens,
                                   const jam::Array<Declaration>& declarations,
                                   juce::StringRef attributeName)
{
    if (auto* declaration { findDeclaration (declarations, attributeName) };
        declaration and declaration->value.size() > 0
        and tokens.at (declaration->value.at (0)).type == map::CssTokenType::dimension)
        return getText (source, *tokens.at (declaration->value.at (0)).get<Document::Token> (Id::unit));

    return {};
}

double CssDocument::getDoubleAttribute (const jam::Array<Document::Token>& tokens,
                                        const jam::Array<Declaration>& declarations,
                                        juce::StringRef attributeName)
{
    if (auto* declaration { findDeclaration (declarations, attributeName) };
        declaration and declaration->value.size() > 0)
        return *tokens.at (declaration->value.at (0)).get<double> (Id::numeric);

    return 0.0;
}

int CssDocument::getPropertyType (const jam::Array<Document::Token>& tokens,
                                  const jam::Array<Declaration>& declarations,
                                  juce::StringRef attributeName)
{
    if (auto* declaration { findDeclaration (declarations, attributeName) };
        declaration and declaration->value.size() > 0)
        return tokens.at (declaration->value.at (0)).type;

    return map::CssTokenType::ident;
}

juce::String CssDocument::getPropertyValue (const std::string& source,
                                            const jam::Array<Document::Token>& tokens,
                                            const jam::Array<Declaration>& declarations,
                                            juce::StringRef property)
{
    if (auto* declaration { findDeclaration (declarations, property) })
        return toText (source, tokens, declaration->value);

    return {};
}

CssDocument::StyleSheet CssDocument::parseStyleSheet (const std::string& source, jam::Array<Document::Token>& tokens)
{
    return { getRules (source, tokens, 0, tokens.size()) };
}

juce::String CssDocument::stripCustomPropertyPrefix (const juce::String& property)
{
    static const juce::String prefix { Chars::doubleDash };

    if (property.startsWith (prefix)) return property.substring (prefix.length());
    return property;
}

juce::uint32 CssDocument::toColourValue (const std::string& source,
                                         const jam::Array<Document::Token>& tokens,
                                         const jam::Array<Declaration>& declarations,
                                         const juce::String& property)
{
    static constexpr juce::uint32 opaqueAlphaMask { 0xff000000u };
    static constexpr juce::uint32 alphaByteMask { 0x000000ffu };
    static constexpr int alphaByteShift { 24 };
    static constexpr int rgbByteShift { 8 };
    static constexpr int shortHexLength { 3 };
    static constexpr int longHexLength { 6 };
    static constexpr int alphaHexLength { 8 };

    const auto raw { getStringAttribute (source, tokens, declarations, property) };

    if (raw.equalsIgnoreCase (Id::transparent))
        return {};

    bool isPlausibleHex { raw.length() == shortHexLength or raw.length() == longHexLength
                          or raw.length() == alphaHexLength };

    for (auto cursor { raw.getCharPointer() }; isPlausibleHex and not cursor.isEmpty();)
        isPlausibleHex = Document::isHexDigit (cursor.getAndAdvance());

    if (not isPlausibleHex)
        return {};

    if (raw.length() == alphaHexLength)
    {
        const auto rgba { static_cast<juce::uint32> (raw.getHexValue32()) };
        return ((rgba & alphaByteMask) << alphaByteShift) | (rgba >> rgbByteShift);
    }

    juce::String expanded;

    if (raw.length() == shortHexLength)
    {
        for (auto cursor { raw.getCharPointer() }; not cursor.isEmpty(); ++cursor)
            expanded << *cursor << *cursor;
    }
    else
    {
        expanded = raw;
    }

    return opaqueAlphaMask | static_cast<juce::uint32> (expanded.getHexValue32());
}

void CssDocument::addColours (Document& document, Document::Element& section,
                              const jam::Array<Document::Token>& tokens,
                              const jam::Array<Declaration>& declarations)
{
    for (int index { 0 }; index < declarations.size(); ++index)
    {
        const auto property { declarations.at (index).property };

        section.add<juce::uint32> (juce::Identifier { property }, toColourValue (document.getSource(), tokens, declarations, property));
    }
}

void CssDocument::addAppearance (Document& document, Document::Element& section,
                                 const jam::Array<Document::Token>& tokens,
                                 const jam::Array<Declaration>& declarations)
{
    for (int index { 0 }; index < declarations.size(); ++index)
    {
        const auto property { declarations.at (index).property };

        section.add<juce::Identifier> (juce::Identifier { property },
                                       juce::Identifier { getVariable (document.getSource(), tokens, declarations, property) });
    }
}

void CssDocument::addMetrics (Document& document, Document::Element& section, Document::Element& unit,
                              const jam::Array<Document::Token>& tokens,
                              const jam::Array<Declaration>& declarations)
{
    for (int index { 0 }; index < declarations.size(); ++index)
    {
        const auto property { declarations.at (index).property };
        const juce::Identifier id { stripCustomPropertyPrefix (property) };

        section.add<double> (id, getDoubleAttribute (tokens, declarations, property));

        const auto text { getUnit (document.getSource(), tokens, declarations, property) };

        if (text.isNotEmpty())
            unit.add<juce::String> (id, text);
    }
}

void CssDocument::addStyleAttribute (Document& document, Document::Element& section,
                                     const jam::Array<Document::Token>& tokens,
                                     const jam::Array<Declaration>& declarations,
                                     const juce::String& property,
                                     const juce::Identifier& id)
{
    static const auto styleAttributes {
        []
        {
            jam::Function::Map<int, void> styleAttributes;

            auto textSerializer { [] (Document& document,
                                      Document::Element& section,
                                      const jam::Array<Document::Token>& tokens,
                                      const jam::Array<Declaration>& declarations,
                                      const juce::String& property,
                                      const juce::Identifier& id)
                                  {
                                      section.add<juce::String> (
                                          id,
                                          getStringAttribute (document.getSource(), tokens, declarations, property));
                                  } };

            for (const auto type : textTokenTypes)
                styleAttributes.add<Document&,
                        Document::Element&,
                        const jam::Array<Document::Token>&,
                        const jam::Array<Declaration>&,
                        const juce::String&,
                        const juce::Identifier&> (type, textSerializer);

            for (const auto type : numericTokenTypes)
                styleAttributes.add<Document&,
                        Document::Element&,
                        const jam::Array<Document::Token>&,
                        const jam::Array<Declaration>&,
                        const juce::String&,
                        const juce::Identifier&> (
                    type,
                    [] (Document& document,
                        Document::Element& section,
                        const jam::Array<Document::Token>& tokens,
                        const jam::Array<Declaration>& declarations,
                        const juce::String& property,
                        const juce::Identifier& id)
                    {
                        section.add<double> (
                            id, getDoubleAttribute (tokens, declarations, property));
                    });

            return styleAttributes;
        }()
    };

    const auto type { getPropertyType (tokens, declarations, property) };

    if (styleAttributes.contains (type))
        styleAttributes.get (type, document, section, tokens, declarations, property, id);
}

void CssDocument::addWindow (Document& document, Document::Element& section,
                             const jam::Array<Document::Token>& tokens,
                             const jam::Array<Declaration>& declarations)
{
    for (int index { 0 }; index < declarations.size(); ++index)
    {
        const auto property { declarations.at (index).property };

        addStyleAttribute (document, section,
                           tokens,
                           declarations,
                           property,
                           juce::Identifier { stripCustomPropertyPrefix (property) });
    }
}

void CssDocument::addWidgetStyle (Document& document,
                                  Document::Element*& styleSection,
                                  const jam::Array<Document::Token>& tokens,
                                  const Rule& rule)
{
    static const int classSelectorLength {
        juce::String::charToString (Chars::dot).length()
    };

    if (styleSection == nullptr)
        styleSection = document.addChild (*document.root, Id::style);

    auto* section { document.addChild (*styleSection,
                                       juce::Identifier { rule.selectorText.substring (classSelectorLength) }) };

    addAppearance (document, *section, tokens, rule.style);
}

const jam::Function::Map<juce::String, void>& CssDocument::getSelectors()
{
    static const auto selectors {
        []
        {
            jam::Function::Map<juce::String, void> selectors;

            selectors.add<Document&, const jam::Array<Document::Token>&, const Rule&> (
                Id::rootSelector.toString(),
                [] (Document& document,
                    const jam::Array<Document::Token>& tokens,
                    const Rule& rule)
                {
                    addColours (document, *document.root->getChildByID (Id::colours), tokens, rule.style);
                });

            selectors.add<Document&, const jam::Array<Document::Token>&, const Rule&> (
                juce::String::charToString (Chars::dot) + Id::appearance.toString(),
                [] (Document& document,
                    const jam::Array<Document::Token>& tokens,
                    const Rule& rule)
                {
                    addAppearance (document, *document.root->getChildByID (Id::appearance), tokens, rule.style);
                });

            selectors.add<Document&, const jam::Array<Document::Token>&, const Rule&> (
                juce::String::charToString (Chars::dot) + Id::metrics.toString(),
                [] (Document& document,
                    const jam::Array<Document::Token>& tokens,
                    const Rule& rule)
                {
                    addMetrics (document,
                               *document.root->getChildByID (Id::metrics),
                               *document.root->getChildByID (Id::unit),
                               tokens,
                               rule.style);
                });

            selectors.add<Document&, const jam::Array<Document::Token>&, const Rule&> (
                juce::String::charToString (Chars::dot) + Id::window.toString(),
                [] (Document& document,
                    const jam::Array<Document::Token>& tokens,
                    const Rule& rule)
                {
                    addWindow (document, *document.root->getChildByID (Id::window), tokens, rule.style);
                });

            const auto skip { [] (Document&, const jam::Array<Document::Token>&, const Rule&)
                              {
                              } };

            selectors.add<Document&, const jam::Array<Document::Token>&, const Rule&> (
                juce::String::charToString (Chars::openBracket), skip);
            selectors.add<Document&, const jam::Array<Document::Token>&, const Rule&> (
                Id::body.toString(), skip);

            return selectors;
        }()
    };

    return selectors;
}

void CssDocument::addRules (Document& document,
                            Document::Element*& fonts,
                            Document::Element*& style,
                            const jam::Array<Document::Token>& tokens,
                            const StyleSheet& sheet,
                            const jam::Function::Map<juce::String, void>& selectors)
{
    static const juce::String classSelectorPrefix { juce::String::charToString (Chars::dot) };
    static const juce::String appearanceSelectorPrefix { juce::String::charToString (Chars::dot)
                                                         + Id::appearance.toString()
                                                         + juce::String::charToString (Chars::dot) };

    for (const auto& rule : sheet.cssRules)
    {
        if (rule.type == map::CssRuleType::fontFaceRule)
        {
            addFonts (document, fonts, tokens, rule);
        }
        else if (rule.type == map::CssRuleType::styleRule)
        {
            if (selectors.contains (rule.selectorText))
            {
                selectors.get (rule.selectorText, document, tokens, rule);
            }
            else if (rule.selectorText.startsWith (appearanceSelectorPrefix))
            {
                auto* record { document.addChild (*document.root->getChildByID (Id::appearance),
                                                  juce::Identifier { rule.selectorText.substring (appearanceSelectorPrefix.length()) }) };

                addAppearance (document, *record, tokens, rule.style);
            }
            else if (rule.selectorText.startsWith (classSelectorPrefix))
            {
                addWidgetStyle (document, style, tokens, rule);
            }
            else if (const auto leadingOperator { rule.selectorText.substring (0, 1) };
                     selectors.contains (leadingOperator))
            {
                selectors.get (leadingOperator, document, tokens, rule);
            }
        }
    }
}

void CssDocument::addFontsAttributes (Document& document, Document::Element& fontsSection,
                                      const jam::Array<Document::Token>& tokens,
                                      const jam::Array<Declaration>& declarations)
{
    for (const auto& id : { Id::rasterizer, Id::gamma, Id::contrast, Id::embolden })
    {
        const juce::String property { juce::String (Chars::doubleDash)
                                      + id.toString() };

        if (auto* found { findDeclaration (declarations, property) }; found)
            addStyleAttribute (document, fontsSection, tokens, declarations, property, id);
    }
}

void CssDocument::addFonts (Document& document,
                            Document::Element*& fontsSection,
                            const jam::Array<Document::Token>& tokens,
                            const Rule& rule)
{
    if (fontsSection == nullptr)
    {
        fontsSection = document.addChild (*document.root, Id::fonts);
        addFontsAttributes (document, *fontsSection, tokens, rule.style);
    }

    addFont (document, *fontsSection, tokens, rule.style);
}

void CssDocument::addFont (Document& document, Document::Element& fontsSection,
                           const jam::Array<Document::Token>& tokens,
                           const jam::Array<Declaration>& declarations)
{
    const juce::String heightKey { juce::String (Chars::doubleDash)
                                   + Id::height.toString() };
    const juce::String kerningKey { juce::String (Chars::doubleDash)
                                    + Id::kerning.toString() };
    const auto src { getStringAttribute (document.getSource(), tokens, declarations, Id::src.toString()) };

    auto* font { document.addChild (fontsSection, Id::font) };

    font->add<juce::String> (Id::id,
                             getStringAttribute (document.getSource(),
                                                 tokens,
                                                 declarations,
                                                 Id::fontFamily.toString()));
    font->add<juce::String> (
        Id::file,
        src.fromLastOccurrenceOf (juce::String::charToString (Chars::slash), false, false));
    font->add<double> (Id::height,
                       getDoubleAttribute (tokens, declarations, heightKey));
    font->add<double> (Id::kerning,
                       getDoubleAttribute (tokens, declarations, kerningKey));
}

juce::String CssDocument::toText (const std::string& source, const jam::Array<Document::Token>& tokens, const jam::Array<int>& indices)
{
    static const auto serializers {
        []
        {
            jam::Function::Map<int, void> serializers;

            auto appendName { [] (const std::string& source, const Document::Token& token, std::string& result)
                              {
                                  result += getText (source, *token.get<Document::Token> (Id::name)).toStdString();
                              } };

            serializers.add<const std::string&, const Document::Token&, std::string&> (map::CssTokenType::ident, appendName);
            serializers.add<const std::string&, const Document::Token&, std::string&> (map::CssTokenType::string, appendName);

            serializers.add<const std::string&, const Document::Token&, std::string&> (map::CssTokenType::delim,
                                                  [] (const std::string& source, const Document::Token& token, std::string& result)
                                                  {
                                                      result += getText (source, token).toStdString();
                                                  });

            serializers.add<const std::string&, const Document::Token&, std::string&> (map::CssTokenType::url,
                                                  [] (const std::string& source, const Document::Token& token, std::string& result)
                                                  {
                                                      result += Chars::urlOpen;
                                                      result += getText (source, *token.get<Document::Token> (Id::name)).toStdString();
                                                      result += static_cast<char> (Chars::closeParen);
                                                  });

            serializers.add<const std::string&, const Document::Token&, std::string&> (map::CssTokenType::hash,
                                                  [] (const std::string& source, const Document::Token& token, std::string& result)
                                                  {
                                                      result += static_cast<char> (Chars::hash);
                                                      result += getText (source, *token.get<Document::Token> (Id::name)).toStdString();
                                                  });

            serializers.add<const std::string&, const Document::Token&, std::string&> (map::CssTokenType::atKeyword,
                                                  [] (const std::string& source, const Document::Token& token, std::string& result)
                                                  {
                                                      result += static_cast<char> (Chars::at);
                                                      result += getText (source, *token.get<Document::Token> (Id::name)).toStdString();
                                                  });

            serializers.add<const std::string&, const Document::Token&, std::string&> (map::CssTokenType::function,
                                                  [] (const std::string& source, const Document::Token& token, std::string& result)
                                                  {
                                                      result += getText (source, *token.get<Document::Token> (Id::name)).toStdString();
                                                      result += static_cast<char> (Chars::openParen);
                                                  });

            serializers.add<const std::string&, const Document::Token&, std::string&> (map::CssTokenType::number,
                                                  [] (const std::string&, const Document::Token& token, std::string& result)
                                                  {
                                                      result += juce::String (*token.get<double> (Id::numeric)).toStdString();
                                                  });

            serializers.add<const std::string&, const Document::Token&, std::string&> (map::CssTokenType::percentage,
                                                  [] (const std::string&, const Document::Token& token, std::string& result)
                                                  {
                                                      result += juce::String (*token.get<double> (Id::numeric)).toStdString();
                                                      result += static_cast<char> (Chars::percent);
                                                  });

            serializers.add<const std::string&, const Document::Token&, std::string&> (map::CssTokenType::dimension,
                                                  [] (const std::string& source, const Document::Token& token, std::string& result)
                                                  {
                                                      result += juce::String (*token.get<double> (Id::numeric)).toStdString();
                                                      result += getText (source, *token.get<Document::Token> (Id::unit)).toStdString();
                                                  });

            return serializers;
        }()
    };

    std::string result;

    for (int index { 0 }; index < indices.size(); ++index)
    {
        const auto& token { tokens.at (indices.at (index)) };

        if (map::cssCodePoints[token.type] != Chars::nullCharacter)
        {
            result += static_cast<char> (map::cssCodePoints[token.type]);
            continue;
        }

        if (serializers.contains (token.type))
            serializers.get (token.type, source, token, result);
    }

    return juce::String::fromUTF8 (result.data(), static_cast<int> (result.size())).trim();
}

const Document::Vocabulary& CssDocument::getCssVocabulary()
{
    static const Document::Vocabulary vocabulary { map::css,
        Chars::cssCommentOpen, Chars::cssCommentClose };

    return vocabulary;
}

bool CssDocument::isIdentStartCodePoint (juce::juce_wchar ch) noexcept
{
    return Document::isLetter (ch) or isNonAscii (ch) or ch == Chars::underscore;
}

bool CssDocument::isIdentCodePoint (juce::juce_wchar ch) noexcept
{
    return isIdentStartCodePoint (ch) or Document::isDigit (ch) or ch == Chars::dash;
}

bool CssDocument::isNonPrintableCodePoint (juce::juce_wchar ch) noexcept
{
    return ch <= Chars::backspace or ch == Chars::verticalTab
           or (ch >= Chars::shiftOut and ch <= Chars::unitSeparator)
           or (ch >= Chars::deleteCharacter and ch <= Chars::applicationProgramCommand);
}

bool CssDocument::isValidEscape (juce::juce_wchar first, juce::juce_wchar second) noexcept
{
    return first == Chars::backslash and not isNewline (second);
}

bool CssDocument::wouldStartIdentSequence (juce::juce_wchar first,
                                           juce::juce_wchar second,
                                           juce::juce_wchar third) noexcept
{
    if (first == Chars::dash)
        return isIdentStartCodePoint (second) or second == Chars::dash
               or isValidEscape (second, third);

    if (isIdentStartCodePoint (first))
        return true;

    if (first == Chars::backslash)
        return isValidEscape (first, second);

    return false;
}

bool CssDocument::wouldStartNumber (juce::juce_wchar first,
                                    juce::juce_wchar second,
                                    juce::juce_wchar third) noexcept
{
    if (first == Chars::plus or first == Chars::dash)
    {
        if (Document::isDigit (second))
            return true;

        return second == Chars::dot and Document::isDigit (third);
    }

    if (first == Chars::dot)
        return Document::isDigit (second);

    return Document::isDigit (first);
}

juce::String CssDocument::getEscapedCodePoint (Cursor& cursor)
{
    if (cursor.isEmpty())
    {
        return juce::String::charToString (Chars::replacementCharacter);
    }

    const auto ch { *cursor };
    ++cursor;

    if (Document::isHexDigit (ch))
    {
        juce::juce_wchar value { static_cast<juce::juce_wchar> (
            juce::CharacterFunctions::getHexDigitValue (ch)) };
        int count { 1 };

        while (count < maximumEscapeHexDigits and not cursor.isEmpty()
               and Document::isHexDigit (*cursor))
        {
            value = value * Document::hexadecimalRadix
                  + static_cast<juce::juce_wchar> (
                        juce::CharacterFunctions::getHexDigitValue (*cursor));
            ++cursor;
            ++count;
        }

        if (not cursor.isEmpty() and Document::isWhitespace (*cursor))
            ++cursor;

        if (value == Chars::nullCharacter
            or (value >= Chars::surrogateRangeStart and value <= Chars::surrogateRangeEnd)
            or value > Chars::maximumCodePoint)
        {
            value = Chars::replacementCharacter;
        }

        return juce::String::charToString (value);
    }

    return juce::String::charToString (ch);
}

juce::String CssDocument::getText (const std::string& source, const Document::Token& span)
{
    juce::String text;
    Cursor cursor { source, span.offset() };
    size_t runStart { cursor.getOffset() };
    const size_t end { span.offset() + span.length() };

    while (cursor.getOffset() < end)
    {
        if (*cursor != Chars::backslash)
        {
            ++cursor;
            continue;
        }

        text += juce::String::fromUTF8 (source.data() + runStart, static_cast<int> (cursor.getOffset() - runStart));
        ++cursor;

        if (cursor.getOffset() >= end)
        {
            runStart = cursor.getOffset();
            break;
        }

        if (isNewline (*cursor))
        {
            ++cursor;
            runStart = cursor.getOffset();
            continue;
        }

        text += getEscapedCodePoint (cursor);
        runStart = cursor.getOffset();
    }

    text += juce::String::fromUTF8 (source.data() + runStart, static_cast<int> (cursor.getOffset() - runStart));
    return text;
}

void CssDocument::getIdentSequence (Cursor& cursor)
{
    for (;;)
    {
        if (cursor.isEmpty())
            return;

        if (isIdentCodePoint (*cursor))
        {
            ++cursor;
            continue;
        }

        const auto first { *cursor };
        const Cursor secondCursor { cursor.source, cursor.getOffset() + 1 };
        const auto second { secondCursor.isEmpty() ? Document::eofCodePoint : *secondCursor };

        if (isValidEscape (first, second))
        {
            ++cursor;
            getEscapedCodePoint (cursor);
            continue;
        }

        return;
    }
}

void CssDocument::getNumberExponent (Cursor& cursor)
{
    const Cursor aheadCursor { cursor.source, cursor.getOffset() + 1 };
    const auto signOrDigit { aheadCursor.isEmpty() ? Document::eofCodePoint : *aheadCursor };

    bool hasExponent { false };

    if (Document::isDigit (signOrDigit))
    {
        hasExponent = true;
    }
    else if ((signOrDigit == Chars::plus or signOrDigit == Chars::dash) and not aheadCursor.isEmpty())
    {
        const Cursor afterSignCursor { cursor.source, aheadCursor.getOffset() + 1 };

        if (not afterSignCursor.isEmpty() and Document::isDigit (*afterSignCursor))
            hasExponent = true;
    }

    if (hasExponent)
    {
        ++cursor;

        if (not cursor.isEmpty() and (*cursor == Chars::plus or *cursor == Chars::dash))
            ++cursor;

        while (not cursor.isEmpty() and Document::isDigit (*cursor))
            ++cursor;
    }
}

double CssDocument::getNumber (Cursor& cursor)
{
    const auto start { cursor.getOffset() };

    if (not cursor.isEmpty() and (*cursor == Chars::plus or *cursor == Chars::dash))
        ++cursor;

    while (not cursor.isEmpty() and Document::isDigit (*cursor))
        ++cursor;

    if (not cursor.isEmpty() and *cursor == Chars::dot)
    {
        const Cursor aheadCursor { cursor.source, cursor.getOffset() + 1 };

        if (not aheadCursor.isEmpty() and Document::isDigit (*aheadCursor))
        {
            ++cursor;

            while (not cursor.isEmpty() and Document::isDigit (*cursor))
                ++cursor;
        }
    }

    if (not cursor.isEmpty() and (*cursor == Chars::lowerE or *cursor == Chars::upperE))
        getNumberExponent (cursor);

    return juce::String::fromUTF8 (cursor.source.data() + start, static_cast<int> (cursor.getOffset() - start)).getDoubleValue();
}

void CssDocument::getEndOfBadUrl (Cursor& cursor)
{
    for (;;)
    {
        if (cursor.isEmpty())
            return;

        const auto ch { *cursor };
        ++cursor;

        if (ch == Chars::closeParen)
            return;

        const auto next { cursor.isEmpty() ? Document::eofCodePoint : *cursor };

        if (isValidEscape (ch, next))
            getEscapedCodePoint (cursor);
    }
}

Document::Token CssDocument::getStringToken (Cursor& cursor, juce::juce_wchar endingCodePoint)
{
    const auto contentStart { cursor.getOffset() };

    for (;;)
    {
        if (cursor.isEmpty())
        {
            Document::Token token;
            token.type = map::CssTokenType::string;
            token.add<Document::Token> (Id::name, Document::Token { contentStart, cursor.getOffset() });
            return token;
        }

        const auto beforeChar { cursor.getOffset() };
        const auto ch { *cursor };

        if (ch == endingCodePoint)
        {
            ++cursor;
            Document::Token token;
            token.type = map::CssTokenType::string;
            token.add<Document::Token> (Id::name, Document::Token { contentStart, beforeChar });
            return token;
        }

        if (isNewline (ch))
        {
            Document::Token token;
            token.type = map::CssTokenType::badString;
            token.add<Document::Token> (Id::name, Document::Token { contentStart, beforeChar });
            return token;
        }

        ++cursor;

        if (ch == Chars::backslash)
        {
            if (cursor.isEmpty())
                continue;

            if (isNewline (*cursor))
            {
                ++cursor;
                continue;
            }

            getEscapedCodePoint (cursor);
        }
    }
}

Document::Token CssDocument::getUrlTail (Cursor& cursor, size_t contentStart, size_t contentEnd)
{
    while (not cursor.isEmpty() and Document::isWhitespace (*cursor))
        ++cursor;

    if (cursor.isEmpty())
    {
        Document::Token token;
        token.type = map::CssTokenType::url;
        token.add<Document::Token> (Id::name, Document::Token { contentStart, contentEnd });
        return token;
    }

    if (*cursor == Chars::closeParen)
    {
        ++cursor;
        Document::Token token;
        token.type = map::CssTokenType::url;
        token.add<Document::Token> (Id::name, Document::Token { contentStart, contentEnd });
        return token;
    }

    getEndOfBadUrl (cursor);
    Document::Token token;
    token.type = map::CssTokenType::badUrl;
    return token;
}

Document::Token CssDocument::getUrlBody (Cursor& cursor)
{
    const auto contentStart { cursor.getOffset() };

    for (;;)
    {
        if (cursor.isEmpty())
        {
            Document::Token token;
            token.type = map::CssTokenType::url;
            token.add<Document::Token> (Id::name, Document::Token { contentStart, cursor.getOffset() });
            return token;
        }

        const auto beforeChar { cursor.getOffset() };
        const auto ch { *cursor };
        ++cursor;

        if (ch == Chars::closeParen)
        {
            Document::Token token;
            token.type = map::CssTokenType::url;
            token.add<Document::Token> (Id::name, Document::Token { contentStart, beforeChar });
            return token;
        }

        if (Document::isWhitespace (ch))
            return getUrlTail (cursor, contentStart, beforeChar);

        if (ch == Chars::doubleQuote or ch == Chars::singleQuote or ch == Chars::openParen
            or isNonPrintableCodePoint (ch))
        {
            getEndOfBadUrl (cursor);
            Document::Token token;
            token.type = map::CssTokenType::badUrl;
            return token;
        }

        if (ch == Chars::backslash)
        {
            const auto next { cursor.isEmpty() ? Document::eofCodePoint : *cursor };

            if (isValidEscape (ch, next))
            {
                getEscapedCodePoint (cursor);
                continue;
            }

            getEndOfBadUrl (cursor);
            Document::Token token;
            token.type = map::CssTokenType::badUrl;
            return token;
        }
    }
}

Document::Token CssDocument::getUrlToken (Cursor& cursor)
{
    while (not cursor.isEmpty() and Document::isWhitespace (*cursor))
        ++cursor;

    return getUrlBody (cursor);
}

Document::Token CssDocument::getNumericToken (Cursor& cursor)
{
    const auto numeric { getNumber (cursor) };

    if (not cursor.isEmpty())
    {
        const auto first { *cursor };
        const Cursor secondCursor { cursor.source, cursor.getOffset() + 1 };
        const auto second { secondCursor.isEmpty() ? Document::eofCodePoint : *secondCursor };
        const Cursor thirdCursor { cursor.source, cursor.getOffset() + 2 };
        const auto third { thirdCursor.isEmpty() ? Document::eofCodePoint : *thirdCursor };

        if (wouldStartIdentSequence (first, second, third))
        {
            const auto unitStart { cursor.getOffset() };
            getIdentSequence (cursor);

            Document::Token token;
            token.type = map::CssTokenType::dimension;
            token.add<double> (Id::numeric, numeric);
            token.add<Document::Token> (Id::unit, Document::Token { unitStart, cursor.getOffset() });
            return token;
        }
    }

    if (not cursor.isEmpty() and *cursor == Chars::percent)
    {
        ++cursor;

        Document::Token token;
        token.type = map::CssTokenType::percentage;
        token.add<double> (Id::numeric, numeric);
        return token;
    }

    Document::Token token;
    token.type = map::CssTokenType::number;
    token.add<double> (Id::numeric, numeric);
    return token;
}

bool CssDocument::getUrlQuoted (Cursor cursor)
{
    while (not cursor.isEmpty() and Document::isWhitespace (*cursor))
        ++cursor;

    return not cursor.isEmpty()
           and (*cursor == Chars::doubleQuote or *cursor == Chars::singleQuote);
}

Document::Token CssDocument::getIdentLikeToken (Cursor& cursor)
{
    const auto nameStart { cursor.getOffset() };
    getIdentSequence (cursor);
    const auto nameEnd { cursor.getOffset() };

    const auto name { getText (cursor.source, Document::Token { nameStart, nameEnd }) };

    if (name.equalsIgnoreCase (Chars::urlFunction) and not cursor.isEmpty()
        and *cursor == Chars::openParen)
    {
        ++cursor;

        if (getUrlQuoted (cursor))
        {
            Document::Token token;
            token.type = map::CssTokenType::function;
            token.add<Document::Token> (Id::name, Document::Token { nameStart, nameEnd });
            return token;
        }

        return getUrlToken (cursor);
    }

    if (not cursor.isEmpty() and *cursor == Chars::openParen)
    {
        ++cursor;

        Document::Token token;
        token.type = map::CssTokenType::function;
        token.add<Document::Token> (Id::name, Document::Token { nameStart, nameEnd });
        return token;
    }

    Document::Token token;
    token.type = map::CssTokenType::ident;
    token.add<Document::Token> (Id::name, Document::Token { nameStart, nameEnd });
    return token;
}

Document::Token CssDocument::getRegionToken (Cursor& cursor)
{
    const auto ch { *cursor };
    ++cursor;

    jassert (ch == Chars::doubleQuote or ch == Chars::singleQuote);

    return getStringToken (cursor, ch);
}

Document::Token CssDocument::getHashOperatorToken (Cursor& cursor, juce::juce_wchar)
{
    ++cursor;

    const auto first { cursor.isEmpty() ? Document::eofCodePoint : *cursor };
    const Cursor secondCursor { cursor.source, cursor.getOffset() + 1 };
    const auto second { secondCursor.isEmpty() ? Document::eofCodePoint : *secondCursor };

    if (isIdentCodePoint (first) or isValidEscape (first, second))
    {
        const auto nameStart { cursor.getOffset() };
        getIdentSequence (cursor);

        Document::Token token;
        token.type = map::CssTokenType::hash;
        token.add<Document::Token> (Id::name, Document::Token { nameStart, cursor.getOffset() });
        return token;
    }

    Document::Token token;
    token.type = map::CssTokenType::delim;
    return token;
}

Document::Token CssDocument::getPlusOperatorToken (Cursor& cursor, juce::juce_wchar ch)
{
    const Cursor firstCursor { cursor.source, cursor.getOffset() + 1 };
    const auto first { firstCursor.isEmpty() ? Document::eofCodePoint : *firstCursor };
    const Cursor secondCursor { cursor.source, cursor.getOffset() + 2 };
    const auto second { secondCursor.isEmpty() ? Document::eofCodePoint : *secondCursor };

    if (wouldStartNumber (ch, first, second))
        return getNumericToken (cursor);

    ++cursor;
    Document::Token token;
    token.type = map::CssTokenType::delim;
    return token;
}

Document::Token CssDocument::getDashOperatorToken (Cursor& cursor, juce::juce_wchar ch)
{
    const Cursor firstCursor { cursor.source, cursor.getOffset() + 1 };
    const auto first { firstCursor.isEmpty() ? Document::eofCodePoint : *firstCursor };
    const Cursor secondCursor { cursor.source, cursor.getOffset() + 2 };
    const auto second { secondCursor.isEmpty() ? Document::eofCodePoint : *secondCursor };

    if (wouldStartNumber (ch, first, second))
        return getNumericToken (cursor);

    if (wouldStartIdentSequence (ch, first, second))
        return getIdentLikeToken (cursor);

    ++cursor;
    Document::Token token;
    token.type = map::CssTokenType::delim;
    return token;
}

Document::Token CssDocument::getDotOperatorToken (Cursor& cursor, juce::juce_wchar ch)
{
    const Cursor firstCursor { cursor.source, cursor.getOffset() + 1 };
    const auto first { firstCursor.isEmpty() ? Document::eofCodePoint : *firstCursor };
    const Cursor secondCursor { cursor.source, cursor.getOffset() + 2 };
    const auto second { secondCursor.isEmpty() ? Document::eofCodePoint : *secondCursor };

    if (wouldStartNumber (ch, first, second))
        return getNumericToken (cursor);

    ++cursor;
    Document::Token token;
    token.type = map::CssTokenType::delim;
    return token;
}

Document::Token CssDocument::getAtKeywordOperatorToken (Cursor& cursor, juce::juce_wchar)
{
    ++cursor;

    const auto first { cursor.isEmpty() ? Document::eofCodePoint : *cursor };
    const Cursor secondCursor { cursor.source, cursor.getOffset() + 1 };
    const auto second { secondCursor.isEmpty() ? Document::eofCodePoint : *secondCursor };
    const Cursor thirdCursor { cursor.source, cursor.getOffset() + 2 };
    const auto third { thirdCursor.isEmpty() ? Document::eofCodePoint : *thirdCursor };

    if (wouldStartIdentSequence (first, second, third))
    {
        const auto nameStart { cursor.getOffset() };
        getIdentSequence (cursor);

        Document::Token token;
        token.type = map::CssTokenType::atKeyword;
        token.add<Document::Token> (Id::name, Document::Token { nameStart, cursor.getOffset() });
        return token;
    }

    Document::Token token;
    token.type = map::CssTokenType::delim;
    return token;
}

Document::Token CssDocument::getOperatorToken (Cursor& cursor)
{
    static const auto operators {
        []
        {
            jam::Function::Map<juce::juce_wchar, Document::Token> operators;

            auto singleCharToken {
                [] (int tokenType)
                {
                    return [tokenType] (Cursor& cursor, juce::juce_wchar)
                    {
                        ++cursor;
                        Document::Token token;
                        token.type = tokenType;
                        return token;
                    };
                }
            };

            operators.add<Cursor&, juce::juce_wchar&> (
                Chars::openParen, singleCharToken (map::CssTokenType::openParen));
            operators.add<Cursor&, juce::juce_wchar&> (
                Chars::closeParen, singleCharToken (map::CssTokenType::closeParen));
            operators.add<Cursor&, juce::juce_wchar&> (
                Chars::comma, singleCharToken (map::CssTokenType::comma));
            operators.add<Cursor&, juce::juce_wchar&> (
                Chars::colon, singleCharToken (map::CssTokenType::colon));
            operators.add<Cursor&, juce::juce_wchar&> (
                Chars::semicolon, singleCharToken (map::CssTokenType::semicolon));
            operators.add<Cursor&, juce::juce_wchar&> (
                Chars::openBracket, singleCharToken (map::CssTokenType::openBracket));
            operators.add<Cursor&, juce::juce_wchar&> (
                Chars::closeBracket, singleCharToken (map::CssTokenType::closeBracket));
            operators.add<Cursor&, juce::juce_wchar&> (
                Chars::openBrace, singleCharToken (map::CssTokenType::openBrace));
            operators.add<Cursor&, juce::juce_wchar&> (
                Chars::closeBrace, singleCharToken (map::CssTokenType::closeBrace));

            operators.add<Cursor&, juce::juce_wchar&> (Chars::hash,
                                [] (Cursor& cursor, juce::juce_wchar ch)
                                {
                                    return getHashOperatorToken (cursor, ch);
                                });

            operators.add<Cursor&, juce::juce_wchar&> (Chars::plus,
                                [] (Cursor& cursor, juce::juce_wchar ch)
                                {
                                    return getPlusOperatorToken (cursor, ch);
                                });

            operators.add<Cursor&, juce::juce_wchar&> (Chars::dash,
                                [] (Cursor& cursor, juce::juce_wchar ch)
                                {
                                    return getDashOperatorToken (cursor, ch);
                                });

            operators.add<Cursor&, juce::juce_wchar&> (Chars::dot,
                                [] (Cursor& cursor, juce::juce_wchar ch)
                                {
                                    return getDotOperatorToken (cursor, ch);
                                });

            // Less-than — CDO omitted per subset; always delim
            operators.add<Cursor&, juce::juce_wchar&> (
                Chars::lessThan,
                [] (Cursor& cursor, juce::juce_wchar)
                {
                    ++cursor;
                    Document::Token token;
                    token.type = map::CssTokenType::delim;
                    return token;
                });

            operators.add<Cursor&, juce::juce_wchar&> (Chars::at,
                                [] (Cursor& cursor, juce::juce_wchar ch)
                                {
                                    return getAtKeywordOperatorToken (cursor, ch);
                                });

            return operators;
        }()
    };

    auto ch { *cursor };

    if (operators.contains (ch))
        return operators.get (ch, cursor, ch);

    // Anything else is a delim
    ++cursor;
    Document::Token token;
    token.type = map::CssTokenType::delim;
    return token;
}

Document::Token CssDocument::getTextToken (Cursor& cursor)
{
    const auto ch { *cursor };

    // Whitespace
    if (Document::isWhitespace (ch))
    {
        ++cursor;

        while (not cursor.isEmpty() and Document::isWhitespace (*cursor))
            ++cursor;

        Document::Token token;
        token.type = map::CssTokenType::whitespace;
        return token;
    }

    // Backslash
    if (ch == Chars::backslash)
    {
        const Cursor nextCursor { cursor.source, cursor.getOffset() + 1 };
        const auto next { nextCursor.isEmpty() ? Document::eofCodePoint : *nextCursor };

        if (isValidEscape (ch, next))
            return getIdentLikeToken (cursor);

        ++cursor;
        Document::Token token;
        token.type = map::CssTokenType::delim;
        return token;
    }

    // Digit
    if (Document::isDigit (ch))
        return getNumericToken (cursor);

    // Ident-start
    if (isIdentStartCodePoint (ch))
        return getIdentLikeToken (cursor);

    // Anything else is a delim
    ++cursor;
    Document::Token token;
    token.type = map::CssTokenType::delim;
    return token;
}

int CssDocument::getAtRuleType (const juce::String& name)
{
    return map::AtRuleType::getInstance()->contains (name) ? map::AtRuleType::getInstance()->get (name) : map::CssRuleType::unknownRule;
}

bool CssDocument::isAtRuleToken (const jam::Array<Document::Token>& tokens, int position)
{
    return tokens.at (position).type == map::CssTokenType::atKeyword;
}

juce::String
CssDocument::getAtKeywordName (const std::string& source, const jam::Array<Document::Token>& tokens, int position)
{
    return getText (source, *tokens.at (position).get<Document::Token> (Id::name));
}

int CssDocument::getPreludeEnd (const jam::Array<Document::Token>& tokens, int end, int position, bool isAtRule)
{
    while (position < end and tokens.at (position).type != map::CssTokenType::openBrace
           and tokens.at (position).type != map::CssTokenType::endOfFile
           and not(isAtRule and tokens.at (position).type == map::CssTokenType::semicolon))
        ++position;

    return position;
}

int CssDocument::getRuleBlock (const jam::Array<Document::Token>& tokens, int blockStart, int end)
{
    int depth { 1 };
    int blockEnd { blockStart };

    while (blockEnd < end and depth > 0)
    {
        const auto blockType { tokens.at (blockEnd).type };

        if (blockType == map::CssTokenType::endOfFile)
            break;

        if (blockType == map::CssTokenType::openBrace)
            ++depth;
        else if (blockType == map::CssTokenType::closeBrace)
            --depth;

        if (depth > 0)
            ++blockEnd;
    }

    return blockEnd;
}

void CssDocument::addRule (const std::string& source,
                           jam::Array<Document::Token>& tokens,
                           int blockStart,
                           int blockEnd,
                           bool isAtRule,
                           const juce::String& atKeywordName,
                           const jam::Array<int>& preludeTokens,
                           jam::Array<Rule>& rules)
{
    Rule rule;
    rule.selectorText = isAtRule ? atKeywordName : toText (source, tokens, preludeTokens);
    rule.type = isAtRule ? getAtRuleType (atKeywordName) : map::CssRuleType::styleRule;

    if (rule.type == map::CssRuleType::mediaRule)
    {
        rule.cssRules = getRules (source, tokens, blockStart, blockEnd);

        if (rule.selectorText.isNotEmpty() or rule.style.size() > 0
            or rule.cssRules.size() > 0)
            rules.add (std::move (rule));

        return;
    }

    auto declarations { getDeclarations (source, tokens, blockStart, blockEnd) };

    if (not isAtRule)
    {
        addSelectorRules (source, tokens, preludeTokens, declarations, rule.type, rules);
        return;
    }

    rule.style = std::move (declarations);

    if (rule.selectorText.isNotEmpty() or rule.style.size() > 0)
        rules.add (std::move (rule));
}

void CssDocument::addSelectorRule (const std::string& source,
                                   const jam::Array<Document::Token>& tokens,
                                   const jam::Array<int>& segment,
                                   const jam::Array<Declaration>& declarations,
                                   int ruleType,
                                   jam::Array<Rule>& rules)
{
    Rule segmentRule;
    segmentRule.type = ruleType;
    segmentRule.selectorText = toText (source, tokens, segment);

    for (int index { 0 }; index < declarations.size(); ++index)
    {
        const auto& declaration { declarations.at (index) };

        jam::Array<int> value;

        for (int valueIndex { 0 }; valueIndex < declaration.value.size(); ++valueIndex)
            value.add (declaration.value.at (valueIndex));

        segmentRule.style.add ({ declaration.property, std::move (value), declaration.important });
    }

    if (segmentRule.selectorText.isNotEmpty() or segmentRule.style.size() > 0)
        rules.add (std::move (segmentRule));
}

void CssDocument::addSelectorRules (const std::string& source,
                                    const jam::Array<Document::Token>& tokens,
                                    const jam::Array<int>& preludeTokens,
                                    const jam::Array<Declaration>& declarations,
                                    int ruleType,
                                    jam::Array<Rule>& rules)
{
    jam::Array<int> segment;

    for (int index { 0 }; index < preludeTokens.size(); ++index)
    {
        if (tokens.at (preludeTokens.at (index)).type == map::CssTokenType::comma)
        {
            addSelectorRule (source, tokens, segment, declarations, ruleType, rules);
            segment = {};
            continue;
        }

        segment.add (preludeTokens.at (index));
    }

    addSelectorRule (source, tokens, segment, declarations, ruleType, rules);
}

jam::Array<CssDocument::Rule> CssDocument::getRules (const std::string& source, jam::Array<Document::Token>& tokens, int start, int end)
{
    jam::Array<Rule> rules;
    int position { start };

    while (position < end and tokens.at (position).type != map::CssTokenType::endOfFile)
    {
        if (tokens.at (position).type == map::CssTokenType::whitespace)
        {
            ++position;
            continue;
        }

        const auto isAtRule { isAtRuleToken (tokens, position) };
        const auto atKeywordName { isAtRule ? getAtKeywordName (source, tokens, position) : juce::String() };
        const auto preludeEnd { getPreludeEnd (tokens, end, position, isAtRule) };

        jam::Array<int> preludeTokens;

        for (int index { position }; index < preludeEnd; ++index)
            preludeTokens.add (index);

        position = preludeEnd;

        if (position >= end or tokens.at (position).type == map::CssTokenType::endOfFile)
            break;

        if (isAtRule and tokens.at (position).type == map::CssTokenType::semicolon)
        {
            ++position;
            continue;
        }

        const int blockStart { position + 1 };
        const int blockEnd { getRuleBlock (tokens, blockStart, end) };

        addRule (source, tokens, blockStart, blockEnd, isAtRule, atKeywordName, preludeTokens, rules);

        position = blockEnd + 1;
    }

    return rules;
}

int CssDocument::getDeclarationEnd (const jam::Array<Document::Token>& tokens, int start, int end)
{
    int position { start };

    while (position < end and tokens.at (position).type != map::CssTokenType::semicolon)
        ++position;

    return position;
}

jam::Array<CssDocument::Declaration>
CssDocument::getDeclarations (const std::string& source, jam::Array<Document::Token>& tokens, int start, int end)
{
    jam::Array<Declaration> declarations;
    int position { start };

    while (position < end)
    {
        if (tokens.at (position).type == map::CssTokenType::whitespace
            or tokens.at (position).type == map::CssTokenType::semicolon)
        {
            ++position;
            continue;
        }

        if (tokens.at (position).type == map::CssTokenType::endOfFile)
            return declarations;

        if (tokens.at (position).type == map::CssTokenType::ident)
        {
            const auto declarationEnd { getDeclarationEnd (tokens, position, end) };
            auto declaration { getDeclaration (source, tokens, position, declarationEnd) };

            if (declaration.property.isNotEmpty())
                declarations.add (std::move (declaration));

            position = declarationEnd;
            continue;
        }

        while (position < end and tokens.at (position).type != map::CssTokenType::semicolon)
            ++position;
    }

    return declarations;
}

jam::Array<int> CssDocument::getDeclarationValue (const jam::Array<Document::Token>& tokens, int start, int end)
{
    jam::Array<int> value;

    for (int position { start };
         position < end and tokens.at (position).type != map::CssTokenType::endOfFile;
         ++position)
        value.add (position);

    return value;
}

int CssDocument::getImportantPriorityIndex (const std::string& source, const jam::Array<Document::Token>& tokens, const jam::Array<int>& value)
{
    int identPosition { value.size() - 1 };

    while (identPosition >= 0
           and tokens.at (value.at (identPosition)).type == map::CssTokenType::whitespace)
        --identPosition;

    if (identPosition < 0
        or tokens.at (value.at (identPosition)).type != map::CssTokenType::ident
        or not getText (source, *tokens.at (value.at (identPosition)).get<Document::Token> (Id::name))
                   .equalsIgnoreCase (Chars::important))
        return value.size();

    int delimPosition { identPosition - 1 };

    while (delimPosition >= 0
           and tokens.at (value.at (delimPosition)).type == map::CssTokenType::whitespace)
        --delimPosition;

    if (delimPosition < 0
        or tokens.at (value.at (delimPosition)).type != map::CssTokenType::delim
        or getText (source, tokens.at (value.at (delimPosition)))
               != juce::String::charToString (Chars::exclamation))
        return value.size();

    return delimPosition;
}

CssDocument::Declaration CssDocument::getDeclaration (const std::string& source, jam::Array<Document::Token>& tokens, int start, int end)
{
    jassert (tokens.at (start).type == map::CssTokenType::ident);

    const auto& propertyToken { tokens.at (start) };
    auto property { getText (source, *propertyToken.get<Document::Token> (Id::name)) };
    int position { start + 1 };

    while (position < end and tokens.at (position).type == map::CssTokenType::whitespace)
        ++position;

    if (position >= end or tokens.at (position).type != map::CssTokenType::colon)
    {
        tokens.at (start).add<bool> (Id::declaration, true);
        return {};
    }

    ++position;

    while (position < end and tokens.at (position).type == map::CssTokenType::whitespace)
        ++position;

    auto value { getDeclarationValue (tokens, position, end) };

    const auto priorityIndex { getImportantPriorityIndex (source, tokens, value) };
    const auto important { priorityIndex < value.size() };

    while (value.size() > priorityIndex)
        value.remove (value.size() - 1);

    while (value.size() > 0
           and tokens.at (value.at (value.size() - 1)).type == map::CssTokenType::whitespace)
    {
        value.remove (value.size() - 1);
    }

    if (value.size() == 0)
    {
        tokens.at (start).add<bool> (Id::declaration, true);
        return {};
    }

    return { property, std::move (value), important };
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
