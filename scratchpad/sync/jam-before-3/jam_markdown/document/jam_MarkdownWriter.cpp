namespace jam
{
/*____________________________________________________________________________*/

juce::String MarkdownWriter::getText (const Document& document) const
{
    jam::Array<const Element*> children;

    for (auto* child : *document.getRoot())
        children.add (child);

    return getText (children);
}

juce::String MarkdownWriter::getText (const Document& document, const juce::String& origin) const
{
    jam::Array<const Element*> children;

    for (auto* child : *document.getRoot())
        if (child->contains (Id::path) and child->get<juce::String> (Id::path)->compare (origin) == 0)
            children.add (child);

    return getText (children);
}

juce::String MarkdownWriter::getText (const jam::Array<const Element*>& children) const
{
    jam::Strings blocks;

    for (auto* child : children)
        blocks.add (getText (*child));

    return blocks.joinIntoString (juce::String::charToString (Chars::newline)
                                      + juce::String::charToString (Chars::newline),
                                  0,
                                  -1)
           + juce::String::charToString (Chars::newline);
}

juce::String MarkdownWriter::getContentText (const Element& element)
{
    jam::Strings inlines;

    for (auto* child : element)
        inlines.add (getInlines().get (child->id, *child));

    return inlines.joinIntoString (juce::String(), 0, -1);
}

juce::String MarkdownWriter::getCellText (const Element& cell) noexcept
{
    const auto raw { *cell.get<juce::String> (Id::rawText) };
    const auto backslash { juce::String::charToString (Chars::backslash) };
    juce::String cellText;
    juce::String pendingBackslashes;

    for (auto cursor { raw.getCharPointer() }; not cursor.isEmpty(); ++cursor)
    {
        const auto character { *cursor };

        if (character == Chars::backslash)
            pendingBackslashes += backslash;
        else if (character == Chars::pipe)
        {
            cellText += pendingBackslashes + pendingBackslashes
                        + Chars::escapedPipe;
            pendingBackslashes = juce::String();
        }
        else
        {
            cellText += pendingBackslashes + juce::String::charToString (character);
            pendingBackslashes = juce::String();
        }
    }

    return cellText + pendingBackslashes;
}

juce::String MarkdownWriter::getFillText (int fillWidth)
{
    return juce::String::repeatedString (juce::String::charToString (Chars::space), fillWidth);
}

std::string_view MarkdownWriter::getView (const juce::String& text)
{
    return { text.toRawUTF8(), text.getNumBytesAsUTF8() };
}

int MarkdownWriter::getFillWidth (const juce::String& text, int width)
{
    return std::max (0, width - jam::AttributedChar::width (getView (text)));
}

juce::String MarkdownWriter::getWrappedText (const jam::ReflowDocument& reflow)
{
    const std::string_view source { reflow.getSource() };
    std::string text;
    int lineIndex { 0 };

    for (auto* line : reflow)
    {
        if (lineIndex > 0)
            text.push_back (static_cast<char> (Chars::newline));

        const auto& rows { *line->get<Document::Tokens> (Id::row) };

        for (int rowIndex { 0 }; rowIndex < rows.size(); ++rowIndex)
        {
            if (rowIndex > 0)
                text.push_back (static_cast<char> (Chars::newline));

            const auto& row { rows.at (rowIndex) };
            text.append (source.substr (row.offset(), row.length()));
        }

        ++lineIndex;
    }

    return juce::String::fromUTF8 (text.data(), static_cast<int> (text.size()));
}

juce::String MarkdownWriter::getWrappedText (const juce::String& text, int limit)
{
    return getWrappedText (jam::ReflowDocument::parse (getView (text), limit, map::LineBreakLanguage::und));
}

int MarkdownWriter::getOpenBlockRow (const jam::ReflowDocument& reflow, const Element& line)
{
    const auto& rows { *line.get<Document::Tokens> (Id::row) };

    if (rows.isEmpty()) return -1;

    const std::string_view source { reflow.getSource() };
    const auto& firstRow { rows.at (0) };
    std::string candidate { source.substr (firstRow.offset(), firstRow.length()) };

    for (int index { 1 }; index < rows.size(); ++index)
    {
        candidate.push_back (static_cast<char> (Chars::newline));

        const auto& row { rows.at (index) };
        candidate.append (source.substr (row.offset(), row.length()));

        if (not MarkdownDocument::isParagraph (candidate))
            return index;
    }

    return -1;
}

juce::String MarkdownWriter::getRightPaddedText (const juce::String& text, int width)
{
    return getFillText (getFillWidth (text, width)) + text;
}

juce::String MarkdownWriter::getCenterPaddedText (const juce::String& text, int width)
{
    const auto fillWidth { getFillWidth (text, width) };
    const auto index { fillWidth / 2 };

    return getFillText (index) + text + getFillText (fillWidth - index);
}

const jam::Function::Map<juce::String, juce::String>& MarkdownWriter::getAlignmentPadding()
{
    static const auto alignmentPadding {
        []
        {
            jam::Function::Map<juce::String, juce::String> alignmentPadding;

            alignmentPadding.add<const juce::String&, int> (Id::right, getRightPaddedText);
            alignmentPadding.add<const juce::String&, int> (Id::center, getCenterPaddedText);

            return alignmentPadding;
        }()
    };

    return alignmentPadding;
}

juce::String MarkdownWriter::getPaddedText (const juce::String& text, int width,
                                            const juce::String& alignment)
{
    const auto& alignmentPadding { getAlignmentPadding() };

    const auto aligned { alignmentPadding.contains (alignment)
                              ? alignmentPadding.get (alignment, text, width)
                              : text + getFillText (getFillWidth (text, width)) };

    return getFillText (cellFlankSpaceWidth) + aligned + getFillText (cellFlankSpaceWidth);
}

juce::String MarkdownWriter::getRowText (const jam::Strings& cells,
                                         const jam::Array<int>& columnWidths,
                                         const jam::Array<juce::String>& alignment,
                                         int lineIndex)
{
    jam::Strings columns;

    for (int column { 0 }; column < columnWidths.size(); ++column)
    {
        const auto cellLines { jam::Strings::fromLines (
            column < cells.size() ? cells.at (column) : juce::String()) };
        const auto text { lineIndex < cellLines.size() ? cellLines.at (lineIndex)
                                                        : juce::String() };

        columns.add (getPaddedText (text, columnWidths.at (column), alignment.at (column)));
    }

    return juce::String::charToString (Chars::pipe)
           + columns.joinIntoString (juce::String::charToString (Chars::pipe), 0, -1)
           + juce::String::charToString (Chars::pipe);
}

juce::String MarkdownWriter::getGridRowText (const jam::Strings& cells,
                                             const jam::Array<int>& columnWidths,
                                             const jam::Array<juce::String>& alignment)
{
    int lineCount { 0 };

    for (int column { 0 }; column < cells.size(); ++column)
        lineCount = std::max (lineCount, jam::Strings::fromLines (cells.at (column)).size());

    jam::Strings result;

    for (int lineIndex { 0 }; lineIndex < lineCount; ++lineIndex)
        result.add (getRowText (cells, columnWidths, alignment, lineIndex));

    return result.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
}

juce::String MarkdownWriter::getBorderText (const jam::Array<int>& columnWidths, juce::juce_wchar marker)
{
    jam::Strings columns;

    for (const auto width : columnWidths)
        columns.add (juce::String::repeatedString (juce::String::charToString (marker),
                                                    width + cellFlankSpaceWidth * 2));

    return juce::String::charToString (Chars::plus)
           + columns.joinIntoString (juce::String::charToString (Chars::plus), 0, -1)
           + juce::String::charToString (Chars::plus);
}

juce::String MarkdownWriter::getBlockquotePrefix()
{
    return juce::String::charToString (Chars::greaterThan) + juce::String::charToString (Chars::space);
}

const jam::Function::Map<int, juce::String>& MarkdownWriter::getPrefixes()
{
    static const auto prefixes {
        []
        {
            jam::Function::Map<int, juce::String> prefixes;

            prefixes.add<const Element&> (map::BlockType::blockquote,
                [] (const Element&) { return getBlockquotePrefix(); });
            prefixes.add<const Element&> (map::BlockType::listItem, getListItemPrefix);

            return prefixes;
        }()
    };

    return prefixes;
}

int MarkdownWriter::getPrefixWidth (const Element& block)
{
    const auto& prefixes { getPrefixes() };
    int width { 0 };

    for (auto* ancestor { block.parent }; ancestor != nullptr; ancestor = ancestor->parent)
        if (ancestor->contains (Id::type) and prefixes.contains (*ancestor->get<int> (Id::type)))
        {
            const auto prefixText { prefixes.get (*ancestor->get<int> (Id::type), *ancestor) };
            width += jam::AttributedChar::width (getView (prefixText));
        }

    return width;
}

static constexpr uint32_t halfwidthHangulFirst { 0xffa0 };
static constexpr uint32_t halfwidthHangulLast { 0xffdc };

static bool isHangul (uint32_t codepoint) noexcept
{
    static constexpr std::array<int, 5> hangulClasses {
        map::LineBreak::H2, map::LineBreak::H3, map::LineBreak::JL, map::LineBreak::JV, map::LineBreak::JT
    };

    const auto lineBreakClass { jam::AttributedChar::lineBreakClass (codepoint) };

    return std::find (hangulClasses.begin(), hangulClasses.end(), lineBreakClass) != hangulClasses.end()
       or (codepoint >= halfwidthHangulFirst and codepoint <= halfwidthHangulLast);
}

static bool isSegmentBreakRemoved (std::string_view run, size_t newlineIndex) noexcept
{
    if (newlineIndex == 0 or newlineIndex + 1 == run.size())
        return false;

    juce::CharPointer_UTF8 before { run.data() + newlineIndex };
    --before;
    const auto beforeCodepoint { static_cast<uint32_t> (*before) };

    juce::CharPointer_UTF8 after { run.data() + newlineIndex + 1 };
    const auto afterCodepoint { static_cast<uint32_t> (*after) };

    return jam::AttributedChar::isEastAsian (beforeCodepoint)
       and jam::AttributedChar::isEastAsian (afterCodepoint)
       and not isHangul (beforeCodepoint)
       and not isHangul (afterCodepoint);
}

std::string MarkdownWriter::getSourceText (const Element& block)
{
    std::string raw;

    for (auto* child : block)
    {
        if (child->id == Id::text)
            raw.append (getView (*child->get<juce::String> (Id::text)));
        else
            raw.append (getView (getInlines().get (child->id, *child)));
    }

    std::string source;
    source.reserve (raw.size());

    for (size_t index { 0 }; index < raw.size(); ++index)
    {
        if (raw.at (index) != static_cast<char> (Chars::newline))
        {
            source.push_back (raw.at (index));
        }
        else if (index > 0 and raw.at (index - 1) == static_cast<char> (Chars::backslash))
        {
            source.push_back (static_cast<char> (Chars::newline));
        }
        else if (not isSegmentBreakRemoved (raw, index))
        {
            source.push_back (static_cast<char> (Chars::space));
        }
    }

    return source;
}

juce::String MarkdownWriter::getParagraphText (const Element& block) const
{
    const int limit { lineWrap - getPrefixWidth (block) };
    const auto source { getSourceText (block) };
    auto reflow { jam::ReflowDocument::parse (source, limit, map::LineBreakLanguage::und) };

    for (auto* line : reflow)
    {
        for (int row { getOpenBlockRow (reflow, *line) }; row >= 0; row = getOpenBlockRow (reflow, *line))
        {
            reflow.setGlue (*line, line->get<Document::Tokens> (Id::row)->at (row).offset());
            reflow.setRows (*line, limit);
        }
    }

    return getWrappedText (reflow);
}

juce::String MarkdownWriter::getHeadingText (const Element& block)
{
    const auto level { block.get<juce::String> (Id::level)->getIntValue() };

    return juce::String::repeatedString (juce::String::charToString (Chars::hash), level)
           + juce::String::charToString (Chars::space) + getContentText (block);
}

juce::String MarkdownWriter::getBlockquoteText (const Element& block) const
{
    const auto prefix { getBlockquotePrefix() };
    jam::Strings lines;

    for (auto* child : block)
    {
        if (lines.size() > 0)
            lines.add (juce::String());

        lines.addLines (getText (*child));
    }

    jam::Strings result;

    for (int index { 0 }; index < lines.size(); ++index)
    {
        const auto text { lines.at (index) };
        result.add (text.isEmpty() ? juce::String::charToString (Chars::greaterThan)
                                   : prefix + text);
    }

    return result.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
}

juce::String MarkdownWriter::getFencedBlockText (const Element& block)
{
    const auto fence { *block.get<juce::String> (Id::marker) };
    const auto info { block.get<juce::String> (Id::info, juce::String()) };
    const auto text { block.firstChild != nullptr and block.firstChild->contains (Id::text)
                           ? *block.firstChild->get<juce::String> (Id::text)
                           : juce::String() };

    jam::Strings lines { fence + info };
    lines.addLines (text);
    lines.add (fence);

    return lines.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
}

juce::String MarkdownWriter::getThematicBreakText (const Element&)
{
    return juce::String::repeatedString (juce::String::charToString (Chars::dash),
                                         thematicBreakMarkerWidth);
}

juce::String MarkdownWriter::getHtmlBlockText (const Element& block)
{
    return block.firstChild != nullptr and block.firstChild->contains (Id::text)
               ? *block.firstChild->get<juce::String> (Id::text)
               : juce::String();
}

juce::String MarkdownWriter::getLinkText (const Element& element, bool isImage,
                                          const juce::Identifier& urlAttribute)
{
    juce::String inner { *element.get<juce::String> (urlAttribute) };

    if (element.contains (Id::title))
        inner << Chars::space
              << jam::Format::withEnclosure (*element.get<juce::String> (Id::title),
                                             Chars::doubleQuote);

    return (isImage ? juce::String::charToString (Chars::exclamation) : juce::String())
           + jam::Format::withEnclosure (getContentText (element), Chars::openBracket)
           + jam::Format::withEnclosure (inner, Chars::openParen);
}

juce::String MarkdownWriter::getListText (const Element& block) const
{
    jam::Strings lines;

    for (auto* child : block)
        lines.addLines (getText (*child));

    return lines.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
}

jam::Strings MarkdownWriter::getListItemLines (const Element& block) const
{
    jam::Strings lines;

    for (auto* child : block)
    {
        if (lines.size() > 0)
            lines.add (juce::String());

        lines.addLines (getText (*child));
    }

    return lines;
}

int MarkdownWriter::getListItemIndex (const Element& block)
{
    int index { 0 };

    for (auto* child : *block.parent)
    {
        if (child == &block)
            return index;

        ++index;
    }

    return index;
}

juce::String MarkdownWriter::getListItemMarker (const Element& block)
{
    const auto index { getListItemIndex (block) };

    return block.parent->isTag (Id::ol)
               ? juce::String (index + 1) + juce::String::charToString (Chars::dot)
               : juce::String::charToString (Chars::dash);
}

juce::String MarkdownWriter::getListItemPrefix (const Element& block)
{
    return getListItemMarker (block) + juce::String::charToString (Chars::space);
}

juce::String MarkdownWriter::getListItemText (const Element& block) const
{
    const auto lines { getListItemLines (block) };
    const auto prefix { getListItemPrefix (block) };
    jam::Strings result;

    for (int index { 0 }; index < lines.size(); ++index)
    {
        const auto text { lines.at (index) };
        result.add (index == 0
                         ? prefix + text
                         : (text.isEmpty() ? juce::String()
                                           : juce::String::charToString (Chars::space)
                                                 + juce::String::charToString (Chars::space)
                                                 + text));
    }

    return result.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
}

bool MarkdownWriter::isGridTable (const Element& block) noexcept
{
    for (auto* child : block)
        if (MarkdownDocument::isTableBorder (*child))
            return true;

    return false;
}

jam::Array<juce::String> MarkdownWriter::getTableAlignment (const Element& header)
{
    jam::Array<juce::String> alignment;

    for (auto* cell : header)
        alignment.add (cell->get<juce::String> (Id::alignment, juce::String()));

    return alignment;
}

jam::Array<jam::ReflowDocument> MarkdownWriter::getCellReflows (const Element& block, const Element& header, int limit)
{
    jam::Array<jam::ReflowDocument> cells;

    for (auto* row : block)
    {
        if (not MarkdownDocument::isTableBorder (*row) and row != &header)
        {
            for (auto* cell : *row)
                cells.add (jam::ReflowDocument::parse (getView (getCellText (*cell)), limit, map::LineBreakLanguage::und));
        }
    }

    return cells;
}

int MarkdownWriter::getWrappedWidth (const jam::ReflowDocument& reflow)
{
    const std::string_view source { reflow.getSource() };
    int width { 0 };

    for (auto* line : reflow)
    {
        for (const auto& row : *line->get<Document::Tokens> (Id::row))
            width = std::max (width, jam::AttributedChar::width (source.substr (row.offset(), row.length())));
    }

    return width;
}

int MarkdownWriter::getLinesWidth (const juce::String& text)
{
    int width { 0 };

    for (const auto& line : jam::Strings::fromLines (text))
        width = std::max (width, jam::AttributedChar::width (getView (line)));

    return width;
}

jam::Array<int> MarkdownWriter::getClampedColumnWidths (jam::Array<int> columnWidths, const Element& header,
                                                        const jam::Array<jam::ReflowDocument>& cells)
{
    const auto columnCount { columnWidths.size() };
    int headerColumn { 0 };

    for (auto* cell : header)
    {
        columnWidths.set (headerColumn, std::max (columnWidths.at (headerColumn), getLinesWidth (getCellText (*cell))));
        ++headerColumn;
    }

    for (int cellIndex { 0 }; cellIndex < cells.size(); ++cellIndex)
    {
        const auto column { cellIndex % columnCount };
        columnWidths.set (column, std::max (columnWidths.at (column), getWrappedWidth (cells.at (cellIndex))));
    }

    return columnWidths;
}

jam::Array<int> MarkdownWriter::getMinimumColumnWidths (const jam::Array<juce::String>& alignment)
{
    jam::Array<int> columnWidths;

    for (int column { 0 }; column < alignment.size(); ++column)
        columnWidths.add (alignment.at (column).compare (Id::center.toString()) == 0
                               ? centerAlignedMinimumColumnWidth
                               : (alignment.at (column).isNotEmpty()
                                      ? alignedMinimumColumnWidth
                                      : unalignedMinimumColumnWidth));

    return columnWidths;
}

jam::Array<int> MarkdownWriter::getTableColumnWidths (const Element& block,
                                                      const jam::Array<juce::String>& alignment) const
{
    auto columnWidths { getMinimumColumnWidths (alignment) };

    for (auto* child : block)
        if (not MarkdownDocument::isTableBorder (*child))
        {
            int column { 0 };

            for (auto* cell : *child)
            {
                columnWidths.set (column, std::max (columnWidths.at (column), getLinesWidth (getCellText (*cell))));
                ++column;
            }
        }

    return columnWidths;
}

jam::Strings MarkdownWriter::getPipeTableAlignmentMarkers (const jam::Array<int>& columnWidths,
                                                           const jam::Array<juce::String>& alignment)
{
    jam::Strings markers;

    for (int column { 0 }; column < alignment.size(); ++column)
    {
        const auto isLeading { alignment.at (column).compare (Id::left.toString()) == 0
                               or alignment.at (column).compare (Id::center.toString()) == 0 };
        const auto isTrailing { alignment.at (column).compare (Id::right.toString()) == 0
                                or alignment.at (column).compare (Id::center.toString()) == 0 };
        const auto dashes { columnWidths.at (column)
                            - (isLeading ? alignmentMarkerColonWidth : 0)
                            - (isTrailing ? alignmentMarkerColonWidth : 0) };

        markers.add ((isLeading ? juce::String::charToString (Chars::colon) : juce::String())
                     + juce::String::repeatedString (juce::String::charToString (Chars::dash),
                                                     dashes)
                     + (isTrailing ? juce::String::charToString (Chars::colon) : juce::String()));
    }

    return markers;
}

juce::String MarkdownWriter::getGridRowsText (const Element& block, const Element& header,
                                              const jam::Array<int>& columnWidths,
                                              const jam::Array<juce::String>& alignment,
                                              const jam::Strings& bodyCells)
{
    jam::Strings lines { getBorderText (columnWidths, Chars::dash) };
    int cellIndex { 0 };

    for (auto* child : block)
    {
        if (MarkdownDocument::isTableBorder (*child))
        {
            lines.add (getBorderText (columnWidths, Chars::dash));
        }
        else
        {
            jam::Strings cells;

            for (auto* cell : *child)
                cells.add (child == &header ? getCellText (*cell) : bodyCells.at (cellIndex++));

            lines.add (getGridRowText (cells, columnWidths, alignment));

            if (child == &header)
                lines.add (getBorderText (columnWidths, Chars::equals));
        }
    }

    return lines.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
}

juce::String MarkdownWriter::getGridTableText (const Element& block, const Element& header,
                                               const jam::Array<juce::String>& alignment) const
{
    if (maxTableWidth > 0)
    {
        auto minimumWidths { getMinimumColumnWidths (alignment) };
        const auto columnCount { minimumWidths.size() };
        const auto overhead { columnCount + 1 + columnCount * cellFlankSpaceWidth * 2 };
        const auto limit { (maxTableWidth - overhead) / columnCount };
        const auto cells { getCellReflows (block, header, limit) };
        const auto columnWidths { getClampedColumnWidths (std::move (minimumWidths), header, cells) };
        jam::Strings bodyCells;

        for (const auto& cell : cells)
            bodyCells.add (getWrappedText (cell));

        return getGridRowsText (block, header, columnWidths, alignment, bodyCells);
    }

    jam::Strings bodyCells;

    for (auto* row : block)
    {
        if (not MarkdownDocument::isTableBorder (*row) and row != &header)
        {
            for (auto* cell : *row)
                bodyCells.add (getCellText (*cell));
        }
    }

    return getGridRowsText (block, header, getTableColumnWidths (block, alignment), alignment, bodyCells);
}

juce::String MarkdownWriter::getPipeTableText (const Element& block, const Element& header,
                                               const jam::Array<int>& columnWidths,
                                               const jam::Array<juce::String>& alignment)
{
    const auto markers { getPipeTableAlignmentMarkers (columnWidths, alignment) };
    jam::Strings lines;

    for (auto* child : block)
    {
        jam::Strings cells;

        for (auto* cell : *child)
            cells.add (getCellText (*cell));

        lines.add (getRowText (cells, columnWidths, alignment, 0));

        if (child == &header)
            lines.add (getRowText (markers, columnWidths, alignment, 0));
    }

    return lines.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
}

juce::String MarkdownWriter::getTableText (const Element& block) const
{
    const auto& header { *block.firstChild };
    const auto alignment { getTableAlignment (header) };

    if (isGridTable (block))
        return getGridTableText (block, header, alignment);

    return getPipeTableText (block, header, getTableColumnWidths (block, alignment), alignment);
}

juce::String MarkdownWriter::getLiteralText (const Element& element)
{
    return *element.get<juce::String> (Id::text);
}

juce::String MarkdownWriter::getLineBreakText (const Element&)
{
    return juce::String::charToString (Chars::backslash)
           + juce::String::charToString (Chars::newline);
}

juce::String MarkdownWriter::getCodeText (const Element& element)
{
    juce::String text;

    if (element.firstChild != nullptr and element.firstChild->contains (Id::text))
        text = *element.firstChild->get<juce::String> (Id::text);

    int width { 0 };
    int index { 0 };

    while (index < text.length())
    {
        int column { 0 };

        while (index < text.length() and text[index] == Chars::backtick)
        {
            ++column;
            ++index;
        }

        width = std::max (width, column);

        if (column == 0)
            ++index;
    }

    const auto fence { juce::String::repeatedString (juce::String::charToString (Chars::backtick),
                                                      width + codeFenceEscapeMargin) };

    return fence + text + fence;
}

juce::String MarkdownWriter::getEmphasisText (const Element& element)
{
    return juce::String::charToString (Chars::underscore) + getContentText (element)
           + juce::String::charToString (Chars::underscore);
}

juce::String MarkdownWriter::getStrongText (const Element& element)
{
    const auto marker { juce::String::repeatedString (juce::String::charToString (Chars::asterisk),
                                                       strongMarkerWidth) };

    return marker + getContentText (element) + marker;
}

juce::String MarkdownWriter::getStrikethroughText (const Element& element)
{
    const auto marker { juce::String::repeatedString (juce::String::charToString (Chars::tilde),
                                                       strikethroughMarkerWidth) };

    return marker + getContentText (element) + marker;
}

const jam::Function::Map<int, juce::String>& MarkdownWriter::getBlocks()
{
    static const auto blocks {
        []
        {
            jam::Function::Map<int, juce::String> blocks;
            blocks.add<const MarkdownWriter&, const Element&> (map::BlockType::paragraph, &MarkdownWriter::getParagraphText);
            blocks.add<const MarkdownWriter&, const Element&> (map::BlockType::heading,
                [] (const MarkdownWriter&, const Element& block) { return getHeadingText (block); });
            blocks.add<const MarkdownWriter&, const Element&> (map::BlockType::blockquote, &MarkdownWriter::getBlockquoteText);
            blocks.add<const MarkdownWriter&, const Element&> (map::BlockType::codeBlock,
                [] (const MarkdownWriter&, const Element& block) { return getFencedBlockText (block); });
            blocks.add<const MarkdownWriter&, const Element&> (map::BlockType::mermaid,
                [] (const MarkdownWriter&, const Element& block) { return getFencedBlockText (block); });
            blocks.add<const MarkdownWriter&, const Element&> (map::BlockType::thematicBreak,
                [] (const MarkdownWriter&, const Element& block) { return getThematicBreakText (block); });
            blocks.add<const MarkdownWriter&, const Element&> (map::BlockType::htmlBlock,
                [] (const MarkdownWriter&, const Element& block) { return getHtmlBlockText (block); });
            blocks.add<const MarkdownWriter&, const Element&> (map::BlockType::image,
                [] (const MarkdownWriter&, const Element& block) { return getLinkText (block, true, Id::url); });
            blocks.add<const MarkdownWriter&, const Element&> (map::BlockType::list, &MarkdownWriter::getListText);
            blocks.add<const MarkdownWriter&, const Element&> (map::BlockType::listItem, &MarkdownWriter::getListItemText);
            blocks.add<const MarkdownWriter&, const Element&> (map::BlockType::table, &MarkdownWriter::getTableText);
            return blocks;
        }()
    };

    return blocks;
}

const jam::Function::Map<juce::String, juce::String>& MarkdownWriter::getInlines()
{
    static const auto inlines {
        []
        {
            jam::Function::Map<juce::String, juce::String> inlines;

            inlines.add<const Element&> (Id::text, getLiteralText);
            inlines.add<const Element&> (Id::br, getLineBreakText);
            inlines.add<const Element&> (Id::code, getCodeText);
            inlines.add<const Element&> (Id::em, getEmphasisText);
            inlines.add<const Element&> (Id::strong, getStrongText);
            inlines.add<const Element&> (Id::del, getStrikethroughText);
            inlines.add<const Element&> (Id::a,
                                         [] (const Element& element) -> juce::String
                                         {
                                             return getLinkText (element, false, Id::href);
                                         });
            inlines.add<const Element&> (Id::img,
                                         [] (const Element& element) -> juce::String
                                         {
                                             return getLinkText (element, true, Id::href);
                                         });

            return inlines;
        }()
    };

    return inlines;
}

juce::String MarkdownWriter::getText (const Element& element) const
{
    if (element.contains (Id::type))
        return getBlocks().get (*element.get<int> (Id::type), *this, element);

    return getInlines().get (element.id, element);
}

/*____________________________________________________________________________*/
}// namespace jam
