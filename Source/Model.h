#pragma once
#include <JuceHeader.h>
#include "generated/Identifiers.h"
#include "Jobs.h"
#include "Transforms.h"

/**
 * @class Model
 * @brief The parsed manifest and every data file it declares, spliced into
 *        one addressable jam::MarkdownDocument, plus the row and reference
 *        lookups the rest of the engine reads it through.
 *
 * A Model is built once, by parse(), from a manifest file and the data
 * files its index declares -- every table lands in the one document tree,
 * addressed by table id and (parent, id) as jam::MarkdownDocument already
 * provides. Model adds no second copy of that tree; it only answers
 * questions the tree does not already answer directly -- resolving an
 * @-sigiled alias to its symbol, reading a structure scope's shape name,
 * and telling an output table apart from a plain data table.
 */
class Model : public jam::MarkdownDocument
{
public:
    /** Constructs an empty model with no parsed tables. */
    Model() = default;

    using jam::MarkdownDocument::getTables;
    using jam::MarkdownDocument::getTableHeaderRow;

    /**
     * @brief Parses @p documentFile as the manifest and every data file its
     *        index declares, the data files parsed in parallel through
     *        Jobs::run(), and returns the resulting Model.
     *
     * @param documentFile The manifest file to parse.
     * @returns The parsed Model, empty when @p documentFile does not exist
     *          as a file.
     */
    static std::unique_ptr<Model> parse (const juce::File& documentFile)
    {
        auto document { std::make_unique<Model>() };

        if (documentFile.existsAsFile())
            parse (*document, documentFile);

        return document;
    }

    /**
     * @brief Resolves @p alias against @p file's index, when @p alias is
     *        an @-sigiled identifier.
     *
     * @param file  The file whose index @p alias is resolved against.
     * @param alias The @-sigiled alias to resolve.
     * @returns @p alias's symbol, or an empty string when @p alias is not
     *          an @-sigiled identifier or is absent from @p file's
     *          index.
     */
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

    /**
     * @brief Resolves @p alias against @p row's own file's index.
     *
     * @param row   The row whose file's index @p alias is resolved
     *              against.
     * @param alias The @-sigiled alias to resolve.
     * @returns @p alias's symbol, or an empty string when @p alias does not
     *          resolve.
     */
    juce::String getValue (Element& row, const juce::String& alias) const
    {
        const auto origin { *row.parent->get<juce::String> (Id::path) };
        return getValue (origin, alias);
    }

    /**
     * @brief Returns @p row's resolved value for @p column.
     *
     * @param row    The row @p column's cell is read from.
     * @param column The column whose cell value is read.
     * @returns @p row's resolved value for @p column.
     */
    const juce::String& getValue (Element& row, const juce::Identifier& column) const
    {
        return *getTableCell (row, column)->get<juce::String> (Id::value);
    }

    /**
     * @brief Reads @p scope's own @c template:\<id\> head, when it carries
     *        one.
     *
     * @param scope The blockquote scope whose head is read.
     * @returns The shape id named by @p scope's head, or an empty string
     *          when @p scope carries no @c template:\<id\> head.
     */
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

    /**
     * @brief Answers whether @p table is an output table -- not the index,
     *        and carrying a @c file column on its header row.
     *
     * @param table The table to test.
     * @returns @c true when @p table is an output table.
     */
    bool isOutputTable (Element& table) const noexcept
    {
        auto* headerRow { getTableHeaderRow (table) };
        return not table.isTag (Id::index) and headerRow != nullptr
               and getTableCell (*headerRow, Id::file) != nullptr;
    }

    /**
     * @brief Returns the manifest's declared template file, resolved
     *        against getOutput().
     *
     * @returns The template file named by the index row whose symbol has
     *          the @c .cast extension.
     */
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

    /**
     * @brief Resolves @p relativePath against the manifest's own
     *        directory.
     *
     * @param relativePath The path to resolve, relative to the manifest's
     *                     directory.
     * @returns The resolved file.
     */
    juce::File getOutput (juce::StringRef relativePath) const
    {
        return path.getChildFile (relativePath);
    }

    /**
     * @brief Resolves @p reference's address -- @c alias\[:table\] -- to
     *        the table it names.
     *
     * @param row       The row @p reference's alias part is resolved
     *                  against.
     * @param reference The address: an alias name, optionally followed by
     *                  @c :table.
     * @returns The referenced table, or @c nullptr when @p reference's
     *          alias does not resolve.
     */
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

        jam::Array<juce::String> tableOrigins;

        for (auto* indexRow : document.getTableRows (Id::index))
        {
            const auto pathCell { document.getTableValue (*indexRow, Id::symbol) };

            if (juce::File::createFileWithoutCheckingPath (pathCell).hasFileExtension (
                    Extensions::md)
                and pathCell != manifestOrigin)
                tableOrigins.add (pathCell);
        }

        parse (document, parent, tableOrigins);

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

    static void parse (Model& document, const juce::File& parent,
                       const jam::Array<juce::String>& tableOrigins)
    {
        jam::Array<jam::MarkdownDocument> parsedTables;
        parsedTables.resize (tableOrigins.size());

        Jobs::run (tableOrigins.size(),
            [&parent, &tableOrigins, &parsedTables] (int index)
            {
                const auto& origin { tableOrigins.at (index) };
                const auto file { parent.getChildFile (origin) };

                parsedTables.at (index) = jam::MarkdownDocument::parse (
                    file.loadFileAsString(), origin);
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
                if (codeChild == nullptr
                    and (node.isTag (Id::code)
                         or (node.contains (Id::type)
                             and *node.get<int> (Id::type) == map::BlockType::codeBlock)))
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
                const auto transform { formatCell->getAllSubText() };

                if (Transforms::contains (transform))
                    value = Transforms::getTransformed (transform, value, {});
            }

        if (not isFormatColumn)
            precedingAuthored = authored;

        cell.add<juce::String> (Id::value, value);
    }

    /**
     * @brief Stamps every one of @p row's cells with its resolved value,
     *        through addValue().
     *
     * @pre @p row carries the same cell count as @p headerRow.
     *
     * @param headerRow @p row's table's header row.
     * @param row       The row whose cells are stamped.
     */
    void addValues (Element& headerRow, Element& row)
    {
        juce::String precedingAuthored;
        auto* headerCell { headerRow.firstChild };
        auto* cell { row.firstChild };

        while (cell != nullptr)
        {
            jassert (headerCell != nullptr);

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
