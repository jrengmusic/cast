#pragma once
#include <JuceHeader.h>
#include "generated/Identifiers.h"
#include "Operators.h"

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
                                              + Id::format.toString() };

        return getTableHeaders (*row.parent).contains (formatColumn.toString())
                  ? getTableValue (row, formatColumn)
                  : juce::String();
    }

    juce::String getValue (juce::StringRef file, juce::StringRef alias) const
    {
        const juce::String aliasText { alias };

        if (juce::Identifier::isValidIdentifier (aliasText))
        {
            for (auto* table : getTables (Id::index))
            {
                if (table->contains (Id::path) and *table->get<juce::String> (Id::path) == file)
                {
                    const auto symbol {
                        getTableValue (*table, Id::symbol, juce::Identifier (aliasText))
                    };
                    const auto format {
                        getTableHeaders (*table).contains (Id::format.toString())
                            ? getTableValue (*table, Id::format, juce::Identifier (aliasText))
                            : juce::String()
                    };

                    return format.isNotEmpty() ? Transforms::getTransformed (format, symbol)
                                               : symbol;
                }
            }
        }

        return {};
    }

    juce::String getValue (Element& row, juce::StringRef alias) const
    {
        const auto origin { *row.parent->get<juce::String> (Id::path) };
        return getValue (origin, alias);
    }

    juce::String getToken (Element& row, const juce::Identifier& name) const
    {
        const auto cell { getTableValue (row, Id::token) };
        const auto commaText { juce::String::charToString (chars::comma) };
        const auto names {
            jam::Strings::fromTokens (jam::Format::getPreColon (cell), commaText, {})
        };
        const auto values {
            jam::Strings::fromTokens (jam::Format::getPostColon (cell), commaText, {})
        };

        for (int index { 0 }; index < names.size(); ++index)
        {
            if (names.at (index).trim() == name.toString())
            {
                const auto value { values.at (index).trim() };
                const auto symbol { getValue (row, value) };

                return symbol.isNotEmpty() ? symbol : value;
            }
        }

        return {};
    }

    juce::File getFile (Element& row, juce::StringRef alias) const
    {
        const auto symbolPath { getValue (row, alias) };

        if (symbolPath.isNotEmpty())
            return getOutput (symbolPath);

        return {};
    }

    juce::File getOutput (juce::StringRef relativePath) const
    {
        return path.getChildFile (relativePath);
    }

    Element* getTables (Element& row, juce::StringRef reference) const
    {
        const juce::String referenceText { reference };
        const auto sourceName { jam::Format::getPreColon (referenceText) };
        const auto tableName { jam::Format::getPostColon (referenceText) };
        const auto declaredPath { getValue (row, sourceName) };

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

    bool isTemplatePath (Element& row, juce::StringRef cell) const noexcept
    {
        return getValue (row, cell).endsWith (
            juce::String::charToString (chars::dot) + extensions::cast);
    }

    bool isReference (Element& row, juce::StringRef cell) const noexcept
    {
        const juce::String cellText { cell };
        return cellText.containsChar (chars::colon) and getTables (row, cell) != nullptr;
    }

private:
    juce::File path;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Model)
};
