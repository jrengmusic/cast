# CAST — Codegen Annotated Source of Truth — Specification

Version: 1.0

This text is the contract for the `cast` binary. It is also the binary's help page:
`cast` run with no `CAST.md` found prints this spec verbatim.

---

## 1. Model

CAST is a dumb engine over three tracked artifact kinds — the single source of truth:

1. **Relations** — GFM tables in markdown files. Rows are tuples, columns are attributes.
2. **Templates** — opaque text with exactly one construct: `@hole@`.
3. **Manifest** — `CAST.md`, one per framework or product project. The manifest is the
   invocation: argv selects, never defines.

Generated files are untracked build artifacts. CAST carries no interpreter and no
domain knowledge. Engine semantics: **select → project → join**.

---

## 2. Relations (input tables)

- Any `## section` heading followed by a GFM table is a relation named by the heading.
- Rows are tuples; columns are attributes; shape is free per table.
- The first column is the row key: a lookup addressing a row by value matches against
  column 0. Key uniqueness is the author's contract for keyed access; enumeration
  (projection, whole-column predicates) is position-based and needs no key.
- Cells flatten to plain strings. Code spans (backticks) unwrap to their content.
- **Hazard rule (FATAL):** any cell containing `<`, `>`, or a URI scheme outside a code
  span is an error. Wrap such content in backticks.
- Numbers pass through as authored strings — never reparsed, never reformatted.
- Row order is authored order. The engine never sorts.

## 3. Templates

- Opaque bytes plus one construct: `@hole@`. No conditionals, no loops, no expressions.
- A hole is **scalar** (one cell or one manifest value) or **aggregate** (a table
  projected through fragment templates in authored row order). The manifest decides
  which; the template grammar never grows a second construct.
- Two tiers:
  - **Root** templates — exactly one per output file.
  - **Fragment** templates — per-row expansions that fill slots in roots. Fragments
    never produce files.
- Runtime logic (function bodies, framework calls) lives verbatim in template text.
  Templates never contain generate-time logic.

## 4. Manifest — CAST.md

Four table kinds:

- `## outputs` — maps output path ← root template ← input table list. One output ↔ one
  root template per invocation.
- `## dispatch` — maps (table, column, value or presence) → fragment template → slot.
  Value-keyed or presence-keyed template selection per row.
- `## transforms` — maps column → named transform from the closed vocabulary (§6).
- `## constraints` — maps column → predicate from the closed vocabulary (§7),
  including FK rows.

Errors (generation FATAL):
- Orphan template — a template file no manifest row references.
- Undeclared output — an output produced by no `## outputs` row.
- Unmapped fragment — a dispatch row naming a fragment that does not exist, or a
  fragment no dispatch row maps.

The configure-dependency list is derived from the manifest — never hand-maintained.

## 5. Determinism

- Output bytes are a total function of (tables, templates, CAST.md, binary).
- Fixed LF line endings. No timestamps, no absolute paths, no hostnames, no
  environment contribution.
- Write-if-different: an output file is rewritten only when its bytes change.
- Fixpoint: running twice yields an empty diff.

## 6. Transform vocabulary (closed)

1. `toUpper` — uppercase the value.
2. `toTitle` — titlecase display names.
3. `toKebab` — kebab-case.
4. `escapeCpp` — C string literal escaping: `"` → `\"`, `\` → `\\`, non-ASCII bytes → `\xNN`.
5. `utf8Bytes` — `U+XXXX` codepoint token(s) → UTF-8 `\xNN` byte escape sequence.
6. `codepointHex` — glyph or `0xNN` cell → `0xNNNN` integer literal.
7. `codepointLabel` — codepoint → zero-padded uppercase `U+XXXX` notation.
8. `qualifySymbol` — two-part `A::b` symbol → `juce::A::b`; three-plus parts verbatim.
9. `symbolFromFile` — filename with dots replaced by underscores (BinaryData symbol).

## 7. Predicate vocabulary (closed)

1. `matches <regex>` — cell matches the pattern.
2. `unique` — column uniqueness; scope is the declared column set (single table or
   shared registry across tables).
3. `existsIn <table>.<column>` — FK: cell value keys a row in the referenced table.
   `<column>` names the target table's key (column 0); resolution is keyed access.
4. `oneOf a|b|c` — cell is in the closed value set (empty cell permitted when listed).
5. `range` — numeric cell within the row's declared min/max columns.
6. `parity <table>.<column>` — key-set equality across tables (e.g. localisation
   languages).
7. `fileExists <root>` — cell resolves to an existing file under the declared root.
8. `onePerGroup <column>` — exactly one marked row per distinct group value.

## 8. Failure

Every failure is FATAL with a non-zero exit, before any output is written, naming
`file:row (column)` and the rule:

    float row 1 (preamp gain): default outside [min, max]

A failing configure run stops before any TU compiles.

## 9. CLI

- `cast` — find `CAST.md` in the current directory, regenerate all declared outputs.
- `cast <path>/CAST.md` — explicit manifest.
- `cast CAST.md <output>` — regenerate one declared output row (selection only —
  argv never defines behavior).
- `cast --version` — version + source commit; the same stamp appears in generated
  file banners.
- `cast --help` — banner + this specification, exit 0.
- No `CAST.md` found — print this specification and exit non-zero.

## 10. Integration contract

- Frameworks carry the exact per-platform `cast` binaries (tracked, macOS + Windows).
  No `find_program`, no version assert. Source SSOT: `~/Documents/Poems/dev/cast`.
- Invocation is `codegen.cmake`, first include before `project()`:
  host-platform dispatch to the carried binary, `execute_process (cast CAST.md)`,
  manifest-derived inputs registered as `CMAKE_CONFIGURE_DEPENDS`.
- CMake is a consumer. It never implements codegen.
- Generated output is header-only, grouped by construct kind. Root templates and
  manifest structure are framework-invariant.
