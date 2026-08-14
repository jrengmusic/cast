#pragma once
#include <JuceHeader.h>
#include "generated/Identifiers.h"

template <typename Function>
static void runJobs (int count, Function&& function) noexcept
{
    juce::ThreadPool pool;

    for (int index { 0 }; index < count; ++index)
        pool.addJob ([&function, index] { function (index); });

    while (pool.getNumJobs() > 0)
        juce::Thread::sleep (1);
}

class Model : public jam::MarkdownDocument
{
public:
    Model() = default;

    using jam::MarkdownDocument::getTables;

    static std::unique_ptr<Model> parse (const juce::File& documentFile)
    {
        auto document { std::make_unique<Model>() };

        if (documentFile.existsAsFile())
        {
            const auto parent { documentFile.getParentDirectory() };

            document->path = parent;
            document->appendChildren (jam::MarkdownDocument::parse (
                documentFile.loadFileAsString(), documentFile.getRelativePathFrom (parent)));

            const auto indexTables { document->getTables (Id::index) };

            if (not indexTables.isEmpty())
            {
                const auto indexRows { document->getTableRows (*indexTables.at (0)) };
                const auto markdownExtension {
                    juce::String::charToString (chars::dot) + extensions::md
                };

                jam::Array<juce::File> tableFiles;
                jam::Array<juce::String> tableOrigins;

                const auto manifestOrigin { documentFile.getRelativePathFrom (parent) };

                for (auto* indexRow : indexRows)
                {
                    const auto pathCell { document->getTableValue (*indexRow, Id::path) };

                    if (pathCell.endsWith (markdownExtension) and pathCell != manifestOrigin)
                    {
                        tableFiles.add (parent.getChildFile (pathCell));
                        tableOrigins.add (pathCell);
                    }
                }

                jam::Array<jam::MarkdownDocument> parsedTables;
                parsedTables.resize (tableFiles.size());

                runJobs (tableFiles.size(),
                    [&tableFiles, &tableOrigins, &parsedTables] (int index)
                    {
                        const auto& file { tableFiles.at (index) };

                        parsedTables[index] = jam::MarkdownDocument::parse (
                            file.loadFileAsString(), tableOrigins.at (index));
                    });

                for (auto& table : parsedTables)
                    document->appendChildren (std::move (table));
            }
        }

        return document;
    }

    juce::String getPath (juce::StringRef alias) const
    {
        const auto indexTables { getTables (Id::index) };

        if (not indexTables.isEmpty())
        {
            for (auto* indexRow : getTableRows (*indexTables.at (0)))
            {
                if (getTableValue (*indexRow, Id::alias) == alias)
                    return getTableValue (*indexRow, Id::path);
            }
        }

        return {};
    }

    juce::File getFile (juce::StringRef alias) const
    {
        const auto resolvedPath { getPath (alias) };

        if (resolvedPath.isNotEmpty())
            return getOutput (resolvedPath);

        return {};
    }

    juce::File getOutput (juce::StringRef relativePath) const
    {
        return path.getChildFile (relativePath);
    }

    Element* getTables (juce::StringRef reference) const
    {
        const juce::String referenceText { reference };
        const auto sourceName { jam::Format::getPreColon (referenceText) };
        const auto tableName { jam::Format::getPostColon (referenceText) };
        const auto declaredPath { getPath (sourceName) };

        if (declaredPath.isNotEmpty())
        {
            for (auto* candidate : getTables (juce::Identifier (tableName)))
            {
                if (candidate->contains (Id::path)
                    and *candidate->get<juce::String> (Id::path) == declaredPath)
                    return candidate;
            }
        }

        return nullptr;
    }

private:
    juce::File path;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Model)
};
