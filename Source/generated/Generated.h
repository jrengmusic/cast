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

#include "ProjectInfo.h" ///< Product name, version, and sourcecommit.
#include "Identifiers.h" ///< Every Id:: name and transform-operationname.
#include "Text.h"        ///< One string per fatal, plus the successline.
#include "Files.h"       ///< File and directory names the enginereads and writes.
#include "HashMaps.h"    ///< Banner palette and per-extension commentsyntax.

struct Generated
{
    jam::SharedInstance<map::Generated> generated { std::in_place };///< Every generated registry, constructed once.
};
