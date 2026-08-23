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

                jam::Array<juce::File> tableFiles;
                jam::Array<juce::String> tableOrigins;

                const auto manifestOrigin { documentFile.getRelativePathFrom (parent) };

                for (auto* indexRow : indexRows)
                {
                    const auto pathCell { document->getTableValue (*indexRow, Id::symbol) };

                    if (juce::File::createFileWithoutCheckingPath (pathCell).hasFileExtension (
                            Extensions::md)
                        and pathCell != manifestOrigin)
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

                        parsedTables.at (index) = jam::MarkdownDocument::parse (
                            file.loadFileAsString(), tableOrigins.at (index));
                    });

                for (auto& table : parsedTables)
                    document->appendChildren (std::move (table));
            }
        }

        return document;
    }

    juce::String getValue (juce::StringRef file, juce::StringRef alias) const
    {
        if (alias.isEmpty() or *alias.text != Chars::at)
            return {};

        const juce::String aliasText { alias };

        if (juce::Identifier::isValidIdentifier (aliasText))
        {
            for (auto* table : getTables (Id::index))
            {
                if (table->contains (Id::path) and *table->get<juce::String> (Id::path) == file)
                {
                    return getTableValue (*table, Id::symbol, juce::Identifier (aliasText));
                }
            }
        }

        return {};
    }

    juce::String getValue (Element& row, const juce::String& alias) const
    {
        const auto origin { *row.parent->get<juce::String> (Id::path) };
        return getValue (origin, alias);
    }

    juce::String getValue (Element& row, const juce::Identifier& column) const
    {
        const auto headers { getTableHeaders (*row.parent) };
        const auto columnIndex { headers.indexOf (column.toString()) };

        jassert (columnIndex >= 0);

        jam::Array<Element*> cells;

        for (auto* cell : row)
            cells.add (cell);

        const auto getCellValue = [] (Element& cell) -> juce::String
        {
            const auto rawText { *cell.get<juce::String> (Id::rawText) };
            const auto isBackticked { rawText.length() >= 2
                                      and rawText.startsWithChar (Chars::backtick)
                                      and rawText.endsWithChar (Chars::backtick) };

            return isBackticked
                       ? jam::Format::toLiteral (rawText.substring (1, rawText.length() - 1))
                       : cell.getAllSubText();
        };

        auto resolved { getCellValue (*cells.at (columnIndex)) };

        if (resolved.isEmpty())
        {
            int sourceIndex { columnIndex - 1 };

            while (sourceIndex >= 0 and headers.at (sourceIndex) == Id::format.toString())
                --sourceIndex;

            if (sourceIndex >= 0)
                resolved = getCellValue (*cells.at (sourceIndex));
        }

        if (resolved.startsWithChar (Chars::at))
            return getValue (row, resolved);

        const auto formatIndex { columnIndex + 1 };
        const auto hasFormat { formatIndex < headers.size()
                               and headers.at (formatIndex) == Id::format.toString() };

        if (hasFormat)
        {
            jassert (formatIndex + 1 >= headers.size()
                   or headers.at (formatIndex + 1) != Id::format.toString());

            const auto operation { cells.at (formatIndex)->getAllSubText() };

            if (operation.isNotEmpty())
                resolved = Transforms::getTransformed (operation, resolved, {});
        }

        return resolved;
    }

    Element* getStructure (Element& row) const
    {
        return getTableHeaders (*row.parent).contains (Id::structure.toString())
                  ? getTableCell (row, Id::structure)
                  : nullptr;
    }

    juce::String getStructure (Element& row, int depth) const
    {
        juce::String head;

        if (auto* scope { getStructure (row, depth, Id::structure) })
            for (auto* block : *scope)
                if (block->isTag (Id::p))
                {
                    const auto blockText { block->getAllSubText() };

                    if (jam::Format::getPreColon (blockText).trim() == Id::templatePath.toString())
                        head = jam::Format::getPostColon (blockText).trim();
                }

        return head;
    }

    Element* getStructure (Element& row, int depth, const juce::Identifier& column) const
    {
        auto* scope { getTableCell (row, column) };

        for (int scopeDepth { 0 }; scopeDepth < depth and scope != nullptr; ++scopeDepth)
        {
            Element* nested { nullptr };

            for (auto* block : *scope)
                if (block->isTag (Id::blockquote))
                    nested = block;

            scope = nested;
        }

        return scope;
    }

    jam::HashMap<juce::Identifier, juce::String>
    getSource (Element& row, int depth, const juce::Identifier& column) const
    {
        jam::HashMap<juce::Identifier, juce::String> bullets;

        if (auto* scope { getStructure (row, depth, column) })
            for (auto* block : *scope)
                if (block->isTag (Id::ul))
                    for (auto* item : *block)
                        if (item->isTag (Id::li))
                        {
                            const auto itemText { item->getAllSubText() };
                            const auto key { jam::Format::getPreColon (itemText).trim() };
                            const auto value { jam::Format::getPostColon (itemText).trim() };
                            bullets.try_emplace (juce::Identifier (key), value);
                        }

        return bullets;
    }

    jam::Array<std::tuple<int, juce::Identifier, juce::String>>
    getSource (Element& row, const juce::Identifier& column) const
    {
        jam::Array<std::tuple<int, juce::Identifier, juce::String>> bullets;

        if (auto* cell { getTableCell (row, column) })
        {
            std::function<void (Element&, int)> walk;
            walk = [&bullets, &walk] (Element& scope, int depth)
            {
                for (auto* block : scope)
                {
                    if (block->isTag (Id::ul))
                        for (auto* item : *block)
                            if (item->isTag (Id::li))
                            {
                                const auto itemText { item->getAllSubText() };
                                const auto key { jam::Format::getPreColon (itemText).trim() };
                                const auto value { jam::Format::getPostColon (itemText).trim() };
                                bullets.add (
                                    std::make_tuple (depth, juce::Identifier (key), value));
                            }

                    if (block->isTag (Id::blockquote))
                        walk (*block, depth + 1);
                }
            };

            walk (*cell, 0);
        }

        return bullets;
    }

    bool isOutputTable (Element& table) const noexcept
    {
        return not table.isTag (Id::index) and getTableHeaders (table).contains (Id::file.toString());
    }

    juce::File getFile (Element& row, juce::StringRef alias) const
    {
        const auto symbolPath { getValue (row, alias) };

        if (symbolPath.isNotEmpty())
            return getOutput (symbolPath);

        return {};
    }

    juce::File getFile() const
    {
        juce::File templateFile;

        for (auto* indexRow : getTableRows (Id::index))
        {
            const auto pathCell { getTableValue (*indexRow, Id::symbol) };

            if (juce::File::createFileWithoutCheckingPath (pathCell).hasFileExtension (
                    Extensions::cast))
                templateFile = getOutput (pathCell);
        }

        return templateFile;
    }

    juce::File getOutput (juce::StringRef relativePath) const
    {
        return path.getChildFile (relativePath);
    }

    Element* getTables (Element& row, juce::StringRef reference) const
    {
        const auto parts { jam::Strings::fromTokens (
            reference, juce::String::charToString (Chars::colon), {}) };
        const auto sourceName { parts.size() > 0 ? parts.at (0).trim() : juce::String() };
        const auto declaredPath { getValue (row, sourceName) };

        if (declaredPath.isNotEmpty())
        {
            const auto tableName { parts.size() > 1 ? parts.at (1).trim() : juce::String() };

            if (tableName.isNotEmpty())
            {
                for (auto* candidate : getTables (juce::Identifier (tableName)))
                    if (candidate->contains (Id::path)
                        and *candidate->get<juce::String> (Id::path) == declaredPath)
                        return candidate;
            }
            else
            {
                jam::Array<Element*> fileTables;

                for (auto* candidate : getTables())
                    if (candidate->contains (Id::path)
                        and *candidate->get<juce::String> (Id::path) == declaredPath)
                        fileTables.add (candidate);

                if (fileTables.size() == 1)
                    return fileTables.at (0);

                const auto fileStem { juce::File::createFileWithoutCheckingPath (declaredPath)
                                          .getFileNameWithoutExtension() };

                for (auto* candidate : fileTables)
                    if (candidate->id == juce::Identifier (fileStem))
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
