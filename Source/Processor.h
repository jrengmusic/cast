#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Model.h"
#include "Validator.h"
#include "Writer.h"

struct Processor
{
    Processor (const juce::File& documentFile)
        : documentFile (documentFile)
        , model (Model::parse (documentFile))
        , writer (*model)
    {
        jam::Stamp::getInstance()->addIfNotAlreadyThere (jam::Stamp::Entry {});
    }

    juce::Result generate (const juce::String& output = {})
    {
        if (const auto validation { Validator::isValid (*model) }; not validation.wasOk())
            return validation;

        if (writer.toFile (model->getOutput (output)))
            return juce::Result::ok();

        return juce::Result::fail ({});
    }

    juce::Result format()
    {
        static const jam::MarkdownWriter formatter;
        static const jam::MarkdownValidator validator;

        jam::Array<juce::File> tableFiles { documentFile };
        const auto markdownExtension { juce::String::charToString (Chars::dot) + Extensions::md };

        for (auto* row : model->getTableRows (Id::index))
        {
            const auto pathCell { model->getTableValue (*row, Id::symbol) };

            if (pathCell.endsWith (markdownExtension))
                tableFiles.addIfNotAlreadyThere (model->getOutput (pathCell));
        }

        for (const auto& file : tableFiles)
        {
            const auto current { file.loadFileAsString() };
            const auto document { jam::MarkdownDocument::parse (
                current, file.getRelativePathFrom (documentFile.getParentDirectory())) };

            if (const auto validation { validator.isValid (document) }; not validation.wasOk())
                return validation;

            if (const auto canonical { formatter.getText (document) }; canonical != current)
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
    juce::File documentFile;
    std::unique_ptr<Model> model;
    Writer writer;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Processor)
};
