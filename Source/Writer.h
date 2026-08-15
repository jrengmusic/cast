#pragma once
#include <JuceHeader.h>
#include "Model.h"
#include "TemplateDocument.h"

struct Writer
{
    using Elements = jam::Array<jam::Document::Element*>;

    Writer (const Model& document)
        : model (document)
    {
    }

    bool toFile (const juce::File& outputPath)
    {
        std::atomic<bool> written { true };

        for (auto* table : model.getTables())
        {
            const auto columns { model.getTableHeaders (*table) };

            if (columns.contains (Id::file.toString()) and table->id != Id::index)
            {
                const auto rows { model.getTableRows (*table) };
                const auto& bodyColumn { *columns.begin() };

                jam::Array<juce::String> files;
                jam::HashMap<juce::String, Elements> rowsByFile;

                for (auto* row : rows)
                {
                    const auto origin { *row->parent->get<juce::String> (Id::path) };
                    const auto file {
                        model.getValue (origin, model.getTableValue (*row, Id::file))
                    };
                    files.addIfNotAlreadyThere (file);
                    rowsByFile[file].add (row);
                }

                runJobs (
                    files.size(),
                    [this,
                     &outputPath,
                     &files,
                     &rowsByFile,
                     &columns,
                     &bodyColumn,
                     &written] (int index)
                    {
                        const auto& file { files.at (index) };
                        const auto& fileRows { rowsByFile.at (file) };
                        auto* firstRow { fileRows.first() };

                        juce::String code;
                        jam::Array<juce::Identifier> placeholders;

                        for (auto* row : fileRows)
                        {
                            const auto bodyCell { model.getTableValue (
                                *row, juce::Identifier (bodyColumn)) };
                            const auto& document { TemplateDocument::getOrCreate (
                                model.getFile (*row, bodyCell)) };
                            const auto body {
                                document.build (model, *row, {}).root->getAllSubText()
                            };

                            if (row == firstRow)
                                placeholders = document.getPlaceholders (model, *row);

                            if (code.isEmpty())
                                code = body;
                            else
                            {
                                const auto lineBreakCell { model.getTableValue (
                                    *row, Id::lineBreak) };
                                const auto separator {
                                    lineBreakCell.isNotEmpty()
                                        ? TemplateDocument::getOrCreate (
                                              model.getFile (*row, lineBreakCell))
                                              .build (model, *row, {})
                                              .root->getAllSubText()
                                        : juce::String()
                                };

                                code << separator << body;
                            }
                        }

                        for (const auto& column : columns)
                        {
                            const auto cell { model.getTableValue (
                                *firstRow, juce::Identifier (column)) };

                            if (column != bodyColumn and column != Id::file.toString()
                                and not column.endsWith (Id::lineBreak.toString())
                                and not placeholders.contains (juce::Identifier (column))
                                and model.isTemplatePath (*firstRow, cell))
                            {
                                const auto& wrapper { TemplateDocument::getOrCreate (
                                    model.getFile (*firstRow, cell)) };

                                for (const auto& placeholder :
                                     wrapper.getPlaceholders (model, *firstRow))
                                    placeholders.addIfNotAlreadyThere (placeholder);

                                code =
                                    wrapper.build (model, *firstRow, {}, code).root->getAllSubText();
                            }
                        }

                        auto outputFile { jam::File::getOrCreate (outputPath, file) };
                        written = outputFile.replaceWithText (
                            getBanner (file) + code,
                            false,
                            false,
                            juce::String::charToString (chars::newline).toRawUTF8());
                    });
            }
        }

        return written;
    }

private:
    const Model& model;

    juce::String getBanner (const juce::String& file) const noexcept
    {
        static const auto document { jam::MarkdownDocument::parse (
            BinaryData::getString (files::castOutput)) };

        const auto extension { jam::Format::onlyExtensionFromFilename (file) };
        juce::String banner;

        const auto& syntax { map::commentSyntax.at (extension) };

        banner << syntax.at (Id::bannerOpen) << chars::newline
               << document.getCodeBlock (Id::banner)->getAllSubText() << chars::newline
               << syntax.at (Id::bannerClose) << chars::newline
               << chars::newline << syntax.at (Id::pragma)
               << chars::newline << chars::newline;

        return banner;
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Writer)
};
