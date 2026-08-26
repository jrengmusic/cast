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
                for (auto* table : *this)
                {
                    if (table->id == Id::index)
                    {
                        if (table->contains (Id::path) and *table->get<juce::String> (Id::path) == file)
                        {
                            return getTableValue (*table, Id::symbol, juce::Identifier (aliasText));
                        }
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
     *        against getFile().
     *
     * @returns The template file named by the index row whose symbol has
     *          the @c .cast extension.
     */
    juce::File getTemplateFile() const
    {
        juce::File templateFile;

        for (auto* indexRow : getTableRows (Id::index))
        {
            const auto pathCell { getTableValue (*indexRow, Id::symbol) };

            if (juce::File::createFileWithoutCheckingPath (pathCell).hasFileExtension (
                    Extensions::cast))
                templateFile = getFile (pathCell);
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
    juce::File getFile (juce::StringRef relativePath) const
    {
        return directory.getChildFile (relativePath);
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
    Element* getTable (Element& row, juce::StringRef reference) const
    {
        const auto parts { jam::Strings::fromTokens (
            reference, juce::String::charToString (Chars::colon), {}) };
        const auto sourceName { parts.size() > 0 ? parts.at (0).trim() : juce::String() };
        const auto tableName { parts.size() > 1 ? parts.at (1).trim() : juce::String() };
        const auto declaredPath { getValue (row, sourceName) };

        return declaredPath.isNotEmpty() ? getTable (declaredPath, tableName) : nullptr;
    }

private:
    /**
     * @brief Answers whether @p element's own @c type equals @p blockType.
     *
     * @param element   The element whose block type is tested.
     * @param blockType The block type compared against.
     * @returns @c true when @p element carries a @c type equal to
     *          @p blockType.
     */
    static bool isBlockType (const Element& element, int blockType)
    {
        return element.contains (Id::type) and *element.get<int> (Id::type) == blockType;
    }

    static void parse (Model& document, const juce::File& documentFile)
    {
        const auto parent { documentFile.getParentDirectory() };
        const auto manifestOrigin { documentFile.getRelativePathFrom (parent) };

        document.directory = parent;

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
        addComments (document);

        for (auto* table : document.getTables())
        {
            auto* headerRow { getTableHeaderRow (*table) };

            for (auto* row : document.getTableRows (*table))
            {
                document.addValues (*headerRow, *row);

                for (const auto& column : { Id::structure, Id::list, Id::separator })
                    if (auto* cell { document.getTableCell (*row, column) })
                        addBindings (*cell);
            }
        }
    }

    static void addComments (Model& document)
    {
        Element* precedingBlock { nullptr };

        for (auto* child : document)
        {
            if (isBlockType (*child, map::BlockType::table))
            {
                juce::String comment;

                if (precedingBlock != nullptr and precedingBlock->contains (Id::type))
                {
                    const auto precedingType { *precedingBlock->get<int> (Id::type) };

                    if (precedingType == map::BlockType::paragraph
                        or precedingType == map::BlockType::codeBlock)
                        comment = precedingBlock->getAllSubText();
                }

                child->add<juce::String> (Id::comment, comment);
            }

            precedingBlock = child;
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

    /**
     * @brief Returns @p cell's backtick code child, searched depth-first.
     *
     * @param cell The cell searched for a code child.
     * @returns @p cell's code child, or @c nullptr when it carries none --
     *          a cell's literality is this pointer's mere existence.
     */
    static const Element* getLiteral (Element& cell)
    {
        const Element* codeChild { nullptr };

        cell.applyFunctionRecursively (
            [&codeChild] (const Element& node) -> bool
            {
                if (codeChild == nullptr
                    and (node.isTag (Id::code) or isBlockType (node, map::BlockType::codeBlock)))
                    codeChild = &node;

                return codeChild == nullptr;
            });

        return codeChild;
    }

    void addValue (Element& headerCell, Element& row, Element& cell, juce::String& precedingAuthored)
    {
        juce::String transform;

        if (auto* formatCell { cell.nextSibling })
            if (formatCell->id == Id::format)
                transform = formatCell->getAllSubText();

        const auto* literal { getLiteral (cell) };
        juce::String authored;

        if (literal != nullptr)
        {
            juce::String text { literal->getAllSubText() };

            if (Transforms::contains (transform))
                text = Transforms::getTransformed (transform, text, {});

            authored = jam::Format::toLiteral (text);
        }
        else
        {
            authored = cell.getAllSubText();
        }

        const auto isFormatColumn { headerCell.id == Id::format };
        const auto isManifestRow { isOutputTable (*row.parent) };
        auto value { authored.isNotEmpty() or isManifestRow ? authored : precedingAuthored };

        if (value.startsWithChar (Chars::at))
            value = getValue (row, value);

        if (literal == nullptr and Transforms::contains (transform))
            value = Transforms::getTransformed (transform, value, {});

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

    Element* getTable (const juce::String& declaredPath, const juce::String& tableName) const
    {
        if (tableName.isNotEmpty())
        {
            const juce::Identifier tableId { jam::Format::toValidID (tableName) };

            for (auto* candidate : *this)
                if (candidate->id == tableId)
                    if (candidate->contains (Id::path) and *candidate->get<juce::String> (Id::path) == declaredPath)
                        return candidate;

            return nullptr;
        }

        jam::Array<Element*> fileTables;

        for (auto* candidate : *this)
            if (isBlockType (*candidate, map::BlockType::table))
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

    juce::File directory;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Model)
};
