/**
 * @file pngpriv.h
 * @brief Forwarding shim resolving the vendored SIMD defilter sources'
 *        hardcoded `#include "../pngpriv.h"` against JUCE's real private
 *        libpng header.
 *
 * The vendored intel/arm sources (intel_init.c, filter_sse2_intrinsics.c,
 * arm_init.c, filter_neon_intrinsics.c, palette_neon_intrinsics.c) each
 * hardcode `#include "../pngpriv.h"`, matching upstream libpng's intel/ and
 * arm/ subdirectory layout. Those five files are not modified; this shim
 * sits one directory above pnglib/ so their relative include resolves,
 * forwarding to the real private header reachable via the JUCE module root
 * include path.
 */

#pragma once

#include <juce_graphics/image_formats/pnglib/pngpriv.h>
