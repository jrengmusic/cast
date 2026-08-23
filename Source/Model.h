#pragma once
#include <JuceHeader.h>
#include "generated/Identifiers.h"
#include "Operators.h"

static constexpr int indefiniteTimeoutMs { -1 };

template <typename Function>
static void runJobs (int count, Function&& function)
{
    juce::ThreadPool pool;

    for (int index { 0 }; index < count; ++index)
        pool.addJob ([&function, index] { function (index); });

    pool.removeAllJobs (false, indefiniteTimeoutMs);
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
            parse (*document, documentFile);

        return document;
    }

    juce::String getValue (juce::StringRef file, juce::StringRef alias) const
    {
        if (alias.isNotEmpty() and *alias.text == Chars::at)
        {
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
        }

        return {};
    }

    juce::String getValue (Element& row, const juce::String& alias) const
    {
        const auto origin { *row.parent->get<juce::String> (Id::path) };
        return getValue (origin, alias);
    }

    const juce::String& getValue (Element& row, const juce::Identifier& column) const
    {
        return *getTableCell (row, column)->get<juce::String> (Id::value);
    }

    juce::String getStructure (Element& scope) const
    {
        for (auto* block : scope)
            if (block->isTag (Id::p))
            {
                const auto blockText { block->getAllSubText() };

                if (jam::Format::getPreColon (blockText).trim() == Id::templatePath.toString())
                    return jam::Format::getPostColon (blockText).trim();
            }

        return {};
    }

    bool isOutputTable (Element& table) const noexcept
    {
        auto* headerRow { getTableHeaderRow (table) };
        return not table.isTag (Id::index) and headerRow != nullptr
               and getTableCell (*headerRow, Id::file) != nullptr;
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
        const auto tableName { parts.size() > 1 ? parts.at (1).trim() : juce::String() };
        const auto declaredPath { getValue (row, sourceName) };

        return declaredPath.isNotEmpty() ? getTables (declaredPath, tableName) : nullptr;
    }

private:
    static void parse (Model& document, const juce::File& documentFile)
    {
        const auto parent { documentFile.getParentDirectory() };
        const auto manifestOrigin { documentFile.getRelativePathFrom (parent) };

        document.path = parent;
        document.appendChildren (jam::MarkdownDocument::parse (
            documentFile.loadFileAsString(), manifestOrigin));

        jam::Array<juce::File> tableFiles;
        jam::Array<juce::String> tableOrigins;

        for (auto* indexRow : document.getTableRows (Id::index))
        {
            const auto pathCell { document.getTableValue (*indexRow, Id::symbol) };

            if (juce::File::createFileWithoutCheckingPath (pathCell).hasFileExtension (
                    Extensions::md)
                and pathCell != manifestOrigin)
            {
                tableFiles.add (parent.getChildFile (pathCell));
                tableOrigins.add (pathCell);
            }
        }

        parse (document, tableFiles, tableOrigins);

        for (auto* table : document.getTables())
        {
            auto* headerRow { getTableHeaderRow (*table) };

            for (auto* row : document.getTableRows (*table))
            {
                document.addValues (*headerRow, *row);

                for (const auto& column : { Id::structure, Id::placeholder, Id::separator })
                    if (auto* cell { document.getTableCell (*row, column) })
                        addBindings (*cell);
            }
        }
    }

    static void parse (Model& document, const jam::Array<juce::File>& tableFiles,
                       const jam::Array<juce::String>& tableOrigins)
    {
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
            document.appendChildren (std::move (table));
    }

    static void addBindings (Element& scope)
    {
        juce::String precedingBinding;

        for (auto* block : scope)
        {
            if (block->isTag (Id::ul))
                for (auto* item : *block)
                {
                    const auto itemText { item->getAllSubText() };
                    auto value { jam::Format::getPostColon (itemText).trim() };

                    if (value.isEmpty())
                        value = jam::Format::toCamelCase (precedingBinding);

                    item->add<juce::String> (Id::value, value);

                    precedingBinding = value;
                }

            if (block->isTag (Id::blockquote))
                addBindings (*block);
        }
    }

    static juce::String getCellValue (Element& cell)
    {
        const Element* codeChild { nullptr };

        cell.applyFunctionRecursively (
            [&codeChild] (const Element& node) -> bool
            {
                if (codeChild == nullptr and node.isTag (Id::code))
                    codeChild = &node;

                return codeChild == nullptr;
            });

        if (codeChild != nullptr)
            return jam::Format::toLiteral (codeChild->getAllSubText());

        return cell.getAllSubText();
    }

    void addValue (Element& headerCell, Element& row, Element& cell, juce::String& precedingAuthored)
    {
        const auto authored { getCellValue (cell) };
        const auto isFormatColumn { headerCell.id == Id::format };
        const auto isManifestRow { isOutputTable (*row.parent) };
        auto value { authored.isNotEmpty() or isManifestRow ? authored : precedingAuthored };

        if (value.startsWithChar (Chars::at))
            value = getValue (row, value);

        if (auto* formatCell { cell.nextSibling })
            if (formatCell->id == Id::format)
            {
                const auto operation { formatCell->getAllSubText() };

                if (operation.isNotEmpty())
                    value = Transforms::getTransformed (operation, value, {});
            }

        if (not isFormatColumn)
            precedingAuthored = authored;

        cell.add<juce::String> (Id::value, value);
    }

    void addValues (Element& headerRow, Element& row)
    {
        juce::String precedingAuthored;
        auto* headerCell { headerRow.firstChild };
        auto* cell { row.firstChild };

        while (cell != nullptr)
        {
            addValue (*headerCell, row, *cell, precedingAuthored);
            headerCell = headerCell->nextSibling;
            cell = cell->nextSibling;
        }
    }

    Element* getTables (const juce::String& declaredPath, const juce::String& tableName) const
    {
        if (tableName.isNotEmpty())
        {
            for (auto* candidate : getTables (juce::Identifier (jam::Format::toValidID (tableName))))
                if (candidate->contains (Id::path) and *candidate->get<juce::String> (Id::path) == declaredPath)
                    return candidate;

            return nullptr;
        }

        jam::Array<Element*> fileTables;

        for (auto* candidate : getTables())
            if (candidate->contains (Id::path) and *candidate->get<juce::String> (Id::path) == declaredPath)
                fileTables.add (candidate);

        if (fileTables.size() == 1)
            return fileTables.at (0);

        const auto fileStem { juce::File::createFileWithoutCheckingPath (declaredPath)
                                  .getFileNameWithoutExtension() };

        for (auto* candidate : fileTables)
            if (candidate->id == juce::Identifier (jam::Format::toValidID (fileStem)))
                return candidate;

        return nullptr;
    }

    juce::File path;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Model)
};
