/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION
   ID:                           jam_markdown
   vendor:                       JRENG! Architectural Modules
   version:                      0.0.1
   name:                         JAM Markdown
   description:                  Clean-room native CommonMark + GFM markdown parsing and rendering
   website:                      https://jrengmusic.com
   license:                      Proprietary
   dependencies:                 juce_core,
                                 jam_core,
                                 juce_gui_basics
  END_JUCE_MODULE_DECLARATION
 *******************************************************************************/
#pragma once
#include <juce_core/juce_core.h>
#include <jam_core/jam_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#if __has_include (<hb.h>)
#include <hb.h>
#endif

// Config: gates MermaidDiagram construction and drawing in the layout and document dispatch tables
#ifndef JAM_MARKDOWN_MERMAID
#define JAM_MARKDOWN_MERMAID 0
#endif

#if JUCE_MODULE_AVAILABLE_jam_mermaid_diagram && JAM_MARKDOWN_MERMAID
#include <jam_mermaid_diagram/jam_mermaid_diagram.h>
#endif

#include "document/jam_MarkdownDocument.h"
#include "document/jam_MarkdownValidator.h"
#include "document/jam_MarkdownWriter.h"

#include "widget/jam_MarkdownComponent.h"
