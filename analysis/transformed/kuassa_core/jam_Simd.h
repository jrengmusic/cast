/**
 * @file jam_Simd.h
 * @brief SSOT for SIMD architecture detection. arm_neon.h and emmintrin.h are
 *        platform headers — the submodule zero-include discipline's documented
 *        exception.
 */

#pragma once

#if defined(__arm64__) || defined(__aarch64__)
    #define JAM_SIMD_NEON 1
    #include <arm_neon.h>
#elif defined(__x86_64__) || defined(_M_X64)
    #define JAM_SIMD_SSE2 1
    #include <emmintrin.h>
#endif
