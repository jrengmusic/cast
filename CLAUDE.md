# CAST — Universal Headless Codegen

**Project ID:** cast
**Repository:** /Users/jreng/Documents/Poems/dev/cast
**Version:** 0.1.0
**Stack:** C++17, JUCE + JAM framework
**Type:** CLI — headless table-driven code generator

---

## Current State

**Last Sprint:** Address-Form Shapes + Positional Map Rung — CAST as straight string replacement ✅ (2026-08-31)

**Active Work:** None — jam/cast byte-identical fixpoint; EVE CMakeLists.txt generated + configures

**Active ODE:** None

**Active Debt:** None (DEBT.md is empty)

---

## Layer Order & Key Directories

### `Source/` — Core Engine & CLI

| File | Purpose | Key Responsibility |
|------|---------|-------------------|
| **main.cpp** | CLI entry point | Program dispatch, help/version output, `debug::Log::Scope` |
| **Processor.h** | Orchestrator | Owns Model + TemplateDocument + Writer; `generate()` (validate → write), `format()` (canonicalize origin .md files, parallel via Jobs) |
| **Model.h** | Master document | Manifest parse → parallel per-file table parse → splice into ONE MarkdownDocument; value/table/alias resolution |
| **Validator.h** | Manifest + table gates | Structure, source-count, address, format, index, hazard fatals — the only gate |
| **Writer.h** | Output rendering | One job per output-file group; write-if-different; failure collection |
| **Shapes.h** | Shape replacements | `:::list:::` expansion, bindings, depth indent, row merge law |
| **Items.h** | Item rendering | Source discovery (cells/@address/column/binding), padding law, column-aligned rendering |
| **TemplateDocument.h** | Template pool | One parsed document per `.cast` index row; shape lookup by `@alias:fence` address |
| **Transforms.h** | String transforms | Case/escape/comment framing per extension (`map::commentSyntax`) |
| **Jobs.h** | Thread pool | `Jobs::run (count, perIndex)` parallel dispatch |
| **Help.h** | --help rendering | Help text via StyleManager + markdown (HELP.md + style.css via BinaryData) |
| **generated/** | Codegen outputs | Bimaps.h, Files.h, Generated.h, HashMaps.h, Identifiers.h, Text.h — cast's own fixpoint outputs |
| **resources/** | cast-output.md | Banner artwork source (markdown fenced code) |

### `cast/` — Metadata, Data Tables & Templates

| File | Purpose |
|------|---------|
| **CAST.md** | Generation manifest: outputs, wiring rows (`list | separator | structure | file`); shapes addressed `@code:<fence>` into `../../jam/cast/code.cast` |
| **identifiers.md** | Identifier table → generated/Identifiers.h |
| **tokens.md, text.md, comments.md, files.md, banner.md** | Data tables (one table per generated concern) |

### Doxygen

| Location | Content |
|----------|---------|
| **docs/xml/** | Generated indices — cast headers + jam_core/jam_style/jam_terminal/jam_markdown |
| **docs/tagfile.xml** | Cross-reference tag file |

---

## Key Docs

| Document | Scope | Status |
|----------|-------|--------|
| **SPEC.md** | Normative feature spec (project root) — SPEC is normative; HELP.md is derived and carries no authority | ✅ Current |
| **Source/HELP.md** | Help text rendered by --help, derived from SPEC | ✅ Current |
| **carol/SPRINT-LOG.md** | Cross-session memory: sprint history + active handoff | ✅ Current |
| **DEBT.md** | Inter-sprint ledger | Empty |

---

## Build & Environment

**Build System:** CMake + Ninja (via JAM's BuildSetup)

**Key Modules:**
- `juce_core` — JUCE framework core
- `jam_core` (sole includer of jam_Generated.h) → `jam_style`, `jam_terminal`, `jam_markdown` (transitive)

**Runtime:**
- `./cast <manifest>` generates; `--format` runs by default, re-canonicalizing all origin .md files
- `cast_BinaryData` embeds `Source/HELP.md` + `Source/style.css`

**Doxygen:**
- Read doxygen XML before any C++ file search (use doxygen-protocol skill)
- Zero-warning policy; docs regenerated via JAM build process

**LSP:** Ignore false positives from JUCE module system

---

## Contract & Principles

**Governing Contracts (inherited from ~/.carol/):**
- CAROL.md — Role-based orchestration, ARCHITECT supremacy, evidence-only reasoning
- MANIFESTO.md (global) — BLESSED: Bound, Lean, Explicit, SSOT, Stateless, Encapsulation, Deterministic
- CODING.md (global) — fail-fast, assert on preconditions, no manual flags, debug::Log only
- NAMES.md (global) — semantic names, verb-noun functions, no type encoding

**Project-Specific Principles:**
1. **SSOT:** tables declare each identifier once; all engine outputs reference via Id:: (no magic strings)
2. **Engine Contract:** Constraint scope = column set; one enumeration pass per gate; no duplicate application
3. **Master State:** Parse all files in parallel → splice into ONE MarkdownDocument → validate the master → write-if-different (fixpoint-safe, diff-minimal)
4. **Vocabulary:** No precomputed name-lists — manifest rows are the addresses; validation entry points are `isValid` predicates returning juce::Result; no ad-hoc string parsing outside jam::MarkdownDocument::parse(); optimistic semantics in names
5. **Fragment Naming:** Singular shape names — no "Row" suffix
6. **Engine Reading:** jam::MarkdownDocument::parse() only; no manual line parsing; documents are immutable after construction — provenance (Id::path/Id::line) is stamped at parse, never after
7. **Invariant Ownership:** Validator establishes every invariant once, before Writer runs; downstream rendering trusts unconditionally — no re-checks (MANIFESTO D)

---

## Activation Notes

- Read SPEC.md before changes (determinism contract is non-negotiable; SPEC is normative, HELP.md derived)
- Generated files in Source/generated/ are cast's own fixpoint outputs — regenerate via `./cast cast/CAST.md`, never hand-edit
- Doxygen prose is written last (after implementation + audit completes; never during planning or code review)
- Use doxygen-protocol skill before any C++ file/symbol search
