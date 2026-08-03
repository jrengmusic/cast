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
