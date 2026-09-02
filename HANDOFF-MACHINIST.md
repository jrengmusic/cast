# MACHINIST Handoff — CAST Migration

**Date:** 2026-09-02 (updated end of nvim-as-IDE / project-state sprint)
**Objective:** Migrate JAM/KANJUT/CIUM off the centralized JAM cmake toolchain
(`configure_app`/`AppBuilder.cmake`/`PluginBuilder.cmake`) onto CAST — a self-hosted
markdown-table-driven codegen tool (`~/Documents/Poems/dev/cast/`) that generates a
fully self-sufficient `CMakeLists.txt` per project, Projucer-equivalent, with zero
coupling back to JAM's cmake helpers.

**5-phase roster, ARCHITECT-approved:**
1. Build/sign/install CAST's own binary — **DONE**
2. Self-host CAST's own `CMakeLists.txt` via CAST — **DONE**
3. `eve` as the plugin-fence baseline — **DONE**
4. Wire nvim directly to `cast` — **DONE (macOS); Windows blocked CAST-side**
5. Roll out to KANJUT/CIUM and the remaining projects — **NOT STARTED, now blocking**

---

## Where we are right now

Three projects are CAST-managed: `cast`, `eve`, `jam` (codegen only, no build).
Everything else in `~/Documents/Poems` is still legacy CMake.

The whole lifecycle is data-driven. `cast cast/CAST.md` (no flag) runs the default
flow declared in `project-info.md`'s `## toolchain` table; `--debug` and `--no-sign`
run their own rows; an undeclared `--word` is fatal. Both `cast` and `eve` carry the
identical three-argument shape:

| argument | build | signing |
|---|---|---|
| *(blank)* | Release, `Builds/Release` | `CAST_SIGN=ON` — full sign/notarize/staple/install |
| `debug` | Debug, `Builds/Debug` | none |
| `no-sign` | Release, `Builds/Release` | `CAST_SIGN=OFF` |

Every fence in that chain is a tokenized template in `cast/cmake.cast`; every
parameter (identity, entitlements, notary profile, install dir) is a visible cell in
`project-info.md`. `build.sh`/`install.sh` are deleted — the generated CMakeLists owns
sign → notarize → install. `@user-module` in `## index` is the single per-framework
path knob (`CAST_USER_MODULE_PATH` is gone).

`cast` ships a notarized, stapled `.pkg` into `___builds___` and installs the signed
binary to `~/.local/bin`. `eve` produces VST3/AU (Accepted + stapled) and AAX
(`wraptool`), with a byte-identical QA-vs-installed archive at
`___builds___/{date} EVE v0.1.0 macOS`.

---

## Phase 4 — nvim is the IDE (REWRITTEN 2026-09-02)

The previous handoff described `isCastManaged` branching, `build-debug.sh`/`.bat`,
`clean-build.sh`/`.bat`, `cmake-picker.lua`, `dap/configurations.lua` and
`.nvim-dap-config`. **All of that is deleted.** Committed as `~/.config` `e5cc66b`.

**Contract (ARCHITECT):** CAST owns the build toolchain end-to-end; everything
editor-side — LSP, DAP, navigator, doxygen, clean — is nvim's, driven from one
materialised project state. DATA DICTATE LOGIC.

### The state

`.project` (Lua, project root, gitignored) is the materialised AST, written
write-if-different by `core/project.lua`. Sections: `sources`, `manifest`,
`toolchain`, `dependencies`, `configurations`, `targets`, `launch`, `compile`,
`selection`. `selection` (configuration / target / host) is the only user-owned
field and is the only thing carried across a re-parse — everything else is derived.

- `core/project.lua` — registry (`getOrCreate` per root), parse driver, findings
  validator, deterministic serializer, `fs_event` watchers on the declared sources,
  and the `User ProjectChanged` event. A change re-parses the whole document; nothing
  is ever patched in place.
- `core/project/cast.lua` — the CAST locator. `project-info.md` + `cast/CAST.md` +
  `compile_commands.json` → the AST. Targets come from the `## format` table × a
  platform-keyed JUCE artefact-layout constant table; configurations come from the
  `## toolchain` table's `argument` column and each cmake row's `-B` directory.

Sources are `project-info.md` and the selected configuration's
`compile_commands.json`. `CMakeLists.txt` and `.clangd` are outputs, never inputs.

### The listeners

| Module | Responsibility |
|---|---|
| `core/clangd.lua` | Writes project-root and user-module-root `.clangd` (the latter with `-include JuceHeader.h`) plus the root `compile_commands.json` copy, all write-if-different |
| `dap/launch.lua` | DAP configurations from `targets` × `launch`; target/host picker; persists the choice through `project.setSelection` |
| `core/build.lua` | One flow for every project — `bb`/`br`/`bB`/`bR`/`bc`/`bk`/`bd`, whatdbg launch/attach, clean deletes every declared build directory and re-parses |
| `core/navigator.lua` | Was `cmake-picker.lua`, minus all clangd and project derivation — symlink tree, picker, grep, replace |
| `core/doxygen.lua` | Reads `dependencies` for JUCE and the user-module root; no hardcoded paths |
| `lsp/clangd.lua` | Root directory from `project.getProjectFor(bufname)` |

C++ GUI projects (including plugins) build and run the Standalone under whatdbg by
default; a project with no prior selection asks once, then persists it. whatdbg is
the DAP adapter on both platforms — every `codelldb` reference is gone from
`~/.config`. `JUCE.clang-format` now lives at `~/.config/nvim/clang-format/`.

### Consequence — legacy projects have no build path

There is no fallback branch any more. A project without `project-info.md` +
`cast/CAST.md` gets a loud notification and nothing else: no build, no debug, no
clean, no doxygen, no navigator. This is the ratified "migrate or nothing" rule, and
it makes Phase 5 the blocking item.

---

## Open for MACHINIST

### 1. Phase 5 migration queue (blocking DX)

Each of these needs `project-info.md` + `cast/CAST.md` + `cast/cmake.cast` before it
can be built or debugged from nvim again:

| Project | Kind | Notes |
|---|---|---|
| `dev/whatdbg` | console app | Highest leverage — it is the debugger every other project launches under |
| `dev/end` | GUI app | |
| `dev/tit` | GUI app | |
| `dev/caroline` | GUI app | |
| `kuassa/jreng-eq-strip` | plugin | KANJUT — carries DEBT-20260831T021428 |
| `kuassa/jreng-filter-strip` | plugin | KANJUT — same |
| `iqala` / CIUM | — | No project authored yet |

`eve` is the plugin template; `cast` is the console-app template.

**Related debt (DEBT.md):** DEBT-20260831T021428 (KANJUT wholesale conformance,
validated by JFS building with KANJUT) and DEBT-20260831T021425 (`plugin_bootstrap`
conformance, JFS must build with JAM) both land inside this rollout.

### 2. Windows — blocked CAST-side

`cast/cmake.cast` has only `WIN32` runtime/link branches (`:13`, `:44`); there is no
Windows toolchain flow and no sign/install chain. No Windows build of `cast` exists.
The nvim side is platform-clean already (whatdbg on both platforms, `vim.fs` paths,
`isdirectory` gating), so the Windows DX is inert until a Windows `cast` binary and a
Windows fence chain exist. Untestable from this machine.

### 3. Known engine/authoring items (COUNSELOR territory)

- **Manifest canonicalizer defect** — `format()` scrambles the `## toolchain` table's
  `argument` column against `flag` when consecutive data rows share no `+---+`
  separator. Workaround in force: every row carries its own separator line, applied
  uniformly in both `cast/project-info.md` and `eve/project-info.md`.
- CLAP is cells-held — wiring lands at `jam_clap` conformance.
- Debug AAX is unsigned (Pro Tools requires PACE even for dev builds).
- QA date stamps are UTC.

---

## Historical record

`cake` (`~/Documents/Poems/dev/cake/`) was declared obsolete by CAST and its repo
deleted wholesale (ARCHITECT command).

`RFC-toolchain-parity.md` (cast repo root) catalogued everything the centralized JAM
toolchain produced beyond configure/build — framework-root `.clangd`, `xattr -cr` +
`codesign --verify`, notarization stapling, a signed `.pkg` for console apps, AAX
PACE signing, dated QA archives. The parity sprint that followed delivered every item
on that list.

Defects found and fixed during Phase 3/4 and now closed: the swapped `## toolchain`
`argument` column; `Processor::run()` discarding all child-process output (now
streams through `jam_subprocess`); `jam_Subprocess`'s heap bleed and byte-cap unit
bug; the `Builds/Ninja` → `Builds` layout assumption across the (now-deleted) nvim
build scripts; the always-run `post-build` custom target that solved Ninja's
`POST_BUILD` relink-skip no-op; `jam_markdown`'s hb dependency re-keyed to
`__has_include`; `ColourId` relocated to the unguarded `jam/generated/jam_ColourIds.h`.
