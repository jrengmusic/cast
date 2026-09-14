// VMA leak diagnostics — debug builds only; routes VmaDeviceMemoryBlock::Destroy's
// unfreed-allocation dump (vk_mem_alloc.h VMA_LEAK_LOG_FORMAT, falls back to
// VMA_DEBUG_LOG_FORMAT when undefined) to stdout. printf is used directly because
// this macro expands inside VMA's own translation unit, below jam_core's
// debug::Log — unavailable at this layer.
#if JUCE_DEBUG
    #define VMA_DEBUG_LOG_FORMAT(format, ...) do { printf (format "\n", __VA_ARGS__); fflush (stdout); } while (false)
#endif

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
