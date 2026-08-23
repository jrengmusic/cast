#pragma once
#include <JuceHeader.h>
#include "Model.h"
#include "TemplateDocument.h"

struct Validator : jam::MarkdownValidator
{
    static juce::Result isValid (const Model& model)
    {
        return isManifest (model);
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
                    const auto shapeId { model.getStructure (*row, 0) };

                    if (shapeId.isEmpty())
                        return juce::Result::fail (getLocation (*table, *row, Id::structure.toString())
                                                   + Id::diagnosticSeparator
                                                   + text::Diagnostics::failTemplateMissing);

                    for (int depth { 0 }; model.getStructure (*row, depth).isNotEmpty(); ++depth)
                    {
                        const auto depthShapeId { model.getStructure (*row, depth) };
                        const juce::Identifier codeId { depthShapeId };

                        if (templateDocument.getCodeBlock (codeId) == nullptr)
                            return juce::Result::fail (
                                getLocation (*table, *row, Id::structure.toString())
                                + Id::diagnosticSeparator + text::Diagnostics::failTemplateMissing
                                + Id::diagnosticSeparator + depthShapeId);
                    }
                }

        return juce::Result::ok();
    }

    static juce::Result
    isPlaceholders (const Model& model, const TemplateDocument& templateDocument)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                    for (const auto& column : { Id::placeholder, Id::structure })
                        for (auto& entry : model.getSource (*row, column))
                        {
                            const auto& [entryDepth, entryKey, entryValue] { entry };
                            juce::ignoreUnused (entryDepth, entryKey);

                            if (jam::Format::getPreColon (entryValue).trim()
                                == Id::templatePath.toString())
                            {
                                const juce::Identifier codeId {
                                    jam::Format::getPostColon (entryValue).trim()
                                };

                                if (templateDocument.getCodeBlock (codeId) == nullptr)
                                    return juce::Result::fail (
                                        getLocation (*table, *row, column.toString())
                                        + Id::diagnosticSeparator
                                        + text::Diagnostics::failTemplateMissing
                                        + Id::diagnosticSeparator + codeId.toString());
                            }
                        }

        return juce::Result::ok();
    }

    static juce::Result isReference (const Model& model)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                    for (const auto& column : { Id::placeholder, Id::structure })
                        for (auto& entry : model.getSource (*row, column))
                        {
                            const auto& [entryDepth, entryKey, entryValue] { entry };
                            juce::ignoreUnused (entryDepth, entryKey);

                            if (entryValue.startsWithChar (Chars::at))
                            {
                                const auto parts { jam::Strings::fromTokens (
                                    entryValue, juce::String::charToString (Chars::colon), {}) };
                                const auto aliasName { parts.at (0).trim() };

                                if (model.getValue (*row, aliasName).isEmpty())
                                    return juce::Result::fail (
                                        getLocation (*table, *row, column.toString())
                                        + Id::diagnosticSeparator
                                        + text::Diagnostics::failAliasMissing
                                        + Id::diagnosticSeparator + aliasName);

                                if (parts.size() > 1)
                                {
                                    const auto tableReference { parts.size() > 2
                                        ? aliasName + juce::String::charToString (Chars::colon)
                                              + parts.at (1).trim()
                                        : aliasName };
                                    auto* referencedTable { model.getTables (*row, tableReference) };

                                    if (referencedTable == nullptr)
                                        return juce::Result::fail (
                                            getLocation (*table, *row, column.toString())
                                            + Id::diagnosticSeparator
                                            + text::Diagnostics::failTableMissing
                                            + Id::diagnosticSeparator + tableReference);

                                    const auto columnName { parts.at (parts.size() - 1).trim() };

                                    if (not model.getTableHeaders (*referencedTable)
                                                .contains (columnName))
                                        return juce::Result::fail (
                                            getLocation (*table, *row, column.toString())
                                            + Id::diagnosticSeparator
                                            + text::Diagnostics::failColumnUnknown
                                            + Id::diagnosticSeparator + columnName);
                                }
                            }
                        }

        return juce::Result::ok();
    }

    static juce::Result
    isAssembled (const Model& model, const TemplateDocument& templateDocument)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                {
                    for (int depth { 0 }; model.getStructure (*row, depth).isNotEmpty(); ++depth)
                    {
                        const auto codeId { juce::Identifier (model.getStructure (*row, depth)) };
                        const auto nestedShapeId { model.getStructure (*row, depth + 1) };

                        if (nestedShapeId.isNotEmpty() and templateDocument.getPlaceholders().contains (codeId))
                        {
                            const auto placeholderKeys { model.getSource (*row, depth, Id::placeholder) };
                            const auto structureKeys { model.getSource (*row, depth, Id::structure) };

                            const auto hasUnbound { std::any_of (
                                templateDocument.getPlaceholders().at (codeId).begin(),
                                templateDocument.getPlaceholders().at (codeId).end(),
                                [&placeholderKeys, &structureKeys] (const juce::Identifier& name)
                                {
                                    return not placeholderKeys.contains (name)
                                           and not structureKeys.contains (name);
                                }) };

                            if (not hasUnbound)
                                return juce::Result::fail (
                                    getLocation (*table, *row, Id::structure.toString())
                                    + Id::diagnosticSeparator + text::Diagnostics::failOrphan
                                    + Id::diagnosticSeparator + nestedShapeId);
                        }
                    }
                }

        return juce::Result::ok();
    }

    static juce::Result isFormatted (const Model& model)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
            {
                const auto headers { model.getTableHeaders (*table) };

                if (headers.contains (Id::format.toString()))
                    for (auto* row : model.getTableRows (*table))
                    {
                        const auto operation { model.getTableValue (*row, Id::format) };

                        if (operation.isNotEmpty() and not Transforms::contains (operation))
                            return juce::Result::fail (
                                getLocation (*table, *row, Id::format.toString())
                                + Id::diagnosticSeparator + text::Diagnostics::failUnknownTransform
                                + Id::diagnosticSeparator + operation);
                    }
            }

        return juce::Result::ok();
    }

    static juce::Result isUnique (const Model& model)
    {
        const auto indexTables { model.getTables (Id::index) };
        jassert (not indexTables.isEmpty());

        const auto manifestOrigin { *indexTables.at (0)->get<juce::String> (Id::path) };

        for (auto* table : model.getTables())
        {
            const auto tableOrigin { *table->get<juce::String> (Id::path) };

            if (tableOrigin != manifestOrigin and not table->isTag (Id::index))
            {
                const auto headers { model.getTableHeaders (*table) };

                for (const auto& column : headers)
                {
                    if (column != Id::format.toString())
                    {
                        jam::Strings seen;

                        for (auto* row : model.getTableRows (*table))
                        {
                            auto* cell { model.getTableCell (*row, juce::Identifier (column)) };
                            jassert (cell != nullptr);

                            const auto rawText { *cell->get<juce::String> (Id::rawText) };

                            if (rawText.isNotEmpty() and not rawText.startsWithChar (Chars::at)
                                and not Transforms::contains (rawText))
                            {
                                const auto value { model.getValue (*row, juce::Identifier (column)) };

                                if (seen.contains (value, false))
                                    return juce::Result::fail (
                                        getLocation (*table, *row, column) + Id::diagnosticSeparator
                                        + text::Diagnostics::failDuplicate + value
                                        + juce::String::charToString (Chars::doubleQuote));

                                seen.add (value);
                            }
                        }
                    }
                }
            }
        }

        return juce::Result::ok();
    }

    static juce::Result isManifest (const Model& model)
    {
        if (const auto result { isIndex (model) }; not result.wasOk())
            return result;

        if (const auto result { unique (model, Id::alias.toString(), {}) }; not result.wasOk())
            return result;

        if (const auto result { isUnique (model) }; not result.wasOk())
            return result;

        if (const auto result { isFormatted (model) }; not result.wasOk())
            return result;

        const auto templateFile { model.getFile() };

        const TemplateDocument templateDocument {
            jam::MarkdownDocument::parse (templateFile.loadFileAsString())
        };

        if (const auto result { isStructure (model, templateDocument) }; not result.wasOk())
            return result;

        if (const auto result { isPlaceholders (model, templateDocument) }; not result.wasOk())
            return result;

        if (const auto result { isReference (model) }; not result.wasOk())
            return result;

        return isAssembled (model, templateDocument);
    }

    const Rules& getRules() const override
    {
        static const Rules rules {
            []
            {
                Rules rules;

                rules.add<const jam::Document&> (Id::index.toString(),
                    [] (const jam::Document& document) -> juce::Result
                    {
                        return isValid (static_cast<const Model&> (document));
                    });

                return rules;
            }()
        };

        return rules;
    }
};
