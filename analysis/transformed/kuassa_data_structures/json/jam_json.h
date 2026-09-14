/**
 * @file jam_json.h
 * @brief Bidirectional conversion between juce::var JSON values and juce::ValueTree.
 *
 * JSON objects become ValueTree children keyed by property name; JSON arrays
 * become a child tree tagged with @c Id::jsonArray; JSON primitives become a
 * property named @c Id::value.
 */

#pragma once

namespace jam::json
{ /*____________________________________________________________________________*/

/** @brief Converts a parsed JSON value into a ValueTree.
 *  @param source  JSON value (object, array, or primitive) as produced by juce::JSON::parse.
 *  @param rootId  Node type identifier for the returned tree.
 *  @return  A ValueTree encoding @p source: objects as children keyed by property name,
 *           arrays as an @c Id::jsonArray-tagged child, primitives as an @c Id::value property.
 */
juce::ValueTree fromJson (const juce::var& source,
                          const juce::Identifier& rootId);

/** @brief Converts a ValueTree built by fromJson() back into a JSON value.
 *  @param tree  A valid ValueTree, typically produced by fromJson().
 *  @return  The equivalent juce::var — object, array, or primitive.
 */
juce::var toJson (const juce::ValueTree& tree);

/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam::json
