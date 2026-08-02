#pragma once
#include <JuceHeader.h>

namespace cast
{

struct OutputEntry
{
    juce::String outputPath;
    juce::String rootTemplatePath;
    juce::StringArray inputTables;
};

struct DispatchEntry
{
    juce::String tableName;
    juce::String columnName;
    juce::String matchValue;
    juce::String fragmentPath;
    juce::String slotName;
};

struct TransformEntry
{
    juce::String columnName;
    juce::String transformName;
};

struct ConstraintEntry
{
    juce::String columnName;
    juce::String predicate;
};

struct Manifest
{
    jam::Array<OutputEntry>    outputs;
    jam::Array<DispatchEntry>  dispatches;
    jam::Array<TransformEntry> transforms;
    jam::Array<ConstraintEntry> constraints;
    jam::Array<juce::String>   configureDepends;

    static juce::Result parse (const juce::File& castFile, Manifest& result);
};

} // namespace cast
