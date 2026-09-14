/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION
   ID:                          jam_dsp
   vendor:                      JRENG! Architectural Modules
   version:                     0.0.1
   name:                        JAM DSP
   description:                 DSP processors — filters, waveshaping, hysteresis modelling, transient control, oversampling, FIR, noise generation, spectrum analysis
   website:                     https://jrengmusic.com
   license:                     Proprietary
   dependencies:                juce_dsp,
                                jam_core
   OSXFrameworks:
   iOSFrameworks:
 END_JUCE_MODULE_DECLARATION
 *******************************************************************************/

/**
 * @file jam_dsp.h
 * @brief jam DSP module — filters, waveshaping, hysteresis modelling, transient
 *        control, oversampling, FIR, noise generation, and spectrum analysis.
 *
 * Submodule include order: audio block / dry-wet mixer utilities -> filters
 * (Filter base, Orfanidis, RBJ, StateVariable, Butterworth, Linkwitz-Riley
 * splitter, brick-wall) -> transient control -> hysteresis (Saturator,
 * StateSpace, Preisach) -> waveshaper (Atan) -> preamp (SRPP) -> purest gain
 * -> noise generator -> channel utilities -> spectrum analyzer (FIFO,
 * processor) -> smoothing utilities (state transition, chain) -> FIR
 * (base, direct form, halfband polyphase) -> oversampler.
 */
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <jam_core/jam_core.h>
#include "utilities/jam_AudioBlock.h"
#include "utilities/jam_DryWetMixer.h"
#include "filter/jam_Filter.h"
#include "filter/jam_Orfanidis.h"
#include "filter/jam_RBJ.h"
#include "filter/jam_StateVariable.h"
#include "filter/jam_Butterworth.h"
#include "filter/jam_LinkwitzRiley3BandSplitter.h"
#include "filter/jam_BrickWall.h"
#include "transient_control/jam_TransientControl.h"
#include "hysteresis/jam_Saturator.h"
#include "hysteresis/jam_StateSpace.h"
#include "hysteresis/jam_Preisach.h"
#include "waveshaper/jam_Atan.h"
#include "preamp/jam_SRPP.h"
#include "purest_gain/jam_PurestGain.h"
#include "noise_generator/jam_Noise.h"
#include "utilities/jam_Channel.h"
#include "utilities/jam_ChannelOpsBase.h"
#include "analyzer/jam_SpectrumFIFO.h"
#include "analyzer/jam_SpectrumProcessor.h"
#include "utilities/jam_SmoothStateTransition.h"
#include "utilities/jam_SmoothChain.h"

#include "fir/jam_FIR.h"
#include "fir/jam_DirectForm.h"
#include "fir/jam_HalfbandPolyphase.h"
#include "oversampler/jam_Oversampler.h"
//#include "utilities/jam_Oversampling.h"

//#include "engine/jam_TrinsientAnalogModel_V1.h"
//#include "engine/jam_TrinsientAnalogModel_V2.h"
