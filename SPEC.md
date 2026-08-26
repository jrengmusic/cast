# CAST Specification

**Version 0.4**

---

## 1. Scope and Authority

SPEC is normative. `Source/HELP.md` is derived from it and carries no authority; where
the two disagree, SPEC governs and HELP.md is rewritten.

CAST reads tables, iterates rows, and replaces tokens in templates. It has no knowledge
of any target language.

The engine hardcodes these and nothing else:

- the markers `:::token:::`, `@`, `` ` ``, `template:<id>`, `- key: value`, `> `
- the manifest column names
- the reserved source word `cells`
- the reserved token names `list` (expansion, §6.5) and `comment` (documentation, §5.4)
- the index table name and its columns
- the reserved column names `format` and `comment`
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

A cell holding a fenced code block is the multi-line form of the same law. A code span
cannot carry a line break; a fence is how a multi-line datum is authored. Each line
between the delimiters is one line of the cell's value, joined by line breaks.

Whitespace a cell carries for alignment is not data, and a cell is delimited by its
pipes on every line it spans. The reader strips that padding from every line — a fenced
cell's content lines included — so `|this|` and `| this |` are one datum. A space at the
very start or end of a line therefore cannot be authored as a space; it is written as a
codepoint token and decoded by an operation (§9).

A `|` is written into a cell by preceding it with a backslash. The row scanner consumes
that backslash when it rejoins the split, so a datum that must itself contain a
backslash before the pipe is authored with two.

### 3.3 Formatter

CAST rewrites its own declared markdown to canonical form, write-if-different.

- layout only: cell content, row order and authored borders survive
- borders are re-emitted exactly where they were authored
- every line of every cell pads to its column's width, both sides, with no exception for
  a cell that holds a fence — the right edge is always aligned. `|this|` is a legal row
  and is rewritten as `| this |`
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
| fenced | `toLiteral` applied to the fence's content, line breaks included |
| blank | the preceding column's value, verbatim |

A backtick is not decoration and not an escape — it *is* the `toLiteral` operation. A
backticked cell needs no `format` column beside it. A fenced cell is the same operation
over a datum that spans lines (§3.2), and needs one only where a line's own edge space
must survive the formatter's padding (§9).

A blank cell takes the preceding column's authored value without that column's own
formatting. Its own `format`, if present, still applies. Blank inheritance is a data
table rule — a blank manifest cell is blank and inherits nothing (§6).

The substrate is markdown: an HTML entity in a plain cell decodes at parse — `&amp;`
reaches the model as `&`. A datum that must carry an entity's own spelling authors it
double-encoded (`&amp;amp;` yields `&amp;`), which is also the formatter's canonical
emission. Backticked and fenced spans decode nothing.

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
- a `comment` cell (§5.4) — documentation repeats freely

### 5.4 Documentation

`comment` is a reserved column name, like `format` (§5.2): documentation, never data.
Its entries are exempt from §5.3 uniqueness, and the `cells` source (§6.5) never emits
it.

Two documentation channels, both data:

- **row** — the `comment` column
- **table** — the text between a table's `## heading` and the table itself, a
  paragraph or a fenced block, stamped onto the table at parse (§11.1)

`:::comment:::` is the comment marker — the only place documentation reaches an
output, and the only text the engine formats: the marker's replacement is rendered in
the **output language's** comment syntax, selected by the output file's extension
through the comment-syntax table. Comment syntax never appears in a template.

Marker position selects the form, exactly as marker position selects the expansion
axis (§7):

- alone on its line — block form: block open, brief marker, the text, block close
  (`/** @brief text */` for a C++ target)
- inline, after content — single-line form: the language's comment marker and the
  text (`///< text`)

The marker resolves by scope: in an item shape it reads the source row's `comment`
column; at shape level it reads the documentation of the first table the shape's
expansions address. A missing comment is not an error and not a special case: the
replacement is empty, and the line trims or collapses exactly like any other emptied
placeholder line — plain replacement, no elision machinery.

---

## 6. Manifest

Four reserved column names, in canonical authored order:

```
| list | separator | structure | file |
```

### 6.1 Bindings

A binding is a bullet, `- <name>: <value>`, whose name is anything but `list`. It feeds
the template token of that name, paired **by name**. The value is an address, plain
text, or `template:<id>`, resolved the same way everywhere a bullet value is read.

### 6.2 Blank Binding

A bullet with a name and **no source** takes the value of the preceding binding of the
same shape, through `toCamel` (§8):

```
- name: Screen
- instance:              instance is "screen"

- name: WindowFX
- instance:              instance is "windowFX"   FX is an abbreviation

- name: CSI
- instance:              instance is "CSI"        wholly an abbreviation
```

This is the identifier form of the preceding value — the conventional relationship
between a type name and an instance name. A blank binding at the first position of its
shape has no predecessor and its value is empty.

A blank binding is also a **selector**: a row that declares it is a row that
participates wherever that binding's name is used as a source (§6.5).

### 6.3 Structure

The structure column is read top to bottom. Two lines name a shape:

- `template:<id>` — the shape renders once
- `- list: template:<id>` — the shape renders once per item of the line's source (§6.5)

Both are **sources**: each fills one `:::list:::` occurrence of the shape above it. The
first line of the column is the row's own shape.

A named bullet binds one token of the nearest shape line above it (§6.1).

### 6.4 Arity

A shape's arity is its count of `:::list:::` occurrences (§7). A shape consumes the next
*arity* source lines, in order; each consumed source consumes its own arity first, so
nesting is declared by the template and never authored. A shape whose arity does not
match the sources the column supplies is fatal (§10.1).

`> ` count is indentation, and nothing else: one tab (four spaces) per `>`, the
**absolute** column — measured from column 0 of the output file — at which that line's
shape renders. No `>` is column 0. Indentation never selects a token, an owner, or a
nesting level.

The list and separator columns carry the same `>` count as the structure line they pair
with, and it is read: a line pairs with the line carrying the same `>` count at the same
ordinal within that count (§6.5). One symbol, one meaning, in all three columns.

### 6.5 Expansion and Pairing

`:::list:::` is the only expansion token (§7). Every other token is a named token and
takes its value from, in order:

1. the binding of that name (§6.1) — for an item shape, the source row's own binding
2. the column of that name — on the manifest row, or, for an item shape, on the source
   row
3. the table documentation (§5.4), for the name `comment` at shape level

A supplier that carries no text renders empty, and a line left empty by its own
placeholder is elided (§7). A named token that no binding, column, or documentation
names at all is fatal (§10.1).

Expansion pairs by order:

- The list column's `- list:` lines pair with the structure column's `- list:` lines by
  `>` count and by ordinal within that count (§6.4). The list column names **what
  iterates**; the structure line names **the shape** each item renders through.
- A shape consumes the source lines that follow it, bounded by its arity (§6.4). Its
  `:::list:::` occurrences, in block order, take those sources in the same order.
- A separator line pairs with the `- list:` line of the same ordinal (§6.6).

A `- list:` source is one of:

- an **address** — `@file:table:column` iterates that table's rows
- a **column name** — iterates the distinct values of that column across output rows
- a **binding name** — iterates the rows that declare that blank binding (§6.2)
- **`cells`** — iterates the enclosing expansion's current row's data cells, in column
  order; `format` columns apply to their bound column (§5.2) and never emit

A list-column line with no structure partner renders its items **verbatim** — the
datum itself, unshaped — and fills the occupied occurrence of the shape it follows.
That is the ordinary form for `cells`, whose items are the row's own values. A
structure `- list:` line, or a separator line past the row join, with no list-column
line of its ordinal is fatal (§10.1).

A shape-level `:::comment:::` (§5.4) falls back to the documentation of the first
table addressed by the shape's own sources, in authored order.

Matching the template, the tables and the expression is the author's responsibility.
The engine reads the expression and generates; its one check is the count: a shape
whose arity does not match the sources supplied is fatal (§6.4, §10.1).

### 6.6 Separator

The separator column carries `- list:` lines mirroring the list column's. The line at
ordinal K is the join text for the expansion wired at ordinal K. No line, or a blank
value, joins by newline. A value resolves like any bullet value (§6.1) —
`template:<id>` names a block whose text is the join.

The column's leading `>`-less line is the row join: it joins the diverging values of the
rows declaring the same `file` (§6.7). Its partner is those rows themselves — it is
exempt from the list-column partner requirement (§6.5), and the lines after it carry the
ordinals. There is no other separator mechanism.

### 6.7 File Merge

Rows declaring the same `file` render as one. Their first structure line is
byte-identical by authorship and is emitted once.

Merging is per line. At each structure line, the rows that carry the same value for that
line emit it once; the rows whose values differ emit each value, in authored row order,
joined by the row join — the first row's row-join line resolved as §6.6, a non-blank
separator framed by one blank line on each side. The sources a line consumes (§6.4)
merge inside that line by the same rule, so merging follows the structure the template
declares and never crosses it. One row declaring a file merges to itself.

Same-file rows are authored contiguously. A row declaring a file already closed by an
intervening row of another file is fatal (§10.1) — one file is written once.

---

## 7. Templates

One `.cast` file per data directory. Fenced code blocks; the fence's info string is the
block's id. A shape is named `template:<id>`.

A block is literal output text and nothing else. Structure only — no conditionals, no
logic, no formatting.

`:::list:::` is the expansion token. A block's occurrence count is its arity (§6.4);
each occurrence is filled by one source — its items joined by that source's separator
(§6.6):

- a marker at **column 0** fills vertically: every line of the join is prefixed by the
  source line's indent (§6.4)
- a marker **inside a line** fills horizontally, in place, unindented

Marker position is the only axis. There is no axis vocabulary and no second mechanism.

Any other `:::token:::` is a named token, replaced by the binding or column of that
name (§6.5). Any symbol delimited by `:::` is a valid placeholder, and the interior is
its name **verbatim** — the interior is never split, and `:::name:operation:::` is not
a form. A template never names an operation; operations are declared in the table
(§5.2). A named token may occur more than once in a block; every occurrence carries the
same value.

Delimiters, wraps, braces and punctuation are structure and are authored in the block —
except the quotes around a literal, which `toLiteral` supplies (§9). Comment frames are
structure too (§5.4).

### 7.1 File Tokens

A token fed by the `file` column carries the **file name**, not the declared path. The
manifest declares where a file is written; a template that names a file — an include,
for instance — receives `jam_Identifiers.h`, never `../diff/jam_Identifiers.h`.

### 7.2 Alignment

A join of more than one item rendered from a single-line shape aligns its token
columns: fill spaces are inserted into the literal between two tokens, immediately
after that literal's first whitespace run — at the literal's end when it contains
none — sized to the preceding token's deficit against the byte width of that token's
widest replacement across the join set. Fill never enters a token's replacement, so a
token wrapped in literals (a quoted include path) emits verbatim. Content before the
first token and after the last token never pads — a single-token shape emits unpadded
and no emitted line carries trailing whitespace. Multi-line shapes and single-item
joins render unpadded. There is no column limit and no wrapping — a long line stays
long.

---

## 8. Operations

Operations are optional. A table may carry finished text and use none.

The operation keywords are the only vocabulary the engine hardcodes. They transform a
datum's own characters:

- **case** — `toUpper`, `toTitle`, `toPascal`, `toCamel`, `toKebab`, `toSnake`, `toScreamingSnake`
- **encoding** — `toLiteral`, `toUTF8`, `fromUTF8`, `toHex`, `toCodepoint`, `fromCodepoint`
- **text** — `join`, `toFileName`
- **comment** — `toComment`, `toCommentBlock`, `brief`, for the banner CAST stamps and
  the `:::comment:::` marker (§5.4)

In every case operation, an all-uppercase word is an abbreviation and passes through
unchanged in every position. `WindowFX` camel-cases to `windowFX`; `CSI` and `C4Type`
are unchanged.

A `format` cell names one of these and only one. Two operations are never chained in a
`format` cell — a datum that needs a different shape is authored in that shape. The one
composition the engine performs is its own: a backticked or fenced cell is delimited and
escaped by `toLiteral` before the cell's operation applies (§9).

---

## 9. Safety Contract

Text that CAST places inside a target-language literal is made safe by CAST, never by
the author.

`toLiteral` delimits and escapes as one operation: the value is quoted, backslashes and
quotes are escaped, control characters become their named escapes, and bytes above
`0x7F` become hex escape sequences. Delimiting and escaping never travel separately, and
a template never authors the quotes itself.

The author writes a real line break; `toLiteral` writes `\n`. The mapping from control
character to escape character is data, declared in a table like any other, never a
constant inside the operation.

An escape the author cannot write as a real character is written as its escape
sequence, and `toLiteral` passes it through: a backslash followed by one of the
data-declared escape characters is an authored escape and survives verbatim; a
backslash followed by a backslash is one literal backslash, doubled on output; a
backslash followed by anything else is a literal backslash, doubled as before. `\n`
authors a line break, `\\n` authors the two characters backslash-n — both remain
expressible.

A space the author cannot write is written as `U+XXXX`, and `fromUTF8` decodes every
such token in a datum back to the character it names, leaving every other byte untouched.
This is how a value carries a space at the very start or end of one of its lines, where
the formatter's own padding would otherwise be indistinguishable from it (§3.2). The
token is ordinary text to every other part of the engine — the reader has nothing to
strip, because there is no space there to strip.

`toLiteral` and a `format` operation compose: a backticked or fenced cell is quoted and
escaped first, and the operation then applies to that value.

§9's guarantee covers text CAST wraps into a string literal. A datum that is already
target-language source — a char literal like `'&'`, a type name, an expression — is
authored plain and passes verbatim; its safety is the author's, like a template's own
structure.

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
| index `symbol` cell empty | §4.1 |
| output row declares no structure | §6.3 |
| same-file output rows not contiguous | §6.7 |
| a shape's arity does not match the sources supplied | §6.4 |
| a structure or separator `- list:` line without its list-column line of the same ordinal, the row join excepted | §6.5, §6.6 |
| duplicate binding name among one shape's bindings | §6.1 |
| a named token no binding, column, or documentation names | §6.5 |
| unterminated `:::` marker in a shape block | §7 |

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
binding's value (§6.2), a table's documentation (§5.4) — it is stamped once onto its
own Element at parse, beside the provenance the parser already stamps, and read back
thereafter.

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
- a named token with no supplier yielding an empty string rather than failing (§6.5, §10.1)
- an occurrence index clamped to the last value supplied rather than trusting the arity
  gate (§6.4, §11.2)
- merging resolved per token occurrence across every row declaring the file, rather than
  per line (§6.7)
- each line's template id, indentation and paired source recomputed at every read rather
  than stamped once at parse (§11.1)
