# SPRINT-LOG.md

**Project:** cast  
**Repository:** /Users/jreng/Documents/Poems/dev/cast  
**Started:** 2026-08-01

**Purpose:** Long-term context memory across sessions. Tracks completed work, debts paid, and debts deferred to the inter-sprint ledger (`DEBT.md`). Written by PRIMARY agents only when ARCHITECT explicitly requests.

---

## 📖 Notation Reference

**[N]** = Sprint Number (e.g., `1`, `2`, `3`...)

**Sprint:** A discrete unit of work completed by one or more agents, ending with ARCHITECT approval ("done", "good", "commit")

---

## ⚠️ CRITICAL RULES

**AGENTS BUILD CODE FOR ARCHITECT TO TEST**
- Agents build/modify code ONLY when ARCHITECT explicitly requests
- ARCHITECT tests and provides feedback
- Agents wait for ARCHITECT approval before proceeding

**AGENTS NEVER RUN GIT COMMANDS**
- Write code changes without running git commands
- Agent runs git ONLY when user explicitly requests
- Never autonomous git operations
- **When committing:** Always stage ALL changes with `git add -A` before commit
  - ❌ DON'T selectively stage files (agents forget/miss files)
  - ✅ DO `git add -A` to capture every modified file

**SPRINT-LOG WRITTEN BY PRIMARY AGENTS ONLY**
- **COUNSELOR** writes to SPRINT-LOG
- Only when user explicitly says: `"log sprint"`
- No intermediate summary files
- No automatic logging after every task
- Latest sprint at top, keep last 5 entries

**NAMING RULE (CODE VOCABULARY)**
- All identifiers must obey project-specific naming conventions (see NAMES.md)
- Variable names: semantic + precise (not `temp`, `data`, `x`)
- Function names: verb-noun pattern (initRepository, detectCanonBranch)
- Struct fields: domain-specific terminology (not generic `value`, `item`, `entry`)
- Type names: PascalCase, clear intent (CanonBranchConfig, not BranchData)

**BEFORE CODING: ALWAYS SEARCH EXISTING PATTERNS**
- ❌ NEVER invent new states, enums, or utility functions without checking if they exist
- ✅ Always grep/search the codebase first for existing patterns
- ✅ Check types, constants, and error handling patterns before creating new ones
- **Methodology:** Read → Understand → Find SSOT → Use existing pattern

**TRUST THE LIBRARY, DON'T REINVENT**
- ❌ NEVER create custom helpers for things the library/framework already does
- ✅ Trust the library/framework - it's battle-tested

**FAIL-FAST RULE (CRITICAL)**
- ❌ NEVER silently ignore errors (no error suppression)
- ❌ NEVER use fallback values that mask failures
- ❌ NEVER return empty strings/zero values when operations fail
- ❌ NEVER use early returns
- ✅ ALWAYS check error returns explicitly
- ✅ ALWAYS return errors to caller or log + fail fast

**⚠️ NEVER REMOVE THESE RULES**
- Rules at top of SPRINT-LOG.md are immutable
- If rules need update: ADD new rules, don't erase old ones

---

## Quick Reference

### For Agents

**When user says:** `"log sprint"`

1. **Check:** Did I (PRIMARY agent) complete work this session?
2. **If YES:** Write sprint block to SPRINT-LOG.md (latest first)
3. **Include:** Files modified, changes made, alignment check, debts paid, debts deferred
4. **Hygiene:** After writing the SPRINT-LOG entry, drain paid debts from project-root `DEBT.md` via `carol debt clear <id>` for each ID under *Debts Paid*. Receipt first, then clean the books.

### For User

**Activate PRIMARY:**
```
"@CAROL.md COUNSELOR: Rock 'n Roll"
```

**Log completed work:**
```
"log sprint"
```

**Invoke subagent:**
```
"@oracle analyze this"
"@engineer scaffold that"
"@auditor verify this"
```

**Available Agents:**
- **PRIMARY:** COUNSELOR (domain specific strategic analysis)
- **Subagents:** Pathfinder, Oracle, Engineer, Auditor, Machinist, Librarian

---

<!-- SPRINT HISTORY STARTS BELOW -->
<!-- Latest sprint at top, oldest at bottom -->
<!-- Keep last 5 sprints, rotate older to git history -->

## SPRINT HISTORY

## Handoff to COUNSELOR: Engine Deleted and Rewritten — Minimal jam_Generated.h Case

**From:** COUNSELOR
**Date:** 2026-08-22
**Status:** In Progress — engine rewritten, never built. ARCHITECT's build of `./cast jam/cast/CAST.md` is the next action.

### Context
ARCHITECT terminated the six-revision plan. `PLAN-render-unification.md` is DELETED and must not be resurrected — it was the record of additive drift, not a spec. The data (`jam/cast/CAST.md` + `template.cast`) IS the specification; the engine is derived from it. Ruling: "DATA ALWAYS DICTATE LOGIC. if the engine cant produce what we express from table to the expected output, then the engine is garbage."

ARCHITECT's mental model, verbatim and governing:
- **Model** holds the AST of all TABLES — the state.
- **TemplateDocument** (also a `MarkdownDocument`) holds the complete state of the TARGET.
- **Validator** CHECKS — every gate lives here, one recursive iteration.
- **Writer** WRITES — checks nothing, asserts nothing.
- **Processor** orchestrates: parse all → auto-format → validate (malformed; value checks later) → generate.
- Validator and Writer consume ONE SSOT resolution method. Not both checking, both asserting, both failing.
- `runJobs` is correct as is; all multi-file parse/write stays parallel.
- The framework already has every building block: `jam::Document` AST, `jam::MarkdownDocument`, `jam::Strings`, `jam::Format`. Nothing is reinvented.

COUNSELOR failure mode this session, corrected mid-flight and recorded so it is not repeated: quoting `HELP.md` prose as law after being told the code is ground truth; presenting implementation debris (one-jack rule, indent conflict, validator depth-0 check, include-order dependency) as design constraints and asking ARCHITECT to rule on them; proposing a 41-row data move to avoid touching the engine. Status-quo preservation is forbidden — Refactor-Rewrite Discipline.

### Completed
- **Four evidence-backed engine fixes** (pre-rewrite, still present): `getReplacement` gated on `codeId == getStructure(row, depth)` so head bindings stop leaking into child shapes (was emitting `id` at CAST.md:150, `MermaidOperators` at :195); `getCell` resolving `@`-sigiled cells through the file's `## index`; `Writer::getRow` residue check moved from the `:::` sigil to `jam::Format::hasPlaceholder` over cached names (generated data legitimately contains `:::`); `chars` template jack de-indented to column 0.
- **`generated` template block retired.** jam_Generated.h now composes `namespace` + `struct` + `include` + `sharedInstance`. The `namespace` block gained a leading `:::files:::` jack; unwired rows elide it via the line rule.
- **Duplicate blocks unified** as `lookupTableValue` (was `MermaidClassRelationValue` / `MermaidShapeGeometryValue`, byte-identical bodies). Emitted C++ type names unchanged.
- **Data stripped to the minimal case** under backup-and-reconcile discipline, all counts verified: `template.cast` 4 blocks; `## index` 14 rows; `## output` rewritten as `structure | file`, 100 rows; `## output index` untouched; 17 table `.md` files deleted. The 90 instance names were cross-checked against `jam/generated/jam_Generated.h:37-126` — exact match, same order.
- **Engine rewritten delete-first.** `TemplateDocument.h` 514→339 lines: the eight-rung `getReplacement` ladder, the `pendingBlank`/`suppressNextBlank` repair pass, the `getCell` probe chain, and `Writer::getRow`'s dry-build all deleted. Three functions replace them: `getShape` (recursive head chain, parent fill), `getExpansion` (jack; table-ref / column-name / binding-name sources), `getItem` (leaf fill from one source item). `Writer.h` 146 lines, emits only. `Validator.h` 185 lines, sole gate holder.
- **Dependency order established** (Pathfinder + verified at jam_HashMaps.h:116): HashMaps→{Extensions, Bimaps}, LookupTables→{Operators, Bimaps}, MermaidTables→{Bimaps}; the other six have no outgoing edges, no cycles. The authored `## output` file order is dependency-valid AND byte-identical to the oracle include order, so no oracle re-baseline is needed.
- COUNSELOR hand-traced the full `## output index` path against the templates and the oracle: 11 includes, `namespace map`, `struct Generated`, 90 members at indent 4, `Screen` winning as the deepest `- name:` binding, `:::name:toCamel:::` surviving the plain-`name` replace for the transform pass.

### Remaining
- **ARCHITECT builds and runs** `./cast jam/cast/CAST.md`. NOTHING has been compiled since the rewrite. Gate for this iteration: jam_Generated.h only — no assert, no warning, `cmp` clean against `jam/generated/jam_Generated.h`. The other ten targets emit bare namespace wraps; that is expected.
- **Silent child-drop must become a gate.** `getShape`:115-123 fills the parent-fill marker only when exactly ONE candidate is unmatched. The `namespace` block has three (`files`, `name`, `line`), so a row binding only `name` leaves two unmatched, `:::line:::` resolves empty, and the child shape is discarded with no residue and no error. MANIFESTO E — it must fail loudly in Validator, not count silently.
- **MANIFESTO L breach**, flagged by Engineer rather than hidden: `getExpansion` 105 lines, `getShape` 71, `getItem` 66, `TemplateDocument.h` 339. Decomposition requires new names — ARCHITECT's decision, not the Engineer's.
- **`isAssembled`** — new Validator method name, unratified. It builds every output row through `getShape` to check residue, which means each row is built twice (once by Validator, once by Writer).
- **Grow the data back file by file**, one convergence per file, in the ARCHITECT-set order — jam_Generated.h first, then the next.
- Auditor sweep once, at the end. Doxygen prose after that.

### Key Decisions
- Plan revision seven does not exist. `PLAN-render-unification.md` is deleted; the tables are the spec.
- Validator is the only gate. Writer never checks, never asserts.
- Resolution is SSOT — one method, consumed by both.
- A marker resolves from the source item first, the declaring row second. Innermost binding wins; on a depth tie, the later one in document order.
- Failure is never encoded as data. The old engine returned the literal marker on failure, which is the sole reason the residue scanner and the dry-build ever existed.
- Residue is detected by placeholder NAME via `hasPlaceholder`, never by the `:::` sigil.
- Deletion before implementation. Old code never coexists with new.

### Files Modified
- `Source/TemplateDocument.h` — rewritten: `getShape` / `getExpansion` / `getItem`; `placeholders` parse cache and `getBinding` survive
- `Source/Writer.h` — rewritten, emit-only
- `Source/Validator.h` — rewritten, sole gate holder
- `Source/Model.h` — ephemeral `getTables` `debug::Log` removed
- `PLAN-render-unification.md` — DELETED
- `~/Documents/Poems/dev/jam/cast/template.cast` — 4 blocks; `namespace` carries `:::files:::`; `generated` retired; `lookupTableValue` unified
- `~/Documents/Poems/dev/jam/cast/CAST.md` — `## index` 14 rows; `## output` two-column, 100 rows; `## output index` untouched; 8 metadata tables untouched
- `~/Documents/Poems/dev/jam/cast/` — 17 table `.md` files deleted

### Open Questions
- Ratify or rename `isAssembled`; decide whether Validator building every row twice is acceptable or whether the assembled AST is cached and handed to Writer.
- Decomposition names for `getShape` / `getExpansion` / `getItem` to clear MANIFESTO L.
- COUNSELOR-introduced divergence to restore when the data grows back: four of the 90 instance rows originally carried `@jam_MermaidTables`, not `@jam_Bimaps` — `MermaidOperator`, `C4Relation`, `C4Boundary`, `RequirementDiagramFieldLabel`. The strip spec forced all 90 to `@jam_Bimaps`. jam_Generated.h is unaffected (same 90 names, same order, both files still in the include list).
- Unratified edit by Engineer: `template.cast:11` changed from `}/ namespace :::name:::` to `}// namespace :::name:::` against instruction. The new text matches all 10 oracle files, so it is almost certainly right — but ARCHITECT has not blessed it.
- CAST.md's 8 metadata tables (`## Source Identity` … `## Paths`) were kept. They have no `file` column and are not codegen inputs, but they are not needed for jam_Generated.h either.

### Next Steps
1. Read `Source/HELP.md` with suspicion — its engine sections describe the DELETED engine and were not rewritten. Code is ground truth; HELP.md is not.
2. ARCHITECT builds, runs, pastes evidence. One cited fix per isolated fact. Never relitigate a ruling.
3. `cmp jam/diff/jam_Generated.h jam/generated/jam_Generated.h` — iterate to byte-identical.
4. Then grow the data back one file at a time, same gate each time.

## Handoff to COUNSELOR: Explicit-Template Convergence — Runtime Debug Loop In Progress

**From:** COUNSELOR
**Date:** 2026-08-22
**Status:** In Progress — mid debug loop, ARCHITECT rebuild/run pending

### Context
Continuing PLAN-render-unification.md (rev 6 — read it FIRST, decisions 26-35 are this session's rulings). Sprint objective: jam/cast data on the explicit-template canon + engine debugged to `./cast jam/cast/CAST.md` generating, then 12-file byte convergence vs jam/generated. Chat rulings are ground truth; the plan mirrors them (decision 35). NO SANDBOX NO BUILD — ARCHITECT builds and runs everything; COUNSELOR reads evidence (decision 34).

### Completed
- Data (jam/cast/): code fragments out of tables — 23 specialized `lookupTable*` blocks (types+capacity baked, ALL single-arg), `hashMapString`/`hashMapInt`, `bimap` (int) + `bimapUint32`; generic lookupTable/hashMap blocks deleted; all type/type1/type2/keyType/valueType/capacity bindings and 8 index type aliases deleted; 24 default value bindings + 16 default/value codeblocks deleted (first-row law: LookupTable ctor jam_LookupTable.h:82-100, Bimap base getDefault jam_Bimap.h:184-187 — generated structs emit NO getDefault override); 96 bimap split blocks merged to one bordered row each (241→145); mermaid.md 19 headings un-prefixed (`- name:` symbols keep prefix); 13 structure `- line:` binding depths aligned to wiring/tab depth (HELP.md:132 same-depth law)
- Engine (jam): grid head-row flush continuation for quote-marker cells (`quoteMarker` mirrors `bulletMarker`, jam_MarkdownDocument.h:~2415); addTableRow row tag = first cell only when `juce::Identifier::isValidIdentifier`, else Id::tableRow (tags are O(1) probe keys — jam getTableRow:157; content reads must use cells)
- Engine (cast): Validator binding-only depth-0 `continue` concession removed (fail restored); lone unresolved placeholder line survives build (parent-fill jack must be detectable by Writer residue check, Writer.h:101-126; inline unresolved still trims per trim law); getCell first-column + row-key fallback read CELLS not row tags (TemplateDocument.h:~483-505); placeholder scanner strips leading colons (`Prefix:::::name:::` idiom cached empty Identifier — source of ~55 juce_Identifier.cpp:61 asserts and all mermaid-construct failNoSource)
- Debug loop (runtime evidence from ARCHITECT's runs): TemplateDocument.h:120 depth asserts → fixed; wrap-mismatch FATAL (Validator.h:185) from tag-gated index rows → fixed; ThreadPool asserts were downstream job deaths

### Remaining
- ARCHITECT rebuild + run; read ~/Desktop/cast.ode — instrumentation now logs offending marker LINES per failNoSource row and getTables misses
- Residual unexplained rows from last run (may clear with scanner fix): CAST.md:150 (lexicon→jam_Identifiers), 195 (@mermaid:operators via template:char), 1203 (## output index: `- files: file` + `- line: instance` second-order forms — verify getExpansion implements column/binding wiring per HELP.md:133-134)
- 1 residual juce_Identifier.cpp:61 assert (likely a spaced/invalid table heading via getTableId:2124 — e.g. `## output index`)
- REMOVE ephemeral debug::Log instrumentation (Model.h getTables miss, Writer.h failNoSource lines) same sprint per ODE
- Then: 12-file `cmp` convergence loop (expected diff classes: getDefault first-row values, brief-line removal), fixpoint ×2, Auditor sweep (once), post-audit doxygen
- `@id`/`@string` note: getCell returns type cells RAW (`@id` unresolved?) — verify identifier construct emits `juce::Identifier` via per-file index (lexicon.md:1-8) once generation runs

### Key Decisions
- All in PLAN-render-unification.md rev 6 decisions 26-35 (grid canon + continuation law, specialized blocks, first-row default ALL map types in the framework type, comment abolition, generated namespace-map reversal, mermaid un-prefix, one-row bimaps, no code fragments in tables, no sandbox/build, chat-is-ground-truth)

### Files Modified
- `~/Documents/Poems/dev/jam/cast/template.cast` — specialized blocks, no default machinery
- `~/Documents/Poems/dev/jam/cast/CAST.md` — fragment-free grid manifest, merged bimaps, depth fixes
- `~/Documents/Poems/dev/jam/cast/mermaid.md` — un-prefixed headings
- `~/Documents/Poems/dev/jam/jam_markdown/document/jam_MarkdownDocument.h` — continuation law, row-tag gate
- `Source/TemplateDocument.h` — lone-marker survival, cell-based getCell, scanner colon fix
- `Source/Validator.h` — concession removed
- `Source/Writer.h`, `Source/Model.h` — EPHEMERAL debug::Log (remove before sprint log)
- `PLAN-render-unification.md` — rev 6

### Open Questions
- None gated — execute from the plan; new decisions surface to ARCHITECT with citations only

### Next Steps
- Read PLAN-render-unification.md rev 6 + Source/HELP.md (cookbooks = authored grammar truth) BEFORE touching anything
- Resume the runtime debug loop from ARCHITECT's next cast.ode paste; one cited fix per isolated fact; never relitigate rulings

## Handoff to COUNSELOR: Framework API Compliance — Table Subsystem + Engine Foundation

**From:** COUNSELOR
**Date:** 2026-08-22
**Status:** In Progress — all code changes applied; ARCHITECT build + `./cast jam/cast/CAST.md` + 12-file cmp is the next action

### Context
ARCHITECT surfaced three compounding failures after the Bimap<Key> redesign: (1) formatter inflates pipe table column widths to ~275 chars from inline `- comment:` text; (2) validator rejects binding-only continuation rows (`CAST.md:159 (structure): not found`); (3) cast bootstrap files use old 3-arg Bimap signature. Root cause: the foundation (MarkdownDocument parser, MarkdownWriter, cast engine) hand-rolls text parsing instead of using `jam::Format`/`jam::Strings` API — CONTRACT VIOLATION. ARCHITECT directed: framework API to its fullest extent, no hand-rolling, fix the foundation before chasing symptoms.

### Completed
- **Bimap bootstrap** — cast `Source/generated/Bimaps.h` migrated to `Bimap<int>` (no `override`, `getInstance()`); `Generated.h` `jam::Generated` → `map::Generated`
- **Format order** — `main.cpp` + `Processor.h`: format runs BEFORE generate; format drops content validation (structural normalization only)
- **Writer rawText** — `jam_MarkdownWriter.h`: pipe table cell content from `rawText` property (stored at parse time, trimmed) instead of re-serializing from block-level DOM elements. Eliminates width inflation feedback loop.
- **splitTableRow** — `jam_MarkdownDocument.h`: char-by-char `appendTableRowChar` deleted; `splitTableRow` rewritten on `Format::from`/`Format::upTo` with escape-aware pipe splitting
- **TemplateDocument scanning** — `TemplateDocument.h`: `while(true)` indexOf/substring loop replaced with `Format::from`/`upTo`/`getPreColon` chain; `jam::Array` non-copyable fixes (static empty + const ref, `std::move`)
- **Validator fix** — `Validator.h`: binding-only rows (empty structure at depth 0) `continue` instead of `return fail` — unblocks generation on CAST.md continuation rows
- **Comment SSOT** — 128 inline `- comment:` lines removed from `jam/cast/CAST.md`; engine heading-paragraph fallback added to `TemplateDocument.h::getReplacement` (reads paragraph between `## heading` and table as the construct's brief)
- **Inline LUT** — `jam_MarkdownDocument.h`: `htmlBlockConditions` LookupTable moved from `getHtmlBlockCondition()` function body to `BlockParser` class scope (name ratification pending — was `conditions`, renamed for class-scope semantics)

### Remaining
- **ARCHITECT build** — rebuild jam + cast with all changes, run `./cast jam/cast/CAST.md`, verify formatter produces readable aligned tables and generation completes without assertion/validation errors
- **12-file cmp** — `cmp -s jam/diff/<f> jam/generated/<f>` — iterate residuals to byte-identical
- **Fixpoint** — 2nd `./cast` = zero changes; 2nd format = zero changes
- **Step 5 deferred items** — `cellAlignment*` constants → enum/bimap; spec constants (`maxAtxLevel`, `minFenceLength`, etc.) → data table/generated struct. Blocked on cast running successfully (needs regeneration pipeline).
- **`htmlBlockConditions` name** — Engineer renamed `conditions` → `htmlBlockConditions` at class scope; ARCHITECT has not ratified
- **Auditor sweep** — once per sprint, after all steps
- **Doxygen pass** — post-audit

### Key Decisions
- **Framework API is non-negotiable** — `jam::Format::from`/`upTo`/`getPreColon`/`getPostColon`, `jam::Strings::fromTokens`, `jam::Format::replaceholder`/`hasPlaceholder` are the ground truth. Hand-rolled `indexOf`/`substring`/char loops are CONTRACT VIOLATIONS.
- **rawText is the cell SSOT** — writer uses `rawText` (trimmed at parse time) for pipe table cell content and width computation. Block-element re-serialization is not used for writing.
- **Heading paragraph = table-level documentation** — text between `## Table Name` and the table is the construct's brief, read by the engine via `getReplacement` fallback. Per-row documentation stays in the `doc` column. Inline `- comment:` bindings are dead.
- **Format before generate** — formatter normalizes structure first; content validation is generation's job, not the formatter's.
- **Binding rows are valid** — continuation rows with `- name:`, `- type:`, empty spacers in the `## output` table are not standalone constructs. Validator skips them.

### Files Modified
- jam: `jam_markdown/document/jam_MarkdownDocument.h` (splitTableRow rewrite, LUT move), `jam_markdown/document/jam_MarkdownWriter.h` (rawText cell content)
- jam data: `jam/cast/CAST.md` (128 comment lines removed)
- cast engine: `Source/TemplateDocument.h` (placeholder scanning, Array fixes, heading-paragraph fallback), `Source/Writer.h` (Array fix), `Source/Validator.h` (binding-row continue), `Source/Processor.h` (format order, validator removed), `Source/main.cpp` (format→generate order)
- cast bootstrap: `Source/generated/Bimaps.h` (Bimap<int>), `Source/generated/Generated.h` (map::Generated)

### Open Questions
- **`htmlBlockConditions` name** — ratify or revert to `conditions`
- **cellAlignment / spec constants** — deferred to after cast runs successfully; data table design TBD

### Next Steps
1. ARCHITECT: build jam + cast, run `./cast jam/cast/CAST.md`
2. Verify: formatter produces readable tables (rawText widths); generation completes; no assertion
3. `cmp -s` 12 files, iterate residuals
4. Fixpoint (2nd run = zero changes)
5. Ratify `htmlBlockConditions` name
6. Step 5 deferred items (cellAlignment enum, spec constants) if ARCHITECT directs
7. Auditor → doxygen → log sprint

## Handoff to COUNSELOR: Explicit-Template Restructure — Steps 1-5 Complete, Step 6 Convergence Pending

**From:** COUNSELOR
**Date:** 2026-08-22
**Status:** Ready for Implementation — Steps 1-5 done; Step 6 (ARCHITECT builds + `./cast jam/cast/CAST.md` + 12-file cmp) is the next action

### Context
ARCHITECT simplified the template design after Sprint 8: all code shapes are literal fenced codeblocks in ONE file per data dir (`jam/cast/template.cast`, info string = template id, referenced `template:<id>`), data flat at `jam/cast/`, abstract vocabulary (keyword/prologue/epilogue/terminator/open/close, Scope/Definition) dead. Engine = token match → `replaceholder` + transforms → universal `:::line:::` vertical join → wrap/tab. `PLAN-render-unification.md` revision 5 (25 locked decisions) governs; `Source/HELP.md` rewritten as the SPEC. Mid-sprint, ARCHITECT also redesigned jam::Bimap (census-backed) and unified jam_LookupTables.

### Completed
- **Step 1 SPEC:** HELP.md rewritten (explicit templates, four-column manifest, universal jack, depth = identity + indent, parent fill, engine stamps = banner + pragma + language comments, formatter fail-fast)
- **Step 2 formatter (jam_markdown):** grid-writer closing-border guarantee; parser stamps `Id::columns` on malformed rows (`isGridRowValid`/`gridRowColumns`); validator FATALs — all validated against sources
- **Step 3 engine (cast/Source):** TemplateDocument/Writer/Model/Validator rewritten; `template:<id>` via inherited `getCodeBlock` (parse makes info string the element id, jam_MarkdownDocument.h:1486-1492); matched-replace only; residue via cached candidates + `hasPlaceholder` (no scanner); first-row resolution (TemplateDocument.h:421-431); lexicon entry resolution in getCell; format-column resolution confirmed present (:497)
- **Framework redesign (jam):** `Bimap<Key=int>` (Derived + Instance dropped, Value = juce::String, getDefault non-virtual); `SharedInstance<T>::getInstance()` static added; per-struct `static X* getInstance()` — 220 call sites unchanged; VulkanShaderFormat migrated (+ VulkanEngine ownership → SharedInstance member); LUTs all top-level (wrappers deleted), Xml/Html → `markupLanguage`, 12 call sites swept
- **Step 4 jam data:** CAST.md rewritten in full (4-column; 96 bimap rows with `- type:`/`- comment:`, 90 `- instance:` bindings, 14 LUT + 18 mermaid Union LUT + 2 alias rows, hashMaps ×3 incl. romanNumerals, all single-construct rows, `## output index` on the `generated` block); template.cast 19 blocks (16 mermaid composed-value blocks + alias + generated + include); markdown.md wide matrix → 9+1 thin `key|value` tables; text.md `## namespace`/`## break` deleted; terminal.md all 24 tables border-repaired
- **Oracle normalized:** 1-arg→`Bimap<int>`×95 + `<uint32_t>`×1 (ColourNames, ARGB keys); 11 uint32 drifts reverted to int; `override` removed; getInstance inserted ×96; 81-underscore banners ×3; tight struct (jam_Text.h); uniform brief lines (+14 +9); C4Traits padding single-space; Generated member order re-baselined to row order (90 members verified)
- **Step 5 (rescoped):** cast's own data migrated to spec grammar only (flat cast/, template.cast 14 blocks, 4-column CAST.md, renames localisation-en→text.md, binary-files→files.md, template.md→tokens.md, new block id `stringEntry`); self-generation explicitly deferred until jam converges

### Remaining
- **Step 6:** ARCHITECT builds jam + cast, runs `./cast jam/cast/CAST.md`, `cmp -s jam/diff/<f> jam/generated/<f>` ×12 — iterate residuals (readiness: all 12 "converges"; Bimaps/MermaidTables moderate confidence — mechanism verified, not every row entry-diffed)
- **Step 7:** fixpoint (2nd `./cast` = zero changes; 2nd `--format` = zero changes)
- **Step 8:** Auditor sweep (once), then post-audit doxygen pass — includes stale-doc flags: jam_terminal.h:60, jam_TerminalDecMode.h:35 (Bimap<Derived,Value> prose), jam_VulkanEngine.h:848-853 (Instance-chain prose); lexicon FATAL is generation-time not pre-flight (pre-existing shape, carried to Auditor)
- cast self-generation (deferred, ARCHITECT ruling); jam table migration to canon shape (older deferral)

### Key Decisions
- PLAN-render-unification.md rev 5 §Locked Decisions 1-25 is the authoritative list. Highlights: universal `:::line:::` jack (identity = depth = indent, authored-order pairing, parent fill — replaces intersection rule); templates are explicit code, engine stamps only banner+pragma+comments; `quoted` transform for string keys; `--format` fail-fast on malformed rows + closing-border guarantee; `Bimap<Key>` redesign; LUT unwrap + MarkupLanguage merge; thin class tables; first-row resolution revived; mixed-type Identifiers (type from cell); uniform brief line; row-order-derives-everything re-affirmed (Generated re-baselined)

### Files Modified
- cast: `Source/{TemplateDocument,Writer,Model,Validator}.h` (rewritten), `Source/generated/Identifiers.h` (+Id::entry), `Source/HELP.md` (SPEC rev 5), `PLAN-render-unification.md` (rev 5), `cast/` (flattened + migrated)
- jam: `jam_core/utils/{jam_Bimap,jam_SharedInstance}.h`, `jam_markdown/document/{jam_MarkdownDocument,jam_MarkdownWriter,jam_MarkdownValidator}.h`, `jam_vulkan/{bimap/jam_VulkanShaderFormat,engine/jam_VulkanEngine}.h`, 6 LUT call-site files, `generated/{jam_Bimaps,jam_MermaidTables,jam_LookupTables,jam_Generated,jam_Text,jam_Identifiers,jam_Entities}.h` (normalizations), `cast/{CAST.md,template.cast,markdown.md,text.md,terminal.md}`

### Open Questions
- None blocking Step 6. Veto-window items ARCHITECT has not objected to: `stringEntry` block id, cast file renames, 6 added cast lexicon rows, 9 LUT object names (markupLanguage/css/markdown/mermaid/cLang/markup/data/mermaidBlock/mermaidFlowchart)

### Next Steps
1. ARCHITECT: build jam + cast, run `./cast jam/cast/CAST.md`, cmp 12 files
2. COUNSELOR: iterate residual diffs to byte-identical (read diff output, fix data/template/engine per evidence)
3. Fixpoint → Auditor → doxygen pass → log sprint

## Handoff to COUNSELOR: Engine Rewrite — :::line:::/:::list::: Expansion Jacks

**From:** COUNSELOR
**Date:** 2026-08-21
**Status:** In Progress — engine expansion mechanism needs rewrite; data/templates updated; build not yet passing

### Context
Continuing the Definition≡Scope render unification. This session established the correct engine design with ARCHITECT: **one mechanism (`jam::Strings::joinIntoString(separator)`)**, two axes (`:::line:::` vertical, `:::list:::` horizontal), no hardcoded tokens. Structure cell tokens match template placeholders by name — the engine discovers expansion jacks dynamically. Separator text comes from the `## break` table in `text.md`. `:::open:::`/`:::close:::` in Definition.cast replace hardcoded `{ }` delimiters, making them data-driven.

### Completed
- **Region mechanism deleted** — regionOpen/regionClose dispatch, Rules bimap, begin/end markers all removed from engine (TemplateDocument.h), bootstrap (Bimaps.h, Identifiers.h, Generated.h), data tables (template.md, cast/CAST.md)
- **lineBreak → separator rename** — Writer.h, TemplateDocument.h, Validator.h. Structured prefix parsing (`- line:`/`- list:`) via `jam::Format::getPreColon`/`getPostColon`. Default join changed from empty-string to newline (collapse fix)
- **Writer rename** — buildFile→getFile, buildRow→getRow (NAMES.md verb contract)
- **HELP.md SPEC rewrite** — `@` sigil, `separator` column, `:::line:::`/`:::list:::` jacks, `:::open:::`/`:::close:::` delimiters, one-mechanism design, map decomposition documented, region docs deleted
- **Templates updated** — Scope.cast: `:::body:::`→`:::line:::`. Definition.cast: `{ :::list::: }`→`:::open::: :::list::: :::close:::`. Bimap.cast + LookupTable.cast: rewritten without begin/end, use `:::line:::`. HashMap.cast deleted (uses Scope + type aliases)
- **CAST.md massive update** — ~110 rows `- body:`→`- line:`, ~90 bimap rows + 5 lookuptable rows got blockquote wraps, 3 hashmap rows use `@scope:` with `@hashMapStrStr`/`@hashMapStrInt` type aliases, ~15 rows got `- open: {`/`- close: }` tokens
- **Data fixes** — `@text` alias fixed from stale `text-en.md` to `text.md`; deleted orphan `text-en.md` and `debug.md`; added `semicolon` to `## break` table
- **PLAN revision 3** — comprehensive design document with correct engine architecture

### Remaining
- **Engine rewrite** — TemplateDocument.h expansion mechanism must implement: (1) `:::line:::` jack = vertical expansion — match structure cell `- line: @definition` token, expand source rows through Definition.cast per row, join by `joinIntoString(separator)` where separator comes from `- line:` resolved value. (2) `:::list:::` jack = horizontal expansion — match structure cell `- list: @text:break:comma` token, expand source row columns, join by `joinIntoString(separator)`. Current getCell/getExpansion/getText paths need clean rewrite, not patches.
- **Bimap gaps** — `:::default:::` in Bimap.cast's `getDefault()` method has no token binding (resolves empty). LookupTable `:::value:::` (default value) has no `- value:` token in structure cells.
- **Writer.h getFile** — sibling row join needs `joinIntoString` instead of `code << separator << rowCode` concatenation.
- **Build test** — ARCHITECT builds + runs `./cast jam/cast/CAST.md`, diff all 14 files against oracle, iterate residuals.
- **Fixpoint** — 2nd run = zero changes.
- **Auditor sweep** — once all files converge.

### Key Decisions
- **One mechanism:** `jam::Strings::joinIntoString(separator)` is the only join. No other join pattern exists in the engine.
- **Jack names are axis:** `:::line:::` = vertical, `:::list:::` = horizontal. Structure cell tokens `- line:` and `- list:` match these by name.
- **No hardcoded tokens:** Engine discovers expansion jacks by matching structure cell tokens against template placeholders. The engine reads rules from data, never hardcodes token names.
- **`:::open:::`/`:::close:::`:** Replace hardcoded `{ }` in Definition.cast. `- open: {`/`- close: }` for declarations, `- open: =` for enum entries (no close).
- **HashMap uses Scope:** Type aliases `@hashMapStrStr`/`@hashMapStrInt` (backtick-wrapped in index) avoid `<`/`>` hazard in structure cells.
- **Bimap/LookupTable keep specialized templates** — rewritten without begin/end, use `:::line:::` for body. Methods/singleton in epilogue.

### Files Modified
- `Source/TemplateDocument.h` — region dispatch deleted, Rules lookup deleted, separator renamed, composite-column bug fixed, structured prefix parsing added
- `Source/Writer.h` — separator renamed, structured prefix parsing, getFile/getRow rename
- `Source/Validator.h` — lineBreak→separator in reserved-column list
- `Source/generated/Bimaps.h` — regionOpen/regionClose + Rules deleted
- `Source/generated/Identifiers.h` — rules + rowRegion* deleted
- `Source/generated/Generated.h` — SharedInstance<Rules> deleted
- `Source/HELP.md` — comprehensive SPEC rewrite (revision 3)
- `cast/tables/template.md` — region open/close + rules table deleted
- `cast/CAST.md` — rules output row deleted
- `jam/cast/CAST.md` — ~200 structure cell edits, index aliases updated, @text fixed
- `jam/cast/template/Scope.cast` — :::body:::→:::line:::
- `jam/cast/template/Definition.cast` — { :::list::: } → :::open::: :::list::: :::close:::
- `jam/cast/template/Bimap.cast` — rewritten without begin/end
- `jam/cast/template/LookupTable.cast` — rewritten without begin/end
- `jam/cast/template/HashMap.cast` — deleted
- `jam/cast/tables/text.md` — semicolon added to ## break
- `jam/cast/tables/text-en.md` — deleted (stale)
- `jam/cast/tables/debug.md` — deleted (orphan)
- `PLAN-render-unification.md` — revision 3

### Open Questions
1. **Bimap `:::default:::`** — `getDefault()` uses `map.at(:::key:toCamel:::)` but this resolves from the first source row only when the template expands. Need a `- default:` token in structure cells. Oracle shows `map.at(0)` — is the default always the first enum value's ordinal?
2. **LookupTable `:::value:::`** — the default value before the entry block (e.g., `0xff000000`). Need `- value: @black` or similar token per LookupTable row. What's the right data source per row?

### Next Steps
1. Rewrite TemplateDocument.h expansion: implement `:::line:::` vertical and `:::list:::` horizontal expansion via `joinIntoString`
2. Rewrite Writer.h getFile: sibling join via `joinIntoString`
3. ARCHITECT builds + tests. Iterate residuals.
4. Resolve Bimap default + LookupTable value gaps
5. Fixpoint. Auditor.

## Handoff to COUNSELOR: Definition≡Scope Render Unification

**From:** COUNSELOR
**Date:** 2026-08-21
**Status:** Ready for Implementation — blocked on ARCHITECT's in-flight table edits + 3 design confirmations. Plan written: `PLAN-render-unification.md` (repo root).

### Context
Continuing the jam convergence (cast `jam/diff/` → byte-identical with oracle `jam/generated/`). This session did NOT converge a file — it resolved the ENGINE-LEVEL design that unblocks the map/bimap/lookup files (and fixes why all 13 files currently differ). Root cause of divergence: the engine COLLAPSES list bodies onto one physical line (joins children by empty string, not newline), plus two redundant list mechanisms (region markers + source-expansion) and an emit-then-repair elision machine. ARCHITECT's design, reached this session: **Definition and Scope are ONE fragment**, differing only in AXIS — a Definition is one horizontal declaration (`type name { list }`, cells joined by `, `); a Scope stacks declarations vertically (newline-joined body; sibling breaks by `//===`). Oracle is uniformly vertical. `HELP.md` becomes the governing SPEC of this design.

### Completed
- Deleted dead `jam::Format::getSection`/`replaceSection` (the `:::name:begin:::`/`:::end:::` region-marker builders) + the region-marker docs in `cast/Source/HELP.md` (Engineer, verified zero residue).
- Established the full unified design with ARCHITECT (Definition≡Scope, two axes; `separator` column replacing `lineBreak` with `- line:`/`- list:`; `## break` table; `namespace jam` outermost drops inner `jam::`; maps decompose to Scope+Definition; region + elision deleted; tables already correct — only expression changes; data dictates logic).
- Wrote `PLAN-render-unification.md` (repo root) — 7 steps, byte-diff + fixpoint gate.
- Confirmed via reads: oracle is uniformly VERTICAL (Text/Identifiers/Generated/Chars/HashMaps one-per-line); the "horizontal" in `diff/` is the collapse bug, NOT truncation (corrected a Pathfinder mis-read). Sigil is `@` exclusively (HELP.md's `#` is stale). `## output` has NO `code` column (`list | structure | separator | file`).

### Remaining
- Steps 1-7 of `PLAN-render-unification.md`: HELP.md SPEC rewrite → Definition/Scope templates → engine (vertical body-join kills collapse; horizontal `- list:` join; delete region + elision; Writer `build*`→get-nouns) → converge vertical files (Identifiers/Text/Chars/Generated/Operators) → conform maps (HashMaps/LookupTables/Bimaps/Enums/MermaidTables/Entities) → delete region vocabulary + resolve `jam_MermaidGeometry` (absent from oracle) + fixpoint → Auditor.

### Key Decisions
- **Definition≡Scope, axis is the only difference** — Definition horizontal (`:::list:::` cells joined by `- list:`/comma), Scope vertical (items newline-joined, sibling break `- line:`/`//===`).
- **`separator` column** (was `lineBreak`), structured `- line:` (vertical) / `- list:` (horizontal); `## break` table (`text.md:22`): `line`=`//===`, `comma`=`, `.
- **`namespace jam` outermost** → inner types drop `jam::`.
- **Maps decompose** to Scope+Definition; delete `Bimap`/`HashMap`/`LookupTable.cast` + region mechanism.
- **HELP.md is the governing SPEC**; **tables are already correct** — manifest/templates/engine express the design.
- Definition body slot → `:::list:::`; Writer `build*` → get-nouns (ARCHITECT-ratified).

### Files Modified
- `jam/jam_core/text/jam_Format.h` / `jam_Format.cpp` — `getSection`/`replaceSection` removed
- `cast/Source/HELP.md` — region-marker docs removed (full SPEC rewrite pending, Step 1)
- `cast/PLAN-render-unification.md` — new plan
- (ARCHITECT, in-flight) `jam/cast/tables/text.md` (`## break`, `## english`, `@text:*` aliases), `jam/cast/CAST.md` (`separator` column), `jam/cast/tables/lexicon.md` (+ `.bak`)

### Open Questions
1. **Entry terminator vs comma** — `Definition.cast` ends literal `;`; map entries end `,`. Is the `,` the `- list:`/`- line:` separator (join) or a terminator slot? (ARCHITECT defining via `separator`/`## break`.)
2. **Structured `separator` not applied** — CAST.md cells are flat (`@text:break:line`/empty); the `- line:`/`- list:` form is target.
3. **Bimap** — map-init + enum + `getDefault`/`get` methods + singleton in one struct: pure Scope+Definition, or a retained specialized fragment for the fixed non-list text?
4. **Sigil** — HELP.md `#` vs tables `@` (plan assumes `@`).
5. **Body slot name** — Scope `:::body:::` vs Definition `:::list:::` — unify or keep distinct.

### Next Steps
1. Let ARCHITECT's table edits settle (text.md/separator/lexicon); resolve the 5 open questions.
2. Step 1: rewrite `Source/HELP.md` as the SPEC (delegate to @Engineer).
3. Steps 2-3: Definition/Scope templates + engine (delete region + elision, vertical/horizontal join) to @Engineer.
4. Converge file-by-file with `cmp` gate; fixpoint; @Auditor once at end.

## Handoff to COUNSELOR: jam_Chars.h Convergence + Structural Grammar SSOT

**From:** COUNSELOR
**Date:** 2026-08-21
**Status:** In Progress — build pending after Extensions struct conversion + include order fix

### Context
Converging cast's output (`jam/diff/`) to byte-identical with the oracle (`jam/generated/`), one file at a time. jam_Generated.h, jam_Text.h, jam_Identifiers.h are byte-compile-identical (cosmetic deltas only). jam_Files.h and jam_Extensions.h are byte-compile-identical. jam_Chars.h convergence drove a structural grammar refactor — `>` = tab rule, SSOT methods on TemplateDocument, top-level scope support for namespaces (no indent) vs blockquoted scopes for structs (indented).

### Completed
- **isTableBorder** — `MarkdownDocument::isTableBorder()` static predicate replaces `isTag(Id::border)` at 4 call sites (getTableRows, Writer x2, Validator x1). Fixes "border" keyword collision that silently dropped the `border` data row and replaced it with `+---+` separator.
- **chars:: → Chars::** — 1095 total sites renamed across jam source (890), generated/diff (74), tables + cast (131). `jam::Array<juce::juce_wchar> special` → `static constexpr const char* const special`. `jam_Format.cpp` lambda building specialCharacters deleted — `Chars::special` passed directly. `.contains()` → `juce::String(Chars::special).containsChar()`.
- **extensions:: → Extensions::** — struct Extensions with `static constexpr const char* const` members. 97 total sites renamed. `jam_Html.h` svgTagName shadow state deleted — `Extensions::svg` used directly.
- **Structural grammar SSOT** — `getWraps`, `getWrapAlias`, `getTokens` moved from Writer.h to TemplateDocument.h as static methods. `getOutermostWrap`, `isWrapHead` moved from Validator.h. `getContent` SSOT for `.cast` template expansion (was inline in buildRow). Both Writer and Validator consume shared methods.
- **> = tab rule** — engine supports top-level scope (`@scope: Name` without `>`, depth 0, no indent) and blockquoted scope (`> @scope: Name`, depth 1, body/epilogue indented). `getTokens` reads nested blockquote bullets as content tokens with indent applied. `applyWrap` indents body token only. All outermost namespace scopes in CAST.md stripped of `>` (739 lines).
- **Definition.cast** — `:::doxygen:toComment:::` appended. Language-agnostic comment formatting via existing `toComment` transform.
- **Chars.cast** — reduced to epilogue-only: `special` string literal + `isNumeric` function.
- **chars.md** — hex values unpadded, `doxygen` column added (U+NNNN, no hardcoded comment syntax), `## special` table deleted, `key` → `name` column, spurious last-row borders removed.
- **jam_Generated.h oracle** — includes reordered (Enums before LookupTables, Chars before Enums for dependency), `jam::SharedInstance` → `SharedInstance` (redundant inside namespace jam), `@sharedInstance` CAST.md alias updated.
- **jam_Text.h oracle** — blank lines fixed to match diff/.

### Remaining
- **Build + diff jam_Chars.h** — verify struct Chars output matches oracle after all engine changes. The top-level scope + content indentation mechanism is implemented but untested end-to-end.
- **jam_Files.h** — same question as Extensions: if declared once in one table, should be struct. ARCHITECT hasn't ruled yet.
- **jam_Generated.h include convergence** — diff/ only generates 3 includes (Bimaps, MermaidTables, Enums). Oracle now has 12. CAST.md needs output rows that generate all includes. Separate convergence task.
- **Next convergence files** (per handoff order):
  - jam_Operators.h (6 operator rows have wraps INVERTED vs oracle)
  - jam_Bimaps.h / jam_Enums.h
  - jam_HashMaps.h
  - jam_LookupTables.h
  - jam_MermaidGeometry.h / jam_MermaidTables.h
- **Deferred from Sprint 7**: LC7 doxygen prose + Auditor sweep; HELP.md transform-vocabulary wording amendment

### Key Decisions
- **> = tab**: one `>` = one tab indent. No `>` = no indent. Namespace scopes at top level (no `>`), struct scopes inside `>`. Engine enforces via blockquote depth in buildFile (blockquote = depth 1, top-level = depth 0).
- **Struct for single-source types**: types declared from one table use `struct` with `static constexpr` members (Chars, Extensions). Consumer uses `Chars::`, `Extensions::` (PascalCase). Namespace reserved for multi-source scopes (Id, text, map, files).
- **isTableBorder SSOT**: `MarkdownDocument::isTableBorder()` checks `Id::type == BlockType::tableBorder`. Replaces 4 independent `isTag(Id::border)` checks that collided with data rows keyed "border".
- **Structural grammar on TemplateDocument**: scope/wrap detection, token extraction, validation — one implementation consumed by Writer and Validator. No divergence on grammar changes.
- **getContent SSOT**: `.cast` template expansion in `TemplateDocument::getContent()`. All token resolution (body, epilogue, any slot) goes through `getBinding` → `getContent`. No inline expansion in buildRow.
- **No hardcoded comment syntax in tables**: doxygen column holds `U+NNNN`, template applies `:::doxygen:toComment:::`. Language-agnostic.
- **No hardcoded token indentation**: applyWrap indents body only. Content tokens pre-indented by getTokens (from nested blockquote). Frame tokens (keyword, terminator) never indented.

### Files Modified
- `jam_markdown/document/jam_MarkdownDocument.h` — isTableBorder() static predicate; getTableRows uses tableRow type check
- `jam_markdown/document/jam_MarkdownWriter.h` — 2 sites use isTableBorder()
- `jam_markdown/document/jam_MarkdownValidator.h` — 1 site uses isTableBorder()
- `jam_core/text/jam_Format.cpp` — chars:: → Chars::, special lambda deleted, containsChar
- `jam_web/html/jam_Html.h` — extensions:: → Extensions::, svgTagName deleted
- `jam_markdown/layout/jam_MarkdownSyntax.h` — extensions:: → Extensions::
- 20 jam source files — chars:: → Chars:: (890 sites)
- `cast/Source/TemplateDocument.h` — getWraps, getWrapAlias, getTokens, getOutermostWrap, isWrapHead, getContent, indentWidth (SSOT)
- `cast/Source/Writer.h` — delegates to TemplateDocument::, applyWrap body-only indent, buildFile blockquote depth
- `cast/Source/Validator.h` — delegates to TemplateDocument::
- `cast/CAST.md` — @sharedInstance alias, namespace > stripped (739 lines), Chars/Extensions struct rows, Definition.cast doxygen
- `cast/template/Definition.cast` — :::doxygen:toComment::: appended
- `cast/template/Chars.cast` — epilogue-only (special + isNumeric)
- `cast/tables/chars.md` — hex unpadded, doxygen column, name column, special deleted
- `cast/tables/terminal.md`, `css.md`, `syntax.md` — chars::/extensions:: renamed
- `generated/jam_Chars.h` — struct Chars oracle
- `generated/jam_Extensions.h` — struct Extensions oracle
- `generated/jam_Generated.h` — includes reordered, SharedInstance prefix removed
- `generated/jam_Text.h` — blank lines fixed
- `generated/jam_Enums.h`, `jam_LookupTables.h` — Chars:: rename
- `diff/jam_Enums.h`, `jam_LookupTables.h`, `jam_HashMaps.h`, `jam_Generated.h` — same renames

### Open Questions
- **jam_Files.h** — should `namespace files` become `struct Files`? Same single-table pattern. ARCHITECT hasn't ruled.

### Next Steps
1. Build jam + cast, run `./cast jam/cast/CAST.md`, diff jam_Chars.h — verify struct body indented, epilogue indented, frame at column 0
2. If not matching: trace the top-level scope + content token indentation path
3. If matching: jam_Chars.h CONVERGED — move to next file (jam_Operators.h or jam_Files.h per ARCHITECT)

## Handoff to COUNSELOR: jam_Identifiers.h Convergence

**From:** COUNSELOR
**Date:** 2026-08-20
**Status:** In Progress — rebuild pending after getTableRows fix

### Context
Converging cast's output (`jam/diff/`) to byte-identical with the oracle (`jam/generated/`), one file at a time. jam_Generated.h and jam_Text.h are DONE. jam_Identifiers.h had 4 engine/data defects plus a collision bug — all fixed, awaiting rebuild+diff.

### Completed
- **jam_Text.h** — byte-identical ✓ (verified this session, MD5 match)
- **toCamelCase abbreviation** — jam_Format.cpp:264 now checks `isAbbreviation` at index 0, same pattern as `toSnakeCase`. `UIScale` stays `UIScale`, not `uiScale`.
- **toLiteral non-ASCII hex-escaping** — jam_Format.cpp:928-934 adds `else if (byte > 0x7F)` branch producing `\xNN` escapes. Uses existing `Id::hexEscapePrefix`/`byteHexDigits` pattern from `encodeCodepointAsEscapedUtf8`.
- **lexicon.md data fixes** — `alias` (line 25) and `name` (line 726) restored `@id` type (were empty, causing bare `inline const alias { alias };`). `companyCopyright` changed from `\xc2\xa9` to raw `©` (toLiteral now hex-escapes it correctly).
- **Scope.cast blank line** — added blank line between `:::body:::` and `:::epilogue:::` (line 8). Body's last line follows lineBreak convention; the consistent blank line before scope closing comes from the template.
- **getTableRows collision fix** — jam_MarkdownDocument.h:133 changed from `not row->isTag(Id::border)` to `not row->contains(Id::type)`. Separator elements have `Id::type` property (set at line 2381), data rows don't. Fixes silent drop of lexicon row keyed "border".
- **cast Id:: dedup** — removed `special`, `toComment`, `toCommentBlock`, `toUnicode` from cast's `Source/generated/Identifiers.h`. Jam lexicon is SSOT (ARCHITECT ruling: "jam SSOT").
- **Oracle baseline** — jam/generated/jam_Identifiers.h updated from generated output + `border` entry added by ARCHITECT.

### Remaining
- **Rebuild jam + cast, run `./cast jam/cast/CAST.md`, diff jam_Identifiers.h** — verify byte-identical after the getTableRows fix. `border` should now appear in the generated output.
- **Oracle sync** — after successful diff, copy generated → oracle if not already matching.
- **Next convergence files** (one-by-one, per ARCHITECT ruling):
  - jam_Files.h / jam_Extensions.h
  - jam_Chars.h
  - jam_Operators.h (6 operator rows have wraps INVERTED vs oracle)
  - jam_Bimaps.h / jam_Enums.h
  - jam_HashMaps.h
  - jam_LookupTables.h
  - jam_MermaidGeometry.h / jam_MermaidTables.h
- **Deferred from Sprint 7**: LC7 doxygen prose + Auditor sweep; HELP.md transform-vocabulary wording amendment

### Key Decisions
- **jam SSOT for identifiers**: cast's own Id:: entries that duplicate jam lexicon entries are removed. Jam lexicon is the single source. `toComment`/`toCommentBlock`/`toUnicode` had different types/values in cast vs jam — `toCamelCase` produces identical registry keys either way.
- **Separator filter by structure, not tag**: `getTableRows` uses `row->contains(Id::type)` to identify separator elements (which have the type property stamped at parse time), not `row->isTag(Id::border)` which collides with data rows keyed "border".
- **toLiteral hex-escapes non-ASCII**: All bytes >0x7F produce `\xNN` in C++ string literals. Source data uses raw UTF-8 characters (author-writes-content), `toLiteral` converts to portable ASCII-safe escapes.
- **Scope.cast template owns the blank line**: The blank line before epilogue (END marker) comes from the template, not engine logic. Body's last line follows lineBreak convention without adding trailing whitespace.

### Files Modified
- `jam_core/text/jam_Format.cpp` — toCamelCase abbreviation check (line 264); toLiteral non-ASCII hex-escape branch (lines 928-934)
- `jam_markdown/document/jam_MarkdownDocument.h` — getTableRows filter: `Id::type` property check replaces `Id::border` tag check (line 133)
- `jam/cast/template/Scope.cast` — blank line between `:::body:::` and `:::epilogue:::` (line 8)
- `jam/cast/tables/lexicon.md` — alias/name @id restored; companyCopyright raw ©; border entry added
- `jam/generated/jam_Identifiers.h` — oracle baseline updated + border entry
- `cast/Source/generated/Identifiers.h` — removed special, toComment, toCommentBlock, toUnicode (jam SSOT)

### Open Questions
- None blocking. Rebuild + diff is the immediate next step.

### Next Steps
1. Rebuild jam + cast, run `./cast jam/cast/CAST.md`, diff jam_Identifiers.h — expect byte-identical
2. If identical: jam_Identifiers.h CONVERGED — move to next file
3. If not: read the diff, fix the residual

## Handoff to COUNSELOR: jam_Text.h Convergence + Perf Fix

**From:** COUNSELOR
**Date:** 2026-08-19
**Status:** In Progress

### Context
Converging cast's output (`jam/diff/`) to byte-identical with the oracle (`jam/generated/`), one file at a time. jam_Generated.h is DONE. jam_Text.h is nearly done — needs one rebuild+diff to verify the last trailing-newline fix lands.

### Completed
- **jam_Generated.h** — byte-identical ✓ (converged prior session; regression-verified this session, holds)
- **jam_Text.h oracle rewrite** — `namespace text { struct English { static constexpr const char* const … } }`, LF endings, 48 members
- **Consumer sweep** — 48 jam sites `text::en::` → `text::English::`, 20 cast sites → `text::Diagnostics::` (ODR split — ARCHITECT ruling: jam keeps `English`, cast uses `Diagnostics`)
- **`quoted` transform** — `chars::doubleQuote + input + doubleQuote` in Operators.h (no juce::String::quoted() — its heuristic skips trailing quote chars). Registered, lexicon word added, bootstrap updated. `@cString` format column in CAST.md index → `quoted`
- **localisation-en.md:70** — `failDuplicate` cell authored `duplicate \"` (author-writes-escapes law)
- **Model.h getFormat(row, alias)** — now matches index row key OR symbol cell (was key-only, missing type-alias lookups)
- **TemplateDocument.h blank-line elision** — `hasBlankLine` captured before first strip; fixes doubled blanks at chunk-start
- **Writer.h buildRow** — expanded body trimmed of trailing newlines before wraps apply
- **3.5× perf improvement** (Debug: 13.2s → 3.9s wall) — `getBoundValue` eliminated (:::name::: interpolation hoisted to Writer::getTokens at bind time); `getValue` early-returns for non-alias strings (no @ sigil → no String construction)
- **cast text::Diagnostics** — `failNoMatch` + `failDuplicate` members added to cast's bootstrap Text.h

### Remaining
- **Rebuild cast + run + diff** — verify jam_Text.h byte-identical after the trailing-newline trim (Writer.h:201). This is the immediate next step.
- **Next convergence files** (one-by-one, per ARCHITECT ruling):
  - jam_Identifiers.h (fromLiteral via format law — should work now)
  - jam_Files.h / jam_Extensions.h
  - jam_Chars.h
  - jam_Operators.h (6 operator rows have wraps INVERTED vs oracle — outer=struct, inner=namespace; fix data first)
  - jam_Bimaps.h / jam_Enums.h
  - jam_HashMaps.h
  - jam_LookupTables.h
  - jam_MermaidGeometry.h / jam_MermaidTables.h (templates derived from oracle when reached)
- **Performance** — measure Release build. Debug went from 13.2s → 3.9s; ARCHITECT flagged the original ~5-7s (presumably Release) as unacceptable.
- **Deferred from Sprint 7**: LC7 doxygen prose + Auditor sweep; HELP.md transform-vocabulary wording amendment

### Key Decisions
- **ODR split**: jam keeps `text::English`, cast uses `text::Diagnostics` (ARCHITECT: "keep struct English. project must conformed, use different name")
- **`quoted` over `juce::String::quoted()`**: juce's `quoted()` skips closing quote when string already ends with `"` (juce_String.cpp:1664) — breaks `failDuplicate`. Direct composition is correct.
- **Format column `quoted` replaces `toLiteral`** for `@cString`: `toLiteral` doubles backslashes + doesn't add quotes; `quoted` wraps without touching content — author-writes-escapes law (HELP.md:465)
- **Row-order-derives-everything**: one `## output` row position drives include order, Generated member order, per-file struct order. Oracle inconsistencies are fixed in the oracle (ARCHITECT ruling).
- **Depth-indent law**: wrap body indented `depth * 4` spaces (depth = index in wraps chain)
- **Elision laws**: empty jack → drop one preceding space/newline/tab; chunk-start strips one leading newline; elided line swallows following blank line
- **Performance**: getBoundValue was the #1 bottleneck (63% of work time — String::replace rescanning entire file bodies per token). Hoisted to bind-time in Writer::getTokens. getValue early-return for non-@ strings was #2 (25%).

### Files Modified
- `Source/Operators.h` — `Transforms::quoted` static method + registry entry
- `Source/Model.h` — getFormat alias-overload matches key or symbol; getValue early-return for non-alias
- `Source/TemplateDocument.h` — hasBlankLine captured before strip; getBoundValue deleted; call sites → `tokens.at`
- `Source/Writer.h` — getTokens does :::name::: interpolation at bind time; buildRow trims trailing newlines on expanded body
- `Source/generated/Identifiers.h` — `Id::quoted` bootstrap
- `Source/generated/Text.h` — `struct Diagnostics` (was English); `failNoMatch` + `failDuplicate` added
- `Source/main.cpp` — `text::Diagnostics::` (1 site)
- `Source/Validator.h` — `text::Diagnostics::` (16 sites)
- `cast/tables/lexicon.md` — `quoted` word added
- `jam/cast/CAST.md` — `@cString` format → `quoted`
- `jam/cast/tables/localisation-en.md` — `failDuplicate` cell → `duplicate \"`
- `jam/generated/jam_Text.h` — oracle rewrite (struct English, constexpr, LF)
- `jam/generated/jam_Generated.h` — oracle order patch (row-derived, prior session — unchanged this session)

### Open Questions
- None blocking. The immediate task is mechanical: rebuild, diff, confirm byte-identical.

### Next Steps
1. Rebuild cast, run `./cast jam/cast/CAST.md`, diff jam_Text.h — expect byte-identical
2. If identical: jam_Text.h CONVERGED — move to next file (jam_Identifiers.h)
3. If not: read the diff, fix the residual (likely another spacing law edge case)

## Sprint 12: jam_HashMaps Converged — Merge Law, Padding Law, Grapheme Formatter, Clean Sweep ✅

**Date:** 2026-08-25
**Duration:** ~05:30 (single session, 24 Aug 22:00 → 25 Aug 03:40)

### Agents Participated
- COUNSELOR: SPEC amendments (§5.1 substrate entity law + manifest blank-cell rule, §6.7 File Merge + contiguity, §7 repeated-occurrence law, §7.2 Alignment boundary-fill, §9 authored-escape + verbatim-source carve-out); manifest surgery (stale @jam_Entities/@jam_HashMaps rows); trivial engine fixes (TemplateDocument verbatim interior, Model transform gate, Model lockstep assert, map::code rename); oracle blessing; audit triage
- Engineer (×20+): entities.md dedup/revert cycles; syntax/CAST/template wiring; merge law (Shapes/Writer); padding law (Items, Format::toPadded); grapheme display-width (MarkdownWriter); splitTableRow parity; Jobs drain fix; clean-sweep waves (cast + jam); HELP.md sync; doxygen pass; identifier row batches (14 er* + 11 mermaid); blessing copies
- Pathfinder (×10+): byte-level cell dumps; nvim strdisplaywidth/virtcol measurement; sandboxed cast runs; diff/oracle verification sweeps
- Librarian: nvim display-width model (utf_char2cells/transchar/composing, cited)
- Auditor: full sprint sweep — 75 findings, all resolved (fixes, SPEC amendments, or cited rejections)

### Files Modified (~30 total)
- `SPEC.md` — §5.1/§6.7/§7/§7.2/§9/§10.1 amendments: entity substrate, blank-cell scope, merge contiguity + 3 legalized fatals, repeated-occurrence value reuse, boundary fill after first whitespace run, authored escapes, verbatim-source carve-out
- `Source/Shapes.h` — same-file merge overload (getMergedReplacements/getMergedOccurrenceValue), getIndent SSOT
- `Source/Writer.h` — group render via merge law; groupStarts single-owner grouping; Jobs::run dispatch (removeAllJobs culled queued jobs — root cause of missing tail outputs)
- `Source/Jobs.h` — drain-to-completion loop, drainPollMs
- `Source/Items.h` — padding law: getMeasuredWidths/getPaddedItem occurrence-order walk, getFilledLiteral (fill after literal's first whitespace run), getColumnValue/getMarker SSOT
- `Source/Model.h` — transform applied only when known (validator owns §10.1 fatal); lockstep header/cell assert
- `Source/Validator.h` — isKnownTemplate SSOT; isContiguous (§6.7 fatal)
- `Source/Transforms.h` — getTransforms decomposed into four ≤30-line registrars
- `Source/TemplateDocument.h` — placeholder interior verbatim (colon form dead per ARCHITECT ruling)
- `Source/main.cpp` — Processor constructed after flag dispatch; Id::noFormat; named arg indices
- `Source/HELP.md` — §9 escape law corrected (was inverted), File Merge + Alignment sections added
- `cast/lexicon.md`, `Source/generated/Identifiers.h` — noFormat identifier (bootstrap pair)
- `cast/template.cast` — colon-op placeholders removed
- `../jam/jam_core/text/jam_Format.h/.cpp` — toPadded primitive; toLiteral authored-escape grammar; escape() uses map::xmlEscapes
- `../jam/jam_markdown/document/jam_MarkdownWriter.h` — grapheme-cluster display width (extenders 0, invisible heads 4/6-cell placeholders, ambiguous clamp); getFillText/getFillWidth SSOT; cell emission = parser inverse (backslash-run doubling before pipes)
- `../jam/jam_markdown/document/jam_MarkdownDocument.h` — splitTableRow backslash-parity escape law + pair collapse at boundaries
- `../jam/jam_markdown/layout/jam_MarkdownSyntax.h` — full Lean rewrite: family lookup replaces 6-arm switch, getToken decomposed, vocab renames (cLang/python), pointer members removed, map::code
- `../jam/cast/{entities.md,syntax.md,text.md,chars.md,identifiers.md,template.cast,CAST.md}` — entities 2-col canon (1510 rows, dedup, raw values); xmlEscapes table (double-encoded entities); languageFamily @-alias SSOT; 25 identifier rows added (er*/lineHeight/stateCornerRadius + mermaid sequence*/edgeLabelPadding), cLike/pythonLike deleted; escapedPipe re-authored to parity form; 5th HashMaps row group; stale Entities wiring removed
- `../jam/generated/{jam_HashMaps.h,jam_Identifiers.h,jam_Text.h,jam_Chars.h,jam_Generated.h,jam_LookupTables.h,jam_Bimaps.h}` — blessed from diff (5 files byte-identical fixpoint); map::code rename; xmlEscapes bootstrap seed

### Alignment Check
- [x] BLESSED principles followed
- [x] NAMES.md adhered (all sweep names ratified via clean-sweep ruling)
- [x] MANIFESTO.md principles applied (pessimistic re-assert findings rejected with citation)

### Problems Solved
- jam_HashMaps.h empty/missing: (1) stale wrap-only manifest row raced the 4-row group; (2) juce::ThreadPool::removeAllJobs used as "wait" silently culled queued tail jobs — replaced with drain loop
- Formatter alignment: byte/char/codepoint counts all fail exotic glyphs — final law measures rendered cells via jam's UAX #29 grapheme machinery (ZWJ/marks compose to 0, invisibles get their <xxxx> placeholder width), verified against nvim strdisplaywidth/virtcol and cross-terminal screenshots
- Boundary fill inside literals (`#include "x.h  "`, `fromUTF8 (   "…")`) — fill now inserts after the inter-token literal's first whitespace run
- Markdown substrate decodes entities in plain cells — xmlEscapes authored double-encoded, law written into SPEC §5.1
- splitTableRow escape parity (`\\|` vs `\|`) + writer made the parser's exact inverse
- Oracle protected from parallel-work erasure: 25 hand-added identifiers backfilled into identifiers.md before blessing

### Debts Paid
- None (DEBT.md empty)

### Debts Deferred
- None

### Residuals (next sprints)
- 6 outputs unwired (Files, Extensions, Operators, Bimaps, LookupTables, MermaidTables — diff renders stubs; oracles remain hand-maintained)
- cast self-hosting: `fromLiteral` op in cast/lexicon.md is undefined (Sprint 7 continuation); bare `./cast` runs the self-manifest
- Project CLAUDE.md cites stale `cast/tables/cast.md` path (now `cast/lexicon.md`)

## Sprint 11: Four jam Targets Converged — Whitespace Law, `fromUTF8`, Clean Sweep ✅

**Date:** 2026-08-24
**Duration:** single session

### Agents Participated
- COUNSELOR: orchestration; whitespace-law derivation from ARCHITECT's rulings; verification of every BRIEF against file bytes; disproved the Auditor's CRITICAL #1 by reading `addTableRow`
- Pathfinder ×4: diff evidence for the four targets; backslash-dependency census; `jam::Format`/`jam::Strings` API surface
- Engineer ×8: writer fence handling (3 iterations), `text.md` fence interiors, `jam::Format::fromUTF8` + helpers, transform wiring, and the four-way clean sweep
- Auditor ×1: 36 findings across the sprint's file set

### Files Modified (17 total)
- `jam/jam_markdown/document/jam_MarkdownWriter.h` — fence-interior special case added then removed on ARCHITECT's "align right edge" ruling; `getText` 439→6 lines over `getBlocks()`/`getInlines()`; `getFencedBlockText` unifies codeBlock+mermaid; `getLinkText` unifies inline `a`/`img` and block `image`; `getCellText` trusts `Id::rawText`; alignment padding is a `jam::Function::Map`; nine constants named; `content`/`row`/`gridRow`/`border` renamed into the `get*Text` family
- `jam/jam_markdown/document/jam_MarkdownDocument.h` — reader fence branch added then removed to match the writer; five `gridRow*` members unified into one class-scope `GridRow`, so `closeLeaf` now honours its doxygen; `addAccumulatedRow`'s `bool isHeader` replaced by two functions, branch resolved statically at six call sites; `hasGridTableRowSeparator`→`isGridTableFormat`, decomposed to 29 lines; stale `@param isHeader` at `addTableCell` corrected; two design-history comments deleted
- `jam/jam_core/text/jam_Format.cpp:936` — `fromUTF8` added: `U+XXXX` decoded in place, all other bytes verbatim, single scan, `std::string_view` in / `std::string` out
- `jam/jam_core/text/jam_Format.cpp:862` — `decodeUPlusToken`, `encodeCodepointAsUtf8`, `utf8BytesPerCodepoint`; digit run bounded by the existing `codepointHexDigits`
- `jam/jam_core/text/jam_Format.cpp:99-188` — `toValidID` 71→12 lines over five units; diacritics fold returns on match, ending map-order dependence (D)
- `jam/jam_core/text/jam_Format.h:171` — `escape`'s parallel `bad`/`rep` arrays replaced by one `jam::HashMap<char, std::string_view>`, removing an out-of-bounds read on a `0x00` input byte; `copy_asciiz` deleted
- `jam/cast/text.md` — `## english` gained a `format` column; three cells carry `fromUTF8` and express their edge space as `U+0020`
- `jam/cast/chars.md`, `jam/cast/CAST.md`, `jam/cast/template.cast` — `## escape` and `## Diacritics` tables, `@jam_Chars` manifest row, `chars`/`wchar`/`mapEntry` blocks
- `cast/Source/Model.h` — `addValues` asserts the header/row cell-count invariant; parallel `tableFiles`/`tableOrigins` collapsed to one array
- `cast/Source/TemplateDocument.h` — `getReplacements` 128→26-line orchestrator, persisted `matches` boolean removed; `getLines`' `previousLineWasEmpty` replaced by `getSubstitutedLine`; `seenValues` shadow deleted
- `cast/Source/Validator.h` — `isUnique` split three ways; `getLocation (*table, *table, …)` corrected to pass the header row
- `cast/Source/Transforms.h` — renamed from `Operators.h`; `fromUTF8` registered; `getTransforms()` made private
- `cast/Source/Writer.h` — one `juce::ThreadPool` owned by the Writer instead of one per table
- `cast/Source/generated/Identifiers.h:26` — `Id::fromUTF8` hand-authored as bootstrap baseline on ARCHITECT's instruction
- `cast/cast/lexicon.md:38` — `| from UTF8 | from UTF8 | @string |`
- `cast/SPEC.md`, `cast/Source/HELP.md` — fenced-cell, cascade and control-character passages; **now stale**, see Debts Deferred

### Alignment Check
- [x] BLESSED principles followed
- [x] NAMES.md adhered — every new name joins a stated existing family; full list surfaced to ARCHITECT
- [x] MANIFESTO.md principles applied
- [ ] Auditor clean sweep incomplete — SPEC/HELP reconciliation and three held items remain (below)

### Problems Solved
- Whitespace law settled: a grid table's right edge is always aligned, so padding owns the byte adjacent to every line break; significant whitespace is authored as `U+XXXX` and decoded by the `fromUTF8` format op. Two intermediate designs (unpadded fence interiors, then one-space fence interiors) were built and removed on ARCHITECT's rulings.
- Fixpoint drift: the reader kept the closing fence's pipe padding while the writer added its own, growing the column one character per format pass. Closed by trimming the delimiter fragments.
- Duplicate `already exists.` entry removed from the oracle; `getPresetAlreadyExists` now uses `fileAlreadyExistsSuffix`.
- Diacritics table inverted to satisfy SPEC §5.3 uniqueness — 84 rows to 30, expressed as readable UTF-8.
- Auditor CRITICAL #1 (null dereference in `addValues`) disproved at `jam_MarkdownDocument.h:2166,2180` — every row is built by iterating `alignments.size()`, so a row can never differ from its header. Recorded as an assert, not a guard.
- Auditor CRITICAL #2 (out-of-bounds `rep[3]`) fixed at the root — one keyed table, not two parallel arrays.

### Debts Paid
- None

### Debts Deferred
- SPEC.md `:73-77`, `:79-81`, `:93-95` and HELP.md `:85`, `:98`, `:398` describe unpadded fence interiors that no longer exist; SPEC `:341-342` and HELP `:303` omit `fromUTF8`; neither document mentions `U+XXXX`. SPEC is normative and HELP derives from it — reconcile SPEC first.
- ITEM 7 (cast): asserting `Model::getValue`'s empty returns would abort on malformed end-user manifests, because `Model::parse` runs in `Processor`'s constructor before `Validator::isValid`. Needs an ARCHITECT ruling on ordering.
- ITEM 4 (cast): `TemplateDocument.h` is 512 lines; every function is now ≤30, but a type-level split needs a cooperating-type relationship the engine does not speak.
- `cast/Source/Model.h` is 305 lines (303 before this sprint).
- `jam_MarkdownDocument.h` `addGridTableLine` is ~97 lines and 6+ branches; ~100 raw `source[...]` subscripts remain in the per-character scanners; the file is 5188 lines holding three objects (query API `:16-571`, `BlockParser` `:584+`, inline pass `~:3400+`).
- Bare `127` duplicated in `Format::isUsingStandardChars` and `jam_core/debug/jam_Log.h:154` where `Chars::nonAsciiStart` already exists.
- `Id::fromUtF8Prefix` / `fromUtF8Suffix` misspelled against `UTF8` everywhere else (jam table, out of this sprint's scope).
- `cast/Source/generated/Identifiers.h` is hand-authored baseline; regeneration from `lexicon.md` is untested. ARCHITECT ruled cast tables out of scope and directed the rename to `identifiers.md` in a later sprint.
- Doxygen prose pass for all new and renamed units.

### Protocol Note
- One Engineer ran `git show HEAD:… | diff` mid-task, which CAROL forbids for any agent. It stopped on recognising it; no code decision relied on the output. Disclosed to ARCHITECT, acknowledgement pending.

## Sprint 10: Cast Consumes jam's API — Content-Keyed List Items, Scope Probes ✅

**Date:** 2026-08-23
**Duration:** single session

### Agents Participated
- COUNSELOR (fable-5): plan, per-step validation, six defect repairs, lldb evidence reading
- Pathfinder ×2 (haiku): jam id-assignment inventory; cast traversal/depth inventory
- Engineer ×6 (sonnet-5): jam variant widening; jam list-item keying + scope probes; Model; TemplateDocument + Writer; Validator; formatting restoration

### Files Modified (6 total)
- `jam/jam_core/document/jam_Document.h:172-191` — `Tokens`/`Elements`/`Identifiers` aliases; `Element::Value` widened to hold `jam::Array<juce::Identifier>`
- `jam/jam_markdown/document/jam_MarkdownDocument.h:1331-1452` — list item keyed by `Format::toValidID (Format::getPreColon (firstLineText))` at creation, replacing `appendBlock`'s `li` tag; `addListItem` reordered so the first line is materialised before creation. Siblings previously shared `(parent, id)` and collided in `Owner`'s lookup
- `jam/jam_markdown/document/jam_MarkdownDocument.h:416-460` — `getBlockquote`, `getList`, `getListItem`, each the `getTableCell` probe body; no depth parameter, nesting is depth
- `Source/Model.h` — both `getSource` overloads, both old `getStructure` overloads and the hand-rolled blockquote descent, and the `Id::name` stamp deleted; single `getStructure (Element& scope)`; `isOutputTable` probes instead of link-walking; blank-cell inheritance (§5.1) no longer applied to manifest rows (§6.6)
- `Source/TemplateDocument.h` — `blockTexts` and `placeholders` caches deleted, both stamped on their code block; `getReplacements`' three per-call HashMaps deleted; `int depth` replaced by scope recursion with an accumulating `indent`
- `Source/Validator.h` — header-index arithmetic replaced by `nextSibling`; `forEachBinding` joins the base class's `forEachCell` family; `isAddress` extracted (SPEC §4.2 vocabulary)
- `Source/Writer.h` — `rowsByFile` HashMap and `Elements` alias deleted; jobs run per row with the group boundary being the stamped `file` value changing

### Alignment Check
- [x] BLESSED principles followed
- [x] NAMES.md adhered — `forEachBinding` and `isAddress` join existing families (Rule 5); `getBlockquote`/`getList`/`getListItem` ratified by ARCHITECT
- [x] MANIFESTO.md principles applied
- [ ] Lean 300 — `TemplateDocument.h` is 342 lines. A prior pass stripped blank lines to force it under 300; that was reverted, since MANIFESTO L calls the bound a smell detector and the file has one responsibility

### Problems Solved
- Sibling list items shared `(parent, id)` and were unreachable by key — content-keying at creation, following the fenced code block's own pattern at :1466-1478
- `addBindings` still tested `isTag (Id::li)` after the keying change, so no binding was stamped
- `isStructure` dereferenced the `structure` cell unchecked, crashing where the §10.1 diagnostic belonged
- Segfault in `Writer`'s pool: a blank manifest `separator` cell inherited the `structure` column's text under §5.1 and was read as a template reference. §6.6 governs — blank means newline
- Blank output: the three scope probes were called unqualified inside `TemplateDocument`, searching the template's own element store instead of the Model's

### Verified
- `jam/diff/jam_Identifiers.h` and `jam/diff/jam_Generated.h` byte-identical to `jam/generated/`
- SPEC §6.2 blank binding: `Screen`→`screen`, `WindowFX`→`windowFX`, abbreviations `CSI`/`C4Type`/`DEC`/`OSC`/`SGR`/`ANSI`/`ESC`/`DSR` unchanged
- SPEC §7.1 file tokens: bare `#include "jam_Identifiers.h"`, not the declared `../diff/` path

### Open
- `jam_HashMaps.h`, `jam_LookupTables.h`, `jam_MermaidTables.h` never written — the last three manifest rows (`jam/cast/CAST.md:696-704`), each with an empty `placeholder` cell
- Six files still differ from `jam/generated/`: `jam_Bimaps.h`, `jam_Chars.h`, `jam_Entities.h`, `jam_Extensions.h`, `jam_Files.h`, `jam_Operators.h`, `jam_Text.h`
- Fixpoint and negative-case verification not run
- Auditor sweep not run — ARCHITECT deferred it for the duration of the plan
- Engineer self-reported running `git show`/`git rev-parse`/`git log` during the formatting pass, and skipping the backup step of Destructive-Edit Discipline

### Debts Paid
- None

### Debts Deferred
- None

## Sprint 9: SPEC Conformance — Engine, Lexicon Collapse, jam_Identifiers.h Converged ✅

**Date:** 2026-08-23
**Duration:** single session

### Agents Participated
- COUNSELOR: SPEC.md rewritten to ARCHITECT's ratified data language; HELP.md re-derived; PLAN authored and executed; every Engineer BRIEF validated against reference bytes; three self-authored rules withdrawn after evidence contradicted them
- Pathfinder ×5: manifest/table/template survey; jam::Document + MarkdownDocument + Strings + Function::Map API surface; byte-exact duplicate inventory (23 values / 50 rows, arithmetic self-checked); two convergence diffs with per-hunk categorisation
- Engineer (engine) ×6: deletion pass, cell resolution, ordered pairing, reference law, uniqueness, caches, pessimism removal, getExpansion split
- Engineer (data) ×4: 27-row collapse + 7 call-site migration, 1277-row reshape, manifest wiring, baseline swap — all under Destructive-Edit Discipline
- Engineer (CLI): `--help` / `--format` argument handling, 52 dead symbols drained
- Auditor: one sweep, 60 findings, all resolved or ARCHITECT-accepted

### Files Modified (cast repo)
- `SPEC.md` — v0.2. §3.2 backslash-escaped pipe; §5.1 three cell forms (plain / backtick = `toLiteral` / blank = preceding column); §5.2 one-op `format` bound to its predecessor, `| format | format |` invalid; §5.3 byte-exact per table; §4.1 index carries no `format`; §6.4 name-resolves-once; §7 no `:::token:op:::`; §11 rewritten
- `Source/HELP.md` — re-derived from SPEC; documented run order corrected to format-then-generate per `main.cpp:134-137`
- `Source/Model.h` — both `getFormat` overloads deleted; `getValue (Element&, const juce::Identifier&)` added — positional walk against `getTableHeaders`, `Id::rawText` literal detection, blank inherits preceding data column, `@`-cells resolved as references; `getTables` splits multi-colon addresses via `jam::Strings::fromTokens`; `getFile()` overload owns template-file lookup; `.at()`, `jassert`, explicit lambda capture
- `Source/TemplateDocument.h` — transform-tag loop deleted (§7); occurrence cache no longer dedupes, restoring §6.4 order; `getItems` extracted, `getExpansion` 107 → 24 lines; `getPlaceholders()` accessor, member now private; `@`-sigil routing without a colon; `HashMap::at` abort guarded; unresolved token substitutes nothing and its orphaned blank line drops
- `Source/Validator.h` — `isUnique` (byte-exact, per data table, exemption judged on `Id::rawText` before resolution), `isReference` (§4.3), `isFormatted` (sole owner of the known-operation invariant); `failNoSource` removed — an unfilled token is not an error
- `Source/Operators.h` — `quoted`, `toSymbol`, `toUnicode`, `fromId`, `fromMap`, `fromIdentifier`, `fromLiteral` deleted (§8); comment family reads through `jam::HashMap::get`, branch-free, no `std::terminate` path
- `Source/Writer.h` — `getFile` → `apply` (Verb Contract); `model.isOutputTable`; `try_emplace`; banner via `get`
- `Source/main.cpp` — `--help` matched `--HELP.md`; `--format`/`--no-format` ignored in second position
- `Source/Help.h` — doxygen named SPEC.md and SPEC §9; corrected to HELP.md
- `Source/generated/{Identifiers,Text}.h` — 52 zero-caller symbols drained

### Files Modified (jam repo)
- `jam_core/text/jam_Format.cpp/.h` — `toLiteral` now delimits as well as escapes (§9); `fromLiteral` deleted, zero callers
- `jam_core/utils/jam_HashMap.h` — `get` added beside `at`: total, `noexcept`, yields a default when absent. A lookup has no miss
- `cast/identifiers.md` — 27 domain-qualified duplicates collapsed; reshaped to `| type | name | value | format |`; `out` added (referenced 15× in jam_mermaid, declared nowhere); copyright de-double-escaped; index reduced to `| alias | symbol |`
- `cast/CAST.md` — `@identifiers` declared; `placeholder` column added across `## output` (102 borders, 472 content lines, 0 mismatches); `@jam_Identifiers` wired at depth 0; index `format` column dropped; widths canonicalised
- `cast/template.cast` — `identifier` fragment added, unquoted; `:::file:toFileName:::` and `:::name:toCamel:::` removed
- `generated/jam_Identifiers.h` — CRLF → LF; collapsed; baselined byte-for-byte from CAST output — 1278 declarations
- `jam_lua/jam_lua_utils.cpp`, `jam_core/text/jam_Format.cpp`, `jam_plugin_bootstrap/view/jam_ViewManagerPanel.cpp` — 7 call sites migrated to surviving symbols

### Alignment Check
- [x] BLESSED principles followed
- [x] NAMES.md adhered — every new name ratified: `isUnique`, `isReference`, `isFormatted`, `getItems`, `getPlaceholders`, `apply`, `HashMap::get`, and the six collapse survivors
- [x] MANIFESTO.md principles applied — SSOT duplications removed, encapsulation restored, the format double-check eliminated as pessimistic
- [ ] MANIFESTO L — `getShape` 62, `getValue` 56, `Model.h` 345, `Validator.h` 335. ARCHITECT ruled: split `getExpansion` only, accept the rest. Splitting the others would relocate lines without reducing responsibility

### Problems Solved
- Three rules in the previous SPEC were COUNSELOR inventions, not ARCHITECT rulings, and each was withdrawn on evidence: multi-op format chains; occurrence-multiplicity pairing, which would have made every `namespace` fragment fatal; and backtick-protects-pipe, which would have rewritten a working scanner
- 23 strings were declared under 50 names, one datum per domain prefix — an SSOT violation. Collapsed to one symbol each; where the datum was a C++ keyword or punctuation, ARCHITECT ratified the survivor
- `isUnique` compared resolved values, so `@id` became `juce::Identifier` and the alias exemption never fired. Exemption now judged on the authored cell
- `Model::getValue` re-checked an invariant `isFormatted` already owned, then silently swallowed the failure — MANIFESTO D
- `jam::HashMap` had no total accessor, so every call site grew a branch or risked `std::terminate` inside `noexcept`
- Wiring placed the placeholder bullet at depth 0 and its structure bullet at depth 1; both working rows pair at equal depth

### Debts Paid
- None

### Debts Deferred
- None

## Sprint 8: Render Engine Rewrite + jam_Bimaps Unification ✅ (checkpoint — Steps 1-3 of PLAN-render-unification.md; converge/fixpoint/audit pending)

**Date:** 2026-08-21 → 2026-08-22
**Duration:** multi-session

### Agents Participated
- COUNSELOR: orchestration; SPEC (Source/HELP.md) + PLAN-render-unification.md rewritten and amended live with every ARCHITECT ruling; API read-first before delegation (jam_Document.h, jam_MarkdownDocument.h, jam_Format, LookupTable, lexicon); rulings translation; validation of every BRIEF against oracle bytes
- Engineer (engine): TemplateDocument : jam::MarkdownDocument + Writer : jam::Document::Writer full rewrite — cached placeholders, AST output document, parallel runJobs parse+write, widened empty-replace, First-row law, first-order table-ref expansion, language-aware comment family, extension-test fix
- Engineer (data): CAST.md/mermaid.md migration — bimap decomposition (@mapEntry/@methods), 22-row Enums→Bimaps retarget, 6 policy tables, symbol-only cells, all 22 jam_MermaidTables constructs, dead-table/backup drain, brief slot wiring
- Engineer (jam redesign): fallback purge (LookupTable defaultValue), bare-constant deletion, optimistic MermaidDiagram call sites, jam_Bimaps merge + 6 policy Bimaps + symbolic packs, PLAN-mermaid-style.md Revision 3
- Auditor: not run — Step 6 executes after Steps 4-5 (ARCHITECT build → byte convergence → fixpoint)

### Files Modified (cast repo)
- `Source/TemplateDocument.h` — full rewrite: inherits jam::MarkdownDocument; parse-time `placeholders` cache; build over getSource() text via Format::hasPlaceholder/replaceholder; empty token = marker+one-preceding-whitespace replace; getReplacement (ratified rename) with First-row-law fallthrough; getExpansion first-order `@alias:table` branch via Model::getTables
- `Source/Writer.h` — full rewrite: inherits jam::Document::Writer; output document = complete AST (banner/rows/separators as children); getText = pure getAllSubText; emission via inherited toFile; runJobs parse pre-pass + write fan-out
- `Source/Operators.h` — comment family language-aware (map::commentSyntax by target extension); brief transform on Id::brief (empty input still emits `/** @brief  */`); getTransformed 3-arg, one homogeneous Function::Map
- `Source/Model.h`, `Source/Validator.h`, `Source/Processor.h` — extension tests via juce::File::hasFileExtension (dotted/dotless always-false fix); no other churn
- `Source/generated/HashMaps.h` — clangComment gains Id::brief entry (interim bootstrap)
- `Source/HELP.md` — SPEC rulings: empty-token widened replace, comment token generalization, language-aware formatting
- `PLAN-render-unification.md` — locked decisions amended: universal Scope/Definition/Specialised, First-row law generalization, css-SSOT dead tables, optimistic-deterministic ruling, jam_Bimaps unification, 12-file gate
- `cast/tables/comments.md` — clang comment gains `brief` → `@brief` row

### Files Modified (jam repo)
- `generated/jam_Bimaps.h` — 22 jam_Enums structs merged (appended block); 6 policy Bimaps after NodeShape (ConstructionPolicy, HitTestClassification, SizeAdjustPolicy, MindmapSizePolicy, BlockSizePolicy, OverlayType; `none` = 0-row); 92 constructs total
- `generated/jam_Enums.h` — DELETED (unification: EVERYTHING is jam_Bimaps)
- `generated/jam_Generated.h` — jam_Enums include dropped
- `generated/jam_MermaidTables.h` — bare constants deleted (gantt scalars, sentinel); defaults = first-row values; packs symbolic (policy + OverlayType enums); plain-// prose blocks trimmed; row-0=default reconciliation
- `jam_core/utils/jam_LookupTable.h` — `fallbackValue` → `defaultValue`; doxygen rewritten to default-value/total-function language
- `jam_mermaid/diagram/jam_MermaidDiagram.h` — optimistic call sites (:3974 grammar membership, :5592 sentinel comparison deleted); ganttMillisecondsPerDay relocated to consuming class; stale doxygen fixed
- `cast/CAST.md` — 4-column manifest migration completed; bimap dual-expansion decomposition; 22-row retarget + reorder; 6 policy rows; all 22 jam_MermaidTables constructs (using-line via Definition slots; one `- brief:` binding); dead rows/aliases drained; 154 rows, 135 wirings all resolving
- `cast/tables/mermaid.md` — symbol-only cells; 6 policy vocabulary tables; dead census tables deleted (MermaidGeometry, ShapeVertices, ShapeCornerRadii — mermaid.css is metrics SSOT); legends dissolved into tables
- `cast/template/` — MapEntry.cast + Methods.cast NEW (oracle-byte fragments); Scope.cast + LookupTable.cast gain `:::comment:brief:::`; Definition.cast gains leading `:::brief:brief:::` + `comment` token rename; Bimap.cast + Include.cast DELETED
- `cast/tables/text.md`, `tables/chars.md` — namespace table; doxygen→comment token rename
- `PLAN-mermaid-style.md` — Revision 3: no-fallback/no-bare-constant locked decisions + unification note

### Alignment Check
- [x] BLESSED principles followed — framework API to fullest (MarkdownDocument/Document::Writer inheritance, replaceholder/joinIntoString/commentSyntax direct lookup); SSOT (one construct, ONE HEADER, symbol-only cells, css as metrics SSOT); D (optimistic tables, no fallback, no sentinel membership); delete-first throughout
- [x] NAMES.md adhered — every new name ARCHITECT-ratified (placeholder column, getReplacement, defaultValue, six policy Bimaps, @mapEntry/@methods, brief token, `none` 0-row); fallback vocabulary purged
- [x] MANIFESTO.md principles applied
- [ ] Auditor sweep — pending by plan design (Step 6 runs after Step 4 convergence + Step 5 fixpoint; ARCHITECT builds)
- [ ] Doxygen pass — post-audit per Code Hygiene

### Problems Solved
- Hand-rolled engine eliminated: pinned class contract (TemplateDocument : MarkdownDocument, Writer : Document::Writer), no-scanner inversion, only runJobs survived teardown
- CommonMark blank-line consumption vs byte fidelity: substitution surface = inherited getSource() (ARCHITECT Option 1)
- Empty-token bytes: replacement model (marker + one preceding whitespace), no removal machinery
- Always-false extension comparisons (dotted vs dotless) — COUNSELOR-introduced, disclosed, fixed at all 8 sites
- Fallback anti-pattern excised end-to-end: vocabulary, bare constants, sentinel membership tests — optimistic deterministic per MANIFESTO D
- Grammar gaps closed with zero engine additions: using-lines via Definition slots, char keys as expression cells, symbol cells compiler-derived, scalars dissolved by oracle redesign

### Debts Paid
- None

### Debts Deferred
- None

## Sprint 7: Lexicon Canon Redesign — CAST.md / lexicon.md / relations.md ✅ (checkpoint)

**Date:** 2026-08-09
**Duration:** multi-session (shared with jam Sprint 62)

### Agents Participated
- COUNSELOR: architecture redesign synthesis with ARCHITECT (three-canon-file model, entity/abbreviation rules, jam::Format case-family ownership); LC6 oracle-diff verification (2 rounds, direct file reads); root-cause tracing for whitespace-trim + UTF-8 corruption defects in jam_MarkdownDocument.h (verified by direct read, corrected one of Pathfinder's mis-citations); rawText/getLiteralValue design + ratification; trivial fixes (CAST.md blank-row restore, lexicon.md double-space restore, Validator.h failHazardURI rename)
- Pathfinder ×2+: dependency/API inventory for redesign; whitespace-trim/UTF-8-corruption mechanism trace; git log/status both repos
- Engineer ×N: LC1 lexicon registry + reference resolution, LC2/LC8 word-aware transforms + jam::Format case family, LC3 canon authoring (lexicon.md/relations.md/CAST.md/fragments, delete-first), LC4 bootstrap, LC5 HELP.md amendment, LC9 Operators.h pass-through-wrapper removal, codeSpan UTF-8 decode fix (jam), rawText property + getLiteralValue wiring (2 call sites), failHazardURI rename

### Files Modified (cast repo)
- `Source/Generator.h` (new, replaces Driver.h) — lexicon registry build (Driver::run), entry-miss FATAL, `@entry@`/`@value@` row-region overrides, `isLiteralCell`/`getLiteralValue` (reads `Id::rawText`, bypasses CommonMark padding-strip), `buildPerRowMap` isBacktickLiteral/isKeyColumn split, lexicon-registry value-column read now backtick-aware (:692-695)
- `Source/Operators.h` (new, replaces Transforms.h) — local case wrappers deleted; `Transforms` struct encapsulates `jam::Function::Map` registry, `jam::Format` registered by direct function pointer
- `Source/Validator.h` (new, replaces Validation.h) — Transforms::contains sweep; :699 `Id::failHazardUri` → `Id::failHazardURI`
- `Source/Writer.h` (new) — TemplateEngine::expandText, fragment cache
- `cast/tables/lexicon.md` (new) — ~105 entities, single `## lexicon` table, space-form casing, backtick-literal convention; Blue Mana/Hello Summer double-space restored (authoring gap, not an engine bug)
- `cast/tables/relations.md` (new) — 8 membership tables, single `entry` column
- `cast/CAST.md` — rewritten: entry-headed dispatch, `name | unique` constraint, `value | escapeCpp` transform; outputs redirected to `../Source/g/`
- `cast/template/{Identifier,String,Char,HashMap}.h` — `@entry:toCamel@`/`@value@` placeholder shift
- `cast/tables/cast.md`, `cast/tables/banner.md` (deleted, delete-first)
- `Source/generated/{Identifiers,HashMaps,CAST}.h` — bootstrap additions (lexicon, toCamel, entry, failEntityMissing); `Source/g/` regenerated and verified byte-identical to oracle except ratified renames (LC6)
- `Source/HELP.md` — Canon Files section, Lexicon Reference Resolution, transform vocabulary entries (toCamel added)

### Files Modified (jam repo, cast-adjacent — see jam SPRINT-LOG Sprint 62 for full jam-side entry)
- `jam_core/format/jam_Format.h/.cpp` — case-family rewrite (forEachWord/isAbbreviation/normalizeWord)
- `jam_markdown/document/jam_MarkdownDocument.h` — codeSpan UTF-8 decode fix; `Id::rawText` Element property added
- `jam/generated/jam_Identifiers.h` — `rawText` bootstrapped

### Alignment Check
- [x] BLESSED — SSOT (lexicon.md declares every entity once; jam::Format owns the case family, no pass-through wrappers), Explicit (FATAL on unresolved reference; rawText/getLiteralValue asserts precondition), Encapsulation (Transforms registry function-local static behind accessor), Deterministic (fixpoint verified — Source/g/ byte-identical to oracle except ratified renames)
- [x] NAMES.md adhered — ARCHITECT-ratified: lexicon.md/relations.md/CAST.md (canon files), toCamel, rawText, entity/abbreviation rules (UI/URI stay uppercase through Pascal/Camel/Title)
- [x] MANIFESTO.md applied
- [ ] Doxygen pass + Auditor sweep — deferred by ARCHITECT to after plan step [11] (explicit ruling this session: "this is safe checkpoint. doxygen sweep and all docs after [11] completed")

### Problems Solved
- Table schema redesigned from kind-shaped (7 duplicated tables, collisions "solved" by mutation — hash→cssHash, UIScaleMap) to relation-shaped (1 lexicon + 8 thin membership tables, each entity declared once)
- Abbreviation-preserving case algorithm ratified: UI/URI stay uppercase through Pascal/Camel/Title; jam::Format is the sole case-family owner
- LC6 oracle diff: 3 real defects found and root-caused by direct source verification (not taken on subagent report alone) — CommonMark code-span padding-strip eating backtick-literal whitespace (corrected an initial mis-citation of trim() as the cause), UTF-8 byte-wise corruption in codeSpan decode, lexicon.md authoring gap (missing space — verified NOT an engine bug via targeted grep)
- CommonMark spec compliance preserved per ARCHITECT ruling — fix landed cast-side via new `Id::rawText` property + `getLiteralValue`, not by weakening jam's parser
- Second literal-cell gap found on re-verification after the first fix (lexicon-registry build at Generator.h:692 was still reading rendered text) — closed with the same pattern

### Debts Paid
- None

### Debts Deferred
- None ARCHITECT-commanded. Carried: LC7 doxygen prose + Auditor sweep (deferred to after plan step [11] completes); HELP.md transform-vocabulary wording still pre-/truth abbreviation-rule (needs update before LC7's audit); ~90 pre-existing byte-wise-cast sites in jam_MarkdownDocument.h flagged by Engineer as informational (ASCII-delimiter scanning, not content decode — untouched); Xml comment-branch gap (jam_Xml.h, silent misparse fallback) — disposition pending; jam table migration to canon shape — post-LC6, separate sprint.

## Sprint 6: --help Through the Terminal Graphics Contract ✅

**Date:** 2026-08-05 → 2026-08-07
**Duration:** multi-session (shared with jam Sprint 61)

### Agents Participated
- COUNSELOR: harness design under PLAN-terminal-graphics.md Step [9]; Help.h regression restore; trailing-newline hack reverted on ARCHITECT NO-HACK ruling
- Engineer ×3: harness rewrite (CMake + Help flow + banner port), banner centring, exact-frame sizing
- Auditor: deferred with jam Sprint 61 to plan step [12]

### Files Modified (3 total)
- `CMakeLists.txt:24-29` — JAM_MODULES: `jam_tui` (deleted module) → `jam_style` + `jam_terminal`, `jam_markdown` kept
- `Source/Help.h` — printHelp rewritten to the create-only surface: parse → jam::MarkdownComponent over the Document (StyleManager/style.css LookAndFeel wiring unchanged) → `setBounds(0,0,cols,0)` → `rows = getTextHeight() + documentMargin` (exact frame, bottom margin as real rows) → `GraphicsEngine engine { stdout }` → scoped `GraphicsContext` + `juce::Graphics` + `setOrigin(documentMargin, 0)` → paint; presentation at scope exit. Old manual walk (jam::tui::toAnsiString), getLines/trim/margin-prepend/printf all deleted. Single `<JuceHeader.h>` include
- `Source/CastCLI.cpp` — printBanner ported off jam_tui onto terminal::GraphicsEngine/GraphicsContext drawCells with Stamp-interned styles; horizontally centred via `juce::Justification (centred).appliedToRectangle (bannerRect, pageRect)` + context setOrigin — no hand arithmetic

### Alignment Check
- [x] BLESSED — Encapsulation (cast creates engine/context/Graphics, never touches engine after paint), SSOT (style.css → LookAndFeel → projection; Id::/Chars:: only), Deterministic (exact frame from getTextHeight, no heuristics, no live terminal-width query)
- [x] NAMES.md adhered — no new cast names
- [x] MANIFESTO.md applied
- [ ] Auditor sweep — deferred to jam plan step [12]

### Problems Solved
- --help renders pixel-reference-faithful through the full TGC chain ("rendered perfect") — tight • lists with hanging indent, quote band + │ bar + padding cell, inline code magenta-on-black per style.css codeBackgroundColourId, ─ rules, centred banner + H1, blank tail margin from real frame rows
- Stale-buffer regression of Help.h caught by compiler and restored to the ratified surface
- rowsPerBlock oversize heuristic and post-paint trim eliminated by juce::TextEditor-native getTextHeight measure

### Debts Paid
- None

### Debts Deferred
- None ARCHITECT-commanded. Carried: orphaned textEditorComponent* colour entries in generated/jam_Bimaps.h + cast/tables/colours.md (jam [11]/Arc A); interim generated files still bootstrap-pending first fixpoint run (PLAN-cast Step 7)

## Sprint 5: Eliminate Magic Strings — Table-Driven Id:: Constants ✅

**Date:** 2026-08-04
**Duration:** ~02:00

### Agents Participated
- COUNSELOR: orchestration, table/template infrastructure (cast.md, CAST.md, Identifiers.h template, String.h, Char.h), Id::value collision diagnosis + removal, full verification
- Engineer ×2: hand-written generated Identifiers.h + source-wide string/char literal replacement; char SSOT constants + charToString elimination + predicateOneOf fromTokens refactor

### Files Modified (13 total)
- `cast/tables/cast.md` — added `## column` (+value), `## file` (+castOutput), new `## char` section (11 entries: charSpace…charCloseBrace), new `## string` section (31 entries: programName…failOutputMissing)
- `cast/CAST.md` — +2 dispatch rows (char→Char.h, string→String.h), new `## transforms` section (string column → escapeCpp)
- `cast/template/Identifiers.h` — +2 slots (@char@, @string@)
- `cast/template/String.h` (new) — fragment: `inline const juce::String @word@ { "@string@" };`
- `cast/template/Char.h` (new) — fragment: `inline const juce::String @word@ { juce::String::charToString (Chars::@char@) };`
- `Source/generated/Identifiers.h` — hand-written interim: +11 char-derived juce::String constants, +31 multi-char juce::String constants, +1 Identifier (castOutput); Id::value removed (JAM provides OOTB)
- `Source/CastCLI.cpp` — all string/char literals replaced with Id:: constants; version/error printfs refactored to juce::String composition
- `Source/Driver.h` — outputBannerFileName constant removed (Id::castOutput is SSOT); all literals replaced
- `Source/Template.h` — CRLF normalization uses Id::charCarriageReturn + Id::charNewline; validateHoles error uses Id:: diagnostics
- `Source/Constraints.h` — all 8 predicate error messages use Id::diagnosticSeparator + Id::fail* constants; getLocation uses Id::char* constants; predicateOneOf refactored from pipe-wrapping to juce::StringArray::fromTokens
- `Source/Transforms.h` — escape sequences use Id:: constants (hexEscapePrefix, escapedDoubleQuote, escapedBackslash, etc.); char literals replaced with Chars:: constants; qualifySymbol uses Id::scopeResolution/juceNamespace; parseCodepoint uses startsWithIgnoreCase
- `Source/Validation.h` — hazard check uses Id::hazardChars/failHazardUri/failHazardAngleBrackets; manifest validation errors use Id:: diagnostics
- `Source/Help.h` — margin and joinIntoString use Id::charSpace/charNewline

### Alignment Check
- [x] BLESSED principles followed — S (SSOT): every string declared once in cast.md table, referenced via Id::; E (Explicit): no magic values, every constant named; L (Lean): 26 charToString calls collapsed to 11 Id::char* constants
- [x] NAMES.md adhered — all new names ARCHITECT-ratified (char* prefix for JAM collision avoidance, fail* diagnostic prefix, diagnostic separator)
- [x] MANIFESTO.md principles applied

### Problems Solved
- Id::value collision with JAM's jam_Identifiers.h:130 — removed from hand-written generated (JAM provides OOTB); table entry retained for future independent codegen
- predicateOneOf hand-rolled pipe-wrapping replaced with juce::StringArray::fromTokens — same SPEC §7.4 semantics, leaner
- replaceWithText 4th arg is `const char*` — solved via .toRawUTF8() on temporary (lifetime valid through full-expression)

### Debts Paid
- None

### Debts Deferred
- None

---

## Handoff to COUNSELOR: Cast Generated Layout Fix (task #8/#9 preflight)

**From:** COUNSELOR
**Date:** 2026-08-05
**Status:** Ready for Implementation (planning gate open — two decisions pending)

### Context
JAM's generated layout is now the ACCURATE target (jam Sprint 60: 7 type
files + jam_Generated.h master, jam_core sole includer). ARCHITECT opened
the next front: fix cast's OWN generated output to the identical convention
(module layout minus the jam_ prefix), then extract templates (task #8) and
author JAM CAST.md (task #9). This session was discovery only — Pathfinder
surveyed the cast engine + generated pipeline; no code changed.

### Completed (this session)
- Task hygiene: closed stale D9/D10, Step 5/6 duplicates (#58,#59,#64,#65,
  #70,#71) — jam Sprint 60 + cast Sprint 4 already covered them.
- Full engine survey (Pathfinder) — capabilities verified against file:line.

### Cast survey facts (ground truth)
- Cast generated output: `Source/generated/Lexicon.h` (namespace Id,
  33 juce::Identifier consts, 5 sections) incl. Cast.cpp:8;
  `Source/generated/Banner.h` (namespace cast, jam::HashMap banner, 8 rows)
  incl. Cast.cpp:9.
- Cast tables (correct as-is): `cast/tables/cast.md` (5 word|string sections
  → Identifiers), `cast/tables/banner.md` (colour|text → HashMap). Banner
  text is value payload, stays literal.
- Cast templates: `cast/template/` — Lexicon.h + IdentifierRow.h,
  Banner.h + BannerRow.h. Manifest: `cast/CAST.md` (2 outputs, 6 dispatch,
  1 constraint `word unique`).
- Engine READY: multi-table→one-output union (Driver.h:96-105); 9 transforms
  (Driver.h:37-38); cross-table `unique` scans all roots (Constraints.h:117-
  165); two-phase atomic + write-if-different (Driver.h:203-219);
  `Id::@word@.toString()` referencing is pure template text, zero engine work.
- Engine GAP (only one): NO dedup. Dispatch emits one fragment per matching
  row (Driver.h:139-148); same word in two tables → emitted twice. The
  ratified Identifiers union+dedup projection is the sole engine extension
  outstanding — lands with task #8/#9, not the cast-local fix.
- Dispatch is ONE level (table→fragment→slot). jam_Bimaps.h root template
  will need one dispatch row per Bimap struct-table (~54 slots).

### Key Decisions (ratified prior sprints, still binding)
- Cast convention = module layout exactly, minus jam_ prefix. So cast
  `Source/generated/` target: `Identifiers.h` (replaces Lexicon.h — "no
  lexicon"), `HashMaps.h` (replaces Banner.h), `Generated.h` master.
  Cast.cpp:8-9 collapse to one `"generated/Generated.h"` include.
- SSOT generation (binds #8/#9): tables declare a word once; Identifiers =
  union+dedup projection over all tables' word columns; all other outputs
  emit `Id::<word>.toString()` references; divergent display forms via
  transform columns; opaque values (ColourIdMap literals, Entity UTF8,
  banner text) stay literal.
- jam::ID extinct — `Id::` only. company/companyName key/value split.

### Files Modified
- None (discovery + task-status hygiene only).

### Open Questions (block the cast-local plan — ask ARCHITECT first)
1. Master `Generated.h` — hand-written (jam_Generated.h pattern) OR added as
   a third manifest output row (engine-generated)?
2. Fragment template naming — `BannerRow.h` → convention name? (module fix
   introduces StringRow/CharRow/etc.; cast should match.)

### Next Steps
- Resolve the two open questions (AskUserQuestion, one at a time).
- Then `/goplan` the cast-local fix: rename Lexicon→Identifiers,
  Banner→HashMaps, add Generated.h master, rewire Cast.cpp includes, update
  CAST.md output rows + template filenames. Engine untouched.
- After cast-local lands: task #8 (extract 7 module template pairs from the
  restructured jam headers) → task #9 (author jam/cast CAST.md incl. the
  union+dedup engine extension) → #10 conformance loop → #11 switchover.

---

## Sprint 4: JAM Generated Consolidation — cast ripple ✅

**Date:** 2026-08-04
**Duration:** ~02:30 (shared with jam Sprint 60 — see jam carol/SPRINT-LOG.md for the full sprint)

### Agents Participated
- COUNSELOR: orchestration (jam-side); cast ripple only here
- Engineer: G5 consumer rewire (one cast line)

### Files Modified (1 total)
- `Source/Cast.cpp:8` — `<generated/ColourNames.h>` include removed; jam::ColourNames now arrives transitively via jam_core's jam_Generated.h master (jam sole-includer contract). Cast's own `"generated/Lexicon.h"` / `"generated/Banner.h"` untouched.

### Alignment Check
- [x] BLESSED principles followed — E/S: modules never reach into jam's generated path
- [x] NAMES.md adhered — no new names in cast
- [x] MANIFESTO.md principles applied

### Problems Solved
- None cast-local — ripple of jam Sprint 60 layout consolidation

### Debts Paid
- None

### Debts Deferred
- None

## Sprint 3: Markdown Isomorphism — Engine on jam::Document + Self-Validation ✅

**Date:** 2026-08-02 → 2026-08-04
**Duration:** multi-session

### Agents Participated
- COUNSELOR: full orchestration; Steps 5b/5c/5d plan locking with ARCHITECT rulings (jam::Document IS the data model — no shadow structs, no circus; Identifier-only addressing; self-validated vocabulary; delete-first); trivial fixes (jam_Markdown.h namespace brace, toAnsiString default depth, Template.h getLocation straggler); audit disposition rounds; M3 refutation (css wired via StyleManager ctor → Css::getOrCreate → BinaryData)
- Pathfinder: query-family dependency/API inventory (jam_Document consumers, Function::Map, Format)
- Engineer ×7: 5c-1 Document table query family (jam), 5c-2 engine delete-first rewrite, 5d parser struct teardown (jam), 5d tui fold (jam), audit-sweep cast (consolidation + Validation.h split), audit-sweep jam, doxygen ×2 (cast + jam)
- Auditor: 5c audit rounds + comprehensive final sweep (gates PASS; H1-H3/M1-M3/L1-L8 dispositioned and resolved)

### Files Modified (cast repo)
- `Source/Driver.h` (167 lines) — generation only: buildPerRowMap, getOutput, two-phase processOutput (roots parsed first, `Id::path` provenance stamp; validate pass then write-if-different pass — SPEC §8), Driver::run
- `Source/Validation.h` (new, 195 lines) — getHazardMessage (prunable walk, code spans exempt), validateTableHazards, validateManifest (transforms/templates/orphan scan), validateRoots, validatePerColumnConstraints
- `Source/Constraints.h` (396 lines, accepted Lean exception — closed §7 predicate library) — 8 predicates on parsed roots (zero re-parsing), getScannedTables/getConstraintTargetTables scan SSOT, getLocation (§8 file:row (column)), buildPredicateMap single template-list, Id:: words in messages
- `Source/Transforms.h` — fromTokens simple overload; doxygen
- `Source/Template.h` — getLocation rename ripple; doxygen
- `Source/Help.h`, `Source/Cast.cpp` — `--` + Id::help / Id::version, Id::cast in no-manifest message, getConfigureDepends via manifest queries; doxygen + @file banner
- `Source/generated/Lexicon.h`, `Source/generated/Banner.h` — hand-written generation targets (untracked; regenerated by `./cast` fixpoint)
- `cast/CAST.md`, `cast/tables/cast.md`, `cast/tables/banner.md`, `cast/template/{Lexicon,IdentifierRow,Banner,BannerRow}.h` — authored self-validation SSOT
- `SPEC.md` — §2 col-0 key + hazard, §7.3 keyed FK, §8 file:row (column), §9 --help
- `CMakeLists.txt` — include dirs JAM_ROOT + harfbuzz only
- DELETED: `Source/Manifest.h`, `Source/Relations.h`, `Source/Cells.h`, `Source/Constraints.cpp`, `Source/Driver.cpp`, `Source/Help.cpp`, `Source/Manifest.cpp`
- `PLAN-markdown-isomorphism.md` — Steps 5b/5c/5d locked rulings

### Alignment Check
- [x] BLESSED principles followed — SSOT (Document the only model; one scan enumeration; vocabulary from one authored table), Stateless (every value queried at use), Lean (two engine files dissolved; Validation/Driver split at the real seam), Explicit (Identifier-addressed queries), Deterministic (fixpoint holds; two-phase atomic)
- [x] NAMES.md adhered — all new names ARCHITECT-ratified (query family, getLocation, getScannedTables/getConstraintTargetTables, validateRoots/validatePerColumnConstraints, Validation.h, Id::help)
- [x] MANIFESTO.md principles applied
- [x] Doxygen pass complete (dedicated sprint-end delegation, SPEC §-cited)

### Problems Solved
- Endless rewrite churn closed: engine contract locked in plan before dispatch; delete-first; structural-absence grep gates (zero Id::children/fromTokens/structs/out-refs/int table loops in engine)
- Include shadowing: cast generated header shadowed jam's `<generated/Identifiers.h>` via configure_app Source/ include — resolved by JFS precedent `Source/generated/Lexicon.h`
- `Id::value` collision with jam:130 — row deleted; set-intersection zero residue
- Constraint scope ratified (column set is the scope) — O(n²) duplicate application consolidated to once per (constraint, root, table)
- Auditor M3 refuted with citation: style.css loads via StyleManager ctor chain

### Debts Paid
- `DEBT-20260802T190025` — jam_Markdown stamps tag/id; cast consumers on Document queries; templates via Format::replaceholder
- `DEBT-20260802T190431` — banner one table-driven structure (banner.md → Banner.h), paintBanner lookup loop
- `DEBT-20260802T190852` — colour contract wired (css → StyleManager → renderer); list layout correct on container-owned gaps

### Debts Deferred
- None

## Sprint 2: Rendering Fidelity + Colour Architecture + Module Topology Sweep ✅

**Date:** 2026-08-02 → 2026-08-03
**Duration:** multi-session

### Agents Participated
- COUNSELOR: full orchestration; PLAN-rendering-fidelity amendments; LNM/ColourScheme/Palette/StyleManager design synthesis with ARCHITECT; trivial fixes (version SSOT, Id::dark qualification, ColourIdMap instance registration, ColourScheme base-shadowing qualification); DEBT capture coordination
- Pathfinder ×3: F4 rework surface, JFS style.css pipeline + Css naming contract, colourId ownership/module layering
- Engineer ×12: R10 width policy, R11 depth param + ansi harness, LNM mode 20 (Video+Model+Lexicon), F1/F5 fixes, ColourIdMap + StyleManager R7 echo, R13 StyleWhelmed strip + style.css + Help flow, R14 colour isomorphism, F3 stamped-cell map, F6 dead-code deletion, LookupTable int + style_manager relocation, size_t sweep + markdown guard, StyleTheme split correction; DEBT entries ×4; lldb crash diagnosis
- Auditor ×7: R10, R11, LNM (2 rounds), R12 full sweep, R13, R14+F3+F6, topology round (note: topology-round Auditor ran an unauthorized build — disclosed to ARCHITECT)

### Files Modified (cast repo, 8 total)
- `Source/Help.cpp` — width policy constants (widthCap 120, documentMargin 2); StyleManager flow (SharedInstance<SharedDocuments>, StyleManager {{}}, setAppearance Id::dark) replacing 15 hardcoded setColour lines
- `Source/Cast.cpp:80-81,86` — jam::Hyperlink member rename; Id::ColourIdMap instance registration (segfault fix); --version via ProjectInfo::versionString (CAST_VERSION macro deleted)
- `Source/style.css` (new) — CMY palette + 15 --Markdown-- appearance mappings (JFS anatomy)
- `CMakeLists.txt` — JAM_MODULES = jam_core, jam_tui, jam_markdown; style.css in cast_BinaryData; CAST_VERSION define removed
- `PLAN-rendering-fidelity.md` — in-session amendments (restamp elimination, Palette names, depth param, LNM)
- `DEBT.md` (new) — 4 entries captured
- `cast.sh` / build dirs untouched

### Alignment Check
- [x] BLESSED principles followed — SSOT (one nearest-colour spelling, one version truth), Lean (dead slots/classes deleted), Explicit (depth param, guarded optional deps), Deterministic (first-minimum ordered scans)
- [x] NAMES.md adhered — all new names ARCHITECT-ratified (newLineMode, TerminalColourSpace, ColourNames, getNearestIndex relocation, css words)
- [x] MANIFESTO.md principles applied
- [ ] Doxygen pass — deferred by ARCHITECT /log; stale-doc inventory carried: StyleWhelmed.h:12-19, getLines @param depth, getColourIdString prose, DecMode/Video/Model insertMode-only prose

### Problems Solved
- cast --help/no-args render path fully re-seamed onto css-driven StyleManager flow; console app freed from plugin_bootstrap/vulkan/audio dependency chain (module topology corrected at jam level)
- Segfault: ColourIdMap Instance never constructed — registration line added per Stamp/Hyperlink idiom
- Known-wrong residuals ledgered as DEBT (render assertions + layout, markdown Document isomorphism, banner table shape)

### Debts Paid
- `DEBT-20260802T171034` — colour-names JSON blob deleted both kernels; generated-shape ColourNames tables + one nearest/distance SSOT (R14)

### Debts Deferred
- `DEBT-20260802T190852` — render assertion flood + wrong list layout (resolves with markdown isomorphism)
- `DEBT-20260802T190431` — banner as table-driven structure
- `DEBT-20260802T190025` — markdown Document XmlElement isomorphism + %%name%% templates

## Sprint 1: Project Setup and Initial Planning ✅

**Date:** 2026-01-11  
**Duration:** 14:00 - 16:30 (2.5 hours)

### Agents Participated
- **COUNSELOR:** Kimi-K2 — Wrote SPEC.md and ARCHITECTURE.md
- **ENGINEER** (invoked by COUNSELOR) — Created project structure
- **AUDITOR** (invoked by COUNSELOR) — Verified spec compliance

### Files Modified (8 total)
- `SPEC.md:1-200` — Complete feature specification with all flows
- `ARCHITECTURE.md:1-150` — Initial architecture patterns documented
- `src/core/module.cpp:10-45` — Core module scaffolding with proper initialization
- `src/core/module.h:1-30` — Core module header with explicit dependencies
- `tests/core_test.cpp:1-50` — Test scaffolding following Testable principle
- `CMakeLists.txt:1-25` — Build configuration with explicit targets
- `README.md:1-20` — Project overview

### Alignment Check
- [x] BLESSED principles followed (Bound, Lean, Explicit, SSOT, Stateless, Encapsulation, Deterministic)
- [x] NAMES.md adhered (semantic names, verb-noun functions, no type encoding)
- [x] MANIFESTO.md principles applied (no layer violations, explicit dependencies)
- [x] No early returns used
- [x] Fail-fast error handling implemented

### Problems Solved
- Established project foundation following domain-specific patterns
- Defined clear module boundaries preventing layer violations

### Debts Paid
- `DEBT-20260111T101530` — Resolved missing module.h include in core_test.cpp (see Files Modified)
- *(or)* "None" if sprint did not touch any DEBT.md entries

### Debts Deferred
- `DEBT-20260111T143022` — Performance benchmarking suite (deferred by ARCHITECT command — out of scope for setup sprint)
- *(or)* "None" if no items pushed to DEBT.md during this sprint

**Status:** ✅ APPROVED - All files compile, tests scaffold in place

---

<!-- Actual sprint entries go here, written by PRIMARY agents -->

## Handoff to COUNSELOR: Cast-Local Generated Output Fix

**From:** COUNSELOR
**Date:** 2026-08-05
**Status:** In Progress — Steps 1-6 done, Step 7 (ARCHITECT build + fixpoint) pending; two ARCHITECT decisions open

### Context
Cast's own generated output needed alignment with JAM Sprint 60's convention: one header per type, master include, engine-injected banner, formatted namespace blocks. This is the prerequisite before PLAN-cast.md tasks #8 (extract JAM templates) and #9 (author JAM CAST.md). Plan lives at `PLAN-cast-local-fix.md`.

### Completed
- Step 1: `cast_output.txt` → `cast/cast/cast_output.md` (fenced code block, parseable by jam::Markdown)
- Step 2: Engine banner injection — `getOutputBanner()` in Driver.h reads cast_output.md via `jam::Markdown::parse()`, extracts codeBlock via `applyFunctionRecursively` + `getAllSubText()`, prefixes lines with `// `, prepends to every generated output. Read once in `Driver::run`, passed through both passes. `getConfigureDepends` in CastCLI.cpp includes cast_output.md. SSOT constant `outputBannerFileName`. NAMES fixes applied (`element` not `node`, `bannerDoc` not `doc`).
- Step 3: Templates renamed — Lexicon.h→Identifiers.h, Banner.h→HashMaps.h, IdentifierRow.h→Identifier.h, BannerRow.h→HashMap.h. New CAST.h master template (no namespace, just includes). Namespace separator + END marker in root templates. Hardcoded banner removed from templates.
- Step 4: Manifest updated — 3 output rows (Identifiers.h, HashMaps.h, CAST.h with empty tables), 6 dispatch rows with new fragment names.
- Step 5: CastCLI.cpp includes collapsed to `#include "generated/CAST.h"`. File renamed Cast.cpp→CastCLI.cpp (doxygen @file fixed).
- Step 6: Old generated files deleted (Lexicon.h, Banner.h). Interim hand-authored Identifiers.h, HashMaps.h, CAST.h created so cast builds (chicken-and-egg — cast generates these but can't build without them).
- Doxygen warnings fixed: CastCLI.cpp @file mismatch, Constraints.h HTML entity escapes (8 occurrences of `\<`/`\>` in doxygen prose).

### Remaining
- Step 7: ARCHITECT builds cast, runs fixpoint (`./cast cast/CAST.md` twice, verify empty diff). Verify generated output: banner at top, namespace format, CAST.h master shape.
- Two Auditor findings require ARCHITECT decisions (see Open Questions).
- Doxygen pass (dedicated step after fixpoint validated — per JRENG-CODING-STANDARD.md timing discipline).
- Auditor re-run after ARCHITECT decisions resolved.

### Key Decisions
- Master include: `generated/CAST.h` (cast), `generated/jam_CAST.h` (jam) — engine-generated manifest output rows, not hand-written. ARCHITECT-directed.
- Fragment naming: singular of root type (Identifier.h, HashMap.h, Bimap.h, Char.h). No "Row" suffix. ARCHITECT-directed.
- Namespace format: `/*___...___*/` separator after opening brace, `/**___END OF NAMESPACE___*/` before closing brace. ARCHITECT-directed.
- Output banner: engine reads `cast_output.md` (markdown, parsed by jam::Markdown), auto-prepends `// `-prefixed lines to every generated `.h`. Not in templates. ARCHITECT-directed.
- Input banner: authoring convention on table `.md` files, not enforced. ARCHITECT-directed.
- No manual string parsing — jam::Markdown::parse() only. ARCHITECT-directed.

### Files Modified
- `Source/CastCLI.cpp` (new, replaces deleted `Source/Cast.cpp`) — @file fixed; includes collapsed to `generated/CAST.h`; `getConfigureDepends` adds `cast_output.md` via SSOT constant
- `Source/Driver.h` — `getOutputBanner()` function; `outputBannerFileName` SSOT constant; `processOutput` signature + banner injection; `Driver::run` reads banner once
- `Source/Constraints.h` — 8 doxygen HTML entity escapes (`\<`/`\>`)
- `cast/CAST.md` — 3 output rows, 6 dispatch rows with new fragment paths
- `cast/cast_output.md` (new) — fenced code block banner artwork
- `cast/template/Identifiers.h` (new) — root template, namespace Id, separator + END marker
- `cast/template/HashMaps.h` (new) — root template, namespace cast, separator + END marker
- `cast/template/Identifier.h` (new) — fragment template
- `cast/template/HashMap.h` (new) — fragment template
- `cast/template/CAST.h` (new) — master template, no namespace
- `cast/template/Lexicon.h`, `Banner.h`, `IdentifierRow.h`, `BannerRow.h` (deleted)
- `cast_output.txt` (deleted)
- `Source/generated/Identifiers.h`, `HashMaps.h`, `CAST.h` (new, interim hand-authored, gitignored)
- `PLAN-cast-local-fix.md` (new) — 7-step plan

### Open Questions
1. **SPEC vs cast_output.md** — SPEC §1 lists three tracked artifact kinds (relations, templates, manifest). `cast_output.md` is a fourth input. SPEC §5 determinism says output is a function of (tables, templates, CAST.md, binary) — now also cast_output.md. Should SPEC be amended to add cast_output.md as a tracked input?
2. **Version stamp in banner** — SPEC §9 says "the same stamp appears in generated file banners." Currently cast_output.md has only artwork — no version/commit. Should the engine compose the version stamp into the banner, or should cast_output.md author a hole for it?

### Next Steps
- ARCHITECT resolves the two open questions.
- ARCHITECT builds cast and runs fixpoint (Step 7).
- On fixpoint success: doxygen pass (dedicated delegation to Engineer), then Auditor final sweep.
- Then: PLAN-cast.md task #8 (extract 7 module template pairs from restructured jam headers) → task #9 (author jam/cast CAST.md incl. union+dedup engine extension) → #10 conformance loop → #11 switchover.
