/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION
   ID:                          jam_data_structures
   vendor:                      JRENG! Architectural Modules
   version:                     0.0.1
   name:                        JAM Data Structures
   description:                 ValueTree management and data model utilities — model, parameters, JSON conversion
   website:                     https://jrengmusic.com
   license:                     Proprietary
   dependencies:                jam_core,
                                jam_graphics,
                                juce_data_structures,
                                juce_gui_basics,
                                juce_audio_processors,
   OSXFrameworks:
   iOSFrameworks:
 END_JUCE_MODULE_DECLARATION
 *******************************************************************************/

/**
 * @file jam_data_structures.h
 * @brief Data structures module — Model, ParameterBase/ParameterText,
 *        juce::var/ValueTree JSON conversion, AudioModel/SettingsModel,
 *        and ParameterManager, built atop jam_core.
 */

#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <jam_core/jam_core.h>
#include <jam_graphics/jam_graphics.h>

#include "model/jam_ParameterBase.h"
#include "model/jam_ParameterText.h"
#include "model/jam_Model.h"
#include "json/jam_json.h"

#include "parameter/jam_ParameterManager.h"
#include "model/jam_AudioModel.h"
#include "registry/jam_Traits.h"
#include "registry/jam_Registry.h"
#include "model/jam_SettingsModel.h"
