#pragma once
#include <JuceHeader.h>
#include "Document.h"
#include "Operators.h"

struct Writer
{
    Writer (const Document& castDocument) : document (castDocument) {}

    bool toFile (const juce::File& generatedOutputPath)
    {
        const auto rows { document.getTableRows (Id::generated) };

        return std::all_of (rows.begin(), rows.end(), [&] (auto* row) {
            const auto outputFile { generatedOutputPath.getChildFile (
                document.getTableValue (Id::generated, Id::output, row->id)) };

            outputFile.getParentDirectory().createDirectory();

            return outputFile.replaceWithText (document.getTemplate (
                document.getTableValue (Id::generated, Id::templatePath, row->id)));
        });
    }

private:
    const Document& document;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Writer)
};
