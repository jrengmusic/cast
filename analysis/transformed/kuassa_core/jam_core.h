/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION
   ID:                          jam_core
   vendor:                      JRENG! Architectural Modules
   version:                     0.0.1
   name:                        JAM Core
   description:                 JAM Core
   website:                     https://jrengmusic.com
   license:                     Proprietary
   dependencies:                juce_core,
                                juce_data_structures,
                                juce_events,
                                juce_graphics,
   OSXFrameworks:               CoreServices
   iOSFrameworks:
  END_JUCE_MODULE_DECLARATION
 *******************************************************************************/

/**
 * @file jam_core.h
 * @brief Core module — atomics, buffers, document/file/value/format utilities,
 *        and the debug::Log diagnostics entry point, built atop jam_lexicon.
 */

#pragma once

//==============================================================================
/** Config: JAM_USING_MULTI_ORIENTATION
    Enables Multi-orientation for Portrait and Landscape.
*/
#ifndef JAM_USING_MULTI_ORIENTATION
#define JAM_USING_MULTI_ORIENTATION 0
#endif

/** Config: JAM_USING_OVERSAMPLING
    Enables Oversampling.
*/
#ifndef JAM_USING_OVERSAMPLING
#define JAM_USING_OVERSAMPLING 1
#endif

/** Config: JAM_USING_AQUATIC_PRIME
    Enables the Aquatic Prime licensing system.
*/
#ifndef JAM_USING_AQUATIC_PRIME
#define JAM_USING_AQUATIC_PRIME 0
#endif

//==============================================================================
#include <ciso646>
#include <cassert>
#include <any>
#include <complex>
#include <regex>
#include <charconv>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>

#include "utils/jam_Instance.h"
#include "utils/jam_SharedInstance.h"
#include "debug/jam_Log.h"
#include "utils/jam_HashMap.h"
#include "utils/jam_Bimap.h"
#include "utils/jam_Array.h"
#include "utils/jam_LookupEntry.h"
#include "utils/jam_LookupTable.h"
#include "utils/jam_Id.h"

#include "jam_Simd.h"

#include "utilities/jam_BitCast.h"
#include "utilities/jam_Union.h"

#include <generated/jam_Generated.h>

#if JUCE_MODULE_AVAILABLE_juce_audio_processors
#include <juce_audio_processors/juce_audio_processors.h>
#endif

#include <ProjectInfo.h>
#include "misc/jam_UUID.h"
#include "utilities/jam_ToInt.h"
#include "utilities/jam_Size.h"
#include "utilities/jam_Bounds.h"
#include "utilities/jam_Codepoint.h"
#include "text/jam_Strings.h"
#include "text/jam_Format.h"
#include "atomic/jam_Atomic.h"
#include "utilities/jam_Math.h"
#include "utilities/detail/jam_IsHashable.h"
#include "utilities/jam_Owner.h"
#include "utilities/jam_AnyOwner.h"
#include "utilities/jam_Hash.h"
#include "utilities/jam_AnyMap.h"
#include "utilities/jam_PluginHost.h"
#include "misc/jam_Validator.h"

#if JUCE_WINDOWS
#include "utilities/jam_Platform.h"
#endif

#include "function_map/jam_Function.h"
#include "utilities/jam_SharedResource.h"
#include "utilities/jam_SharedResources.h"
#include "text/jam_Grapheme.h"
#include "text/jam_AttributedChar.h"
#include "text/jam_Stamp.h"
#include "binary_data/jam_Raw.h"
#include "document/jam_Document.h"
#include "document/jam_CodeDocument.h"
#include "binary_codec/jam_BinaryCodec.h"
#include "file/jam_File.h"
#include "file/jam_Listener.h"

#include "value/jam_Value.h"
#include "utilities/jam_Decibels.h"
#include "utilities/jam_Frequency.h"
#include "utilities/jam_Taper.h"
#include "utilities/jam_URL.h"
#include "xml/jam_XML.h"
#include "xml/jam_XmlValidator.h"

#include "buffer/jam_Buffer.h"
#include "buffer/jam_Block.h"
#include "buffer/jam_Resizer.h"

#include "concurrency/jam_Mailbox.h"
#include "concurrency/jam_BufferSPSC.h"
