#pragma once
#include <JuceHeader.h>
#include "Model.h"
#include "TemplateDocument.h"

struct Validator : jam::MarkdownValidator
{
    static juce::Result isValid (const Model& model, const TemplateDocument& templateDocument)
    {
        return isManifest (model, templateDocument);
    }

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

    static juce::Result
    isPlaceholders (const Model& model, const TemplateDocument& templateDocument)
    {
        for (const auto& column : { Id::placeholder, Id::structure })
            if (const auto result { forEachBinding (model, column,
                    [&templateDocument, &column] (Element& table, Element& row,
                        const juce::String& entryValue) -> juce::Result
                    {
                        if (jam::Format::getPreColon (entryValue).trim()
                            == Id::templatePath.toString())
                        {
                            const juce::Identifier codeId {
                                jam::Format::getPostColon (entryValue).trim()
                            };

                            if (templateDocument.getCodeBlock (codeId) == nullptr)
                                return juce::Result::fail (
                                    getLocation (table, row, column.toString())
                                    + Id::diagnosticSeparator
                                    + text::Diagnostics::failTemplateMissing
                                    + Id::diagnosticSeparator + codeId.toString());
                        }

                        return juce::Result::ok();
                    }) };
                not result.wasOk())
                return result;

        return juce::Result::ok();
    }

    static juce::Result isReference (const Model& model)
    {
        for (const auto& column : { Id::placeholder, Id::structure })
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

        return juce::Result::ok();
    }

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

    static juce::Result isFormatted (const Model& model)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
            {
                auto* headerRow { model.getTableRow (*table, Id::headerRow) };

                for (auto* cell : *headerRow)
                    if (cell->id == Id::format and cell->nextSibling != nullptr
                        and cell->nextSibling->id == Id::format)
                        return juce::Result::fail (getLocation (*table, *table, Id::format.toString())
                                                   + Id::diagnosticSeparator
                                                   + text::Diagnostics::failFormatAdjacent);
            }

        return forEachCell (model, Id::format.toString(),
            [&model] (Element& table, Element& row, const juce::String& operation) -> juce::Result
            {
                if (model.isOutputTable (table) and operation.isNotEmpty()
                    and not Transforms::contains (operation))
                    return juce::Result::fail (getLocation (table, row, Id::format.toString())
                                               + Id::diagnosticSeparator
                                               + text::Diagnostics::failUnknownTransform
                                               + Id::diagnosticSeparator + operation);

                return juce::Result::ok();
            });
    }

    static juce::Result isUnique (const Model& model)
    {
        const auto indexTables { model.getTables (Id::index) };
        const auto manifestOrigin { *indexTables.at (0)->get<juce::String> (Id::path) };

        for (auto* table : model.getTables())
        {
            const auto tableOrigin { *table->get<juce::String> (Id::path) };

            if (tableOrigin != manifestOrigin and not table->isTag (Id::index))
            {
                const auto headers { model.getTableHeaders (*table) };
                const auto rows { model.getTableRows (*table) };

                for (const auto& column : headers)
                {
                    if (column != Id::format.toString())
                    {
                        jam::HashSet<juce::String> seen;

                        for (auto* row : rows)
                        {
                            if (auto* cell { model.getTableCell (*row, juce::Identifier (column)) })
                            {
                                const auto rawText { *cell->get<juce::String> (Id::rawText) };

                                if (not rawText.startsWithChar (Chars::at))
                                {
                                    if (const auto& value { *cell->get<juce::String> (Id::value) };
                                        value.isNotEmpty())
                                    {
                                        if (seen.contains (value))
                                            return juce::Result::fail (
                                                getLocation (*table, *row, column) + Id::diagnosticSeparator
                                                + text::Diagnostics::failDuplicate + value
                                                + juce::String::charToString (Chars::doubleQuote));

                                        seen.insert (value);
                                    }
                                }
                            }
                        }
                    }
                }
            }
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

        if (const auto result { isStructure (model, templateDocument) }; not result.wasOk())
            return result;

        if (const auto result { isPlaceholders (model, templateDocument) }; not result.wasOk())
            return result;

        return isReference (model);
    }
};
