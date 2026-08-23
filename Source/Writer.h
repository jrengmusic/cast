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
        const auto templateFile { model.getFile() };

        templateDocument = std::make_unique<TemplateDocument> (
            jam::MarkdownDocument::parse (templateFile.loadFileAsString()));

        std::atomic<bool> written { true };

        for (auto* table : model.getTables())
        {
            if (model.isOutputTable (*table))
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

                    auto [fileEntry, wasInserted] { rowsByFile.try_emplace (file) };
                    juce::ignoreUnused (wasInserted);
                    fileEntry->second.add (row);
                }

                runJobs (files.size(),
                         [this, &outputPath, &files, &rowsByFile, &written] (int index)
                         {
                             const auto& file { files.at (index) };
                             const auto& fileRows { rowsByFile.at (file) };

                             TemplateDocument output;
                             output.addChild (*output.root, Id::text)
                                 ->add<juce::String> (Id::text, getBanner (file));
                             apply (output, fileRows);

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

    void apply (TemplateDocument& output, const Elements& fileRows) const
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
                ->add<juce::String> (Id::text,
                    templateDocument->getShape (model, *fileRows.at (index), 0));
        }

        output.addChild (*output.root, Id::text)
            ->add<juce::String> (Id::text, juce::String::charToString (Chars::newline));
    }

    juce::String getBanner (const juce::String& file) const noexcept
    {
        static const auto document { jam::MarkdownDocument::parse (
            BinaryData::getString (files::castOutput)) };

        const auto extension { jam::Format::onlyExtensionFromFilename (file) };
        const auto syntax { map::commentSyntax.get (extension) };
        juce::String banner;

        banner << syntax.get (Id::bannerOpen) << Chars::newline
               << document.getCodeBlock (Id::banner)->getAllSubText() << Chars::newline
               << syntax.get (Id::bannerClose) << Chars::newline << Chars::newline
               << syntax.get (Id::pragma) << Chars::newline << Chars::newline;

        return banner;
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Writer)
};
