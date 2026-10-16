#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Model.h"
#include "Validator.h"
#include "Toolchain.h"
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
 * run through Toolchain.
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
    Processor (const juce::String& text, const juce::String& origin, const juce::File& directory, const jam::HashMap<juce::Identifier, int>& reflowWidths);

    /**
     * @brief Validates the parsed manifest's full structure through
     *        Validator::isValid(), stamps every wrapper's chain through
     *        Model::addChains(), writes its declared outputs through the
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
    juce::Result generate (const juce::String& output = {}, const juce::String& toolchainArgument = {});

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
    juce::Result format (const jam::MarkdownWriter& formatter);

    /**
     * @brief Returns the manifest origin's own canonical text, paired with
     *        the manifest's own structural validation result.
     *
     * @param formatter The formatter the manifest origin's canonical text
     *                  is read from.
     * @returns The manifest's own structural validation result, paired
     *          with its origin's canonical text.
     */
    std::pair<juce::Result, juce::String> getText (const jam::MarkdownWriter& formatter) const;

private:
    /**
     * @brief Runs every @c ## toolchain row whose @c argument cell
     *        equals @p toolchainArgument, in row order, starting each
     *        one's own @c command, followed by its @c flag when it
     *        declares one, and waiting for it to exit.
     *
     * The run stops at the first row that fails. A non-empty
     * @p toolchainArgument that matches no row is a failure.
     *
     * @param toolchainArgument The CLI-selected toolchain group -- runs
     *                          only the @c ## toolchain rows whose
     *                          @c argument column equals this value;
     *                          empty selects the blank-cell, default rows.
     * @returns juce::Result::ok() when @p toolchainArgument matches at
     *          least one row when non-empty and every selected row starts
     *          and exits zero, or the first failure encountered.
     */
    juce::Result run (const juce::String& toolchainArgument);

    /**
     * @brief Returns every spliced block's own origin file, deduplicated.
     *
     * @returns Every distinct origin path found across the master
     *          document's root children, in discovery order.
     */
    jam::Strings getOrigins() const;

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
    juce::String writeOriginIfChanged (const juce::String& origin, const jam::MarkdownWriter& formatter) const;

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
    juce::Result getWriteResult (const jam::Array<juce::String>& formatFailures) const;

    /**
     * @brief Runs @p row's own @c command, followed by its @c flag when it
     *        declares one, through Toolchain::runProcess().
     *
     * The command line in a failure is the @c command, followed by the
     * @c flag when the row declares one.
     *
     * @param row The @c ## toolchain row whose @c command and @c flag are
     *            run.
     * @returns juce::Result::ok() when the process of @p row starts and
     *          exits zero, or a failure naming its command line.
     */
    juce::Result runToolchainRow (const Model::Element& row);

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
