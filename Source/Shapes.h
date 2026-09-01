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
                 cursor != nullptr and occurrence < Items::getArity (templateDocument, line); ++occurrence)
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
            if (line.contains (Id::line))
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
                 table == nullptr and occurrence < Items::getArity (templateDocument, line); ++occurrence)
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

        if (row.parent->get<juce::String> (Id::comment)->isNotEmpty())
            return row.parent;

        return nullptr;
    }

    /**
     * @brief Resolves @p name's value for @p line, in the order SPEC
     *        §6.5 declares: a binding of that name, then @p line's own
     *        maps' @p name row, then @p row's own @p name column, and,
     *        for the name @c comment, @p commentTable's own
     *        documentation.
     *
     * @param model            The model @p row and @p line belong to.
     * @param templateDocument The template document each map value is
     *                         read through.
     * @param row              The row @p name's binding and column are
     *                         resolved against.
     * @param line             The structure line whose binding and maps
     *                         are searched.
     * @param commentTable     The table @p name's documentation is read
     *                         from, when @p name is @c comment.
     * @param name             The token or column name to resolve.
     * @returns @p name's resolved value, or an empty string when none of
     *          the four rungs names it.
     */
    static juce::String getTokenValue (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<Element*>& tables, Element& row, Element& line, Element* commentTable,
        const juce::Identifier& name, const juce::String& joinText, int parentIndent,
        bool isAtColumnZero, const juce::String& extension)
    {
        if (auto* binding { model.getBinding (row, Id::structure, line, name) })
        {
            if (binding->contains (Id::templatePath))
                return getShapeText (model, templateDocument, tables, row, *binding, joinText,
                    parentIndent, isAtColumnZero, extension);

            return templateDocument.getValue (model, row, *binding->get<juce::String> (Id::value));
        }

        if (name == Id::comment)
            return commentTable != nullptr ? *commentTable->get<juce::String> (Id::comment)
                                           : juce::String();

        for (int occurrence { 0 }; auto* map { model.getMap (row, line, occurrence) }; ++occurrence)
            if (auto* cell { model.getTableCell (*map, Id::value, name) })
                return *cell->get<juce::String> (Id::value);

        if (auto* cell { model.getTableCell (row, name) })
        {
            const auto& columnValue { *cell->get<juce::String> (Id::value) };
            return name == Id::file ? jam::Format::toFileName (columnValue) : columnValue;
        }

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
     * @returns The substituted line text.
     */
    static juce::String getSubstitutedLine (const Model& model,
        const TemplateDocument& templateDocument, const jam::Array<Element*>& tables,
        const jam::Array<Element*>& rows, const jam::Array<Element*>& lines, Element* commentTable,
        const jam::Document::Identifiers& tokens, jam::HashMap<juce::Identifier, int>& occurrence,
        const juce::String& templateLine, const juce::String& joinText, int parentIndent,
        const juce::String& extension)
    {
        auto lineText { templateLine };

        for (const auto& name : tokens)
        {
            const auto marker { Items::getMarker (lineText, name) };

            if (marker.isNotEmpty())
            {
                const auto isAtColumnZero { templateLine.indexOf (marker) == 0 };
                auto [occurrenceEntry, inserted] { occurrence.try_emplace (name, 0) };
                auto& [occurrenceName, tokenOccurrence] { *occurrenceEntry };
                auto* markerCommentTable { name == Id::comment
                                               ? getCommentTable (model, templateDocument, *rows.first(),
                                                     *lines.first())
                                               : commentTable };
                auto value { name == Id::list
                                 ? getFill (model, templateDocument, tables, rows, lines,
                                       tokenOccurrence, joinText, parentIndent, isAtColumnZero, extension)
                                 : getTokenValue (model, templateDocument, tables, *rows.first(),
                                       *lines.first(), markerCommentTable, name, joinText, parentIndent,
                                       isAtColumnZero, extension) };

                if (name == Id::comment and value.isNotEmpty())
                    value = templateLine.trim() == marker
                                ? Transforms::toCommentBlock (value, extension)
                                : Transforms::toComment (value, extension);

                const auto interior { marker.substring (
                    Id::tripleColon.length(), marker.length() - Id::tripleColon.length()) };

                lineText = jam::Format::replaceholder (lineText, interior, value);
                ++tokenOccurrence;
            }
        }

        return lineText;
    }

    static juce::String getLines (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<Element*>& tables, const jam::Array<Element*>& rows,
        const jam::Array<Element*>& lines, Element* commentTable, Element& line,
        const jam::Document::Identifiers& tokens, const juce::String& joinText, int parentIndent,
        const juce::String& extension)
    {
        jam::Strings textLines;
        auto previousLineIsEmpty { false };
        jam::HashMap<juce::Identifier, int> occurrence;

        for (const auto& templateLine : jam::Strings::fromLines (
                 *templateDocument.getCodeBlock (line)->get<juce::String> (Id::value)))
        {
            const auto lineText { getSubstitutedLine (model, templateDocument, tables, rows, lines,
                commentTable, tokens, occurrence, templateLine, joinText, parentIndent, extension) };
            const auto lineHasPlaceholder { Items::getMarkers (templateLine).size() > 0 };
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
     * @brief Renders @p line's own shape through the array overload, from
     *        @p row and @p line alone.
     *
     * @param model            The model @p row and @p line belong to.
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param tables           The tables searched when a nested @c list
     *                         token expands.
     * @param row              The row @p line belongs to.
     * @param line             The paragraph structure line to render.
     * @param joinText         The join text passed through to getShape().
     * @param parentIndent     The parent shape's own indent, subtracted
     *                         from @p line's computed indent.
     * @param isAtColumnZero   Whether the enclosing @c list marker began
     *                         at column zero.
     * @param extension        The target file extension a comment value is
     *                         commented for.
     * @returns The rendered, indented shape text.
     */
    static juce::String getShapeText (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<Element*>& tables, Element& row, Element& line, const juce::String& joinText,
        int parentIndent, bool isAtColumnZero, const juce::String& extension)
    {
        jam::Array<Element*> rows;
        jam::Array<Element*> sourceLines;
        rows.add (&row);
        sourceLines.add (&line);

        return getShapeText (model, templateDocument, tables, rows, sourceLines, joinText, parentIndent,
            isAtColumnZero, extension);
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
     * @brief Renders each of @p sourceLines' own list-item source through
     *        Items::getJoinedItems(), deduplicating byte-identical
     *        renders across @p rows, joins the distinct renders by
     *        @p joinText, then indents the result to @p sourceLines' own
     *        structure depth.
     *
     * @param model            The model @p tables and @p rows belong to.
     * @param templateDocument The template document each item's shape is
     *                         read from.
     * @param tables           The tables searched when a nested @c list
     *                         token expands.
     * @param rows             The rows corresponding, index by index, to
     *                         @p sourceLines.
     * @param sourceLines      The list-item structure lines to render,
     *                         sharing one structure depth.
     * @param joinText         The join text between the distinct rendered
     *                         items.
     * @param parentIndent     The parent shape's own indent, subtracted
     *                         from @p sourceLines' computed indent.
     * @param isAtColumnZero   Whether the enclosing @c list marker began
     *                         at column zero.
     * @param extension        The target file extension a comment value is
     *                         commented for.
     * @returns The rendered, indented item-group text.
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
            const auto& shapeTokens { *templateDocument.getCodeBlock (sourceLine)
                                            ->get<jam::Document::Identifiers> (Id::placeholder) };
            const auto shapeHasListToken {
                std::find (shapeTokens.begin(), shapeTokens.end(), Id::list) != shapeTokens.end()
            };
            auto listMarkerIsInline { false };

            if (shapeHasListToken)
            {
                const auto& shapeValue {
                    *templateDocument.getCodeBlock (sourceLine)->get<juce::String> (Id::value)
                };
                const auto marker { Items::getMarker (shapeValue, Id::list) };

                for (const auto& shapeLine : jam::Strings::fromLines (shapeValue))
                {
                    const auto markerPosition { shapeLine.indexOf (marker) };

                    if (markerPosition >= 0)
                    {
                        listMarkerIsInline = markerPosition != 0;
                        break;
                    }
                }
            }

            auto* separatorLine { model.getSeparator (row, indent, ordinal) };
            const auto value { separatorLine != nullptr
                                    ? templateDocument.getValue (
                                          model, row, *separatorLine->get<juce::String> (Id::value))
                                    : juce::String() };
            const auto join { not listMarkerIsInline and value.isNotEmpty()
                                   ? value
                                   : juce::String::charToString (Chars::newline) };
            const auto childJoin { value };
            const auto itemText { Items::getJoinedItems (model, templateDocument, tables, row,
                sourceValue, sourceLine, join, indent, ordinal, childJoin, extension) };

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
            const auto arity { Items::getArity (templateDocument, *structureLine) };
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
            const auto& tokens { *templateDocument.getCodeBlock (line)
                                       ->get<jam::Document::Identifiers> (Id::placeholder) };
            jam::Strings key;
            key.add (*line.get<juce::String> (Id::templatePath));
            key.add (*line.get<juce::String> (Id::info));

            for (const auto& name : tokens)
                if (name != Id::list)
                    key.add (getTokenValue (model, templateDocument, tables, row, line, commentTable,
                        name, joinText, parentIndent, false, extension));

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
                const auto& tokens { *templateDocument.getCodeBlock (*groupLines.first())
                                           ->get<jam::Document::Identifiers> (Id::placeholder) };

                groupTexts.add (getLines (model, templateDocument, tables, groupRows, groupLines,
                    commentTable, *groupLines.first(), tokens, joinText, parentIndent, extension));
            }

        return groupTexts.joinIntoString (joinText, 0, -1);
    }
};
