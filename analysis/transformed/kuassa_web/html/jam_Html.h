#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct HtmlDocument
 * @brief WHATWG-authored-subset HTML tokenizer and tree constructor over jam::Document.
 */
struct HtmlDocument : Document
{
    /**
     * @brief Memoized layout Document for a lexicon-registered HTML resource name.
     *
     * Built once per name and cached in the live SharedDocuments registry: tokens are
     * lexed via getToken(), then build() drives the open-element stack into a detached
     * scratch Document, whose root Element (its id set to the top tag) is appended as
     * the returned Document's single child on success. A failed parse leaves the
     * returned Document's root childless -- callers test
     * `document.root->firstChild == nullptr` to detect that case.
     *
     * @param name The lexicon-registered resource identifier.
     * @return The memoized Document for name, with a childless root on parse failure.
     */
    static const Document& getOrCreate (const juce::Identifier& name);

protected:

    /** @brief HTML segmentation vocabulary handed to jam::Document. */
    const Vocabulary& getVocabulary() const override { return getHtmlVocabulary(); }

    /** @brief Lexes one token at cursor -- markup-operator dispatch to getEndTag()/getDoctype()/getStartTag(), comment segments to getComment(), text otherwise -- and appends it, returning its consumed length. */
    int getToken (Cursor& cursor, int segmentType) override;

    /**
     * @brief Tree construction entry point -- appends the terminating endOfFile token,
     *        then drives the open-element stack over the token stream into a detached
     *        scratch Document, attaches it onto this Document's root only on full
     *        success (root sentinel stays childless on any failure).
     */
    void build() override;

private:

    /** @brief Transient parse-time token accumulator, populated by getToken() and consumed by build(). */
    jam::Array<Document::Token> tokens;

    /** @brief Cached UTF-8 lengths of the Tokens markup markers, spared re-measuring per token. */
    static inline const int endTagOpenLength { juce::String (Chars::endTagOpen).length() };
    static inline const int declarationOpenLength { juce::String (Chars::declarationOpen).length() };


    /** @brief Element property keys reserved for the constructed-tree shape -- a colliding source attribute is a parse failure. Tag and children are Element structure (id/links), not properties, so only text and svg remain reserved. */
    static const jam::Array<juce::Identifier>& getStructuralKeys();

    // =========================================================================
    // html tokenizer segmentation vocabulary (jam::Document)
    // =========================================================================

    /** @brief HTML operator and comment-pair vocabulary handed to jam::Document for segmentation. */
    static const Document::Vocabulary& getHtmlVocabulary();

    // =========================================================================
    // getCommentToken
    // =========================================================================

    /** @brief Comment-segment walk -- probes from cursor to the vocabulary's comment-close marker (or source end), returning the resulting comment token. */
    static Document::Token getCommentToken (Cursor cursor);

    // =========================================================================
    // getMarkupToken
    // =========================================================================

    /** @brief Operator-prefix dispatch -- routes a markup-operator cursor to getEndTag()/getDoctype() by literal prefix, or to getStartTag() on letter lookahead; empty on neither match. */
    static std::optional<Document::Token> getMarkupToken (Cursor cursor);

    // =========================================================================
    // numeric character reference state
    // =========================================================================

    /** @brief Numeric scan state -- consumes a run of decimal or hex digits at @p cursor, advancing it past them, and returns the accumulated code point value, or empty when no digits were consumed. */
    static std::optional<juce::juce_wchar> getCharacterReferenceValue (Cursor& cursor, bool isHexadecimal);

    /** @brief Numeric character reference state -- consumes a `#`-prefixed decimal or hex code point, advancing @p cursor past it, and appends its UTF-8 encoding to @p result. */
    static void appendNumericCharacterReference (std::string& result, Cursor& cursor);

    // =========================================================================
    // named character reference state
    // =========================================================================

    /** @brief Named character reference state -- resolves a `;`-terminated entity name via the generated map::Entity table, advancing @p cursor past it, and appends the replacement's UTF-8 bytes to @p result. */
    static void appendNamedCharacterReference (std::string& result, Cursor& cursor);

    // =========================================================================
    // character reference expansion (character reference / attribute value)
    // =========================================================================

    /** @brief Character reference state -- dispatches a numeric or named `&...;` reference to its expansion, advancing @p cursor past it and appending the decoded bytes to @p result. */
    static void appendCharacterReference (std::string& result, Cursor& cursor);

    // =========================================================================
    // getText
    // =========================================================================

    /** @brief Data state -- consumes character data up to the next tag, verbatim; character-reference expansion is deferred to buildTree. */
    static Document::Token getText (Cursor cursor);

    /**
     * @brief Decodes a raw [start, end) byte span, expanding character references -- the
     *        buildTree-time counterpart of the tokenizer's data-state getText(). Accumulates
     *        into a byte buffer and decodes to juce::String once, at the property-materialization
     *        boundary.
     */
    static juce::String getText (const std::string& source, size_t start, size_t end);

    // =========================================================================
    // getTagName
    // =========================================================================

    /** @brief Tag name state -- scans a start tag's name characters and returns the resulting span. */
    static Document::Token getTagName (Cursor cursor);

    // =========================================================================
    // getOrCreateAttributes
    // =========================================================================

    /** @brief Returns @p token's attributes property array, creating it empty on first use. */
    static jam::Array<Document::Property<Document::Token>>* getOrCreateAttributes (Document::Token& token);

    // =========================================================================
    // addAttribute
    // =========================================================================

    static juce::String getAttributeName (Cursor& cursor, const Document::Token& token);

    static Document::Token getAttributeValue (Cursor& cursor, bool quoted, juce::juce_wchar quoteCharacter, const Document::Token& token);

    static Document::Token getQuotedValue (Cursor& cursor, const Document::Token& token);

    static void addEmptyAttribute (Cursor& cursor, Document::Token& token, const juce::String& attributeName);

    /**
     * @brief Attribute cycle -- name, optional value span, appended to token's attributes property. Self-closing detection stays with the orchestrator.
     *
     * @param cursor The cursor positioned at the start of the attribute name.
     * @param token The token being built; the parsed attribute is appended to its attributes array.
     */
    static void addAttribute (Cursor& cursor, Document::Token& token);

    // =========================================================================
    // getStartTag
    // =========================================================================

    static void getTagAttributes (Cursor& cursor, Document::Token& token);

    /** @brief Tag open state -- orchestrates a start tag's name, attribute list, and self-closing marker. */
    static Document::Token getStartTag (Cursor cursor);

    // =========================================================================
    // getEndTag
    // =========================================================================

    /** @brief End tag open state -- consumes an end tag's name and discards to `>`. */
    static Document::Token getEndTag (Cursor cursor);

    // =========================================================================
    // getComment
    // =========================================================================

    /** @brief Comment state -- forwards the Document comment segment's own span, delimiters included; comment content is not modeled. */
    static Document::Token getComment (size_t start, size_t end);

    // =========================================================================
    // getDoctype
    // =========================================================================

    /** @brief DOCTYPE state -- discards the declaration body up to `>` (doctype content is not modeled). */
    static Document::Token getDoctype (Cursor cursor);

    /** @brief Checks whether a tag name is a void element (no matching end tag expected). */
    static bool isVoidTag (const juce::String& name) noexcept;

    /** @brief Copies a start tag's non-style attributes onto the constructed element, expanding character references. */
    static bool addAttributes (const std::string& source, Document::Element& element, const Document::Token& token);

    static juce::String getUnit (const Css::StyleDeclaration& style, const juce::String& property);

    static bool addStyleDeclaration (const std::string& source,
                                      Document::Element& element,
                                      const Document::Token& token,
                                      const Css::StyleDeclaration& style,
                                      const juce::String& property,
                                      const jam::Function::Map<int, bool>& styleAttributes);

    /**
     * @brief Parses a start tag's `style="..."` attribute via Css::getStyle() and folds each property onto the element.
     *
     * Dispatch is partitioned by CssTokenType using Css::textTokenTypes/Css::numericTokenTypes:
     * text tokens fold verbatim, `number` folds as a bare double, and `dimension`/`percentage`
     * additionally write a `\<property\>-unit` companion attribute (the dimension's unit, or `%`
     * for a percentage). A var() reference is folded as its custom-property name, bypassing
     * the token-type dispatch. Any structural-key or existing-attribute collision is a failure.
     *
     * @param source The byte source backing @p token's spans, used for diagnostics.
     * @param element The constructed element to fold attributes onto.
     * @param token The start tag token, used for its own name span in diagnostics.
     * @param styleText The decoded `style` attribute value.
     * @return True on success, false on an attribute collision.
     */
    static bool addStyleAttributes (const std::string& source, Document::Element& element, const Document::Token& token, const juce::String& styleText);

    /**
     * @brief Links a new element under the currently open element, or -- when the open-element
     *        stack is empty -- claims scratch's own root as the document's single top-level
     *        element by assigning its id in place.
     *
     * A second stack-empty call after the top-level element already has an id is a multiple-
     * root-elements condition: the offending element is still attached, as a child of the
     * existing root, and the root is marked Id::root once for HtmlValidator to report.
     *
     * @param scratch The scratch Document being built.
     * @param stack The open-element stack, top-of-stack is the current parent.
     * @param tagId The tag identifier of the element being opened.
     * @return The linked element.
     */
    static Document::Element* attachElement (Document& scratch, jam::Array<Document::Element*>& stack, const juce::Identifier& tagId);

    /** @brief Adds non-whitespace character data as the text content of the current open element, expanding character references. */
    static bool addText (const std::string& source, const Document::Token& token, jam::Array<Document::Element*>& stack);

    static int getSvgSpan (const jam::Array<Document::Token>& tokens, const std::string& source, int index);

    /**
     * @brief Slices the verbatim source span of an `\<svg\>` element, depth-tracking nested svg tags to its matching end tag.
     *
     * On success, index is advanced past the closing `\</svg\>` tag.
     * An unclosed element is a parse failure -- logged and reported by an empty return.
     *
     * @param tokens The full token stream, positioned at the opening `\<svg\>` start tag.
     * @param source The byte buffer of the tokenized source text.
     * @param index The current token index, advanced in place to the closing end tag on success.
     * @return The verbatim source text of the svg element, or an empty string on unclosed-element failure.
     */
    static juce::String getSvg (const jam::Array<Document::Token>& tokens,
                                const std::string& source,
                                int& index);

    static bool addElementAttributes (const std::string& source, Document::Element& element, const Document::Token& token);

    static bool addStartTagElement (const Document::Token& token, const jam::Array<Document::Token>& tokens, const std::string& source,
                                     int& index, jam::Array<Document::Element*>& stack, Document& scratch);

    /** @brief End-tag arm -- pops the open-element stack; a tag-name mismatch marks the still-open element Id::endTag once for HtmlValidator to report, an empty stack is a no-op. */
    static bool addEndTagElement (const Document::Token& token, const jam::Array<Document::Token>&, const std::string& source,
                                   int&, jam::Array<Document::Element*>& stack, Document&);

    static bool addCharacterElement (const Document::Token& token, const jam::Array<Document::Token>&, const std::string& source,
                                      int&, jam::Array<Document::Element*>& stack, Document&);

    static bool skip (const Document::Token&, const jam::Array<Document::Token>&, const std::string&,
                      int&, jam::Array<Document::Element*>&, Document&);

    /**
     * @brief Tree construction entry point -- drives the open-element stack over the token stream
     *        into scratch, to completion or failure.
     *
     * Every token type is dispatched exhaustively through the treeConstruction map --
     * startTag/endTag/character carry element-tree logic, comment/doctype/endOfFile are
     * no-op skips. After the loop, the stack must be empty and scratch's root must carry a
     * tag id; an unclosed-element or empty-document result is a post-loop validation failure.
     * On any failure, scratch is left partially built and discarded by the caller -- nothing
     * from a failed parse is ever linked onto the returned Document.
     *
     * @param scratch Receives the constructed element tree on success (root id set to the top tag).
     * @param tokens The full token stream, terminated by an endOfFile token.
     * @param source The byte buffer of the tokenized source text, used by getSvg().
     * @return True on a complete, single-root document; false otherwise.
     */
    static bool buildTree (Document& scratch, const jam::Array<Document::Token>& tokens, const std::string& source);
};

using Html = HtmlDocument;

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
