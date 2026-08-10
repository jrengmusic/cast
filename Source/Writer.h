#pragma once
#include <JuceHeader.h>
#include "Validator.h"

namespace cast
{
/*____________________________________________________________________________*/

/**
 * @brief Placeholder-name to fill-value map for one template expansion.
 *
 * SPEC §3: a placeholder is either scalar (a single juce::String) or aggregate (a
 * juce::StringArray of per-row fragment texts, joined in authored row order).
 */
using SubstitutionMap = jam::HashMap<juce::String, std::variant<juce::String, juce::StringArray>>;

/**
 * @brief Flattens one substitution map entry to its final fill text.
 *
 * A scalar value passes through; an aggregate value's entries are
 * concatenated with no separator, in authored row order.
 *
 * @param value The placeholder's scalar or aggregate value.
 * @return The text to substitute for the placeholder.
 */
static juce::String getPlaceholderValue (const std::variant<juce::String, juce::StringArray>& value)
{
    return std::visit (
        [] (const auto& v) -> juce::String
        {
            using T = std::decay_t<decltype (v)>;

            if constexpr (std::is_same_v<T, juce::String>)
                return v;
            else
                return v.joinIntoString ({});
        },
        value);
}

/**
 * @brief Rejects a template containing a placeholder the substitution map cannot fill.
 *
 * Scans @p templateText for `@placeholder@` constructs and reports the first whose
 * name has no entry in @p values. Expanded output is never scanned, so a cell
 * value that is itself placeholder-shaped is data, not an unresolved placeholder.
 *
 * @param templateFile The template's file, for error locations.
 * @param templateText The template text, before substitution.
 * @param values       The placeholder substitution map.
 * @param context      The dispatch or output row this expansion belongs to, for diagnostics.
 * @return juce::Result::ok() when every placeholder has an entry; otherwise a
 *         SPEC §8 `file:row (column)` failure naming the first unfilled placeholder.
 */
static juce::Result validatePlaceholders (const juce::File& templateFile,
                                          const juce::String& templateText,
                                          const SubstitutionMap& values,
                                          const juce::String& context)
{
    static const std::regex placeholderPattern { "@[A-Za-z0-9:]+@" };
    const std::string textAsStdString { templateText.toStdString() };

    for (auto match { std::sregex_iterator (textAsStdString.begin(), textAsStdString.end(), placeholderPattern) };
         match != std::sregex_iterator();
         ++match)
    {
        const auto placeholderName { juce::String (match->str (0)) };
        const auto keyName { placeholderName.substring (1, placeholderName.length() - 1) };

        if (not values.contains (keyName))
        {
            const auto openPos { static_cast<int> (match->position (0)) };
            const auto row { juce::jmax (
                1, juce::StringArray::fromLines (templateText.substring (0, openPos)).size()) };

            juce::StringArray availableKeys;

            for (const auto& [availableName, availableValue] : values)
                availableKeys.add (juce::String::charToString (chars::at) + availableName + juce::String::charToString (chars::at));

            return juce::Result::fail (getLocation (templateFile.getFullPathName(), row, placeholderName)
                                       + Id::diagnosticSeparator + text::en::failNoSource
                                       + Id::diagnosticSeparator + placeholderName
                                       + juce::String::charToString (chars::newline) + context
                                       + juce::String::charToString (chars::newline) + Id::availableLabel
                                       + availableKeys.joinIntoString (juce::String::charToString (chars::space)));
        }
    }

    return juce::Result::ok();
}

/// @brief SPEC §3 template expansion: fills `@placeholder@` constructs from a substitution map.
struct TemplateEngine
{
    /**
     * @brief Expands @p text, substituting every placeholder in @p values.
     *
     * Each substitution uses jam::Format::replaceholder() to fill the
     * `@placeholderName@` construct; line endings are normalized to LF (SPEC §5).
     *
     * @param text   The template text to expand.
     * @param values The placeholder substitution map.
     * @return The expanded, LF-normalized text.
     */
    static juce::String expandText (const juce::String& text, const SubstitutionMap& values)
    {
        auto result { text };

        for (const auto& [placeholderName, placeholderValue] : values)
            result = jam::Format::replaceholder (result, placeholderName, getPlaceholderValue (placeholderValue));

        return result.replace (juce::String::charToString (chars::carriageReturn) + juce::String::charToString (chars::newline), juce::String::charToString (chars::newline))
            .replace (juce::String::charToString (chars::carriageReturn), juce::String::charToString (chars::newline));
    }

    /**
     * @brief Expands @p templateFile, substituting every placeholder in @p values.
     *
     * @param templateFile The template file to expand.
     * @param values       The placeholder substitution map.
     * @return The expanded, LF-normalized text.
     */
    static juce::String expand (const juce::File& templateFile, const SubstitutionMap& values)
    {
        return expandText (templateFile.loadFileAsString(), values);
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace cast
