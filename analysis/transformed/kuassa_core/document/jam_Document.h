/**
 * @file jam_Document.h
 * @brief Generic text tokenization/parse engine plus a typed document store
 *        backed by an Owner<Element> arena, addressed through intrusive
 *        parent/child/sibling Element* links, each Element carrying its own
 *        bounded key/value property array.
 */

#pragma once
namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct Document
 * @brief Generic text-data parse engine and typed, Owner-backed document store.
 *
 * Document supplies a domain-agnostic tokenizer/parser (preprocess(),
 * getTokens(), parse()) and stores its resulting typed model as a tree of
 * Element nodes, owned by a single jam::Owner\<Element\>, so every consuming
 * domain (HTML, CSS, and similar authored-subset formats) reuses the same
 * engine while injecting its own Vocabulary (operator/region/quote/comment
 * character sets) and Rules (token-consumption, comment, and build hooks).
 *
 * Element addresses are stable for the lifetime of the owning Document --
 * Owner stores each Element behind a unique_ptr, so intrusive Element*
 * parent/child/sibling links never dangle across insertion or subtree
 * adoption (appendChild moves ownership, not memory).
 *
 * Each embedded resource is parsed at most once per process. A concrete
 * document type — MarkdownDocument, XML — supplies its own getOrCreate(),
 * which memoizes the parsed Document in the live SharedDocuments registry
 * keyed by resource name, so repeated lookups for the same resource return
 * the same instance instead of re-tokenizing.
 */
struct Document
{
    /**
     * @struct Property
     * @brief A document property — an identifier paired with its value.
     * @tparam ValueType   The property's payload type.
     */
    template<typename ValueType>
    struct Property
    {
        juce::Identifier name;
        ValueType value;
    };

    /** @brief Packed (byteOffset, byteLength) pair addressing a span of a Document's source -- the storage type behind Token::span and every cell-span array built by table parsing. */
    using Span = jam::Union<uint32_t, uint32_t>;

    /**
     * @struct Token
     * @brief A classified span of text produced by getTokens(), and the
     *        general-purpose token shape shared by every domain tokenizer
     *        built on Document (Xml, Html, Css, Markdown, and similar).
     *
     * Sub-spans, flags, and decoded payloads a domain tokenizer needs beyond
     * (start, end, type) live in properties, keyed by juce::Identifier and
     * addressed through get()/add()/contains() -- the same keyed-pair shape
     * as Element::properties, scoped to Token::Value.
     */
    struct Token
    {
        Span span { Span::pack (0, 0) };///< Packed (byteOffset, byteLength) into the owning Document's source.
        int type {
            map::DocumentTokenType::text
        };///< The span's classification (map::DocumentTokenType).

        Token() = default;

        /**
         * @brief Construct a Token spanning [startOffset, endOffset) of the owning Document's source.
         * @param startOffset  Byte offset of the span's start (inclusive).
         * @param endOffset    Byte offset of the span's end (exclusive).
         * @param typeIn       The span's classification (map::DocumentTokenType).
         */
        Token (size_t startOffset,
               size_t endOffset,
               int typeIn = map::DocumentTokenType::text) noexcept;

        /** @brief Byte offset of the span's start, into the owning Document's source. */
        uint32_t offset() const noexcept { return span.unpack<0>(); }

        /** @brief Byte length of the span. */
        uint32_t length() const noexcept { return span.unpack<1>(); }

        /**
         * @brief Repacks this Token's span to [startOffset, endOffset).
         * @param startOffset  Byte offset of the span's start (inclusive).
         * @param endOffset    Byte offset of the span's end (exclusive).
         */
        void setSpan (size_t startOffset, size_t endOffset) noexcept;

        /** The closed set of payload types a Token's property can hold. */
        using Value = std::variant<bool,
                                   double,
                                   juce::String,
                                   Token,
                                   jam::Array<Property<Token>>,
                                   int,
                                   jam::Union<uint16_t, uint16_t>>;

        jam::Array<Property<Value>> properties;///< This token's own key/value pairs.

        /**
         * @brief Retrieve a property's payload by key (non-const).
         * @tparam ValueType   The expected payload type.
         * @param key  The property key to look up.
         * @return A pointer to the stored payload.
         * @warning Throws std::out_of_range if the key is absent; throws std::bad_variant_access if the stored type does not match ValueType.
         */
        template<typename ValueType>
        ValueType* get (const juce::Identifier& key)
        {
            for (auto& [propertyKey, propertyValue] : properties)
                if (propertyKey == key)
                    return &std::get<ValueType> (propertyValue);

            throw std::out_of_range ((juce::String ("Token::get: absent key ") + key.toString()).toStdString());
        }

        /** Const overload of get(). */
        template<typename ValueType>
        const ValueType* get (const juce::Identifier& key) const
        {
            return const_cast<Token*> (this)->get<ValueType> (key);
        }

        /**
         * @brief Test whether a property key is present on this token.
         * @param key  The property key to test.
         * @return True if the key exists on this token's properties array.
         */
        bool contains (const juce::Identifier& key) const noexcept;

        /**
         * @brief Construct and append a property of type ValueType under @p key.
         * @tparam ValueType   The payload type to construct.
         * @tparam Args  Constructor argument types.
         * @param key    The property key.
         * @param args   Arguments forwarded to ValueType's constructor.
         * @return A pointer to the newly stored payload.
         */
        template<typename ValueType, typename... Args>
        ValueType* add (const juce::Identifier& key, Args&&... args)
        {
            properties.add ({ key, Value { ValueType (std::forward<Args> (args)...) } });
            auto& [propertyKey, propertyValue] { properties.last() };
            return std::get_if<ValueType> (&propertyValue);
        }
    };

    struct Element;

    using Tokens      = jam::Array<Token>;
    using Elements    = jam::Array<Element*>;
    using Identifiers = jam::Array<juce::Identifier>;

    /** The closed set of payload types an Element's value can hold. */
    using Value = std::variant<std::monostate,
                               bool,
                               int,
                               double,
                               juce::uint32,
                               juce::Identifier,
                               juce::String,
                               Tokens,
                               juce::int64,
                               juce::var,
                               Element*,
                               Elements,
                               Identifiers>;

    /**
     * @struct Element
     * @brief One node in the tree: a structural tree node carrying its own
     *        bounded set of properties.
     *
     * Children are structural, linked via firstChild/lastChild/nextSibling.
     * Properties are leaf key/value pairs, bounded per element and never
     * looked up via the owning Owner -- they live directly in the
     * properties array rather than as linked Element nodes.
     */
    struct Element
    {
        juce::Identifier id;///< Tag for structural nodes, key for keyed nodes.

        Element* parent { nullptr };
        Element* firstChild { nullptr };
        Element* lastChild { nullptr };
        Element* nextSibling { nullptr };

        jam::Array<Property<Value>> properties;///< This node's own key/value pairs.

        /** Content hash over (parent, id) only -- backs Owner's O(1) keyed lookup. */
        struct Hash
        {
            size_t operator() (const Element& element) const noexcept;
        };

        /** Content equality over (parent, id) only -- matches Hash. */
        bool operator== (const Element& other) const noexcept;

        struct Iterator
        {
            Element* current { nullptr };

            bool operator!= (const Iterator& other) const noexcept;
            Element* operator*() const noexcept { return current; }

            Iterator& operator++() noexcept;
        };

        Iterator begin() const noexcept { return Iterator { firstChild }; }
        Iterator end() const noexcept { return Iterator { nullptr }; }

        /**
         * @brief Find the first direct child whose id is @p idToLookFor.
         *        Link walk, O(children) -- the mirror of juce::XmlElement::getChildByName.
         */
        Element* getChildByID (const juce::Identifier& idToLookFor) const noexcept;

        /**
         * @brief Retrieve a property's payload by key (non-const).
         * @tparam ValueType   The expected payload type.
         * @param key  The property key to look up.
         * @return A pointer to the stored payload.
         * @warning Throws std::out_of_range if the key is absent; throws std::bad_variant_access if the stored type does not match ValueType.
         */
        template<typename ValueType>
        ValueType* get (const juce::Identifier& key)
        {
            for (auto& [propertyKey, propertyValue] : properties)
                if (propertyKey == key)
                    return &std::get<ValueType> (propertyValue);

            throw std::out_of_range ((juce::String ("Element::get: absent key ") + key.toString()).toStdString());
        }

        /** Const overload of get(). */
        template<typename ValueType>
        const ValueType* get (const juce::Identifier& key) const
        {
            return const_cast<Element*> (this)->get<ValueType> (key);
        }

        /**
         * @brief Retrieve a property's payload by value, or @p defaultValue when absent.
         * @tparam ValueType   The expected payload type.
         * @param key           The property key to look up.
         * @param defaultValue  The value returned when @p key is absent, or stored
         *                      under a different type than ValueType.
         * @return The stored payload by value, or @p defaultValue.
         */
        template<typename ValueType>
        ValueType get (const juce::Identifier& key, const ValueType& defaultValue) const
        {
            for (const auto& [propertyKey, propertyValue] : properties)
                if (propertyKey == key)
                    if (const auto* value { std::get_if<ValueType> (&propertyValue) })
                        return *value;

            return defaultValue;
        }

        /**
         * @brief Test whether a property key is present on this node.
         * @param key  The property key to test.
         * @return True if the key exists on this node's properties array.
         */
        bool contains (const juce::Identifier& key) const noexcept;

        /**
         * @brief Test whether the property stored under @p key holds ValueType.
         * @tparam ValueType   The payload type to test for.
         * @param key  The property key to look up.
         * @return True if @p key is present and its payload holds ValueType;
         *         false if @p key is absent or holds a different type.
         */
        template<typename ValueType>
        bool isType (const juce::Identifier& key) const noexcept
        {
            for (const auto& [propertyKey, propertyValue] : properties)
                if (propertyKey == key)
                    return std::holds_alternative<ValueType> (propertyValue);

            return false;
        }

        /** @brief Test whether the juce::String property stored under @p key equals @p value.
         *  @param key    The property key to look up.
         *  @param value  The text to match.
         *  @return True if @p key is present, holds a juce::String, and that string equals @p value's text; false otherwise. */
        bool hasProperty (const juce::Identifier& key, const juce::Identifier& value) const noexcept
        {
            for (const auto& [propertyKey, propertyValue] : properties)
                if (propertyKey == key)
                    if (const auto* text { std::get_if<juce::String> (&propertyValue) })
                        return text->compare (value.toString()) == 0;

            return false;
        }

        /**
         * @brief Test whether this node's own id matches @p tag.
         * @param tag  The tag identifier to match.
         */
        bool isTag (const juce::Identifier& tag) const noexcept { return id == tag; }

        /** @brief Concatenates the text of this node and every descendant, depth-first. */
        juce::String getAllSubText() const;

        /**
         * @brief Applies @p function to this Element and every descendant, self-first.
         *
         * If @p function is invocable returning bool, its return value controls
         * recursion: a false return prunes descent into that node's children,
         * while true continues the depth-first walk. If @p function's return
         * type is not bool (including void), every node in the subtree is
         * visited unconditionally.
         *
         * @tparam Function A callable type taking (const Element&), optionally returning bool.
         * @param function  Function invoked once per Element in the subtree.
         */
        template<typename Function>
        void applyFunctionRecursively (const Function& function) const
        {
            if constexpr (std::is_invocable_r_v<bool, Function, const Element&>)
            {
                if (function (*this))
                    for (auto* child : *this)
                        child->applyFunctionRecursively (function);
            }
            else
            {
                function (*this);

                for (auto* child : *this)
                    child->applyFunctionRecursively (function);
            }
        }

        /**
         * @brief Invokes @p function for every (key, value) property on this node.
         * @param function  Called once per property, in stored order.
         */
        void applyToProperties (
            const std::function<void (const juce::Identifier&, const Value&)>& function) const;

        /**
         * @brief Construct and append a property of type ValueType under @p key.
         * @tparam ValueType   The payload type to construct.
         * @tparam Args  Constructor argument types.
         * @param key    The property key.
         * @param args   Arguments forwarded to ValueType's constructor.
         * @return A pointer to the newly stored payload.
         */
        template<typename ValueType, typename... Args>
        ValueType* add (const juce::Identifier& key, Args&&... args)
        {
            properties.add ({ key, Value { ValueType (std::forward<Args> (args)...) } });
            auto& [propertyKey, propertyValue] { properties.last() };
            return std::get_if<ValueType> (&propertyValue);
        }
    };

    /**
     * @struct Vocabulary
     * @brief View over a language's generated byte-class table and comment literals.
     */
    struct Vocabulary
    {
        const jam::LookupTable<int, int, 256>&
            classes;///< Generated byte-class table (map::Byte values), indexed by byte.
        std::string_view commentOpen;///< Literal sequence that opens a comment span.
        std::string_view commentClose;///< Literal sequence that closes a comment span.

        /**
         * @brief The generated byte class for a byte.
         * @param ch  The byte to classify.
         * @return The map::Byte class assigned to @p ch.
         */
        int getClass (juce::juce_wchar ch) const noexcept { return classes[static_cast<int> (ch)]; }
    };

    /**
     * @struct Cursor
     * @brief Byte cursor over a Document's source, mirroring
     *        juce::CharPointerType's advance/dereference shape for the
     *        engine's parse() walk.
     */
    struct Cursor
    {
        const std::string& source;///< The Document source this cursor walks.
        size_t position;///< Current byte offset into source.

        /** @brief True once position has reached or passed the end of source. */
        bool isEmpty() const noexcept { return position >= source.size(); }

        /** @brief The byte at the current position, widened to juce_wchar. */
        juce::juce_wchar operator*() const noexcept;

        /** @brief Advances the cursor by one byte. */
        Cursor& operator++() noexcept;

        /** @brief Advances the cursor by @p numBytes bytes. */
        Cursor& operator+= (int numBytes) noexcept;

        /** @brief The cursor's current byte offset into source. */
        size_t getOffset() const noexcept { return position; }

        /** @brief True if @p pattern occurs literally at the cursor's current position. */
        bool startsWith (std::string_view pattern) const noexcept;
    };

    /**
     * @struct Validator
     * @brief Domain-supplied rule set, checked against a Document as a whole.
     *
     * A domain injects its keyed validation rules through getRules(); isValid()
     * runs every rule deterministically and aggregates every failure rather than
     * stopping at the first one.
     */
    struct Validator
    {
        using Rules = jam::Function::Map<juce::String, juce::Result>;

        virtual ~Validator() = default;

        /* contract */
        virtual const Rules& getRules() const = 0;

        /* machinery */
        juce::Result isValid (const Document& document) const;
    };

    /**
     * @struct Writer
     * @brief Domain-supplied text renderer for a Document, plus a shared
     *        file-write path.
     */
    struct Writer
    {
        virtual ~Writer() = default;

        /* contract */
        virtual juce::String getText (const Document& document) const = 0;

        /* machinery */
        bool toFile (const Document& document, const juce::File& file) const;
    };

    /** Code point substituted, along with lone surrogates, during preprocess(). */
    static constexpr juce::juce_wchar eofCodePoint { 0 };

    /** Radix for hexadecimal numeric literals, for use by domain Rules. */
    static constexpr int hexadecimalRadix { 16 };

    /** Radix for decimal numeric literals, for use by domain Rules. */
    static constexpr int decimalRadix { 10 };

    /**
     * @brief Creates a fresh document with a single root Element ready for
     *        property and child construction.
     */
    Document() { root = elements.add (std::make_unique<Element>()).get(); }

    virtual ~Document() = default;

    Document (Document&&) noexcept = default;
    Document& operator= (Document&&) noexcept = default;

    Element* root { nullptr };///< The tree's root element.

    Element::Iterator begin() const noexcept { return root->begin(); }
    Element::Iterator end() const noexcept { return root->end(); }

    /** @brief Returns the preprocessed UTF-8 source bytes backing every Token span produced by getTokens()/parse(). */
    const std::string& getSource() const noexcept { return source; }

    /**
     * @brief Appends @p text as a new appendix past the end of the parsed source.
     *
     * The append is byte-stable: every offset already handed out by earlier
     * parsing stays valid, since nothing before the previous end is
     * rewritten. lineOffsets is extended for every newline found within
     * @p text, keeping getLineNumber() accurate over the appended range.
     *
     * @param text  The bytes to append past the current end of source.
     * @return The byte offset at which @p text begins.
     */
    size_t addSource (std::string_view text);

    /** @brief Materializes @p token's span as a juce::String, decoded from source as UTF-8. */
    juce::String getText (const Token& token) const;

    /**
     * @brief Returns the 1-based line number containing @p byteOffset.
     *
     * Binary search over lineOffsets, the line-start cache built once per
     * parse() -- zero allocation, no per-call source re-scan.
     *
     * @param byteOffset  Byte offset into getSource().
     * @return The 1-based line number containing @p byteOffset.
     */
    uint32_t getLineNumber (uint32_t byteOffset) const noexcept;

    /**
     * @brief Normalizes line endings and invalid/surrogate code points for tokenization.
     *
     * Rewrites CR and CRLF sequences to LF, form feed to LF, and replaces
     * eofCodePoint and any lone UTF-16 surrogate with the Unicode
     * replacement character, producing a single contiguous copy safe to
     * pass to getTokens().
     *
     * @param input The raw source text to normalize.
     * @return The normalized text.
     */
    static juce::String preprocess (const juce::String& input);

    /**
     * @brief Test whether @p ch is an ASCII decimal digit ('0'-'9').
     * @param ch  The character to test.
     * @return True if @p ch is a decimal digit.
     */
    static bool isDigit (juce::juce_wchar ch) noexcept;

    /**
     * @brief Test whether @p ch is an ASCII hexadecimal digit (0-9, A-F, a-f).
     * @param ch  The character to test.
     * @return True if @p ch is a hexadecimal digit.
     */
    static bool isHexDigit (juce::juce_wchar ch) noexcept;

    /**
     * @brief Test whether @p ch is an ASCII letter (A-Z, a-z).
     * @param ch  The character to test.
     * @return True if @p ch is an ASCII letter.
     */
    static bool isLetter (juce::juce_wchar ch) noexcept;

    /**
     * @brief Test whether @p ch is a recognized whitespace character (space, tab, newline).
     * @param ch  The character to test.
     * @return True if @p ch is whitespace.
     */
    static bool isWhitespace (juce::juce_wchar ch) noexcept;

    /**
     * @brief Splits a line of preprocessed text into classified Token spans.
     *
     * Walks @p line once, classifying runs of characters per @p vocabulary:
     * characters classed map::Byte::operators in Vocabulary::classes
     * outside quotes/regions form an operators run; map::Byte::regionOpen/
     * regionClose classed characters increment/decrement a nesting depth, and
     * the enclosed span (delimiters included) is classified as region;
     * map::Byte::quote classed characters toggle a quote state, and the
     * quoted span (quote characters included) is likewise classified as
     * region; a match of
     * Vocabulary::commentOpen closes the current run and delegates to
     * addCommentToken() to consume through Vocabulary::commentClose.
     *
     * @param text       The preprocessed UTF-8 bytes to tokenize.
     * @param vocabulary The domain's operator/region/quote/comment character sets.
     * @return The ordered list of classified tokens spanning @p text.
     *
     * @note Asserts Vocabulary::commentOpen and Vocabulary::commentClose are
     *       either both empty or both non-empty.
     */
    static jam::Array<Token> getTokens (const std::string& text, const Vocabulary& vocabulary);

    // Constructs an Element, links it at the end of parent's child chain.
    Element* addChild (Element& parent, const juce::Identifier& id);

    /**
     * @brief Appends @p child's subtree onto @p parent's child chain.
     *
     * @p child's Owner<Element> entries are moved wholesale into this
     * Document's Owner -- each Element lives behind a unique_ptr, so the
     * move relocates ownership only, never the Element's address, leaving
     * every intrusive link within the subtree valid without relinking.
     * Only @p child's root Element gains a new parent link.
     *
     * @param parent  The Element to append the subtree to.
     * @param child   The standalone subtree to adopt.
     * @return The appended subtree's root, now living in this Document.
     */
    Element* appendChild (Element& parent, Document&& child);

    /**
     * @brief Appends @p child's direct children onto this Document's root
     *        child chain, discarding @p child's own root.
     *
     * Delegates to appendChildren (Element&, Document&&) using root as the
     * target parent -- see that overload for the full ownership-transfer
     * mechanics.
     *
     * @param child  The standalone subtree whose direct children are
     *               adopted; ownership of @p child's Owner<Element> entries
     *               is moved wholesale into this Document, leaving @p child
     *               emptied of its elements.
     * @return The first adopted child, or nullptr if @p child.root has no children.
     */
    Element* appendChildren (Document&& child) { return appendChildren (*root, std::move (child)); }

    /**
     * @brief Appends @p child's direct children onto @p parent's child chain, discarding @p child's own root.
     *
     * Walks @p child.root's child chain, relinking each child's parent to
     * @p parent and chaining it onto parent's existing child list -- the
     * same firstChild/lastChild/nextSibling mechanics appendChild() uses.
     * @p child's Owner<Element> entries -- including @p child.root itself --
     * are moved wholesale into this Document's Owner -- each Element lives
     * behind a unique_ptr, so the move relocates ownership only, never the
     * Element's address, leaving every intrusive link within the subtree
     * valid without relinking. @p child.root itself is never linked into
     * the tree: its parent and sibling links stay null, so it can never
     * collide with a (parent, id) probe.
     *
     * @p child's source is appended onto this Document's own source via
     * addSource(), and every span within the adopted subtree is rebased by
     * the resulting base offset via setSpanOffsets(), so every Token span
     * embedded in the subtree's properties keeps addressing the correct
     * bytes once the two sources are merged.
     *
     * @param parent  The Element whose child chain adopts @p child's children.
     * @param child   The standalone subtree whose direct children are adopted.
     * @return The first adopted child, or nullptr if @p child.root has no children.
     */
    Element* appendChildren (Element& parent, Document&& child);

private:
    /**
     * @brief Consumes a comment span at @p cursor and appends it as a Token.
     *
     * @p cursor must already point at a confirmed match of
     * Vocabulary::commentOpen. Skips past the open sequence, scans forward
     * to the matching Vocabulary::commentClose (or to end of input if none
     * is found), and appends the full span (open through close, inclusive)
     * to @p tokens as map::DocumentTokenType::comment.
     *
     * @param tokens      The token list to append the comment Token to.
     * @param text        The byte source @p cursor indexes into.
     * @param cursor      Byte offset positioned at the start of Vocabulary::commentOpen.
     * @param vocabulary  The domain's operator/region/quote/comment character sets.
     * @return The number of bytes spanned by the consumed comment,
     *         for the caller to advance its own cursor by.
     */
    static int addCommentToken (jam::Array<Token>& tokens,
                                const std::string& text,
                                size_t cursor,
                                const Vocabulary& vocabulary);

    static bool isCommentOpen (const std::string& text,
                               size_t cursor,
                               const Vocabulary& vocabulary,
                               bool insideQuoteOrRegion) noexcept;

    static char getQuote (char currentQuote, char character) noexcept;

    static int getDepth (int currentDepth, char quote, int byteClass) noexcept;

    /**
     * @brief Rebases @p token's span, and every span reachable through its
     *        properties, by @p offset.
     *
     * Every Token::Value alternative is matched with an explicit constexpr
     * arm, so a new alternative added to the variant fails to compile here
     * until its rebase behavior is decided.
     *
     * @param token   The token whose span (and property spans) are rebased.
     * @param offset  The byte offset added to every span within @p token.
     */
    static void setSpanOffsets (Token& token, size_t offset);

    /**
     * @brief Rebases every span reachable from @p element's properties, and
     *        its full child subtree, by @p offset.
     *
     * Applied by appendChildren() to an adopted subtree once its source has
     * been appended past this Document's own source, so every Token span
     * embedded in the subtree's properties keeps pointing at the correct
     * byte range within the merged source. Every Element::Value alternative
     * is matched with an explicit constexpr arm, so a new alternative added
     * to the variant fails to compile here until its rebase behavior is
     * decided.
     *
     * @param element  The subtree root whose properties and descendants are rebased.
     * @param offset   The byte offset added to every span within @p element's subtree.
     */
    static void setSpanOffsets (Element& element, size_t offset);

protected:
    /**
     * @brief Parses a raw buffer into this Document, via the derived class's
     *        getToken()/build() hooks.
     *
     * Decodes @p data/@p size to a juce::String, normalizes it with
     * preprocess(), segments it with getTokens() using the vocabulary from
     * getVocabulary(), then walks the segmented text: for each segment run,
     * getToken() is called once and the cursor advances by the length it
     * returns. Once the entire input is consumed, build() is invoked to
     * assemble the typed model.
     *
     * @param data       Pointer to the raw source bytes.
     * @param size       Size of @p data in bytes.
     *
     * @note A non-positive length returned by getToken() truncates parsing
     *       (jassertfalse, debug log).
     */
    void parse (const char* data, int size);

    /**
     * @brief Consumes one token at @p cursor's position and advances it.
     * @param cursor       The cursor positioned at the token's start.
     * @param segmentType  The classification of the segment @p cursor sits within.
     * @return The token's length in bytes; parse() advances @p cursor by this amount.
     */
    virtual int getToken (Cursor& cursor, int segmentType) = 0;

    /** @brief Assembles the typed model into this Document, once the entire input is consumed. */
    virtual void build() = 0;

    /** @brief The language's vocabulary — a view over its generated class table and comment literals. */
    virtual const Vocabulary& getVocabulary() const = 0;

    jam::Owner<Element> elements;///< Sole storage: ownership plus O(1) content-hash lookup.
    std::string
        source;///< Preprocessed UTF-8 source bytes backing every Token span produced by getTokens()/parse().
    jam::Array<uint32_t>
        lineOffsets;///< Byte offset of each line's start in source, built once by parse(); backs getLineNumber().

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Document)
};

/**
 * @struct SharedDocuments
 * @brief Registry of parsed documents, keyed by BinaryData resource name.
 *
 * SharedDocuments is the memoization registry a concrete document type's
 * getOrCreate() — MarkdownDocument::getOrCreate(), XML::getOrCreate() —
 * reads from and writes into. It is owned via SharedInstance by
 * PluginEditorLayout — joined across every editor sharing the same layout
 * resources, destroyed with the last holder — and reached through the
 * Instance chain at lookup time.
 */
struct SharedDocuments
    : public jam::SegmentedHashMap<juce::Identifier, std::unique_ptr<Document>>
    , public Instance<SharedDocuments>
{
};

/**_____________________________END OF NAMESPACE______________________________*/
} /** namespace jam */
