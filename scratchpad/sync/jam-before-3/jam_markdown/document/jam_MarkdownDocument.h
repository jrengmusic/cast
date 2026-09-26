#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct MarkdownDocument
 * @brief CommonMark/GFM tokenizer/tree constructor over jam::Document, plus the markdown table query API.
 *
 * Tables are markdown-domain vocabulary (Id::headerRow/Id::th/map::BlockType::table),
 * so the table query API lives here rather than on jam::Document, which
 * stays free of any table knowledge.
 *
 * A complete document is held const; its readers return \c const \c
 * Element* -- the reader API is const-only, and a const method never
 * returns mutable interior access. Creation and state-update code holds
 * the document mutably and reaches elements through the creation API and
 * structural navigation it owns -- never by re-finding through readers.
 */
struct MarkdownDocument : Document
{
    MarkdownDocument() = default;

    /**
     * @brief Memoized layout Document for a lexicon-registered markdown resource name.
     *
     * Built once per name and cached by the live SharedDocuments registry:
     * tokens are lexed via getToken(), then build() drives the block/inline
     * grammar to completion. A failed parse leaves the returned Document's
     * root sentinel childless -- callers test `document.getRoot()->firstChild` to
     * detect that case.
     *
     * @param name The lexicon-registered resource identifier.
     * @return The memoized MarkdownDocument for name, with a childless root on parse failure.
     */
    static const MarkdownDocument& getOrCreate (const juce::Identifier& name);

    /**
     * @brief Parses dynamic markdown source text into a fresh Document.
     *
     * @param documentText The markdown source text, owned by the caller.
     * @return The parsed Document, with a childless root sentinel on parse failure.
     */
    static MarkdownDocument parse (std::string_view documentText);

    /**
     * @brief Parses dynamic markdown source text into a fresh Document.
     *
     * @param documentText The markdown source text, owned by the caller.
     * @return The parsed Document, with a childless root sentinel on parse failure.
     */
    static MarkdownDocument parse (const juce::String& documentText);

    /**
     * @brief Parses dynamic markdown source text into a fresh Document, setting table provenance.
     *
     * Performs today's parse(), then sets Id::path (@p origin) on every
     * direct-child Element and Id::line (the source line number of the
     * row's Id::offset property, via getLineNumber()) on every data row of
     * every table. A row without Id::offset is left without Id::line.
     *
     * @param documentText The markdown source text, owned by the caller.
     * @param origin       The resource path or name set as Id::path on every direct-child Element.
     * @return The parsed Document, with a childless root sentinel on parse failure.
     */
    static MarkdownDocument parse (const juce::String& documentText, const juce::String& origin);

    /**
     * @brief Returns every direct child of root that is a table, in authored order.
     * @return The root's table children (Id::type == map::BlockType::table), authored order.
     */
    jam::Array<const Element*> getTables() const;

    /**
     * @brief Returns every root's direct child whose own id is @p tableId, in authored order.
     * @param tableId  Identifier of the table's own id (see getTable()).
     * @return The matching table children, in authored order.
     */
    jam::Array<const Element*> getTables (const juce::Identifier& tableId) const;

    /**
     * @brief Test whether @p element is a grid-table border row.
     * @param element  The Element to test.
     * @return True if @p element carries Id::type == map::BlockType::tableBorder.
     */
    static bool isTableBorder (const Element& element) noexcept;

    /**
     * @brief Test whether @p element is a table.
     * @param element  The Element to test.
     * @return True if @p element carries Id::type == map::BlockType::table.
     */
    static bool isTable (const Element& element) noexcept;

    static bool isParagraph (std::string_view text);

    /**
     * @brief Returns the data rows of a table, in authored order.
     *
     * Finds @p table's header row via getTableHeaderRow(), then collects
     * every other direct child that is not a border row.
     *
     * @param table  The table Element whose data rows are collected.
     * @return The data rows in authored order, header and border rows excluded,
     *         or an empty array if @p table has no header row.
     */
    jam::Array<const Element*> getTableRows (const Element& table) const;

    /**
     * @brief Returns the header text of every column in a table, in authored order.
     * @param table  The table Element whose header row's cell ids are collected.
     * @return The column headers in authored order, or an empty array if
     *         @p table has no header row.
     */
    jam::Array<juce::String> getTableHeaders (const Element& table) const;

    /**
     * @brief Finds the data row keyed @p rowId, among @p table's direct children.
     *
     * Rows are keyed nodes -- their own Element::id is the row key -- so lookup
     * is a single O(1) Owner probe.
     *
     * @param table  The table Element whose children are searched.
     * @param rowId  Key of the row to match.
     * @return A pointer to the matching row, or nullptr if not found.
     */
    const Element* getTableRow (const Element& table, const juce::Identifier& rowId) const;

    /**
     * @brief Finds the cell at the intersection of a column and a row in a table.
     * @param table  The table Element whose rows are searched.
     * @param colId  The cell's own id -- see addTableCell().
     * @param rowId  Key of the row to match.
     * @return A pointer to the matching cell, or nullptr if the row or column is not found.
     */
    const Element* getTableCell (const Element& table,
                                 const juce::Identifier& colId,
                                 const juce::Identifier& rowId) const;

    /**
     * @brief Returns the plain text of a table cell identified by column and row.
     * @param table  The table Element whose rows are searched.
     * @param colId  The cell's own id -- see addTableCell().
     * @param rowId  Key of the row to match.
     * @return getTableCell()'s subtext, or an empty string if the cell is not found.
     */
    juce::String getTableValue (const Element& table,
                                const juce::Identifier& colId,
                                const juce::Identifier& rowId) const;

    /**
     * @brief Finds the cell keyed @p colId, among @p row's direct children.
     *
     * Cells are keyed nodes -- their own Element::id is the column id -- so
     * lookup is a single O(1) Owner probe.
     *
     * @param row    The row Element whose children are searched.
     * @param colId  The cell's own id -- see addTableCell().
     * @return A pointer to the matching cell, or nullptr if not found.
     */
    const Element* getTableCell (const Element& row, const juce::Identifier& colId) const;

    /**
     * @brief Returns the plain text of a table cell in @p row identified by column.
     * @param row    The row Element whose cells are searched.
     * @param colId  The cell's own id -- see addTableCell().
     * @return getTableCell()'s subtext, or an empty string if the cell is not found.
     */
    juce::String getTableValue (const Element& row, const juce::Identifier& colId) const;

    /**
     * @brief Returns a view over a table cell's raw value span, without materializing a copy.
     * @param row    The row Element whose cells are searched.
     * @param colId  The cell's own id -- see addTableCell().
     * @return getValueView() of the matching cell, or an empty view if the cell is not found.
     */
    std::string_view getTableValueView (const Element& row, const juce::Identifier& colId) const;

    /**
     * @brief Returns a view over @p element's Id::tokens value span, without materializing a copy.
     *
     * A keyed node's own id is its pre-colon key; the text after the colon,
     * when present, is stored as @p element's first Id::tokens entry. A
     * key-only node (nothing after the colon) carries no Id::tokens
     * property.
     *
     * @param element  The keyed node whose value span is read.
     * @return A view over @p element's value span, decoded from getSource(),
     *         or an empty view if @p element has no Id::tokens property.
     */
    std::string_view getValueView (const Element& element) const;

    /**
     * @brief Finds the list nested in a blockquote nested in a table cell.
     *
     * A no-op-returning chain through getTableCell(), getBlockquote(), and
     * getList() -- the shape a table cell's value takes when authored as a
     * blockquoted list.
     *
     * @param row    The row Element whose cells are searched.
     * @param colId  The cell's own id -- see addTableCell().
     * @return A pointer to the nested list, or nullptr if the cell, its
     *         blockquote, or the blockquote's list is not found.
     */
    const Element* getTableList (const Element& row, const juce::Identifier& colId) const;

    /**
     * @brief Returns a view over a keyed list item's value span, reached through a table cell.
     *
     * Resolves the cell's nested list via getTableList(), finds the list
     * item keyed @p itemId, then returns a view over that item's value span
     * via getValueView().
     *
     * @param row     The row Element whose cells are searched.
     * @param colId   The cell's own id -- see addTableCell().
     * @param itemId  The list item's own id -- its pre-colon key.
     * @return A view over the matching list item's value span, or an empty
     *         view if the cell, its list, or the item is not found.
     */
    std::string_view getTableValueView (const Element& row,
                                        const juce::Identifier& colId,
                                        const juce::Identifier& itemId) const;

    /**
     * @brief Test whether a table cell holds non-empty text.
     * @param row    The row Element whose cells are searched.
     * @param colId  The cell's own id -- see addTableCell().
     * @return True if the matching cell's subtext (getTableValue()) is not empty.
     */
    bool hasTableValue (const Element& row, const juce::Identifier& colId) const noexcept;

    /**
     * @brief Returns the row-key text of every data row in a table, in authored order.
     *
     * Locates the table via getTable(), finds its header row via
     * getTableHeaderRow(), then collects every other direct child's own id
     * -- data rows are keyed nodes whose id is their row key.
     *
     * @param tableId  Identifier of the table's own id (see getTable()).
     * @return The row keys in authored order, or an empty array if the table
     *         or its header row is not found.
     */
    jam::Array<juce::String> getTableRowKeys (const juce::Identifier& tableId) const;

    /**
     * @brief Returns the data rows of a table, in authored order.
     *
     * Locates the table via getTable(), finds its header row via
     * getTableHeaderRow(), then collects every other direct child's own
     * Element* -- rows are identities, not just row-key text, so callers
     * needing to disambiguate rows sharing a key use this instead of
     * getTableRowKeys().
     *
     * @param tableId  Identifier of the table's own id (see getTable()).
     * @return The data rows in authored order, header row excluded, or an
     *         empty array if the table or its header row is not found.
     */
    jam::Array<const Element*> getTableRows (const juce::Identifier& tableId) const;

    /**
     * @brief Returns the header text of every column in a table, in authored order.
     *
     * Locates the table via getTable(), finds its header row via
     * getTableHeaderRow(), then collects the subtext of every `th` cell in
     * that row.
     *
     * @param tableId  Identifier of the table's own id (see getTable()).
     * @return The column headers in authored order, or an empty array if the
     *         table or its header row is not found.
     */
    jam::Array<juce::String> getTableHeaders (const juce::Identifier& tableId) const;

    /**
     * @brief Finds the cell at the intersection of a column and a row in a table.
     *
     * Locates the table via getTable() (O(1) probe), matches @p colId
     * against the header row's `th` cell texts to resolve a column index
     * (bounded walk over a small constant number of columns), resolves the
     * data row via a (table, rowId) probe (O(1) -- data rows are keyed
     * nodes), then returns the cell at the resolved column index in that
     * row.
     *
     * @param tableId  Identifier of the table's own id (see getTable()).
     * @param colId    Header text of the column to match.
     * @param rowId    Key of the row to match (see getTableRowKeys()).
     * @return A pointer to the matching cell, or nullptr if the table,
     *         column, or row is not found.
     */
    const Element* getTableCell (const juce::Identifier& tableId,
                                 const juce::Identifier& colId,
                                 const juce::Identifier& rowId) const;

    /**
     * @brief Finds the data row keyed @p rowId in the table keyed @p tableId.
     *
     * Locates the table via getTable() (O(1) probe), then resolves the row
     * via a (table, rowId) probe (O(1) -- data rows are keyed nodes),
     * mirroring getTableCell() minus the column resolution.
     *
     * @param tableId  Identifier of the table's own id (see getTable()).
     * @param rowId    Key of the row to match (see getTableRowKeys()).
     * @return A pointer to the matching row, or nullptr if the table or row is not found.
     */
    const Element* getTableRow (const juce::Identifier& tableId, const juce::Identifier& rowId) const;

    /**
     * @brief Returns the plain text of a table cell identified by column and row.
     *
     * @param tableId  Identifier of the table's own id (see getTable()).
     * @param colId    Header text of the column to match.
     * @param rowId    Key of the row to match.
     * @return getTableCell()'s subtext, or an empty string if the cell is not found.
     */
    juce::String getTableValue (const juce::Identifier& tableId,
                                const juce::Identifier& colId,
                                const juce::Identifier& rowId) const;

    /**
     * @brief Finds the fenced code block keyed @p codeId, among root's direct children.
     *
     * Code blocks are keyed nodes -- their own Element::id is the info-string
     * derived identifier -- so lookup is a single O(1) Owner probe.
     *
     * @param codeId  Identifier of the code block's own id.
     * @return A pointer to the matching code block, or nullptr if not found.
     */
    const Element* getCodeBlock (const juce::Identifier& codeId) const noexcept;

    /**
     * @brief Finds the blockquote (Id::blockquote) directly nested under @p scope.
     * @param scope  The Element whose direct children are searched.
     * @return A pointer to the matching blockquote, or nullptr if not found.
     */
    const Element* getBlockquote (const Element& scope) const noexcept;

    /**
     * @brief Finds the unordered list (Id::ul) directly nested under @p scope.
     * @param scope  The Element whose direct children are searched.
     * @return A pointer to the matching list, or nullptr if not found.
     */
    const Element* getList (const Element& scope) const noexcept;

    /**
     * @brief Finds the list item keyed @p itemId, among @p list's direct children.
     *
     * List items are keyed nodes -- their own Element::id is their pre-colon
     * key -- so lookup is a single O(1) Owner probe.
     *
     * @param list    The list Element whose children are searched.
     * @param itemId  The list item's own id -- its pre-colon key.
     * @return A pointer to the matching list item, or nullptr if not found.
     */
    const Element* getListItem (const Element& list, const juce::Identifier& itemId) const noexcept;

protected:
    /** @brief Markdown segmentation vocabulary handed to jam::Document. */
    const Vocabulary& getVocabulary() const override { return getMarkdownVocabulary(); }

    /** @brief getToken() override -- classifies one block-level token (newline/operator/text run) at cursor, appending it to tokens; returns its consumed length. */
    int getToken (Cursor& cursor, int segmentType) override;

    /**
     * @brief Document::build() override -- walks newline-delimited tokens through addLine(), then runs the inline pass.
     *
     * Splits the tokenized source into lines by newline Token boundaries and
     * feeds each to addLine(), threading containerDepth/isLeafOpen/leafLines,
     * the deferred leaf-text map, and the reference-definition maps across
     * lines. Closes any leaf still open at end of input, then runs
     * addLeafText() over the finished block tree to populate every leaf's
     * inline content from the deferred text.
     */
    void build() override;

    /**
     * @brief Find the table whose own id is @p tableId, among root's direct children.
     *
     * Tables are keyed nodes -- their own Element::id is the heading/table
     * identifier -- so lookup is a single O(1) Owner probe.
     *
     * @param tableId  Identifier of the table's own id.
     * @return A pointer to the matching table, or nullptr if not found.
     */
    const Element* getTable (const juce::Identifier& tableId) const noexcept;

    /**
     * @brief Find the first direct `Id::headerRow` child of @p table -- the header row.
     * @param table  The table node whose children are searched.
     * @return A pointer to the header row, or nullptr if none.
     */
    static const Element* getTableHeaderRow (const Element& table) noexcept;

private:
    /** @brief Transient parse-time token accumulator, populated by getToken() and consumed by build(). */
    jam::Array<Document::Token> tokens;

    static bool isMarkdownOperatorChar (juce::juce_wchar ch) noexcept;

    static constexpr int maxLabelLength { 999 };

    struct BlockParser
    {
        /**
         * @brief Binds this parser to a document and its attach base.
         * @param newDocument  The Document instances are appended to and read source from.
         * @param newRoot      The attach base this parser's container walk starts from --
         *                     the document root for the document-level parser, or a table
         *                     cell for a per-cell parser.
         */
        BlockParser (Document& newDocument, Element& newRoot);

        jam::HashMap<Element*, juce::String> leafText;
        jam::HashMap<juce::String, juce::String> referenceDestinations;
        jam::HashMap<juce::String, juce::String> referenceTitles;

    private:
        /**
         * @brief State of the grid-table row currently being accumulated across physical source lines.
         *
         * A grid-table row can span several physical lines (multi-line cell
         * content); this struct is the scan-state that collects those lines
         * until a border or alignment row closes the row and the accumulated
         * text is split into cells.
         */
        struct GridRow
        {
            /** @brief Raw source lines accumulated for the row currently being built, one token per physical line. */
            jam::Array<Document::Token> lines;

            /** @brief Byte offset of the row's first accumulated source line, set as Id::offset on the row Element. */
            uint32_t offset { 0 };

            /** @brief True while every line accumulated so far agrees with columnCount; false once a mismatch is recorded. */
            bool isValid { true };

            /** @brief Column count established by the row's first accumulated line -- the count every later line is expected to match. */
            int columnCount { 0 };

            /** @brief Actual cell count found on the line that disagreed with columnCount, recorded onto \c Id::columns when isValid is false. */
            int columnMismatch { 0 };
        };

        Document& document;
        Element& root;
        int containerDepth { 0 };
        bool isLeafOpen { false };
        bool isGridTableHeaderDone { false };

        /** @brief Scan-state for the grid-table row currently being accumulated; reset by closeLeaf(). */
        GridRow gridRow;
        jam::Array<Document::Token> leafLines;

        // =========================================================================
        // =========================================================================

        static constexpr int maxUnindentedColumns { 3 };
        static constexpr int indentedCodeColumns { 4 };
        static constexpr int minFenceLength { 3 };
        static constexpr int minThematicBreakLength { 3 };
        static constexpr int maxListMarkerGap { 4 };
        static constexpr int maxOrderedDigits { 9 };
        static constexpr int maxAtxLevel { 6 };
        static constexpr int minSoleImageLength { 4 };
        static constexpr int cellAlignmentNone { 0 };
        static constexpr int cellAlignmentRight { 1 };
        static constexpr int cellAlignmentLeft { 2 };
        static constexpr int cellAlignmentCenter { 3 };
        static constexpr int invalidCellAlignment { -1 };
        static constexpr int indentMismatch { -1 };

        /** @brief Shortest possible grid-table border content -- room for an opening `+` and a closing `+` with nothing between them. */
        static constexpr int minGridBorderLength { 2 };

        /** @brief Number of trailing border lines that close a grid table without themselves counting as a row separator. */
        static constexpr int closingBorderCount { 1 };

        // The open-block stack is the tree itself: the currently open containers
        // are exactly the "last child" chain from the constructor-bound root, one
        // conceptual container (blockquote or listItem) per depth -- `list`
        // wrapper nodes are transparent hops skipped by the walk below. containerDepth
        // is the only scan-state residue left over from that stack -- a plain int
        // threaded by reference, mirroring isLeafOpen/leafLines. leafText
        // threads deferred raw leaf text (keyed by Element*) alongside it -- the
        // Document arena has no property-removal primitive, so a leaf's raw text
        // is never written onto the Element itself until the inline pass has
        // resolved it into children (see closeLeaf()/addLeafText()).
        using AddFunction = bool (BlockParser::*) (size_t start, size_t end, size_t& remainderStart);

        /**
         * @brief Member-function pointer for a whole-line grid-table dispatch candidate.
         *
         * Unlike AddFunction, an AddLineFunction never reports a mid-line
         * remainder -- the line it is given is either entirely consumed
         * (returns true) or entirely left for the next candidate (returns
         * false).
         */
        using AddLineFunction = bool (BlockParser::*) (size_t start, size_t end);

        // =========================================================================
        // =========================================================================

        /**
         * @brief Walks the last-child chain from root to the conceptual container at @p depth.
         * @param depth     Zero-based conceptual container depth (blockquote/listItem count, `list` wrappers skipped).
         * @return Reference to the blockquote or listItem Element open at @p depth, relative to root.
         */
        Element& getBlockAt (int depth);

        /**
         * @brief Walks the last-child chain from root to the `list` Element wrapping the container at @p depth.
         * @param depth     Zero-based conceptual container depth (blockquote/listItem count, `list` wrappers skipped).
         * @return Reference to the nearest enclosing `list` Element, or root if none was crossed.
         */
        Element& getListAt (int depth);

        Element& getParent();

        Element& getLeaf() { return *getParent().lastChild; }

        static int getChildCount (const Element& element) noexcept;

        /**
         * @brief Appends a child Element tagged and typed from @p blockType's BlockTag entry.
         * @param parent     The Element to append the block to.
         * @param blockType  map::BlockType value; must have a BlockTag entry.
         * @return Reference to the newly appended block Element.
         */
        Element& appendBlock (Element& parent, int blockType);

        // =========================================================================
        // span/text scanning primitives -- char-level cursor helpers shared across
        // block grammars
        // =========================================================================

        static bool isBlankLine (const std::string& source, size_t start, size_t end) noexcept;

        static size_t trimStart (const std::string& source, size_t start, size_t end) noexcept;

        static size_t trimEnd (const std::string& source, size_t start, size_t end) noexcept;

        static int countLeadingSpaces (const std::string& source,
                                       size_t start,
                                       size_t end,
                                       int maxColumns) noexcept;

        static bool
        hasUnindentedStart (const std::string& source, size_t start, size_t end) noexcept;

        // One materialization point for an accumulated leaf's span list -- every
        // leaf close (code/html/paragraph/table-split) reads through here
        // exactly once, joining with a single newline separator per span. Spans
        // are non-contiguous in source (indentation/marker prefixes are stripped
        // per line), so this is a genuine multi-span join, not a single slice --
        // accumulated once into a reserved std::string, then materialized in one
        // fromUTF8 pass rather than growing a juce::String per line.
        static juce::String
        joinIntoString (const std::string& source, const jam::Array<Document::Token>& lineSpans);

        // =========================================================================
        // Document Element construction helpers
        // =========================================================================

        void closeCodeBlock (Element& block, jam::Array<Document::Token> lines);

    public:
        /**
         * @brief Finalizes the currently open leaf block, recording its accumulated lines for the inline pass.
         *
         * A no-op if no leaf is open. Otherwise reads the open leaf at
         * @p containerDepth: code blocks route through closeCodeBlock(), HTML blocks
         * and paragraphs record @p leafLines' joined text into @p leafText
         * (paragraphs first trying the sole-image promotion), and an open
         * table flushes the row accumulated in gridRow -- as a data row if
         * the header row is already closed, otherwise as the header row.
         * The raw text is never written onto the Element as a property here
         * -- the Document arena has no removal primitive, so it stays in
         * @p leafText until addLeafText() resolves it into the leaf's
         * inline children. Always clears the open-leaf scan state on
         * return, including gridRow and the grid-table header flag.
         */
        void closeLeaf();

    private:
        // --- Paragraph (spec §4.8) -----------------------------------------------

        bool addSoleImage (Element& block, const jam::Array<Document::Token>& lines);

        void addParagraphLine (size_t start, size_t end);

        // --- Setext headings (spec §4.3) -----------------------------------------

        static int getSetextLevel (const std::string& source, size_t start, size_t end) noexcept;

        static bool isSetextUnderline (const std::string& source, size_t start, size_t end) noexcept;

        // --- Thematic break (spec §4.1) ------------------------------------------
        //
        // A break's markers may be separated by spaces/tabs (e.g. "- - -"), so its
        // content can span several heterogeneous token runs -- the grammar is
        // examined here at the char level rather than bounded to one lead token.

        static bool
        isThematicBreakContent (const std::string& source, size_t start, size_t end) noexcept;

        static bool isThematicBreak (const std::string& source, size_t start, size_t end) noexcept;

        bool addThematicBreak (size_t start, size_t end, size_t& remainderStart);

        // --- ATX heading (spec §4.2) ---------------------------------------------

        static juce::String trimAtxClosingSequence (const juce::String& text);

        /**
         * @brief Level of an ATX heading marker (1-6 '#' characters) at @p start, if valid.
         * @return 1-maxAtxLevel if @p start is a run of 1-6 '#' characters terminated
         *         by end-of-line, space, or tab; else 0.
         */
        static int getAtxHeadingLevel (const std::string& source, size_t start, size_t end) noexcept;

        static bool isAtxHeading (const std::string& source, size_t start, size_t end) noexcept;

        static juce::String
        getAtxHeadingText (const std::string& source, size_t start, size_t end, int level);

        /**
         * @brief Appends a heading Element, sets its level and id (for table/anchor lookup),
         *        and records its raw text for the inline pass.
         * @param parent    The Element to append the heading to.
         * @param level     Heading level 1-6.
         * @param text      The heading's rendered text, also stored verbatim as Id::id.
         */
        void addHeading (Element& parent, int level, const juce::String& text);

        bool addAtxHeading (size_t start, size_t end, size_t& remainderStart);

        // --- Blockquote (spec §5.1) ----------------------------------------------

        static int
        getBlockquoteMarker (const std::string& source, size_t start, size_t end) noexcept;

        static bool isBlockquote (const std::string& source, size_t start, size_t end) noexcept;

        bool addBlockquote (size_t start, size_t end, size_t& remainderStart);

        // --- List items + lists (spec §5.2/§5.3) ---------------------------------

        static int
        getTaskListMarkerLength (const std::string& source, size_t start, size_t end) noexcept;

        static bool
        isTaskListMarkerChecked (const std::string& source, size_t start, size_t end) noexcept;

        /**
         * @brief Width of a bullet list marker ('-', '+', or '*') at @p start, if present.
         * @return 1 if @p start begins with a bullet marker character, else 0.
         */
        static int
        getBulletMarkerWidth (const std::string& source, size_t start, size_t end) noexcept;

        static int
        getOrderedMarkerWidth (const std::string& source, size_t start, size_t end) noexcept;

        static int getOrderedMarkerValue (const std::string& source,
                                          size_t start,
                                          size_t end,
                                          int width) noexcept;

        static int
        getListMarkerWidth (const std::string& source, size_t contentStart, size_t end) noexcept;

        /**
         * @brief Content indent width of a list item, measured from just past its marker.
         *
         * @p start points just after the marker. Returns 0 if there is no gap and
         * the line is not blank (a malformed marker). Otherwise returns
         * @p markerWidth plus the gap width, capped to @p markerWidth + 1 for a
         * blank first line or a gap wider than maxListMarkerGap (spec §5.2).
         *
         * @param source       The document's source text.
         * @param start        Cursor positioned just after the list marker.
         * @param end          End of the line.
         * @param markerWidth  Width in columns of the marker itself.
         * @return The full indent width (marker plus gap) content is offset by, or 0 if malformed.
         */
        static int getListItemIndentWidth (const std::string& source,
                                           size_t start,
                                           size_t end,
                                           int markerWidth) noexcept;

        static int
        getListItemGapLength (const std::string& source, size_t start, size_t end) noexcept;

        static bool isListItem (const std::string& source, size_t start, size_t end,
                                bool interruptingParagraph);

        static bool isSameListType (const Element& list,
                                    bool ordered,
                                    juce::juce_wchar bulletChar,
                                    juce::juce_wchar delim);

        void addListItem (bool ordered,
                          juce::juce_wchar bulletChar,
                          juce::juce_wchar delim,
                          int startNumber,
                          int indent,
                          const juce::String& text,
                          size_t valueStart,
                          size_t valueEnd);

        bool
        addListItem (size_t start, size_t end, size_t& remainderStart, bool allContainersMatched);

        // --- Fenced code blocks (spec §4.5) --------------------------------------

        static int measureFenceRun (const std::string& source,
                                    size_t start,
                                    size_t end,
                                    juce::juce_wchar fenceChar) noexcept;

        static int getFenceOpenWidth (const std::string& source, size_t start, size_t end);

        static bool isFencedCode (const std::string& source, size_t start, size_t end);

        bool addFencedCodeBlock (size_t start, size_t end);

        bool addFencedCode (size_t start, size_t end, size_t& remainderStart);

        static bool isClosingFence (const std::string& source,
                                    size_t start,
                                    size_t end,
                                    const juce::String& fenceToken) noexcept;

        bool addFencedCodeLine (size_t start, size_t end);

        // --- Indented code blocks (spec §4.4) ------------------------------------

        void addIndentedCodeBlock (Element& parent, size_t firstLineStart, size_t firstLineEnd);

        // --- HTML blocks (spec §4.6) ----------------------------------------------

        static int
        getEndOfTagName (const std::string& source, size_t start, size_t end, int cursor) noexcept;

        static juce::String
        tagText (const std::string& source, size_t start, size_t end, int from, int to);

        static bool isHtmlBlockType1Start (const std::string& source, size_t start, size_t end);

        static bool
        isHtmlBlockType2Start (const std::string& source, size_t start, size_t end) noexcept;

        static bool
        isHtmlBlockType3Start (const std::string& source, size_t start, size_t end) noexcept;

        static bool
        isHtmlBlockType4Start (const std::string& source, size_t start, size_t end) noexcept;

        static bool
        isHtmlBlockType5Start (const std::string& source, size_t start, size_t end) noexcept;

        static bool isHtmlBlockType6Start (const std::string& source, size_t start, size_t end);

        static bool isHtmlBlockType7Start (const std::string& source, size_t start, size_t end);

        static const jam::Function::Map<int, bool>& getHtmlBlockConditions();

        static int getHtmlBlockCondition (const std::string& source, size_t start, size_t end);

        static bool isHtmlBlock (const std::string& source, size_t start, size_t end,
                                 bool interruptingParagraph);

        static bool spanContains (const std::string& source,
                                  size_t start,
                                  size_t end,
                                  const char* literal) noexcept;

        static bool
        isHtmlBlockEnd (const std::string& source, size_t start, size_t end, int conditionType);

        void addHtmlBlock (Element& parent, int conditionType);

        bool addHtmlBlockLine (size_t start, size_t end);

        bool addHtmlBlock (size_t start, size_t end, size_t& remainderStart);

        // --- Link reference definitions (spec §4.7) -------------------------------

        // No escape processing here -- the scanned range is a contiguous slice of
        // source, so it is bounded once (by closeBracket or maxLabelLength) and
        // materialized in a single span construction, not accumulated character by
        // character.
        static Document::Token
        getBracketedText (const std::string& source, size_t start, size_t end, int cursor);

        static Document::Token
        getReferenceLabel (const std::string& source, size_t start, size_t end);

        static Document::Token
        getReferenceDestinationAndTitle (const std::string& source,
                                         size_t start,
                                         size_t end,
                                         int cursor);

        // First definition for a given normalized label wins (spec §4.7) -- a
        // duplicate later in the document is read but never overwrites the maps.
        bool addReferenceLine (const std::string& source, size_t start, size_t end);

        // --- Sole-content images (spec §6.4) --------------------------------------

        static Document::Token
        getAbsoluteImageTail (const std::string& source, size_t start, size_t end, int cursor);

        bool addReferenceDefinition (size_t start, size_t end, size_t& remainderStart);

        // --- Tables (GFM §4.10 extension) -----------------------------------------

        /**
         * @brief The number of consecutive backslash characters ending text.
         *
         * @param text The text to scan from its end.
         * @return The count of trailing backslash characters.
         */
        static int getTrailingBackslashCount (const juce::String& text) noexcept;

        /**
         * @brief The number of consecutive backslash characters ending at @p position, bounded by @p start.
         * @param source    The document source to scan.
         * @param start     Byte offset the backward scan never crosses.
         * @param position  Byte offset to scan backward from (exclusive).
         * @return The count of trailing backslash characters immediately before @p position.
         */
        static int getTrailingBackslashCount (const std::string& source,
                                              size_t start,
                                              size_t position) noexcept;

        /**
         * @brief Halves text's trailingBackslashCount trailing backslashes, each escaped pair collapsing to one literal backslash.
         *
         * @param text                   The text whose trailing backslashes are collapsed.
         * @param trailingBackslashCount The number of trailing backslashes text ends with, from getTrailingBackslashCount().
         * @return text with its trailing backslash run replaced by half as many backslashes.
         */
        static juce::String getWithCollapsedBackslashes (const juce::String& text,
                                                          int trailingBackslashCount);

        /**
         * @brief Splits a table row's source line into its cell texts, honoring backslash-escaped pipes.
         *
         * A pipe's escape status follows the parity of the run of backslashes
         * immediately preceding it: an even count (including zero) means the
         * backslashes are themselves already-escaped pairs, so the pipe is a
         * genuine column separator, and the backslash run collapses to half
         * its length through getWithCollapsedBackslashes(). An odd count
         * means the final backslash escapes the pipe, so the pipe is literal
         * cell content, not a separator, and the run likewise collapses to
         * half its length before the pipe is appended back onto the
         * in-progress cell. A leading or trailing pipe delimiting the whole
         * row is stripped first, itself subject to the same parity check
         * against an escaping backslash.
         *
         * @param row The table row's source line.
         * @return The row's cell texts, in column order, with escaped pipes and backslash pairs resolved.
         */
        static jam::Strings splitTableRow (const juce::String& row) noexcept;

        static int getAlignmentOrdinal (bool leftColon, bool rightColon);

        static juce::String getAlignmentTokenText (int ordinal);

        static int getCellAlignment (const juce::String& cell) noexcept;

        static jam::Array<int>
        getRowAlignments (const std::string& source, size_t start, size_t end) noexcept;

        /**
         * @brief Drops leading and trailing zero-length spans from @p spans.
         * @param spans  The spans to trim.
         * @return @p spans with any leading and trailing zero-length entries removed.
         */
        static jam::Array<Document::Span> getTrimmedSpans (const jam::Array<Document::Span>& spans);

        /**
         * @brief Materializes a cell's per-line spans as newline-joined text.
         *
         * Trims leading and trailing zero-length spans via getTrimmedSpans(),
         * then decodes each remaining span from source and joins the results
         * with a single newline separator per span.
         *
         * @param spans  The cell's per-line source spans.
         * @return The cell's text, newline-joined across its lines.
         */
        juce::String getCellText (const jam::Array<Document::Span>& spans) const;

        /**
         * @brief Appends one table cell, keying it to its column and optional alignment,
         *        and records its raw text for the inline pass.
         * @param row       The table row to append the cell to.
         * @param text      The cell's raw text (trimmed on store).
         * @param alignment The column's alignment token ("left"/"right"/"center"), or empty for none.
         * @param colId     The cell's own id -- derived from the header text for a header cell,
         *                  or matched from the header row's cell id for a data cell.
         * @param spans     The cell's source spans, threaded to addCellLines() to build its
         *                  inline content.
         */
        void addTableCell (Element& row,
                           const juce::String& text,
                           const juce::String& alignment,
                           const juce::Identifier& colId,
                           const jam::Array<Document::Span>& spans);

        /**
         * @brief The trimmed content span of a table row's source line, with its delimiting pipes stripped.
         *
         * Trims leading/trailing whitespace, then strips a leading pipe
         * unconditionally and a trailing pipe when it is not itself escaped
         * by an odd-length run of backslashes (per getTrailingBackslashCount()).
         *
         * @param source     The document's source text.
         * @param lineToken  The row's source-line token.
         * @return The row's content span, pipes and surrounding whitespace stripped.
         */
        static Document::Span getRowSpan (const std::string& source,
                                          const Document::Token& lineToken) noexcept;

        /**
         * @brief Splits [@p start, @p end) at unescaped pipes, appending each cell's trimmed span.
         *
         * A pipe is a genuine column separator when the run of backslashes
         * immediately preceding it (per getTrailingBackslashCount()) has
         * even length; otherwise it is literal cell content. One entry is
         * appended to @p cells and @p cellHasEscape per resulting cell,
         * including the final cell running to @p end.
         *
         * @param cells          Appended with each cell's trimmed span, in column order.
         * @param cellHasEscape  Appended with true for each cell containing an escaped
         *                       backslash or pipe, parallel to @p cells.
         * @param start          Byte offset of the row content's start.
         * @param end            Byte offset of the row content's end.
         * @return True if any cell in the row contains an escaped backslash or pipe.
         */
        bool addCells (jam::Array<Document::Span>& cells,
                       jam::Array<bool>& cellHasEscape,
                       size_t start,
                       size_t end) const;

        /**
         * @brief Appends one row's cell spans onto @p cellSpans, one entry per column.
         *
         * A column beyond @p cells' size gets an empty span at @p end (a row
         * short of columns). A column whose cell contains an escaped
         * backslash or pipe is appended onto source as a resolved appendix
         * via Document::addSource() -- its collapsed, unescaped text from
         * @p collapsedCells -- so the stored span addresses the resolved
         * text directly rather than the raw escaped source.
         *
         * @param cellSpans       Per-column span arrays; column @p index appends its span to
         *                        cellSpans.at(index).
         * @param cells           This row's raw cell spans, from addCells().
         * @param cellHasEscape   Per-cell escape flag, parallel to @p cells.
         * @param collapsedCells  This row's cell texts with escapes resolved, from splitTableRow(),
         *                        used only for cells flagged in @p cellHasEscape.
         * @param end             Byte offset of the row content's end, used for a missing column's empty span.
         */
        void addColumnSpans (jam::Array<jam::Array<Document::Span>>& cellSpans,
                             const jam::Array<Document::Span>& cells,
                             const jam::Array<bool>& cellHasEscape,
                             const jam::Strings& collapsedCells,
                             size_t end) const;

        /**
         * @brief Splits every accumulated row line into per-column cell spans.
         *
         * For each line, resolves its content span via getRowSpan(), splits
         * it at unescaped pipes via addCells(), resolves escaped cells
         * against splitTableRow() when needed, then appends the result onto
         * the per-column arrays via addColumnSpans().
         *
         * @param lines        The row's accumulated source-line tokens.
         * @param columnCount  The table's column count -- sizes the returned per-column arrays.
         * @return One span array per column, each entry one span per accumulated line.
         */
        jam::Array<jam::Array<Document::Span>>
        splitCellSpans (const jam::Array<Document::Token>& lines, int columnCount) const;

        /**
         * @brief Builds a cell's inline content from its per-line source spans.
         *
         * Trims @p spans via getTrimmedSpans(); if any remain, runs a nested
         * BlockParser attached at @p cell over each span as its own line,
         * closes its leaf, then runs a nested InlineParser to resolve the
         * deferred leaf text into @p cell's inline children.
         *
         * @param cell   The table cell Element to populate.
         * @param spans  The cell's per-line source spans.
         */
        void addCellLines (Element& cell, const jam::Array<Document::Span>& spans);

        /**
         * @brief Appends a table row from per-column cell spans, keying one cell per column.
         *
         * The header row is tagged `tr`. A data row is a keyed node whose own id
         * is its first cell's trimmed text -- Document::getTable*() depends on
         * this keying.
         *
         * @param table       The table to append the row to.
         * @param alignments  Per-column alignment tokens, sized to the table's column count.
         * @param isHeader    True for the header row, false for a data row.
         * @param rowOffset   Byte offset of the row's first source line, set as Id::offset on the row.
         * @param cellSpans   Per-column source spans, one entry per column, from splitCellSpans().
         * @return Reference to the newly appended row Element.
         */
        Element& addTableRow (Element& table,
                              const jam::Strings& alignments,
                              bool isHeader,
                              uint32_t rowOffset,
                              const jam::Array<jam::Array<Document::Span>>& cellSpans);

        /**
         * @brief Resolves a table's own id: the nearest preceding sibling heading's text,
         *        or Id::table when no heading precedes it (anonymous table).
         * @param parent  The container the table is (or will be) appended under.
         */
        static juce::Identifier getTableId (const Element& parent);

        /**
         * @brief Replaces @p parent's last child with a freshly keyed Element, preserving position.
         *
         * The Document arena has no child-removal primitive and mutating an
         * Element's id post-insertion would desynchronize Owner's content-hash
         * lookup (used by Document::getTable()) -- so the replacement Element is
         * a genuine new insertion (correct hash bucket), and the superseded
         * Element is unlinked from the sibling chain in its place. It stays
         * allocated but unreachable by any tree walk, mirroring the old
         * by-value model's "replace via move-and-drop" outcome.
         *
         * @param parent    The Element whose last child is being replaced.
         * @param id        The replacement Element's own id.
         * @return The inserted Element, now occupying @p parent's last-child slot.
         */
        Element* replaceLastChild (Element& parent, const juce::Identifier& id);

        void promoteTable (Document::Span span, const jam::Strings& alignments);

        static bool isTable (const std::string& source, size_t start, size_t end) noexcept;

        bool addTable (size_t start, size_t end);

        Element& addTableRowAtLeaf (Element& table,
                                    uint32_t rowOffset,
                                    const jam::Array<jam::Array<Document::Span>>& cellSpans);

        // --- Grid tables (Pandoc grid table extension) ----------------------------

        /**
         * @brief Checks that the content between a border's opening and closing `+` is a valid `+`-delimited segment sequence.
         *
         * The content between @p contentStart and @p contentEnd (exclusive of
         * the two `+` characters themselves) must consist of one or more
         * dash/equals segments, each separated by a `+`, with no empty
         * segment and no other character.
         *
         * @param source       The document's source text.
         * @param contentStart Byte offset of the border's opening `+`.
         * @param contentEnd   Byte offset one past the border's closing `+`.
         * @return True if the content is a valid grid-table border segment sequence.
         */
        static bool
        isGridBorderContent (const std::string& source, size_t contentStart, size_t contentEnd) noexcept;

        static bool isGridTableBorder (const std::string& source, size_t start, size_t end) noexcept;

        static bool
        isGridTableHeaderSeparator (const std::string& source, size_t start, size_t end) noexcept;

        /**
         * @brief Finds the end of the physical line starting at @p lineStart.
         * @param source    The document's source text.
         * @param lineStart Byte offset of the line's first character.
         * @return Byte offset of the line's terminating newline, or \c source.size() if the line runs to the end of the source.
         */
        static size_t getLineEnd (const std::string& source, size_t lineStart) noexcept;

        /**
         * @brief Checks whether the row's first cell -- between the leading `\|` at @p contentStart and the next `\|` -- is blank.
         * @param source       The document's source text.
         * @param contentStart Byte offset of the line's leading `\|`.
         * @param lineEnd      Byte offset of the end of the line.
         * @return True if the first cell contains only whitespace.
         */
        static bool
        isBlankLeadingCell (const std::string& source, size_t contentStart, size_t lineEnd) noexcept;

        /**
         * @brief Finds where a line's leading `\|` sits, past its unindented leading whitespace.
         * @param source    The document's source text.
         * @param lineStart Byte offset of the line's first character.
         * @param lineEnd   Byte offset of the end of the line.
         * @return Byte offset of the leading `\|`, or @p lineEnd if the line's first non-indent character is not a `\|`.
         */
        static size_t
        getPipeContentStart (const std::string& source, size_t lineStart, size_t lineEnd) noexcept;

        /**
         * @brief Decides whether the table opening at @p end is in Pandoc's grid-table form.
         *
         * Scans the lines following the opening border. The table is grid
         * form if either a row-separator border is found beyond the closing
         * border, or a row past the header separator leads with a blank
         * first cell.
         *
         * @param source The document's source text.
         * @param end    Byte offset of the end of the table's opening border line.
         * @return True if the table is in grid-table form.
         */
        static bool isGridTableFormat (const std::string& source, size_t end) noexcept;

        bool addGridTable (size_t start, size_t end, size_t& remainderStart);

        /**
         * @brief Flushes the accumulated grid-table row as the table's header row, then clears gridRow's accumulated lines.
         *
         * A no-op if no lines are accumulated. Every column's alignment is
         * left empty -- grid-table syntax carries no per-column alignment
         * token.
         *
         * @param table The table to append the header row to.
         */
        void addAccumulatedHeaderRow (Element& table);

        /**
         * @brief Flushes the accumulated grid-table row as a data row, then clears gridRow's accumulated lines.
         *
         * A no-op if no lines are accumulated. When gridRow.isValid is
         * false, sets the row's actual cell count (gridRow.columnMismatch)
         * as \c Id::columns.
         *
         * @param table The table to append the data row to.
         */
        void addAccumulatedRow (Element& table);

        /**
         * @brief Consumes a blank line inside an open grid table by closing it.
         * @param start Start of the line content.
         * @param end   End of the line.
         * @return True if the line was blank -- the grid table is closed and the line is consumed.
         */
        bool addGridTableBlankLine (size_t start, size_t end);

        /**
         * @brief Consumes a grid-table border line, flushing whichever row it closes.
         *
         * Once the header row is already closed, a border line flushes the
         * accumulated row as a data row and appends a border marker. Before
         * the header row is closed, a border line carrying `=` (the header
         * separator) flushes the accumulated row as the header row. Any
         * other border line encountered before the header separator is
         * consumed without flushing anything.
         *
         * @param start Start of the line content.
         * @param end   End of the line.
         * @return True if the line was a grid-table border -- the line is consumed either way.
         */
        bool addGridTableBorderLine (size_t start, size_t end);

        /**
         * @brief Consumes a pipe-table-style alignment row found inside a grid table, flushing whichever row it closes.
         *
         * A line that parses as alignment tokens (e.g. `:---:`, `---:`) acts
         * as a row boundary: it flushes the accumulated row as a data row
         * once the header is done, or as the header row otherwise. The
         * alignment tokens themselves are discarded -- grid-table cells
         * carry no per-column alignment.
         *
         * @param start Start of the line content.
         * @param end   End of the line.
         * @return True if the line parsed as alignment tokens -- the line is consumed.
         */
        bool addGridTableAlignmentRow (size_t start, size_t end);

        /**
         * @brief Marks the row currently being accumulated as invalid, recording the offending cell count.
         * @param columnCount The cell count found on the line that disagreed with gridRow.columnCount.
         */
        void setGridRowColumnMismatch (int columnCount) noexcept;

        /**
         * @brief Appends one physical source line to the grid-table row currently being accumulated.
         *
         * On the row's first line, establishes gridRow.columnCount and
         * gridRow.offset; for a data row, that first line's cell count is
         * also checked against the header row's own column count. On every
         * later line of the same row, the cell count is checked against
         * gridRow.columnCount. Either check that disagrees records the
         * mismatch via setGridRowColumnMismatch().
         *
         * @param start Start of the line content.
         * @param end   End of the line.
         * @param cells The line's content already split into per-column cell text.
         */
        void addGridRowLine (size_t start, size_t end, const jam::Strings& cells);

        /**
         * @brief Consumes a `\|`-led content line by appending it to the row currently being accumulated.
         *
         * When the open table is not in grid-table form, this line is
         * itself a new row: the row accumulated so far is flushed first
         * (as a data row once the header is done, otherwise as the header
         * row) before the new line starts the next one.
         *
         * @param start Start of the line content.
         * @param end   End of the line.
         * @return True if the trimmed line starts with `\|` -- the line is consumed.
         */
        bool addGridTableContentLine (size_t start, size_t end);

        /**
         * @brief Tries each grid-table line candidate at @p start in order, dispatching the first match.
         *
         * Returns false immediately if no table leaf is open. Otherwise
         * tries, in order: blank line, border line, pipe-table-style
         * alignment row, content line. The first candidate that returns
         * true has fully consumed the line. If none match, the open table
         * is closed and the line is left unconsumed for the caller to try
         * elsewhere.
         *
         * @param start Start of the line content.
         * @param end   End of the line.
         * @return True if the line was consumed by a grid-table candidate.
         */
        bool addGridTableLine (size_t start, size_t end);

        /**
         * @brief Continues an open indented code block, or else opens/continues indented code or a paragraph.
         *
         * If an indented code block is already open and @p start still carries
         * the required indent, the line is appended to it. Otherwise the open
         * indented code block (if any) is closed, and the line opens a new
         * indented code block (when indented and no paragraph is open) or
         * continues/opens a paragraph via addParagraphLine().
         */
        void addFallbackLeaf (size_t start, size_t end);

        bool addLeafForTableInterrupt (size_t start, size_t end, size_t& remainderStart);

        bool
        addContainer (size_t start, size_t end, size_t& remainderStart, bool allContainersMatched);

        bool interruptTable (size_t start, size_t end, size_t& remainderStart);

        bool addTableRowLine (size_t start, size_t end);

        // --- Container continuation (spec §5.1 rule 1/2, §5.2 rule 1) -----------

        static int
        getListIndent (const std::string& source, size_t start, size_t end, int indent) noexcept;

        /**
         * @brief Counts how many currently open containers @p start continues, from the outermost in.
         *
         * Walks the open-container chain (getBlockAt()) outward-to-inward,
         * matching each blockquote's `>` marker or each list item's indent
         * against @p start, stopping at the first container that does not
         * continue.
         *
         * @return The number of open containers (0..containerDepth) that continuation matched.
         */
        int getContinuationDepth (size_t start, size_t end);

        size_t advanceThroughContinuation (size_t start, size_t end, int matchedDepth);

        void setListsLoose();

        /**
         * @brief Closes containers past @p matchedDepth, unless the line is a lazy paragraph continuation.
         *
         * If @p matchedDepth equals the current open depth, every container
         * continued and nothing changes. Otherwise the containers below
         * @p matchedDepth failed to continue; they are closed by lowering
         * containerDepth to @p matchedDepth and closing the open leaf, unless exactly
         * the innermost container failed, a paragraph is open, the line is not
         * blank, and the line does not begin a new block (spec §5's lazy
         * continuation rule for paragraphs inside containers).
         */
        void applyContinuationResult (int matchedDepth, size_t start, size_t end);

        // --- Per-line leafBlocks/containerBlocks walk (spec chapter 5's "open new blocks" phase) --

        /**
         * @brief Tries each leaf-block grammar at @p start in spec priority order, opening the first match.
         *
         * Tries, in order: thematic break, ATX heading, fenced code, HTML block,
         * link reference definition. Stops at the first grammar that matches.
         *
         * @param start                  Start of the line content (past container syntax).
         * @param end                    End of the line.
         * @param remainderStart         Set to the position past the consumed leaf-opening syntax on match.
         * @return True if a leaf block was opened (or a reference definition consumed).
         */
        bool addLeaf (size_t start, size_t end, size_t& remainderStart);

        /**
         * @brief Repeatedly opens containers and leaf blocks at @p start until a leaf is opened or nothing matches.
         *
         * Alternates addLeaf() and addContainer(): each opened container
         * advances @p remainderStart and the scan continues from there, so
         * nested containers (e.g. a blockquote wrapping a list item) are opened
         * in one call before the leaf grammar is tried again at the innermost
         * position.
         *
         * @return True once a leaf block is opened; false if neither a leaf nor a container matched.
         */
        bool addBlocks (size_t start, size_t end, size_t& remainderStart, bool allContainersMatched);

        // --- Blank line (spec §4.8/§5.3) -----------------------------------------

        void closeOnBlankLine (size_t lineEnd);

        // --- Per-line entry point -------------------------------------------------

        bool promoteSetextHeading (size_t start, size_t end);

    public:
        /**
         * @brief Processes one source line: continuation, then leaf-specific and block-open dispatch.
         *
         * First resolves how many open containers @p lineStart continues
         * (getContinuationDepth()) and applies the result (closing containers
         * or accepting a lazy paragraph continuation). Then tries, in order:
         * fenced code continuation, HTML block continuation, table row
         * continuation, table promotion from an open paragraph, setext heading
         * promotion, a blank-line close, and finally the general
         * container/leaf-opening walk (addBlocks()), falling back to
         * addFallbackLeaf() (indented code or paragraph) if nothing matched.
         */
        void addLine (size_t lineStart, size_t lineEnd);
    };

    static size_t advance (size_t start, size_t end, int count) noexcept;

    static bool isAsciiLetter (juce::juce_wchar ch) noexcept;

    static bool isTagNameChar (juce::juce_wchar ch) noexcept;

    static juce::juce_wchar charAt (const std::string& source, size_t start, size_t end, int index) noexcept;

    static int getEndOfOpenTag (const std::string& source, size_t start, size_t end, int index);

    static int getEndOfClosingTag (const std::string& source, size_t start, size_t end, int index);

    // --- Raw HTML tag grammar (spec §6.6, shared with type-7 detection) -----

    static bool isAsciiPunctuation (juce::juce_wchar ch) noexcept;

    static bool isHarfBuzzSpaceSeparator (juce::juce_wchar ch) noexcept;

    static bool isUnicodeWhitespace (juce::juce_wchar ch) noexcept;

    static constexpr juce::juce_wchar maxAsciiCodepoint { 0x7f };

    static bool isHarfBuzzPunctuationOrSymbol (juce::juce_wchar ch) noexcept;

    static bool isUnicodePunctuation (juce::juce_wchar ch) noexcept;

    static int getEndOfRawTagName (const std::string& source, size_t start, size_t end, int index);

    static bool isUnquotedAttributeChar (juce::juce_wchar ch) noexcept;

    static int getEndOfRawAttributeValue (const std::string& source, size_t start, size_t end, int index);

    static int getEndOfRawAttribute (const std::string& source, size_t start, size_t end, int index);

    // --- Link/image destination + title grammar (spec §6.3) -----------------
    //
    // Each function below returns a Document::Token whose span encodes the
    // consumed extent (offset/length pack) and whose properties carry the
    // decoded value (Id::href and/or Id::title) at the point of creation --
    // a genuine span-plus-value pack, never a naked pair/tuple. A default
    // (zero-property) Token is the failure sentinel; a resolved title may
    // legitimately be an empty string, so callers test contains(Id::title)/
    // contains(Id::href), never string emptiness, to decide success.

    static Document::Token
    getBracketedDestination (const std::string& source, size_t start, size_t end, int index);

    static Document::Token getBareDestination (const std::string& source, size_t start, size_t end, int index);

    static Document::Token
    getLinkDestination (const std::string& source, size_t start, size_t end, int index);

    static Document::Token getQuotedTitle (const std::string& source,
                                           size_t start,
                                           size_t end,
                                           int index,
                                           juce::juce_wchar closeQuote);

    static Document::Token getLinkTitle (const std::string& source, size_t start, size_t end, int index);

    static Document::Token getEndOfLinkTail (const std::string& source, size_t start, size_t end, int cursor);

    static Document::Token getInlineTail (const std::string& source, size_t start, size_t end, int index);

    // Spec §4.7 label normalization: case-fold + whitespace collapse, built on
    // Document::isWhitespace rather than a token split -- no juce::StringArray.
    static juce::String normalizeLabel (const juce::String& label);

    // =========================================================================
    // inline pass -- each closed leaf's deferred text (spec chapter 6) is lexed
    // directly (addToken() alone, over Document::preprocess()'d bytes, into a
    // flat jam::Array<Document::Token> -- no Document instance is built for it);
    // delimiter/bracket matches are then resolved over token indices, and a
    // single forward walk opens/closes em/strong/del/code/a Elements directly
    // at those token indices, attaching each in-place at open time (the arena's
    // Owner<Element> storage gives every Element a stable address, so there is
    // no detached-subtree staging step), replacing the leaf's deferred text
    // with a children tree of interleaved text nodes and inline elements.
    // =========================================================================

    struct InlineParser
    {
        InlineParser (Document& newDocument, const BlockParser& newBlocks);

        /**
         * @brief Recursively resolves every leaf's deferred raw text (see @p leafText) into an inline-parsed children tree.
         *
         * Descends children-first, then dispatches by block type: paragraph/
         * heading/tableCell deferred text runs through addInlines() (full
         * inline grammar); codeBlock/mermaid/htmlBlock deferred text is wrapped
         * verbatim as a single text-node child; an image's deferred alt text is
         * likewise run through addInlines() to populate its description as
         * inline children. @p leafText is read-only here -- nothing is ever
         * written back onto the Element as a raw-text property, so no stale
         * text property can shadow the newly built children (getAllSubText()
         * checks Id::text first).
         */
        void addLeafText (Element& element);

    private:
        Document& document;
        const BlockParser& blocks;

        static constexpr int minSchemeLength { 2 };
        static constexpr int maxSchemeLength { 32 };
        static constexpr int maxEntityDigits { 8 };
        static constexpr int maxEntityNameLength { 32 };
        static constexpr int maxTildeRunLength { 2 };
        static constexpr int minimumHardBreakSpaces { 2 };

        static juce::juce_wchar charBefore (const std::string& source,
                                            const jam::Array<Document::Token>& tokens,
                                            int index) noexcept;

        static juce::juce_wchar charAfter (const std::string& source,
                                           const jam::Array<Document::Token>& tokens,
                                           int index) noexcept;

        static bool isLeftFlanking (juce::juce_wchar before, juce::juce_wchar after) noexcept;

        static bool isRightFlanking (juce::juce_wchar before, juce::juce_wchar after) noexcept;

        static bool canOpenDelimiter (juce::juce_wchar marker,
                                      juce::juce_wchar before,
                                      bool leftFlanking,
                                      bool rightFlanking) noexcept;

        static bool canCloseDelimiter (juce::juce_wchar marker,
                                       juce::juce_wchar before,
                                       juce::juce_wchar after,
                                       bool leftFlanking,
                                       bool rightFlanking) noexcept;

        static bool isTildeOverflow (const std::string& source,
                                    const Document::Token& token) noexcept;

        static bool tokenCanOpen (const std::string& source,
                                  const jam::Array<Document::Token>& tokens,
                                  int index) noexcept;

        static bool tokenCanClose (const std::string& source,
                                   const jam::Array<Document::Token>& tokens,
                                   int index) noexcept;

        static void getDelimiter (const std::string& source,
                                  jam::Array<Document::Token>& tokens,
                                  int index) noexcept;

        // Spec rule 17 (rule of three).
        static bool ruleOfThreeAllows (int openerLen,
                                       int closerLen,
                                       bool openerCanOpen,
                                       bool openerCanClose,
                                       bool closerCanOpen,
                                       bool closerCanClose) noexcept;

        static int findMatchingOpener (const std::string& source,
                                       const jam::Array<Document::Token>& tokens,
                                       int closerIndex) noexcept;

        static void deactivateInteriorDelimiters (jam::Array<Document::Token>& tokens,
                                                  int openerIndex,
                                                  int closerIndex) noexcept;

        static Document::Token applyEmphasisMatch (const std::string& source,
                                                    jam::Array<Document::Token>& tokens,
                                                    int openerIndex,
                                                    int closerIndex);

        static void closeEmphasis (const std::string& source,
                                   jam::Array<Document::Token>& tokens,
                                   int closerIndex,
                                   jam::Array<Document::Token>& runs);

        static jam::Array<Document::Token> processEmphasis (const std::string& source,
                                                             jam::Array<Document::Token>& tokens);

        // --- Lexer state functions (Html/Css idiom -- always return a Document::Token) -----

        static Document::Token getText (const std::string& source,
                                        size_t cursor,
                                        jam::Array<Document::Token>& tokens) noexcept;

        static Document::Token getEscape (const std::string& source, size_t cursor) noexcept;

        static Document::Token
        getNumericCharacterReference (const std::string& source, size_t cursor) noexcept;

        static Document::Token
        getNamedCharacterReference (const std::string& source, size_t cursor) noexcept;

        static Document::Token
        getCharacterReference (const std::string& source, size_t cursor) noexcept;

        static int measureMarkerRun (const std::string& source,
                                     size_t cursor,
                                     juce::juce_wchar marker) noexcept;

        static Document::Token getCodeSpan (const std::string& source, size_t cursor) noexcept;

        static bool isSchemeChar (juce::juce_wchar ch) noexcept;

        static bool isAutolinkUriChar (juce::juce_wchar ch) noexcept;

        static bool isEmailLocalChar (juce::juce_wchar ch) noexcept;

        static bool isEmailDomainChar (juce::juce_wchar ch) noexcept;

        // --- GFM extended (bare, no-brackets) autolinks (spec/GFM appendix §6.9) --

        static bool isDomainLabelChar (juce::juce_wchar ch) noexcept;

        static int getExtendedDomainLength (const std::string& source, size_t cursor) noexcept;

        static Document::Token getExtendedAutolinkSpan (const std::string& source,
                                                                       size_t fullStart,
                                                                       size_t domainStart,
                                                                       int domainLength) noexcept;

        static Document::Token
        getWwwAutolink (const std::string& source, size_t cursor) noexcept;

        static Document::Token
        getUrlAutolink (const std::string& source, size_t cursor) noexcept;

        static Document::Token
        getExtendedAutolink (const std::string& source, size_t cursor) noexcept;

        static Document::Token
        getBareEmailAutolink (const std::string& source,
                              size_t cursor,
                              jam::Array<Document::Token>& tokens) noexcept;

        static int getSchemeLength (juce::String::CharPointerType cursor) noexcept;

        static Document::Token getHtmlTagSpan (const std::string& source,
                                               size_t cursor) noexcept;

        static Document::Token
        getAutolinkUri (const std::string& source, size_t afterOpen) noexcept;

        static Document::Token
        getAutolinkEmail (const std::string& source, size_t afterOpen) noexcept;

        static Document::Token getRawHtml (const std::string& source, size_t cursor) noexcept;

        static Document::Token
        getEmphasisDelimiter (const std::string& source, size_t cursor) noexcept;

        static Document::Token getLinkOpen (const std::string& source, size_t cursor) noexcept;

        static Document::Token getImageOpen (const std::string& source, size_t cursor) noexcept;

        // Array-indexed sibling of the size_t-based getReferenceLabel above --
        // shortcut/collapsed reference-form label read during bracket closing,
        // where the scan position is already an int index into the leaf's source.
        static Document::Token
        getInlineReferenceLabel (const std::string& source, size_t start, size_t end, int index) noexcept;

        // Resolves a shortcut/collapsed reference label against blocks' reference
        // maps. Returns a Token carrying Id::href/Id::title on success (its span
        // length is the reference tail's consumed extent -- 0 is a legitimate
        // consumed length for the pure-shortcut form); a Token with neither
        // property means the label did not resolve against a known definition.
        Document::Token getReferenceValue (const std::string& source,
                                           size_t labelStart,
                                           size_t cursor,
                                           size_t next) const noexcept;

        static int getOpenBracketIndex (const jam::Array<Document::Token>& tokens) noexcept;

        // Resolves the closing ']', attaching Id::href/Id::title onto the
        // matched open bracket token at creation and Id::open onto the
        // returned close token only once resolution succeeds, so no
        // destination/title/index value ever travels back through the caller
        // as a naked pair -- each value attaches to the token that owns it
        // the moment it is known. Every active bracket up to and including
        // the matched one is deactivated here too, mirroring the prior
        // caller-side walk.
        Document::Token getLinkClose (const std::string& source,
                                      size_t cursor,
                                      jam::Array<Document::Token>& tokens) noexcept;

        static Document::Token
        getToken (const std::string& source, size_t cursor, jam::Array<Document::Token>& tokens);

        static int addInlineToken (jam::Array<Document::Token>& tokens,
                                   Document::Token token,
                                   size_t cursor) noexcept;

        int addToken (jam::Array<Document::Token>& tokens, const std::string& source, size_t cursor);

        // --- Phase 3 -- single forward walk: token indices -> nested inline tree --
        //
        // Each run packs (openerIndex, closerIndex, styleFlag) as a Token's own
        // (span, type); a resolved link/image open token carries its own matching
        // close index as Id::close (mirrored back onto the close token as Id::open
        // by addToken()). The walk below opens/closes Elements directly at those
        // indices, attaching each one in-place (at its final tree position) the
        // moment it opens, with no intermediate string-offset bookkeeping and no
        // detached-subtree staging.

        void addEmphasis (Element& rootElement,
                          int index,
                          const jam::Array<Document::Token>& runs,
                          jam::Array<Element*>& stack,
                          jam::Array<int>& closeIndices);

        void addLinkElement (Element& parent,
                             const Document::Token& token,
                             jam::Array<Element*>& stack,
                             jam::Array<int>& closeIndices);

        // Decodes a token's text content from its span and type -- pure
        // function of (inlineSource, token), independent of tree position.
        static juce::String getDecodedText (const std::string& inlineSource, const Document::Token& token);

        // Appends text to parent's trailing text child, merging into an
        // existing run rather than starting a new sibling.
        void appendTextChild (Element& parent, const juce::String& text);

        // Backtick code span content (spec §6.1): newlines fold to spaces,
        // then a single leading+trailing space pair strips unless the whole
        // content is spaces.
        void addCodeSpanElement (Element& parent, const Document::Token& token, const std::string& inlineSource);

        // Hard vs soft line break (spec §6.7): a two-byte break token is
        // always hard; a one-byte '\n' promotes to hard only when it trims
        // at least minimumHardBreakSpaces trailing spaces off the preceding
        // text run, otherwise it appends as a literal newline.
        void addLineBreakElement (Element& parent, const Document::Token& token);

        // Builds the per-token-type inline construction dispatch table --
        // emphasisDelimiter/codeSpan/lineBreak each resolve to their own
        // Element-building rule, closed over the given stack's current
        // active parent.
        jam::Function::Map<int, void> getInlineConstructionTable (
            Element& element, jam::Array<Element*>& stack, const std::string& inlineSource);

        // `inlineSource` is addInlines()' own nested-lex source bytes -- distinct
        // from `document` (the outer/parent Document, used here only to addChild()
        // into the outer tree). Every span/text resolution below MUST go through
        // `inlineSource`, never `document.getSource()`: `tokens`' offsets were
        // minted against the nested lex's own source, and stay valid only as
        // long as they are resolved against that same buffer.
        void buildInlineTree (Element& element,
                              const std::string& inlineSource,
                              const jam::Array<Document::Token>& tokens,
                              const jam::Array<Document::Token>& runs);

        /**
         * @brief Runs the inline pass on @p text, populating @p element's children with the resulting inline tree.
         *
         * Re-lexes @p text directly (addToken() builds tokens and bracket-stack
         * properties over already block-preprocessed bytes -- no Document instance
         * is built for the inline lex), resolves emphasis delimiter runs
         * (getDelimiter(), processEmphasis()), then builds the nested inline
         * element tree in one forward walk (buildInlineTree()).
         */
        // Lexes inlineSource end to end via addToken(), appending every
        // produced Token to tokens -- the inline pass's own tokenizing walk,
        // independent of delimiter resolution and tree construction.
        void addTokens (jam::Array<Document::Token>& tokens, const std::string& inlineSource);

        void addInlines (Element& element, const juce::String& text);
    };

    // =========================================================================
    // tokenizer -- Vocabulary/getToken classify marker runs, text spans, and
    // newlines from Vocabulary::operators alone; build() reads Token::type at
    // line-lead positions to dispatch block grammar, reusing getOperatorToken/
    // getTextToken directly for that classification.
    // =========================================================================

    static Document::Token getOperatorToken (const std::string& source, size_t cursor) noexcept;

    static Document::Token getTextToken (const std::string& source, size_t cursor) noexcept;

    /** @brief Markdown-domain classification vocabulary handed to jam::Document for segmentation. */
    static const Document::Vocabulary& getMarkdownVocabulary();
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
