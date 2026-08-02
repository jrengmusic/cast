#pragma once
#include "Manifest.h"

namespace cast
{

struct Driver
{
    jam::Array<juce::String> configureDepends;

    juce::Result run (const juce::File& manifestFile, const juce::String& outputFilter = {});
};

} // namespace cast
