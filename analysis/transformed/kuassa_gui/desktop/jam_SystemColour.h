/**
 * @file jam_SystemColour.h
 * @brief Native OS system colour lookup.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @brief Resolves a system colour identifier to the current native OS colour.
 * @param identifier  System colour identifier (e.g. "system-window").
 * @return The resolved native colour, or `juce::Colours::magenta` when
 *         @p identifier is unrecognised on the current platform.
 */
juce::Colour getSystemColour (const juce::String& identifier);

/**_____________________________END_OF_NAMESPACE______________________________*/
} // namespace jam
