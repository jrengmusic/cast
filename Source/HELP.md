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

Two column names are reserved everywhere: `format` and `comment`. Inside the manifest, four more are reserved: `list`, `separator`, `structure`, `file`.

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

Three doc channels, all data:

- **row** — a column named `comment`. An item shape's `:::comment:::` takes it like any other column.
- **table** — write a paragraph or a fenced block between the `## table name` heading and the table. A shape-level `:::comment:::` falls back to it when no reference names something else.
- **fence** — a fenced block carrying an info string, anywhere in a data file, not bound to a table. The info string is its name; the fence text is its prose. Address it — or a table — by name with a `- comment: @file:<name>` bullet in the manifest's list column, at the shape line's own `>` count. A wrapper shape declared across several rows carries the same reference on every declaring row, exactly like a binding.

At shape level, with no comment reference authored, `:::comment:::` falls back to the documentation of the first table the shape's own sources address, in authored order; with no source addressing a table, to the documentation of the row's own table.

You never author the comment frame (`/** @brief ... */`, `///< ...`) yourself — you write prose only. CAST renders the frame from the comment-syntax table, chosen by the language of the file the row writes to:

- the marker alone on its line — block form. Multi-line prose renders one prose line per output line; single-line prose renders open, text, close on one line.
- the marker inline, after content — single-line form: the language's comment glyph, then the text.

A missing comment is not an error — the marker renders empty, and an emptied line trims or collapses like any other placeholder line.

### Uniqueness

Within one table, every identity column's entries — `name`, `key`, `alias` — must be unique, compared byte for byte. `circleCross` and `CircleCross` are two different entries. Every other column is payload — `value`, `type`, `format`, `comment` — and payload repeats by design: many rows may map to the same payload.

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

A block is the literal text of the output. Braces, keywords, punctuation — all authored, all verbatim. There are no conditionals, no loops, and no formatting. Comment frames are the one exception: CAST renders them itself, from the comment-syntax table (see Documentation) — never author one in a template.

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
- a column address — `- list: @colours:colours:key` names one column of the enclosing expansion's table; it feeds an **inline** `:::list:::`, never a column-0 one
- a column name — `- list: file` iterates the unique values of that column
- a binding name — `- list: instance` selects the rows that declare that binding

### structure — what shape

Two lines name a shape. `template:namespace` renders it once; `- list: template:<id>` renders it once per item of the line's source. Both are sources: each fills one `:::list:::` of the shape above it. A named bullet binds one token of the nearest shape line above it: `- name: jam` fills its `:::name:::`.

### Arity — how deep

A shape's arity is how many times its block names `:::list:::`. It consumes that many of the source lines that follow, in order, and each of those consumes its own arity first. Nesting comes from the template; you never author it.

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
| > > > - list: @tokens:token type    | template:namespace              |
| > > - list: @tokens:token type      | - name: map                     |
|                                     | template:bimap                  |
|                                     | - name: TemplateTokenType       |
|                                     | - type: int                     |
|                                     | > > > - list: template:mapEntry |
|                                     | > > - list: template:enum       |
+-------------------------------------+---------------------------------+
```

The namespace names `:::list:::` once, so it takes the next source — the bimap, rendered once at column 0. The bimap names it twice — map region first, enum region second — so it takes the next two: mapEntry at three tabs, then enum at two. Matching template, tables and expression is yours; CAST reads the expression and generates. Its one check is the count — a mismatch stops the run and names the block and the row.

### separator — how items join

`- list:` lines mirroring the list column's. The line at a given ordinal joins that expansion's items. Blank or absent joins by newline; anything else joins by that text — `template:<id>` names a block whose text is the join.

The leading `>`-less separator line is the row join: rows merged into one file join their diverging values by it.

### file — merging rows into one file

Rows that declare the same `file` render as one merged shape; the wrap is emitted once per file. Merging is per token, per occurrence: a value that is byte-identical across the group's rows resolves once, and values that differ join in authored row order by the first row's row-join line, framed by one blank line on each side. A group of one row merges to itself.

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

The comment family exists for the banner CAST stamps and for the `:::comment:::` marker, both formatted for the output file's language.

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
| > > - list: @colours:colours:key       |                    | > > - list: template:entry    |
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
| > - list: @xml:token:key | template:namespace             | @jam_Xml |
|                          | - name: jam                    |          |
|                          | template:bimap                 |          |
|                          | - name: XmlTokenType           |          |
|                          | > - list: template:pair        |          |
+--------------------------+--------------------------------+----------+
```

The namespace names `:::list:::` once, so it takes the next source — the bimap, rendered once at column 0. The bimap names it once, so it takes the wiring line: rows of `@xml:token:key`, each through `template:pair`, at one tab.

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
