#pragma once
#include <JuceHeader.h>
#include "Model.h"
#include "Operators.h"

struct Writer
{
    // Ensure Element is the actual element type, not a container
    using Element = jam::Document::Element;

    Writer (const Document& document)
        : model (document)
    {
    }

    bool toFile (const juce::File& outputPath)
    {
        const auto rows { model.getTableRows (Id::generated) };
        return std::all_of (rows.begin(), rows.end(), forEachFile (outputPath));
    }

private:
    const Document& model;

    //==============================================================================
    juce::String
    getCellValue (const juce::Identifier& columnId, const juce::Identifier& rowId) const noexcept
    {
        return model.getTableValue (Id::generated, columnId, rowId);
    }

    juce::String getTemplate (const juce::Identifier& rowId) const noexcept
    {
        auto banner { jam::MarkdownDocument::parse (BinaryData::getString (files::castOutput)) };
        std::string newline { chars::newline };
        juce::StringArray codeTemplate { banner.getCodeBlock (Id::banner)->getAllSubText(),
                                         newline,
                                         Id::pragmaOnce,
                                         getCellValue (Id::templatePath, rowId) };

        return codeTemplate.joinIntoString (newline);
    };

    juce::String getOutput (const juce::Identifier& rowId) const noexcept
    {
        return getCellValue (Id::output, rowId);
    };

    std::function<bool (Element*)> forEachFile (const juce::File& outputPath) const
    {
        auto path { jam::File::getOrCreateDirectory (outputPath) };

        return [this, &path] (Element* row) -> bool
        {
            auto filename { getOutput (row->id) };
            auto file { path.getChildFile (filename) };
            file.create();
            return file.replaceWithText (getTemplate (row->id));
        };
    };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Writer)
};
