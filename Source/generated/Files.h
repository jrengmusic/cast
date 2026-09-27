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
 * @brief CAST's own file and directory names, referenced by the engine.
 */

#pragma once

namespace files
{
/*_____________________________________________________________________________*/

/** @brief File and directory names CAST reads and writes: manifest, help, banner, style files, stdin name, sync info. */

inline const juce::String cast                { juce::String::fromUTF8 ("spell.md")             };///< Generation manifest.
inline const juce::String castDirectory       { juce::String::fromUTF8 ("cast")                 };///< Sync style directory under a target root.
inline const juce::String castFormat          { juce::String::fromUTF8 (".cast-format")         };///< Style file name, searched first.
inline const juce::String castFormatAlternate { juce::String::fromUTF8 ("_cast-format")         };///< Style file name, searched second.
inline const juce::String castHelp            { juce::String::fromUTF8 ("HELP.md")              };///< Rendered help text.
inline const juce::String castOutput          { juce::String::fromUTF8 ("cast-output.md")       };///< Banner artwork source.
inline const juce::String standardInput       { juce::String::fromUTF8 ("<stdin>")              };///< Stdin file name in a diagnostic.
inline const juce::String userModulesInfo     { juce::String::fromUTF8 ("user-modules-info.md") };///< Sync root info file name.

/**______________________________END OF NAMESPACE______________________________*/
}// namespace files
