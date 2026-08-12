#pragma once
#include <JuceHeader.h>
#include "generated/CAST.h"
#include "Model.h"
#include "Writer.h"

struct Processor
{
    Processor (const juce::File& documentFile)
        : model (Document::parse (documentFile))
        , writer (*model)
    {
        jam::Stamp::getInstance()->addIfNotAlreadyThere (jam::Stamp::Entry {});
    }

    juce::Result generate (const juce::String& output = {})
    {
        if (writer.toFile (model->getOutput (output)))
            return juce::Result::ok();

        return juce::Result::fail ({});
    }

private:
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;
    jam::Stamp stamp;
    jam::Hyperlink hyperlink;
    jam::Grapheme grapheme;
    Generated generated;

    //==============================================================================
    std::unique_ptr<Document> model;
    Writer writer;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Processor)
};
