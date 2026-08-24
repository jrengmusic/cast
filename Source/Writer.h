#pragma once
#include <JuceHeader.h>
#include "Jobs.h"
#include "Model.h"
#include "Shapes.h"
#include "TemplateDocument.h"

/**
 * @struct Writer
 * @brief Renders every output table's rows into their declared files and
 *        writes them, one table at a time, one juce::ThreadPool job per
 *        output file.
 *
 * Each table's rows dispatch through Jobs::run(), one job per row index,
 * blocking until every file has written.
 */
struct Writer : jam::Document::Writer
{
    using jam::Document::Writer::toFile;

    /**
     * @brief Constructs a Writer bound to @p newModel's rows and
     *        @p newTemplateDocument's shapes.
     *
     * @param newModel            The model whose output tables are written.
     * @param newTemplateDocument The template document rows are rendered
     *                            against.
     */
    Writer (const Model& newModel, const TemplateDocument& newTemplateDocument)
        : model (newModel)
        , templateDocument (newTemplateDocument)
    {
    }

    /**
     * @brief Returns @p document's root subtree text, verbatim.
     *
     * @param document The document whose text is returned.
     * @returns @p document's root subtree text.
     */
    juce::String getText (const jam::Document& document) const override
    {
        return document.root->getAllSubText();
    }

    /**
     * @brief Renders and writes every output table's rows to their
     *        declared files under @p outputPath, one job per file.
     *
     * @param outputPath The directory output files are resolved against.
     * @returns juce::Result::ok() when every file wrote successfully, or a
     *          failure naming every file that failed to write.
     */
    juce::Result toFile (const juce::File& outputPath)
    {
        jam::Strings failures;

        for (auto* table : model.getTables())
        {
            if (model.isOutputTable (*table))
            {
                const auto rows { model.getTableRows (*table) };

                jam::Array<int> groupStarts;

                for (int index { 0 }; index < rows.size(); ++index)
                    if (index == 0
                        or model.getValue (*rows.at (index), Id::file)
                               != model.getValue (*rows.at (index - 1), Id::file))
                        groupStarts.add (index);

                jam::Array<juce::String> tableFailures;
                tableFailures.resize (groupStarts.size());

                Jobs::run (groupStarts.size(),
                    [this, &outputPath, &rows, &groupStarts, &tableFailures] (int index)
                    {
                        const auto start { groupStarts.at (index) };
                        const auto& file { model.getValue (*rows.at (start), Id::file) };

                        TemplateDocument output;
                        output.addChild (*output.root, Id::text)
                            ->add<juce::String> (Id::text, getBanner (file));
                        apply (output, rows, start);

                        const auto outputFile { jam::File::getOrCreate (outputPath, file) };

                        if (not toFile (output, outputFile))
                            tableFailures.at (index) = outputFile.getFullPathName();
                    });

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
        jam::Array<Model::Element*> groupRows;

        for (; index < rows.size() and model.getValue (*rows.at (index), Id::file) == file;
             ++index)
            groupRows.add (rows.at (index));

        output.addChild (*output.root, Id::text)
            ->add<juce::String> (
                Id::text, Shapes::getShape (model, templateDocument, groupRows, joinText));

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
