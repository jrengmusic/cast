# CAST: Codegen Annotated Source of Truth

**Version 0.1.0** — this guide tracks SPEC version 0.9.

You see this guide because you ran `cast --help`, or because `cast` could not find a `spell.md` in the current directory.

CAST reads your data (tables), your shapes (templates), and your wiring (the manifest), and writes files. It knows nothing about any programming language. It resolves references, iterates rows, and replaces tokens — nothing else.

This guide tells you how to use CAST. `SPEC.md` fixes the rules that this guide describes. Where this guide and SPEC disagree, SPEC is right and this guide is wrong.

---

## Command Line

Arguments select what runs. They never carry generation rules.

- `cast` — find `spell.md` here, format each declared markdown file, regenerate each declared output, then run the default-flow `## toolchain` rows
- `cast <path>/spell.md` — the same, with a specific manifest
- `cast spell.md <directory>` — write each declared output under that directory in place of the declared paths
- `cast spell.md --format` or `cast --format spell.md` — format only, no generation; the flag reads before or after the manifest
- `cast spell.md --no-format` or `cast --no-format spell.md` — generate only, no formatting
- `cast spell.md --<word>` — after format and generate, run only the `## toolchain`
  rows whose `argument` cell equals `word`; the default-flow rows do not run
- `cast --version` — the version and the source commit, the same stamp that the generated banners embed
- `cast --help` — this guide
- `--max-table-width n` and `--line-wrap n` — each a value pair, in any position on
  the line; each composes with `--format`, `--no-format` and the manifest argument.
  `--line-wrap` must be a positive integer; `--max-table-width` must be a
  non-negative integer — any other value is fatal. `--line-wrap` defaults to 100,
  `--max-table-width` to 0

The default order is format, then generate, then the default-flow toolchain rows. `--format` and `--no-format` exclude each other. Each also excludes an output directory and a `--<word>` toolchain argument. One manifest, one flag, nothing else on the line.

---

## Sync

`cast --sync <source-root> <target-root>` mirrors one framework's kernel onto another's. It composes with nothing else on the line — no manifest, no other flag — and a `--sync` line without exactly its two roots is fatal.

Each root carries a `user-modules-info.md` file at its top level: `## identity` (`key | value | boundary`), `## module` (`name | class`, plus data columns manifest wiring reads, never sync), and `## ignore` (`value`). The two roots must differ. Sync reads exactly these three tables — any other table in the file belongs to the manifest and is never read by sync.

The transform is one ordered replacement list, longest source first — sources of equal length order by their text, descending, and sources with equal text keep their authored row order, so the list is total. Each `## identity` key present in both files contributes a pair — source value to target value — except `namespace`, which contributes no plain pair: its bare word occurs inside unrelated names, so it never replaces on its own. When two or more rows share a byte-equal source value, the first authored row contributes the pair and the later rows contribute none. Three keys also compose, and each must be present in both files: `namespace` adds exactly two pairs, `namespace <source>` to `namespace <target>` and `<source>::` to `<target>::`; `filePrefix` and `macroPrefix` contribute their plain pairs, and `filePrefix` also transforms every path segment — directory and file names alike. A row whose `boundary` cell is `word` matches whole words only; every other row matches plain text.

Sync visits the source's `kernel` rows — every other class value is provision. A `kernel` row names a directory, and at each root, every kernel row's directory must exist, and every `<filePrefix>*` directory on disk must be declared. After the path transform, the two files' kernel sets must correspond one to one. Root-relative paths use `/` on every host. A file whose root-relative path matches an `## ignore` row (`*` wildcards) is skipped. A source file that cannot be read is fatal. A file with a NUL byte in its first 8000 bytes copies byte-for-byte. Every other file transforms as text and normalizes to LF. A `.md` file re-canonicalizes through the formatter after the transform only when its kernel scope's own name carries no `filePrefix` — the data scopes, never the module directories. Every write is write-if-different, and a write or delete that fails is fatal. Before a write, a target file whose on-disk name differs from the transformed path only by case is renamed to the transformed path, so mirror-delete compares exact names on every host; a rename that fails is fatal as an output that cannot be written.

A source text file that already contains a pair's target value is fatal — the transform must be invertible, and a target token on the source side proves it is not.

A target file inside a kernel scope with no transformed source counterpart and no `## ignore` match is deleted. `provision` rows are never walked, never written, never deleted from.

The report is the written paths and the deleted paths, one per line, nothing else. Zero lines is the pass: the roots are in sync.

---

## The Three Artifacts

Everything lives flat in one data directory.

| Artifact  | What it is                                               |
| --------- | -------------------------------------------------------- |
| `spell.md`| the manifest — what gets generated, from what, into what |
| `*.md`    | data tables                                              |
| `*.cast`  | template files — any number, each holding code shapes    |

The manifest's index declares each input file one time. CAST parses exactly what you declare — it never scans a directory. Generated outputs are build artifacts. Do not edit them.

---

## Writing Tables

### Rows

CAST decides a table's row boundaries one time, for the whole table:

- if the body has a border between rows, borders delimit rows
- if it has none, and no line starts with an empty first cell, each `|` line is one row
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

Four column names are reserved everywhere: `format`, `comment`, `brief`, and `value`. Inside the manifest's wiring tables, four more are reserved: `list`, `separator`, `structure`, `file`. A data table — wherever it lives, the manifest included — can use any of those four words as an ordinary column.

### Cells

A cell is one of these forms:

| Cell           | Meaning                                                                       |
| -------------- | ----------------------------------------------------------------------------- |
| plain text     | that text, verbatim                                                           |
| `` `text` ``   | `toLiteral` of those bytes — quoted and escaped, ready to drop into a literal |
| a fenced block | the same, over a datum that spans lines                                       |
| empty          | nothing — an empty string                                                     |

An empty cell is not an error and is not a special case. It is string replacement as nothing, in every table, reserved or not. A token that it fills renders empty. A template line left empty by its only placeholder elides for free, exactly like each other emptied placeholder line. An empty `format` cell formats nothing: CAST writes the value verbatim.

Backticks are not decoration and are not an escape. A backtick **is** the `toLiteral` operation, thus a backticked cell never needs a `format` column beside it.

A code span cannot hold a line break. When the datum has one, author the cell as a fence. Each line between the delimiters is one line of the value, and line breaks join the lines:

```
+-------------------------------+-----------------------------------+
| alertNewerVersionPresetSuffix | ```                               |
|                               |                                   |
|                               | To use this preset correctly:     |
|                               | ```                               |
+-------------------------------+-----------------------------------+
```

That cell's value begins with a line break, because the fence's first content line is empty. Write the real characters. `toLiteral` turns them into `\n` for you.

Pipes delimit a cell on each line that the cell spans, and the padding between them is never data — `|this|` and `| this |` are the same datum, and the formatter rewrites the first as the second. That holds inside a fence too.

Thus you cannot write a space at the very start or end of a line as a space — it would look the same as padding. Write it as `U+0020` and put `fromUTF8` in the row's `format` cell:

```
+-------------------------------+------------------------------------------+----------+
| alertNewerVersionPresetSuffix | ```                                      | fromUTF8 |
|                               |                                          |          |
|                               | please install the newest version:U+0020 |          |
|                               | ```                                      |          |
+-------------------------------+------------------------------------------+----------+
```

`fromUTF8` decodes each `U+XXXX` token in the value and touches no other byte.

To put a `|` inside a cell, write a backslash before it. The scanner eats that backslash when it rejoins the split. Thus a datum that needs a backslash _and_ a pipe is written with two — ```\\|``` yields the two bytes `\|`.

### Format

`format` is a column. It binds to the column immediately to its left, and it names **exactly one** operation. It is optional — CAST writes a column with no `format` beside it exactly as authored.

```
| key | format | value | format |
| key | format | value |
| key | value  | format |
| key | value  |
```

CAST rejects `| format | format |` — a format has no format.

No chaining exists. If a datum needs a different shape, author it in that shape:

```
+------+-------------+-------------+-----------+
| type | name        | value       | format    |
+======+=============+=============+===========+
| @id  | circleCross | circleCross | toLiteral |
+------+-------------+-------------+-----------+

name  ->  circleCross      verbatim
value ->  "circleCross"    circleCross, quoted and escaped by toLiteral
```

The table declares the formatting. A template never formats anything.

### Documentation

Three doc channels exist, all data:

- **row** — a column named `comment`. An item shape's `:::[comment]:::` takes it like each other column.
- **table** — write a paragraph or a fenced block between the `## table name` heading and the table. A shape-level `:::[comment]:::` falls back to it when no reference names something else.
- **fence** — a fenced block that carries an info string, anywhere in a data file, not bound to a table. The info string is its name, and the fence text is its prose. Address it — or a table — by name with a `- [comment]: @file:<name>` bullet in the manifest's list column, at the shape line's own `>` count. A wrapper shape declared across several rows carries the same reference on each declaring row, exactly like a binding.

At shape level, with no comment reference authored, `:::[comment]:::` falls back to the documentation of the first table that the shape's own sources address, in authored order. With no source that addresses a table, it falls back to the documentation of the row's own table.

You never author the comment frame (`/** @brief ... */`, `///< ...`) yourself — you write prose only. CAST renders the frame from the comment-syntax table, keyed by the output file's extension — except when the file's exact name has a row in the manifest-syntax table (`CMakeLists.txt` is CMake, not text; that row's value replaces the extension as the key):

- the marker alone on its line — block form. Multi-line prose renders one prose line per output line. Single-line prose renders open, text, close on one line.
- the marker inline, after content — single-line form: the language's comment glyph, then the text, its lines joined by one space.

A missing comment is not an error — the marker renders empty, and an emptied line trims or collapses like each other placeholder line.

### The banner

Each generated file carries the CAST banner, in that file's own comment syntax, with the file's documentation joined to it. You never author it.

By default it goes at the top. Write `:::[banner]:::` in a template and it goes there in place of the top — wherever the marker sits, in any shape, one time per occurrence. That is how a shell script keeps `#!` on line 1 and an XML file keeps its declaration on line 1: the template holds line 1, thus the template places the banner.

A language with no block comment — shell, TOML, YAML — gets its single-line marker on each banner line. A file whose render uses a fence marked `[no-banner]` gets no banner at all.

CAST writes an output whose extension names no comment syntax — or which has no extension — in C syntax. CAST does not refuse it.

### Uniqueness

Within one table, each identity column's entries — `name`, `key`, `alias`, `file` — must be unique, compared byte for byte. `circleCross` and `CircleCross` are two different entries. Each other column is payload — `value`, `type`, `format`, `comment` — and payload repeats by design: many rows can map to the same payload. The manifest's wiring tables are exempt — wiring repeats templates, separators and files by design.

---

## References

The `@` sigil is part of the name. A cell that starts with `@` is a reference. Anything else is data. You cannot confuse the two.

### The index

Any file can declare `## index`. It is file-local — a `@` cell resolves against the index of the file that it is written in, never another file's. It can carry a `format` column like each other table — an index datum is formatted like a cell (`@space` declares `U+0020` with `fromUTF8`).

```
## index

+---------+------------------+
| alias   | symbol           |
+=========+==================+
| @id     | juce::Identifier |
| @string | juce::String     |
+---------+------------------+
```

The manifest's own index declares the input files:

```
| @identifiers | identifiers.md |
```

### Addressing

```
@file : table : column
```

Write the table part only when you need it. You can omit it when the table has its file's name, or when the file holds exactly one table — the index never counts for that purpose.

CAST stops with an error if an alias is undeclared, declared two times, points at a missing file, or names a table or column that does not exist.

### Filtering rows by a cell

```
@file : table : column = value
```

Add `=value` to the column part, and the address selects only that table's rows whose `column` cell equals `value`, byte for byte. `value` can be empty. An empty `value` matches a row whose cell is itself blank — nothing (see Cells). Thus one table serves two disjoint row sets — a compiler-stage split, say — without a second table:

```
+-------------------+-------------+----------+--------+
| name              | mac         | win      | stage  |
+===================+=============+==========+========+
| optimization      | -O3         | /O2      |        |
| deadCodeStripping | -dead_strip | /OPT:REF | linker |
+-------------------+-------------+----------+--------+
```

`@project-info:release:stage=` selects the compiler rows (`stage` blank). `@project-info:release:stage=linker` selects the linker rows — one table, two wiring lines, no duplication.

---

## Templates

Code shapes live in `.cast` files — as many as you declare in the index. Each is markdown: fenced code blocks, and the fence's info string is the block's name. A shape is addressed like each other reference: `@code:namespace` is the `namespace` block of the file that the `@code` alias names. Two files can both have an `entry` block — `@code:entry` and `@cmake:entry` are different shapes.

A fence name can open with one bracket group — `[sh]script`, `[no-banner]script`. The group is part of the name and part of the address, matched byte for byte: `@code:[sh]script`. Nothing after the group is read, thus `[sh]-script` is a different, equally legal name. Two words are legal inside it: an extension without its dot, which tells CAST what language the file is and outranks the file's own extension; or `no-banner`, which means the file carries no banner. Any other word stops the run.

````markdown
```identifier
inline const :::type::: :::name::: { juce::String::fromUTF8 (:::value:::) };
```

```entry
{ :::[list]::: },
```
````

A block is the literal text of the output. Braces, keywords, punctuation — all authored, all verbatim. No conditionals, no loops, and no formatting exist. Target-language directives (`#if`, `#endif`) are literal text like everything else — CAST never reads them, and slots inside such an arm take sources by the ordinary arity law. Comment frames are the one exception: CAST renders them itself, from the comment-syntax table (see Documentation) — never author one in a template.

Square brackets mark a name that CAST reserves — `:::[list]:::`, `:::[comment]:::`, `:::[banner]:::`, and the bullets `- [list]:`, `- [comment]:`, `- [begin]:` and `- [end]:`. Everything without brackets is yours, thus `:::list:::` and `- comment:` are ordinary names of your own. Column names are the exception: a table already scopes its columns, thus reserved column names carry no brackets.

Token names are yours — free text, matched exactly as you wrote them. `:::macro-guard:::` pairs with `- macro-guard:`, and `:::keyType:::` pairs with a `keyType` column. You never reshape a name to please the engine. Two fences can use the same token name — each shape's own suppliers feed its own occurrences, thus an outer shape's binding never leaks into a wrapper's token of the same name.

**`:::[list]:::` is the one expansion token.** A block's occurrence count is its arity: each occurrence takes one source, its items joined by that source's separator. Where the marker sits decides the axis:

- at **column 0** — vertical: the join fills line by line, each line indented by the source line's `>` count
- **inside a line** — horizontal: the join lands in place, unindented

```
struct :::name:::          vertical — items stack, indent from the wiring
{
:::[list]:::
};

{ :::[list]::: },            horizontal — items join in place, e.g. by ", "
```

Each other `:::token:::` is a named token. The binding, map row, or column of that name replaces it (see Maps under The Manifest). A token carries no operation — `:::token:op:::` is not a form. Do not author the quotes around a literal. `toLiteral` supplies them, and a doubled pair produces `""value""`.

A join of more than one item from a single-line shape aligns their token columns. Fill spaces land in the literal between two tokens, right after that literal's first run of whitespace. The fill size matches the widest replacement that any item in the join gives that token. Fill never lands inside a token's own replacement, thus a quoted include path still emits verbatim. Nothing pads before the first token or after the last token, and no emitted line carries trailing whitespace. A single item, a multi-line shape, or a single-line shape that carries an inline `:::[list]:::` renders unpadded — no column limit and no wrapping exist.

---

## The Manifest

`spell.md` reserves four column names:

```
+------+-----------+-----------+------+
| list | separator | structure | file |
+======+===========+===========+======+
```

`separator` is optional. Blank means newline.

A wiring table is any manifest table with a `structure` column — `## output` and `## output index` in every project here. Nothing else is wiring: your index, `## toolchain`, and any plain data table that you keep in the manifest are data, and a column named `file` in them is just a column. A data table can live right beside the wiring that reads it — an address whose first part names no index alias names a table of the writing file itself, thus `@headers` in the manifest addresses a `## headers` table in the same file. Aliases are scoped to the index. A manifest never aliases its own file.

### list — what iterates

`- [list]: <source>` lines. Each one paired with a structure `- [list]:` line is an expansion, and the source says what feeds it:

- an address — `- [list]: @xml:XmlTokenType` iterates that table's rows. When the addressed table carries a `file` column, the row that matches the writing row's own file is excluded — a file never lists itself, the same as the column-name form below
- a column address — `- [list]: @colours:colours:key` names one column of the enclosing expansion's table. It feeds an **inline** `:::[list]:::`, never a column-0 one
- a cell-match filter — `- [list]: @file:table:column=value` iterates only that table's rows whose `column` cell equals `value`, byte for byte (see Filtering rows by a cell)
- a column name — `- [list]: file` iterates the unique values of that column, in first-appearance order, and excludes the value that belongs to the writing row's own file: a file never lists itself
- a binding name — `- [list]: instance` selects the rows that declare a binding of that name, blank or valued. It walks the wiring tables in manifest order and each table's rows in authored order

To declare the binding is what selects the row — a blank cell only changes where its value comes from. Bindings inside a wrapper's chain (see Wrappers) are the wrapper's private render data and are never selected. CAST tries a bare source name as a column first, thus never give a selector the same name as a wiring-table column. A row can declare several selector bindings. Each name selects its own rows, and two selectors on one row feed two slots by ordinal like any two sources.

A `- [list]:` line with no structure partner at its `>` count is a **map** for that count's shape paragraph — see Maps.

### structure — what shape

Two lines name a shape. `@code:namespace` renders it one time. `- [list]: @code:<id>` renders it one time per item of the line's source. Both are sources: each fills one `:::[list]:::` of the shape above it. A named bullet binds one token of the nearest shape line above it: `- name: jam` fills its `:::name:::`.

A named bullet whose value is a shape address is a **wrapper** — a third way that a shape enters, through a named token in place of a `:::[list]:::` slot. See Wrappers below.

### Arity — how deep

A shape's arity is how many times its block names `:::[list]:::`. It consumes that many of the source lines that follow, in order, and each of those consumes its own arity first. Nesting comes from the template. You never author it. A wrapper counts like a shape line, not a source: it consumes its own arity's worth of lines and fills a named token — it never occupies a `:::[list]:::` slot.

### Indent — where

`> ` count is indentation, nothing else:

```
- [list]: ...            renders at column 0
> - [list]: ...          renders at one tab
> > > - [list]: ...      renders at three tabs
```

One tab is four spaces, **absolute** — measured from column 0 of the output file, not from the enclosing shape. The list and separator columns carry the same count as the structure line that they pair with, and CAST reads it: a line pairs with the line that carries the same `>` count at the same ordinal within that count. One symbol, one meaning, in all three columns.

### Pairing — always by order

The list column's `- [list]:` lines pair with the structure column's `- [list]:` lines by `>` count and by ordinal within that count. A shape's `:::[list]:::` occurrences, top to bottom, take the sources that follow it in the same order.

```
+------------------------------------+---------------------------------+
| list                               | structure                       |
+====================================+=================================+
| > > > - [list]: @tokens:token type | @code:namespace                 |
| > > - [list]: @tokens:token type   | - name: map                     |
|                                    | @code:bimap                     |
|                                    | - name: TemplateTokenType       |
|                                    | - type: int                     |
|                                    | > > > - [list]: @code:map-entry |
|                                    | > > - [list]: @code:enum        |
+------------------------------------+---------------------------------+
```

The namespace names `:::[list]:::` one time, thus it takes the next source — the bimap, rendered one time at column 0. The bimap names it two times — map region first, enum region second — thus it takes the next two: map-entry at three tabs, then enum at two. To match the template, the tables and the expression is yours. CAST reads the expression and generates. Its one check is the count — a row that supplies more sources than its shapes demand stops the run, and the diagnostic names the block and the row. To supply fewer is legal: your sources fill a shape's trailing slots, and the leading unfilled slots render empty.

### Wrappers — a shape in a named token

Sometimes a slot needs a name. A fence whose slots are three anonymous `:::[list]:::` says nothing about which one is the guard. Give the slot its own token and fill it with a **wrapper** — a binding whose value is a shape address:

````markdown
```generated
struct Generated
{
:::[list]:::
:::macro-guard:::
};
```

```macro-guard
#if :::macro:::
:::[list]:::
#endif // :::macro:::
```
````

```
+----------------------+-----------------------------------+----------------+
| list                 | structure                         | file           |
+======================+===================================+================+
| - [list]: file       | @code:generated                   | @jam_Generated |
| > - [list]: instance | - macro: #pragma once             |                |
|                      | - [list]: @code:include           |                |
|                      | > - [list]: @code:shared-instance |                |
|                      |                                   |                |
|                      | - macro-guard: @code:macro-guard  |                |
|                      | - macro: @guiBasics               |                |
|                      |                                   |                |
|                      | > @code:shared-instance           |                |
|                      | > - type: ColourId                |                |
|                      | > - instance: colourId            |                |
+----------------------+-----------------------------------+----------------+
```

`- macro-guard: @code:macro-guard` fills `:::macro-guard:::` with the guard shape's rendering. From that line on, the wrapper reads like any shape line: the bullets after it bind **its** tokens (`- macro: @guiBasics` fills the guard's `:::macro:::`, not generated's — each shape reads its own), and it consumes the next source lines up to its own arity — here the bare `shared-instance`, indented one tab, named by its own bindings. Same arity, same scope, same indent law as each other wrapper. The guard's one `:::[list]:::` collapses a whole group when you feed it an expansion in place of a bare line.

Everything that the wrapper consumes is its private render data. Its bindings select nothing — `- instance: colourId` above names the guarded member and never enters the `- [list]: instance` member list.

### Maps — straight replacement

A block full of named tokens and no rows to iterate — a build manifest, a config file — reads its values from a table one row per token:

```
+----------------------------------+---------------------------+-------------+
| list                             | structure                 | file        |
+==================================+===========================+=============+
| - [list]: @project-info:cmake    | @cmake:cmake              | @CMakeLists |
| > - [list]: @project-info:module | > - [list]: @cmake:module |             |
+----------------------------------+---------------------------+-------------+

## cmake
| key            | value  |
| minimumVersion | 4.2.0  |
| cxxStandard    | 17     |
```

`:::minimumVersion:::` in the `cmake` block takes the `value` of the row whose first column is `minimumVersion`. The first column is the key — `key | value`, or `name | type | value | comment`, any table whose first column is an identity. The same table under a `- [list]:` expansion is rows. Under a shape paragraph it is a map. The reader decides, never the table.

Map lines are the list column's `- [list]:` lines at a `>` count beyond the structure column's `- [list]:` lines at that count, first in order. Column-address lines (`@file:table:column`, the inline sources) are never counted — the address form tells them apart. Map lines belong to that count's shape paragraphs in order. A blank `- [list]:` closes one paragraph's group and starts the next. Fewer groups than paragraphs fill the last paragraphs — the same slot law as expansions. A different `>` count is a different scope. A map line with no paragraph at its count stops the run. So does a map table with no `value` column.

### separator — how items join

`- [list]:` lines that mirror the list column's lines. The line at a given ordinal joins that expansion's items. Blank or absent joins by newline. Anything else joins by that text — `@code:<id>` names a block whose text is the join, and `@space` names an index datum (`U+0020` with `fromUTF8`) for a join of one space.

The leading `>`-less separator line is the row join: rows merged into one file join their diverging values by it.

### file — merging rows into one file

Rows that declare the same `file` render as one merged shape, and the wrap emits one time per file. Merging is per line. At each structure line, rows that carry the same value emit it one time. Rows whose values differ emit each value, in authored row order, joined by the first row's row-join line, with one blank line on each side. A group of one row merges to itself.

Same-file rows must be authored contiguously — a row for a file already closed by an intervening row of another file is a fatal error.

### file documentation — a `- [comment]:` binding, wired to a table

The wiring table carries no documentation column. A file's documentation is a `- [comment]:` binding in the **structure** cell of the file group's first row. First appearance counts in authored order — the same rule that decides output-file ordering. The binding's value is a table address: `@file:table:column`, or the local form `@table:column` for a table of the manifest itself. Aliases are scoped to the index, thus a manifest never aliases its own file. The addressed table carries a `file` column and documentation columns. CAST reads the row whose `file` matches the group's own output file. It takes the cell of the column that the address names — the `comment` column when the address names none. CAST writes the resolved text between the banner and the file's own text, in the file's comment syntax. No binding, no matching row, or a blank cell writes nothing extra.

This is the pattern for real projects: one small `## headers` table holds every file's header prose in one place — a `brief` column for the doc block, a `comment` column for a single-liner — and every output group points one binding at it.

```markdown
## headers

| file    | brief                           | comment |
| Out.h   | ```                             |         |
|         | @file Out.h                     |         |
|         | @brief One generated namespace. |         |
|         | ```                             |         |

## output

| list                  | separator | structure                    | file    |
| - [list]: @data:rows    |           | @code:namespace              | @Out.h  |
|                       |           | - macro: #pragma once        |         |
|                       |           | - name: Out                  |         |
|                       |           | - [comment]: @headers:brief    |         |
|                       |           | - [list]: @code:entry          |         |
```

The file match, not the row, decides which prose comes back. To declare `## headers` as a table that CAST also lists an output's includes from (`- [list]: @headers`, per "Declared membership instead of a derived sweep" below) needs no extra care: the self-exclusion law (a file never lists itself) already keeps a file's own row out of its own include sweep, even though the same table supplies that file's header.

The `@` sigil law (a `@`-sigiled value is a reference, never data) separates this binding's two readers. A `- [comment]:` binding whose value is plain text is per-item prose, the same as always: the item-shape reader finds the binding before it falls to the source row's own `comment` column, and renders the bound text. The file-documentation reader resolves only the address-valued form. A reader that renders prose never treats a reference as its prose. One structure cell can carry both — the sigil decides which reader takes which.

### toolchain — commands after the write

`## toolchain` is an optional table, `| argument | command | flag |`, reserved by name — not by file. CAST looks it up across each file that the manifest's index declares. Thus a project that keeps its codegen manifest and its toolchain data apart declares `## toolchain` in the data file, not `spell.md` itself. Its rows run after each output has written, in authored order, one child process per row. A failed row fails the run — never the writes already on disk.

`argument` is optional and, when declared, selects which rows run. A blank `argument` cell marks a default-flow row. `cast spell.md`, with no trailing flag, runs only those. `cast spell.md --<word>` runs only the rows whose `argument` cell equals `word` — a `word` that matches no row is fatal.

```
+----------+---------+----------------------------------------------------------+
| argument | command | flag                                                     |
+==========+=========+==========================================================+
|          | cmake   | -S . -B Builds/Ninja -G Ninja -DCMAKE_BUILD_TYPE=Release |
+----------+---------+----------------------------------------------------------+
|          | cmake   | --build Builds/Ninja                                     |
+----------+---------+----------------------------------------------------------+
| debug    | cmake   | -S . -B Builds/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug   |
+----------+---------+----------------------------------------------------------+
| debug    | cmake   | --build Builds/Debug                                     |
+----------+---------+----------------------------------------------------------+
```

`cast spell.md` configures and builds `Builds/Ninja` in Release. `cast spell.md --debug` configures and builds `Builds/Debug` in Debug — the two default-flow rows do not run.

### format — column widths

`## format` is an optional table, `| name | width |`, reserved by name — not by file, like `## toolchain`. The table name and the reserved column name `format` do not meet — one names a table, the other a cell. `name` is a column name, and an identity column: a column named two times is the duplicate fatal. `width` is that column's wrap width, a positive integer. Every grid table that carries a column of that name reflows its body cells at that width — the authored soft line breaks collapse first, then the text wraps. A column the table does not name keeps its natural width. No `## format` table means no column reflows. A split puts a line break into the cell, and a plain cell's line break reads as a newline; an inline comment's lines join at the output. The author sets each named column's width for that column's content. A fenced cell's lines stay lines, each split at the width. A grid table whose body has no border between rows never splits.

```
+---------+-------+
| name    | width |
+=========+=======+
| comment | 40    |
+---------+-------+
```

A header row without `name` or `width`, or a `width` cell that is not a positive integer, is fatal — during formatting only.

### Regions — patching an existing file

An output row whose structure declares `- [begin]:` and `- [end]:` bindings does not create a file — it patches one that already exists. CAST finds the first line containing the resolved `[begin]` value, then the first later line containing the resolved `[end]` value, and replaces every line strictly between them with the rendered shape. The delimiter lines stay, and everything outside them stays byte for byte. No banner renders, and no file documentation renders — the file already carries its own framing, and the delimiter pair marks where CAST's own ownership begins and ends.

```
+------+-----------+----------------------------+----------+
| list | separator | structure                  | file     |
+======+===========+============================+==========+
|      |           | - [begin]: BEGIN_GENERATED | @Out.h   |
|      |           | - [end]: END_GENERATED     |          |
|      |           | @code:namespace            |          |
+------+-----------+----------------------------+----------+
```

A `[begin]` binding with no `[end]`, or the reverse, is fatal. A delimiter value that matches no line, or an `[end]` value whose first match is at or before `[begin]`, is fatal. A region row's own file must already exist — CAST does not create it. One file is either region-written or whole-written: a file shared between a region row and a whole-file row is fatal. Same-file region rows merge like any other same-file rows (see file — merging rows into one file).

---

## Operations

Operations are optional. A table can carry finished text and use none at all.

**Case** — `toUpper`, `toTitle`, `toPascal`, `toCamel`, `toKebab`, `toSnake`, `toScreamingSnake`

An all-uppercase word is an abbreviation and survives each case operation in each position: `UI scale` becomes `UIScale`, and `fail hazard URI` becomes `failHazardURI`.

**Encoding** — `toLiteral`, `toUTF8`, `fromUTF8`, `toHex`, `toCodepoint`, `fromCodepoint`

`toLiteral` produces a complete string literal: it quotes the value, escapes backslashes and quotes, turns control characters into their named escapes, and turns bytes above `0x7F` into hex escapes. Quoting and escaping are one operation and never travel separately. To write a cell in backticks is the same thing, spelled shorter.

Write the real character, not its escape. A line break in the datum comes out as `\n`. A `"` comes out as `\"`. `©` comes out as `\xc2\xa9`.

An escape that you cannot write as a real character is written as its escape sequence, and `toLiteral` passes it through: a backslash followed by a data-declared escape character is an authored escape and survives verbatim — `\n` authors a line break. A backslash followed by a backslash is one literal backslash, doubled on output — `\\n` authors the two characters backslash-n. Both stay expressible.

`fromUTF8` is the exception, for the one character that you cannot write: a space at the very start or end of a line, which the formatter's padding would swallow. Write `U+0020`, and the operation decodes it back to a space and touches no other byte.

A `format` cell names one operation, never two. The one composition that CAST performs is its own — a backticked or fenced cell is quoted and escaped first, and the cell's operation then applies to that value. That is how `fromUTF8` reaches inside a finished literal.

**Text** — `join`, `toFileName`

**Comment** — `toComment`, `toCommentBlock`, `brief`

The comment family exists for the banner that CAST stamps and for the `:::[comment]:::` marker, both formatted for the output file's language.

---

## Cookbook

### Constants from a declaration table

```markdown
## identifiers

+------+--------------------+-----------------------+-----------+
| type | name               | value                 | format    |
+======+====================+=======================+===========+
| @id  | circleCross        | circleCross           | toLiteral |
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

Row one's `value` is authored plain text, quoted by its `toLiteral` format. Rows two and three are backticked, which already means `toLiteral` — thus their `format` cell stays empty. A blank `value` cell would render an empty string, not `circleCross` — every value that a row needs is authored on that row.

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
{ :::[list]::: },
```
````

```
+--------------------------------------+--------------------+---------------------------+
| list                                 | separator          | structure                 |
+======================================+====================+===========================+
| > > - [list]: @colours               | > > - [list]: `, ` | ...                       |
| > > - [list]: @colours:colours:key   |                    | > > - [list]: @code:entry |
| > > - [list]: @colours:colours:value |                    |                           |
+--------------------------------------+--------------------+---------------------------+
```

```cpp
        { 0, 0xff000000 },
        { 1, 0xffcd0000 },
```

Rows stack vertically at two tabs. `@colours` is the entry's own source. The two column-address lines that follow it, at the same two tabs, name `key` then `value` explicitly — one line per column — and join horizontally into the inline `:::[list]:::` by the separator authored at the source line's own ordinal, `, `. A third column tomorrow is one more address line, same template.

### Wrapping a shape in another

```markdown
## output

+----------------------------+------------------------+----------+
| list                       | structure              | file     |
+============================+========================+==========+
| > - [list]: @xml:token:key | @code:namespace        | @jam_Xml |
|                            | - name: jam            |          |
|                            | @code:bimap            |          |
|                            | - name: XmlTokenType   |          |
|                            | > - [list]: @code:pair |          |
+----------------------------+------------------------+----------+
```

The namespace names `:::[list]:::` one time, thus it takes the next source — the bimap, rendered one time at column 0. The bimap names it one time, thus it takes the wiring line: rows of `@xml:token:key`, each through `@code:pair`, at one tab.

### Guarding one member of an expansion

The Wrappers section's example, end to end, is the pattern for "everything in this list, plus one that only exists behind a macro": the plain members come through the `- [list]: instance` selector, the guarded one comes through the `:::macro-guard:::` wrapper with its own bindings, and the output is

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

One guard, one member — and because the wrapper's slot is an ordinary slot, to feed it `> - [list]: <selector>` in place of a bare line collapses any number of same-macro members into the one region.

### Declared membership instead of a derived sweep

When a derived source starts to sweep in rows that you never meant — `- [list]: file` collects a build manifest into an include list — declare the membership as data, in the manifest itself:

```markdown
## headers

| file          |
| ProjectInfo.h |
| Identifiers.h |
```

and wire `- [list]: @headers` — the first part names no index alias, thus it names the manifest's own table. The list is now exactly what the table says, and the next member is one row.

### Named commands, wired as wrapper tokens

A generated build step — `codesign`, `notarize`, an install copy — is a command that
never changes shape. Only its parameters (an identity, a path, a profile) come from
data. Author the command one time, as its own named fence, with the parameters as
ordinary tokens:

````markdown
```codesign
COMMAND codesign --force --options runtime --entitlements "${CMAKE_SOURCE_DIR}/:::entitlementsPath:::" --sign ":::identity:::" $<TARGET_FILE:${PROJECT_NAME}>
```
````

Wire it as a wrapper — a named binding, not a `:::[list]:::` slot — so the frame around it
stays one constant, dumb `add_custom_command`:

```
| list | separator | structure                    | file        |
|      |           | - codesign: @cmake:codesign  | @CMakeLists |
```

`:::identity:::` and `:::entitlementsPath:::` resolve the same way that each other token
does — through the row's own maps (`- [list]: @project-info:signing` declared earlier in
the same wiring row). To delete the wrapper line deletes the step. The command text
itself is never duplicated, never baked into the frame, and never repeated per
platform or per build.

### One table, two configurations, two compilers

A per-platform, per-configuration flag set needs neither a table per platform nor a
table per configuration — one table, split by a cell-match filter (see Filtering rows
by a cell) on a `stage` column that tells a compile flag from a link flag:

```markdown
## release

+-------------------+-------------+----------+--------+
| name              | mac         | win      | stage  |
+===================+=============+==========+========+
| optimization      | -O3         | /O2      |        |
| deadCodeStripping | -dead_strip | /OPT:REF | linker |
+-------------------+-------------+----------+--------+

## debug

+---------------+------+------+--------+
| name          | mac  | win  | stage  |
+===============+======+======+========+
| optimization  | -O0  | /Od  |        |
| debugSymbols  | -g   | /Zi  |        |
+---------------+------+------+--------+
```

```
+----------------------------------------------+----------------------+----------------------+
| list                                         | separator            | structure            |
+==============================================+======================+======================+
| - [list]: @project-info:release:stage=       | - [list]: @semicolon | - [list]: @cmake:mac |
| - [list]: @project-info:release:stage=linker | - [list]: @semicolon | - [list]: @cmake:mac |
| - [list]: @project-info:debug:stage=         | - [list]: @semicolon | - [list]: @cmake:mac |
+----------------------------------------------+----------------------+----------------------+
```

Each wiring line names the same table two times, split only by what its `stage`
filter selects — the release compile flags, the release link flags, the debug
compile flags. The mac and win rows work the same way. Both read through their own
column in the `@cmake:mac` / `@cmake:win` shape. A row with a blank `mac` or `win`
cell contributes nothing to that platform's join — nothing is nothing, not an
omitted row.

---

## Canonical Markdown

CAST rewrites each declared markdown file to canonical form, write-if-different.

- layout only — cell content, row order and authored borders all survive; a column that `## format` names reflows at its width, backtick literals and fenced cells included
- the formatter re-emits borders exactly where you authored them. It neither adds nor removes one
- columns pad to their widest cell, on each line of each cell, a fence's content lines included — the right edge is always aligned, and `|this|` comes back as `| this |`
- `format (format (x)) == format (x)` — a canonical file reformats to itself, byte for byte
- a malformed table is reported with its `path:line`, and the file is never rewritten

A line break in a plain cell reads as a newline. A grid-table body cell in a column
that `## format` names (see format — column widths) reflows at that width: its authored
soft line breaks collapse by the paragraph rule, then it splits, break only. A column
that `## format` does not name keeps its natural width — no split — unless
`--max-table-width` is positive, in which case the unnamed columns share that width
after the named columns and the border overhead. The default 0 means no split. A share
of zero or less is the natural width. A run with no break opportunity that is wider
than the width splits at the width by character count. A backticked literal splits like
any other text, and the split reads back as a space inside the literal — name a width
that fits the literal. A fenced cell's lines are content: no line joins another, and a
line wider than the width splits the same way — the split is a line break in the value. A grid table whose body has no border between rows never splits —
a split line would carry an empty first cell, and the row law would then read the body
differently. A paragraph reflows at `--line-wrap`, default 100. No wrapped line starts
a block. The format stays a fixpoint under every rule.

---

## Determinism and Failure

Your tables, your templates, your manifest, and the binary determine the output bytes. No timestamps, no paths, no host state. Files use LF.

Write-if-different: a second run produces an empty diff.

No warnings exist. Every failure is fatal, exits non-zero, and writes no output file. Diagnostics name the true physical line in the file, not the row's ordinal position:

```
identifiers.md:412 (name): duplicate "circleCross"
spell.md:133 (structure): template not found: namespace
spell.md:36 (structure): nested shape has more than one candidate
```

Because CAST runs during the configure phase, a failure stops your build before compilation starts.

---

## CMake Integration

- **No discovery.** Carry the exact per-platform `cast` binaries. Do not use `find_program()`. Do not assert versions.
- **Source of truth** for the binary is `~/Documents/Poems/dev/cast`.
- **Invocation** is `codegen.cmake`, included before `project()`.
- **Dependencies** for `CMAKE_CONFIGURE_DEPENDS` come from the manifest — never hand-maintained.
- **Role.** CMake is a dispatcher. It never implements generation logic.
