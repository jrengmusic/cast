/*******************************************************************************
                        Codegen Annotated Source of Truth
————————————————————————————————————————————————————————————————————————————————

            ░░████████████░░████████████░░████████████░░████████████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████        ░░████  ░░████░░████            ░░████
            ░░████        ░░████████████░░████████████    ░░████
            ░░████        ░░████  ░░████        ░░████    ░░████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████████████░░████  ░░████░░████████████    ░░████

————————————————————————————————————————————————————————————————————————————————
                         FOR YOUR EYES ONLY, DO NOT EDIT
********************************************************************************/

/**
 * @file HashMaps.h
 * @brief CAST's banner palette and per-extension comment-syntax tables.
 */

#pragma once

namespace map
{
/*_____________________________________________________________________________*/

extern const jam::HashMap<juce::String, juce::String> banner;

//==============================================================================

extern const jam::HashMap<juce::Identifier, juce::String> clangComment;

//==============================================================================

extern const jam::HashMap<juce::Identifier, juce::String> cmakeComment;

//==============================================================================

extern const jam::HashMap<juce::Identifier, juce::String> cssComment;

//==============================================================================

extern const jam::HashMap<juce::Identifier, juce::String> gomodComment;

//==============================================================================

extern const jam::HashMap<juce::Identifier, juce::String> htmlComment;

//==============================================================================

extern const jam::HashMap<juce::Identifier, juce::String> luaComment;

//==============================================================================

extern const jam::HashMap<juce::Identifier, juce::String> mermaidComment;

//==============================================================================

extern const jam::HashMap<juce::Identifier, juce::String> pythonComment;

//==============================================================================

extern const jam::HashMap<juce::Identifier, juce::String> rubyComment;

//==============================================================================

extern const jam::HashMap<juce::Identifier, juce::String> shellComment;

//==============================================================================

extern const jam::HashMap<juce::Identifier, juce::String> sqlComment;

//==============================================================================

extern const jam::HashMap<juce::String, jam::HashMap<juce::Identifier, juce::String>> commentSyntax;

//==============================================================================

extern const jam::HashMap<juce::String, juce::String> manifestSyntax;

/**______________________________END OF NAMESPACE______________________________*/
}// namespace map
