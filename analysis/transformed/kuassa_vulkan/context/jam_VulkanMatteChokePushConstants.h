/**
 * @file jam_VulkanMatteChokePushConstants.h
 * @brief Push-constant block for the matte_choke.comp compute shader.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/** @brief Push-constant layout for ComputeID::matteChoke — matches the
 *  MatteChokePC block declared in shaders/matte_choke.comp (16 bytes). */
struct VulkanMatteChokePushConstants
{
    int width;               ///< Source/destination image width in pixels.
    int height;              ///< Source/destination image height in pixels.
    uint32_t sourceTextureIndex; ///< Bindless array slot of the source texture.
    int radius;              ///< Erosion radius in pixels.
};

static_assert (sizeof (VulkanMatteChokePushConstants) == 16, "VulkanMatteChokePushConstants must be 16 bytes (matches shaders/matte_choke.comp's MatteChokePC block)");

/** @brief Shared 2D workgroup edge dimension (16 × 16 tiles) — paired with
 *  local_size_x and local_size_y in both matte_choke.comp and matte_feather.comp.
 *  VulkanImageEffects::applyMatteChoke() and applyMatteFeather() divide the
 *  source image dimensions by this value to compute their dispatch group counts. */
static constexpr int matteWorkgroupSize { 16 };

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
