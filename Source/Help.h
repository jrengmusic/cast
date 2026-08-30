#pragma once
#include <JuceHeader.h>

static constexpr int widthCap { 80 };

/**
 * @brief Prints the HELP.md text to stdout.
 *
 * @param specText The HELP.md text, printed verbatim on `--help` and when
 *                 no manifest is found.
 */
static inline void printHelp (const juce::String& specText)
{
    printf ("%s", specText.toRawUTF8());
}
