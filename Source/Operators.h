#pragma once
#include <JuceHeader.h>

struct Transforms
{
    static juce::String toComment (const juce::String& input, const juce::String& extension) noexcept
    {
        const auto syntax { map::commentSyntax.get (extension) };

        return (syntax.get (Id::comment) + juce::String::charToString (Chars::space) + input)
            .trim();
    }

    static juce::String
    toCommentBlock (const juce::String& input, const juce::String& extension) noexcept
    {
        const auto syntax { map::commentSyntax.get (extension) };

        return (syntax.get (Id::blockOpen) + juce::String::charToString (Chars::space) + input
               + juce::String::charToString (Chars::space) + syntax.get (Id::blockClose))
            .trim();
    }

    static juce::String toBrief (const juce::String& input, const juce::String& extension) noexcept
    {
        const auto syntax { map::commentSyntax.get (extension) };

        return (syntax.get (Id::blockOpen) + juce::String::charToString (Chars::space)
               + syntax.get (Id::brief) + juce::String::charToString (Chars::space) + input
               + juce::String::charToString (Chars::space) + syntax.get (Id::blockClose))
            .trim();
    }

    static bool contains (const juce::String& name) noexcept
    {
        return getTransforms().contains (name);
    }

    static juce::String getTransformed (const juce::String& name,
                                        const juce::String& input,
                                        const juce::String& extension)
    {
        jassert (contains (name));

        return getTransforms().get (name, input, extension);
    }

    static const jam::Function::Map<juce::String, juce::String>& getTransforms() noexcept
    {
        static const jam::Function::Map<juce::String, juce::String> transforms {
            []()
            {
                jam::Function::Map<juce::String, juce::String> map;

                map.add<const juce::String&, const juce::String&> (
                    jam::Format::toCamelCase (Id::toUpper),
                    [] (const juce::String& input, const juce::String&)
                    {
                        return jam::Format::toUpperCase (input);
                    });
                map.add<const juce::String&, const juce::String&> (
                    jam::Format::toCamelCase (Id::toUTF8),
                    [] (const juce::String& input, const juce::String&)
                    {
                        return jam::Format::toUTF8 (input);
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
                    jam::Format::toCamelCase (Id::toComment), &Transforms::toComment);
                map.add<const juce::String&, const juce::String&> (
                    jam::Format::toCamelCase (Id::toCommentBlock), &Transforms::toCommentBlock);
                map.add<const juce::String&, const juce::String&> (
                    jam::Format::toCamelCase (Id::brief), &Transforms::toBrief);
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
                map.add<const juce::String&, const juce::String&> (
                    jam::Format::toCamelCase (Id::join),
                    [] (const juce::String& input, const juce::String&)
                    {
                        return jam::Format::join (input);
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
                map.add<const juce::String&, const juce::String&> (
                    jam::Format::toCamelCase (Id::toFileName),
                    [] (const juce::String& input, const juce::String&)
                    {
                        return static_cast<juce::String (*) (const juce::String&)> (
                            &jam::Format::toFileName) (input);
                    });

                return map;
            }()
        };

        return transforms;
    }
};
