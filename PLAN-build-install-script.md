# PLAN: CAST Build+Install+Sign Script (Phase 1 of CAST Migration)

**RFC:** none — objective from ARCHITECT prompt (session-derived 5-phase CAST migration strategy; this plan covers Phase 1 only)
**Date:** 2026-08-31
**BLESSED Compliance:** verified
**Language Constraints:** bash shell scripts (macOS), CMake/Ninja invocation — `~/.carol/CODING.md`'s C++ rules don't apply directly; shell conventions taken from established sibling scripts (`tit/build.sh`, `tit/install.sh`)

## Overview

Build, sign, and install the current `cast` CLI binary — unchanged CMakeLists.txt, still `configure_app`-based — to `~/.local/bin/cast`. This bootstraps a working `cast` binary, required before Phase 2 can dogfood CAST against its own generated manifest.

## Role Note (conflict citation)

`/goplan`'s template assumes COUNSELOR+@Engineer delegation and per-step COUNSELOR validation. MACHINIST never delegates to Engineer (CAROL.md role definition) and executes directly. Steps below are executed by MACHINIST directly; "Validation" is MACHINIST's own verification against CODING.md/NAMES.md — no Engineer/Auditor round-trip for this machine-level task.

## Dependency & API Inventory

- `cast/CMakeLists.txt:20-30` — `configure_app(TARGET_NAME cast_App PRODUCT_NAME cast VERSION 0.1.0 ...)`, unchanged this phase
- Artifact path (JUCE `_artefacts` convention, confirmed via `tit/build.sh:28` precedent — `TARGET_NAME titc` → `titc_artefacts`): `Builds/Ninja/cast_App_artefacts/Release/cast`
- `___sign___/sign.sh:1-50` — `sign.sh [--debugger] [--no-notarize] [BINARY]`; identity `Developer ID Application: Bayu Ardianto (9BDSN9TDX3)`; entitlements `entitlements.plist`; notarizes via `xcrun notarytool submit --keychain-profile notary --wait` when `NOTARIZE=1` (default)
- Established script shape — `tit/build.sh` (build-only) + `tit/install.sh` (clean+configure+build+install, self-contained): `#!/usr/bin/env bash`, `set -e`, `SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"`, `cd "$SCRIPT_DIR"`, OS-branched cpu-count detection (`sysctl -n hw.logicalcpu` / `nproc`), `mkdir -p "$HOME/.local/bin"` before copy
- No `build.sh`/`install.sh` exists at `cast/` root today (confirmed by direct `find`)

## Validation Gate

MACHINIST verifies each step directly against `~/.carol/CODING.md` (shell conventions already established in sibling scripts) and `~/.carol/NAMES.md` (no new names — variables mirror `tit`'s: `SCRIPT_DIR`, `ARTIFACT`, `INSTALL_DIR`, `detect_cpu_count`). No Auditor sweep for this phase — ARCHITECT reviews directly.

## Steps

### Step 1: `cast/build.sh`
**Scope:** new file `~/Documents/Poems/dev/cast/build.sh`
**Action:** Mirror `tit/build.sh` exactly — clean `Builds/Ninja`, configure Release, build with OS-detected parallelism, report the artifact path (`Builds/Ninja/cast_App_artefacts/Release/cast`). Build-only, no signing, no install — matches the `tit` precedent's division of labor.
**Validation:** shebang/`set -e`/`cd` pattern matches `tit/build.sh:1-5` verbatim; reported binary path matches `cast_App`'s real artifact path.

### Step 2: `cast/install.sh`
**Scope:** new file `~/Documents/Poems/dev/cast/install.sh`
**Action:** Mirror `tit/install.sh` (clean+configure+build+install, OS-branched), with a signing stage inserted after the build and before the copy: invoke `~/Documents/Poems/dev/___sign___/sign.sh` against the built artifact, notarizing by default (confirmed), accepting an optional `--no-notarize` passthrough for fast local iteration. Copy the signed binary to `$HOME/.local/bin/cast`.
**Validation:** `mkdir -p "$HOME/.local/bin"` present before copy (tit precedent); sign step runs on the artifact before the copy step, never after — no re-copy of an unsigned binary over a signed one (same ordering discipline as `build-debug.sh:78-83`'s comment on not duplicating post-build copies); `--no-notarize` forwards cleanly to `sign.sh --no-notarize`.

### Step 3: Manual verification (ARCHITECT, not MACHINIST)
**Scope:** none — verification only
**Action:** Run `install.sh`; confirm `cast --version` and `cast --help` work from `~/.local/bin/cast`; confirm `codesign --verify --verbose ~/.local/bin/cast` passes.
**Validation:** exit code 0 on all three checks. Per Build protocol ("AGENTS BUILD CODE FOR ARCHITECT TO TEST"), MACHINIST does not run this step.

## BLESSED Alignment

- **Explicit** — artifact/install paths derived from the real target/product names, no hardcoded guesses
- **Lean** — two scripts, one responsibility each, mirrors the existing sibling pattern exactly, no new abstraction
- **Single Source of Truth** — target/product names read from `CMakeLists.txt`'s own `configure_app` call, never duplicated as separate constants
- **Deterministic** — Release-only, OS-branch is the only variation, already an established codebase pattern

## Risks / Open Questions

None for this phase. Phases 2–5 (self-hosting, `eve` rename, post-build/signing fence, Ninja Multi-Config switch, nvim wiring) are explicitly out of scope for this PLAN — each gets its own plan once its design is locked, per the agreed incremental strategy.
