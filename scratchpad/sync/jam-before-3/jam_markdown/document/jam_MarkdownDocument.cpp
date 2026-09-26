namespace jam
{
/*____________________________________________________________________________*/

const MarkdownDocument& MarkdownDocument::getOrCreate (const juce::Identifier& name)
{
    auto* registry { SharedDocuments::getInstance() };
    jassert (registry != nullptr);

    if (registry->contains (name))
        return static_cast<const MarkdownDocument&> (*registry->at (name));

    using namespace BinaryData;
    Raw binary (name.toString());

    auto document { std::make_unique<MarkdownDocument>() };
    document->Document::parse (binary.data, binary.size);

    auto& ref { *document };
    registry->try_emplace (name, std::move (document));
    return ref;
}

MarkdownDocument MarkdownDocument::parse (std::string_view documentText)
{
    MarkdownDocument document;
    document.Document::parse (documentText.data(), static_cast<int> (documentText.size()));
    return document;
}

MarkdownDocument MarkdownDocument::parse (const juce::String& documentText)
{
    return parse (std::string_view { documentText.toRawUTF8(),
                                     static_cast<size_t> (documentText.getNumBytesAsUTF8()) });
}

MarkdownDocument MarkdownDocument::parse (const juce::String& documentText, const juce::String& origin)
{
    auto document { parse (documentText) };

    for (auto* child : *document.root)
        child->add<juce::String> (Id::path, origin);

    for (auto* table : *document.root)
        if (isTable (*table))
        {
            auto* headerRow { getTableHeaderRow (*table) };

            for (auto* row : *table)
                if (row != headerRow and row->contains (Id::offset))
                    row->add<int> (Id::line,
                                   static_cast<int> (document.getLineNumber (
                                       static_cast<uint32_t> (*row->get<int> (Id::offset)))));
        }

    return document;
}

jam::Array<const Document::Element*> MarkdownDocument::getTables() const
{
    jam::Array<const Element*> result;

    for (auto* child : *getRoot())
        if (isTable (*child))
            result.add (child);

    return result;
}

jam::Array<const Document::Element*> MarkdownDocument::getTables (const juce::Identifier& tableId) const
{
    jam::Array<const Element*> result;

    for (auto* child : *getRoot())
        if (child->id == tableId)
            result.add (child);

    return result;
}

bool MarkdownDocument::isTableBorder (const Element& element) noexcept
{
    return element.contains (Id::type)
           and *element.get<int> (Id::type) == map::BlockType::tableBorder;
}

bool MarkdownDocument::isTable (const Element& element) noexcept
{
    return element.contains (Id::type)
           and *element.get<int> (Id::type) == map::BlockType::table;
}

bool MarkdownDocument::isParagraph (std::string_view text)
{
    const auto document { parse (text) };
    const auto* firstChild { document.getRoot()->firstChild };

    return firstChild != nullptr and firstChild->nextSibling == nullptr
       and *firstChild->get<int> (Id::type) == map::BlockType::paragraph;
}

jam::Array<const Document::Element*> MarkdownDocument::getTableRows (const Element& table) const
{
    jam::Array<const Element*> result;

    if (auto* headerRow { getTableHeaderRow (table) })
        for (auto* row : table)
            if (row != headerRow and not isTableBorder (*row))
                result.add (row);

    return result;
}

jam::Array<juce::String> MarkdownDocument::getTableHeaders (const Element& table) const
{
    jam::Array<juce::String> result;

    if (auto* headerRow { getTableHeaderRow (table) })
        for (auto* cell : *headerRow)
            result.add (cell->id.toString());

    return result;
}

const Document::Element* MarkdownDocument::getTableRow (const Element& table, const juce::Identifier& rowId) const
{
    return getChildByID (table, rowId);
}

const Document::Element* MarkdownDocument::getTableCell (const Element& table,
                                                          const juce::Identifier& colId,
                                                          const juce::Identifier& rowId) const
{
    const Element* result { nullptr };

    if (auto* row { getTableRow (table, rowId) })
        result = getTableCell (*row, colId);

    return result;
}

juce::String MarkdownDocument::getTableValue (const Element& table,
                                              const juce::Identifier& colId,
                                              const juce::Identifier& rowId) const
{
    juce::String result;

    if (auto* cell { getTableCell (table, colId, rowId) })
        result = cell->getAllSubText();

    return result;
}

const Document::Element* MarkdownDocument::getTableCell (const Element& row, const juce::Identifier& colId) const
{
    return getChildByID (row, colId);
}

juce::String MarkdownDocument::getTableValue (const Element& row, const juce::Identifier& colId) const
{
    juce::String result;

    if (auto* cell { getTableCell (row, colId) })
        result = cell->getAllSubText();

    return result;
}

std::string_view MarkdownDocument::getTableValueView (const Element& row, const juce::Identifier& colId) const
{
    std::string_view result;

    if (auto* cell { getTableCell (row, colId) })
        result = getValueView (*cell);

    return result;
}

std::string_view MarkdownDocument::getValueView (const Element& element) const
{
    std::string_view result;

    if (element.contains (Id::tokens))
    {
        const auto& valueTokens { *element.get<jam::Array<Document::Token>> (Id::tokens) };
        const auto& valueToken { valueTokens.at (0) };
        result = std::string_view (getSource().data() + valueToken.offset(), valueToken.length());
    }

    return result;
}

const Document::Element* MarkdownDocument::getTableList (const Element& row, const juce::Identifier& colId) const
{
    const Element* result { nullptr };

    if (auto* cell { getTableCell (row, colId) })
        if (auto* quote { getBlockquote (*cell) })
            result = getList (*quote);

    return result;
}

std::string_view MarkdownDocument::getTableValueView (const Element& row,
                                                       const juce::Identifier& colId,
                                                       const juce::Identifier& itemId) const
{
    std::string_view result;

    if (auto* list { getTableList (row, colId) })
        if (auto* item { getListItem (*list, itemId) })
            result = getValueView (*item);

    return result;
}

bool MarkdownDocument::hasTableValue (const Element& row, const juce::Identifier& colId) const noexcept
{
    return getTableValue (row, colId).isNotEmpty();
}

jam::Array<juce::String> MarkdownDocument::getTableRowKeys (const juce::Identifier& tableId) const
{
    jam::Array<juce::String> result;

    if (auto* table { getTable (tableId) })
        for (auto* row : getTableRows (*table))
            result.add (row->id.toString());

    return result;
}

jam::Array<const Document::Element*> MarkdownDocument::getTableRows (const juce::Identifier& tableId) const
{
    jam::Array<const Element*> result;

    if (auto* table { getTable (tableId) })
        result = getTableRows (*table);

    return result;
}

jam::Array<juce::String> MarkdownDocument::getTableHeaders (const juce::Identifier& tableId) const
{
    jam::Array<juce::String> result;

    if (auto* table { getTable (tableId) })
        result = getTableHeaders (*table);

    return result;
}

const Document::Element* MarkdownDocument::getTableCell (const juce::Identifier& tableId,
                                                          const juce::Identifier& colId,
                                                          const juce::Identifier& rowId) const
{
    const Element* result { nullptr };

    if (auto* table { getTable (tableId) })
        result = getTableCell (*table, colId, rowId);

    return result;
}

const Document::Element* MarkdownDocument::getTableRow (const juce::Identifier& tableId, const juce::Identifier& rowId) const
{
    const Element* result { nullptr };

    if (auto* table { getTable (tableId) })
        result = getTableRow (*table, rowId);

    return result;
}

juce::String MarkdownDocument::getTableValue (const juce::Identifier& tableId,
                                              const juce::Identifier& colId,
                                              const juce::Identifier& rowId) const
{
    juce::String result;

    if (auto* table { getTable (tableId) })
        result = getTableValue (*table, colId, rowId);

    return result;
}

const Document::Element* MarkdownDocument::getCodeBlock (const juce::Identifier& codeId) const noexcept
{
    return getChildByID (*root, codeId);
}

const Document::Element* MarkdownDocument::getBlockquote (const Element& scope) const noexcept
{
    return getChildByID (scope, Id::blockquote);
}

const Document::Element* MarkdownDocument::getList (const Element& scope) const noexcept
{
    return getChildByID (scope, Id::ul);
}

const Document::Element* MarkdownDocument::getListItem (const Element& list, const juce::Identifier& itemId) const noexcept
{
    return getChildByID (list, itemId);
}

int MarkdownDocument::getToken (Cursor& cursor, int segmentType)
{
    const auto& source { cursor.source };
    const auto position { cursor.getOffset() };

    if ((*Document::Cursor { source, position })
        == Chars::newline)
    {
        auto end { position };
        ++end;
        tokens.add (Document::Token (position, end));
        return 1;
    }

    auto token { segmentType == map::DocumentTokenType::operators
                     ? getOperatorToken (source, position)
                     : getTextToken (source, position) };
    const auto length { static_cast<int> (token.length()) };
    tokens.add (std::move (token));
    return length;
}

void MarkdownDocument::build()
{
    root->id = map::BlockTag::getInstance()->get (map::BlockType::document);
    root->add<int> (Id::type, map::BlockType::document);

    BlockParser blocks { *this, *root };

    if (not tokens.isEmpty())
    {
        const auto& source { getSource() };
        auto lineStart { tokens.first().offset() };

        for (const auto& token : tokens)
        {
            if (token.length() == 1
                and (*Document::Cursor { source, token.offset() })
                        == Chars::newline)
            {
                const auto tokenStartSpan { token.offset() };
                blocks.addLine (lineStart, tokenStartSpan);
                lineStart = token.offset() + token.length();
            }
        }

        const auto lastTokenEnd { tokens.last().offset() + tokens.last().length() };

        if (lineStart != lastTokenEnd)
            blocks.addLine (lineStart, lastTokenEnd);

        blocks.closeLeaf();
    }

    InlineParser inlines { *this, blocks };
    inlines.addLeafText (*root);
}

const Document::Element* MarkdownDocument::getTable (const juce::Identifier& tableId) const noexcept
{
    return getChildByID (*root, tableId);
}

const Document::Element* MarkdownDocument::getTableHeaderRow (const Element& table) noexcept
{
    for (auto* row : table)
        if (row->isTag (Id::headerRow))
            return row;

    return nullptr;
}

bool MarkdownDocument::isMarkdownOperatorChar (juce::juce_wchar ch) noexcept
{
    return getMarkdownVocabulary().getClass (ch) == map::Byte::operators;
}

size_t MarkdownDocument::advance (size_t start, size_t end, int count) noexcept
{
    auto ptr { start };

    for (int i { 0 }; i < count and ptr != end; ++i)
        ++ptr;

    return ptr;
}

bool MarkdownDocument::isAsciiLetter (juce::juce_wchar ch) noexcept
{
    return (ch >= Chars::upperA and ch <= Chars::upperZ)
           or (ch >= Chars::lowerA and ch <= Chars::lowerZ);
}

bool MarkdownDocument::isTagNameChar (juce::juce_wchar ch) noexcept
{
    return isAsciiLetter (ch) or (ch >= Chars::zero and ch <= Chars::nine) or ch == Chars::dash;
}

juce::juce_wchar MarkdownDocument::charAt (const std::string& source, size_t start, size_t end, int index) noexcept
{
    const auto position { start + static_cast<size_t> (index) };
    return position < end ? (*Document::Cursor { source, position }) : 0;
}

int MarkdownDocument::getEndOfOpenTag (const std::string& source, size_t start, size_t end, int index)
{
    const auto nameEnd { getEndOfRawTagName (source, start, end, index) };

    if (nameEnd == index)
        return index;

    auto cursor { nameEnd };

    for (auto attrEnd { getEndOfRawAttribute (source, start, end, cursor) }; attrEnd != cursor;
         attrEnd = getEndOfRawAttribute (source, start, end, cursor))
        cursor = attrEnd;

    while (charAt (source, start, end, cursor) == Chars::space
           or charAt (source, start, end, cursor) == Chars::tab)
        ++cursor;

    if (charAt (source, start, end, cursor) == Chars::slash)
        ++cursor;

    return charAt (source, start, end, cursor) == Chars::greaterThan ? cursor + 1 : index;
}

int MarkdownDocument::getEndOfClosingTag (const std::string& source, size_t start, size_t end, int index)
{
    if (charAt (source, start, end, index) != Chars::slash)
        return index;

    const auto nameEnd { getEndOfRawTagName (source, start, end, index + 1) };

    if (nameEnd == index + 1)
        return index;

    auto cursor { nameEnd };

    while (charAt (source, start, end, cursor) == Chars::space
           or charAt (source, start, end, cursor) == Chars::tab)
        ++cursor;

    return charAt (source, start, end, cursor) == Chars::greaterThan ? cursor + 1 : index;
}

bool MarkdownDocument::isAsciiPunctuation (juce::juce_wchar ch) noexcept
{
    return (ch >= Chars::exclamation and ch <= Chars::slash)
           or (ch >= Chars::colon and ch <= Chars::at)
           or (ch >= Chars::openBracket and ch <= Chars::backtick)
           or (ch >= Chars::openBrace and ch <= Chars::tilde);
}

#if __has_include (<hb.h>)

bool MarkdownDocument::isHarfBuzzSpaceSeparator (juce::juce_wchar ch) noexcept
{
    return hb_unicode_general_category (
               hb_unicode_funcs_get_default(), static_cast<hb_codepoint_t> (ch))
           == HB_UNICODE_GENERAL_CATEGORY_SPACE_SEPARATOR;
}

bool MarkdownDocument::isUnicodeWhitespace (juce::juce_wchar ch) noexcept
{
    const auto isControlWhitespace { ch == Chars::tab or ch == Chars::newline
                                     or ch == Chars::formFeed or ch == Chars::carriageReturn };

    return isControlWhitespace or isHarfBuzzSpaceSeparator (ch);
}

bool MarkdownDocument::isHarfBuzzPunctuationOrSymbol (juce::juce_wchar ch) noexcept
{
    const auto category { hb_unicode_general_category (
        hb_unicode_funcs_get_default(), static_cast<hb_codepoint_t> (ch)) };

    return category >= HB_UNICODE_GENERAL_CATEGORY_CONNECT_PUNCTUATION
           and category <= HB_UNICODE_GENERAL_CATEGORY_OTHER_SYMBOL;
}

bool MarkdownDocument::isUnicodePunctuation (juce::juce_wchar ch) noexcept
{
    return ch <= maxAsciiCodepoint ? isAsciiPunctuation (ch)
                                   : isHarfBuzzPunctuationOrSymbol (ch);
}

#else

bool MarkdownDocument::isHarfBuzzSpaceSeparator (juce::juce_wchar) noexcept
{
    return false;
}

bool MarkdownDocument::isUnicodeWhitespace (juce::juce_wchar ch) noexcept
{
    return ch == Chars::tab or ch == Chars::newline
           or ch == Chars::formFeed or ch == Chars::carriageReturn or ch == Chars::space;
}

bool MarkdownDocument::isHarfBuzzPunctuationOrSymbol (juce::juce_wchar) noexcept
{
    return false;
}

bool MarkdownDocument::isUnicodePunctuation (juce::juce_wchar ch) noexcept
{
    return ch <= maxAsciiCodepoint and isAsciiPunctuation (ch);
}

#endif

int MarkdownDocument::getEndOfRawTagName (const std::string& source, size_t start, size_t end, int index)
{
    if (not isAsciiLetter (charAt (source, start, end, index)))
        return index;

    int cursor { index + 1 };

    while (isTagNameChar (charAt (source, start, end, cursor)))
        ++cursor;

    return cursor;
}

bool MarkdownDocument::isUnquotedAttributeChar (juce::juce_wchar ch) noexcept
{
    return ch != 0 and ch != Chars::space and ch != Chars::tab and ch != Chars::singleQuote
           and ch != Chars::doubleQuote and ch != Chars::equals and ch != Chars::lessThan
           and ch != Chars::greaterThan and ch != Chars::backtick;
}

int MarkdownDocument::getEndOfRawAttributeValue (const std::string& source, size_t start, size_t end, int index)
{
    const auto quote { charAt (source, start, end, index) };

    if (quote == Chars::singleQuote or quote == Chars::doubleQuote)
    {
        auto cursor { index + 1 };

        while (charAt (source, start, end, cursor) != 0 and charAt (source, start, end, cursor) != quote)
            ++cursor;

        return charAt (source, start, end, cursor) == quote ? cursor + 1 : index;
    }

    auto cursor { index };

    while (isUnquotedAttributeChar (charAt (source, start, end, cursor)))
        ++cursor;

    return cursor;
}

int MarkdownDocument::getEndOfRawAttribute (const std::string& source, size_t start, size_t end, int index)
{
    auto cursor { index };

    while (charAt (source, start, end, cursor) == Chars::space
           or charAt (source, start, end, cursor) == Chars::tab)
        ++cursor;

    const auto nameEnd { getEndOfRawTagName (source, start, end, cursor) };

    if (cursor == index or nameEnd == cursor)
        return index;

    cursor = nameEnd;
    const auto specEnd { cursor };

    while (charAt (source, start, end, cursor) == Chars::space
           or charAt (source, start, end, cursor) == Chars::tab)
        ++cursor;

    if (charAt (source, start, end, cursor) != Chars::equals)
        return specEnd;

    ++cursor;

    while (charAt (source, start, end, cursor) == Chars::space
           or charAt (source, start, end, cursor) == Chars::tab)
        ++cursor;

    const auto valueEnd { getEndOfRawAttributeValue (source, start, end, cursor) };
    return valueEnd != cursor ? valueEnd : index;
}

Document::Token
MarkdownDocument::getBracketedDestination (const std::string& source, size_t start, size_t end, int index)
{
    int cursor { index + 1 };
    juce::String text;

    while (charAt (source, start, end, cursor) != 0
           and charAt (source, start, end, cursor) != Chars::greaterThan)
    {
        const auto ch { charAt (source, start, end, cursor) };
        const auto escaped { ch == Chars::backslash
                             and isAsciiPunctuation (charAt (source, start, end, cursor + 1)) };

        if (ch == Chars::lessThan or ch == Chars::newline)
            return {};

        text += escaped ? charAt (source, start, end, cursor + 1) : ch;
        cursor += escaped ? 2 : 1;
    }

    if (charAt (source, start, end, cursor) == 0)
        return {};

    Document::Token token (
        start + static_cast<size_t> (index), start + static_cast<size_t> (cursor) + 1);
    token.add<juce::String> (Id::href, std::move (text));
    return token;
}

Document::Token MarkdownDocument::getBareDestination (const std::string& source, size_t start, size_t end, int index)
{
    int cursor { index };
    int parenDepth { 0 };
    juce::String text;

    while (charAt (source, start, end, cursor) != 0)
    {
        const auto ch { charAt (source, start, end, cursor) };
        const auto escaped { ch == Chars::backslash
                             and isAsciiPunctuation (charAt (source, start, end, cursor + 1)) };
        const auto stopper { not escaped
                             and (ch <= Chars::space or ch == Chars::deleteCharacter
                                  or (ch == Chars::closeParen and parenDepth == 0)) };

        if (stopper)
            break;

        parenDepth += (not escaped and ch == Chars::openParen) ? 1 : 0;
        parenDepth -= (not escaped and ch == Chars::closeParen) ? 1 : 0;
        text += escaped ? charAt (source, start, end, cursor + 1) : ch;
        cursor += escaped ? 2 : 1;
    }

    if (text.isEmpty() or parenDepth != 0)
        return {};

    Document::Token token (start + static_cast<size_t> (index), start + static_cast<size_t> (cursor));
    token.add<juce::String> (Id::href, std::move (text));
    return token;
}

Document::Token
MarkdownDocument::getLinkDestination (const std::string& source, size_t start, size_t end, int index)
{
    if (charAt (source, start, end, index) == Chars::lessThan)
        return getBracketedDestination (source, start, end, index);

    return getBareDestination (source, start, end, index);
}

Document::Token MarkdownDocument::getQuotedTitle (const std::string& source,
                                                  size_t start,
                                                  size_t end,
                                                  int index,
                                                  juce::juce_wchar closeQuote)
{
    int cursor { index + 1 };
    juce::String text;

    while (charAt (source, start, end, cursor) != 0 and charAt (source, start, end, cursor) != closeQuote)
    {
        const auto ch { charAt (source, start, end, cursor) };
        const auto escaped { ch == Chars::backslash
                             and isAsciiPunctuation (charAt (source, start, end, cursor + 1)) };

        text += escaped ? charAt (source, start, end, cursor + 1) : ch;
        cursor += escaped ? 2 : 1;
    }

    if (charAt (source, start, end, cursor) == 0)
        return {};

    Document::Token token (
        start + static_cast<size_t> (index), start + static_cast<size_t> (cursor) + 1);
    token.add<juce::String> (Id::title, std::move (text));
    return token;
}

Document::Token MarkdownDocument::getLinkTitle (const std::string& source, size_t start, size_t end, int index)
{
    const auto opener { charAt (source, start, end, index) };

    if (opener == 0)
        return {};

    const auto isQuoteForm { opener == Chars::doubleQuote or opener == Chars::singleQuote
                             or opener == Chars::openParen };
    const auto closer { opener == Chars::openParen ? Chars::closeParen : opener };

    return isQuoteForm ? getQuotedTitle (source, start, end, index, closer) : Document::Token();
}

Document::Token MarkdownDocument::getEndOfLinkTail (const std::string& source, size_t start, size_t end, int cursor)
{
    int position { cursor };
    int spaceRun { 0 };

    while (isUnicodeWhitespace (charAt (source, start, end, position + spaceRun)))
        ++spaceRun;

    const auto titleToken { spaceRun > 0 ? getLinkTitle (source, start, end, position + spaceRun)
                                         : Document::Token() };
    const auto titleLength { static_cast<int> (titleToken.length()) };
    position += titleLength > 0 ? spaceRun + titleLength : 0;

    while (isUnicodeWhitespace (charAt (source, start, end, position)))
        ++position;

    if (charAt (source, start, end, position) != Chars::closeParen)
        return {};

    Document::Token token (
        start + static_cast<size_t> (cursor), start + static_cast<size_t> (position) + 1);
    token.add<juce::String> (
        Id::title, titleLength > 0 ? *titleToken.get<juce::String> (Id::title) : juce::String());
    return token;
}

Document::Token MarkdownDocument::getInlineTail (const std::string& source, size_t start, size_t end, int index)
{
    if (charAt (source, start, end, index) != Chars::openParen)
        return {};

    int cursor { index + 1 };

    while (isUnicodeWhitespace (charAt (source, start, end, cursor)))
        ++cursor;

    if (charAt (source, start, end, cursor) == Chars::closeParen)
    {
        Document::Token token (
            start + static_cast<size_t> (index), start + static_cast<size_t> (cursor) + 1);
        token.add<juce::String> (Id::href, juce::String());
        token.add<juce::String> (Id::title, juce::String());
        return token;
    }

    const auto destinationToken { getLinkDestination (source, start, end, cursor) };

    if (not destinationToken.contains (Id::href))
        return {};

    cursor += static_cast<int> (destinationToken.length());
    const auto tailToken { getEndOfLinkTail (source, start, end, cursor) };

    if (not tailToken.contains (Id::title))
        return {};

    cursor += static_cast<int> (tailToken.length());
    Document::Token token (start + static_cast<size_t> (index), start + static_cast<size_t> (cursor));
    token.add<juce::String> (Id::href, *destinationToken.get<juce::String> (Id::href));
    token.add<juce::String> (Id::title, *tailToken.get<juce::String> (Id::title));
    return token;
}

juce::String MarkdownDocument::normalizeLabel (const juce::String& label)
{
    juce::String collapsed;
    bool pendingSpace { false };

    for (auto ptr { label.getCharPointer() }; not ptr.isEmpty(); ++ptr)
    {
        const auto ch { *ptr };

        if (Document::isWhitespace (ch))
        {
            pendingSpace = collapsed.isNotEmpty();
        }
        else
        {
            if (pendingSpace)
                collapsed += Chars::space;

            collapsed += ch;
            pendingSpace = false;
        }
    }

    return collapsed.toLowerCase();
}

Document::Token MarkdownDocument::getOperatorToken (const std::string& source, size_t cursor) noexcept
{
    auto end { cursor };
    ++end;

    while ((end < source.size())
           and isMarkdownOperatorChar (
               (*Document::Cursor { source, end })))
        ++end;

    return Document::Token (cursor, end, map::DocumentTokenType::operators);
}

Document::Token MarkdownDocument::getTextToken (const std::string& source, size_t cursor) noexcept
{
    auto end { cursor };
    ++end;

    while ((end < source.size())
           and (*Document::Cursor { source, end })
                   != Chars::newline
           and not isMarkdownOperatorChar (
               (*Document::Cursor { source, end })))
        ++end;

    return Document::Token (cursor, end, map::DocumentTokenType::text);
}

const Document::Vocabulary& MarkdownDocument::getMarkdownVocabulary()
{
    static const Document::Vocabulary vocabulary { map::markdown, {}, {} };
    return vocabulary;
}

/*____________________________________________________________________________*/
} /** namespace jam */
