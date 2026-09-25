# CAST Specification

**Version 0.9**

---

## 1. Scope and Authority

This document is the specification, and it governs. `Source/HELP.md` is derived from
this document and has no authority. Where the two documents disagree, this document
is correct, and HELP.md must change.

CAST reads tables, iterates rows, and replaces tokens in templates. CAST has no
knowledge of a target language.

The engine hardcodes these items and no other items:

- the markers `:::token:::`, `@`, `````, `- key: value`, `> `, `[word]`
- the manifest column names
- the reserved bullet names `[list]` (source, §6.5), `[comment]` (documentation reference, §5.4), and `[begin]` / `[end]` (region delimiters, §6.10)
- the reserved token names `[list]` (expansion, §6.5), `[comment]` (documentation, §5.4) and `[banner]` (§5.5)
- the reserved fence prefix `[no-banner]` (§7.3)
- the index table name and its columns
- the reserved column names `format`, `comment`, `brief`, and `value` (map payload, §6.5)
- the identity column names `name`, `key`, `alias`, `file` (§5.3)
- the toolchain table name and its columns
- the manifest-syntax table name (§5.4)
- the operation keywords
- the sync file name `user-modules-info.md`, its table names `identity`, `module`,
  and `ignore`, the columns `key`, `class`, and `boundary`, the class keywords
  `kernel` and `provision`, the boundary keyword `word`, and the composed identity
  keys `namespace`, `filePrefix`, and `macroPrefix` (§2.2)
- the `.cast` extension, which marks an index symbol as a template file (§4.2), the `.md`
  extension, which marks an index symbol as a file that must exist (§4.4), and the `.h`
  extension, the comment-syntax key an output falls back to (§5.4)

Every other name in every file is data.

**Square brackets mark a reserved name.** Each name that the engine reserves is
written in brackets — `:::[list]:::`, `- [comment]:`, the `[sh]` prefix of a fence.
A name without brackets always belongs to the author. The bare words `list`,
`comment` and `banner` are ordinary token names and binding names. Column names are
the one exception: a table gives each of its columns a scope (§6), thus reserved
column names have no brackets.

### 1.1 Data Is Correct

The tables, the templates and the manifest specify the intent. When the engine and
the data disagree, **the engine is wrong**. No rule in this document permits a change
to a data file to satisfy the engine. No diagnostic exists that reports data because
of its shape.

A rule that is not written in this document is not a rule. The engine implements
exactly the fatals in this document and adds none of its own.

---

## 2. Artifacts

Three kinds of artifact exist, all flat in one data directory:

- **manifest** — `spell.md`
- **data files** — markdown, each with one or more tables
- **template files** — each `.cast` file that the index declares, any number

The manifest's index declares each input file exactly one time. CAST parses the files
that the index declares. CAST does not scan a directory and does not glob.

Generated outputs are build artifacts. Do not edit a generated output by hand.

### 2.1 Invocation

The command line selects what runs. The command line never carries a generation rule.

```
cast [<manifest>] [<directory> | --format | --no-format | --<word>] [--max-table-width n] [--line-wrap n]
cast --sync <source-root> <target-root>
cast --version
cast --help
```

- No arguments: the manifest is `spell.md` in the working directory.
- `<manifest>`: a manifest path. The flag reads before or after it.
- `--max-table-width n` and `--line-wrap n`: each is a value pair, in any position on
  the line. Each composes with `--format`, `--no-format` and the manifest argument.
  §3.3 governs their effect.
- `--line-wrap` must be a positive integer; `--max-table-width` must be a non-negative
  integer — any other value is fatal (§10.1).
- `<directory>`: each declared output writes under this directory, in place of the
  declared paths' default root. The engine resolves the directory against the
  manifest's own directory.
- The default order is format, then generate, then the default-flow toolchain rows
  (§6.9).
- `--format`: format only. The engine re-canonicalizes each declared markdown file
  (§3.3), write-if-different. No generation and no toolchain occur.
- `--no-format`: generate only. No format occurs.
- `--<word>`: after format and generate, the engine runs only the `## toolchain` rows
  whose `argument` cell equals `word` (§6.9). The default-flow rows do not run.
- `--sync`: no manifest — the two arguments are framework root directories, and the
  run follows §2.2 alone. It composes with nothing else on the line, and a `--sync`
  line without exactly two roots is fatal (§10.1).
- `--version`: the version and the source commit — the same stamp that the banners
  embed.
- `--help`: the guide. HELP.md is derived from this specification and has no
  authority of its own (§1).

`--format` and `--no-format` exclude each other. Each also excludes `<directory>` and
`--<word>`. One manifest, one flag, nothing else on the line.

### 2.2 Sync

`cast --sync <source-root> <target-root>` mirrors one framework's kernel onto
another's. Each root carries a `user-modules-info.md` file at its top level, with
three tables: `## identity` (`key | value | boundary`), `## module` (`name | class`,
plus data columns that manifest wiring reads, never sync), and `## ignore`
(`value`). Sync reads the two info files and no manifest. The two roots must
differ (§10.1). Sync reads exactly these three tables — any other table in the file
belongs to the manifest and is never read by sync.

**The transform is one ordered replacement list, longest source first** — sources
of equal length order by their text, descending, and sources with equal text keep
their authored row order, so the list is total. Each
`## identity` key present in both files contributes one pair: the source file's
value → the target file's value — except `namespace`, which contributes no plain
pair. When two or more rows share a byte-equal source value, the first authored row
contributes the pair and the later rows contribute none. The three composed keys
must be present in both files (§10.1): `namespace`
contributes exactly two pairs, `namespace <source>` → `namespace <target>` and
`<source>::` → `<target>::` — its bare value never replaces, because the bare word
occurs inside unrelated names; `filePrefix` and `macroPrefix`
contribute their plain pairs, and `filePrefix` also transforms every path segment —
directory names and file names alike. A row whose `boundary` cell is `word` matches
whole words only: the characters before and after the match are absent or outside
`[A-Za-z0-9_]`. Every other row matches plain text.

**The walk.** Sync visits the source's `kernel` rows. A `kernel` row names a
directory; every other class value is provision. At each root, every kernel row's
directory must exist, and every `<filePrefix>*` directory on disk must be declared
(§10.1). After the path transform, the two files' kernel sets must correspond one
to one (§10.1). Root-relative paths use `/` on every host. A file whose
root-relative path matches an `## ignore` row (`*` wildcards) is skipped. A source
file that cannot be read is fatal (§10.1). A file with a NUL byte in its first
8000 bytes copies byte-for-byte. Every other file transforms as text and normalizes
to LF (§10). A `.md` file re-canonicalizes through the formatter (§3.3) after the
transform only when its kernel scope's own name carries no `filePrefix` — the data
scopes, never the module directories. Every write is write-if-different, and a
write or delete that fails is fatal (§10.1). Before a write, a target file whose
on-disk name differs from the transformed path only by case is renamed to the
transformed path, so mirror-delete compares exact names on every host; a rename
that fails is fatal as an output that cannot be written (§10).

**Contamination.** A source text file that already contains a pair's target value
is fatal, and the diagnostic names the file and the token (§10.1) — the transform
must be invertible, and a target token on the source side proves it is not.

**Mirror-delete.** A target file inside a kernel scope with no transformed source
counterpart and no `## ignore` match is deleted. `provision` rows are never walked,
never written, never deleted from.

**The report** is the written paths and the deleted paths, one per line, nothing
else. Zero lines is the pass: the roots are in sync. Generated outputs are not
carried — the target regenerates them by running cast against its own manifest.

---

## 3. Markdown Substrate

CAST reads CommonMark plus the pandoc grid-table extension.

### 3.1 Row Law

The engine decides a table's row boundaries one time, for the whole table:

- the body contains a border between rows → borders delimit rows
- no border, and no line carries an empty first cell → each `|` line is one row
- no border, and some line carries an empty first cell → the body is one row

A grid table whose body carries no border between rows is legal and ordinary — one
`|` line per row. Its opening border, its header separator and its closing border are
structure, not content. They are never a diagnostic.

### 3.2 Literals

A backtick code span is verbatim data. Its bytes are the cell's value, with no
change. This includes leading spaces, trailing spaces, punctuation, and non-ASCII
characters.

A cell that holds a fenced code block is the multi-line form of the same law. A code
span cannot carry a line break. A fence is how the author writes a multi-line datum.
Each line between the delimiters is one line of the cell's value. Line breaks join
the lines.

Whitespace that a cell carries for alignment is not data. Pipes delimit a cell on
each line that the cell spans. The reader strips that padding from each line — the
content lines of a fenced cell included — thus `|this|` and `| this |` are one datum.
A space at the very start or the very end of a line thus cannot be authored as a
space. The author writes it as a codepoint token, and an operation decodes it (§9).

To put a `|` in a cell, the author writes a backslash before it. The row scanner
consumes that backslash when it rejoins the split. Thus a datum that must itself
contain a backslash before the pipe is authored with two backslashes.

### 3.3 Formatter

CAST rewrites its own declared markdown to canonical form, write-if-different.

- layout only: cell content, row order and authored borders survive
- the formatter re-emits borders exactly where the author put them
- each line of each cell pads to its column's width, on both sides. A cell that holds
  a fence is not an exception — the right edge is always aligned. `|this|` is a legal
  row, and the formatter rewrites it as `| this |`
- `format (format (x)) == format (x)`
- the engine reports a malformed table and never rewrites that file

A line break in a plain cell reads as a newline. A grid-table body cell in a column
that `## format` names (§6.11) reflows at that width: its authored soft line breaks
collapse by the paragraph rule, then it splits, break only. A column that `## format`
does not name keeps its natural width — no split — unless `--max-table-width` is
positive, in which case the unnamed columns share that width after the named columns
and the border overhead. The default 0 means no split. A share of zero or less is the
natural width. A run with no break opportunity that is wider than the width splits at
the width by character count. A backticked literal splits like any other text, and the
split reads back as a space inside the literal — the author names a width that fits the
literal. A fenced cell's lines are content: no line joins another, and a line wider than
the width splits the same way — the split is a line break in the value (§3.2).
A grid table whose body has no border between rows never splits: a split line would
carry an empty first cell, and §3.1 would then read the body by another row law. A
paragraph reflows at `--line-wrap`, default 100. No wrapped line starts a block. The
format stays a fixpoint under every rule.

---

## 4. Reference Law

The `@` sigil is part of the alias name. A cell with the `@` sigil is a reference. A
bare word is data. The two cannot collide.

### 4.1 Index

`## index` is reserved, optional, and file-local:

```
| alias | symbol |
```

A cell with the `@` sigil resolves against the index of the file that contains the
cell. It never resolves against the index of a different file.

An index can carry a `format` column, like each other table (§5.2). `## index` must
contain the `alias` column and the `symbol` column. It does not restrict the other
columns.

### 4.2 Address Form

```
@file : table : column
```

The author can omit the **column** part, and only that part. Two parts are always
`@file : table`:

```
@comments : clang comment        file comments.md, table "clang comment"
@text : diagnostics              file text.md,     table "diagnostics"
@lexicon                         file lexicon.md,  table chosen by the rule below
```

One part is `@file` alone. The table is then the table with the file's name, or the
file's only table. The index never counts as a table for this rule.

The first part resolves against the writing file's index. A first part that names no
index alias names a **table of the writing file itself** — the local form. The parts
then shift left: `@table` is that table, and `@table : column` names its column. A
file never aliases itself. The local form is the only way that a manifest addresses
its own tables.

An alias whose symbol names a `.cast` file addresses shapes, not tables:

```
@template : fence
```

```
@code : namespace                file code.cast,  fenced block "namespace"
@cmake : module                  file cmake.cast, fenced block "module"
```

The symbol's extension, and nothing else, tells the two forms apart. `template` is
not a word that the engine knows. `@code` and `@cmake` are aliases like each other
alias, declared in the writing file's index.

A column part that carries an `=` is a **cell-match filter**, not a bare column name:

```
@file : table : column = value
```

It names the rows of `table` whose `column` cell equals `value`, byte-exactly.
`value` can be empty. An empty `value` matches a row whose `column` cell is itself
blank (§5.1). Thus one table serves two disjoint row sets — a compiler-stage split,
for example — without a second table. The engine reads a filter as a list source
(§6.5). A filter is never a bare value address.

### 4.3 Names Are Free Text

A heading, a table name and a column name are free text. `## clang comment` is a
legal table name, and `comment syntax` is a legal column name.

The engine keys them internally with `Format::toValidID`. The engine applies it
identically at both ends — where a name is declared and where a name is referenced.
An author never writes an identifier-shaped name to satisfy the engine. A name is
never a diagnostic because of its shape.

### 4.4 Fatal

- an address's first part naming neither an index alias nor a table of the writing file
- two index rows declaring the same alias
- an index symbol naming a file that does not exist
- an address naming a table or column that does not exist

---

## 5. Data Tables

Column names are free (§4.3). They are data. The engine matches them to template
tokens by name. A column is named for what it yields.

### 5.1 Cell Forms

A data cell is exactly one of:

| Form       | Value                                                            |
| ---------- | ---------------------------------------------------------------- |
| plain      | the text, verbatim                                               |
| backticked | `toLiteral` applied to the span's bytes                          |
| fenced     | `toLiteral` applied to the fence's content, line breaks included |
| blank      | nothing — an empty string                                        |

A backtick is not decoration and is not an escape — the backtick _is_ the `toLiteral`
operation. A backticked cell needs no `format` column beside it. A fenced cell is the
same operation over a datum that spans lines (§3.2). A fenced cell needs a `format`
column only where the edge space of one of its own lines must survive the formatter's
padding (§9).

A blank cell is nothing. It resolves to an empty string, always, in every table — a
data table, a table kept in the manifest (§6), the manifest's own wiring, `## index`,
`## toolchain`, all alike. No cell inherits from a preceding cell, anywhere. An empty
string is not an error and is not a special case — it is string replacement as
nothing. A template token that it fills renders empty. A line that its own
placeholder leaves empty elides for free, exactly like each other emptied placeholder
line (§7). An empty `format` cell formats nothing — the engine writes the value that
it binds to verbatim (§5.2).

The law governs a cell that the engine reads as a **value**. It does not govern a
cell that the engine reads as an **address**. An address names a thing, and a name
that is nothing names nothing. The `## index` `symbol` cell is the one such cell, and
a blank one is fatal (§4.1, §10.1). A blank `comment` cell documents nothing (§5.4).
Both are this one law, applied to their own column.

The substrate is markdown. An HTML entity in a plain cell decodes at parse — `&amp;`
reaches the model as `&`. A datum that must carry an entity's own spelling is
authored double-encoded: `&amp;amp;` yields `&amp;`. The double-encoded form is also
the formatter's canonical emission. Backticked spans and fenced spans decode nothing.

Column order carries no meaning, except the format-column position rule of §5.2.

### 5.2 Format

`format` is a reserved column name. A `format` column binds to the column immediately
before it. A `format` cell names **exactly one** operation. The column is optional. A
column with no `format` column beside it is written verbatim.

```
| key | format | value | format |
| key | format | value |
| key | value  | format |
| key | value  |
```

`| format | format |` is invalid — a format has no format. This is the only fatal
that §5.2 declares.

An empty `format` cell formats nothing.

The table declares the formatting. A template never formats.

### 5.3 Uniqueness

Within one table, the entries of each identity column — `name`, `key`, `alias`,
`file` — are unique. The engine compares them byte-exactly.

Byte-exact means that `circleCross` and `CircleCross` are two entries.

Each other column is payload — `value`, `type`, `format`, `comment`. Payload repeats
by design: many rows can map to the same payload.

The manifest's **wiring** tables are exempt. Wiring repeats templates, separators and
files by design (§6). A data table kept in the manifest (§6) is a data table — the
engine gates its identity columns like those of each other table.

### 5.4 Documentation

`comment` and `brief` are reserved column names, like `format` (§5.2). They are
documentation, never data. Their entries are exempt from §5.3 uniqueness. No
expansion (§6.5) emits them.

Three documentation channels exist, all data:

- **row** — the `comment` column
- **table** — the text between a table's `## heading` and the table itself, a
  paragraph or a fenced block. The parser stamps it onto the table at parse (§11.1)
- **fence** — a fenced block that carries an info string, anywhere in a data file,
  not bound to a table. The info string is its name, and the fence text is its prose.
  The parser stamps it at parse (§11.1), and a comment reference addresses it

`:::[comment]:::` is the comment marker — the only place where documentation reaches
an output. The author writes prose. Documentation tags (`@file`, `@brief`) are
authored text. Comment syntax is not authored text. The engine renders the marker's
replacement in the **output language's** comment syntax. The file that the row writes
to selects the language, through the comment-syntax table. Comment syntax never
appears in a template, a cell, or a fence.

The comment-syntax key is the output file's extension — except when the output file's
exact name has a row in the manifest-syntax table. Build manifests carry
deterministic names whose extensions do not name their language (`CMakeLists.txt` is
CMake, not text). A manifest-syntax hit replaces the extension as the comment-syntax
key. Each other output falls through to its extension. A fence prefix outranks both
(§7.3).

An extension that names no comment-syntax table — and a file with no extension at
all — resolves to the C table. The engine always writes the file: an output that CAST
cannot key is written in C syntax, not refused. A wrong comment mark is a bad line in
one file. A refusal is no file at all.

Marker position selects the form, exactly as marker position selects the expansion
axis (§7):

- alone on its line — block form. Multi-line prose renders one prose line per output
  line: the block-open glyph first, then each prose line behind the block-line glyph,
  then the block-close glyph behind one space. A blank prose line renders the glyph
  alone. The block-line glyph applies when the language declares one. Single-line
  prose renders on one line: block open, the text, block close.
- inline, after content — single-line form: the language's comment glyph and the text,
  its lines joined by one space

The marker resolves by scope. In an item shape, it reads the source row's `comment`
column. At shape level, it reads, in order:

1. the row's **comment reference** — a `- [comment]: @file:<name>` bullet in the list
  column at the shape line's own `>` count. The bullet pairs with that line exactly
  as a `- [list]:` line pairs (§6.5). The address names a table (its table
  documentation) or a fence (§5.4). A wrapper shape declared across several rows
  carries the same reference on each declaring row — the reference is part of the
  shape's per-row declaration, like a binding.
2. positionally — the documentation of the first table that the shape's expansions
  address. When no source addresses a table, it reads the documentation of the row's
  own table.

A missing comment is not an error and is not a special case. The replacement is
empty. The line trims or collapses exactly like each other emptied placeholder line —
plain replacement, no elision machinery.

### 5.5 Banner

Every output file carries the banner. The engine stamps it. A template never authors
it. The engine renders it in the output language's own comment syntax, keyed exactly
as `:::[comment]:::` is keyed (§5.4). The file's documentation (§6.8) is part of the
banner and travels with it.

`:::[banner]:::` places the banner. The banner renders where the marker sits — in any
shape, at any depth, one time for each occurrence. A file whose render carries no
such marker takes the banner at the top, before its own first line. That is the only
difference that the marker makes: it moves the banner, it never creates one.

Position belongs to the author, because line 1 is not the engine's to know. A shell
script needs `#!` first. An XML file needs its declaration first. No extension tells
the engine whether this particular file has one. The template holds line 1, thus the
template places the banner.

A language that declares a banner frame frames the banner with it. A language that
declares no frame takes its single-line marker before each banner line — the same
rendering that `:::[comment]:::` gives multi-line prose (§5.4). Shell, TOML and YAML
have no block comment, and this is how they carry a banner at all.

A file gets no banner when a fence of its render carries the `[no-banner]` prefix
(§7.3). One marked fence suppresses the banner for the whole file, in every position.

---

## 6. Manifest

Four reserved column names, in canonical authored order:

```
| list | separator | structure | file |
```

A manifest table can also carry a `comment` column (§5.4): documentation for the
row's items. An item shape's `:::[comment]:::` reads it exactly as it reads a data
table's comment column. The marker always reads the item's own source row (§5.4). A
manifest row's comment cell thus speaks where the manifest row itself is the source,
as a selector's rows are. Items sourced from a data table read that table's own rows.
An absent row comment renders empty, never a fallback.

A **wiring table** is a table of the manifest that carries a `structure` column. No
other table is wiring. No column name is reserved outside the manifest's wiring
tables: a column named `file` — or any other word from this section — in a data table
is plain data.

The manifest can declare **itself** in its own index. Its non-wiring tables are then
addressable data, exactly as a data file's tables are (§4.1). Membership lists and
other wiring-adjacent data can live beside the wiring that reads them.

### 6.1 Bindings

A binding is a bullet, `- <name>: <value>`, whose name is anything but `[list]`. It
feeds the template token of that name, paired **by name**. The value is an address —
a data symbol or a shape (§4.2) — or plain text. The engine resolves it the same way
everywhere it reads a bullet value.

A binding whose value is a shape address is a **wrapper**: that shape's rendering
replaces the token of the binding's name. The wrapper is a shape line at its authored
position in the structure column. It consumes the source lines that follow it,
bounded by its own arity (§6.4). The bindings after it, up to the next shape line,
bind **its** tokens. Its `>` count is the indent that its shape renders at. This is
how a shape enters through a named token — `:::[list]:::` remains the only expansion
token, and a wrapper never fills one (§6.4).

Everything that a wrapper consumes — its bindings and its sources — is the wrapper's
own render data. It is invisible to selectors (§6.5). A binding inside a wrapper's
chain declares nothing to the rest of the manifest.

### 6.2 Blank Binding

A bullet with a name and **no source** takes the value of the preceding binding of
the same shape, through `toCamel` (§8):

```
- name: Screen
- instance:              instance is "screen"

- name: WindowFX
- instance:              instance is "windowFX"   FX is an abbreviation

- name: CSI
- instance:              instance is "CSI"        wholly an abbreviation
```

This is the identifier form of the preceding value — the conventional relation
between a type name and an instance name. A blank binding at the first position of
its shape has no predecessor, and its value is empty.

A binding — blank or valued — is also a **selector** mark. A row that declares it is
a row that participates wherever that binding's name is used as a source (§6.5).
Declaration is participation. A blank cell only changes where the value comes from,
never whether the engine sees the row. A binding inside a wrapper's chain (§6.1) is
not a declaration and never participates.

### 6.3 Structure

The engine reads the structure column from top to bottom. Two lines name a shape, by
its address (§4.2):

- `@template:fence` — the shape renders one time
- `- [list]: @template:fence` — the shape renders one time per item of the line's
  source (§6.5)

Both are **sources**: each fills one `:::[list]:::` occurrence of the shape above it.
The first line of the column is the row's own shape.

A named bullet binds one token of the nearest shape line above it (§6.1). A wrapper
(§6.1) is a shape line in this reading order: the bullets after it bind its tokens,
and the source lines after it are its own to consume.

### 6.4 Arity

A shape's arity is its count of `:::[list]:::` occurrences (§7). A shape consumes the
next _arity_ source lines, in order. Each consumed source consumes its own arity
first. Thus the template declares the nesting, and the author never authors it. To
supply **more** sources than the row's shapes demand is fatal (§10.1). To supply
fewer is legal: the authored sources fill a shape's **trailing** slots, and the
leading unfilled slots render empty and elide (§6.5). An under-supplied shape is the
authored way to use part of a template.

A wrapper (§6.1) counts like a shape line, not like a source. It adds its own arity
to the row's demand and supplies nothing — it fills a named token, never a
`:::[list]:::` slot.

`> ` count is indentation, and nothing else: one tab (four spaces) per `>`. This is
the **absolute** column — measured from column 0 of the output file — at which that
line's shape renders. No `>` is column 0. Indentation never selects a token, an
owner, or a nesting level.

The list and separator columns carry the same `>` count as the structure line that
they pair with, and the engine reads it: a line pairs with the line that carries the
same `>` count at the same ordinal within that count (§6.5). One symbol, one meaning,
in all three columns.

### 6.5 Expansion and Pairing

`:::[list]:::` is the only expansion token (§7). Each other token is a named token
and takes its value from, in order:

1. the binding of that name (§6.1) — for an item shape, the source row's own binding
2. the shape's **maps** — the row of that name in each map table, first declared
  first (below)
3. the column of that name — on the manifest row, or, for an item shape, on the
  source row
4. the table documentation (§5.4), for the name `[comment]` at shape level

**Maps.** A map is a table that the engine reads one row per token. The row whose
**first column** equals the token name supplies its `value` cell. `key | value` is
the plain form. `name | type | value | comment` is the same map with more payload.
Any table whose first column is an identity column (§5.3) is a map when a shape reads
it as one. The same table, iterated by a `- [list]:` expansion, is a record table.
Orientation belongs to the reader, never to the table.

A shape paragraph declares its maps in the list column, positionally, under the same
slot law as expansions. At a `>` count, the list column's `- [list]:` lines in excess
of the structure column's `- [list]:` lines at that count are map lines. The engine
reads them first, in authored order. Column-address lines (`@file:table:column`, the
inline sources below) are neither expansions nor maps, and the engine never counts
them: the address form is the discriminator, as it is for the inline marker. Map
lines group to that count's shape paragraphs by order. A blank `- [list]:` closes the
current paragraph's group and opens the next paragraph's group. Fewer groups than
paragraphs fill the trailing paragraphs, as fewer sources than arity fill a shape's
trailing slots (§6.4). A different `>` count is a different scope and needs no
placeholder. A map line with no shape paragraph at its count is fatal (§10.1). A map
table with no `value` column is fatal (§10.1).

`[comment]` at shape level skips rungs 2 and 3. A map row named `comment` is data. A
manifest row's comment column documents the row's items (§6), never the row's own
shape — shape documentation is the table channel (§5.4).

A supplier that carries no text renders empty. A line that its own placeholder leaves
empty elides (§7). A named token that no binding, column, or documentation names
renders empty the same way — an unsupplied token is empty text, never an error. The
template declares each slot that a file could carry. The data declares, per row,
which slots carry text.

Expansion pairs by order:

- The list column's `- [list]:` lines pair with the structure column's `- [list]:`
  lines by `>` count and by ordinal within that count (§6.4). The list column names
  **what iterates**. The structure line names **the shape** that each item renders
  through.
- A shape consumes the source lines that follow it, bounded by its arity (§6.4). Its
  `:::[list]:::` occurrences, in block order, take those sources in the same order.
- A separator line pairs with the `- [list]:` line of the same ordinal (§6.6).

A `- [list]:` source paired with a structure `- [list]:` line is one of:

- an **address** — `@file:table` iterates that table's rows
- a **column address** — `@file:table:column` names one column of the enclosing
  expansion's table. It feeds an **inline** `:::[list]:::` (§7), never a column-0 one
- a **cell-match filter** — `@file:table:column=value` (§4.2) iterates only that
  table's rows whose `column` cell equals `value`, byte-exactly. An empty `value`
  matches a blank cell (§5.1). Thus a filtered address, not a second table, splits
  one table into disjoint row sets
- a **column name** — iterates the distinct values of that column across output rows.
  The value that belongs to the writing row's own file is excluded: a file never
  lists itself
- a **binding name** — iterates the rows that declare a binding of that name (§6.2),
  blank or valued. A binding inside a wrapper's chain (§6.1) never counts

The same exclusion extends to the **address** form. When an address's table carries a
`file` column, the row whose `file` value equals the writing row's own file is
excluded from that table's iterated rows. A file never lists itself, whether the
source names a column or a table.

The engine tries a bare source name as a column first: a name that matches both a
column and a binding resolves as the **column**. A selector name thus must not
collide with any wiring-table column name. A binding-name source walks the wiring
tables in manifest order, and each table's rows in authored order. A column-name
source yields its distinct values in first-appearance order — never sorted. A row can
declare more than one selector binding. Each name selects its own row set, and two
selectors on one row feed two slots by ordinal, like any two sources.

An inline `:::[list]:::`'s sources are the column-address lines that follow the item
shape's own source line. They sit at the **same** `>` count, at consecutive ordinals,
in authored order — one line per column, and each line names its column. The current
row's value of each addressed column fills the marker, joined by the within-line join
(§6.6). No implicit column set exists: each consumed column is named.

A list-column line with no structure partner is a map line (above). A structure
`- [list]:` line, or a separator line past the row join, with no list-column line of
its ordinal is fatal (§10.1).

A shape-level `:::[comment]:::` (§5.4) falls back to the documentation of the first
table that the shape's own sources address, in authored order. When no source
addresses a table, it falls back to the documentation of the row's own table.

To match the template, the tables and the expression is the author's responsibility.
The engine reads the expression and generates. Its one check is the count: a row
that supplies more sources than its shapes demand is fatal (§6.4, §10.1).

### 6.6 Separator

The separator column carries `- [list]:` lines that mirror the list column's lines.
The line at ordinal K is the join text for the expansion wired at ordinal K. No line,
or a blank value, joins by newline. A value resolves like any bullet value (§6.1) — a
shape address names a block whose text is the join. An index alias names a datum,
formatted by the index's own `format` column (§4.1). That is how a join of one space
is authored (`U+0020`, `fromUTF8`, §9).

For an item shape whose `:::[list]:::` is inline (§6.5), the separator line at the
item source line's own ordinal is the **within-line join** — the text between the
addressed column values. The join between the items themselves is the newline
default. An inline marker declares no other item join.

The column's leading `>`-less line is the row join: it joins the diverging values of
the rows that declare the same `file` (§6.7). Its partner is those rows themselves —
it is exempt from the list-column partner requirement (§6.5), and the lines after it
carry the ordinals. No other separator mechanism exists.

### 6.7 File Merge

Rows that declare the same `file` render as one. Their first structure line is
byte-identical by authorship, and the engine emits it one time.

Merging is per line. At each structure line, the rows that carry the same value for
that line emit it one time. The rows whose values differ emit each value, in authored
row order, joined by the row join. The row join is the first row's row-join line,
resolved as §6.6 resolves it. One blank line frames a non-blank row join on each
side. The sources
that a line consumes (§6.4) merge inside that line by the same rule. Thus merging
follows the structure that the template declares and never crosses it. One row that
declares a file merges to itself.

Same-file rows are authored contiguously. A row that declares a file already closed
by an intervening row of another file is fatal (§10.1) — one file is written one
time.

### 6.8 File Documentation

A wiring table carries no documentation column. The structure column wires a file's
documentation: a `- [comment]:` binding whose value is a table address, on the file
group's **first** row. First appearance counts in authored order — the same grouping
law as §6.7, and the same law that the engine applies to output-file ordering.

The address is `@file:table` or `@file:table:column` (§4.2) — or, for a table of the
manifest itself, the local form `@table` or `@table:column`. The addressed table
carries a `file` column and documentation columns. The engine reads the row whose
`file` value equals the group's own output file name and takes the cell of the column
that the address names — the `comment` column when the address names none. The
addressed cell is plain text or the fenced documentation form (§3.2): each fence line
is one line of the file's documentation prose. Three cases write no file
documentation: a group whose first row carries no address-valued `- [comment]:`
binding, a file absent from the addressed table, and an addressed cell that is blank.
Each is plain replacement, no special case (§5.4).

The engine renders the resolved prose in the file's own comment syntax, exactly as a
shape-level `:::[comment]:::` renders (§5.4). The engine writes the prose immediately
after the banner, with one blank line on each side. It is part of the banner and goes
wherever the banner goes (§5.5): a file that places the banner with `:::[banner]:::`
places its documentation there too, and a file with no banner carries no file
documentation. The addressed table's documentation columns are documentation, never
data (§5.4). A fence there is not a literal, `toLiteral` never applies, and the `@`
sigil law (§4) does not apply inside a fence's prose. Documentation is never a
reference, thus a prose line that starts with `@file` or `@brief` is text. The
binding's own value is the one exception — it is a reference, and the engine resolves
it as it resolves each other address (§4.2).

The `@` sigil law (§4) is what separates this binding's two readers. A `- [comment]:`
binding whose value is plain text is per-item prose, exactly as before: an item-shape
reader finds the binding before it falls to the source row's own `comment` column,
and renders the bound text. The file-documentation reader reads only the
address-valued form. An item-shape reader, which renders prose, never treats a
reference as its prose. One structure cell can carry both — the reference for the
file header and the text for the row's items. The sigil decides which reader takes
which — the same rule that each `@`-sigiled value already follows (§4).

### 6.9 Toolchain

`## toolchain` is a reserved table name, and it is optional. The reservation is by
name, not by file. The engine looks up each `## toolchain` table across the whole
spliced document — each file that the manifest's index declares. Thus the table can
live in the manifest itself or in any declared data file. A project that separates
its codegen manifest from its toolchain data keeps `## toolchain` in the latter.

```
| argument | command | flag |
```

`argument` is optional. A table declared without it is `| command | flag |`. The
unknown-word fatal (§10.1) applies in both cases: a `--<word>` CLI argument that
matches no row's `argument` cell is fatal, whether the table declares the column or
not. A table with no `argument` column has no row whose `argument` cell could ever
equal a non-empty `word`, thus any non-empty `--<word>` against it is unconditionally
fatal. A blank `argument` cell marks the row as a **default-flow** row — a blank cell
is nothing, as it is everywhere (§5.1).

When the table is declared, its rows run after each declared output has written, in
authored row order — one child process per row. The command cell is the executable,
taken verbatim — argv[0], never split. The flag cell is the argument list. It splits
on spaces, and a double-quoted span becomes one argument with the quote marks
removed. A blank flag cell runs the command alone.

The CLI selects which rows run. With no `--<word>` argument, only the rows whose
`argument` cell is blank run — the default flow. With `cast <manifest> --<word>`,
only the rows whose `argument` cell equals `word`, byte-exactly, run. The
default-flow rows do not run. A `--<word>` that matches no row's `argument` cell,
across each declared `## toolchain` table, is fatal (§10.1), and the diagnostic names
`word`.

The command resolves through the caller's environment. PATH, the working directory,
and everything else that the process inherits are the caller's responsibility. The
engine adds no resolution, no shell, and no quoting of its own.

A row whose process cannot start, or exits nonzero, fails the run. The diagnostic
carries the row's own command text. The rows after it do not run. The declared
outputs are already on disk when the toolchain runs — a toolchain failure fails the
run, never the write.

### 6.10 Region

An output row whose structure declares a `- [begin]:` binding and a `- [end]:`
binding is a **region row**. Its `file` names a file that already exists — the row
does not create it, and a region row whose file does not exist is fatal (§10.1).

The binding values are delimiter text, resolved like any bullet value (§6.1). The
engine finds the first line of the file that contains the `[begin]` value, then the
first later line that contains the `[end]` value. The lines strictly between them are
the region. The engine replaces the region with the rendered shape,
write-if-different. The delimiter lines stay, and every line outside them stays,
byte-for-byte.

A region renders the shape and nothing else. No banner renders, and no file
documentation renders (§6.8) — the file already carries its own framing, and the
delimiter pair marks where the engine's ownership begins and ends.

A `[begin]` binding without an `[end]` binding, or the reverse, is fatal. A delimiter
value that matches no line, or an `[end]` value whose first match is at or before the
`[begin]` match, is fatal. One file is either region-written or whole-written: a file
shared between a region row and a whole-file row is fatal (§10.1). Same-file region
rows merge by the §6.7 law inside the one region.

### 6.11 Format

`## format` is a reserved table name, and it is optional. The reservation is by name,
not by file, exactly as `## toolchain` (§6.9): the engine looks the table up across the
whole spliced document. The table name and the reserved column name `format` (§5.2)
do not meet — one names a table, the other a cell.

```
| name | width |
```

`name` is a column name. `width` is the wrap width of every body cell in a column of
that name, in every grid table the run formats, a positive integer. `name` is an
identity column (§5.3), thus a column named two times is the duplicate fatal.

The table drives the formatter (§3.3) and nothing else. A column the table names
reflows at its width. A column it does not name keeps its natural width. No `## format`
table means no column reflows — the formatter's default. A split inserts a line break
into the cell, and a plain cell's line break reads as a newline (§3.3); an inline
comment's lines join at the output (§5.4). The author sets each named column's width
for that column's content: a backticked literal wider than the width splits, and its
value reads back with a space at the split (§3.3). A fenced cell's lines stay lines,
each split at the width (§3.3). A grid table whose body has no border between rows never splits (§3.3).

A header row that declares no `name` or no `width` column is fatal. A `width` cell that
is not a positive integer is fatal (§10.1). Both checks run during formatting only —
the table drives the formatter and nothing else.

---

## 7. Templates

A template file is any `.cast` file that the index declares. It holds fenced code
blocks, and the fence's info string is the block's id. A shape is addressed
`@template:fence` (§4.2). Two template files can declare the same fence id, because
each address names its file.

A block is literal output text and nothing else. Structure only — no conditionals, no
logic, no formatting. Target-language conditional directives (`#if`, `#endif`, and
their kin) are literal text like each other text: the engine neither reads nor
evaluates them, and slots inside such an arm take sources by the ordinary arity law
(§6.4).

`:::[list]:::` is the expansion token. A block's occurrence count is its arity
(§6.4). One source fills each occurrence — its items joined by that source's
separator (§6.6):

- a marker at **column 0** fills vertically: the source line's indent prefixes each
  line of the join (§6.4)
- a marker **inside a line** fills horizontally, in place, unindented

Marker position is the only axis. No axis vocabulary and no second mechanism exist.

Any other `:::token:::` is a named token. The binding, map row, or column of that
name replaces it (§6.5). Any symbol delimited by `:::` is a valid placeholder, and
the interior is its name **verbatim** — the engine never splits the interior, and
`:::name:operation:::` is not a form. The interior is free text (§4.3). The engine
matches the marker in the block exactly as authored, and pairing with bindings, maps,
and columns keys it as each other name is keyed — identically at both ends. An author
never shapes a token name for the engine. `:::macro-guard:::` and `:::macroGuard:::`
are each legal, and each pairs with the bullet spelled its own way. A template never
names an operation — the table declares operations (§5.2). A named token can occur
more than one time in a block, and each occurrence carries the same value. Two blocks
can name the same token: each shape's own suppliers feed its own occurrences — an
outer shape's binding never reaches into a wrapper's token of the same name (§6.1).

Delimiters, wraps, braces and punctuation are structure, and the author writes them
in the block — except the quotes around a literal, which `toLiteral` supplies (§9).
The engine renders comment frames from the comment-syntax table (§5.4).

### 7.1 File Tokens

A token fed by the `file` column carries the **file name**, not the declared path.
The manifest declares where a file is written. A template that names a file — an
include, for example — receives `jam_Identifiers.h`, never
`../diff/jam_Identifiers.h`.

### 7.2 Alignment

A join of more than one item, rendered from a single-line shape, aligns its token
columns. The engine inserts fill spaces into the literal between two tokens,
immediately after that literal's first whitespace run — at the literal's end when it
contains none. The fill size is the preceding token's deficit against the byte width
of that token's widest replacement across the join set. Fill never enters a token's
replacement, thus a token wrapped in literals (a quoted include path) emits verbatim.
Content before the first token and after the last token never pads — a single-token
shape emits unpadded, and no emitted line carries trailing whitespace. Multi-line
shapes, single-item joins, and single-line shapes that carry an inline `:::[list]:::`
render unpadded. No column limit and no wrapping exist — a long line stays long.

### 7.3 Fence Prefix

A fence's info string is its name (§7), and the name can open with one bracket group:

```
[sh]script
[no-banner]script
```

The group is part of the name. It is part of the address too, matched byte for
byte — `@code:[sh]script`, never `@code:script`. Nothing after the group is read: a
dash is an ordinary character, thus `[sh]-script` and `[sh]script` are two different,
equally legal names. One group per fence — a second group is a name, not a prefix.

Two words are legal inside the group:

- **an extension, written without its dot** — `[sh]`, `[py]`, `[xml]`. It is the
  file's comment-syntax key, and it outranks the output file's own extension and the
  manifest-syntax table alike (§5.4). This is how a file with no extension, or one
  whose extension does not name its language, declares what it is.
- **`no-banner`** — the file carries no banner (§5.5).

The fence always wins. The extension is a guess about the file's language. The prefix
is the author's statement about it.

Any other word is fatal, and so is a group that never closes (§10.1). The engine
reads no meaning into the word beyond these two lookups. An extension word is matched
against the comment-syntax table (§5.4), which is data, thus the engine learns no
language here either.

The engine gates each shape address, in each column that carries one — `structure`,
`separator` and `list` alike. The engine honours a `no-banner` prefix from those same
three columns: any fence of the render suppresses the banner, wherever it is wired.

An extension word answers for the whole output file, thus one fence answers: the
first shape line of the group's first row (§6.7). A prefix on a later fence of the
same file states nothing new and is not read.

---

## 8. Operations

Operations are optional. A table can carry finished text and use none.

The operation keywords are the only vocabulary that the engine hardcodes. They
transform a datum's own characters:

- **case** — `toUpper`, `toTitle`, `toPascal`, `toCamel`, `toKebab`, `toSnake`, `toScreamingSnake`
- **encoding** — `toLiteral`, `toUTF8`, `fromUTF8`, `toHex`, `toCodepoint`, `fromCodepoint`
- **text** — `join`, `toFileName`
- **comment** — `toComment`, `toCommentBlock`, `brief`, for the banner CAST stamps and
  the `:::[comment]:::` marker (§5.4)

In each case operation, an all-uppercase word is an abbreviation and passes through
with no change, in each position. `WindowFX` camel-cases to `windowFX`. `CSI` and
`C4Type` do not change.

A `format` cell names one of these operations and only one. Two operations are never
chained in a `format` cell — a datum that needs a different shape is authored in that
shape. The one composition that the engine performs is its own: `toLiteral` delimits
and escapes a backticked or fenced cell before the cell's operation applies (§9).

---

## 9. Safety Contract

CAST makes safe the text that it places inside a target-language literal. The author
never does.

`toLiteral` delimits and escapes as one operation: it quotes the value, escapes
backslashes and quotes, turns control characters into their named escapes, and turns
bytes above `0x7F` into hex escape sequences. Delimiting and escaping never travel
separately, and a template never authors the quotes itself.

The author writes a real line break, and `toLiteral` writes `\n`. The mapping from
control character to escape character is data, declared in a table like each other
datum, never a constant inside the operation.

When the author cannot write an escape as a real character, the author writes its
escape sequence, and `toLiteral` passes it through. A backslash followed by one of the
data-declared escape characters is an authored escape and survives verbatim. A
backslash followed by a backslash is one literal backslash, doubled on output. A
backslash followed by anything else is a literal backslash, doubled as before. `\n`
authors a line break, and `\\n` authors the two characters backslash-n — both remain
expressible.

When the author cannot write a space, the author writes `U+XXXX`. `fromUTF8` decodes each
such token in a datum back to the character that it names, and touches no other byte.
This is how a value carries a space at the very start or end of one of its lines,
where the formatter's own padding would otherwise be indistinguishable from it
(§3.2). The token is ordinary text to each other part of the engine — the reader has
nothing to strip, because no space is there to strip.

`toLiteral` and a `format` operation compose: the engine quotes and escapes a
backticked or fenced cell first, and the operation then applies to that value.

§9's guarantee covers text that CAST wraps into a string literal. A datum that is
already target-language source — a char literal like `'&'`, a type name, an
expression — is authored plain and passes verbatim. Its safety belongs to the author,
like a template's own structure.

The author writes the datum. CAST makes it legal.

---

## 10. Determinism and Failure

The tables, the templates, the manifest, and the binary determine the output bytes.
No timestamps, no paths, no host state. Files use LF.

Write-if-different. A second run produces an empty diff.

No warnings exist. Every failure is fatal, exits non-zero, and writes no output file.
A diagnostic names the true physical line:

```
file:line (column): rule
```

### 10.1 The Whole Fatal Set

These, and nothing else:

| Rule                                                                                                              | Clause     |
| ----------------------------------------------------------------------------------------------------------------- | ---------- |
| alias absent from the writing file's index                                                                        | §4.4       |
| duplicate alias in one index                                                                                      | §4.4       |
| index symbol names a file that does not exist                                                                     | §4.4       |
| address names a table or column that does not exist                                                               | §4.4       |
| `\| format \| format \|` adjacency                                                                                | §5.2       |
| `format` cell names an operation that is not in §8                                                                | §8         |
| duplicate entry within one identity column of one table                                                           | §5.3       |
| a shape address names a fence that does not exist in its template file                                            | §7         |
| a fence prefix naming neither a comment-syntax extension nor `no-banner`, or a bracket group that never closes    | §7.3       |
| a map line with no shape paragraph at its `>` count                                                               | §6.5       |
| a map table with no `value` column                                                                                | §6.5       |
| an output file that cannot be written                                                                             | §10        |
| a toolchain row whose process cannot start or exits nonzero                                                       | §6.9       |
| a `--<word>` CLI argument matching no toolchain row's `argument` cell                                             | §6.9       |
| malformed table, during formatting only                                                                           | §3.3       |
| index `symbol` cell empty                                                                                         | §4.1       |
| output row declares no structure                                                                                  | §6.3       |
| same-file output rows not contiguous                                                                              | §6.7       |
| a row supplies more sources than its shapes demand                                                                | §6.4       |
| a structure or separator `- [list]:` line without its list-column line of the same ordinal, the row join excepted | §6.5, §6.6 |
| duplicate binding name among one shape's bindings                                                                 | §6.1       |
| a comment reference naming neither a table nor a fence                                                            | §5.4       |
| unterminated `:::` marker in a shape block                                                                        | §7         |
| a `## toolchain` header row declaring no `command` or no `flag` column                                            | §6.9       |
| a region row whose file does not exist                                                                            | §6.10      |
| a `- [begin]:` binding without `- [end]:`, or the reverse                                                         | §6.10      |
| a region delimiter value matching no line, or `[end]` matching at or before `[begin]`                             | §6.10      |
| a file shared between region rows and whole-file rows                                                             | §6.10      |
| `user-modules-info.md` absent at a sync root                                                                      | §2.2       |
| a composed identity key absent from either sync file                                                              | §2.2       |
| a kernel row naming no directory at its root, or a `<filePrefix>*` directory undeclared — checked at each root    | §2.2       |
| kernel sets not corresponding one to one after the path transform                                                 | §2.2       |
| a source text file containing a pair's target value — names the file and the token                                | §2.2       |
| sync source root equals target root                                                                               | §2.2       |
| a `--sync` line without exactly two roots                                                                         | §2.1       |
| a sync source file that cannot be read                                                                            | §2.2       |
| a sync delete that fails                                                                                          | §2.2       |
| the manifest file does not exist                                                                                  | §2.1       |
| a `--line-wrap` value below 1, a negative `--max-table-width`, or a flag value that is not an integer             | §2.1       |
| a `## format` header row declaring no `name` or no `width` column, during formatting only                         | §6.11      |
| a `## format` `width` cell that is not a positive integer, during formatting only                                 | §6.11      |

Any check that the engine performs and that is not in this table is a defect in the
engine.

---

## 11. Engine Contract

This section binds the implementation, not the author.

### 11.1 The Document Is the Store

The parsed markdown is a complete AST. The parser builds it one time. It is
addressable in O(1) by `(parent, id)` and traversable in authored order by intrusive
child links. It holds every table, row, cell and bullet.

Nothing derived from it is copied into a second structure. No cache mirrors it. No
array is materialised to hold pointers that it already holds. No per-call container
is built to answer a question that it can already answer. A second copy of a truth
that the AST owns is a defect.

Where a value must be computed and not read — a cell's form (§5.1), a blank binding's
value (§6.2), a table's documentation (§5.4) — the parser stamps it one time onto its
own Element, beside the provenance that the parser already stamps. The engine reads
it back thereafter.

### 11.2 One Owner Per Invariant

The engine establishes each invariant exactly one time, at its owner, and each
downstream reader trusts it without conditions. A downstream re-check is a defect. A
null test for a case that the owner has excluded is a defect. A fallback value,
substituted where the owner has already guaranteed presence, is a defect. The
spelling — a guard, an assert, or a silent empty return — does not matter.

The validator alone owns validity. The reader and the writer decide nothing, test
nothing, and report nothing.

### 11.3 No Defensive Programming

A guard must name the specific scenario that it prevents. A guard that cannot is
removed, and the ownership that made it feel necessary is fixed in its place.

Failure is loud. A missing key throws where it is looked up. It does not yield an
empty string that travels downstream and emits blank output.

A key that a table never declares is not a missing key. A comment-syntax table
declares the marks that its language owns and no others (§5.4): CSS declares no
single-line mark, and `go.mod` declares no block frame. To read such a key answers
_this language has none_, and the reader takes the shape that the language does
declare. The table itself is the key that must exist — and an extension that names no
table is not an error either, because §5.4 resolves it to the C-family table.

---

## 12. Conformance

The engine implements superseded rules. Each is debt against this document:

- merging resolved per whole shape, keyed by the concatenation of every non-list
  token value, in place of per line (§6.7)
- each line's paired source walked at every read in place of stamped once at parse
  (§11.1) — template id, indentation and line are stamped
- per-call containers materialised where the document already holds the answer
  (§11.1): selector row collection, wrapper private-scope collection, item
  replacement and width maps, merge-group key arrays
- a wrapper's binding scope (§6.1) recovered positionally at read in place of held
  as one stamped ownership truth — a wrapper item is owned by its parent and owns
  its own chain, and the two roles currently share one stamp with a position
  tiebreak
- the template files' own token scan duplicated across the template pool and the
  marker primitives — layering keeps them apart
