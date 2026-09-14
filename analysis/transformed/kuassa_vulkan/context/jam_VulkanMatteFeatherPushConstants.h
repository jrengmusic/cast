/**
 * @file jam_VulkanMatteFeatherPushConstants.h
 * @brief Push-constant block for the matte_feather.comp compute shader.
 */
#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/** @brief Push-constant layout for ComputeID::matteFeather — matches the
 *  MatteFeatherPC block declared in shaders/matte_feather.comp (20 bytes). */
struct VulkanMatteFeatherPushConstants
{
    int width;               ///< Source/destination image width in pixels.
    int height;              ///< Source/destination image height in pixels.
    uint32_t sourceTextureIndex; ///< Bindless array slot of the source texture.
    float radius;            ///< Feather radius in pixels.
    float curve;             ///< Exponent applied to the distance-to-alpha mapping.
};

static_assert (sizeof (VulkanMatteFeatherPushConstants) == 20);

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
