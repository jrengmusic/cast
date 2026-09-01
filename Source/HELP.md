# CAST: Codegen Annotated Source of Truth

**Version 0.1.0**

You are seeing this because you ran `cast --help`, or because `cast` could not find a `CAST.md` in the current directory.

CAST reads your data (tables), your shapes (templates), and your wiring (the manifest), and writes files. It knows nothing about any programming language. It resolves references, iterates rows, and replaces tokens — nothing else.

This guide explains how to use CAST. The rules it describes are fixed in `SPEC.md`; where this guide and SPEC disagree, SPEC is right and this guide is wrong.

---

## Command Line

Arguments select what to run. They never carry generation rules.

- `cast` — find `CAST.md` here, format every declared markdown file, then regenerate every declared output
- `cast <path>/CAST.md` — the same, using a specific manifest
- `cast CAST.md <directory>` — write every declared output under that directory instead
- `cast CAST.md --format` or `cast --format CAST.md` — format only, no generation; the flag reads either before or after the manifest
- `cast CAST.md --no-format` or `cast --no-format CAST.md` — generate only, no formatting
- `cast CAST.md --<word>` — after format and generate, run only the `## toolchain`
  rows whose `argument` cell equals `word`, instead of the default-flow rows
- `cast --version` — version and source commit, the same stamp embedded in generated banners
- `cast --help` — this guide

Default order is format, then generate. `--format` and `--no-format` are mutually exclusive with each other and with an output directory or a `--<word>` toolchain argument — one manifest, one flag, nothing else on the line.

---

## The Three Artifacts

Everything lives flat in one data directory.

| Artifact | What it is |
|---|---|
| `CAST.md` | the manifest — what gets generated, from what, into what |
| `*.md` | data tables |
| `*.cast` | template files — any number, each holding code shapes |

Every input file is declared once in the manifest's index. CAST parses exactly what you declare — it never scans a directory. Generated outputs are build artifacts; do not edit them.

---

## Writing Tables

### Rows

A table's row boundaries are decided once, for the whole table:

- if the body has a border between rows, borders delimit rows
- if it has none, and no line starts with an empty first cell, every `|` line is one row
- if it has none, and some line starts with an empty first cell, the whole body is one row

```
+-------------+-------+          +-------------+-------+
| key         | value |          | key         | value |
+=============+=======+          +=============+=======+
| block quote | 3     |          | block quote | 3     |
+-------------+-------+          | code fence  | 4     |
| code fence  | 4     |          +-------------+-------+
+-------------+-------+
   two rows, borders delimit        two rows, one line each
```

Row order is authored order. CAST never sorts.

### Columns

Column names are yours. Name a column for what it produces, because a template addresses it by that name — a column called `value` fills `:::value:::`.

Two column names are reserved everywhere: `format` and `comment`. Inside the manifest's wiring tables, four more are reserved: `list`, `separator`, `structure`, `file`. A data table — wherever it lives, the manifest included — may use any of those four words as an ordinary column.

### Cells

A cell is one of three things:

| Cell | Meaning |
|---|---|
| plain text | that text, verbatim |
| `` `text` `` | `toLiteral` of those bytes — quoted and escaped, ready to drop into a literal |
| a fenced block | the same, over a datum that spans lines |
| empty | nothing — an empty string |

An empty cell is not an error and not a special case — it is string replacement as nothing, in every table, reserved or not. A token it fills renders empty, and a template line left empty by its only placeholder elides for free, exactly like any other emptied placeholder line. An empty `format` cell formats nothing: the value is written verbatim.

Backticks are not decoration and not an escape. A backtick **is** the `toLiteral` operation, so a backticked cell never needs a `format` column beside it.

A code span cannot hold a line break. When the datum has one, author the cell as a fence — each line between the delimiters is one line of the value, joined by line breaks:

```
+-------------------------------+-----------------------------------+
| alertNewerVersionPresetSuffix | ```                               |
|                               |                                   |
|                               | To use this preset correctly:     |
|                               | ```                               |
+-------------------------------+-----------------------------------+
```

That cell's value begins with a line break, because the fence's first content line is empty. Write the real characters; `toLiteral` turns them into `\n` for you.

A cell is delimited by its pipes on every line it spans, and the padding between them is never data — `|this|` and `| this |` are the same datum, and the formatter rewrites the first as the second. That holds inside a fence too.

So a space at the very start or end of a line cannot be written as a space; it would be indistinguishable from padding. Write it as `U+0020` and put `fromUTF8` in the row's `format` cell:

```
+-------------------------------+---------------------------------------+----------+
| alertNewerVersionPresetSuffix | ```                                   | fromUTF8 |
|                               |                                       |          |
|                               | please install the newest version:U+0020 |       |
|                               | ```                                   |          |
+-------------------------------+---------------------------------------+----------+
```

`fromUTF8` decodes every `U+XXXX` token in the value and leaves every other byte alone.

To put a `|` inside a cell, precede it with a backslash. The scanner eats that backslash when it rejoins the split, so a datum that needs a backslash *and* a pipe is written with two — `` `\\|` `` yields the two bytes `\|`.

### Format

`format` is a column, it binds to the column immediately to its left, and it names **exactly one** operation. It is optional — a column with no `format` beside it is written exactly as authored.

```
| key | format | value | format |
| key | format | value |
| key | value  | format |
| key | value  |
```

`| format | format |` is rejected — a format has no format.

There is no chaining. If a datum needs a different shape, author it in that shape:

```
+------+-------------+-------------+-----------+
| type | name        | value       | format    |
+======+=============+=============+===========+
| @id  | circleCross | circleCross | toLiteral |
+------+-------------+-------------+-----------+

name  ->  circleCross      verbatim
value ->  "circleCross"    circleCross, quoted and escaped by toLiteral
```

Formatting is declared in the table. A template never formats anything.

### Documentation

Three doc channels, all data:

- **row** — a column named `comment`. An item shape's `:::comment:::` takes it like any other column.
- **table** — write a paragraph or a fenced block between the `## table name` heading and the table. A shape-level `:::comment:::` falls back to it when no reference names something else.
- **fence** — a fenced block carrying an info string, anywhere in a data file, not bound to a table. The info string is its name; the fence text is its prose. Address it — or a table — by name with a `- comment: @file:<name>` bullet in the manifest's list column, at the shape line's own `>` count. A wrapper shape declared across several rows carries the same reference on every declaring row, exactly like a binding.

At shape level, with no comment reference authored, `:::comment:::` falls back to the documentation of the first table the shape's own sources address, in authored order; with no source addressing a table, to the documentation of the row's own table.

You never author the comment frame (`/** @brief ... */`, `///< ...`) yourself — you write prose only. CAST renders the frame from the comment-syntax table, keyed by the output file's extension — except when the file's exact name has a row in the manifest-syntax table (`CMakeLists.txt` is CMake, not text; that row's value replaces the extension as the key):

- the marker alone on its line — block form. Multi-line prose renders one prose line per output line; single-line prose renders open, text, close on one line.
- the marker inline, after content — single-line form: the language's comment glyph, then the text.

A missing comment is not an error — the marker renders empty, and an emptied line trims or collapses like any other placeholder line.

### Uniqueness

Within one table, every identity column's entries — `name`, `key`, `alias` — must be unique, compared byte for byte. `circleCross` and `CircleCross` are two different entries. Every other column is payload — `value`, `type`, `format`, `comment` — and payload repeats by design: many rows may map to the same payload.

---

## References

The `@` sigil is part of the name. A cell starting with `@` is a reference; anything else is data. They cannot be confused for each other.

### The index

Any file may declare `## index`. It is file-local — a `@` cell resolves against the index of the file it is written in, never another file's. It may carry a `format` column like any other table — an index datum is formatted like any cell (`@space` declares `U+0020` with `fromUTF8`).

```
## index

+---------+------------------+
| alias   | symbol           |
+=========+==================+
| @id     | juce::Identifier |
| @string | juce::String     |
+---------+------------------+
```

The manifest's own index is where input files are declared:

```
| @identifiers | identifiers.md |
```

### Addressing

```
@file : table : column
```

Write the table part only when you need it. It is omissible when the table is named after its file, or when the file holds exactly one table — the index never counts for that purpose.

CAST stops with an error if an alias is undeclared, declared twice, points at a missing file, or names a table or column that does not exist.

### Filtering rows by a cell

```
@file : table : column = value
```

Add `=value` to the column part and the address selects only that table's rows whose `column` cell equals `value`, byte for byte. `value` can be empty, matching a row whose cell is itself blank — nothing (see Cells). This is how one table serves two disjoint row sets — a compiler-stage split, say — without a second table:

```
+----------------------+-----------------------------+---------+--------------------------+
| name                 | mac                          | win     | stage                    |
+======================+===============================+=========+==========================+
| optimization         | -O3                           | /O2     |                          |
| deadCodeStripping    | -dead_strip                   | /OPT:REF | linker                   |
+----------------------+-------------------------------+---------+--------------------------+
```

`@project-info:release:stage=` selects the compiler rows (`stage` blank); `@project-info:release:stage=linker` selects the linker rows — one table, two wiring lines, no duplication.

---

## Templates

Code shapes live in `.cast` files — as many as you declare in the index. Each is markdown: fenced code blocks, and the fence's info string is the block's name. A shape is addressed like any other reference: `@code:namespace` is the `namespace` block of the file the `@code` alias names. Two files may both have an `entry` block; `@code:entry` and `@cmake:entry` are different shapes.

````markdown
```identifier
inline const :::type::: :::name::: { juce::String::fromUTF8 (:::value:::) };
```

```entry
{ :::list::: },
```
````

A block is the literal text of the output. Braces, keywords, punctuation — all authored, all verbatim. There are no conditionals, no loops, and no formatting. Target-language directives (`#if`, `#endif`) are literal text like everything else — CAST never reads them, and slots inside such an arm take sources by the ordinary arity law. Comment frames are the one exception: CAST renders them itself, from the comment-syntax table (see Documentation) — never author one in a template.

Token names are yours — free text, matched exactly as you wrote them. `:::macro-guard:::` pairs with `- macro-guard:`, `:::keyType:::` with a `keyType` column; you never reshape a name to please the engine. Two fences may use the same token name; each shape's own suppliers feed its own occurrences, so an outer shape's binding never leaks into a wrapper's token of the same name.

**`:::list:::` is the one expansion token.** A block's occurrence count is its arity: each occurrence takes one source, its items joined by that source's separator. Where the marker sits decides the axis:

- at **column 0** — vertical: the join fills line by line, each line indented by the source line's `>` count
- **inside a line** — horizontal: the join lands in place, unindented

```
struct :::name:::          vertical — items stack, indent from the wiring
{
:::list:::
};

{ :::list::: },            horizontal — items join in place, e.g. by ", "
```

Every other `:::token:::` is a named token, replaced by the binding, map row, or column of that name (see Maps under The Manifest). A token carries no operation — `:::token:op:::` is not a form. Do not author the quotes around a literal. `toLiteral` supplies them, and doubling up produces `""value""`.

Joining more than one item from a single-line shape aligns their token columns: fill spaces land in the literal between two tokens, right after that literal's first run of whitespace, sized to the widest replacement any item in the join gives that token. Fill never lands inside a token's own replacement, so a quoted include path still emits verbatim. Nothing pads before the first token or after the last, and no emitted line carries trailing whitespace. A single item, or a multi-line shape, renders unpadded — there is no column limit and no wrapping.

---

## The Manifest

`CAST.md` reserves four column names:

```
+------+-----------+-----------+------+
| list | separator | structure | file |
+======+===========+===========+======+
```

`separator` is optional. Blank means newline.

A wiring table is any manifest table with a `structure` column — `## output` and `## output index` in every project here. Nothing else is wiring: your index, `## toolchain`, and any plain data table you keep in the manifest are data, and a column named `file` in them is just a column. The manifest may declare itself in its own index, so a data table can live right beside the wiring that reads it — declare `| @headers | CAST.md |` and `@headers:headers` addresses a `## headers` table in the manifest.

### list — what iterates

`- list: <source>` lines. Each one paired with a structure `- list:` line is an expansion; the source says what feeds it:

- an address — `- list: @xml:XmlTokenType:key` iterates that table's rows; when the addressed table carries a `file` column, the row matching the writing row's own file is excluded — a file never lists itself, same as the column-name form below
- a column address — `- list: @colours:colours:key` names one column of the enclosing expansion's table; it feeds an **inline** `:::list:::`, never a column-0 one
- a column name — `- list: file` iterates the unique values of that column, in first-appearance order, excluding the value belonging to the writing row's own file: a file never lists itself
- a binding name — `- list: instance` selects the rows that declare a binding of that name, blank or valued, walking the wiring tables in manifest order and rows in authored order

Declaring the binding is what selects the row — a blank cell only changes where its value comes from. Bindings inside a wrapper's chain (see Wrappers) are the wrapper's private render data and are never selected. A bare source name is tried as a column first, so never give a selector the same name as a wiring-table column. A row may declare several selector bindings; each name selects its own rows, and two selectors on one row feed two slots by ordinal like any two sources.

A `- list:` line with no structure partner at its `>` count is a **map** for that count's shape paragraph — see Maps.

### structure — what shape

Two lines name a shape. `@code:namespace` renders it once; `- list: @code:<id>` renders it once per item of the line's source. Both are sources: each fills one `:::list:::` of the shape above it. A named bullet binds one token of the nearest shape line above it: `- name: jam` fills its `:::name:::`.

A named bullet whose value is a shape address is a **wrapper** — a third way a shape enters, through a named token instead of a `:::list:::` slot. See Wrappers below.

### Arity — how deep

A shape's arity is how many times its block names `:::list:::`. It consumes that many of the source lines that follow, in order, and each of those consumes its own arity first. Nesting comes from the template; you never author it. A wrapper counts like a shape line, not a source: it consumes its own arity's worth of lines and fills a named token — it never occupies a `:::list:::` slot.

### Indent — where

`> ` count is indentation, nothing else:

```
- list: ...            renders at column 0
> - list: ...          renders at one tab
> > > - list: ...      renders at three tabs
```

One tab is four spaces, **absolute** — measured from column 0 of the output file, not from the enclosing shape. The list and separator columns carry the same count as the structure line they pair with, and it is read: a line pairs with the line carrying the same `>` count at the same ordinal within that count. One symbol, one meaning, in all three columns.

### Pairing — always by order

The list column's `- list:` lines pair with the structure column's `- list:` lines by `>` count and by ordinal within that count. A shape's `:::list:::` occurrences, top to bottom, take the sources that follow it in the same order.

```
+-------------------------------------+---------------------------------+
| list                                | structure                       |
+=====================================+=================================+
| > > > - list: @tokens:token type    | @code:namespace                 |
| > > - list: @tokens:token type      | - name: map                     |
|                                     | @code:bimap                     |
|                                     | - name: TemplateTokenType       |
|                                     | - type: int                     |
|                                     | > > > - list: @code:map-entry   |
|                                     | > > - list: @code:enum          |
+-------------------------------------+---------------------------------+
```

The namespace names `:::list:::` once, so it takes the next source — the bimap, rendered once at column 0. The bimap names it twice — map region first, enum region second — so it takes the next two: map-entry at three tabs, then enum at two. Matching template, tables and expression is yours; CAST reads the expression and generates. Its one check is the count — a mismatch stops the run and names the block and the row.

### Wrappers — a shape in a named token

Sometimes a slot needs a name. A fence whose slots are three anonymous `:::list:::` says nothing about which one is the guard; give the slot its own token and fill it with a **wrapper** — a binding whose value is a shape address:

````markdown
```generated
struct Generated
{
:::list:::
:::macro-guard:::
};
```

```macro-guard
#if :::macro:::
:::list:::
#endif // :::macro:::
```
````

```
+--------------------+----------------------------------+----------------+
| list               | structure                        | file           |
+====================+==================================+================+
| - list: file       | @code:generated                  | @jam_Generated |
| > - list: instance | - macro: #pragma once            |                |
|                    | - list: @code:include            |                |
|                    | > - list: @code:shared-instance  |                |
|                    |                                  |                |
|                    | - macro-guard: @code:macro-guard |                |
|                    | - macro: @guiBasics              |                |
|                    |                                  |                |
|                    | > @code:shared-instance          |                |
|                    | > - type: ColourId               |                |
|                    | > - instance: colourId           |                |
+--------------------+----------------------------------+----------------+
```

`- macro-guard: @code:macro-guard` fills `:::macro-guard:::` with the guard shape's rendering. From that line on, the wrapper reads like any shape line: the bullets after it bind **its** tokens (`- macro: @guiBasics` fills the guard's `:::macro:::`, not generated's — each shape reads its own), and it consumes the next source lines up to its own arity — here the bare `shared-instance`, indented one tab, named by its own bindings. Same arity, same scope, same indent law as every other wrapper; the guard's one `:::list:::` collapses a whole group when you feed it an expansion instead of a bare line.

Everything the wrapper consumes is its private render data. Its bindings select nothing — `- instance: colourId` above names the guarded member without ever entering the `- list: instance` member list.

### Maps — straight replacement

A block full of named tokens and no rows to iterate — a build manifest, a config file — reads its values from a table one row per token:

```
+---------------------------------+-----------------+--------------+
| list                            | structure       | file         |
+=================================+=================+==============+
| - list: @project-info:cmake     | @cmake:cmake    | @CMakeLists  |
| > - list: @project-info:module  | > - list: @cmake:module |      |
+---------------------------------+-----------------+--------------+

## cmake
| key            | value  |
| minimumVersion | 4.2.0  |
| cxxStandard    | 17     |
```

`:::minimumVersion:::` in the `cmake` block takes the `value` of the row whose first column is `minimumVersion`. The first column is the key — `key | value`, or `name | type | value | comment`, any table whose first column is an identity. The same table under a `- list:` expansion is rows; under a shape paragraph it is a map. The reader decides, never the table.

Map lines are the list column's `- list:` lines at a `>` count beyond the structure column's `- list:` lines at that count, first in order — column-address lines (`@file:table:column`, the inline sources) are never counted; the address form tells them apart. They belong to that count's shape paragraphs in order; a blank `- list:` closes one paragraph's group and starts the next, and fewer groups than paragraphs fill the last paragraphs — the same slot law as expansions. A different `>` count is a different scope. A map line with no paragraph at its count stops the run; so does a map table with no `value` column.

### separator — how items join

`- list:` lines mirroring the list column's. The line at a given ordinal joins that expansion's items. Blank or absent joins by newline; anything else joins by that text — `@code:<id>` names a block whose text is the join, `@space` an index datum (`U+0020` with `fromUTF8`) for a join of one space.

The leading `>`-less separator line is the row join: rows merged into one file join their diverging values by it.

### file — merging rows into one file

Rows that declare the same `file` render as one merged shape; the wrap is emitted once per file. Merging is per line: at each structure line, rows that carry the same value emit it once, and rows whose values differ emit each value in authored row order, joined by the first row's row-join line, framed by one blank line on each side. A group of one row merges to itself.

Same-file rows must be authored contiguously — a row for a file already closed by an intervening row of another file is a fatal error.

### comment — file documentation, wired to a table

The wiring table's own `comment` column doubles as file documentation. Rows sharing one `file` render as one merged output (see File above); CAST reads the `comment` cell of that group's **first** row, by first appearance in authored order — the same rule that decides output-file ordering — and writes the resolved text between the banner and the file's own text, in the file's comment syntax. A blank first-row `comment` writes nothing extra.

That cell is either the fenced prose directly, or a whole-cell table address — `@file:table` — naming a table with `file` and `comment` columns. CAST reads that table's row whose `file` matches the group's own output file, and takes its `comment` cell as the header. This is the pattern for real projects: one small `## headers` table holds every file's header prose in one place, and every output row's `comment` cell just points at it.

```markdown
## headers

| file    | comment                         |
| Out.h   | ```                             |
|         | @file Out.h                     |
|         | @brief One generated namespace. |
|         | ```                             |

## output

| list                  | separator | structure              | file    | comment           |
| - list: @data:rows    |           | @code:namespace        | @Out.h  | @headers:headers  |
|                       |           | - macro: #pragma once  |         |                    |
|                       |           | - name: Out             |         |                    |
|                       |           | - list: @code:entry     |         |                    |
```

Every output row that needs a header points the same `@headers:headers` address at the one table; the file match, not the row, decides which prose comes back. Declaring `## headers` as a table CAST also lists an output's includes from (`- list: @headers:headers`, per "Declared membership instead of a derived sweep" below) needs no extra care: the self-exclusion law (a file never lists itself) already keeps a file's own row out of its own include sweep, even though the same row supplies that file's header.

The same `comment` cell is still an ordinary column: any item shape sourced from that row reads it for its own `:::comment:::` too — this is unchanged from before the table form existed. The `@` sigil law (a `@`-sigiled cell is a reference, never data) settles what happens when one row does both jobs: an address-valued `comment` cell is a reference, so a reader that renders prose skips it and gets nothing — only the file-documentation reader above resolves the address. A row that is both a merge-group's first row and separately selected elsewhere as an item therefore supplies the file header correctly and contributes no per-item prose from that same cell.

If that row still needs its own per-item prose, give it a `- comment: <text>` binding in the structure column — plain text, no `@`. The item-shape reader finds the binding before it ever reaches the (address-valued, empty) column, so the bound text renders exactly where the plain-column text used to. This binding never reaches the shape's own `:::comment:::` — that marker keeps reading the list-column comment reference or the table's documentation, same as always, regardless of what the row's structure column binds.

### toolchain — commands after the write

`## toolchain` is an optional table, `| argument | command | flag |`, reserved by name — not by file. CAST looks it up across every file the manifest's index declares, so a project that keeps its codegen manifest and its toolchain data apart declares `## toolchain` in the data file, not `CAST.md` itself. Its rows run after every output has written, in authored order, one child process per row. A failing row fails the run — never the writes already on disk.

`argument` is optional and, when declared, selects which rows run. A blank `argument` cell marks a default-flow row; `cast CAST.md`, with no trailing flag, runs only those. `cast CAST.md --<word>` runs only the rows whose `argument` cell equals `word` instead — a `word` matching no row is fatal.

```
+----------+--------+---------------------------------------------------------+
| argument | command | flag                                                    |
+==========+========+===========================================================+
|          | cmake  | -S . -B Builds/Ninja -G Ninja -DCMAKE_BUILD_TYPE=Release |
+----------+--------+---------------------------------------------------------+
|          | cmake  | --build Builds/Ninja                                     |
+----------+--------+---------------------------------------------------------+
| debug    | cmake  | -S . -B Builds/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug   |
+----------+--------+---------------------------------------------------------+
| debug    | cmake  | --build Builds/Debug                                     |
+----------+--------+---------------------------------------------------------+
```

`cast CAST.md` configures and builds `Builds/Ninja` in Release. `cast CAST.md --debug` configures and builds `Builds/Debug` in Debug instead — the two default-flow rows do not run.

---

## Operations

Operations are optional. A table may carry finished text and use none at all.

**Case** — `toUpper`, `toTitle`, `toPascal`, `toCamel`, `toKebab`, `toSnake`, `toScreamingSnake`

An all-uppercase word is an abbreviation and survives every case operation in every position: `UI scale` becomes `UIScale`, `fail hazard URI` becomes `failHazardURI`.

**Encoding** — `toLiteral`, `toUTF8`, `fromUTF8`, `toHex`, `toCodepoint`, `fromCodepoint`

`toLiteral` produces a complete string literal: it quotes the value, escapes backslashes and quotes, turns control characters into their named escapes, and turns bytes above `0x7F` into hex escapes. Quoting and escaping are one operation and never travel separately. Writing a cell in backticks is the same thing, spelled shorter.

Write the real character, not its escape. A line break in the datum comes out as `\n`; a `"` comes out as `\"`; `©` comes out as `\xc2\xa9`.

An escape you cannot write as a real character is written as its escape sequence, and `toLiteral` passes it through: a backslash followed by a data-declared escape character is an authored escape and survives verbatim — `\n` authors a line break. A backslash followed by a backslash is one literal backslash, doubled on output — `\\n` authors the two characters backslash-n. Both stay expressible.

`fromUTF8` is the exception, for the one character you cannot write: a space at the very start or end of a line, which the formatter's padding would swallow. Write `U+0020` and the operation decodes it back to a space, leaving every other byte alone.

A `format` cell names one operation, never two. The one composition CAST performs is its own — a backticked or fenced cell is quoted and escaped first, and the cell's operation then applies to that value. That is how `fromUTF8` reaches inside a finished literal.

**Text** — `join`, `toFileName`

**Comment** — `toComment`, `toCommentBlock`, `brief`

The comment family exists for the banner CAST stamps and for the `:::comment:::` marker, both formatted for the output file's language.

---

## Cookbook

### Constants from a declaration table

```markdown
## identifiers

+------+--------------------+-----------------------+-----------+
| type | name               | value                 | format    |
+======+====================+=======================+===========+
| @id  | circleCross        | circleCross            | toLiteral |
| @id  | applicationSupport | `Application Support` |           |
| @id  | cdataOpen          | `<![CDATA[`           |           |
+------+--------------------+-----------------------+-----------+
```

````markdown
```identifier
inline const :::type::: :::name::: { juce::String::fromUTF8 (:::value:::) };
```
````

```cpp
inline const juce::Identifier circleCross { juce::String::fromUTF8 ("circleCross") };
inline const juce::Identifier applicationSupport { juce::String::fromUTF8 ("Application Support") };
inline const juce::Identifier cdataOpen { juce::String::fromUTF8 ("<![CDATA[") };
```

Row one's `value` is authored plain text, quoted by its `toLiteral` format. Rows two and three are backticked, which already means `toLiteral` — so their `format` cell stays empty. A blank `value` cell would render an empty string, not `circleCross` — every value a row needs is authored on that row.

### Any-arity entries with explicit column addresses

```markdown
## colours

+-----+------------+
| key | value      |
+=====+============+
| 0   | 0xff000000 |
| 1   | 0xffcd0000 |
+-----+------------+
```

````markdown
```entry
{ :::list::: },
```
````

```
+---------------------------------------+-------------------+-------------------------------+
| list                                   | separator         | structure                     |
+=========================================+===================+===============================+
| > > - list: @colours                   | > > - list: `, `  | ...                           |
| > > - list: @colours:colours:key       |                    | > > - list: @code:entry       |
| > > - list: @colours:colours:value     |                    |                                |
+---------------------------------------+-------------------+-------------------------------+
```

```cpp
        { 0, 0xff000000 },
        { 1, 0xffcd0000 },
```

Rows stack vertically at two tabs; `@colours` is the entry's own source. The two column-address lines that follow it, at the same two tabs, name `key` then `value` explicitly — one line per column — and join horizontally into the inline `:::list:::` by the separator authored at the source line's own ordinal, `, `. A third column tomorrow is one more address line, same template.

### Wrapping a shape in another

```markdown
## output

+--------------------------+--------------------------------+----------+
| list                     | structure                      | file     |
+==========================+================================+==========+
| > - list: @xml:token:key | @code:namespace                | @jam_Xml |
|                          | - name: jam                    |          |
|                          | @code:bimap                    |          |
|                          | - name: XmlTokenType           |          |
|                          | > - list: @code:pair           |          |
+--------------------------+--------------------------------+----------+
```

The namespace names `:::list:::` once, so it takes the next source — the bimap, rendered once at column 0. The bimap names it once, so it takes the wiring line: rows of `@xml:token:key`, each through `@code:pair`, at one tab.

### Guarding one member of an expansion

The Wrappers section's example, end to end, is the pattern for "everything in this list, plus one that only exists behind a macro": the plain members come through the `- list: instance` selector, the guarded one comes through the `:::macro-guard:::` wrapper with its own bindings, and the output is

```cpp
struct Generated
{
    jam::SharedInstance<Screen> screen { std::in_place };
    ...
#if JUCE_MODULE_AVAILABLE_juce_gui_basics
    jam::SharedInstance<ColourId> colourId { std::in_place };
#endif // JUCE_MODULE_AVAILABLE_juce_gui_basics
};
```

One guard, one member — and because the wrapper's slot is an ordinary slot, feeding it `> - list: <selector>` instead of a bare line collapses any number of same-macro members into the one region.

### Declared membership instead of a derived sweep

When a derived source starts sweeping in rows you never meant — `- list: file` collecting a build manifest into an include list — declare the membership as data, in the manifest itself:

```markdown
| @headers | CAST.md |          in ## index — the manifest indexes itself

## headers

| file          |
| ProjectInfo.h |
| Identifiers.h |
```

and wire `- list: @headers:headers`. The list is now exactly what the table says, and the next member is one row.

### Named commands, wired as wrapper tokens

A generated build step — `codesign`, `notarize`, an install copy — is a command that
never changes shape; only its parameters (an identity, a path, a profile) come from
data. Author the command once, as its own named fence, with the parameters as ordinary
tokens:

````markdown
```codesign
COMMAND codesign --force --options runtime --entitlements "${CMAKE_SOURCE_DIR}/:::entitlementsPath:::" --sign ":::identity:::" $<TARGET_FILE:${PROJECT_NAME}>
```
````

Wire it as a wrapper — a named binding, not a `:::list:::` slot — so the frame around it
stays one constant, dumb `add_custom_command`:

```
| list | separator | structure                    | file        |
|      |           | - codesign: @cmake:codesign  | @CMakeLists |
```

`:::identity:::` and `:::entitlementsPath:::` resolve the same way any other token
does — through the row's own maps (`- list: @project-info:signing` declared earlier in
the same wiring row). Deleting the wrapper line deletes the step; the command text
itself is never duplicated, never baked into the frame, and never repeated per
platform or per build.

### One table, two configurations, two compilers

A per-platform, per-configuration flag set needs neither a table per platform nor a
table per configuration — one table, split by a cell-match filter (see Filtering rows
by a cell) on a `stage` column that tells a compile flag from a link flag:

```markdown
## release

+-------------------+--------------+----------+--------+
| name               | mac          | win      | stage  |
+====================+==============+==========+========+
| optimization       | -O3          | /O2      |        |
| deadCodeStripping   | -dead_strip  | /OPT:REF | linker |
+--------------------+--------------+----------+--------+

## debug

+---------------+------+------+--------+
| name          | mac  | win  | stage  |
+===============+======+======+========+
| optimization  | -O0  | /Od  |        |
| debugSymbols  | -g   | /Zi  |        |
+---------------+------+------+--------+
```

```
+---------------------------------------------+---------------------+---------------------+
| list                                         | separator            | structure            |
+===============================================+=====================+=======================+
| - list: @project-info:release:stage=         | - list: @semicolon  | - list: @cmake:mac    |
| - list: @project-info:release:stage=linker   | - list: @semicolon  | - list: @cmake:mac    |
| - list: @project-info:debug:stage=           | - list: @semicolon  | - list: @cmake:mac    |
+-----------------------------------------------+---------------------+-----------------------+
```

Each wiring line names the same table twice, split only by what its `stage` filter
selects — the release compile flags, the release link flags, the debug compile
flags — mac and win rows the same way, both read through their own column in the
`@cmake:mac` / `@cmake:win` shape. A row with a blank `mac` or `win` cell contributes
nothing to that platform's join — nothing is nothing, not an omitted row.

---

## Canonical Markdown

Every declared markdown file is rewritten to canonical form, write-if-different.

- layout only — cell content, backtick literals, fenced cells, row order and authored borders all survive
- borders are re-emitted exactly where you authored them; the formatter neither adds nor removes one
- columns pad to their widest cell, on every line of every cell, a fence's content lines included — the right edge is always aligned, and `|this|` comes back as `| this |`
- `format (format (x)) == format (x)` — a canonical file reformats to itself, byte for byte
- a malformed table is reported with its `path:line` and the file is never rewritten

---

## Determinism and Failure

Output bytes are determined by your tables, your templates, your manifest, and the binary. No timestamps, no paths, no host state. Files use LF.

Write-if-different: a second run produces an empty diff.

There are no warnings. Every failure is fatal, exits non-zero, and writes no output file. Diagnostics name the true physical line in the file, not the row's ordinal position:

```
identifiers.md:412 (name): duplicate "circleCross"
CAST.md:133 (structure): template not found: namespace
CAST.md:36 (structure): nested shape has more than one candidate
```

Because CAST runs during the configure phase, a failure stops your build before compilation starts.

---

## CMake Integration

- **No discovery.** Carry the exact per-platform `cast` binaries. Do not use `find_program()`. Do not assert versions.
- **Source of truth** for the binary is `~/Documents/Poems/dev/cast`.
- **Invocation** is `codegen.cmake`, included before `project()`.
- **Dependencies** for `CMAKE_CONFIGURE_DEPENDS` come from the manifest — never hand-maintained.
- **Role.** CMake is a dispatcher. It never implements generation logic.
