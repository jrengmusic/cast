# PLAN: KANJUT Oracle — hand-produce the target tree from jam canon

**RFC:** none — objective from ARCHITECT prompt (RFC-sync.md §8 superseded; its mirror/diff framing has no reader here)
**Date:** 2026-09-09
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + JAM — LANGUAGE.md §"C++ / JUCE" is the reference implementation, all principles as written

---

## Context

`cast --sync` cannot be written against a guess. This sprint produces the **oracle**: a hand-made
`___lib___` that is jam conformance — mechanically renamed jam plus hand-authored provision — which
builds `jreng-filter-strip` with Aquatic Prime live. Once it exists, the verb is iterated until
`cast --sync` reproduces it byte-for-byte, and write-if-different (`Processor.h:192`) is the pass:
a run that writes nothing is the proof. The residual diff against the oracle *is* the `## provision`
declaration — measured, not guessed.

**jam is canon.** Nothing target-only survives except what a provision guard protects.
The engine, `user-modules-info.md`, and the SPEC amendments are **not** this sprint.

---

## Language / Framework Constraints

- LANGUAGE.md C++/JUCE: 300-line file threshold is a smell detector, not a split mandate. No file is
  split in this sprint; the 30-line/3-branch thresholds apply unchanged.
- No code is authored in cast's `Source/`. This sprint is data, tree production, and product code.

---

## Dependency & API Inventory

**Module set — 13 kernel**, from `dev/jfs/CMakeLists.txt:292-309`: `jam_core`, `jam_debug`,
`jam_data_structures`, `jam_dsp`, `jam_gui`, `jam_graphics`, `jam_animation`, `jam_freetype`,
`jam_vulkan`, `jam_style`, `jam_markdown`, `jam_web`, `jam_plugin_bootstrap`.
Excluded 7: `jam_audio_devices`, `jam_clap`, `jam_fuzzy`, `jam_mermaid_diagram`, `jam_subprocess`,
`jam_terminal`, `jam_whelmed`.

**Separation confirmed.** No `dependencies:` line among the 13 names any of the 7. One cross-include
exists — `jam_markdown/jam_markdown.h:29-31`, guarded `#if JUCE_MODULE_AVAILABLE_jam_mermaid_diagram
&& JAM_MARKDOWN_MERMAID` — inert when the module is not linked.

**Registration is project-side.** `cmake.cast:166`'s `:::[list]:::` is filled per `## user module`
row (`cast/CAST.md:42` wires `@project-info:user module` → `@cmake:module`). `jam/CMakeLists.txt` is
not consumed by the cast toolchain.

**Token rows — four, all delimited, no new string primitive:**

```
namespace jam  →  namespace kuassa
jam::          →  kuassa::
jam_           →  kuassa_
JAM_           →  KUASSA_
```

Applied to file contents **and** path segments. Verified safe: `jam_*/**` carries no quoted or
bracketed bare `jam`; vendored FreeType's 14 substring hits (`Hangul Jamo`, `TT_UCR_HANGUL_JAMO`
`ttnameid.h:924`, `TT_MS_LANGID_ENGLISH_JAMAICA` `:488`, `jammed` `ftobjs.c:1665`) match none of the
four. `kuassa_aquatic_prime/` carries zero `jam` hits, case-insensitive.
Bare `jam` survives in doxygen prose (`jam_Buffer.h:3`, `jam_Block.h:3`) — deterministic, no code effect.

**Established patterns reused, not invented:**
- Table file family in `jam/cast/`: `bimaps.md`, `chars.md`, `colours.md`, `files.md`,
  `identifiers.md`, `lookuptables.md`, `mermaid.md`, `syntax.md`, `text.md`, `entities.md`.
  `terminal.md` joins it (NAMES Rule 5, nearest sibling — no ratification).
- Generated-header family in `jam/generated/`: `jam_Bimaps.h`, `jam_Colours.h`, `jam_Mermaid.h`, …
  `jam_Terminal.h` joins it.
- Umbrella wiring: `CAST.md:2419-2429 ## output index` builds `jam_Generated.h`'s include list from
  `- [list]: @headers` and its member list from `> - [list]: instance` — a binding-name selector
  (SPEC §6.5). Adding a `## headers` row adds the include; the `- instance:` bindings travel with
  their own `## output` rows.
- `## project info` row family, `dev/jfs/project-info.md:23-45` — `publicKey` joins it.
- Destructive-Edit Discipline (CAROL) governs the seed script: dry-run with match counts → backup →
  write → post-count reconciliation.

---

## Validation Gate

Each step is validated by COUNSELOR before the next — against MANIFESTO.md (BLESSED), NAMES.md,
~/.carol/CODING.md, and this locked PLAN. @Auditor runs **once**, after the final step, covering the
whole sprint.

---

## Steps

### Step 1: Split the terminal vocabulary into its own table and its own output
**Scope:** `jam/cast/bimaps.md`, `jam/cast/terminal.md` (new), `jam/cast/CAST.md`,
`jam/generated/jam_Terminal.h` (generated), `jam/generated/jam_Bimaps.h` (regenerated),
`jam/generated/jam_Generated.h` (regenerated)

**Action:** Move these 23 tables out of `bimaps.md` into a new `cast/terminal.md`, verbatim, in file
order: `Screen` (:11), `MouseTracking` (:812), `DEC` (:1223), `OSC` (:1254), `SGR` (:1277),
`ColorMode` (:1328), `UnderlineStyle` (:1339), `CSI` (:1354), `WindowOps` (:1399), `DSR` (:1411),
`TabClear` (:1422), `CursorShape` (:1433), `DECRQSS` (:1449), `CsiIntermediate` (:1466), `ESC`
(:1481), `CharsetIntermediate` (:1499), `CharsetDesignator` (:1512), `DecEscIntermediate` (:1523),
`DecEscFinal` (:1533), `ModeReport` (:1543), `ANSI` (:1557), `ShellIntegration` (:1568),
`KeyboardAssignMode` (:1581).

In `CAST.md`: add `| @terminal | terminal.md |` and `| @jam_Terminal | ../generated/jam_Terminal.h |`
to `## index`; retarget the 23 `## output` row blocks (`:206-215`, `:582-588`, `:897-1203`) from
`@bimaps:<Name>` to `@terminal:<Name>` and from `@jam_Bimaps` to `@jam_Terminal`, carrying their
`- instance:` bindings unchanged; add a `jam_Terminal.h` row to `## headers` (`:89-157`) in the
family's existing brief style.

**Validation:** `cast cast/CAST.md` succeeds. `jam_Bimaps.h` loses exactly 23 registries,
`jam_Terminal.h` gains exactly those 23, `jam_Generated.h` gains one include and keeps all 23
`- instance:` members. A second run writes nothing. `jam_terminal` and `jam_whelmed` still compile.
No table content edited — move only.

---

### Step 2: Delete the dead fonts include
**Scope:** `jam/jam_debug/jam_debug.h`

**Action:** Delete line 30, `#include <JamFontsBinaryData.h>`.

**Validation:** No `fonts::` symbol exists anywhere in `jam_debug` — the only consumer of
`jam::fonts::` in all of jam is `jam_whelmed/style/jam_StyleWhelmed.cpp:30,33,36,39,42,45`, an
excluded module. `jam_debug` compiles. `JamFontsBinaryData.h` no longer reaches the 13.

---

### Step 3: Rename the Vulkan shader header to the filePrefix form
**Scope:** `jam/jam_vulkan/jam_vulkan.h:215`, `dev/jfs/cast/cmake.cast:335`

**Action:** `JamVulkanShaderData.h` → `jam_VulkanShaderData.h` in the include and in the
`HEADER_NAME` argument. `NAMESPACE "jam"` at `:334` is unchanged — it is project-side and the KANJUT
template is authored separately (Step 8).

**Validation:** `dev/jfs` builds. The name is now covered by the `jam_` token row, so no Pascal
rename pair exists in the row set.

---

### Step 4: Delete the framework CMakeLists
**Scope:** `jam/CMakeLists.txt`

**Action:** Delete the file. It is the only CMake artifact in jam (`**/*.cmake` → none) and is not
consumed by the cast toolchain.

**Validation:** `dev/jfs` configures and builds. No framework-level CMake remains in jam.

---

### Step 5: Rename both old trees to reference-only
**Scope:** `kuassa/___lib___`, `kuassa/jreng-filter-strip`

**Action:** ARCHITECT renames both to reference names of his choosing. Each carries its own `.git`,
so no history is affected. They are read-only sources for Steps 7 and 9 and are deleted only after
the sprint's gate passes.

**Validation:** Both readable at their new paths; canonical paths free.

---

### Step 6: Produce the seed
**Scope:** new `kuassa/___lib___`

**Action:** Under Destructive-Edit Discipline — dry-run printing per-row match counts, backup, write,
post-count reconciliation — copy from jam into the new tree and apply the four token rows to file
contents and path segments:

| Carried | Not carried |
|---|---|
| the 13 kernel module directories | the 7 excluded module directories |
| `cast/` — `CAST.md`, `code.cast`, and 8 data files | `cast/mermaid.md`, `cast/terminal.md` |
| `generated/` | `CMakeLists.txt` (deleted in Step 4), `docs/`, `carol/`, `DEBT.md`, `RFC-*.md`, `PLAN-*.md`, `tagfile.xml`, `DOCS.html`, `jam.txt` |
| `resources/` (svg 38, spv 54, shaders 31, fonts 6, `mermaid.css`) | |
| `patch/` (5 files) | |

Then prune the seed's `cast/CAST.md`: delete the `@mermaid`, `@jam_Mermaid`, `@terminal`,
`@jam_Terminal` `## index` rows; the `## output` row blocks writing `@jam_Mermaid` (from `:1406`) and
`@jam_Terminal`; and the `jam_Mermaid.h` / `jam_Terminal.h` rows from `## headers`. Delete
`generated/kuassa_Mermaid.h` and `generated/kuassa_Terminal.h`.

**Validation:** Zero occurrences of `namespace jam`, `jam::`, `jam_`, `JAM_` in the new tree.
Vendored FreeType byte-identical to jam's except the directory name. No `@`-alias in `CAST.md` names a
missing file (SPEC §4.4). `cast ___lib___/cast/CAST.md` regenerates `generated/` and a second run
writes nothing. `kuassa_Generated.h` carries no `Mermaid::` and no terminal member.

---

### Step 7: Author the provision
**Scope:** new `kuassa/___lib___` — `kuassa_aquatic_prime/` and 8 kernel files

**Action:** Carry `kuassa_aquatic_prime/` verbatim from the reference tree (`AquaticPrime.h` 212,
`AquaticPrime.cpp` 368, `kuassa_AquaticPrime.h` 8, `kuassa_AquaticPrime.cpp` 3,
`kuassa_aquatic_prime.h` 32, `kuassa_aquatic_prime.cpp` 28, plus vendored openssl). Then hand-add,
verbatim from the reference tree, each guarded region into its renamed counterpart:

| File | Regions |
|---|---|
| `kuassa_core/kuassa_core.h` | the `KUASSA_USING_AQUATIC_PRIME` default, beside the `KUASSA_USING_OVERSAMPLING` block |
| `kuassa_data_structures/parameter/kuassa_ParameterManager.h` | 3 methods; 2 members; `Ext::license()` |
| `kuassa_data_structures/parameter/kuassa_ParameterManager.cpp` | ctor inits — `publicKey` now from `ProjectInfo::publicKey` |
| `kuassa_data_structures/model/kuassa_AudioModel.h` | 7 methods, replacing the unguarded `isEvaluating()` stub |
| `kuassa_data_structures/model/kuassa_AudioModel.cpp` | `refreshEvaluationStatus (true)` inside `setState`; the 7 bodies |
| `kuassa_gui/windows/kuassa_MessageBox.h` | 3 templates |
| `kuassa_gui/file_chooser/kuassa_FileChooser.h` | `copyLicense` |
| `kuassa_plugin_bootstrap/view/kuassa_ViewManagerContent.cpp` | the `Id::user` row; the `#if !` visibility arm |

Also confirm `onUserTreeChanged` exists on the renamed `AudioModel`; if jam has no such member it is
part of this guarded surface.

**Validation:** Exactly these 8 files carry the guard, nothing else outside
`kuassa_aquatic_prime/`. Regions are byte-identical to the reference tree modulo the four token rows.
Each region compiles with the macro at 0 and at 1.

---

### Step 8: Produce the new product tree
**Scope:** new `kuassa/jreng-filter-strip`, from `dev/jfs`

**Action:** Copy `dev/jfs` and apply the four token rows. Then edit its `project-info.md`:
the `@user-module` index alias (`:10`) points at `___lib___`; the `## user module` table (`:297-327`)
carries the 13 renamed rows plus a `kuassa_aquatic_prime` row; the define (`:376`) becomes
`KUASSA_USING_AQUATIC_PRIME=1`; and a `publicKey` row joins `## project info` (`:23-45`) as
`@char` / `toLiteral`, its value from the reference tree's `Source/layout/metadata.md:18`.
Author `cast/cmake.cast` for KANJUT: `NAMESPACE "kuassa"`, `HEADER_NAME "kuassa_VulkanShaderData.h"`,
the fonts binary-data block, and the `kuassa_freetype` warning-suppression paths (`:262-268`).

**Validation:** No `jam` token remains. `cast cast/CAST.md` regenerates and a second run writes
nothing. It configures against the new `___lib___`.

---

### Step 9: Restore the license lane
**Scope:** new `kuassa/jreng-filter-strip/Source`

**Action:** Port from the reference tree onto the current jfs shape, each guarded by
`#if KUASSA_USING_AQUATIC_PRIME`:

- `FilterStripProcessor.cpp` — the `model.onUserTreeChanged` callback setting
  `audioProcessor.setNoiseEnabled (model.isEvaluating())`, and `model.refreshEvaluationStatus (true)`,
  into the constructor body (reference `:30-38`; current body is empty at `:29-30`).
- `FilterStripView.cpp` — the `globalFocusChanged` override (reference `:43-70`); absent entirely in
  jfs.
- `FilterStripViewPanelCallbacks.cpp` — an `importLicense` action calling
  `fileChooser.copyLicense (model)`, expressed in the current `menuActions.add<>` form (`:223-236`),
  not the old `case` form (reference `:198-202`).

No change needed to `Source/layout/AboutLayout.html:120` (`licensedTo` label is byte-identical),
`generated/Identifiers.h:68` (`licensedTo`), or `generated/Bimaps.h:56-71`
(`importLicense` already `conditional`).

**Validation:** Every restored site uses the current jam API — `map::PluginMenu::visibilities`, not
`conditionals`; `juce::Identifier` keys, not `.toStdString()`. No name appears that is not already in
the tree or in this plan (CODING.md, no identifier latitude).

---

### Step 10: Build and prove
**Scope:** new `kuassa/jreng-filter-strip`

**Action:** Iterate the build until it compiles and links against the new `___lib___` with
`KUASSA_USING_AQUATIC_PRIME=1`. Every compile failure is fixed in the oracle — never by editing jam.

**Validation:** Builds clean. ARCHITECT confirms the license lane at runtime: `licensedTo` renders,
the focus-change prompt fires, `importLicense` imports, evaluation noise follows `isEvaluating()`.

---

## BLESSED Alignment

- **B** — every carried artifact has one owner: jam owns the kernel, the oracle owns provision.
  The reference trees are read-only and deleted only after the gate.
- **L** — no file is split or created to move lines. `terminal.md` and `jam_Terminal.h` separate a
  *responsibility* (terminal vocabulary) that the 13 kernel modules do not reference at all.
- **E (Explicit)** — the four token rows are named and ordered; nothing is inferred from context.
  Every prune is a declared row deletion, never a silent omission.
- **S (SSOT)** — jam is the single source; the oracle holds no second copy of anything jam owns.
  `publicKey` lands in the one table that already owns product metadata.
- **S (Stateless)** — the seed script holds no state between runs; the tree is its only output.
- **E (Encapsulation)** — provision is bounded by the macro guard and by one module directory.
  Kernel files carry regions, not a parallel API.
- **D** — the sprint's whole purpose is a deterministic target: the same jam plus the same rows must
  yield the same tree, which is what the next sprint's fixpoint measures.

---

## Risks / Open Questions

- **`jam_style/jam_StyleMermaid.h` is a kernel file with mermaid-specific content**, reading
  `files::mermaidStyleSheet` (from `jam_Files.h`, not `mermaid.md`) and `resources/mermaid.css`.
  Dropping `mermaid.md` does not break it, and `resources/` is carried whole — but it means KANJUT
  inherits a Mermaid stylesheet path. Not a blocker; flagged for ARCHITECT's ruling.
- **`jam_LookupTables.h`'s brief names terminal colour tables** ("Direct-indexed lookup tables for
  terminal colour, markup, and syntax dispatch"). `lookuptables.md` is carried whole in Step 6. If any
  of its tables are terminal-only, they are dead weight in KANJUT — measurable after Step 6, not
  before.
- **`onUserTreeChanged`** was not found in jam's `AudioModel` public list. Step 7 resolves it as
  provision if jam genuinely lacks it.
- **Steps 5 and 10 are ARCHITECT's hands** — the renames and the runtime confirmation.
