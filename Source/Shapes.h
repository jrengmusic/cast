#pragma once
#include <JuceHeader.h>
#include "Items.h"
#include "Model.h"
#include "TemplateDocument.h"

/**
 * @struct Shapes
 * @brief Static renderer that substitutes a shape's @c :::token::: markers
 *        with a Model row's resolved values, walking a structure scope's
 *        nested paragraph and list-item lines depth by depth.
 *
 * Shapes owns no state of its own; every member is a pure function of the
 * Model rows, TemplateDocument shapes, and structure lines it is called
 * with. Comment resolution walks a shape's own list sources in authored
 * order to the first one addressing a table, falling back to the row's
 * own table when none does; each marker substitutes its first remaining
 * occurrence per template line; a shape's @c list marker is inline when
 * it is authored anywhere but column zero of its own template line.
 */
struct Shapes
{
    using Element = Model::Element;

    /** The number of spaces one structure depth indents. */
    static constexpr int indentWidth { 4 };

    /**
     * @brief Returns the number of @c :::list::: placeholders @p line's
     *        own shape declares -- the count of following structure lines
     *        the shape consumes as its sources.
     *
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param line              The structure line whose shape's
     *                         placeholder tokens are counted.
     * @returns The number of @c list placeholders declared by @p line's
     *          shape.
     */
    static int getArity (const TemplateDocument& templateDocument, Element& line)
    {
        const juce::Identifier shapeId { *line.get<juce::String> (Id::templatePath) };
        const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };

        return static_cast<int> (std::count (tokens.begin(), tokens.end(), Id::list));
    }

    /**
     * @brief Returns the next structure line after @p line, skipping past
     *        every list source @p line's own shape consumes when @p line
     *        is a shape paragraph.
     *
     * @param model            The model @p line belongs to.
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param line             The structure line to advance past.
     * @returns The next structure line, or @c nullptr when none remains.
     */
    static Element*
    getLineAfter (const Model& model, const TemplateDocument& templateDocument, Element& line)
    {
        auto* cursor { model.getNextLine (line) };

        if (line.isTag (Id::p))
            for (int occurrence { 0 };
                 cursor != nullptr and occurrence < getArity (templateDocument, line); ++occurrence)
                cursor = getLineAfter (model, templateDocument, *cursor);

        return cursor;
    }

    /**
     * @brief Returns the @p occurrence-th source line following @p line,
     *        walking forward through getLineAfter() one full source at a
     *        time.
     *
     * @param model            The model @p line belongs to.
     * @param templateDocument The template document each walked line's
     *                         shape is read from.
     * @param line             The structure line @p occurrence is counted
     *                         from.
     * @param occurrence       How many sources to walk past before
     *                         returning.
     * @returns The @p occurrence-th source line, or @c nullptr when none
     *          remains.
     */
    static Element* getSourceLine (const Model& model, const TemplateDocument& templateDocument,
        Element& line, int occurrence)
    {
        auto* cursor { model.getNextLine (line) };

        for (int index { 0 }; cursor != nullptr and index < occurrence; ++index)
            cursor = getLineAfter (model, templateDocument, *cursor);

        return cursor;
    }

    /**
     * @brief Returns @p row's structure scope's first shape line -- the
     *        paragraph or list item whose @c shape ordinal is zero.
     *
     * @param model The model @p row belongs to.
     * @param row   The row whose structure scope is searched.
     * @returns @p row's first shape line, or @c nullptr when its
     *          structure scope declares none.
     */
    static Element* getFirstLine (const Model& model, Element& row)
    {
        Element* line { nullptr };

        model.getTableCell (row, Id::structure)
            ->applyFunctionRecursively (
                [&line] (const Element& candidate) -> bool
                {
                    if (line == nullptr and candidate.contains (Id::shape)
                        and *candidate.get<int> (Id::shape) == 0
                        and (candidate.isTag (Id::p) or candidate.id == Id::list))
                        line = const_cast<Element*> (&candidate);

                    return line == nullptr;
                });

        return line;
    }

    /**
     * @brief Resolves @p line's comment table -- a list item's own
     *        addressed table, a paragraph's explicitly authored
     *        @-comment address, or, absent one, the first table addressed
     *        by @p line's list sources in authored order, falling back to
     *        @p row's own table when none of its sources address one.
     *
     * @param model            The model @p row and @p line belong to.
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param row              The row @p line's sources are addressed
     *                         against.
     * @param line             The structure line whose comment table is
     *                         resolved.
     * @returns The resolved comment table or code block, @p row's own
     *          table when no source resolves one but it carries a
     *          comment, or @c nullptr when neither resolves.
     */
    static Element* getCommentTable (const Model& model, const TemplateDocument& templateDocument,
        Element& row, Element& line)
    {
        Element* table { nullptr };

        if (not line.isTag (Id::p))
        {
            table = model.getTable (row,
                *model.getSource (row, *line.get<int> (Id::level), *line.get<int> (Id::line))
                     ->get<juce::String> (Id::value));
        }
        else if (auto* comment { model.getComment (row, *line.get<int> (Id::level), *line.get<int> (Id::line)) })
        {
            const auto& value { *comment->get<juce::String> (Id::value) };

            table = model.getTable (row, value);

            if (table == nullptr)
                return model.getCodeBlock (juce::Identifier (jam::Format::getPostColon (value).trim()));
        }
        else
        {
            for (int occurrence { 0 };
                 table == nullptr and occurrence < getArity (templateDocument, line); ++occurrence)
            {
                auto* sourceLine { getSourceLine (model, templateDocument, line, occurrence) };

                if (sourceLine != nullptr and not sourceLine->isTag (Id::p))
                    table = model.getTable (row,
                        *model.getSource (row, *sourceLine->get<int> (Id::level),
                             *sourceLine->get<int> (Id::line))
                             ->get<juce::String> (Id::value));
            }
        }

        if (table != nullptr)
            return table;

        if (row.parent->contains (Id::comment) and row.parent->get<juce::String> (Id::comment)->isNotEmpty())
            return row.parent;

        return nullptr;
    }

    /**
     * @brief Resolves @p name's value for one structure line -- @p line's
     *        own binding, the resolved comment when @p name is
     *        @c Id::comment, or @p row's own column cell.
     *
     * @param model            The model @p row and @p line belong to.
     * @param templateDocument The template document a binding's value is
     *                         resolved through.
     * @param row              The row @p line and @p commentTable belong
     *                         to.
     * @param line             The structure line @p name's binding is read
     *                         from.
     * @param commentTable     The comment table resolved by
     *                         getCommentTable(), or @c nullptr when none
     *                         resolved.
     * @param name             The token or column name to resolve.
     * @returns The resolved value, or an empty string when nothing
     *          resolves @p name.
     */
    static juce::String getTokenValue (const Model& model, const TemplateDocument& templateDocument,
        Element& row, Element& line, Element* commentTable, const juce::Identifier& name)
    {
        if (auto* binding { model.getBinding (row, Id::structure, line, name) })
            return templateDocument.getBinding (model, row, *binding->get<juce::String> (Id::value));

        if (name == Id::comment)
            return commentTable != nullptr ? *commentTable->get<juce::String> (Id::comment)
                                           : juce::String();

        if (auto* cell { model.getTableCell (row, name) })
            return *cell->get<juce::String> (Id::value);

        return {};
    }

    /**
     * @brief Substitutes each of @p tokens' first remaining marker in
     *        @p templateLine with its resolved value -- the @c list token
     *        filled by getFill(), every other token resolved through
     *        getTokenValue() -- wrapping the @c comment token's value as a
     *        block comment when its marker spans the entire trimmed line,
     *        or an inline comment otherwise.
     *
     * @param model            The model @p rows and @p lines belong to.
     * @param templateDocument The template document @p tokens and each
     *                         shape's code block are read from.
     * @param tables           The tables searched when the @c list token
     *                         expands.
     * @param rows             The rows corresponding, index by index, to
     *                         @p lines.
     * @param lines            The structure lines corresponding, index by
     *                         index, to @p rows.
     * @param commentTable     The comment table reused for every token but
     *                         @c comment, which resolves its own table
     *                         from @p lines' first entry.
     * @param tokens           The shape's placeholder tokens to
     *                         substitute.
     * @param occurrence       Each token's substitution count so far,
     *                         advanced by one per substituted marker.
     * @param templateLine     The shape's authored template line to
     *                         substitute.
     * @param joinText         The join text passed through to the
     *                         @c list token's expansion.
     * @param parentIndent     The parent shape's own indent, passed
     *                         through to the @c list token's expansion.
     * @param extension        The target file extension a comment value is
     *                         commented for.
     * @returns The substituted line text, paired with whether
     *          @p templateLine carried at least one of @p tokens' markers.
     */
    static std::pair<juce::String, bool> getSubstitutedLine (const Model& model,
        const TemplateDocument& templateDocument, const jam::Array<Element*>& tables,
        const jam::Array<Element*>& rows, const jam::Array<Element*>& lines, Element* commentTable,
        const jam::Document::Identifiers& tokens, jam::HashMap<juce::Identifier, int>& occurrence,
        const juce::String& templateLine, const juce::String& joinText, int parentIndent,
        const juce::String& extension)
    {
        auto lineText { templateLine };
        auto lineHasPlaceholder { false };

        for (const auto& name : tokens)
            if (jam::Format::hasPlaceholder (lineText, name.toString()))
            {
                lineHasPlaceholder = true;
                const auto isAtColumnZero { templateLine.indexOf (Items::getMarker (name)) == 0 };
                auto& tokenOccurrence { occurrence[name] };
                auto* markerCommentTable { name == Id::comment
                                               ? getCommentTable (model, templateDocument, *rows.first(),
                                                     *lines.first())
                                               : commentTable };
                auto value { name == Id::list
                                 ? getFill (model, templateDocument, tables, rows, lines,
                                       tokenOccurrence, joinText, parentIndent, isAtColumnZero, extension)
                                 : getTokenValue (model, templateDocument, *rows.first(), *lines.first(),
                                       markerCommentTable, name) };

                if (name == Id::comment and value.isNotEmpty())
                    value = templateLine.trim() == Items::getMarker (name)
                                ? Transforms::toCommentBlock (value, extension)
                                : Transforms::toComment (value, extension);

                lineText = jam::Format::upTo (lineText, Items::getMarker (name), false) + value
                         + jam::Format::from (lineText, Items::getMarker (name), false);
                ++tokenOccurrence;
            }

        return { lineText, lineHasPlaceholder };
    }

    /**
     * @brief Renders @p shapeId's full template text, substituting every
     *        authored line through getSubstitutedLine() and collapsing
     *        consecutive blank lines produced by an empty substitution
     *        into one.
     *
     * @param model            The model @p rows and @p lines belong to.
     * @param templateDocument The template document @p shapeId's code
     *                         block is read from.
     * @param tables           The tables searched when the @c list token
     *                         expands.
     * @param rows             The rows corresponding, index by index, to
     *                         @p lines.
     * @param lines            The structure lines corresponding, index by
     *                         index, to @p rows.
     * @param commentTable     The comment table reused for every token but
     *                         @c comment.
     * @param shapeId          The shape whose template lines are rendered.
     * @param tokens           @p shapeId's placeholder tokens.
     * @param joinText         The join text passed through to the
     *                         @c list token's expansion.
     * @param parentIndent     The parent shape's own indent, passed
     *                         through to the @c list token's expansion.
     * @param extension        The target file extension a comment value is
     *                         commented for.
     * @returns @p shapeId's rendered text, one substituted line per
     *          authored template line, joined by newline.
     */
    static juce::String getLines (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<Element*>& tables, const jam::Array<Element*>& rows,
        const jam::Array<Element*>& lines, Element* commentTable, const juce::Identifier& shapeId,
        const jam::Document::Identifiers& tokens, const juce::String& joinText, int parentIndent,
        const juce::String& extension)
    {
        jam::Strings textLines;
        auto previousLineIsEmpty { false };
        jam::HashMap<juce::Identifier, int> occurrence;

        for (const auto& templateLine : jam::Strings::fromLines (
                 *templateDocument.getCodeBlock (shapeId)->get<juce::String> (Id::value)))
        {
            const auto [lineText, lineHasPlaceholder] { getSubstitutedLine (model, templateDocument,
                tables, rows, lines, commentTable, tokens, occurrence, templateLine, joinText,
                parentIndent, extension) };
            const auto lineIsEmpty { lineHasPlaceholder and lineText.trim().isEmpty() };

            if (lineIsEmpty)
                previousLineIsEmpty = true;
            else if (previousLineIsEmpty and lineText.trim().isEmpty())
                previousLineIsEmpty = false;
            else
            {
                textLines.add (lineText);
                previousLineIsEmpty = false;
            }
        }

        return textLines.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
    }

    /**
     * @brief Prefixes every non-empty line of @p text with the spaces
     *        needed to align @p level's indent against @p parentIndent,
     *        when @p isAtColumnZero.
     *
     * @param text           The text to indent.
     * @param level          The structure depth @p text's target indent is
     *                       computed from.
     * @param parentIndent   The parent shape's own indent, subtracted from
     *                       @p level's computed indent.
     * @param isAtColumnZero Whether the placeholder @p text fills began at
     *                       column zero in its template line -- when
     *                       @c false, @p text is returned unindented.
     * @returns @p text, indented when @p isAtColumnZero and the computed
     *          indent is non-zero, or @p text itself otherwise.
     */
    static juce::String getIndentedText (const juce::String& text, int level, int parentIndent,
        bool isAtColumnZero)
    {
        auto indentedText { text };

        if (isAtColumnZero)
        {
            const auto indent { level * indentWidth - parentIndent };

            if (indent != 0)
            {
                const auto prefix { juce::String::repeatedString (
                    juce::String::charToString (Chars::space), indent) };
                jam::Strings prefixedLines;

                for (const auto& textLine : jam::Strings::fromLines (text))
                    prefixedLines.add (textLine.isNotEmpty() ? prefix + textLine : textLine);

                indentedText = prefixedLines.joinIntoString (
                    juce::String::charToString (Chars::newline), 0, -1);
            }
        }

        return indentedText;
    }

    /**
     * @brief Renders @p sourceLines' shared shape group through
     *        getShape(), then indents the result to @p sourceLines' own
     *        structure depth.
     *
     * @param model            The model @p tables, @p rows, and
     *                         @p sourceLines belong to.
     * @param templateDocument The template document each shape's code
     *                         block is read from.
     * @param tables           The tables searched when a nested @c list
     *                         token expands.
     * @param rows             The rows corresponding, index by index, to
     *                         @p sourceLines.
     * @param sourceLines      The paragraph structure lines to render,
     *                         sharing one structure depth.
     * @param joinText         The join text passed through to getShape().
     * @param parentIndent     The parent shape's own indent, subtracted
     *                         from @p sourceLines' computed indent.
     * @param isAtColumnZero   Whether the enclosing @c list marker began
     *                         at column zero.
     * @param extension        The target file extension a comment value is
     *                         commented for.
     * @returns The rendered, indented shape group text.
     */
    static juce::String getShapeText (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<Element*>& tables, const jam::Array<Element*>& rows,
        const jam::Array<Element*>& sourceLines, const juce::String& joinText, int parentIndent,
        bool isAtColumnZero, const juce::String& extension)
    {
        const auto text { getShape (model, templateDocument, tables, rows, sourceLines, joinText,
            *sourceLines.first()->get<int> (Id::level) * indentWidth, extension) };

        return getIndentedText (
            text, *sourceLines.first()->get<int> (Id::level), parentIndent, isAtColumnZero);
    }

    /**
     * @brief Renders each of @p rows' addressed sources through
     *        Items::getJoinedItems(), one item group per (row,
     *        sourceLine) pair -- its own nested list joined by its
     *        @c separator line when the group's shape declares a @c list
     *        token not authored inline, or by newline otherwise --
     *        deduplicates identical groups, joins the result with
     *        @p joinText, then indents it to @p sourceLines' own
     *        structure depth.
     *
     * A shape's @c list marker is inline when it is authored anywhere but
     * the very start of its own template line.
     *
     * @param model            The model @p tables, @p rows, and
     *                         @p sourceLines belong to.
     * @param templateDocument The template document each shape's code
     *                         block is read from.
     * @param tables           The tables each item's source is addressed
     *                         against.
     * @param rows             The rows corresponding, index by index, to
     *                         @p sourceLines.
     * @param sourceLines      The list-item structure lines to render,
     *                         sharing one structure depth.
     * @param joinText         The join text between distinct rendered item
     *                         groups.
     * @param parentIndent     The parent shape's own indent, subtracted
     *                         from @p sourceLines' computed indent.
     * @param isAtColumnZero   Whether the enclosing @c list marker began
     *                         at column zero.
     * @param extension        The target file extension a comment value is
     *                         commented for.
     * @returns The rendered, indented item text.
     */
    static juce::String getItemText (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<Element*>& tables, const jam::Array<Element*>& rows,
        const jam::Array<Element*>& sourceLines, const juce::String& joinText, int parentIndent,
        bool isAtColumnZero, const juce::String& extension)
    {
        jam::Strings itemTexts;

        for (int index { 0 }; index < rows.size(); ++index)
        {
            auto& row { *rows.at (index) };
            auto& sourceLine { *sourceLines.at (index) };
            const auto indent { *sourceLine.get<int> (Id::level) };
            const auto ordinal { *sourceLine.get<int> (Id::line) };
            const auto sourceValue {
                *model.getSource (row, indent, ordinal)->get<juce::String> (Id::value)
            };
            const auto shapeId { juce::Identifier (*sourceLine.get<juce::String> (Id::templatePath)) };
            const auto& shapeTokens { *templateDocument.getCodeBlock (shapeId)
                                            ->get<jam::Document::Identifiers> (Id::placeholder) };
            const auto shapeHasListToken {
                std::find (shapeTokens.begin(), shapeTokens.end(), Id::list) != shapeTokens.end()
            };
            auto listMarkerIsInline { false };

            if (shapeHasListToken)
            {
                const auto marker { Items::getMarker (Id::list) };

                for (const auto& shapeLine : jam::Strings::fromLines (
                         *templateDocument.getCodeBlock (shapeId)->get<juce::String> (Id::value)))
                {
                    const auto markerPosition { shapeLine.indexOf (marker) };

                    if (markerPosition >= 0)
                        listMarkerIsInline = markerPosition != 0;
                }
            }

            auto* separatorLine {
                listMarkerIsInline ? nullptr : model.getSeparator (row, indent, ordinal)
            };
            const auto join { separatorLine != nullptr
                                   ? templateDocument.getBinding (
                                         model, row, *separatorLine->get<juce::String> (Id::value))
                                   : juce::String::charToString (Chars::newline) };
            auto* childSeparatorLine { model.getSeparator (row, indent, ordinal) };
            const auto childJoin { childSeparatorLine != nullptr
                                        ? templateDocument.getBinding (
                                              model, row, *childSeparatorLine->get<juce::String> (Id::value))
                                        : juce::String() };
            const auto itemText { Items::getJoinedItems (model, templateDocument, tables, row,
                sourceValue, shapeId, join, indent, ordinal, childJoin, extension) };

            if (not itemTexts.contains (itemText, false))
                itemTexts.add (itemText);
        }

        const auto text { itemTexts.joinIntoString (joinText, 0, -1) };

        return getIndentedText (
            text, *sourceLines.first()->get<int> (Id::level), parentIndent, isAtColumnZero);
    }

    /**
     * @brief Fills one @c list token occurrence -- resolves @p occurrence's
     *        source line for every entry in @p lines whose authored
     *        sources reach that far, aligning under-supplied sources to
     *        the shape's trailing slots and eliding each leading unfilled
     *        slot, renders the paragraph sources through getShapeText()
     *        and the list-item sources through getItemText(), and joins
     *        the two groups with @p joinText.
     *
     * @param model            The model @p tables and @p rows belong to.
     * @param templateDocument The template document each source's shape is
     *                         read from.
     * @param tables           The tables searched when a nested @c list
     *                         token expands.
     * @param rows             The rows corresponding, index by index, to
     *                         @p lines.
     * @param lines            The structure lines whose @p occurrence-th
     *                         source is resolved and rendered.
     * @param occurrence       Which of @p lines' list-token occurrences to
     *                         fill.
     * @param joinText         The join text between the paragraph and item
     *                         groups' rendered text.
     * @param parentIndent     The parent shape's own indent, passed
     *                         through to getShapeText() and getItemText().
     * @param isAtColumnZero   Whether the @c list marker being filled
     *                         began at column zero.
     * @param extension        The target file extension a comment value is
     *                         commented for.
     * @returns The rendered text for @p occurrence, paragraph sources
     *          followed by item sources, joined by @p joinText.
     */
    static juce::String getFill (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<Element*>& tables, const jam::Array<Element*>& rows,
        const jam::Array<Element*>& lines, int occurrence, const juce::String& joinText,
        int parentIndent, bool isAtColumnZero, const juce::String& extension)
    {
        jam::Array<Element*> shapeRows;
        jam::Array<Element*> shapeSourceLines;
        jam::Array<Element*> itemRows;
        jam::Array<Element*> itemSourceLines;

        for (int index { 0 }; index < rows.size(); ++index)
        {
            auto* structureLine { lines.at (index) };
            const auto arity { getArity (templateDocument, *structureLine) };
            auto availableCount { 0 };

            while (availableCount < arity
                   and getSourceLine (model, templateDocument, *structureLine, availableCount)
                           != nullptr)
                ++availableCount;

            const auto skippedCount { arity - availableCount };

            if (occurrence >= skippedCount)
            {
                auto* sourceLine { getSourceLine (
                    model, templateDocument, *structureLine, occurrence - skippedCount) };

                if (sourceLine->isTag (Id::p))
                {
                    shapeRows.add (rows.at (index));
                    shapeSourceLines.add (sourceLine);
                }
                else
                {
                    itemRows.add (rows.at (index));
                    itemSourceLines.add (sourceLine);
                }
            }
        }

        jam::Strings texts;

        if (not shapeRows.isEmpty())
            texts.add (getShapeText (model, templateDocument, tables, shapeRows, shapeSourceLines,
                joinText, parentIndent, isAtColumnZero, extension));

        if (not itemRows.isEmpty())
            texts.add (getItemText (model, templateDocument, tables, itemRows, itemSourceLines,
                joinText, parentIndent, isAtColumnZero, extension));

        return texts.joinIntoString (joinText, 0, -1);
    }

    /**
     * @brief Renders @p rows and @p lines' shared shape -- grouping
     *        entries that resolve to the same shape id and non-@c list
     *        token values, rendering each distinct group once through
     *        getLines(), and joining the group texts with @p joinText.
     *
     * @param model            The model @p tables and @p rows belong to.
     * @param templateDocument The template document each entry's shape is
     *                         read from.
     * @param tables           The tables searched when a @c list token
     *                         expands.
     * @param rows             The rows corresponding, index by index, to
     *                         @p lines.
     * @param lines            The structure lines corresponding, index by
     *                         index, to @p rows.
     * @param joinText         The join text between distinct rendered
     *                         groups.
     * @param parentIndent     The parent shape's own indent, passed
     *                         through to each group's rendering.
     * @param extension        The target file extension a comment value is
     *                         commented for.
     * @returns Every distinct group's rendered text, joined by
     *          @p joinText.
     */
    static juce::String getShape (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<Element*>& tables, const jam::Array<Element*>& rows,
        const jam::Array<Element*>& lines, const juce::String& joinText, int parentIndent,
        const juce::String& extension)
    {
        jam::Strings keys;

        for (int index { 0 }; index < rows.size(); ++index)
        {
            auto& row { *rows.at (index) };
            auto& line { *lines.at (index) };
            auto* commentTable { getCommentTable (model, templateDocument, row, line) };
            const auto shapeId { juce::Identifier (*line.get<juce::String> (Id::templatePath)) };
            const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                       ->get<jam::Document::Identifiers> (Id::placeholder) };
            jam::Strings key;
            key.add (shapeId.toString());

            for (const auto& name : tokens)
                if (name != Id::list)
                    key.add (getTokenValue (model, templateDocument, row, line, commentTable, name));

            keys.add (key.joinIntoString (juce::String::charToString (Chars::newline), 0, -1));
        }

        jam::Strings groupTexts;
        jam::Strings seenKeys;

        for (int index { 0 }; index < rows.size(); ++index)
            if (not seenKeys.contains (keys.at (index), true))
            {
                seenKeys.add (keys.at (index));

                jam::Array<Element*> groupRows;
                jam::Array<Element*> groupLines;

                for (int candidate { 0 }; candidate < rows.size(); ++candidate)
                    if (keys.at (candidate) == keys.at (index))
                    {
                        groupRows.add (rows.at (candidate));
                        groupLines.add (lines.at (candidate));
                    }

                auto* commentTable {
                    getCommentTable (model, templateDocument, *groupRows.first(), *groupLines.first())
                };
                const auto shapeId {
                    juce::Identifier (*groupLines.first()->get<juce::String> (Id::templatePath))
                };
                const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                           ->get<jam::Document::Identifiers> (Id::placeholder) };

                groupTexts.add (getLines (model, templateDocument, tables, groupRows, groupLines,
                    commentTable, shapeId, tokens, joinText, parentIndent, extension));
            }

        return groupTexts.joinIntoString (joinText, 0, -1);
    }
};
