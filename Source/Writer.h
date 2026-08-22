#pragma once
#include <JuceHeader.h>
#include "Model.h"
#include "TemplateDocument.h"

struct Writer : jam::Document::Writer
{
    using Elements = jam::Array<jam::Document::Element*>;
    using jam::Document::Writer::toFile;

    Writer (const Model& document)
        : model (document)
    {
    }

    juce::String getText (const jam::Document& document) const override
    {
        return document.root->getAllSubText();
    }

    bool toFile (const juce::File& outputPath)
    {
        juce::File templateFile;

        for (auto* indexRow : model.getTableRows (Id::index))
        {
            const auto pathCell { model.getTableValue (*indexRow, Id::symbol) };

            if (juce::File::createFileWithoutCheckingPath (pathCell).hasFileExtension (
                    Extensions::cast))
                templateFile = model.getOutput (pathCell);
        }

        templateDocument = std::make_unique<TemplateDocument> (
            jam::MarkdownDocument::parse (templateFile.loadFileAsString()));

        std::atomic<bool> written { true };

        for (auto* table : model.getTables())
        {
            const auto columns { model.getTableHeaders (*table) };

            if (columns.contains (Id::file.toString()) and table->id != Id::index)
            {
                const auto rows { model.getTableRows (*table) };

                jam::Array<juce::String> files;
                jam::HashMap<juce::String, Elements> rowsByFile;

                for (auto* row : rows)
                {
                    const auto origin { *row->parent->get<juce::String> (Id::path) };
                    const auto file { model.getValue (
                        origin, model.getTableValue (*row, Id::file)) };
                    files.addIfNotAlreadyThere (file);
                    rowsByFile[file].add (row);
                }

                runJobs (files.size(),
                         [this, &outputPath, &files, &rowsByFile, &written] (int index)
                         {
                             const auto& file { files.at (index) };
                             const auto& fileRows { rowsByFile.at (file) };

                             TemplateDocument output;
                             output.addChild (*output.root, Id::text)
                                 ->add<juce::String> (Id::text, getBanner (file));
                             getFile (output, fileRows);

                             const auto outputFile { jam::File::getOrCreate (outputPath, file) };

                             if (not toFile (output, outputFile))
                                 written = false;
                         });
            }
        }

        return written;
    }

private:
    const Model& model;
    std::unique_ptr<TemplateDocument> templateDocument;

    juce::String getRow (Model::Element& row) const
    {
        jam::Array<int> depths;

        for (int depth { 0 }; model.getStructure (row, depth).isNotEmpty(); ++depth)
            depths.add (depth);

        juce::String innerText;

        for (int index { depths.size() - 1 }; index >= 0; --index)
        {
            const auto depth { depths.at (index) };
            const auto shapeId { juce::Identifier (model.getStructure (row, depth)) };

            jam::HashMap<juce::Identifier, juce::String> injection;

            if (index < depths.size() - 1)
            {
                const auto dryBuild { templateDocument->build (
                    model, row, row, depth, {}, shapeId) };

                static const jam::Array<juce::Identifier> empty;
                const auto& candidates { templateDocument->placeholders.contains (shapeId)
                                              ? templateDocument->placeholders.at (shapeId)
                                              : empty };

                jam::Array<juce::Identifier> residue;

                for (const auto& candidate : candidates)
                    if (jam::Format::hasPlaceholder (dryBuild, candidate.toString()))
                        residue.addIfNotAlreadyThere (candidate);

                jassert (residue.size() == 1);

                if (residue.size() == 1)
                    injection.try_emplace (residue.first(), innerText);
                else
                    jam::debug::Log::write (jam::MarkdownValidator::getLocation (
                                                *row.parent, row, Id::structure.toString())
                                            + Id::diagnosticSeparator
                                            + text::Diagnostics::failNoMatch);
            }

            innerText = templateDocument->build (model, row, row, depth, injection, shapeId)
                            .trimCharactersAtEnd (juce::String::charToString (Chars::newline));
        }

        jassert (not innerText.contains (Id::tripleColon));

        if (innerText.contains (Id::tripleColon))
            jam::debug::Log::write (
                jam::MarkdownValidator::getLocation (*row.parent, row, Id::structure.toString())
                + Id::diagnosticSeparator + text::Diagnostics::failNoSource);

        return innerText;
    }

    void getFile (TemplateDocument& output, const Elements& fileRows) const
    {
        auto* firstRow { fileRows.first() };
        auto* separatorCell { model.getTableCell (*firstRow, Id::separator) };
        const auto hasJackEntries { separatorCell != nullptr
                                    and std::any_of (separatorCell->begin(),
                                                     separatorCell->end(),
                                                     [] (Model::Element* block)
                                                     {
                                                         return block->isTag (Id::ul);
                                                     }) };

        auto joinText { juce::String::charToString (Chars::newline) };

        if (separatorCell != nullptr and not hasJackEntries)
        {
            const auto flatValue { separatorCell->getAllSubText() };

            if (flatValue.isNotEmpty())
            {
                const auto resolved { templateDocument->getBinding (
                    model, *firstRow, 0, flatValue, Id::separator) };

                joinText = juce::String::charToString (Chars::newline)
                           + juce::String::charToString (Chars::newline) + resolved
                           + juce::String::charToString (Chars::newline)
                           + juce::String::charToString (Chars::newline);
            }
        }

        for (int index { 0 }; index < fileRows.size(); ++index)
        {
            if (index > 0)
                output.addChild (*output.root, Id::text)->add<juce::String> (Id::text, joinText);

            output.addChild (*output.root, Id::text)
                ->add<juce::String> (Id::text, getRow (*fileRows.at (index)));
        }
    }

    juce::String getBanner (const juce::String& file) const noexcept
    {
        static const auto document { jam::MarkdownDocument::parse (
            BinaryData::getString (files::castOutput)) };

        const auto extension { jam::Format::onlyExtensionFromFilename (file) };
        juce::String banner;

        const auto& syntax { map::commentSyntax.at (extension) };

        banner << syntax.at (Id::bannerOpen) << Chars::newline
               << document.getCodeBlock (Id::banner)->getAllSubText() << Chars::newline
               << syntax.at (Id::bannerClose) << Chars::newline << Chars::newline
               << syntax.at (Id::pragma) << Chars::newline << Chars::newline;

        return banner;
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Writer)
};
