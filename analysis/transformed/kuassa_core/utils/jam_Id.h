/**
 * @file jam_Id.h
 * @brief Casing helpers for the generated lexicon's tag/type identifier forms.
 */

namespace Id
{
/*____________________________________________________________________________*/

/**
 * @brief Upper-cases @p word's string form for use as a tag string.
 *
 * @param word  Identifier whose name is upper-cased.
 * @return      @p word's name, upper-cased, as a juce::String.
 */
inline juce::String toTag (const juce::Identifier& word)
{
    return word.toString().toUpperCase();
}

inline juce::String toTag (const juce::String& word)
{
    return word.toUpperCase();
}

/**
 * @brief Upper-cases @p word's string form for use as a type identifier.
 *
 * @param word  Identifier whose name is upper-cased.
 * @return      A new juce::Identifier constructed from @p word's name, upper-cased.
 */
inline juce::Identifier toType (const juce::Identifier& word)
{
    return juce::Identifier { word.toString().toUpperCase() };
}

/**_____________________________END_OF_NAMESPACE______________________________*/
}// namespace Id
