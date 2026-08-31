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
            if (block->isTag (Id::p) and block->contains (Id::templatePath))
                return *block->get<juce::String> (Id::templatePath);

        return {};
    }

    /**
     * @brief Returns @p row's list bullet at blockquote depth @p indent --
     *        the number of authored @c > markers -- and position
     *        @p ordinal among that depth's own list bullets, counted in
     *        authored order.
     *
     * @param row     The row whose @c list column is searched.
     * @param indent  The blockquote nesting depth to search, one per
     *                authored @c >.
     * @param ordinal The bullet's position among @p indent's own list
     *                bullets, counted separately from shape paragraphs
     *                and comment bullets at the same depth.
     * @returns The addressed list bullet, or @c nullptr when none exists
     *          at (@p indent, @p ordinal).
     */
    Element* getSource (Element& row, int indent, int ordinal) const
    {
        Element* item { nullptr };

        getTableCell (row, Id::list)->applyFunctionRecursively (
            [&item, indent, ordinal] (const Element& candidate) -> bool
            {
                if (item == nullptr and candidate.parent->isTag (Id::ul) and candidate.id == Id::list
                    and *candidate.get<int> (Id::level) == indent
                    and *candidate.get<int> (Id::line) == ordinal)
                    item = const_cast<Element*> (&candidate);

                return item == nullptr;
            });

        return item;
    }

    /**
     * @brief Returns @p row's separator bullet at blockquote depth
     *        @p indent and position @p ordinal -- the same (depth,
     *        ordinal) coordinate addressing getSource(), read from the
     *        @c separator column instead of the @c list column.
     *
     * @param row     The row whose @c separator column is searched.
     * @param indent  The blockquote nesting depth to search, one per
     *                authored @c >.
     * @param ordinal The bullet's position among @p indent's own list
     *                bullets, matching getSource()'s addressing.
     * @returns The addressed separator bullet, or @c nullptr when none
     *          exists at (@p indent, @p ordinal).
     */
    Element* getSeparator (Element& row, int indent, int ordinal) const
    {
        Element* item { nullptr };

        getTableCell (row, Id::separator)->applyFunctionRecursively (
            [&item, indent, ordinal] (const Element& candidate) -> bool
            {
                if (item == nullptr and candidate.parent->isTag (Id::ul) and candidate.id == Id::list
                    and *candidate.get<int> (Id::level) == indent
                    and *candidate.get<int> (Id::line) == ordinal)
                    item = const_cast<Element*> (&candidate);

                return item == nullptr;
            });

        return item;
    }

    /**
     * @brief Returns @p row's comment bullet paired with the shape
     *        paragraph at blockquote depth @p indent and shape ordinal
     *        @p ordinal -- the comment counted at position @p ordinal
     *        among @p indent's own comment bullets, its own running count
     *        kept independently of, but authored in lockstep with, that
     *        depth's shape paragraphs.
     *
     * @param row     The row whose @c list column is searched.
     * @param indent  The blockquote nesting depth to search, one per
     *                authored @c >.
     * @param ordinal The paired shape paragraph's own ordinal at
     *                @p indent, answered by the comment bullet counted at
     *                the same position among @p indent's own comment
     *                bullets.
     * @returns The paired comment bullet, or @c nullptr when @p indent
     *          carries no comment at that position.
     */
    Element* getComment (Element& row, int indent, int ordinal) const
    {
        Element* item { nullptr };

        getTableCell (row, Id::list)->applyFunctionRecursively (
            [&item, indent, ordinal] (const Element& candidate) -> bool
            {
                if (item == nullptr and candidate.parent->isTag (Id::ul) and candidate.id == Id::comment
                    and *candidate.get<int> (Id::level) == indent
                    and *candidate.get<int> (Id::line) == ordinal)
                    item = const_cast<Element*> (&candidate);

                return item == nullptr;
            });

        return item;
    }

    /**
     * @brief Walks forward from @p line, depth-first through children
     *        then siblings, unwinding to each ancestor's next sibling
     *        until leaving the enclosing table cell, to the next
     *        paragraph or list item carrying a @c shape ordinal.
     *
     * @param line The structure line to advance from.
     * @returns The next structure line in document order, or @c nullptr
     *          when @p line is the cell's last one.
     */
    Element* getNextLine (Element& line) const
    {
        auto* walk { &line };
        Element* candidate { nullptr };

        while (candidate == nullptr
               or not (candidate->contains (Id::shape)
                       and (candidate->isTag (Id::p) or candidate->id == Id::list)))
        {
            candidate = walk->firstChild != nullptr ? walk->firstChild : walk->nextSibling;

            while (candidate == nullptr and not isBlockType (*walk, map::BlockType::tableCell))
            {
                walk = walk->parent;
                candidate = walk->nextSibling;
            }

            if (candidate == nullptr)
                return nullptr;

            walk = candidate;
        }

        return candidate;
    }

    /**
     * @brief Returns @p row's @p name-named binding item declared under
     *        the shape line whose own @c shape ordinal matches @p line's
     *        -- the document-order position shared by a shape paragraph
     *        or list source and the bindings authored beneath it.
     *
     * @param row    The row whose @p column cell is searched.
     * @param column The column @p line's own binding is read from.
     * @param line   The structure line whose paired bindings are searched
     *               for @p name.
     * @param name   The binding name to find.
     * @returns The addressed binding item, or @c nullptr when @p line
     *          declares no binding named @p name.
     */
    Element* getBinding (Element& row, const juce::Identifier& column, Element& line,
                         const juce::Identifier& name) const
    {
        Element* item { nullptr };
        const auto ordinal { *line.get<int> (Id::shape) };

        getTableCell (row, column)->applyFunctionRecursively (
            [&item, ordinal, name] (const Element& candidate) -> bool
            {
                if (item == nullptr and candidate.parent->isTag (Id::ul) and candidate.id != Id::list
                    and candidate.id == name and *candidate.get<int> (Id::shape) == ordinal)
                    item = const_cast<Element*> (&candidate);

                return item == nullptr;
            });

        return item;
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

    /**
     * @brief Answers whether @p value is an @-sigiled address naming a
     *        column -- @c \@alias:table:column, three or more
     *        colon-separated parts.
     *
     * @param value The authored value to test.
     * @returns @c true when @p value is an @-sigiled address with a
     *          column part.
     */
    static bool isColumnAddress (const juce::String& value)
    {
        const auto parts { jam::Strings::fromTokens (
            value, juce::String::charToString (Chars::colon), {}) };

        return value.startsWithChar (Chars::at) and parts.size() > 2;
    }

    /**
     * @brief Returns @p value's column part -- an isColumnAddress()
     *        address's last colon-separated segment, converted to a
     *        valid identifier.
     *
     * @pre isColumnAddress (value)
     *
     * @param value The @-sigiled column address to read.
     * @returns @p value's column name.
     */
    static juce::Identifier getColumn (const juce::String& value)
    {
        const auto parts { jam::Strings::fromTokens (
            value, juce::String::charToString (Chars::colon), {}) };

        return juce::Identifier (jam::Format::toValidID (parts.at (parts.size() - 1).trim()));
    }

    /**
     * The manifest file's own relative path, stamped at parse() -- the
     * origin every table declared in the manifest's own file compares
     * against for the manifest-origin exemption.
     */
    juce::String manifestOrigin;

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
        document.manifestOrigin = manifestOrigin;

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

        const auto templateDocument { jam::MarkdownDocument::parse (
            document.getTemplateFile().loadFileAsString()) };

        for (auto* table : document.getTables())
        {
            auto* headerRow { getTableHeaderRow (*table) };

            for (auto* row : document.getTableRows (*table))
            {
                document.addValues (*headerRow, *row);

                for (const auto& column : { Id::structure, Id::list, Id::separator })
                    if (auto* cell { document.getTableCell (*row, column) })
                    {
                        juce::String precedingBinding;
                        addBindings (*cell, precedingBinding);
                        jam::Array<int> ordinals;
                        jam::Array<int> shapeOrdinals;
                        jam::Array<int> commentOrdinals;
                        int lineIndex { 0 };
                        addLines (*cell, 0, ordinals, shapeOrdinals, commentOrdinals, lineIndex,
                            templateDocument);
                    }
            }
        }
    }

    static void addComments (Model& document)
    {
        Element* precedingBlock { nullptr };
        bool precedingBoundToTable { false };

        for (auto* child : document)
        {
            if (isBlockType (*child, map::BlockType::table))
            {
                juce::String comment;

                if (precedingBoundToTable)
                    comment = precedingBlock->getAllSubText();

                child->add<juce::String> (Id::comment, comment);
            }

            const auto isNamedFence { isBlockType (*child, map::BlockType::codeBlock)
                                      and child->contains (Id::info)
                                      and child->get<juce::String> (Id::info)->isNotEmpty() };

            const auto boundToNextTable { child->nextSibling != nullptr
                                          and isBlockType (*child->nextSibling, map::BlockType::table)
                                          and (isBlockType (*child, map::BlockType::paragraph)
                                               or isBlockType (*child, map::BlockType::codeBlock))
                                          and *child->get<juce::String> (Id::path)
                                                  == *child->nextSibling->get<juce::String> (Id::path) };

            if (isNamedFence and not boundToNextTable)
                child->add<juce::String> (Id::comment, child->getAllSubText());

            precedingBlock = child;
            precedingBoundToTable = boundToNextTable;
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

    static void addBindings (Element& scope, juce::String& precedingBinding)
    {
        for (auto* block : scope)
        {
            if (block->isTag (Id::p)
                and jam::Format::getPreColon (block->getAllSubText()).trim()
                        == Id::templatePath.toString())
                precedingBinding.clear();

            if (block->isTag (Id::ul))
                for (auto* item : *block)
                {
                    const auto itemText { item->getAllSubText() };
                    auto value { jam::Format::getPostColon (itemText).trim() };

                    if (value.isEmpty())
                        value = jam::Format::toCamelCase (precedingBinding);

                    item->add<juce::String> (Id::value, value);

                    precedingBinding = jam::Format::getPreColon (value).trim()
                                                == Id::templatePath.toString()
                                            ? juce::String()
                                            : value;
                }

            if (block->isTag (Id::blockquote))
                addBindings (*block, precedingBinding);
        }
    }

    static void addLines (Element& cell, int indent, jam::Array<int>& ordinals,
        jam::Array<int>& shapeOrdinals, jam::Array<int>& commentOrdinals, int& lineIndex,
        const jam::MarkdownDocument& templateDocument)
    {
        if (indent == ordinals.size())
            ordinals.resize (indent + 1);

        if (indent == shapeOrdinals.size())
            shapeOrdinals.resize (indent + 1);

        if (indent == commentOrdinals.size())
            commentOrdinals.resize (indent + 1);

        for (auto* block : cell)
        {
            if (block->isTag (Id::p))
            {
                const auto blockText { block->getAllSubText() };

                if (jam::Format::getPreColon (blockText).trim() == Id::templatePath.toString())
                {
                    const auto shapeName { jam::Format::getPostColon (blockText).trim() };

                    block->add<int> (Id::level, indent);
                    block->add<int> (Id::line, shapeOrdinals.at (indent)++);
                    block->add<juce::String> (Id::templatePath, shapeName);
                    block->add<int> (Id::shape, lineIndex);
                    ++lineIndex;
                }
            }

            if (block->isTag (Id::ul))
                for (auto* item : *block)
                {
                    if (item->id == Id::list)
                    {
                        item->add<int> (Id::level, indent);
                        item->add<int> (Id::line, ordinals.at (indent)++);
                        item->add<int> (Id::shape, lineIndex);
                        ++lineIndex;
                    }
                    else if (item->id == Id::comment)
                    {
                        item->add<int> (Id::level, indent);
                        item->add<int> (Id::line, commentOrdinals.at (indent)++);
                        item->add<int> (Id::shape, lineIndex - 1);
                    }
                    else
                    {
                        item->add<int> (Id::shape, lineIndex - 1);
                    }

                    const auto& blockText { *item->get<juce::String> (Id::value) };

                    if (jam::Format::getPreColon (blockText).trim() == Id::templatePath.toString())
                        item->add<juce::String> (Id::templatePath, jam::Format::getPostColon (blockText).trim());
                }

            if (block->isTag (Id::blockquote))
                addLines (*block, indent + 1, ordinals, shapeOrdinals, commentOrdinals, lineIndex,
                    templateDocument);
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
        const auto isCommentColumn { headerCell.id == Id::comment };
        const auto isCommentProse { isCommentColumn
                                    and (literal == nullptr
                                         or cell.getAllSubText() != literal->getAllSubText()) };
        juce::String authored;

        if (literal != nullptr and not isCommentProse)
        {
            juce::String text { literal->getAllSubText() };

            if (Transforms::contains (transform))
                text = Transforms::getTransformed (transform, text, {});

            authored = isCommentColumn ? text : jam::Format::toLiteral (text);
        }
        else if (isCommentProse)
        {
            authored = *cell.get<juce::String> (Id::rawText);
        }
        else
        {
            authored = cell.getAllSubText();
        }

        const auto isFormatColumn { headerCell.id == Id::format };
        const auto isManifestRow { isOutputTable (*row.parent) };
        auto value { authored.isNotEmpty() or isManifestRow or isCommentColumn ? authored
                                                                               : precedingAuthored };

        if (not isCommentColumn and value.startsWithChar (Chars::at))
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
