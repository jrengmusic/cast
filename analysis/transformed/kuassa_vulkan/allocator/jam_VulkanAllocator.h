// Upstream VMA header. Includes Vulkan declarations only — function bodies live in
// the .cpp (VMA_IMPLEMENTATION) to avoid ODR violations across TUs.
#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
