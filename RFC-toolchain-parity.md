# RFC — Toolchain Parity: What the Centralized JAM CMake Toolchain Produces Beyond Config/Build

**Author:** MACHINIST
**Date:** 2026-09-01
**Status:** Delivered — the parity sprint that followed (2026-09-01/02) closed every
gap catalogued below (see `HANDOFF-MACHINIST.md`, Phase 3 (eve) — DELIVERED). This
document is retained as the findings record, not as an open plan.

---

## 1. Scope

ARCHITECT asked for a thorough read of the whole flow of the old, centralized JAM
CMake toolchain (`jam/cmake/AppBuilder.cmake`, `jam/cmake/PluginBuilder.cmake`, and
every script they invoke) — specifically **everything it produces besides
configuring and building** — so CAST's toolchain can be brought to identical
behavior. Every claim below is a direct citation (file:line) from reading the actual
source, not the old jam-side doc comments.

**Files read in full:** `AppBuilder.cmake`, `PluginBuilder.cmake`, `ClangdConfig.cmake`,
`SignMac.cmake`, `SignAAX.cmake`, `CopyPluginRelease.cmake`, `BuildConfig.cmake`.
**Files skimmed for structure:** `BuildSetup.cmake`, `GenerateJucer.cmake`.
**Confirmed dead/unreferenced:** `CopyPluginDebug.cmake` — not called from
`AppBuilder.cmake` or `PluginBuilder.cmake`; only self-reference and one RFC
mention exist (`jam/RFC-echo.md`). Excluded below.

---

## 2. Producers, one per mechanism

### 2.1 `.clangd` generation — `ClangdConfig.cmake`

Invoked as `cmake -P` POST_BUILD from both builders:
`AppBuilder.cmake:873-884` (unconditional — **every config, every build**, not
Release-gated) and `PluginBuilder.cmake:1105-1113` (same, unconditional).

What it actually does (`ClangdConfig.cmake:23-181`):

- Extracts every `-D` macro and `-I` include path from `compile_commands.json`
  via regex (`:58-83`), merges with platform fallback macros (`:86-105`).
- Detects the host target triple (`arm64-apple-darwin` etc., `:39-55`) and emits
  a `--target=` flag plus a `Remove:` list stripping `-arch`/`-mmacosx-version-min*`
  (macOS only) so clangd's own triple inference isn't fought by CMake's forwarded
  flags.
- **Writes TWO `.clangd` files, not one:**
  1. `${PROJECT_ROOT}/.clangd` (`:125-142`)
  2. `${JAM_ROOT}/.clangd` (`:160-177`) — the **framework root's own `.clangd`**,
     with an extra `-include <TARGET>_artefacts/JuceLibraryCode/JuceHeader.h`
     (`:153-158`) so jam module headers — which never appear as their own entry
     in `compile_commands.json` — still get full JUCE type resolution as
     standalone translation units.
- Removes a stale `compile_commands.json` symlink from `PROJECT_ROOT` if present
  (`:144-147`).

**CAST's current equivalent** (`core/cmake-picker.lua:812-835` `syncClangd`,
`:846-935` async variant): writes **only the project's own `.clangd`** — copies
`compile_commands.json` to the project root and regenerates `.clangd` there. It
does **not** write a `${FRAMEWORK_ROOT}/.clangd`. This is the concrete parity gap:
opening a `jam_core`/`jam_markdown` source file directly (outside any project's own
tree) has no `.clangd` at `jam/` pointing at a compilation database + forced
`JuceHeader.h` include, so clangd resolution for framework-only editing sessions is
weaker than the old toolchain provided.

Client-side vs build-graph-side is otherwise the same mechanism (compile flags,
target triple, macro extraction) — CAST's version runs in nvim's Lua on the
client, not baked into the CMake build graph as a `cmake -P` subprocess; that's a
deliberate, already-discussed difference (see the "why faster" thread this
session), not a functional gap.

### 2.2 Codesigning + notarization — `SignMac.cmake`

Invoked `AppBuilder.cmake:836-853` (apps, Release + macOS only) and
`PluginBuilder.cmake:1030-1041` (plugins, once per active format target).

Beyond plain `codesign --sign`, it does, in order (`SignMac.cmake:90-405`):

1. **`$ENV{JAM_NOTARIZE}=OFF` escape hatch** (`:104-110`) — nvim's `build-debug.sh`
   sets this for a plain `<leader>br` Release build so iteration builds skip
   signing entirely. This is the exact feature ARCHITECT asked for on CAST's side
   this session (`--no-sign` / `CAST_SIGN` option) — already added
   (`project-info.md` `## toolchain`, `cast/cmake.cast:8,165`).
2. `xattr -cr` strip of extended attributes before signing (`:140-148`) — **CAST's
   `cmake.cast` codesign fence does not do this.**
3. **`codesign --verify --verbose` after signing** (`:167-175`) — CAST's fence signs
   and moves on; it never verifies the signature it just produced.
4. Notarization submit + wait (`:182-192`, `:316-326`/`:366-376`) — CAST has this
   (`cast/cmake.cast:225-227`).
5. **`xcrun stapler staple` after `status: Accepted`**
   (`:335-349` pkg path, `:387-401` app/zip path) — **CAST's toolchain does not
   staple.** This is the most significant gap: an un-stapled, notarized binary
   still requires network access to Apple's servers on first launch elsewhere to
   pass Gatekeeper; a stapled one works fully offline.
6. **Console-app path builds a signed `.pkg` installer**, not a raw binary copy
   (`:199-350`): stages the binary under `opt/<app_name>/`, writes a `postinstall`
   script that symlinks it into the *real* logged-in user's `~/.local/bin`
   (`:236-245`, resolved via `stat -f "%Su" /dev/console`, correct even when
   installed via `sudo`), builds the `.pkg` with `pkgbuild`, signs it with
   `productsign` (Installer identity, separate from the Application identity —
   `:284-314`), then notarizes and staples the `.pkg` itself (not the raw binary).
   **CAST's install step is structurally different**: it copies the raw signed
   binary directly to `$ENV{HOME}/.local/bin` via `cmake -E copy` + `rename`
   (`cast/cmake.cast:230-238`) — no `.pkg`, no separate Installer-identity signing,
   no postinstall symlink step. This is a real design divergence, not obviously a
   bug — CAST's simpler direct-copy avoids an entire signing identity and PACE-style
   packaging step — but it means CAST-built console apps are not currently
   distributable as a signed installer the way whatdbg/tit/cake's are.

### 2.3 AAX signing — `SignAAX.cmake`

`PluginBuilder.cmake:1013-1027`, plugin-format-specific, PACE `wraptool` signing
with `--account`/`--wcguid`/`--signid`, optional `--notarize-keychain-profile`
(`SignAAX.cmake:59-91`). Reads a `Wrap Config GUID` out of an `AAX.info` file at
the plugin root via regex (`PluginBuilder.cmake:969-980`).

**CAST has no equivalent fence at all.** Irrelevant for `cast` itself (a console
app), but a hard requirement the day any AAX-format plugin (Phase 5) migrates.

### 2.4 Versioned QA-build archive copy — `CopyPluginRelease.cmake`

`PluginBuilder.cmake:1001-1010` (plugin → QA dir) and `:1046-1056` (signed
artifact copied *back* from the QA dir to the system plugin path) — invoked
**twice per format target**, both via `cmake -P`.

Copies the built bundle, preserving structure/permissions/symlinks
(`file(INSTALL ... USE_SOURCE_PERMISSIONS)`, `CopyPluginRelease.cmake:59-66`), into
a **dated, versioned archive directory**:
`${FRAMEWORK_PATH}/___builds___/<YYYY-MM-DD> <PluginName> v<Version> <Platform>/`
(`PluginBuilder.cmake:946-963`). This is a historical build ledger — every Release
build leaves a timestamped copy, nothing is ever overwritten in place.

**CAST has no equivalent.** Its install step (`cast/cmake.cast:229-238`) always
writes to the same fixed path (`$ENV{HOME}/.local/bin/${PROJECT_NAME}`,
atomically via tmp-then-rename) — no history, no per-version archive. For `cast`
itself (a single dev tool you always want "the latest") this is arguably correct
behavior, not a gap; for a shipped plugin where QA needs to compare builds across
versions, it is a real capability CAST does not have yet.

### 2.5 `.jucer` generation — `GenerateJucer.cmake`

Opt-in (`GENERATE_JUCER` flag, `PluginBuilder.cmake:1068-1080`), not part of the
default flow for either builder. Produces a `.jucer` XML project file — file tree,
module list, defines, per-IDE module paths — for opening the project in Projucer.
Backs up any existing `.jucer` before overwriting (`GenerateJucer.cmake:447`).

**CAST has no equivalent, and per SPEC/HANDOFF this is intentionally out of
scope** — CAST replaces Projucer entirely rather than feeding it. Listed here only
for completeness of "everything the old toolchain can produce," not as a gap to
close.

### 2.6 Framework-level setup — `BuildSetup.cmake` / `BuildConfig.cmake`

Both run once per configure, before any target exists. Sourced identity
(`Metadata.cmake`), warning-flag policy, JUCE discovery + the same
ephemeral-patched-JUCE-tree pattern CAST already replicates
(`cmake.cast:69-75` mirrors `BuildSetup.cmake`'s `PATCHED_JUCE_PATH` logic almost
verbatim), VST2 SDK discovery, Vulkan toolchain discovery, module discovery. All of
this is either already expressed as CAST manifest data (`## project info`,
`## cmake`, `## signing`) or is JAM/Vulkan-specific and irrelevant to `cast` itself
(a `juce_core`-only console app). No new finding beyond what CAST's own
`project-info.md`/`cmake.cast` already cover for the fields that apply to it.

`BuildConfig.cmake:133` also sets `CMAKE_CONFIGURATION_TYPES` — a **multi-config
generator** variable, silently ignored under `-G Ninja` (see this session's
single-config-vs-multi-config discussion) — dead weight under the generator both
toolchains actually use, not a parity item.

---

## 3. Parity gap summary (for a future sprint to plan against)

| Capability | Old toolchain | CAST today | Applies to `cast` itself? |
|---|---|---|---|
| Framework-root `.clangd` (`-include JuceHeader.h`) | Yes (`ClangdConfig.cmake:160-177`) | No | Yes — jam module editing outside a project tree |
| `xattr -cr` before signing | Yes | No | Yes |
| `codesign --verify` after signing | Yes | No | Yes |
| Notarization **stapling** | Yes | **No** | Yes — biggest gap, offline Gatekeeper |
| Signed `.pkg` installer + postinstall symlink (ConsoleApp) | Yes | No (raw copy) | Yes, if a signed installer is ever wanted |
| AAX PACE `wraptool` signing | Yes | No | Only plugins (Phase 5) |
| Versioned QA-build archive (`___builds___/`) | Yes | No | Only plugins/apps needing build history |
| `.jucer` generation | Yes (opt-in) | No — by design | No, CAST replaces Projucer |

No manifest or engine change is proposed by this document — ARCHITECT/COUNSELOR
decide which of these become CAST manifest rows and in what order.
