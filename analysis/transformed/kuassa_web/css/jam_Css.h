#pragma once

namespace jam
{
/*____________________________________________________________________________*/

struct CssDocument : Document
{
    /** @brief One parsed `property: value` pair, with its !important flag. */
    struct Declaration
    {
        juce::String property;
        jam::Array<int> value;
        bool important { false };
    };

    /**
     * @class StyleDeclaration
     * @brief CSSOM-shaped read-only view over a parsed declaration block (CSSStyleDeclaration subset).
     */
    class StyleDeclaration
    {
    public:
        StyleDeclaration() = default;

        /** @brief Owning constructor — moves the byte source and token array in; indices in Declaration::value resolve into this->tokens against this->bytes. */
        explicit StyleDeclaration (std::string&& sourceBytes,
                                   jam::Array<Document::Token>&& sourceTokens,
                                   jam::Array<Declaration>&& sourceDeclarations);

        StyleDeclaration (StyleDeclaration&&) noexcept = default;
        StyleDeclaration& operator= (StyleDeclaration&&) noexcept = default;

        /** @brief Number of declarations in the block (CSSStyleDeclaration::length). */
        int length() const noexcept { return declarations.size(); }

        /**
         * @brief Returns the property name at index (CSSStyleDeclaration::item).
         *
         * @param index The zero-based declaration index.
         * @return The property name, or an empty string if index is out of range.
         */
        juce::String item (int index) const;

        /**
         * @brief Returns the serialized value text for a property (CSSStyleDeclaration::getPropertyValue).
         *
         * @param property The property name to look up.
         * @return The value tokens serialized back to text, or an empty string if unset.
         */
        juce::String getPropertyValue (juce::StringRef property) const;

        /**
         * @brief Resolves a `var(--name)` reference on a property to its custom-property name.
         *
         * @param attributeName The property name whose value is checked for a var() reference.
         * @return The referenced custom-property name, or an empty string if the value is not a var() reference.
         */
        juce::String getVariable (juce::StringRef attributeName) const;

        /**
         * @brief Checks whether a property has a declaration in this block.
         *
         * @param attributeName The property name to check.
         * @return True if a declaration for attributeName exists, false otherwise.
         */
        bool hasAttribute (juce::StringRef attributeName) const noexcept;

        /**
         * @brief Reads a property's first value token, serialized to text by its token type.
         *
         * @param attributeName The property name to look up.
         * @return The serialized value string, or an empty string if the property is unset.
         */
        juce::String getStringAttribute (juce::StringRef attributeName) const;

        /**
         * @brief Reads the unit suffix of a property's first value token, if it is a dimension.
         *
         * @param attributeName The property name to look up.
         * @return The unit string, or an empty string if the property is unset or not a dimension.
         */
        juce::String getUnit (juce::StringRef attributeName) const;

        /**
         * @brief Reads the numeric value of a property's first value token.
         *
         * @param attributeName The property name to look up.
         * @return The numeric value, or 0.0 if the property is unset.
         */
        double getDoubleAttribute (juce::StringRef attributeName) const;

        /**
         * @brief Reads the token type of a property's first value token (CssTokenType).
         *
         * @param attributeName The property name to look up.
         * @return The CssTokenType of the first value token, or CssTokenType::ident if the property is unset.
         */
        int getPropertyType (juce::StringRef attributeName) const;

    private:
        const Declaration* findDeclaration (juce::StringRef property) const;

        std::string bytes;
        jam::Array<Document::Token> tokens;
        jam::Array<Declaration> declarations;
    };

    /** @brief One parsed CSS rule -- style rule, at-rule, or nested media rule (CSSRule subset). */
    struct Rule
    {
        int type { map::CssRuleType::unknownRule };
        juce::String selectorText;
        jam::Array<Declaration> style;
        jam::Array<Rule> cssRules;
    };

    /** @brief Top-level parse result -- the ordered list of rules (CSSStyleSheet subset). */
    struct StyleSheet
    {
        jam::Array<Rule> cssRules;
    };

    /** @brief CssTokenType values serialized as verbatim text (ident/string/url/hash) -- the text-token partition shared with jam_Html.h. */
    static constexpr std::array<int, 4> textTokenTypes {
        map::CssTokenType::ident,
        map::CssTokenType::string,
        map::CssTokenType::url,
        map::CssTokenType::hash,
    };

    /** @brief CssTokenType values serialized as a number plus an optional unit/percent suffix (number/dimension/percentage) -- the numeric-token partition shared with jam_Html.h. */
    static constexpr std::array<int, 3> numericTokenTypes {
        map::CssTokenType::number,
        map::CssTokenType::dimension,
        map::CssTokenType::percentage,
    };

    /**
     * @brief Tokenizes and parses an inline declaration block (e.g. an HTML style="..." attribute) into a StyleDeclaration.
     *
     * @param input The declaration-block source text, without surrounding braces.
     * @return The parsed StyleDeclaration.
     */
    static StyleDeclaration getStyle (const juce::String& input);

    /**
     * @brief Memoized stylesheet Document for a lexicon-registered CSS resource name.
     *
     * Built once per name and cached by SharedDocuments: colours/appearance/metrics/unit
     * are each a child element of the document root, holding one property per declaration
     * (opaque ARGB juce::uint32, var()-resolved juce::Identifier, double, or unit juce::String,
     * respectively, in authored order). window/fonts/style are child elements addressed by
     * Id::window/Id::fonts/Id::style; per-class widget styles and per-face font entries are in
     * turn child elements of those, populated by the addRules() selector dispatch.
     *
     * @param name The lexicon-registered resource identifier.
     * @return The memoized Document for name.
     */
    static const Document& getOrCreate (const juce::Identifier& name);

protected:

    /** @brief CSS segmentation vocabulary handed to jam::Document. */
    const Vocabulary& getVocabulary() const override { return getCssVocabulary(); }

    /** @brief Lexes one token at cursor via the 4.3.1 getToken() dispatch and appends it, returning its consumed length -- walks a probe copy so parse() remains the sole advancer of cursor. */
    int getToken (Cursor& cursor, int segmentType) override;

    /**
     * @brief Tree/model construction entry point -- assembles the CSSOM-shaped
     *        model directly onto this Document's root, once the entire token
     *        stream has been accumulated by getToken().
     */
    void build() override;

private:

    static constexpr int stylesheetProduction { 0 };
    static constexpr int declarationsProduction { 1 };

    int entryProduction { stylesheetProduction };

    /** @brief Transient parse-time token accumulator, populated by getToken() and consumed by build()/getStyle(). */
    jam::Array<Document::Token> tokens;

    /** @brief Transient parse-time declarations accumulator, populated by build() under declarationsProduction and consumed by getStyle(). */
    jam::Array<Declaration> declarations;

    static const Declaration* findDeclaration (const jam::Array<Declaration>& declarations,
                                               juce::StringRef property);

    static juce::String getVariable (const std::string& source,
                                     const jam::Array<Document::Token>& tokens,
                                     const jam::Array<Declaration>& declarations,
                                     juce::StringRef attributeName);

    static juce::String getStringAttribute (const std::string& source,
                                            const jam::Array<Document::Token>& tokens,
                                            const jam::Array<Declaration>& declarations,
                                            juce::StringRef attributeName);

    static juce::String getUnit (const std::string& source,
                                 const jam::Array<Document::Token>& tokens,
                                 const jam::Array<Declaration>& declarations,
                                 juce::StringRef attributeName);

    static double getDoubleAttribute (const jam::Array<Document::Token>& tokens,
                                      const jam::Array<Declaration>& declarations,
                                      juce::StringRef attributeName);

    static int getPropertyType (const jam::Array<Document::Token>& tokens,
                                const jam::Array<Declaration>& declarations,
                                juce::StringRef attributeName);

    static juce::String getPropertyValue (const std::string& source,
                                          const jam::Array<Document::Token>& tokens,
                                          const jam::Array<Declaration>& declarations,
                                          juce::StringRef property);

    /** @brief Parses a full token stream into a StyleSheet (5.4.1 getRules over the whole range). */
    static StyleSheet parseStyleSheet (const std::string& source, jam::Array<Document::Token>& tokens);

    /** @brief Strips the `--` custom-property prefix from a property name. */
    static juce::String stripCustomPropertyPrefix (const juce::String& property);

    /**
     * @brief Resolves a property's colour value to an ARGB juce::uint32.
     *
     * Accepts the `transparent` keyword and 3-, 6-, or 8-digit hex values, expanding a
     * 3-digit shorthand to its 6-digit form before parsing. 3- and 6-digit values are
     * opaque-masked; an 8-digit value is CSS Color Level 4 `#RRGGBBAA` -- its trailing
     * alpha byte is moved to the top byte to match juce's ARGB layout. Any other value
     * is an implausible-hex parse error -- logged and asserted, returning transparent black.
     *
     * @param source The byte source the token spans index into.
     * @param tokens The full token stream.
     * @param declarations The parsed declaration list to search for property.
     * @param property The property name whose value is resolved.
     * @return The ARGB colour value.
     */
    static juce::uint32 toColourValue (const std::string& source,
                                       const jam::Array<Document::Token>& tokens,
                                       const jam::Array<Declaration>& declarations,
                                       const juce::String& property);

    /** @brief Copies every declaration in block into section as an opaque-colour property, in authored order. */
    static void addColours (Document& document, Document::Element& section,
                            const jam::Array<Document::Token>& tokens,
                            const jam::Array<Declaration>& declarations);

    /** @brief Copies every declaration in block into section as a var()-resolved juce::Identifier property, in authored order. */
    static void addAppearance (Document& document, Document::Element& section,
                               const jam::Array<Document::Token>& tokens,
                               const jam::Array<Declaration>& declarations);

    /**
     * @brief Copies every declaration in block into section as a double metric property, with its unit companion on unit when present.
     *
     * @param document The document owning section and unit.
     * @param section Receives one double property per declaration, keyed by the custom-property-stripped name.
     * @param unit Receives the matching juce::String unit-suffix property, only for declarations that carry one.
     * @param tokens The full token stream declarations indexes into.
     * @param declarations The declaration list to copy into section and unit.
     */
    static void addMetrics (Document& document, Document::Element& section, Document::Element& unit,
                            const jam::Array<Document::Token>& tokens,
                            const jam::Array<Declaration>& declarations);

    /** @brief Folds one declaration onto section as a typed property, dispatching by CssTokenType (text verbatim, numeric as double). */
    static void addStyleAttribute (Document& document, Document::Element& section,
                                   const jam::Array<Document::Token>& tokens,
                                   const jam::Array<Declaration>& declarations,
                                   const juce::String& property,
                                   const juce::Identifier& id);

    /** @brief Folds every declaration in block onto section via addStyleAttribute(), stripping the custom-property prefix from each id. */
    static void addWindow (Document& document, Document::Element& section,
                           const jam::Array<Document::Token>& tokens,
                           const jam::Array<Declaration>& declarations);

    /** @brief Folds a class-selector rule's appearance declarations into the shared style element, as a child element keyed by the class name. */
    static void addWidgetStyle (Document& document,
                                Document::Element*& styleSection,
                                const jam::Array<Document::Token>& tokens,
                                const Rule& rule);

    /** @brief Builds the selector-text -> handler dispatch table used by addRules(). */
    static const jam::Function::Map<juce::String, void>& getSelectors();

    /**
     * @brief Dispatches every rule in sheet -- font-face rules to addFonts(), style rules to the matching selector handler or addWidgetStyle().
     *
     * @param document The document being built.
     * @param fonts Tracks the lazily created fonts section across calls.
     * @param style Tracks the lazily created style section across calls.
     * @param tokens The full token stream each rule's declarations index into.
     * @param sheet The parsed rule list to fold into document.
     * @param selectors The selector-text dispatch table built by build().
     */
    static void addRules (Document& document,
                          Document::Element*& fonts,
                          Document::Element*& style,
                          const jam::Array<Document::Token>& tokens,
                          const StyleSheet& sheet,
                          const jam::Function::Map<juce::String, void>& selectors);

    /** @brief Folds a font-face rule's rasterizer/gamma/contrast/embolden custom properties onto fontsSection, when present. */
    static void addFontsAttributes (Document& document, Document::Element& fontsSection,
                                    const jam::Array<Document::Token>& tokens,
                                    const jam::Array<Declaration>& declarations);

    /** @brief Lazily creates the fonts section and its font-face attributes on first call, then appends this rule as a font entry. */
    static void addFonts (Document& document,
                          Document::Element*& fontsSection,
                          const jam::Array<Document::Token>& tokens,
                          const Rule& rule);

    /** @brief Appends one font-face rule as an Id::font child element of fontsSection -- id, filename, height, and kerning properties. */
    static void addFont (Document& document, Document::Element& fontsSection,
                         const jam::Array<Document::Token>& tokens,
                         const jam::Array<Declaration>& declarations);

    /**
     * @brief Serializes a token sequence back to CSS text.
     *
     * Accumulates through a std::string (amortized O(1) append) and converts to
     * juce::String exactly once at the return boundary.
     *
     * @param source The byte source the token spans index into.
     * @param tokens The full token stream.
     * @param indices The token sequence to serialize.
     * @return The serialized, trimmed CSS text.
     * @note A token type with neither a fixed code point nor a registered serializer contributes no text.
     */
    static juce::String toText (const std::string& source, const jam::Array<Document::Token>& tokens, const jam::Array<int>& indices);

    static constexpr int maximumEscapeHexDigits { 6 };

    // =========================================================================
    // css segmentation vocabulary
    // =========================================================================

    /** @brief CSS operator and comment-pair vocabulary handed to jam::Document for segmentation. */
    static const Document::Vocabulary& getCssVocabulary();

    // =========================================================================
    // 4.2 code point classification
    // =========================================================================

    /** @brief 4.2 non-ASCII code point. */
    static bool isNonAscii (juce::juce_wchar ch) noexcept { return ch >= Chars::nonAsciiStart; }

    /** @brief 4.2 ident-start code point. */
    static bool isIdentStartCodePoint (juce::juce_wchar ch) noexcept;

    /** @brief 4.2 ident code point. */
    static bool isIdentCodePoint (juce::juce_wchar ch) noexcept;

    /** @brief 4.2 non-printable code point. */
    static bool isNonPrintableCodePoint (juce::juce_wchar ch) noexcept;

    /** @brief 4.2 newline code point. */
    static bool isNewline (juce::juce_wchar ch) noexcept { return ch == Chars::newline; }

    // =========================================================================
    // 4.3.8 isValidEscape
    // =========================================================================

    /** @brief 4.3.8 isValidEscape. */
    static bool isValidEscape (juce::juce_wchar first, juce::juce_wchar second) noexcept;

    // =========================================================================
    // 4.3.9 wouldStartIdentSequence
    // =========================================================================

    /** @brief 4.3.9 wouldStartIdentSequence. */
    static bool wouldStartIdentSequence (juce::juce_wchar first,
                                         juce::juce_wchar second,
                                         juce::juce_wchar third) noexcept;

    // =========================================================================
    // 4.3.10 wouldStartNumber
    // =========================================================================

    /** @brief 4.3.10 wouldStartNumber. */
    static bool wouldStartNumber (juce::juce_wchar first,
                                  juce::juce_wchar second,
                                  juce::juce_wchar third) noexcept;

    // =========================================================================
    // 4.3.7 getEscapedCodePoint
    // =========================================================================

    /** @brief 4.3.7 consume an escaped code point, advancing @p cursor past it. */
    static juce::String getEscapedCodePoint (Cursor& cursor);

    // =========================================================================
    // getText -- consumption-time escape decoding over a raw content span
    // =========================================================================

    /** @brief Decodes @p span's raw bytes against @p source, expanding backslash escapes -- the consumption-time counterpart of the tokenizer's boundary-only scans. */
    static juce::String getText (const std::string& source, const Document::Token& span);

    // =========================================================================
    // 4.3.11 getIdentSequence
    // =========================================================================

    /** @brief 4.3.11 consume an ident sequence, advancing @p cursor to its end. */
    static void getIdentSequence (Cursor& cursor);

    // =========================================================================
    // 4.3.12 getNumber
    // =========================================================================

    /** @brief 4.3.12 consume a number's exponent part, advancing @p cursor past it when present. */
    static void getNumberExponent (Cursor& cursor);

    /** @brief 4.3.12 consume a number, advancing @p cursor past it. */
    static double getNumber (Cursor& cursor);

    // =========================================================================
    // 4.3.14 getEndOfBadUrl
    // =========================================================================

    /** @brief 4.3.14 consume the remnants of a bad url, advancing @p cursor past it. */
    static void getEndOfBadUrl (Cursor& cursor);

    // =========================================================================
    // 4.3.5 getStringToken
    // =========================================================================

    /** @brief 4.3.5 consume a string token, advancing @p cursor past it. */
    static Document::Token getStringToken (Cursor& cursor, juce::juce_wchar endingCodePoint);

    // =========================================================================
    // 4.3.6 getUrlToken
    // =========================================================================

    /** @brief 4.3.6 consume a url token's whitespace-then-close-or-badurl tail, advancing @p cursor past it. */
    static Document::Token getUrlTail (Cursor& cursor, size_t contentStart, size_t contentEnd);

    /** @brief 4.3.6 consume a url token's body from its first non-whitespace byte, advancing @p cursor past it. */
    static Document::Token getUrlBody (Cursor& cursor);

    /** @brief 4.3.6 consume a url token, advancing @p cursor past it. */
    static Document::Token getUrlToken (Cursor& cursor);

    // =========================================================================
    // 4.3.3 getNumericToken
    // =========================================================================

    /** @brief 4.3.3 consume a numeric token -- number, dimension, or percentage -- advancing @p cursor past it. */
    static Document::Token getNumericToken (Cursor& cursor);

    // =========================================================================
    // 4.3.4 getIdentLikeToken
    // =========================================================================

    /** @brief 4.3.4 peeks past whitespace after `url(` for a quote character, without consuming @p cursor. */
    static bool getUrlQuoted (Cursor cursor);

    /** @brief 4.3.4 consume an ident-like token -- ident, function, url, or bad-url -- advancing @p cursor past it. */
    static Document::Token getIdentLikeToken (Cursor& cursor);

    // =========================================================================
    // 4.3.1 getToken -- region-type classification (map::DocumentTokenType::region)
    // =========================================================================

    /** @brief 4.3.1 getToken -- region-type dispatch for Document-identified quoted spans. */
    static Document::Token getRegionToken (Cursor& cursor);

    // =========================================================================
    // 4.3.1 getToken -- operators hash classification (spec: "next is
    // ident code point or next two are valid escape")
    // =========================================================================

    /** @brief 4.3.1 getToken -- `#` classification into a hash token or delim. */
    static Document::Token getHashOperatorToken (Cursor& cursor, juce::juce_wchar);

    // =========================================================================
    // 4.3.1 getToken -- operators plus classification
    // =========================================================================

    /** @brief 4.3.1 getToken -- `+` classification into a numeric token or delim. */
    static Document::Token getPlusOperatorToken (Cursor& cursor, juce::juce_wchar ch);

    // =========================================================================
    // 4.3.1 getToken -- operators dash classification
    // =========================================================================

    /** @brief 4.3.1 getToken -- `-` classification into a numeric token, ident-like token, or delim. */
    static Document::Token getDashOperatorToken (Cursor& cursor, juce::juce_wchar ch);

    // =========================================================================
    // 4.3.1 getToken -- operators dot classification
    // =========================================================================

    /** @brief 4.3.1 getToken -- dot classification into a numeric token or delim. */
    static Document::Token getDotOperatorToken (Cursor& cursor, juce::juce_wchar ch);

    // =========================================================================
    // 4.3.1 getToken -- operators at-keyword classification
    // =========================================================================

    /** @brief 4.3.1 getToken -- `@` classification into an at-keyword token or delim. */
    static Document::Token getAtKeywordOperatorToken (Cursor& cursor, juce::juce_wchar);

    // =========================================================================
    // 4.3.1 getToken -- operators-type classification (map::DocumentTokenType::operators)
    // =========================================================================

    /** @brief 4.3.1 getToken -- operators-type dispatch (map::DocumentTokenType::operators) by leading code point. */
    static Document::Token getOperatorToken (Cursor& cursor);

    // =========================================================================
    // 4.3.1 getToken -- text-type classification (map::DocumentTokenType::text)
    // =========================================================================

    /** @brief 4.3.1 getToken -- text-type dispatch (map::DocumentTokenType::text) by leading code point. */
    static Document::Token getTextToken (Cursor& cursor);

    // =========================================================================
    // CSSOM 4.2 at-keyword name -> CssRuleType dispatch
    // =========================================================================

    /** @brief CSSOM 4.2 at-keyword name -> CssRuleType dispatch. */
    static int getAtRuleType (const juce::String& name);

    // =========================================================================
    // 5.4.1 getRules -- prelude classification
    // =========================================================================

    /** @brief 5.4.1 tests whether the token at position opens an at-rule prelude. */
    static bool isAtRuleToken (const jam::Array<Document::Token>& tokens, int position);

    /** @brief 5.4.1 reads the at-keyword name at position. */
    static juce::String
    getAtKeywordName (const std::string& source, const jam::Array<Document::Token>& tokens, int position);

    /** @brief 5.4.1 locates a rule's prelude end -- the first '{'/end-of-file, or ';' when isAtRule, from position to end. */
    static int getPreludeEnd (const jam::Array<Document::Token>& tokens, int end, int position, bool isAtRule);

    // =========================================================================
    // 5.4.8 consume a simple block -- boundary-finding
    // =========================================================================

    /** @brief 5.4.8 consume a simple block -- depth-tracked scan from blockStart to its matching close brace. */
    static int getRuleBlock (const jam::Array<Document::Token>& tokens, int blockStart, int end);

    // =========================================================================
    // 5.4.2 / 5.4.3 rule construction
    // =========================================================================

    /** @brief 5.4.2 / 5.4.3 construct a rule -- selectorText/type assignment, mediaRule recursion, getDeclarations. */
    static void addRule (const std::string& source,
                         jam::Array<Document::Token>& tokens,
                         int blockStart,
                         int blockEnd,
                         bool isAtRule,
                         const juce::String& atKeywordName,
                         const jam::Array<int>& preludeTokens,
                         jam::Array<Rule>& rules);

    /** @brief 5.4.2 / 5.4.3 constructs one comma-split selector segment's rule -- style is cloned field-by-field (jam::Array is move-only) so each segment owns its own declarations. */
    static void addSelectorRule (const std::string& source,
                                 const jam::Array<Document::Token>& tokens,
                                 const jam::Array<int>& segment,
                                 const jam::Array<Declaration>& declarations,
                                 int ruleType,
                                 jam::Array<Rule>& rules);

    /** @brief 5.4.2 / 5.4.3 comma-split selector list -- one rule per segment, each sharing the once-parsed declarations. */
    static void addSelectorRules (const std::string& source,
                                  const jam::Array<Document::Token>& tokens,
                                  const jam::Array<int>& preludeTokens,
                                  const jam::Array<Declaration>& declarations,
                                  int ruleType,
                                  jam::Array<Rule>& rules);

    // =========================================================================
    // 5.4.1 getRules
    // =========================================================================

    /** @brief 5.4.1 consume a list of rules. */
    static jam::Array<Rule> getRules (const std::string& source, jam::Array<Document::Token>& tokens, int start, int end);

    // =========================================================================
    // 5.4.5 getDeclarations
    // =========================================================================

    /** @brief 5.4.5 locates one declaration's run end -- the next semicolon or end. */
    static int getDeclarationEnd (const jam::Array<Document::Token>& tokens, int start, int end);

    /** @brief 5.4.5 consume a list of declarations. */
    static jam::Array<Declaration>
    getDeclarations (const std::string& source, jam::Array<Document::Token>& tokens, int start, int end);

    // =========================================================================
    // 5.4.6 getDeclaration
    // =========================================================================

    /** @brief 5.4.6 consume a declaration's value-token run from position to end. */
    static jam::Array<int> getDeclarationValue (const jam::Array<Document::Token>& tokens, int start, int end);

    /** @brief 5.4.4 locates a declaration's !important tail -- the index within value where the tail begins, or value.size() when absent. */
    static int getImportantPriorityIndex (const std::string& source, const jam::Array<Document::Token>& tokens, const jam::Array<int>& value);

    /** @brief 5.4.6 consume a declaration, including the !important tail (5.4.4). Marks the property token Id::declaration on a parse failure (missing colon or empty value), for CssValidator. */
    static Declaration getDeclaration (const std::string& source, jam::Array<Document::Token>& tokens, int start, int end);

    // =========================================================================
    // toText
    // =========================================================================

};

using Css = CssDocument;

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
