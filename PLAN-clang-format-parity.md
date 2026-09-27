# PLAN: clang-format Parity — Stdout Format and `.cast-format`

**RFC:** RFC-clang-format-parity.md
**Date:** 2026-09-27
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + JAM (LANGUAGE.md C++/JUCE — L adapted)
**Status:** Locked — approved by ARCHITECT 2026-09-27

## Overview

`cast --format` gets the clang-format command line: stdin/stdout by default, `-i` in place,
`--style=file[:<path>]`, `--assume-filename=<path>`. Formatter configuration moves out of the
manifest into `.cast-format` / `_cast-format`, found by the clang-format rules. One style
resolution feeds the manifest run, the format run, and `--sync`.

## Context — facts read this session

- `## format` is read by the **reader**, not only the writer. `Model::getJoinedValue`
  (Model.h:1540-1546) joins a named column's rows by nothing, from `reflowWidths`
  (Model.h:1696). Thus every manifest run — `--no-format` included — needs the resolved
  style before `Model::parse` stamps values. SPEC §3.2:202-207 states the same law.
- Widths now: `Model::addReflowWidths` (Model.h:833-846) reads every `## format` table of the
  spliced Model, called at `Model::parse` (Model.h:870) — before `Validator::isFormat`
  runs (Processor.h:56, :87). A header without `width` dereferences a null cell
  (Model.h:839-844) before the gate reports it.
- `jam::MarkdownWriter` reads only `Id::rawText` and `Id::path` (jam_MarkdownWriter.cpp:20,
  :81, :700). Thus `getText (model, origin)` depends only on that origin's blocks plus the
  writer's `lineWrap` and column widths — invariant 7.1 holds when both sides build the
  writer from the same style.
- `Processor::format` (Processor.h:85-107) builds `jam::MarkdownWriter { lineWrap,
  model->getReflowWidths() }`; `writeOriginIfChanged` (Processor.h:201-216) gets and writes.
- Sync formats with `static const jam::MarkdownWriter formatter;` (Sync.h:874-880), called
  at Sync.h:1161 inside `getSyncOutcome`, reached by `run` (37) → `runGates` (98) →
  `runWalk` (185) → `runTransform` (203) → `getSyncResults` (279) → `getSyncOutcome` (1136).
- main.cpp: screen clear on a terminal (500-507); `cast: done` (389-393, 457-461); sync
  arity `argc == syncArgumentCount` (421, 483); lineWrap from `getFlagValue` with default
  `jam::MarkdownWriter::defaultLineWrap` (512); a missing manifest prints banner + help to
  stdout (411-419).
- JUCE: `File::loadFileAsString` → `InputStream::readEntireStreamAsString` →
  `MemoryOutputStream::toString` → `String::createStringFromData` (juce_File.cpp:568-576,
  juce_InputStream.cpp:241-246, juce_MemoryOutputStream.cpp:207-210). Stdin read into a
  `juce::MemoryOutputStream` and decoded by `toString()` gives byte parity with a file read.
- Existing names reused: `Id::style` (jam_Identifiers.h:1185), `Id::file` (:512),
  `Id::doubleDash` (:422), `Chars::dash` / `Chars::colon` / `Chars::equals`
  (jam_Chars.h:51, :49, :58), `Id::lineWrap` (identifiers.md:37), `text::Diagnostics::failNotFound`
  (text.md:15).
- `std::pair<juce::Result, …>` return is established: Sync.h:1179 (`juce::Result::ok()
  paired with every deleted path`).
- The five `## format` tables: cast/spell.md:1-7, jam/cast/spell.md:11-17,
  whatdbg/cast/spell.md:1-7, kuassa/user_modules/cast/spell.md:11-17 (`comment | 40` each);
  kuassa/jreng-filter-strip/project-info.md:3-11 (`comment | 60`, `value | 60`), declared by
  jfs `cast/spell.md:8` as `../project-info.md`.
- No data table has a column named `line-wrap` (RFC §6.1.5 check, grep over the five
  projects).
- LANGUAGE.md:34 — a single-responsibility C++ file stays one file above 300 lines. main.cpp
  (543 lines) keeps one responsibility: command-line parse and dispatch.

## Language / Framework Constraints

- LANGUAGE.md C++/JUCE L: header-only single-responsibility units stay single files.
- CODING.md: `not/and/or`; brace init; no out-parameters; no `==` on strings (use
  `compare (…) == 0` or `juce::Identifier` compare); `.at()`; no bail-out guards.
- Platform code (`_setmode`, `_fileno`, `_O_BINARY`) lives in main.cpp under `#ifdef _WIN32`,
  beside the existing `<io.h>` include (main.cpp:11-16); `<fcntl.h>` joins that block.

## Dependency & API Inventory

- **JAM `jam::MarkdownWriter`** (jam_MarkdownWriter.h:30-55): `MarkdownWriter (int lineWrap,
  jam::HashMap<juce::Identifier, int> columnWidth)`; `getText (document)`;
  `getText (document, origin)`. Used as is. No JAM change.
- **JAM `jam::MarkdownValidator`** (Processor.h:91-94): structural gate, used as is.
- **JAM `jam::MarkdownDocument::parse (text, origin)`** (Model.h:865): the one parser.
- **JUCE**: `juce::MemoryOutputStream` + `toString()` for stdin; `juce::File::getParentDirectory`,
  `getChildFile`, `existsAsFile`, `getRelativePathFrom` for the style search.
- **cast**: `Validator::isFormat` (Validator.h:1564-1591) validates the style file — its
  width loop already covers the `line-wrap` row (positive integer). No Validator code change.
  `Jobs::run` unchanged.

## Style Resolution (RFC §6.2, §6.4 — one path)

`getStyleFile (directory)` checks `.cast-format`, then `_cast-format`, in one directory
(clang-format order, observed — §Risks 1). `getNearestStyleFile (directory)` walks
`directory` and each parent through `getStyleFile`.

| Run                              | Style file, first match wins                                                   |
|----------------------------------|--------------------------------------------------------------------------------|
| `cast [<manifest>] …`            | `getNearestStyleFile (manifest directory)` — tier 2 is the walk's first step   |
| `cast --format -i <file> …`      | `--style=file:<path>`, else `getNearestStyleFile (file directory)`             |
| `cast --format <file> …`         | `--style=file:<path>`, else `getNearestStyleFile (file directory)`             |
| `cast --format` (stdin)          | `--style=file:<path>`, else `getNearestStyleFile (assume-filename dir or CWD)` |
| `cast --sync …`                  | `--style=file:<path>`, else `getStyleFile (<target-root>/cast)`, no walk       |
| none found                       | empty style: no column reflows, wrap 100                                       |

Several `--format` files resolve their style **per file** — ARCHITECT, 2026-09-27: "Per file"
(answer to the multi-file question). clang-format calls `getStyle` inside its per-file
`format()` (Librarian, ClangFormat.cpp release/20.x). One style per input file, never merged.
SPEC states "one style file per input file" in place of RFC §6.2's "per run".

An explicit `-` file argument reads stdin — clang-format `const bool IsSTDIN = FileName == "-";`
(Librarian, ClangFormat.cpp release/20.x; observed with clang-format 20.1.8); RFC §9
decision 4: *"make it EXACTLY IDENTICAL to clang-format"*.

Then, one sequence everywhere: `Model::parse (styleFile)` → `Validator::isFormat (style)` →
`reflowWidths = style.getReflowWidths()`, `lineWrap = getFlagValue (--line-wrap,
style.getLineWrap())` → `jam::MarkdownWriter formatter { lineWrap, reflowWidths }`. The
`## format` table outside the style file is ordinary data (RFC §6.3, decision 6).

## New Names (NAMES Rule -1 — ratified by plan approval)

| Where           | Name                                           | Value / role                                       |
|-----------------|------------------------------------------------|----------------------------------------------------|
| identifiers.md  | `@id inPlace`                                  | `` `i` `` — `-i` flag word                         |
| identifiers.md  | `@id assumeFilename`                           | `` `assume-filename` `` — flag word                |
| files.md        | `castFormat`                                   | `` `.cast-format` ``                               |
| files.md        | `castFormatAlternate`                          | `` `_cast-format` ``                               |
| files.md        | `castDirectory`                                | `` `cast` `` — sync style directory (RFC §6.4)     |
| files.md        | `standardInput`                                | `` `<stdin>` `` — stdin origin (RFC §5.3.8)        |
| text.md         | `failInPlaceInput`                             | `` `cannot use -i when reading from stdin` `` (clang-format 20.1.8 text, observed) |
| text.md         | `failFlagUnknown`                              | `` `unknown flag` ``                               |
| text.md         | `failStandardInput`                            | `` `standard input cannot be read` ``              |
| Model.h         | `getLineWrap()`                                | style `line-wrap` row width, else default 100      |
| main.cpp        | `getInPlaceFlag`, `getStyleFlag`, `getStyleFilePrefix`, `getAssumeFilenamePrefix` | flag texts, `getFormatFlag` family (main.cpp:43-77) |
| main.cpp        | `isInPlace`, `getFormatFiles`, `getStylePath`, `getAssumeFilename`, `getUnknownFlag` | format-line reads |
| main.cpp        | `getStyleFile`, `getNearestStyleFile`, `getStandardInputText` | style search, stdin read              |
| main.cpp        | `runFormat`, `runFormatInPlace`, `runFormatOutput` | format dispatch, beside `runDocument`/`runSync` |
| Processor.h     | `getText (formatter)`                          | manifest origin's canonical text, paired with the validation result |

**Names added in execution (NAMES Rule 5 families, no ratification needed; Auditor finding 17):**
`getAssumeFilenameFlag` (replaces `getAssumeFilenamePrefix` — juce::ArgumentList reads the value
after `=`, so the text is the flag word, `getXFlag` family main.cpp:55-89),
`getStyleFilePrefix` value `file:` (the `--style` value prefix, same reason),
`getStandardInputArgument` (`get` family), `runStyled` (`run` family beside `runDocument`),
`isFound` (Result-returning `is` predicate, CLAUDE.md principle 4), `syncRootCount` (replaces
`syncArgumentCount`), `getManifestOrigin` (Model `get` family, proven caller Processor::getText),
`getCodeSpanRow` / `getGlueRow` / `getCodeSpans` (siblings of `getOpenBlockRow`),
`absentFlagValue` (sibling of `invalidFlagValue`). Step 3.5 uses `getTableRow (*table,
Id::lineWrap)` in place of `getTableCell (…)`: jam keys the row `line-wrap`
(jam_MarkdownBlockParser.cpp:1730, juce_Identifier.cpp:82-83).

RFC §8.6 places the config names and `<stdin>` in identifiers.md. They go to files.md: files.md:12
is the file-name table (`files::cast`, `files::userModulesInfo`), NAMES Rule 5 nearest sibling.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md
(BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope
drift). @Auditor runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 0: Copy PLAN
**Scope:** `PLAN-clang-format-parity.md` (new, project root).
**Action:** COUNSELOR writes this locked plan to the project root.
**Validation:** content equals the approved plan.

### Step 1: SPEC.md
**Scope:** SPEC.md §2, §2.1, §2.2, §3.3, §6.11, §10.1.
**Action (COUNSELOR writes, ASD-STE100):**
1. §2 (line 66-67): name the one exception to "does not scan a directory and does not glob" —
   the fixed-name style search, two names per directory, not a glob.
2. §2.1: the grammar of RFC §5.1 verbatim; the flag table of §5.2; the behavior list of §5.3
   items 1-8; §5.4 changes; exclusion rule rewritten — `--format` excludes `--no-format`,
   `<directory>`, `--<word>`; in `--format` mode any other dash argument is `unknown flag`.
3. §2.2: sync style precedence (RFC §6.4); `--sync` "composes with nothing else" names
   `--style=file:<path>`.
4. §6.11 becomes the style-file section: names, content, `line-wrap` row, precedence
   (RFC §6.1-6.3), "one style file per input file, never merged"; `## format` outside the
   style file is ordinary data. §3.2/§3.3 references to `## format` point at the style file.
5. §10.1: rows for `-i` without a file, an unknown flag in `--format` mode, stdin that cannot
   be read, a `--format` file or `--style=file:<path>` that does not exist (the existing
   "manifest file does not exist" row widens); `## format` rows scoped to the style file;
   the sync arity row names the optional flag.
**Validation:** each sentence traces to an RFC §5/§6/§9 line or an ARCHITECT ruling; no rule
absent from them.

### Step 2: Data tables and regenerate
**Scope:** cast/identifiers.md, cast/files.md, cast/text.md; Source/generated/Identifiers.h,
Files.h, Text.h (regenerated, never hand-edited).
**Action (@Engineer):** add the rows of §New Names, sibling form pasted in the prompt
(identifiers.md:37, files.md:18-21, text.md:47-49). Regenerate with the installed binary:
`cast cast/spell.md --no-sign` (SPRINT-LOG:142).
**Validation:** generated headers carry exactly the new constants; no other diff.

### Step 3: Model.h — style in, reader reads it
**Scope:** Source/Model.h only.
**Action (@Engineer):**
1. Inner parse takes `(Model& document, const juce::String& text, const juce::String& origin,
   const juce::File& directory)`; the file overload loads the text and calls it.
2. New public `static std::unique_ptr<Model> parse (const juce::String& text,
   const juce::String& origin, const juce::File& directory,
   jam::HashMap<juce::Identifier, int> reflowWidths)` — sets `reflowWidths` at creation.
   `parse (const juce::File&)` keeps its signature (style file, sync info files) with empty
   widths; a missing file gives the empty Model (Model.h:42).
3. Delete `addReflowWidths` and its call (Model.h:833-846, :870).
4. `getReflowWidths()` becomes `jam::HashMap<juce::Identifier, int> getReflowWidths() const` —
   builds from this Model's `## format` rows, keyed as today
   (`jam::Format::toValidID (rawText)`, Model.h:843), skipping the row whose `name` is
   `Id::lineWrap` (Identifier compare). Called on the style Model only.
5. `int getLineWrap() const` — the `line-wrap` row's width via
   `getTableCell (*table, Id::width, Id::lineWrap)` (pattern Model.h:89), else
   `jam::MarkdownWriter::defaultLineWrap`.
6. `getJoinedValue` reads the member `reflowWidths` directly (Model.h:1542).
**Validation:** no `getTables (Id::format)` on a data Model; `reflowWidths` written only at
creation; no guard; names per §New Names only.

### Step 4: Processor.h — writer in, get split from write
**Scope:** Source/Processor.h only.
**Action (@Engineer):**
1. Constructor `Processor (const juce::String& text, const juce::String& origin,
   const juce::File& directory, jam::HashMap<juce::Identifier, int> reflowWidths)` →
   `Model::parse (text, origin, directory, std::move (reflowWidths))`.
2. `juce::Result format (const jam::MarkdownWriter& formatter)` — the `-i` path; drops the
   `isFormat` call and the local writer (Processor.h:87-90).
3. `std::pair<juce::Result, juce::String> getText (const jam::MarkdownWriter& formatter) const`
   — `{ validator.isValid (*model), formatter.getText (*model, manifestOrigin) }`; the stdout
   path. `writeOriginIfChanged` and `getText` both read `formatter.getText (*model, origin)`
   (jam_MarkdownWriter.h:55) — one get path (RFC §8.9, invariant 7.1).
4. `generate` drops the `isFormat` call (Processor.h:56-57).
**Validation:** Processor never reads `## format`; no out-parameter; no fallback.

### Step 5: Sync.h — writer threaded through
**Scope:** Source/Sync.h only.
**Action (@Engineer):** `run (sourceRoot, targetRoot, const jam::MarkdownWriter& formatter)`;
pass `formatter` through `runGates`, `runWalk`, `runTransform`, `getSyncResults` (lambda
capture list explicit), `getSyncOutcome`; `getCanonicalMarkdown (text, formatter)` drops the
`static const` writer (Sync.h:876).
**Validation:** no default writer left; capture lists explicit; no other behavior change.

### Step 6: main.cpp — format line, style resolution, sync, manifest run
**Scope:** Source/main.cpp only.
**Action (@Engineer):**
1. Flag texts: `getInPlaceFlag` (`-i`), `getStyleFlag` (`--style=file`),
   `getStyleFilePrefix` (`--style=file:`), `getAssumeFilenamePrefix` (`--assume-filename=`),
   built from `Chars::dash`, `Id::doubleDash`, `Id::style`, `Id::file`, `Chars::equals`,
   `Chars::colon`, `Id::inPlace`, `Id::assumeFilename`.
2. Format mode: `--format` at any position. Reads: `isInPlace`, `getStylePath`,
   `getAssumeFilename`, `getFormatFiles` (every argument that is not a flag, not the
   `--line-wrap` value; a lone `-` is a stdin input), `getUnknownFlag` (first dash argument
   outside the format flag set and not a lone `-`). Each input resolves its own style.
3. `getStyleFile` / `getNearestStyleFile` per §Style Resolution; `--style=file:<path>`
   resolves against CWD and must exist (`failNotFound`).
4. `getStandardInputText` — `_setmode (_fileno (stdin), _O_BINARY)` on Windows, `fread`
   into `juce::MemoryOutputStream`, `toString()`; `ferror` → `failStandardInput`.
5. `runFormatInPlace` — per file: `Processor { text, origin, directory, widths }`,
   `format (formatter)`; stdout empty.
6. `runFormatOutput` — per input (files, else stdin with origin = assume-filename's file name
   and its directory, else `files::standardInput` and CWD): `processor.getText (formatter)`;
   all texts collect first; on success `_setmode (_fileno (stdout), _O_BINARY)` on Windows
   and `fwrite` each text (pattern Processor.h:309); on any failure stdout stays empty.
7. `runFormat` dispatch: unknown flag → `failFlagUnknown`; `-i` with no file or with `-` →
   `failInPlaceInput`; missing file → `failNotFound` on stderr only (no banner, no help).
8. `main`: screen clear (500-507) skipped in format mode; no `cast: done` in format mode.
9. `runDocument`: resolves the style from the manifest directory, builds widths + writer,
   `Processor { manifest text, origin, directory, widths }`; `formatOnly` parameter removed.
10. `runSyncArguments`: accepts `argc == 4` or `argc == 5` with `--style=file:<path>`;
    builds the writer from the sync style; `Sync::run (source, target, formatter)`.
11. Diagnostic form unchanged: `cast: <message>` (main.cpp:398-400).
**Validation:** every flag text built from identifiers; no string `==`; stdout holds only
canonical text in format mode; no screen clear sequence; no guard without a named threat.

### Step 7: Build and self-host
**Scope:** build output; Source/generated/* (regenerated).
**Action (@Engineer):** `build.bat` (Windows clang-cl Release), install per prior sprints;
run `./cast cast/spell.md` twice and `cast --format -i cast/spell.md` twice — zero changes on
the second runs.
**Validation:** clean compile; fixpoint (SPRINT-LOG:1732 proof form).

### Step 8: Migration (RFC §6.3)
**Scope:** create `.cast-format` beside each manifest; remove the `## format` table from:
cast/cast/spell.md, jam/cast/spell.md, whatdbg/cast/spell.md,
kuassa/user_modules/cast/spell.md (tables → `<project>/cast/.cast-format`), and
kuassa/jreng-filter-strip/project-info.md (→ `jreng-filter-strip/cast/.cast-format`).
cast first (RFC §8.13).
**Action (@Engineer):** move the table rows verbatim; run `cast --format -i <manifest>` per
project.
**Validation:** each project formats to zero changes (RFC §11.11); regenerated outputs
byte-identical.

### Step 9: HELP.md, CLAUDE.md
**Scope:** Source/HELP.md (Command Line, Sync, `### format — column widths`), CLAUDE.md:83.
**Action (COUNSELOR):** derive from Step 1 SPEC text; `--format` instructions gain `-i`.
Rebuild so `cast --help` embeds it.
**Validation:** HELP states nothing SPEC does not.

### Step 10: Acceptance proofs (RFC §11)
**Scope:** session scratchpad only; no production repo write except Step 8.
**Action (@Engineer):** items 1-15 of RFC §11, each with the command and observed bytes
(hash compare), including the §4.2 fixture moved to `.cast-format`, CR-byte check (item 12),
terminal and pipe runs (item 13). Item 16 macOS: ARCHITECT builds and runs on macOS.
**Validation:** every item passes with evidence.

### Step 11: RFC §12.3 — crash on a wiring row with only an item line
ARCHITECT, 2026-09-27: *"@RFC-clang-format-parity.md 12 fold into plan"*.
**Scope:** the RFC §12.3 reproduction in the scratchpad; the fix in the owner file that the
root cause names (Validator.h when an existing §10.1 fatal covers the input).
**Action:**
1. @Engineer reproduces RFC §12.3 (`| - [list]: @d:d | - [list]: @t:line | @o |`, pipe table,
   no bare shape) with the Step 7 binary. Debug with whatdbg over DAP (whatdbg --help) or
   `debug::Log` only; 15-minute budget. Report the faulting frame, file:line.
2. COUNSELOR reads the call chain at that frame and the SPEC §10.1 row that covers the input
   (candidate: "output row declares no structure", §6.3). An existing row → the Validator
   establishes it once, before the Writer (SPEC §11.2). No row covers it → stop, one-line
   report to ARCHITECT (SPEC §1.1: no fatal outside §10.1).
3. @Engineer applies the fix; diagnostics removed in the same step.
**Validation:** the reproduction exits non-zero with `file:line (column): rule` on stderr and
writes no output; cast's own fixpoint unchanged.
**Finding (whatdbg, Debug build):** the fault is the jam parser, not a missing gate. It
reproduces only with the RFC's 3-cell row under the 4-column header; a 4-cell row runs clean.
`addTableRow` builds `logicalSpans` per row cell (jam_MarkdownBlockParser.cpp:1719-1723);
`addTableCells` loops the header's column count and calls `logicalSpans.at (column)` (:1748-1753)
→ `std::out_of_range` (jam_Array.h:354), uncaught → fast-fail 0xC0000409.
SPEC §3.2: "A pipe table keeps GFM" — GFM pads a short row with empty cells and ignores excess.
**Fix:** `addTableRow` builds exactly one span list per header column (empty for a missing cell)
— creation, jam `jam_MarkdownBlockParser.cpp` only. The padded row then meets the engine's
existing §10.1 gates.

### Step 12: RFC §12.1, §12.2 — HELP.md gaps
**Scope:** Source/HELP.md (and SPEC.md where SPEC lacks the rule HELP derives from).
**Action (COUNSELOR):**
1. §12.1: read the item-shape token read path (Shapes.h / Items.h binding discovery) and cite
   file:line for "an item shape takes its tokens from row columns; a named bullet binds the
   nearest bare shape". Write the sentence in HELP beside HELP.md:333 only as the code does it.
2. §12.2: read the plain-cell read path (Model.h `getAuthoredText`, :1492-1509, and the jam
   parse it reads) and cite file:line for markdown backslash escapes in a plain cell. Write
   the sentence in HELP beside HELP.md:103/:125 and, if absent, in SPEC §3.2.
**Validation:** each new sentence carries a program citation in the step record; HELP
states nothing SPEC does not.

### Step 13: Paragraph wrap breaks inside a code span
ARCHITECT, 2026-09-27: *"if it's true fix it, fold into plan"* (claim: the formatter breaks
a paragraph inside a code span, after a `/` or at a space; the span then renders with a space).
**Facts:** `MarkdownWriter::getParagraphText` reflows the paragraph source at UAX #14
opportunities (jam_MarkdownWriter.cpp:473-489) and glues back only block-opening rows
(`getOpenBlockRow`, :230-252, :481-485). No code-span guard exists on that path.
**Action:**
1. @Engineer reproduces with the Step 7 binary (scratchpad, `--line-wrap 30`).
2. When reproduced: @Engineer glues every segment that starts inside a code span of the
   paragraph source, through the existing `ReflowDocument::setGlue` + `setRows`
   (jam_ReflowDocument.h:66-79) — the pattern of :481-485. Scope: jam
   `jam_markdown/document/jam_MarkdownWriter.{h,cpp}` only. Rebuild cast; sync jam → KANJUT
   (`cast --sync`), scratchpad proof first (SPRINT-LOG decision 3, lossless-reflow).
**Validation:** no output line opens or closes inside a backtick span; `read (format (x))
== read (x)`; cast/jam fixpoints unchanged.

### Step 14: Two inline `:::[list]:::` feeds on one row
ARCHITECT, 2026-09-27: *"if it's true fix it, fold into plan"* (claim, unproven: with two
inline feeds on one wiring row, only the last expansion receives its column address,
HELP.md:322).
**Finding:** the claim as worded is false; the observed defect: an item shape fills every
`:::[list]:::` with one string — `getChildValue` joins every child column address
(Items.h:333-357) and `getItem` fills each `list` token with it. Fixture: `{ foobar = foobar },`.
SPEC §6.4:665-666 says slots consume sources in order. jam's lookup tables feed two column
addresses into ONE slot joined by `comma` (jam/cast/spell.md:1597-1605).
**Decision:** ARCHITECT, 2026-09-27: *"Per slot, keep 1-slot join"* — slot k takes the k-th
column address when the item shape has 2+ slots; a 1-slot item keeps the separator join.
**Action:** @Engineer changes Items.h at the owner (getChildValue / getItem); COUNSELOR states
both rules in SPEC §6.4 and HELP.md beside the column-address bullet.
**Validation:** the fixture renders `{ foo = bar },`; jam's lookup tables and every fixpoint
byte-identical.

### Step 15: Audit
@Auditor once, over Steps 1-14. Every finding resolved in-sprint (CAROL DCF §5).

### Step 16: Doxygen pass (post-audit, before /log)
ARCHITECT, 2026-09-27: *"dont forget comprehensive doxygen pass before you /log … double
check, probably there are also missing doxygen docs sweep in all files touched by couple of
sprints back"*.
**Scope:** every file touched this sprint (cast Source/main.cpp, Model.h, Processor.h, Sync.h,
Items.h, Validator.h; jam jam_markdown/document/jam_MarkdownWriter.{h,cpp},
jam_MarkdownBlockParser.cpp) plus every file the SPRINT-LOG "Files Modified" lists of the last
three logged sprints name (sync-unibreak-wrap, lossless-reflow, no-marks-reflow) in cast, jam
and KANJUT.
**Action:** @Engineer per repo: every public/private declaration in scope carries a doxygen
block in the header (CODING.md DOXYGEN DISCIPLINE: header only, @param matches the signature,
no stale names, escaped markup, no SPEC/PLAN references); regenerate doxygen per the
doxygen-protocol skill; zero warnings.
**Validation:** doxygen run log with zero warnings per repo; COUNSELOR spot-reads each changed
block against its code.

## BLESSED Alignment

- **B** — main owns the style Model and the writer for one run; Processor and Sync receive
  them by const reference, never store a pointer.
- **L** — each new function has one job; flag reads follow the `getXFlag`/`isX` family;
  main.cpp stays one file per LANGUAGE.md:34.
- **E** — every input visible in signatures (`formatter`, `reflowWidths`); no hidden default
  writer (Sync.h:876 removed); failures loud, stdout empty on failure.
- **S (SSOT)** — one style file per input file; `reflowWidths` built once, passed to both the
  reader (Model) and the writer; one get path (`MarkdownWriter::getText`).
- **S (Stateless)** — Processor and Sync hold no style state beyond the Model's creation input.
- **E (Encapsulation)** — Validator alone validates the style (Validator.h:1564), before any
  consumer reads it; the reader-before-gate null dereference (Model.h:839-844) disappears.
- **D** — same bytes from stdout, `-i`, the manifest run, and sync for one style (7.1, 7.6).

## Risks / Open Questions

1. Resolved: `.cast-format` wins over `_cast-format` in one directory. Observed with
   clang-format 20.1.8 (the formatting.lua:11 binary): `.clang-format` `IndentWidth: 8` +
   `_clang-format` `IndentWidth: 4` → indent 8; swapped → indent 4. `.clang-format` wins both
   ways. Step 1 and `getStyleFile` use this order.
2. Resolved: several `--format` files resolve per file (ARCHITECT: "Per file").
