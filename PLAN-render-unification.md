# PLAN: Unify Definition≡Scope render; document HELP.md SPEC; converge jam/ byte-identical

**RFC:** none — objective from ARCHITECT prompts
**Date:** 2026-08-21
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE / JAM; cast engine (`jam::Document`, `jam::Strings`, `jam::Format`, `jam::Function::Map`)

## Context
cast generates jam's headers. Correctness = **byte-identical** to `generated/` (oracle), proven by `diff/` vs `generated/` (`cmp`/`shasum`) + **fixpoint** (2nd run = zero changes). Today all 13 files differ: the engine **collapses** list bodies onto one physical line (joins children by empty string instead of newline), and carries two redundant list mechanisms (`:::list:begin:::`/`:::list:end:::` region markers **and** table source-expansion) plus an emit-then-repair blank-line **elision machine**.

ARCHITECT's design: **Definition and Scope are one fragment** — same body/list mechanism — differing only in **axis**. A **Definition is one horizontal declaration** (`type name { list }`, cells joined by `, `); a **Scope stacks declarations vertically** (newline-joined body; major breaks between sibling rows by `//===`). The oracle is uniformly vertical (one declaration per line); the "horizontal" output seen in `diff/` is the collapse bug. The region mechanism and elision machine are obsolete. **Tables are already correct** — the manifest, templates, engine, and HELP.md must merely *express* the design. **HELP.md becomes the governing SPEC.**

## Overview
Unify the render on the Definition≡Scope axis model; delete the region + elision machinery; express map/lookup/bimap outputs via Scope+Definition using the structured `separator` (`- line:` vertical / `- list:` horizontal) + the `## break` text table; document the whole design as `Source/HELP.md`. Converge every jam header byte-identical, then fixpoint.

## The Pattern (HELP.md SPEC content)
- **One fragment, two axes.** body/list = *resolve children, join*.
  - **Definition — horizontal:** `:::keyword::: :::type::: :::name:toCamel::: { :::list::: };:::doxygen:toComment:::`. `:::list:::` = the row's packed cells joined by the **horizontal** separator (`- list:` → `@text:break:comma` = `, `), giving `{ key, value }`.
  - **Scope — vertical:** `:::keyword::: :::type::: :::name::: {` / `:::prologue:::` / `:::body:::` / `:::epilogue:::` / `}:::terminator:::`. Body items are **newline-joined**; major breaks between sibling output rows sharing a file via `- line:` (`@text:break:line` = `//===`); empty `separator` = plain newline.
- **`separator` column** (renamed from `lineBreak`), structured `- line:` (vertical) / `- list:` (horizontal). `## break` (text.md): `line`=`//===…`, `comma`=`, `.
- **`namespace jam` outermost** → inner types drop `jam::` (`HashMap<…>`, `Bimap<…>`, `LookupTable<…>`).
- **Maps decompose** to Scope (object) + Definition (entry) — no `Bimap`/`HashMap`/`LookupTable.cast`, no region markers.
- **Genuinely irregular fixed text** (Chars `isNumeric`/`special`; Bimap methods/singleton) stays as specialized fragment content only where it is *not* a list.
- **Data dictates logic:** the tables are the SSOT; the engine holds no per-shape syntax.

## Dependency & API Inventory
- Engine: `Source/TemplateDocument.h` — region handlers (`regionOpen`/`regionClose`, 420-446), elision machine (`build()`, 472-599), value seam (`getCell`, 601-675), source-expansion+join (`getText` overload, 718-754), marker tokeniser (`getMarker`/`isMarker`, begin/end path). `Source/Writer.h` (`toFile`/`buildFile`/`buildRow`/`applyWrap` — get-noun rename per ARCHITECT). `Source/Model.h` (unchanged value resolution).
- Framework used to fullest: `jam::Strings` (`joinIntoString`, `removeEmptyStrings`, `fromLines`), `jam::Format` (`replaceholder`, `getPreColon`/`getPostColon`), `jam::Function::Map` (dispatch), `jam::Document`.
- Templates: `Definition.cast`, `Scope.cast` (+ `Instance.cast`, `Include.cast`, `Chars.cast` retained); `Bimap`/`HashMap`/`LookupTable.cast` to delete.
- Manifest `cast/CAST.md`; `## break` table `cast/tables/text.md:22`; tables already correct (ARCHITECT mid-edit: `separator` col, `text.md`, `lexicon.md`).

## Validation Gate
Per step: ARCHITECT builds + runs `./cast cast/CAST.md`; COUNSELOR diffs `diff/<f>` vs `generated/<f>` (`cmp`/`shasum` — byte-identical is the gate) and validates against MANIFESTO (BLESSED), NAMES, CODING, and locked-plan decisions. Final: all files identical **and** fixpoint. @Auditor runs **once**, after the final step.

## Steps

### Step 1: HELP.md — rewrite as the design SPEC
**Scope:** `Source/HELP.md`.
**Action:** Document the unified design: Definition≡Scope one-fragment/two-axis model; Definition body slot `:::list:::` (horizontal, `- list:` join) with terminator; Scope vertical body (newline join, `- line:` sibling break); the structured `separator` column + `## break` table; `namespace jam` outermost; map/lookup/bimap decomposition to Scope+Definition; region mechanism removed; correct sigil to `@`. Supersede stale sections (line 97 literal `;`, line 100-108 `:::code:::`, line 125 "not decomposable", `#`-sigil law, `code` column).
**Validation:** SPEC internally consistent with actual templates/tables/engine and the oracle outputs; NAMES-compliant vocabulary; no `#`/`@` contradiction.

### Step 2: Templates — Definition body + confirm Scope
**Scope:** `cast/template/Definition.cast`, `Scope.cast`.
**Action:** Apply ARCHITECT's final `Definition.cast` (body slot `:::list:::`, terminator handling per the `separator`/`## break` resolution). Confirm `Scope.cast` vertical body join.
**Validation:** template bytes match ARCHITECT's ratified form; no new names beyond ratified jacks.

### Step 3: Engine — express axes; delete region + elision
**Scope:** `Source/TemplateDocument.h`, `Source/Writer.h`.
**Action:** Body/list join = newline (vertical, Scope) or `- list:` separator (horizontal, Definition), killing the collapse; bind the structured `separator` (`- line:`/`- list:`); preserve empty-slot elision without the emit-then-repair machine; unify Definition/Scope body-expansion; delete the region path (handlers + begin/end marker recognition). Writer `build*` → get-nouns.
**Validation:** MANIFESTO Lean/SSOT (machinery reduced, one join rule); no hand-rolled parsing where `jam::Strings`/`jam::Format` suffice; CODING (asserts, no bail-outs, alt tokens).

### Step 4: Converge vertical Definition/Instance files
**Scope:** manifest rows + verify.
**Action:** Converge `jam_Identifiers.h`, `jam_Text.h`, `jam_Chars.h`, `jam_Generated.h`, `jam_Operators.h` byte-identical (these already use `@definition`/`@instance`; the collapse fix + terminator/body enrichment carry them).
**Validation:** `cmp` byte-identical each; fixpoint-safe.

### Step 5: Conform map/bimap/lookup outputs
**Scope:** `CAST.md` output rows; delete `Bimap`/`HashMap`/`LookupTable.cast`.
**Action:** Express these outputs as Scope (object, `inline const HashMap<…> name { … }`) + Definition entry (`{ :::list::: }`, `- list:` comma, terminator). Handle Bimap's two lists (map-init + enum) and fixed methods/singleton. Converge `jam_HashMaps`, `jam_LookupTables`, `jam_Bimaps`, `jam_Enums`, `jam_MermaidTables`, `jam_Entities`.
**Validation:** `cmp` byte-identical each; no specialized template retained except genuinely-irregular fixed text.

### Step 6: Delete region vocabulary; resolve orphan; fixpoint
**Scope:** `cast/tables/lexicon.md` (`begin` 117, `region`/`region open`/`region close` 899-901), `cast/tables/markdown.md` (regionOpen/Close grammar 8-13), engine remnants; `jam_MermaidGeometry.h`.
**Action:** Delete region vocabulary now dead. Delete `jam_MermaidGeometry` output (absent from oracle). Confirm fixpoint (2nd `./cast` = zero changes).
**Validation:** zero region residue (grep); fixpoint clean.

### Step 7: Auditor sweep
Full-sprint audit vs MANIFESTO/NAMES/CODING + locked plan; COUNSELOR resolves all findings before sprint log.

## BLESSED Alignment
- **SSOT:** one body mechanism, one `## break` table, tables the sole data source; no shadow list mechanism.
- **Lean:** two universal fragments replace region markers + three specialized templates + the elision machine.
- **Explicit / Deterministic:** byte-diff + fixpoint gates; empty-slot elision explicit; data dictates logic.
- **Encapsulation:** documents build/write themselves; no external node-walker.

## Risks / Open Questions
- **Definition entry terminator vs comma.** Latest `Definition.cast` shows literal `;`; map entries end `,`. Resolve whether the `,` is the entry's `- list:`/`- line:` separator (join) or a terminator slot — ARCHITECT defining via the `separator`/`## break` design.
- **Structured `separator` not yet applied.** CAST.md cells are flat (`@text:break:line`/empty); the `- line:`/`- list:` form is target — needs conforming (ARCHITECT mid-edit).
- **Bimap is the hard case:** map-init + enum + `getDefault`/`get` methods + singleton — two lists plus fixed text in one struct; may retain a specialized fragment for the non-list text.
- **Sigil:** HELP.md uses `#`; tables use `@` exclusively. Plan assumes `@` (tables correct); HELP.md rewrite adopts `@` unless ARCHITECT directs a `#` migration.
- **Body slot name:** Scope `:::body:::` vs Definition `:::list:::` — unify or keep distinct.
- **In-flight tables:** ARCHITECT is editing `text.md`/`separator`/`lexicon.md`; Steps 2-3 execute against the settled tables.

## Verification
End-to-end: ARCHITECT runs `./cast cast/CAST.md` after each step; COUNSELOR runs `cmp -s generated/<f> diff/<f>` (and `shasum`) across all headers — byte-identical is the gate. Final acceptance = every header identical **and** a second `./cast` run produces zero changes (fixpoint, MANIFESTO D). @Auditor sweeps once at the end.
