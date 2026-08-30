# PLAN — CAST as THE TOOLCHAIN

**Scope:** Three phases, one sprint each. END is the first consumer; KANJUT/JFS/CIUM conform later to the identical pattern.
**Decided this session:** CAST = **Codegen Annotated Source of Toolchain** (ARCHITECT; lands in SPEC/HELP at Phase 3.2) · one PLAN.md (cast repo, phase = sprint) · END identity: name `END`, plugin code `END.`, manufacturer `JRNG`, version `0.1.0` · formats `Standalone VST3 AU` · --help becomes plain print of HELP.md · `## toolchain` is the reserved manifest table name (ratified).
**Law:** cast owns everything deterministic (tables → text, fixpoint-verified). CMake keeps only machine probing + target topology. JUCE's CMake API is consumed as-is (v8 first-class channel: `juce_add_plugin` → compile definitions, JUCEUtils.cmake:1541/1590-1629). Ninja executes.

---

## Phase 1 — END Oracle (hand-authored CMakeLists.txt + window harness)

Goal: standalone plugin window (vulkan + glass) building from a from-scratch CMakeLists.txt that touches **zero** JAM cmake toolchain (no BuildSetup, no PluginBuilder, no Metadata.cmake). This file is THE ORACLE for Phase 2.

1. Rename `end/CmakeLists.txt` → `CMakeLists.txt` (canonical casing) and rewrite from scratch:
   - JUCE via `add_subdirectory(~/Documents/Poems/JUCE)` path resolution — explicit, no BuildSetup
   - `juce_add_plugin` direct: PRODUCT_NAME "END", PLUGIN_CODE `END.`, PLUGIN_MANUFACTURER_CODE `JRNG`, VERSION 0.1.0, FORMATS Standalone VST3 AU, COMPANY/BUNDLE identity inlined (JRENG / com.jreng)
   - juce + jam module list carried from current file (CmakeLists.txt:56-89) via `juce_add_module` — jam_terminal stays out (current module list is scope)
   - Platform branches inlined: Windows dwmapi/user32 (current :96-105)
   - Whatever jam_vulkan/jam_freetype binary-data steps surface (JamVulkanShaderData.h, JamFontsBinaryData.h — jam/cmake/Metadata.cmake:15-16) — inlined as explicit text. The oracle's job is to surface every hidden BuildSetup/PluginBuilder constant as concrete lines.
2. Minimal sources: ENDProcessor.h/.cpp, ENDView.h/.cpp (currently 0 bytes) — processor passthrough, view = one window with vulkan surface + glass. Window is sufficient; no terminal content.
3. Gate: ARCHITECT configures + builds all three formats; standalone window opens. Oracle frozen on ARCHITECT's "good".

## Phase 2 — Templatization + Convergence (cast strips, END tables, byte-identical)

Goal: cast generates the oracle CMakeLists.txt byte-for-byte from END tables.

1. Strip cast: delete Help.h rendering path (StyleManager/markdown/terminal::Graphics) + style.css; `--help` prints HELP.md raw via BinaryData; module deps reduced to core engine (jam_markdown stays — Model parses with it; jam_terminal/jam_style drop); cast's own CMake trimmed to match.
2. Author together (jam precedent — template variance points are the table fields):
   - `template.cast` blocks: universal JUCE-project CMakeLists skeleton (the cookie cutter)
   - END `cast/` tables: metadata (identity rows), modules, platform, sources — field vocabulary derived from `juce_add_plugin`'s keyword surface (JUCEUtils.cmake `_juce_initialise_target`, :2016 region), extended with domain fields JUCE never sees
   - END `CAST.md` manifest wiring tables → `CMakeLists.txt` output
3. Data additions as surfaced: comment-syntax row for `#` (txt/cmake extension) in comments.md if the template emits comments.
4. Converge: `./cast` on END manifest → diff vs oracle until byte-identical; double-run fixpoint; `--format` zero churn. Same gate as jam 9/9.
5. Gate: diff empty, ARCHITECT round-trip builds clean from the generated file.

## Phase 3 — Ignition + Release Binary (CAST is THE TOOLCHAIN)

Goal: `cast CAST.md` = generate + continue the toolchain; SSOT signed binary.

1. Engine: reserved `## toolchain` table — Validator gate; when present, after generation completes, execute rows in authored order (tool + args via juce::ChildProcess, streamed output, exit code propagated; first failure stops the chain). Absent → pure generate. Generation core stays execution-free.
2. SPEC.md: reserved-table semantics, toolchain-agnostic contract (§ new); HELP.md synced.
3. END adopts: `## toolchain` rows run cmake configure + ninja build. END never touches JAM cmake toolchain again.
4. Release: cast Release build, codesign + notarize (ARCHITECT runs signing/notarization); SSOT binary lives at `framework/cast/` for now (ARCHITECT, this session — exact root confirmed at Phase 3).
5. Horizon (out of these sprints): KANJUT, JFS, CIUM conform — per-project tables + shared template, framework cmake layers (BuildSetup/PluginBuilder/Metadata.cmake/table_parse) retire.

---

**Open decisions carried:** none — install path decided (`framework/cast/`, exact root confirmed at Phase 3).
