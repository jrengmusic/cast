# PLAN: Arity-Driven Order — `>` Is Indent Only

**RFC:** none — objective from ARCHITECT prompt (laws ratified in-session)
**Date:** 2026-08-26
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + JAM (jam::MarkdownDocument AST, jam::Strings, jam::HashMap, jam::Array, jam::Format)

## Overview

`>` currently means two things at once: shape nesting and rendered indent. This plan removes the first meaning. Nesting comes from arity — a shape consumes as many following structure lines as it has `:::list:::` occurrences — and `>` is left as absolute output indent. The change is subtractive: no new line form, no new cell layout, no new identifier.

## Context

Wiring `jam_Mermaid.h` this session hit the conflation three times: nine bimap heads in one row collided on `duplicate binding: name` (jam/cast/CAST.md:987 — binding scope is per depth); struct-inside-struct could not express its members' indent, because an entry line can never sit shallower than its own head; every nesting level added an unremovable +4 shift (Shapes.h:237). The Mermaid container structs — the remaining majority of the oracle — are unreachable while `>` carries scope.

## Ratified Laws

1. **Arity-driven order.** A shape's `:::list:::` occurrence count is its arity. Reading the structure cell top to bottom, a shape consumes the next *arity* source lines; each consumed source consumes its own arity first. Nesting is implied by the template, never authored.
2. **Line forms unchanged.** `template:<id>` — the shape renders once. `- list: template:<id>` — the shape renders once per item of its paired list-column line. A list-column line with no structure partner renders its items verbatim (today's `cells` rule, §6.5).
3. **Bindings belong to the nearest preceding shape line.** Scope is per shape, not per depth — this dissolves the duplicate-binding collision.
4. **`>` = absolute output indent.** N `>` on a structure line means column N × 4 from column 0 of the output file. Nothing else. No `>` = column 0.
5. **Columns pair by order.** The Nth `- list:` line of the list column is the source of the Nth `- list:` line of the structure column; separator likewise. `>` in the list and separator columns is a readability mirror, never read.
6. **Row join.** The separator column's leading `>`-less line is the file group's merge join (§6.7), exempt from pairing.
7. **Merge unchanged (§6.7).** Same-file rows merge; repeated wrappers merge; diverging values join by the row-join text.

Worked traces, oracle-verified:

```
list                        separator                      structure
                            - list: template:linebreak     template:namespace      arity 1
                                                           - name: map
                                                           template:bimap          arity 2
                                                           - name: Screen
                                                           - base: @bimap
- list: @bimaps:Screen                                     > > > - list: template:nameEntry    col 12
- list: @bimaps:Screen                                     > > - list: template:enumEntry      col 8
```
→ `struct Screen` col 0 (jam_Bimaps.h:23), map entries col 12 (:28), enum col 8 (:35) ✓

```
list                          separator                    structure
                              - list: template:linebreak   template:struct         arity 1
                                                           - name: Mermaid
                                                           > template:staticLookupTable   col 4, arity 1
                                                           > - type: @byteClass
                                                           > - name: characters
- list: @mermaid:characters                                > > > - list: template:entry    col 12, arity 1
- list: cells                 - list: template:comma       (no partner — verbatim)
```
→ `static constexpr … characters` col 4, entries col 12 (jam_Mermaid.h:1262-1264) ✓

Containers become plain data: `template:struct` (Mermaid) → `> template:struct` (Xy) → `> > template:bimap` (Series), entries at their own `>`.

## Language / Framework Constraints

All markdown read through `jam::MarkdownDocument::parse()` — no manual line parsing (project principle 6). Documents immutable after construction. Values parsed with `jam::Format::getPreColon`/`getPostColon` (jam_Format.h:413/418); placeholders with `hasPlaceholder`/`replaceholder` (:457/:444); joins with `jam::Strings::joinIntoString` (jam_Strings.h:1006).

## Dependency & API Inventory (Pathfinder, cited)

- **Existing vocabulary reused, nothing added:** `Shapes::Sources = jam::Array<Element*>` (Shapes.h:23) already holds the structure lines; `addSources` (:76-104) already collects them in authored order; `addSourceReplacement` (:214-239) already branches the two forms on `source.id == Id::list` vs a head-carrying blockquote.
- **To delete:** `getScopeAtDepth` (:28), `getListLine` (:38), `addListSources` (:57), the `sourceDepths` / `sourceOrdinals` / per-depth `ordinals` arrays, and the blockquote recursion inside `addSources`.
- **Indent:** `getListValue` (:241-259) is the only indent site; `indentWidth {4}` (:26); nested shift `listDepths.add (depth - 1)` (:237).
- **Arity source:** the block's placeholder set is already stamped — `getCodeBlock (shapeId)->get<jam::Document::Identifiers> (Id::placeholder)` (:175); per-line occurrence counting exists in `getSubstitutedLine` (:292-310) and `isMarkerCountValid` (Validator.h:294-299).
- **Model:** `getStructure` (Model.h:117), `addBindings` (:303 — recursive, predecessor per depth), inherited `getBlockquote` / `getList` / `getTableCell`.
- **Validator depth-walkers:** `forEachBinding` (:73), `isStructure` (:178), `isBindingCountValid` (:218, :252), `isMarkerCountValid` (:283, :321), `isColumnPartnered` (:431) / `isPaired` (:454), `isSourceCountValid`; order in `isManifest` (:721-759).
- **Writer:** merge path `Shapes::getShape (…, groupRows, joinText)` (Writer.h:185) — untouched.
- **SPEC:** §6 (255-378), §7 (381-427), §10.1 (509-532).
- **Re-authoring surface:** jam/cast/CAST.md 205 `template:` references; cast/cast/CAST.md 44.

## Validation Gate

Each step validated by COUNSELOR before the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md and the seven laws. @Auditor runs ONCE after the final step. No comments/doxygen in engine code pre-audit. ARCHITECT builds and runs everything; agents never build, never run git.

**Oracle ownership:** `jam/generated/jam_Mermaid.h` is maintained by a parallel session. Re-read it before every convergence check and reconcile `jam/cast/mermaid.md` to it. The oracle is never edited here.

## Steps

### Step 1: SPEC.md + HELP.md
**Scope:** `SPEC.md`, `Source/HELP.md`
**Action:** §6.2 predecessor is the preceding binding of the same shape (done). §6.3: a shape consumes the next *arity* sources; the two line forms; nesting implied. §6.4: `>` is absolute indent on structure lines — delete the nested-head clause and the depth-as-nesting clause. §6.5: bindings scope to the nearest preceding shape; columns pair by order; the verbatim rule stands unchanged. §6.6: leading `>`-less separator line is the row join; the rest pair by order. §10.1: duplicate binding is *within one shape's bindings*; the count fatal is *arity against the sources supplied*; the partner fatal is *a structure `- list:` line without its list-column line, row join excepted*.
**Validation:** every law present; no unratified rule; fatal set consistent with Step 3.

### Step 2: Engine — ordered pass and rendering
**Scope:** `Source/Shapes.h`, `Source/Model.h`
**Action (delete first):** delete `getScopeAtDepth`, `getListLine`, `addListSources`, the depth/ordinal arrays and the blockquote recursion. `addSources` becomes one ordered pass over the structure cell filling `Sources`; bindings assigned to the nearest preceding shape line; consumption bounded by each shape's arity; each `- list:` source pairs with the list and separator columns by ordinal. Rendering: a column-0 `:::list:::` fill indents to `indent × indentWidth − parentIndent`, blank lines never indented; `parentIndent` threads down. `Model::addBindings` predecessor becomes per shape, no recursion.
**Validation:** no bail-out guards, positive nesting, `not/and/or`, `.at()`, brace init, structured bindings; MANIFESTO L on every unit; zero new identifiers.

### Step 3: Engine — gates
**Scope:** `Source/Validator.h`
**Action:** Move `forEachBinding`, `isStructure`, `isBindingCountValid`, `isMarkerCountValid`, `isColumnPartnered`/`isPaired`, `isSourceCountValid` onto the Step-2 pass: binding uniqueness per shape; every named shape exists; marker count per shape; every structure `- list:` line has its list-column line; arity matches the sources supplied. `isManifest` call order unchanged.
**Validation:** fatal set matches SPEC §10.1 exactly — no extra checks; each gate enumerates once.

### Step 4: cast's own manifest
**Scope:** `cast/CAST.md`, `cast/template.cast`
**Action:** Re-author every structure cell: scope `>` deleted from heads and bindings; `- list:` lines keep their absolute `>`; list/separator columns ordered by expansion.
**Gate (ARCHITECT):** build cast, `./cast cast/CAST.md` — Identifiers/Text/Files/HashMaps/Bimaps/LookupTables byte-identical to current outputs.

### Step 5: jam manifest
**Scope:** `jam/cast/CAST.md`
**Action:** Same re-authoring. Namespace blocks: wrapper and shape at no-`>`; entry lines keep today's counts (`> > >` = 12, `> >` = 8). Mermaid blocks: `template:struct` at 0, shapes at `>`, bimap entries `> > > >` / `> > >`, LUT entries `> > >`.
**Gate (ARCHITECT):** `./cast jam/cast/CAST.md` — six converged headers byte-identical; jam_Mermaid.h front section unchanged from its converged state.

### Step 6: Mermaid containers
**Scope:** `jam/cast/CAST.md` (+ `jam/cast/template.cast` only if an entry frame is missing)
**Action:** Wire `Shape`, `Marker`, `Arrowhead`, `C4`, `Class`, `Er`, `Sequence`, `State`, `Git`, `Gantt`, `Kanban`, `Mindmap`, `Architecture`, `Xy`, `Requirement` as `template:struct` under `Mermaid`, members one level deeper. Union-packed and string-payload members follow as their own increments; any new entry block or alias goes to ARCHITECT first.
**Gate (ARCHITECT):** per-container diff against the re-read oracle; residual limited to the known re-baseline class.

### Step 7: Fixpoint + Auditor
**Action:** second `./cast` run and second `--format` produce zero changes on both manifests. @Auditor once, whole sprint; COUNSELOR resolves every finding. Doxygen prose is a separate post-audit pass.

## BLESSED Alignment

- **B:** each shape owns its bindings and sources, built once per render.
- **L:** engine shrinks — three helpers, two parallel arrays and six recursive gate twins collapse into one ordered pass.
- **E:** one symbol, one meaning. Arity and partner mismatches stay fatal and loud.
- **S (SSOT):** nesting declared once, in the template, as occurrence count; indent declared once, on the line it applies to.
- **S (Stateless):** Shapes/Items/Validator remain pure static functions of their arguments.
- **E (Encapsulation):** Validator is the only gate; Writer emits only.
- **D:** same tables + templates + manifest → same bytes; fixpoint proves it.

## Risks / Open Questions

- **Re-authoring blast radius:** 205 + 44 `template:` references; Steps 4-5 run under Destructive-Edit Discipline (dry-run, backup, verify).
- **Oracle churn:** the parallel session may change jam_Mermaid.h mid-sprint; reconciling `mermaid.md` is ours and repeats before each gate.
- **Re-baseline (carried from Phase A):** jam_Mermaid.h's front section differs from generated output only in hand-formatting — 12 one-line getters, 7 enum-padding hunks, 8 map-entry hunks. ARCHITECT's manual step, unchanged by this plan.
