/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION
   ID:                           jam_graphics
   vendor:                       JRENG! Architectural Modules
   version:                      0.0.1
   name:                         JAM Graphics
   description:                  Graphics utilities, blur, shadows, colours, fonts, mesh
   website:                      https://jrengmusic.com
   license:                      Proprietary
   dependencies:                 juce_core,
                                  juce_data_structures,
                                  juce_graphics,
                                  juce_gui_basics,
                                  jam_core,
  OSXFrameworks:                  Accelerate,
                                  CoreText,
                                  CoreFoundation,
                                  CoreGraphics,
                                  ApplicationServices,
 END_JUCE_MODULE_DECLARATION
 *******************************************************************************/

/**
 * @file jam_graphics.h
 * @brief Graphics module — colour maps, SVG, glyph/text primitives, geometry,
 *        images, mesh loaders, and drawable utilities.
 */

#pragma once
#include <csetjmp>
#include <cstring>
#include <deque>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <jam_core/jam_core.h>

// =========================================================================
// Image — binary-resource-backed juce::Image lookup (Id::png/Id::jpg,
// jam_core lexicon, already available via jam_core included above); no
// dependency on anything else below, so it precedes the list below.
// =========================================================================

#include "image_formats/jam_ImageLoader.h"
#include "images/jam_ImageTransition.h"

// =========================================================================
// text/, geometry/ — atomic display unit (must precede Stamp and glyph types)
// =========================================================================

#include "geometry/jam_Cell.h"
#include "geometry/jam_CellPoint.h"
#include "geometry/jam_CellRectangle.h"
#include "text/jam_Row.h"
#include "text/jam_ShapedTextOptions.h"

// =========================================================================
// Hyperlink — hyperlink document carrier table (hyperlinkId → uri/id)
// =========================================================================

#include "text/jam_Hyperlink.h"

// =========================================================================
// ColourScheme — ValueTree-mirrored AnyMap colour registry (must precede SVG)
// =========================================================================

#include "colour_scheme/jam_ColourScheme.h"

// =========================================================================
// AttributedString — must precede AttributedGraphics.h (AttributedText::text
// is a jam::AttributedString member)
// =========================================================================

#include "fonts/jam_AttributedString.h"

// =========================================================================
// SVG
// =========================================================================

#include "svg/jam_Svg.h"

#include "svg/jam_AttributedGraphics.h"

// =========================================================================
// Graphics utilities
// =========================================================================

#include "geometry/jam_Rectangle.h"
#include "utilities/jam_Drawable.h"
#include "colours/jam_ColoursUtilities.h"

// Rotary/Perimeter geometry (depend on Points/Lines, Drawable, Svg::Format above)
#include "geometry/jam_Rotary.h"
#include "geometry/jam_Perimeter.h"
#include "geometry/jam_Logo.h"

// =========================================================================
// Fonts — SIMD blending (jam_GlyphAtlas.h stays in jam_vulkan/font/ — it owns
// a GPU-resident mirror that jam_graphics cannot depend on without inverting
// the jam_graphics/jam_vulkan module arrow). jam::GlyphArrangement stays
// there too, for the same GlyphAtlas dependency; see
// jam_vulkan/font/jam_GlyphArrangement.h.
//
// jam::Typeface (identity table + per-Typeface reverse cmap) lives here —
// both jam_vulkan (GlyphArrangement's forward cmap shaping) and jam_terminal
// (terminal::GraphicsContext::drawGlyphs' reverse glyph->codepoint lookup)
// consume it, and jam_terminal does not depend on jam_vulkan. <hb.h> precedes
// it (vendored under JUCE's own juce_graphics/fonts/harfbuzz/, include path
// added by AppBuilder.cmake/PluginBuilder.cmake's "harfbuzz include path"
// block, mirroring jam_vulkan's own hb.h include).
// =========================================================================
#include <hb.h>
#include "fonts/jam_Typeface.h"
#include "fonts/jam_SimdBlend.h"
#include "utilities/jam_GraphicsUtils.h"

// =========================================================================
// Mesh — Earcut single-ring triangulation (relocated from the former
// jam_vulkan/earcut/: pure geometry, no vk:: dependency, so it can serve both
// the Vulkan LLGC path and
// jam::WavefrontObj's n-gon triangulation below) + Wavefront OBJ/MTL mesh
// loader — both stay juce_core-family only (no vk::, no 3D-math),
// so the native Vulkan mesh path (jam_vulkan already depends on jam_graphics)
// and a future runtime-JS asset seam can consume the same parsed mesh data.
// =========================================================================

#include "mesh/jam_Earcut.h"
#include "mesh/jam_WavefrontObj.h"
