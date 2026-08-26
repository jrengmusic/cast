#pragma once
#include <JuceHeader.h>
#include "Items.h"
#include "Model.h"
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
     * @brief Checks that @p shapeId names a code block that exists in
     *        @p templateDocument.
     *
     * @param templateDocument The template document @p shapeId is looked up
     *                         against.
     * @param table            The table named in a failure's location.
     * @param row              The row named in a failure's location.
     * @param column           The column named in a failure's location.
     * @param shapeId          The shape id to look up.
     * @returns juce::Result::ok() when @p shapeId resolves, or a failure
     *          naming the missing shape.
     */
    static juce::Result isKnownTemplate (const TemplateDocument& templateDocument, Element& table,
        Element& row, const juce::Identifier& column, const juce::String& shapeId)
    {
        if (templateDocument.getCodeBlock (juce::Identifier (shapeId)) == nullptr)
            return juce::Result::fail (getLocation (table, row, column.toString())
                                       + Id::diagnosticSeparator
                                       + text::Diagnostics::failTemplateMissing
                                       + Id::diagnosticSeparator + shapeId);

        return juce::Result::ok();
    }

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
    static juce::Result isValid (const Model& model, const TemplateDocument& templateDocument)
    {
        return isManifest (model, templateDocument);
    }

    /**
     * @brief Invokes @p function once per bullet found by walking
     *        @p column's blockquote scopes, depth by depth, across every
     *        output table's rows.
     *
     * @param model    The model whose output tables are walked.
     * @param column   The column whose blockquote scopes carry the binding
     *                 lists to walk.
     * @param function Callable invoked as
     *                 @c function(table,row,bulletValue) for each bullet
     *                 found, returning a juce::Result.
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

    template <typename Function>
    static juce::Result
    forEachBinding (Element& table, Element& row, Element& scope, Function&& function)
    {
        for (auto* child : scope)
        {
            if (child->isTag (Id::ul))
                for (auto* item : *child)
                    if (const auto result { function (
                            table, row, *item->get<juce::String> (Id::value)) };
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
     * @brief Invokes @p function once per cell of every non-index table's
     *        rows, passing the cell's authored, unresolved text.
     *
     * Where forEachBinding() walks one named column's binding lists,
     * forEachEntry() walks every cell of every non-index table and yields
     * each cell's authored @c Id::rawText, unresolved.
     *
     * @param model    The model whose non-index tables are walked.
     * @param function Callable invoked as
     *                 @c function(table,row,columnId,rawText) for each
     *                 cell, returning a juce::Result.
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
                        if (const auto result { function (
                                *table, *row, cell->id, *cell->get<juce::String> (Id::rawText)) };
                            not result.wasOk())
                            return result;

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
    static juce::Result isIndex (const Model& model)
    {
        for (auto* indexTable : model.getTables (Id::index))
            for (auto* row : model.getTableRows (*indexTable))
            {
                const auto pathCell { model.getTableValue (*row, Id::symbol) };

                if (pathCell.isEmpty())
                    return juce::Result::fail (getLocation (*indexTable, *row, Id::symbol.toString())
                                               + Id::diagnosticSeparator
                                               + text::Diagnostics::failNotFound);

                if (juce::File::createFileWithoutCheckingPath (pathCell)
                        .hasFileExtension (Extensions::md)
                    and not model.getFile (pathCell).existsAsFile())
                    return juce::Result::fail (getLocation (*indexTable, *row, Id::symbol.toString())
                                               + Id::diagnosticSeparator
                                               + text::Diagnostics::failOutputMissing
                                               + Id::diagnosticSeparator + pathCell);

                if (juce::File::createFileWithoutCheckingPath (pathCell)
                        .hasFileExtension (Extensions::cast)
                    and not model.getFile (pathCell).existsAsFile())
                    return juce::Result::fail (getLocation (*indexTable, *row, Id::symbol.toString())
                                               + Id::diagnosticSeparator
                                               + text::Diagnostics::failTemplateMissing
                                               + Id::diagnosticSeparator + pathCell);
            }

        return juce::Result::ok();
    }

    /**
     * @brief Checks that every output row declares a structure and that
     *        every depth's named shape exists in @p templateDocument.
     *
     * @param model            The model whose output tables are checked.
     * @param templateDocument The template document each depth's shape is
     *                         looked up against.
     * @returns juce::Result::ok() when every row's structure resolves at
     *          every depth, or a failure naming the missing structure or
     *          shape.
     */
    static juce::Result isStructure (const Model& model, const TemplateDocument& templateDocument)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                {
                    auto& structureScope { *model.getTableCell (*row, Id::structure) };

                    if (model.getStructure (structureScope).isEmpty())
                        return juce::Result::fail (
                            getLocation (*table, *row, Id::structure.toString())
                            + Id::diagnosticSeparator + text::Diagnostics::failTemplateMissing);

                    juce::String shapeId;

                    structureScope.applyFunctionRecursively (
                        [&shapeId, &templateDocument] (const Element& candidate) -> bool
                        {
                            if (shapeId.isEmpty() and candidate.contains (Id::templatePath)
                                and templateDocument.getCodeBlock (juce::Identifier (
                                        *candidate.get<juce::String> (Id::templatePath)))
                                       == nullptr)
                                shapeId = *candidate.get<juce::String> (Id::templatePath);

                            return shapeId.isEmpty();
                        });

                    if (shapeId.isNotEmpty())
                        return isKnownTemplate (templateDocument, *table, *row, Id::structure, shapeId);
                }

        return juce::Result::ok();
    }

    /**
     * @brief Checks that no two bindings under @p scope share the same
     *        name, then checks the same for every nested blockquote scope.
     *
     * @param table  The table @p row belongs to, named in a failure's
     *               location.
     * @param row    The row @p scope belongs to, named in a failure's
     *               location.
     * @param column The column @p scope was read from, named in a
     *               failure's location.
     * @param scope  The blockquote scope whose bindings are checked.
     * @param seen   The binding names seen for the current shape, reset at
     *               each shape line.
     * @returns juce::Result::ok() when every scope's bindings are unique,
     *          or a failure naming the duplicate binding.
     */
    static juce::Result isBindingCountValid (Element& table, Element& row,
        const juce::Identifier& column, Element& scope, jam::HashSet<juce::Identifier>& seen)
    {
        for (auto* child : scope)
        {
            if (child->isTag (Id::p) and child->contains (Id::templatePath))
                seen.clear();

            if (child->isTag (Id::ul))
                for (auto* item : *child)
                {
                    if (item->id == Id::list)
                        seen.clear();
                    else
                    {
                        if (seen.contains (item->id))
                            return juce::Result::fail (getLocation (table, row, column.toString())
                                                       + Id::diagnosticSeparator
                                                       + text::Diagnostics::failBindingDuplicate
                                                       + Id::diagnosticSeparator + item->id.toString());

                        seen.insert (item->id);
                    }
                }

            if (child->isTag (Id::blockquote))
                if (const auto result { isBindingCountValid (table, row, column, *child, seen) };
                    not result.wasOk())
                    return result;
        }

        return juce::Result::ok();
    }

    /**
     * @brief Checks every output row's @c structure and @c separator
     *        scopes for duplicate binding names, through
     *        isBindingCountValid().
     *
     * @param model The model whose output tables are checked.
     * @returns juce::Result::ok() when every row's bindings are unique, or
     *          the first failing row's result.
     */
    static juce::Result isBindingCountValid (const Model& model)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                    for (const auto& column : { Id::structure, Id::separator })
                        if (auto* scope { model.getTableCell (*row, column) })
                        {
                            jam::HashSet<juce::Identifier> seen;

                            if (const auto result {
                                    isBindingCountValid (*table, *row, column, *scope, seen) };
                                not result.wasOk())
                                return result;
                        }

        return juce::Result::ok();
    }

    /**
     * @brief Checks that every depth of @p structureScope's own shape
     *        carries an even count of @c ::: markers.
     *
     * @param templateDocument The template document each depth's shape is
     *                         read from.
     * @param table            The table @p row belongs to, named in a
     *                         failure's location.
     * @param row              The row @p structureScope belongs to, named
     *                         in a failure's location.
     * @param structureScope   The row's own @c structure scope, walked
     *                         depth by depth.
     * @returns juce::Result::ok() when every depth's marker count is even,
     *          or a failure naming the shape with an odd count.
     */
    static juce::Result isMarkerCountValid (const TemplateDocument& templateDocument, Element& table,
        Element& row, Element& structureScope)
    {
        juce::String shapeId;

        structureScope.applyFunctionRecursively (
            [&shapeId, &templateDocument] (const Element& candidate) -> bool
            {
                if (shapeId.isEmpty() and candidate.contains (Id::templatePath))
                {
                    const auto& blockText { *templateDocument.getCodeBlock (juce::Identifier (
                            *candidate.get<juce::String> (Id::templatePath)))
                            ->get<juce::String> (Id::value) };
                    int count { 0 };
                    int cursor { 0 };

                    for (auto position { blockText.indexOf (cursor, Id::tripleColon) }; position >= 0;
                         position = blockText.indexOf (cursor, Id::tripleColon))
                    {
                        ++count;
                        cursor = position + Id::tripleColon.length();
                    }

                    if (count % 2 != 0)
                        shapeId = *candidate.get<juce::String> (Id::templatePath);
                }

                return shapeId.isEmpty();
            });

        if (shapeId.isNotEmpty())
            return juce::Result::fail (getLocation (table, row, Id::structure.toString())
                                       + Id::diagnosticSeparator + text::Diagnostics::failMarkerUnterminated
                                       + Id::diagnosticSeparator + shapeId);

        return juce::Result::ok();
    }

    /**
     * @brief Checks every output row's @c structure scope for an odd
     *        @c ::: marker count, through isMarkerCountValid().
     *
     * @param model            The model whose output tables are checked.
     * @param templateDocument The template document each row's shapes are
     *                         read from.
     * @returns juce::Result::ok() when every row's markers are balanced,
     *          or the first failing row's result.
     */
    static juce::Result
    isMarkerCountValid (const Model& model, const TemplateDocument& templateDocument)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                {
                    auto& structureScope { *model.getTableCell (*row, Id::structure) };

                    if (const auto result {
                            isMarkerCountValid (templateDocument, *table, *row, structureScope) };
                        not result.wasOk())
                        return result;
                }

        return juce::Result::ok();
    }

    static juce::Result isShapeSupplied (const Model& model, const TemplateDocument& templateDocument,
        Element& table, Element& row, const juce::Identifier& column, Element& precedingShape,
        const jam::HashSet<juce::Identifier>& seen)
    {
        const auto shapeId { juce::Identifier (*precedingShape.get<juce::String> (Id::templatePath)) };
        const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                  ->get<jam::Document::Identifiers> (Id::placeholder) };
        jam::Array<juce::String> headers;

        if (not precedingShape.isTag (Id::p))
        {
            const auto indent { *precedingShape.get<int> (Id::level) };
            const auto ordinal { *precedingShape.get<int> (Id::line) };
            auto* sourceItem { model.getSource (row, indent, ordinal) };
            const auto& sourceValue { *sourceItem->get<juce::String> (Id::value) };

            if (auto* sourceTable { model.getTable (row, sourceValue) })
                headers = model.getTableHeaders (*sourceTable);
        }

        for (const auto& name : tokens)
            if (name != Id::list and name != Id::comment and not seen.contains (name)
                and model.getTableCell (row, name) == nullptr and not headers.contains (name.toString()))
                return juce::Result::fail (getLocation (table, row, column.toString())
                                           + Id::diagnosticSeparator + text::Diagnostics::failNotFound
                                           + Id::diagnosticSeparator + name.toString());

        return juce::Result::ok();
    }

    static juce::Result isPlaceholderScope (const Model& model, const TemplateDocument& templateDocument,
        Element& table, Element& row, const juce::Identifier& column, Element& scope,
        Element*& precedingShape, jam::HashSet<juce::Identifier>& seen)
    {
        for (auto* block : scope)
        {
            if (block->isTag (Id::p) and block->contains (Id::templatePath) and column == Id::structure)
            {
                if (precedingShape != nullptr)
                    if (const auto result { isShapeSupplied (
                            model, templateDocument, table, row, column, *precedingShape, seen) };
                        not result.wasOk())
                        return result;

                precedingShape = block;
                seen.clear();
            }

            if (block->isTag (Id::ul))
                for (auto* item : *block)
                {
                    if (item->contains (Id::templatePath))
                        if (const auto result { isKnownTemplate (templateDocument, table, row, column,
                                *item->get<juce::String> (Id::templatePath)) };
                            not result.wasOk())
                            return result;

                    if (item->id == Id::list and column == Id::structure)
                    {
                        if (precedingShape != nullptr)
                            if (const auto result { isShapeSupplied (
                                    model, templateDocument, table, row, column, *precedingShape, seen) };
                                not result.wasOk())
                                return result;

                        precedingShape = item;
                        seen.clear();
                    }
                    else if (item->id != Id::list)
                        seen.insert (item->id);
                }

            if (block->isTag (Id::blockquote))
                if (const auto result { isPlaceholderScope (model, templateDocument, table, row, column,
                        *block, precedingShape, seen) };
                    not result.wasOk())
                    return result;
        }

        return juce::Result::ok();
    }

    static juce::Result
    isPlaceholders (const Model& model, const TemplateDocument& templateDocument)
    {
        for (const auto& column : { Id::structure, Id::separator })
            for (auto* table : model.getTables())
                if (model.isOutputTable (*table))
                    for (auto* row : model.getTableRows (*table))
                        if (auto* scope { model.getTableCell (*row, column) })
                        {
                            Element* precedingShape { nullptr };
                            jam::HashSet<juce::Identifier> seen;

                            if (const auto result { isPlaceholderScope (model, templateDocument, *table,
                                    *row, column, *scope, precedingShape, seen) };
                                not result.wasOk())
                                return result;

                            if (precedingShape != nullptr and column == Id::structure)
                                if (const auto result { isShapeSupplied (model, templateDocument, *table,
                                        *row, column, *precedingShape, seen) };
                                    not result.wasOk())
                                    return result;
                        }

        return juce::Result::ok();
    }

    static int getOccurrenceCount (const juce::String& blockText, const juce::Identifier& name)
    {
        const auto marker { Items::getMarker (name) };
        int count { 0 };
        int cursor { 0 };

        for (auto position { blockText.indexOf (cursor, marker) }; position >= 0;
             position = blockText.indexOf (cursor, marker))
        {
            ++count;
            cursor = position + marker.length();
        }

        return count;
    }

    static juce::Result isSourceCountValid (const Model& model, const TemplateDocument& templateDocument)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                {
                    auto& structureScope { *model.getTableCell (*row, Id::structure) };
                    int demanded { 0 };
                    int supplied { 0 };

                    structureScope.applyFunctionRecursively (
                        [&demanded, &supplied, &templateDocument] (const Element& candidate)
                        {
                            if (candidate.isTag (Id::p) and candidate.contains (Id::templatePath))
                            {
                                ++supplied;
                                demanded += getOccurrenceCount (*templateDocument.getCodeBlock (
                                        juce::Identifier (*candidate.get<juce::String> (Id::templatePath)))
                                        ->get<juce::String> (Id::value), Id::list);
                            }

                            if (candidate.parent->isTag (Id::ul) and candidate.id == Id::list)
                                ++supplied;
                        });

                    if (demanded != supplied - 1)
                        return juce::Result::fail (getLocation (*table, *row, Id::structure.toString())
                                                   + Id::diagnosticSeparator
                                                   + text::Diagnostics::failAmbiguous);
                }

        return juce::Result::ok();
    }

    static juce::Result isColumnPartnered (const Model& model, Element& table, Element& row,
        const juce::Identifier& column, Element& scope)
    {
        for (auto* block : scope)
        {
            if (block->isTag (Id::ul))
                for (auto* item : *block)
                    if (item->id == Id::list)
                    {
                        const auto indent { *item->get<int> (Id::level) };
                        const auto ordinal { *item->get<int> (Id::line) };

                        if (not (column == Id::separator and indent == 0 and ordinal == 0)
                            and model.getSource (row, indent, ordinal) == nullptr)
                            return juce::Result::fail (getLocation (table, row, column.toString())
                                                       + Id::diagnosticSeparator
                                                       + text::Diagnostics::failOrphan);
                    }

            if (block->isTag (Id::blockquote))
                if (const auto result { isColumnPartnered (model, table, row, column, *block) };
                    not result.wasOk())
                    return result;
        }

        return juce::Result::ok();
    }

    static juce::Result isPaired (const Model& model)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                {
                    if (const auto result { isColumnPartnered (
                            model, *table, *row, Id::structure, *model.getTableCell (*row, Id::structure)) };
                        not result.wasOk())
                        return result;

                    if (const auto result { isColumnPartnered (
                            model, *table, *row, Id::separator, *model.getTableCell (*row, Id::separator)) };
                        not result.wasOk())
                        return result;
                }

        return juce::Result::ok();
    }

    static juce::Result isReference (const Model& model)
    {
        for (const auto& column : { Id::structure, Id::separator, Id::list })
            if (const auto result { forEachBinding (model, column,
                    [&model, &column] (Element& table, Element& row,
                        const juce::String& entryValue) -> juce::Result
                    {
                        if (entryValue.startsWithChar (Chars::at))
                            return isAddress (model, table, row, column, entryValue);

                        return juce::Result::ok();
                    }) };
                not result.wasOk())
                return result;

        return forEachEntry (model,
            [&model] (Element& table, Element& row, const juce::Identifier& column,
                const juce::String& entryValue) -> juce::Result
            {
                if (entryValue.startsWithChar (Chars::at))
                    return isAddress (model, table, row, column, entryValue);

                return juce::Result::ok();
            });
    }

    /**
     * @brief Checks that @p entryValue's alias resolves in @p row's
     *        writing file's index, and, when @p entryValue names a table
     *        or column, that each part resolves in turn.
     *
     * @param model      The model @p row belongs to.
     * @param table      The table @p row belongs to, named in a failure's
     *                   location.
     * @param row        The row @p entryValue was authored on.
     * @param column     The column @p entryValue was authored under, named
     *                   in a failure's location.
     * @param entryValue The @-sigiled address:
     *                   @c \@alias\[:table\[:column\]\].
     * @returns juce::Result::ok() when every declared part resolves, or a
     *          failure naming the missing alias, table, or column.
     */
    static juce::Result isAddress (const Model& model, Element& table, Element& row,
        const juce::Identifier& column, const juce::String& entryValue)
    {
        const auto parts { jam::Strings::fromTokens (
            entryValue, juce::String::charToString (Chars::colon), {}) };
        const auto aliasName { parts.at (0).trim() };

        if (model.getValue (row, aliasName).isEmpty())
            return juce::Result::fail (getLocation (table, row, column.toString())
                                       + Id::diagnosticSeparator
                                       + text::Diagnostics::failAliasMissing
                                       + Id::diagnosticSeparator + aliasName);

        if (parts.size() > 1)
        {
            const auto tableReference { aliasName + juce::String::charToString (Chars::colon)
                                        + parts.at (1).trim() };
            auto* referencedTable { model.getTable (row, tableReference) };

            if (referencedTable == nullptr)
                return juce::Result::fail (getLocation (table, row, column.toString())
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failTableMissing
                                           + Id::diagnosticSeparator + tableReference);

            if (parts.size() > 2)
            {
                const auto columnName { parts.at (2).trim() };

                if (not model.getTableHeaders (*referencedTable)
                            .contains (jam::Format::toValidID (columnName)))
                    return juce::Result::fail (getLocation (table, row, column.toString())
                                               + Id::diagnosticSeparator
                                               + text::Diagnostics::failColumnUnknown
                                               + Id::diagnosticSeparator + columnName);
            }
        }

        return juce::Result::ok();
    }

    /**
     * @brief Checks that no header row carries two adjacent @c format
     *        columns, and that every authored @c format cell names a
     *        known transform.
     *
     * @param model The model whose output tables are checked.
     * @returns juce::Result::ok() when every table's @c format columns are
     *          well formed and every named transform is known, or a
     *          failure naming the adjacency or the unknown transform.
     */
    static juce::Result isFormatted (const Model& model)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
            {
                auto* headerRow { model.getTableRow (*table, Id::headerRow) };

                for (auto* cell : *headerRow)
                    if (cell->id == Id::format and cell->nextSibling != nullptr
                        and cell->nextSibling->id == Id::format)
                        return juce::Result::fail (getLocation (*table, *headerRow, Id::format.toString())
                                                   + Id::diagnosticSeparator
                                                   + text::Diagnostics::failFormatAdjacent);
            }

        return forEachCell (model, Id::format.toString(),
            [&model] (Element& table, Element& row, const juce::String& transform) -> juce::Result
            {
                if (model.isOutputTable (table) and transform.isNotEmpty()
                    and not Transforms::contains (transform))
                    return juce::Result::fail (getLocation (table, row, Id::format.toString())
                                               + Id::diagnosticSeparator
                                               + text::Diagnostics::failUnknownTransform
                                               + Id::diagnosticSeparator + transform);

                return juce::Result::ok();
            });
    }

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
    static juce::Result isContiguous (const Model& model, Element& table)
    {
        jam::HashSet<juce::String> closedFiles;
        juce::String previousFile;

        for (auto* row : model.getTableRows (table))
        {
            const auto& file { model.getValue (*row, Id::file) };

            if (file != previousFile)
            {
                if (previousFile.isNotEmpty())
                    closedFiles.insert (previousFile);

                previousFile = file;
            }

            if (closedFiles.contains (file))
                return juce::Result::fail (getLocation (table, *row, Id::file.toString())
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failDuplicate + file
                                           + juce::String::charToString (Chars::doubleQuote));
        }

        return juce::Result::ok();
    }

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
    static juce::Result isUniqueColumn (const Model& model, Element& table,
        const jam::Array<Element*>& rows, const juce::String& column)
    {
        jam::HashSet<juce::String> seen;

        for (auto* row : rows)
            if (auto* cell { model.getTableCell (*row, juce::Identifier (column)) })
            {
                const auto rawText { *cell->get<juce::String> (Id::rawText) };

                if (not rawText.startsWithChar (Chars::at))
                    if (const auto& value { *cell->get<juce::String> (Id::value) }; value.isNotEmpty())
                    {
                        if (seen.contains (value))
                            return juce::Result::fail (getLocation (table, *row, column)
                                                       + Id::diagnosticSeparator
                                                       + text::Diagnostics::failDuplicate + value
                                                       + juce::String::charToString (Chars::doubleQuote));

                        seen.insert (value);
                    }
            }

        return juce::Result::ok();
    }

    /**
     * @brief Checks every one of @p table's columns, other than
     *        @c format, for uniqueness through isUniqueColumn().
     *
     * @param model The model @p table belongs to.
     * @param table The table whose columns are checked.
     * @returns juce::Result::ok() when every column is unique, or the
     *          first failing column's result.
     */
    static juce::Result isUniqueTable (const Model& model, Element& table)
    {
        const auto rows { model.getTableRows (table) };

        for (const auto& column : model.getTableHeaders (table))
            if (column != Id::format.toString() and column != Id::comment.toString())
                if (const auto result { isUniqueColumn (model, table, rows, column) };
                    not result.wasOk())
                    return result;

        return juce::Result::ok();
    }

    /**
     * @brief Checks every non-index table not declared in the manifest's
     *        own file for column uniqueness, through isUniqueTable().
     *
     * @param model The model whose data-file tables are checked.
     * @returns juce::Result::ok() when every checked table is unique, or
     *          the first failing table's result.
     */
    static juce::Result isUnique (const Model& model)
    {
        juce::StringArray manifestOrigin;

        for (auto* table : model.getTables (Id::index))
            manifestOrigin.add (*table->get<juce::String> (Id::path));

        for (auto* table : model.getTables())
        {
            const auto tableOrigin { *table->get<juce::String> (Id::path) };

            if (not manifestOrigin.contains (tableOrigin) and not table->isTag (Id::index))
                if (const auto result { isUniqueTable (model, *table) }; not result.wasOk())
                    return result;
        }

        return juce::Result::ok();
    }

    static juce::Result isManifest (const Model& model, const TemplateDocument& templateDocument)
    {
        if (const auto result { isIndex (model) }; not result.wasOk())
            return result;

        if (const auto result { unique (model, Id::alias.toString(), {}) }; not result.wasOk())
            return result;

        if (const auto result { isUnique (model) }; not result.wasOk())
            return result;

        if (const auto result { isFormatted (model) }; not result.wasOk())
            return result;

        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                if (const auto result { isContiguous (model, *table) }; not result.wasOk())
                    return result;

        if (const auto result { isStructure (model, templateDocument) }; not result.wasOk())
            return result;

        if (const auto result { isBindingCountValid (model) }; not result.wasOk())
            return result;

        if (const auto result { isMarkerCountValid (model, templateDocument) }; not result.wasOk())
            return result;

        if (const auto result { isPaired (model) }; not result.wasOk())
            return result;

        if (const auto result { isReference (model) }; not result.wasOk())
            return result;

        if (const auto result { isPlaceholders (model, templateDocument) }; not result.wasOk())
            return result;

        return isSourceCountValid (model, templateDocument);
    }
};
