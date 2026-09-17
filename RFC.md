# RFC — Windows MSVC Environment Activation for `cast`

**Status:** open, for COUNSELOR
**Date:** 2026-09-17
**Author:** MACHINIST (bootstrap session, first Windows build of `cast`)

---

## 1. Summary

`cast` built and ran on Windows for the first time this session. One design gap
remains: `cast`'s own `## toolchain` rows (`cmake`, `ninja`) need the MSVC compiler
environment (`INCLUDE`, `LIB`, `LIBPATH`, `PATH` to `cl.exe`) on Windows, and nothing
in `cast` itself sets that environment up. SPEC.md says environment resolution is the
caller's job. On Windows, most callers do not do that job. This RFC asks: should
`cast` gain a Windows-only exception to that rule?

---

## 2. Background

### 2.1 What happened this session

`cast` had never been built on Windows. The build was not blocked by a self-hosting
loop in `CMakeLists.txt` — that file is a plain JUCE console-app build
(`CMakeLists.txt:1-229`), no self-reference. Three real blockers surfaced and were
fixed, in order:

1. **`jam/generated/` was gitignored**, so a fresh Windows clone had no generated
   headers at all. ARCHITECT tracked the directory in git and pulled it. Separately,
   `cast/Source/generated/` (cast's own codegen output, e.g. `ProjectInfo.h`) was
   copied over from a macOS machine, since nothing on Windows could generate it yet.
2. **`jam_core.h` never included `juce_gui_basics`.** `jam_PluginHost.h:74-93`
   (`getHostScale()`) calls `juce::Desktop::getInstance()` under
   `#if JUCE_WINDOWS && JUCE_MODULE_AVAILABLE_juce_gui_basics`. On macOS,
   `JUCE_WINDOWS` is false, so this branch had never been compiled, on any platform,
   before this session. `jam_core`'s own module declaration
   (`jam_core.h:10`) does not list `juce_gui_basics` as a dependency — by design,
   confirmed by ARCHITECT. The existing soft-dependency pattern for
   `juce_audio_processors` (`jam_core.h:79-81`, guarded
   `#include <juce_audio_processors/juce_audio_processors.h>`) had no matching entry
   for `juce_gui_basics`. Fixed by adding the same pattern:
   ```cpp
   #if JUCE_MODULE_AVAILABLE_juce_gui_basics
   #include <juce_gui_basics/juce_gui_basics.h>
   #endif
   ```
3. **The installed binary had no `.exe` suffix.** `cast/cast/cmake.cast`'s
   `install-rename` block used the bare `${PROJECT_NAME}` literal instead of
   CMake's portable `${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}`. On Windows this
   installed a file literally named `cast` (no extension), which neither `cmd.exe`
   nor nvim's libuv-based process spawn (`uv_spawn` → `CreateProcess`, same
   resolution rules as `cmd.exe`) can find via PATH — both require the extension;
   only MSYS2 bash's exec layer tolerates the bare name. This was also the root
   cause of an earlier, apparently separate failure ("cast: cast
   ../jam/cast/spell.md: toolchain command failed") — `cast` spawning itself as a
   child process for the jam codegen `## toolchain` row failed for the identical
   reason. Fixing the suffix fixed both symptoms at once. Fixed in the template
   (`cast/cast/cmake.cast:281-283`), then `CMakeLists.txt` was regenerated through
   `cast` itself — confirming the self-hosted codegen round-trip works correctly on
   Windows.

A wrapper script, `cast/build.bat`, was added to do the one-time bootstrap: vswhere
→ `vcvarsall.bat x64` → prepend VS-bundled Ninja to PATH → run the `no-sign`
toolchain row's `cmake`/`ninja` commands directly. This mirrors the existing pattern
in `jam/jam_vulkan/lib/build.bat:7-26`. It is a bootstrap tool, not part of `cast`'s
own toolchain.

### 2.2 The remaining gap

After all three fixes, `cast` built and ran correctly — `cast --version`,
`cast --help`, and a self-hosted regeneration of `CMakeLists.txt` all worked, from
both bash and `cmd.exe`, once the environment was pre-activated by `build.bat`.

ARCHITECT then ran `cast` directly, from a shell that had not been pre-activated
(no `build.bat`, no nvim). It failed:

```
cast: cmake -S . -B Builds/Release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCAST_SIGN=ON: toolchain command failed
```

This is the blank-argument (default-flow) `cmake` row — `project-info.md:183`. It
failed because `cmake`, `ninja`, and `cl.exe` are not on PATH by default on this
machine; they only resolve once something has run `vcvarsall.bat` in the current
process. Confirmed: `cmake --version`, `ninja --version`, `where cl` all fail in a
plain, non-activated shell on this machine.

---

## 3. The relevant SPEC clause

`SPEC.md:778-780`, §6.9 Toolchain:

> The command resolves through the caller's environment. PATH, the working
> directory, and everything else that the process inherits are the caller's
> responsibility. The engine adds no resolution, no shell, and no quoting of its
> own.

This is a general, platform-neutral design principle — `cast` does not manage
environment for the processes it spawns, by design. It has not needed exception on
macOS, because Xcode's `clang` and Homebrew/MacPorts' `cmake`/`ninja` are always on
PATH the moment a shell starts. Windows has no equivalent guarantee: the MSVC
toolchain (`cl.exe`, and the headers/libs it needs) is inert until
`vcvarsall.bat` runs in the current process.

`## toolchain`'s table schema is also platform-neutral: `argument | command | flag`
only (`SPEC.md:754`), no `mac`/`win` split, unlike `## release`/`## debug`
(`project-info.md:200-234`), which do have per-platform columns. A toolchain row
cannot express "run this command differently on Windows" today.

---

## 4. Where activation already happens, and where it does not

Two things already do the vcvarsall→PATH activation, independently, for their own
purposes:

- **`nvim/lua/core/options.lua:9-43`** (`msvc_dev_env()`) — runs at nvim startup,
  captures the full `vcvarsall.bat x64` environment into `vim.env`. Every child
  process nvim spawns via `vim.fn.jobstart` (confirmed: `core/traffic.lua:99`, no
  explicit `env` override) inherits it. This means **nvim-triggered `cast` builds
  (`<leader>bb`, `<leader>br`, etc.) already inherit a correctly activated
  environment** — `core/cast-build.lua` only ever invokes the `debug` or `no-sign`
  toolchain rows (`M.TOOLCHAIN_ARGUMENT`, `cast-build.lua:24-27`), never the blank
  default-flow row ARCHITECT's manual test hit. This RFC's outcome likely does
  **not** block the nvim-wiring work already in progress at `~/.config` — it needs
  empirical confirmation, not a design change, unless COUNSELOR's investigation
  finds otherwise.
- **`cast/build.bat`** — the one-time bootstrap script added this session. Same
  pattern, scoped to a single manual invocation.

Nothing does this activation **inside `cast` itself**. Any other Windows launch
path — a bare terminal, a CI runner, a future script that is not nvim and not
`build.bat` — hits the same failure ARCHITECT reproduced.

---

## 5. Options

**Option A — `cast` self-activates on Windows.**
Port `msvc_dev_env()`'s logic (vswhere → `vcvarsall.bat x64` → env capture) into
`cast`'s own C++, gated to Windows only, run once before the first `## toolchain`
row spawns. Requires amending `SPEC.md` §6.9 with an explicit, platform-scoped
exception — the current text is unqualified and platform-neutral. Makes `cast`
correct from any Windows shell, matching its behavior on macOS.

**Option B — keep "caller activates first," document and enforce it.**
No SPEC or engine change. Every Windows launch path must pre-activate before
invoking `cast` — true today only for nvim (`options.lua`) and the one-off
`build.bat`. Any future launch path (a new script, a CI runner, a bare terminal
alias) must repeat the same vswhere→vcvarsall pattern itself. Consistent with
SPEC's stated design; pushes the burden onto every caller, forever.

Not evaluated in depth here, flagged as likely non-viable: wrapping individual
`## toolchain` rows with `cmd /c call vcvarsall.bat ... && <command>` in the data
table itself. Mechanically this can work — `cmd.exe`'s own argument parsing chains
the commands within one subprocess, satisfying SPEC's "no shell of its own"
constraint since `cmd.exe` is the literal spawned program, not an engine-added
shell. But `## toolchain` has no platform column (§3 above), so a `win`-only row
would need that schema to grow first — a smaller engine change than Option A, but
still an engine change, not a pure data edit. It also does not fix `cast` invoked
without any manifest argument at all (`cast --help`, `cast --version`), which do
not run any toolchain row but might reasonably be expected to work from a bare
shell regardless.

---

## 6. Open questions for COUNSELOR

- Which option — A, B, or the third variant in §5 — does ARCHITECT want?
- If A: exact SPEC.md §6.9 wording for the Windows exception.
- If A: does the Windows activation belong in `cast`'s core engine, or in a
  Windows-only compilation unit that the rest of the engine never sees?
- Either way: should `## toolchain` gain a platform column
  (`argument | command | flag | platform`, or split `flag` into `mac`/`win` like
  `## release`/`## debug` already do), independent of this RFC's outcome, since the
  schema gap is real regardless of which option is chosen?
