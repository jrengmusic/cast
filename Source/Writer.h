#pragma once
#include <JuceHeader.h>
#include "Items.h"
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
        static const auto newlineText { juce::String::charToString (Chars::newline) };

        jam::Strings failures;
        const auto tables { model.getTables() };

        for (auto* table : tables)
            if (model.isOutputTable (*table))
                failures.addArray (toFile (*table, outputPath), 0, -1);

        if (failures.size() > 0)
            return juce::Result::fail (failures.joinIntoString (newlineText, 0, -1));

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
                       .compare (model.getValue (*rows.at (index - 1), Id::file)) != 0)
                groupStarts.add (index);

        return groupStarts;
    }

    /**
     * @brief Resolves and creates every output-file group's own file --
     *        each @p groupStarts' own row's declared @c file, resolved
     *        against @p outputPath.
     *
     * @param outputPath   The directory output files are resolved against.
     * @param rows         The table's own rows, read for each group's own
     *                     declared @c file.
     * @param groupStarts  Each group's own first index into @p rows.
     * @returns Each group's own resolved, directory-created output file,
     *          in @p groupStarts' own order.
     */
    jam::Array<juce::File> getOutputFiles (const juce::File& outputPath,
        const jam::Array<Model::Element*>& rows, const jam::Array<int>& groupStarts) const
    {
        jam::Array<juce::File> outputFiles;
        outputFiles.resize (groupStarts.size());

        for (int index { 0 }; index < groupStarts.size(); ++index)
        {
            const auto& file { model.getValue (*rows.at (groupStarts.at (index)), Id::file) };
            outputFiles.at (index) = jam::File::getOrCreate (outputPath, file);
            outputFiles.at (index).getParentDirectory().createDirectory();
        }

        return outputFiles;
    }

    /**
     * @brief Renders and writes one output-file group's own rows through
     *        apply(), framed by its own banner and file-header comment,
     *        write-if-different. The group's own comment-syntax key is
     *        read from its own first shape line's fence prefix when one
     *        is authored -- @c \[no-banner\] excepted -- or resolved
     *        through Transforms::getCommentSyntaxKey() otherwise. When
     *        any of the group's own rows carries an @c \[no-banner\]
     *        fence, no banner renders; otherwise the rendered banner
     *        either substitutes a @c :::\[banner\]::: marker authored
     *        anywhere in the group's own shape text, or, absent one, is
     *        prepended before it. When the banner is empty and a marker
     *        is authored, the marker's own line is dropped rather than
     *        left blank.
     *
     * @param rows        The table's own rows, sliced to @p index's own
     *                    group by @p groupStarts.
     * @param tables      The tables searched when a nested @c list token
     *                    expands.
     * @param groupStarts Each group's own first index into @p rows.
     * @param outputFiles Each group's own resolved output file, index by
     *                    index with @p groupStarts.
     * @param index       The group's own index into @p groupStarts and
     *                    @p outputFiles.
     * @returns @p index's own resolved output file's full path when it
     *          needed rewriting and the write failed, or an empty string
     *          when its text was already canonical or wrote successfully.
     */
    juce::String toFile (const jam::Array<Model::Element*>& rows,
        const jam::Array<Model::Element*>& tables, const jam::Array<int>& groupStarts,
        const jam::Array<juce::File>& outputFiles, int index) const
    {
        static const auto dotText { juce::String::charToString (Chars::dot) };

        const auto start { groupStarts.at (index) };
        const auto& outputFile { outputFiles.at (index) };
        const auto groupEnd { index + 1 < groupStarts.size() ? groupStarts.at (index + 1) : rows.size() };

        auto* firstLine { Shapes::getFirstLine (model, *rows.at (start)) };
        const auto fencePrefix { Transforms::getFencePrefix (*firstLine->get<juce::String> (Id::info)) };
        const auto noBanner { Id::noBanner.toString() };
        const auto extension { fencePrefix.isNotEmpty() and fencePrefix.compare (noBanner) != 0
                                    ? dotText + fencePrefix
                                    : Transforms::getCommentSyntaxKey (outputFile.getFileName()) };

        auto comment { getFileComment (*rows.at (start), outputFile.getFileName()) };

        if (comment.isNotEmpty())
            comment = Transforms::toCommentBlock (comment, extension);

        Model::Element* noBannerLine { nullptr };

        for (int rowIndex { start }; rowIndex < groupEnd and noBannerLine == nullptr; ++rowIndex)
            for (const auto& column : { Id::structure, Id::separator, Id::list })
            if (auto* structureScope { model.getTableCell (*rows.at (rowIndex), column) })
                structureScope->applyFunctionRecursively (
                    [&noBannerLine, &noBanner] (const Model::Element& candidate) -> bool
                    {
                        if (noBannerLine == nullptr and candidate.contains (Id::templatePath)
                            and Transforms::getFencePrefix (*candidate.get<juce::String> (Id::info))
                                    .compare (noBanner) == 0)
                            noBannerLine = const_cast<Model::Element*> (&candidate);

                        return noBannerLine == nullptr;
                    });

        const auto banner { noBannerLine != nullptr ? juce::String{} : getBanner (extension, comment) };

        jam::MarkdownDocument rendered;
        apply (rendered, tables, rows, start, groupEnd, extension);

        jam::MarkdownDocument output;
        output.addChild (*output.root, Id::text)
            ->add<juce::String> (Id::text, getBody (getText (rendered), banner));

        const auto current { outputFile.loadFileAsString() };
        const auto canonical { getText (output) };

        return canonical.compare (current) != 0 and not toFile (output, outputFile)
                   ? outputFile.getFullPathName()
                   : juce::String{};
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
        const auto outputFiles { getOutputFiles (outputPath, rows, groupStarts) };

        jam::Array<juce::String> tableFailures;
        tableFailures.resize (groupStarts.size());

        Jobs::run (groupStarts.size(),
            [this, &rows, &groupStarts, &tables, &tableFailures, &outputFiles] (int index)
            { tableFailures.at (index) = toFile (rows, tables, groupStarts, outputFiles, index); });

        for (const auto& failedFile : tableFailures)
            if (failedFile.isNotEmpty())
                failures.add (failedFile + Id::diagnosticSeparator
                             + text::Diagnostics::failOutputWrite);

        return failures;
    }

    /**
     * @brief Resolves @p firstRow's separator column's row join, when it
     *        declares one.
     *
     * @param firstRow The output-file group's own first row, whose
     *                 separator column's row join is resolved.
     * @returns The resolved row-join text, or an empty string when
     *          @p firstRow's separator column declares no row join.
     */
    juce::String getRowJoin (Model::Element& firstRow) const
    {
        auto* rowJoinLine { model.getRowJoin (firstRow) };

        if (rowJoinLine != nullptr)
            return templateDocument.getValue (
                model, firstRow, *rowJoinLine->get<juce::String> (Id::value));

        return {};
    }

    /**
     * @brief Resolves @p firstRow's own file-header comment -- @p firstRow's
     *        structure-column @c comment binding, paired with its own first
     *        shape line, read as a wired table address when its value is
     *        @-sigiled. A plain-text @c comment binding at that position is
     *        the item-prose channel (SPEC §6.8), never file documentation,
     *        and resolves empty here.
     *
     * @pre When @p firstRow's @c comment binding is @-sigiled, it resolves
     *      to a table -- established once by Validator::isReference()
     *      before the writer ever runs.
     *
     * @param firstRow The output-file group's own first row, whose
     *                 @c comment binding is resolved.
     * @param file     The group's own output file, matched against the
     *                 addressed table's @c file column when @p firstRow's
     *                 @c comment binding is a table address.
     * @returns The resolved comment text, or an empty string when
     *          @p firstRow declares no @-sigiled @c comment binding, or the
     *          addressed table carries no row for @p file.
     */
    juce::String getFileComment (Model::Element& firstRow, const juce::String& file) const
    {
        auto* firstLine { Shapes::getFirstLine (model, firstRow) };

        static const juce::Identifier commentMarker { jam::Format::toValidID (
            jam::Format::withEnclosure (Id::comment.toString(), Chars::openBracket)) };

        auto* commentBinding { model.getBinding (firstRow, Id::structure, *firstLine, commentMarker) };

        if (commentBinding != nullptr)
        {
            const auto& commentValue { *commentBinding->get<juce::String> (Id::value) };

            if (Model::isAddress (commentValue))
            {
                auto* headerTable { model.getTable (firstRow, commentValue) };

                const auto fileName { jam::Format::toFileName (file) };
                const auto column { model.isColumnAddress (firstRow, commentValue)
                                         ? model.getColumn (firstRow, commentValue)
                                         : Id::comment };

                for (auto* headerRow : model.getTableRows (*headerTable))
                {
                    auto* fileCell { model.getTableCell (*headerRow, Id::file) };

                    if (fileCell != nullptr
                        and jam::Format::toFileName (*fileCell->get<juce::String> (Id::value))
                                .compare (fileName)
                            == 0)
                        if (auto* headerCommentCell { model.getTableCell (*headerRow, column) })
                            return *headerCommentCell->get<juce::String> (Id::value);
                }
            }
        }

        return {};
    }

    /**
     * @brief Renders @p rows' own [@p index, @p groupEnd) slice through
     *        Shapes::getShape(), then appends the rendered text and a
     *        trailing newline as text children of @p output's root.
     *
     * @param output    The document the rendered text is appended to.
     * @param tables    The tables searched when a nested @c list token
     *                  expands.
     * @param rows      The output-file group's own rows.
     * @param index     The group's own first index into @p rows.
     * @param groupEnd  The index one past the group's own last row in
     *                  @p rows.
     * @param extension The target file extension a comment value is
     *                  commented for.
     */
    void apply (jam::MarkdownDocument& output, const jam::Array<Model::Element*>& tables,
               const jam::Array<Model::Element*>& rows, int index, int groupEnd,
               const juce::String& extension) const
    {
        static const auto newlineText { juce::String::charToString (Chars::newline) };

        auto* firstRow { rows.at (index) };
        const auto rowJoinValue { getRowJoin (*firstRow) };
        const auto joinText { rowJoinValue.isNotEmpty()
                                  ? newlineText
                                        + Chars::newline + rowJoinValue
                                        + Chars::newline
                                        + Chars::newline
                                  : newlineText };

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
            ->add<juce::String> (Id::text, newlineText);
    }

    /**
     * @brief Composes the output body from @p shapeText and @p banner --
     *        @p banner substituted at a @c :::\[banner\]::: marker
     *        authored anywhere in @p shapeText, or, absent one, prepended
     *        before it. When @p banner is empty and a marker is authored,
     *        the marker's own line is dropped rather than left blank.
     *
     * @param shapeText The rendered shape text @p banner is composed into.
     * @param banner    The rendered banner text, or an empty string when
     *                  no banner renders.
     * @returns @p shapeText framed by @p banner.
     */
    static juce::String getBody (const juce::String& shapeText, const juce::String& banner)
    {
        static const auto newlineText { juce::String::charToString (Chars::newline) };
        static const juce::Identifier bannerMarker { jam::Format::toValidID (
            jam::Format::withEnclosure (Id::banner.toString(), Chars::openBracket)) };

        const auto marker { Items::getMarker (shapeText, bannerMarker) };

        if (marker.isEmpty())
            return banner.isNotEmpty() ? banner + Chars::newline
                                             + Chars::newline
                                             + shapeText
                                       : shapeText;

        const auto substituted { jam::Format::replaceholder (shapeText,
            marker.substring (Id::tripleColon.length(), marker.length() - Id::tripleColon.length()),
            banner) };

        if (banner.isNotEmpty())
            return substituted;

        const auto shapeLines { jam::Strings::fromLines (shapeText) };
        const auto substitutedLines { jam::Strings::fromLines (substituted) };
        jam::Strings bodyLines;

        for (int lineIndex { 0 }; lineIndex < substitutedLines.size(); ++lineIndex)
            if (not (shapeLines.at (lineIndex).contains (marker)
                     and substitutedLines.at (lineIndex).trim().isEmpty()))
                bodyLines.add (substitutedLines.at (lineIndex));

        return bodyLines.joinIntoString (newlineText, 0, -1);
    }

    /**
     * @brief Renders the cast-output banner for @p extension, framed by
     *        @p extension's own @c Id::bannerOpen / @c Id::bannerClose
     *        when declared, or wrapped line-by-line through
     *        Transforms::toCommentBlock() when @p extension declares no
     *        banner frame, followed by @p comment when it is not empty.
     *        The result carries no trailing newline -- the caller
     *        supplies its own separator before the shape text that
     *        follows.
     *
     * @param extension The target file extension the banner is framed
     *                  for.
     * @param comment   The file's own header comment, appended after the
     *                  banner when not empty.
     * @returns The rendered banner, or an empty string when the parsed
     *          banner document declares no @c banner code block.
     */
    juce::String getBanner (const juce::String& extension, const juce::String& comment) const
    {
        static const auto document { jam::MarkdownDocument::parse (
            BinaryData::getString (files::castOutput)) };

        const auto& syntax { map::commentSyntax.at (extension) };
        juce::String banner;

        if (auto* bannerBlock { document.getCodeBlock (Id::banner) })
        {
            banner << (syntax.get (Id::bannerOpen).isNotEmpty()
                           ? syntax.get (Id::bannerOpen)
                                 + Chars::newline
                                 + bannerBlock->getAllSubText()
                                 + Chars::newline
                                 + syntax.get (Id::bannerClose)
                           : Transforms::toCommentBlock (bannerBlock->getAllSubText(), extension));

            if (comment.isNotEmpty())
                banner << Chars::newline << Chars::newline << comment;
        }

        return banner;
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Writer)
};
