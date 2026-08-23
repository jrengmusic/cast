# CAST Specification

**Version 0.2**

---

## 1. Scope and Authority

SPEC is normative. `Source/HELP.md` is derived from it and carries no authority; where
the two disagree, SPEC governs and HELP.md is rewritten.

CAST resolves references, iterates rows, and replaces tokens in templates. It has no
knowledge of any target language.

The engine hardcodes these and nothing else:

- the markers `:::token:::`, `@`, `` ` ``, `template:<id>`, `- key: value`, `> `
- the manifest column names
- the index table name and its columns
- the column name `format`
- the operation keywords

Every other name in every file is data.

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

The index declares aliases only. It carries no `format` column; formatting is declared
in the data table that uses the value (§5.2).

### 4.2 Address Form

```
@file : table : column
```

The table part is omissible when the table is named after its file, or when the file
holds exactly one table. A file that declares `## index` must also declare a table named
after the file; the index never counts as the sole table.

### 4.3 Fatal

- an alias absent from the writing file's index
- two index rows declaring the same alias
- an index symbol naming a file that does not exist
- an address naming a table or column that does not exist

---

## 5. Data Tables

Column names are free. They are data, matched to template tokens by name. A column is
named for what it yields.

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

`| format | format |` is invalid — a format has no format.

An empty `format` cell formats nothing.

Formatting is declared in the table. A template never formats.

### 5.3 Uniqueness

Within one table, the entries of every column are unique, compared byte-exactly. Cells
holding a reserved operation name or an alias are exempt, because those repeat by
design.

Byte-exact means `circleCross` and `CircleCross` are two entries.

---

## 6. Manifest

Four reserved column names:

```
| placeholder | structure | separator | file |
```

`separator` is optional. Blank means newline.

### 6.1 Placeholder

Wiring bullets, `- <token>: <source>`. The bullet's name is the template token that
expands. The source is an address, a column name, or a binding name.

### 6.2 Structure

A head line is `template:<id>` — the shape at its depth. A bullet binds one token of
that depth's shape.

### 6.3 Depth

`> ` count is both identity and indent:

```
template:something          depth 0    no indent
> - this: nested            depth 1    one tab
> > template:something      depth 2    two tabs
```

One tab is four spaces. The same token name at two depths is two tokens; the deeper is
the inner scope and wins within it.

### 6.4 Pairing

A token name resolves once per depth. Every occurrence of that name in the fragment
carries the same value — a shape may name its token as often as its structure requires.

The fragment's **distinct** token names that no binding claims pair with that depth's
remaining sources, in authored order. A nested head is a source. Any mismatch is fatal
and names the fragment and the row.

### 6.5 Separator

Optional. Named per token, keyed as the wiring is. Blank joins by newline; any other
value joins by that value. There is no second axis — a horizontal shape is one item
authored on one line.

---

## 7. Templates

One `.cast` file per data directory. Fenced code blocks; the fence's info string is the
block's id. A shape is named `template:<id>`; a missing id is fatal.

A block is literal output text and nothing else. Structure only — no conditionals, no
logic, no formatting.

`:::token:::` is replaced by the resolved value of the column or binding of that name.
A token carries no operation: `:::token:op:::` is not a form.

Jack markers sit at column 0. Indent comes from the wiring depth, never from the block.

Delimiters, wraps, braces and punctuation are structure and are authored in the block —
except the quotes around a literal, which `toLiteral` supplies (§9).

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
unchanged in every position.

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

---

## 11. Conformance

The engine implements superseded rules. Each is debt against this document:

- `format` read from `## index` rather than as a paired column (§5.2)
- formatting applied inside the template via `:::token:op:::` (§7)
- operations registered beyond §8 — `quoted`, `toSymbol`, `toUnicode`, `fromId`, `fromMap`, `fromIdentifier`, `fromLiteral`
- parent fill by elimination, with silent line elision, rather than ordered pairing (§6.4)
- backtick not treated as `toLiteral` (§5.1)
- blank cell not resolved to the preceding column (§5.1)
- uniqueness unchecked beyond index aliases (§5.3)
- an unknown alias, table or column resolving to empty rather than fatal (§4.3)
- an address limited to one colon, so `@file:table:column` cannot be written (§4.2)
