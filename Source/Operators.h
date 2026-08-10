#pragma once
#include <JuceHeader.h>

namespace cast
{
/*____________________________________________________________________________*/

static const juce::String intType { "int" };
static const juce::String stringType { "juce::String" };
static const juce::String identifierType { "juce::Identifier" };

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
        static const jam::Function::Map<juce::String, juce::String> transforms { [] ()
        {
            jam::Function::Map<juce::String, juce::String> map;

            map.add<const juce::String&> (Id::toUpper.toString(), &jam::Format::toUpperCase);
            map.add<const juce::String&> (Id::toUTF8.toString(), &jam::Format::toUTF8);
            map.add<const juce::String&> (Id::toTitle.toString(), &jam::Format::toTitleCase);
            map.add<const juce::String&> (Id::toKebab.toString(), &jam::Format::toKebabCase);
            map.add<const juce::String&> (Id::toPascal.toString(), &jam::Format::toPascalCase);
            map.add<const juce::String&> (Id::toCamel.toString(), &jam::Format::toCamelCase);
            map.add<const juce::String&> (Id::toSnake.toString(), &jam::Format::toSnakeCase);
            map.add<const juce::String&> (Id::toScreamingSnake.toString(), &jam::Format::toScreamingSnakeCase);
            map.add<const juce::String&> (Id::join.toString(), &jam::Format::join);
            map.add<const juce::String&> (Id::toLiteral.toString(), &jam::Format::toLiteral);
            map.add<const juce::String&> (Id::toHex.toString(), &jam::Format::toHex);
            map.add<const juce::String&> (Id::toCodepoint.toString(), &jam::Format::toCodepoint);
            map.add<const juce::String&> (Id::toSymbol.toString(), &jam::Format::toSymbol);
            map.add<const juce::String&> (Id::fromId.toString(), &jam::Format::fromId);
            map.add<const juce::String&> (Id::fromMap.toString(), &jam::Format::fromMap);
            map.add<const juce::String&> (Id::fromIdentifier.toString(), &jam::Format::fromIdentifier);
            map.add<const juce::String&> (Id::fromLiteral.toString(), &jam::Format::fromLiteral);
            map.add<const juce::String&> (Id::fromCodepoint.toString(), &jam::Format::fromCodepoint);

            return map;
        }() };

        return transforms;
    }

    static const jam::HashMap<juce::String, juce::String>& getTypes() noexcept
    {
        static const jam::HashMap<juce::String, juce::String> types { [] ()
        {
            jam::HashMap<juce::String, juce::String> map;

            map.insert ({ Id::fromId.toString(), stringType });
            map.insert ({ Id::fromMap.toString(), intType });
            map.insert ({ Id::fromIdentifier.toString(), identifierType });
            map.insert ({ Id::fromLiteral.toString(), stringType });
            map.insert ({ Id::fromCodepoint.toString(), stringType });

            return map;
        }() };

        return types;
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace cast
