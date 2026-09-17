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
 * @file Files.h
 * @brief CAST's own document file names, referenced by the engine.
 */

#pragma once

namespace files
{
/*_____________________________________________________________________________*/

/** @brief File names CAST reads and writes during a generation run. */

inline const juce::String cast            { juce::String::fromUTF8 ("spell.md")             };///< Generation manifest.
inline const juce::String castHelp        { juce::String::fromUTF8 ("HELP.md")              };///< Rendered help text.
inline const juce::String castOutput      { juce::String::fromUTF8 ("cast-output.md")       };///< Banner artwork source.
inline const juce::String userModulesInfo { juce::String::fromUTF8 ("user-modules-info.md") };///< Sync root info file name.

/**______________________________END OF NAMESPACE______________________________*/
}// namespace files
