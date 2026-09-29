#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Model.h"
#include "Validator.h"
#include "Toolchain.h"
#include "Pack.h"
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
 * manifest declares, in parallel, write-if-different. The toolchain rows
 * run through Toolchain, and the @c pack command builds its archives
 * through Pack.
 */
struct Processor
{
    /**
     * @brief Parses @p text into a Model and every @c .cast file its index
     *        declares into a TemplateDocument, and constructs the Writer
     *        that renders through them.
     *
     * @param text         The manifest text to parse.
     * @param origin       The manifest's own origin path.
     * @param directory    The manifest's own directory, resolving every
     *                     path the manifest declares.
     * @param reflowWidths The style's own @c name/@c width pairs, stamped
     *                     onto the parsed Model at creation.
     */
    Processor (const juce::String& text, const juce::String& origin, const juce::File& directory, const jam::HashMap<juce::Identifier, int>& reflowWidths)
        : model (Model::parse (text, origin, directory, reflowWidths))
        , templateDocument (*model)
        , writer (*model, templateDocument)
    {
        jam::Stamp::getInstance()->addIfNotAlreadyThere (jam::Stamp::Entry {});
    }

    /**
     * @brief Validates the parsed manifest's full structure through
     *        Validator::isValid(), writes its declared outputs through the
     *        Writer, then runs every selected @c ## toolchain row through
     *        run().
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
     * @param formatter The formatter every changed origin's canonical text
     *                  is read from.
     * @returns juce::Result::ok() when the manifest's own markdown
     *          validates and every changed file writes successfully, or a
     *          failure naming every file that failed to write.
     */
    juce::Result format (const jam::MarkdownWriter& formatter)
    {
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

    /**
     * @brief Returns the manifest origin's own canonical text, paired with
     *        the manifest's own structural validation result.
     *
     * @param formatter The formatter the manifest origin's canonical text
     *                  is read from.
     * @returns The manifest's own structural validation result, paired
     *          with its origin's canonical text.
     */
    std::pair<juce::Result, juce::String> getText (const jam::MarkdownWriter& formatter) const
    {
        if (const auto result { validator.isValid (*model) }; not result.wasOk())
            return { result, {} };

        return { juce::Result::ok(), formatter.getText (*model, model->getManifestOrigin()) };
    }

private:
    /**
     * @brief Runs every @c ## toolchain row whose @c argument column
     *        equals @p toolchainArgument and whose host matches, starting
     *        each one's own @c command, followed by its @c flag when it
     *        declares one, and waiting for it to exit.
     *
     * Host selection uses Toolchain::isHostSelected(): a row runs when its
     * @c host cell is empty or names this host. A row that host selection
     * removes still counts as a match for a non-empty
     * @p toolchainArgument.
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
                const auto argument { model->getColumnValue (*row, Id::argument) };

                if (argument.compare (toolchainArgument) == 0)
                {
                    hasMatchedToolchainRow = true;

                    if (Toolchain::isHostSelected (*model, *row))
                        if (const auto result { runToolchainRow (*row, toolchainArgument) }; not result.wasOk())
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
     *        declares one, through Toolchain::runProcess().
     *
     * The command word @c pack is not started as a process. It dispatches
     * to Pack::toArchives() with @p toolchainArgument.
     *
     * @param row               The @c ## toolchain row whose @c command and
     *                          @c flag are run.
     * @param toolchainArgument The CLI-selected toolchain group, passed
     *                          through to Pack::toArchives() when the
     *                          command is @c pack.
     * @returns juce::Result::ok() when @p row's own process starts and
     *          exits zero, or the archives are built, or a failure naming
     *          its own command line or the failed archive.
     */
    juce::Result runToolchainRow (const Model::Element& row, const juce::String& toolchainArgument)
    {
        const auto& command { model->getValue (row, Id::command) };

        if (command.compare (Id::pack.toString()) == 0)
            return Pack::toArchives (*model, toolchainArgument);

        const auto flag { model->getValue (row, Id::flag) };
        const auto arguments { Toolchain::getToolchainArguments (command, flag) };
        const auto diagnosticLine { flag.isNotEmpty()
                                        ? command + Chars::space + flag
                                        : command };

        return Toolchain::runProcess (arguments, diagnosticLine);
    }

    //==============================================================================
    /** The parsed manifest, spliced into one addressable document. */
    std::unique_ptr<Model> model;
    /** Every @c .cast template file the manifest's index declares, parsed once. */
    TemplateDocument templateDocument;
    /** Renders the manifest's declared outputs through model and templateDocument. */
    Writer writer;
    /** Validates an origin file's own markdown structure during format(). */
    const jam::MarkdownValidator validator;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Processor)
};
