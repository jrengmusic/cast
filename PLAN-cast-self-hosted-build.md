# PLAN: CAST Self-Hosted Build (Phase 2 of CAST Migration)

**RFC:** none — objective from ARCHITECT prompt (session-derived 5-phase CAST migration strategy; this plan covers Phase 2 only)
**Date:** 2026-09-01
**BLESSED Compliance:** verified
**Language Constraints:** CMake (raw, no JAM `cmake/` coupling — no `configure_app`, no `AppBuilder.cmake`, no `PluginBuilder.cmake`, no `SignMac.cmake`/`BuildConfig.cmake` cache variables); CAST manifest/template authoring per `cast/SPEC.md`

## Overview

Replace `cast/CMakeLists.txt`'s `configure_app(...)` call with a CAST-generated, fully self-sufficient `CMakeLists.txt` that talks directly to JUCE's own CMake API — including its own `POST_BUILD` codesign+notarize step, with every signing parameter a visible `project-info.md` row. Nothing is inherited from JAM's build helpers or `___sign___/sign.sh`; the generated file owns its entire build+sign lifecycle, Projucer-equivalent.

## Correction carried into this plan

Earlier framing treated signing as `install.sh`'s job (external `sign.sh` call, per Phase 1). ARCHITECT corrected this: **a Release build must be signed and notarized by the self-sufficient `CMakeLists.txt` itself** — not relayed to any other script. Phase 1's `install.sh` signing step becomes redundant once this lands and will need revisiting in a follow-up (not touched by this plan — flagged, not executed).

## Dependency & API Inventory

**Build surface enumerated (Pathfinder, this session) — nothing globbed for source, glob only for uniform resource sets (matches eve's own `## layout glob` precedent, `eve/cast/cmake.cast:335-337`):**
- 18 source files under `cast/Source/` (`main.cpp`, `Help.h`, `Items.h`, `Jobs.h`, `Model.h`, `Processor.h`, `Shapes.h`, `TemplateDocument.h`, `Transforms.h`, `Validator.h`, `Writer.h`, plus 6 under `Source/generated/`)
- 3 explicit binary files: `Source/HELP.md`, `Source/style.css`, `Source/resources/cast-output.md`
- 37 SVG assets, glob-shaped: `${CAST_USER_MODULE_PATH}/resources/svg/*.svg` (identical glob pattern already used by JAM's own default, no need to enumerate individually)
- Modules: `juce_core` (JUCE), `jam_core` + `jam_markdown` (user modules)
- Identity: company `JRENG`, bundle domain `com.jreng`, email `info@jrengmusic.com`, website `https://jrengmusic.com` (`jam/cmake/Metadata.cmake`)
- No icon files, no platform-specific source filtering, no user-defined modules beyond the two JAM ones
- `cast/CMakeLists.txt:1` — `cmake_minimum_required(VERSION 3.25)` (note: differs from eve's `4.2.0` — each project's own real minimum, not copied)
- JUCE version: `8.0.14` (same shared `~/Documents/Poems/JUCE` checkout eve uses)
- Signing (`jam/cmake/SignMac.cmake:140-162` codesign invocation, `:351-374` raw-artifact notarize-via-zip path — **not** the `.pkg`/`productsign`/staple path, which only applies to bundles/installers and is what's currently broken): `codesign --force --options runtime --entitlements <path> --sign <identity>`, then `ditto -c -k --keepParent`, `xcrun notarytool submit --keychain-profile <profile> --wait`. No stapling — `stapler staple` only supports `.app`/`.pkg`/`.dmg`, not a bare Mach-O binary; `___sign___/sign.sh` already proved this exact flow (codesign+zip+notarytool, no staple) works for a raw CLI binary.
- Entitlements content: `jam/cmake/entitlements.plist` and `___sign___/entitlements.plist` are byte-identical (`allow-unsigned-executable-memory`, `disable-library-validation`) — pre-existing duplicate, not fixed here. cast gets its **own** project-local copy (self-sufficiency — no reaching into `jam/cmake/` or `___sign___/`), reusing this proven content rather than guessing a minimal set.

## Universal schema (per session-agreed rename)

- `## jam module` → `## user module` (already agreed) — columns unchanged (`root | name | comment`)
- `jamPath` → `userModulePath`; `CAST_JAM_PATH` → `CAST_USER_MODULE_PATH`
- New table `## signing` (proposed name, needs ratification — NAMES.md Rule -1) — `key | value | comment`, same shape as `## cmake`
- New table `## binary glob` (proposed name, replacing eve's app-specific `## layout glob` with a generic name reusable by both `app` and `plugin` fences) — `root | name | extension | comment`, same `root`/`name` alias-indirection shape as `## user module`/`## patch` (`root` is an index alias like `@jam`, never a raw `${CAST_USER_MODULE_PATH}` literal — that would re-declare the path outside the index, a second SSOT for the same value), plus `extension` for the glob suffix this table's purpose needs. Schema stays mechanical — "svg", "resources/svg" are row *data*, never a column or table name.

## New files this plan creates

1. `cast/entitlements.plist` — project-local copy of the proven entitlements content
2. Additions to `cast/project-info.md`: `## index`, `## cmake`, `## user module`, `## signing`, `## source`, `## define`, `## include`, `## binary glob`
3. `cast/cast/cmake.cast` — shared fences (toolchain, JUCE patch, module loading, target sources/defines/includes/link, binary data, report) + `app` target fence (`juce_add_console_app`) + `postBuild` fence (codesign + notarize, `$<CONFIG:Release>`-guarded, zero dependency on `sign.sh` or JAM's `BuildConfig.cmake` cache variables)
4. New `CAST.md` output row: `@project-info` + `@cmake:app` + `@cmake:postBuild` → `@CMakeLists` (new index alias for `../CMakeLists.txt`)

## Validation Gate

MACHINIST verifies directly against `~/.carol/CODING.md`/`NAMES.md` and this plan's locked table names. Manual round-trip validation (Step 5) is the real gate — generated `CMakeLists.txt` must build AND its own `POST_BUILD` step must sign+notarize successfully, matching the "Accepted" result already proven possible in Phase 1's build log, before `configure_app` is deleted from `cast/CMakeLists.txt`.

## Steps

### Step 1: `cast/entitlements.plist`
**Action:** copy `jam/cmake/entitlements.plist` content verbatim to a new project-local file.
**Validation:** byte-identical content, new location only.

### Step 2: `cast/project-info.md` additions
**Action:** add `## index` (`@jam` → `${CAST_USER_MODULE_PATH}`), `## cmake` (minimumVersion 3.25, cxxStandard 17, deploymentTarget 11.0, description "Universal headless codegen", jucePath, userModulePath, juceVersion 8.0.14), `## user module` (jam_core, jam_markdown only — not all 20 JAM modules), `## signing` (identity, entitlementsPath, notaryProfile), `## source` (18 files), `## define` (DONT_SET_USING_JUCE_NAMESPACE=1, JUCE_DONT_DECLARE_PROJECTINFO=1, CAST_COMMIT), `## include` (JAM root, Source/generated), `## binary` (HELP.md, style.css, cast-output.md as explicit `name | value | comment` rows), `## binary glob` (one row: `root=@jam`, `name=resources/svg`, `extension=svg` — alias-indirected, mechanical columns, no domain word in the schema).
**Validation:** every value traceable to the Pathfinder-enumerated build surface above — nothing invented, nothing dropped.

### Step 3: `cast/cast/cmake.cast`
**Action:** author shared fences (mirroring `eve/cast/cmake.cast:1-166,189-350` structure, generic-module version) + `app` fence (`juce_add_console_app(${PROJECT_NAME} PRODUCT_NAME ... VERSION ... COMPANY_NAME ...)`) + `postBuild` fence (the codesign+notarize `add_custom_command(TARGET ... POST_BUILD ...)`, `$<CONFIG:Release>`-guarded, reading `:::identity:::`/`:::entitlementsPath:::`/`:::notaryProfile:::` tokens).
**Validation:** zero hardcoded paths/identities in the template — every value a token bound to a `project-info.md` row (per ARCHITECT's explicit instruction this session).

### Step 4: `cast/cast/CAST.md` wiring
**Action:** add `@CMakeLists` index alias, add the output row wiring `project-info.md`'s tables through `@cmake:app` + `@cmake:postBuild` to `@CMakeLists`.
**Validation:** arity/source count matches the fence's `:::list:::` occurrences (SPEC §6.4) — checked by running `cast` itself (fatal on mismatch, per SPEC §10.1).

### Step 5: Manual round-trip validation (ARCHITECT + MACHINIST together)
**Action:** run the Phase-1-installed `cast` against the new manifest, inspect the generated `CMakeLists.txt`, build it Release, confirm the `POST_BUILD` step signs and notarizes successfully (status: Accepted) without invoking `sign.sh` or any JAM cmake helper.
**Validation:** `codesign --verify` passes, notarization log shows Accepted, binary behavior identical to Phase 1's.

### Step 6: Delete old `configure_app` call
**Action:** only after Step 5 passes — remove the `configure_app(...)` block from `cast/CMakeLists.txt`, replacing it with whatever minimal bootstrap is needed to invoke `cast` itself (or confirm this happens entirely outside CMakeLists.txt, via `install.sh`/`build.sh` calling `cast` before `cmake -S -B`).
**Validation:** clean-tree rebuild from scratch succeeds with zero JAM `cmake/` includes anywhere in the generated file.

## BLESSED Alignment

- **Explicit** — every signing parameter (identity, entitlements path, notary profile) a visible table row; no hidden `BuildConfig.cmake` cache variable, no external script call
- **Single Source of Truth** — `cast` and `eve` will share the same `cmake.cast` fences for everything except target creation and, now, potentially signing (signing fence is shared too — identical mechanism for app and plugin, only the artifact path/type differs)
- **Bound** — the generated `CMakeLists.txt` owns its full lifecycle (build → sign → notarize); no ambiguous handoff to a script that may or may not run

## Risks / Open Questions

- Table names `## signing` and `## binary glob` are proposed, not yet ratified (NAMES.md Rule -1) — flagging before Step 2 writes them.
- Phase 1's `install.sh` signing step becomes redundant once this lands; not touched by this plan, needs its own follow-up decision.
- Whether the `plugin` fence (Phase 3, `eve`) reuses this exact `postBuild` fence unchanged, or needs AAX-specific extension (PACE account, wcguid) — deferred to Phase 3 per the agreed incremental strategy.
