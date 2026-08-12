#pragma once
#include <JuceHeader.h>
#include "generated/CAST.h"
#include "Document.h"
#include "Writer.h"

struct Processor
{
    Processor (const juce::File& documentFile)
        : document (Document::parse (documentFile))
        , writer (*document)
    {
        jam::Stamp::getInstance()->addIfNotAlreadyThere (jam::Stamp::Entry {});
    }

    juce::Result generate (const juce::String& output = {})
    {
        if (writer.toFile (document->getOutput (output)))
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
    std::unique_ptr<Document> document;
    Writer writer;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Processor)
};
