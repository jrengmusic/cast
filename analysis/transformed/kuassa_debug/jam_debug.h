/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION
 ID             :   jam_debug
 vendor         :   JRENG
 version        :   0.0.1
 name           :   JAM Debug
 description    :   Debug instruments — console, log, model monitor
 website        :
 license        :   Proprietary
 dependencies   :   juce_gui_basics,
                    juce_gui_extra,
                    jam_core,
                    jam_graphics,
                    jam_gui,
                    jam_style,
 END_JUCE_MODULE_DECLARATION
*******************************************************************************/

/**
 * @file jam_debug.h
 * @brief Debug instruments module header — console, build info, model monitor, DebugWidget.
 */

#pragma once
#include <thread>
#include <juce_gui_basics/juce_gui_basics.h>
#include <jam_core/jam_core.h>
#include <jam_gui/jam_gui.h>
#include <jam_style/jam_style.h>
#include "look_and_feel/jam_StyleDebug.h"
#include "build_info/jam_DebugBuildInfo.h"
#include "console/jam_DebugConsole.h"
#include "model_monitor/jam_DebugModelMonitor.h"
#include "jam_DebugWidget.h"

namespace jam
{
/*____________________________________________________________________________*/

//==============================================================================

/** @brief Prints `name = value` to the debug console via jam::DebugConsole::print. */
#define cout(name) jam::DebugConsole::print (#name, (name))

//==============================================================================
namespace debug
{
/*____________________________________________________________________________*/

#if JUCE_DEBUG

/**
 * @brief Reports a fatal error and terminates the program.
 *
 * Always writes the message to stderr, then unconditionally
 * terminates the program. Safe to call from noexcept functions.
 *
 * @note If you hit this during development and see no output,
 *       it may be because the Debug Widget has already taken
 *       over the console. In that case, temporarily disable
 *       the widget creation (guarded by JUCE_DEBUG) to allow
 *       messages to appear in the IDE console.
 *
 * @param message The error message to print before termination.
 */
[[noreturn]] inline void error (const juce::String& message) noexcept
{
    std::cerr << message.toRawUTF8() << std::endl;
    std::abort();
}

#endif// JUCE_DEBUG
/**_____________________________END OF NAMESPACE______________________________*/
}// namespace debug
/**_____________________________END OF NAMESPACE______________________________*/
}// namespace jam
