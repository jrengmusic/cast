namespace jam
{
/*____________________________________________________________________________*/

const Document& HtmlDocument::getOrCreate (const juce::Identifier& name)
{
    auto* registry { SharedDocuments::getInstance() };
    jassert (registry != nullptr);

    if (registry->contains (name))
        return *registry->at (name);

    using namespace BinaryData;
    Raw binary (name.toString());

    auto document { std::make_unique<HtmlDocument>() };
    document->Document::parse (binary.data, binary.size);

    auto& ref { *document };
    registry->try_emplace (name, std::move (document));
    return ref;
}

int HtmlDocument::getToken (Cursor& cursor, int segmentType)
{
    if (segmentType == map::DocumentTokenType::comment)
    {
        Document::Token token { getCommentToken (Cursor { cursor }) };
        const auto length { static_cast<int> (token.length()) };
        tokens.add (std::move (token));
        return length;
    }

    if (segmentType == map::DocumentTokenType::operators and *cursor == Chars::lessThan)
        if (auto markupToken { getMarkupToken (Cursor { cursor }) })
        {
            const auto length { static_cast<int> (markupToken->length()) };
            tokens.add (std::move (*markupToken));
            return length;
        }

    Document::Token token { getText (Cursor { cursor }) };
    const auto length { static_cast<int> (token.length()) };
    tokens.add (std::move (token));
    return length;
}

void HtmlDocument::build()
{
    Document::Token endOfFile;
    endOfFile.type = map::HtmlTokenType::endOfFile;
    tokens.add (std::move (endOfFile));

    HtmlDocument tree;

    if (buildTree (tree, tokens, getSource()))
    {
        root = appendChild (*root, std::move (tree));
        root->parent = nullptr;
    }
}

const jam::Array<juce::Identifier>& HtmlDocument::getStructuralKeys()
{
    static const jam::Array<juce::Identifier> keys { Id::text, Extensions::svg, Id::root, Id::endTag };
    return keys;
}

const Document::Vocabulary& HtmlDocument::getHtmlVocabulary()
{
    static const Document::Vocabulary vocabulary { map::markupLanguage,
        Chars::markupCommentOpen, Chars::doubleDashChevronRight };

    return vocabulary;
}

Document::Token HtmlDocument::getCommentToken (Cursor cursor)
{
    const auto& vocabulary { getHtmlVocabulary() };
    const auto start { cursor.getOffset() };
    auto probe { cursor };

    while (not probe.isEmpty() and not probe.startsWith (vocabulary.commentClose))
        ++probe;

    if (not probe.isEmpty())
        probe += static_cast<int> (vocabulary.commentClose.size());

    return getComment (start, probe.getOffset());
}

std::optional<Document::Token> HtmlDocument::getMarkupToken (Cursor cursor)
{
    static const auto markupOperators { [] {
        jam::Function::Map<juce::String, Document::Token> markupOperators;
        markupOperators.add<Cursor> (Chars::endTagOpen, &getEndTag);
        markupOperators.add<Cursor> (Chars::declarationOpen, &getDoctype);
        return markupOperators;
    } () };

    for (const auto& [operatorPrefix, operatorHandler] : markupOperators)
        if (cursor.startsWith (std::string_view (
                operatorPrefix.toRawUTF8(), static_cast<size_t> (operatorPrefix.getNumBytesAsUTF8()))))
            return markupOperators.get (operatorPrefix, Cursor { cursor });

    auto probe { cursor };
    ++probe;

    if (not probe.isEmpty() and Document::isLetter (*probe))
        return getStartTag (Cursor { cursor });

    return std::nullopt;
}

std::optional<juce::juce_wchar> HtmlDocument::getCharacterReferenceValue (Cursor& cursor, bool isHexadecimal)
{
    juce::juce_wchar value { Chars::nullCharacter };
    bool hasDigits { false };

    if (isHexadecimal)
    {
        while (not cursor.isEmpty() and Document::isHexDigit (*cursor))
        {
            hasDigits = true;
            value = value * Document::hexadecimalRadix
                  + static_cast<juce::juce_wchar> (
                        juce::CharacterFunctions::getHexDigitValue (*cursor));
            ++cursor;
        }
    }
    else
    {
        while (not cursor.isEmpty() and Document::isDigit (*cursor))
        {
            hasDigits = true;
            value = value * Document::decimalRadix + (*cursor - Chars::zero);
            ++cursor;
        }
    }

    if (hasDigits)
        return value;

    return std::nullopt;
}

void HtmlDocument::appendNumericCharacterReference (std::string& result, Cursor& cursor)
{
    const auto afterHash { cursor.getOffset() };
    bool isHexadecimal { false };

    if (not cursor.isEmpty() and (*cursor == Chars::lowerX or *cursor == Chars::upperX))
    {
        isHexadecimal = true;
        ++cursor;
    }

    auto characterValue { getCharacterReferenceValue (cursor, isHexadecimal) };

    if (not characterValue.has_value())
    {
#if JUCE_DEBUG
        debug::Log::write ("appendNumericCharacterReference: numeric reference with no digits");
#endif
        result += static_cast<char> (Chars::ampersand);
        result += static_cast<char> (Chars::hash);
        cursor += static_cast<int> (afterHash - cursor.getOffset());
        return;
    }

    if (not cursor.isEmpty() and *cursor == Chars::semicolon)
        ++cursor;

    auto value { *characterValue };

    if (value == Chars::nullCharacter
        or (value >= Chars::surrogateRangeStart and value <= Chars::surrogateRangeEnd)
        or value > Chars::maximumCodePoint)
    {
        value = Chars::replacementCharacter;
    }

    const auto encoded { juce::String::charToString (static_cast<juce::juce_wchar> (value)) };
    result.append (encoded.toRawUTF8(), static_cast<size_t> (encoded.getNumBytesAsUTF8()));
}

void HtmlDocument::appendNamedCharacterReference (std::string& result, Cursor& cursor)
{
    const auto nameStart { cursor.getOffset() };

    while (not cursor.isEmpty() and Document::isLetter (*cursor))
        ++cursor;

    const auto nameEnd { cursor.getOffset() };

    if (not cursor.isEmpty() and *cursor == Chars::semicolon)
    {
        const auto& entities { map::entities };
        const auto name { juce::String::fromUTF8 (cursor.source.data() + nameStart, static_cast<int> (nameEnd - nameStart)) };
        const auto replacement { entities.contains (name) ? entities.at (name) : juce::String() };
        ++cursor;

        if (replacement.isNotEmpty())
        {
            result.append (replacement.toRawUTF8(), static_cast<size_t> (replacement.getNumBytesAsUTF8()));
            return;
        }

#if JUCE_DEBUG
        debug::Log::write ("appendNamedCharacterReference: unknown named entity");
#endif
        result += static_cast<char> (Chars::ampersand);
        result.append (cursor.source, nameStart, nameEnd - nameStart);
        result += static_cast<char> (Chars::semicolon);
        return;
    }

#if JUCE_DEBUG
    debug::Log::write ("appendNamedCharacterReference: unterminated named entity");
#endif
    result += static_cast<char> (Chars::ampersand);
    result.append (cursor.source, nameStart, nameEnd - nameStart);
}

void HtmlDocument::appendCharacterReference (std::string& result, Cursor& cursor)
{
    ++cursor;

    if (cursor.isEmpty())
    {
        result += static_cast<char> (Chars::ampersand);
        return;
    }

    if (*cursor == Chars::hash)
    {
        ++cursor;
        appendNumericCharacterReference (result, cursor);
        return;
    }

    appendNamedCharacterReference (result, cursor);
}

Document::Token HtmlDocument::getText (Cursor cursor)
{
    const auto start { cursor.getOffset() };

    if (not cursor.isEmpty() and *cursor == Chars::lessThan)
    {
#if JUCE_DEBUG
        debug::Log::write ("getText: unexpected character after <");
#endif
        ++cursor;
    }

    while (not cursor.isEmpty() and *cursor != Chars::lessThan)
        ++cursor;

    return Document::Token { start, cursor.getOffset(), map::HtmlTokenType::character };
}

juce::String HtmlDocument::getText (const std::string& source, size_t start, size_t end)
{
    std::string result;
    result.reserve (end - start);

    Cursor cursor { source, start };
    auto runStart { cursor.getOffset() };

    while (cursor.getOffset() < end)
    {
        if (*cursor != Chars::ampersand)
        {
            ++cursor;
            continue;
        }

        result.append (source, runStart, cursor.getOffset() - runStart);
        appendCharacterReference (result, cursor);
        runStart = cursor.getOffset();
    }

    result.append (source, runStart, cursor.getOffset() - runStart);

    return juce::String::fromUTF8 (result.data(), static_cast<int> (result.size()));
}

Document::Token HtmlDocument::getTagName (Cursor cursor)
{
    const auto nameStart { cursor.getOffset() };

    while (not cursor.isEmpty()
           and not Document::isWhitespace (*cursor)
           and *cursor != Chars::slash
           and *cursor != Chars::greaterThan)
        ++cursor;

    return Document::Token { nameStart, cursor.getOffset() };
}

jam::Array<Document::Property<Document::Token>>* HtmlDocument::getOrCreateAttributes (Document::Token& token)
{
    using Attributes = jam::Array<Document::Property<Document::Token>>;

    if (not token.contains (Id::attributes))
        token.add<Attributes> (Id::attributes);

    return token.get<Attributes> (Id::attributes);
}

juce::String HtmlDocument::getAttributeName (Cursor& cursor, const Document::Token& token)
{
    const auto attributeNameStart { cursor.getOffset() };

    while (not cursor.isEmpty() and *cursor != Chars::equals
           and not Document::isWhitespace (*cursor)
           and *cursor != Chars::greaterThan
           and *cursor != Chars::slash)
        ++cursor;

    const auto attributeName { getText (cursor.source, attributeNameStart, cursor.getOffset()) };

#if JUCE_DEBUG
    if (attributeName.isEmpty())
    {
        const auto* nameToken { token.get<Document::Token> (Id::name) };
        debug::Log::write ("addAttribute: empty attribute name on <"
                            + getText (cursor.source, nameToken->offset(), nameToken->offset() + nameToken->length())
                            + ">");
    }
#endif

    return attributeName;
}

Document::Token HtmlDocument::getAttributeValue (Cursor& cursor, bool quoted, juce::juce_wchar quoteCharacter, const Document::Token& token)
{
    const auto valueStart { cursor.getOffset() };

    if (quoted)
    {
        while (not cursor.isEmpty() and *cursor != quoteCharacter)
            ++cursor;
    }
    else
    {
        while (not cursor.isEmpty() and not Document::isWhitespace (*cursor) and *cursor != Chars::greaterThan)
            ++cursor;
    }

    const auto valueEnd { cursor.getOffset() };

    if (cursor.isEmpty())
    {
#if JUCE_DEBUG
        const auto* nameToken { token.get<Document::Token> (Id::name) };
        debug::Log::write ("addAttribute: unexpected EOF in attribute value on <"
                            + getText (cursor.source, nameToken->offset(), nameToken->offset() + nameToken->length())
                            + ">");
#endif
        return Document::Token { valueStart, valueEnd };
    }

    if (quoted)
        ++cursor;

    return Document::Token { valueStart, valueEnd };
}

Document::Token HtmlDocument::getQuotedValue (Cursor& cursor, const Document::Token& token)
{
    while (not cursor.isEmpty() and Document::isWhitespace (*cursor))
        ++cursor;

    if (cursor.isEmpty())
    {
#if JUCE_DEBUG
        const auto* nameToken { token.get<Document::Token> (Id::name) };
        debug::Log::write ("addAttribute: unexpected EOF before attribute value on <"
                            + getText (cursor.source, nameToken->offset(), nameToken->offset() + nameToken->length())
                            + ">");
#endif
        return Document::Token { cursor.getOffset(), cursor.getOffset() };
    }

    if (*cursor == Chars::doubleQuote or *cursor == Chars::singleQuote)
    {
        const auto quoteCharacter { *cursor };
        ++cursor;
        return getAttributeValue (cursor, true, quoteCharacter, token);
    }

    return getAttributeValue (cursor, false, Chars::nullCharacter, token);
}

void HtmlDocument::addEmptyAttribute (Cursor& cursor, Document::Token& token, const juce::String& attributeName)
{
    if (attributeName.isNotEmpty())
        getOrCreateAttributes (token)->add ({ juce::Identifier { attributeName }, Document::Token { cursor.getOffset(), cursor.getOffset() } });
}

void HtmlDocument::addAttribute (Cursor& cursor, Document::Token& token)
{
    const auto attributeName { getAttributeName (cursor, token) };

    if (cursor.isEmpty())
    {
#if JUCE_DEBUG
        const auto* nameToken { token.get<Document::Token> (Id::name) };
        debug::Log::write ("addAttribute: unexpected EOF in attribute name on <"
                            + getText (cursor.source, nameToken->offset(), nameToken->offset() + nameToken->length())
                            + ">");
#endif
        addEmptyAttribute (cursor, token, attributeName);
        return;
    }

    while (not cursor.isEmpty() and Document::isWhitespace (*cursor))
        ++cursor;

    if (cursor.isEmpty())
    {
#if JUCE_DEBUG
        const auto* nameToken { token.get<Document::Token> (Id::name) };
        debug::Log::write ("addAttribute: unexpected EOF after attribute name on <"
                            + getText (cursor.source, nameToken->offset(), nameToken->offset() + nameToken->length())
                            + ">");
#endif
        addEmptyAttribute (cursor, token, attributeName);
        return;
    }

    if (*cursor != Chars::equals)
    {
        addEmptyAttribute (cursor, token, attributeName);
        return;
    }

    ++cursor;

    auto value { getQuotedValue (cursor, token) };

    if (attributeName.isNotEmpty())
        getOrCreateAttributes (token)->add ({ juce::Identifier { attributeName }, std::move (value) });
}

void HtmlDocument::getTagAttributes (Cursor& cursor, Document::Token& token)
{
    for (;;)
    {
        while (not cursor.isEmpty() and Document::isWhitespace (*cursor))
            ++cursor;

        if (cursor.isEmpty())
        {
#if JUCE_DEBUG
            debug::Log::write ("getStartTag: unexpected EOF before attribute name");
#endif
            return;
        }

        if (*cursor == Chars::greaterThan)
        {
            ++cursor;
            return;
        }

        if (*cursor == Chars::slash)
        {
            ++cursor;

            if (not cursor.isEmpty() and *cursor == Chars::greaterThan)
            {
                ++cursor;
                token.add<bool> (Id::selfClosing, true);
                return;
            }

#if JUCE_DEBUG
            debug::Log::write ("getStartTag: expected > after /");
#endif
            continue;
        }

        addAttribute (cursor, token);
    }
}

Document::Token HtmlDocument::getStartTag (Cursor cursor)
{
    const auto start { cursor.getOffset() };
    ++cursor;

    Document::Token token;
    token.type = map::HtmlTokenType::startTag;

    auto nameToken { getTagName (Cursor { cursor }) };
    cursor += static_cast<int> (nameToken.length());
    token.add<Document::Token> (Id::name, std::move (nameToken));

    if (cursor.isEmpty())
    {
#if JUCE_DEBUG
        debug::Log::write ("getStartTag: unexpected EOF in tag name");
#endif
        token.setSpan (start, cursor.getOffset());
        return token;
    }

    getTagAttributes (cursor, token);

    token.setSpan (start, cursor.getOffset());
    return token;
}

Document::Token HtmlDocument::getEndTag (Cursor cursor)
{
    const auto start { cursor.getOffset() };
    cursor += endTagOpenLength;

    Document::Token token;
    token.type = map::HtmlTokenType::endTag;

    if (cursor.isEmpty() or not Document::isLetter (*cursor))
    {
#if JUCE_DEBUG
        debug::Log::write ("getEndTag: expected letter after </");
#endif
        while (not cursor.isEmpty() and *cursor != Chars::greaterThan)
            ++cursor;

        if (not cursor.isEmpty())
            ++cursor;

        token.setSpan (start, cursor.getOffset());
        return token;
    }

    // ==== tag name state ====

    const auto nameStart { cursor.getOffset() };

    while (not cursor.isEmpty()
           and not Document::isWhitespace (*cursor)
           and *cursor != Chars::greaterThan)
        ++cursor;

    token.add<Document::Token> (Id::name, Document::Token { nameStart, cursor.getOffset() });

    if (cursor.isEmpty())
    {
#if JUCE_DEBUG
        debug::Log::write ("getEndTag: unexpected EOF in tag name");
#endif
        token.setSpan (start, cursor.getOffset());
        return token;
    }

    // ==== discard to end of tag ====

    while (not cursor.isEmpty() and *cursor != Chars::greaterThan)
        ++cursor;

    if (not cursor.isEmpty())
    {
        ++cursor;
    }
#if JUCE_DEBUG
    else
    {
        debug::Log::write ("getEndTag: unexpected EOF before >");
    }
#endif

    token.setSpan (start, cursor.getOffset());
    return token;
}

Document::Token HtmlDocument::getComment (size_t start, size_t end)
{
    return Document::Token { start, end, map::HtmlTokenType::comment };
}

Document::Token HtmlDocument::getDoctype (Cursor cursor)
{
    const auto start { cursor.getOffset() };
    cursor += declarationOpenLength;

    while (not cursor.isEmpty() and *cursor != Chars::greaterThan)
        ++cursor;

    if (not cursor.isEmpty())
    {
        ++cursor;
    }
#if JUCE_DEBUG
    else
    {
        debug::Log::write ("getDoctype: unexpected EOF in doctype");
    }
#endif

    return Document::Token { start, cursor.getOffset(), map::HtmlTokenType::doctype };
}

bool HtmlDocument::isVoidTag (const juce::String& name) noexcept
{
    return map::HtmlVoidTag::getInstance()->contains (name);
}

bool HtmlDocument::addAttributes (const std::string& source, Document::Element& element, const Document::Token& token)
{
    if (token.contains (Id::attributes))
        for (const auto& [attributeKey, attributeValue] : *token.get<jam::Array<Document::Property<Document::Token>>> (Id::attributes))
        {
            if (Id::style != attributeKey)
            {
                if (element.contains (attributeKey) or getStructuralKeys().contains (attributeKey))
                {
#if JUCE_DEBUG
                    const auto* nameToken { token.get<Document::Token> (Id::name) };
                    debug::Log::write ("addAttributes: colliding attribute \""
                                        + attributeKey.toString()
                                        + "\" on element <"
                                        + getText (source, nameToken->offset(), nameToken->offset() + nameToken->length())
                                        + ">");
#endif
                    return false;
                }

                element.add<juce::String> (attributeKey, getText (source, attributeValue.offset(), attributeValue.offset() + attributeValue.length()));
            }
        }

    return true;
}

juce::String HtmlDocument::getUnit (const Css::StyleDeclaration& style, const juce::String& property)
{
    return style.getPropertyType (property) == map::CssTokenType::dimension
               ? style.getUnit (property)
               : juce::String::charToString (Chars::percent);
}

bool HtmlDocument::addStyleDeclaration (const std::string& source,
                                        Document::Element& element,
                                        const Document::Token& token,
                                        const Css::StyleDeclaration& style,
                                        const juce::String& property,
                                        const jam::Function::Map<int, bool>& styleAttributes)
{
    const juce::Identifier propertyKey { property };

    if (element.contains (propertyKey) or getStructuralKeys().contains (propertyKey))
    {
#if JUCE_DEBUG
        const auto* nameToken { token.get<Document::Token> (Id::name) };
        debug::Log::write ("addStyleAttributes: colliding attribute \""
                            + property
                            + "\" on element <"
                            + getText (source, nameToken->offset(), nameToken->offset() + nameToken->length())
                            + ">");
#endif
        return false;
    }

    const auto variable { style.getVariable (property) };

    if (variable.isNotEmpty())
    {
        element.add<juce::String> (propertyKey, variable);
        return true;
    }

    const auto type { style.getPropertyType (property) };

    if (styleAttributes.contains (type))
        return styleAttributes.get (type, source, element, token, style, property);

    return true;
}

bool HtmlDocument::addStyleAttributes (const std::string& source, Document::Element& element, const Document::Token& token, const juce::String& styleText)
{
    static const auto styleAttributes { [] {
        jam::Function::Map<int, bool> styleAttributes;

        auto addTextAttribute { [] (const std::string&,
                                  Document::Element& element,
                                  const Document::Token&,
                                  const Css::StyleDeclaration& style,
                                  const juce::String& property)
        {
            element.add<juce::String> (juce::Identifier { property }, style.getPropertyValue (property));
            return true;
        } };

        for (const auto type : Css::textTokenTypes)
            styleAttributes.add<const std::string&, Document::Element&, const Document::Token&, const Css::StyleDeclaration&, const juce::String&>
                (type, addTextAttribute);

        auto addNumberAttribute { [] (const std::string&,
                                   Document::Element& element,
                                   const Document::Token&,
                                   const Css::StyleDeclaration& style,
                                   const juce::String& property)
        {
            element.add<double> (juce::Identifier { property }, style.getDoubleAttribute (property));
            return true;
        } };

        auto addUnitAttribute { [] (const std::string& source,
                                             Document::Element& element,
                                             const Document::Token& token,
                                             const Css::StyleDeclaration& style,
                                             const juce::String& property)
        {
            element.add<double> (juce::Identifier { property }, style.getDoubleAttribute (property));

            const auto unitAttribute { Format::appendWithDash (property, Id::unit.toString()) };

            if (element.contains (juce::Identifier { unitAttribute }))
            {
#if JUCE_DEBUG
                const auto* nameToken { token.get<Document::Token> (Id::name) };
                debug::Log::write ("addStyleAttributes: colliding attribute \""
                                    + unitAttribute
                                    + "\" on element <"
                                    + getText (source, nameToken->offset(), nameToken->offset() + nameToken->length())
                                    + ">");
#endif
                return false;
            }

            element.add<juce::String> (juce::Identifier { unitAttribute }, getUnit (style, property));
            return true;
        } };

        for (const auto type : Css::numericTokenTypes)
        {
            if (type == map::CssTokenType::number)
            {
                styleAttributes.add<const std::string&, Document::Element&, const Document::Token&, const Css::StyleDeclaration&, const juce::String&>
                    (type, addNumberAttribute);
            }
            else
            {
                styleAttributes.add<const std::string&, Document::Element&, const Document::Token&, const Css::StyleDeclaration&, const juce::String&>
                    (type, addUnitAttribute);
            }
        }

        return styleAttributes;
    } () };

    const auto style { Css::getStyle (styleText) };

    for (int index { 0 }; index < style.length(); ++index)
        if (not addStyleDeclaration (source, element, token, style, style.item (index), styleAttributes))
            return false;

    return true;
}

Document::Element* HtmlDocument::attachElement (Document& scratch, jam::Array<Document::Element*>& stack, const juce::Identifier& tagId)
{
    if (not stack.isEmpty())
        return scratch.addChild (*stack.last(), tagId);

    if (scratch.root->id.isValid())
    {
        if (not scratch.root->contains (Id::root))
            scratch.root->add<bool> (Id::root, true);

        return scratch.addChild (*scratch.root, tagId);
    }

    scratch.root->id = tagId;
    return scratch.root;
}

bool HtmlDocument::addText (const std::string& source, const Document::Token& token, jam::Array<Document::Element*>& stack)
{
    const auto text { getText (source, token.offset(), token.offset() + token.length()) };

    if (not text.trim().isEmpty())
    {
        if (stack.isEmpty())
        {
#if JUCE_DEBUG
            debug::Log::write ("addText: non-whitespace text before root element");
#endif
            return false;
        }

        auto& element { *stack.last() };

        if (element.contains (Id::text))
            *element.get<juce::String> (Id::text) += text;
        else
            element.add<juce::String> (Id::text, text);
    }

    return true;
}

int HtmlDocument::getSvgSpan (const jam::Array<Document::Token>& tokens, const std::string& source, int index)
{
    int depth { tokens.at (index).contains (Id::selfClosing) ? 0 : 1 };
    int position { index + 1 };

    while (position < tokens.size() and depth > 0)
    {
        const auto* nameToken { tokens.at (position).type == map::HtmlTokenType::startTag or tokens.at (position).type == map::HtmlTokenType::endTag
                                    ? tokens.at (position).get<Document::Token> (Id::name)
                                    : nullptr };

        const auto tagName { nameToken != nullptr
                                  ? getText (source, nameToken->offset(), nameToken->offset() + nameToken->length())
                                  : juce::String() };

        if (tokens.at (position).type == map::HtmlTokenType::startTag
            and tagName == Extensions::svg
            and not tokens.at (position).contains (Id::selfClosing))
            ++depth;
        else if (tokens.at (position).type == map::HtmlTokenType::endTag
                 and tagName == Extensions::svg)
            --depth;

        ++position;
    }

    return depth > 0 ? -1 : position;
}

juce::String HtmlDocument::getSvg (const jam::Array<Document::Token>& tokens,
                                   const std::string& source,
                                   int& index)
{
    const auto start { tokens.at (index).offset() };
    const auto position { getSvgSpan (tokens, source, index) };

    if (position < 0)
    {
#if JUCE_DEBUG
        debug::Log::write ("getSvg: unclosed elements at end of file");
#endif
        return {};
    }

    index = position - 1;

    const auto& closingToken { tokens.at (index) };
    const auto end { closingToken.offset() + closingToken.length() };

    return juce::String::fromUTF8 (source.data() + start, static_cast<int> (end - start));
}

bool HtmlDocument::addElementAttributes (const std::string& source, Document::Element& element, const Document::Token& token)
{
    if (not addAttributes (source, element, token))
        return false;

    const Document::Token* styleAttribute { nullptr };

    if (token.contains (Id::attributes))
        for (const auto& [attributeKey, attributeValue] : *token.get<jam::Array<Document::Property<Document::Token>>> (Id::attributes))
            if (attributeKey == Id::style)
                styleAttribute = &attributeValue;

    if (styleAttribute != nullptr)
        return addStyleAttributes (source, element, token, getText (source, styleAttribute->offset(), styleAttribute->offset() + styleAttribute->length()));

    return true;
}

bool HtmlDocument::addStartTagElement (const Document::Token& token, const jam::Array<Document::Token>& tokens, const std::string& source,
                                       int& index, jam::Array<Document::Element*>& stack, Document& scratch)
{
    const auto* nameToken { token.get<Document::Token> (Id::name) };
    const auto tagName { getText (source, nameToken->offset(), nameToken->offset() + nameToken->length()) };

    auto* element { attachElement (scratch, stack, juce::Identifier { tagName }) };

    if (not addElementAttributes (source, *element, token))
        return false;

    if (tagName == Extensions::svg)
    {
        const auto svg { getSvg (tokens, source, index) };

        if (svg.isEmpty())
            return false;

        element->add<juce::String> (Extensions::svg, svg);

        return true;
    }

    if (isVoidTag (tagName) or token.contains (Id::selfClosing))
        return true;

    stack.add (element);
    return true;
}

bool HtmlDocument::addEndTagElement (const Document::Token& token, const jam::Array<Document::Token>&, const std::string& source,
                                     int&, jam::Array<Document::Element*>& stack, Document&)
{
    const auto* nameToken { token.get<Document::Token> (Id::name) };
    const juce::Identifier endTagId { getText (source, nameToken->offset(), nameToken->offset() + nameToken->length()) };

    if (not stack.isEmpty())
    {
        if (stack.last()->id != endTagId and not stack.last()->contains (Id::endTag))
            stack.last()->add<bool> (Id::endTag, true);

        stack.remove (stack.size() - 1);
    }

    return true;
}

bool HtmlDocument::addCharacterElement (const Document::Token& token, const jam::Array<Document::Token>&, const std::string& source,
                                        int&, jam::Array<Document::Element*>& stack, Document&)
{
    return addText (source, token, stack);
}

bool HtmlDocument::skip (const Document::Token&, const jam::Array<Document::Token>&, const std::string&,
                         int&, jam::Array<Document::Element*>&, Document&)
{
    return true;
}

bool HtmlDocument::buildTree (Document& scratch, const jam::Array<Document::Token>& tokens, const std::string& source)
{
    static const auto treeConstruction { [] {
        jam::Function::Map<int, bool> treeConstruction;

        treeConstruction.add<const Document::Token&, const jam::Array<Document::Token>&, const std::string&,
                int&, jam::Array<Document::Element*>&, Document&> (map::HtmlTokenType::startTag, &addStartTagElement);
        treeConstruction.add<const Document::Token&, const jam::Array<Document::Token>&, const std::string&,
                int&, jam::Array<Document::Element*>&, Document&> (map::HtmlTokenType::endTag, &addEndTagElement);
        treeConstruction.add<const Document::Token&, const jam::Array<Document::Token>&, const std::string&,
                int&, jam::Array<Document::Element*>&, Document&> (map::HtmlTokenType::character, &addCharacterElement);
        treeConstruction.add<const Document::Token&, const jam::Array<Document::Token>&, const std::string&,
                int&, jam::Array<Document::Element*>&, Document&> (map::HtmlTokenType::comment, &skip);
        treeConstruction.add<const Document::Token&, const jam::Array<Document::Token>&, const std::string&,
                int&, jam::Array<Document::Element*>&, Document&> (map::HtmlTokenType::doctype, &skip);
        treeConstruction.add<const Document::Token&, const jam::Array<Document::Token>&, const std::string&,
                int&, jam::Array<Document::Element*>&, Document&> (map::HtmlTokenType::endOfFile, &skip);

        return treeConstruction;
    } () };

    jam::Array<Document::Element*> stack;

    for (int index { 0 }; index < tokens.size(); ++index)
    {
        const auto& token { tokens.at (index) };

        if (not treeConstruction.get (token.type, token, tokens, source, index, stack, scratch))
            return false;
    }

    if (stack.isEmpty() and scratch.root->id.isValid())
        return true;

#if JUCE_DEBUG
    debug::Log::write ("buildTree: incomplete document -- unclosed elements or no root element");
#endif
    return false;
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
