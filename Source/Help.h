#pragma once
#include <JuceHeader.h>

/**
 * @brief Prints the HELP.md text to stdout.
 *
 * @param helpText The HELP.md text, printed verbatim on `--help` and when
 *                 no manifest is found.
 */
static inline void printHelp (const juce::String& helpText)
{
    printf ("%s", helpText.toRawUTF8());
}
