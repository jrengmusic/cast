# CAST Specification

**Version 0.3**

---

## 1. Scope and Authority

SPEC is normative. `Source/HELP.md` is derived from it and carries no authority; where
the two disagree, SPEC governs and HELP.md is rewritten.

CAST reads tables, iterates rows, and replaces tokens in templates. It has no knowledge
of any target language.

The engine hardcodes these and nothing else:

- the markers `:::token:::`, `@`, `` ` ``, `template:<id>`, `- key: value`, `> `
- the manifest column names
- the index table name and its columns
- the column name `format`
- the operation keywords

Every other name in every file is data.

### 1.1 Data Is Correct

The tables, the templates and the manifest are the specification of intent. When the
engine and the data disagree, **the engine is wrong**. No rule in this document licenses
changing a data file to suit the engine, and no diagnostic exists to report data for
being shaped the way it is shaped.

A rule not written in this document is not a rule. The engine implements exactly the
fatals listed here and adds none of its own.

---

## 2. Artifacts

Three kinds, all flat in one data directory:

- **manifest** — `CAST.md`
- **data files** — markdown, one or more tables each
- **template file** — one `.cast` file per data directory

Every input file is declared exactly once in the manifest's index. CAST parses what it
declares: no directory scanning, no globbing.

Generated outputs are build artifacts and are never edited by hand.

---

## 3. Markdown Substrate

CAST reads CommonMark plus the pandoc grid-table extension.

### 3.1 Row Law

A table's row boundaries are decided once, for the whole table:

- the body contains a border between rows → borders delimit rows
- no border, and no line carries an empty first cell → every `|` line is one row
- no border, and some line carries an empty first cell → the body is one row

A grid table whose body carries no border between rows is legal and ordinary — one `|`
line per row. Its opening border, header separator and closing border are structure, not
content, and are never a diagnostic.

### 3.2 Literals

A backtick code span is verbatim data. Its bytes — leading and trailing spaces,
punctuation, non-ASCII characters — are the cell's value, untouched.

A `|` is written into a cell by preceding it with a backslash. The row scanner consumes
that backslash when it rejoins the split, so a datum that must itself contain a
backslash before the pipe is authored with two.

### 3.3 Formatter

CAST rewrites its own declared markdown to canonical form, write-if-different.

- layout only: cell content, row order and authored borders survive
- borders are re-emitted exactly where they were authored
- `format (format (x)) == format (x)`
- a malformed table is reported and the file is never rewritten

---

## 4. Reference Law

The `@` sigil is part of the alias name. A `@`-sigiled cell is a reference; a bare word
is data. The two cannot collide.

### 4.1 Index

`## index` is reserved, optional, and file-local:

```
| alias | symbol |
```

A `@`-sigiled cell resolves against the index of the file in which that cell is written
— never another file's index.

An index may carry a `format` column like any other table (§5.2). Nothing about `##
index` restricts its columns beyond `alias` and `symbol` being present.

### 4.2 Address Form

```
@file : table : column
```

The **column** part is the omissible one. Two parts are always `@file : table`:

```
@comments : clang comment        file comments.md, table "clang comment"
@text : diagnostics              file text.md,     table "diagnostics"
@lexicon                         file lexicon.md,  table chosen by the rule below
```

One part is `@file` alone. The table is then the one named after the file, or the file's
only table. The index never counts as a table for that purpose.

### 4.3 Names Are Free Text

A heading, a table name and a column name are free text. `## clang comment` is a legal
table name and `comment syntax` is a legal column name.

The engine keys them internally with `Format::toValidID`, applied identically at both
ends — where a name is declared and where it is referenced. An author never writes an
identifier-shaped name to satisfy the engine, and a name is never a diagnostic for its
shape.

### 4.4 Fatal

- an alias absent from the writing file's index
- two index rows declaring the same alias
- an index symbol naming a file that does not exist
- an address naming a table or column that does not exist

---

## 5. Data Tables

Column names are free (§4.3). They are data, matched to template tokens by name. A
column is named for what it yields.

### 5.1 Cell Forms

A data cell is exactly one of:

| Form | Value |
|---|---|
| plain | the text, verbatim |
| backticked | `toLiteral` applied to the span's bytes |
| blank | the preceding column's value, verbatim |

A backtick is not decoration and not an escape — it *is* the `toLiteral` operation. A
backticked cell needs no `format` column beside it.

A blank cell takes the preceding column's authored value without that column's own
formatting. Its own `format`, if present, still applies.

Column order carries no meaning beyond these two adjacency rules and §5.2's.

### 5.2 Format

`format` is a reserved column name. A `format` column binds to the column immediately
before it and names **exactly one** operation. It is optional; a column with no `format`
beside it is written verbatim.

```
| key | format | value | format |
| key | format | value |
| key | value  | format |
| key | value  |
```

`| format | format |` is invalid — a format has no format. This is the only fatal §5.2
declares.

An empty `format` cell formats nothing.

Formatting is declared in the table. A template never formats.

### 5.3 Uniqueness

Within one table, the entries of every column are unique, compared byte-exactly.

Byte-exact means `circleCross` and `CircleCross` are two entries.

Cells holding a reserved mechanism are exempt, because a mechanism repeats by design:

- an operation name in a `format` cell (§8)
- an alias (§4)

---

## 6. Manifest

Four reserved column names:

```
| placeholder | structure | separator | file |
```

`separator` is optional. Blank means newline.

### 6.1 Bindings

A binding is a bullet, `- <token>: <source>`.

The bullet's name is the template token it feeds. The source is an address, a column
name, or another binding's name.

### 6.2 Blank Binding

A bullet with a name and **no source** takes the value of the preceding binding at the
same depth, through `toCamel` (§8):

```
> - name: Screen
> - instance:              instance is "screen"

> - name: WindowFX
> - instance:              instance is "windowFX"   FX is an abbreviation

> - name: CSI
> - instance:              instance is "CSI"        wholly an abbreviation
```

This is the identifier form of the preceding value — the conventional relationship
between a type name and an instance name. A blank binding at the first position of its
depth has no predecessor and its value is empty.

A blank binding is also a **selector**: a row that declares it is a row that
participates wherever that binding's name is used as a source (§6.1).

### 6.3 Structure

A head line is `template:<id>` — the shape at its depth. A bullet binds one token of
that depth's shape.

### 6.4 Depth

`> ` count is both identity and indent:

```
template:something          depth 0    no indent
> - this: nested            depth 1    one tab
> > template:something      depth 2    two tabs
```

One tab is four spaces. The same token name at two depths is two tokens; the deeper is
the inner scope and wins within it.

### 6.5 Pairing

A token name resolves once per depth. Every occurrence of that name in the fragment
carries the same value — a shape may name its token as often as its structure requires.

A fragment token takes its value from, in order:

1. the binding of that name at that depth
2. the column of that name on the row
3. the empty string

The fragment's **distinct** token names that no binding claims pair with that depth's
remaining sources, in authored order. A nested head is a source.

More than one unclaimed token is ordinary — authored order settles it. Neither the count
of unclaimed tokens nor the count of remaining sources is a diagnostic.

### 6.6 Separator

Optional. Named per token, keyed as the wiring is. Blank joins by newline; any other
value joins by that value. There is no second axis — a horizontal shape is one item
authored on one line.

---

## 7. Templates

One `.cast` file per data directory. Fenced code blocks; the fence's info string is the
block's id. A shape is named `template:<id>`.

A block is literal output text and nothing else. Structure only — no conditionals, no
logic, no formatting.

`:::token:::` is replaced by the value of the column or binding of that name (§6.5).
Any symbol delimited by `:::` is a valid placeholder, and the interior is its name
**verbatim** — the interior is never split, and `:::name:operation:::` is not a form.
A template never names an operation; operations are declared in the table (§5.2).

Jack markers sit at column 0. Indent comes from the wiring depth, never from the block.

Delimiters, wraps, braces and punctuation are structure and are authored in the block —
except the quotes around a literal, which `toLiteral` supplies (§9).

### 7.1 File Tokens

A token fed by the `file` column carries the **file name**, not the declared path. The
manifest declares where a file is written; a template that names a file — an include,
for instance — receives `jam_Identifiers.h`, never `../diff/jam_Identifiers.h`.

---

## 8. Operations

Operations are optional. A table may carry finished text and use none.

The operation keywords are the only vocabulary the engine hardcodes. They transform a
datum's own characters:

- **case** — `toUpper`, `toTitle`, `toPascal`, `toCamel`, `toKebab`, `toSnake`, `toScreamingSnake`
- **encoding** — `toLiteral`, `toUTF8`, `toHex`, `toCodepoint`, `fromCodepoint`
- **text** — `join`, `toFileName`
- **comment** — `toComment`, `toCommentBlock`, `brief`, for the banner CAST stamps

In every case operation, an all-uppercase word is an abbreviation and passes through
unchanged in every position. `WindowFX` camel-cases to `windowFX`; `CSI` and `C4Type`
are unchanged.

A `format` cell names one of these and only one. Chaining is not a form — a datum that
needs a different shape is authored in that shape.

---

## 9. Safety Contract

Text that CAST places inside a target-language literal is made safe by CAST, never by
the author.

`toLiteral` delimits and escapes as one operation: the value is quoted, backslashes and
quotes are escaped, and bytes above `0x7F` become escape sequences. Delimiting and
escaping never travel separately, and a template never authors the quotes itself.

The author writes the datum. CAST makes it legal.

---

## 10. Determinism and Failure

Output bytes are determined by the tables, the templates, the manifest, and the binary.
No timestamps, no paths, no host state. Files use LF.

Write-if-different. A second run produces an empty diff.

There are no warnings. Every failure is fatal, exits non-zero, and writes no output
file. A diagnostic names the true physical line:

```
file:line (column): rule
```

### 10.1 The Whole Fatal Set

These, and nothing else:

| Rule | Clause |
|---|---|
| alias absent from the writing file's index | §4.4 |
| duplicate alias in one index | §4.4 |
| index symbol names a file that does not exist | §4.4 |
| address names a table or column that does not exist | §4.4 |
| `\| format \| format \|` adjacency | §5.2 |
| `format` cell names an operation that is not in §8 | §8 |
| duplicate entry within one column of one table | §5.3 |
| `template:<id>` names a block that does not exist | §7 |
| malformed table, during formatting only | §3.3 |

Any check the engine performs that is not in this table is a defect in the engine.

---

## 11. Engine Contract

This section binds the implementation, not the author.

### 11.1 The Document Is the Store

The parsed markdown is a complete AST, built once, addressable in O(1) by
`(parent, id)` and traversable in authored order by intrusive child links. It holds
every table, row, cell and bullet.

Nothing derived from it is copied into a second structure. No cache mirrors it, no array
is materialised to hold pointers it already holds, and no per-call container is built to
answer a question it can already answer. A second copy of a truth the AST owns is a
defect.

Where a value must be computed rather than read — a cell's form (§5.1), a blank
binding's value (§6.2) — it is stamped once onto its own Element at parse, beside the
provenance the parser already stamps, and read back thereafter.

### 11.2 One Owner Per Invariant

Every invariant is established exactly once, at its owner, and trusted unconditionally
everywhere downstream. A downstream re-check, a null test for a case the owner has
excluded, or a fallback value substituted where the owner has already guaranteed
presence, is a defect — whether it is spelled as a guard, an assert, or a silent empty
return.

Validity is owned by the validator alone. The reader and the writer decide nothing, test
nothing, and report nothing.

### 11.3 No Defensive Programming

A guard must name the specific scenario it prevents. A guard that cannot is removed, and
the ownership that made it feel necessary is fixed instead.

Failure is loud. A missing key throws where it is looked up; it does not yield an empty
string that travels downstream and emits blank output.

---

## 12. Conformance

The engine implements superseded rules. Each is debt against this document:

- placeholder names truncated at the first colon rather than taken verbatim (§7)
