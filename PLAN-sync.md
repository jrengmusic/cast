# PLAN: cast --sync + region-patch outputs — kernel sync against the hand-authored oracle

**STATUS: COMPLETE — sprint logged 2026-09-10; only the live sync run remains, ARCHITECT's keystroke (see SPRINT-LOG residuals)**

**RFC:** RFC-sync.md (stale, superseded by session evidence) + ARCHITECT rulings this session
**Date:** 2026-09-10
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + JAM

## Context

The oracle exists: `~/Documents/Poems/kuassa/user_modules` is the hand-authored byte-exact target for transform(jam). This sprint gives cast two capabilities and the data that drives them:

1. **Region-patch outputs** — an `## output` row can target an existing file through `- [begin]:` / `- [end]:` delimiter bindings; the engine replaces the lines between the delimiters with the rendered shape. First use: every JUCE module declaration block is generated from one table — vendor/website/license become constants declared once (SSOT).
2. **`cast --sync <source-root> <target-root>`** — deterministic kernel sync, proven by write-if-different silence.

**Measured law (scratchpad harness, verified, zero collateral):** four tokens (`namespace jam`→`namespace kuassa`, `jam::`→`kuassa::`, `jam_`→`kuassa_`, `JAM_`→`KUASSA_`) + ordered literal pairs + whole-word `JAM`→`Kuassa` reproduces the oracle except: padding-only drift in cast/identifiers.md (closed by md canonicalization), 4 CRLF files (closed by LF normalization), 3 stale fossil comments (correct overwrites), the oracle's own `*.jam` prompt bug in text.md (law fixes it), and ~20 hand-flavored prose lines (closed by the jam prose cleanup ruling).

**No sync↔region coupling:** region interiors converge under the pair law itself (`JRENG! Architectural Modules`→`Kuassa`, whole-word `JAM`→`Kuassa` gives `Kuassa Core`, `jrengmusic.com`→`kuassa.com`). Both sides regenerate to the same bytes; sync needs no region exclusion. This supersedes the earlier region-exclusion statement.

## ARCHITECT rulings (this session, all closed)

- cast scope: sync transforms cast/ data files; `CAST.md` target-owned (ignore row); `generated/` re-cast in target.
- Deletions: mirror-delete source leftovers only; provision never walked; ignore rows protect.
- Data file `user-modules-info.md` at each root; tables `## identity` (key | value | format, `word` class), `## module` (name | class, kernel/provision), `## ignore` (wildcard rows). **One `## module` table** carries the declaration columns and serves both sync and generation; each CAST.md indexes `../user-modules-info.md`.
- Region-patch capability **in this sprint**; reserved binding names **`[begin]` / `[end]`** ratified.
- Framework display name = `Kuassa`; `CODING.md`/`MANIFESTO` prose references stay jam-worded; jam prose cleanup approved.

## Verified sync law (Step 1 output — the contract)

Ordered, longest source first, after the four tokens:

| # | Source | Target | Class |
|---|---|---|---|
| 1 | `© 2025. JRENG. All rights reserved.` | `© 2010-2025. PT Kuassa Teknika. All rights reserved.` | literal |
| 2 | `JRENG! Architectural Modules` | `Kuassa` | literal |
| 3 | `jrengmusic.com` | `kuassa.com` | literal |
| 4 | `com.jreng` | `com.kuassa` | literal |
| 5 | `` `.jam` `` | `` `*.kuassa` `` | literal (backticks in the literal) |
| 6 | `*.jam` | `*.kuassa` | literal |
| 7 | `JRENG` | `Kuassa` | literal |
| 8 | `JReng` | `Kuassa` | literal |
| 9 | `Jrng` | `KSS_` | literal |
| 10 | `jam-` | `kuassa-` | literal |
| 11 | `JAM` | `Kuassa` | word — both neighbors absent or non-word (`[A-Za-z0-9_]`); `JAMSheetContentView`, `_JAMAICA`, `JAMO` never match |

Plus: path segments `jam_`→`kuassa_`; text files normalized to LF (SPEC §10:876 already states LF); binary = NUL in first 8000 bytes, byte passthrough; cast-scope `.md` files canonicalized through the jam markdown writer after transform.

Ignore rows: `cast/CAST.md`, `cast/mermaid.md`, `cast/terminal.md`, `*.spv.d`, `*.DS_Store`.

## Dependency & API Inventory

- Writer group machinery: `Writer::toFile` group loop (Writer.h:154-207) — region mode branches here; render via existing `apply()` (Writer.h:337-369); write-if-different comparison (Writer.h:201-206). Region rows render shape only: no banner, no §6.8 file comment.
- Bindings: `Model::getBinding` (Model.h:413-438) with bracketed marker identifiers, same pattern as `[comment]`/`[list]` markers (Writer.h:287-290).
- Info-file parse: `Model::parse` handles a table-only file (Model.h:38, :706-722). Sync reads both roots through it.
- CLI: reserved flags main.cpp:40-43, `reservedFlags` main.cpp:148, dispatch main.cpp:314-344. `sync` joins the set; `--sync` gets an argc-4 arm.
- Walk/copy/write: `juce::File::findChildFiles`, `loadFileAsData`, `replaceWithData`, `getRelativePathFrom`; `juce::String::matchesWildcard` for ignore rows; `juce::String::replace` for literal pairs; `Jobs::run` per file (Jobs.h; Processor.h:84-88 pattern); write-if-different shape Processor.h:185-198.
- Diagnostics: new `fail…` rows join text.md Diagnostics family.
- Module declaration field union (34 headers surveyed): ID, vendor, version, name, description, website, license, dependencies, OSXFrameworks, iOSFrameworks, LinuxLibs, windowsLibs, minimumCppStandard, searchpaths. Constants per framework: vendor, license (with per-module overrides: freetype vendor stays constant, license `FreeType License (BSD-like)`, website `https://freetype.org`, version `2.13.3`; subprocess `0.1.0`).
- New names: `Source/Sync.h`, struct `Sync`; reserved bindings `[begin]`/`[end]` (ratified); identifiers.md gains `@string moduleVendor` (joins the `manufacturerCode` family, Rule 5 — jam `JRENG! Architectural Modules`, KANJUT `Kuassa`); declaration shape fence `moduleDeclaration` in code.cast (fence names are free text, SPEC §4.3). One camel/word primitive: whole-word replace, static in Sync.h, cast-local.

## Validation Gate

COUNSELOR validates each step against MANIFESTO.md, NAMES.md, ~/.carol/CODING.md, and this locked plan before the next. @Auditor runs once, after the final step.

## Steps

### Step 1: Pair-law harness — DONE
Verified 2026-09-10: zero collateral, residual fully classified (26 files: 1 padding-only, 4 CRLF-only, 3 fossil overwrites, 1 text.md prompt fix, 17 prose-cleanup lines, all module-declaration drift self-heals via Step 4 regeneration). Harness: scratchpad `verify_law.py`. Its output is the byte contract for Step 7.

### Step 2: Engine — region-patch outputs
**Scope:** `SPEC.md`, `Source/Writer.h`, `Source/Validator.h` (only if a pre-write check fits its family), `cast/text.md`, `Source/HELP.md`, regenerated `Source/generated/`
**Action:** SPEC first: §1 reserved list gains `[begin]`/`[end]`; a new §6 subsection defines region rows — an output row whose structure declares `- [begin]:` and `- [end]:` bindings targets an existing file; the engine replaces the lines strictly between the first line containing the `[begin]` value and the first subsequent line containing the `[end]` value with the rendered shape; delimiter lines stay; no banner, no file-header comment renders. §10.1 gains: region row's file does not exist; `[begin]` without `[end]` or reverse; a delimiter value matching no line, or `[end]` matching at or before `[begin]`; a file shared between region rows and whole-file rows. Then @Engineer implements in Writer's group path (Writer.h:154-207 branch), diagnostics rows in text.md, HELP re-derived. Design by Contract, CODING.md CRITICAL RULES, no comments/doxygen (post-audit pass).
**Validation:** compiles; `./cast cast/CAST.md` fixpoint holds; every new fatal matches SPEC exactly; `[begin]`/`[end]` marker identifiers built exactly like the `[comment]` marker (Writer.h:287-288); no new names beyond this plan.

### Step 3: Data — user-modules-info.md at both roots
**Scope:** `dev/jam/user-modules-info.md` (new), `kuassa/user_modules/user-modules-info.md` (new), `dev/jam/cast/identifiers.md` + `user_modules/cast/identifiers.md` (one `moduleVendor` row each)
**Action:** @Engineer authors both info files, identical shapes. `## identity`: namespace, filePrefix, macroPrefix, hyphenPrefix + the value keys behind pairs 1-9 of the law, word-class rows marked `word`. `## module`: every module on disk — jam 20 rows (13 kernel, 7 provision), KANJUT 14 (13 kernel + `kuassa_aquatic_prime` provision), plus `cast`, `resources`, `patch` kernel scope rows — with declaration columns: version, name, description, website, license, dependencies, OSXFrameworks, iOSFrameworks, LinuxLibs, windowsLibs, minimumCppStandard, searchpaths (blank cell = field omitted, blank-is-nothing law). Values verbatim from the survey; freetype/subprocess overrides carried. `## ignore`: the five rows of the law. `moduleVendor` row joins each identifiers.md.
**Validation:** kernel sets correspond 1:1 after filePrefix transform; every `<filePrefix>*` directory declared; both files parse through `Model::parse`; no value invented — each cites a surveyed header or the Step 1 contract.

### Step 4: Wiring — declaration regions generated in both frameworks
**Scope:** `dev/jam/cast/CAST.md`, `dev/jam/cast/code.cast`, all 20 jam module headers (regions regenerated); `user_modules/cast/CAST.md`, `user_modules/cast/code.cast`, all 14 KANJUT headers
**Action:** @Engineer adds the `moduleDeclaration` shape to each code.cast; each CAST.md gains an index row for `../user-modules-info.md` and one region output row per module header (file = `../<module>/<module>.h`, source = the `## module` row filtered by name, `[begin]` = `BEGIN_JUCE_MODULE_DECLARATION`, `[end]` = `END_JUCE_MODULE_DECLARATION`, constants — vendor, website — bound through identifiers.md addresses). Run `cast` in each root: regions regenerate; jam's four vendor variants, jam_vulkan's `Kuassa` fossil and `jam.com` URL, jam_debug's blank website, and both debug headers' alignment normalize to the template.
**Validation:** second cast run writes nothing in each root; every header's region matches the template rendering; content outside regions untouched (diff shows region lines only); JUCE module format keys all present per module as surveyed.

### Step 5: jam prose cleanup
**Scope:** ~17 prose lines across jam kernel files (kuassa-side names from the Step 1 residual map back to: jam_core/text 3 files stay as-is — target stale, overwrite is correct; jam_core/utils/jam_HashMap.h, jam_debug StyleDebug, jam_dsp TESTING.md + 5 headers, jam_graphics WavefrontObj, jam_gui 2 files, jam_style StyleCustom, jam_vulkan 4 files) + LF-normalize `jam_dsp/engine/jam_TrinsientAnalogModel_V2.{h,cpp}`, `patch/juce-cached-image-factory-hook.patch`, `patch/juce-vulkan-engine-hook.patch`
**Action:** @Engineer rewrites the framework name in prose to whole-word `JAM` where the residual diff shows a framework-name slot (`jam DSP framework` → `JAM DSP framework`, `jam debug windows` → `JAM debug windows`, `jam LookAndFeel` → `JAM LookAndFeel`, TESTING.md's bare `jam` prose lines, `no framework consumer` stays — target's `KANJUT consumer` normalizes on sync). `CODING.md`/`MANIFESTO` references unchanged (ruling). Convert the 4 CRLF files to LF. Destructive-Edit Discipline: dry-run counts, backup, verify.
**Validation:** rerun the Step 1 harness — the prose residual class collapses to the enumerated expected-overwrite list; no new collateral; jam builds untouched files only where prose/line endings changed.

### Step 6: Engine — Sync.h + --sync
**Scope:** `Source/Sync.h` (new), `Source/main.cpp`, `SPEC.md`, `cast/text.md`, `Source/HELP.md`, regenerated `Source/generated/`
**Action:** SPEC first: §2.1 grammar `cast --sync <source-root> <target-root>`; §1 hardcode list gains the info-file name, three table names, columns `key`/`class`, keywords `kernel`/`provision`, composed identity keys (namespace, filePrefix, macroPrefix, hyphenPrefix), `word` format keyword; a sync operation section states the ordered longest-first law, word boundary, path transform, LF normalization, binary passthrough, cast-md canonicalization, ignore matching, mirror-delete law, report contract (written + deleted paths, zero lines = in sync), `generated/` re-cast note; §10.1 gains: info file missing; required identity key missing; kernel row without directory / undeclared `<filePrefix>*` directory; kernel sets not corresponding; source contamination (target token in source text, names file + token); source root equals target root. Then @Engineer: `sync` joins reservedFlags; argc-4 arm; Sync.h — parse both info files via `Model::parse`, fatals as `isValid` Result predicates (Validator family), ordered replacement list composed + sorted, whole-word static primitive, kernel walk with ignore skip, transform, write-if-different, `Jobs::run` parallel, deletion pass, stdout report; diagnostics rows in text.md.
**Validation:** compiles; cast fixpoint holds; fatal set matches SPEC §10.1 exactly and adds none; no bail-out guards, alternative tokens, `.at()`; MVP data-flow contract respected — Sync holds no state between runs.

### Step 7: Sandbox proof, then live
**Scope:** scratchpad sandbox copy of `user_modules`; live run is ARCHITECT's
**Action:** copy `user_modules` to scratchpad; `cast --sync <jam> <sandbox>`; report must equal the Step 1+5 expected list exactly — zero unexpected writes, zero deletions beyond the enumerated set; second run writes nothing. Then ARCHITECT runs live sync, runs `cast` in `user_modules`, builds.
**Validation:** sandbox report == contract; second run silent; live confirmation is ARCHITECT's.

## BLESSED Alignment

- **B** — the `[begin]`/`[end]` pair is a declared ownership boundary inside a hand file: inside cast's, outside the author's. jam owns the kernel; the target owns provision and CAST.md.
- **L** — Sync.h has one responsibility; the region branch reuses Writer's render path; pairs, classes, and ignores are data rows, not branch chains.
- **E (Explicit)** — every replacement and delimiter is a declared row; every fatal names file + rule; the report lists exactly what changed.
- **S (SSOT)** — vendor/website/license declared once; one `## module` table serves sync and generation; `generated/` and region interiors are never carried, always re-cast.
- **S (Stateless)** — Sync and the region writer hold nothing between runs.
- **E (Encapsulation)** — Sync reads no manifest, renders no template; regions ride the existing Writer group path without touching Shapes/Items.
- **D** — same jam + same tables → byte-identical target; write-if-different silence is the verdict on both engines.

## Risks / Open Questions

- **Region render vs. current header bytes:** Step 4's first run rewrites declaration blocks to the template's rendering — alignment normalizes, the survey's five anomalies disappear. Any field the template drops would surface as a JUCE module-format build failure at the next project build; the survey's field union is the template's column set, so none is expected.
- **Constants through bindings:** vendor/website bind through identifiers.md addresses on the region rows; if address resolution at that position needs a form the engine lacks, the fact stops the step and comes back with a citation — no workaround invented.
- **DEBT.md** two open entries (OpenSSL /MTd; plugin_bootstrap) — outside named scope; ARCHITECT's disposition.
