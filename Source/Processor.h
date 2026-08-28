#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Model.h"
#include "Validator.h"
#include "Writer.h"

/**
 * @struct Processor
 * @brief Owns the parsed Model, TemplateDocument, and Writer for one
 *        manifest, and drives generation and origin-file
 *        canonicalization through them.
 *
 * Processor parses the manifest and its declared template once at
 * construction. generate() then validates the manifest and writes its
 * declared outputs; format() re-canonicalizes every origin file the
 * manifest declares, in parallel, write-if-different.
 */
struct Processor
{
    /**
     * @brief Parses @p manifestFile into a Model and its declared
     *        template file into a TemplateDocument, and constructs the
     *        Writer that renders through them.
     *
     * @param manifestFile The manifest file to parse.
     */
    explicit Processor (const juce::File& manifestFile)
        : model (Model::parse (manifestFile))
        , templateDocument (jam::MarkdownDocument::parse (model->getTemplateFile().loadFileAsString()))
        , writer (*model, templateDocument)
    {
        jam::Stamp::getInstance()->addIfNotAlreadyThere (jam::Stamp::Entry {});
    }

    /**
     * @brief Validates the parsed manifest through Validator::isValid(),
     *        then writes its declared outputs through the Writer.
     *
     * @param output A path resolved against the manifest's own directory,
     *               giving the directory every declared output file is
     *               written under; empty resolves to the manifest's own
     *               directory.
     * @returns juce::Result::ok() when validation succeeds and every
     *          output file writes successfully, or the first failure
     *          encountered.
     */
    juce::Result generate (const juce::String& output = {})
    {
        if (const auto validation { Validator::isValid (*model, templateDocument) }; not validation.wasOk())
            return validation;

        return writer.toFile (model->getFile (output));
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
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;
    jam::Stamp stamp;
    jam::Hyperlink hyperlink;
    jam::Grapheme grapheme;
    Generated generated;

    //==============================================================================
    std::unique_ptr<Model> model;
    TemplateDocument templateDocument;
    Writer writer;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Processor)
};
