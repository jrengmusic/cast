/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION
   ID:                          jam_style
   vendor:                      JRENG! Architectural Modules
   version:                     0.0.1
   name:                        JAM Style
   description:                 JAM Style — LookAndFeel base + ColourScheme-backed colour registry
   website:                     https://jrengmusic.com
   license:                     Proprietary
   dependencies:                jam_graphics,
                                jam_data_structures,
                                jam_web,
                                jam_markdown
   OSXFrameworks:
   iOSFrameworks:
   searchpaths:                 .
 END_JUCE_MODULE_DECLARATION
 *******************************************************************************/

/**
 * @file jam_style.h
 * @brief Module header aggregating all jam_style submodule includes.
 */

#pragma once

#include <jam_graphics/jam_graphics.h>
#include <jam_data_structures/jam_data_structures.h>
#include <jam_web/jam_web.h>
#include <jam_markdown/jam_markdown.h>

#include "jam_StyleCustom.h"
#include "jam_StyleKnob.h"
#include "jam_StyleToggle.h"
#include "jam_StyleShadow.h"

#include "style_manager/jam_StyleManager.h"
#include "style_manager/jam_StyleTheme.h"

#if JUCE_MODULE_AVAILABLE_jam_mermaid_diagram && JAM_MARKDOWN_MERMAID
#include "jam_StyleMermaid.h"
#endif

#if JUCE_MODULE_AVAILABLE_jam_vulkan
#include <jam_vulkan/jam_vulkan.h>
#include "jam_StyleKnob_V1.h"
#include "jam_StyleTogglePush.h"
#include "jam_StyleToggleSlide.h"
#endif

#include "jam_StylePopupTextBox.h"
#include "jam_StyleVariDisplay.h"
