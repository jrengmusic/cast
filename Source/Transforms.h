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
     * @brief Prefixes @p input with @p extension's single-line comment
     *        syntax.
     *
     * @param input     The text to comment.
     * @param extension The target file extension whose comment syntax is
     *                  applied.
     * @returns @p input, prefixed with @p extension's comment marker.
     */
    static juce::String toComment (const juce::String& input, const juce::String& extension) noexcept
    {
        const auto syntax { map::commentSyntax.get (extension) };

        return (syntax.get (Id::comment) + juce::String::charToString (Chars::space) + input)
            .trim();
    }

    /**
     * @brief Wraps @p input in @p extension's block-comment delimiters.
     *
     * @param input     The text to wrap.
     * @param extension The target file extension whose block-comment
     *                  syntax is applied.
     * @returns @p input, wrapped in @p extension's block-comment open and
     *          close markers.
     */
    static juce::String
    toCommentBlock (const juce::String& input, const juce::String& extension) noexcept
    {
        const auto syntax { map::commentSyntax.get (extension) };

        if (not input.containsChar (Chars::newline))
            return (syntax.get (Id::blockOpen) + juce::String::charToString (Chars::space) + input
                   + juce::String::charToString (Chars::space) + syntax.get (Id::blockClose))
                .trim();

        const auto blockLine { syntax.get (Id::blockLine) };
        jam::Strings blockLines;
        blockLines.add (syntax.get (Id::blockOpen));

        for (const auto& proseLine : jam::Strings::fromLines (input))
            blockLines.add (proseLine.isNotEmpty()
                                 ? blockLine + juce::String::charToString (Chars::space) + proseLine
                                 : blockLine);

        blockLines.add (blockLine.isNotEmpty()
                             ? juce::String::charToString (Chars::space) + syntax.get (Id::blockClose)
                             : syntax.get (Id::blockClose));

        return blockLines.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
    }

    /**
     * @brief Wraps @p input in @p extension's block-comment delimiters,
     *        prefixed by its brief marker.
     *
     * @param input     The text to wrap.
     * @param extension The target file extension whose block-comment and
     *                  brief syntax is applied.
     * @returns @p input, wrapped in @p extension's block-comment open and
     *          close markers with the brief marker inserted.
     */
    static juce::String toBrief (const juce::String& input, const juce::String& extension) noexcept
    {
        const auto syntax { map::commentSyntax.get (extension) };

        return (syntax.get (Id::blockOpen) + juce::String::charToString (Chars::space)
               + syntax.get (Id::brief) + juce::String::charToString (Chars::space) + input
               + juce::String::charToString (Chars::space) + syntax.get (Id::blockClose))
            .trim();
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
                return jam::Format::toUpperCase (input);
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
