/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION
  ID:                           jam_animation
  vendor:                       JRENG! Architectural Modules
  version:                      0.0.1
  name:                         JAM Animation
  description:                  Foundation animation classes (Animator, AnimationBase, AnimationScrollingText)
  website:                      https://jrengmusic.com
  license:                      Proprietary
  dependencies:                 juce_gui_basics,
                                jam_graphics,
  OSXFrameworks:
  iOSFrameworks:
 END_JUCE_MODULE_DECLARATION
*******************************************************************************/

/**
 * @file jam_animation.h
 * @brief Module header aggregating all jam_animation submodule includes.
 */

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <jam_graphics/jam_graphics.h>
#include "utilities/jam_BasicPerlinNoise.h"
#include "fade/jam_Animator.h"
#include "base/jam_AnimationBase.h"
#include "logo/jam_AnimationLogo.h"
#include "scrolling_text/jam_AnimationScrollingText.h"
#include "strip/jam_AnimationStrip.h"
#include "scrambled_text/jam_AnimationScrambledText.h"
