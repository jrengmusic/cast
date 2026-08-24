#pragma once
#include <JuceHeader.h>
#include "Jobs.h"
#include "Model.h"
#include "Shapes.h"
#include "TemplateDocument.h"

struct Writer : jam::Document::Writer
{
    using jam::Document::Writer::toFile;

    Writer (const Model& document, const TemplateDocument& templateDocument)
        : model (document)
        , templateDocument (templateDocument)
    {
    }

    juce::String getText (const jam::Document& document) const override
    {
        return document.root->getAllSubText();
    }

    juce::Result toFile (const juce::File& outputPath)
    {
        jam::Strings failures;

        for (auto* table : model.getTables())
        {
            if (model.isOutputTable (*table))
            {
                const auto rows { model.getTableRows (*table) };

                jam::Array<juce::String> tableFailures;
                tableFailures.resize (rows.size());

                for (int index { 0 }; index < rows.size(); ++index)
                    tablePool.addJob (
                        [this, &outputPath, &rows, &tableFailures, index]
                        {
                            const auto& file { model.getValue (*rows.at (index), Id::file) };

                            if (index == 0
                                or file != model.getValue (*rows.at (index - 1), Id::file))
                            {
                                TemplateDocument output;
                                output.addChild (*output.root, Id::text)
                                    ->add<juce::String> (Id::text, getBanner (file));
                                apply (output, rows, index);

                                const auto outputFile { jam::File::getOrCreate (outputPath, file) };

                                if (not toFile (output, outputFile))
                                    tableFailures.at (index) = outputFile.getFullPathName();
                            }
                        });

                tablePool.removeAllJobs (false, Jobs::indefiniteTimeoutMs);

                for (const auto& failedFile : tableFailures)
                    if (failedFile.isNotEmpty())
                        failures.add (failedFile + Id::diagnosticSeparator
                                     + text::Diagnostics::failOutputWrite);
            }
        }

        if (failures.size() > 0)
            return juce::Result::fail (
                failures.joinIntoString (juce::String::charToString (Chars::newline), 0, -1));

        return juce::Result::ok();
    }

private:
    const Model& model;
    const TemplateDocument& templateDocument;
    juce::ThreadPool tablePool;

    void apply (TemplateDocument& output, const jam::Array<Model::Element*>& rows, int index) const
    {
        auto* firstRow { rows.at (index) };
        auto joinText { juce::String::charToString (Chars::newline) };

        if (auto* separatorCell { model.getTableCell (*firstRow, Id::separator) })
        {
            const auto hasTokenEntries { std::any_of (separatorCell->begin(),
                                                     separatorCell->end(),
                                                     [] (Model::Element* block)
                                                     {
                                                         return block->isTag (Id::ul);
                                                     }) };

            if (not hasTokenEntries)
            {
                const auto& value { *separatorCell->get<juce::String> (Id::value) };

                if (value.isNotEmpty())
                {
                    const auto binding { templateDocument.getBinding (
                        model, *firstRow, value) };

                    joinText = juce::String::charToString (Chars::newline)
                               + juce::String::charToString (Chars::newline) + binding
                               + juce::String::charToString (Chars::newline)
                               + juce::String::charToString (Chars::newline);
                }
            }
        }

        const auto& file { model.getValue (*firstRow, Id::file) };

        for (; index < rows.size() and model.getValue (*rows.at (index), Id::file) == file;
             ++index)
        {
            if (rows.at (index) != firstRow)
                output.addChild (*output.root, Id::text)->add<juce::String> (Id::text, joinText);

            output.addChild (*output.root, Id::text)
                ->add<juce::String> (Id::text,
                    Shapes::getShape (model, templateDocument, *rows.at (index),
                        juce::Identifier (model.getStructure (
                            *model.getTableCell (*rows.at (index), Id::structure))),
                        model.getTableCell (*rows.at (index), Id::structure),
                        model.getTableCell (*rows.at (index), Id::placeholder),
                        model.getTableCell (*rows.at (index), Id::separator),
                        juce::String()));
        }

        output.addChild (*output.root, Id::text)
            ->add<juce::String> (Id::text, juce::String::charToString (Chars::newline));
    }

    juce::String getBanner (const juce::String& file) const
    {
        static const auto document { jam::MarkdownDocument::parse (
            BinaryData::getString (files::castOutput)) };

        const auto extension { jam::Format::onlyExtensionFromFilename (file) };
        const auto& syntax { map::commentSyntax.get (extension) };
        juce::String banner;

        if (auto* bannerBlock { document.getCodeBlock (Id::banner) })
            banner << syntax.get (Id::bannerOpen) << Chars::newline
                   << bannerBlock->getAllSubText() << Chars::newline
                   << syntax.get (Id::bannerClose) << Chars::newline << Chars::newline
                   << syntax.get (Id::pragma) << Chars::newline << Chars::newline;

        return banner;
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Writer)
};
