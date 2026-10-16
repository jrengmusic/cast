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
    Writer (const Model& newModel, const TemplateDocument& newTemplateDocument);

    /**
     * @brief Returns @p document's root subtree text, verbatim.
     *
     * @param document The document whose text is returned.
     * @returns @p document's root subtree text.
     */
    juce::String getText (const jam::Document& document) const override;

    /**
     * @brief Writes every output table's rows to their declared files
     *        under @p outputPath, delegating each table to the private
     *        toFile() overload.
     *
     * @param outputPath The directory output files are resolved against.
     * @returns juce::Result::ok() when every file wrote successfully, or a
     *          failure naming every file that failed to write.
     */
    juce::Result toFile (const juce::File& outputPath) const;

private:
    /** The model whose declared outputs are rendered. */
    const Model& model;
    /** The template document each output row's shapes are read from. */
    const TemplateDocument& templateDocument;

    /**
     * @brief Returns the index of @p rows' first row in every run of
     *        consecutive rows sharing the same declared output file.
     *
     * @param rows The rows searched for output-file group boundaries.
     * @returns Every group's first index, in @p rows' own order.
     */
    jam::Array<int> getGroupStarts (const jam::Array<const Model::Element*>& rows) const;

    /**
     * @brief Resolves every output-file group's own file -- each
     *        @p groupStarts' own row's declared @c file, resolved against
     *        @p outputPath. A whole-file group's own file is created when
     *        absent; a region group's own file is resolved
     *        without creating it -- the row does not create the file it
     *        patches, and Validator::isRegionFilePresent()
     *        has already established that it exists.
     *
     * @param outputPath   The directory output files are resolved against.
     * @param rows         The table's own rows, read for each group's own
     *                     declared @c file.
     * @param groupStarts  Each group's own first index into @p rows.
     * @returns Each group's own resolved output file, in @p groupStarts'
     *          own order.
     */
    jam::Array<juce::File> getOutputFiles (const juce::File& outputPath,
        const jam::Array<const Model::Element*>& rows, const jam::Array<int>& groupStarts) const;

    /**
     * @brief Delegates a region group -- one whose first row carries both
     *        the @c \[begin\] and @c \[end\] bindings (Model::isRegionRow())
     *        -- to toRegionFile(); otherwise renders and writes the
     *        group's own rows through apply(), framed by its own banner
     *        and file-header description, write-if-different. The group's own
     *        comment-syntax key is read from its own first shape line's
     *        fence prefix when one is authored -- @c \[no-banner\]
     *        excepted -- or resolved through
     *        Transforms::getCommentSyntaxKey() otherwise. When any of the
     *        group's own rows carries an @c \[no-banner\] fence, no
     *        banner renders; otherwise the rendered banner either
     *        substitutes a @c :::\[banner\]::: marker authored anywhere
     *        in the group's own shape text, or, absent one, is prepended
     *        before it. When the banner is empty and a marker is
     *        authored, the marker's own line is dropped rather than left
     *        blank.
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
     * @returns toRegionFile()'s own result for a region group; otherwise
     *          @p index's own resolved output file's full path when it
     *          needed rewriting and the write failed, or an empty string
     *          when its text was already canonical or wrote successfully.
     */
    juce::String toFile (const jam::Array<const Model::Element*>& rows,
        const jam::Array<const Model::Element*>& tables, const jam::Array<int>& groupStarts,
        const jam::Array<juce::File>& outputFiles, int index) const;

    /**
     * @brief Finds the end of the run of consecutive region rows that
     *        resolve the same @c \[begin\] and @c \[end\] values as the
     *        row at @p runStart.
     *
     * @param rows       The table's own rows.
     * @param runStart   The index of the run's own first row in @p rows.
     * @param groupEnd   The index one past the file group's own last row in
     *                   @p rows.
     * @param beginValue The run's own resolved @c \[begin\] value.
     * @param endValue   The run's own resolved @c \[end\] value.
     * @returns The index one past the run's own last row.
     */
    int getRunEnd (const jam::Array<const Model::Element*>& rows, int runStart, int groupEnd,
        const juce::String& beginValue, const juce::String& endValue) const;

    /**
     * @brief Splices the rendered shape of one run of region rows into
     *        @p lines, between the run's own delimiter lines.
     *
     * @param lines      The region file's own content, split into lines.
     * @param tables     The tables searched when a nested @c list token
     *                   expands.
     * @param rows       The table's own rows.
     * @param runStart   The index of the run's own first row in @p rows.
     * @param runEnd     The index one past the run's own last row in
     *                   @p rows.
     * @param extension  The target file extension the shape renders for.
     * @param beginValue The run's own resolved @c \[begin\] value.
     * @param endValue   The run's own resolved @c \[end\] value.
     * @returns @p lines with the run's own region replaced by the rendered
     *          shape.
     */
    jam::Strings getSplicedLines (const jam::Strings& lines, const jam::Array<const Model::Element*>& tables,
        const jam::Array<const Model::Element*>& rows, int runStart, int runEnd, const juce::String& extension,
        const juce::String& beginValue, const juce::String& endValue) const;

    /**
     * @brief Splices @p rows' own rendered shapes into @p outputFile's own
     *        regions -- the lines strictly between each @c \[begin\] and
     *        @c \[end\] delimiter pair -- write-if-different, every line
     *        outside the regions kept byte-for-byte. Consecutive rows that
     *        resolve the same pair render into one region; the regions
     *        apply in manifest order. No banner and no file documentation
     *        render; Validator has already established each region's
     *        pairing, the file's existence, the delimiters' own presence
     *        and order, and that the regions do not overlap, so this
     *        trusts all of it unconditionally.
     *
     * @param rows          The table's own rows, sliced to the group's
     *                      own [@p start, @p groupEnd) range.
     * @param tables        The tables searched when a nested @c list
     *                      token expands.
     * @param start         The group's own first index into @p rows.
     * @param groupEnd      The index one past the group's own last row in
     *                      @p rows.
     * @param outputFile    The region file patched, already resolved
     *                      without creation by getOutputFiles().
     * @param extension     The target file extension the shape renders
     *                      for.
     * @returns @p outputFile's own full path when it needed rewriting and
     *          the write failed, or an empty string when its text was
     *          already canonical or wrote successfully.
     */
    juce::String toRegionFile (const jam::Array<const Model::Element*>& rows,
        const jam::Array<const Model::Element*>& tables, int start, int groupEnd,
        const juce::File& outputFile, const juce::String& extension) const;

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
    jam::Strings toFile (const Model::Element& table, const juce::File& outputPath) const;

    /**
     * @brief Resolves @p firstRow's separator column's row join, when it
     *        declares one.
     *
     * @param firstRow The output-file group's own first row, whose
     *                 separator column's row join is resolved.
     * @returns The resolved row-join text, or an empty string when
     *          @p firstRow's separator column declares no row join.
     */
    juce::String getRowJoin (const Model::Element& firstRow) const;

    /**
     * @brief Resolves @p firstRow's own file-header description -- @p firstRow's
     *        structure-column @c description binding, paired with its own first
     *        shape line, read as a wired table address when its value is
     *        @-sigiled. A plain-text @c description binding at that position is
     *        the item-prose channel, never file documentation,
     *        and resolves empty here.
     *
     * @pre When @p firstRow's @c description binding is @-sigiled, it resolves
     *      to a table -- established once by Validator::isReference()
     *      before the writer ever runs.
     *
     * @param firstRow The output-file group's own first row, whose
     *                 @c description binding is resolved.
     * @param file     The group's own output file, matched against the
     *                 addressed table's @c file column when @p firstRow's
     *                 @c description binding is a table address.
     * @returns The resolved description text, or an empty string when
     *          @p firstRow declares no @-sigiled @c description binding, or the
     *          addressed table carries no row for @p file.
     */
    juce::String getFileDescription (const Model::Element& firstRow, const juce::String& file) const;

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
     * @param extension The target file extension a description value is
     *                  commented for.
     */
    void apply (jam::MarkdownDocument& output, const jam::Array<const Model::Element*>& tables,
               const jam::Array<const Model::Element*>& rows, int index, int groupEnd,
               const juce::String& extension) const;

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
    static juce::String getBody (const juce::String& shapeText, const juce::String& banner);

    /**
     * @brief Renders the cast-output banner for @p extension, framed by
     *        @p extension's own @c Id::bannerOpen / @c Id::bannerClose
     *        when declared, or wrapped line-by-line through
     *        Transforms::toCommentBlock() when @p extension declares no
     *        banner frame, followed by @p description when it is not empty.
     *        The result carries no trailing newline -- the caller
     *        supplies its own separator before the shape text that
     *        follows.
     *
     * @param extension The target file extension the banner is framed
     *                  for.
     * @param description The file's own header description, appended after the
     *                  banner when not empty.
     * @returns The rendered banner, or an empty string when the parsed
     *          banner document declares no @c banner code block.
     */
    juce::String getBanner (const juce::String& extension, const juce::String& description) const;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Writer)
};
