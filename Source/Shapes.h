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
     * @brief Returns the @p occurrence-th source line following @p line.
     *        The walk starts at Items::getNextSourceLine(), then advances
     *        through Items::getNextShapeLine() one full source at a time.
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
        const Element& line, int occurrence);

    /**
     * @brief Returns @p row's structure scope's first shape line -- the
     *        paragraph or list item whose @c shape ordinal is zero.
     *
     * @param model The model @p row belongs to.
     * @param row   The row whose structure scope is searched.
     * @returns @p row's first shape line, or @c nullptr when its
     *          structure scope declares none.
     */
    static const Element* getFirstLine (const Model& model, const Element& row);

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
        const Element& row, const juce::Identifier& word);

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
        const Element& row, const Element& line);

    /**
     * @brief Resolves @p line's description table. A paragraph or a
     *        wrapper gives its paired description reference, or, absent
     *        one, the first table that its sources address. A list item
     *        gives its own addressed table.
     *        When no table resolves, the result falls back to @p row's own
     *        table.
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
        const Element& row, const Element& line);

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
    static const Element* getItemSourceRow (const Model& model, const Element& row, const Element& line);

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
        bool isAtColumnZero, const juce::String& extension);

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
        const juce::String& extension, const juce::String& templateLine, const juce::String& marker);

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
        const juce::String& extension);

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
        const juce::String& extension);

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
        bool isAtColumnZero);

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
        int parentIndent, bool isAtColumnZero, const juce::String& extension);

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
        bool isAtColumnZero, const juce::String& extension);

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
    static bool isListMarkerInline (const TemplateDocument& templateDocument, const Element& sourceLine);

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
        const juce::String& joinText, int parentIndent, bool isAtColumnZero, const juce::String& extension);

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
        const Element& structureLine, int arity);

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
        int parentIndent, bool isAtColumnZero, const juce::String& extension);

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
        int parentIndent, const juce::String& extension);

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
        int parentIndent, const juce::String& extension);

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
        const juce::String& extension);
};
