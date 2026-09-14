namespace jam
{
/*____________________________________________________________________________*/

juce::String MarkdownWriter::getText (const Document& document) const
{
    jam::Array<Element*> children;

    for (auto* child : *document.root)
        children.add (child);

    return getText (children);
}

juce::String MarkdownWriter::getText (const Document& document, const juce::String& origin) const
{
    jam::Array<Element*> children;

    for (auto* child : *document.root)
        if (child->contains (Id::path) and *child->get<juce::String> (Id::path) == origin)
            children.add (child);

    return getText (children);
}

juce::String MarkdownWriter::getText (const jam::Array<Element*>& children)
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
        inlines.add (getText (*child));

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

int MarkdownWriter::getClusterHeadCells (uint32_t codepoint) noexcept
{
    const int rawWidth { jam::AttributedChar::width (codepoint) };

    if (rawWidth >= 1) return rawWidth;
    if (rawWidth < 0) return 1;

    return jam::AttributedChar::isCombining (codepoint)
               ? 1
               : (codepoint <= unprintableByteMax ? unprintableByteCells
                                                   : unprintableCodepointCells);
}

int MarkdownWriter::getDisplayWidth (const juce::String& text) noexcept
{
    int displayWidth { 0 };
    auto cursor { text.getCharPointer() };
    auto segmentation { jam::AttributedChar::graphemeSegmentationInit() };

    while (not cursor.isEmpty())
    {
        const auto codepoint { static_cast<uint32_t> (cursor.getAndAdvance()) };
        segmentation = jam::AttributedChar::graphemeSegmentationStep (segmentation, codepoint);

        if (not segmentation.addToCurrentCell())
            displayWidth += getClusterHeadCells (codepoint);
    }

    return displayWidth;
}

juce::String MarkdownWriter::getFillText (int fillWidth)
{
    return juce::String::repeatedString (juce::String::charToString (Chars::space), fillWidth);
}

int MarkdownWriter::getFillWidth (const juce::String& text, int width)
{
    return std::max (0, width - getDisplayWidth (text));
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

juce::String MarkdownWriter::getParagraphText (const Element& block)
{
    return getContentText (block);
}

juce::String MarkdownWriter::getHeadingText (const Element& block)
{
    const auto level { block.get<juce::String> (Id::level)->getIntValue() };

    return juce::String::repeatedString (juce::String::charToString (Chars::hash), level)
           + juce::String::charToString (Chars::space) + getContentText (block);
}

juce::String MarkdownWriter::getBlockquoteText (const Element& block)
{
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
                                   : juce::String::charToString (Chars::greaterThan)
                                         + juce::String::charToString (Chars::space) + text);
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

juce::String MarkdownWriter::getListText (const Element& block)
{
    jam::Strings lines;

    for (auto* child : block)
        lines.addLines (getText (*child));

    return lines.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
}

jam::Strings MarkdownWriter::getListItemLines (const Element& block)
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

juce::String MarkdownWriter::getListItemText (const Element& block)
{
    const auto lines { getListItemLines (block) };
    const auto marker { getListItemMarker (block) };
    jam::Strings result;

    for (int index { 0 }; index < lines.size(); ++index)
    {
        const auto text { lines.at (index) };
        result.add (index == 0
                         ? marker + juce::String::charToString (Chars::space) + text
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

jam::Array<int> MarkdownWriter::getTableColumnWidths (const Element& block,
                                                      const jam::Array<juce::String>& alignment)
{
    jam::Array<int> columnWidths;

    for (int column { 0 }; column < alignment.size(); ++column)
        columnWidths.add (alignment.at (column).compare (Id::center.toString()) == 0
                               ? centerAlignedMinimumColumnWidth
                               : (alignment.at (column).isNotEmpty()
                                      ? alignedMinimumColumnWidth
                                      : unalignedMinimumColumnWidth));

    for (auto* child : block)
        if (not MarkdownDocument::isTableBorder (*child))
        {
            int column { 0 };

            for (auto* cell : *child)
            {
                for (const auto& text : jam::Strings::fromLines (getCellText (*cell)))
                    columnWidths.set (column,
                                      std::max (columnWidths.at (column), getDisplayWidth (text)));

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

juce::String MarkdownWriter::getGridTableText (const Element& block, const Element& header,
                                               const jam::Array<int>& columnWidths,
                                               const jam::Array<juce::String>& alignment)
{
    jam::Strings lines { getBorderText (columnWidths, Chars::dash) };

    for (auto* child : block)
    {
        if (MarkdownDocument::isTableBorder (*child))
            lines.add (getBorderText (columnWidths, Chars::dash));
        else
        {
            jam::Strings cells;

            for (auto* cell : *child)
                cells.add (getCellText (*cell));

            lines.add (getGridRowText (cells, columnWidths, alignment));

            if (child == &header)
                lines.add (getBorderText (columnWidths, Chars::equals));
        }
    }

    return lines.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
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

juce::String MarkdownWriter::getTableText (const Element& block)
{
    const auto& header { *block.firstChild };
    const auto alignment { getTableAlignment (header) };
    const auto columnWidths { getTableColumnWidths (block, alignment) };

    if (isGridTable (block))
        return getGridTableText (block, header, columnWidths, alignment);

    return getPipeTableText (block, header, columnWidths, alignment);
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

            blocks.add<const Element&> (map::BlockType::paragraph, getParagraphText);
            blocks.add<const Element&> (map::BlockType::heading, getHeadingText);
            blocks.add<const Element&> (map::BlockType::blockquote, getBlockquoteText);
            blocks.add<const Element&> (map::BlockType::codeBlock, getFencedBlockText);
            blocks.add<const Element&> (map::BlockType::mermaid, getFencedBlockText);
            blocks.add<const Element&> (map::BlockType::thematicBreak, getThematicBreakText);
            blocks.add<const Element&> (map::BlockType::htmlBlock, getHtmlBlockText);
            blocks.add<const Element&> (map::BlockType::image,
                                        [] (const Element& block) -> juce::String
                                        {
                                            return getLinkText (block, true, Id::url);
                                        });
            blocks.add<const Element&> (map::BlockType::list, getListText);
            blocks.add<const Element&> (map::BlockType::listItem, getListItemText);
            blocks.add<const Element&> (map::BlockType::table, getTableText);

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

juce::String MarkdownWriter::getText (const Element& element)
{
    if (element.contains (Id::type))
        return getBlocks().get (*element.get<int> (Id::type), element);

    return getInlines().get (element.id, element);
}

/*____________________________________________________________________________*/
}// namespace jam
