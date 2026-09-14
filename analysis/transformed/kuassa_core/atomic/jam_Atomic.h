/**
 * @file jam_Atomic.h
 * @brief Cross-platform lock-free atomic operations on plain int.
 *
 * Thin wrapper for acquire-load, release-store, and fetch-add semantics on
 * plain `int` values stored inside trivially copyable structs where
 * `std::atomic<int>` cannot be used without breaking trivial copyability.
 *
 * Targeting two compiler families:
 *   - MSVC (non-clang) on Windows: uses volatile reads/writes with
 *     `_ReadBarrier` / `_WriteBarrier` and `_InterlockedExchangeAdd`.
 *   - Everything else (GCC, Clang, clang-cl): uses `__atomic_*` builtins,
 *     which are lock-free on x86-64 and ARM64.
 *
 * `JUCE_WINDOWS and not defined(__clang__)` targets MSVC specifically.
 * clang-cl on Windows supports `__atomic_*` builtins and falls through to
 * the GCC/Clang path.
 *
 * @note All operations are lock-free on x86-64 and ARM64.
 */

#pragma once

#if JUCE_WINDOWS && ! defined(__clang__)
#include <intrin.h>
#endif

namespace jam
{ /*____________________________________________________________________________*/

/**
 * @brief Cross-platform lock-free atomic operations on plain int.
 *
 * Provides acquire-load and release-store semantics for `int` values stored
 * in trivially copyable structs (where `std::atomic<int>` cannot be used
 * without breaking trivial copyability).
 *
 * All operations are lock-free on x86-64 and ARM64.
 */
struct Atomic
{
    /** @brief Atomic load with acquire semantics. */
    static int load (const int& value) noexcept
    {
#if JUCE_WINDOWS && ! defined(__clang__)
        const int result { *static_cast<const volatile int*> (&value) };
        _ReadBarrier();
        return result;
#else
        return __atomic_load_n (&value, __ATOMIC_ACQUIRE);
#endif
    }

    /** @brief Atomic store with release semantics. */
    static void store (int& value, int newValue) noexcept
    {
#if JUCE_WINDOWS && ! defined(__clang__)
        _WriteBarrier();
        *static_cast<volatile int*> (&value) = newValue;
#else
        __atomic_store_n (&value, newValue, __ATOMIC_RELEASE);
#endif
    }

    /** @brief Atomic fetch-and-add with acquire-release semantics. Returns old value. */
    static int fetchAdd (int& value, int addend) noexcept
    {
#if JUCE_WINDOWS && ! defined(__clang__)
        // Windows MSVC _InterlockedExchangeAdd requires volatile long*; int and long are ABI-identical (32-bit) on MSVC — machine-idiom cast.
        return _InterlockedExchangeAdd (reinterpret_cast<volatile long*> (&value), static_cast<long> (addend));
#else
        return __atomic_fetch_add (&value, addend, __ATOMIC_ACQ_REL);
#endif
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
