#pragma once
#include <JuceHeader.h>
#include <jam_core/format/jam_Format.h>
#include "Constraints.h"
#include <regex>

namespace cast
{
/*____________________________________________________________________________*/

/**
 * @brief Hole-name to fill-value map for one template expansion.
 *
 * SPEC §3: a hole is either scalar (a single juce::String) or aggregate (a
 * juce::StringArray of per-row fragment texts, joined in authored row order).
 */
using SubstitutionMap = jam::HashMap<juce::String, std::variant<juce::String, juce::StringArray>>;

/**
 * @brief Flattens one substitution map entry to its final fill text.
 *
 * A scalar value passes through; an aggregate value's entries are
 * concatenated with no separator, in authored row order.
 *
 * @param value The hole's scalar or aggregate value.
 * @return The text to substitute for the hole.
 */
static juce::String getHoleValue (const std::variant<juce::String, juce::StringArray>& value)
{
    return std::visit ([] (const auto& v) -> juce::String
    {
        using T = std::decay_t<decltype (v)>;

        if constexpr (std::is_same_v<T, juce::String>)
            return v;
        else
            return v.joinIntoString ({});
    }, value);
}

/**
 * @brief Rejects expanded text still containing an unresolved `@hole@`.
 *
 * @param templateFile The expanded template's file, for error locations.
 * @param text         The expanded output text.
 * @return juce::Result::ok() when no `@hole@` construct remains; otherwise a
 *         SPEC §8 `file:row (column)` failure naming the first unresolved hole.
 */
static juce::Result validateHoles (const juce::File& templateFile, const juce::String& text)
{
    static const std::regex holePattern { "@[A-Za-z0-9]+@" };
    const std::string textAsStdString { text.toStdString() };
    std::smatch match;

    if (not std::regex_search (textAsStdString, match, holePattern))
        return juce::Result::ok();

    const auto openPos  { static_cast<int> (match.position (0)) };
    const auto holeName { juce::String (match.str (0)) };
    const auto row      { juce::jmax (1, juce::StringArray::fromLines (text.substring (0, openPos)).size()) };

    return juce::Result::fail (getLocation (templateFile.getFullPathName(), row, holeName)
                               + Id::diagnosticSeparator + Id::failUnresolvedHole + Id::diagnosticSeparator + holeName);
}

/// @brief SPEC §3 template expansion: fills `@hole@` constructs from a substitution map.
struct TemplateEngine
{
    /**
     * @brief Expands @p templateFile, substituting every hole in @p values.
     *
     * Each substitution uses jam::Format::replaceholder() to fill the
     * `@holeName@` construct; line endings are normalized to LF (SPEC §5).
     *
     * @param templateFile The template file to expand.
     * @param values       The hole substitution map.
     * @return The expanded, LF-normalized text.
     */
    static juce::String expand (const juce::File& templateFile, const SubstitutionMap& values)
    {
        auto result { templateFile.loadFileAsString() };

        for (const auto& [holeName, holeValue] : values)
            result = jam::Format::replaceholder (result, holeName, getHoleValue (holeValue));

        return result.replace (Id::charCarriageReturn + Id::charNewline, Id::charNewline).replace (Id::charCarriageReturn, Id::charNewline);
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace cast
