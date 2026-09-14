// CoreGraphics helpers header needs native Cocoa/CoreGraphics types; JUCE's own
// graphics/gui_basics module TUs set these same defines before their umbrella include.
#define JUCE_CORE_INCLUDE_NATIVE_HEADERS 1
#define JUCE_CORE_INCLUDE_OBJC_HELPERS 1
#define JUCE_GRAPHICS_INCLUDE_COREGRAPHICS_HELPERS 1

#include "jam_vulkan.cpp"

#if JUCE_MAC
#import <Cocoa/Cocoa.h>
#import <Accelerate/Accelerate.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#import <CoreText/CoreText.h>

#include "context/native/jam_VulkanMetal_mac.mm"
#include "font/native/jam_LowLevelGraphicsGlyphRenderer_mac.mm"
#include "font/native/jam_GlyphAtlas_mac.mm"

#endif
