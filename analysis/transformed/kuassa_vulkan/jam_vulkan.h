/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION
   ID:                          jam_vulkan
   vendor:                      Kuassa
   version:                     0.0.1
   name:                        JAM Vulkan
   description:                 Vulkan rendering backend
   website:                     https://jam.com
   license:                     Proprietary
   dependencies:                juce_graphics, juce_gui_basics, jam_core, jam_graphics, jam_freetype, jam_data_structures
   OSXFrameworks:               QuartzCore Metal IOSurface
   LinuxLibs:                   vulkan
   windowsLibs:                 vulkan-1 d3d11 dxgi dcomp
  END_JUCE_MODULE_DECLARATION
 *******************************************************************************/

/**
 * @file jam_vulkan.h
 * @brief Vulkan rendering backend module — Vulkan/MoltenVK/glm/FreeType/HarfBuzz
 *        includes, VMA allocator, GPU probe, blur, shader compilation, glyph
 *        atlas/typeface/text-rendering, and the unified VulkanEngine resource tree.
 */

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <jam_core/jam_core.h>
#include <jam_graphics/jam_graphics.h>
#include <jam_freetype/jam_freetype.h>
// jam_data_structures — provides jam::Model, bimap/jam_VulkanShaderFormat.cpp's
// directory-to-ValueTree reader.
#include <jam_data_structures/jam_data_structures.h>

/** Config: JAM_VULKAN_RUNTIME_SHADER_COMPILER
    Enables the runtime GLSL/slang -> SPIR-V shader compiler (shaderc), SPIR-V
    reflection (SPIRV-Cross), and every consumer built on them (VulkanShaderComponent,
    render(), the post-process chain).
*/
#ifndef JAM_VULKAN_RUNTIME_SHADER_COMPILER
#define JAM_VULKAN_RUNTIME_SHADER_COMPILER 0
#endif

// Vulkan headers — platform detection before including.
// VK_USE_PLATFORM_METAL_EXT is defined by MoltenVK/mvk_vulkan.h (the canonical definer).
#if JUCE_MAC
// (Metal extension surface comes from MoltenVK below)
#elif JUCE_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#elif JUCE_LINUX
#define VK_USE_PLATFORM_XLIB_KHR
#endif

// MoltenVK — Apple Vulkan-on-Metal implementation (headers pinned under the
// shared Poems/Vulkan/macOS/include/ tree, resolved via the ${VULKAN_PATH}
// BEFORE include root added by AppBuilder.cmake)
#if __APPLE__
#include <MoltenVK/mvk_vulkan.h>
#endif

// vulkan-hpp configuration — no exceptions on the API surface (Result-returning
// overloads are used on all setup paths instead); the general assertion hook is
// routed to jassert (JUCE's own assert, matching this codebase's assertion
// policy everywhere else), but the per-call result assertion is disabled: a
// vk::Result is this module's API surface, consumed by the FrameDisposition
// lookups and Result checks at the call sites — asserting inside the vulkan.hpp
// wrapper would trap before the handler runs. Plain vk:: types throughout
// jam_vulkan (no RAII wrappers).
#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_ASSERT jassert
#define VULKAN_HPP_ASSERT_ON_RESULT(expression) ((void) 0)

// vulkan-hpp — pinned under the shared Poems/Vulkan/macOS/include/vulkan/
// tree, resolved via the ${VULKAN_PATH} BEFORE include root added by
// AppBuilder.cmake
#include <vulkan/vulkan.hpp>

// =========================================================================
// Vendored third-party headers (GLM)
// =========================================================================
// GLM_FORCE_DEPTH_ZERO_TO_ONE: required so glm::perspective()/glm::ortho()
// below produce Vulkan-clip-space-correct matrices (NDC z in [0,1], not GL's
// [-1,1]). GLM_FORCE_LEFT_HANDED is deliberately NOT defined — right-handed
// is glm's own default, and is the correct chirality for jam::
// VulkanOrbitCamera (resource/jam_VulkanOrbitCamera.h), the ONLY consumer of real
// 3D perspective/view math in this module: glm::lookAtLH()/lookAtRH() with
// identical eye/center/up parameters produce a MIRRORED (x-axis-flipped)
// view basis relative to each other (glm/include/ext/matrix_transform.inl —
// lookAtRH's s = normalize(cross(f,up)), lookAtLH's s = normalize(cross(up,f))
// = -s_RH; only the right/x basis vector flips, up stays identical, forward's
// sign convention flips to match — RH's own perspectiveRH_ZO/perspectiveLH_ZO
// pair (glm/include/ext/matrix_clip_space.inl) only ever corrects for THAT
// forward-sign difference, never for the x-basis flip). Combined with
// VulkanOrbitCamera::getProjectionMatrix()'s own necessary Y-flip (see that
// method's doc comment), the LEFT_HANDED configuration this module
// previously forced produced a true mirror (odd total axis inversions:
// LH's own x-flip + the Y-flip) — this was the exact root cause of the
// "3D text reads backward" mirrored-mesh render bug. Right-handed + the
// same Y-flip leaves exactly the one necessary correction (Y only), the
// standard non-mirroring Vulkan+glm convention (Sascha Willems' own
// documented approach).
//
// The two glm::ortho() 2D screen-transform call sites (jam_VulkanLowLevelGraphicsContext.h
// ctor, jam_VulkanGraphics.cpp endFrame()'s compositeProjection) are
// unaffected by this change, independently re-verified for handedness
// specifically (not just depth convention, already verified below): both
// call the explicit-near/far 6-argument glm::ortho(), which dispatches to
// orthoLH_ZO()/orthoRH_ZO() — the two differ ONLY in Result[2][2] (the Z
// term; glm/include/ext/matrix_clip_space.inl), X/Y terms are byte-identical
// between them. Both call sites pass near=-1.0f/far=1.0f and are consumed by
// shader stages whose vertex position is ALWAYS z=0.0 (shaders/instanced.vert:46,
// every other 2D shader) into render passes carrying NO depth/stencil test
// (jam_VulkanGraphicsSetupRenderPass.cpp's 3 main passes are stencil-only,
// jam_VulkanShaderInstance.cpp's fullscreen pipelines set noStencilState())
// — so ortho()'s own Z-term convention is never read by anything, for either
// handedness. The one other glm:: site (VulkanShaderReflection.cpp's glm::mat4
// identityMvp) is a convention-agnostic identity matrix. Removing
// GLM_FORCE_LEFT_HANDED therefore changes zero existing 2D rendered output.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>// glm::inverseTranspose — jam::VulkanOrbitCamera's normal-matrix computation

// FreeType — vendored under jam_freetype/freetype/ (module dependency above adds its
// include/ to the header search path). Included here, once, at the topmost module
// header — mirrors jam_graphics.h's identical precedent (its own jam_freetype
// dependency + direct <ft2build.h> include ahead of any submodule header that needs
// FT_Library/FT_Face) — GlyphAtlas below needs both types complete for its FreeType
// registration API and rasterize() path.
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H
#include FT_OUTLINE_H// FT_Outline_Embolden — the embolden path (jam_GlyphAtlasFreeType.cpp)

// HarfBuzz — vendored under JUCE's own juce_graphics/fonts/harfbuzz/ (include
// path added for this module's target by cmake — see AppBuilder.cmake's
// "jam_vulkan — harfbuzz include path" block). Included here, once, at the
// topmost module header — mirrors the FreeType include block immediately
// above. jam_graphics/fonts/jam_Typeface.h's per-Typeface hb_font_t
// construction (jam_graphics.h, included above, already brings this in
// transitively) and font/jam_GlyphArrangement.h's tryLigature() hb_shape
// call both need this complete ahead of their own include lines below.
#include <hb.h>

// =========================================================================
// Standard library — used by pipeline cache I/O + texture cache
// =========================================================================
#include <filesystem>
#include <fstream>
// VMA include anchor — declarations only here; VMA_IMPLEMENTATION lives in
// jam_vulkan_allocator.cpp (compiled once, no ODR violation).
#include "allocator/jam_VulkanAllocator.h"

// Module headers (submodule files include NOTHING)
// jam::vulkan-scoped vocabulary (RetroArch .slangp manifest key/scale-type/
// wrap-mode vocabulary + .slang fixed-vocabulary member/#pragma spellings) now lives at
// lexicon/jam_vulkan.md, generated into jam_lexicon/generated/jam_Lexicon.h's Id::
// namespace — jam_core.h (included above) already brings this in transitively via
// jam_lexicon; bimap/jam_VulkanShaderFormat.h below (VulkanShaderFormat) populates every one of
// its maps from these Id:: members.
#include "blur/jam_SimdStackBlur.h"// jam:: in-place ARGB/single-channel stack blur — no deps
#include "blur/jam_Blur.h"// Blurs a source juce::Image into one owned destination — delegates to VulkanEngine::blurImage
#include "blur/jam_ImageMatte.h"// Clips a buffered image by an alpha mask with optional feather (edge softening)
#include "blur/jam_ShadowMask.h"// Cached rasterized coverage mask shared by DropShadow/InnerShadow — no deps
#include "blur/jam_DropShadow.h"// Cached path-mask drop shadow — uses ShadowMask, SimdStackBlur above
#include "blur/jam_InnerShadow.h"// Cached path-mask inner shadow — uses ShadowMask (jam_ShadowMask.h above) and SimdStackBlur
#include "resource/jam_VulkanBuffer.h"// RAII buffer wrapper — no deps
#if JAM_VULKAN_RUNTIME_SHADER_COMPILER
#include "resource/jam_VulkanVertex.h"// GPU interleaved mesh vertex POD ({position,normal,uv}, 32 bytes) — no deps; jam::VulkanMesh below interleaves jam::WavefrontObj shapes into this record
#endif
#include "resource/jam_VulkanImage.h"// RAII image wrapper — no deps
#include "context/jam_VulkanColour.h"// Normalised RGBA colour — no deps (included before VulkanPrimitiveRecord, which uses it as a field type)
#if JAM_VULKAN_RUNTIME_SHADER_COMPILER
#include "shader/jam_VulkanShaderUniforms.h"// Shadertoy-compatible push-constant block stamped per VulkanShader execution — no deps, pure data; precedes VulkanShader below, whose ctor assert references VulkanShaderUniforms::maxChannelCount
#include "shader/jam_VulkanShaderPass.h"// One compiled shader pass (name + SPIR-V) — no deps, pure data; precedes VulkanShader below, whose pass chain is built from it
#include "bimap/jam_VulkanShaderFormat.h"// Bimap dispatching shader source format -> wrapper template BinaryData filename, manifest extension, canon pass-name vocabulary, the .slangp manifest key/scale-type/wrap-mode vocabulary (RetroArch's own keys plus the host application's mesh extension — the manifest serves BOTH formats), and directory-to-ValueTree reader — depends on VulkanShaderUniforms::maxChannelCount (included above) and lexicon/jam_vulkan.md's Id:: vocabulary (jam_core.h, included at the top of this file); precedes VulkanShaderPreset below, whose Pass fields default-initialize from VulkanShaderFormat's own enumerators, and VulkanShader below, whose format field is a plain int (VulkanShaderFormat's own enumerators)
#include "shader/jam_VulkanShaderPreset.h"// Parsed RetroArch .slangp preset directives (per-pass scale/filter/wrap/framebuffer-format/frame-count-modulo/alias + top-level parameter overrides) — no deps beyond jam::HashMap/std::optional/jam::Array, pure data; precedes VulkanShader below, whose preset field is this type
#include "shader/jam_VulkanShader.h"// Compiled multi-pass shader description (SPIR-V per pass) — depends on VulkanShaderUniforms::maxChannelCount, VulkanShaderFormat's own enumerators, and VulkanShaderPreset
#include "shader/jam_VulkanShaderReflection.h"// SPIR-V reflection (SPIRV-Cross) -- UBO/push_constant member + sampled-image resource introspection for one compiled shader stage — no deps, pure data + functions
#include "shader/jam_VulkanShaderTextureBindings.h"// Resolved slang-pass texture name -> vk::ImageView/vk::Extent2D pair — no deps beyond jam::HashMap, pure data
#include "shader/jam_VulkanShaderCompiler.h"// Shadertoy/slang GLSL -> SPIR-V compile unit — uses VulkanShader, VulkanShaderFormat, VulkanShaderUniforms::maxChannelCount, map::ImageResample::value (jam_lexicon, included transitively via jam_core.h)
#endif
#include "context/jam_VulkanPrimitiveRecord.h"// Plain POD SSBO record — no deps (colocated in context/ alongside its consumers rather than resource/; included early since VulkanPrimitiveRecordBuffer needs it)
#include "resource/jam_VulkanPrimitiveRecordBuffer.h"// Per-frame growable SSBO of VulkanPrimitiveRecord — uses VulkanBuffer, VulkanPrimitiveRecord
#if JUCE_WINDOWS
#include <d3d11_4.h>
#include <dxgi1_3.h>
#include <dcomp.h>
#include <dwrite_3.h>// IDWriteFactory5/IDWriteInMemoryFontFileLoader — font/jam_GlyphAtlas.h's directWriteFactory/inMemoryFontFileLoader members
#include "context/jam_VulkanCompositionDevice.h"// Shared per-adapter DirectComposition device — uses juce::ComSmartPtr; precedes device/jam_VulkanDevice.h below, which owns one
#include "context/jam_VulkanSharedTextureExport.h"// RAII NT shared-texture export handle — no deps; precedes VulkanComposition below, whose textureExport field is this type
#include "context/jam_VulkanComposition.h"// Windows DirectComposition present leg — uses VulkanCompositionDevice, VulkanSharedTextureExport
#endif
#include "device/jam_VulkanDevice.h"// Shared device — uses VulkanBuffer
#include "context/jam_VulkanPipelineCache.h"// Engine-owned vk::PipelineCache RAII holder — uses VulkanDevice; precedes VulkanEngine, which owns one
#include "resource/jam_VulkanUploadHelpers.h"// Shared recordUploadBarrier/createStagingBuffer — uses VulkanDevice, VulkanBuffer; used by VulkanBindlessTexture and VulkanGraphics's own texture cache below
#if JAM_VULKAN_RUNTIME_SHADER_COMPILER
#include "resource/jam_VulkanMesh.h"// VulkanDevice-local vertex-pulling SSBO + index VulkanBuffer, interleaved/staged-uploaded once from jam::WavefrontObj — uses VulkanBuffer, VulkanVertex, VulkanDevice, createStagingBuffer, jam::WavefrontObj (jam_graphics.h, included above); window-agnostic (no VulkanGraphics dep, unlike VulkanBindlessTexture below)
#include "resource/jam_VulkanOrbitCamera.h"// Pure math+interaction camera (perspective/view/normal matrices + AABB auto-fit) — uses glm only, no vk:: handles
#endif
#include "resource/jam_VulkanBindlessTexture.h"// VulkanImage + per-window bindless registry + staging upload — uses VulkanImage, VulkanDevice, recordUploadBarrier; the SSOT upload path GlyphAtlas below consumes
#include "font/jam_GlyphAtlas.h"// CPU+GPU glyph atlas (bare jam:: namespace, colocated here for its GPU-mirror dependency on VulkanDevice/VulkanBindlessTexture) — must precede VulkanPipelines/VulkanGraphics/LLGC, all of which reference jam::GlyphAtlas::Type
// jam::Typeface (juce::Typeface::Ptr + hb_font_t identity table) now lives in
// jam_graphics/fonts/jam_Typeface.h — jam_terminal needs it too and does not
// depend on jam_vulkan; already in scope transitively via jam_graphics.h,
// included above.
#include "font/jam_BoxDrawing.h"// Procedural box/block/braille rasterizer (ported, ready — not yet wired into GlyphAtlas::rasterize(), see file doc comment) — no deps
#include "font/jam_GlyphArrangement.h"// Monospace shaping engine (restored old glyph::Arrangement semantics) — uses Typeface above + AttributedChar/Stamp/Grapheme (jam_graphics.h)
#include "font/jam_PendingCellRun.h"// Per-glyph cell-run context companion of LowLevelGraphicsGlyphRenderer below — no deps
#include "font/jam_CachedState.h"// Saved transform/fill/clip state companion of LowLevelGraphicsGlyphRenderer below — no deps
#include "font/jam_LowLevelGraphicsGlyphRenderer.h"// CPU fallback LLGC (bare jam:: namespace, uses GlyphAtlas, PendingCellRun, CachedState above) — must precede VulkanEngine, which constructs it
#include "resource/jam_VulkanFrameBuffer.h"// Per-frame VB/IB — uses VulkanBuffer
#include "resource/jam_VulkanTransparencyStack.h"// Offscreen transparency targets — uses VulkanDevice, VulkanImage
#include "resource/jam_VulkanStagingArena.h"// Per-frame growable staging buffer arena backing VulkanGraphics::allocateStaging() — uses VulkanDevice, VulkanBuffer, createStagingBuffer
#include "resource/jam_VulkanWindingScratch.h"// Isolated winding-rule scratch target — uses VulkanDevice, VulkanImage
#if JAM_VULKAN_RUNTIME_SHADER_COMPILER
#include "resource/jam_VulkanRenderResources.h"// Count-parameterized offscreen render target(s) — uses VulkanImage; VulkanShaderInstance's buffer-pass/gather-target building block
#include "resource/jam_VulkanSlangPassResources.h"// One slang VulkanShaderPass's own reflected descriptor set/UBO/pipeline layout — uses VulkanBuffer, VulkanShaderReflection; VulkanShaderInstance's slang-only building block
#include "resource/jam_VulkanShaderInstance.h"// Per-content-hash VulkanShader GPU execution cache — uses VulkanDevice, VulkanImage, VulkanShader, VulkanShaderUniforms, VulkanRenderResources, VulkanSlangPassResources, VulkanShaderTextureBindings
#endif
#include "jam_VulkanShaderData.h"// Generated SPIR-V binary data — at global scope (symbols in jam namespace, included before VulkanPipelines.h)
#include "context/jam_VulkanPipelines.h"// Pipeline collection — uses vk::Device
#include "resource/jam_VulkanBindlessInstance.h"// This window's RAII glyph-atlas bindless-slot withdrawal — uses GlyphAtlas (above); VulkanGraphics below embeds one directly (bindlessInstance member)
#include "context/jam_VulkanImagePushConstants.h"// VulkanImage-pipeline push-constant layout + makeImagePushConstants() — uses VulkanColour; precedes VulkanGraphics/LLGC, both of which construct/push it
#include "context/jam_VulkanGradientPushConstants.h"// VulkanPipelines::ID::gradientFill push-constant layout — no deps
#include "context/jam_VulkanStackBlurPushConstants.h"// VulkanPipelines::ComputeID::stackBlurTexture/stackBlurBuffer push-constant layout — no deps
#include "context/jam_VulkanMatteChokePushConstants.h"// VulkanPipelines::ComputeID::matteChoke push-constant layout + matteWorkgroupSize (shared by matte_choke.comp and matte_feather.comp) — no deps
#include "context/jam_VulkanMatteFeatherPushConstants.h"// VulkanPipelines::ComputeID::matteFeather push-constant layout — no deps
#include "resource/jam_VulkanTextureCache.h"// Engine-owned device-lifetime image texture store (map + upload + dirty + content listener) — uses VulkanDevice, VulkanBindlessTexture; precedes VulkanGraphics, its per-window consumer
#include "resource/jam_VulkanBindlessRegistry.h"// This window's bindless array slot allocator + per-root juce::ImagePixelData::Listener recycling — uses juce::ImagePixelData, jam::Array, jam::HashMap; precedes VulkanGraphics, whose bindlessRegistry member is this type
#include "context/jam_VulkanSwapchain.h"// WSI presentation cluster (surface/swapchain/views/semaphores) — uses VulkanDevice, jam::Array; precedes VulkanGraphics, whose swapchain member is this type
#include "context/jam_VulkanMsaaCalibration.h"// Per-device MSAA sample-count calibration probe — uses VulkanDevice, VulkanImage, VulkanPipelines; precedes VulkanGraphics, whose msaaCalibration member is this type
#include "context/jam_VulkanImageEffects.h"// Bindless compute-driven blur/matte image effects — uses VulkanDevice, VulkanPipelines, VulkanBuffer; precedes VulkanGraphics, whose imageEffects member is this type
#if JAM_VULKAN_RUNTIME_SHADER_COMPILER
#include "context/jam_VulkanShaderRegistry.h"// Runtime-shader-compiler lane's own GPU execution registry — uses VulkanDevice, VulkanPipelines, VulkanShaderInstance, VulkanBindlessRegistry, VulkanStagingArena; precedes VulkanGraphics, whose shaderRegistry member is this type
#endif
#include "context/jam_VulkanGraphics.h"// Per-window graphics — uses VulkanDevice, VulkanBuffer, VulkanImage, VulkanFrameBuffer, VulkanPrimitiveRecordBuffer, VulkanPipelines, VulkanTransparencyStack, VulkanWindingScratch, VulkanSwapchain, GlyphAtlas, VulkanShaderInstance, VulkanBindlessInstance, VulkanBindlessRegistry, VulkanTextureCache, VulkanShaderRegistry
#include "context/jam_VulkanTransformState.h"// Namespace-scope transform accumulator, no deps; precedes LLGC (State::currentTransform's field type)
#include "context/jam_VulkanLowLevelGraphicsContext.h"// LLGC — uses VulkanGraphics, VulkanTransformState
#if JAM_VULKAN_RUNTIME_SHADER_COMPILER
#include "context/jam_VulkanRender.h"// render() — the shader draw injection seam; references VulkanLowLevelGraphicsContext
#include "shader/jam_VulkanShaderComponent.h"// Self-managed juce::Component rendering a compiled VulkanShader — uses render() above
#endif
#include "resource/jam_VulkanImageRenderer.h"// Engine-owned shared offscreen renderer — uses VulkanGraphics, VulkanLowLevelGraphicsContext; precedes VulkanEngine, which owns one
#include "engine/jam_VulkanEngine.h"// Unified resource-ownership tree — uses VulkanDevice, Typeface, Stamp, Grapheme, Hyperlink, GlyphAtlas, VulkanGraphics, VulkanImageRenderer, LLGC
#include "resource/jam_VulkanImagePixelData.h"// VulkanImagePixelData + VulkanImageType — juce::ImagePixelData/ImageType contract for Vulkan-backed images; uses VulkanImageRenderer, VulkanEngine, VulkanBindlessTexture, VulkanImage, VulkanBuffer, VulkanUploadHelpers
