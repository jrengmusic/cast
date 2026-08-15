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
                    const auto pathCell { document->getTableValue (*indexRow, Id::symbol) };

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

    juce::String getFormat (Element& row, const juce::Identifier& column) const
    {
        const juce::Identifier formatColumn { column.toString()
                                              + juce::String::charToString (chars::space)
                                              + Id::format };

        return getTableHeaders (*row.parent).contains (formatColumn.toString())
                  ? getTableValue (row, formatColumn)
                  : juce::String();
    }

    juce::String getPath (juce::StringRef alias) const
    {
        const auto indexTables { getTables (Id::index) };

        if (not indexTables.isEmpty())
        {
            for (auto* indexRow : getTableRows (*indexTables.at (0)))
            {
                if (getTableValue (*indexRow, Id::alias) == alias)
                    return getTableValue (*indexRow, Id::symbol);
            }
        }

        return {};
    }

    juce::String getToken (Element& row, const juce::Identifier& name) const
    {
        const auto cell { getTableValue (row, Id::token) };
        const auto commaText { juce::String::charToString (chars::comma) };
        const auto names {
            juce::StringArray::fromTokens (jam::Format::getPreColon (cell), commaText, {})
        };
        const auto values {
            juce::StringArray::fromTokens (jam::Format::getPostColon (cell), commaText, {})
        };

        for (int index { 0 }; index < names.size(); ++index)
        {
            if (names[index].trim() == name.toString())
            {
                const auto value { values[index].trim() };
                const auto resolved { getPath (value) };

                return resolved.isNotEmpty() ? resolved : value;
            }
        }

        return {};
    }

    juce::String resolve (const juce::String& value) const
    {
        const auto symbol { getPath (value) };

        return symbol.isNotEmpty() ? symbol : value;
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
                if (candidate->contains (Id::symbol)
                    and *candidate->get<juce::String> (Id::symbol) == declaredPath)
                    return candidate;
            }
        }

        return nullptr;
    }

    bool isTemplatePath (juce::StringRef cell) const noexcept
    {
        return getPath (cell).endsWith (juce::String::charToString (chars::dot) + extensions::cast);
    }

    bool isReference (juce::StringRef cell) const noexcept
    {
        const juce::String cellText { cell };
        return cellText.containsChar (chars::colon) and getTables (cell) != nullptr;
    }

private:
    juce::File path;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Model)
};
