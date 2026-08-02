#include "Template.h"

namespace cast
{

static juce::String getHoleValue (const juce::String& holeName, const SubstitutionMap& values)
{
    const auto found { values.find (holeName) };

    if (found == values.end())
        return "${" + holeName + "}";

    const auto& [foundHole, foundValue] { *found };
    return std::visit ([] (const auto& v) -> juce::String
    {
        using T = std::decay_t<decltype (v)>;

        if constexpr (std::is_same_v<T, juce::String>)
            return v;
        else
            return v.joinIntoString ({});
    }, foundValue);
}

juce::Result TemplateEngine::expand (const juce::File& templateFile,
                                     const SubstitutionMap& values,
                                     juce::String& output)
{
    if (not templateFile.existsAsFile())
        return juce::Result::fail ("template not found: " + templateFile.getFullPathName());

    const auto text { templateFile.loadFileAsString() };
    juce::String result;
    int cursor { 0 };
    const int length { text.length() };

    while (cursor < length)
    {
        const auto openPos { text.indexOf (cursor, "${") };

        if (openPos < 0)
        {
            result += text.substring (cursor);
            break;
        }

        result += text.substring (cursor, openPos);

        const auto closePos { text.indexOf (openPos + 2, "}") };

        if (closePos < 0)
        {
            result += text.substring (openPos);
            break;
        }

        const auto holeName { text.substring (openPos + 2, closePos) };
        result += getHoleValue (holeName, values);
        cursor = closePos + 1;
    }

    output = result.replace ("\r\n", "\n").replace ("\r", "\n");
    return juce::Result::ok();
}

} // namespace cast
