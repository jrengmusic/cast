namespace jam
{
/*____________________________________________________________________________*/

MarkdownDocument::InlineParser::InlineParser (Document& newDocument, const BlockParser& newBlocks)
    : document (newDocument)
    , blocks (newBlocks)
{
}

void MarkdownDocument::InlineParser::addLeafText (Element& element)
{
    if (element.contains (Id::type))
    {
        for (auto* child : element)
            addLeafText (*child);

        const auto type { *element.get<int> (Id::type) };
        const auto isTextLeaf { type == map::BlockType::paragraph
                                or type == map::BlockType::heading };
        const auto isRawTextLeaf { type == map::BlockType::codeBlock
                                   or type == map::BlockType::mermaid
                                   or type == map::BlockType::htmlBlock };

        if (isTextLeaf)
        {
            const auto text { blocks.leafText.contains (&element)
                                  ? blocks.leafText.at (&element).trimEnd()
                                  : juce::String() };
            addInlines (element, text);
        }
        else if (isRawTextLeaf)
        {
            const auto text { blocks.leafText.contains (&element)
                                  ? blocks.leafText.at (&element)
                                  : juce::String() };
            auto* textNode { document.addChild (element, Id::text) };
            textNode->add<juce::String> (Id::text, text);
        }
        else if (type == map::BlockType::image)
        {
            const auto description { blocks.leafText.contains (&element)
                                         ? blocks.leafText.at (&element)
                                         : juce::String() };
            addInlines (element, description);
        }
    }
}

juce::juce_wchar MarkdownDocument::InlineParser::charBefore (const std::string& source,
                                                             const jam::Array<Document::Token>& tokens,
                                                             int index) noexcept
{
    if (index > 0)
    {
        const auto& prev { tokens.at (index - 1) };

        if (prev.length() > 0)
        {
            auto last { (prev.offset() + prev.length()) };
            --last;
            return (*Document::Cursor { source, last });
        }
    }

    return Chars::space;
}

juce::juce_wchar MarkdownDocument::InlineParser::charAfter (const std::string& source,
                                                            const jam::Array<Document::Token>& tokens,
                                                            int index) noexcept
{
    return index < tokens.size() and tokens.at (index).length() > 0
               ? (*Document::Cursor { source, tokens.at (index).offset() })
               : Chars::space;
}

bool MarkdownDocument::InlineParser::isLeftFlanking (juce::juce_wchar before, juce::juce_wchar after) noexcept
{
    return not isUnicodeWhitespace (after)
           and (not isUnicodePunctuation (after) or isUnicodeWhitespace (before)
                or isUnicodePunctuation (before));
}

bool MarkdownDocument::InlineParser::isRightFlanking (juce::juce_wchar before, juce::juce_wchar after) noexcept
{
    return not isUnicodeWhitespace (before)
           and (not isUnicodePunctuation (before) or isUnicodeWhitespace (after)
                or isUnicodePunctuation (after));
}

bool MarkdownDocument::InlineParser::canOpenDelimiter (juce::juce_wchar marker,
                                                       juce::juce_wchar before,
                                                       bool leftFlanking,
                                                       bool rightFlanking) noexcept
{
    return marker != Chars::underscore
               ? leftFlanking
               : (leftFlanking and (not rightFlanking or isUnicodePunctuation (before)));
}

bool MarkdownDocument::InlineParser::canCloseDelimiter (juce::juce_wchar marker,
                                                        juce::juce_wchar before,
                                                        juce::juce_wchar after,
                                                        bool leftFlanking,
                                                        bool rightFlanking) noexcept
{
    return marker != Chars::underscore
               ? rightFlanking
               : (rightFlanking and (not leftFlanking or isUnicodePunctuation (after)));
}

bool MarkdownDocument::InlineParser::isTildeOverflow (const std::string& source,
                                                      const Document::Token& token) noexcept
{
    return (*Document::Cursor { source, token.offset() })
               == Chars::tilde
           and static_cast<int> (token.length()) > maxTildeRunLength;
}

bool MarkdownDocument::InlineParser::tokenCanOpen (const std::string& source,
                                                   const jam::Array<Document::Token>& tokens,
                                                   int index) noexcept
{
    const auto& token { tokens.at (index) };
    const auto marker { (*Document::Cursor { source, token.offset() }) };
    const auto before { charBefore (source, tokens, index) };
    const auto after { charAfter (source, tokens, index + 1) };

    return not isTildeOverflow (source, token)
           and canOpenDelimiter (
               marker, before, isLeftFlanking (before, after), isRightFlanking (before, after));
}

bool MarkdownDocument::InlineParser::tokenCanClose (const std::string& source,
                                                    const jam::Array<Document::Token>& tokens,
                                                    int index) noexcept
{
    const auto& token { tokens.at (index) };
    const auto marker { (*Document::Cursor { source, token.offset() }) };
    const auto before { charBefore (source, tokens, index) };
    const auto after { charAfter (source, tokens, index + 1) };

    return not isTildeOverflow (source, token)
           and canCloseDelimiter (marker,
                                 before,
                                 after,
                                 isLeftFlanking (before, after),
                                 isRightFlanking (before, after));
}

void MarkdownDocument::InlineParser::getDelimiter (const std::string& source,
                                                   jam::Array<Document::Token>& tokens,
                                                   int index) noexcept
{
    auto& token { tokens.at (index) };
    token.add<bool> (Id::isOpen, tokenCanOpen (source, tokens, index));
    token.add<bool> (Id::isClose, tokenCanClose (source, tokens, index));
}

bool MarkdownDocument::InlineParser::ruleOfThreeAllows (int openerLen,
                                                        int closerLen,
                                                        bool openerCanOpen,
                                                        bool openerCanClose,
                                                        bool closerCanOpen,
                                                        bool closerCanClose) noexcept
{
    const auto eitherFlexible { (openerCanOpen and openerCanClose)
                                or (closerCanOpen and closerCanClose) };
    const auto sumDivisibleByThree { (openerLen + closerLen) % 3 == 0 };
    const auto bothDivisibleByThree { openerLen % 3 == 0 and closerLen % 3 == 0 };

    return not eitherFlexible or not sumDivisibleByThree or bothDivisibleByThree;
}

int MarkdownDocument::InlineParser::findMatchingOpener (const std::string& source,
                                                        const jam::Array<Document::Token>& tokens,
                                                        int closerIndex) noexcept
{
    const auto& closer { tokens.at (closerIndex) };
    const auto closerMarker { (*Document::Cursor { source, closer.offset() }) };
    const auto closerLen { static_cast<int> (closer.length()) };
    const auto closerCanOpen { *closer.get<bool> (Id::isOpen) };
    const auto closerCanClose { *closer.get<bool> (Id::isClose) };
    int openerIndex { closerIndex - 1 };

    while (openerIndex >= 0)
    {
        const auto& opener { tokens.at (openerIndex) };

        if (opener.type == map::MarkdownTokenType::emphasisDelimiter)
        {
            const auto openerMarker { (*Document::Cursor { source, opener.offset() }) };
            const auto openerLen { static_cast<int> (opener.length()) };
            const auto openerCanOpen { *opener.get<bool> (Id::isOpen) };
            const auto tildeLengthMatch { closerMarker != Chars::tilde
                                          or openerLen == closerLen };
            const auto matches { openerLen > 0 and openerCanOpen
                                 and openerMarker == closerMarker and tildeLengthMatch
                                 and ruleOfThreeAllows (openerLen,
                                                        closerLen,
                                                        openerCanOpen,
                                                        *opener.get<bool> (Id::isClose),
                                                        closerCanOpen,
                                                        closerCanClose) };

            if (matches)
                return openerIndex;
        }

        --openerIndex;
    }

    return -1;
}

void MarkdownDocument::InlineParser::deactivateInteriorDelimiters (jam::Array<Document::Token>& tokens,
                                                                   int openerIndex,
                                                                   int closerIndex) noexcept
{
    for (int i { openerIndex + 1 }; i < closerIndex; ++i)
        if (tokens.at (i).type == map::MarkdownTokenType::emphasisDelimiter)
            tokens.at (i).setSpan (tokens.at (i).offset(), tokens.at (i).offset());
}

Document::Token MarkdownDocument::InlineParser::applyEmphasisMatch (const std::string& source,
                                                                    jam::Array<Document::Token>& tokens,
                                                                    int openerIndex,
                                                                    int closerIndex)
{
    auto& opener { tokens.at (openerIndex) };
    auto& closer { tokens.at (closerIndex) };
    const auto openerMarker { (*Document::Cursor { source, opener.offset() }) };
    const auto isStrike { openerMarker == Chars::tilde };
    const auto openerLen { static_cast<int> (opener.length()) };
    const auto closerLen { static_cast<int> (closer.length()) };
    const auto useLength { isStrike ? openerLen
                                    : ((openerLen >= 2 and closerLen >= 2) ? 2 : 1) };
    const auto styleFlag { static_cast<int> (
        isStrike ? Stamp::strike : (useLength == 2 ? Stamp::bold : Stamp::italic)) };

    deactivateInteriorDelimiters (tokens, openerIndex, closerIndex);
    opener.setSpan (opener.offset(), opener.offset() + (openerLen - useLength));
    closer.setSpan (closer.offset(), closer.offset() + (closerLen - useLength));

    return Document::Token (
        static_cast<size_t> (openerIndex), static_cast<size_t> (closerIndex), styleFlag);
}

void MarkdownDocument::InlineParser::closeEmphasis (const std::string& source,
                                                    jam::Array<Document::Token>& tokens,
                                                    int closerIndex,
                                                    jam::Array<Document::Token>& runs)
{
    auto openerIndex { findMatchingOpener (source, tokens, closerIndex) };

    while (openerIndex >= 0 and tokens.at (closerIndex).length() > 0)
    {
        runs.add (applyEmphasisMatch (source, tokens, openerIndex, closerIndex));
        openerIndex = tokens.at (closerIndex).length() > 0
                          ? findMatchingOpener (source, tokens, closerIndex)
                          : -1;
    }
}

jam::Array<Document::Token> MarkdownDocument::InlineParser::processEmphasis (const std::string& source,
                                                                             jam::Array<Document::Token>& tokens)
{
    jam::Array<Document::Token> runs;

    for (int closerIndex { 0 }; closerIndex < tokens.size(); ++closerIndex)
    {
        const auto& closer { tokens.at (closerIndex) };

        if (closer.type == map::MarkdownTokenType::emphasisDelimiter
            and closer.length() > 0 and *closer.get<bool> (Id::isClose))
            closeEmphasis (source, tokens, closerIndex, runs);
    }

    return runs;
}

Document::Token MarkdownDocument::InlineParser::getText (const std::string& source,
                                                         size_t cursor,
                                                         jam::Array<Document::Token>& tokens) noexcept
{
    if ((*Document::Cursor { source, cursor })
        == Chars::newline)
    {
        auto end { cursor };
        ++end;
        return Document::Token (cursor, end, map::MarkdownTokenType::lineBreak);
    }

    const auto precedingChar { charBefore (source, tokens, tokens.size()) };
    const auto extendedAutolinkAllowed { isUnicodeWhitespace (precedingChar)
                                         or precedingChar == Chars::asterisk
                                         or precedingChar == Chars::underscore
                                         or precedingChar == Chars::tilde
                                         or precedingChar == Chars::openParen };

    const auto isWwwOrUrlAutolinkStart {
        (*Document::Cursor { source, cursor })
            == static_cast<juce::juce_wchar> (Chars::wwwAutolinkPrefix[0])
        or (*Document::Cursor { source, cursor })
               == static_cast<juce::juce_wchar> (
                   Chars::httpAutolinkPrefix[0])
        or (*Document::Cursor { source, cursor })
               == static_cast<juce::juce_wchar> (Chars::ftpAutolinkPrefix[0])
    };

    if (extendedAutolinkAllowed and isWwwOrUrlAutolinkStart)
    {
        auto extended { getExtendedAutolink (source, cursor) };

        if (extended.length() > 0)
            return extended;
    }

    if (extendedAutolinkAllowed
        and (*Document::Cursor { source, cursor })
                == Chars::at)
    {
        auto email { getBareEmailAutolink (source, cursor, tokens) };

        if (email.length() > 0)
            return email;
    }

    const auto start { cursor };
    ++cursor;

    const auto wwwOrUrlStartChar1 { static_cast<juce::juce_wchar> (
        Chars::wwwAutolinkPrefix[0]) };
    const auto wwwOrUrlStartChar2 { static_cast<juce::juce_wchar> (
        Chars::httpAutolinkPrefix[0]) };
    const auto wwwOrUrlStartChar3 { static_cast<juce::juce_wchar> (
        Chars::ftpAutolinkPrefix[0]) };

    while ((cursor < source.size())
           and (*Document::Cursor { source, cursor })
                   != Chars::newline
           and (*Document::Cursor { source, cursor })
                   != Chars::backslash
           and (*Document::Cursor { source, cursor })
                   != Chars::ampersand
           and (*Document::Cursor { source, cursor })
                   != Chars::backtick
           and (*Document::Cursor { source, cursor })
                   != Chars::lessThan
           and (*Document::Cursor { source, cursor })
                   != Chars::asterisk
           and (*Document::Cursor { source, cursor })
                   != Chars::underscore
           and (*Document::Cursor { source, cursor })
                   != Chars::tilde
           and (*Document::Cursor { source, cursor })
                   != Chars::openBracket
           and (*Document::Cursor { source, cursor })
                   != Chars::closeBracket
           and (*Document::Cursor { source, cursor })
                   != Chars::exclamation
           and (*Document::Cursor { source, cursor })
                   != wwwOrUrlStartChar1
           and (*Document::Cursor { source, cursor })
                   != wwwOrUrlStartChar2
           and (*Document::Cursor { source, cursor })
                   != wwwOrUrlStartChar3
           and (*Document::Cursor { source, cursor })
                   != Chars::at)
        ++cursor;

    return Document::Token (start, cursor, map::MarkdownTokenType::text);
}

Document::Token MarkdownDocument::InlineParser::getEscape (const std::string& source, size_t cursor) noexcept
{
    const auto backslash { cursor };
    ++cursor;

    if ((cursor < source.size())
        and (*Document::Cursor { source, cursor })
                == Chars::newline)
    {
        auto end { cursor };
        ++end;
        return Document::Token (backslash, end, map::MarkdownTokenType::lineBreak);
    }

    if ((cursor < source.size())
        and isAsciiPunctuation (
            (*Document::Cursor { source, cursor })))
    {
        auto end { cursor };
        ++end;
        Document::Token result { Document::Token (
            backslash, end, map::MarkdownTokenType::text) };
        result.add<bool> (Id::selfClosing, true);// escape: output start[1]
        return result;
    }

    auto end { backslash };
    ++end;
    return Document::Token (backslash, end, map::MarkdownTokenType::text);
}

Document::Token
MarkdownDocument::InlineParser::getNumericCharacterReference (const std::string& source, size_t cursor) noexcept
{
    const auto innerStart { cursor };
    bool isHexadecimal { false };

    if ((cursor < source.size())
        and ((*Document::Cursor { source, cursor })
                 == Chars::lowerX
             or (*Document::Cursor { source, cursor })
                    == Chars::upperX))
    {
        isHexadecimal = true;
        ++cursor;
    }

    juce::int64 value { 0 };
    int digitCount { 0 };
    const auto radix { isHexadecimal ? Document::hexadecimalRadix
                                     : Document::decimalRadix };

    while ((cursor < source.size()) and digitCount < maxEntityDigits
           and (isHexadecimal ? Document::isHexDigit ((*Document::Cursor { source, cursor }))
                              : Document::isDigit ((*Document::Cursor { source, cursor }))))
    {
        const auto digit { isHexadecimal
                               ? juce::CharacterFunctions::getHexDigitValue (
                                     (*Document::Cursor { source, cursor }))
                               : ((*Document::Cursor { source, cursor })
                                  - Chars::zero) };
        value = value * radix + digit;
        ++cursor;
        ++digitCount;
    }

    if (digitCount == 0 or (cursor >= source.size())
        or (*Document::Cursor { source, cursor })
               != Chars::semicolon)
    {
#if JUCE_DEBUG
        debug::Log::write (
            "Markdown::getNumericCharacterReference: malformed numeric character reference");
#endif
        return {};
    }

    ++cursor;

    const auto codePoint { (value == 0
                            or (value >= Chars::surrogateRangeStart
                                and value <= Chars::surrogateRangeEnd)
                            or value > Chars::maximumCodePoint)
                               ? Chars::replacementCharacter
                               : static_cast<juce::juce_wchar> (value) };

    Document::Token result { Document::Token (
        innerStart, cursor, map::MarkdownTokenType::characterReference) };
    result.add<juce::String> (Id::value, juce::String::charToString (codePoint));
    return result;
}

Document::Token
MarkdownDocument::InlineParser::getNamedCharacterReference (const std::string& source, size_t cursor) noexcept
{
    const auto nameStart { cursor };
    int nameLength { 0 };

    while (
        (cursor < source.size()) and nameLength < maxEntityNameLength
        and (isAsciiLetter ((*Document::Cursor { source, cursor }))
             or ((*Document::Cursor { source, cursor })
                     >= Chars::zero
                 and (*Document::Cursor { source, cursor })
                         <= Chars::nine)))
    {
        ++cursor;
        ++nameLength;
    }

    const juce::String name { juce::String::fromUTF8 (
        source.data() + nameStart, static_cast<int> (cursor - nameStart)) };
    const auto& entities { map::entities };
    const auto replacement { name.isNotEmpty() and entities.contains (name)
                                 ? entities.at (name)
                                 : juce::String() };

    if (replacement.isNotEmpty() and (cursor < source.size())
        and (*Document::Cursor { source, cursor })
                == Chars::semicolon)
    {
        ++cursor;
        Document::Token result { Document::Token (
            nameStart, cursor, map::MarkdownTokenType::characterReference) };
        result.add<juce::String> (Id::value, replacement);
        return result;
    }

    return {};
}

Document::Token
MarkdownDocument::InlineParser::getCharacterReference (const std::string& source, size_t cursor) noexcept
{
    const auto ampersand { cursor };
    ++cursor;

    if ((cursor >= source.size()))
    {
        auto end { ampersand };
        ++end;
        return Document::Token (ampersand, end, map::MarkdownTokenType::text);
    }

    if ((*Document::Cursor { source, cursor })
        == Chars::hash)
    {
        ++cursor;
        const auto numToken { getNumericCharacterReference (source, cursor) };

        if (numToken.length() == 0)
        {
            auto end { ampersand };
            ++end;
            return Document::Token (ampersand, end, map::MarkdownTokenType::text);
        }

        Document::Token result { Document::Token (
            ampersand,
            (numToken.offset() + numToken.length()),
            map::MarkdownTokenType::characterReference) };
        result.add<juce::String> (Id::value, *numToken.get<juce::String> (Id::value));
        return result;
    }

    const auto namedToken { getNamedCharacterReference (source, cursor) };

    if (namedToken.length() == 0)
    {
        auto end { ampersand };
        ++end;
        return Document::Token (ampersand, end, map::MarkdownTokenType::text);
    }

    Document::Token result { Document::Token (ampersand,
                                              (namedToken.offset() + namedToken.length()),
                                              map::MarkdownTokenType::characterReference) };
    result.add<juce::String> (Id::value, *namedToken.get<juce::String> (Id::value));
    return result;
}

int MarkdownDocument::InlineParser::measureMarkerRun (const std::string& source,
                                                      size_t cursor,
                                                      juce::juce_wchar marker) noexcept
{
    int length { 0 };
    auto ptr { cursor };

    while ((ptr < source.size())
           and (*Document::Cursor { source, ptr })
                   == marker)
    {
        ++length;
        ++ptr;
    }

    return length;
}

Document::Token MarkdownDocument::InlineParser::getCodeSpan (const std::string& source, size_t cursor) noexcept
{
    const auto start { cursor };
    const auto openLength { measureMarkerRun (source, cursor, Chars::backtick) };
    auto contentStart { cursor };
    contentStart += openLength;

    auto searchCursor { contentStart };
    size_t closeStart { std::string::npos };
    int closeLength { 0 };

    while ((searchCursor < source.size()) and closeStart == std::string::npos)
    {
        if ((*Document::Cursor { source, searchCursor })
            == Chars::backtick)
        {
            const auto runLength { measureMarkerRun (
                source, searchCursor, Chars::backtick) };

            if (runLength == openLength)
            {
                closeStart = searchCursor;
                closeLength = runLength;
            }
            else
            {
                searchCursor += runLength;
            }
        }
        else
        {
            ++searchCursor;
        }
    }

    if (closeStart == std::string::npos)
    {
        auto end { start };
        end += openLength;
        return Document::Token (start, end, map::MarkdownTokenType::text);
    }

    auto closeEnd { closeStart };
    closeEnd += closeLength;

    Document::Token result { Document::Token (
        start, closeEnd, map::MarkdownTokenType::codeSpan) };
    result.add<Document::Token> (Id::name, Document::Token (contentStart, closeStart));
    return result;
}

bool MarkdownDocument::InlineParser::isSchemeChar (juce::juce_wchar ch) noexcept
{
    return isAsciiLetter (ch) or (ch >= Chars::zero and ch <= Chars::nine)
           or ch == Chars::plus or ch == Chars::dash or ch == Chars::dot;
}

bool MarkdownDocument::InlineParser::isAutolinkUriChar (juce::juce_wchar ch) noexcept
{
    return ch > Chars::space and ch != Chars::lessThan and ch != Chars::greaterThan;
}

bool MarkdownDocument::InlineParser::isEmailLocalChar (juce::juce_wchar ch) noexcept
{
    return isAsciiLetter (ch) or (ch >= Chars::zero and ch <= Chars::nine)
           or ch == Chars::dot or ch == Chars::dash or ch == Chars::plus
           or ch == Chars::underscore;
}

bool MarkdownDocument::InlineParser::isEmailDomainChar (juce::juce_wchar ch) noexcept
{
    return isAsciiLetter (ch) or (ch >= Chars::zero and ch <= Chars::nine)
           or ch == Chars::dash or ch == Chars::dot;
}

bool MarkdownDocument::InlineParser::isDomainLabelChar (juce::juce_wchar ch) noexcept
{
    return isAsciiLetter (ch) or (ch >= Chars::zero and ch <= Chars::nine)
           or ch == Chars::dash or ch == Chars::underscore;
}

int MarkdownDocument::InlineParser::getExtendedDomainLength (const std::string& source, size_t cursor) noexcept
{
    auto scanEnd { cursor };

    while ((scanEnd < source.size()))
        ++scanEnd;

    auto domainEnd { cursor };

    while (domainEnd != scanEnd
           and (isDomainLabelChar ((*Document::Cursor { source, domainEnd }))
                or (*Document::Cursor { source, domainEnd })
                       == Chars::dot))
        ++domainEnd;

    while (domainEnd != cursor)
    {
        auto probe { domainEnd };
        --probe;

        if ((*Document::Cursor { source, probe })
            != Chars::dot)
            break;

        domainEnd = probe;
    }

    jam::Array<juce::String> segments;
    juce::String currentSegment;

    for (auto ptr { cursor }; ptr != domainEnd; ++ptr)
    {
        if ((*Document::Cursor { source, ptr })
            == Chars::dot)
        {
            segments.add (currentSegment);
            currentSegment.clear();
        }
        else
        {
            currentSegment +=
                (*Document::Cursor { source, ptr });
        }
    }

    segments.add (currentSegment);

    auto segmentsValid { segments.size() >= 2 };

    for (const auto& segment : segments)
        segmentsValid = segmentsValid and segment.isNotEmpty();

    const auto lastTwoHaveNoUnderscore {
        segments.size() < 2
        or (not segments.last().containsChar (Chars::underscore)
            and not segments.at (segments.size() - 2).containsChar (Chars::underscore))
    };

    return segmentsValid and lastTwoHaveNoUnderscore
               ? static_cast<int> ((domainEnd - cursor))
               : 0;
}

Document::Token MarkdownDocument::InlineParser::getExtendedAutolinkSpan (const std::string& source,
                                                                         size_t fullStart,
                                                                         size_t domainStart,
                                                                         int domainLength) noexcept
{
    auto pathCursor { domainStart };
    pathCursor += domainLength;

    while (
        (pathCursor < source.size())
        and not isUnicodeWhitespace (
            (*Document::Cursor { source, pathCursor }))
        and (*Document::Cursor { source, pathCursor })
                != Chars::lessThan)
        ++pathCursor;

    const juce::String span { juce::String::fromUTF8 (
        source.data() + fullStart, static_cast<int> (pathCursor - fullStart)) };
    auto length { span.length() };

    const auto isTrailingPunctuation { [] (juce::juce_wchar ch) noexcept
                                       {
                                           return ch == Chars::question
                                                  or ch == Chars::exclamation
                                                  or ch == Chars::dot or ch == Chars::comma
                                                  or ch == Chars::colon
                                                  or ch == Chars::asterisk
                                                  or ch == Chars::underscore
                                                  or ch == Chars::tilde;
                                       } };

    while (length > 0 and isTrailingPunctuation (span[length - 1]))
        --length;

    if (length > 0 and span[length - 1] == Chars::closeParen)
    {
        while (true)
        {
            int openCount { 0 };
            int closeCount { 0 };

            for (int i { 0 }; i < length; ++i)
            {
                openCount += span[i] == Chars::openParen ? 1 : 0;
                closeCount += span[i] == Chars::closeParen ? 1 : 0;
            }

            if (closeCount <= openCount)
                break;

            --length;
        }
    }

    if (length > 0 and span[length - 1] == Chars::semicolon)
    {
        auto entityCursor { length - 2 };

        while (entityCursor >= 0
               and (isAsciiLetter (span[entityCursor])
                    or (span[entityCursor] >= Chars::zero
                        and span[entityCursor] <= Chars::nine)))
            --entityCursor;

        if (entityCursor >= 0 and span[entityCursor] == Chars::ampersand)
            length = entityCursor;
    }

    if (length == 0)
        return {};

    auto trimmedEnd { fullStart };
    trimmedEnd += length;
    return Document::Token (fullStart, trimmedEnd, map::MarkdownTokenType::autolink);
}

Document::Token
MarkdownDocument::InlineParser::getWwwAutolink (const std::string& source, size_t cursor) noexcept
{
    if (not Document::Cursor { source, cursor }.startsWith (Chars::wwwAutolinkPrefix))
        return {};

    const auto domainLength { getExtendedDomainLength (source, cursor) };

    if (domainLength == 0)
        return {};

    return getExtendedAutolinkSpan (source, cursor, cursor, domainLength);
}

Document::Token
MarkdownDocument::InlineParser::getUrlAutolink (const std::string& source, size_t cursor) noexcept
{
    static constexpr std::array<const char*, 3> urlSchemePrefixes {
        Chars::httpAutolinkPrefix,
        Chars::httpsAutolinkPrefix,
        Chars::ftpAutolinkPrefix
    };

    for (const auto* prefix : urlSchemePrefixes)
    {
        if (Document::Cursor { source, cursor }.startsWith (prefix))
        {
            auto domainStart { cursor };
            domainStart += juce::String (prefix).length();

            const auto domainLength { getExtendedDomainLength (source, domainStart) };

            if (domainLength > 0)
                return getExtendedAutolinkSpan (source, cursor, domainStart, domainLength);
        }
    }

    return {};
}

Document::Token
MarkdownDocument::InlineParser::getExtendedAutolink (const std::string& source, size_t cursor) noexcept
{
    auto www { getWwwAutolink (source, cursor) };

    if (www.length() > 0)
        return www;

    return getUrlAutolink (source, cursor);
}

Document::Token
MarkdownDocument::InlineParser::getBareEmailAutolink (const std::string& source,
                                                      size_t cursor,
                                                      jam::Array<Document::Token>& tokens) noexcept
{
    if (tokens.isEmpty())
        return {};

    const auto& prevToken { tokens.last() };

    if (prevToken.length() == 0)
        return {};

    // Count email-local chars from end of preceding token span (all ASCII, single-byte UTF-8).
    int localCount { 0 };
    auto probe { (prevToken.offset() + prevToken.length()) };

    while (localCount < static_cast<int> (prevToken.length()))
    {
        auto check { probe };
        --check;

        if (not isEmailLocalChar (
                (*Document::Cursor { source, check })))
            break;

        probe = check;
        ++localCount;
    }

    if (localCount == 0)
        return {};

    auto afterAt { cursor };
    ++afterAt;

    auto domainEnd { afterAt };

    while ((domainEnd < source.size())
           and (isDomainLabelChar ((*Document::Cursor { source, domainEnd }))
                or (*Document::Cursor { source, domainEnd })
                       == Chars::dot))
        ++domainEnd;

    while (domainEnd != afterAt)
    {
        auto trail { domainEnd };
        --trail;

        if ((*Document::Cursor { source, trail })
            != Chars::dot)
            break;

        domainEnd = trail;
    }

    auto hasInteriorDot { false };

    for (auto ptr { afterAt }; ptr != domainEnd; ++ptr)
        hasInteriorDot =
            hasInteriorDot
            or (*Document::Cursor { source, ptr })
                   == Chars::dot;

    if (domainEnd == afterAt or not hasInteriorDot)
        return {};

    auto lastDomainChar { domainEnd };
    --lastDomainChar;

    if ((*Document::Cursor { source, lastDomainChar })
            == Chars::dash
        or (*Document::Cursor { source, lastDomainChar })
               == Chars::underscore)
        return {};

    // Shrink the preceding token's span to exclude the email local part.
    tokens.last().setSpan (tokens.last().offset(), (probe));

    return Document::Token (probe, domainEnd, map::MarkdownTokenType::autolink);
}

int MarkdownDocument::InlineParser::getSchemeLength (juce::String::CharPointerType cursor) noexcept
{
    const auto validStart { not cursor.isEmpty() and isAsciiLetter (*cursor) };
    auto ptr { cursor };
    int length { validStart ? 1 : 0 };

    if (validStart)
        ++ptr;

    while (validStart and not ptr.isEmpty() and length < maxSchemeLength
           and isSchemeChar (*ptr))
    {
        ++length;
        ++ptr;
    }

    return validStart and length >= minSchemeLength and not ptr.isEmpty()
                   and *ptr == Chars::colon
               ? length
               : 0;
}

Document::Token MarkdownDocument::InlineParser::getHtmlTagSpan (const std::string& source,
                                                                size_t cursor) noexcept
{
    const auto scanEnd { source.size() };

    for (int type { map::HtmlBlockType::comment }; type <= map::HtmlBlockType::cdata;
         ++type)
    {
        const auto openMarker { map::markupOpen[type] };
        const auto closeMarker { map::markupClose[type] };

        if (openMarker == nullptr
            or not Document::Cursor { source, cursor }.startsWith (openMarker))
            continue;

        if (type == map::HtmlBlockType::declaration)
        {
            const auto probePos { cursor + std::strlen (openMarker) };
            if (probePos >= source.size()
                or not isAsciiLetter ((*Document::Cursor { source, probePos })))
                continue;
        }

        auto closeSearch { cursor };
        closeSearch += std::strlen (openMarker);

        while ((closeSearch < source.size())
               and not Document::Cursor { source, closeSearch }.startsWith (closeMarker))
            ++closeSearch;

        if ((closeSearch < source.size()))
            closeSearch += std::strlen (closeMarker);

        return Document::Token (cursor, closeSearch, map::MarkdownTokenType::rawHtml);
    }

    const auto openEnd { getEndOfOpenTag (source, cursor, scanEnd, 1) };
    const auto tagEnd { openEnd > 1 ? openEnd : getEndOfClosingTag (source, cursor, scanEnd, 1) };

    if (tagEnd > 1)
        return Document::Token (
            cursor, advance (cursor, scanEnd, tagEnd), map::MarkdownTokenType::rawHtml);

    return {};
}

Document::Token
MarkdownDocument::InlineParser::getAutolinkUri (const std::string& source, size_t afterOpen) noexcept
{
    const auto schemeLength { getSchemeLength (
        juce::CharPointer_UTF8 (source.data() + afterOpen)) };

    if (schemeLength == 0)
        return {};

    auto uriEnd { afterOpen + schemeLength + 1 };

    while ((uriEnd < source.size())
           and isAutolinkUriChar ((*Document::Cursor { source, uriEnd })))
        ++uriEnd;

    if ((uriEnd < source.size())
        and (*Document::Cursor { source, uriEnd })
                == Chars::greaterThan)
        return Document::Token (afterOpen, uriEnd, map::MarkdownTokenType::autolink);

    return {};
}

Document::Token
MarkdownDocument::InlineParser::getAutolinkEmail (const std::string& source, size_t afterOpen) noexcept
{
    auto localCursor { afterOpen };

    while ((localCursor < source.size())
           and isEmailLocalChar ((*Document::Cursor { source, localCursor })))
        ++localCursor;

    if (localCursor == afterOpen or (localCursor >= source.size())
        or (*Document::Cursor { source, localCursor })
               != Chars::at)
        return {};

    auto domainEnd { localCursor + 1 };

    while ((domainEnd < source.size())
           and isEmailDomainChar ((*Document::Cursor { source, domainEnd })))
        ++domainEnd;

    if (domainEnd == localCursor + 1 or (domainEnd >= source.size())
        or (*Document::Cursor { source, domainEnd })
               != Chars::greaterThan)
        return {};

    return Document::Token (afterOpen, domainEnd, map::MarkdownTokenType::autolink);
}

Document::Token MarkdownDocument::InlineParser::getRawHtml (const std::string& source, size_t cursor) noexcept
{
    const auto start { cursor };
    auto afterOpen { cursor };
    ++afterOpen;

    if (auto uri { getAutolinkUri (source, afterOpen) }; uri.length() > 0)
        return uri;

    if (auto email { getAutolinkEmail (source, afterOpen) }; email.length() > 0)
        return email;

    if (auto tag { getHtmlTagSpan (source, cursor) }; tag.length() > 0)
        return tag;

    auto end { start };
    ++end;
    return Document::Token (start, end, map::MarkdownTokenType::text);
}

Document::Token
MarkdownDocument::InlineParser::getEmphasisDelimiter (const std::string& source, size_t cursor) noexcept
{
    const auto marker { (*Document::Cursor { source, cursor }) };
    auto end { cursor };

    while ((end < source.size())
           and (*Document::Cursor { source, end })
                   == marker)
        ++end;

    return Document::Token (cursor, end, map::MarkdownTokenType::emphasisDelimiter);
}

Document::Token MarkdownDocument::InlineParser::getLinkOpen (const std::string& source, size_t cursor) noexcept
{
    auto end { cursor };
    ++end;
    return Document::Token (cursor, end, map::MarkdownTokenType::linkOpen);
}

Document::Token MarkdownDocument::InlineParser::getImageOpen (const std::string& source, size_t cursor) noexcept
{
    auto next { cursor };
    ++next;

    if ((next < source.size())
        and (*Document::Cursor { source, next })
                == Chars::openBracket)
    {
        auto end { next };
        ++end;
        return Document::Token (cursor, end, map::MarkdownTokenType::imageOpen);
    }

    auto end { cursor };
    ++end;
    return Document::Token (cursor, end, map::MarkdownTokenType::text);
}

Document::Token
MarkdownDocument::InlineParser::getInlineReferenceLabel (const std::string& source, size_t start, size_t end, int index) noexcept
{
    if (charAt (source, start, end, index) != Chars::openBracket)
        return {};

    juce::String label;
    int cursor { index + 1 };

    while (charAt (source, start, end, cursor) != 0
           and charAt (source, start, end, cursor) != Chars::closeBracket
           and label.length() < maxLabelLength)
        label += charAt (source, start, end, cursor++);

    if (charAt (source, start, end, cursor) != Chars::closeBracket)
        return {};

    Document::Token token (
        start + static_cast<size_t> (index), start + static_cast<size_t> (cursor) + 1);
    token.add<juce::String> (Id::label, std::move (label));
    return token;
}

Document::Token MarkdownDocument::InlineParser::getReferenceValue (const std::string& source,
                                                                   size_t labelStart,
                                                                   size_t cursor,
                                                                   size_t next) const noexcept
{
    const juce::String rawLabel { juce::String::fromUTF8 (
        source.data() + labelStart, static_cast<int> (cursor - labelStart)) };
    const auto labelToken { getInlineReferenceLabel (source, next, source.size(), 0) };
    const auto referenceLabel { labelToken.contains (Id::label)
                                    ? *labelToken.get<juce::String> (Id::label)
                                    : juce::String() };
    const auto labelText { referenceLabel.isNotEmpty() ? referenceLabel : rawLabel };
    const auto normalized { normalizeLabel (labelText) };

    if (not blocks.referenceDestinations.contains (normalized))
        return {};

    const auto linkTitle { blocks.referenceTitles.contains (normalized)
                               ? blocks.referenceTitles.at (normalized)
                               : juce::String() };
    Document::Token token (next, next + labelToken.length());
    token.add<juce::String> (Id::href, blocks.referenceDestinations.at (normalized));
    token.add<juce::String> (Id::title, linkTitle);
    return token;
}

int MarkdownDocument::InlineParser::getOpenBracketIndex (const jam::Array<Document::Token>& tokens) noexcept
{
    for (int i { tokens.size() - 1 }; i >= 0; --i)
    {
        const auto& candidate { tokens.at (i) };
        const auto isBracket { candidate.type == map::MarkdownTokenType::linkOpen
                               or candidate.type == map::MarkdownTokenType::imageOpen };

        if (isBracket and *candidate.get<bool> (Id::active))
            return i;
    }

    return -1;
}

Document::Token MarkdownDocument::InlineParser::getLinkClose (const std::string& source,
                                                              size_t cursor,
                                                              jam::Array<Document::Token>& tokens) noexcept
{
    const auto openTokenIndex { getOpenBracketIndex (tokens) };
    auto end { cursor };
    ++end;

    if (openTokenIndex >= 0)
    {
        auto& openToken { tokens.at (openTokenIndex) };
        *openToken.get<bool> (Id::active) = false;

        const auto labelStart { openToken.offset() + openToken.length() };
        const auto next { cursor + 1 };
        auto tailToken { getInlineTail (source, next, source.size(), 0) };
        const auto resolvedToken { tailToken.contains (Id::href)
                                       ? std::move (tailToken)
                                       : getReferenceValue (source, labelStart, cursor, next) };

        if (resolvedToken.contains (Id::href))
        {
            if (openToken.type != map::MarkdownTokenType::imageOpen)
                for (int i { 0 }; i < openTokenIndex; ++i)
                {
                    auto& earlier { tokens.at (i) };
                    const auto isBracket { earlier.type == map::MarkdownTokenType::linkOpen
                                           or earlier.type == map::MarkdownTokenType::imageOpen };

                    if (isBracket)
                        *earlier.get<bool> (Id::active) = false;
                }

            openToken.add<juce::String> (Id::href, *resolvedToken.get<juce::String> (Id::href));

            const auto title { *resolvedToken.get<juce::String> (Id::title) };

            if (title.isNotEmpty())
                openToken.add<juce::String> (Id::title, title);

            openToken.add<double> (Id::close, static_cast<double> (tokens.size()));
            end += resolvedToken.length();
        }
    }

    Document::Token token { cursor, end, map::MarkdownTokenType::linkClose };

    if (openTokenIndex >= 0 and tokens.at (openTokenIndex).contains (Id::href))
        token.add<double> (Id::open, static_cast<double> (openTokenIndex));

    return token;
}

Document::Token
MarkdownDocument::InlineParser::getToken (const std::string& source, size_t cursor, jam::Array<Document::Token>& tokens)
{
    static const auto markers {
        []
        {
            jam::Function::Map<juce::String, Document::Token> markers;
            markers.add<const std::string&, size_t&> (
                juce::String::charToString (Chars::backslash), &getEscape);
            markers.add<const std::string&, size_t&> (
                juce::String::charToString (Chars::ampersand), &getCharacterReference);
            markers.add<const std::string&, size_t&> (
                juce::String::charToString (Chars::backtick), &getCodeSpan);
            markers.add<const std::string&, size_t&> (
                juce::String::charToString (Chars::lessThan), &getRawHtml);
            markers.add<const std::string&, size_t&> (
                juce::String::charToString (Chars::asterisk), &getEmphasisDelimiter);
            markers.add<const std::string&, size_t&> (
                juce::String::charToString (Chars::underscore), &getEmphasisDelimiter);
            markers.add<const std::string&, size_t&> (
                juce::String::charToString (Chars::tilde), &getEmphasisDelimiter);
            return markers;
        }()
    };

    const juce::String key { juce::String::charToString (
        (*Document::Cursor { source, cursor })) };

    if (markers.contains (key))
        return markers.get (key, source, cursor);

    return getText (source, cursor, tokens);
}

int MarkdownDocument::InlineParser::addInlineToken (jam::Array<Document::Token>& tokens,
                                                    Document::Token token,
                                                    size_t cursor) noexcept
{
    const auto length { static_cast<int> ((token.offset() + token.length()) - cursor) };
    tokens.add (std::move (token));
    return length;
}

int MarkdownDocument::InlineParser::addToken (jam::Array<Document::Token>& tokens, const std::string& source, size_t cursor)
{
    if ((*Document::Cursor { source, cursor })
        == Chars::openBracket)
    {
        auto token { getLinkOpen (source, cursor) };
        token.add<bool> (Id::active, true);
        return addInlineToken (tokens, std::move (token), cursor);
    }

    if ((*Document::Cursor { source, cursor })
        == Chars::exclamation)
    {
        auto token { getImageOpen (source, cursor) };

        if (token.type == map::MarkdownTokenType::imageOpen)
            token.add<bool> (Id::active, true);

        return addInlineToken (tokens, std::move (token), cursor);
    }

    if ((*Document::Cursor { source, cursor })
        == Chars::closeBracket)
    {
        auto token { getLinkClose (source, cursor, tokens) };
        return addInlineToken (tokens, std::move (token), cursor);
    }

    auto token { getToken (source, cursor, tokens) };
    return addInlineToken (tokens, std::move (token), cursor);
}

void MarkdownDocument::InlineParser::addEmphasis (Element& rootElement,
                                                  int index,
                                                  const jam::Array<Document::Token>& runs,
                                                  jam::Array<Element*>& stack,
                                                  jam::Array<int>& closeIndices)
{
    jam::Array<int> pendingRuns;

    for (int r { 0 }; r < runs.size(); ++r)
        if (static_cast<int> (runs.at (r).offset()) == index)
            pendingRuns.add (r);

    while (not pendingRuns.isEmpty())
    {
        int bestSlot { 0 };

        for (int slot { 1 }; slot < pendingRuns.size(); ++slot)
            if (runs.at (pendingRuns.at (slot)).length()
                > runs.at (pendingRuns.at (bestSlot)).length())
                bestSlot = slot;

        const auto r { pendingRuns.at (bestSlot) };
        const auto& run { runs.at (r) };
        const auto isStrike { run.type == static_cast<int> (Stamp::strike) };
        const auto isBold { run.type == static_cast<int> (Stamp::bold) };
        auto& parent { stack.isEmpty() ? rootElement : *stack.last() };

        auto* opened { document.addChild (
            parent, isStrike ? Id::del : (isBold ? Id::strong : Id::em)) };

        stack.add (opened);
        closeIndices.add (static_cast<int> (run.offset() + run.length()));
        pendingRuns.remove (bestSlot);
    }
}

void MarkdownDocument::InlineParser::addLinkElement (Element& parent,
                                                     const Document::Token& token,
                                                     jam::Array<Element*>& stack,
                                                     jam::Array<int>& closeIndices)
{
    const auto isImage { token.type == map::MarkdownTokenType::imageOpen };
    auto* opened { document.addChild (parent, isImage ? Id::img : Id::a) };
    opened->add<juce::String> (Id::href, *token.get<juce::String> (Id::href));

    if (token.contains (Id::title))
        opened->add<juce::String> (Id::title, *token.get<juce::String> (Id::title));

    stack.add (opened);
    closeIndices.add (static_cast<int> (*token.get<double> (Id::close)));
}

juce::String MarkdownDocument::InlineParser::getDecodedText (const std::string& inlineSource, const Document::Token& token)
{
    if (token.type == map::MarkdownTokenType::characterReference)
        return *token.get<juce::String> (Id::value);

    if (token.contains (Id::selfClosing))
    {
        auto next { (token.offset()) };
        ++next;
        return juce::String::charToString ((*Document::Cursor { inlineSource, next }));
    }

    return juce::String::fromUTF8 (
        inlineSource.data() + token.offset(), static_cast<int> (token.length()));
}

void MarkdownDocument::InlineParser::appendTextChild (Element& parent, const juce::String& text)
{
    if (text.isNotEmpty())
    {
        if (parent.lastChild != nullptr and parent.lastChild->isTag (Id::text))
        {
            *parent.lastChild->get<juce::String> (Id::text) += text;
        }
        else
        {
            auto* textNode { document.addChild (parent, Id::text) };
            textNode->add<juce::String> (Id::text, text);
        }
    }
}

void MarkdownDocument::InlineParser::addCodeSpanElement (Element& parent, const Document::Token& token, const std::string& inlineSource)
{
    auto* codeElement { document.addChild (parent, Id::code) };

    const auto& contentSpan { *token.get<Document::Token> (Id::name) };
    auto content { juce::String::fromUTF8 (
        inlineSource.data() + contentSpan.offset(), static_cast<int> (contentSpan.length())) };
    content = content.replaceCharacter (Chars::newline, Chars::space);

    const auto allSpaces { content.containsOnly (juce::String::charToString (Chars::space)) };
    const auto stripPadding { content.length() > 1 and content.startsWithChar (Chars::space)
                              and content.endsWithChar (Chars::space) and not allSpaces };
    const auto decoded { stripPadding ? content.substring (1, content.length() - 1) : content };

    if (decoded.isNotEmpty())
    {
        auto* codeText { document.addChild (*codeElement, Id::text) };
        codeText->add<juce::String> (Id::text, decoded);
    }
}

void MarkdownDocument::InlineParser::addLineBreakElement (Element& parent, const Document::Token& token)
{
    if (static_cast<int> (token.length()) == 2)
    {
        document.addChild (parent, Id::br);
    }
    else
    {
        const auto hasTrailingText { parent.lastChild != nullptr
                                     and parent.lastChild->isTag (Id::text) };
        const auto trailing { hasTrailingText ? *parent.lastChild->get<juce::String> (Id::text)
                                              : juce::String() };

        int trailingSpaceCount { 0 };

        while (trailingSpaceCount < trailing.length()
               and trailing[trailing.length() - 1 - trailingSpaceCount] == Chars::space)
            ++trailingSpaceCount;

        if (trailingSpaceCount >= minimumHardBreakSpaces)
        {
            *parent.lastChild->get<juce::String> (Id::text) =
                trailing.dropLastCharacters (trailingSpaceCount);
            document.addChild (parent, Id::br);
        }
        else
        {
            appendTextChild (parent, juce::String::charToString (Chars::newline));
        }
    }
}

jam::Function::Map<int, void> MarkdownDocument::InlineParser::getInlineConstructionTable (
    Element& element, jam::Array<Element*>& stack, const std::string& inlineSource)
{
    auto activeParent = [&stack, &element]() -> Element&
    {
        return stack.isEmpty() ? element : *stack.last();
    };

    jam::Function::Map<int, void> inlineConstruction;

    inlineConstruction.add<const Document::Token&> (
        map::MarkdownTokenType::emphasisDelimiter,
        [activeParent, this, &inlineSource] (const Document::Token& token)
        {
            const auto dLength { static_cast<int> (token.length()) };

            if (dLength > 0)
                appendTextChild (
                    activeParent(),
                    juce::String::repeatedString (
                        juce::String::charToString (
                            (*Document::Cursor { inlineSource, token.offset() })),
                        dLength));
        });

    inlineConstruction.add<const Document::Token&> (
        map::MarkdownTokenType::codeSpan,
        [activeParent, this, &inlineSource] (const Document::Token& token)
        { addCodeSpanElement (activeParent(), token, inlineSource); });

    inlineConstruction.add<const Document::Token&> (
        map::MarkdownTokenType::lineBreak,
        [activeParent, this] (const Document::Token& token)
        { addLineBreakElement (activeParent(), token); });

    return inlineConstruction;
}

void MarkdownDocument::InlineParser::buildInlineTree (Element& element,
                                                      const std::string& inlineSource,
                                                      const jam::Array<Document::Token>& tokens,
                                                      const jam::Array<Document::Token>& runs)
{
    jam::Array<Element*> stack;
    jam::Array<int> closeIndices;

    auto activeParent = [&stack, &element]() -> Element&
    {
        return stack.isEmpty() ? element : *stack.last();
    };

    auto closeElement = [&stack, &closeIndices]
    {
        stack.remove (stack.size() - 1);
        closeIndices.remove (closeIndices.size() - 1);
    };

    const auto inlineConstruction { getInlineConstructionTable (element, stack, inlineSource) };

    for (int index { 0 }; index < tokens.size(); ++index)
    {
        const auto& token { tokens.at (index) };

        while (not closeIndices.isEmpty() and closeIndices.last() == index)
            closeElement();

        if (inlineConstruction.contains (token.type))
        {
            inlineConstruction.get (token.type, token);
        }
        else if (token.type == map::MarkdownTokenType::linkClose)
        {
            if (not token.contains (Id::open))
                appendTextChild (activeParent(),
                                 juce::String::fromUTF8 (inlineSource.data() + token.offset(),
                                                         static_cast<int> (token.length())));
        }
        else if (token.type != map::MarkdownTokenType::linkOpen
                 and token.type != map::MarkdownTokenType::imageOpen)
        {
            appendTextChild (activeParent(), getDecodedText (inlineSource, token));
        }
        else if (not token.contains (Id::close))
        {
            appendTextChild (activeParent(),
                             juce::String::fromUTF8 (inlineSource.data() + token.offset(),
                                                     static_cast<int> (token.length())));
        }
        else
        {
            addLinkElement (activeParent(), token, stack, closeIndices);
        }

        addEmphasis (element, index, runs, stack, closeIndices);
    }

    while (not closeIndices.isEmpty())
        closeElement();
}

void MarkdownDocument::InlineParser::addTokens (jam::Array<Document::Token>& tokens, const std::string& inlineSource)
{
    size_t cursor { 0 };

    while (cursor < inlineSource.size())
    {
        const auto length { addToken (tokens, inlineSource, cursor) };

        if (length <= 0)
        {
#if JUCE_DEBUG
            debug::Log::write (
                "addInlines: addToken consumed nothing -- inline lex truncated");
#endif
            jassertfalse;
            break;
        }

        cursor += static_cast<size_t> (length);
    }
}

void MarkdownDocument::InlineParser::addInlines (Element& element, const juce::String& text)
{
    jam::Array<Document::Token> tokens;

    // Named local, not a temporary: stays alive through buildInlineTree() below,
    // so its own bytes -- not `document`'s -- are the correct resolution target
    // for every Token this nested lex produces (see buildInlineTree's header
    // comment). @p text is already block-preprocessed (joinIntoString() reads
    // from the outer Document's preprocessed source) -- no re-preprocessing here.
    const std::string inlineSource (text.toRawUTF8(), text.getNumBytesAsUTF8());

    addTokens (tokens, inlineSource);

    for (int index { 0 }; index < tokens.size(); ++index)
        if (tokens.at (index).type == map::MarkdownTokenType::emphasisDelimiter)
            getDelimiter (inlineSource, tokens, index);

    const auto runs { processEmphasis (inlineSource, tokens) };

    buildInlineTree (element, inlineSource, tokens, runs);
}

/*____________________________________________________________________________*/
} /** namespace jam */
