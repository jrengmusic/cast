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
 * Each table's rows are grouped by their declared output file, and each
 * group dispatches through Jobs::run(), one job per output-file group,
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
     * @brief Writes every output table's rows to their declared files
     *        under @p outputPath, delegating each table to the private
     *        toFile() overload.
     *
     * @param outputPath The directory output files are resolved against.
     * @returns juce::Result::ok() when every file wrote successfully, or a
     *          failure naming every file that failed to write.
     */
    juce::Result toFile (const juce::File& outputPath) const
    {
        jam::Strings failures;
        const auto tables { model.getTables() };

        for (auto* table : tables)
            if (model.isOutputTable (*table))
                failures.addArray (toFile (*table, outputPath), 0, -1);

        if (failures.size() > 0)
            return juce::Result::fail (
                failures.joinIntoString (juce::String::charToString (Chars::newline), 0, -1));

        return juce::Result::ok();
    }

private:
    const Model& model;
    const TemplateDocument& templateDocument;

    /**
     * @brief Returns the index of @p rows' first row in every run of
     *        consecutive rows sharing the same declared output file.
     *
     * @param rows The rows searched for output-file group boundaries.
     * @returns Every group's first index, in @p rows' own order.
     */
    jam::Array<int> getGroupStarts (const jam::Array<Model::Element*>& rows) const
    {
        jam::Array<int> groupStarts;

        for (int index { 0 }; index < rows.size(); ++index)
            if (index == 0
                or model.getValue (*rows.at (index), Id::file)
                       != model.getValue (*rows.at (index - 1), Id::file))
                groupStarts.add (index);

        return groupStarts;
    }

    /**
     * @brief Renders and writes @p table's rows to their declared files
     *        under @p outputPath, one juce::ThreadPool job per
     *        output-file group.
     *
     * @param table      The output table whose rows are written.
     * @param outputPath The directory output files are resolved against.
     * @returns Every failed file's diagnostic line, or an empty
     *          jam::Strings when every file wrote successfully.
     */
    jam::Strings toFile (Model::Element& table, const juce::File& outputPath) const
    {
        jam::Strings failures;
        const auto tables { model.getTables() };
        const auto rows { model.getTableRows (table) };
        const auto groupStarts { getGroupStarts (rows) };

        jam::Array<juce::String> tableFailures;
        tableFailures.resize (groupStarts.size());

        for (auto start : groupStarts)
        {
            const auto& file { model.getValue (*rows.at (start), Id::file) };
            const auto outputFile { jam::File::getOrCreate (outputPath, file) };
            outputFile.getParentDirectory().createDirectory();
        }

        const auto indexCommentTables { model.getTables (Id::indexComment) };
        auto* indexCommentTable { indexCommentTables.isEmpty() ? nullptr : indexCommentTables.at (0) };

        Jobs::run (groupStarts.size(),
            [this, &outputPath, &rows, &groupStarts, &tables, &tableFailures, indexCommentTable] (int index)
            {
                const auto start { groupStarts.at (index) };
                const auto& file { model.getValue (*rows.at (start), Id::file) };
                const auto extension {
                    juce::File::createFileWithoutCheckingPath (file).getFileExtension()
                };

                juce::String comment;

                if (indexCommentTable != nullptr)
                {
                    const auto& aliasText { *model.getTableCell (*rows.at (start), Id::file)
                                                  ->get<juce::String> (Id::rawText) };

                    if (auto* commentCell { model.getTableCell (
                            *indexCommentTable, Id::comment, juce::Identifier (aliasText)) })
                        if (commentCell->contains (Id::value))
                            comment = *commentCell->get<juce::String> (Id::value);
                }

                if (comment.isNotEmpty())
                    comment = Transforms::toCommentBlock (comment, extension);

                TemplateDocument output;
                output.addChild (*output.root, Id::text)
                    ->add<juce::String> (Id::text, getBanner (extension, comment));

                const auto groupEnd { index + 1 < groupStarts.size()
                                          ? groupStarts.at (index + 1)
                                          : rows.size() };

                apply (output, tables, rows, start, groupEnd, extension);

                const auto outputFile { jam::File::getOrCreate (outputPath, file) };
                const auto current { outputFile.loadFileAsString() };
                const auto canonical { getText (output) };

                if (canonical != current and not toFile (output, outputFile))
                    tableFailures.at (index) = outputFile.getFullPathName();
            });

        for (const auto& failedFile : tableFailures)
            if (failedFile.isNotEmpty())
                failures.add (failedFile + Id::diagnosticSeparator
                             + text::Diagnostics::failOutputWrite);

        return failures;
    }

    juce::String getRowJoin (Model::Element& firstRow) const
    {
        auto* rowJoinLine { model.getSeparator (firstRow, 0, 0) };

        if (rowJoinLine != nullptr)
            return templateDocument.getBinding (
                model, firstRow, *rowJoinLine->get<juce::String> (Id::value));

        return {};
    }

    void apply (TemplateDocument& output, const jam::Array<Model::Element*>& tables,
               const jam::Array<Model::Element*>& rows, int index, int groupEnd,
               const juce::String& extension) const
    {
        auto* firstRow { rows.at (index) };
        const auto rowJoinValue { getRowJoin (*firstRow) };
        const auto joinText { rowJoinValue.isNotEmpty()
                                  ? juce::String::charToString (Chars::newline)
                                        + juce::String::charToString (Chars::newline) + rowJoinValue
                                        + juce::String::charToString (Chars::newline)
                                        + juce::String::charToString (Chars::newline)
                                  : juce::String::charToString (Chars::newline) };

        jam::Array<Model::Element*> groupRows;
        jam::Array<Model::Element*> groupLines;

        for (; index < groupEnd; ++index)
        {
            auto* row { rows.at (index) };
            groupRows.add (row);
            groupLines.add (Shapes::getFirstLine (model, *row));
        }

        output.addChild (*output.root, Id::text)
            ->add<juce::String> (Id::text,
                Shapes::getShape (model, templateDocument, tables, groupRows, groupLines, joinText,
                    0, extension));

        output.addChild (*output.root, Id::text)
            ->add<juce::String> (Id::text, juce::String::charToString (Chars::newline));
    }

    juce::String getBanner (const juce::String& extension, const juce::String& comment) const
    {
        static const auto document { jam::MarkdownDocument::parse (
            BinaryData::getString (files::castOutput)) };

        const auto& syntax { map::commentSyntax.at (extension) };
        juce::String banner;

        if (auto* bannerBlock { document.getCodeBlock (Id::banner) })
        {
            banner << syntax.get (Id::bannerOpen) << Chars::newline
                   << bannerBlock->getAllSubText() << Chars::newline
                   << syntax.get (Id::bannerClose) << Chars::newline << Chars::newline;

            if (comment.isNotEmpty())
                banner << comment << Chars::newline << Chars::newline;
        }

        return banner;
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Writer)
};
