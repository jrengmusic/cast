#pragma once
#include <JuceHeader.h>

/**
 * @struct Transforms
 * @brief The engine's registry of named format operations -- case,
 *        encoding, text, and comment transforms -- looked up by name and
 *        applied to a cell's resolved value.
 */
struct Transforms
{
    /**
     * @brief Wraps @p input in @p extension's single-line comment glyph,
     *        falling through to toCommentBlock() when @p extension
     *        declares none.
     *
     * @p input's own newlines are joined into spaces before either branch,
     * so the rendered comment always stays on one line.
     *
     * @param input     The text to comment.
     * @param extension The target file extension whose comment syntax is
     *                  used.
     * @returns @p input, its newlines joined into spaces, prefixed by
     *          @p extension's own comment glyph, or toCommentBlock()'s own
     *          rendering when @p extension declares no single-line comment
     *          glyph.
     */
    static juce::String toComment (const juce::String& input, const juce::String& extension)
    {
        const auto& syntax { map::commentSyntax.at (extension) };
        const auto comment { syntax.get (Id::comment) };
        const auto joined { input.replaceCharacter (Chars::newline, Chars::space) };

        if (comment.isEmpty())
            return toCommentBlock (joined, extension);

        return (comment + Chars::space + joined).trim();
    }

    /**
     * @brief Finds @p beginValue's own first matching line and, strictly
     *        after it, @p endValue's own first matching line.
     *
     * The search is the one delimiter search every region reader shares:
     * Validator checks its result before a run writes, Writer splices at
     * it, and Sync keeps the target's region through it. When
     * @p beginValue matches no line, @p endValue is not searched, so the
     * end index is negative whenever the begin index is.
     *
     * @param lines      The region file's own content, split into lines.
     * @param beginValue The resolved @c \[begin\] delimiter text.
     * @param endValue   The resolved @c \[end\] delimiter text.
     * @returns @p beginValue's own matching line index paired with
     *          @p endValue's own matching line index, each @c -1 when it
     *          matches no line.
     */
    static std::pair<int, int> getDelimiterLines (
        const jam::Strings& lines, const juce::String& beginValue, const juce::String& endValue)
    {
        static constexpr int noLine { -1 };

        auto beginLine { noLine };

        for (int lineIndex { 0 }; lineIndex < lines.size() and beginLine < 0; ++lineIndex)
            if (lines.at (lineIndex).contains (beginValue))
                beginLine = lineIndex;

        auto endLine { noLine };

        for (int lineIndex { beginLine + 1 }; beginLine >= 0 and lineIndex < lines.size() and endLine < 0;
             ++lineIndex)
            if (lines.at (lineIndex).contains (endValue))
                endLine = lineIndex;

        return { beginLine, endLine };
    }

    /**
     * @brief Builds @p lines with the lines strictly between @p beginLine
     *        and @p endLine replaced by a slice of @p regionLines.
     *
     * @param lines       The file's own content, split into lines.
     * @param beginLine   The index of @p lines' own begin delimiter line,
     *                    kept.
     * @param endLine     The index of @p lines' own end delimiter line,
     *                    kept.
     * @param regionLines The lines the replacement is sliced from.
     * @param regionStart The index of @p regionLines' own first replacement
     *                    line.
     * @param regionEnd   The index one past @p regionLines' own last
     *                    replacement line.
     * @returns @p lines up to and including @p beginLine, then
     *          @p regionLines' own [@p regionStart, @p regionEnd) slice,
     *          then @p lines from @p endLine on.
     */
    static jam::Strings getSplicedLines (const jam::Strings& lines, int beginLine, int endLine,
        const jam::Strings& regionLines, int regionStart, int regionEnd)
    {
        jam::Strings spliced;

        for (int lineIndex { 0 }; lineIndex <= beginLine; ++lineIndex)
            spliced.add (lines.at (lineIndex));

        for (int lineIndex { regionStart }; lineIndex < regionEnd; ++lineIndex)
            spliced.add (regionLines.at (lineIndex));

        for (int lineIndex { endLine }; lineIndex < lines.size(); ++lineIndex)
            spliced.add (lines.at (lineIndex));

        return spliced;
    }

    /**
     * @brief Joins @p lines with LF, keeping the final LF that @p original
     *        carried.
     *
     * @param lines    The lines to join.
     * @param original The text @p lines derive from, whose final LF is
     *                 kept.
     * @returns @p lines joined with LF, with a final LF added when
     *          @p original ends with one and the join does not.
     */
    static juce::String getJoinedText (const jam::Strings& lines, const juce::String& original)
    {
        static const auto newlineText { juce::String::charToString (Chars::newline) };

        auto joined { lines.joinIntoString (newlineText, 0, -1) };

        if (original.endsWith (newlineText) and not joined.endsWith (newlineText))
            joined += newlineText;

        return joined;
    }

    /**
     * @brief Replaces @p sourceFilePrefix with @p targetFilePrefix in
     *        every segment of @p relativePath -- directory names and file
     *        names alike.
     *
     * @param relativePath     The root-relative path to transform.
     * @param sourceFilePrefix The source's own identity @c filePrefix
     *                         value.
     * @param targetFilePrefix The target's own identity @c filePrefix
     *                         value.
     * @returns @p relativePath with @p sourceFilePrefix replaced by
     *          @p targetFilePrefix.
     */
    static juce::String getTransformedPath (
        const juce::String& relativePath, const juce::String& sourceFilePrefix, const juce::String& targetFilePrefix)
    {
        return relativePath.replace (sourceFilePrefix, targetFilePrefix);
    }

    /**
     * @brief Wraps @p input in @p extension's block-comment syntax, when
     *        @p extension declares one -- one line when @p input carries
     *        no newline, otherwise each prose line behind @p extension's
     *        own block-line glyph, the glyph alone on a blank prose line,
     *        closed behind one space when @p extension declares a
     *        block-line glyph. Absent a block-comment open glyph, every
     *        prose line is prefixed instead by @p extension's own
     *        single-line comment glyph and one space, with no open or
     *        close frame line.
     *
     * @param input     The text to comment.
     * @param extension The target file extension whose comment syntax is
     *                  used.
     * @returns @p input, wrapped in @p extension's own block-comment open
     *          and close glyphs, or, absent one, each line prefixed by
     *          @p extension's own single-line comment glyph.
     */
    static juce::String
    toCommentBlock (const juce::String& input, const juce::String& extension)
    {
        static const auto spaceText { juce::String::charToString (Chars::space) };
        static const auto newlineText { juce::String::charToString (Chars::newline) };

        const auto& syntax { map::commentSyntax.at (extension) };
        const auto blockOpen { syntax.get (Id::blockOpen) };
        jam::Strings blockLines;

        if (blockOpen.isNotEmpty())
        {
            if (not input.containsChar (Chars::newline))
                return (blockOpen + Chars::space + input
                       + Chars::space + syntax.get (Id::blockClose))
                    .trim();

            const auto blockLine { syntax.get (Id::blockLine) };
            const auto prefix { blockLine.isNotEmpty()
                                    ? blockLine + Chars::space
                                    : juce::String{} };

            blockLines.add (blockOpen);

            for (const auto& proseLine : jam::Strings::fromLines (input))
                blockLines.add (proseLine.isNotEmpty() ? prefix + proseLine : blockLine);

            blockLines.add (blockLine.isNotEmpty()
                                 ? spaceText + syntax.get (Id::blockClose)
                                 : syntax.get (Id::blockClose));
        }
        else
        {
            const auto prefix { syntax.get (Id::comment) + Chars::space };

            for (const auto& proseLine : jam::Strings::fromLines (input))
                blockLines.add (prefix + proseLine);
        }

        return blockLines.joinIntoString (newlineText, 0, -1);
    }

    /**
     * @brief Wraps @p input in @p extension's block-comment glyphs,
     *        prefixed by @p extension's own @c \@brief tag, falling
     *        through to toComment() when @p extension declares no
     *        block-comment open glyph.
     *
     * @param input     The text to comment.
     * @param extension The target file extension whose comment syntax is
     *                  used.
     * @returns @p input, wrapped in @p extension's own block-comment open
     *          and close glyphs and @c \@brief tag, or toComment()'s own
     *          rendering when @p extension declares no block-comment open
     *          glyph.
     */
    static juce::String toBrief (const juce::String& input, const juce::String& extension)
    {
        const auto& syntax { map::commentSyntax.at (extension) };
        const auto blockOpen { syntax.get (Id::blockOpen) };

        if (blockOpen.isEmpty())
            return toComment (input, extension);

        return (blockOpen + Chars::space
               + syntax.get (Id::brief) + Chars::space + input.replaceCharacter (Chars::newline, Chars::space)
               + Chars::space + syntax.get (Id::blockClose))
            .trim();
    }

    /**
     * @brief Resolves @p file's own comment-syntax key -- @c
     *        map::manifestSyntax's own extension for @p file's exact
     *        name, when it carries an entry, trusted without a repeated
     *        containment check to name a declared @c map::commentSyntax
     *        table; absent one, @p file's own file extension when it
     *        names a declared @c map::commentSyntax table, or @c
     *        Extensions::h's own key otherwise.
     *
     * @param file The output file whose comment-syntax key is resolved.
     * @returns @p file's resolved comment-syntax key.
     */
    static juce::String getCommentSyntaxKey (const juce::String& file)
    {
        static const auto dotText { juce::String::charToString (Chars::dot) };

        const auto outputFile { juce::File::createFileWithoutCheckingPath (file) };

        if (auto syntaxEntry { map::manifestSyntax.find (outputFile.getFileName()) };
            syntaxEntry != map::manifestSyntax.end())
        {
            const auto& [syntaxKey, extension] { *syntaxEntry };

            return extension;
        }

        const auto extension { outputFile.getFileExtension() };

        if (map::commentSyntax.contains (extension))
            return extension;

        return dotText + Extensions::h;
    }

    /**
     * @brief Reads @p fenceName's own bracket prefix -- the word enclosed
     *        by @c Chars::openBracket and its @c Chars::enclosure partner
     *        at the start of @p fenceName -- or an empty string when
     *        @p fenceName opens with no bracket group or carries no
     *        closing bracket to pair it with.
     *
     * @param fenceName The fence name whose bracket prefix is read.
     * @returns @p fenceName's own bracket word, or an empty string when
     *          @p fenceName carries no complete bracket group.
     */
    static juce::String getFencePrefix (const juce::String& fenceName)
    {
        static const auto closeBracket { Chars::enclosure.get (Chars::openBracket) };
        static const auto closeBracketText { juce::String::charToString (closeBracket) };

        if (fenceName.startsWithChar (Chars::openBracket)
            and fenceName.containsChar (closeBracket))
            return jam::Format::withoutEnclosure (
                jam::Format::upTo (fenceName, closeBracketText, true), Chars::openBracket);

        return {};
    }

    /**
     * @brief Answers whether @p name is a known transform.
     *
     * @param name The camel-cased transform name to look up.
     * @returns @c true when @p name is a known transform.
     */
    static bool contains (const juce::String& name) noexcept
    {
        return getTransforms().contains (name);
    }

    /**
     * @brief Applies the transform named @p name to @p input.
     *
     * @param name      The camel-cased transform name to apply.
     * @param input     The text the transform is applied to.
     * @param extension The target file extension, used by the comment
     *                  transforms.
     * @returns @p input, transformed by @p name.
     */
    static juce::String getTransformed (const juce::String& name,
                                        const juce::String& input,
                                        const juce::String& extension)
    {
        jassert (contains (name));

        return getTransforms().get (name, input, extension);
    }

    /**
     * @brief Answers whether @p character sits outside a word --
     *        Sync's own whole-word matching law: the
     *        characters before and after a match are absent or outside
     *        @c [A-Za-z0-9_].
     *
     * @param character The character to test, or Chars::nullCharacter
     *                  when the position it stands for is absent (text
     *                  start or end).
     * @returns @c true when @p character is absent or not a letter,
     *          digit, or underscore.
     */
    static bool isWordBoundaryChar (juce::juce_wchar character) noexcept
    {
        return character == Chars::nullCharacter
               or not (juce::CharacterFunctions::isLetterOrDigit (character) or character == Chars::underscore);
    }

    /**
     * @brief Finds @p word's own next whole-word match in @p text at or
     *        after @p cursor -- the one scan getWordBoundaryReplaced()
     *        and containsWholeWord() each read through.
     *
     * @param text   The text searched.
     * @param word   The word searched for.
     * @param cursor The byte offset the search starts from.
     * @returns @p word's own next whole-word match position, or @c -1
     *          when none remains.
     */
    static int getNextWholeWordMatch (const juce::String& text, const juce::String& word, int cursor)
    {
        for (auto position { text.indexOf (cursor, word) }; position >= 0; position = text.indexOf (cursor, word))
        {
            const auto beforeChar { position > 0 ? text[position - 1] : Chars::nullCharacter };
            const auto matchEnd { position + word.length() };
            const auto afterChar { matchEnd < text.length() ? text[matchEnd] : Chars::nullCharacter };

            if (isWordBoundaryChar (beforeChar) and isWordBoundaryChar (afterChar))
                return position;

            cursor = position + 1;
        }

        return -1;
    }

    /**
     * @brief Replaces every whole-word occurrence of @p source in
     *        @p text with @p target, a non-whole-word occurrence left
     *        untouched.
     *
     * @param text   The text to transform.
     * @param source The word to replace.
     * @param target The replacement text.
     * @returns @p text with every whole-word @p source occurrence
     *          replaced by @p target.
     */
    static juce::String
    getWordBoundaryReplaced (const juce::String& text, const juce::String& source, const juce::String& target)
    {
        juce::String result;
        int cursor { 0 };

        for (auto position { getNextWholeWordMatch (text, source, cursor) }; position >= 0;
             position = getNextWholeWordMatch (text, source, cursor))
        {
            result += text.substring (cursor, position) + target;
            cursor = position + source.length();
        }

        result += text.substring (cursor);
        return result;
    }

    /**
     * @brief Answers whether @p text contains @p word as a whole word --
     *        Sync's own contamination check for a @c word-boundary
     *        identity pair.
     *
     * @param text The text to search.
     * @param word The word to search for.
     * @returns @c true when @p text contains @p word as a whole word.
     */
    static bool containsWholeWord (const juce::String& text, const juce::String& word)
    {
        return getNextWholeWordMatch (text, word, 0) >= 0;
    }

private:
    /**
     * @brief Registers the case transforms -- @c toUpper, @c toTitle,
     *        @c toKebab, @c toPascal, @c toCamel, @c toSnake, and
     *        @c toScreamingSnake -- into @p map.
     *
     * @param map The transform registry the case transforms are added to.
     */
    static void addCaseTransforms (jam::Function::Map<juce::String, juce::String>& map)
    {
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::toUpper),
            [] (const juce::String& input, const juce::String&)
            {
                return input.toUpperCase();
            });
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::toTitle),
            [] (const juce::String& input, const juce::String&)
            {
                return jam::Format::toTitleCase (input);
            });
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::toKebab),
            [] (const juce::String& input, const juce::String&)
            {
                return jam::Format::toKebabCase (input);
            });
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::toPascal),
            [] (const juce::String& input, const juce::String&)
            {
                return jam::Format::toPascalCase (input);
            });
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::toCamel),
            [] (const juce::String& input, const juce::String&)
            {
                return jam::Format::toCamelCase (input);
            });
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::toSnake),
            [] (const juce::String& input, const juce::String&)
            {
                return jam::Format::toSnakeCase (input);
            });
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::toScreamingSnake),
            [] (const juce::String& input, const juce::String&)
            {
                return jam::Format::toScreamingSnakeCase (input);
            });
    }

    /**
     * @brief Registers the encoding transforms -- @c toLiteral, @c toUTF8,
     *        @c fromUTF8, @c toHex, @c toCodepoint, and @c fromCodepoint --
     *        into @p map.
     *
     * @param map The transform registry the encoding transforms are added
     *            to.
     */
    static void addEncodingTransforms (jam::Function::Map<juce::String, juce::String>& map)
    {
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::toUTF8),
            [] (const juce::String& input, const juce::String&)
            {
                return jam::Format::toUTF8 (input);
            });
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::fromUTF8),
            [] (const juce::String& input, const juce::String&)
            {
                const std::string_view text { input.getCharPointer().getAddress() };
                const auto result { jam::Format::fromUTF8 (text) };

                return juce::String::fromUTF8 (result.data(), static_cast<int> (result.size()));
            });
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::toLiteral),
            [] (const juce::String& input, const juce::String&)
            {
                return jam::Format::toLiteral (input);
            });
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::toHex),
            [] (const juce::String& input, const juce::String&)
            {
                return jam::Format::toHex (input);
            });
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::toCodepoint),
            [] (const juce::String& input, const juce::String&)
            {
                return jam::Format::toCodepoint (input);
            });
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::fromCodepoint),
            [] (const juce::String& input, const juce::String&)
            {
                return jam::Format::fromCodepoint (input);
            });
    }

    /**
     * @brief Registers the text transforms -- @c join and @c toFileName --
     *        into @p map.
     *
     * @param map The transform registry the text transforms are added to.
     */
    static void addTextTransforms (jam::Function::Map<juce::String, juce::String>& map)
    {
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::join),
            [] (const juce::String& input, const juce::String&)
            {
                return jam::Format::join (input);
            });
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::toFileName),
            [] (const juce::String& input, const juce::String&)
            {
                return static_cast<juce::String (*) (const juce::String&)> (
                    &jam::Format::toFileName) (input);
            });
    }

    /**
     * @brief Registers the comment transforms -- @c toComment,
     *        @c toCommentBlock, and @c brief -- into @p map.
     *
     * @param map The transform registry the comment transforms are added
     *            to.
     */
    static void addCommentTransforms (jam::Function::Map<juce::String, juce::String>& map)
    {
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::toComment), &Transforms::toComment);
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::toCommentBlock), &Transforms::toCommentBlock);
        map.add<const juce::String&, const juce::String&> (
            jam::Format::toCamelCase (Id::brief), &Transforms::toBrief);
    }

    /**
     * @brief The registry mapping every known transform's camel-cased name
     *        to its implementation, built once on first use -- the case
     *        transforms, the encoding transforms including @c fromUTF8,
     *        the text transforms, and the comment transforms.
     *
     * @returns The transform registry.
     */
    static const jam::Function::Map<juce::String, juce::String>& getTransforms() noexcept
    {
        static const jam::Function::Map<juce::String, juce::String> transforms {
            []()
            {
                jam::Function::Map<juce::String, juce::String> map;

                addCaseTransforms (map);
                addEncodingTransforms (map);
                addTextTransforms (map);
                addCommentTransforms (map);

                return map;
            }()
        };

        return transforms;
    }
};
