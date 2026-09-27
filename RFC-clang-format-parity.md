# RFC — clang-format Parity: Stdout Format and `.cast-format`

**Status:** Decided, for COUNSELOR
**Author:** MACHINIST
**Date:** 27 Sep 2026
**Requested by:** ARCHITECT

---

## 1. Objective

ARCHITECT's direction:

- *"cast is ours, i can make the auto-format exactly like clang-format so the wiring
  could be consistent, only swapping the cli."*
- *"probably best to have .cast-format as the config so it consistently clang-format,
  instead of writing on manifest"*
- *"we want exact parity with clang-format"*

The objective has four points:

1. **DX parity.** `cast --format` gives markdown the same developer experience as
   clang-format gives C++: the same command-line shape, the same flags, the same
   config-file model.
2. **Project aware, with no search for a manifest.** Cast does not glob and does not
   search for a manifest. The config-file search is the one exception, and it follows
   the clang-format rules (§6.2).
3. **A true markdown formatter.** `cast --format` formats any markdown file, like
   clang-format formats any source file. It is not only the formatter of cast's own
   manifest data. The manifest is the one exception: cast is also a toolchain, thus
   `cast --format -i <manifest>` also formats each `.md` file that the manifest's
   index declares (§5.3, item 4).
4. **Only usage changes.** The canonical text that cast writes does not change for a
   project whose widths move to `.cast-format`. The command line and the config
   location change. The one output change is `--sync`: it now applies the target
   project's config (§6.4), and thus matches the target's own format run. The purpose
   is to wire `cast --format` into nvim the same way as clang-format.

This RFC has two parts:

1. **Stdout format.** `cast --format <file>` writes the canonical text to stdout, like
   clang-format. The file on disk does not change. `-i` writes in place.
2. **`.cast-format`.** The formatter configuration moves out of the manifest into a
   dedicated config file, like `.clang-format`. Cast finds it by the clang-format
   rules.

With both parts, the nvim markdown auto-format uses the same wiring as the C++
auto-format. Only the command line changes. Section 10 gives the consumer contract.
The nvim edit is MACHINIST work and is not in this RFC's scope.

Every decision in this RFC is ARCHITECT's. Section 9 records each one. No question
stays open.

---

## 2. Problem

### 2.1 How nvim formats C++ now

`~/.config/nvim/lua/core/formatting.lua:30-70` (`formatBuffer`):

1. Write the unsaved buffer to a temp file (`vim.fn.tempname()`).
2. Run `{ clangFormatBin, '--style=file:' .. STYLE_PATH, tmpfile }` with `jobstart`.
3. Collect stdout (`stdout_buffered = true`).
4. On exit 0, strip CR bytes and replace the buffer lines with stdout.
5. On a non-zero exit, show `clang-format failed (exit N)`.
6. Delete the temp file.

The triggers are:

- Esc from insert mode or visual mode (`core/actions.lua:169-187`).
- The `c:n` mode change (`core/autocommands.lua:159-177`).
- `BufWritePre` (`core/autocommands.lua:180-184`).

The formatter formats the **buffer**, not the file on disk. The temp file only
carries the buffer text. The style path is explicit (`--style=file:<path>`), thus the
temp file location does not matter.

### 2.2 How `cast --format` works now

- `SPEC.md` §2.1 (line 93): *"`--format`: format only. The engine re-canonicalizes
  each declared markdown file (§3.3), write-if-different."*
- The argument is a manifest. `cast --format <path>` parses `<path>` as a manifest
  (`main.cpp:286-294`, `getDocumentFile`). It writes each origin in place
  (`Processor.h:85-106`, `format`; `Processor.h:201-215`, `writeOriginIfChanged`).
- Stdout gets the `cast: done` line only when stdout is a terminal
  (`main.cpp:387-393`).
- No stdin mode exists. No stdout mode exists.

### 2.3 Observed behavior of `--format`

MACHINIST ran these tests in a scratch directory on Windows.

1. A plain markdown file with no index:
   `printf '# t\n\n|a|b|\n|-|-|\n|x|yy|\n' > plain.md; cast --format plain.md`.
   Exit 0. The file changed in place to the canonical pipe table. Stdout was empty.
2. `cast/cast/spell.md` copied to a different directory as `tmp123.md`, then
   `cast --format tmp123.md`. Exit 0. The file formatted in place. No other file
   appeared in the directory.
3. `cat plain.md | cast --format -`. Cast printed the help guide. It read `-` as a
   manifest path that does not exist (`main.cpp:536`, `runDocumentNotFound`).

Result: `cast --format <file>` already formats any markdown file. A file with no
manifest acts as its own manifest. But the command writes in place and prints
nothing. The nvim wiring in §2.1 reads stdout. Thus the wiring cannot swap only the
command line.

---

## 3. The clang-format Contract — The Reference

The target is exact parity. This section is the reference for every rule in §5 and
§6.

### 3.1 From `clang-format --help` (clang-format 20.1.8)

This is the Visual Studio LLVM build that `formatting.lua:11` uses.

- *"If no arguments are specified, it formats the code from standard input and
  writes the result to the standard output."*
- *"If <file>s are given, it reformats the files. If -i is specified together with
  <file>s, the files are edited in-place. Otherwise, the result is written to the
  standard output."*
- `USAGE: clang-format.exe [options] [@<file>] [<file> ...]`
- `-i` — *"Inplace edit <file>s, if specified."*
- `--style=file` — *"load style configuration from a .clang-format file in one of the
  parent directories of the source file (for stdin, see --assume-filename)."* and
  *"--style=file is the default."*
- `--style=file:<format_file_path>` — *"explicitly specify the configuration file."*
- `--assume-filename=<string>` — *"Set filename used to determine the language and
  to find .clang-format file. Only used when reading from stdin. If this is not
  passed, the .clang-format file is searched relative to the current working
  directory when reading stdin."*

### 3.2 Config file names — observed

MACHINIST ran this test on Windows. Each case puts one config file with
`IndentWidth: 8` in an empty directory, beside `a.cpp`. The LLVM default indent is 2.

| Config file                                    | Command                             | Indent | Found |
|------------------------------------------------|-------------------------------------|--------|-------|
| `.clang-format`                                | `clang-format --style=file a.cpp`   | 8      | yes   |
| `_clang-format`                                | `clang-format --style=file a.cpp`   | 8      | yes   |
| `my.clang-format`                              | `clang-format --style=file a.cpp`   | 2      | no    |
| `anyname.yaml`                                 | `--style=file:anyname.yaml a.cpp`   | 8      | yes   |

Result:

1. The directory search finds only two fixed names: `.clang-format` and
   `_clang-format`.
2. An explicit path takes any file name.

---

## 4. Column Widths Now

### 4.1 Where the widths come from

- `HELP.md:511`: *"`## format` is an optional table, `| name | width |`, reserved by
  name — not by file, like `## toolchain`."*
- `Model::addReflowWidths` (`Model.h:833-846`) reads every `## format` table in the
  parsed Model: `for (auto* table : document.getTables (Id::format))`.
- `Model::parse` calls it after the manifest and every declared data file are
  spliced into the Model (`Model.h:857-870`).
- `Processor::format` builds the writer from these widths:
  `jam::MarkdownWriter formatter { lineWrap, model->getReflowWidths() }`
  (`Processor.h:90`).
- The widths apply only to a grid table with a border between body rows.
  `HELP.md:511`: *"A grid table whose body has no border between rows never
  splits."*
- `--sync` formats with a bare writer: `static const jam::MarkdownWriter formatter;`
  (`Sync.h:876`). It uses no widths and the default wrap, 100.

### 4.2 Proof — placement of `## format`

MACHINIST ran these tests in a scratch directory on Windows. Each fixture uses the
same data table:

- `data.md` holds one grid table `| key | comment |`, with a border after each body
  row.
- Row `a` has a 60-column comment cell. Row `b` has `short`.
- The `## format` table has one row: `comment | 20`.

A "wrap" means the row `a` cell splits into 4 rows at 20 columns.

| Case | `## format` location               | Command                    | Result  |
|------|------------------------------------|----------------------------|---------|
| 1    | in `spell.md`                      | `cast --format spell.md`   | wrap    |
| 2    | in a copy of `data.md`, alone      | `cast --format solo.md`    | no wrap |
| 3    | in `style.md`, declared by index   | `cast --format spell.md`   | wrap    |
| 4    | inside `data.md`, via the manifest | `cast --format spell.md`   | wrap    |
| 5    | inside `data.md`, alone            | `cast --format data.md`    | wrap    |
| 6    | in `style.md`, not declared        | `cast --format data.md`    | no wrap |

Case 2 uses the fixture of case 1. The `## format` table is in `spell.md`, not in
`solo.md`.

### 4.3 Conclusions

1. The canonical text of a data file depends on its manifest (cases 1 and 2). A file
   formatted alone does not get the widths of the manifest that declares it.
2. `## format` works now in any file that cast parses: the manifest, a declared
   file, or the formatted file itself (cases 1, 3, 4, 5).
3. Cast never searches for a width configuration. A file that nothing declares is
   ignored (case 6).
4. No data file in `cast/cast/` has a border between body rows. MACHINIST formatted
   each of the 6 files alone. Each output was byte-identical to the file on disk. The
   effect in conclusion 1 does not occur in cast's own data now. It occurs in any
   project whose data files have bordered body rows in a column that `## format`
   names.

### 4.4 Consequence for the stdout mode

The nvim wiring in §2.1 formats a temp file in the temp directory. That path is not
near any manifest. If the widths stay in the manifest, the stdout mode cannot find
them from the temp path. The output then differs from what the manifest run writes,
and the nvim format and the next `cast` run fight each other.

clang-format has no such problem: the style is one explicit file, or one file that a
directory search finds (§3). Section 6 gives cast the same model.

---

## 5. Target Contract — Command Line

### 5.1 Grammar

The new `--format` lines replace the current `--format` line in `SPEC.md` §2.1. The
other lines stay.

```
cast --format [-i] [--style=file:<path>] [--assume-filename=<path>] [--line-wrap n] [<file> ...]
cast [<manifest>] [<directory> | --no-format | --<word>] [--line-wrap n]
cast --sync [--style=file:<path>] <source-root> <target-root>
cast --version
cast --help
```

### 5.2 Flags

The flag words are clang-format's words, verbatim (§9, decision 2):

| Flag                        | Meaning                                                   | clang-format    |
|-----------------------------|-----------------------------------------------------------|-----------------|
| `-i`                        | Write the canonical text back to each file, in place.     | `-i`            |
| `--style=file:<path>`       | Use this config file. Any file name.                      | same            |
| `--style=file`              | Search for the config file (§6.2). The default.           | same            |
| `--assume-filename=<path>`  | Stdin only: the path where the config search starts.      | same            |

`--line-wrap n` stays. It is cast's own flag, and it keeps its current form
(`main.cpp:73-77`, `SPEC.md:84`). It overrides the `line-wrap` row of the resolved
config (§6.1).

### 5.3 Behavior of `--format`

1. **No file argument.** Cast reads stdin and writes the canonical text to stdout.
2. **File arguments, no `-i`.** Cast writes the canonical text of each file to
   stdout. The files on disk do not change.
3. **File arguments and `-i`.** Cast writes each file in place,
   write-if-different. Stdout stays empty.
4. **A manifest with `-i`.** `cast --format -i spell.md` formats the manifest and
   each markdown file that its index declares, in place. This is the current
   `cast --format spell.md` behavior, moved under `-i` (§9, decision 10).
5. **A manifest without `-i`.** Cast writes the canonical text of the manifest file
   only to stdout, the same as any other file.
6. **Stdin and a file argument together.** Cast reads the file argument. Cast reads
   stdin only when no file argument exists. This is the clang-format rule (§3.1).
7. **Success.** Exit 0. Stdout holds only the canonical text. There is no
   `cast: done` line in `--format` mode, even when stdout is a terminal.
8. **Failure.** A non-zero exit. Stdout is empty, thus a consumer never replaces a
   buffer with partial text. Stderr keeps cast's current diagnostic form (§9,
   decision 9). Observed:
   `cast: a.md:5 (width): format width must be a positive integer`. For stdin input,
   the file part is the `--assume-filename` path, or `<stdin>` when that flag is
   absent. `<stdin>` is the clang-format word, observed:
   `<stdin>:1:8: warning: code should be clang-formatted [-Wclang-format-violations]`.

### 5.4 Changes to existing modes

- `cast --format spell.md` without `-i` no longer writes in place (§5.3, item 5).
  Each caller that needs the old behavior uses `-i`. MACHINIST found no nvim caller
  of `--format` (`core/cast-build.lua:34-38` runs `cast <manifest>` and
  `cast <manifest> --<argument>`). The SPRINT-LOG proofs and the kuassa usage
  (`carol/SPRINT-LOG.md:258`) are historical records. Documents that give the command
  as an instruction change to `-i`: `SPEC.md`, `Source/HELP.md`, `CLAUDE.md:83`.
- `cast`, `cast <manifest>`, `--no-format`, `--<word>`, `--version`, `--help` keep
  their behavior. The format step of `cast <manifest>` resolves the config by §6.2.
- `SPEC.md:106-107`: *"`--format` and `--no-format` exclude each other. Each also
  excludes `<directory>` and `--<word>`. One manifest, one flag, nothing else on the
  line."* The `--format` line now takes the flags in §5.2 and one or more files. The
  exclusion with `--no-format`, `<directory>`, and `--<word>` stays.
- The rule *"the flag reads before or after the manifest"* (`SPEC.md:83`) stays for
  `--format`. clang-format also accepts options in any position.

---

## 6. Target Contract — `.cast-format`

### 6.1 The file

1. **Name.** The directory search finds two fixed names: `.cast-format` and
   `_cast-format`. This is the clang-format pair `.clang-format` and `_clang-format`
   (§3.2). `--style=file:<path>` takes any file name (§3.2).
2. **Content.** Markdown with one `## format` table, `| name | width |`. The existing
   parser reads it. `Validator::isFormat` (`Processor.h:87`) validates it. No new
   syntax exists.
3. **Column widths.** Each row names a column and gives its wrap width. This is the
   current `## format` rule (`HELP.md:511`).
4. **Line wrap.** One reserved row: `name` is `line-wrap`, `width` is the paragraph
   wrap. The word is the same word as the `--line-wrap` flag
   (`cast/identifiers.md:37`). `--line-wrap n` overrides the row. With no row and no
   flag, the wrap is `jam::MarkdownWriter::defaultLineWrap` (100,
   `jam_MarkdownWriter.h:22`), the current default (`main.cpp:512`).
5. **The `line-wrap` row is not a column.** The row does not reflow a column named
   `line-wrap`. COUNSELOR confirms that no current data table has a column named
   `line-wrap`.

### 6.2 Precedence — the first match wins

1. **Explicit argument.** `--style=file:<path>`. Any file name.
2. **Manifest directory.** `.cast-format` or `_cast-format` in the directory of the
   manifest, for example `project/cast/.cast-format`. This applies when cast runs a
   manifest: `cast <manifest>` and `cast --format -i <manifest>`.
3. **Directory search.** `.cast-format` or `_cast-format` in the directory of the
   formatted file, then in each parent directory. For stdin input, the search starts
   at the `--assume-filename` path. Without that flag, the search starts at the
   current working directory. This is the clang-format rule (§3.1).
4. **None found.** No column reflows, and the line wrap is 100 or `--line-wrap n`.
   This is the current behavior when no `## format` table exists.

Cast uses exactly one config file per run, or none. Cast never merges two config
files.

When one directory holds both `.cast-format` and `_cast-format`, COUNSELOR checks the
clang-format order and uses the same order. The order is a fact of clang-format, not
a new decision.

### 6.3 `## format` outside the config file

A `## format` table in any file except the resolved config file is an ordinary data
table (§9, decision 6). It has no formatter meaning. Cast does not read widths from
it and does not reject it.

The table name `## format` loses its reservation outside the config file. `HELP.md:511`
changes from *"reserved by name — not by file"* to *"reserved in the config file
only"*. The reserved column name `format` (`HELP.md:104`) does not change.

These files hold a `## format` table now. Each project moves the table to a
`.cast-format` beside its manifest, and removes it from the old file:

1. `dev/cast/cast/spell.md`
2. `dev/jam/cast/spell.md`
3. `dev/whatdbg/cast/spell.md`
4. `kuassa/user_modules/cast/spell.md`
5. `kuassa/jreng-filter-strip/project-info.md` (a file that the manifest declares)

A table that stays behind after the change silently stops applying widths.
Acceptance check 11 (§11) catches it: the migrated project must format to zero
changes.

### 6.4 `--sync`

A sync root is a framework root, and a framework root is a project (§9, decision 8).
Thus sync applies the project rule of §6.2.

Facts:

- The two roots that carry `user-modules-info.md` are `dev/jam` and
  `kuassa/user_modules`. Each one has a `cast/` directory with a manifest:
  `dev/jam/cast/spell.md` and `kuassa/user_modules/cast/spell.md`.
- Each of these manifests holds a `## format` table now (§6.3). After the migration,
  `<root>/cast/.cast-format` is in the manifest directory, the same location as tier
  2 of §6.2.
- Sync writes the target root. Thus the target root's config applies, and the sync
  output matches the target's own `cast --format -i` run.
- Now sync formats with a bare writer: `static const jam::MarkdownWriter formatter;`
  in `getCanonicalMarkdown` (`Sync.h:874-880`). It uses no widths and wrap 100.

Precedence for sync — the first match wins:

1. **Explicit argument.** `--style=file:<path>` on the `--sync` line.
2. **Target project.** `.cast-format` or `_cast-format` in `<target-root>/cast/`.
3. **None found.** No column reflows, and the line wrap is 100. This is the current
   sync behavior.

Sync does no directory search (§6.2, tier 3). It does not search for a manifest.

`SPEC.md` §2.1 says that `--sync` *"composes with nothing else on the line"*. The
`--sync` line now also takes `--style=file:<path>`. The grammar in §5.1 changes to:

```
cast --sync [--style=file:<path>] <source-root> <target-root>
```

`## toolchain` stays in the manifest. It is not formatter configuration.

---

## 7. Invariants

1. **Fixpoint parity.** For a file that a manifest declares, the stdout text of
   `cast --format <file>` is byte-identical to the text that
   `cast --format -i <manifest>` writes for that file, when both resolve the same
   config file.
2. **Idempotence.** Formatting the stdout text again gives the same text.
3. **Line endings.** Stdout uses LF only, the same as `writeOriginIfChanged`
   (`Processor.h:209-212`, `newlineText`). Windows must not add CR to stdout. Put
   stdout in binary mode on Windows (`_setmode (_fileno (stdout), _O_BINARY)`), or
   use an equal method that the codebase already has.
4. **No screen clear.** `main.cpp:500-507` runs `cls` or `clear` when stdout is a
   terminal. In `--format` mode this must not run. When a consumer pipes stdout,
   `isTerminalOutput()` is false and the clear does not run. The terminal case must
   also be correct, for a user who runs the command by hand.
5. **Encoding.** Input and output are UTF-8. `main.cpp:497` already sets the Windows
   console output code page to UTF-8.
6. **One formatter path.** The stdout mode, `-i`, the format step of
   `cast <manifest>`, and `--sync` use one path to get the canonical text and one
   path to resolve the config.

---

## 8. Scope of Change in cast

COUNSELOR confirms the file set in PLAN.md.

1. `SPEC.md` §2.1 — the grammar (§5.1), the flags (§5.2), the behavior (§5.3), and
   the exclusion rules. SPEC is the authority (`SPEC.md:103-104`).
2. `SPEC.md` — a new section for the config file: the names, the content, the
   `line-wrap` row, the precedence (§6). The `## format` column-width section
   changes to point at it. `SPEC.md:66-67` (*"CAST does not scan a directory and
   does not glob"*) gains one named exception: the fixed-name config search. The
   search is not a glob. It checks two fixed names in each directory.
3. `SPEC.md` §2.1 and §2.2 — sync resolves the config (§6.4). The `--sync` line
   takes `--style=file:<path>`. The rule *"composes with nothing else on the line"*
   changes to name this one flag. `Source/main.cpp` `runSyncArguments`
   (`main.cpp:481-492`) checks the arity: `syncArgumentCount { 4 }`
   (`main.cpp:421`). The check changes to accept the optional flag.
4. `SPEC.md` §10.1 (fatal table, near line 1118) — the new fatals, in cast's current
   diagnostic form. Examples: a stdin read that fails, a file that does not
   exist, a `--style=file:<path>` that does not exist, an unknown flag, `-i` with no
   file argument.
5. `Source/HELP.md` — the Command Line section and the column-width section. HELP is
   derived from SPEC and follows it. `CLAUDE.md:83` changes to `-i`.
6. `cast/identifiers.md` — the new flag words (`i`, `style`, `file`,
   `assume-filename`), the config file names, and `<stdin>`. The existing pattern:
   `| @id | lineWrap | \`line-wrap\` |` (`identifiers.md:37`). Regenerate
   `Source/generated/Identifiers.h` through `cast`.
7. `cast/text.md` — new diagnostics. The existing pattern: `failFlagValue`
   (`text.md:47`). Regenerate `Source/generated/Text.h`.
8. `Source/main.cpp` — argument parse and dispatch. The parse gains the single-dash
   form (`-i`) and the `=` form (`--style=file:<path>`, `--assume-filename=<path>`).
   Follow the existing flag family: `getFormatFlag`, `getNoFormatFlag`,
   `getLineWrapFlag` (`main.cpp:43-77`). Add a run function beside `runDocument` and
   `runSync` (`main.cpp:375`, `main.cpp:447`). The stderr form does not change
   (`main.cpp:398-400`).
9. `Source/Processor.h` — a canonical-text path for one origin that returns the text
   and does not write. Now `writeOriginIfChanged` both gets the text and writes it
   (`Processor.h:201-215`). NAMES.md Verb Contract: *"A unit that both computes and
   stores is two units. Split it: `get` returns, the caller `add`s."* The stdout mode
   and `-i` then share one `get` path. Invariant 7.1 then holds by construction.
10. `Source/Model.h` — the widths and the line wrap come from the resolved config,
    not from `getTables (Id::format)` across the Model (`Model.h:833-846`). A
    text-input entry point for stdin: the parse under `Model::parse` already takes
    text (`jam::MarkdownDocument::parse (documentFile.loadFileAsString(),
    manifestOrigin)`, `Model.h:865`; each data file, `Model.h:1067-1068`). Use this
    path. Do not write a second parser.
11. `Source/Validator.h` — `Validator::isFormat` validates the config file, including
    the `line-wrap` row. Its diagnostics keep the current form, which already carries
    the file, the line, and the column name.
12. `Source/Sync.h` — the formatter at `Sync.h:876` resolves the config (§6.4).
13. Migration — the 5 files in §6.3. Cast's own `cast/cast/` goes first, because cast
    regenerates its own sources.

Out of scope: generation, the toolchain rows, `jam_markdown`. The
`jam::MarkdownWriter` and `jam::MarkdownValidator` APIs
(`jam/jam_markdown/document/jam_MarkdownWriter.h:18-30`,
`jam_MarkdownValidator.h:19`) are used as they are. If a change needs JAM, COUNSELOR
stops and reports it to ARCHITECT.

---

## 9. Decisions (ARCHITECT, 27 Sep 2026)

| # | Topic                        | Decision                                                                          |
|---|------------------------------|-----------------------------------------------------------------------------------|
| 1 | Command-line shape           | `--format <file>` writes stdout, like clang-format.                               |
| 2 | Flag words                   | clang-format verbatim: `-i`, `--style=file:<path>`, `--assume-filename=<path>`.   |
| 3 | Config file name             | The search finds `.cast-format` and `_cast-format`, the clang-format convention (§3.2): *"we should just conform to it then"*. The explicit argument takes any name. |
| 4 | Stdin config search          | *"make it EXACTLY IDENTICAL to clang-format"*: `--assume-filename`, else the working directory. |
| 5 | Line wrap in the config      | Yes, as a row in `## format`, `name` = `line-wrap`. `--line-wrap n` overrides it. |
| 6 | `## format` outside config   | *"## format table in any md treated just like any table. a config style is ## format table in \*.cast-format only"* |
| 7 | Stdin and a file together    | The clang-format rule: the file wins; stdin only with no file argument.           |
| 8 | `--sync` formatter           | *"--sync is for framework, could be considered as "project""*: `--style=file:<path>`, else `<target-root>/cast/.cast-format` or `_cast-format`, else none (§6.4). |
| 9 | Error output                 | Keep cast's current form: *"if cast error message already serves its purpose there's no point of make it identical to clang-format. markdown is not C-lang"* |
| 10| Manifest-wide in-place run   | `cast --format -i spell.md` formats the manifest and each declared file.          |
| 11| Precedence                   | Explicit argument, then the manifest directory, then the directory search, then none. |

Decision 3 note: ARCHITECT asked *"clang-format could use arbitrary file name no?"*
The test in §3.2 answers it. An arbitrary name works only through
`--style=file:<path>`. The search finds only the two fixed names. Exact parity gives
cast the same two paths.

---

## 10. Consumer Contract (nvim — MACHINIST, After cast Ships)

This section records what the nvim side needs. MACHINIST does this edit after cast
ships and ARCHITECT approves it.

1. `core/formatting.lua` gets a markdown path with the same structure as
   `formatBuffer`: temp file, `jobstart` with `stdout_buffered`, strip CR, replace
   the buffer lines on exit 0, show a notice on a non-zero exit.
2. `--assume-filename` applies to stdin only (§3.1). Thus nvim uses one of these two
   forms, both exact clang-format parity:
   - a. A cast project: `{ CAST_BINARY, '--format', '--style=file:' .. configPath,
     tmpfile }`. `configPath` comes from the project that `core/project/cast.lua`
     finds. This is the C++ form (§2.1).
   - b. Any buffer: send the buffer text on stdin with
     `{ CAST_BINARY, '--format', '--assume-filename=' .. bufferPath }`. Cast then
     searches from the buffer's real directory (§6.2, tier 3). No temp file.
   MACHINIST picks the form at nvim edit time and presents it to ARCHITECT.
3. `CAST_BINARY` is `cast.exe` on Windows and `cast` on macOS. It is defined now in
   `core/cast-build.lua:16`. Move it to one shared definition, so the build and the
   formatter use the same name.
4. The conform `pandoc_markdown` formatter and the `markdown` entry are removed from
   `plugins/formatting.lua:24-38`.
5. The triggers are the same as for C++: Esc, `c:n`, `BufWritePre`.
6. The cast stderr line (`cast: <file>:<line> (<column>): <message>`) can feed the
   quickfix list. That is a separate nvim decision.

---

## 11. Acceptance Criteria

1. `cast --format plain.md` prints the canonical text on stdout. `plain.md` on disk
   does not change.
2. `cat plain.md | cast --format` prints the same bytes as item 1.
3. `cast --format -i plain.md` writes the file in place. A second run writes
   nothing. Stdout is empty.
4. For each markdown file that `cast/cast/spell.md` declares,
   `cast --format <file>` prints the bytes that `cast --format -i cast/cast/spell.md`
   writes for it.
5. The §4.2 fixture, with the `## format` table moved to `.cast-format` beside
   `spell.md`:
   - `cast --format -i spell.md` wraps the row `a` cell at 20 columns.
   - `cast --format --style=file:<path to .cast-format> <temp copy of data.md>`
     prints the same bytes.
   - `cast --format data.md` in its own directory finds `.cast-format` by tier 3 and
     prints the same bytes.
   - `cat data.md | cast --format --assume-filename=<path to data.md>`, run from an
     unrelated directory, prints the same bytes.
   - `cat data.md | cast --format`, run from the fixture directory, prints the same
     bytes.
6. Names: `_cast-format` is found by the search. `my.cast-format` is not found by the
   search. `--style=file:anyname.md` is used. This mirrors §3.2.
7. Precedence: with `--style=file:<path>` and a different `.cast-format` in the
   manifest directory, the explicit file wins. With no config anywhere, no column
   reflows and the wrap is 100.
8. Line wrap: a `line-wrap` row in `.cast-format` sets the paragraph wrap.
   `--line-wrap n` overrides it.
9. A `## format` table in a data file has no formatter effect and no error.
10. `--sync`:
    - With `<target-root>/cast/.cast-format`, sync formats each re-canonicalized
      `.md` file with the target's config. A second `cast --format -i` run on the
      target manifest then makes zero changes.
    - With `--style=file:<path>`, that file wins over the target's config.
    - With no config, the sync output is byte-identical to the current output.
    - A `.cast-format` in the source root has no effect on the target.
11. Migration: each of the 5 projects in §6.3 formats to zero changes after its
    `## format` table moves to `.cast-format`. The self-hosting fixpoint holds: a
    second `./cast` run and a second `cast --format -i` run make zero changes (the
    existing proof, `carol/SPRINT-LOG.md:1732`).
12. The stdout text contains no CR byte on Windows.
13. In `--format` mode, stdout contains no `cast: done` line and no screen-clear
    sequence. Test this with stdout as a pipe and with stdout as a terminal.
14. A fatal gives a non-zero exit, empty stdout, and cast's current diagnostic line
    on stderr. For stdin without `--assume-filename`, the file part is `<stdin>`.
15. `cast`, `cast <manifest>`, `--no-format`, `--<word>`, `--version`, `--help` give
    the same results as before.
16. Both platforms: Windows (clang-cl build) and macOS.

---

## 12. Additional Findings (nvim keymaps migration, 27 Sep 2026)

MACHINIST moved the nvim keymap generation to `cast`. The result is byte-identical
to the old Lua generator's output, with no change to cast. Thus cast has the
capability. These three findings came from the work. ARCHITECT added them to this
RFC. They are separate from the clang-format parity scope, and COUNSELOR plans them
as ARCHITECT directs.

### 12.1 HELP.md — a bullet on an item shape

- `HELP.md:333`: *"A named bullet binds one token of the nearest shape line above
  it."* The same paragraph calls `- [list]: @code:<id>` a shape line.
- Observed: in a wiring row with `@t:wrap`, then `- [list]: @t:line`, then
  `- tag: PLAIN`, the `:::tag:::` token in `line` renders empty. An index datum
  (`- indent: @sp`) also renders empty.
- Real use (JAM `dev/jam/cast/spell.md`) binds bullets only to the enclosing bare
  shape (`- type: @lineBreakQuotation` at `spell.md:1722`). Per-row values come from
  row columns, for example `type | @byteCodepointByte` on each row of
  `lookuptables.md:1277-1284`.
- Gap: HELP does not say that item shapes take their tokens from row columns only.

### 12.2 HELP.md — backslashes in a plain cell

- `HELP.md:103`: a plain-text cell is *"that text, verbatim"*. `HELP.md:125`:
  *"Every other byte is data"*. `HELP.md:142` describes only the pipe escape.
- Observed: plain cell `x\\y` renders `x\y`. Plain cell `\<x\>` renders `<x>`.
  Plain cell `x\\\\y` renders `x\\y`. Plain cells follow markdown backslash escapes.
- JAM writes backslashes escaped (`chars.md:265`, `` `\\\|` ``).
- Gap: HELP does not say that a plain cell applies markdown backslash escapes.

### 12.3 Crash — a wiring row with only an item line

- Reproduction (Windows, `cast` 0.1.0):
  - `spell.md` has an index with `@d | d.md`, `@t | t.cast`, `@o | o.txt`.
  - It has one wiring row: `| - [list]: @d:d | - [list]: @t:line | @o |` (a pipe
    table, no bare shape in the structure cell).
  - `t.cast` holds one fence, `line`. `d.md` holds one table, `## d`.
- Observed: `cast spell.md` ends with a segmentation fault, exit 139, and prints no
  diagnostic.
- `HELP.md:782`: *"No warnings exist. Every failure is fatal, exits non-zero, and
  writes no output file."* The input is probably invalid. The expected result is a
  fatal diagnostic that names the file and the line, not a crash.

---

**End of RFC**
