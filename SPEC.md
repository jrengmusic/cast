# CAST Specification

**Version 0.8**

---

## 1. Scope and Authority

SPEC is normative. `Source/HELP.md` is derived from it and carries no authority; where
the two disagree, SPEC governs and HELP.md is rewritten.

CAST reads tables, iterates rows, and replaces tokens in templates. It has no knowledge
of any target language.

The engine hardcodes these and nothing else:

- the markers `:::token:::`, `@`, `` ` ``, `- key: value`, `> `
- the manifest column names
- the reserved bullet names `list` (source, §6.5) and `comment` (documentation reference, §5.4)
- the reserved token names `list` (expansion, §6.5) and `comment` (documentation, §5.4)
- the index table name and its columns
- the reserved column names `format`, `comment`, and `value` (map payload, §6.5)
- the identity column names `name`, `key`, `alias` (§5.3)
- the toolchain table name and its columns
- the manifest-syntax table name (§5.4)
- the operation keywords
- the `.cast` extension, which marks an index symbol as a template file (§4.2)

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
- **template files** — every `.cast` file the index declares, any number

Every input file is declared exactly once in the manifest's index. CAST parses what it
declares: no directory scanning, no globbing.

Generated outputs are build artifacts and are never edited by hand.

### 2.1 Invocation

The command line selects what runs; it never carries generation rules.

```
cast [<manifest>] [<directory> | --format | --no-format | --<word>]
cast --version
cast --help
```

- No arguments: the manifest is `CAST.md` in the working directory.
- `<manifest>`: a manifest path; the flag reads before or after it.
- `<directory>`: every declared output writes under it, resolved against the
  manifest's own directory, instead of the declared paths' default root.
- Default order is format, then generate, then the default-flow toolchain rows (§6.9).
- `--format`: format only — every declared markdown file re-canonicalized (§8),
  write-if-different; no generation, no toolchain.
- `--no-format`: generate only — no formatting.
- `--<word>`: after format and generate, run only the `## toolchain` rows whose
  `argument` cell equals `word` (§6.9), instead of the default-flow rows.
- `--version`: the version and source commit, the same stamp the banners embed.
- `--help`: the guide. HELP.md is derived from this specification and carries no
  authority of its own (§1).

`--format` and `--no-format` are mutually exclusive with each other, with
`<directory>`, and with `--<word>` — one manifest, one flag, nothing else on the line.

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

The first part resolves against the writing file's index. A first part naming no index
alias names a **table of the writing file itself** — the local form. The parts then
shift left: `@table` is that table, `@table : column` names its column. A file never
aliases itself; the local form is the only way a manifest addresses its own tables.

An alias whose symbol names a `.cast` file addresses shapes, not tables:

```
@template : fence
```

```
@code : namespace                file code.cast,  fenced block "namespace"
@cmake : module                  file cmake.cast, fenced block "module"
```

The two forms are told apart by the symbol's extension and nothing else. `template` is
not a word the engine knows; `@code` and `@cmake` are aliases like any other, declared
in the writing file's index.

A column part carrying an `=` is a **cell-match filter**, not a bare column name:

```
@file : table : column = value
```

It names the rows of `table` whose `column` cell equals `value`, byte-exactly.
`value` may be empty, matching a row whose `column` cell is itself blank (§5.1) — this
is how one table serves two disjoint row sets, a compiler-stage split for instance,
without a second table. A filter is read as a list source (§6.5); it is never a bare
value address.

### 4.3 Names Are Free Text

A heading, a table name and a column name are free text. `## clang comment` is a legal
table name and `comment syntax` is a legal column name.

The engine keys them internally with `Format::toValidID`, applied identically at both
ends — where a name is declared and where it is referenced. An author never writes an
identifier-shaped name to satisfy the engine, and a name is never a diagnostic for its
shape.

### 4.4 Fatal

- an address's first part naming neither an index alias nor a table of the writing file
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
| blank | nothing — an empty string |

A backtick is not decoration and not an escape — it *is* the `toLiteral` operation. A
backticked cell needs no `format` column beside it. A fenced cell is the same operation
over a datum that spans lines (§3.2), and needs one only where a line's own edge space
must survive the formatter's padding (§9).

A blank cell is nothing: it resolves to an empty string, always, in every table —
a data table, a table kept in the manifest (§6), the manifest's own wiring, `## index`,
`## toolchain`, all alike. There is no inheritance from a preceding cell anywhere. An
empty string is not an error and not a special case — it is string replacement as
nothing: a template token it fills renders empty, and a line left empty by its own
placeholder elides for free, exactly like any other emptied placeholder line (§7). An
empty `format` cell formats nothing — the value it binds to is written verbatim (§5.2).
A blank `comment` cell documents nothing (§5.4). Both are this one law, applied to
their own column.

The substrate is markdown: an HTML entity in a plain cell decodes at parse — `&amp;`
reaches the model as `&`. A datum that must carry an entity's own spelling authors it
double-encoded (`&amp;amp;` yields `&amp;`), which is also the formatter's canonical
emission. Backticked and fenced spans decode nothing.

Column order carries no meaning beyond §5.2's format-column adjacency rule.

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

Within one table, the entries of every identity column — `name`, `key`, `alias` — are
unique, compared byte-exactly.

Byte-exact means `circleCross` and `CircleCross` are two entries.

Every other column is payload — `value`, `type`, `format`, `comment` — and payload
repeats by design: many rows may map to the same payload.

The manifest's **wiring** tables are exempt: wiring repeats templates, separators
and files by design (§6). A data table kept in the manifest (§6) is a data table —
its identity columns are gated like any other.

### 5.4 Documentation

`comment` and `brief` are reserved column names, like `format` (§5.2): documentation,
never data. Their entries are exempt from §5.3 uniqueness, and no expansion (§6.5)
ever emits them. `brief` holds a file's documentation block; `comment` a single-line
documentation — the file-documentation address (§6.8) selects between them by column.

Three documentation channels, all data:

- **row** — the `comment` column
- **table** — the text between a table's `## heading` and the table itself, a
  paragraph or a fenced block, stamped onto the table at parse (§11.1)
- **fence** — a fenced block carrying an info string, anywhere in a data file not
  bound to a table; the info string is its name, the fence text is its prose,
  stamped at parse (§11.1) and addressed by a comment reference

`:::comment:::` is the comment marker — the only place documentation reaches an
output. The author writes prose; documentation tags (`@file`, `@brief`) are authored
text, comment syntax is not. The marker's replacement is rendered in the **output
language's** comment syntax, selected by the file the row writes to, through the
comment-syntax table. Comment syntax never appears in a template, a cell, or a fence.

The comment-syntax key is the output file's extension — except when the output file's
exact name has a row in the manifest-syntax table. Build manifests carry deterministic
names whose extensions do not name their language (`CMakeLists.txt` is CMake, not
text); a manifest-syntax hit replaces the extension as the comment-syntax key, and
every other output falls through to its extension.

Marker position selects the form, exactly as marker position selects the expansion
axis (§7):

- alone on its line — block form. Multi-line prose renders one prose line per output
  line: the block-open glyph, each prose line behind the block-line glyph (the glyph
  alone on a blank prose line), then the block-close glyph behind one space when the
  language declares a block-line glyph. Single-line prose renders on one line: block
  open, the text, block close.
- inline, after content — single-line form: the language's comment glyph and the text

The marker resolves by scope: in an item shape it reads the source row's `comment`
column; at shape level it reads, in order:

1. the row's **comment reference** — a `- comment: @file:<name>` bullet in the list
   column at the shape line's own `>` count, pairing with that line exactly as a
   `- list:` line pairs (§6.5). The address names a table (its table documentation)
   or a fence (§5.4). A wrapper shape declared across several rows carries the same
   reference on every declaring row — the reference is part of the shape's per-row
   declaration, like a binding.
2. positionally — the documentation of the first table the shape's expansions
   address; when no source addresses a table, the documentation of the row's own
   table.

A missing comment is not an error and not a special case: the
replacement is empty, and the line trims or collapses exactly like any other emptied
placeholder line — plain replacement, no elision machinery.

---

## 6. Manifest

Four reserved column names, in canonical authored order:

```
| list | separator | structure | file |
```

A manifest table may also carry a `comment` column (§5.4): documentation for the row's
items, read by an item shape's `:::comment:::` exactly as a data table's comment column
is read. The marker always reads the item's own source row (§5.4) — a manifest row's
comment cell therefore speaks where the manifest row itself is the source, as a
selector's rows are; items sourced from a data table read that table's own rows, and
an absent row comment renders empty, never a fallback.

A **wiring table** is a table of the manifest that carries a `structure` column.
No other table is wiring, and no column name is reserved outside the manifest's
wiring tables: a column named `file` — or any other word from this section — in a
data table is plain data.

The manifest may declare **itself** in its own index. Its non-wiring tables are then
addressable data, exactly as a data file's tables are (§4.1) — membership lists and
other wiring-adjacent data may live beside the wiring that reads them.

### 6.1 Bindings

A binding is a bullet, `- <name>: <value>`, whose name is anything but `list`. It feeds
the template token of that name, paired **by name**. The value is an address — a data
symbol or a shape (§4.2) — or plain text, resolved the same way everywhere a bullet
value is read.

A binding whose value is a shape address is a **wrapper**: the token of the binding's
name is replaced by that shape's rendering. The wrapper is a shape line at its
authored position in the structure column — it consumes the source lines that follow
it, bounded by its own arity (§6.4); the bindings after it, up to the next shape
line, bind **its** tokens; its `>` count is the indent its shape renders at. This is
how a shape enters through a named token — `:::list:::` remains the only expansion
token, and a wrapper never fills one (§6.4).

Everything a wrapper consumes — its bindings and its sources — is the wrapper's own
render data. It is invisible to selectors (§6.5): a binding inside a wrapper's chain
declares nothing to the rest of the manifest.

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

A binding — blank or valued — is also a **selector** mark: a row that declares it is
a row that participates wherever that binding's name is used as a source (§6.5).
Declaration is participation; a blank cell only changes where the value comes from,
never whether the row is seen. A binding inside a wrapper's chain (§6.1) is not a
declaration and never participates.

### 6.3 Structure

The structure column is read top to bottom. Two lines name a shape, by its address
(§4.2):

- `@template:fence` — the shape renders once
- `- list: @template:fence` — the shape renders once per item of the line's source (§6.5)

Both are **sources**: each fills one `:::list:::` occurrence of the shape above it. The
first line of the column is the row's own shape.

A named bullet binds one token of the nearest shape line above it (§6.1). A wrapper
(§6.1) is a shape line in this reading order: the bullets after it bind its tokens,
and the source lines after it are its to consume.

### 6.4 Arity

A shape's arity is its count of `:::list:::` occurrences (§7). A shape consumes the next
*arity* source lines, in order; each consumed source consumes its own arity first, so
nesting is declared by the template and never authored. Supplying **more** sources
than the row's shapes demand is fatal (§10.1). Supplying fewer is legal: authored
sources fill a shape's **trailing** slots, the leading unfilled slots render empty
and elide (§6.5) — an under-supplied shape is the authored way to use part of a
template.

A wrapper (§6.1) counts like a shape line, not like a source: it adds its own arity
to the row's demand and supplies nothing — it fills a named token, never a
`:::list:::` slot.

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
2. the shape's **maps** — the row of that name in each map table, first declared
   first (below)
3. the column of that name — on the manifest row, or, for an item shape, on the source
   row
4. the table documentation (§5.4), for the name `comment` at shape level

**Maps.** A map is a table read one row per token: the row whose **first column** equals
the token name supplies its `value` cell. `key | value` is the plain form;
`name | type | value | comment` is the same map with more payload. Any table whose first
column is an identity column (§5.3) is a map when a shape reads it as one; the same
table iterated by a `- list:` expansion is a record table. Orientation is the reader's,
never the table's.

A shape paragraph declares its maps in the list column, positionally, under the same
slot law as expansions: at a `>` count, the list column's `- list:` lines in excess of
the structure column's `- list:` lines at that count are map lines, read first in
authored order. Column-address lines (`@file:table:column`, the inline sources below)
are neither expansions nor maps and are never counted: the address form is the
discriminator, as it is for the inline marker. Map lines group to that count's shape paragraphs by order: a blank
`- list:` closes the current paragraph's group and opens the next paragraph's; fewer
groups than paragraphs fill the trailing paragraphs, as fewer sources than arity fill a
shape's trailing slots (§6.4). A different `>` count is a different scope and needs no
placeholder. A map line with no shape paragraph at its count is fatal (§10.1). A map
table with no `value` column is fatal (§10.1).

`comment` at shape level skips rungs 2 and 3: a map row named `comment` is data, and a
manifest row's comment column documents the row's items (§6), never the row's own
shape — shape documentation is the table channel (§5.4).

A supplier that carries no text renders empty, and a line left empty by its own
placeholder is elided (§7). A named token that no binding, column, or documentation
names renders empty the same way — an unsupplied token is empty text, never an
error. The template declares every slot a file could carry; the data declares, per
row, which slots carry text.

Expansion pairs by order:

- The list column's `- list:` lines pair with the structure column's `- list:` lines by
  `>` count and by ordinal within that count (§6.4). The list column names **what
  iterates**; the structure line names **the shape** each item renders through.
- A shape consumes the source lines that follow it, bounded by its arity (§6.4). Its
  `:::list:::` occurrences, in block order, take those sources in the same order.
- A separator line pairs with the `- list:` line of the same ordinal (§6.6).

A `- list:` source paired with a structure `- list:` line is one of:

- an **address** — `@file:table` iterates that table's rows
- a **column address** — `@file:table:column` names one column of the enclosing
  expansion's table; it feeds an **inline** `:::list:::` (§7), never a column-0 one
- a **cell-match filter** — `@file:table:column=value` (§4.2) iterates only that
  table's rows whose `column` cell equals `value`, byte-exactly; an empty `value`
  matches a blank cell (§5.1) — this is how a filtered address, not a second table,
  splits one table into disjoint row sets
- a **column name** — iterates the distinct values of that column across output rows,
  excluding the value belonging to the writing row's own file: a file never lists
  itself
- a **binding name** — iterates the rows that declare a binding of that name (§6.2),
  blank or valued; a binding inside a wrapper's chain (§6.1) never counts

The same exclusion extends to the **address** form: when an address's table carries a
`file` column, the row whose `file` value equals the writing row's own file is excluded
from that table's iterated rows — a file never lists itself, whether the source names a
column or a table.

A bare source name is tried as a column first: a name matching both a column and a
binding resolves as the **column**. A selector name therefore must not collide with
any wiring-table column name. A binding-name source walks the wiring tables in
manifest order and each table's rows in authored order. A column-name source yields
its distinct values in first-appearance order — never sorted. A row may declare more
than one selector binding; each name selects its own row set, and two selectors on
one row feed two slots by ordinal, like any two sources.

An inline `:::list:::`'s sources are the column-address lines that follow the item
shape's own source line, at the **same** `>` count, consecutive ordinals, in authored
order — one line per column, each naming the column explicitly. The current row's
value of each addressed column fills the marker, joined by the within-line join
(§6.6). There is no implicit column set: every consumed column is named.

A list-column line with no structure partner is a map line (above). A structure
`- list:` line, or a separator line past the row join, with no list-column line of its
ordinal is fatal (§10.1).

A shape-level `:::comment:::` (§5.4) falls back to the documentation of the first
table addressed by the shape's own sources, in authored order; when no source
addresses a table, it falls back to the documentation of the row's own table.

Matching the template, the tables and the expression is the author's responsibility.
The engine reads the expression and generates; its one check is the count: a shape
whose arity does not match the sources supplied is fatal (§6.4, §10.1).

### 6.6 Separator

The separator column carries `- list:` lines mirroring the list column's. The line at
ordinal K is the join text for the expansion wired at ordinal K. No line, or a blank
value, joins by newline. A value resolves like any bullet value (§6.1) — a shape address
names a block whose text is the join; an index alias names a datum, formatted by the
index's own `format` column (§4.1), which is how a join of one space is authored
(`U+0020`, `fromUTF8`, §9).

For an item shape whose `:::list:::` is inline (§6.5), the separator line at the item
source line's own ordinal is the **within-line join** — the text between the addressed
column values. The join between the items themselves is the newline default; an inline
marker declares no other item join.

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

### 6.8 File Documentation

A wiring table carries no documentation column. A file's documentation is wired in the
structure column: a `- comment:` binding whose value is a table address, on the file
group's **first** row — first appearance in authored order, the same grouping law as
§6.7 and the same law the engine already applies to output-file ordering.

The address is `@file:table` or `@file:table:column` (§4.2) — or, for a table of the
manifest itself, the local form `@table` or `@table:column`. The addressed table
carries a `file` column and documentation columns; the engine reads the row whose
`file` value equals the group's own output file name and takes the cell of the column
the address names — the `comment` column when the address names none. The addressed
cell is plain text or the fenced documentation form (§3.2): each fence line is one line
of the file's documentation prose. A group whose first row carries no address-valued
`- comment:` binding, a file absent from the addressed table, or an addressed cell that
is blank, writes no file documentation — plain replacement, no special case (§5.4).

The resolved prose is rendered in the file's own comment syntax exactly as a
shape-level `:::comment:::` renders (§5.4), written between the banner and the group's
own rendered shape, framed by one blank line on each side. The addressed table's
documentation columns are documentation, never data (§5.4) — a fence there is not a
literal, `toLiteral` never applies, and the `@` sigil law (§4) does not apply inside a
fence's prose: documentation is never a reference, so a prose line beginning with
`@file` or `@brief` is text. The binding's own value is the one exception — it is a
reference, resolved as any other address is (§4.2).

The `@` sigil law (§4) is what separates this binding's two readers. A `- comment:`
binding whose value is plain text is per-item prose, exactly as before: an item-shape
reader finds the binding before it falls to the source row's own `comment` column, and
renders the bound text. The file-documentation reader reads only the address-valued
form; an item-shape reader, which renders prose, never treats a reference as its prose.
One structure cell may carry both — the reference for the file header and the text for
the row's items — the sigil decides which reader takes which, the same rule every
`@`-sigiled value already follows (§4).

### 6.9 Toolchain

`## toolchain` is a reserved table name, and it is optional.
Reservation is by name, not by file: the engine looks up every `## toolchain` table
across the whole spliced document (every file the manifest's index declares), so the
table may be kept in the manifest itself or in any declared data file — a project that
separates its codegen manifest from its toolchain data keeps `## toolchain` in the
latter.

```
| argument | command | flag |
```

`argument` is optional; a table declared without it is `| command | flag |`. The
unknown-word fatal (§10.1) applies regardless: a `--<word>` CLI argument that matches no
row's `argument` cell is fatal whether the table declares the column or not — a table
with no `argument` column has no row whose `argument` cell could ever equal a non-empty
`word`, so any non-empty `--<word>` against it is unconditionally fatal. A blank
`argument` cell marks the row as a **default-flow** row — a blank cell is nothing,
as it is everywhere (§5.1).

When the table is declared, its rows run after every declared output has written, in
authored row order — one child process per row. The command cell is the executable,
taken verbatim — argv[0], never split. The flag cell is the argument list: it splits
on spaces, and a double-quoted span becomes one argument with the quote marks removed.
A blank flag cell runs the command alone.

The CLI selects which rows run. With no `--<word>` argument, only the rows whose
`argument` cell is blank run — the default flow. With `cast <manifest> --<word>`, only
the rows whose `argument` cell equals `word`, byte-exactly, run; the default-flow rows
do not. A `--<word>` matching no row's `argument` cell, across every declared
`## toolchain` table, is fatal (§10.1), naming `word`.

The command resolves through the caller's environment. PATH, working directory, and
everything else the process inherits are the caller's responsibility — the engine adds
no resolution, no shell, and no quoting of its own.

A row whose process cannot start, or exits nonzero, fails the run with the row's own
command text in the diagnostic; the rows after it do not run. The declared outputs are
already on disk when the toolchain runs — a toolchain failure fails the run, never the
write.

---

## 7. Templates

A template file is any `.cast` file the index declares. Fenced code blocks; the fence's
info string is the block's id. A shape is addressed `@template:fence` (§4.2); two
template files may declare the same fence id, since every address names its file.

A block is literal output text and nothing else. Structure only — no conditionals, no
logic, no formatting. Target-language conditional directives (`#if`, `#endif`, and
their kin) are literal text like any other: the engine neither reads nor evaluates
them, and slots inside such an arm take sources by the ordinary arity law (§6.4).

`:::list:::` is the expansion token. A block's occurrence count is its arity (§6.4);
each occurrence is filled by one source — its items joined by that source's separator
(§6.6):

- a marker at **column 0** fills vertically: every line of the join is prefixed by the
  source line's indent (§6.4)
- a marker **inside a line** fills horizontally, in place, unindented

Marker position is the only axis. There is no axis vocabulary and no second mechanism.

Any other `:::token:::` is a named token, replaced by the binding, map row, or column of
that name (§6.5). Any symbol delimited by `:::` is a valid placeholder, and the interior is
its name **verbatim** — the interior is never split, and `:::name:operation:::` is not
a form. The interior is free text (§4.3): the marker in the block is matched exactly
as authored, and pairing with bindings, maps, and columns keys it as every other name
is keyed — identically at both ends. An author never shapes a token name for the
engine; `:::macro-guard:::` and `:::macroGuard:::` are each legal and each pair with
the bullet spelled their own way. A template never names an operation; operations are
declared in the table (§5.2). A named token may occur more than once in a block; every
occurrence carries the same value. Two blocks may name the same token: each shape's
own suppliers feed its own occurrences — an outer shape's binding never reaches into
a wrapper's token of the same name (§6.1).

Delimiters, wraps, braces and punctuation are structure and are authored in the block —
except the quotes around a literal, which `toLiteral` supplies (§9). Comment frames
are rendered by the engine from the comment-syntax table (§5.4).

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
and no emitted line carries trailing whitespace. Multi-line shapes, single-item
joins, and single-line shapes carrying an inline `:::list:::` render unpadded. There
is no column limit and no wrapping — a long line stays long.

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
| duplicate entry within one identity column of one table | §5.3 |
| a shape address names a fence that does not exist in its template file | §7 |
| a map line with no shape paragraph at its `>` count | §6.5 |
| a map table with no `value` column | §6.5 |
| an output file whose comment-syntax key (§5.4) names no comment-syntax table | §5.4 |
| an output file that cannot be written | §10 |
| a toolchain row whose process cannot start or exits nonzero | §6.9 |
| a `--<word>` CLI argument matching no toolchain row's `argument` cell | §6.9 |
| malformed table, during formatting only | §3.3 |
| index `symbol` cell empty | §4.1 |
| output row declares no structure | §6.3 |
| same-file output rows not contiguous | §6.7 |
| a row supplies more sources than its shapes demand | §6.4 |
| a structure or separator `- list:` line without its list-column line of the same ordinal, the row join excepted | §6.5, §6.6 |
| duplicate binding name among one shape's bindings | §6.1 |
| a comment reference naming neither a table nor a fence | §5.4 |
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

- merging resolved per whole shape, keyed by the concatenation of every non-list
  token value, rather than per line (§6.7)
- each line's paired source walked at every read rather than stamped once at parse
  (§11.1) — template id, indentation and line are stamped
- per-call containers materialised where the document already holds the answer
  (§11.1): selector row collection, wrapper private-scope collection, item
  replacement and width maps, merge-group key arrays
- a wrapper's binding scope (§6.1) recovered positionally at read rather than held
  as one stamped ownership truth — a wrapper item is owned by its parent and owns
  its own chain, and the two roles currently share one stamp with a position
  tiebreak
- the template files' own token scan duplicated across the template pool and the
  marker primitives — layering keeps them apart
