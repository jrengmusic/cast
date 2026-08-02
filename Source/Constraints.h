#pragma once
#include "Manifest.h"

namespace cast
{

struct Constraints
{
    static juce::Result validate (const jam::Array<ConstraintEntry>& constraints,
                                  const jam::HashMap<juce::String, const jam::Document*>& tables,
                                  const juce::String& sourceFile);
};

} // namespace cast
