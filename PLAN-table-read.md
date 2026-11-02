# PLAN: Table Read

**RFC:** RFC-table-read.md
**Date:** 2026-11-02
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + JAM (LANGUAGE.md C++/JUCE: one object, one translation unit)

## Overview

`cast --tables [<file> ...]` parses each markdown file with the cast reader and prints its tables as one JSON array on stdout — read-only: no format, no write, no `## toolchain` row. nvim later deletes its own parser (MACHINIST, outside this plan).

ARCHITECT's rulings, verbatim:

| Topic | Ruling |
|---|---|
| Flag | "--tables" |
| Output | "JSON streamed from AST" |
| Value form | "(a) datum, alias resolved" |
| Cross-file address cell | "Print as authored" |
| Fatals | "Format set + index gates (Recommended)" |
| JSON keys | "name for heading" — `file`, `tables`, `name`, `columns`, `rows` |
| Stdin, latency | "ASK ONLY QUESTION NO READ CANT ANSWER" — stdin follows the `--format` precedent (SPEC §2.1 rule 1; RFC Scaffold rule 2); no time budget given, none applies |

Two deviations, accepted with this plan's approval:

1. **"Id::text" is not stamped.** A cell that carries `Id::text` is read first by `MarkdownDocument::getRawText` — `if (const auto* rawText { element.find<juce::String> (Id::text) }) return *rawText;` (jam_MarkdownDocument.cpp:219-220). The formatter reads every cell through it (jam_MarkdownWriter.cpp:93, :724, :890), as do `Validator::isUniqueAlias` (Validator.cpp:832) and `isUniqueColumn` (Validator.cpp:794). A stamp rewrites every origin file on `cast <manifest>` (Processor.cpp:98). Value (a) instead comes from one Model function at print time; no new stamp, no new identifier. Its only reader is the print, once per cell.
2. **Withdrawn by the audit (finding 2).** Generation resolves an alias through `getValue` → the symbol's `Id::value`; the print resolves it to the symbol's authored datum. `Id::value` stays byte-identical to the pre-sprint engine. Original text: **`Id::value` of an `@` cell that names no declared alias** changes from empty (`Model::getValue` returns `{}`, Model.cpp:81) to its authored text, because addValue and the print share one function (MANIFESTO S: "Declare once"). A declared alias resolves as before. Manifest output bytes change only where such a cell is rendered.

## Language / Framework Constraints

- One object, one TU: new `JsonWriter` is `Source/JsonWriter.h` + `Source/JsonWriter.cpp`. `CONFIGURE_DEPENDS` glob picks it up (cmake.cast:121-123; project-info.md:146-147).
- Generated files are never hand-edited (CLAUDE.md Activation Notes). `Id::tables`, `Id::rows` exist only after ARCHITECT regenerates `Source/generated/`.
- No comments or doxygen until the post-audit pass (CAROL.md Code Hygiene).

## Dependency & API Inventory

- **jam_markdown**: `Model` inherits `jam::MarkdownDocument` (Model.h:20). Tables via `getTableRows (const Element&)` (jam_MarkdownDocument.h:169), `getTableHeaderRow` (:552, protected, exposed in Model.h:27), `isTable` (:110). Heading text is the heading block's `Id::id` (jam_MarkdownBlockParser.cpp:1804-1814). `Id::path` stamped on every root child (jam_MarkdownDocument.cpp:45); filter by origin as `MarkdownWriter::getText (document, origin)` does (jam_MarkdownWriter.cpp:26-27).
- **JUCE**: `juce::JSON::escapeString (StringRef)` (juce_JSON.h:218) — ASCII, `\uXXXX`, no quotes (juce_JSON.cpp:598-603). No `var` tree.
- **Chars** (jam_Core.h): `openBrace` 290, `closeBrace` 237, `openBracket` 291, `closeBracket` 238, `colon` 240, `comma` 241, `doubleQuote` 247, `newline` 285.
- **Id**: existing `file` (jam_Core.h:76), `name` (:108), `columns` (:55), `doubleDash` (Identifiers.h:52). New in cast/identifiers.md: `tables`, `rows` (ratified JSON keys; `tables` is also the flag word).
- **Model reused**: `getLiteral` (Model.cpp:887), `isFencedCell` (:904), `getJoinedValue` (:932), `getValue (row, alias)` (:85-89), `isAddress` (:117-120), `parse (text, origin, directory, reflowWidths)` (:14-22).
- **Validator reused**: `isIndex` (Validator.cpp:20-50; empty symbol, missing `.md`/`.cast`), `isUniqueAlias` (:822-845; duplicate alias), `hasTable` (:577-613; §4.4 address rule, alias test `model.getValue (row, sourceName).isNotEmpty()` at :585), `forEachEntry` (Validator.h:118-131; every non-wiring cell).
- **Processor sibling**: `getText (const jam::MarkdownWriter&)` (Processor.cpp:42-48) — `validator.isValid (*model)` (jam base table rules, the malformed-table set) then the writer.
- **main.cpp siblings**: flag getters (`getFormatFlag` :51), `isFormatOnly` (:276-279), `getUnknownFlag` (:598-615), `getFormatFiles` (:636-659), `isFound` (:716-722), `getFormattedText` (:1029-1060), `writeTexts` (:1068-1076), `runFormatOutput` (:1090-1122), `runFormat` (:1135-1156), `runCommandLine` (:1414-1444), `main` (:1458-1485).
- **Writer family name**: `PropertyListWriter`, `ZipWriter`, `BinaryWriter`, `IconWriter` → `JsonWriter` (NAMES Rule 5); method mirrors `MarkdownWriter::getText (document, origin)` (jam_MarkdownWriter.h:54).
- **Diagnostics reused, no new text row**: `failNotFound`, `failFlagUnknown`, `failStandardInput`, `failDuplicate`, `failOutputMissing`, `failTableMissing`, `failColumnUnknown` (cast/text.md).

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after Step 8, covering the whole sprint — never per step.

## Steps

### Step 1: SPEC.md (COUNSELOR)
**Scope:** SPEC.md §1, §2.1, §10.1.
**Action:** §1 list: add the `--tables` flag word and its output keys `file`, `tables`, `name`, `columns`, `rows`. §2.1: grammar line `cast --tables [--style=file:<path>] [--assume-filename=<path>] [<file> ...]`; a `--tables` paragraph with the RFC Scaffold rules 1–8, the output shape (one JSON array, LF-terminated, argument order; element `{"file", "tables": [{"name", "columns", "rows": [{column: value}]}]}`; `file` = the argument as given, or the stdin file name of §2.1 rule 8), the value form (the cell's datum: no `toLiteral`, no `format` operation; a leading `@alias` declared in the file's own `## index` resolved; any other address printed as authored), `## index` printed as a table, authored order kept, duplicate headings kept. §10.1: extend the row "the manifest file, a `--format` file argument…" with "a `--tables` file argument"; extend "a dash argument on a `--format` line…" to "on a `--format` or `--tables` line"; "malformed table, during formatting only" → "during formatting and `--tables` only".
**Validation:** Each new clause traces to a ruling quoted above or an RFC Scaffold rule; ASD-STE100; no other section changes.

### Step 2: HELP.md (COUNSELOR)
**Scope:** Source/HELP.md Command Line (13-35) and a new `### --tables` section after `--format` (37-55).
**Action:** Derived from Step 1, in the shape of the `--format` section.
**Validation:** Text matches SPEC Step 1; no claim SPEC does not make.

### Step 3: Data tables (Engineer)
**Scope:** cast/identifiers.md, cast/text.md.
**Action:** identifiers.md: rows `| @id | tables | `tables` | --tables CLI flag word and JSON output key. |` and `| @id | rows | `rows` | --tables JSON output key. |` in sorted position. text.md `failFlagUnknown` description → "A dash argument on a --format or --tables line that is not a flag word of that line."
**Validation:** Sorted order, existing column shape (identifiers.md:20-22), no other row changes.

### Step 4: Model — one value-(a) function (Engineer)
**Scope:** Source/Model.h, Source/Model.cpp.
**Action:** Replace the private 5-argument `getAuthoredText` (Model.h:1002, Model.cpp:913-930) with public `juce::String getAuthoredText (const Element& headerCell, const Element& row, const Element& cell) const`: literal and not prose → `literal->getAllSubText()`; prose → `getRawText (cell, getSource())`; otherwise `cell.getAllSubText()`; then `getJoinedValue`; then, when `literal == nullptr`, column not description/brief, `headerCell.id != Id::alias`, and `isAddress (text)` → `getValue (row, text)` when not empty, else the text (the declared-alias test of Validator.cpp:585). Prose test (Model.cpp:950-953) becomes a private `static bool isDescriptionProse (const Element& headerCell, const Element& cell)`. `addValue`: format cell (944-946), `getLiteral`, `value = getAuthoredText (headerCell, row, cell)`; `Transforms::getTransformed` when the transform is known, except for a literal cell whose description is prose; `jam::Format::toLiteral` when literal and not prose and not a description column; stamp `Id::value`. Every case except Overview deviation 2 is byte-identical to Model.cpp:913-964 (join is a no-op on a literal: the parser removes code-span newlines in a cell, jam_MarkdownInlineParser.cpp:1339-1340; a fence is excluded by `isFencedCell`).
**Validation:** Read the new bodies against Model.cpp:913-964 case by case (literal, prose, plain, declared alias, undeclared address); no bail-out guards; only Step-4 names.

### Step 5: Validator — read-mode entry point (Engineer)
**Scope:** Source/Validator.h, Source/Validator.cpp.
**Action:** Overload `static juce::Result isValid (const Model& model)` (CLAUDE.md principle 4): `isIndex`, then `isUniqueAlias`, then `forEachEntry` — for each cell whose raw text `Model::isAddress` and whose column is not `description`, `brief`, or `alias`, `hasTable (model, table, row, column, rawText)`.
**Validation:** Existing functions only; `isManifest` unchanged; existing text rows only.

### Step 6: JsonWriter (Engineer)
**Scope:** new Source/JsonWriter.h, Source/JsonWriter.cpp.
**Action:** `struct JsonWriter` with `juce::String getText (const Model& model, const juce::String& origin, const juce::String& file) const`, no data members. Streams `{"file":…,"tables":[…]}`: walk root children in order, keep those whose `Id::path` equals `origin`, track the last heading element; for each table: `name` = that heading's `Id::id` (empty when none), `columns` = header cells' `getAllSubText()` in order, `rows` = for each `getTableRows` row, an object of column name → `model.getAuthoredText (headerCell, row, cell)`. Every string through `juce::JSON::escapeString` inside `Chars::doubleQuote`; punctuation from `Chars`; keys from `Id::file`, `Id::tables`, `Id::name`, `Id::columns`, `Id::rows`. Single line, no `var`, no container.
**Validation:** No value-carrying struct (capability, CODING.md fake-carrier test); no allocation beyond the output string; keys from `Id::` only; no magic characters.

### Step 7: Processor (Engineer)
**Scope:** Source/Processor.h, Source/Processor.cpp.
**Action:** `std::pair<juce::Result, juce::String> getText (const JsonWriter& writer, const juce::String& file) const` beside `getText (const jam::MarkdownWriter&)`: `validator.isValid (*model)` (jam base table rules), then `Validator::isValid (*model)`, then `{ ok, writer.getText (*model, model->getManifestOrigin(), file) }` — the result-return shape of Processor.cpp:42-48.
**Validation:** Mirrors its sibling; no format, write, or toolchain call; nothing else changes.

### Step 8: main.cpp dispatch (Engineer)
**Scope:** Source/main.cpp.
**Action:** `getTablesFlag()` = `Id::doubleDash + Id::tables.toString()` (shape of :51). `isTablesFlag (argc, argv)` = `juce::ArgumentList { argc, argv }.containsOption (getTablesFlag())` (shape of :276-279). `getUnknownFlag` takes `const jam::Strings& flags` in place of its function-local set (:600); `runFormat` passes the current four; `runTables` passes `{ getTablesFlag(), getStandardInputArgument() }`. `getFormattedText` → `template <typename Function> getProcessedText (…, Function&& getText)` receiving `(const Processor&, const jam::MarkdownWriter&)`, body otherwise unchanged; `runFormatOutput` passes `processor.getText (formatter)`; `runTables` passes `processor.getText (JsonWriter {}, file)` with `file` = `input`, or `origin` for stdin. `runTables (argc, argv, lineWrap)` in the shape of `runFormat` + `runFormatOutput`: unknown flag → `isFound` files and style → each input in argument order, first failure `printError`, return 1, stdout empty → success `writeTexts ({ openBracket + texts joined by comma + closeBracket + newline })`, return 0. `runCommandLine`: `if (isTablesFlag) return runTables (...)` after `--help`, before `--format`. `main`: screen-clear condition (:1466) adds `and not isTablesFlag (argc, argv)`.
**Validation:** Flow matches `runFormatOutput`; no `==` on strings; `not`/`and`/`or`; brace init; no names beyond this list; `--tables --format` is fatal as an unknown flag.

### Step 9: Audit, then doxygen pass
**Scope:** every file of Steps 3–8.
**Action:** @Auditor sweeps once; COUNSELOR validates each finding against a CONTRACT clause and resolves it. Then @Pathfinder writes doxygen for each new or changed declaration (Model `getAuthoredText`, `isDescriptionProse`, `addValue`; Validator `isValid (const Model&)`; JsonWriter; Processor `getText (const JsonWriter&, …)`; main.cpp `getTablesFlag`, `isTablesFlag`, `getUnknownFlag`, `getProcessedText`, `runTables`); COUNSELOR reads each block against the code.
**Validation:** Each block states only what the cited code does (CODING.md COMMENTS).

## BLESSED Alignment

- **B:** No new owner; JsonWriter is stateless; the Model lives for one input as today (getFormattedText pattern).
- **L:** Each new function ≤30 lines; dispatch reuses existing helpers.
- **E:** Fatals on stderr, non-zero exit, empty stdout; named keys and characters; no bail-out guards.
- **S (SSOT):** One value-(a) function feeds `Id::value` and the print; validation reuses existing gates; one reader for every caller.
- **S (Stateless):** The print reads the AST in place — no `var` tree, no container (§11.1).
- **E (Encapsulation):** The validator alone decides validity (§11.2); JsonWriter reads through the Model API.
- **D:** Same file, same bytes; no host state; `file` is the caller's own argument.

## Risks / Open Questions

- None open. The two Overview deviations are accepted with approval.
- Regeneration of `Source/generated/` and the build are ARCHITECT's: `Id::tables`, `Id::rows` compile only after regeneration; a byte-identical regeneration of every project manifest confirms Step 4.
