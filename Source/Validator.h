#pragma once
#include <JuceHeader.h>

struct Validator
{
    static juce::Result isValid (const jam::Document& document);
};

inline juce::Result Validator::isValid (const jam::Document& document)
{
    juce::ignoreUnused (document);
    return juce::Result::ok();
}
