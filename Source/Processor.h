#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Model.h"
#include "Validator.h"
#include "Writer.h"

struct Processor
{
    Processor (const juce::File& manifestFile)
        : model (Model::parse (manifestFile))
        , templateDocument (jam::MarkdownDocument::parse (model->getFile().loadFileAsString()))
        , writer (*model, templateDocument)
    {
        jam::Stamp::getInstance()->addIfNotAlreadyThere (jam::Stamp::Entry {});
    }

    juce::Result generate (const juce::String& output = {})
    {
        if (const auto validation { Validator::isValid (*model, templateDocument) };
            not validation.wasOk())
            return validation;

        return writer.toFile (model->getOutput (output));
    }

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

        for (const auto& origin : origins)
        {
            const auto file { model->getOutput (origin) };
            const auto current { file.loadFileAsString() };

            if (const auto canonical { formatter.getText (*model, origin) }; canonical != current)
                file.replaceWithText (canonical,
                    false,
                    false,
                    juce::String::charToString (Chars::newline).toRawUTF8());
        }

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
