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

extern const juce::String cast;               ///< Generation manifest.
extern const juce::String castDirectory;      ///< Sync style directory under a target root.
extern const juce::String castFormat;         ///< Style file name, searched first.
extern const juce::String castFormatAlternate;///< Style file name, searched second.
extern const juce::String castHelp;           ///< Rendered help text.
extern const juce::String castOutput;         ///< Banner artwork source.
extern const juce::String standardInput;      ///< Stdin file name in a diagnostic.
extern const juce::String userModulesInfo;    ///< Sync root info file name.

/**______________________________END OF NAMESPACE______________________________*/
}// namespace files
