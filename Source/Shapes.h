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
 * with. Description resolution walks a shape's own list sources in authored
 * order to the first one addressing a table, falling back to the row's
 * own table when none does; each marker substitutes its first remaining
 * occurrence per template line; a shape's @c list marker is inline when
 * it is authored anywhere but column zero of its own template line.
 */
struct Shapes
{
    /** Model's own element type, brought into Shapes's own scope. */
    using Element = Model::Element;

    /** The number of spaces one structure depth indents. */
    static constexpr int indentWidth { 4 };

    /**
     * @brief Returns the next structure line after @p line, skipping past
     *        every list source @p line's own shape consumes when @p line
     *        is a shape paragraph, through Model::getNextShapeLine().
     *
     * @param model            The model @p line belongs to.
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param line             The structure line to advance past.
     * @returns The next structure line, or @c nullptr when none remains.
     */
    static const Element*
    getLineAfter (const Model& model, const TemplateDocument& templateDocument, const Element& line)
    {
        return model.getNextShapeLine (line,
            [&templateDocument] (const Element& candidate) { return Items::getArity (templateDocument, candidate); });
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
    static const Element* getSourceLine (const Model& model, const TemplateDocument& templateDocument,
        const Element& line, int occurrence)
    {
        const Element* cursor { model.getNextLine (line) };

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
    static const Element* getFirstLine (const Model& model, const Element& row)
    {
        static const auto listMarker { Model::getReservedName (Id::list) };

        const Element* line { nullptr };

        model.getTableCell (row, Id::structure)
            ->applyFunctionRecursively (
                [&line] (const Element& candidate) -> bool
                {
                    if (line == nullptr and candidate.contains (Id::shape)
                        and *candidate.get<int> (Id::shape) == 0
                        and (candidate.isTag (Id::p) or candidate.id == listMarker))
                        line = &candidate;

                    return line == nullptr;
                });

        return line;
    }

    /**
     * @brief Resolves @p row's own @c \[begin\] or @c \[end\] region
     *        delimiter value through @p templateDocument.
     *
     * @pre @p row carries the @p word binding -- established once by
     *      Validator::isRegionPaired() before any reader runs.
     *
     * @param model            The model @p row belongs to.
     * @param templateDocument The template document the binding's value is
     *                         resolved through.
     * @param row              The region row whose delimiter is resolved.
     * @param word             The reserved word that names the delimiter,
     *                         @c Id::begin or @c Id::end.
     * @returns The resolved delimiter text.
     */
    static juce::String getRegionValue (const Model& model, const TemplateDocument& templateDocument,
        const Element& row, const juce::Identifier& word)
    {
        auto* binding { model.getBinding (
            row, Id::structure, *getFirstLine (model, row), Model::getReservedName (word)) };

        return templateDocument.getValue (model, row, *binding->get<juce::String> (Id::value));
    }

    /**
     * @brief Resolves @p line's own first list source addressing a table,
     *        walking @p line's own arity of sources in authored order.
     *
     * @param model            The model @p row and @p line belong to.
     * @param templateDocument The template document @p line's own arity
     *                         is read from.
     * @param row              The row @p line's sources are addressed
     *                         against.
     * @param line             The structure line whose sources are
     *                         walked.
     * @returns The first source's own addressed table, or @c nullptr when
     *          none of @p line's sources address one.
     */
    static const Element* getSourceTable (const Model& model, const TemplateDocument& templateDocument,
        const Element& row, const Element& line)
    {
        const Element* table { nullptr };

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

        return table;
    }

    /**
     * @brief Resolves @p line's description table -- a list item's own
     *        addressed table, a paragraph's explicitly authored
     *        @-description address, or, absent one, the first table addressed
     *        by @p line's list sources in authored order, falling back to
     *        @p row's own table when none of its sources address one.
     *
     * @param model            The model @p row and @p line belong to.
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param row              The row @p line's sources are addressed
     *                         against.
     * @param line             The structure line whose description table is
     *                         resolved.
     * @returns The resolved description table or code block, @p row's own
     *          table when no source resolves one but it carries a
     *          description, or @c nullptr when neither resolves.
     */
    static const Element* getDescriptionTable (const Model& model, const TemplateDocument& templateDocument,
        const Element& row, const Element& line)
    {
        const Element* table { nullptr };

        if (not line.isTag (Id::p))
        {
            if (line.contains (Id::line))
                table = model.getTable (row,
                    *model.getSource (row, *line.get<int> (Id::level), *line.get<int> (Id::line))
                         ->get<juce::String> (Id::value));
        }
        else if (auto* description { model.getDescription (row, *line.get<int> (Id::level), *line.get<int> (Id::line)) })
        {
            const auto& value { *description->get<juce::String> (Id::value) };

            table = model.getTable (row, value);

            if (table == nullptr)
                return model.getCodeBlock (juce::Identifier (jam::Format::getPostColon (value).trim()));
        }
        else
        {
            table = getSourceTable (model, templateDocument, row, line);
        }

        if (table != nullptr)
            return table;

        if (row.parent->get<juce::String> (Id::description)->isNotEmpty())
            return row.parent;

        return nullptr;
    }

    /**
     * @brief Returns @p line's own item-shape source row -- the filtered
     *        or plain table row @p line's own paired list source
     *        addresses, the same resolution Items::getItem() reads for a
     *        nested item-shape -- for an item shape, on
     *        the source row.
     *
     * @param model The model @p row and @p line belong to.
     * @param row   The row @p line's own list source is addressed against.
     * @param line  The item-shape list bullet whose source row is resolved.
     * @returns @p line's own resolved source row, or @c nullptr when its
     *          own paired list source addresses no table row -- an
     *          under-supplied filter elides rather than failing.
     */
    static const Element* getItemSourceRow (const Model& model, const Element& row, const Element& line)
    {
        auto* source { model.getSource (row, *line.get<int> (Id::level), *line.get<int> (Id::line)) };
        const auto& sourceValue { *source->get<juce::String> (Id::value) };
        auto* sourceTable { model.getTable (row, sourceValue) };
        auto sourceRows { Items::getTableSourceRows (model, row, *sourceTable, sourceValue) };

        return sourceRows.isEmpty() ? nullptr : sourceRows.at (0);
    }

    /**
     * @brief Resolves @p name's value for @p line, in a declared order:
     *        for the name @c description, @p descriptionTable's own
     *        documentation -- the shape-level channel -- a list-column
     *        description reference or table documentation, never a
     *        structure-column binding), short-circuiting before any other
     *        rung; for every other name, in order, a binding of that
     *        name, then @p line's own maps' @p name row, then, for @p line
     *        an item shape, its own source row's @p name column, or,
     *        otherwise, @p row's own @p name column.
     *
     * @param model            The model @p row and @p line belong to.
     * @param templateDocument The template document each map value is
     *                         read through.
     * @param tables           The tables searched when a shape-valued
     *                         binding's own nested @c list token expands.
     * @param row              The row @p name's binding and column are
     *                         resolved against.
     * @param line             The structure line whose binding and maps
     *                         are searched.
     * @param descriptionTable The table @p name's documentation is read
     *                         from, when @p name is @c description.
     * @param name             The token or column name to resolve.
     * @param joinText         The join text passed through to
     *                         getShapeText() when @p name binds to a
     *                         shape-valued binding.
     * @param parentIndent     The parent shape's own indent, passed
     *                         through to getShapeText().
     * @param isAtColumnZero   Whether the enclosing @c list marker began
     *                         at column zero, passed through to
     *                         getShapeText().
     * @param extension        The target file extension a description value
     *                         is commented for.
     * @returns @p name's resolved value, or an empty string when none of
     *          the rungs names it -- including when @p line is an item
     *          shape whose own paired list source elides for
     *          under-supply, addressing no row.
     */
    static juce::String getTokenValue (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<const Element*>& tables, const Element& row, const Element& line, const Element* descriptionTable,
        const juce::Identifier& name, const juce::String& joinText, int parentIndent,
        bool isAtColumnZero, const juce::String& extension)
    {
        static const auto descriptionMarker { Model::getReservedName (Id::description) };
        static const auto listMarker { Model::getReservedName (Id::list) };

        if (name == descriptionMarker)
            return descriptionTable != nullptr ? *descriptionTable->get<juce::String> (Id::description)
                                           : juce::String{};

        if (auto* binding { model.getBinding (row, Id::structure, line, name) })
        {
            if (binding->contains (Id::templatePath))
                return getShapeText (model, templateDocument, tables, row, *binding, joinText,
                    parentIndent, isAtColumnZero, extension);

            return templateDocument.getValue (model, row, *binding->get<juce::String> (Id::value));
        }

        if (auto* cell { model.getMapCell (row, line, name) })
            return *cell->get<juce::String> (Id::value);

        auto* columnRow { line.id == listMarker ? getItemSourceRow (model, row, line) : &row };

        if (columnRow == nullptr)
            return {};

        if (auto* cell { model.getTableCell (*columnRow, name) })
        {
            const auto& columnValue { *cell->get<juce::String> (Id::value) };
            return name == Id::file ? model.getFileName (columnValue) : columnValue;
        }

        return {};
    }

    /**
     * @brief Resolves @p name's own marker value -- getFill()'s own filled
     *        occurrence for the @c list token, getTokenValue()'s own
     *        resolved value for every other token -- then comments a
     *        non-empty @c description value as a block comment when
     *        @p templateLine's own trimmed text equals @p marker, or an
     *        inline comment otherwise.
     *
     * @param model            The model @p rows and @p lines belong to.
     * @param templateDocument The template document each shape and its
     *                         placeholder tokens are read from.
     * @param tables           The tables searched when the @c list token
     *                         expands.
     * @param rows             The rows corresponding, index by index, to
     *                         @p lines.
     * @param lines            The structure lines corresponding, index by
     *                         index, to @p rows.
     * @param descriptionTable The table @p name's documentation is read
     *                         from, when @p name is @c description.
     * @param name             The token name to resolve.
     * @param tokenOccurrence  @p name's own substitution count so far,
     *                         passed through to getFill().
     * @param joinText         The join text passed through to the
     *                         @c list token's expansion.
     * @param parentIndent     The parent shape's own indent, passed
     *                         through to the @c list token's expansion.
     * @param isAtColumnZero   Whether @p marker began at column zero in
     *                         @p templateLine.
     * @param extension        The target file extension a description value
     *                         is commented for.
     * @param templateLine     @p marker's own authored template line.
     * @param marker           @p name's own verbatim marker as authored
     *                         in @p templateLine.
     * @returns @p name's resolved, commented value.
     */
    static juce::String getMarkerValue (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<const Element*>& tables, const jam::Array<const Element*>& rows,
        const jam::Array<const Element*>& lines, const Element* descriptionTable, const juce::Identifier& name,
        int tokenOccurrence, const juce::String& joinText, int parentIndent, bool isAtColumnZero,
        const juce::String& extension, const juce::String& templateLine, const juce::String& marker)
    {
        static const auto listMarker { Model::getReservedName (Id::list) };
        static const auto descriptionMarker { Model::getReservedName (Id::description) };

        auto value { name == listMarker
                         ? getFill (model, templateDocument, tables, rows, lines,
                               tokenOccurrence, joinText, parentIndent, isAtColumnZero, extension)
                         : getTokenValue (model, templateDocument, tables, *rows.first(),
                               *lines.first(), descriptionTable, name, joinText, parentIndent,
                               isAtColumnZero, extension) };

        if (name == descriptionMarker and value.isNotEmpty())
            value = templateLine.trim().compare (marker) == 0
                        ? Transforms::toCommentBlock (value, extension)
                        : Transforms::toComment (value, extension);

        return value;
    }

    /**
     * @brief Substitutes each of @p tokens' first remaining marker in
     *        @p templateLine with its resolved value -- the @c list token
     *        filled by getFill(), every other token resolved through
     *        getTokenValue() -- wrapping the @c description token's value as a
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
     * @param extension        The target file extension a description value is
     *                         commented for.
     * @returns The substituted line text.
     */
    static juce::String getSubstitutedLine (const Model& model,
        const TemplateDocument& templateDocument, const jam::Array<const Element*>& tables,
        const jam::Array<const Element*>& rows, const jam::Array<const Element*>& lines,
        const jam::Document::Identifiers& tokens, jam::HashMap<juce::Identifier, int>& occurrence,
        const juce::String& templateLine, const juce::String& joinText, int parentIndent,
        const juce::String& extension)
    {
        static const auto listMarker { Model::getReservedName (Id::list) };

        auto lineText { templateLine };
        auto* descriptionTable { getDescriptionTable (model, templateDocument, *rows.first(), *lines.first()) };

        for (const auto& name : tokens)
        {
            const auto marker { Items::getMarker (lineText, name) };

            if (marker.isNotEmpty())
            {
                auto [occurrenceEntry, inserted] { occurrence.try_emplace (name, 0) };
                auto& [occurrenceName, tokenOccurrence] { *occurrenceEntry };

                if (name == listMarker)
                {
                    for (auto position { lineText.indexOf (marker) }; position >= 0;)
                    {
                        const auto isAtColumnZero { position == 0 and templateLine.indexOf (marker) == 0 };
                        const auto value { getMarkerValue (model, templateDocument, tables, rows, lines,
                            descriptionTable, name, tokenOccurrence, joinText, parentIndent, isAtColumnZero,
                            extension, templateLine, marker) };
                        lineText = lineText.replaceSection (position, marker.length(), value);
                        ++tokenOccurrence;
                        position = lineText.indexOf (position + value.length(), marker);
                    }
                }
                else
                {
                    const auto isAtColumnZero { templateLine.indexOf (marker) == 0 };
                    const auto value { getMarkerValue (model, templateDocument, tables, rows, lines,
                        descriptionTable, name, tokenOccurrence, joinText, parentIndent, isAtColumnZero,
                        extension, templateLine, marker) };
                    lineText = jam::Format::replaceholder (lineText, marker.substring (
                        Id::tripleColon.length(), marker.length() - Id::tripleColon.length()), value);
                    ++tokenOccurrence;
                }
            }
        }

        return lineText;
    }

    /**
     * @brief Renders @p line's own shape, line by line, through
     *        getSubstitutedLine(), collapsing every run of lines a
     *        substituted placeholder left empty down to at most one
     *        blank line.
     *
     * @param model            The model @p rows and @p lines belong to.
     * @param templateDocument The template document @p line's shape and
     *                         placeholder tokens are read from.
     * @param tables           The tables searched when the @c list token
     *                         expands.
     * @param rows             The rows corresponding, index by index, to
     *                         @p lines.
     * @param lines            The structure lines corresponding, index by
     *                         index, to @p rows.
     * @param line             The structure line whose shape's own
     *                         template lines are rendered.
     * @param tokens           The shape's placeholder tokens to
     *                         substitute.
     * @param joinText         The join text passed through to the
     *                         @c list token's expansion.
     * @param parentIndent     The parent shape's own indent, passed
     *                         through to the @c list token's expansion.
     * @param extension        The target file extension a description value
     *                         is commented for.
     * @returns @p line's own shape's rendered text, joined by newline.
     */
    static juce::String getLines (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<const Element*>& tables, const jam::Array<const Element*>& rows,
        const jam::Array<const Element*>& lines, const Element& line,
        const jam::Document::Identifiers& tokens, const juce::String& joinText, int parentIndent,
        const juce::String& extension)
    {
        static const auto newlineText { juce::String::charToString (Chars::newline) };

        jam::Strings textLines;
        auto previousLineIsEmpty { false };
        jam::HashMap<juce::Identifier, int> occurrence;

        for (const auto& templateLine : jam::Strings::fromLines (
                 *templateDocument.getCodeBlock (line)->get<juce::String> (Id::value)))
        {
            const auto lineText { getSubstitutedLine (model, templateDocument, tables, rows, lines,
                tokens, occurrence, templateLine, joinText, parentIndent, extension) };
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

        return textLines.joinIntoString (newlineText, 0, -1);
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
        static const auto spaceText { juce::String::charToString (Chars::space) };
        static const auto newlineText { juce::String::charToString (Chars::newline) };

        auto indentedText { text };

        if (isAtColumnZero)
        {
            const auto indent { level * indentWidth - parentIndent };

            if (indent != 0)
            {
                const auto prefix { juce::String::repeatedString (spaceText, indent) };
                jam::Strings prefixedLines;

                for (const auto& textLine : jam::Strings::fromLines (text))
                    prefixedLines.add (textLine.isNotEmpty() ? prefix + textLine : textLine);

                indentedText = prefixedLines.joinIntoString (newlineText, 0, -1);
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
     * @param extension        The target file extension a description value is
     *                         commented for.
     * @returns The rendered, indented shape text.
     */
    static juce::String getShapeText (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<const Element*>& tables, const Element& row, const Element& line, const juce::String& joinText,
        int parentIndent, bool isAtColumnZero, const juce::String& extension)
    {
        jam::Array<const Element*> rows;
        jam::Array<const Element*> sourceLines;
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
     * @param extension        The target file extension a description value is
     *                         commented for.
     * @returns The rendered, indented shape group text.
     */
    static juce::String getShapeText (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<const Element*>& tables, const jam::Array<const Element*>& rows,
        const jam::Array<const Element*>& sourceLines, const juce::String& joinText, int parentIndent,
        bool isAtColumnZero, const juce::String& extension)
    {
        const auto text { getShape (model, templateDocument, tables, rows, sourceLines, joinText,
            *sourceLines.first()->get<int> (Id::level) * indentWidth, extension) };

        return getIndentedText (
            text, *sourceLines.first()->get<int> (Id::level), parentIndent, isAtColumnZero);
    }

    /**
     * @brief Answers whether @p sourceLine's own shape's @c list marker is
     *        authored anywhere but column zero of its own template line.
     *
     * @param templateDocument The template document @p sourceLine's shape
     *                         is read from.
     * @param sourceLine       The structure line whose shape's @c list
     *                         marker is checked.
     * @returns @c true when @p sourceLine's shape declares a @c list
     *          marker not at column zero, @c false when it declares one
     *          at column zero or declares none.
     */
    static bool isListMarkerInline (const TemplateDocument& templateDocument, const Element& sourceLine)
    {
        static const auto listMarker { Model::getReservedName (Id::list) };

        const auto& shapeTokens { *templateDocument.getCodeBlock (sourceLine)
                                        ->get<jam::Document::Identifiers> (Id::placeholder) };

        if (std::find (shapeTokens.begin(), shapeTokens.end(), listMarker) == shapeTokens.end())
            return false;

        const auto& shapeValue {
            *templateDocument.getCodeBlock (sourceLine)->get<juce::String> (Id::value)
        };
        const auto marker { Items::getMarker (shapeValue, listMarker) };

        for (const auto& shapeLine : jam::Strings::fromLines (shapeValue))
        {
            const auto markerPosition { shapeLine.indexOf (marker) };

            if (markerPosition >= 0)
                return markerPosition != 0;
        }

        return false;
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
     * @param shapeLines       The structure lines whose own @c shape
     *                         ordinals select the map tables, one per
     *                         @p sourceLines entry.
     * @param joinText         The join text between the distinct rendered
     *                         items.
     * @param parentIndent     The parent shape's own indent, subtracted
     *                         from @p sourceLines' computed indent.
     * @param isAtColumnZero   Whether the enclosing @c list marker began
     *                         at column zero.
     * @param extension        The target file extension a description value is
     *                         commented for.
     * @returns The rendered, indented item-group text.
     */
    static juce::String getItemText (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<const Element*>& tables, const jam::Array<const Element*>& rows,
        const jam::Array<const Element*>& sourceLines, const jam::Array<const Element*>& shapeLines,
        const juce::String& joinText, int parentIndent, bool isAtColumnZero, const juce::String& extension)
    {
        static const auto newlineText { juce::String::charToString (Chars::newline) };

        jam::Strings itemTexts;

        for (int index { 0 }; index < rows.size(); ++index)
        {
            const auto& row { *rows.at (index) };
            const auto& sourceLine { *sourceLines.at (index) };
            const auto indent { *sourceLine.get<int> (Id::level) };
            const auto ordinal { *sourceLine.get<int> (Id::line) };
            const auto sourceValue {
                *model.getSource (row, indent, ordinal)->get<juce::String> (Id::value)
            };
            const auto listMarkerIsInline { isListMarkerInline (templateDocument, sourceLine) };
            auto* separatorLine { model.getSeparator (row, indent, ordinal) };
            const auto value { separatorLine != nullptr
                                    ? templateDocument.getValue (
                                          model, row, *separatorLine->get<juce::String> (Id::value))
                                    : juce::String{} };
            const auto join { not listMarkerIsInline and value.isNotEmpty()
                                   ? value
                                   : newlineText };
            const auto childJoin { value };
            const auto itemText { Items::getJoinedItems (model, templateDocument, tables, row,
                sourceValue, sourceLine, *shapeLines.at (index), join, indent, ordinal, childJoin, extension) };

            if (not itemTexts.contains (itemText, false))
                itemTexts.add (itemText);
        }

        const auto text { itemTexts.joinIntoString (joinText, 0, -1) };

        return getIndentedText (
            text, *sourceLines.first()->get<int> (Id::level), parentIndent, isAtColumnZero);
    }

    /**
     * @brief Counts how many of @p structureLine's own arity of source
     *        lines actually exist, walking getSourceLine() up to @p arity.
     *
     * @param model            The model @p structureLine belongs to.
     * @param templateDocument The template document each walked line's
     *                         shape is read from.
     * @param structureLine    The structure line whose available sources
     *                         are counted.
     * @param arity            The upper bound on sources to count.
     * @returns The number of @p structureLine's own sources found, up to
     *          @p arity.
     */
    static int getAvailableCount (const Model& model, const TemplateDocument& templateDocument,
        const Element& structureLine, int arity)
    {
        auto availableCount { 0 };

        while (availableCount < arity
               and getSourceLine (model, templateDocument, structureLine, availableCount) != nullptr)
            ++availableCount;

        return availableCount;
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
     * @param extension        The target file extension a description value is
     *                         commented for.
     * @returns The rendered text for @p occurrence, paragraph sources
     *          followed by item sources, joined by @p joinText.
     */
    static juce::String getFill (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<const Element*>& tables, const jam::Array<const Element*>& rows,
        const jam::Array<const Element*>& lines, int occurrence, const juce::String& joinText,
        int parentIndent, bool isAtColumnZero, const juce::String& extension)
    {
        jam::Array<const Element*> shapeRows;
        jam::Array<const Element*> shapeSourceLines;
        jam::Array<const Element*> itemRows;
        jam::Array<const Element*> itemSourceLines;
        jam::Array<const Element*> itemShapeLines;

        for (int index { 0 }; index < rows.size(); ++index)
        {
            auto* structureLine { lines.at (index) };
            const auto arity { Items::getArity (templateDocument, *structureLine) };
            const auto availableCount { getAvailableCount (model, templateDocument, *structureLine, arity) };
            const auto skippedCount { arity - availableCount };

            if (occurrence >= skippedCount)
            {
                auto* sourceLine { getSourceLine (
                    model, templateDocument, *structureLine, occurrence - skippedCount) };
                jassert (sourceLine != nullptr);

                if (sourceLine->isTag (Id::p))
                {
                    shapeRows.add (rows.at (index));
                    shapeSourceLines.add (sourceLine);
                }
                else
                {
                    itemRows.add (rows.at (index));
                    itemSourceLines.add (sourceLine);
                    itemShapeLines.add (structureLine);
                }
            }
        }

        jam::Strings texts;

        if (not shapeRows.isEmpty())
            texts.add (getShapeText (model, templateDocument, tables, shapeRows, shapeSourceLines,
                joinText, parentIndent, isAtColumnZero, extension));

        if (not itemRows.isEmpty())
            texts.add (getItemText (model, templateDocument, tables, itemRows, itemSourceLines,
                itemShapeLines, joinText, parentIndent, isAtColumnZero, extension));

        return texts.joinIntoString (joinText, 0, -1);
    }

    /**
     * @brief Returns @p line's own grouping key -- its template path, info,
     *        and every non-@c list token's own resolved value, joined by
     *        newline -- the key getShape() groups rows by before
     *        rendering.
     *
     * @param model            The model @p row and @p line belong to.
     * @param templateDocument The template document @p line's shape and
     *                         placeholder tokens are read from.
     * @param tables           The tables searched when a nested @c list
     *                         token expands.
     * @param row              The row @p line's tokens are resolved
     *                         against.
     * @param line             The structure line whose grouping key is
     *                         built.
     * @param joinText         The join text passed through to a
     *                         shape-valued token's own expansion.
     * @param parentIndent     The parent shape's own indent, passed
     *                         through to a shape-valued token's own
     *                         expansion.
     * @param extension        The target file extension a description value
     *                         is commented for.
     * @returns @p line's own grouping key.
     */
    static juce::String getGroupKey (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<const Element*>& tables, const Element& row, const Element& line, const juce::String& joinText,
        int parentIndent, const juce::String& extension)
    {
        static const auto newlineText { juce::String::charToString (Chars::newline) };
        static const auto listMarker { Model::getReservedName (Id::list) };

        auto* descriptionTable { getDescriptionTable (model, templateDocument, row, line) };
        const auto& tokens { *templateDocument.getCodeBlock (line)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };
        jam::Strings key;
        key.add (*line.get<juce::String> (Id::templatePath));
        key.add (*line.get<juce::String> (Id::info));

        for (const auto& name : tokens)
            if (name != listMarker)
                key.add (getTokenValue (model, templateDocument, tables, row, line, descriptionTable,
                    name, joinText, parentIndent, false, extension));

        return key.joinIntoString (newlineText, 0, -1);
    }

    /**
     * @brief Renders one grouping key's own rows and lines, selected by
     *        @p indices out of @p rows and @p lines, through getLines().
     *
     * @param model            The model @p tables, @p rows, and @p lines
     *                         belong to.
     * @param templateDocument The template document the group's own shape
     *                         and placeholder tokens are read from.
     * @param tables           The tables searched when a nested @c list
     *                         token expands.
     * @param rows             Every row getShape() is grouping.
     * @param lines            The structure lines corresponding, index by
     *                         index, to @p rows.
     * @param indices          The group's own indices into @p rows and
     *                         @p lines.
     * @param joinText         The join text passed through to the
     *                         @c list token's expansion.
     * @param parentIndent     The parent shape's own indent, passed
     *                         through to getLines().
     * @param extension        The target file extension a description value
     *                         is commented for.
     * @returns The group's own rendered text.
     */
    static juce::String getGroupText (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<const Element*>& tables, const jam::Array<const Element*>& rows,
        const jam::Array<const Element*>& lines, const jam::Array<int>& indices, const juce::String& joinText,
        int parentIndent, const juce::String& extension)
    {
        jam::Array<const Element*> groupRows;
        jam::Array<const Element*> groupLines;

        for (const auto candidateIndex : indices)
        {
            groupRows.add (rows.at (candidateIndex));
            groupLines.add (lines.at (candidateIndex));
        }

        const auto& tokens { *templateDocument.getCodeBlock (*groupLines.first())
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };

        return getLines (model, templateDocument, tables, groupRows, groupLines,
            *groupLines.first(), tokens, joinText, parentIndent, extension);
    }

    /**
     * @brief Groups @p rows' own lines by their shared template path,
     *        info, and non-@c list token values, renders each group once
     *        through getLines(), and joins every group's own rendered
     *        text by @p joinText.
     *
     * @param model            The model @p tables and @p rows belong to.
     * @param templateDocument The template document each line's shape is
     *                         read from.
     * @param tables           The tables searched when a nested @c list
     *                         token expands.
     * @param rows             The rows corresponding, index by index, to
     *                         @p lines.
     * @param lines            The structure lines corresponding, index by
     *                         index, to @p rows.
     * @param joinText         The join text between each group's own
     *                         rendered text.
     * @param parentIndent     The parent shape's own indent, passed
     *                         through to getLines().
     * @param extension        The target file extension a description value
     *                         is commented for.
     * @returns Every group's own rendered text, in first-occurrence
     *          order, joined by @p joinText.
     */
    static juce::String getShape (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<const Element*>& tables, const jam::Array<const Element*>& rows,
        const jam::Array<const Element*>& lines, const juce::String& joinText, int parentIndent,
        const juce::String& extension)
    {
        jam::Strings keys;

        for (int index { 0 }; index < rows.size(); ++index)
            keys.add (getGroupKey (model, templateDocument, tables, *rows.at (index), *lines.at (index),
                joinText, parentIndent, extension));

        jam::HashMap<juce::String, jam::Array<int>> groupIndices;

        for (int index { 0 }; index < rows.size(); ++index)
            groupIndices[keys.at (index)].add (index);

        jam::Strings groupTexts;

        for (const auto& [key, indices] : groupIndices)
            groupTexts.add (getGroupText (model, templateDocument, tables, rows, lines, indices,
                joinText, parentIndent, extension));

        return groupTexts.joinIntoString (joinText, 0, -1);
    }
};
