// Direct2D helpers header needs HWND declared first (JUCE_CORE_INCLUDE_NATIVE_HEADERS)
// — mirrors jam_vulkan.mm's own top-of-TU CoreGraphics-helpers defines for mac.
// Guarded on the raw compiler macro rather than JUCE_WINDOWS: JUCE_WINDOWS is a
// JUCE-defined macro (juce_core/system/juce_TargetPlatform.h, derived from these
// same _WIN32/_WIN64 primitives) not yet defined this early — before jam_vulkan.h's
// own juce_gui_basics.h include below, which is the include this gate must precede.
#if defined(_WIN32) || defined(_WIN64)
#define JUCE_CORE_INCLUDE_NATIVE_HEADERS 1
#define JUCE_GRAPHICS_INCLUDE_DIRECT2D_HELPERS 1
#define JUCE_CORE_INCLUDE_COM_SMART_PTR 1
#endif

#include "jam_vulkan.h"
#include "blur/jam_ImageMatte.cpp"
#include "blur/jam_Blur.cpp"
#include "blur/jam_DropShadow.cpp"
#include "blur/jam_InnerShadow.cpp"
#if JAM_VULKAN_RUNTIME_SHADER_COMPILER
#include "bimap/jam_VulkanShaderFormat.cpp"
#endif
#include "allocator/jam_VulkanAllocator.cpp"
#if JAM_VULKAN_RUNTIME_SHADER_COMPILER
#include "shader/jam_VulkanShaderReflection.cpp"
#include "shader/jam_VulkanShaderPreset.cpp"
#include "shader/jam_VulkanShaderCompiler.cpp"
#endif
#if JUCE_WINDOWS
#include "context/jam_VulkanCompositionDevice.cpp"
#include "context/jam_VulkanComposition.cpp"
#endif
#include "device/jam_VulkanDevice.cpp"
#include "resource/jam_VulkanUploadHelpers.cpp"
#include "font/jam_GlyphAtlasCore.cpp"
#include "font/jam_GlyphAtlasComposite.cpp"
#include "font/jam_GlyphAtlasEdgeTable.cpp"
#include "font/jam_GlyphAtlasFreeType.cpp"
#include "font/jam_GlyphAtlasNative.cpp"
#include "font/jam_GlyphAtlasCache.cpp"
#include "font/jam_GlyphAtlasGpuMirror.cpp"
#include "font/jam_GlyphArrangement.cpp"
#include "font/jam_LowLevelGraphicsGlyphRenderer.cpp"
#include "font/jam_GlyphConstraintTable.cpp"
#include "resource/jam_VulkanFrameBuffer.cpp"
#include "resource/jam_VulkanPrimitiveRecordBuffer.cpp"
#include "resource/jam_VulkanTransparencyStack.cpp"
#include "resource/jam_VulkanStagingArena.cpp"
#include "resource/jam_VulkanWindingScratch.cpp"
#include "context/jam_VulkanPipelines.cpp"
#include "context/jam_VulkanPipelinesState.cpp"
#if JAM_VULKAN_RUNTIME_SHADER_COMPILER
#include "resource/jam_VulkanShaderInstance.cpp"
#include "resource/jam_VulkanShaderInstanceSlang.cpp"
#include "resource/jam_VulkanShaderInstanceMesh.cpp"
#endif
#include "resource/jam_VulkanTextureCache.cpp"
#include "resource/jam_VulkanBindlessRegistry.cpp"
#include "context/jam_VulkanSwapchain.cpp"
#include "context/jam_VulkanMsaaCalibration.cpp"
#include "context/jam_VulkanMsaaCalibrationPipeline.cpp"
#include "context/jam_VulkanMsaaCalibrationMeasurement.cpp"
#include "context/jam_VulkanImageEffects.cpp"
#if JAM_VULKAN_RUNTIME_SHADER_COMPILER
#include "context/jam_VulkanShaderRegistry.cpp"
#endif
#include "context/jam_VulkanGraphics.cpp"
#include "context/jam_VulkanGraphicsSetupRenderPass.cpp"
#include "context/jam_VulkanGraphicsSetupDrawState.cpp"
#include "context/jam_VulkanGraphicsSetupSceneTarget.cpp"
#include "context/jam_VulkanLowLevelGraphicsContext.cpp"
#include "context/jam_VulkanLowLevelGraphicsContextImage.cpp"
#include "context/jam_VulkanLowLevelGraphicsContextPath.cpp"
#include "context/jam_VulkanLowLevelGraphicsContextTransparency.cpp"
#include "context/jam_VulkanLowLevelGraphicsContextGlyph.cpp"
#include "resource/jam_VulkanImageRenderer.cpp"
#if JAM_VULKAN_RUNTIME_SHADER_COMPILER
#include "context/jam_VulkanGraphicsSlangPass.cpp"
#include "context/jam_VulkanShaderRegistryMesh.cpp"
#include "context/jam_VulkanLowLevelGraphicsContextRender.cpp"
#endif
