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
     * @returns @p alias's resolved symbol -- the matching index row's
     *          @c symbol cell's own resolved value, formatted through the
     *          index's @c format column when it declares one -- or an
     *          empty string when @p alias is not an @-sigiled identifier
     *          or is absent from @p file's index.
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
                            if (auto* symbolCell {
                                    getTableCell (*table, Id::symbol, juce::Identifier (aliasText)) })
                                return *symbolCell->get<juce::String> (Id::value);
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
     * @brief Answers whether @p value is a shape address -- @-sigiled,
     *        and resolving through @p row's own file's index to a symbol
     *        naming a @c .cast file.
     *
     * @param row   The row @p value is resolved against.
     * @param value The authored value to test.
     * @returns @c true when @p value is a shape address.
     */
    bool isShape (Element& row, const juce::String& value) const
    {
        return isAddress (value)
               and juce::File::createFileWithoutCheckingPath (
                       getValue (row, jam::Format::getPreColon (value).trim()))
                       .hasFileExtension (Extensions::cast);
    }

    /**
     * @brief Answers whether @p value is an @-sigiled reference -- the
     *        sigil law of SPEC §4: a @-sigiled cell is a reference, a bare
     *        word is data.
     *
     * @param value The authored value to test.
     * @returns @c true when @p value starts with the @ sigil.
     */
    static bool isAddress (const juce::String& value) noexcept
    {
        return value.startsWithChar (Chars::at);
    }

    /**
     * @brief Returns @p scope's own shape paragraph's resolved template
     *        file path -- the first non-nested paragraph stamped with
     *        @c Id::templatePath.
     *
     * @param scope The blockquote scope searched for its own shape
     *              paragraph.
     * @returns The resolved @c .cast file path, or an empty string when
     *          @p scope carries no shape paragraph of its own.
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
     * Bullets carrying no @c Id::line stamp -- map bullets -- are
     * skipped.
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
        return getPairedListItem (row, Id::list, indent, ordinal);
    }

    /**
     * @brief Returns @p row's separator bullet at blockquote depth
     *        @p indent and position @p ordinal -- the same (depth,
     *        ordinal) coordinate addressing getSource(), read from the
     *        @c separator column instead of the @c list column.
     *
     * Bullets carrying no @c Id::line stamp -- the row join and map
     * bullets -- are skipped.
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
        return getPairedListItem (row, Id::separator, indent, ordinal);
    }

    /**
     * @brief Returns @p row's separator column's row join -- the leading
     *        @c >-less @c list bullet carrying no @c Id::line stamp.
     *
     * @param row The row whose @c separator column is searched.
     * @returns The row-join bullet, or @c nullptr when @p row's separator
     *          column declares none.
     */
    Element* getRowJoin (Element& row) const
    {
        return getPairedItem (row, Id::separator,
            [] (const Element& candidate)
            { return candidate.id == Id::list and not candidate.contains (Id::line); });
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
        return getPairedItem (row, Id::list,
            [indent, ordinal] (const Element& candidate)
            {
                return candidate.id == Id::comment and *candidate.get<int> (Id::level) == indent
                       and *candidate.get<int> (Id::line) == ordinal;
            });
    }

    /**
     * @brief Returns @p row's structure scope's shape paragraph at
     *        blockquote depth @p indent and position @p ordinal among that
     *        depth's own shape paragraphs.
     *
     * @param row     The row whose @c structure column is searched.
     * @param indent  The blockquote nesting depth to search, one per
     *                authored @c >.
     * @param ordinal The paragraph's position among @p indent's own shape
     *                paragraphs, counted in authored order.
     * @returns The addressed shape paragraph, or @c nullptr when none
     *          exists at (@p indent, @p ordinal).
     */
    Element* getParagraph (Element& row, int indent, int ordinal) const
    {
        Element* item { nullptr };

        getTableCell (row, Id::structure)->applyFunctionRecursively (
            [&item, indent, ordinal] (const Element& candidate) -> bool
            {
                if (item == nullptr and candidate.isTag (Id::p) and candidate.contains (Id::line)
                    and *candidate.get<int> (Id::level) == indent
                    and *candidate.get<int> (Id::line) == ordinal)
                    item = const_cast<Element*> (&candidate);

                return item == nullptr;
            });

        return item;
    }

    /**
     * @brief Returns @p row's @p occurrence-th map table for @p line's
     *        own shape -- the @p occurrence-th non-empty, @c >-less
     *        @c list bullet sharing @p line's own @c shape ordinal,
     *        resolved through getTable().
     *
     * @param row        The row whose @c list column is searched.
     * @param line       The structure line whose maps are searched,
     *                   matched by its own @c shape ordinal.
     * @param occurrence The map's position among @p line's own maps,
     *                   counted in authored order.
     * @returns The addressed map table, or @c nullptr when @p line
     *          declares no map at @p occurrence.
     */
    Element* getMap (Element& row, Element& line, int occurrence) const
    {
        Element* item { nullptr };
        int matchOrdinal { 0 };

        getTableCell (row, Id::list)->applyFunctionRecursively (
            [&item, &matchOrdinal, &line, occurrence] (const Element& candidate) -> bool
            {
                if (item == nullptr and candidate.parent->isTag (Id::ul) and candidate.id == Id::list
                    and not candidate.contains (Id::line)
                    and *candidate.get<int> (Id::shape) == *line.get<int> (Id::shape)
                    and candidate.get<juce::String> (Id::value)->isNotEmpty()
                    and matchOrdinal++ == occurrence)
                    item = const_cast<Element*> (&candidate);

                return item == nullptr;
            });

        return item != nullptr ? getTable (row, *item->get<juce::String> (Id::value)) : nullptr;
    }

    /**
     * @brief Returns @p line's own next structure line, skipping past
     *        every source @p line's own arity, read through @p arityOf,
     *        consumes, when @p line is itself a shape line -- the
     *        arity-bounded walk Shapes::getLineAfter() and
     *        Items::getPrivateShapes() each read through.
     *
     * @tparam ArityOf A callable invoked as @c arityOf(candidate),
     *                 returning @p candidate's own arity.
     * @param line    The structure line to advance past.
     * @param arityOf The callable each visited line's own arity is read
     *                through.
     * @returns The next structure line, or @c nullptr when none remains.
     */
    template <typename ArityOf>
    Element* getNextShapeLine (Element& line, ArityOf&& arityOf) const
    {
        auto* cursor { getNextLine (line) };

        if (line.isTag (Id::p))
            for (int occurrence { 0 }; cursor != nullptr and occurrence < arityOf (line); ++occurrence)
                cursor = getNextShapeLine (*cursor, arityOf);

        return cursor;
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

                if (isBlockType (*walk, map::BlockType::tableCell))
                    break;

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
     *        -- the document-order position, at or after @p line itself,
     *        shared by a shape paragraph or list source and the bindings
     *        authored beneath it. Restricting the search to @p line's own
     *        position onward disambiguates a shape-valued binding's own
     *        @c shape ordinal, shared with the shape line above it, from
     *        that shape line's own same-named bindings authored earlier.
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
        auto reachedLine { false };

        getTableCell (row, column)->applyFunctionRecursively (
            [&item, &reachedLine, &line, ordinal, name] (const Element& candidate) -> bool
            {
                if (not reachedLine and &candidate == &line)
                    reachedLine = true;

                if (reachedLine and item == nullptr and candidate.parent->isTag (Id::ul)
                    and candidate.id != Id::list and candidate.id == name
                    and *candidate.get<int> (Id::shape) == ordinal)
                    item = const_cast<Element*> (&candidate);

                return item == nullptr;
            });

        return item;
    }

    /**
     * @brief Answers whether @p table is an output table -- reads @p table's
     *        own @c wiring stamp, set once at parse() for every table:
     *        declared in the manifest's own file, not the index, and
     *        carrying a @c structure column on its header row.
     *
     * @param table The table to test.
     * @returns @c true when @p table is an output table.
     */
    bool isOutputTable (Element& table) const noexcept
    {
        return *table.get<bool> (Id::wiring);
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
     * @brief Resolves @p reference's address -- @c alias\[:table\], or,
     *        absent an alias hit, @c table naming a table of @p row's own
     *        document -- to the table it names.
     *
     * @param row       The row @p reference's alias part is resolved
     *                  against, and whose own document the local form
     *                  resolves against.
     * @param reference The address: an alias name, optionally followed by
     *                  @c :table, or, absent an alias hit, a table name of
     *                  @p row's own document.
     * @returns The referenced table, or @c nullptr when neither the alias
     *          nor the local table name resolves.
     */
    Element* getTable (Element& row, juce::StringRef reference) const
    {
        const auto parts { jam::Strings::fromTokens (
            reference, juce::String::charToString (Chars::colon), {}) };
        const auto sourceName { parts.size() > 0 ? parts.at (0).trim() : juce::String{} };
        const auto declaredPath { getValue (row, sourceName) };

        if (declaredPath.isNotEmpty())
        {
            const auto tableName { parts.size() > 1 ? parts.at (1).trim() : juce::String{} };
            return getTable (declaredPath, tableName);
        }

        return getTable (*row.parent->get<juce::String> (Id::path),
                         sourceName.trimCharactersAtStart (juce::String::charToString (Chars::at)));
    }

    /**
     * @brief Answers whether @p value is an @-sigiled address naming a
     *        column -- @c \@alias:table:column when @p value's first
     *        segment hits @p row's own index, or, absent an alias hit,
     *        the local form @c \@table:column -- carrying no @c = filter
     *        at the column part.
     *
     * @param row   The row @p value's first segment is resolved against,
     *              to tell the alias form from the local form.
     * @param value The authored value to test.
     * @returns @c true when @p value is an @-sigiled address with a
     *          column part.
     */
    bool isColumnAddress (Element& row, const juce::String& value) const
    {
        const auto parts { jam::Strings::fromTokens (
            value, juce::String::charToString (Chars::colon), {}) };
        const auto sourceName { parts.size() > 0 ? parts.at (0).trim() : juce::String{} };
        const auto columnIndex { getValue (row, sourceName).isNotEmpty() ? 2 : 1 };

        return isAddress (value) and parts.size() > columnIndex
               and not parts.at (columnIndex).containsChar (Chars::equals);
    }

    /**
     * @brief Returns @p value's column part -- an isColumnAddress()
     *        address's own column segment, converted to a valid
     *        identifier.
     *
     * @pre isColumnAddress (row, value)
     *
     * @param row   The row @p value's first segment is resolved against,
     *              to tell the alias form from the local form.
     * @param value The @-sigiled column address to read.
     * @returns @p value's column name.
     */
    juce::Identifier getColumn (Element& row, const juce::String& value) const
    {
        const auto parts { jam::Strings::fromTokens (
            value, juce::String::charToString (Chars::colon), {}) };
        const auto sourceName { parts.size() > 0 ? parts.at (0).trim() : juce::String{} };
        const auto columnIndex { getValue (row, sourceName).isNotEmpty() ? 2 : 1 };

        return juce::Identifier (jam::Format::toValidID (parts.at (columnIndex).trim()));
    }

    /**
     * @brief Answers whether @p value is an @-sigiled address naming a
     *        cell-match filter -- @c \@alias:table:column=value when
     *        @p value's first segment hits @p row's own index, or, absent
     *        an alias hit, the local form @c \@table:column=value --
     *        carrying an @c = sign at the filter part.
     *
     * @param row   The row @p value's first segment is resolved against,
     *              to tell the alias form from the local form.
     * @param value The authored value to test.
     * @returns @c true when @p value is an @-sigiled address with a
     *          filter part.
     */
    bool isFilteredAddress (Element& row, const juce::String& value) const
    {
        const auto parts { jam::Strings::fromTokens (
            value, juce::String::charToString (Chars::colon), {}) };
        const auto sourceName { parts.size() > 0 ? parts.at (0).trim() : juce::String{} };
        const auto filterIndex { getValue (row, sourceName).isNotEmpty() ? 2 : 1 };

        return isAddress (value) and parts.size() > filterIndex
               and parts.at (filterIndex).containsChar (Chars::equals);
    }

    /**
     * @brief Returns @p value's filter column -- an isFilteredAddress()
     *        address's own filter segment, up to its @c =, converted to a
     *        valid identifier.
     *
     * @pre isFilteredAddress (row, value)
     *
     * @param row   The row @p value's first segment is resolved against,
     *              to tell the alias form from the local form.
     * @param value The @-sigiled filter address to read.
     * @returns @p value's filter column name.
     */
    juce::Identifier getFilterColumn (Element& row, const juce::String& value) const
    {
        const auto parts { jam::Strings::fromTokens (
            value, juce::String::charToString (Chars::colon), {}) };
        const auto sourceName { parts.size() > 0 ? parts.at (0).trim() : juce::String{} };
        const auto filterIndex { getValue (row, sourceName).isNotEmpty() ? 2 : 1 };
        const auto filterPart { parts.at (filterIndex) };

        return juce::Identifier (jam::Format::toValidID (
            filterPart.upToFirstOccurrenceOf (
                juce::String::charToString (Chars::equals), false, false).trim()));
    }

    /**
     * @brief Returns @p value's filter value -- an isFilteredAddress()
     *        address's own filter segment, from its @c = onward.
     *
     * @pre isFilteredAddress (row, value)
     *
     * @param row   The row @p value's first segment is resolved against,
     *              to tell the alias form from the local form.
     * @param value The @-sigiled filter address to read.
     * @returns @p value's filter value, empty when the filter matches a
     *          blank cell.
     */
    juce::String getFilterValue (Element& row, const juce::String& value) const
    {
        const auto parts { jam::Strings::fromTokens (
            value, juce::String::charToString (Chars::colon), {}) };
        const auto sourceName { parts.size() > 0 ? parts.at (0).trim() : juce::String{} };
        const auto filterIndex { getValue (row, sourceName).isNotEmpty() ? 2 : 1 };
        const auto filterPart { parts.at (filterIndex) };

        return filterPart.fromFirstOccurrenceOf (
            juce::String::charToString (Chars::equals), false, false).trim();
    }

private:
    /**
     * @brief Returns @p column's own first list item under @p row matching
     * @p predicate -- the one scan getSource(), getSeparator(),
     * getRowJoin(), and getComment() each read through.
     *
     * @param row       The row whose @p column cell is searched.
     * @param column    The column searched.
     * @param predicate The per-candidate test, called as
     *                  @c predicate(candidate).
     * @returns The first matching item, or @c nullptr when none exists.
     */
    template <typename Predicate>
    Element* getPairedItem (Element& row, const juce::Identifier& column, Predicate&& predicate) const
    {
        Element* item { nullptr };

        getTableCell (row, column)->applyFunctionRecursively (
            [&item, &predicate] (const Element& candidate) -> bool
            {
                if (item == nullptr and candidate.parent->isTag (Id::ul) and predicate (candidate))
                    item = const_cast<Element*> (&candidate);

                return item == nullptr;
            });

        return item;
    }

    /**
     * @brief Returns @p column's own list item under @p row at
     *        blockquote depth @p indent and position @p ordinal -- the
     *        one predicate getSource() and getSeparator() each read
     *        through, parameterized by @p column.
     *
     * @param row     The row whose @p column cell is searched.
     * @param column  The column searched.
     * @param indent  The blockquote nesting depth to search, one per
     *                authored @c >.
     * @param ordinal The bullet's position among @p indent's own list
     *                bullets, counted separately from shape paragraphs
     *                and comment bullets at the same depth.
     * @returns The addressed list item, or @c nullptr when none exists
     *          at (@p indent, @p ordinal).
     */
    Element* getPairedListItem (Element& row, const juce::Identifier& column, int indent, int ordinal) const
    {
        return getPairedItem (row, column,
            [indent, ordinal] (const Element& candidate)
            {
                return candidate.id == Id::list and candidate.contains (Id::line)
                       and *candidate.get<int> (Id::level) == indent
                       and *candidate.get<int> (Id::line) == ordinal;
            });
    }

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

    /**
     * @brief Returns @p document's own index rows' distinct data-file
     *        origins, excluding @p manifestOrigin.
     *
     * @param document       The model whose index rows are read.
     * @param manifestOrigin The manifest's own origin path, excluded from
     *                       the result.
     * @returns Every index row's own @c symbol resolving to a @c .md file
     *          other than @p manifestOrigin, in row order.
     */
    static jam::Array<juce::String>
    getTableOrigins (const Model& document, const juce::String& manifestOrigin)
    {
        jam::Array<juce::String> tableOrigins;

        for (auto* indexRow : document.getTableRows (Id::index))
        {
            const auto pathCell { document.getTableValue (*indexRow, Id::symbol) };

            if (juce::File::createFileWithoutCheckingPath (pathCell).hasFileExtension (
                    Extensions::md)
                and pathCell != manifestOrigin)
                tableOrigins.add (pathCell);
        }

        return tableOrigins;
    }

    /**
     * @brief Parses @p documentFile as @p document's own manifest,
     *        splices in every data file its index declares, stamps
     *        every table's own @c wiring, then stamps every row through
     *        addRow().
     *
     * @param document     The model parsed into.
     * @param documentFile The manifest file to parse.
     */
    static void parse (Model& document, const juce::File& documentFile)
    {
        const auto parent { documentFile.getParentDirectory() };
        const auto manifestOrigin { documentFile.getRelativePathFrom (parent) };

        document.directory = parent;
        document.manifestOrigin = manifestOrigin;

        document.appendChildren (jam::MarkdownDocument::parse (
            documentFile.loadFileAsString(), manifestOrigin));

        parse (document, parent, getTableOrigins (document, manifestOrigin));
        addComments (document);

        for (auto* table : document.getTables())
            table->add<bool> (Id::wiring,
                not table->isTag (Id::index) and table->contains (Id::path)
                    and *table->get<juce::String> (Id::path) == document.manifestOrigin
                    and document.getTableCell (*document.getTableHeaderRow (*table), Id::structure)
                            != nullptr);

        for (auto* table : document.getTables())
            for (auto* row : document.getTableRows (*table))
                addRow (document, *row);
    }

    /**
     * @brief Returns @p row's own per-depth map-bullet count and blank-bullet
     *        count -- @p listCell's own list-bullet count in excess of
     *        @p structureCell's own shape count at each depth, and, from
     *        @p listCell alone, each depth's own blank-valued bullet count.
     *
     * @param listCell      @p row's own @c list cell, or @c nullptr when
     *                      absent.
     * @param structureCell @p row's own @c structure cell, or @c nullptr
     *                      when absent.
     * @param document      The model @p row belongs to.
     * @param row           The row whose columns are counted.
     * @returns Each depth's own map-bullet excess, paired with each
     *          depth's own blank-valued bullet count.
     */
    static std::pair<jam::Array<int>, jam::Array<int>> getExcess (Element* listCell,
        Element* structureCell, const Model& document, Element& row)
    {
        jam::Array<int> counts;
        jam::Array<int> blanks;
        jam::Array<int> structureCounts;

        if (listCell != nullptr)
            addListCount (*listCell, 0, counts, blanks, document, row);

        if (structureCell != nullptr)
            addListCount (*structureCell, 0, structureCounts, document, row);

        counts.resize (juce::jmax (counts.size(), structureCounts.size()));
        structureCounts.resize (counts.size());
        blanks.resize (counts.size());

        jam::Array<int> excess;
        excess.resize (counts.size());

        for (int level { 0 }; level < counts.size(); ++level)
            excess.set (level, juce::jmax (0, counts.at (level) - structureCounts.at (level)));

        return { std::move (excess), std::move (blanks) };
    }

    /**
     * @brief Stamps @p row's cells with their resolved values, then walks
     *        its @c list, @c structure, and @c separator columns to
     *        stamp every bullet and paragraph with its structure-line
     *        addressing -- level, ordinal, and shared @c shape ordinal --
     *        and every map bullet with the shape ordinal it belongs to.
     *
     * @param document The model @p row belongs to.
     * @param row      The row stamped.
     */
    static void addRow (Model& document, Element& row)
    {
        document.addValues (*document.getTableHeaderRow (*row.parent), row);
        auto* listCell { document.getTableCell (row, Id::list) };
        auto* structureCell { document.getTableCell (row, Id::structure) };
        auto* separatorCell { document.getTableCell (row, Id::separator) };

        if (listCell != nullptr)
            addBindings (*listCell, document, row);

        if (structureCell != nullptr)
            addBindings (*structureCell, document, row);

        if (separatorCell != nullptr)
            addBindings (*separatorCell, document, row);

        auto [excess, blanks] { getExcess (listCell, structureCell, document, row) };

        jam::Array<int> shapeOrdinals;
        jam::Array<int> listShapeOrdinals;
        jam::Array<int> separatorShapeOrdinals;
        jam::Array<int> structureExcess;
        jam::Array<int> rowJoinExcess { 1 };

        if (structureCell != nullptr)
            addColumn (*structureCell, structureExcess, shapeOrdinals, document, row);
        if (listCell != nullptr)
            addColumn (*listCell, excess, listShapeOrdinals, document, row);
        if (separatorCell != nullptr)
            addColumn (*separatorCell, rowJoinExcess, separatorShapeOrdinals, document, row);

        addMaps (document, row, blanks, shapeOrdinals);
    }

    /**
     * @brief Stamps @p cell's own structure lines through addLines(),
     *        from a fresh ordinal, comment-ordinal, and map-ordinal
     *        state and a document-order @c shape ordinal starting at
     *        zero.
     *
     * @param cell          The column cell to stamp.
     * @param excess        Each depth's own map-bullet count, addLines()'s
     *                      own map/expansion split.
     * @param shapeOrdinals Each depth's next shape-paragraph ordinal,
     *                      advanced by addLines().
     * @param document      The model @p row belongs to.
     * @param row           The row @p cell belongs to.
     */
    static void addColumn (Element& cell, jam::Array<int>& excess,
        jam::Array<int>& shapeOrdinals, const Model& document, Element& row)
    {
        jam::Array<int> ordinals;
        jam::Array<int> commentOrdinals, mapOrdinal, paragraphOwner;
        int lineIndex { 0 };

        addLines (cell, 0, ordinals, shapeOrdinals, commentOrdinals, mapOrdinal, excess, lineIndex,
            document, row, paragraphOwner);
    }

    /**
     * @brief Stamps every table with its own header-adjacent @c comment
     *        -- a preceding paragraph or named code block bound to that
     *        table, and every named code block not bound to a following
     *        table with its own prose as its @c comment.
     *
     * @param document The model whose top-level blocks are walked.
     */
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

    /**
     * @brief Parses every one of @p tableOrigins' own data files, in
     *        parallel through Jobs::run(), and splices each parsed
     *        document into @p document.
     *
     * @param document     The model each parsed data file is spliced
     *                     into.
     * @param parent       The directory @p tableOrigins' own relative
     *                     paths are resolved against.
     * @param tableOrigins The data files' own relative paths to parse.
     */
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

    /**
     * @brief Stamps every binding item under @p scope with its resolved
     *        value, through the recursive overload, from an empty
     *        preceding binding name.
     *
     * @param scope    The blockquote scope whose binding items are
     *                 stamped.
     * @param document The model @p row belongs to.
     * @param row      The row @p scope belongs to.
     */
    static void addBindings (Element& scope, const Model& document, Element& row)
    {
        juce::String precedingBinding;
        addBindings (scope, precedingBinding, document, row);
    }

    static void
    addBindings (Element& scope, juce::String& precedingBinding, const Model& document, Element& row)
    {
        for (auto* block : scope)
        {
            if (block->isTag (Id::p) and document.isShape (row, block->getAllSubText()))
                precedingBinding.clear();

            if (block->isTag (Id::ul))
                for (auto* item : *block)
                {
                    const auto itemText { item->getAllSubText() };
                    auto value { jam::Format::getPostColon (itemText).trim() };

                    if (value.isEmpty() and item->id != Id::list)
                        value = jam::Format::toCamelCase (precedingBinding);

                    item->add<juce::String> (Id::value, value);

                    precedingBinding = document.isShape (row, value) ? juce::String{} : value;
                }

            if (block->isTag (Id::blockquote))
                addBindings (*block, precedingBinding, document, row);
        }
    }

    /**
     * @brief Counts @p scope's own @c list bullets per blockquote depth,
     *        through the blank-counting overload, discarding the blank
     *        count.
     *
     * @param scope    The blockquote scope whose list bullets are counted.
     * @param indent   The blockquote depth @p scope itself sits at.
     * @param counts   Each depth's own list-bullet count, resized and
     *                 accumulated.
     * @param document The model @p row belongs to.
     * @param row      The row @p scope belongs to.
     */
    static void
    addListCount (Element& scope, int indent, jam::Array<int>& counts, const Model& document, Element& row)
    {
        jam::Array<int> blanks;
        addListCount (scope, indent, counts, blanks, document, row);
    }

    /**
     * @brief Counts @p scope's own @c list bullets per blockquote depth
     *        -- every bullet whose value is not a column address -- and,
     *        among them, how many carry an empty value.
     *
     * @param scope    The blockquote scope whose list bullets are counted.
     * @param indent   The blockquote depth @p scope itself sits at.
     * @param counts   Each depth's own list-bullet count, resized and
     *                 accumulated.
     * @param blanks   Each depth's own empty-valued list-bullet count,
     *                 resized and accumulated.
     * @param document The model @p row belongs to.
     * @param row      The row @p scope belongs to.
     */
    static void addListCount (Element& scope, int indent, jam::Array<int>& counts, jam::Array<int>& blanks,
        const Model& document, Element& row)
    {
        if (indent == counts.size())
            counts.resize (indent + 1);

        if (indent == blanks.size())
            blanks.resize (indent + 1);

        for (auto* block : scope)
        {
            if (block->isTag (Id::ul))
                for (auto* item : *block)
                    if (item->id == Id::list
                        and not document.isColumnAddress (row, *item->get<juce::String> (Id::value)))
                    {
                        ++counts.at (indent);

                        if (item->get<juce::String> (Id::value)->isEmpty())
                            ++blanks.at (indent);
                    }

            if (block->isTag (Id::blockquote))
                addListCount (*block, indent + 1, counts, blanks, document, row);
        }
    }

    /**
     * @brief Stamps one list-column @p item with its structure-line
     *        addressing -- an expansion @c list bullet with level,
     *        ordinal, and a fresh @c shape ordinal; a comment bullet
     *        with level, ordinal, and its enclosing shape's ordinal; a
     *        shape-valued binding with level and its enclosing paragraph's
     *        own ordinal, shared with the shape line above it; any other
     *        bullet with its enclosing paragraph's own ordinal -- and,
     *        when @p item's own value is a shape address, with its
     *        resolved template path and info.
     *
     * A @c list bullet counts as an expansion once its own position
     * among @p indent's map-eligible bullets reaches @p excess's map
     * count for that depth, or immediately when it is a column address;
     * every earlier bullet at that depth is a map bullet instead,
     * advancing @p mapOrdinal.
     *
     * @param item            The list-column item stamped.
     * @param indent          The blockquote depth @p item sits at.
     * @param ordinals        Each depth's next expansion-bullet ordinal.
     * @param commentOrdinals Each depth's next comment-bullet ordinal.
     * @param mapOrdinal      Each depth's own map-bullet position,
     *                        advanced past the depth's non-expansion
     *                        bullets.
     * @param excess          Each depth's own map-bullet count, the
     *                        threshold @p mapOrdinal is compared against.
     * @param lineIndex       The document-order @c shape ordinal,
     *                        advanced by one whenever @p item is stamped
     *                        as an expansion.
     * @param document        The model @p row belongs to.
     * @param row             The row @p item belongs to.
     * @param paragraphOwner  Each depth's own last shape-paragraph
     *                        ordinal, the owner a binding or shape-valued
     *                        binding at that depth is stamped with.
     */
    static void addItem (Element& item, int indent, jam::Array<int>& ordinals,
        jam::Array<int>& commentOrdinals, jam::Array<int>& mapOrdinal, const jam::Array<int>& excess,
        int& lineIndex, const Model& document, Element& row, const jam::Array<int>& paragraphOwner)
    {
        const auto blockText { *item.get<juce::String> (Id::value) };
        const auto isShapeValue { document.isShape (row, blockText) };

        if (item.id == Id::list)
        {
            const auto isColumn { document.isColumnAddress (row, blockText) };
            if (isColumn or mapOrdinal.at (indent) >= excess.at (indent))
            {
                item.add<int> (Id::level, indent);
                item.add<int> (Id::line, ordinals.at (indent)++);
                item.add<int> (Id::shape, lineIndex);
                ++lineIndex;
            }
            if (not isColumn) ++mapOrdinal.at (indent);
        }
        else if (item.id == Id::comment)
        {
            item.add<int> (Id::level, indent);
            item.add<int> (Id::line, commentOrdinals.at (indent)++);
            item.add<int> (Id::shape, lineIndex - 1);
        }
        else if (isShapeValue)
        {
            item.add<int> (Id::level, indent);
            item.add<int> (Id::shape, paragraphOwner.at (indent));
        }
        else
            item.add<int> (Id::shape, paragraphOwner.at (indent));

        if (isShapeValue)
        {
            item.add<juce::String> (Id::templatePath,
                document.getValue (row, jam::Format::getPreColon (blockText).trim()));
            item.add<juce::String> (Id::info, jam::Format::getPostColon (blockText).trim());
        }
    }

    /**
     * @brief Stamps @p block with its structure-line addressing -- level,
     *        ordinal, resolved template path, info, and @c shape ordinal
     *        -- when @p block is itself a shape paragraph.
     *
     * @param block          The paragraph block stamped when it is a shape.
     * @param indent         The blockquote depth @p block sits at.
     * @param shapeOrdinals  Each depth's next shape-paragraph ordinal,
     *                       advanced when @p block is stamped.
     * @param lineIndex      The document-order @c shape ordinal, advanced
     *                       by one when @p block is stamped.
     * @param paragraphOwner Each depth's own last shape-paragraph ordinal,
     *                       set to @p block's own ordinal when stamped.
     * @param document       The model @p row belongs to.
     * @param row            The row @p block belongs to.
     */
    static void addParagraph (Element& block, int indent, jam::Array<int>& shapeOrdinals, int& lineIndex,
        jam::Array<int>& paragraphOwner, const Model& document, Element& row)
    {
        const auto blockText { block.getAllSubText() };

        if (document.isShape (row, blockText))
        {
            block.add<int> (Id::level, indent);
            block.add<int> (Id::line, shapeOrdinals.at (indent)++);
            block.add<juce::String> (Id::templatePath,
                document.getValue (row, jam::Format::getPreColon (blockText).trim()));
            block.add<juce::String> (Id::info, jam::Format::getPostColon (blockText).trim());
            block.add<int> (Id::shape, lineIndex);
            paragraphOwner.set (indent, lineIndex);
            ++lineIndex;
        }
    }

    /**
     * @brief Walks @p cell's own paragraphs, list items, and nested
     *        blockquote scopes, stamping each shape paragraph with its
     *        level, ordinal, resolved template path, info, and @c shape
     *        ordinal, delegating each list item to addItem(), and
     *        recursing into every nested blockquote at @p indent + 1.
     *
     * @param cell            The column cell walked.
     * @param indent          The blockquote depth @p cell's own
     *                        top-level content sits at.
     * @param ordinals        Each depth's next expansion-bullet ordinal,
     *                        passed through to addItem().
     * @param shapeOrdinals   Each depth's next shape-paragraph ordinal,
     *                        advanced for every stamped paragraph.
     * @param commentOrdinals Each depth's next comment-bullet ordinal,
     *                        passed through to addItem().
     * @param mapOrdinal      Each depth's own map-bullet position, passed
     *                        through to addItem().
     * @param excess          Each depth's own map-bullet count, passed
     *                        through to addItem().
     * @param lineIndex       The document-order @c shape ordinal,
     *                        advanced by one for every stamped paragraph
     *                        or expansion bullet.
     * @param document        The model @p row belongs to.
     * @param row             The row @p cell belongs to.
     * @param paragraphOwner  Each depth's own last shape-paragraph
     *                        ordinal, advanced by every stamped paragraph
     *                        and passed through to addItem().
     */
    static void addLines (Element& cell, int indent, jam::Array<int>& ordinals,
        jam::Array<int>& shapeOrdinals, jam::Array<int>& commentOrdinals, jam::Array<int>& mapOrdinal,
        jam::Array<int>& excess, int& lineIndex, const Model& document, Element& row,
        jam::Array<int>& paragraphOwner)
    {
        if (indent == ordinals.size()) ordinals.resize (indent + 1);
        if (indent == shapeOrdinals.size()) shapeOrdinals.resize (indent + 1);
        if (indent == commentOrdinals.size()) commentOrdinals.resize (indent + 1);
        if (indent == mapOrdinal.size()) mapOrdinal.resize (indent + 1);
        if (indent == excess.size()) excess.resize (indent + 1);
        if (indent == paragraphOwner.size()) paragraphOwner.resize (indent + 1);

        for (auto* block : cell)
        {
            if (block->isTag (Id::p))
                addParagraph (*block, indent, shapeOrdinals, lineIndex, paragraphOwner, document, row);
            if (block->isTag (Id::ul))
                for (auto* item : *block)
                    addItem (*item, indent, ordinals, commentOrdinals, mapOrdinal, excess, lineIndex,
                        document, row, paragraphOwner);
            if (block->isTag (Id::blockquote))
                addLines (*block, indent + 1, ordinals, shapeOrdinals, commentOrdinals, mapOrdinal,
                    excess, lineIndex, document, row, paragraphOwner);
        }
    }

    /**
     * @brief Stamps every @c >-less @c list bullet under @p scope with
     *        the @c shape ordinal of the shape paragraph it belongs to
     *        -- the owning paragraph counted from @p scope's own depth's
     *        trailing paragraphs, one paragraph per run of map bullets
     *        closed by a blank-valued one -- then recurses into every
     *        nested blockquote.
     *
     * @param document       The model @p row belongs to.
     * @param row            The row @p scope belongs to.
     * @param scope          The blockquote scope whose map bullets are
     *                       stamped.
     * @param indent         The blockquote depth @p scope itself sits at.
     * @param blanks         Each depth's own empty-valued list-bullet
     *                       count.
     * @param paragraphCount Each depth's own shape-paragraph count.
     * @param group          Each depth's own count of map-bullet runs
     *                       closed so far, advanced past each
     *                       blank-valued bullet.
     */
    static void addMaps (const Model& document, Element& row, Element& scope, int indent,
        const jam::Array<int>& blanks, const jam::Array<int>& paragraphCount, jam::Array<int>& group)
    {
        if (indent == group.size())
            group.resize (indent + 1);

        for (auto* block : scope)
        {
            if (block->isTag (Id::ul))
                for (auto* item : *block)
                    if (item->id == Id::list and not item->contains (Id::line))
                    {
                        const auto groupCount { (indent < blanks.size() ? blanks.at (indent) : 0) + 1 };
                        const auto totalParagraphs {
                            indent < paragraphCount.size() ? paragraphCount.at (indent) : 0
                        };
                        const auto owner { totalParagraphs >= groupCount
                                               ? totalParagraphs - groupCount + group.at (indent)
                                               : -1 };

                        if (owner >= 0)
                            if (auto* paragraph { document.getParagraph (row, indent, owner) })
                                item->add<int> (Id::shape, *paragraph->get<int> (Id::shape));

                        if (item->get<juce::String> (Id::value)->isEmpty())
                            ++group.at (indent);
                    }

            if (block->isTag (Id::blockquote))
                addMaps (document, row, *block, indent + 1, blanks, paragraphCount, group);
        }
    }

    /**
     * @brief Stamps every @c >-less @c list-column map bullet under
     *        @p row's list column with the @c shape ordinal of the shape
     *        paragraph it belongs to, through the recursive overload.
     *
     * @param document       The model @p row belongs to.
     * @param row            The row whose map bullets are stamped.
     * @param blanks         Each depth's own empty-valued list-bullet
     *                       count, from addListCount().
     * @param paragraphCount Each depth's own shape-paragraph count.
     */
    static void addMaps (const Model& document, Element& row, const jam::Array<int>& blanks,
        const jam::Array<int>& paragraphCount)
    {
        if (auto* listCell { document.getTableCell (row, Id::list) })
        {
            jam::Array<int> group;
            addMaps (document, row, *listCell, 0, blanks, paragraphCount, group);
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

    /**
     * @brief Returns @p cell's own authored text -- its literal child,
     *        transformed and formatted for its column kind, its stamped
     *        comment prose, or its own subtext, in that order of
     *        preference.
     *
     * @param cell            The cell whose authored text is read.
     * @param literal         @p cell's own backtick code child, or
     *                        @c nullptr when it carries none.
     * @param isCommentColumn Whether @p cell belongs to a @c comment or
     *                        @c brief column.
     * @param isCommentProse  Whether @p cell's own comment column carries
     *                        prose rather than a literal.
     * @param transform       @p cell's own @c format cell's transform
     *                        name, applied to a literal's text.
     * @returns @p cell's own authored text, resolved in preference order.
     */
    static juce::String getAuthoredText (Element& cell, const Element* literal, bool isCommentColumn,
        bool isCommentProse, const juce::String& transform)
    {
        if (literal != nullptr and not isCommentProse)
        {
            juce::String text { literal->getAllSubText() };

            if (Transforms::contains (transform))
                text = Transforms::getTransformed (transform, text, {});

            return isCommentColumn ? text : jam::Format::toLiteral (text);
        }

        if (isCommentProse)
            return *cell.get<juce::String> (Id::rawText);

        return cell.getAllSubText();
    }

    /**
     * @brief Stamps @p cell with its resolved value -- the authored
     *        literal or comment prose, transformed by @p cell's own
     *        @c format cell when present, then resolved through
     *        getValue() when the result is an @-sigiled reference outside
     *        the @c alias and @c comment columns.
     *
     * A blank cell resolves to an empty string, always -- it renders
     * nothing and elides in templates like an absent trailing value
     * (SPEC §5.1). There is no inheritance from a preceding cell.
     *
     * @param headerCell @p cell's own header cell, naming its column.
     * @param row        The row @p cell belongs to.
     * @param cell       The cell stamped with its resolved value.
     */
    void addValue (Element& headerCell, Element& row, Element& cell)
    {
        juce::String transform;

        if (auto* formatCell { cell.nextSibling })
            if (formatCell->id == Id::format)
                transform = formatCell->getAllSubText();

        const auto* literal { getLiteral (cell) };
        const auto isCommentColumn { headerCell.id == Id::comment or headerCell.id == Id::brief };
        const auto isCommentProse { isCommentColumn
                                    and (literal == nullptr
                                         or cell.getAllSubText() != literal->getAllSubText()) };

        auto value { getAuthoredText (cell, literal, isCommentColumn, isCommentProse, transform) };

        if (not isCommentColumn and headerCell.id != Id::alias and isAddress (value))
            value = getValue (row, value);

        if (literal == nullptr and Transforms::contains (transform))
            value = Transforms::getTransformed (transform, value, {});

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
        auto* headerCell { headerRow.firstChild };
        auto* cell { row.firstChild };

        while (cell != nullptr)
        {
            jassert (headerCell != nullptr);

            addValue (*headerCell, row, *cell);
            headerCell = headerCell->nextSibling;
            cell = cell->nextSibling;
        }
    }

    /**
     * @brief Resolves @p declaredPath's own unnamed table -- its one
     *        non-index table, or, when it declares more than one, the
     *        table whose id matches its own file's stem.
     *
     * @param declaredPath The data file whose unnamed table is resolved.
     * @returns The resolved table, or @c nullptr when @p declaredPath
     *          declares no table, or more than one and none matches its
     *          own file stem.
     */
    Element* getUnnamedTable (const juce::String& declaredPath) const
    {
        jam::Array<Element*> fileTables;

        for (auto* candidate : *this)
            if (isBlockType (*candidate, map::BlockType::table) and not candidate->isTag (Id::index))
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

    /**
     * @brief Resolves @p declaredPath's own table -- @p tableName's own
     *        table when named, or, absent one, @p declaredPath's one
     *        table, or the table whose id matches its own file's stem
     *        when @p declaredPath declares more than one.
     *
     * @param declaredPath The data file whose table is resolved.
     * @param tableName    The table name to match, or an empty string to
     *                     resolve @p declaredPath's own single or
     *                     stem-matching table.
     * @returns The resolved table, or @c nullptr when none resolves.
     */
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

        return getUnnamedTable (declaredPath);
    }

    juce::File directory;

    /**
     * The manifest file's own relative path, stamped at parse() -- the
     * origin every table declared in the manifest's own file compares
     * against for the manifest-origin exemption.
     */
    juce::String manifestOrigin;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Model)
};
