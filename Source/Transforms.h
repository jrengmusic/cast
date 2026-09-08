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
     * @param input     The text to comment.
     * @param extension The target file extension whose comment syntax is
     *                  used.
     * @returns @p input, prefixed by @p extension's own comment glyph, or
     *          toCommentBlock()'s own rendering when @p extension declares
     *          no single-line comment glyph.
     */
    static juce::String toComment (const juce::String& input, const juce::String& extension)
    {
        const auto& syntax { map::commentSyntax.at (extension) };
        const auto comment { syntax.get (Id::comment) };

        if (comment.isEmpty())
            return toCommentBlock (input, extension);

        return (comment + Chars::space + input).trim();
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
               + syntax.get (Id::brief) + Chars::space + input
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
