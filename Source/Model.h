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

    /** Brings the base class's header-row lookup into Model's own scope. */
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
    static std::unique_ptr<Model> parse (const juce::File& documentFile);

    /**
     * @brief Parses @p text as a manifest sourced from @p origin, under
     *        @p directory, with @p reflowWidths set on the returned Model
     *        before parsing.
     *
     * @param text         The manifest source text to parse.
     * @param origin       @p text's own relative path.
     * @param directory    @p origin's own parent directory.
     * @param reflowWidths The column widths set on the returned Model
     *                     before parsing.
     * @returns The parsed Model.
     */
    static std::unique_ptr<Model> parse (const juce::String& text, const juce::String& origin,
        const juce::File& directory, const jam::HashMap<juce::Identifier, int>& reflowWidths);

    /**
     * @brief Returns every @c ## format row's own resolved @c name mapped
     *        to its @c width, excluding the @c line-wrap row.
     *
     * @returns Every reflowed column's own identifier mapped to its width.
     */
    jam::HashMap<juce::Identifier, int> getReflowWidths() const;

    /**
     * @brief Returns the @c line-wrap row's own resolved @c width, among
     *        every @c ## format table's own rows.
     *
     * @returns The @c line-wrap row's width, or
     *          jam::MarkdownWriter::defaultLineWrap when no @c ## format
     *          table declares one.
     */
    int getLineWrap() const;

    /**
     * @brief Returns the manifest's own relative path, stamped at parse().
     *
     * @returns manifestOrigin.
     */
    const juce::String& getManifestOrigin() const noexcept;

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
    juce::String getValue (juce::StringRef file, juce::StringRef alias) const;

    /**
     * @brief Resolves @p alias against @p row's own file's index.
     *
     * @param row   The row whose file's index @p alias is resolved
     *              against.
     * @param alias The @-sigiled alias to resolve.
     * @returns @p alias's symbol, or an empty string when @p alias does not
     *          resolve.
     */
    juce::String getValue (const Element& row, const juce::String& alias) const;

    /**
     * @brief Returns @p row's resolved value for @p column.
     *
     * @param row    The row @p column's cell is read from.
     * @param column The column whose cell value is read.
     * @returns @p row's resolved value for @p column.
     */
    const juce::String& getValue (const Element& row, const juce::Identifier& column) const;

    /**
     * @brief Returns @p row's resolved value for @p column, or an empty
     *        string when @p row declares no such column.
     *
     * @param row    The row @p column's cell is read from.
     * @param column The column whose cell value is read.
     * @returns @p row's resolved value for @p column, or an empty string
     *          when @p row carries no cell for @p column.
     */
    juce::String getColumnValue (const Element& row, const juce::Identifier& column) const;

    /**
     * @brief Answers whether @p value is a shape address -- @-sigiled,
     *        and resolving through @p row's own file's index to a symbol
     *        naming a @c .cast file.
     *
     * @param row   The row @p value is resolved against.
     * @param value The authored value to test.
     * @returns @c true when @p value is a shape address.
     */
    bool isShape (const Element& row, const juce::String& value) const;

    /**
     * @brief Returns the Identifier that a bracketed reserved name takes in
     *        the parsed document -- @p word enclosed in square brackets,
     *        then normalized as the parser normalizes a list item's key.
     *
     * @param word The reserved word, without brackets.
     * @returns The Identifier of @p word's own bracketed form.
     */
    static juce::Identifier getReservedName (const juce::Identifier& word);

    /**
     * @brief Answers whether @p value is an @-sigiled reference -- a
     *        @-sigiled cell is a reference, a bare word is data.
     *
     * @param value The authored value to test.
     * @returns @c true when @p value starts with the @ sigil.
     */
    static bool isAddress (const juce::String& value) noexcept;

    /**
     * @brief Returns @p scope's own shape line's resolved template file
     *        path -- the first non-nested paragraph stamped with
     *        @c Id::templatePath, or, absent one, an item-shape list's own
     *        first item when it carries @c Id::templatePath -- the first
     *        line of the column is the row's own shape, whether authored
     *        as a bare paragraph or a @c - [list]: item.
     *
     * @param scope The blockquote scope searched for its own shape line.
     * @returns The resolved @c .cast file path, or an empty string when
     *          @p scope carries no shape line of its own.
     */
    juce::String getStructure (const Element& scope) const;

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
     *                and description bullets at the same depth.
     * @returns The addressed list bullet, or @c nullptr when none exists
     *          at (@p indent, @p ordinal).
     */
    const Element* getSource (const Element& row, int indent, int ordinal) const;

    /**
     * @brief Returns @p row's @p index-th (0-based) consecutive column-
     *        address source line authored directly after @p sourceOrdinal
     *        at blockquote depth @p indent.
     *
     * The walk stops at the first source line that is absent or not
     * isColumnAddress() -- every source line up to and including @p index
     * must itself be a column address for a result to return.
     *
     * @param row           The row @p sourceOrdinal's child column
     *                      addresses are read from.
     * @param indent        The blockquote nesting depth to search.
     * @param sourceOrdinal The structure position after which child column
     *                      addresses are read.
     * @param index         The 0-based position among the consecutive
     *                      column-address lines following @p sourceOrdinal.
     * @returns The addressed column-address source line, or @c nullptr
     *          when fewer than @p index + 1 consecutive column-address
     *          lines follow @p sourceOrdinal.
     */
    const Element* getColumnAddress (const Element& row, int indent, int sourceOrdinal, int index) const;

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
    const Element* getSeparator (const Element& row, int indent, int ordinal) const;

    /**
     * @brief Returns @p row's separator column's row join -- the leading
     *        @c >-less @c list bullet carrying no @c Id::line stamp.
     *
     * @param row The row whose @c separator column is searched.
     * @returns The row-join bullet, or @c nullptr when @p row's separator
     *          column declares none.
     */
    const Element* getRowJoin (const Element& row) const;

    /**
     * @brief Returns @p row's description bullet paired with @p line -- the
     *        list column bullet whose @c shape ordinal equals the @c shape
     *        ordinal of @p line.
     *
     * @param row  The row whose @c list column is searched.
     * @param line The shape paragraph or wrapper whose description is read.
     * @returns The paired description bullet, or @c nullptr when @p line
     *          has no description.
     */
    const Element* getDescription (const Element& row, const Element& line) const;

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
    const Element* getParagraph (const Element& row, int indent, int ordinal) const;

    /**
     * @brief Returns @p row's @p occurrence-th map table for @p line's
     *        own shape -- the @p occurrence-th non-empty, @c >-less
     *        @c list bullet whose @c shape ordinal equals @p line's owner,
     *        read through getOwner(), resolved through getTable(). A
     *        wrapper reads the maps of the paragraph that owns it.
     *
     * @param row        The row whose @c list column is searched.
     * @param line       The structure line whose maps are searched,
     *                   matched by its owner's @c shape ordinal.
     * @param occurrence The map's position among @p line's own maps,
     *                   counted in authored order.
     * @returns The addressed map table, or @c nullptr when @p line
     *          declares no map at @p occurrence.
     */
    const Element* getMap (const Element& row, const Element& line, int occurrence) const;

    /**
     * @brief Returns the @c value cell of the row named @p name, searched
     *        through @p line's own map tables in authored order.
     *
     * @param row  The row whose @c list column holds @p line's maps.
     * @param line The structure line whose maps are searched, matched by
     *             its own @c shape ordinal.
     * @param name The map row's name, matched in the @c value column.
     * @returns The first matching @c value cell, or @c nullptr when none
     *          of @p line's maps carries a row named @p name.
     */
    const Element* getMapCell (const Element& row, const Element& line, const juce::Identifier& name) const;

    /**
     * @brief Walks forward from @p line, depth-first through children
     *        then siblings, unwinding to each ancestor's next sibling
     *        until leaving the enclosing table cell, to the next structure
     *        line -- a shape paragraph, a @c list bullet, or a wrapper --
     *        carrying a @c shape ordinal.
     *
     * @param line The structure line to advance from.
     * @returns The next structure line in document order, or @c nullptr
     *          when @p line is the cell's last one.
     */
    const Element* getNextLine (const Element& line) const;

    /**
     * @brief Stamps every wrapper's chain across the output tables' rows.
     *
     * For each wrapper in a row's @c structure cell, every element after
     * the wrapper in document order, up to the chain end, is stamped
     * @c chain with the wrapper's @c shape ordinal. An element that
     * already carries @c chain keeps it, thus the outermost chain wins.
     * The wrapper itself is its parent's binding. Its own chain does not
     * stamp it.
     *
     * @tparam GetChainEnd Callable taking a wrapper element and returning
     *                     the @c const @c Element* that ends the wrapper's
     *                     chain, or @c nullptr when the chain runs to the
     *                     end of the cell.
     * @param getChainEnd  The callable that returns each wrapper's chain
     *                     end. The chain end is not stamped.
     */
    template <typename GetChainEnd>
    void addChains (const GetChainEnd& getChainEnd)
    {
        const auto getSuccessor = [] (Element& element, const Element& cell) -> Element*
        {
            if (element.firstChild != nullptr) return element.firstChild;

            for (const Element* climb { &element }; climb != &cell; climb = climb->parent)
                if (climb->nextSibling != nullptr) return climb->nextSibling;

            return nullptr;
        };

        for (auto* table : *root)
            if (isTable (*table) and isOutputTable (*table))
                for (auto* row : *table)
                    if (auto* cell { row->getChildByID (Id::structure) })
                        for (auto* wrapper { cell }; wrapper != nullptr; wrapper = getSuccessor (*wrapper, *cell))
                            if (isWrapper (*wrapper))
                            {
                                static const auto listMarker { getReservedName (Id::list) };
                                const Element* end { getChainEnd (*wrapper) };
                                const auto shape { *wrapper->get<int> (Id::shape) };
                                jam::Array<int> chainedShapes;
                                chainedShapes.add (shape);

                                for (auto* chained { getSuccessor (*wrapper, *cell) };
                                     chained != nullptr and chained != end;
                                     chained = getSuccessor (*chained, *cell))
                                    if (chained->contains (Id::shape))
                                    {
                                        const auto isLine { chained->isTag (Id::p) or chained->id == listMarker
                                                            or isWrapper (*chained) };
                                        auto isChained { false };

                                        if (isLine)
                                        {
                                            chainedShapes.add (*chained->template get<int> (Id::shape));
                                            isChained = not isWrapper (*chained)
                                                        or chainedShapes.contains (getOwner (*chained));
                                        }
                                        else
                                            isChained = chainedShapes.contains (getOwner (*chained));

                                        if (isChained and not chained->contains (Id::chain))
                                            chained->template add<int> (Id::chain, shape);
                                    }
                            }
    }

    /**
     * @brief Returns @p row's @p name-named binding item owned by @p line
     *        -- the first non-@c list item named @p name whose owner, read
     *        through getOwner(), equals @p line's own @c shape ordinal. A
     *        wrapper's own bindings carry the wrapper's ordinal, and the
     *        wrapper item itself carries its parent's, thus neither one
     *        reaches the other's tokens.
     *
     * @param row    The row whose @p column cell is searched.
     * @param column The column @p line's own binding is read from.
     * @param line   The structure line whose paired bindings are searched
     *               for @p name.
     * @param name   The binding name to find.
     * @returns The addressed binding item, or @c nullptr when @p line
     *          declares no binding named @p name.
     */
    const Element* getBinding (const Element& row, const juce::Identifier& column, const Element& line,
                         const juce::Identifier& name) const;

    /**
     * @brief Answers whether @p table is an output table -- reads @p table's
     *        own @c wiring stamp, set once at parse() for every table:
     *        declared in the manifest's own file, not the index, and
     *        carrying a @c structure column on its header row.
     *
     * @param table The table to test.
     * @returns @c true when @p table is an output table.
     */
    bool isOutputTable (const Element& table) const noexcept;

    /**
     * @brief Answers whether @p row's own structure column carries a
     *        bullet keyed @p marker anywhere in its scope -- the scan
     *        hasRegionBegin() and hasRegionEnd() each read through,
     *        parameterized by their own marker.
     *
     * @param row    The row whose structure column is searched.
     * @param marker The binding marker id searched for.
     * @returns @c true when @p row's structure column carries @p marker.
     */
    bool hasRegionBinding (const Element& row, const juce::Identifier& marker) const;

    /**
     * @brief Answers whether @p row declares a @c - [begin]: binding.
     *
     * @param row The row to test.
     * @returns @c true when @p row carries a @c \[begin\] binding.
     */
    bool hasRegionBegin (const Element& row) const;

    /**
     * @brief Answers whether @p row declares a @c - [end]: binding.
     *
     * @param row The row to test.
     * @returns @c true when @p row carries an @c \[end\] binding.
     */
    bool hasRegionEnd (const Element& row) const;

    /**
     * @brief Answers whether @p row is a region row -- carrying both its
     *        own @c \[begin\] and @c \[end\] bindings. This
     *        is the one region-row definition Validator and Writer both
     *        read: Validator::isRegionPaired() catches a row that carries
     *        exactly one binding, which this predicate alone would treat
     *        as a whole-file row.
     *
     * @param row The row to test.
     * @returns @c true when @p row carries both region bindings.
     */
    bool isRegionRow (const Element& row) const;

    /**
     * @brief Resolves @p relativePath against the manifest's own
     *        directory.
     *
     * @param relativePath The path to resolve, relative to the manifest's
     *                     directory.
     * @returns The resolved file.
     */
    juce::File getFile (juce::StringRef relativePath) const;

    /**
     * @brief Returns the file name of @p path, resolved against the
     *        manifest's own directory.
     *
     * @param path The path to resolve, relative to the manifest's
     *             directory.
     * @returns The resolved file's name, or an empty string when @p path
     *          is empty.
     */
    juce::String getFileName (juce::StringRef path) const;

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
    const Element* getTable (const Element& row, juce::StringRef reference) const;

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
    bool isColumnAddress (const Element& row, const juce::String& value) const;

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
    juce::Identifier getColumn (const Element& row, const juce::String& value) const;

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
    bool isFilteredAddress (const Element& row, const juce::String& value) const;

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
    juce::Identifier getFilterColumn (const Element& row, const juce::String& value) const;

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
    juce::String getFilterValue (const Element& row, const juce::String& value) const;

    /**
     * @brief Answers whether @p candidate is a wrapper -- a structure
     *        list item, not a @c list bullet, whose value is a shape
     *        address.
     *
     * @param candidate The element tested.
     * @returns @c true when @p candidate is a wrapper.
     */
    static bool isWrapper (const Element& candidate);

    /**
     * @brief Returns the @c shape ordinal of the line that owns @p item --
     *        a wrapper's parent, stamped as @c owner, or, for any other
     *        binding, the nearest shape paragraph or wrapper above it,
     *        stamped as @c shape.
     *
     * @param item The binding item whose owner is read.
     * @returns The owning line's @c shape ordinal.
     */
    static int getOwner (const Element& item);

private:
    /**
     * @brief Returns @p column's own first list item under @p row matching
     * @p predicate -- the one scan getSource(), getSeparator(),
     * getRowJoin(), and getDescription() each read through.
     *
     * @param row       The row whose @p column cell is searched.
     * @param column    The column searched.
     * @param predicate The per-candidate test, called as
     *                  @c predicate(candidate).
     * @returns The first matching item, or @c nullptr when none exists.
     */
    template <typename Predicate>
    const Element* getPairedItem (const Element& row, const juce::Identifier& column, Predicate&& predicate) const
    {
        const Element* item { nullptr };

        getTableCell (row, column)->applyFunctionRecursively (
            [&item, &predicate] (const Element& candidate) -> bool
            {
                if (item == nullptr and candidate.parent->isTag (Id::ul) and predicate (candidate))
                    item = &candidate;

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
     *                and description bullets at the same depth.
     * @returns The addressed list item, or @c nullptr when none exists
     *          at (@p indent, @p ordinal).
     */
    const Element* getPairedListItem (const Element& row, const juce::Identifier& column, int indent, int ordinal) const;

    /**
     * @brief Answers whether @p element's own @c type equals @p blockType.
     *
     * @param element   The element whose block type is tested.
     * @param blockType The block type compared against.
     * @returns @c true when @p element carries a @c type equal to
     *          @p blockType.
     */
    static bool isBlockType (const Element& element, int blockType);

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
    getTableOrigins (const Model& document, const juce::String& manifestOrigin);

    /**
     * @brief Parses @p text as @p document's own manifest, sourced from
     *        @p origin under @p directory, splices in every data file its
     *        index declares, stamps every table's own @c wiring, then
     *        stamps every row through addRow().
     *
     * @param document  The model parsed into.
     * @param text      The manifest source text to parse.
     * @param origin    @p text's own relative path.
     * @param directory @p origin's own parent directory.
     */
    static void parse (Model& document, const juce::String& text, const juce::String& origin,
        const juce::File& directory);

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
    static std::pair<jam::Array<int>, jam::Array<int>> getExcess (const Element* listCell,
        const Element* structureCell, const Model& document, const Element& row);

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
    static void addRow (Model& document, Element& row);

    /**
     * @brief Stamps @p cell's own structure lines through addLines(),
     *        from a fresh ordinal, description-ordinal, and map-ordinal
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
        jam::Array<int>& shapeOrdinals, const Model& document, const Element& row);

    /**
     * @brief Stamps every table with its own header-adjacent @c description
     *        -- a preceding paragraph or named code block bound to that
     *        table, and every named code block not bound to a following
     *        table with its own prose as its @c description.
     *
     * @param document The model whose top-level blocks are walked.
     */
    static void addDescriptions (Model& document);

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
                       const jam::Array<juce::String>& tableOrigins);

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
    static void addBindings (Element& scope, const Model& document, const Element& row);

    /**
     * @brief Stamps every binding item under @p scope with its resolved
     *        value, @p precedingBinding carrying the running binding name
     *        across the recursive walk, reset at each shape paragraph.
     *
     * @param scope             The blockquote scope whose binding items
     *                          are stamped.
     * @param precedingBinding  The running binding name, cleared at each
     *                          shape paragraph and updated at each
     *                          binding item.
     * @param document          The model @p row belongs to.
     * @param row               The row @p scope belongs to.
     */
    static void
    addBindings (Element& scope, juce::String& precedingBinding, const Model& document, const Element& row);

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
    addListCount (const Element& scope, int indent, jam::Array<int>& counts, const Model& document, const Element& row);

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
    static void addListCount (const Element& scope, int indent, jam::Array<int>& counts, jam::Array<int>& blanks,
        const Model& document, const Element& row);

    /**
     * @brief Stamps one list-column @p item with its structure-line
     *        addressing -- an expansion @c list bullet with level,
     *        ordinal, and a fresh @c shape ordinal; a description bullet
     *        with level, ordinal, and its enclosing shape's ordinal; a
     *        wrapper -- a shape-valued binding -- with level, @c owner
     *        (its parent paragraph's ordinal), and its own new @c shape
     *        ordinal; any other binding with its binding owner's ordinal
     *        as @c shape -- and,
     *        when @p item's own value is a shape address, with its
     *        resolved template path and info.
     *
     * A wrapper sets its depth's binding owner to its own ordinal, thus the
     * bindings after it belong to the wrapper.
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
     * @param descriptionOrdinals Each depth's next description-bullet ordinal.
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
     *                        ordinal, the @c owner a wrapper at that depth
     *                        is stamped with.
     * @param bindingOwner    Each depth's own nearest shape paragraph or
     *                        wrapper ordinal, the @c shape a plain binding
     *                        at that depth is stamped with. A wrapper sets
     *                        it to its own ordinal.
     */
    static void addItem (Element& item, int indent, jam::Array<int>& ordinals,
        jam::Array<int>& descriptionOrdinals, jam::Array<int>& mapOrdinal, const jam::Array<int>& excess,
        int& lineIndex, const Model& document, const Element& row, const jam::Array<int>& paragraphOwner,
        jam::Array<int>& bindingOwner);

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
     *                       A wrapper is stamped @c owner with it.
     * @param bindingOwner   Each depth's own nearest shape paragraph or
     *                       wrapper ordinal, set to @p block's own ordinal
     *                       when stamped.
     * @param document       The model @p row belongs to.
     * @param row            The row @p block belongs to.
     */
    static void addParagraph (Element& block, int indent, jam::Array<int>& shapeOrdinals, int& lineIndex,
        jam::Array<int>& paragraphOwner, jam::Array<int>& bindingOwner, const Model& document,
        const Element& row);

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
     * @param descriptionOrdinals Each depth's next description-bullet ordinal,
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
     *                        ordinal, set by every stamped paragraph and
     *                        passed through to addItem(). A wrapper is
     *                        stamped @c owner with it.
     * @param bindingOwner    Each depth's own nearest shape paragraph or
     *                        wrapper ordinal, the owner a plain binding is
     *                        stamped with. A paragraph or wrapper sets it
     *                        to its own ordinal.
     */
    static void addLines (Element& cell, int indent, jam::Array<int>& ordinals,
        jam::Array<int>& shapeOrdinals, jam::Array<int>& descriptionOrdinals, jam::Array<int>& mapOrdinal,
        jam::Array<int>& excess, int& lineIndex, const Model& document, const Element& row,
        jam::Array<int>& paragraphOwner, jam::Array<int>& bindingOwner);

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
    static void addMaps (const Model& document, const Element& row, Element& scope, int indent,
        const jam::Array<int>& blanks, const jam::Array<int>& paragraphCount, jam::Array<int>& group);

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
        const jam::Array<int>& paragraphCount);

    /**
     * @brief Stamps each description bullet in @p scope with the @c owner
     *        ordinal of its paired shape line, then descends into each
     *        blockquote. The description bullet at ordinal k pairs with the
     *        k-th shape line at the same depth in @p structureCell. A shape
     *        line is a shape paragraph or a wrapper.
     *
     * @param structureCell The structure cell that holds the shape lines.
     * @param scope         The list column scope whose bullets are stamped.
     * @param indent        The blockquote depth of @p scope.
     */
    static void addDescriptionReferences (const Element& structureCell, Element& scope, int indent);

    /**
     * @brief Stamps every description bullet in @p row's list column with
     *        the @c owner ordinal of its paired shape line, through the
     *        recursive overload.
     *
     * @param row The row whose description bullets are stamped.
     */
    static void addDescriptionReferences (Element& row);

    /**
     * @brief Returns @p cell's backtick code child, searched depth-first.
     *
     * @param cell The cell searched for a code child.
     * @returns @p cell's code child, or @c nullptr when it carries none --
     *          a cell's literality is this pointer's mere existence.
     */
    static const Element* getLiteral (const Element& cell);

    /**
     * @brief Answers whether @p cell owns a fenced block among its own
     *        direct children -- a code block or mermaid diagram, per
     *        jam::MarkdownDocument::isFencedBlock() -- the same test the
     *        writer runs to decide a cell's own fence status, shared here
     *        so getJoinedValue()'s fence exemption never drifts from it.
     *
     * @param cell The cell tested for a fenced direct child.
     * @returns @c true when @p cell owns a fenced direct child.
     */
    bool isFencedCell (const Element& cell) const;

    /**
     * @brief Returns @p cell's own authored text -- its literal child,
     *        transformed and formatted for its column kind, its stamped
     *        description prose, or its own subtext, in that order of
     *        preference.
     *
     * @param cell            The cell whose authored text is read.
     * @param literal         @p cell's own backtick code child, or
     *                        @c nullptr when it carries none.
     * @param isDescriptionColumn Whether @p cell belongs to a @c description or
     *                        @c brief column.
     * @param isDescriptionProse  Whether @p cell's own description column carries
     *                        prose rather than a literal.
     * @param transform       @p cell's own @c format cell's transform
     *                        name, applied to a literal's text.
     * @returns @p cell's own authored text, resolved in preference order.
     */
    juce::String getAuthoredText (const Element& cell, const Element* literal, bool isDescriptionColumn,
        bool isDescriptionProse, const juce::String& transform) const;

    /**
     * @brief Returns @p value with every newline removed, when
     *        @p headerCell names a column reflowWidths declares and
     *        @p cell owns no fenced direct child (isFencedCell()) --
     *        undoing jam's row-wrap of a grid-table cell's own one-line
     *        value. A fence's own newlines are authored lines, kept as
     *        is.
     *
     * Runs on getAuthoredText()'s own raw return, before addValue()'s
     * own later @c getValue() resolution and @c Transforms::getTransformed()
     * call. For a literal cell (backtick or fence), getAuthoredText()
     * already ran @c jam::Format::toLiteral() on its text internally, and
     * a fenced cell is excluded here, so this call is a no-op on both --
     * a backtick span's own wrap newline is already folded to nothing by
     * jam inside the cell, leaving nothing to remove either way. For a
     * plain, unfenced cell, getAuthoredText() returns raw, unescaped
     * text, so this is the one point a row-wrapped value's real newlines
     * are still present to remove -- before a @c toLiteral-named
     * transform (Transforms.h) could later escape one into the
     * two-character sequence @c \\n, past this function's own reach.
     *
     * @param headerCell @p cell's own header cell, naming its column.
     * @param cell       @p value's own cell, tested for a fenced direct
     *                   child.
     * @param value      @p cell's own authored text, straight from
     *                   getAuthoredText(), possibly row-wrapped.
     * @returns @p value with every newline removed when reflowed and
     *          unfenced, or @p value unchanged otherwise.
     */
    juce::String getJoinedValue (const Element& headerCell, const Element& cell, const juce::String& value) const;

    /**
     * @brief Stamps @p cell with its resolved value -- its own authored
     *        literal or description prose, joined into one line through
     *        getJoinedValue() first when @p headerCell names a
     *        reflowWidths column, then resolved through getValue() when
     *        the joined result is an @-sigiled reference outside the
     *        @c alias, @c description and @c brief columns, then transformed by
     *        @p cell's own @c format cell when present.
     *
     * The join runs first, on getAuthoredText()'s own raw return --
     * see getJoinedValue() for the per-cell-kind order this depends on:
     * a literal cell's own text is already toLiteral-escaped inside
     * getAuthoredText() by the time the join sees it (a no-op there,
     * fenced or not), while a plain cell's text still carries its real,
     * row-wrapped newlines at this point -- removed here, before this
     * function's own @c Transforms::getTransformed() call below could
     * otherwise escape one away under a @c toLiteral-named transform.
     *
     * A blank cell resolves to an empty string, always -- it renders
     * nothing and elides in templates like an absent trailing value.
     * There is no inheritance from a preceding cell.
     *
     * @param headerCell @p cell's own header cell, naming its column.
     * @param row        The row @p cell belongs to.
     * @param cell       The cell stamped with its resolved value.
     */
    void addValue (const Element& headerCell, const Element& row, Element& cell);

    /**
     * @brief Stamps every one of @p row's cells with its resolved value,
     *        through addValue(), each cell's own header cell found by its
     *        column id.
     *
     * @param headerRow @p row's table's header row.
     * @param row       The row whose cells are stamped.
     */
    void addValues (const Element& headerRow, Element& row);

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
    const Element* getUnnamedTable (const juce::String& declaredPath) const;

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
    const Element* getTable (const juce::String& declaredPath, const juce::String& tableName) const;

    /**
     * The manifest file's own parent directory, stamped at parse() -- the
     * directory getFile() resolves every relative path against.
     */
    juce::File directory;

    /**
     * The manifest file's own relative path, stamped at parse() -- the
     * origin every table declared in the manifest's own file compares
     * against for the manifest-origin exemption.
     */
    juce::String manifestOrigin;

    /**
     * The column widths the reader joins by, set at creation.
     */
    jam::HashMap<juce::Identifier, int> reflowWidths;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Model)
};
