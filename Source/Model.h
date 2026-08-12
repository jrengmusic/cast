#pragma once
#include <JuceHeader.h>
#include "generated/Identifiers.h"

class Document : public jam::MarkdownDocument
{
public:
    Document() = default;

    static std::unique_ptr<Document> parse (const juce::File& documentFile)
    {
        auto document { std::make_unique<Document>() };

        if (documentFile.existsAsFile())
        {
            const auto parent { documentFile.getParentDirectory() };

            document->path = parent;
            document->appendChildren (
                jam::MarkdownDocument::parse (
                    documentFile.loadFileAsString(), documentFile.getRelativePathFrom (parent)));

            const auto tablesDirectory { parent.getChildFile (Id::tables.toString()) };
            const auto wildcard { juce::String::charToString (chars::asterisk)
                                  + juce::String::charToString (chars::dot) + extensions::md };

            auto tableFiles { tablesDirectory.findChildFiles (juce::File::findFiles,
                                                              false,
                                                              wildcard) };
            tableFiles.sort();

            for (const auto& file : tableFiles)
                document->appendChildren (
                    jam::MarkdownDocument::parse (
                        file.loadFileAsString(), file.getRelativePathFrom (parent)));
        }

        return document;
    }

    juce::String getTemplate (juce::StringRef relativePath) const
    {
        return path.getChildFile (relativePath).loadFileAsString();
    }

    juce::File getOutput (juce::StringRef relativePath) const
    {
        return path.getChildFile (relativePath);
    }

private:
    juce::File path;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Document)
};
