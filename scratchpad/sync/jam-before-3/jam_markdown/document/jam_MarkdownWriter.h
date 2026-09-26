#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct MarkdownWriter
 * @brief Renders a jam::Document (or a Id::path-filtered subset of one) back into CommonMark/GFM markdown source text.
 *
 * Two lookups cover the full element vocabulary: getBlocks() keyed by
 * Id::type (map::BlockType) for container and leaf blocks, getInlines()
 * keyed by an element's own id for inline spans. getText (const Element&)
 * picks the right lookup per element, and every renderer below produces one
 * grammar construct's exact textual form, recursing back into content-bearing
 * elements via getText() and getContentText().
 */
struct MarkdownWriter : Document::Writer
{
    using Element = Document::Element;

    static constexpr int defaultMaxTableWidth { 0 };
    static constexpr int defaultLineWrap { 100 };

    explicit MarkdownWriter (int newMaxTableWidth = defaultMaxTableWidth, int newLineWrap = defaultLineWrap)
        : maxTableWidth (newMaxTableWidth), lineWrap (newLineWrap)
    {
    }

    /**
     * @brief Renders the document's whole root subtree into a single markdown string.
     *
     * Collects every root-level child in authored order, then joins their
     * individually rendered text with a blank line between blocks, via
     * getText (const jam::Array\<Element*\>&).
     *
     * @param document The document whose root's children are rendered.
     * @return The document's full rendered markdown source text.
     */
    juce::String getText (const Document& document) const override;

    /**
     * @brief Renders only the root-level children stamped with Id::path equal to @p origin.
     *
     * @param document The document whose root's children are filtered and rendered.
     * @param origin   The Id::path value a root-level child must carry to be included.
     * @return The rendered markdown source text of the matching children only.
     */
    juce::String getText (const Document& document, const juce::String& origin) const;

private:
    const int maxTableWidth;
    const int lineWrap;

    /** @brief Column padding added on both sides of every rendered table cell. */
    static constexpr int cellFlankSpaceWidth { 1 };

    /** @brief Number of dash characters rendered for a thematic break. */
    static constexpr int thematicBreakMarkerWidth { 3 };

    /**
     * @brief Minimum rendered content width of a center-aligned pipe-table column.
     *
     * Wide enough to hold both alignment-marker colons plus at least one
     * content character.
     */
    static constexpr int centerAlignedMinimumColumnWidth { 5 };

    /**
     * @brief Minimum rendered content width of a left- or right-aligned pipe-table column.
     *
     * Wide enough to hold one alignment-marker colon plus at least one
     * content character.
     */
    static constexpr int alignedMinimumColumnWidth { 4 };

    /** @brief Minimum rendered content width of an unaligned pipe-table column. */
    static constexpr int unalignedMinimumColumnWidth { 3 };

    /** @brief Width in characters of a single alignment-marker colon in a pipe-table's delimiter row. */
    static constexpr int alignmentMarkerColonWidth { 1 };

    /**
     * @brief Extra backtick width added to an inline code span's fence beyond its content's longest backtick run.
     *
     * Guarantees the fence can never be mistaken for the span's own content.
     */
    static constexpr int codeFenceEscapeMargin { 1 };

    /** @brief Width in characters of the marker pair wrapping strong-emphasis text. */
    static constexpr int strongMarkerWidth { 2 };

    /** @brief Width in characters of the marker pair wrapping strikethrough text. */
    static constexpr int strikethroughMarkerWidth { 2 };

    /**
     * @brief Renders a set of sibling elements, each rendered as its own block, separated by a blank line.
     *
     * @param children The elements to render, in the order they should appear.
     * @return The rendered elements joined by a blank line, with a trailing newline.
     */
    juce::String getText (const jam::Array<const Element*>& children) const;

    /**
     * @brief Renders an element's inline children back-to-back, with no separator between them.
     *
     * @param element The element whose direct children are rendered as inline content.
     * @return The concatenated rendered text of every direct child.
     */
    static juce::String getContentText (const Element& element);

    /**
     * @brief Returns a table cell's raw text with every literal pipe escaped and each backslash run before a pipe doubled.
     *
     * The row splitter reads escape parity — an odd trailing backslash run
     * escapes a pipe, an even run does not — so a literal pipe emits as its
     * escaped form with any directly preceding backslash run doubled, keeping
     * the run odd. Backslashes not abutting a pipe emit verbatim.
     *
     * @param cell The table cell element to render.
     * @return The cell's raw text in the row splitter's canonical escaped form.
     */
    static juce::String getCellText (const Element& cell) noexcept;

    /**
     * @brief A run of space characters, @p fillWidth long.
     *
     * @param fillWidth The number of space characters to repeat.
     * @return A string of @p fillWidth spaces.
     */
    static juce::String getFillText (int fillWidth);

    /**
     * @brief The space count needed to widen @p text's rendered display width up to @p width.
     *
     * @param text  The text whose display width is measured.
     * @param width The target column width.
     * @return @p width minus @p text's display width, floored at zero.
     */
    static int getFillWidth (const juce::String& text, int width);

    static std::string_view getView (const juce::String& text);

    static juce::String getWrappedText (const jam::ReflowDocument& reflow);

    static juce::String getWrappedText (const juce::String& text, int limit);

    static int getOpenBlockRow (const jam::ReflowDocument& reflow, const Element& line);

    static std::string getSourceText (const Element& block);

    /**
     * @brief Pads text with leading spaces so it is right-aligned within @p width.
     *
     * @param text  The text to pad.
     * @param width The target column width.
     * @return @p text padded on the left to @p width characters.
     */
    static juce::String getRightPaddedText (const juce::String& text, int width);

    /**
     * @brief Pads text with spaces on both sides so it is centered within @p width.
     *
     * Any odd remainder of padding is placed on the right side.
     *
     * @param text  The text to pad.
     * @param width The target column width.
     * @return @p text centered within @p width characters.
     */
    static juce::String getCenterPaddedText (const juce::String& text, int width);

    /**
     * @brief Lookup from a table column's alignment value to the padding function for that alignment.
     *
     * Built once. Holds an entry only for Id::right and Id::center — a left
     * or unaligned column pads through getPaddedText()'s own right-padding
     * fallback rather than an entry here.
     *
     * @return The alignment-to-padding-function lookup.
     */
    static const jam::Function::Map<juce::String, juce::String>& getAlignmentPadding();

    /**
     * @brief Pads a cell's text to its column width per @p alignment, then flanks it with one space on each side.
     *
     * Looks up @p alignment in getAlignmentPadding(); a match dispatches to
     * that alignment's padding function, otherwise the text is left-aligned
     * via right-padding. Every rendered line of every cell — including a
     * fenced code block's content lines — passes through here, so a table's
     * right edge stays aligned regardless of what a cell contains.
     *
     * @param text      The line of cell content to pad.
     * @param width     The column's rendered content width, excluding flank spaces.
     * @param alignment The column's alignment value (Id::left/Id::right/Id::center, or empty).
     * @return @p text padded to @p width and flanked by cellFlankSpaceWidth spaces on each side.
     */
    static juce::String getPaddedText (const juce::String& text, int width,
                                       const juce::String& alignment);

    /**
     * @brief Renders one physical line of a table row, across every column, at @p lineIndex.
     *
     * A cell may itself span several lines (e.g. a fenced code block); this
     * renders only the line at @p lineIndex from each cell, padding empty
     * when a cell has fewer lines than @p lineIndex demands.
     *
     * @param cells        The row's cell text, one entry per column, each possibly multi-line.
     * @param columnWidths The rendered content width of each column.
     * @param alignment    The alignment value of each column.
     * @param lineIndex    The zero-based line index within each cell to render.
     * @return The pipe-delimited row line for @p lineIndex.
     */
    static juce::String getRowText (const jam::Strings& cells,
                                    const jam::Array<int>& columnWidths,
                                    const jam::Array<juce::String>& alignment,
                                    int lineIndex);

    /**
     * @brief Renders every physical line of a table row, across the tallest cell's line count.
     *
     * Determines the row's line count as its tallest cell, then renders each
     * line via getRowText(), joined by newlines — this is what lets a
     * grid-table row hold a multi-line cell such as a fenced code block.
     *
     * @param cells        The row's cell text, one entry per column, each possibly multi-line.
     * @param columnWidths The rendered content width of each column.
     * @param alignment    The alignment value of each column.
     * @return The row's full rendered text, one or more lines joined by newlines.
     */
    static juce::String getGridRowText (const jam::Strings& cells,
                                        const jam::Array<int>& columnWidths,
                                        const jam::Array<juce::String>& alignment);

    /**
     * @brief Renders a grid-table horizontal border line, one run of @p marker per column.
     *
     * @param columnWidths The rendered content width of each column.
     * @param marker       The character repeated to fill each column's border segment.
     * @return The plus-jointed border line.
     */
    static juce::String getBorderText (const jam::Array<int>& columnWidths, juce::juce_wchar marker);

    static juce::String getBlockquotePrefix();

    static const jam::Function::Map<int, juce::String>& getPrefixes();

    static int getPrefixWidth (const Element& block);

    /**
     * @brief Renders a paragraph block as its inline content, unwrapped.
     *
     * @param block The paragraph element to render.
     * @return The paragraph's rendered inline content.
     */
    juce::String getParagraphText (const Element& block) const;

    /**
     * @brief Renders an ATX heading as a run of hash characters at Id::level, followed by its inline content.
     *
     * @param block The heading element to render.
     * @return The rendered ATX heading line.
     */
    static juce::String getHeadingText (const Element& block);

    /**
     * @brief Renders a blockquote by prefixing every line of its rendered children with a quote marker.
     *
     * Each child block is rendered, its lines collected with a blank line
     * between successive children, then every resulting line is prefixed
     * with a quote marker and a space, or a bare quote marker for an empty
     * line.
     *
     * @param block The blockquote element to render.
     * @return The rendered blockquote, one quote-prefixed line per source line.
     */
    juce::String getBlockquoteText (const Element& block) const;

    /**
     * @brief Renders a fenced code block or a mermaid block as an opening fence, its literal text, and a matching closing fence.
     *
     * Both map::BlockType::codeBlock and map::BlockType::mermaid route
     * through this one renderer — a mermaid block is a fenced code block
     * whose Id::info happens to read the mermaid keyword, so the two share
     * identical fence/text/fence structure and need no separate rendering
     * path.
     *
     * @param block The fenced-code or mermaid element to render.
     * @return The fence, info string, literal content, and closing fence, each on its own line.
     */
    static juce::String getFencedBlockText (const Element& block);

    /**
     * @brief Renders a thematic break as a run of thematicBreakMarkerWidth dash characters.
     * @return The thematic break line.
     */
    static juce::String getThematicBreakText (const Element&);

    /**
     * @brief Renders a raw HTML block as its stored literal text, unmodified.
     *
     * @param block The HTML-block element to render.
     * @return The block's literal text, or an empty string if none was recorded.
     */
    static juce::String getHtmlBlockText (const Element& block);

    /**
     * @brief Renders an inline link, an inline image, or a promoted block image, distinguished only by @p isImage and @p urlAttribute.
     *
     * One renderer covers all three forms. @p isImage selects whether the
     * result is prefixed with the image marker, producing an image rather
     * than a link. @p urlAttribute selects which property on @p element
     * holds the destination URL: a parsed inline link or inline image reads
     * it from Id::href, while a block-level image promoted from a
     * sole-image paragraph during parsing reads it from Id::url instead.
     * Passing the URL attribute that does not match @p element's actual
     * origin renders a link or image whose destination is missing, since the
     * property being read is the wrong one for that element.
     *
     * @param element      The link, inline-image, or block-image element to render.
     * @param isImage      True to prefix the result with the image marker, producing an image rather than a link.
     * @param urlAttribute The property on @p element holding the destination URL — Id::href for a parsed inline link or image, Id::url for a promoted block image.
     * @return The rendered link or image, with an optional title suffix when Id::title is present.
     */
    static juce::String getLinkText (const Element& element, bool isImage,
                                     const juce::Identifier& urlAttribute);

    /**
     * @brief Renders a list by rendering each list item and joining their lines.
     *
     * @param block The list element to render.
     * @return The list items' rendered lines, joined by newlines.
     */
    juce::String getListText (const Element& block) const;

    /**
     * @brief Renders a list item's children into individual lines, with a blank line between successive children.
     *
     * @param block The list-item element whose children are rendered.
     * @return The item's rendered content split into individual lines, ready for marker prefixing by getListItemText().
     */
    jam::Strings getListItemLines (const Element& block) const;

    /**
     * @brief Zero-based position of @p block among its parent's children, in authored order.
     *
     * @param block The list-item element to locate.
     * @return The item's index within its parent list.
     */
    static int getListItemIndex (const Element& block);

    /**
     * @brief The marker text for a list item — a number and dot for an ordered list, a dash for a bullet list.
     *
     * @param block The list-item element to render a marker for.
     * @return The item's ordinal followed by a dot when its parent is an ordered list, otherwise a bare dash.
     */
    static juce::String getListItemMarker (const Element& block);

    static juce::String getListItemPrefix (const Element& block);

    /**
     * @brief Renders a list item as its marker followed by its content, with continuation lines indented to align under the marker.
     *
     * The first line is prefixed with the item's marker and a space; every
     * subsequent non-empty line is indented by two spaces to align under
     * that marker's content column, and an empty line stays empty.
     *
     * @param block The list-item element to render.
     * @return The item's fully rendered, marker-prefixed and indented text.
     */
    juce::String getListItemText (const Element& block) const;

    /**
     * @brief Whether a table renders using grid-table syntax rather than pipe-table syntax.
     *
     * A table is a grid table if any of its direct children is a table
     * border row (map::BlockType::tableBorder) — a marker only grid-table
     * source produces.
     *
     * @param block The table element to inspect.
     * @return True if @p block contains a table-border row.
     */
    static bool isGridTable (const Element& block) noexcept;

    /**
     * @brief Column alignment values for a table, read from its header row's cells.
     *
     * @param header The table's header row element.
     * @return One alignment value per column, in column order — empty for an unaligned column.
     */
    static jam::Array<juce::String> getTableAlignment (const Element& header);

    static jam::Array<jam::ReflowDocument> getCellReflows (const Element& block, const Element& header, int limit);

    static int getWrappedWidth (const jam::ReflowDocument& reflow);

    static int getLinesWidth (const juce::String& text);

    static jam::Array<int> getClampedColumnWidths (jam::Array<int> columnWidths, const Element& header,
                                                   const jam::Array<jam::ReflowDocument>& cells);

    static jam::Array<int> getMinimumColumnWidths (const jam::Array<juce::String>& alignment);

    /**
     * @brief Rendered content width of every column in a table, wide enough for its alignment marker and its widest cell line.
     *
     * Starts each column at the minimum width its alignment demands
     * (centerAlignedMinimumColumnWidth, alignedMinimumColumnWidth, or
     * unalignedMinimumColumnWidth), then walks every non-border row's
     * cells — each split into lines via getCellText() — widening the column
     * to fit its longest line.
     *
     * @param block     The table element whose rows are measured.
     * @param alignment The column alignment values, in column order.
     * @return The rendered content width of every column, in column order.
     */
    jam::Array<int> getTableColumnWidths (const Element& block,
                                          const jam::Array<juce::String>& alignment) const;

    /**
     * @brief The delimiter-row text for every column of a pipe table, encoding its alignment as leading/trailing colons.
     *
     * @param columnWidths The rendered content width of each column.
     * @param alignment    The alignment value of each column.
     * @return One delimiter string per column — dashes flanked by a colon on the aligned side or sides.
     */
    static jam::Strings getPipeTableAlignmentMarkers (const jam::Array<int>& columnWidths,
                                                      const jam::Array<juce::String>& alignment);

    static juce::String getGridRowsText (const Element& block, const Element& header,
                                         const jam::Array<int>& columnWidths,
                                         const jam::Array<juce::String>& alignment,
                                         const jam::Strings& bodyCells);

    /**
     * @brief Renders a table using grid-table syntax — a bordered line around every row, with a double border beneath the header.
     *
     * @param block     The table element to render.
     * @param header    The table's header row element, used to place the double border beneath it.
     * @param alignment The alignment value of each column.
     * @return The table's full grid-table rendered text.
     */
    juce::String getGridTableText (const Element& block, const Element& header,
                                   const jam::Array<juce::String>& alignment) const;

    /**
     * @brief Renders a table using pipe-table syntax — one row per line, with a delimiter row beneath the header.
     *
     * @param block        The table element to render.
     * @param header       The table's header row element, used to place the delimiter row beneath it.
     * @param columnWidths The rendered content width of each column.
     * @param alignment    The alignment value of each column.
     * @return The table's full pipe-table rendered text.
     */
    static juce::String getPipeTableText (const Element& block, const Element& header,
                                          const jam::Array<int>& columnWidths,
                                          const jam::Array<juce::String>& alignment);

    /**
     * @brief Renders a table, choosing grid-table or pipe-table syntax per isGridTable().
     *
     * @param block The table element to render.
     * @return The table's full rendered text.
     */
    juce::String getTableText (const Element& block) const;

    /**
     * @brief Renders a plain text run as its literal text, unmodified.
     *
     * @param element The text element to render.
     * @return The element's literal text.
     */
    static juce::String getLiteralText (const Element& element);

    /**
     * @brief Renders a hard line break as a trailing backslash followed by a newline.
     * @return The rendered hard line break.
     */
    static juce::String getLineBreakText (const Element&);

    /**
     * @brief Renders an inline code span, fencing it with a backtick run one character wider than the longest backtick run already present in its content.
     *
     * Scans the span's literal text for its widest run of consecutive
     * backticks, then fences the content with a run one character wider on
     * each side (codeFenceEscapeMargin), so the fence can never be confused
     * with a backtick run inside the content itself.
     *
     * @param element The inline code element to render.
     * @return The fenced inline code span.
     */
    static juce::String getCodeText (const Element& element);

    /**
     * @brief Renders emphasis by wrapping its content in a single underscore on each side.
     *
     * @param element The emphasis element to render.
     * @return The underscore-wrapped rendered content.
     */
    static juce::String getEmphasisText (const Element& element);

    /**
     * @brief Renders strong emphasis by wrapping its content in a strongMarkerWidth run of asterisks on each side.
     *
     * @param element The strong-emphasis element to render.
     * @return The asterisk-wrapped rendered content.
     */
    static juce::String getStrongText (const Element& element);

    /**
     * @brief Renders strikethrough by wrapping its content in a strikethroughMarkerWidth run of tildes on each side.
     *
     * @param element The strikethrough element to render.
     * @return The tilde-wrapped rendered content.
     */
    static juce::String getStrikethroughText (const Element& element);

    /**
     * @brief Lookup from a block element's Id::type (map::BlockType) to the renderer that produces its markdown text.
     *
     * Built once. Covers every block type: paragraph, heading, blockquote,
     * fenced code and mermaid (both via getFencedBlockText()), thematic
     * break, raw HTML, image (via getLinkText() with isImage true and
     * Id::url as the URL attribute), list, list item, and table.
     *
     * @return The block-type-to-renderer lookup.
     */
    static const jam::Function::Map<int, juce::String>& getBlocks();

    /**
     * @brief Lookup from an inline element's own id to the renderer that produces its markdown text.
     *
     * Built once. Covers every inline kind: literal text, line break, code
     * span, emphasis, strong emphasis, strikethrough, link (via
     * getLinkText() with isImage false and Id::href as the URL attribute),
     * and image (via getLinkText() with isImage true and Id::href as the
     * URL attribute).
     *
     * @return The inline-id-to-renderer lookup.
     */
    static const jam::Function::Map<juce::String, juce::String>& getInlines();

    /**
     * @brief Renders one element by dispatching to the lookup that matches its kind.
     *
     * An element carrying Id::type is a block, rendered via getBlocks()
     * keyed by that type; any other element is an inline span, rendered via
     * getInlines() keyed by its own id.
     *
     * @param element The element to render.
     * @return The element's fully rendered markdown text.
     */
    juce::String getText (const Element& element) const;
};

}// namespace jam
