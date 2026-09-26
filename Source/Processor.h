#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Model.h"
#include "Validator.h"
#include "Writer.h"

/**
 * @struct Processor
 * @brief Owns the parsed Model, the pool of parsed template files in its
 *        TemplateDocument, and the Writer for one manifest, and drives
 *        generation and origin-file canonicalization through them.
 *
 * Processor parses the manifest and its declared template files once at
 * construction. generate() then validates the manifest and writes its
 * declared outputs; format() re-canonicalizes every origin file the
 * manifest declares, in parallel, write-if-different.
 */
struct Processor
{
    /**
     * @brief Parses @p manifestFile into a Model and every @c .cast file
     *        its index declares into a TemplateDocument, and constructs
     *        the Writer that renders through them.
     *
     * @param manifestFile The manifest file to parse.
     */
    explicit Processor (const juce::File& manifestFile)
        : model (Model::parse (manifestFile))
        , templateDocument (*model)
        , writer (*model, templateDocument)
    {
        jam::Stamp::getInstance()->addIfNotAlreadyThere (jam::Stamp::Entry {});
    }

    /**
     * @brief Validates the parsed manifest's own @c ## format shape
     *        through Validator::isFormat(), then its full structure
     *        through Validator::isValid(), writes its declared outputs
     *        through the Writer, then runs every selected @c ## toolchain
     *        row through run().
     *
     * @param output             A path resolved against the manifest's own
     *                           directory, giving the directory every
     *                           declared output file is written under;
     *                           empty resolves to the manifest's own
     *                           directory.
     * @param toolchainArgument  The CLI-selected toolchain group, passed
     *                           through to run().
     * @returns juce::Result::ok() when validation succeeds, every output
     *          file writes successfully, and run() succeeds, or the first
     *          failure encountered.
     */
    juce::Result generate (const juce::String& output = {}, const juce::String& toolchainArgument = {})
    {
        if (const auto formatShape { Validator::isFormat (*model) }; not formatShape.wasOk())
            return formatShape;

        if (const auto validation { Validator::isValid (*model, templateDocument) }; not validation.wasOk())
            return validation;

        if (const auto written { writer.toFile (model->getFile (output)) }; not written.wasOk())
            return written;

        return run (toolchainArgument);
    }

    /**
     * @brief Re-canonicalizes every origin file the manifest declares,
     *        rewriting each one, in parallel, whose canonical text
     *        differs from what is currently on disk.
     *
     * Runs Validator::isFormat() first, then constructs the MarkdownWriter
     * with @p lineWrap and the Model's own reflowWidths (Model::parse's
     * own single source for every @c ## format row's @c name/@c width
     * pair -- see Model::getReflowWidths()), before running the jam
     * structural validator and rewriting each changed origin.
     *
     * @param lineWrap The column at which the writer wraps a paragraph's
     *                 rendered lines.
     * @returns juce::Result::ok() when the manifest's own markdown
     *          validates and every changed file writes successfully, or a
     *          failure naming every file that failed to write.
     */
    juce::Result format (int lineWrap)
    {
        if (const auto result { Validator::isFormat (*model) }; not result.wasOk())
            return result;

        const jam::MarkdownWriter formatter { lineWrap, model->getReflowWidths() };
        static const jam::MarkdownValidator validator;

        if (const auto result { validator.isValid (*model) }; not result.wasOk())
            return result;

        const auto origins { getOrigins() };
        jam::Array<juce::String> formatFailures;
        formatFailures.resize (origins.size());

        Jobs::run (origins.size(),
            [this, &origins, &formatFailures, &formatter] (int index)
            {
                formatFailures.at (index) = writeOriginIfChanged (origins.at (index), formatter);
            });

        return getWriteResult (formatFailures);
    }

private:
    /**
     * @brief Runs every @c ## toolchain row whose @c argument column
     *        equals @p toolchainArgument, starting each one's own
     *        @c command, followed by its @c flag when it declares one,
     *        and waiting for it to exit.
     *
     * @param toolchainArgument The CLI-selected toolchain group -- runs
     *                          only the @c ## toolchain rows whose
     *                          @c argument column equals this value;
     *                          empty selects the blank-cell, default rows.
     * @returns juce::Result::ok() when @p toolchainArgument matches at
     *          least one row when non-empty and every selected row starts
     *          and exits zero, or the first failure encountered.
     */
    juce::Result run (const juce::String& toolchainArgument)
    {
        const auto isToolchainArgumentGiven { toolchainArgument.isNotEmpty() };
        auto hasMatchedToolchainRow { false };

        for (auto* table : model->getTables (Id::toolchain))
        {
            for (auto* row : model->getTableRows (*table))
            {
                const auto argument { getColumnValue (*row, Id::argument) };

                if (argument.compare (toolchainArgument) == 0)
                {
                    hasMatchedToolchainRow = true;

                    if (const auto result { runToolchainRow (*row) }; not result.wasOk())
                        return result;
                }
            }
        }

        if (isToolchainArgumentGiven and not hasMatchedToolchainRow)
            return juce::Result::fail (toolchainArgument + Id::diagnosticSeparator
                                       + text::Diagnostics::failToolchainArgument);

        return juce::Result::ok();
    }

    /**
     * @brief Returns @p row's resolved value for @p column, or an empty
     *        string when @p row declares no such column.
     *
     * @param row    The row @p column's cell is read from.
     * @param column The column whose cell value is read.
     * @returns @p row's resolved value for @p column, or an empty string
     *          when @p row carries no cell for @p column.
     */
    juce::String getColumnValue (const Model::Element& row, const juce::Identifier& column) const
    {
        auto* cell { model->getTableCell (row, column) };

        return cell != nullptr ? *cell->get<juce::String> (Id::value) : juce::String {};
    }

    /**
     * @brief Returns every spliced block's own origin file, deduplicated.
     *
     * @returns Every distinct origin path found across the master
     *          document's root children, in discovery order.
     */
    jam::Strings getOrigins() const
    {
        jam::Strings origins;

        for (auto* block : *model->getRoot())
        {
            const auto origin { *block->get<juce::String> (Id::path) };
            origins.addIfNotAlreadyThere (origin, false);
        }

        return origins;
    }

    /**
     * @brief Rewrites @p origin's own file with @p formatter's own
     *        canonical text, when that text differs from what is
     *        currently on disk.
     *
     * @param origin    The origin path to canonicalize, resolved against
     *                  the manifest's own directory.
     * @param formatter The formatter @p origin's canonical text is read
     *                  from.
     * @returns @p origin's own resolved file's full path when it needed
     *          rewriting and the write failed, or an empty string when
     *          @p origin's text was already canonical or wrote
     *          successfully.
     */
    juce::String writeOriginIfChanged (const juce::String& origin, const jam::MarkdownWriter& formatter) const
    {
        static const auto newlineText { juce::String::charToString (Chars::newline) };

        const auto file { model->getFile (origin) };
        const auto current { file.loadFileAsString() };

        if (const auto canonical { formatter.getText (*model, origin) }; canonical.compare (current) != 0)
            if (not file.replaceWithText (canonical,
                false,
                false,
                newlineText.toRawUTF8()))
                return file.getFullPathName();

        return {};
    }

    /**
     * @brief Collects @p formatFailures' own non-empty entries into one
     *        failure result.
     *
     * @param formatFailures Every output-file group's own writeOriginIfChanged()
     *                       result, one per origin, empty when that origin
     *                       wrote successfully or needed no write.
     * @returns juce::Result::ok() when every entry is empty, or a failure
     *          naming every file that failed to write.
     */
    juce::Result getWriteResult (const jam::Array<juce::String>& formatFailures) const
    {
        static const auto newlineText { juce::String::charToString (Chars::newline) };

        jam::Strings failures;

        for (const auto& failedFile : formatFailures)
            if (failedFile.isNotEmpty())
                failures.add (failedFile + Id::diagnosticSeparator + text::Diagnostics::failOutputWrite);

        if (failures.size() > 0)
            return juce::Result::fail (failures.joinIntoString (newlineText, 0, -1));

        return juce::Result::ok();
    }

    /**
     * @brief Runs @p row's own @c command, followed by its @c flag when it
     *        declares one, through runProcess().
     *
     * @param row The @c ## toolchain row whose @c command and @c flag are
     *            run.
     * @returns juce::Result::ok() when @p row's own process starts and
     *          exits zero, or a failure naming its own command line.
     */
    juce::Result runToolchainRow (const Model::Element& row)
    {
        const auto& command { model->getValue (row, Id::command) };
        const auto flag { getColumnValue (row, Id::flag) };
        const auto arguments { getToolchainArguments (command, flag) };
        const auto diagnosticLine { flag.isNotEmpty()
                                        ? command + Chars::space + flag
                                        : command };

        return runProcess (arguments, diagnosticLine);
    }

    /**
     * @brief Tokenizes @p command and @p flag into one argv array.
     *
     * @param command The toolchain row's own @c command, placed at
     *                argument index zero.
     * @param flag    The toolchain row's own @c flag, tokenized after
     *                @p command.
     * @returns @p command followed by @p flag's own whitespace-tokenized
     *          arguments.
     */
    juce::StringArray getToolchainArguments (const juce::String& command, const juce::String& flag) const
    {
        juce::StringArray arguments { command };
        arguments.addTokens (flag, true);

        return arguments;
    }

    /**
     * @brief Runs @p arguments as a child process in the current working
     *        directory, streaming its output to stdout and blocking until
     *        it exits.
     *
     * @param arguments      The process argv, index zero the executable.
     * @param diagnosticLine The command line named in a failure, when the
     *                       process exits non-zero.
     * @returns juce::Result::ok() when the process exits zero, or a
     *          failure naming @p diagnosticLine.
     */
    juce::Result runProcess (const juce::StringArray& arguments, const juce::String& diagnosticLine)
    {
        juce::WaitableEvent finished;
        auto exitCode { -1 };

        jam::Subprocess subprocess;
        subprocess.launch (arguments,
                           juce::File::getCurrentWorkingDirectory(),
                           [&finished, &exitCode] (int processExitCode, const std::string&)
                           {
                               exitCode = processExitCode;
                               finished.signal();
                           },
                           [] (std::string_view chunk, bool)
                           {
                               fwrite (chunk.data(), 1, chunk.size(), stdout);
                               fflush (stdout);
                           });

        finished.wait();

        if (exitCode != 0)
            return juce::Result::fail (
                diagnosticLine + Id::diagnosticSeparator + text::Diagnostics::failToolchain);

        return juce::Result::ok();
    }

    juce::ScopedJuceInitialiser_GUI libraryInitialiser;
    Generated generated;
    jam::Stamp stamp;

    //==============================================================================
    std::unique_ptr<Model> model;
    TemplateDocument templateDocument;
    Writer writer;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Processor)
};
