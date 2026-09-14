namespace jam
{
/*____________________________________________________________________________*/

MarkdownDocument::BlockParser::BlockParser (Document& newDocument, Element& newRoot)
    : document (newDocument)
    , root (newRoot)
{
}

Document::Element& MarkdownDocument::BlockParser::getBlockAt (int depth)
{
    Element* element { &root };
    int blockDepth { -1 };

    while (blockDepth < depth)
    {
        element = element->lastChild;

        if (*element->get<int> (Id::type) != map::BlockType::list)
            ++blockDepth;
    }

    return *element;
}

Document::Element& MarkdownDocument::BlockParser::getListAt (int depth)
{
    Element* element { &root };
    Element* lastList { &root };
    int blockDepth { -1 };

    while (blockDepth < depth)
    {
        element = element->lastChild;

        if (*element->get<int> (Id::type) == map::BlockType::list)
            lastList = element;
        else
            ++blockDepth;
    }

    return *lastList;
}

Document::Element& MarkdownDocument::BlockParser::getParent()
{
    return containerDepth == 0 ? root : getBlockAt (containerDepth - 1);
}

int MarkdownDocument::BlockParser::getChildCount (const Element& element) noexcept
{
    int count { 0 };

    for (auto* child : element)
    {
        (void) child;
        ++count;
    }

    return count;
}

Document::Element& MarkdownDocument::BlockParser::appendBlock (Element& parent, int blockType)
{
    auto* block { document.addChild (parent, map::BlockTag::getInstance()->get (blockType)) };
    block->add<int> (Id::type, blockType);
    return *block;
}

bool MarkdownDocument::BlockParser::isBlankLine (const std::string& source, size_t start, size_t end) noexcept
{
    for (auto ptr { start }; ptr != end; ++ptr)
        if ((*Document::Cursor { source, ptr })
                != Chars::space
            and (*Document::Cursor { source, ptr })
                    != Chars::tab
            and (*Document::Cursor { source, ptr })
                    != Chars::newline)
            return false;

    return true;
}

size_t MarkdownDocument::BlockParser::trimStart (const std::string& source, size_t start, size_t end) noexcept
{
    auto ptr { start };

    while (ptr != end
           and ((*Document::Cursor { source, ptr })
                    == Chars::space
                or (*Document::Cursor { source, ptr })
                       == Chars::tab))
        ++ptr;

    return ptr;
}

size_t MarkdownDocument::BlockParser::trimEnd (const std::string& source, size_t start, size_t end) noexcept
{
    while (end != start)
    {
        auto probe { end };
        --probe;

        if ((*Document::Cursor { source, probe })
                != Chars::space
            and (*Document::Cursor { source, probe })
                    != Chars::tab)
            break;

        end = probe;
    }

    return end;
}

int MarkdownDocument::BlockParser::countLeadingSpaces (const std::string& source,
                                                       size_t start,
                                                       size_t end,
                                                       int maxColumns) noexcept
{
    int columns { 0 };
    auto ptr { start };

    while (ptr != end
           and (*Document::Cursor { source, ptr })
                   == Chars::space
           and columns < maxColumns)
    {
        ++columns;
        ++ptr;
    }

    return columns;
}

bool
MarkdownDocument::BlockParser::hasUnindentedStart (const std::string& source, size_t start, size_t end) noexcept
{
    int columns { 0 };
    auto ptr { start };

    while (ptr != end
           and (*Document::Cursor { source, ptr })
                   == Chars::space
           and columns <= maxUnindentedColumns)
    {
        ++columns;
        ++ptr;
    }

    return columns <= maxUnindentedColumns
           and (ptr == end
                or (*Document::Cursor { source, ptr })
                       != Chars::tab);
}

juce::String
MarkdownDocument::BlockParser::joinIntoString (const std::string& source, const jam::Array<Document::Token>& lineSpans)
{
    size_t totalLength { 0 };

    for (int index { 0 }; index < lineSpans.size(); ++index)
        totalLength += lineSpans.at (index).length() + (index > 0 ? 1 : 0);

    std::string joined;
    joined.reserve (totalLength);

    for (int index { 0 }; index < lineSpans.size(); ++index)
    {
        if (index > 0)
            joined += Chars::newline;

        const auto& lineSpan { lineSpans.at (index) };
        joined.append (source.data() + lineSpan.offset(), lineSpan.length());
    }

    return juce::String::fromUTF8 (joined.data(), static_cast<int> (joined.size()));
}

void MarkdownDocument::BlockParser::closeCodeBlock (Element& block, jam::Array<Document::Token> lines)
{
    const auto fenced { block.contains (Id::marker)
                        and block.get<juce::String> (Id::marker)->isNotEmpty() };

    if (not fenced)
        while (lines.size() > 0 and lines.last().length() == 0)
            lines.remove (lines.size() - 1);

    const auto info { block.get<juce::String> (Id::info, juce::String()) };
    const auto infoFirstWord { info.upToFirstOccurrenceOf (
        juce::String::charToString (Chars::space), false, false) };

    if (fenced and infoFirstWord.compare (Id::mermaid.toString()) == 0)
    {
        *block.get<int> (Id::type) = map::BlockType::mermaid;
        block.id = Id::mermaid;
    }

    leafText.try_emplace (&block, joinIntoString (document.getSource(), lines));
}

void MarkdownDocument::BlockParser::closeLeaf()
{
    if (isLeafOpen)
    {
        auto& leaf { getLeaf() };
        const auto type { *leaf.get<int> (Id::type) };

        if (type == map::BlockType::codeBlock)
            closeCodeBlock (leaf, std::move (leafLines));
        else if (type == map::BlockType::htmlBlock)
            leafText.try_emplace (&leaf, joinIntoString (document.getSource(), leafLines));
        else if (type == map::BlockType::paragraph)
        {
            if (not addSoleImage (leaf, leafLines))
                leafText.try_emplace (
                    &leaf, joinIntoString (document.getSource(), leafLines));
        }
        else if (type == map::BlockType::table)
        {
            if (isGridTableHeaderDone)
                addAccumulatedRow (leaf);
            else
                addAccumulatedHeaderRow (leaf);
        }
    }

    isLeafOpen = false;
    leafLines.clear();
    isGridTableHeaderDone = false;
    gridRow = GridRow();
}

bool MarkdownDocument::BlockParser::addSoleImage (Element& block, const jam::Array<Document::Token>& lines)
{
    if (lines.size() != 1)
        return false;

    const auto& source { document.getSource() };
    const auto rawStart { (lines.first().offset()) };
    const auto rawEnd { (lines.first().offset() + lines.first().length()) };
    const auto start { trimStart (source, rawStart, rawEnd) };
    const auto end { trimEnd (source, start, rawEnd) };

    if (static_cast<int> ((end - start)) < minSoleImageLength
        or charAt (source, start, end, 0) != Chars::exclamation
        or charAt (source, start, end, 1) != Chars::openBracket)
        return false;

    const auto altToken { getBracketedText (source, start, end, 2) };
    const auto alt { *altToken.get<juce::String> (Id::label) };
    const auto cursor { 2 + static_cast<int> (altToken.length()) };

    if (charAt (source, start, end, cursor) != Chars::closeBracket)
        return false;

    const auto tailToken { getAbsoluteImageTail (source, start, end, cursor + 1) };

    if (not tailToken.contains (Id::href))
        return false;

    *block.get<int> (Id::type) = map::BlockType::image;
    block.id = Id::img;
    block.add<juce::String> (Id::url, *tailToken.get<juce::String> (Id::href));
    block.add<juce::String> (Id::title, *tailToken.get<juce::String> (Id::title));
    leafText.try_emplace (&block, alt);
    return true;
}

void MarkdownDocument::BlockParser::addParagraphLine (size_t start, size_t end)
{
    const auto& source { document.getSource() };
    leafLines.add (Document::Token (trimStart (source, start, end), end));

    if (not isLeafOpen)
    {
        appendBlock (getParent(), map::BlockType::paragraph);
        isLeafOpen = true;
    }
}

int MarkdownDocument::BlockParser::getSetextLevel (const std::string& source, size_t start, size_t end) noexcept
{
    const auto indent { countLeadingSpaces (source, start, end, maxUnindentedColumns + 1) };

    if (indent > maxUnindentedColumns)
        return 0;

    auto contentStart { advance (start, end, indent) };
    const auto contentEnd { trimEnd (source, contentStart, end) };

    if (contentStart == contentEnd)
        return 0;

    const auto marker { (*Document::Cursor { source, contentStart }) };
    const auto isEquals { marker == Chars::equals };

    if (not isEquals and marker != Chars::dash)
        return 0;

    if (not isMarkdownOperatorChar (marker))
        return 0;

    const auto leadRun { getOperatorToken (source, contentStart) };

    if (leadRun.type != map::DocumentTokenType::operators
        or (leadRun.offset() + leadRun.length()) != contentEnd)
        return 0;

    for (auto ptr { contentStart }; ptr != contentEnd; ++ptr)
        if ((*Document::Cursor { source, ptr })
            != marker)
            return 0;

    return isEquals ? 1 : 2;
}

bool
MarkdownDocument::BlockParser::isThematicBreakContent (const std::string& source, size_t start, size_t end) noexcept
{
    juce::juce_wchar marker { 0 };
    int markerCount { 0 };

    for (auto ptr { start }; ptr != end; ++ptr)
    {
        const auto ch { (*Document::Cursor { source, ptr }) };

        if (ch != Chars::space and ch != Chars::tab)
        {
            if (ch != Chars::dash and ch != Chars::underscore and ch != Chars::asterisk)
                return false;

            if (markerCount > 0 and ch != marker)
                return false;

            marker = ch;
            ++markerCount;
        }
    }

    return markerCount >= minThematicBreakLength;
}

bool MarkdownDocument::BlockParser::addThematicBreak (size_t start, size_t end, size_t& remainderStart)
{
    const auto& source { document.getSource() };
    const auto matched { hasUnindentedStart (source, start, end)
                         and isThematicBreakContent (
                             source, trimStart (source, start, end), end) };

    if (matched)
    {
        closeLeaf();

        appendBlock (getParent(), map::BlockType::thematicBreak);
        remainderStart = end;
    }

    return matched;
}

juce::String MarkdownDocument::BlockParser::trimAtxClosingSequence (const juce::String& text)
{
    auto stripped { text };

    while (stripped.endsWithChar (Chars::hash))
        stripped = stripped.dropLastCharacters (1);

    const auto hashesDropped { text.length() != stripped.length() };
    const auto closingValid { hashesDropped
                              and (stripped.isEmpty()
                                   or stripped.endsWithChar (Chars::space)
                                   or stripped.endsWithChar (Chars::tab)) };

    return (closingValid ? stripped : text).trim();
}

int MarkdownDocument::BlockParser::getAtxHeadingLevel (const std::string& source, size_t start, size_t end) noexcept
{
    const auto leadChar { charAt (source, start, end, 0) };

    if (not isMarkdownOperatorChar (leadChar) or leadChar != Chars::hash)
        return 0;

    const auto leadRun { getOperatorToken (source, start) };

    if (leadRun.type != map::DocumentTokenType::operators)
        return 0;

    int hashCount { 0 };
    auto ptr { (leadRun.offset()) };
    const auto leadRunEnd { (leadRun.offset() + leadRun.length()) };

    for (; ptr != leadRunEnd
           and (*Document::Cursor { source, ptr })
                   == Chars::hash
           and hashCount <= maxAtxLevel;
         ++ptr)
        ++hashCount;

    const auto validCount { hashCount >= 1 and hashCount <= maxAtxLevel };
    const auto terminated {
        ptr == end
        or (*Document::Cursor { source, ptr })
               == Chars::space
        or (*Document::Cursor { source, ptr })
               == Chars::tab
    };

    return validCount and terminated ? hashCount : 0;
}

juce::String
MarkdownDocument::BlockParser::getAtxHeadingText (const std::string& source, size_t start, size_t end, int level)
{
    const auto textStart { trimStart (source, advance (start, end, level), end) };
    const auto textEnd { trimEnd (source, textStart, end) };
    return trimAtxClosingSequence (juce::String::fromUTF8 (
        source.data() + textStart, static_cast<int> (textEnd - textStart)));
}

void MarkdownDocument::BlockParser::addHeading (Element& parent, int level, const juce::String& text)
{
    auto* heading { document.addChild (parent, map::HeadingTag::getInstance()->get (level)) };
    heading->add<int> (Id::type, map::BlockType::heading);
    heading->add<juce::String> (Id::level, juce::String (level));
    heading->add<juce::String> (Id::id, text);
    leafText.try_emplace (heading, text);
}

bool MarkdownDocument::BlockParser::addAtxHeading (size_t start, size_t end, size_t& remainderStart)
{
    const auto& source { document.getSource() };
    const auto trimmedStart { trimStart (source, start, end) };
    const auto level { hasUnindentedStart (source, start, end)
                           ? getAtxHeadingLevel (source, trimmedStart, end)
                           : 0 };

    if (level == 0)
        return false;

    closeLeaf();
    addHeading (getParent(), level, getAtxHeadingText (source, trimmedStart, end, level));
    remainderStart = end;
    return true;
}

int
MarkdownDocument::BlockParser::getBlockquoteMarker (const std::string& source, size_t start, size_t end) noexcept
{
    const auto indent { countLeadingSpaces (source, start, end, maxUnindentedColumns + 1) };

    if (indent > maxUnindentedColumns
        or charAt (source, start, end, indent) != Chars::greaterThan)
        return 0;

    const auto hasGapSpace { charAt (source, start, end, indent + 1) == Chars::space };

    return indent + 1 + (hasGapSpace ? 1 : 0);
}

bool MarkdownDocument::BlockParser::addBlockquote (size_t start, size_t end, size_t& remainderStart)
{
    const auto length { getBlockquoteMarker (document.getSource(), start, end) };
    const auto matched { length > 0 };

    if (matched)
    {
        closeLeaf();
        appendBlock (getParent(), map::BlockType::blockquote);
        containerDepth += 1;
        remainderStart = advance (start, end, length);
    }

    return matched;
}

int
MarkdownDocument::BlockParser::getTaskListMarkerLength (const std::string& source, size_t start, size_t end) noexcept
{
    static constexpr int markerLength { 3 };
    const auto length { static_cast<int> ((end - start)) };
    const auto stateChar { charAt (source, start, end, 1) };
    const auto isCheckbox {
        charAt (source, start, end, 0) == Chars::openBracket and length >= markerLength
        and charAt (source, start, end, 2) == Chars::closeBracket
        and (stateChar == Chars::space or stateChar == Chars::lowerX
             or stateChar == Chars::upperX)
        and (length == markerLength
             or charAt (source, start, end, markerLength) == Chars::space)
    };

    return isCheckbox ? markerLength : 0;
}

bool
MarkdownDocument::BlockParser::isTaskListMarkerChecked (const std::string& source, size_t start, size_t end) noexcept
{
    return charAt (source, start, end, 1) != Chars::space;
}

int
MarkdownDocument::BlockParser::getBulletMarkerWidth (const std::string& source, size_t start, size_t end) noexcept
{
    const auto ch { charAt (source, start, end, 0) };
    const auto isBullet { ch == Chars::dash or ch == Chars::plus or ch == Chars::asterisk };

    return isBullet ? 1 : 0;
}

int
MarkdownDocument::BlockParser::getOrderedMarkerWidth (const std::string& source, size_t start, size_t end) noexcept
{
    int digitCount { 0 };
    auto ptr { start };

    while (ptr != end
           and (*Document::Cursor { source, ptr })
                   >= Chars::zero
           and (*Document::Cursor { source, ptr })
                   <= Chars::nine
           and digitCount < maxOrderedDigits)
    {
        ++digitCount;
        ++ptr;
    }

    const auto delim { charAt (source, start, end, digitCount) };
    const auto matched { digitCount >= 1
                         and (delim == Chars::dot or delim == Chars::closeParen) };

    return matched ? digitCount + 1 : 0;
}

int MarkdownDocument::BlockParser::getOrderedMarkerValue (const std::string& source,
                                                          size_t start,
                                                          size_t end,
                                                          int width) noexcept
{
    const auto valueEnd { advance (start, end, width - 1) };
    return juce::String::fromUTF8 (
               source.data() + start, static_cast<int> (valueEnd - start))
        .getIntValue();
}

int
MarkdownDocument::BlockParser::getListMarkerWidth (const std::string& source, size_t contentStart, size_t end) noexcept
{
    const auto bulletWidth { getBulletMarkerWidth (source, contentStart, end) };

    if (bulletWidth > 0)
        return isThematicBreakContent (source, contentStart, end) ? 0 : bulletWidth;

    return getOrderedMarkerWidth (source, contentStart, end);
}

int MarkdownDocument::BlockParser::getListItemIndentWidth (const std::string& source,
                                                           size_t start,
                                                           size_t end,
                                                           int markerWidth) noexcept
{
    const auto gap { countLeadingSpaces (source, start, end, maxListMarkerGap + 1) };
    const auto blank { isBlankLine (source, start, end) };

    if (gap == 0 and not blank)
        return 0;

    return markerWidth + (blank or gap > maxListMarkerGap ? 1 : gap);
}

int
MarkdownDocument::BlockParser::getListItemGapLength (const std::string& source, size_t start, size_t end) noexcept
{
    const auto gap { countLeadingSpaces (source, start, end, maxListMarkerGap + 1) };
    const auto blank { isBlankLine (source, start, end) };

    return blank ? 0 : (gap <= maxListMarkerGap ? gap : 1);
}

bool MarkdownDocument::BlockParser::isSameListType (const Element& list,
                                                    bool ordered,
                                                    juce::juce_wchar bulletChar,
                                                    juce::juce_wchar delim)
{
    const auto existing { list.get<juce::String> (Id::marker, juce::String()) };
    const auto existingChar { existing.isNotEmpty() ? existing[0] : juce::juce_wchar (0) };
    const auto existingOrdered { existingChar == Chars::dot
                                 or existingChar == Chars::closeParen };

    return ordered == existingOrdered and existingChar == (ordered ? delim : bulletChar);
}

void MarkdownDocument::BlockParser::addListItem (bool ordered,
                                                 juce::juce_wchar bulletChar,
                                                 juce::juce_wchar delim,
                                                 int startNumber,
                                                 int indent,
                                                 const juce::String& text,
                                                 size_t valueStart,
                                                 size_t valueEnd)
{
    auto& parent { getParent() };
    auto* lastChild { parent.lastChild };
    const auto reuseExisting {
        lastChild != nullptr and *lastChild->get<int> (Id::type) == map::BlockType::list
        and isSameListType (*lastChild, ordered, bulletChar, delim)
    };

    if (not reuseExisting)
    {
        auto* list { document.addChild (parent, ordered ? Id::ol : Id::ul) };
        list->add<int> (Id::type, map::BlockType::list);
        list->add<juce::String> (
            Id::marker, juce::String::charToString (ordered ? delim : bulletChar));

        if (ordered)
            list->add<juce::String> (Id::start, juce::String (startNumber));
    }

    auto& list { *parent.lastChild };
    const auto position { getChildCount (list) };
    const auto startValue { list.contains (Id::start)
                                ? list.get<juce::String> (Id::start)->getIntValue()
                                : 0 };
    const auto displayNumber { startValue + position };
    const auto token { ordered ? juce::String (displayNumber)
                                     + juce::String::charToString (delim)
                               : juce::String::charToString (bulletChar) };

    const auto itemId { text.isNotEmpty()
                            ? juce::Identifier (Format::toValidID (Format::getPreColon (text)))
                            : map::BlockTag::getInstance()->get (map::BlockType::listItem) };

    auto* item { document.addChild (list, itemId) };
    item->add<int> (Id::type, map::BlockType::listItem);
    item->add<juce::String> (Id::marker, token);
    item->add<juce::String> (Id::indent, juce::String (indent));

    if (valueEnd > valueStart)
    {
        jam::Array<Document::Token> valueTokens;
        valueTokens.add (Document::Token (valueStart, valueEnd));
        item->add<jam::Array<Document::Token>> (Id::tokens, std::move (valueTokens));
    }

    containerDepth += 1;
}

int MarkdownDocument::BlockParser::getListItemLeadWidth (size_t contentStart,
                                                         size_t end,
                                                         int markerWidth,
                                                         bool ordered,
                                                         int orderedValue,
                                                         bool allContainersMatched)
{
    const auto& source { document.getSource() };
    const auto interruptingParagraph { allContainersMatched and isLeafOpen
                                       and *getLeaf().get<int> (Id::type)
                                               == map::BlockType::paragraph };
    const auto afterMarkerStart { advance (contentStart, end, markerWidth) };
    const auto startsBlank { isBlankLine (source, afterMarkerStart, end) };
    const auto badOrderedStart { ordered and orderedValue != 1 };

    if (interruptingParagraph and (startsBlank or badOrderedStart))
        return 0;

    return getListItemIndentWidth (source, afterMarkerStart, end, markerWidth);
}

bool
MarkdownDocument::BlockParser::addListItem (size_t start, size_t end, size_t& remainderStart, bool allContainersMatched)
{
    const auto& source { document.getSource() };
    const auto contentStart { advance (
        start, end, countLeadingSpaces (source, start, end, maxUnindentedColumns)) };
    const auto markerWidth { getListMarkerWidth (source, contentStart, end) };

    if (markerWidth == 0)
        return false;

    const auto ordered { Document::isDigit (charAt (source, contentStart, end, 0)) };
    const auto delim { ordered ? charAt (source, contentStart, end, markerWidth - 1)
                               : juce::juce_wchar (0) };
    const auto bulletChar { ordered ? juce::juce_wchar (0)
                                    : charAt (source, contentStart, end, 0) };
    const auto orderedValue {
        ordered ? getOrderedMarkerValue (source, contentStart, end, markerWidth) : 0
    };

    const auto indentWidth { getListItemLeadWidth (
        contentStart, end, markerWidth, ordered, orderedValue, allContainersMatched) };

    if (indentWidth == 0)
        return false;

    closeLeaf();

    const auto afterMarkerStart { advance (contentStart, end, markerWidth) };
    const auto gapLength { getListItemGapLength (source, afterMarkerStart, end) };
    auto firstLineStart { advance (afterMarkerStart, end, gapLength) };
    const auto taskLength { getTaskListMarkerLength (source, firstLineStart, end) };
    const auto checked { taskLength > 0
                             and isTaskListMarkerChecked (source, firstLineStart, end) };

    if (taskLength > 0)
        firstLineStart = trimStart (source, advance (firstLineStart, end, taskLength), end);

    const auto textStart { trimStart (source, firstLineStart, end) };
    const auto textEnd { trimEnd (source, textStart, end) };
    const juce::String text { juce::String::fromUTF8 (
        source.data() + textStart, static_cast<int> (textEnd - textStart)) };

    int colonOffset { 0 };

    while (charAt (source, textStart, textEnd, colonOffset) != 0
           and charAt (source, textStart, textEnd, colonOffset) != Chars::colon)
        ++colonOffset;

    const auto hasColon { charAt (source, textStart, textEnd, colonOffset) == Chars::colon };
    const auto valueStart { hasColon
                                ? trimStart (source, textStart + static_cast<size_t> (colonOffset) + 1, textEnd)
                                : textEnd };
    const auto valueEnd { textEnd };

    addListItem (ordered, bulletChar, delim, orderedValue, indentWidth, text, valueStart, valueEnd);

    if (taskLength > 0)
    {
        auto& openedItem { getParent() };
        openedItem.add<juce::String> (Id::checked, Format::fromBoolean (checked));
    }

    remainderStart = firstLineStart;
    return true;
}

int MarkdownDocument::BlockParser::measureFenceRun (const std::string& source,
                                                    size_t start,
                                                    size_t end,
                                                    juce::juce_wchar fenceChar) noexcept
{
    int length { 0 };
    auto ptr { start };

    while (ptr != end
           and (*Document::Cursor { source, ptr })
                   == fenceChar)
    {
        ++length;
        ++ptr;
    }

    return length;
}

int MarkdownDocument::BlockParser::getFenceOpenWidth (const std::string& source, size_t start, size_t end)
{
    const auto indent { countLeadingSpaces (source, start, end, maxUnindentedColumns) };
    const auto contentStart { advance (start, end, indent) };
    const auto fenceChar { charAt (source, contentStart, end, 0) };

    if (fenceChar != Chars::backtick and fenceChar != Chars::tilde)
        return 0;

    const auto leadRun { getOperatorToken (source, contentStart) };

    if (leadRun.type != map::DocumentTokenType::operators)
        return 0;

    const auto fenceLength { measureFenceRun (
        source, contentStart, (leadRun.offset() + leadRun.length()), fenceChar) };

    if (fenceLength < minFenceLength)
        return 0;

    const auto infoTrimStart { trimStart (
        source, advance (contentStart, end, fenceLength), end) };
    const auto infoTrimEnd { trimEnd (source, infoTrimStart, end) };
    const juce::String info { juce::String::fromUTF8 (
        source.data() + infoTrimStart, static_cast<int> (infoTrimEnd - infoTrimStart)) };
    const auto validInfo { fenceChar != Chars::backtick
                           or not info.containsChar (Chars::backtick) };

    return validInfo ? fenceLength : 0;
}

bool MarkdownDocument::BlockParser::addFencedCodeBlock (size_t start, size_t end)
{
    const auto& source { document.getSource() };
    const auto fenceLength { getFenceOpenWidth (source, start, end) };

    if (fenceLength == 0)
        return false;

    const auto indent { countLeadingSpaces (source, start, end, maxUnindentedColumns) };
    const auto contentStart { advance (start, end, indent) };
    const auto infoTrimStart { trimStart (
        source, advance (contentStart, end, fenceLength), end) };
    const auto infoEnd { trimEnd (source, infoTrimStart, end) };
    const juce::String info { juce::String::fromUTF8 (
        source.data() + infoTrimStart, static_cast<int> (infoEnd - infoTrimStart)) };
    const auto tokenEnd { advance (contentStart, end, fenceLength) };
    const juce::String token { juce::String::fromUTF8 (
        source.data() + contentStart, static_cast<int> (tokenEnd - contentStart)) };

    closeLeaf();

    const auto infoFirstWord { info.upToFirstOccurrenceOf (
        juce::String::charToString (Chars::space), false, false) };
    const auto blockId { infoFirstWord.isNotEmpty()
                             ? juce::Identifier (infoFirstWord)
                             : map::BlockTag::getInstance()->get (map::BlockType::codeBlock) };

    auto* block { document.addChild (getParent(), blockId) };
    block->add<int> (Id::type, map::BlockType::codeBlock);
    block->add<juce::String> (Id::marker, token);
    block->add<juce::String> (Id::info, info);
    block->add<juce::String> (Id::indent, juce::String (indent));
    isLeafOpen = true;
    leafLines.clear();
    return true;
}

bool MarkdownDocument::BlockParser::addFencedCode (size_t start, size_t end, size_t& remainderStart)
{
    const auto matched { addFencedCodeBlock (start, end) };

    if (matched)
        remainderStart = end;

    return matched;
}

bool MarkdownDocument::BlockParser::isClosingFence (const std::string& source,
                                                    size_t start,
                                                    size_t end,
                                                    const juce::String& fenceToken) noexcept
{
    const auto indent { countLeadingSpaces (source, start, end, maxUnindentedColumns) };
    const auto contentStart { advance (start, end, indent) };
    const auto fenceChar { fenceToken[0] };

    if (charAt (source, contentStart, end, 0) != fenceChar)
        return false;

    const auto leadRun { getOperatorToken (source, contentStart) };
    const auto runLength {
        leadRun.type == map::DocumentTokenType::operators
            ? measureFenceRun (
                  source, contentStart, (leadRun.offset() + leadRun.length()), fenceChar)
            : 0
    };

    return runLength >= fenceToken.length()
           and isBlankLine (source, advance (contentStart, end, runLength), end);
}

bool MarkdownDocument::BlockParser::addFencedCodeLine (size_t start, size_t end)
{
    const auto& source { document.getSource() };
    const auto fencedOpen { isLeafOpen
                            and *getLeaf().get<int> (Id::type) == map::BlockType::codeBlock
                            and getLeaf().contains (Id::marker) };

    if (not fencedOpen)
        return false;

    auto& leaf { getLeaf() };
    const auto fenceToken { *leaf.get<juce::String> (Id::marker) };

    if (isClosingFence (source, start, end, fenceToken))
    {
        closeLeaf();
        return true;
    }

    const auto fenceIndent { leaf.get<juce::String> (Id::indent)->getIntValue() };
    leafLines.add (Document::Token (
        advance (start, end, countLeadingSpaces (source, start, end, fenceIndent)), end));
    return true;
}

void MarkdownDocument::BlockParser::addIndentedCodeBlock (Element& parent, size_t firstLineStart, size_t firstLineEnd)
{
    appendBlock (parent, map::BlockType::codeBlock);
    isLeafOpen = true;
    leafLines.clear();
    leafLines.add (Document::Token (firstLineStart, firstLineEnd));
}

int
MarkdownDocument::BlockParser::getEndOfTagName (const std::string& source, size_t start, size_t end, int cursor) noexcept
{
    if (not isAsciiLetter (charAt (source, start, end, cursor)))
        return cursor;

    auto ptr { advance (start, end, cursor + 1) };
    auto index { cursor + 1 };

    while (ptr != end
           and isTagNameChar (
               (*Document::Cursor { source, ptr })))
    {
        ++ptr;
        ++index;
    }

    return index;
}

juce::String
MarkdownDocument::BlockParser::tagText (const std::string& source, size_t start, size_t end, int from, int to)
{
    const auto tagStart { advance (start, end, from) };
    const auto tagEnd { advance (start, end, to) };
    return juce::String::fromUTF8 (
        source.data() + tagStart, static_cast<int> (tagEnd - tagStart));
}

bool MarkdownDocument::BlockParser::isHtmlBlockType1Start (const std::string& source, size_t start, size_t end)
{
    const auto nameEnd { getEndOfTagName (source, start, end, 1) };
    const auto after { charAt (source, start, end, nameEnd) };

    return nameEnd > 1
           and map::HtmlType1Tag::getInstance()->contains (
               tagText (source, start, end, 1, nameEnd))
           and (after == 0 or after == Chars::space or after == Chars::tab
                or after == Chars::greaterThan);
}

bool
MarkdownDocument::BlockParser::isHtmlBlockType2Start (const std::string& source, size_t start, size_t end) noexcept
{
    (void) end;
    return Document::Cursor { source, start }.startsWith (Chars::markupCommentOpen);
}

bool
MarkdownDocument::BlockParser::isHtmlBlockType3Start (const std::string& source, size_t start, size_t end) noexcept
{
    (void) end;
    return Document::Cursor { source, start }.startsWith (Chars::processingOpen);
}

bool
MarkdownDocument::BlockParser::isHtmlBlockType4Start (const std::string& source, size_t start, size_t end) noexcept
{
    return Document::Cursor { source, start }.startsWith (Chars::declarationOpen)
           and isAsciiLetter (charAt (source, start, end, 2));
}

bool
MarkdownDocument::BlockParser::isHtmlBlockType5Start (const std::string& source, size_t start, size_t end) noexcept
{
    (void) end;
    return Document::Cursor { source, start }.startsWith (Chars::cdataOpen);
}

bool MarkdownDocument::BlockParser::isHtmlBlockType6Start (const std::string& source, size_t start, size_t end)
{
    const auto hasSlash { charAt (source, start, end, 1) == Chars::slash };
    const auto nameStart { hasSlash ? 2 : 1 };
    const auto nameEnd { getEndOfTagName (source, start, end, nameStart) };

    if (nameEnd == nameStart
        or not map::HtmlBlockTag::getInstance()->contains (
            tagText (source, start, end, nameStart, nameEnd)))
        return false;

    const auto after { charAt (source, start, end, nameEnd) };
    const auto selfClosing { after == Chars::slash
                             and charAt (source, start, end, nameEnd + 1)
                                     == Chars::greaterThan };

    return after == 0 or after == Chars::space or after == Chars::tab
           or after == Chars::greaterThan or selfClosing;
}

bool MarkdownDocument::BlockParser::isHtmlBlockType7Start (const std::string& source, size_t start, size_t end)
{
    const auto openEnd { getEndOfOpenTag (source, start, end, 1) };
    const auto tagEnd { openEnd > 1 ? openEnd : getEndOfClosingTag (source, start, end, 1) };

    return tagEnd > 1 and isBlankLine (source, advance (start, end, tagEnd), end);
}

int MarkdownDocument::BlockParser::getHtmlBlockCondition (const std::string& source, size_t start, size_t end)
{
    if (charAt (source, start, end, 0) != Chars::lessThan)
        return map::HtmlBlockType::none;

    for (int type { map::HtmlBlockType::rawText }; type <= map::HtmlBlockType::anyTag;
         ++type)
    {
        const auto condition { htmlBlockConditions[type] };

        if (condition != nullptr and condition (source, start, end))
            return type;
    }

    return map::HtmlBlockType::none;
}

bool MarkdownDocument::BlockParser::spanContains (const std::string& source,
                                                  size_t start,
                                                  size_t end,
                                                  const char* literal) noexcept
{
    for (auto ptr { start }; ptr != end; ++ptr)
        if (Document::Cursor { source, ptr }.startsWith (literal))
            return true;

    return false;
}

bool
MarkdownDocument::BlockParser::isHtmlBlockEnd (const std::string& source, size_t start, size_t end, int conditionType)
{
    if (conditionType == map::HtmlBlockType::rawText)
    {
        for (const auto* marker : map::rawTextEnd)
            if (marker != nullptr and spanContains (source, start, end, marker))
                return true;

        return false;
    }

    const auto closeMarker { map::markupClose[conditionType] };

    return closeMarker != nullptr and spanContains (source, start, end, closeMarker);
}

void MarkdownDocument::BlockParser::addHtmlBlock (Element& parent, int conditionType)
{
    auto& block { appendBlock (parent, map::BlockType::htmlBlock) };
    block.add<juce::String> (Id::mode, juce::String (conditionType));
    isLeafOpen = true;
    leafLines.clear();
}

bool MarkdownDocument::BlockParser::addHtmlBlockLine (size_t start, size_t end)
{
    const auto& source { document.getSource() };
    const auto htmlOpen { isLeafOpen
                          and *getLeaf().get<int> (Id::type) == map::BlockType::htmlBlock };

    if (not htmlOpen)
        return false;

    const auto conditionType { getLeaf().get<juce::String> (Id::mode)->getIntValue() };
    const auto blankClosesHere { (conditionType == map::HtmlBlockType::blockTag
                                  or conditionType == map::HtmlBlockType::anyTag)
                                 and isBlankLine (source, start, end) };

    if (blankClosesHere)
        return false;

    leafLines.add (Document::Token (start, end));

    if (isHtmlBlockEnd (source, start, end, conditionType))
        closeLeaf();

    return true;
}

bool MarkdownDocument::BlockParser::addHtmlBlock (size_t start, size_t end, size_t& remainderStart)
{
    const auto& source { document.getSource() };

    if (not hasUnindentedStart (source, start, end))
        return false;

    const auto contentStart { trimStart (source, start, end) };
    const auto conditionType { getHtmlBlockCondition (source, contentStart, end) };
    const auto interruptingParagraph {
        isLeafOpen and *getLeaf().get<int> (Id::type) == map::BlockType::paragraph
    };

    if (conditionType == map::HtmlBlockType::none
        or (conditionType == map::HtmlBlockType::anyTag and interruptingParagraph))
        return false;

    closeLeaf();
    addHtmlBlock (getParent(), conditionType);
    addHtmlBlockLine (contentStart, end);
    remainderStart = end;
    return true;
}

Document::Token
MarkdownDocument::BlockParser::getBracketedText (const std::string& source, size_t start, size_t end, int cursor)
{
    const auto textStart { advance (start, end, cursor) };
    auto ptr { textStart };

    while (ptr != end
           and (*Document::Cursor { source, ptr })
                   != Chars::closeBracket
           and static_cast<int> (ptr - textStart) < maxLabelLength)
        ++ptr;

    Document::Token token (textStart, ptr);
    token.add<juce::String> (Id::label,
        juce::String::fromUTF8 (source.data() + textStart, static_cast<int> (ptr - textStart)));
    return token;
}

Document::Token
MarkdownDocument::BlockParser::getReferenceLabel (const std::string& source, size_t start, size_t end)
{
    const auto indent { countLeadingSpaces (source, start, end, maxUnindentedColumns + 1) };

    if (indent > maxUnindentedColumns
        or charAt (source, start, end, indent) != Chars::openBracket)
        return {};

    const auto rawLabelToken { getBracketedText (source, start, end, indent + 1) };
    const auto rawLabel { *rawLabelToken.get<juce::String> (Id::label) };
    const auto cursor { indent + 1 + static_cast<int> (rawLabelToken.length()) };
    const auto closed { charAt (source, start, end, cursor) == Chars::closeBracket };
    const auto hasColon { closed
                          and charAt (source, start, end, cursor + 1) == Chars::colon };

    if (rawLabel.trim().isEmpty() or not hasColon)
        return {};

    Document::Token token (start, start + static_cast<size_t> (cursor + 2));
    token.add<juce::String> (Id::label, rawLabel);
    return token;
}

Document::Token
MarkdownDocument::BlockParser::getReferenceDestinationAndTitle (const std::string& source,
                                                                size_t start,
                                                                size_t end,
                                                                int cursor)
{
    auto ptr { advance (start, end, cursor) };

    while (ptr != end
           and (*Document::Cursor { source, ptr })
                   == Chars::space)
    {
        ++ptr;
        ++cursor;
    }

    const auto destinationToken { getLinkDestination (source, start, end, cursor) };

    if (not destinationToken.contains (Id::href))
        return {};

    cursor += static_cast<int> (destinationToken.length());
    int spaceRun { 0 };

    while (charAt (source, start, end, cursor + spaceRun) == Chars::space)
        ++spaceRun;

    const auto titleToken { spaceRun > 0 ? getLinkTitle (source, start, end, cursor + spaceRun)
                                         : Document::Token() };
    const auto titleLength { static_cast<int> (titleToken.length()) };
    cursor += titleLength > 0 ? spaceRun + titleLength : 0;

    Document::Token token (start, start + static_cast<size_t> (cursor));
    token.add<juce::String> (Id::href, *destinationToken.get<juce::String> (Id::href));
    token.add<juce::String> (
        Id::title, titleLength > 0 ? *titleToken.get<juce::String> (Id::title) : juce::String());
    return token;
}

bool MarkdownDocument::BlockParser::addReferenceLine (const std::string& source, size_t start, size_t end)
{
    const auto labelToken { getReferenceLabel (source, start, end) };

    if (not labelToken.contains (Id::label))
        return false;

    const auto tailToken { getReferenceDestinationAndTitle (
        source, start, end, static_cast<int> (labelToken.length())) };

    if (not tailToken.contains (Id::href))
        return false;

    if (not isBlankLine (
            source, advance (start, end, static_cast<int> (tailToken.length())), end))
        return false;

    const auto key { normalizeLabel (*labelToken.get<juce::String> (Id::label)) };

    if (not referenceDestinations.contains (key))
    {
        referenceDestinations.try_emplace (key, *tailToken.get<juce::String> (Id::href));
        referenceTitles.try_emplace (key, *tailToken.get<juce::String> (Id::title));
    }

    return true;
}

Document::Token
MarkdownDocument::BlockParser::getAbsoluteImageTail (const std::string& source, size_t start, size_t end, int cursor)
{
    const auto tailToken { getInlineTail (source, start, end, cursor) };
    const auto isWholeLine { tailToken.contains (Id::href)
                             and cursor + static_cast<int> (tailToken.length())
                                     == static_cast<int> (end - start) };
    const auto url { isWholeLine ? *tailToken.get<juce::String> (Id::href) : juce::String() };

    if (not (isWholeLine and juce::File::isAbsolutePath (url)))
        return {};

    Document::Token token (start + static_cast<size_t> (cursor),
        start + static_cast<size_t> (cursor) + tailToken.length());
    token.add<juce::String> (Id::href, url);
    token.add<juce::String> (Id::title, *tailToken.get<juce::String> (Id::title));
    return token;
}

bool MarkdownDocument::BlockParser::addReferenceDefinition (size_t start, size_t end, size_t& remainderStart)
{
    const auto paragraphOpen {
        isLeafOpen and *getLeaf().get<int> (Id::type) == map::BlockType::paragraph
    };

    if (paragraphOpen)
        return false;

    const auto matched { addReferenceLine (document.getSource(), start, end) };

    if (matched)
    {
        closeLeaf();
        remainderStart = end;
    }

    return matched;
}

int MarkdownDocument::BlockParser::getTrailingBackslashCount (const juce::String& text) noexcept
{
    auto trailingBackslashCount { 0 };
    auto scan { text };

    while (scan.endsWithChar (Chars::backslash))
    {
        ++trailingBackslashCount;
        scan = scan.dropLastCharacters (1);
    }

    return trailingBackslashCount;
}

int MarkdownDocument::BlockParser::getTrailingBackslashCount (const std::string& source,
                                                              size_t start,
                                                              size_t position) noexcept
{
    auto trailingBackslashCount { 0 };

    for (auto cursor { position };
         cursor != start
         and (*Document::Cursor { source, cursor - 1 })
                 == Chars::backslash;
         --cursor)
        ++trailingBackslashCount;

    return trailingBackslashCount;
}

juce::String MarkdownDocument::BlockParser::getWithCollapsedBackslashes (const juce::String& text,
                                                                         int trailingBackslashCount)
{
    const auto prefix { text.dropLastCharacters (trailingBackslashCount) };
    const auto backslash { juce::String::charToString (Chars::backslash) };

    return prefix + juce::String::repeatedString (backslash, trailingBackslashCount / 2);
}

jam::Strings MarkdownDocument::BlockParser::splitTableRow (const juce::String& row) noexcept
{
    auto trimmed { row.trim() };

    if (trimmed.startsWithChar (Chars::pipe))
        trimmed = trimmed.substring (1);

    if (trimmed.endsWithChar (Chars::pipe))
    {
        const auto withoutPipe { trimmed.dropLastCharacters (1) };
        const auto trailingBackslashCount { getTrailingBackslashCount (withoutPipe) };

        if (trailingBackslashCount % 2 == 0)
            trimmed = getWithCollapsedBackslashes (withoutPipe, trailingBackslashCount);
    }

    const auto pipe { juce::String::charToString (Chars::pipe) };
    jam::Strings cells;
    auto remaining { trimmed };
    juce::String cell;

    while (remaining.containsChar (Chars::pipe))
    {
        cell += jam::Format::upTo (remaining, pipe, false);
        remaining = jam::Format::from (remaining, pipe, false);

        const auto trailingBackslashCount { getTrailingBackslashCount (cell) };

        if (trailingBackslashCount % 2 == 1)
            cell = getWithCollapsedBackslashes (cell, trailingBackslashCount) + pipe;
        else
        {
            cells.add (getWithCollapsedBackslashes (cell, trailingBackslashCount));
            cell = juce::String();
        }
    }

    cells.add (cell + remaining);
    return cells;
}

int MarkdownDocument::BlockParser::getAlignmentOrdinal (bool leftColon, bool rightColon)
{
    static constexpr std::array delimiterAlignments {
        std::array { cellAlignmentNone, cellAlignmentRight  },
        std::array { cellAlignmentLeft, cellAlignmentCenter }
    };

    return delimiterAlignments.at (leftColon).at (rightColon);
}

juce::String MarkdownDocument::BlockParser::getAlignmentTokenText (int ordinal)
{
    static const jam::Strings tokens { juce::String(),
                                       Id::right.toString(),
                                       Id::left.toString(),
                                       Id::center.toString() };

    return tokens.at (ordinal);
}

int MarkdownDocument::BlockParser::getCellAlignment (const juce::String& cell) noexcept
{
    auto content { cell.trim() };
    const auto leftColon { content.startsWithChar (Chars::colon) };
    const auto rightColon { content.endsWithChar (Chars::colon) };

    if (leftColon)
        content = content.substring (1);

    if (rightColon and content.isNotEmpty())
        content = content.dropLastCharacters (1);

    if (content.isEmpty()
        or not content.containsOnly (juce::String::charToString (Chars::dash)))
        return invalidCellAlignment;

    return getAlignmentOrdinal (leftColon, rightColon);
}

jam::Array<int>
MarkdownDocument::BlockParser::getRowAlignments (const std::string& source, size_t start, size_t end) noexcept
{
    const juce::String line { juce::String::fromUTF8 (
        source.data() + start, static_cast<int> (end - start)) };

    if (not line.containsChar (Chars::pipe))
        return {};

    const auto cells { splitTableRow (line) };

    jam::Array<int> alignments;

    for (const auto& cell : cells)
    {
        const auto alignment { getCellAlignment (cell) };

        if (alignment == invalidCellAlignment)
            return {};

        alignments.add (alignment);
    }

    return alignments;
}

jam::Array<Document::Span> MarkdownDocument::BlockParser::getTrimmedSpans (const jam::Array<Document::Span>& spans)
{
    auto start { 0 };
    auto end { spans.size() };

    while (start < end)
    {
        const auto length { spans.at (start).unpack<1>() };

        if (length != 0)
            break;

        ++start;
    }

    while (end > start)
    {
        const auto length { spans.at (end - 1).unpack<1>() };

        if (length != 0)
            break;

        --end;
    }

    jam::Array<Document::Span> trimmed;

    for (int index { start }; index < end; ++index)
        trimmed.add (spans.at (index));

    return trimmed;
}

juce::String MarkdownDocument::BlockParser::getCellText (const jam::Array<Document::Span>& spans) const
{
    const auto& source { document.getSource() };
    const auto trimmedSpans { getTrimmedSpans (spans) };

    jam::Strings lines;

    for (int index { 0 }; index < trimmedSpans.size(); ++index)
    {
        const auto offset { trimmedSpans.at (index).unpack<0>() };
        const auto length { trimmedSpans.at (index).unpack<1>() };
        lines.add (
            juce::String::fromUTF8 (source.data() + offset, static_cast<int> (length)));
    }

    return lines.joinIntoString (juce::String::charToString (Chars::newline));
}

void MarkdownDocument::BlockParser::addTableCell (Element& row,
                                                  const juce::String& text,
                                                  const juce::String& alignment,
                                                  const juce::Identifier& colId,
                                                  const jam::Array<Document::Span>& spans)
{
    auto* cell { document.addChild (row, colId) };
    cell->add<int> (Id::type, map::BlockType::tableCell);

    if (alignment.isNotEmpty())
        cell->add<juce::String> (Id::alignment, alignment);

    const auto trimmed { text.trim() };
    cell->add<juce::String> (Id::rawText, trimmed);

    if (spans.size() > 0)
    {
        const auto& source { document.getSource() };
        const auto rawStart { spans.at (0).unpack<0>() };
        const auto rawEnd { rawStart + spans.at (0).unpack<1>() };
        const auto start { trimStart (source, rawStart, rawEnd) };
        const auto end { trimEnd (source, start, rawEnd) };

        if (end > start)
        {
            jam::Array<Document::Token> valueTokens;
            valueTokens.add (Document::Token (start, end));
            cell->add<jam::Array<Document::Token>> (Id::tokens, std::move (valueTokens));
        }
    }

    addCellLines (*cell, spans);
}

Document::Span MarkdownDocument::BlockParser::getRowSpan (const std::string& source,
                                                          const Document::Token& lineToken) noexcept
{
    const auto lineStart { lineToken.offset() };
    const auto lineEnd { lineToken.offset() + lineToken.length() };
    auto start { trimStart (source, lineStart, lineEnd) };
    auto end { trimEnd (source, start, lineEnd) };

    if (start != end
        and (*Document::Cursor { source, start })
                == Chars::pipe)
        ++start;

    if (start != end
        and (*Document::Cursor { source, end - 1 })
                == Chars::pipe)
    {
        const auto trailingBackslashCount { getTrailingBackslashCount (source, start, end - 1) };

        if (trailingBackslashCount % 2 == 0)
            --end;
    }

    return Document::Span::pack (static_cast<uint32_t> (start),
                                 static_cast<uint32_t> (end - start));
}

bool MarkdownDocument::BlockParser::addCells (jam::Array<Document::Span>& cells,
                                              jam::Array<bool>& cellHasEscape,
                                              size_t start,
                                              size_t end) const
{
    const auto& source { document.getSource() };
    bool lineHasEscape { false };
    bool hasEscape { false };

    for (auto cursor { start }; cursor != end; ++cursor)
    {
        const auto ch { (*Document::Cursor { source, cursor }) };

        if (ch == Chars::backslash and cursor + 1 != end
            and ((*Document::Cursor { source, cursor + 1 })
                     == Chars::backslash
                 or (*Document::Cursor { source, cursor + 1 })
                        == Chars::pipe))
            hasEscape = true;

        if (ch != Chars::pipe)
            continue;

        const auto trailingBackslashCount { getTrailingBackslashCount (source, start, cursor) };

        if (trailingBackslashCount % 2 == 0)
        {
            const auto spanStart { trimStart (source, start, cursor) };
            const auto spanEnd { trimEnd (source, spanStart, cursor) };
            cells.add (Document::Span::pack (static_cast<uint32_t> (spanStart),
                                             static_cast<uint32_t> (spanEnd - spanStart)));
            cellHasEscape.add (hasEscape);
            lineHasEscape = lineHasEscape or hasEscape;
            hasEscape = false;
            start = cursor + 1;
        }
    }

    const auto spanStart { trimStart (source, start, end) };
    const auto spanEnd { trimEnd (source, spanStart, end) };
    cells.add (Document::Span::pack (
        static_cast<uint32_t> (spanStart), static_cast<uint32_t> (spanEnd - spanStart)));
    cellHasEscape.add (hasEscape);
    lineHasEscape = lineHasEscape or hasEscape;

    return lineHasEscape;
}

void MarkdownDocument::BlockParser::addColumnSpans (jam::Array<jam::Array<Document::Span>>& cellSpans,
                                                    const jam::Array<Document::Span>& cells,
                                                    const jam::Array<bool>& cellHasEscape,
                                                    const jam::Strings& collapsedCells,
                                                    size_t end) const
{
    for (int column { 0 }; column < cellSpans.size(); ++column)
    {
        Document::Span span {
            column < cells.size()
                ? cells.at (column)
                : Document::Span::pack (static_cast<uint32_t> (end), 0)
        };

        if (column < cells.size() and column < collapsedCells.size()
            and cellHasEscape.at (column))
        {
            const auto collapsedText { collapsedCells.at (column).trim() };
            const auto appendixOffset { document.addSource (std::string_view (
                collapsedText.toRawUTF8(),
                static_cast<size_t> (collapsedText.getNumBytesAsUTF8()))) };
            span = Document::Span::pack (
                static_cast<uint32_t> (appendixOffset),
                static_cast<uint32_t> (collapsedText.getNumBytesAsUTF8()));
        }

        cellSpans.at (column).add (span);
    }
}

jam::Array<jam::Array<Document::Span>>
MarkdownDocument::BlockParser::splitCellSpans (const jam::Array<Document::Token>& lines, int columnCount) const
{
    const auto& source { document.getSource() };
    jam::Array<jam::Array<Document::Span>> cellSpans;
    cellSpans.resize (columnCount);

    for (const auto& lineToken : lines)
    {
        const auto rowSpan { getRowSpan (source, lineToken) };
        const auto start { static_cast<size_t> (rowSpan.unpack<0>()) };
        const auto end { start + static_cast<size_t> (rowSpan.unpack<1>()) };

        jam::Array<Document::Span> cells;
        jam::Array<bool> cellHasEscape;
        const auto lineHasEscape { addCells (cells, cellHasEscape, start, end) };

        const auto collapsedCells { lineHasEscape
                                        ? splitTableRow (juce::String::fromUTF8 (
                                              source.data() + lineToken.offset(),
                                              static_cast<int> (lineToken.length())))
                                        : jam::Strings() };

        addColumnSpans (cellSpans, cells, cellHasEscape, collapsedCells, end);
    }

    return cellSpans;
}

void MarkdownDocument::BlockParser::addCellLines (Element& cell, const jam::Array<Document::Span>& spans)
{
    const auto trimmedSpans { getTrimmedSpans (spans) };

    if (trimmedSpans.size() > 0)
    {
        BlockParser cellBlocks { document, cell };

        for (int index { 0 }; index < trimmedSpans.size(); ++index)
        {
            const auto offset { trimmedSpans.at (index).unpack<0>() };
            const auto length { trimmedSpans.at (index).unpack<1>() };
            cellBlocks.addLine (offset, offset + length);
        }

        cellBlocks.closeLeaf();

        InlineParser inlines { document, cellBlocks };
        inlines.addLeafText (cell);
    }
}

Document::Element& MarkdownDocument::BlockParser::addTableRow (Element& table,
                                                               const jam::Strings& alignments,
                                                               bool isHeader,
                                                               uint32_t rowOffset,
                                                               const jam::Array<jam::Array<Document::Span>>& cellSpans)
{
    const auto rowKey { cellSpans.size() == 0 ? juce::String()
                                               : getCellText (cellSpans.at (0)) };
    auto* row { document.addChild (
        table,
        isHeader ? Id::headerRow
                 : (juce::Identifier::isValidIdentifier (rowKey) ? juce::Identifier (rowKey)
                                                                 : Id::tableRow)) };
    row->add<int> (Id::type, map::BlockType::tableRow);
    row->add<int> (Id::offset, static_cast<int> (rowOffset));

    if (isHeader)
    {
        for (int column { 0 }; column < alignments.size(); ++column)
        {
            const auto text { getCellText (cellSpans.at (column)) };
            addTableCell (*row,
                          text,
                          alignments.at (column),
                          juce::Identifier (Format::toValidID (text)),
                          cellSpans.at (column));
        }
    }
    else
    {
        auto headerCell { getTableHeaderRow (table)->begin() };

        for (int column { 0 }; column < alignments.size(); ++column)
        {
            const auto text { getCellText (cellSpans.at (column)) };
            addTableCell (
                *row, text, alignments.at (column), (*headerCell)->id, cellSpans.at (column));
            ++headerCell;
        }
    }

    return *row;
}

juce::Identifier MarkdownDocument::BlockParser::getTableId (const Element& parent)
{
    juce::Identifier headingId;

    for (auto* child : parent)
        if (*child->get<int> (Id::type) == map::BlockType::heading)
            headingId =
                juce::Identifier (Format::toValidID (*child->get<juce::String> (Id::id)));

    return headingId == juce::Identifier() ? Id::table : headingId;
}

Document::Element* MarkdownDocument::BlockParser::replaceLastChild (Element& parent, const juce::Identifier& id)
{
    auto* replaced { parent.lastChild };
    auto* inserted { document.addChild (parent, id) };

    if (parent.firstChild == replaced)
    {
        parent.firstChild = inserted;
    }
    else
    {
        auto* sibling { parent.firstChild };

        while (sibling->nextSibling != replaced)
            sibling = sibling->nextSibling;

        sibling->nextSibling = inserted;
    }

    return inserted;
}

void MarkdownDocument::BlockParser::promoteTable (Document::Span span, const jam::Strings& alignments)
{
    const auto offset { span.unpack<0>() };
    const auto length { span.unpack<1>() };
    auto& parent { getParent() };
    leafLines.remove (leafLines.size() - 1);

    const auto tableId { getTableId (parent) };
    auto* paragraph { parent.lastChild };

    Element* table { nullptr };

    if (leafLines.isEmpty())
    {
        table = replaceLastChild (parent, tableId);
    }
    else
    {
        leafText.try_emplace (paragraph, joinIntoString (document.getSource(), leafLines));
        table = document.addChild (parent, tableId);
    }

    table->add<int> (Id::type, map::BlockType::table);

    jam::Array<Document::Token> lines;
    lines.add (Document::Token (offset, offset + length));
    const auto cellSpans { splitCellSpans (lines, alignments.size()) };

    addTableRow (*table, alignments, true, offset, cellSpans);
    isGridTableHeaderDone = true;

    leafLines.clear();
}

bool MarkdownDocument::BlockParser::addTable (size_t start, size_t end)
{
    const auto paragraphOpen { isLeafOpen
                               and *getLeaf().get<int> (Id::type)
                                       == map::BlockType::paragraph
                               and not leafLines.isEmpty() };

    if (not paragraphOpen)
        return false;

    const auto rowAlignments { getRowAlignments (document.getSource(), start, end) };

    if (rowAlignments.isEmpty())
        return false;

    const auto& headerToken { leafLines.last() };
    const juce::String headerLine { juce::String::fromUTF8 (
        document.getSource().data() + headerToken.offset(),
        static_cast<int> (headerToken.length())) };
    const auto headerCells { splitTableRow (headerLine) };

    if (rowAlignments.size() != headerCells.size())
        return false;

    jam::Strings alignments;

    for (const auto ordinal : rowAlignments)
        alignments.add (getAlignmentTokenText (ordinal));

    promoteTable (headerToken.span, alignments);
    return true;
}

Document::Element& MarkdownDocument::BlockParser::addTableRowAtLeaf (Element& table,
                                                                     uint32_t rowOffset,
                                                                     const jam::Array<jam::Array<Document::Span>>& cellSpans)
{
    auto* headerRow { table.firstChild };
    jam::Strings alignments;

    for (auto* cell : *headerRow)
    {
        const auto alignment { cell->contains (Id::alignment)
                                   ? *cell->get<juce::String> (Id::alignment)
                                   : juce::String() };
        alignments.add (alignment);
    }

    return addTableRow (table, alignments, false, rowOffset, cellSpans);
}

bool
MarkdownDocument::BlockParser::isGridBorderContent (const std::string& source, size_t contentStart, size_t contentEnd) noexcept
{
    auto segmentLength { 0 };
    auto hasSegment { false };

    for (auto ptr { contentStart + 1 }; ptr != contentEnd - 1; ++ptr)
    {
        const auto ch { (*Document::Cursor { source, ptr }) };

        if (ch == Chars::plus)
        {
            if (segmentLength == 0)
                return false;

            segmentLength = 0;
        }
        else if (ch == Chars::dash or ch == Chars::equals)
        {
            ++segmentLength;
            hasSegment = true;
        }
        else
            return false;
    }

    return hasSegment and segmentLength > 0;
}

bool MarkdownDocument::BlockParser::isGridTableBorder (const std::string& source, size_t start, size_t end) noexcept
{
    const auto indent { countLeadingSpaces (source, start, end, maxUnindentedColumns) };
    const auto contentStart { advance (start, end, indent) };
    const auto contentEnd { trimEnd (source, contentStart, end) };

    if (contentEnd - contentStart < minGridBorderLength
        or (*Document::Cursor { source, contentStart })
               != Chars::plus
        or (*Document::Cursor { source, contentEnd - 1 })
               != Chars::plus)
        return false;

    return isGridBorderContent (source, contentStart, contentEnd);
}

bool
MarkdownDocument::BlockParser::isGridTableHeaderSeparator (const std::string& source, size_t start, size_t end) noexcept
{
    if (not isGridTableBorder (source, start, end))
        return false;

    const auto indent { countLeadingSpaces (source, start, end, maxUnindentedColumns) };
    const auto contentStart { advance (start, end, indent) };
    const auto contentEnd { trimEnd (source, contentStart, end) };

    for (auto ptr { contentStart }; ptr != contentEnd; ++ptr)
        if ((*Document::Cursor { source, ptr })
            == Chars::equals)
            return true;

    return false;
}

size_t MarkdownDocument::BlockParser::getLineEnd (const std::string& source, size_t lineStart) noexcept
{
    auto lineEnd { lineStart };

    while (lineEnd != source.size()
           and (*Document::Cursor { source, lineEnd })
                   != Chars::newline)
        ++lineEnd;

    return lineEnd;
}

bool
MarkdownDocument::BlockParser::isBlankLeadingCell (const std::string& source, size_t contentStart, size_t lineEnd) noexcept
{
    auto cellEnd { contentStart + 1 };

    while (cellEnd != lineEnd
           and (*Document::Cursor { source, cellEnd })
                   != Chars::pipe)
        ++cellEnd;

    return isBlankLine (source, contentStart + 1, cellEnd);
}

size_t
MarkdownDocument::BlockParser::getPipeContentStart (const std::string& source, size_t lineStart, size_t lineEnd) noexcept
{
    const auto indent { countLeadingSpaces (source, lineStart, lineEnd, maxUnindentedColumns) };
    const auto contentStart { advance (lineStart, lineEnd, indent) };

    if (contentStart != lineEnd
        and (*Document::Cursor { source, contentStart })
                == Chars::pipe)
        return contentStart;

    return lineEnd;
}

bool MarkdownDocument::BlockParser::isGridTableFormat (const std::string& source, size_t end) noexcept
{
    auto rowSeparatorCount { 0 };
    auto isHeaderSeparatorFound { false };

    for (auto lineStart { end + 1 }; lineStart <= source.size();)
    {
        const auto lineEnd { getLineEnd (source, lineStart) };
        const auto isBorderLine { isGridTableBorder (source, lineStart, lineEnd) };
        const auto pipeContentStart { getPipeContentStart (source, lineStart, lineEnd) };

        if (isHeaderSeparatorFound and pipeContentStart != lineEnd
            and isBlankLeadingCell (source, pipeContentStart, lineEnd))
            return true;

        if (isBlankLine (source, lineStart, lineEnd)
            or (not isBorderLine and pipeContentStart == lineEnd))
            return rowSeparatorCount > closingBorderCount;

        if (isHeaderSeparatorFound and isBorderLine)
            ++rowSeparatorCount;
        isHeaderSeparatorFound = isHeaderSeparatorFound
                                 or isGridTableHeaderSeparator (source, lineStart, lineEnd);

        lineStart = lineEnd + 1;
    }

    return rowSeparatorCount > closingBorderCount;
}

bool MarkdownDocument::BlockParser::addGridTable (size_t start, size_t end, size_t& remainderStart)
{
    if (not isGridTableBorder (document.getSource(), start, end))
        return false;

    closeLeaf();

    auto& parent { getParent() };
    const auto tableId { getTableId (parent) };
    auto* table { document.addChild (parent, tableId) };
    table->add<int> (Id::type, map::BlockType::table);

    if (isGridTableFormat (document.getSource(), end))
        table->add<juce::String> (
            Id::format, map::OpenBlock::getInstance()->get (map::OpenBlock::gridTable));

    isLeafOpen = true;
    isGridTableHeaderDone = false;
    remainderStart = end;
    return true;
}

void MarkdownDocument::BlockParser::addAccumulatedHeaderRow (Element& table)
{
    if (not gridRow.lines.isEmpty())
    {
        jam::Strings alignments;

        for (int column { 0 }; column < gridRow.columnCount; ++column)
            alignments.add (juce::String());

        const auto cellSpans { splitCellSpans (gridRow.lines, gridRow.columnCount) };
        addTableRow (table, alignments, true, gridRow.offset, cellSpans);

        gridRow.lines.clear();
        gridRow.isValid = true;
    }
}

void MarkdownDocument::BlockParser::addAccumulatedRow (Element& table)
{
    if (not gridRow.lines.isEmpty())
    {
        const auto cellSpans { splitCellSpans (gridRow.lines, gridRow.columnCount) };
        auto& row { addTableRowAtLeaf (table, gridRow.offset, cellSpans) };

        if (not gridRow.isValid)
            row.add<int> (Id::columns, gridRow.columnMismatch);

        gridRow.lines.clear();
        gridRow.isValid = true;
    }
}

bool MarkdownDocument::BlockParser::addGridTableBlankLine (size_t start, size_t end)
{
    if (not isBlankLine (document.getSource(), start, end))
        return false;

    closeLeaf();
    return true;
}

bool MarkdownDocument::BlockParser::addGridTableBorderLine (size_t start, size_t end)
{
    const auto& source { document.getSource() };

    if (not isGridTableBorder (source, start, end))
        return false;

    if (isGridTableHeaderDone)
    {
        addAccumulatedRow (getLeaf());

        auto* border { document.addChild (getLeaf(), Id::border) };
        border->add<int> (Id::type, map::BlockType::tableBorder);
    }
    else if (isGridTableHeaderSeparator (source, start, end))
    {
        addAccumulatedHeaderRow (getLeaf());
        isGridTableHeaderDone = true;
    }

    return true;
}

bool MarkdownDocument::BlockParser::addGridTableAlignmentRow (size_t start, size_t end)
{
    if (getRowAlignments (document.getSource(), start, end).isEmpty())
        return false;

    if (isGridTableHeaderDone)
        addAccumulatedRow (getLeaf());
    else
    {
        addAccumulatedHeaderRow (getLeaf());
        isGridTableHeaderDone = true;
    }

    return true;
}

void MarkdownDocument::BlockParser::setGridRowColumnMismatch (int columnCount) noexcept
{
    gridRow.isValid = false;
    gridRow.columnMismatch = columnCount;
}

void MarkdownDocument::BlockParser::addGridRowLine (size_t start, size_t end, const jam::Strings& cells)
{
    if (gridRow.lines.isEmpty())
    {
        gridRow.columnCount = cells.size();
        gridRow.offset = static_cast<uint32_t> (start);

        if (isGridTableHeaderDone
            and cells.size() != getChildCount (*getLeaf().firstChild))
            setGridRowColumnMismatch (cells.size());
    }
    else if (gridRow.isValid and cells.size() != gridRow.columnCount)
    {
        setGridRowColumnMismatch (cells.size());
    }

    gridRow.lines.add (Document::Token (start, end));
}

bool MarkdownDocument::BlockParser::addGridTableContentLine (size_t start, size_t end)
{
    const auto& source { document.getSource() };
    const juce::String rowLine { juce::String::fromUTF8 (
        source.data() + start, static_cast<int> (end - start)) };

    if (not rowLine.trim().startsWithChar (Chars::pipe))
        return false;

    const auto cells { splitTableRow (rowLine) };
    const auto isGridTable { getLeaf().contains (Id::format)
                             and *getLeaf().get<juce::String> (Id::format)
                                     == map::OpenBlock::getInstance()->get (map::OpenBlock::gridTable) };

    if (not isGridTable)
    {
        if (isGridTableHeaderDone)
            addAccumulatedRow (getLeaf());
        else
            addAccumulatedHeaderRow (getLeaf());
    }

    addGridRowLine (start, end, cells);
    return true;
}

bool MarkdownDocument::BlockParser::addGridTableLine (size_t start, size_t end)
{
    if (not isLeafOpen or *getLeaf().get<int> (Id::type) != map::BlockType::table)
        return false;

    static constexpr std::array gridTableLines { &BlockParser::addGridTableBlankLine,
                                                &BlockParser::addGridTableBorderLine,
                                                &BlockParser::addGridTableAlignmentRow,
                                                &BlockParser::addGridTableContentLine };

    for (const auto& addLineFunction : gridTableLines)
        if ((this->*addLineFunction) (start, end))
            return true;

    closeLeaf();
    return false;
}

void MarkdownDocument::BlockParser::addFallbackLeaf (size_t start, size_t end)
{
    const auto& source { document.getSource() };
    const auto hasCodeIndent { countLeadingSpaces (source, start, end, indentedCodeColumns)
                               >= indentedCodeColumns };
    const auto indentedCodeOpen { isLeafOpen
                                  and *getLeaf().get<int> (Id::type)
                                          == map::BlockType::codeBlock
                                  and not getLeaf().contains (Id::marker) };

    if (indentedCodeOpen and hasCodeIndent)
    {
        leafLines.add (Document::Token (advance (start, end, indentedCodeColumns), end));
        return;
    }

    if (indentedCodeOpen)
        closeLeaf();

    const auto paragraphOpen {
        isLeafOpen and *getLeaf().get<int> (Id::type) == map::BlockType::paragraph
    };

    if (hasCodeIndent and not paragraphOpen)
        addIndentedCodeBlock (getParent(), advance (start, end, indentedCodeColumns), end);
    else
        addParagraphLine (start, end);
}

bool MarkdownDocument::BlockParser::addLeafForTableInterrupt (size_t start, size_t end, size_t& remainderStart)
{
    static constexpr std::array interruptingLeafBlocks { &BlockParser::addThematicBreak,
                                                         &BlockParser::addAtxHeading,
                                                         &BlockParser::addFencedCode,
                                                         static_cast<AddFunction> (
                                                             &BlockParser::addHtmlBlock) };

    for (const auto& addFunction : interruptingLeafBlocks)
        if ((this->*addFunction) (start, end, remainderStart))
            return true;

    return false;
}

bool
MarkdownDocument::BlockParser::addContainer (size_t start, size_t end, size_t& remainderStart, bool allContainersMatched)
{
    if (addBlockquote (start, end, remainderStart))
        return true;

    return addListItem (start, end, remainderStart, allContainersMatched);
}

bool MarkdownDocument::BlockParser::interruptTable (size_t start, size_t end, size_t& remainderStart)
{
    while (true)
    {
        if (addLeafForTableInterrupt (start, end, remainderStart))
            return true;

        if (not addContainer (start, end, remainderStart, true))
            return false;

        start = remainderStart;
    }
}

bool MarkdownDocument::BlockParser::addTableRowLine (size_t start, size_t end)
{
    const auto& source { document.getSource() };
    const auto tableOpen { isLeafOpen
                           and *getLeaf().get<int> (Id::type) == map::BlockType::table
                           and not isBlankLine (source, start, end) };

    if (not tableOpen)
        return false;

    const auto containerDepthBefore { containerDepth };
    size_t remainderStart { start };

    if (interruptTable (start, end, remainderStart))
        return true;

    if (containerDepth > containerDepthBefore)
    {
        addFallbackLeaf (remainderStart, end);
        return true;
    }

    return addGridTableLine (start, end);
}

int
MarkdownDocument::BlockParser::getListIndent (const std::string& source, size_t start, size_t end, int indent) noexcept
{
    if (isBlankLine (source, start, end))
        return 0;

    const auto columns { countLeadingSpaces (source, start, end, indent) };

    return columns < indent ? indentMismatch : columns;
}

int MarkdownDocument::BlockParser::getContinuationDepth (size_t start, size_t end)
{
    const auto& source { document.getSource() };
    auto cursor { start };
    int depth { 0 };

    for (; depth < containerDepth; ++depth)
    {
        auto& container { getBlockAt (depth) };
        const auto isBlockquote { *container.get<int> (Id::type)
                                  == map::BlockType::blockquote };
        const auto length {
            isBlockquote
                ? getBlockquoteMarker (source, cursor, end)
                : getListIndent (source,
                                 cursor,
                                 end,
                                 container.get<juce::String> (Id::indent)->getIntValue())
        };

        if (length == indentMismatch or (isBlockquote and length == 0))
            break;

        cursor = advance (cursor, end, length);
    }

    return depth;
}

size_t MarkdownDocument::BlockParser::advanceThroughContinuation (size_t start, size_t end, int matchedDepth)
{
    const auto& source { document.getSource() };
    auto cursor { start };

    for (int depth { 0 }; depth < matchedDepth; ++depth)
    {
        auto& container { getBlockAt (depth) };
        const auto isBlockquote { *container.get<int> (Id::type)
                                  == map::BlockType::blockquote };
        const auto length {
            isBlockquote
                ? getBlockquoteMarker (source, cursor, end)
                : getListIndent (source,
                                 cursor,
                                 end,
                                 container.get<juce::String> (Id::indent)->getIntValue())
        };

        cursor = advance (cursor, end, length);
    }

    return cursor;
}

void MarkdownDocument::BlockParser::setListsLoose()
{
    for (int depth { 0 }; depth < containerDepth; ++depth)
    {
        auto& container { getBlockAt (depth) };

        if (*container.get<int> (Id::type) == map::BlockType::listItem)
        {
            auto& owningList { getListAt (depth) };

            if (not owningList.contains (Id::tight))
                owningList.add<juce::String> (Id::tight, Format::fromBoolean (false));
        }
    }
}

void MarkdownDocument::BlockParser::applyContinuationResult (int matchedDepth, size_t start, size_t end)
{
    if (matchedDepth < containerDepth)
    {
        const auto& source { document.getSource() };
        const auto onlyInnermostFailed { matchedDepth == containerDepth - 1 };
        const auto paragraphOpen {
            isLeafOpen and *getLeaf().get<int> (Id::type) == map::BlockType::paragraph
        };
        const auto unindentedStart { trimStart (source, start, end) };
        const auto listContentStart { advance (
            start, end, countLeadingSpaces (source, start, end, maxUnindentedColumns)) };
        const auto beginsBlock {
            (hasUnindentedStart (source, start, end)
             and (isThematicBreakContent (source, unindentedStart, end)
                  or getAtxHeadingLevel (source, unindentedStart, end) > 0))
            or getBlockquoteMarker (source, start, end) > 0
            or getFenceOpenWidth (source, start, end) > 0
            or getListMarkerWidth (source, listContentStart, end) > 0
        };
        const auto lazyContinuation { onlyInnermostFailed and paragraphOpen
                                      and not isBlankLine (source, start, end)
                                      and not beginsBlock };

        if (not lazyContinuation)
        {
            closeLeaf();
            containerDepth = matchedDepth;
        }
    }
}

bool MarkdownDocument::BlockParser::addLeaf (size_t start, size_t end, size_t& remainderStart)
{
    static const auto leafBlocks {
        []
        {
            jam::Function::Map<int, bool> leafBlocks;

            leafBlocks.add<BlockParser&, size_t&, size_t&, size_t&> (
                map::OpenBlock::thematicBreak, &BlockParser::addThematicBreak);
            leafBlocks.add<BlockParser&, size_t&, size_t&, size_t&> (
                map::OpenBlock::atxHeading, &BlockParser::addAtxHeading);
            leafBlocks.add<BlockParser&, size_t&, size_t&, size_t&> (
                map::OpenBlock::fencedCode, &BlockParser::addFencedCode);
            leafBlocks.add<BlockParser&, size_t&, size_t&, size_t&> (
                map::OpenBlock::htmlBlock,
                static_cast<AddFunction> (&BlockParser::addHtmlBlock));
            leafBlocks.add<BlockParser&, size_t&, size_t&, size_t&> (
                map::OpenBlock::referenceDefinition, &BlockParser::addReferenceDefinition);
            leafBlocks.add<BlockParser&, size_t&, size_t&, size_t&> (
                map::OpenBlock::gridTable, &BlockParser::addGridTable);

            return leafBlocks;
        }()
    };

    return leafBlocks.get (map::OpenBlock::thematicBreak, *this, start, end, remainderStart)
           or leafBlocks.get (map::OpenBlock::atxHeading, *this, start, end, remainderStart)
           or leafBlocks.get (map::OpenBlock::fencedCode, *this, start, end, remainderStart)
           or leafBlocks.get (map::OpenBlock::htmlBlock, *this, start, end, remainderStart)
           or leafBlocks.get (
               map::OpenBlock::referenceDefinition, *this, start, end, remainderStart)
           or leafBlocks.get (map::OpenBlock::gridTable, *this, start, end, remainderStart);
}

bool MarkdownDocument::BlockParser::addBlocks (size_t start, size_t end, size_t& remainderStart, bool allContainersMatched)
{
    while (true)
    {
        if (addLeaf (start, end, remainderStart))
            return true;

        if (not addContainer (start, end, remainderStart, allContainersMatched))
            return false;

        start = remainderStart;
    }
}

void MarkdownDocument::BlockParser::closeOnBlankLine (size_t lineEnd)
{
    const auto indentedCodeOpen { isLeafOpen
                                  and *getLeaf().get<int> (Id::type)
                                          == map::BlockType::codeBlock
                                  and not getLeaf().contains (Id::marker) };

    if (indentedCodeOpen)
    {
        leafLines.add (Document::Token (lineEnd, lineEnd));
        return;
    }

    closeLeaf();
    setListsLoose();
}

bool MarkdownDocument::BlockParser::promoteSetextHeading (size_t start, size_t end)
{
    const auto paragraphOpen {
        isLeafOpen and *getLeaf().get<int> (Id::type) == map::BlockType::paragraph
    };
    const auto setextLevel { paragraphOpen
                                 ? getSetextLevel (document.getSource(), start, end)
                                 : 0 };

    if (setextLevel == 0)
        return false;

    const auto text { joinIntoString (document.getSource(), leafLines) };
    auto& leaf { getLeaf() };
    *leaf.get<int> (Id::type) = map::BlockType::heading;
    leaf.id = map::HeadingTag::getInstance()->get (setextLevel);
    leaf.add<juce::String> (Id::level, juce::String (setextLevel));
    leaf.add<juce::String> (Id::id, text);
    leafText.try_emplace (&leaf, text);
    isLeafOpen = false;
    leafLines.clear();
    return true;
}

void MarkdownDocument::BlockParser::addLine (size_t lineStart, size_t lineEnd)
{
    const auto matchedDepth { getContinuationDepth (lineStart, lineEnd) };
    const auto allContainersMatched { matchedDepth == containerDepth };
    auto start { advanceThroughContinuation (lineStart, lineEnd, matchedDepth) };

    applyContinuationResult (matchedDepth, start, lineEnd);

    size_t remainderStart { start };

    if (addFencedCodeLine (start, lineEnd) or addHtmlBlockLine (start, lineEnd)
        or addTableRowLine (start, lineEnd) or addTable (start, lineEnd)
        or promoteSetextHeading (start, lineEnd))
        return;

    if (isBlankLine (document.getSource(), start, lineEnd))
    {
        closeOnBlankLine (lineEnd);
        return;
    }

    if (not addBlocks (start, lineEnd, remainderStart, allContainersMatched))
        addFallbackLeaf (remainderStart, lineEnd);
}

/*____________________________________________________________________________*/
} /** namespace jam */
