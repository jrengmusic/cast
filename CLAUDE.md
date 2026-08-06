# CAST — Universal Headless Codegen

**Project ID:** cast  
**Repository:** /Users/jreng/Documents/Poems/dev/cast  
**Version:** 0.1.0  
**Stack:** C++17, JUCE + JAM framework  
**Type:** CLI — headless table-driven code generator  

---

## Current State

**Last Sprint:** Sprint 5 — Eliminate Magic Strings ✅ (2026-08-04)

**Active Work:** Handoff — Cast-Local Generated Output Fix (PLAN-cast-local-fix.md)
- Steps 1–6 complete: engine banner injection, template renaming, manifest update, interim generated files
- Step 7 pending: ARCHITECT build + fixpoint validation; two decisions open (SPEC amendment, version stamp in banner)
- Status: Ready for implementation after ARCHITECT rulings

**Active ODE:** None

**Active Debt:** None (DEBT.md is empty)

---

## Layer Order & Key Directories

### `Source/` — Core Engine & CLI

| File | Purpose | Key Responsibility |
|------|---------|-------------------|
| **CastCLI.cpp** | CLI entry point + version | Program dispatch, help/version output |
| **Driver.h** | Generation driver | Two-phase atomic: parse tables, validate, write-if-different |
| **Constraints.h** | Predicate library | 8 column-level + table-level validation predicates |
| **Validation.h** | Manifest + table validation | Hazard detection, constraint application, manifest integrity |
| **Transforms.h** | String transformations | Case/escape/encoding conversions (9 transforms) |
| **Template.h** | Fragment substitution | @slot@ interpolation, template validation |
| **Help.h** | Help/--help rendering | CLI help text generation via StyleManager + markdown |
| **generated/** | Codegen outputs | Identifiers.h, HashMaps.h, CAST.h (master) — auto-generated, interim hand-written |

### `cast/` — Metadata & Templates

| File | Purpose |
|------|---------|
| **tables/cast.md** | Identifier table (30+ entries): chars, strings, IDs |
| **tables/banner.md** | Banner lookups (8 rows × 2 columns) |
| **CAST.md** | Generation manifest: 3 outputs, 6 dispatch rows, constraint: word unique |
| **template/{Identifiers,HashMaps}.h** | Root templates (namespace, separators, END marker) |
| **template/{Identifier,HashMap}.h** | Fragment templates (substitution targets) |
| **template/CAST.h** | Master include template (no namespace) |
| **cast_output.md** | Banner artwork source (markdown fenced code) |

### Doxygen

| Location | Content |
|----------|---------|
| **docs/xml/** | Generated indices — cast headers + jam_core/jam_style/jam_terminal/jam_markdown |
| **docs/tagfile.xml** | Cross-reference tag file |

---

## Key Docs

| Document | Scope | Status |
|----------|-------|--------|
| **Source/HELP.md** | Feature spec + help text (markdown + ASCII) — *renamed from SPEC.md, moved per git* | ✅ Current (Sprint 5) |
| **ARCHITECTURE.md** | *Not present* — topology documented inline in SPRINT-LOG.md handoffs | — |
| **PLAN-cast.md** | Active long-term plan (extracted from SPRINT-LOG context) | 🔄 Active |
| **PLAN-cast-local-fix.md** | 7-step plan: banner injection, template rename, manifest update, fixpoint | 🔄 In Progress (Step 7 pending) |
| **carol/SPRINT-LOG.md** | Cross-session memory: 5 sprints + active handoff | ✅ Current |

---

## Build & Environment

**Build System:** CMake + Ninja (via JAM's BuildSetup)

**Key Modules:**
- `juce_core` — JUCE framework core
- `jam_core` (sole includer of jam_Generated.h) → `jam_style`, `jam_terminal`, `jam_markdown` (transitive)

**Bootstrap State:**
- Hand-written interim `Source/generated/{Identifiers,HashMaps,CAST}.h` checked in for chicken-and-egg bootstrap
- Removed on first successful `./cast cast/CAST.md` fixpoint run
- `cast_BinaryData` embeds `Source/HELP.md` + `Source/style.css`

**Doxygen:**
- Read doxygen XML before any C++ file search (use doxygen-protocol skill)
- docs/xml/ contains generated indices (45 files); see docs/tagfile.xml for cross-reference
- Regeneration: check JAM build process (Doxyfile not at cast root)

**LSP:** Ignore false positives from JUCE module system

---

## Contract & Principles

**Governing Contracts (inherited from ~/.carol/):**
- CAROL.md — Role-based orchestration, ARCHITECT supremacy, evidence-only reasoning
- MANIFESTO.md (global) — BLESSED: Bound, Lean, Explicit, SSOT, Stateless, Encapsulation, Deterministic
- CODING.md (global) — fail-fast, assert on preconditions, no manual flags, debug::Log only
- NAMES.md (global) — semantic names, verb-noun functions, no type encoding

**Project-Specific Principles (per Sprint 5 & active handoff):**
1. **SSOT:** tables declare each identifier once; all engine outputs reference via Id:: (no magic strings)
2. **Engine Contract:** Constraint scope = column set; one enumeration pass per (constraint, root, table) tuple; no duplicate application
3. **Two-Phase Atomic:** Parse all roots → validate → write-if-different (fixpoint-safe, diff-minimal)
4. **Vocabulary:** Identifier-addressed queries only (getScannedTables, getConstraintTargetTables); no ad-hoc string parsing outside jam::Markdown::parse()
5. **Fragment Naming:** Singular (Identifier.h, HashMap.h, Char.h, Bimap.h) — no "Row" suffix
6. **Engine Reading:** jam::Markdown::parse() only; no manual line parsing or XML construction

---

## Activation Notes

- Read Source/HELP.md before changes (determinism contract is non-negotiable)
- PLAN-cast-local-fix.md is the active plan; Step 7 + two ARCHITECT decisions are open
- Interim generated files (Identifiers.h, HashMaps.h, CAST.h) exist for bootstrap; do not commit regenerated versions
- Doxygen prose is written last (after implementation + audit completes; never during planning or code review)
- Use doxygen-protocol skill before any C++ file/symbol search
