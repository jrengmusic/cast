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
     * @brief Validates the parsed manifest through Validator::isValid(),
     *        writes its declared outputs through the Writer, then runs
     *        every selected @c ## toolchain row through run().
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
     * @returns juce::Result::ok() when the manifest's own markdown
     *          validates and every changed file writes successfully, or a
     *          failure naming every file that failed to write.
     */
    juce::Result format()
    {
        static const jam::MarkdownWriter formatter;
        static const jam::MarkdownValidator validator;

        if (const auto result { validator.isValid (*model) }; not result.wasOk())
            return result;

        jam::Array<juce::String> origins;

        for (auto* table : model->getTables())
        {
            const auto origin { *table->get<juce::String> (Id::path) };
            origins.addIfNotAlreadyThere (origin);
        }

        jam::Array<juce::String> formatFailures;
        formatFailures.resize (origins.size());

        Jobs::run (origins.size(),
            [this, &origins, &formatFailures] (int index)
            {
                const auto& origin { origins.at (index) };
                const auto file { model->getFile (origin) };
                const auto current { file.loadFileAsString() };

                if (const auto canonical { formatter.getText (*model, origin) }; canonical != current)
                    if (not file.replaceWithText (canonical,
                        false,
                        false,
                        juce::String::charToString (Chars::newline).toRawUTF8()))
                        formatFailures.at (index) = file.getFullPathName();
            });

        jam::Strings failures;

        for (const auto& failedFile : formatFailures)
            if (failedFile.isNotEmpty())
                failures.add (failedFile + Id::diagnosticSeparator + text::Diagnostics::failOutputWrite);

        if (failures.size() > 0)
            return juce::Result::fail (
                failures.joinIntoString (juce::String::charToString (Chars::newline), 0, -1));

        return juce::Result::ok();
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
        auto toolchainArgumentMatched { toolchainArgument.isEmpty() };

        for (auto* table : model->getTables (Id::toolchain))
        {
            for (auto* row : model->getTableRows (*table))
            {
                auto* argumentCell { model->getTableCell (*row, Id::argument) };
                const auto argument { argumentCell != nullptr
                                          ? *argumentCell->get<juce::String> (Id::value)
                                          : juce::String{} };

                if (argument == toolchainArgument)
                {
                    toolchainArgumentMatched = true;

                    const auto& command { model->getValue (*row, Id::command) };
                    const auto& flag { model->getValue (*row, Id::flag) };
                    const auto line { flag.isNotEmpty()
                                          ? command + juce::String::charToString (Chars::space) + flag
                                          : command };

                    juce::ChildProcess process;

                    if (not process.start (line))
                        return juce::Result::fail (
                            line + Id::diagnosticSeparator + text::Diagnostics::failToolchain);

                    process.waitForProcessToFinish (-1);

                    if (process.getExitCode() != 0)
                        return juce::Result::fail (
                            line + Id::diagnosticSeparator + text::Diagnostics::failToolchain);
                }
            }
        }

        if (not toolchainArgumentMatched)
            return juce::Result::fail (toolchainArgument + Id::diagnosticSeparator
                                       + text::Diagnostics::failToolchainArgument);

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
