#pragma once
#include <JuceHeader.h>
#include <variant>

namespace cast
{

using SubstitutionMap = jam::HashMap<juce::String, std::variant<juce::String, juce::StringArray>>;

struct TemplateEngine
{
    static juce::Result expand (const juce::File& templateFile,
                                const SubstitutionMap& values,
                                juce::String& output);
};

} // namespace cast
