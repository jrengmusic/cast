# RFC — Table read: cast prints the tables of any markdown file for its callers
Date: 2026-11-02
Status: Ready for COUNSELOR handoff
Author: MACHINIST
Targets: `SPEC.md` §1, §2.1, §10.1; `Source/HELP.md`; `Source/main.cpp` dispatch; the engine read path

## Problem Statement

Other tools read cast-format markdown tables with their own copies of the cast reader. Each copy can drift from cast.

1. nvim finds a project by the file `project-info.md` (`~/.config/nvim/lua/core/project/cast.lua:19`, `~/.config/nvim/lua/core/project.lua:35`).
2. nvim parses markdown tables with `~/.config/nvim/lua/core/markdown-table.lua`. That file is a second parser for the cast table grammar. Its own comment states that it "Mirrors jam_MarkdownBlockParser.cpp's splitTableRow() pipe-parity logic" (`markdown-table.lua:13-14`).
3. nvim also copies part of the cast reference law:
   - `@alias` resolution, one level only (`cast.lua:110-112`).
   - Backtick removal on a code-span cell (`cast.lua:97-99`). Its comment states that it gives "what jam::MarkdownDocument::getTableValues() yields, and what CAST renders for it".
4. nvim reads two kinds of file with that parser:
   - the cast project data file `project-info.md` (`cast.lua:317-327`);
   - the scout benchmark manifests `<project>/benchmark/*.md` (`cast.lua:151-169`).
5. The copy already broke once. Commit `f17b22b` in `~/.config` fixed the `## toolchain` reader for the `cmake -P gh.cmake` row.

ARCHITECT's direction, verbatim, in order:

> "instead of copying jam md doc, what if we give features to cast to return whatever value vim need? so it only need to run cast with args"

> "i think 2 is better. so it didn't pollute project with so many table-template to generate. write RFC to cast repo, i will delegate counselor to update the engine. after that i will back to you and clean up vim md parsing"

> "why not the design of using cast to those manifest is as parser-reader to output whatever caller need to read? so it's not hardcoded to read cast or scout manfest? but to read whatever on table and get its value"

The objective: cast gets a read-only command line mode. The mode takes any markdown file, parses it with the cast reader, and prints its tables and their values on stdout. It is not bound to a cast manifest or to a scout manifest. A caller (nvim first) runs cast and decodes the output, and deletes its own markdown parser.

This RFC replaces `RFC-manifest-query.md` of the same session. ARCHITECT approved the rename. The earlier draft bound the mode to `cast <manifest>`. The third quote above moved the design to any file.

## Research Summary

### What nvim reads today

From `project-info.md`, `cast.lua` `M.build` (`cast.lua:347-381`) reads these tables:

| Table | Columns read | Values used | Lua source |
|---|---|---|---|
| `## index` | `alias`, `symbol` | alias map | `cast.lua:354` |
| `## project info` | `name`, `value` | `projectName`, `versionString` | `cast.lua:355`, `:369` |
| `## cmake` | `key`, `value` | `jucePath`, `userModulePath`, `targetName`, `productName`, `juceTargetFunction`, `bundleIdentifier` (required, `cast.lua:29`) | `cast.lua:329-335` |
| `## toolchain` | `argument`, `command`, `flag` | each `cmake` row with `-B <dir>` and `CMAKE_BUILD_TYPE=<type>` | `cast.lua:127-149` |
| `## plugin format` | `value` | format list. When the table is absent, the project is an application. | `cast.lua:171-180` |
| `## juce module` | `value` | module list | `cast.lua:372` |
| `## user module` | `root`, `name` | module root `<root>/<name>` | `cast.lua:220-226` |

From each `benchmark/*.md`, `cast.lua:151-169` reads the `## toolchain` table, for the same `cmake -B` rows.

The required tables are `index`, `project info`, `cmake`, and `toolchain` (`cast.lua:28`). `## plugin format` is optional:
- `dev/end/project-info.md` has no `## plugin format` heading.
- `kuassa/jreng-filter-strip/project-info.md:187` has one.

`dev/eve/benchmark/burn.md` has the tables `## run` (line 1), `## profiler` (line 17), and `## toolchain` (line 27), and no `## index`.

### Cell forms in the real data

Observed in `dev/end/project-info.md`:

- An alias cell: `## cmake` `userModulePath` = `@user-module`. The file's own `## index` maps it to `${CMAKE_CURRENT_SOURCE_DIR}/../jam`. `## user module` `root` = `@user-module` on each row.
- A backticked cell: `## cmake` `bundleIdentifier` = `` `com.JRENG.END` ``, and `deploymentTarget` = `` `11.0` ``.
- A CMake variable inside a plain cell: `## cmake` `jucePath` = `${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE`.
- A `format` column with an operation: `## project info` `projectName` row, `format` = `toLiteral`.

No cell in `dev/end/project-info.md`, `kuassa/jreng-filter-strip/project-info.md`, or `dev/eve/benchmark/burn.md` holds a cross-file address (`@alias : table`). Command: `grep -c "| *@[A-Za-z-]* *: *[a-z]"` over the three files. Result: 0, 0, 0.

### SPEC clauses that bind the design

- §1: "CAST reads tables, iterates rows, and replaces tokens in templates. CAST has no knowledge of a target language." The same section lists every name that the engine hardcodes, and states "Every other name in every file is data." A new flag word joins that list, as the `--pack` and `--format` flag words do.
- §2.1: "The command line selects what runs. The command line never carries a generation rule."
- §2.1, the `--format` line: it is the precedent for a mode that reads any markdown file with no manifest.
  - Rule 1: "No file argument: the engine reads stdin and writes the canonical text to stdout."
  - Rule 2: "File arguments, no `-i`: the engine writes the canonical text of each file to stdout, in argument order. The files on disk do not change."
  - Rule 7: "Success: exit 0. Stdout holds only the canonical text."
  - Rule 8: "Failure: a non-zero exit. Stdout is empty. Stderr holds the diagnostic (§10)."
  - "Stdout carries LF line ends only, on every host. Input and output are UTF-8."
- §3.1 Row Law and §3.2 Literals: the reader rules that nvim's copy reproduces in part.
- §4.1: "`## index` is reserved, optional, and file-local". "A cell with the `@` sigil resolves against the index of the file that contains the cell. It never resolves against the index of a different file." A single file therefore resolves its own alias cells with no manifest.
- §4.2: an address `@file : table : column` resolves its first part against the writing file's index.
- §4.4: fatal — "an address's first part naming neither an index alias nor a table of the writing file", "two index rows declaring the same alias", "an index symbol naming a file that does not exist", "an address naming a table or column that does not exist".
- §5.1: a backticked cell's value is "`toLiteral` applied to the span's bytes". §9: "§9's guarantee covers text that CAST wraps into a string literal."
- §6.9: `## toolchain` is a reserved table name for a manifest run. A read mode runs no row, so the table is only data to it.
- §10: "The tables, the templates, the manifest, and the binary determine the output bytes. No timestamps, no paths, no host state."
- §11.1: "The parsed markdown is a complete AST … Nothing derived from it is copied into a second structure."

### Engine facts

- `CLAUDE.md` (cast), Project-Specific Principle 6: "jam::MarkdownDocument::parse() only; no manual line parsing; documents are immutable after construction".
- `CLAUDE.md` (cast) gives the read path: `Model.h` does the "Manifest parse → parallel per-file table parse → splice into ONE MarkdownDocument; value/table/alias resolution". `Processor.h` owns `generate()` and `format()`.
- JUCE has a JSON writer: `JUCE/modules/juce_core/json/juce_JSON.h`. `juce_core` is a cast module (`CLAUDE.md`, Key Modules).
- nvim decodes JSON with its built-in `vim.json.decode`. `cast.lua:266` already uses it for `compile_commands.json`.

### What stays outside cast

These derivations in `cast.lua` are CMake or JUCE knowledge. SPEC §1 keeps them out of cast. They stay in nvim:

- `${VAR}` and `$ENV{NAME}` expansion (`cast.lua:116-121`).
- The build directory read from the `-B` argument of a `cmake` row (`cast.lua:128`).
- The JUCE artefact path layout for each format (`cast.lua:40-68`).

## Principles and Rationale

- **SSOT.** cast owns the table grammar (§3) and the reference law (§4). nvim's parser is a second source of the same truth (MANIFESTO S: "A second copy of any truth is a bug waiting to happen"). With this mode, cast is the one reader for every caller and every file. nvim keeps only the target knowledge that SPEC §1 excludes from cast.
- **Encapsulation.** A caller asks cast for data through cast's public interface, its command line. It does not reproduce the parse. The mode knows no caller and no file type. It reads tables, as ARCHITECT stated: "so it's not hardcoded to read cast or scout manfest? but to read whatever on table and get its value".
- **Explicit.** The mode follows the `--format` precedent: product on stdout, diagnostic on stderr, non-zero exit on failure, empty stdout on failure.
- **Deterministic.** The same file gives the same bytes on stdout (§10). The output carries no absolute path and no host state.
- **Lean.** No new table and no new template in any project. ARCHITECT: "so it didn't pollute project with so many table-template to generate." The engine gets one dispatch path and one stdout writer. The reader is the existing one.
- **Read-only.** The mode formats nothing, writes no file, and runs no `## toolchain` row.

Considered and rejected:

1. **A template that renders a data file for nvim.** ARCHITECT rejected it: "so it didn't pollute project with so many table-template to generate."
2. **A mode bound to `cast <manifest>`** (the earlier draft, `RFC-manifest-query.md`). ARCHITECT moved the design away from it with the third quote. It could not read the scout benchmark manifests, which no `spell.md` index declares. So nvim would keep `markdown-table.lua`.
3. **A per-table address query with the §4.2 law as-is**, for example `@project-info:plugin format`. §4.4 makes an absent table fatal. `dev/end` has no `## plugin format`, so a fixed query set fails on application projects. The caller would need to know which tables exist before it asks. The whole-file print below has no such failure. The caller picks tables by heading.
4. **Expansion of `${VAR}` in cast.** That is CMake knowledge. SPEC §1 forbids it.
5. **Values with `format` operations or `toLiteral` applied.** Those operations make target-language text (§8, §9). The mode wraps nothing into a string literal, so §9 does not apply. nvim needs the datum, `com.JRENG.END` and not `"com.JRENG.END"`. That is the form that `cast.lua:97-99` makes today. Open Question 3 holds the decision.

## Scaffold

### Command line

The grammar line joins §2.1. `<flag>` is the open name (Open Question 1).

```
cast --<flag> [<file> ...]
```

The rules follow from the `--format` precedent (§2.1) and §10. COUNSELOR states them in SPEC:

1. The line takes one or more markdown file paths. The engine resolves each path against the working directory, as `--format` file arguments resolve. A file path that is a symbolic link resolves to its target, as for `--format` (§2.1).
2. Stdin: no file argument, or the argument `-`, reads stdin, as `--format` rule 1 does. Open Question 5 decides whether the mode keeps this.
3. For each file, the engine parses with the cast reader (§3, `jam::MarkdownDocument::parse()`). It applies the §4.1 file-local index to the file's own cells.
4. The mode reads no manifest, formats nothing, writes no file, and runs no `## toolchain` row.
5. Success: exit 0. Stdout holds only the output, files in argument order.
6. Failure: a non-zero exit. Stdout is empty. Stderr holds the diagnostic in the `file:line (column): rule` form (§10).
7. `--<flag>` excludes `--format`, `--no-format`, a manifest argument, `<directory>`, `--<word>`, `--sync`, and `--pack`. Each combination is fatal (a new §10.1 row).
8. Stdout uses LF on every host, and UTF-8.

### Output content (any format)

For each file, in argument order:
- `file` — the path as the caller gave it on the line. It is the caller's own input, so §10 "no paths" holds: the mode adds no path the caller did not give.
- `tables` — every table of the file, in authored order.

For each table:
- `heading` — the table name, as authored (`cmake`, `plugin format`).
- `columns` — the column names, as authored, in authored order.
- `rows` — in authored order. Each row maps each column name to the cell value. Open Question 3 decides the value form.

`## index` is a table like the others in this output. A caller that resolves aliases itself (Open Question 3, option b) reads it from there.

Two tables in one file can have the same heading. The output keeps both, in authored order. The caller decides.

### Example — the read of the nvim consumer after the change

This is for MACHINIST's follow-up. It is not this RFC's scope.

| nvim need today | Source after the change |
|---|---|
| `markdown-table.lua` `getTables`, `getRecords`, `getColumn` | `cast --<flag> project-info.md benchmark/*.md`, decoded by `vim.json.decode` (if Open Question 2 picks JSON) |
| `cast.lua:97-99` backtick removal | cast (value form, Open Question 3) |
| `cast.lua:110-112` alias resolution | cast (Open Question 3) |
| `cast.lua:151-169` benchmark `## toolchain` | the same cast call |
| `cast.lua:116-121` `${VAR}` expansion | stays in nvim (SPEC §1) |
| `cast.lua:128` `-B` build directory | stays in nvim (SPEC §1) |
| `cast.lua:40-68` artefact layouts | stays in nvim (SPEC §1) |

Result: nvim deletes `markdown-table.lua` completely. `cast.lua:9` is its only `require` in `~/.config/nvim/lua`.

## BLESSED Compliance Checklist

- [x] Bounds — the mode owns no new state. It reads the document that the parse builds, and the run ends with the print.
- [x] Lean — no project gets a new table or template. The engine gets one dispatch path and one stdout writer.
- [x] Explicit — stdout product, stderr diagnostic, non-zero exit: the §2.1 `--format` precedent.
- [x] SSOT — one reader of the cast table grammar, for every caller and every file. nvim's copy is deleted in the follow-up.
- [x] Stateless — the run is a pure read of each parsed document.
- [x] Encapsulation — callers use the command line. The mode knows no caller and no file type.
- [ ] Deterministic — holds when Open Questions 2 and 3 fix one output form and one value form.

Flag for COUNSELOR: §11.1 forbids "a second structure" built from the AST. The output writer must serialize from the parsed document directly. If it builds a `juce::var` tree for `juce::JSON`, check that tree against §11.1. A transient tree that lives only for the print can count as a per-call container (§12 lists such containers as debt).

## Open Questions

1. **Flag name.** NAMES.md Rule -1: a new CLI word is ARCHITECT's decision. SPEC §1 adds the word to the hardcoded list. Settled by ARCHITECT.
2. **Output format.** JSON is one option: cast writes it with `juce::JSON`, and nvim decodes it with `vim.json.decode`, so nvim needs no parser. Any format that nvim must split by hand brings back a hand parser on the nvim side. Settled by ARCHITECT.
3. **Value form.** For each cell, print one of these:
   - (a) the datum: the plain text, the bytes inside a code span, or the fence content, with a leading `@alias` resolved against the file's own `## index` (§4.1), and no `format` operation;
   - (b) the datum with no alias resolution. The caller resolves from the printed `## index` table;
   - (c) the value that generation renders, with `toLiteral` and the `format` operation applied.

   The nvim consumer needs (a) or (b). Settled by ARCHITECT.
4. **Fatals and validation.** Which checks run in this mode? Candidates from §10.1 that apply to one file: a duplicate alias in one index (§4.4); an index symbol that names a file that does not exist (§4.4); an alias cell whose alias is absent from the file's index (§4.4); a malformed table (§3.3 lists it as fatal "during formatting only"). New rows: `--<flag>` combined with another mode; a file argument that does not exist (the `--format` row "a `--format` file argument … does not exist" is the model). Their wording is ARCHITECT's. Settled by ARCHITECT; COUNSELOR drafts.
5. **Stdin.** Does the mode read stdin when no file argument is given, as `--format` rule 1 does? nvim reads files from disk, so it does not need stdin. Settled by ARCHITECT.
6. **Cross-file address cells.** A cell can hold a §4.2 address to a table of another file (`@alias : table`). Its value is a table, not a scalar. The three test files hold none (Research Summary). Options: print the address text as authored; or make it fatal in this mode. Settled by ARCHITECT.
7. **Latency.** nvim runs the read when a project loads and when a watched source changes (`project.lua:154-179`). Does ARCHITECT set a time budget? If yes, COUNSELOR measures with `scout` (CAROL.md Profiling) on `dev/end/project-info.md` and `kuassa/jreng-filter-strip/project-info.md`. Settled by ARCHITECT.

## Handoff Notes

- Scope of this RFC: the cast engine, `SPEC.md`, and `Source/HELP.md` (derived from SPEC, §1). The nvim cleanup is a separate MACHINIST task after the engine change. ARCHITECT: "after that i will back to you and clean up vim md parsing".
- SPEC is normative. Update SPEC first, then HELP.md, then the engine (`CLAUDE.md`, Activation Notes).
- The mode reuses the existing reader. `CLAUDE.md` Principle 6 forbids a second line parser in cast too.
- The project marker stays `project-info.md` on the nvim side (`cast.lua:19`). nvim decides which files to pass. The engine needs no knowledge of them.
- Session context, for MACHINIST's follow-up and not for this RFC: in the same session, ARCHITECT also discussed a configure-only flow (a `compile_commands.json` without a build) and the shared `.clangd` file at the user-module root. Neither needs an engine change. A configure-only flow is a `## toolchain` row set with its own `argument` (§6.9).
- Test data:
  - `dev/end/project-info.md` — application, no `## plugin format`, alias cells, backticked cells.
  - `kuassa/jreng-filter-strip/project-info.md` — plugin, `## plugin format` at line 187.
  - `dev/whatdbg/project-info.md` — console application.
  - `dev/eve/benchmark/burn.md` — scout manifest, no `## index`.

  The nvim locator gives the expected values for the `project-info.md` files today. Run `nvim --headless` in each project root and read `.project`.
