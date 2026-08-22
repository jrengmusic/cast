# PLAN: Explicit-template render engine; converge jam/ byte-identical

**RFC:** none — design ratified in ARCHITECT sessions 2026-08-21/22
**Date:** 2026-08-22 (revision 5 — supersedes revision 4 entirely)
**SPEC:** `Source/HELP.md` (rewritten in Step 1 — the governing document)
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE / JAM. LANGUAGE.md C++ override: engine classes stay single-header.

## Context

ARCHITECT simplified the template structure after the Sprint 8 checkpoint: all code shapes now live as literal fenced codeblocks in **one file per data dir** — `jam/cast/template.cast` — where the codeblock **info string is the template id**. All data files are **flat at `jam/cast/`** (no tables/ or template/ subdirs). The abstract vocabulary (prologue/epilogue/terminator/keyword/open/close, Scope/Definition decomposition, type aliases, `## break`/`## namespace` text tables) is dead. The engine reduces to: **token matching (zero hardcoded names) → `replaceholder` + transforms → `:::line:::` vertical join → wrap/tab**. Correctness gate unchanged: 12 oracle files byte-identical + fixpoint.

Additionally, two `--format` grid-table defects ARCHITECT surfaced are fixed this sprint.

## The Design (supersedes rev 4 §The Design)

**One template file. Codeblock id = template. Table mirrors structure + assigns placeholders. Engine only matches and replaces.**

- `template.cast` codeblocks, info string = id: `namespace, chars, char, wchar, struct, sharedInstance, hashMap, pair, identifier, lookupTable, bimap, enum, separator` (+ `include`, authored in Step 4). Reference form from tables: **`template:<id>`**.
- Resolution is framework-native: a parsed fenced codeblock carries `Id::info` (jam_MarkdownDocument.h:1491-1494); body via `getAllSubText()` (jam_Document.h:310-321). `template.cast` parses through `jam::MarkdownDocument::parse` — no custom tokenizer.
- The universal jack `:::line:::` always expands **vertical**: per-source-row build → `jam::Strings::joinIntoString` (jam_Strings.h:1006), default separator newline; jack identity = raw `>` depth (= indent tabs), multi-jack templates pair occurrences with wiring bullets in authored order (decision 13); `template:separator` block = the `//===` sibling/row break.
- All other placeholders are direct cell/binding fills via `jam::Format::replaceholder`/`hasPlaceholder` (jam_Format.h:438-445), with `:::name:transform:::` tags over the closed `Transforms` registry (Operators.h:54-196). Quoting of string keys = **`:::key:quoted:::`** (ARCHITECT-ratified; `Transforms::quoted` exists, Operators.h:128). Cells stay symbol-only.
- Wrap/tab stands: `>` = one tab, deterministic wrapping shapes (namespace, struct). Dead-placeholder residue after all candidates apply = FATAL (no-scanner inversion kept).
- Engine stamps exactly two things itself: the **banner + `#pragma once`** (existing `Writer::getBanner` pattern, Writer.h:240-254; banner art SSOT = `Source/resources/cast-output.md`) and **per-language comment syntax** (`map::commentSyntax` from comments.md). Everything else is template text.
- **Author owns template structure and tokens.** The engine executes them, never interprets or repairs.

## Locked Decisions (revision 5)

| # | Decision |
|---|----------|
| 1 | Four-column manifest stands: `placeholder \| structure \| separator \| file`. Table expression = structure mirror + placeholder-token assignment, nothing else |
| 2 | Placeholder names are data — engine hardcodes none (unchanged) |
| 3 | `>` = raw depth = tabs, uniform across keyed columns (unchanged) |
| 4 | **Templates are explicit code** — one `template.cast`, codeblock info string = id, `template:<id>` reference. No table→alias→fragment indirection for code. Supersedes rev 4 decisions 4, 10, 11, 12 (Scope/Definition/Specialised decomposition, fragment files, `.cast`-per-shape) |
| 5 | First-row law is now template text (bimap's `map.at (0)` is literal code), not an engine mechanism. Engine-side generalization of rev 4 decision 5 is dropped |
| 6 | `text.md ## break` and `## namespace` die — separator is `template:separator`, namespace banner lines are literal text in the `namespace` block. Supersedes rev 4 decision 6 |
| 7 | Engine's only stamps: banner + `#pragma once` + per-language comments. `#pragma once` is part of the banner stamp (Writer.h:240-254) — no fileHead template block |
| 8 | String-key quotes come from the `quoted` transform tag (`:::key:quoted:::`) — cells symbol-only (ARCHITECT-ratified this session) |
| 9 | 12-file oracle gate + fixpoint + jam_Bimaps unification (rev 4 decision 9) unchanged |
| 10 | `--format` malformed grid row (authored cell count/pipe positions disagreeing with header) = **fail fast**: report file:line, refuse to rewrite — no silent pad/truncate (ARCHITECT-ratified this session) |
| 11 | Grid-table writer guarantees a closing bottom border — a table missing it is normalized, never left open |
| 12 | Engine rewrite delete-first, Design by Contract, framework API to fullest, NAMES.md verb semantics (rev 4 decision 13 unchanged). Class contract kept: `TemplateDocument : jam::MarkdownDocument`, `Writer : jam::Document::Writer`, `Model : jam::MarkdownDocument`, `Validator : jam::MarkdownValidator`; `runJobs` survives (Model.h:7-16) |
| 13 | **Universal jack** (ARCHITECT, this session): every jack is `:::line:::` — jack identity is its raw `>` depth, uniform across placeholder/structure/separator columns, and that depth IS the indent (tabs) of the expanded rows. A template with multiple `:::line:::` occurrences pairs them with the row's `- line:` wiring bullets in authored order (Nth occurrence ↔ Nth bullet). Parent fill: a containing template has exactly one `:::line:::`; the child's assembled output fills it — the rev-4 intersection rule is retired. Containment heads (`template:<id>` lines) keep pure-containment depth, no indent arithmetic; only bullet depth carries indent. `mapLine`/`enumLine` vocabulary dies. `line` remains data convention, never an engine symbol |
| 14 | Engine vocabulary ruling: retaining established codebase/framework names (`build` — framework override pattern; `getFile`/`getRow` — prior-sprint ratified; `getCell`; existing Validator dispatch names) is execution, not improvisation — Rule -1 gates only new names. Deleted by name: `getWraps`, `getWrapAlias`, `getTokens`, `getOutermostWrap`, `isWrapHead`, and all engine use of `Id::keyword/prologue/epilogue/terminator/open/close/scope/body/list`. Second-order binding-name selection matches the binding at any depth |
| 15 | **Bimap key type is parametrized, one block**: the bimap template carries `:::type:::` at the three key-type sites; every bimap row binds `- type: int` or `- type: uint32_t` explicitly. No second block — DRY/SSOT |
| 16 | **Oracle banner normalized**: the 80-underscore decorator variant in jam_Bimaps.h / jam_LookupTables.h / jam_Entities.h is pre-fixed to the canonical 81-underscore form (text.md/template.cast SSOT) before convergence |
| 17 | **jam_Generated.h has its own `generated` template block** — literal code: include jack at top (before the namespace opens), namespace frame, struct with the member jack; its two `:::line:::`-family jacks pair with the `- files:` and `- line: instance` wirings in authored order. Cookbook 6's namespace-binding composition is retired |
| 18 | **Bimap redesign (ARCHITECT, census-backed):** `jam::Bimap<Key>` — Value fixed `juce::String`, Derived and `Instance` inheritance dropped (census: zero LIFO use, zero non-singleton constructions; SharedInstance is the independent owner). One arity for all 92: `jam::Bimap<int>` ×91 (11 uint32_t-drifted bimaps revert to int), `jam::Bimap<uint32_t>` ×1 (ColourNames — ARGB keys). One template block, `- type:` binding per row. Each generated struct carries `static X& getInstance()` resolving through SharedInstance's registry — the 220 existing call sites compile unchanged. `getDefault()` non-virtual. jam_VulkanShaderFormat migrates to the same shape. jam_MermaidTables.h bimap sites swept to the new form |
| 19 | **LUT unification (ARCHITECT):** all 15 lookup tables top-level unwrapped — object name IS the LUT; the one-member wrapper structs die. `Xml::classes` ≡ `Html::classes` (byte-identical) merge into one SSOT table **MarkupLanguage**; call sites (jam_XML.h:454, jam_Html.h:163) retarget. `MermaidBlock::classes` stays generated (WIP consumer, zero call sites today — not dead-coded) |
| 20 | **First-row resolution revived (ARCHITECT):** the bimap block's default line is `return map.at (:::value:::);` — an unfilled placeholder naming a source-table column resolves from that table's ROW 0 (rev-4 decision-5 engine rule, narrowly). No `- default:` binding exists; the oracle's at(0)/at(1)/symbolic variants are all row-0 value cells. Templates never carry a literal `map.at (0)` |
| 21 | **Tight struct shape canonical (ARCHITECT):** no blank lines around the member jack — jam_Generated.h's form; jam_Text.h (struct English) and any other airy struct normalized in the oracle |
| 22 | **Thin LUT tables (ARCHITECT):** markdown.md's wide `## vocabulary` matrix restructures into 9 two-column relations (entry \| value), one per char-class LUT, members only — existing table-ref wiring serves them; no new wiring form, no empty-cell semantics |
| 23 | **C4Traits alignment normalized (ARCHITECT):** the oracle's manual numeral-alignment padding in mermaidC4Traits drops to single spaces — cosmetic alignment is not load-bearing data; the generic entry block converges |
| 24 | **Mixed-type identifiers file (ARCHITECT):** jam_Identifiers.h emits the WHOLE lexicon — the identifier block's `:::type:::` resolves from each row's own `type` cell (`@id` → `juce::Identifier`, `@string` → `juce::String`); no filter wiring exists. Oracle amended: the 20 `inline const juce::String` lines join at their authored lexicon row positions |
| 25 | **Uniform LUT brief line (ARCHITECT):** every lookup-table construct carries the doc-brief line exactly like bimaps — `/** @brief  */` empty where undocumented, filled where authored. Oracle normalized (the 14 clean LUTs gain the empty line); the lookupTable block gains the leading `:::comment:brief:::` line; every LUT row binds `- comment:` (empty allowed). No optional-line engine primitive exists |

## Dependency & API Inventory (Pathfinder, verified)

- `jam::Format::replaceholder`/`hasPlaceholder` — jam_Format.h:438-445, default delimiter `Id::tripleColon`
- `jam::Strings::joinIntoString` — jam_Strings.h:1006
- Fenced codeblock: `Id::type=codeBlock`, `Id::info` = info string — jam_MarkdownDocument.h:1491-1494; body via `getAllSubText()` jam_Document.h:310-321
- `Transforms` registry incl. `quoted` — Operators.h:54-196, :128; transform-tag application TemplateDocument.h:442-473
- Banner/pragma stamp — Writer.h:240-254; comment syntax — comments.md → `map::commentSyntax`
- `--format` flow — main.cpp:132-139 → Processor::format() Processor.h:29-61 (parse → validate → canonical text → write-if-different)
- Grid-table parse: `addGridTableLine` jam_MarkdownDocument.h:2363-2436; malformed-row pad/truncate at :2415-2426; rows/borders append in authored order (jam_Document.h:768-784) — the on-disk border damage (terminal.md:366, :377) is prior-state corruption, repaired as data in Step 4
- Grid-table write: jam_MarkdownWriter.h:421-443 (replays border children; no closing-border guarantee today — Step 2 adds it)

## Validation Gate

Each step validated by COUNSELOR before the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked decisions above. @Auditor runs ONCE, after the final step. ARCHITECT builds and tests every build-affecting step. No comments/doxygen until the post-audit pass.

## Steps

### Step 1: SPEC rewrite
**Scope:** `Source/HELP.md`
**Action:** COUNSELOR rewrites HELP.md as the governing SPEC of the explicit-template design: `template.cast` grammar (info-string ids, `template:<id>` refs, `:::line:::` vertical law, `quoted` tag, separator block), four-column manifest grammar with cookbook rows for every construct (namespace, chars, bimap dual-jack, hashMap, lookupTable, identifier, includes/`## output index`), engine stamps (banner+pragma, comments), wrap/tab law, FATAL rules (dead placeholder, dead wiring), `--format` fail-fast contract. The exact structure-cell grammar for each construct is settled with ARCHITECT during this step — before any engine or data work.
**Validation:** every rev-5 locked decision appears; no rev-4 dead vocabulary survives outside a "superseded" note.

### Step 2: jam formatter fixes
**Scope:** `jam_markdown/document/jam_MarkdownDocument.h`, `jam_MarkdownWriter.h`, `jam_MarkdownValidator.h`
**Action:** @Engineer — (a) writer grid branch: guarantee a closing dash border after the last row (decision 11); (b) fail-fast malformed rows: parse records the authored cell count on the row when it disagrees with the header's column count (property name ARCHITECT-ratified — see Risks); `jam::MarkdownValidator` FATALs on it with path:line; `Processor::format` already aborts on validator failure (Processor.h:29-61) — the pad/truncate at :2415-2426 then never feeds a rewrite. No behavior change for well-formed tables.
**Validation:** round-trip of a well-formed grid table byte-stable; unclosed table gains exactly one bottom border; CAST.md:440-shape input refuses with file:line.

### Step 3: Engine rewrite
**Scope:** `Source/TemplateDocument.h`, `Writer.h`, `Model.h`, `Validator.h` (Operators.h, Processor.h touched only where the old vocabulary leaks in)
**Action:** @Engineer — delete-first re-cut to the reduced contract: `template:<id>` resolution via codeblock `Id::info` lookup on the parsed `template.cast`; matched-replace sequence (cell fill + transform tags + `:::line:::` vertical join + wrap/tab) as the only mechanism; all old structure vocabulary resolution (keyword/prologue/epilogue/terminator/open/close/@scope machinery, fragment-alias template loading) deleted. Survivors: class inheritance contract, `runJobs`, Transforms registry, banner/pragma stamp, comment stamping, parse-time placeholder cache. Zero hardcoded placeholder names.
**Validation:** grep gate — no `Id::keyword/prologue/epilogue/terminator/open/close/scope` in engine sources; only the four reserved columns + grammar sigils remain engine vocabulary.

### Step 4: jam data migration
**Scope:** `jam/cast/` (flat)
**Action:** @Engineer per COUNSELOR row-specs — (a) `template.cast` gaps: add `include` block; apply `:::key:quoted:::` tags where string keys need quotes (ARCHITECT's authored blocks are truth; COUNSELOR proposes exact diffs for ratification); (b) rewrite `CAST.md` clean on the four-column shape per SPEC cookbook — all constructs, includes/index row, jam_Bimaps unification rows kept; (c) delete dead data: `text.md ## namespace` + `## break`, stale aliases, any obsolete sections; (d) repair damaged tables: `terminal.md` SGR/ColorMode borders (:366, :377), `CAST.md:440` malformed row; `--format` clean afterward (ARCHITECT runs).
**Validation:** every wiring resolves; `--format` fixpoint on all jam/cast files; zero old-vocabulary residue.

### Step 5: cast tables spec-adherence (rescoped — ARCHITECT 2026-08-22)
**Scope:** `cast/CAST.md`, `cast/cast/` → flat + `template.cast`
**Action:** @Engineer — migrate cast's own manifest and tables to the current SPEC grammar (four columns, `template:<id>`, universal `:::line:::`, flat layout) so the data adheres to spec. **Self-generation is deferred**: jam `diff/ == generated/` converges first; cast regenerating its own `Source/generated/` bootstrap is a later sprint.
**Validation:** cast tables parse and validate under the new grammar; zero old vocabulary; no generation gate.

### Step 6: Converge
ARCHITECT builds + runs `./cast jam/cast/CAST.md`; `cmp -s jam/diff/<f> jam/generated/<f>` — 12 files, iterate residuals to byte-identical.

### Step 7: Fixpoint
2nd `./cast` = zero changes; 2nd `--format` = zero changes.

### Step 8: Auditor sweep
Full-sprint audit vs MANIFESTO/NAMES/CODING + locked decisions. Residuals to ARCHITECT verbatim. Doxygen prose after.

## BLESSED Alignment

- **B/E (Encapsulation):** engine consumes framework parse trees only; templates are data, engine never owns shape knowledge
- **L:** engine shrinks — one resolution mechanism replaces three paths + structure vocabulary; YAGNI enforced (no fileHead block, no engine first-row law)
- **E (Explicit):** code is visible verbatim in template.cast; fail-fast formatter (no silent pad/truncate); FATAL dead placeholders
- **S (SSOT):** one template file, one banner source, symbol-only cells, comments.md as sole comment-syntax truth
- **D:** byte-identical + fixpoint is the verdict, both for generation and `--format`

## Risks / Open Questions

1. **Malformed-row property name** (Step 2b) — new name, ARCHITECT ratifies. Candidate: reuse existing lexicon (`Id::cells` if present) or ARCHITECT names it.
2. **Structure-cell grammar per construct** — pinned in Step 1 with ARCHITECT before Steps 3-4 begin; nothing delegated without it.
3. **Step 5 scope** — cast self-hosting migration included because the engine rewrite kills the old manifest shape; ARCHITECT may descope/defer it explicitly.
