#pragma once
#include <JuceHeader.h>
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
                    for (auto* scope { model.getTableCell (*row, column) }; scope != nullptr;
                         scope = model.getBlockquote (*scope))
                        if (auto* list { model.getList (*scope) })
                            for (auto* item : *list)
                                if (const auto result { function (
                                        *table, *row, *item->get<juce::String> (Id::value)) };
                                    not result.wasOk())
                                    return result;

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
        const auto indexTables { model.getTables (Id::index) };

        if (indexTables.isEmpty())
            return juce::Result::fail (Id::index.toString() + Id::diagnosticSeparator
                                       + text::Diagnostics::failTableMissing);

        Element& indexTable { *indexTables.at (0) };

        for (auto* row : model.getTableRows (indexTable))
        {
            const auto pathCell { model.getTableValue (*row, Id::symbol) };

            if (pathCell.isEmpty())
                return juce::Result::fail (getLocation (indexTable, *row, Id::symbol.toString())
                                           + Id::diagnosticSeparator + text::Diagnostics::failNotFound);

            if (juce::File::createFileWithoutCheckingPath (pathCell).hasFileExtension (Extensions::md)
                and not model.getOutput (pathCell).existsAsFile())
                return juce::Result::fail (getLocation (indexTable, *row, Id::symbol.toString())
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failOutputMissing + Id::diagnosticSeparator
                                           + pathCell);

            if (juce::File::createFileWithoutCheckingPath (pathCell).hasFileExtension (Extensions::cast)
                and not model.getOutput (pathCell).existsAsFile())
                return juce::Result::fail (getLocation (indexTable, *row, Id::symbol.toString())
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
                    auto* scope { model.getTableCell (*row, Id::structure) };

                    if (scope == nullptr or model.getStructure (*scope).isEmpty())
                        return juce::Result::fail (
                            getLocation (*table, *row, Id::structure.toString())
                            + Id::diagnosticSeparator + text::Diagnostics::failTemplateMissing);

                    for (; scope != nullptr; scope = model.getBlockquote (*scope))
                        if (const auto depthShapeId { model.getStructure (*scope) };
                            depthShapeId.isNotEmpty())
                            if (templateDocument.getCodeBlock (juce::Identifier (depthShapeId))
                                == nullptr)
                                return juce::Result::fail (
                                    getLocation (*table, *row, Id::structure.toString())
                                    + Id::diagnosticSeparator
                                    + text::Diagnostics::failTemplateMissing
                                    + Id::diagnosticSeparator + depthShapeId);
                }

        return juce::Result::ok();
    }

    /**
     * @brief Checks that every @c template:\<id\> named in a placeholder,
     *        structure, or separator binding names a shape that exists in
     *        @p templateDocument.
     *
     * @param model            The model whose bindings are checked.
     * @param templateDocument The template document each named shape is
     *                         looked up against.
     * @returns juce::Result::ok() when every named shape exists, or a
     *          failure naming the missing shape.
     */
    static juce::Result
    isPlaceholders (const Model& model, const TemplateDocument& templateDocument)
    {
        for (const auto& column : { Id::placeholder, Id::structure, Id::separator })
            if (const auto result { forEachBinding (model, column,
                    [&templateDocument, &column] (Element& table, Element& row,
                        const juce::String& entryValue) -> juce::Result
                    {
                        if (jam::Format::getPreColon (entryValue).trim()
                            == Id::templatePath.toString())
                        {
                            const juce::Identifier shapeId {
                                jam::Format::getPostColon (entryValue).trim()
                            };

                            if (templateDocument.getCodeBlock (shapeId) == nullptr)
                                return juce::Result::fail (
                                    getLocation (table, row, column.toString())
                                    + Id::diagnosticSeparator
                                    + text::Diagnostics::failTemplateMissing
                                    + Id::diagnosticSeparator + shapeId.toString());
                        }

                        return juce::Result::ok();
                    }) };
                not result.wasOk())
                return result;

        return juce::Result::ok();
    }

    /**
     * @brief Checks that every @-sigiled bullet value and every
     *        @-sigiled table entry resolves against its writing file's
     *        index, through isAddress().
     *
     * isReference() drives both forEachBinding() over the placeholder,
     * structure, and separator columns, and forEachEntry() over every
     * table cell -- an @-sigiled binding and an @-sigiled data entry
     * are checked by the same address resolution.
     *
     * @param model The model whose bindings and entries are checked.
     * @returns juce::Result::ok() when every address resolves, or a
     *          failure naming the missing alias, table, or column.
     */
    static juce::Result isReference (const Model& model)
    {
        for (const auto& column : { Id::placeholder, Id::structure, Id::separator })
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
            auto* referencedTable { model.getTables (row, tableReference) };

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
            if (column != Id::format.toString())
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
        const auto indexTables { model.getTables (Id::index) };
        const auto manifestOrigin { *indexTables.at (0)->get<juce::String> (Id::path) };

        for (auto* table : model.getTables())
        {
            const auto tableOrigin { *table->get<juce::String> (Id::path) };

            if (tableOrigin != manifestOrigin and not table->isTag (Id::index))
                if (const auto result { isUniqueTable (model, *table) }; not result.wasOk())
                    return result;
        }

        return juce::Result::ok();
    }

    /**
     * @brief Runs every manifest fatal against @p model and
     *        @p templateDocument, in the order they are listed: index,
     *        alias uniqueness, column uniqueness, formatting, structure,
     *        placeholders, then reference resolution.
     *
     * @param model            The parsed manifest and its data tables.
     * @param templateDocument The parsed template file the manifest wires
     *                         against.
     * @returns juce::Result::ok() when every fatal passes, or the first
     *          failing check's result.
     */
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

        if (const auto result { isStructure (model, templateDocument) }; not result.wasOk())
            return result;

        if (const auto result { isPlaceholders (model, templateDocument) }; not result.wasOk())
            return result;

        return isReference (model);
    }
};
