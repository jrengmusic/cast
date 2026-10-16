#pragma once
#include <JuceHeader.h>
#include "Items.h"
#include "Model.h"
#include "Shapes.h"
#include "TemplateDocument.h"

/**
 * @struct Validator
 * @brief Applies every manifest-level fatal to a parsed Model and
 *        TemplateDocument -- index integrity, uniqueness, formatting,
 *        structure, placeholders, and reference resolution.
 *
 * Validator extends jam::MarkdownValidator with cast's own manifest checks;
 * it owns no state and reports the first violation found, in check order.
 */
struct Validator : jam::MarkdownValidator
{
    /**
     * @brief Checks that @p line's own shape resolves to a code block in
     *        @p templateDocument.
     *
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param table            The table @p row belongs to, named in a
     *                         failure's location.
     * @param row              The row @p line belongs to, named in a
     *                         failure's location.
     * @param column           The column @p line was authored under,
     *                         named in a failure's location.
     * @param line             The structure line whose shape is checked.
     * @returns juce::Result::ok() when @p line's shape resolves, or a
     *          failure naming @p line's own info.
     */
    static juce::Result hasTemplate (const TemplateDocument& templateDocument, const Element& table,
        const Element& row, const juce::Identifier& column, const Element& line);

    /**
     * @brief Runs every manifest-level check against @p model and
     *        @p templateDocument, in order, stopping at the first failure.
     *
     * @param model            The parsed manifest and its data tables.
     * @param templateDocument The parsed template file the manifest wires
     *                         against.
     * @returns juce::Result::ok() when every check passes, or the first
     *          failing check's result.
     */
    static juce::Result isValid (const Model& model, const TemplateDocument& templateDocument);

    /**
     * @brief Invokes @p function once per bullet found by walking
     *        @p column's blockquote scopes, depth by depth, across every
     *        output table's rows.
     *
     * @param model    The model whose output tables are walked.
     * @param column   The column whose blockquote scopes carry the binding
     *                 lists to walk.
     * @param function Callable invoked as
     *                 @c function(table,row,bulletId,bulletValue) for each
     *                 bullet found, returning a juce::Result.
     * @returns juce::Result::ok() when @p function succeeds for every
     *          bullet, or the first failing invocation's result.
     */
    template <typename Function>
    static juce::Result
    forEachBinding (const Model& model, const juce::Identifier& column, Function&& function)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                    if (auto* scope { model.getTableCell (*row, column) })
                        if (const auto result { forEachBinding (*table, *row, *scope, function) };
                            not result.wasOk())
                            return result;

        return juce::Result::ok();
    }

    /**
     * @brief Invokes @p function once per bullet found by walking
     *        @p scope's blockquote lists, depth by depth, through the
     *        recursive overload.
     *
     * @param table    The table @p row belongs to, passed through to
     *                 @p function.
     * @param row      The row @p scope belongs to, passed through to
     *                 @p function.
     * @param scope    The blockquote scope walked.
     * @param function Callable invoked as
     *                 @c function(table,row,bulletId,bulletValue) for each
     *                 bullet found, returning a juce::Result.
     * @returns juce::Result::ok() when @p function succeeds for every
     *          bullet, or the first failing invocation's result.
     */
    template <typename Function>
    static juce::Result
    forEachBinding (const Element& table, const Element& row, const Element& scope, Function&& function)
    {
        for (auto* child : scope)
        {
            if (child->isTag (Id::ul))
                for (auto* item : *child)
                    if (const auto result { function (
                            table, row, item->id, *item->get<juce::String> (Id::value)) };
                        not result.wasOk())
                        return result;

            if (child->isTag (Id::blockquote))
                if (const auto result { forEachBinding (table, row, *child, function) };
                    not result.wasOk())
                    return result;
        }

        return juce::Result::ok();
    }

    /**
     * @brief Invokes @p function once per non-wiring cell across every
     *        non-index table's rows -- every cell but @c list,
     *        @c separator, and @c structure, the columns wiring an output
     *        row's own expansion.
     *
     * @param model    The model whose non-index tables are walked.
     * @param function Callable invoked as
     *                 @c function(table,row,columnId,rawText) for each
     *                 non-wiring cell, returning a juce::Result.
     * @returns juce::Result::ok() when @p function succeeds for every
     *          cell, or the first failing invocation's result.
     */
    template <typename Function>
    static juce::Result forEachEntry (const Model& model, Function&& function)
    {
        for (auto* table : model.getTables())
            if (not table->isTag (Id::index))
                for (auto* row : model.getTableRows (*table))
                    for (auto* cell : *row)
                        if (cell->id != Id::list and cell->id != Id::separator
                            and cell->id != Id::structure)
                        {
                            if (const auto result { function (*table, *row, cell->id,
                                    Model::getRawText (*cell, model.jam::Document::getSource())) };
                                not result.wasOk())
                                return result;
                        }

        return juce::Result::ok();
    }

    /**
     * @brief Checks that the manifest's index table exists and that every
     *        row's symbol names a file that exists on disk.
     *
     * @param model The model whose index table is checked.
     * @returns juce::Result::ok() when the index exists and every entry
     *          resolves to an existing file, or a failure naming the
     *          missing table, symbol, output, or template.
     */
    static juce::Result isIndex (const Model& model);

    /**
     * @brief Checks that every output row declares a non-empty structure
     *        scope resolving to a template file, and that every one of
     *        its lines carrying a template path resolves to a code
     *        block.
     *
     * @param model            The model whose output tables are checked.
     * @param templateDocument The template document each structure
     *                         line's shape is read from.
     * @returns juce::Result::ok() when every row's structure resolves, or
     *          a failure naming the missing structure or the missing
     *          template.
     */
    static juce::Result isStructure (const Model& model, const TemplateDocument& templateDocument);

    /**
     * @brief Checks that no two bindings under @p scope share the same
     *        name and the same owner, then checks the same for every nested
     *        blockquote scope.
     *
     * Two non-@c list bullets carrying @c shape are duplicates when they
     * share the same id and the same Model::getOwner().
     *
     * @param table  The table @p row belongs to, named in a failure's
     *               location.
     * @param row    The row @p scope belongs to, named in a failure's
     *               location.
     * @param column The column @p scope was read from, named in a
     *               failure's location.
     * @param scope  The blockquote scope whose bindings are checked.
     * @returns juce::Result::ok() when every scope's bindings are unique,
     *          or a failure naming the duplicate binding.
     */
    static juce::Result isBindingCountValid (const Element& table, const Element& row,
        const juce::Identifier& column, const Element& scope);

    /**
     * @brief Checks every output row's @c structure and @c separator
     *        scopes for duplicate binding names, through
     *        isBindingCountValid().
     *
     * @param model The model whose output tables are checked.
     * @returns juce::Result::ok() when every row's bindings are unique, or
     *          the first failing row's result.
     */
    static juce::Result isBindingCountValid (const Model& model);

    /**
     * @brief Checks that every shape line under @p structureScope
     *        resolves to a code block carrying an even count of
     *        @c ::: markers -- every marker opened is closed.
     *
     * @param templateDocument The template document each shape line's
     *                         code block is read from.
     * @param table            The table @p row belongs to, named in a
     *                         failure's location.
     * @param row              The row @p structureScope belongs to,
     *                         named in a failure's location.
     * @param structureScope   The structure scope whose shape lines are
     *                         checked.
     * @returns juce::Result::ok() when every shape's markers pair, or a
     *          failure naming the unterminated shape.
     */
    static juce::Result isMarkerCountValid (const TemplateDocument& templateDocument, const Element& table,
        const Element& row, const Element& structureScope);

    /**
     * @brief Checks every output row's structure scope for unterminated
     *        shape markers, through the recursive overload.
     *
     * @param model            The model whose output tables are checked.
     * @param templateDocument The template document each shape line's
     *                         code block is read from.
     * @returns juce::Result::ok() when every row's markers pair, or the
     *          first failing row's result.
     */
    static juce::Result
    isMarkerCountValid (const Model& model, const TemplateDocument& templateDocument);

    /**
     * @brief Checks that every bracketed marker interior in @p scope's
     *        shape code blocks is a reserved name -- @c \[list\],
     *        @c \[description\] or @c \[banner\].
     *
     * @param templateDocument The template document @p scope's shapes are
     *                         read from.
     * @param table            The table @p row belongs to, named in a
     *                         failure's location.
     * @param row              The row @p scope belongs to, named in a
     *                         failure's location.
     * @param column           The column @p scope was read from, named in a
     *                         failure's location.
     * @param scope            The blockquote scope walked.
     * @returns juce::Result::ok() when every bracketed interior is a
     *          reserved name, or a failure naming the first unknown one.
     */
    static juce::Result isReservedName (const TemplateDocument& templateDocument, const Element& table,
        const Element& row, const juce::Identifier& column, const Element& scope);

    /**
     * @brief Checks that every bracketed token in an output row's
     *        @c structure, @c separator and @c list shapes, and every
     *        bracket-form bullet name in those columns, is a reserved
     *        name -- @c \[list\], @c \[description\], @c \[begin\] or @c \[end\]
     *        for a bullet.
     *
     * @param model            The model whose output tables are checked.
     * @param templateDocument The template document each shape is read
     *                         from.
     * @returns juce::Result::ok() when every bracketed name is reserved,
     *          or the first failing entry's result.
     */
    static juce::Result isReservedName (const Model& model, const TemplateDocument& templateDocument);

    /**
     * @brief Checks that @p precedingShape, when it is a list-item shape
     *        addressing a column, resolves through hasTable().
     *
     * @param model          The model @p precedingShape's source is
     *                       resolved against.
     * @param table          The table @p row belongs to, named in a
     *                       failure's location.
     * @param row            The row @p precedingShape belongs to, named
     *                       in a failure's location.
     * @param column         The column @p precedingShape was authored
     *                       under, named in a failure's location.
     * @param precedingShape The most recently seen shape line, checked
     *                       when it is a list-item shape.
     * @returns juce::Result::ok() when @p precedingShape is a paragraph
     *          shape or its column-address source resolves, or a failure
     *          naming the unresolved reference.
     */
    static juce::Result isShapeSupplied (const Model& model, const Element& table, const Element& row,
        const juce::Identifier& column, const Element& precedingShape);

    /**
     * @brief Walks @p scope, checking every shape line's template through
     *        hasTemplate(), and, at each new shape line, the previously
     *        seen shape through isShapeSupplied(), then recurses into
     *        every nested blockquote.
     *
     * @param model            The model @p row belongs to.
     * @param templateDocument The template document each shape line's
     *                         code block is read from.
     * @param table            The table @p row belongs to, named in a
     *                         failure's location.
     * @param row              The row @p scope belongs to, named in a
     *                         failure's location.
     * @param column           The column @p scope was read from, named
     *                         in a failure's location.
     * @param scope            The blockquote scope walked.
     * @param precedingShape   The most recently seen shape line, or
     *                         @c nullptr when @p scope opens with none
     *                         seen yet, checked through isShapeSupplied()
     *                         at the next shape line.
     * @returns @p precedingShape's own successor -- the last shape line
     *          seen across @p scope and its nested blockquotes -- paired
     *          with juce::Result::ok() when every checked shape resolves,
     *          or the first failing check's result.
     */
    static std::pair<juce::Result, const Element*> isPlaceholderScope (const Model& model,
        const TemplateDocument& templateDocument, const Element& table, const Element& row,
        const juce::Identifier& column, const Element& scope, const Element* precedingShape);

    /**
     * @brief Checks every output row's @c structure and @c separator
     *        scopes for unresolved templates and unsupplied shapes,
     *        through isPlaceholderScope(), then checks a @c structure
     *        scope's own last shape line through isShapeSupplied().
     *
     * @param model            The model whose output tables are checked.
     * @param templateDocument The template document each shape line's
     *                         code block is read from.
     * @returns juce::Result::ok() when every row's placeholders resolve,
     *          or the first failing row's result.
     */
    static juce::Result
    isPlaceholders (const Model& model, const TemplateDocument& templateDocument);

    /**
     * @brief Checks that the render walk reaches every source line of each
     *        output row -- from the row's first line through
     *        Items::getNextShapeLine(), the walk the renderer consumes by.
     *        A source line left after the walk would render nowhere.
     *
     * @param model            The model whose output tables are checked.
     * @param templateDocument The template document each shape line's
     *                         arity is read from.
     * @returns juce::Result::ok() when every row's walk consumes all of
     *          its source lines, or a failure naming the first row that
     *          leaves one.
     */
    static juce::Result isSourceCountValid (const Model& model, const TemplateDocument& templateDocument);

    /**
     * @brief Checks that no output row's 2-or-more-slot item shape
     *        supplies more column addresses than its own arity demands.
     *
     * @param model            The model whose output tables are checked.
     * @param templateDocument The template document each shape line's
     *                         arity is read from.
     * @returns juce::Result::ok() when every item shape's supplied column
     *          addresses stay within its own arity, or a failure naming
     *          the ambiguous row.
     */
    static juce::Result isItemSourceCountValid (const Model& model, const TemplateDocument& templateDocument);

    /**
     * @brief Checks that every expansion @c list bullet under @p scope,
     *        when @p column is not the row's own @c list column, pairs
     *        with a bullet in @p row's own @c list column at its own
     *        (depth, ordinal) through Model::getSource(), and that every
     *        @-sigiled description bullet, when @p column is not @c structure,
     *        resolves through Model::getTable() -- the alias form or,
     *        absent an alias hit, the local form -- or names a code block
     *        carrying documentation. A description bullet whose value is not
     *        @-sigiled is plain prose, never a reference, and is
     *        exempt. Bullets carrying no @c Id::line stamp -- the row join
     *        and map bullets -- are exempt from the pairing check. A
     *        @c structure description bullet is exempt from this address check
     *        -- it is the file-documentation binding, validated instead by
     *        isReference() through hasTable().
     *
     * @param model  The model @p row belongs to.
     * @param table  The table @p row belongs to, named in a failure's
     *               location.
     * @param row    The row @p scope belongs to, named in a failure's
     *               location.
     * @param column The column @p scope was read from, named in a
     *               failure's location.
     * @param scope  The blockquote scope walked.
     * @returns juce::Result::ok() when every bullet pairs and resolves,
     *          or a failure naming the orphan bullet.
     */
    static juce::Result isPaired (const Model& model, const Element& table, const Element& row,
        const juce::Identifier& column, const Element& scope);

    /**
     * @brief Checks every output row's @c structure, @c separator, and
     *        @c list scopes, through the recursive overload.
     *
     * @param model The model whose output tables are checked.
     * @returns juce::Result::ok() when every row's bullets pair and
     *          resolve, or the first failing row's result.
     */
    static juce::Result isPaired (const Model& model);

    /**
     * @brief Checks that every @-sigiled binding across @p model's
     *        @c structure, @c separator, and @c list scopes, and every
     *        @-sigiled non-wiring cell outside the @c description and
     *        @c brief columns, resolves through hasTable() --
     *        a shape address is exempt, resolved instead by
     *        Validator::isStructure(). A @c separator or @c list description
     *        binding is exempt -- it is item prose, validated instead by
     *        isPaired(). A @c structure description binding is not exempt --
     *        it is the file-documentation binding, validated here like any
     *        other address.
     *
     * @param model The model whose bindings and cells are checked.
     * @returns juce::Result::ok() when every @-sigiled entry resolves,
     *          or the first failing entry's result.
     */
    static juce::Result isReference (const Model& model);

    /**
     * @brief Checks that every map bullet under @p scope carries a
     *        @c shape ordinal, and, when it names a table, that the
     *        table exists and declares a @c value column, then recurses
     *        into every nested blockquote.
     *
     * @param model The model @p row belongs to.
     * @param table The table @p row belongs to, named in a failure's
     *              location.
     * @param row   The row @p scope belongs to, named in a failure's
     *              location.
     * @param scope The blockquote scope walked.
     * @returns juce::Result::ok() when every map bullet resolves, or a
     *          failure naming the orphan map, missing table, or unknown
     *          column.
     */
    static juce::Result
    isMap (const Model& model, const Element& table, const Element& row, const Element& scope);

    /**
     * @brief Checks every output row's @c list column's map bullets,
     *        through the recursive overload.
     *
     * @param model The model whose output tables are checked.
     * @returns juce::Result::ok() when every row's map bullets resolve,
     *          or the first failing row's result.
     */
    static juce::Result isMap (const Model& model);

    /**
     * @brief Checks that @p entryValue's first segment resolves -- against
     *        @p row's writing file's index when it hits an alias, or,
     *        absent an alias hit, as a table of @p row's own document --
     *        and, when @p entryValue names a table or column, that each
     *        part resolves in turn.
     *
     * @param model      The model @p row belongs to.
     * @param table      The table @p row belongs to, named in a failure's
     *                   location.
     * @param row        The row @p entryValue was authored on.
     * @param column     The column @p entryValue was authored under, named
     *                   in a failure's location.
     * @param entryValue The @-sigiled address: the alias form
     *                   @c \@alias\[:table\[:column\[=value\]\]\], or,
     *                   absent an alias hit, the local form
     *                   @c \@table\[:column\[=value\]\]. A filter's
     *                   @c =value suffix is stripped before the column
     *                   part is checked.
     * @returns juce::Result::ok() when every declared part resolves, or a
     *          failure naming the missing table or column.
     */
    static juce::Result hasTable (const Model& model, const Element& table, const Element& row,
        const juce::Identifier& column, const juce::String& entryValue);

    /**
     * @brief Checks that every output row carries either both the
     *        @c \[begin\] and @c \[end\] bindings or neither --
     *        Model::isRegionRow() requires both, so a row with
     *        exactly one is otherwise silently treated as a whole-file
     *        row; this catches that case explicitly.
     *
     * @param model The model whose output tables are checked.
     * @returns juce::Result::ok() when every row's region bindings are
     *          paired, or a failure naming the unpaired row.
     */
    static juce::Result isRegionPaired (const Model& model);

    /**
     * @brief Checks that every region row's own declared file exists on
     *        disk -- a region row does not create the file it
     *        patches.
     *
     * @param model The model whose output tables are checked.
     * @returns juce::Result::ok() when every region row's own file
     *          exists, or a failure naming the missing file.
     */
    static juce::Result isRegionFilePresent (const Model& model);

    /**
     * @brief Reads @p row's own region file once and checks that its own
     *        resolved @c \[begin\] and @c \[end\] delimiters both match,
     *        in order, through Transforms::getDelimiterLines().
     *
     * @param model            The model @p row belongs to.
     * @param templateDocument The template document @p row's own bindings
     *                         are resolved through.
     * @param table            The table @p row belongs to, named in a
     *                         failure's location.
     * @param row              The region row whose delimiters are
     *                         checked.
     * @returns juce::Result::ok() when both delimiters match, in order,
     *          or a failure naming @p row's own file.
     */
    static juce::Result isRegionDelimited (
        const Model& model, const TemplateDocument& templateDocument, const Element& table, const Element& row);

    /**
     * @brief Finds @p row's own @c \[begin\] and @c \[end\] delimiter
     *        lines in @p row's own region file.
     *
     * @param model            The model @p row belongs to.
     * @param templateDocument The template document @p row's own bindings
     *                         are resolved through.
     * @param row              The region row whose delimiter lines are
     *                         found.
     * @returns The begin and end line indices Transforms::getDelimiterLines()
     *          returns.
     */
    static std::pair<int, int> getRegionLines (
        const Model& model, const TemplateDocument& templateDocument, const Element& row);

    /**
     * @brief Checks that the regions of one file do not overlap, and that
     *        one region is written by consecutive rows only -- a row that
     *        resolves a region the file's own earlier rows already wrote,
     *        after a row of another region, fails.
     *
     * @param model            The model whose output tables are checked.
     * @param templateDocument The template document each region row's own
     *                         bindings are resolved through.
     * @returns juce::Result::ok() when every file's regions are disjoint
     *          and consecutive, or a failure naming the first offending
     *          row.
     */
    static juce::Result isRegionDisjoint (const Model& model, const TemplateDocument& templateDocument);

    /**
     * @brief Checks every region row's own delimiters through the
     *        per-row overload.
     *
     * @param model            The model whose output tables are checked.
     * @param templateDocument The template document each region row's own
     *                         bindings are resolved through.
     * @returns juce::Result::ok() when every region row's delimiters
     *          match, in order, or the first failing row's result.
     */
    static juce::Result isRegionDelimited (const Model& model, const TemplateDocument& templateDocument);

    /**
     * @brief Checks that no declared @c file is shared between a region
     *        row and a whole-file row, in one enumeration
     *        pass -- each file's own first-seen classification is
     *        recorded, and a later row of the same file that disagrees
     *        with it fails at that row's own location.
     *
     * @param model The model whose output tables are checked.
     * @returns juce::Result::ok() when every declared file's rows agree
     *          on region-or-whole classification, or a failure naming the
     *          first row that disagrees.
     */
    static juce::Result isRegionExclusive (const Model& model);

    /**
     * @brief Checks that no header row carries two adjacent @c format
     *        columns, and that every authored @c format cell names a
     *        known transform.
     *
     * @param model The model whose tables are checked.
     * @returns juce::Result::ok() when every table's @c format columns are
     *          well formed and every named transform is known, or a
     *          failure naming the adjacency or the unknown transform.
     */
    static juce::Result isFormatted (const Model& model);

    /**
     * @brief Checks that every row declaring a given @c file value stays
     *        contiguous with the rows that already declared that file --
     *        no other file's rows may interleave between them.
     *
     * @param model The model @p table belongs to.
     * @param table The output table whose rows are checked.
     * @returns juce::Result::ok() when every file's rows stay contiguous,
     *          or a failure naming the file that reappears after its group
     *          closed.
     */
    static juce::Result isContiguous (const Model& model, const Element& table);

    /**
     * @brief Checks that @p column's resolved, non-reference values are
     *        unique, byte-exact, across @p rows.
     *
     * A cell whose authored text is an @-sigiled reference is exempt --
     * a reference legitimately repeats what its address already resolves
     * to once.
     *
     * @param model  The model @p rows belong to.
     * @param table  The table @p rows belong to, named in a failure's
     *               location.
     * @param rows   The rows whose @p column entries are checked.
     * @param column The column checked for duplicate values.
     * @returns juce::Result::ok() when every non-reference value is
     *          unique, or a failure naming the duplicate value.
     */
    static juce::Result isUniqueColumn (const Model& model, const Element& table,
        const jam::Array<const Element*>& rows, const juce::Identifier& column);

    /**
     * @brief Checks each of @p table's identity columns -- @c name,
     *        @c key, @c alias, and @c file -- for uniqueness through
     *        isUniqueColumn().
     *
     * @param model The model @p table belongs to.
     * @param table The table whose identity columns are checked.
     * @returns juce::Result::ok() when every identity column is unique,
     *          or the first failing column's result.
     */
    static juce::Result isUniqueTable (const Model& model, const Element& table);

    /**
     * @brief Checks that every table carrying an @c alias column declares
     *        each alias value once.
     *
     * @param model The model whose tables are checked.
     * @returns juce::Result::ok() when every table's alias values are
     *          unique, or a failure naming the duplicate alias.
     */
    static juce::Result isUniqueAlias (const Model& model);

    /**
     * @brief Checks every non-index, non-wiring table for column
     *        uniqueness, through isUniqueTable() -- a data table kept in
     *        the manifest is gated like any other, @c ## format included;
     *        wiring tables are exempt.
     *
     * @param model The model whose data tables are checked.
     * @returns juce::Result::ok() when every checked table is unique, or
     *          the first failing table's result.
     */
    static juce::Result isUnique (const Model& model);

    /**
     * @brief Checks that every output row's @c structure, @c separator,
     *        and @c list scopes carry only legal fence-prefix bracket
     *        words -- a complete bracket group whose word is either a
     *        comment-syntax extension found in @c map::commentSyntax
     *        under its own leading dot, or @c Id::noBanner. A bracket
     *        group with no closing bracket fails the same gate.
     *
     * @param model The model whose output tables are checked.
     * @returns juce::Result::ok() when every fence's bracket word is
     *          legal, or a failure naming the illegal word.
     */
    static juce::Result isFencePrefix (const Model& model);

    /**
     * @brief Checks that every declared @c ## toolchain table's header row
     *        declares both a @c command and a @c flag column --
     *        the invariant Processor::generate() trusts unconditionally
     *        when it reads a row's @c command and @c flag cells.
     *
     * @param model The model whose @c ## toolchain tables are checked.
     * @returns juce::Result::ok() when every declared @c ## toolchain
     *          table carries both columns, or a failure naming the table
     *          missing one.
     */
    static juce::Result isToolchain (const Model& model);

    /**
     * @brief Checks every declared @c ## format table's header, column
     *        uniqueness, and every row's own @c width -- the invariant
     *        Processor::format() trusts unconditionally when it builds
     *        its column-width map.
     *
     * Every @c ## format table's header row must carry a @c name and a
     * @c width column (text::Diagnostics::failFormatColumn). The table
     * then passes isUniqueTable(). Every row's own @c width must
     * round-trip as an integer greater than zero
     * (text::Diagnostics::failFormatWidth).
     *
     * @param model The model whose @c ## format tables are checked.
     * @returns juce::Result::ok() when every declared @c ## format table
     *          carries both columns, is unique, and every row's own
     *          @c width is a positive integer, or the first failure's
     *          result.
     */
    static juce::Result isFormat (const Model& model);

    /**
     * @brief Runs every manifest-level check against @p model and
     *        @p templateDocument, in declared order, stopping at the
     *        first failure -- the one driver isValid() reads through.
     *
     * @param model            The parsed manifest and its data tables.
     * @param templateDocument The parsed template file the manifest wires
     *                         against.
     * @returns juce::Result::ok() when every check passes, or the first
     *          failing check's result.
     */
    static juce::Result isManifest (const Model& model, const TemplateDocument& templateDocument);
};
