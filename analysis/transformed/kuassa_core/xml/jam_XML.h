#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct XmlDocument
 * @brief XML tokenizer/tree constructor over jam::Document, plus juce::XmlElement helpers.
 */
struct XmlDocument : Document
{
    /**
     * @brief Memoized layout Document for a lexicon-registered XML resource name.
     *
     * Built once per name and cached by Document::getOrCreate(): tokens are lexed,
     * then buildTree() drives the open-element stack to a root element adopted as
     * the returned Document's root. A failed parse leaves the returned Document's
     * root with an unset id -- callers test `document.root->id.isNull()` to detect
     * that case.
     *
     * @param name The lexicon-registered resource identifier.
     * @return The memoized Document for name, with an unset root id on parse failure.
     */
    static const XmlDocument& getOrCreate (const juce::Identifier& name)
    {
        auto* registry { SharedDocuments::getInstance() };
        jassert (registry != nullptr);

        if (registry->contains (name))
            return static_cast<const XmlDocument&> (*registry->at (name));

        using namespace BinaryData;
        Raw binary (name.toString());

        auto document { std::make_unique<XmlDocument>() };
        document->Document::parse (binary.data, binary.size);

        auto& ref { *document };
        registry->try_emplace (name, std::move (document));
        return ref;
    }

    /**
     * @brief Parses dynamic XML source text into a fresh Document.
     *
     * @param text The XML source text, owned by the caller.
     * @return The parsed Document, with an unset root id on parse failure.
     */
    static XmlDocument parse (const juce::String& text)
    {
        XmlDocument document;
        document.Document::parse (text.toRawUTF8(), static_cast<int> (text.getNumBytesAsUTF8()));
        return document;
    }

    /**
     * @brief Apply a function recursively to an XML element and all its children.
     *
     * @tparam Function A callable type taking (juce::XmlElement*).
     * @param xml       Pointer to the root XmlElement. Must not be nullptr.
     * @param function  Function to apply to each element.
     *
     * @note Traverses depth-first. Asserts if xml is nullptr.
     */
    template<typename Function>
    static void applyFunctionRecursively (juce::XmlElement* xml, const Function& function)
    {
        assert (xml != nullptr);

        function (xml);

        for (auto* e : xml->getChildIterator())
            applyFunctionRecursively (e, function);
    }

    /**
     * @brief Recursively converts a juce::XmlElement tree into a jam::Document,
     *        emitting exactly the shape Xml::parse() produces.
     *
     * @param xml The XmlElement to convert.
     * @return A Document whose root Element holds xml's tag on Element::id, every
     *         attribute (All-String policy), nested children as link-chain
     *         descendants, and Id::text (concatenated non-whitespace text-node
     *         content).
     */
    static XmlDocument toDocument (const juce::XmlElement& xml)
    {
        XmlDocument document;
        document.root->id = juce::Identifier { xml.getTagName() };

        populateXmlAttributes (*document.root, xml);
        populateXmlChildren (document, *document.root, xml);

        return document;
    }

    /**
     * @brief Find the first child element (recursively) with a matching attribute.
     *
     * @param xml            Pointer to the XmlElement to search. Must not be nullptr.
     * @param attributeName  Name of the attribute to match.
     * @param attributeValue Value to compare (case-insensitive).
     * @return Pointer to the matching XmlElement, or nullptr if not found.
     */
    static juce::XmlElement* getChildByAttribute (juce::XmlElement* xml,
                                                  juce::StringRef attributeName,
                                                  juce::StringRef attributeValue)
    {
        assert (xml != nullptr);

        if (xml->getStringAttribute (attributeName).equalsIgnoreCase (attributeValue))
            return xml;

        for (auto* e : xml->getChildIterator())
            if (auto* child { getChildByAttribute (e, attributeName, attributeValue) })
                return child;

        return nullptr;
    }

    /**
     * @brief Find a child element by its "id" attribute.
     *
     * @param xml        Pointer to the XmlElement to search.
     * @param idToLookFor Value of the id attribute to match.
     * @return Pointer to the matching XmlElement, or nullptr if not found.
     */
    static juce::XmlElement*
    getChildByID (const std::unique_ptr<juce::XmlElement>& xml, juce::StringRef idToLookFor)
    {
        return getChildByAttribute (xml.get(), Id::id, idToLookFor);
    }

    /**
     * @brief Retrieve a typed attribute value from a nested child of the Document's root.
     *
     * Looks up document.root->getChildByID(name)->getChildByID(childName), then
     * returns the stored value for property, typed exactly as ValueType.
     *
     * @tparam ValueType The stored attribute's type -- juce::String, as all
     *                    attributes are stored as text.
     * @param document   The Document to search.
     * @param name       Name of the parent element.
     * @param childName  Name of the child element.
     * @param property   Attribute name to retrieve.
     * @return The stored attribute value, or a default-constructed ValueType if not found.
     */
    template<typename ValueType>
    static ValueType get (const Document& document,
                          juce::StringRef name,
                          juce::StringRef childName,
                          juce::StringRef property) noexcept
    {
        const juce::Identifier nameKey { juce::String (name) };
        const juce::Identifier childKey { juce::String (childName) };
        const juce::Identifier propertyKey { juce::String (property) };

        if (const auto* child { document.root->getChildByID (nameKey) })
            if (const auto* grandchild { child->getChildByID (childKey) })
                if (grandchild->contains (propertyKey))
                    return *grandchild->get<ValueType> (propertyKey);

        return {};
    }

    /**
     * @brief Retrieve a typed attribute value from a direct child of the Document's root.
     *
     * @tparam ValueType The stored attribute's type -- juce::String, as all
     *                    attributes are stored as text.
     * @param document The Document to search.
     * @param name     Name of the child element.
     * @param property Attribute name to retrieve.
     * @return The stored attribute value, or a default-constructed ValueType if not found.
     */
    template<typename ValueType>
    static ValueType
    get (const Document& document, juce::StringRef name, juce::StringRef property) noexcept
    {
        const juce::Identifier nameKey { juce::String (name) };
        const juce::Identifier propertyKey { juce::String (property) };

        if (const auto* child { document.root->getChildByID (nameKey) })
            if (child->contains (propertyKey))
                return *child->get<ValueType> (propertyKey);

        return {};
    }

    //==============================================================================
    /**
     * @brief Load an XML element from a JUCE BinaryData resource.
     *
     * @param resourceFileName Name of the resource file in BinaryData.
     * @return A std::unique_ptr<juce::XmlElement> parsed from the resource.
     *
     * @note Uses BinaryData::Raw to access the embedded resource.
     */
    static auto getFromBinary (const juce::String& resourceFileName)
    {
        /** fallback if BinaryData namespace use as default to call
         getNamedResourceOriginalFilename (const char*) */
        using namespace BinaryData;
        /*________________________________________________________________________*/

        Raw binary (resourceFileName);

        return juce::parseXML (juce::String::createStringFromData (binary.data, binary.size));
    }

protected:
    /** @brief XML segmentation vocabulary handed to jam::Document. */
    const Vocabulary& getVocabulary() const override { return getXmlVocabulary(); }

    /** @brief Lexes one token at cursor -- comment span, markup-operator dispatch to getEndTag()/getDeclaration()/getPrologue()/getStartTag(), text otherwise -- and appends it, returning its consumed length. */
    int getToken (Cursor& cursor, int segmentType) override
    {
        if (segmentType == map::DocumentTokenType::comment)
        {
            const auto start { cursor.getOffset() };
            Cursor commentCursor { cursor };

            while (not commentCursor.isEmpty())
            {
                if (commentCursor.startsWith (Chars::doubleDashChevronRight))
                {
                    commentCursor += commentCloseLength;
                    break;
                }

                ++commentCursor;
            }

            Document::Token token { start, commentCursor.getOffset(), map::XmlTokenType::comment };
            const auto length { static_cast<int> (token.length()) };
            tokens.add (std::move (token));
            return length;
        }

        if (segmentType == map::DocumentTokenType::operators and *cursor == Chars::lessThan)
            if (auto markupToken { getMarkupToken (Cursor { cursor }, *this) })
            {
                const auto length { static_cast<int> (markupToken->length()) };
                tokens.add (std::move (*markupToken));
                return length;
            }

        Document::Token token { getText (Cursor { cursor }) };
        const auto length { static_cast<int> (token.length()) };
        tokens.add (std::move (token));
        return length;
    }

    /**
     * @brief Tree construction entry point -- drives the open-element stack over the
     *        token stream, building a detached Document and adopting its element
     *        as this Document's root only on full success (the root keeps its
     *        unset id on any failure).
     */
    void build() override
    {
        static const auto treeConstruction {
            []
            {
                jam::Function::Map<int, bool> treeConstruction;
                treeConstruction.add<const Document::Token&, jam::Array<Document::Element*>&, Document&, const Document&> (map::XmlTokenType::startTag, &addStartTagElement);
                treeConstruction.add<const Document::Token&, jam::Array<Document::Element*>&, Document&, const Document&> (map::XmlTokenType::endTag, &addEndTagElement);
                treeConstruction.add<const Document::Token&, jam::Array<Document::Element*>&, Document&, const Document&> (map::XmlTokenType::text, &addText);
                treeConstruction.add<const Document::Token&, jam::Array<Document::Element*>&, Document&, const Document&> (map::XmlTokenType::processingInstruction, &skipToken);
                treeConstruction.add<const Document::Token&, jam::Array<Document::Element*>&, Document&, const Document&> (map::XmlTokenType::declaration, &skipToken);
                treeConstruction.add<const Document::Token&, jam::Array<Document::Element*>&, Document&, const Document&> (map::XmlTokenType::comment, &skipToken);
                return treeConstruction;
            }()
        };

        XmlDocument tree;
        jam::Array<Document::Element*> stack;

        for (const auto& token : tokens)
            if (not treeConstruction.get (token.type, token, stack, static_cast<Document&> (tree), static_cast<const Document&> (*this)))
                return;

        if (stack.isEmpty() and not tree.root->id.isNull())
        {
            root = appendChild (*root, std::move (tree));
            root->parent = nullptr;
            return;
        }

#if JUCE_DEBUG
        debug::Log::write ("build: incomplete document -- unclosed elements or no root element");
#endif
    }

private:
    /** @brief Transient parse-time token accumulator, populated by getToken() and consumed by build(). */
    jam::Array<Document::Token> tokens;

    /** @brief Cached UTF-8 lengths of the Tokens markup markers, spared re-measuring per token. */
    static constexpr int endTagOpenLength {
        static_cast<int> (std::string_view (Chars::endTagOpen).size())
    };
    static constexpr int declarationOpenLength {
        static_cast<int> (std::string_view (Chars::declarationOpen).size())
    };
    static constexpr int prologOpenLength {
        static_cast<int> (std::string_view (Chars::processingOpen).size())
    };
    static constexpr int selfCloseTagLength {
        static_cast<int> (std::string_view (Chars::selfCloseTag).size())
    };
    static constexpr int processingCloseLength {
        static_cast<int> (std::string_view (Chars::processingClose).size())
    };
    static constexpr int commentCloseLength {
        static_cast<int> (std::string_view (Chars::doubleDashChevronRight).size())
    };

    /** @brief Property keys reserved for the constructed-Document shape -- a colliding source attribute is a parse failure. */
    static const jam::Array<juce::Identifier>& getStructuralKeys()
    {
        static const jam::Array<juce::Identifier> keys { Id::text, Id::root, Id::endTag };
        return keys;
    }

    // =========================================================================
    // xml tokenizer segmentation vocabulary (jam::Document)
    // =========================================================================

    /** @brief XML operator vocabulary handed to jam::Document for segmentation. */
    static const Document::Vocabulary& getXmlVocabulary()
    {
        static const Document::Vocabulary vocabulary { map::markupLanguage,
                                                       Chars::markupCommentOpen,
                                                       Chars::doubleDashChevronRight };

        return vocabulary;
    }

    // =========================================================================
    // getText
    // =========================================================================

    /** @brief Data state -- consumes character data up to the next tag, verbatim. */
    static Document::Token getText (Cursor cursor)
    {
        const auto start { cursor.getOffset() };

        if (not cursor.isEmpty() and *cursor == Chars::lessThan)
        {
#if JUCE_DEBUG
            debug::Log::write ("getText: cursor positioned at <");
#endif
            ++cursor;
        }

        while (not cursor.isEmpty() and *cursor != Chars::lessThan)
            ++cursor;

        return Document::Token { start, cursor.getOffset(), map::XmlTokenType::text };
    }

    // =========================================================================
    // getTagName
    // =========================================================================

    /** @brief Tag name state -- scans a start tag's name characters and returns the resulting span. */
    static Document::Token getTagName (Cursor cursor)
    {
        const auto nameStart { cursor.getOffset() };

        while (not cursor.isEmpty() and not Document::isWhitespace (*cursor)
               and *cursor != Chars::slash and *cursor != Chars::greaterThan)
            ++cursor;

        return Document::Token { nameStart, cursor.getOffset() };
    }

    // =========================================================================
    // getOrCreateAttributes
    // =========================================================================

    /** @brief Returns @p token's attributes property array, creating it empty on first use. */
    static jam::Array<Document::Property<Document::Token>>*
    getOrCreateAttributes (Document::Token& token)
    {
        using Attributes = jam::Array<Document::Property<Document::Token>>;

        if (not token.contains (Id::attributes))
            token.add<Attributes> (Id::attributes);

        return token.get<Attributes> (Id::attributes);
    }

    // =========================================================================
    // addAttribute
    // =========================================================================

    /** @brief Attribute name state -- scans up to `=`, whitespace, `>`, or `/`, returning the name text. */
    static juce::String getAttributeName (Cursor& cursor, const Document& document)
    {
        const auto attributeNameStart { cursor.getOffset() };

        while (not cursor.isEmpty() and *cursor != Chars::equals
               and not Document::isWhitespace (*cursor) and *cursor != Chars::greaterThan
               and *cursor != Chars::slash)
            ++cursor;

        return document.getText (Document::Token { attributeNameStart, cursor.getOffset() });
    }

    /** @brief Attribute value state -- scans to the closing quote, logging and returning the partial span on unterminated EOF. */
    static Document::Token getAttributeValue (Cursor& cursor,
                                              juce::juce_wchar quoteCharacter,
                                              const Document::Token& token,
                                              const Document& document)
    {
        const auto valueStart { cursor.getOffset() };

        while (not cursor.isEmpty() and *cursor != quoteCharacter)
            ++cursor;

        const auto valueEnd { cursor.getOffset() };

        if (cursor.isEmpty())
        {
#if JUCE_DEBUG
            debug::Log::write ("addAttribute: unexpected EOF in attribute value on <"
                               + document.getText (*token.get<Document::Token> (Id::name))
                               + ">");
#endif
            return Document::Token { valueStart, valueEnd };
        }

        ++cursor;
        return Document::Token { valueStart, valueEnd };
    }

    /** @brief Quoted-value state -- skips whitespace, requires a double-quote character, then delegates to getAttributeValue(). */
    static Document::Token
    getQuotedValue (Cursor& cursor, const Document::Token& token, const Document& document)
    {
        while (not cursor.isEmpty() and Document::isWhitespace (*cursor))
            ++cursor;

        if (cursor.isEmpty() or (*cursor != Chars::doubleQuote and *cursor != Chars::singleQuote))
        {
#if JUCE_DEBUG
            debug::Log::write ("addAttribute: expected a quoted value on <"
                               + document.getText (*token.get<Document::Token> (Id::name))
                               + ">");
#endif
            cursor += static_cast<int> (cursor.source.size() - cursor.getOffset());
            return Document::Token { cursor.getOffset(), cursor.getOffset() };
        }

        const auto quoteCharacter { *cursor };
        ++cursor;

        return getAttributeValue (cursor, quoteCharacter, token, document);
    }

    /**
     * @brief Attribute cycle -- name, `=`, quoted value, appended to token's attributes property.
     *
     * @param cursor The cursor positioned at the start of the attribute name, advanced past it.
     * @param token The token being built; the parsed attribute is appended to its attributes property.
     * @param document The Document whose source backs @p cursor, used to decode diagnostic spans.
     */
    static void addAttribute (Cursor& cursor, Document::Token& token, const Document& document)
    {
        const auto attributeName { getAttributeName (cursor, document) };

        while (not cursor.isEmpty() and Document::isWhitespace (*cursor))
            ++cursor;

        if (cursor.isEmpty() or *cursor != Chars::equals)
        {
#if JUCE_DEBUG
            debug::Log::write ("addAttribute: expected \"=\" after attribute name on <"
                               + document.getText (*token.get<Document::Token> (Id::name))
                               + ">");
#endif
            if (attributeName.isNotEmpty())
                getOrCreateAttributes (token)->add ({
                    juce::Identifier { attributeName },
                     Document::Token { cursor.getOffset(), cursor.getOffset() }
                });

            return;
        }

        ++cursor;

        auto value { getQuotedValue (cursor, token, document) };

        if (attributeName.isNotEmpty())
            getOrCreateAttributes (token)->add ({ juce::Identifier { attributeName }, std::move (value) });
    }

    // =========================================================================
    // getStartTag
    // =========================================================================

    /** @brief Attribute list state -- scans attribute cycles until `>`, self-close, or EOF, mutating cursor and token in place. */
    static void getTagAttributes (Cursor& cursor, Document::Token& token, const Document& document)
    {
        for (;;)
        {
            while (not cursor.isEmpty() and Document::isWhitespace (*cursor))
                ++cursor;

            if (cursor.isEmpty())
            {
#if JUCE_DEBUG
                debug::Log::write (
                    "getStartTag: unexpected EOF before attribute name on <"
                    + document.getText (*token.get<Document::Token> (Id::name))
                    + ">");
#endif
                return;
            }

            if (cursor.startsWith (Chars::selfCloseTag))
            {
                cursor += selfCloseTagLength;
                token.add<bool> (Id::selfClosing, true);
                return;
            }

            if (*cursor == Chars::greaterThan)
            {
                ++cursor;
                return;
            }

            addAttribute (cursor, token, document);
        }
    }

    /** @brief Tag open state -- orchestrates a start tag's name, attribute list, and self-closing marker. */
    static Document::Token getStartTag (Cursor cursor, const Document& document)
    {
        const auto start { cursor.getOffset() };
        ++cursor;

        Document::Token token;
        token.type = map::XmlTokenType::startTag;

        auto nameToken { getTagName (Cursor { cursor }) };
        cursor += static_cast<int> (nameToken.length());
        token.add<Document::Token> (Id::name, std::move (nameToken));

        getTagAttributes (cursor, token, document);

        token.setSpan (start, cursor.getOffset());
        return token;
    }

    // =========================================================================
    // getEndTag
    // =========================================================================

    /** @brief End tag open state -- consumes an end tag's name and discards to `>`, returning the end tag Token. */
    static Document::Token getEndTag (Cursor cursor)
    {
        const auto start { cursor.getOffset() };
        cursor += endTagOpenLength;

        Document::Token token;
        token.type = map::XmlTokenType::endTag;

        const auto nameStart { cursor.getOffset() };

        while (not cursor.isEmpty() and not Document::isWhitespace (*cursor)
               and *cursor != Chars::greaterThan)
            ++cursor;

        token.add<Document::Token> (Id::name, Document::Token { nameStart, cursor.getOffset() });

        while (not cursor.isEmpty() and *cursor != Chars::greaterThan)
            ++cursor;

        if (not cursor.isEmpty())
        {
            ++cursor;
        }
#if JUCE_DEBUG
        else
        {
            debug::Log::write ("getEndTag: unexpected EOF before >");
        }
#endif

        token.setSpan (start, cursor.getOffset());
        return token;
    }

    // =========================================================================
    // getDeclaration
    // =========================================================================

    /** @brief Declaration state -- discards a Chars::declarationOpen markup declaration (covers DOCTYPE) to Chars::greaterThan; returns the declaration's span as a map::XmlTokenType::declaration Token. */
    static Document::Token getDeclaration (Cursor cursor)
    {
        const auto start { cursor.getOffset() };
        cursor += declarationOpenLength;

        while (not cursor.isEmpty() and *cursor != Chars::greaterThan)
            ++cursor;

        if (not cursor.isEmpty())
        {
            ++cursor;
        }
#if JUCE_DEBUG
        else
        {
            debug::Log::write ("getDeclaration: unexpected EOF in declaration");
        }
#endif

        return Document::Token { start, cursor.getOffset(), map::XmlTokenType::declaration };
    }

    // =========================================================================
    // getPrologue
    // =========================================================================

    /** @brief Prologue state -- discards a Chars::processingOpen processing instruction to Chars::processingClose; returns the instruction's span as a map::XmlTokenType::processingInstruction Token. */
    static Document::Token getPrologue (Cursor cursor)
    {
        const auto start { cursor.getOffset() };
        cursor += prologOpenLength;

        while (not cursor.isEmpty())
        {
            if (cursor.startsWith (Chars::processingClose))
            {
                cursor += processingCloseLength;
                break;
            }

            ++cursor;
        }

#if JUCE_DEBUG
        if (cursor.isEmpty())
            debug::Log::write ("getPrologue: unexpected EOF in prologue");
#endif

        return Document::Token { start, cursor.getOffset(), map::XmlTokenType::processingInstruction };
    }

    // =========================================================================
    // getMarkupToken
    // =========================================================================

    /** @brief Operator-prefix dispatch -- routes a markup-operator cursor to getEndTag()/getDeclaration()/getPrologue() by literal prefix, or to getStartTag() on letter lookahead; empty on neither match. */
    static std::optional<Document::Token> getMarkupToken (Cursor cursor, const Document& document)
    {
        static const auto markupOperators {
            []
            {
                jam::Function::Map<juce::String, Document::Token> markupOperators;
                markupOperators.add<Cursor> (Chars::endTagOpen, &getEndTag);
                markupOperators.add<Cursor> (Chars::declarationOpen, &getDeclaration);
                markupOperators.add<Cursor> (Chars::processingOpen, &getPrologue);
                return markupOperators;
            }()
        };

        for (const auto& [operatorPrefix, operatorFunction] : markupOperators)
            if (cursor.startsWith (std::string_view (
                    operatorPrefix.toRawUTF8(), static_cast<size_t> (operatorPrefix.getNumBytesAsUTF8()))))
                return markupOperators.get (operatorPrefix, Cursor { cursor });

        auto probe { cursor };
        ++probe;

        if (not probe.isEmpty() and Document::isLetter (*probe))
            return getStartTag (Cursor { cursor }, document);

        return std::nullopt;
    }

    /** @brief Copies a juce::XmlElement's attributes onto element, all-String attributes. */
    static void populateXmlAttributes (Document::Element& element, const juce::XmlElement& xml)
    {
        for (int index { 0 }; index < xml.getNumAttributes(); ++index)
        {
            const juce::Identifier attributeKey { xml.getAttributeName (index) };

            assert (not element.contains (attributeKey)
                    and not getStructuralKeys().contains (attributeKey));

            element.add<juce::String> (attributeKey, xml.getAttributeValue (index));
        }
    }

    /** @brief Recursively copies a juce::XmlElement's children onto element -- concatenated non-whitespace text under Id::text, nested elements as link-chain descendants. */
    static void
    populateXmlChildren (Document& document, Document::Element& element, const juce::XmlElement& xml)
    {
        for (auto* child : xml.getChildIterator())
        {
            if (child->isTextElement())
            {
                const auto text { child->getText().trim() };

                if (text.isEmpty())
                    continue;

                if (element.contains (Id::text))
                    *element.get<juce::String> (Id::text) += text;
                else
                    element.add<juce::String> (Id::text, text);

                continue;
            }

            auto* childElement { document.addChild (
                element, juce::Identifier { child->getTagName() }) };

            populateXmlAttributes (*childElement, *child);
            populateXmlChildren (document, *childElement, *child);
        }
    }

    /** @brief Copies a start tag's buffered attribute pairs onto the constructed element, all typed as juce::String. */
    static bool addAttributes (const Document& document,
                               Document::Element& element,
                               const Document::Token& token)
    {
        if (token.contains (Id::attributes))
            for (const auto& [attributeKey, attributeValue] :
                 *token.get<jam::Array<Document::Property<Document::Token>>> (Id::attributes))
            {
                if (element.contains (attributeKey) or getStructuralKeys().contains (attributeKey))
                {
#if JUCE_DEBUG
                    debug::Log::write ("addAttributes: colliding attribute \"" + attributeKey.toString()
                                       + "\" on element <"
                                       + document.getText (*token.get<Document::Token> (Id::name))
                                       + ">");
#endif
                    return false;
                }

                element.add<juce::String> (attributeKey, document.getText (attributeValue));
            }

        return true;
    }

    /** @brief Adds non-whitespace character data as the text content of the current open element. */
    static bool addText (const Document::Token& token,
                         jam::Array<Document::Element*>& stack,
                         Document&,
                         const Document& document)
    {
        const auto text { document.getText (token) };

        if (not text.trim().isEmpty())
        {
            if (stack.isEmpty())
            {
#if JUCE_DEBUG
                debug::Log::write ("addText: non-whitespace text before root element");
#endif
                return false;
            }

            auto& element { *stack.last() };

            if (element.contains (Id::text))
                *element.get<juce::String> (Id::text) += text;
            else
                element.add<juce::String> (Id::text, text);
        }

        return true;
    }

    static bool skipToken (const Document::Token&, jam::Array<Document::Element*>&, Document&, const Document&)
    {
        return true;
    }

    /**
     * @brief Links a new element under the currently open element, or -- when the open-element
     *        stack is empty -- claims tree's own root as the document's single top-level
     *        element by assigning its id in place.
     *
     * A second stack-empty call after the top-level element already has an id is a multiple-
     * root-elements condition: the offending element is still attached, as a child of the
     * existing root, and the root is marked Id::root once for XmlValidator to report.
     *
     * @param tree The scratch Document being built.
     * @param stack The open-element stack, top-of-stack is the current parent.
     * @param tagId The tag identifier of the element being opened.
     * @return The linked element.
     */
    static Document::Element*
    attachElement (Document& tree, jam::Array<Document::Element*>& stack, const juce::Identifier& tagId)
    {
        if (not stack.isEmpty())
            return tree.addChild (*stack.last(), tagId);

        if (not tree.root->id.isNull())
        {
            if (not tree.root->contains (Id::root))
                tree.root->add<bool> (Id::root, true);

            return tree.addChild (*tree.root, tagId);
        }

        tree.root->id = tagId;
        return tree.root;
    }

    /** @brief Start-tag arm -- resolves the tag element (root or child), copies attributes, and pushes it onto the open-element stack unless self-closing. */
    static bool addStartTagElement (const Document::Token& token,
                                    jam::Array<Document::Element*>& stack,
                                    Document& tree,
                                    const Document& document)
    {
        const juce::Identifier tagId { document.getText (*token.get<Document::Token> (Id::name)) };

        auto* element { attachElement (tree, stack, tagId) };

        if (not addAttributes (document, *element, token))
            return false;

        if (not token.contains (Id::selfClosing))
            stack.add (element);

        return true;
    }

    /** @brief End-tag arm -- pops the open-element stack; a tag-name mismatch marks the still-open element Id::endTag once for XmlValidator to report, an empty stack is a no-op. */
    static bool addEndTagElement (const Document::Token& token,
                                  jam::Array<Document::Element*>& stack,
                                  Document&,
                                  const Document& document)
    {
        const juce::Identifier endTagId { document.getText (*token.get<Document::Token> (Id::name)) };

        if (not stack.isEmpty())
        {
            if (stack.last()->id != endTagId and not stack.last()->contains (Id::endTag))
                stack.last()->add<bool> (Id::endTag, true);

            stack.remove (stack.size() - 1);
        }

        return true;
    }
};

using Xml = XmlDocument;

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
