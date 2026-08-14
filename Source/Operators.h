#pragma once
#include <JuceHeader.h>

/// @brief SPEC §6 transform vocabulary lookup.
struct Transforms
{
    /** @brief Reports whether @p name is in the closed transform vocabulary (SPEC §6). */
    static bool contains (const juce::String& name) noexcept
    {
        return getTransforms().contains (name);
    }

    /**
     * @brief Applies the named transform to @p input.
     *
     * @param name  The transform's identifier string; must be in the closed
     *              vocabulary (SPEC §6).
     * @param input The value to transform.
     * @return The transformed value.
     * @note Asserts @p name is a known transform.
     */
    static juce::String getTransformed (const juce::String& name, const juce::String& input)
    {
        jassert (contains (name));

        return getTransforms().get (name, input);
    }

    /** @brief The closed transform vocabulary (SPEC §6), keyed by identifier string. */
    static const jam::Function::Map<juce::String, juce::String>& getTransforms() noexcept
    {
        static const jam::Function::Map<juce::String, juce::String> transforms {
            []()
            {
                jam::Function::Map<juce::String, juce::String> map;

                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toUpper), &jam::Format::toUpperCase);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toUTF8), &jam::Format::toUTF8);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toTitle), &jam::Format::toTitleCase);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toKebab), &jam::Format::toKebabCase);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toPascal), &jam::Format::toPascalCase);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toCamel), &jam::Format::toCamelCase);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toComment), &jam::Format::toComment);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toCommentBlock), &jam::Format::toCommentBlock);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toSnake), &jam::Format::toSnakeCase);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toScreamingSnake),
                    &jam::Format::toScreamingSnakeCase);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::join), &jam::Format::join);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toLiteral), &jam::Format::toLiteral);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toHex), &jam::Format::toHex);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toCodepoint), &jam::Format::toCodepoint);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toSymbol), &jam::Format::toSymbol);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toUnicode), &jam::Format::toUnicode);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::fromId), &jam::Format::fromId);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::fromMap), &jam::Format::fromMap);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::fromIdentifier), &jam::Format::fromIdentifier);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::fromLiteral), &jam::Format::fromLiteral);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::fromCodepoint), &jam::Format::fromCodepoint);
                map.add<const juce::String&> (
                    jam::Format::toCamelCase (Id::toFileName),
                    static_cast<juce::String (*) (const juce::String&)> (&jam::Format::toFileName));

                return map;
            }()
        };

        return transforms;
    }
};
