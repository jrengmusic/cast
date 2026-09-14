namespace jam
{
/*____________________________________________________________________________*/

Document::Token::Token (size_t startOffset,
                        size_t endOffset,
                        int typeIn) noexcept
    : span (Span::pack (
          static_cast<uint32_t> (startOffset),
          static_cast<uint32_t> (endOffset - startOffset)))
    , type (typeIn)
{
    jassert (endOffset >= startOffset);
}

void Document::Token::setSpan (size_t startOffset, size_t endOffset) noexcept
{
    span = Span::pack (
        static_cast<uint32_t> (startOffset),
        static_cast<uint32_t> (endOffset - startOffset));
}

bool Document::Token::contains (const juce::Identifier& key) const noexcept
{
    for (const auto& [propertyKey, propertyValue] : properties)
        if (propertyKey == key)
            return true;

    return false;
}

size_t Document::Element::Hash::operator() (const Element& element) const noexcept
{
    const auto parentHash { std::hash<const void*> {}(element.parent) };
    const auto idHash { std::hash<juce::Identifier> {}(element.id) };
    return parentHash ^ (idHash + 0x9e3779b9 + (parentHash << 6) + (parentHash >> 2));
}

bool Document::Element::operator== (const Element& other) const noexcept
{
    return parent == other.parent and id == other.id;
}

bool Document::Element::Iterator::operator!= (const Iterator& other) const noexcept
{
    return current != other.current;
}

Document::Element::Iterator& Document::Element::Iterator::operator++() noexcept
{
    current = current->nextSibling;
    return *this;
}

Document::Element* Document::Element::getChildByID (const juce::Identifier& idToLookFor) const noexcept
{
    auto* child { firstChild };

    while (child != nullptr and child->id != idToLookFor)
        child = child->nextSibling;

    return child;
}

bool Document::Element::contains (const juce::Identifier& key) const noexcept
{
    for (const auto& [propertyKey, propertyValue] : properties)
        if (propertyKey == key)
            return true;

    return false;
}

juce::String Document::Element::getAllSubText() const
{
    if (contains (Id::text))
        return *get<juce::String> (Id::text);

    juce::String result;

    for (auto* child : *this)
        result += child->getAllSubText();

    return result;
}

void Document::Element::applyToProperties (
    const std::function<void (const juce::Identifier&, const Value&)>& function) const
{
    for (const auto& [propertyKey, propertyValue] : properties)
        function (propertyKey, propertyValue);
}

juce::juce_wchar Document::Cursor::operator*() const noexcept
{
    return static_cast<juce::juce_wchar> (
        static_cast<unsigned char> (source.at (position)));
}

Document::Cursor& Document::Cursor::operator++() noexcept
{
    ++position;
    return *this;
}

Document::Cursor& Document::Cursor::operator+= (int numBytes) noexcept
{
    position += static_cast<size_t> (numBytes);
    return *this;
}

bool Document::Cursor::startsWith (std::string_view pattern) const noexcept
{
    return source.compare (position, pattern.size(), pattern) == 0;
}

juce::Result Document::Validator::isValid (const Document& document) const
{
    jam::Strings keys;

    for (const auto& [ruleKey, rule] : getRules())
        keys.add (ruleKey);

    keys.sort (false);

    jam::Strings failures;

    for (const auto& key : keys)
    {
        if (const auto result { getRules().get (key, document) }; result.failed())
            failures.add (result.getErrorMessage());
    }

    if (failures.size() > 0)
        return juce::Result::fail (
            failures.joinIntoString (juce::String::charToString (Chars::newline), 0, -1));

    return juce::Result::ok();
}

bool Document::Writer::toFile (const Document& document, const juce::File& file) const
{
    file.getParentDirectory().createDirectory();
    return file.replaceWithText (getText (document),
                                 false,
                                 false,
                                 juce::String::charToString (Chars::newline).toRawUTF8());
}

size_t Document::addSource (std::string_view text)
{
    const auto offset { source.size() };
    source.append (text);

    for (size_t byteIndex { offset }; byteIndex < source.size(); ++byteIndex)
        if (source.at (byteIndex) == Chars::newline)
            lineOffsets.add (static_cast<uint32_t> (byteIndex + 1));

    return offset;
}

juce::String Document::getText (const Token& token) const
{
    return juce::String::fromUTF8 (
        source.data() + token.offset(), static_cast<int> (token.length()));
}

uint32_t Document::getLineNumber (uint32_t byteOffset) const noexcept
{
    const auto position { std::upper_bound (
        lineOffsets.begin(), lineOffsets.end(), byteOffset) };
    return static_cast<uint32_t> (std::distance (lineOffsets.begin(), position));
}

juce::String Document::preprocess (const juce::String& input)
{
    juce::String result;
    result.preallocateBytes (input.getCharPointer().sizeInBytes());

    auto cursor { input.getCharPointer() };
    auto runStart { cursor };

    while (not cursor.isEmpty())
    {
        auto beforeChar { cursor };
        auto ch { cursor.getAndAdvance() };

        if (ch == Chars::carriageReturn)
        {
            result.appendCharPointer (runStart, beforeChar);

            if (not cursor.isEmpty() and *cursor == Chars::newline)
                cursor.getAndAdvance();

            result += Chars::newline;
            runStart = cursor;
        }
        else if (ch == Chars::formFeed)
        {
            result.appendCharPointer (runStart, beforeChar);
            result += Chars::newline;
            runStart = cursor;
        }
        else if (ch == eofCodePoint
                 or (ch >= Chars::surrogateRangeStart and ch <= Chars::surrogateRangeEnd))
        {
            result.appendCharPointer (runStart, beforeChar);
            result += Chars::replacementCharacter;
            runStart = cursor;
        }
    }

    result.appendCharPointer (runStart, cursor);

    return result;
}

bool Document::isDigit (juce::juce_wchar ch) noexcept
{
    return ch >= Chars::zero and ch <= Chars::nine;
}

bool Document::isHexDigit (juce::juce_wchar ch) noexcept
{
    return isDigit (ch) or (ch >= Chars::upperA and ch <= Chars::upperF)
           or (ch >= Chars::lowerA and ch <= Chars::lowerF);
}

bool Document::isLetter (juce::juce_wchar ch) noexcept
{
    return (ch >= Chars::upperA and ch <= Chars::upperZ)
           or (ch >= Chars::lowerA and ch <= Chars::lowerZ);
}

bool Document::isWhitespace (juce::juce_wchar ch) noexcept
{
    return ch == Chars::newline or ch == Chars::tab or ch == Chars::space;
}

jam::Array<Document::Token> Document::getTokens (const std::string& text, const Vocabulary& vocabulary)
{
    jassert (vocabulary.commentOpen.empty() == vocabulary.commentClose.empty());

    jam::Array<Token> tokens;

    size_t cursor { 0 };
    size_t tokenStart { 0 };
    int currentType { map::DocumentTokenType::text };
    bool tokenOpen { false };
    int depth { 0 };
    char quote { Chars::nullCharacter };

    auto closeToken = [&tokens, &tokenStart, &currentType, &tokenOpen] (size_t tokenEnd)
    {
        if (tokenOpen)
            tokens.add ({ tokenStart, tokenEnd, currentType });

        tokenOpen = false;
    };

    auto openToken = [&tokenStart, &currentType, &tokenOpen] (size_t start, int type)
    {
        tokenStart = start;
        currentType = type;
        tokenOpen = true;
    };

    while (cursor < text.size())
    {
        const auto beforeChar { cursor };
        const auto insideQuoteOrRegion { depth > 0 or quote != Chars::nullCharacter };

        if (isCommentOpen (text, cursor, vocabulary, insideQuoteOrRegion))
        {
            closeToken (beforeChar);

            const auto length { addCommentToken (tokens, text, cursor, vocabulary) };
            cursor += static_cast<size_t> (length);
            tokenStart = cursor;
            continue;
        }

        const auto character { text.at (cursor++) };
        const auto characterIndex { static_cast<unsigned char> (character) };
        const auto byteClass { vocabulary.getClass (characterIndex) };

        depth = getDepth (depth, quote, byteClass);

        if (byteClass == map::Byte::quote)
            quote = getQuote (quote, character);

        const auto isActiveAfter { depth > 0 or quote != Chars::nullCharacter };

        if (insideQuoteOrRegion or isActiveAfter)
        {
            if (not (tokenOpen and currentType == map::DocumentTokenType::region))
            {
                closeToken (beforeChar);
                openToken (beforeChar, map::DocumentTokenType::region);
            }

            if (not isActiveAfter)
                closeToken (cursor);
        }
        else
        {
            const auto type { byteClass == map::Byte::operators
                                  ? map::DocumentTokenType::operators
                                  : map::DocumentTokenType::text };

            if (not (tokenOpen and type == currentType))
            {
                closeToken (beforeChar);
                openToken (beforeChar, type);
            }
        }
    }

    closeToken (cursor);

    return tokens;
}

Document::Element* Document::addChild (Element& parent, const juce::Identifier& id)
{
    auto child { std::make_unique<Element>() };
    child->id = id;
    child->parent = &parent;

    auto* childElement { elements.add (std::move (child)).get() };

    if (parent.firstChild == nullptr)
        parent.firstChild = childElement;
    else
        parent.lastChild->nextSibling = childElement;

    parent.lastChild = childElement;

    return childElement;
}

Document::Element* Document::appendChild (Element& parent, Document&& child)
{
    auto* childRoot { child.root };
    childRoot->parent = &parent;

    for (auto& entry : child.elements)
        elements.add (std::move (entry));

    if (parent.firstChild == nullptr)
        parent.firstChild = childRoot;
    else
        parent.lastChild->nextSibling = childRoot;

    parent.lastChild = childRoot;

    return childRoot;
}

Document::Element* Document::appendChildren (Element& parent, Document&& child)
{
    const auto baseOffset { addSource (child.getSource()) };

    Element* firstAdopted { nullptr };
    auto* element { child.root->firstChild };

    while (element != nullptr)
    {
        element->parent = &parent;
        setSpanOffsets (*element, baseOffset);

        if (parent.firstChild == nullptr)
            parent.firstChild = element;
        else
            parent.lastChild->nextSibling = element;

        parent.lastChild = element;

        if (firstAdopted == nullptr)
            firstAdopted = element;

        element = element->nextSibling;
    }

    for (auto& entry : child.elements)
        elements.add (std::move (entry));

    return firstAdopted;
}

int Document::addCommentToken (jam::Array<Token>& tokens,
                                const std::string& text,
                                size_t cursor,
                                const Vocabulary& vocabulary)
{
    auto commentCursor { cursor + vocabulary.commentOpen.size() };

    while (commentCursor < text.size()
           and text.compare (
                   commentCursor, vocabulary.commentClose.size(), vocabulary.commentClose)
                   != 0)
        ++commentCursor;

    if (commentCursor < text.size())
        commentCursor += vocabulary.commentClose.size();

    tokens.add ({ cursor, commentCursor, map::DocumentTokenType::comment });

    return static_cast<int> (commentCursor - cursor);
}

bool Document::isCommentOpen (const std::string& text,
                              size_t cursor,
                              const Vocabulary& vocabulary,
                              bool insideQuoteOrRegion) noexcept
{
    return (not insideQuoteOrRegion) and not vocabulary.commentOpen.empty()
           and text.compare (cursor, vocabulary.commentOpen.size(), vocabulary.commentOpen) == 0;
}

char Document::getQuote (char currentQuote, char character) noexcept
{
    if (currentQuote == Chars::nullCharacter) return character;
    if (currentQuote == character) return Chars::nullCharacter;

    return currentQuote;
}

int Document::getDepth (int currentDepth, char quote, int byteClass) noexcept
{
    if (quote == Chars::nullCharacter and byteClass == map::Byte::regionOpen) return currentDepth + 1;
    if (quote == Chars::nullCharacter and currentDepth > 0 and byteClass == map::Byte::regionClose) return currentDepth - 1;

    return currentDepth;
}

void Document::setSpanOffsets (Token& token, size_t offset)
{
    token.span = Span::pack (token.offset() + static_cast<uint32_t> (offset), token.length());

    for (auto& [propertyKey, propertyValue] : token.properties)
    {
        std::visit (
            [offset] (auto& payload)
            {
                using ValueType = std::decay_t<decltype (payload)>;

                if constexpr (std::is_same_v<ValueType, bool>)
                {
                }
                else if constexpr (std::is_same_v<ValueType, double>)
                {
                }
                else if constexpr (std::is_same_v<ValueType, juce::String>)
                {
                }
                else if constexpr (std::is_same_v<ValueType, Token>)
                {
                    setSpanOffsets (payload, offset);
                }
                else if constexpr (std::is_same_v<ValueType, jam::Array<Property<Token>>>)
                {
                    for (auto& [propertyKey, propertyValue] : payload)
                        setSpanOffsets (propertyValue, offset);
                }
                else if constexpr (std::is_same_v<ValueType, int>)
                {
                }
                else if constexpr (std::is_same_v<ValueType, jam::Union<uint16_t, uint16_t>>)
                {
                }
                else
                {
                    static_assert (sizeof (ValueType) == 0, "unhandled Token::Value alternative");
                }
            },
            propertyValue);
    }
}

void Document::setSpanOffsets (Element& element, size_t offset)
{
    for (auto& [propertyKey, propertyValue] : element.properties)
    {
        std::visit (
            [offset] (auto& payload)
            {
                using ValueType = std::decay_t<decltype (payload)>;

                if constexpr (std::is_same_v<ValueType, std::monostate>)
                {
                }
                else if constexpr (std::is_same_v<ValueType, bool>)
                {
                }
                else if constexpr (std::is_same_v<ValueType, int>)
                {
                }
                else if constexpr (std::is_same_v<ValueType, double>)
                {
                }
                else if constexpr (std::is_same_v<ValueType, juce::uint32>)
                {
                }
                else if constexpr (std::is_same_v<ValueType, juce::Identifier>)
                {
                }
                else if constexpr (std::is_same_v<ValueType, juce::String>)
                {
                }
                else if constexpr (std::is_same_v<ValueType, Tokens>)
                {
                    for (auto& token : payload)
                        setSpanOffsets (token, offset);
                }
                else if constexpr (std::is_same_v<ValueType, juce::int64>)
                {
                }
                else if constexpr (std::is_same_v<ValueType, juce::var>)
                {
                }
                else if constexpr (std::is_same_v<ValueType, Element*>)
                {
                    if (payload != nullptr)
                        setSpanOffsets (*payload, offset);
                }
                else if constexpr (std::is_same_v<ValueType, Elements>)
                {
                    for (auto* child : payload)
                        if (child != nullptr)
                            setSpanOffsets (*child, offset);
                }
                else if constexpr (std::is_same_v<ValueType, Identifiers>)
                {
                }
                else
                {
                    static_assert (sizeof (ValueType) == 0, "unhandled Element::Value alternative");
                }
            },
            propertyValue);
    }

    for (auto* child : element)
        setSpanOffsets (*child, offset);
}

void Document::parse (const char* data, int size)
{
    const auto& vocabulary { getVocabulary() };

    const auto raw { juce::String::createStringFromData (data, size) };
    const auto text { preprocess (raw) };
    source = std::string (text.toRawUTF8(), text.getNumBytesAsUTF8());

    lineOffsets.add (0);

    for (size_t byteIndex { 0 }; byteIndex < source.size(); ++byteIndex)
        if (source.at (byteIndex) == Chars::newline)
            lineOffsets.add (static_cast<uint32_t> (byteIndex + 1));

    const auto segments { getTokens (source, vocabulary) };

    Cursor cursor { source, 0 };
    int segmentIndex { 0 };

    while (not cursor.isEmpty())
    {
        while (segmentIndex < segments.size()
               and (segments.at (segmentIndex).offset() + segments.at (segmentIndex).length())
                       <= cursor.getOffset())
            ++segmentIndex;

        const auto segmentType { segmentIndex < segments.size()
                                     ? segments.at (segmentIndex).type
                                     : static_cast<int> (map::DocumentTokenType::text) };

        const auto length { getToken (cursor, segmentType) };

        if (length <= 0)
        {
#if JUCE_DEBUG
            debug::Log::write ("Document::parse: getToken consumed nothing -- parse truncated");
#endif
            jassertfalse;
            break;
        }

        cursor += length;
    }

    build();
}

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
