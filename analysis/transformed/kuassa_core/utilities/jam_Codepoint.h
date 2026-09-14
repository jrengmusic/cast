/**
 * @file jam_Codepoint.h
 * @brief Conversions between char32_t codepoints and JUCE's wide-character types.
 */

#pragma once

namespace jam
{ /*____________________________________________________________________________*/

/** @brief Narrows a char32_t codepoint to juce::juce_wchar. */
inline juce::juce_wchar toChar (char32_t codepoint) noexcept
{
    return static_cast<juce::juce_wchar> (codepoint);
}

/** @brief Widens a juce::juce_wchar to a char32_t codepoint. */
inline char32_t toCodepoint (juce::juce_wchar ch) noexcept
{
    return static_cast<char32_t> (ch);
}

/** @brief Reinterprets a char32_t codepoint as a plain uint32_t. */
inline uint32_t toU32 (char32_t codepoint) noexcept
{
    return static_cast<uint32_t> (codepoint);
}

/**_____________________________END OF NAMESPACE______________________________*/
} // namespace jam
