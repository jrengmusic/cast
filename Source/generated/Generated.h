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
 * @file Generated.h
 * @brief CAST's generated-header umbrella — re-exports every generated concern.
 *
 * Aggregate of CAST's generated registries. Owns the jam::Generated shared-
 * instance aggregate, giving one construction point for every generated symbol
 * the engine references.
 */

#pragma once

#include "ProjectInfo.h" ///< Product name, version, and source commit.
#include "Identifiers.h" ///< Every Id:: name and transform-operation name.
#include "Text.h"        ///< One diagnostic string per fatal.
#include "Files.h"       ///< Names of the documents the engine embeds.
#include "HashMaps.h"    ///< Banner palette and per-extension comment syntax.

struct Generated
{
    jam::SharedInstance<map::Generated> generated { std::in_place };
};
