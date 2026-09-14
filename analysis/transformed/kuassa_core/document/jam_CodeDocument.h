/**
 * @file jam_CodeDocument.h
 * @brief Fenced code-block tokenizer over jam::Document — one lexer, four
 *        language families selected by the fence's info string.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct CodeDocument
 * @brief A Document whose tokenizer classifies one fenced code block, the
 *        language selected by the fence's info string.
 *
 * CodeDocument is the code-view tokenizer: it parses one code block into
 * `map::SyntaxTokenType`-classed tokens (comment, string, punctuation,
 * number, keyword), selecting the byte-classification Vocabulary and keyword
 * set per language family. The parsed tokens are stored on the root element
 * under `Id::tokens`. Multiple grammars under one Document are the solved
 * case the per-family Vocabulary selection below implements.
 */
struct CodeDocument : Document
{
    /**
     * @brief Parses @p codeText as the language named by @p infoString.
     *
     * Resolves @p infoString's first space-delimited word to a language
     * family; an unrecognized or empty info string yields an empty document.
     * On success the returned document carries its family and the tokenized
     * block (tokens stamped on the root under `Id::tokens`).
     *
     * @param codeText   The fenced code block's source text.
     * @param infoString The fence's info string, e.g. "cpp" or "js title".
     * @return The parsed CodeDocument, or an empty document when the info
     *         string names no known language family.
     */
    static CodeDocument parse (const juce::String& codeText, const juce::String& infoString)
    {
        const auto family { getLanguageFamily (infoString) };

        if (family != invalidFamily)
        {
            CodeDocument document;
            document.family = family;
            document.Document::parse (codeText.toRawUTF8(),
                                      static_cast<int> (codeText.getNumBytesAsUTF8()));
            return document;
        }

        return {};
    }

protected:

    /** @brief Returns the byte-classification vocabulary for this document's language family. */
    const Vocabulary& getVocabulary() const override
    {
        return getVocabularyForFamily (family);
    }

    /**
     * @brief Lexes one token at @p cursor, dispatching by @p segmentType.
     *
     * The domain lexer seat of jam::Document. Comment, region (quoted
     * string), and operator segments dispatch through a `Function::Map` to
     * their scanners; otherwise the character class selects whitespace,
     * number, identifier, or a single-byte advance. Each scanner appends its
     * token to the document's token accumulator and returns the consumed
     * byte length.
     *
     * @param cursor      The cursor positioned at the token's start, advanced past it.
     * @param segmentType The classification of the segment @p cursor sits within
     *                    (map::DocumentTokenType).
     * @return The token's length in bytes.
     */
    int getToken (Cursor& cursor, int segmentType) override
    {
        static const auto segments {
            []
            {
                jam::Function::Map<int, Document::Token> segments;

                segments.add<Cursor&, const Vocabulary&> (map::DocumentTokenType::comment,
                    [] (Cursor& cursor, const Vocabulary& vocabulary) { return addCommentToken (cursor, vocabulary); });

                segments.add<Cursor&, const Vocabulary&> (map::DocumentTokenType::region,
                    [] (Cursor& cursor, const Vocabulary&) { return addStringToken (cursor); });

                segments.add<Cursor&, const Vocabulary&> (map::DocumentTokenType::operators,
                    [] (Cursor& cursor, const Vocabulary& vocabulary) { return addPunctuationToken (cursor, vocabulary); });

                return segments;
            }()
        };

        const auto& vocabulary { getVocabularyForFamily (family) };
        Cursor walkCursor { cursor.source, cursor.getOffset() };

        if (segments.contains (segmentType))
        {
            const auto start { walkCursor.getOffset() };
            auto token { segments.get (segmentType, walkCursor, vocabulary) };
            tokens.add (std::move (token));
            return static_cast<int> (walkCursor.getOffset() - start);
        }

        const auto ch { *cursor };

        if (Document::isWhitespace (ch))
            return getWhitespaceLength (walkCursor);

        if (Document::isDigit (ch))
            return addNumberToken (walkCursor);

        if (Document::isLetter (ch) or ch == Chars::underscore)
            return addIdentifierToken (walkCursor);

        return singleByteAdvance;
    }

    /** @brief Stores the accumulated token stream on the root element under `Id::tokens`. */
    void build() override
    {
        root->add<jam::Array<Document::Token>> (Id::tokens, std::move (tokens));
    }

private:

    /** Sentinel family ordinal returned when an info string names no known language family. */
    static constexpr int invalidFamily { -1 };
    /** Cursor advance, in bytes, for a byte that starts no token of its own. */
    static constexpr int singleByteAdvance { 1 };

    jam::Array<Document::Token> tokens;
    int family { invalidFamily };

    /**
     * @brief Consumes a comment span starting at position, returning it as a comment token.
     *
     * @param cursor     The cursor positioned at the comment's opening delimiter, advanced past the comment.
     * @param vocabulary The language family's comment open/close literal sequences.
     * @return The comment token spanning the consumed bytes.
     */
    static Document::Token addCommentToken (Cursor& cursor, const Vocabulary& vocabulary)
    {
        const auto startOffset { cursor.getOffset() };
        cursor += static_cast<int> (vocabulary.commentOpen.size());

        while (not cursor.isEmpty() and not cursor.startsWith (vocabulary.commentClose))
            ++cursor;

        if (not cursor.isEmpty())
            cursor += static_cast<int> (vocabulary.commentClose.size());

        return { startOffset, cursor.getOffset(), map::SyntaxTokenType::comment };
    }

    /**
     * @brief Consumes a quoted string literal starting at position, returning it as a string token.
     *
     * The quote character at position sets the closing delimiter; the scan
     * advances to the next occurrence of that same character, or to the end
     * of source when none closes it.
     *
     * @param cursor The cursor positioned at the opening quote character, advanced past the string literal.
     * @return The string token spanning the consumed bytes.
     */
    static Document::Token addStringToken (Cursor& cursor)
    {
        const auto startOffset { cursor.getOffset() };
        const auto quote { *cursor };
        ++cursor;

        while (not cursor.isEmpty() and *cursor != quote)
            ++cursor;

        if (not cursor.isEmpty())
            ++cursor;

        return { startOffset, cursor.getOffset(), map::SyntaxTokenType::string };
    }

    /**
     * @brief Consumes a run of consecutive operator-class bytes starting at position, returning it as a punctuation token.
     *
     * @param cursor     The cursor positioned at the run's first byte, advanced past the run.
     * @param vocabulary The language family's byte classification, used to test each byte's class.
     * @return The punctuation token spanning the consumed bytes.
     */
    static Document::Token addPunctuationToken (Cursor& cursor, const Vocabulary& vocabulary)
    {
        const auto startOffset { cursor.getOffset() };

        while (not cursor.isEmpty() and vocabulary.getClass (*cursor) == map::Byte::operators)
            ++cursor;

        return { startOffset, cursor.getOffset(), map::SyntaxTokenType::punctuation };
    }

    /**
     * @brief The length of a run of consecutive whitespace bytes starting at position.
     *
     * @param cursor The cursor positioned at the run's first byte, advanced past the run.
     * @return The number of consecutive whitespace bytes starting at cursor.
     */
    static int getWhitespaceLength (Cursor& cursor)
    {
        const auto startOffset { cursor.getOffset() };

        while (not cursor.isEmpty() and Document::isWhitespace (*cursor))
            ++cursor;

        return static_cast<int> (cursor.getOffset() - startOffset);
    }

    /**
     * @brief Consumes a numeric literal starting at position, recording it as a number token.
     *
     * A leading zero followed by 'x' or 'X' consumes a run of hex digits;
     * otherwise the scan consumes digits, a decimal point, exponent letters
     * ('e'/'E'), the float suffix 'f', and underscore digit separators.
     *
     * @param cursor The cursor positioned at the literal's first digit, advanced past the literal.
     * @return The number of bytes consumed by the numeric literal.
     */
    int addNumberToken (Cursor& cursor)
    {
        const auto startOffset { cursor.getOffset() };
        const auto ch { *cursor };
        ++cursor;

        if (ch == Chars::zero and not cursor.isEmpty()
            and (*cursor == Chars::lowerX or *cursor == Chars::upperX))
        {
            ++cursor;

            while (not cursor.isEmpty() and Document::isHexDigit (*cursor))
                ++cursor;
        }
        else
        {
            while (not cursor.isEmpty()
                   and (Document::isDigit (*cursor) or *cursor == Chars::dot
                        or *cursor == Chars::lowerE or *cursor == Chars::upperE
                        or *cursor == Chars::lowerF or *cursor == Chars::underscore))
                ++cursor;
        }

        tokens.add ({ startOffset, cursor.getOffset(), map::SyntaxTokenType::number });
        return static_cast<int> (cursor.getOffset() - startOffset);
    }

    /**
     * @brief Consumes an identifier starting at position, recording it as a keyword token when it matches the current language family's keyword set.
     *
     * The scan consumes letters, digits, and underscores; the resulting
     * identifier is looked up in family's keyword set, and a token is
     * recorded only when it matches -- a non-keyword identifier consumes
     * bytes with no token added.
     *
     * @param cursor The cursor positioned at the identifier's first character, advanced past it.
     * @return The number of bytes consumed by the identifier.
     */
    int addIdentifierToken (Cursor& cursor)
    {
        const auto startOffset { cursor.getOffset() };

        while (not cursor.isEmpty()
               and (Document::isLetter (*cursor) or Document::isDigit (*cursor)
                    or *cursor == Chars::underscore))
            ++cursor;

        const auto identifier { juce::String::fromUTF8 (cursor.source.data() + startOffset,
                                                         static_cast<int> (cursor.getOffset() - startOffset)) };

        if (getKeywordsForFamily (family).contains (identifier))
            tokens.add ({ startOffset, cursor.getOffset(), map::SyntaxTokenType::keyword });

        return static_cast<int> (cursor.getOffset() - startOffset);
    }

    /**
     * @brief The byte-classification vocabulary shared by every C-family language, block-commented as CSS is.
     *
     * @return The C-family vocabulary, built once.
     */
    static const Document::Vocabulary& getCLangVocabulary()
    {
        static const Document::Vocabulary vocabulary { map::code,
            Chars::cssCommentOpen, Chars::cssCommentClose };
        return vocabulary;
    }

    /**
     * @brief The byte-classification vocabulary shared by Python and shell, carrying no block-comment markers.
     *
     * @return The Python/shell vocabulary, built once.
     */
    static const Document::Vocabulary& getPythonVocabulary()
    {
        static const Document::Vocabulary vocabulary { map::code, {}, {} };
        return vocabulary;
    }

    /** @brief Markup-language byte-classification vocabulary, comment-delimited with \<!-- and --\>. */
    static const Document::Vocabulary& getMarkupVocabulary()
    {
        static const Document::Vocabulary vocabulary { map::markup,
            Chars::markupCommentOpen, Chars::doubleDashChevronRight };
        return vocabulary;
    }

    /** @brief Data-language (JSON/YAML) byte-classification vocabulary, carrying no comment markers. */
    static const Document::Vocabulary& getDataVocabulary()
    {
        static const Document::Vocabulary vocabulary { map::data, {}, {} };
        return vocabulary;
    }

    /**
     * @brief Lookup from a language family ordinal (map::Family) to its byte-classification vocabulary.
     *
     * Built once. Python and shell share getPythonVocabulary(); every other
     * listed family gets its own vocabulary function.
     *
     * @return The family-to-vocabulary lookup.
     */
    static const jam::Function::Map<int, const Document::Vocabulary&>& getVocabularyByFamily()
    {
        static const auto vocabularyByFamily {
            []
            {
                jam::Function::Map<int, const Document::Vocabulary&> vocabularyByFamily;

                vocabularyByFamily.add<> (map::Family::cLang, getCLangVocabulary);
                vocabularyByFamily.add<> (map::Family::python, getPythonVocabulary);
                vocabularyByFamily.add<> (map::Family::shell, getPythonVocabulary);
                vocabularyByFamily.add<> (map::Family::markup, getMarkupVocabulary);
                vocabularyByFamily.add<> (map::Family::data, getDataVocabulary);
                vocabularyByFamily.add<> (map::Family::js, getCLangVocabulary);

                return vocabularyByFamily;
            }()
        };

        return vocabularyByFamily;
    }

    /**
     * @brief Resolves family's byte-classification vocabulary, falling back to the C-family vocabulary when family is unlisted.
     *
     * @param family The language family ordinal (map::Family), or invalidFamily.
     * @return family's vocabulary, or getCLangVocabulary() when family carries no entry.
     */
    static const Document::Vocabulary& getVocabularyForFamily (int family)
    {
        const auto& vocabularyByFamily { getVocabularyByFamily() };

        return vocabularyByFamily.contains (family)
                   ? vocabularyByFamily.get (family)
                   : getCLangVocabulary();
    }

    /**
     * @brief Returns the keyword set for @p family.
     *
     * Python and JavaScript map to their own keyword tables; every other
     * family falls back to the C++ keyword set.
     *
     * @param family The language family ordinal (map::Family).
     * @return The keyword bimap consulted when classifying an identifier.
     */
    static const jam::Bimap<int>& getKeywordsForFamily (int family)
    {
        switch (family)
        {
            case map::Family::python:     return *map::PythonKeyword::getInstance();
            case map::Family::js:         return *map::JsKeyword::getInstance();
            default:                      return *map::CppKeyword::getInstance();
        }
    }

    /**
     * @brief A fenced code-block info string's first space-delimited word, lowercased.
     *
     * @param infoString The fence's info string, e.g. "cpp" or "js title".
     * @return infoString's first word, lowercased, or an empty string when infoString is empty.
     */
    static juce::String getFirstWord (const juce::String& infoString)
    {
        return infoString.upToFirstOccurrenceOf (
            juce::String::charToString (Chars::space), false, false).toLowerCase();
    }

    /**
     * @brief Resolves an info string's first word to a language family ordinal.
     *
     * The first word is looked up in `map::languageFamily` (file extension →
     * family); a word with no entry, or an empty info string, yields
     * `invalidFamily`.
     *
     * @param infoString The fence's info string.
     * @return The family ordinal (map::Family), or invalidFamily when unlisted.
     */
    static int getLanguageFamily (const juce::String& infoString)
    {
        const auto firstWord { getFirstWord (infoString) };

        if (firstWord.isEmpty())
            return invalidFamily;

        const auto& families { map::languageFamily };

        if (families.contains (firstWord))
            return families.at (firstWord);

        return invalidFamily;
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
