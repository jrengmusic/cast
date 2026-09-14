/**
 * @file jam_Platform.h
 * @brief Windows platform version detection utilities.
 *
 * Provides `getWindowsBuildNumber()` as the SSOT for OS build-number
 * lookups, with `isWindows10()` and `isWindows11()` derived from it.
 * Uses `RtlGetVersion` to obtain the real OS build number, bypassing the
 * application compatibility manifest shim that causes `GetVersionEx` to lie.
 *
 * @par Build number thresholds
 * - Windows 11 21H2 (original release): build 22000
 * - Windows 11 22H2 (Mica / Acrylic 11 backends): build 22621
 *
 * @note This header is Windows-only.  Include it only inside `#if JUCE_WINDOWS`
 *       guards at the call site.
 */

#pragma once

#ifndef NOMINMAX
 #define NOMINMAX
#endif
#include <windows.h>
#include <winternl.h>
#ifdef small
 #undef small
#endif
#ifdef ALTERNATE
 #undef ALTERNATE
#endif
#ifdef MessageBox
 #undef MessageBox
#endif

static constexpr const wchar_t* const ntdllLibraryName { L"ntdll.dll" };
static constexpr const char* const rtlGetVersionSymbol { "RtlGetVersion" };

/**
 * @brief Returns the current Windows build number (DWORD), or 0 if detection fails.
 *
 * Uses `RtlGetVersion` from `ntdll.dll` to obtain the real OS build number,
 * bypassing the application compatibility manifest shim that causes
 * `GetVersionEx` to lie.
 *
 * Result is cached in a `static const` — computed once per process lifetime.
 * SSOT for build-number lookups.  Two consumers: `isWindows10()` and
 * availability checks (e.g. `BackgroundBlur::isMicaAvailable`).
 *
 * @return Build number on success; @c 0 if version detection fails.
 */
static DWORD getWindowsBuildNumber() noexcept
{
    using FnRtlGetVersion = NTSTATUS (NTAPI*) (OSVERSIONINFOEXW*);

    static const DWORD cached { []() noexcept -> DWORD
    {
        DWORD result { 0 };

        const HMODULE ntdll { GetModuleHandleW (ntdllLibraryName) };

        if (ntdll != nullptr)
        {
            const FnRtlGetVersion rtlGetVersion
            {
                reinterpret_cast<FnRtlGetVersion> (
                    GetProcAddress (ntdll, rtlGetVersionSymbol))
            };

            if (rtlGetVersion != nullptr)
            {
                OSVERSIONINFOEXW osvi {};
                osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFOEXW);

                if (rtlGetVersion (&osvi) == 0)
                    result = osvi.dwBuildNumber;
            }
        }

        return result;
    }() };

    return cached;
}

/**
 * @brief Returns true if the current OS is Windows 10 (build < 22000).
 *
 * Derived from `getWindowsBuildNumber()`.  Windows 11 is build >= 22000.
 * This function returns `false` (Windows 11 path) when the build number
 * cannot be determined, which is the safe default — the Windows 11 path
 * is canon.
 *
 * @return @c true  on Windows 10 (build < 22000).
 * @return @c false on Windows 11+ or if version detection fails.
 */
static bool isWindows10() noexcept
{
    return getWindowsBuildNumber() < 22000;
}

/**
 * @brief Returns true if the current OS is Windows 11 (build >= 22000).
 *
 * Inverse of `isWindows10()`.  Uses the same cached `getWindowsBuildNumber()`
 * probe.
 *
 * @return @c true  on Windows 11+ (build >= 22000).
 * @return @c false on Windows 10 or if version detection fails.
 */
static bool isWindows11 () noexcept
{
    return not isWindows10();
}
