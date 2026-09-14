/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION
   ID:                          jam_web
   vendor:                      JRENG! Architectural Modules
   version:                     0.0.1
   name:                        JAM Web
   description:                 HTML authored-subset and CSS Syntax Level 3 subset tokenizers and parsers
   website:                     https://jrengmusic.com
   license:                     Proprietary
   dependencies:                juce_core,
                                jam_core,
   OSXFrameworks:
   iOSFrameworks:
   searchpaths:                 .
 END_JUCE_MODULE_DECLARATION
 *******************************************************************************/

/**
 * @file jam_web.h
 * @brief Module header aggregating all jam_web submodule includes.
 */

#pragma once

#include <juce_core/juce_core.h>
#include <jam_core/jam_core.h>

#include "css/jam_Css.h"
#include "css/jam_CssValidator.h"
#include "html/jam_Html.h"
#include "html/jam_HtmlValidator.h"
