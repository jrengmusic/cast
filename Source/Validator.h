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

    static juce::Result
    isAssembled (const Model& model, const TemplateDocument& templateDocument)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                {
                    const auto assembled { templateDocument.getShape (model, *row, 0) };

                    for (const auto& [blockId, names] : templateDocument.placeholders)
                    {
                        juce::ignoreUnused (blockId);

                        for (const auto& name : names)
                            if (jam::Format::hasPlaceholder (assembled, name.toString()))
                                return juce::Result::fail (
                                    getLocation (*table, *row, Id::structure.toString())
                                    + Id::diagnosticSeparator + text::Diagnostics::failNoSource
                                    + Id::diagnosticSeparator + name.toString());
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

        juce::File templateFile;

        for (auto* indexRow : model.getTableRows (Id::index))
        {
            const auto pathCell { model.getTableValue (*indexRow, Id::symbol) };

            if (juce::File::createFileWithoutCheckingPath (pathCell).hasFileExtension (
                    Extensions::cast))
                templateFile = model.getOutput (pathCell);
        }

        const TemplateDocument templateDocument {
            jam::MarkdownDocument::parse (templateFile.loadFileAsString())
        };

        if (const auto result { isStructure (model, templateDocument) }; not result.wasOk())
            return result;

        if (const auto result { isPlaceholders (model, templateDocument) }; not result.wasOk())
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
