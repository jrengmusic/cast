#pragma once
#include <JuceHeader.h>

namespace cast
{

struct Transforms
{
    static juce::Result apply (const juce::String& name, const juce::String& input, juce::String& output);
};

} // namespace cast
