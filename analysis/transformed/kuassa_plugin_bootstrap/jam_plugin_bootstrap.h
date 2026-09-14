/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION
   ID:                          jam_plugin_bootstrap
   vendor:                      JRENG! Architectural Modules
   version:                     0.0.1
   name:                        JAM Plugin Bootstrap
   description:                 Document-driven plugin bootstrap — view construction, style management, plugin editor base, standalone shell
   website:                     https://jrengmusic.com
   license:                     Proprietary
   dependencies:                jam_core, jam_data_structures, jam_gui, jam_markdown, jam_style, jam_vulkan, jam_web,
                                juce_audio_processors,
   OSXFrameworks:
   iOSFrameworks:
 END_JUCE_MODULE_DECLARATION
 *******************************************************************************/

/**
 * @file jam_plugin_bootstrap.h
 * @brief Document-driven plugin bootstrap — view construction, style
 *        manager, the abstract plugin editor base, and the standalone shell.
 */

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <jam_core/jam_core.h>
#include <jam_data_structures/jam_data_structures.h>
#include <jam_gui/jam_gui.h>
#include <jam_markdown/jam_markdown.h>
#include <jam_style/jam_style.h>
#include <jam_vulkan/jam_vulkan.h>
#include <jam_web/jam_web.h>

#include "parameter/jam_ParameterLayout.h"
#include "layout/jam_PluginEditorLayout.h"
#include "view/jam_ScaledContent.h"
#include "view/jam_ViewManager.h"
#include "view/jam_ViewPanel.h"
#include "view/jam_ViewEditor.h"
#include "view/jam_PluginEditor.h"
#include "view/jam_ViewContent.h"
#include "view/jam_ViewSettings.h"

#if JucePlugin_Build_Standalone
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

#include "standalone/jam_AudioStandaloneApp.h"
