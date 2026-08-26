# CAST: Codegen Annotated Source of Truth

**Version 0.0.1**

You are seeing this because you ran `cast --help`, or because `cast` could not find a `CAST.md` in the current directory.

CAST reads your data (tables), your shapes (templates), and your wiring (the manifest), and writes files. It knows nothing about any programming language. It resolves references, iterates rows, and replaces tokens — nothing else.

This guide explains how to use CAST. The rules it describes are fixed in `SPEC.md`; where this guide and SPEC disagree, SPEC is right and this guide is wrong.

---

## Command Line

Arguments select what to run. They never carry generation rules.

- `cast` — find `CAST.md` here, format every declared markdown file, then regenerate every declared output
- `cast <path>/CAST.md` — the same, using a specific manifest
- `cast CAST.md <output>` — regenerate one declared output only
- `cast CAST.md --format` — format only, no generation
- `cast CAST.md --no-format` — generate only, no formatting
- `cast --version` — version and source commit, the same stamp embedded in generated banners
- `cast --help` — this guide

Default order is format, then generate.

---

## The Three Artifacts

Everything lives flat in one data directory.

| Artifact | What it is |
|---|---|
| `CAST.md` | the manifest — what gets generated, from what, into what |
| `*.md` | data tables |
| `*.cast` | one file holding every code shape |

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

Two column names are reserved: `format`, and inside the manifest, `list`, `separator`, `structure`, `file`.

### Cells

A cell is one of three things:

| Cell | Meaning |
|---|---|
| plain text | that text, verbatim |
| `` `text` `` | `toLiteral` of those bytes — quoted and escaped, ready to drop into a literal |
| a fenced block | the same, over a datum that spans lines |
| empty | the **preceding** column's value, verbatim |

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
+------+-------------+-------+-----------+
| type | name        | value | format    |
+======+=============+=======+===========+
| @id  | circleCross |       | toLiteral |
+------+-------------+-------+-----------+

name  ->  circleCross      verbatim
value ->  "circleCross"    empty, so it takes name, then toLiteral quotes it
```

Formatting is declared in the table. A template never formats anything.

### Documentation

Two doc channels, both data:

- **per row** — a column named `comment`. An entry shape's `:::comment:::` takes it like any column.
- **per table** — write a paragraph or a fenced block between the `## table name` heading and the table. A shape-level `:::comment:::` resolves to it; no text means empty.

The comment frame (`/** @brief ... */`, `///< ...`) is authored in the template — it is structure, like braces.

### Uniqueness

Within one table, every column's entries must be unique, compared byte for byte. `circleCross` and `CircleCross` are two different entries. Cells holding an alias or an operation name are exempt, because those repeat by design.

---

## References

The `@` sigil is part of the name. A cell starting with `@` is a reference; anything else is data. They cannot be confused for each other.

### The index

Any file may declare `## index`. It is file-local — a `@` cell resolves against the index of the file it is written in, never another file's. It declares aliases and nothing else.

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

Write the table part only when you need it. It is omissible when the table is named after its file, or when the file holds exactly one table. A file that declares `## index` must also declare a table named after itself — the index never counts as the only table.

CAST stops with an error if an alias is undeclared, declared twice, points at a missing file, or names a table or column that does not exist.

---

## Templates

Every code shape lives in one `.cast` file. It is markdown: fenced code blocks, and the fence's info string is the block's name.

````markdown
```identifier
inline const :::type::: :::name::: { juce::String::fromUTF8 (:::value:::) };
```

```entry
{ :::list::: },
```
````

A block is the literal text of the output. Braces, keywords, punctuation, comment frames — all authored, all verbatim. There are no conditionals, no loops, and no formatting.

**`:::list:::` is the one expansion token.** Each occurrence is one expansion: its items joined by that expansion's separator. Where the marker sits decides the axis:

- at **column 0** — vertical: the join fills line by line, each line indented by the wiring line's `>` depth
- **inside a line** — horizontal: the join lands in place, unindented

```
struct :::name:::          vertical — items stack, indent from the wiring
{
:::list:::
};

{ :::list::: },            horizontal — items join in place, e.g. by ", "
```

Every other `:::token:::` is a named token, replaced by the binding or column of that name. A token carries no operation — `:::token:op:::` is not a form. Do not author the quotes around a literal. `toLiteral` supplies them, and doubling up produces `""value""`.

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

### list — what iterates

`- list: <source>` lines. Each one is an expansion; the source says what feeds it:

- an address — `- list: @xml:XmlTokenType:key` iterates that table's rows
- a column name — `- list: file` iterates the unique values of that column
- a binding name — `- list: instance` selects the rows that declare that binding
- `cells` — iterates the enclosing expansion's current row's cells, in column order

### structure — what shape

A head line names a shape: `template:namespace`. A named bullet binds one token of that shape: `- name: jam` fills its `:::name:::`. A `- list: template:<id>` bullet names the shape each item of the matching expansion renders through.

### Depth — where, and how deep

`> ` count is indentation, nothing else:

```
- list: ...            fills at column 0
> - list: ...          fills at one tab
> > > - list: ...      fills at three tabs
```

One tab is four spaces, **absolute** — measured from column 0 of the output file, not from the enclosing shape. A nested head (`> template:bimap`) adds no indent of its own; its block's lines land exactly where the block authored them.

A `- list:` line one level deeper than a row expansion runs once per row of it — that is how `cells` knows which row to read.

### Pairing — always by position, never by depth order

The list column's `- list:` lines pair with the structure column's `- list:` lines at the same depth and ordinal. Each shape owns the `- list:` lines and nested heads beneath its head, down to the next head; the shape's `:::list:::` occurrences, top to bottom, consume those sources **in authored order**.

```
+-------------------------------------+---------------------------------+
| list                                | structure                       |
+=====================================+=================================+
| > > > - list: @tokens:token type    | template:namespace              |
| > > - list: @tokens:token type      | - name: map                     |
|                                     | > template:bimap                |
|                                     | > - name: TemplateTokenType     |
|                                     | > - type: int                   |
|                                     | > > > - list: template:mapEntry |
|                                     | > > - list: template:enum       |
+-------------------------------------+---------------------------------+
```

The bimap block names `:::list:::` twice — map region first, enum region second. The first occurrence takes the first authored wiring line (mapEntry, three tabs), the second takes the second (enum, two tabs). Matching template, tables and expression is yours; CAST reads the expression and generates. Its one check is the count — a mismatch stops the run and names the block and the row.

### separator — how items join

`- list:` lines mirroring the list column's depths. The line at a given depth and ordinal joins that expansion's items. Blank or absent joins by newline; anything else joins by that text — `template:<id>` names a block whose text is the join.

The **depth-0** separator line is the row join: rows merged into one file join their diverging values by it.

### file — merging rows into one file

Rows that declare the same `file` render as one merged shape; the wrap is emitted once per file. Merging is per token, per occurrence: a value that is byte-identical across the group's rows resolves once, and values that differ join in authored row order by the first row's depth-0 separator, framed by one blank line on each side. A group of one row merges to itself.

Same-file rows must be authored contiguously — a row for a file already closed by an intervening row of another file is a fatal error.

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

The comment family exists for the banner CAST stamps, formatted for the output file's language.

---

## Cookbook

### Constants from a declaration table

```markdown
## identifiers

+------+--------------------+-----------------------+-----------+
| type | name               | value                 | format    |
+======+====================+=======================+===========+
| @id  | circleCross        |                       | toLiteral |
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

Row one's `value` is empty, so it takes `name` and its `format` quotes it. Rows two and three are backticked, which already means `toLiteral` — so their `format` cell stays empty.

### Any-arity entries with cells

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
+----------------------+-------------------+-------------------------------+
| list                 | separator         | structure                     |
+======================+===================+===============================+
| > > - list: @colours |                   | ...                           |
| > > > - list: cells  | > > > - list: `, `| > > - list: template:entry    |
+----------------------+-------------------+-------------------------------+
```

```cpp
        { 0, 0xff000000 },
        { 1, 0xffcd0000 },
```

Rows stack vertically at two tabs; each row's cells join horizontally by `, ` into the inline `:::list:::`. Three columns tomorrow — same template.

### Wrapping a shape in another

```markdown
## output

+--------------------------+--------------------------------+----------+
| list                     | structure                      | file     |
+==========================+================================+==========+
| > - list: @xml:token:key | template:namespace             | @jam_Xml |
|                          | - name: jam                    |          |
|                          | > template:bimap               |          |
|                          | > - name: XmlTokenType         |          |
|                          | > - list: template:pair        |          |
+--------------------------+--------------------------------+----------+
```

The namespace's `:::list:::` is fed by the nested head — the bimap's built text. The bimap's own `:::list:::` is fed by the wiring line: rows of `@xml:token:key`, each through `template:pair`, indented one tab.

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
identifiers.md:412 (name): duplicate entry "circleCross"
CAST.md:133 (structure): template not found: namespace
template.cast (bimap): 2 occurrences of :::list:::, 1 source
```

Because CAST runs during the configure phase, a failure stops your build before compilation starts.

---

## CMake Integration

- **No discovery.** Carry the exact per-platform `cast` binaries. Do not use `find_program()`. Do not assert versions.
- **Source of truth** for the binary is `~/Documents/Poems/dev/cast`.
- **Invocation** is `codegen.cmake`, included before `project()`.
- **Dependencies** for `CMAKE_CONFIGURE_DEPENDS` come from the manifest — never hand-maintained.
- **Role.** CMake is a dispatcher. It never implements generation logic.
