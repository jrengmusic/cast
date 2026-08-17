# CAST: Codegen Annotated Source of Truth
**Version 0.0.1**

Welcome to the `cast` help page. If you are seeing this, you either ran `cast --help`, or `cast` could not find a `CAST.md` file in your current directory. 

CAST is a strictly deterministic code generator. It has no domain knowledge, no interpreter, and no conditional logic inside its templates. It simply takes your data (Tables), your formatting (Templates), and your instructions (Manifest) to generate code.

---

## Quick Start: Command Line Interface

CAST is controlled entirely via the command line and the `CAST.md` manifest file. It never accepts generation rules via arguments—arguments are only for *selecting* what to run.

*   `cast` — Finds `CAST.md` in the current directory and regenerates **all** declared outputs, then formats every declared markdown file to canonical form.
*   `cast <path>/CAST.md` — Regenerates all outputs using a specific manifest file.
*   `cast CAST.md <output>` — Regenerates **only one** specific output file declared in the manifest.
*   `cast CAST.md --format` — Formats every declared markdown file to canonical form. **No code generation.**
*   `cast CAST.md --no-format` — Generates outputs, skips the formatting pass.
*   `cast --version` — Prints the version and source commit (this same stamp is embedded in generated file banners).
*   `cast --help` — Prints this guide and exits successfully.
*   *No `CAST.md` found* — Prints this guide and exits with an error code.

Default behavior is **generate, then format**: after a successful generation the declared markdown corpus (manifest + tables) is rewritten to canonical form, write-if-different (Canonical Markdown, below).

---

## The 3 Artifact Types

CAST operates on exactly three types of files. Generated output files are considered untracked build artifacts and should not be edited by hand.

### 1. Relations (Input Tables)
Your data lives in standard Markdown files as **Pandoc grid tables** — one format everywhere for consistency (`+---+` frame, `+===+` header separator); the parser reads GFM pipe tables as the same relation.

**Row law — column 0 is the key, and the key delimits rows:** a `|` line with a non-empty column-0 cell starts a row; a line with an empty column-0 cell continues the previous row, its cells joining by newline. Border lines are always visual — group separators, frames, emphasis — never semantics; author them wherever they aid reading, or nowhere. Multi-line cells (the manifest `structure` column) need no borders — their continuation lines carry an empty key by construction. Cells may contain arbitrary block content.
*   **Naming:** Any `## Heading` immediately followed by a table creates a "Relation" named after that heading.
*   **Keys:** The **first column (Column 0)** is the row key. Looking up a row by value always checks this column.

#### Map tables: `key | value`

Every map-type table — bimaps, rules, classification maps, anything consumed as a
keyed lookup — has exactly the columns `key | value`. The `key` cell is the lookup
token; the `value` cell is explicit data. **Ordinals are arbitrary data**: an enum's
values are authored in the `value` column, never derived from row position. Nonzero
starts, gaps, and bit flags are all just cells.

```markdown
## template token type

| key          | value |
| ------------ | ----- |
| text         | 0     |
| placeholder  | 1     |
| region open  | 2     |
| region close | 3     |
```
*   **Cells:** All cells become plain strings. Text inside backticks (`` ` ``) unwraps to its plain content. Numbers are kept exactly as you typed them—CAST will never reformat or reparse numbers.
*   **Order:** Rows are always output in the exact order you authored them. CAST never sorts rows.

**Lingua law.** Column 0's header states its own semantics — three lanes, never mixed: `name` **declares** (registries only: `lexicon`, `chars`, `files`, `colours`); `entry` **refers** (lexicon-resolved, registry-validated, FATAL on a miss); `key` **indexes** (raw lookup data local to its own table, never resolved against the lexicon, emitted as-is).

#### Cell Resolution
Before a placeholder's transform tag is applied, each cell value is resolved against the row key:
*   An **empty cell** resolves to the row key (column-0 value).
*   A cell whose text **names a transform** (e.g. `toUpper`) resolves to that transform applied to the row key.
*   Any **other cell** is its own literal value.
*   A cell **wrapped in backticks** is a literal and is never resolved — no transform lookup, no empty-cell resolution.
*   The **first column (the row key itself)** is never resolved.
The resolved value is then passed through any declared column transform.

> **FATAL HAZARD RULE:** If a cell contains `<`, `>`, or a URI scheme (like `http://`) and it is *not* wrapped in backticks, CAST will crash. Always wrap special characters in backticks.

#### Lexicon Reference Resolution

When `lexicon.md` is present in the tables directory and a patched source table's column-0 header is `entry`, each entry cell is resolved as a reference into the lexicon registry:

*   The entry cell is looked up **byte-exactly** in the lexicon's `name` column — a reference must match its declaration's casing verbatim. A reference differing only in case is an undeclared entity (FATAL).
*   On hit: `:::entry:::` fills with the **canonical name** (as declared in the lexicon, spaces and casing preserved). `:::value:::` fills with the referenced entity's value, passed through standard cell resolution and any placeholder transform tag.
*   On miss: **FATAL** — the entity is not declared.
*   When no `lexicon.md` is present, `entry`-headed tables expand without reference resolution (backward compatible).
*   **References are literal; format ops are declaration-scoped.** A reference cell is written as a byte-exact literal and is never passed through a format op — projection belongs solely to a declaration table's own emission site. Emission of a reference derives the entity's generated symbol from its declaration (see Reference Derivation, Transform Vocabulary below).

### 2. Templates (Formatting)
Templates are plain text files with the `.cast` extension containing exactly one special construct: the tripleColon marker. There are no loops, no `if` statements, and no expressions.

*   **Placeholder:** `:::name:::` — replaced by the same-named cell of the row being built. `:::name:transform:::` applies a registered transform to the resolved cell (Transform Vocabulary, below).
*   **Region:** `:::name:begin:::` … `:::name:end:::` — the body between the markers repeats once per row of the region's source table. Region markers consume their trailing newline; placeholders do not.
*   **Marker rule:** a marker opens at `:::` not followed by another `:`; it closes at the leftmost `:::`. The interior splits at its first `:` into name and word — the word is either a registered transform (placeholder) or a rule keyword (`begin` / `end`).
*   **Separator:** consecutive rows sharing an output file are joined by the text named in the row's `lineBreak` column — a `#alias:table:entry` reference into a text table; empty cell = no separator.
*   **Selection is emergent:** a source row expands only when every jack the region references carries a signal (non-empty cell); an empty referenced cell drops the row.
*   **Empty-token whitespace rule:** when a token binding resolves to empty text, the token AND exactly one whitespace separator immediately preceding it in the fragment text are both elided. `:::keyword::: :::type::: :::name:::` with an empty `type` emits `keyword name`, not `keyword  name`. An empty `:::prologue:::` followed by a newline emits no blank line.
*   **Logic:** any actual code logic (function bodies, framework calls) lives as plain text in the template. Templates do not execute logic.

#### Fragment Vocabulary

Two universal shapes cover the regular output constructs. Fragments contain zero hardcoded keywords — `namespace`, `struct`, `static constexpr`, `inline const` are all token data, never fragment text. New fragments only for genuinely new *shapes*; never for new keywords.

`Definition.cast` — one declaration with initializer, repeated once per source-table row:
```
:::keyword::: :::type::: :::name:toCamel::: { :::value::: };
```

`Scope.cast` — a named scope receiving its content through `:::code:::`:
```
:::keyword::: :::type::: :::name:::
{
:::prologue:::
:::code:::
:::epilogue:::
}:::terminator:::
```

`namespace Id` = `#scope` with keyword `namespace`, name `Id`, terminator `// namespace :::name:::`. `struct XmlOperators` = `#scope` with keyword `struct`, terminator `;`. `static constexpr const char* const x { "value" };` = `#definition` with keyword `static constexpr`, type `const char* const`. One grammar, any language — the engine knows zero syntax.

#### Jack Vocabulary

| Jack | Meaning |
|------|---------|
| `keyword` | The leading keyword(s): `namespace`, `struct`, `static constexpr`, `inline const` |
| `type` | The type token: `const char* const`, `juce::Identifier`, or empty (namespaces have no type) |
| `name` | The declared name: `Id`, `XmlOperators`, `declarationOpen` |
| `value` | The initializer value |
| `code` | Reserved: receives the built content of child fragments (nesting target) |
| `prologue` | Paired scope boundary — text after the opening brace (e.g. decorative banner) |
| `epilogue` | Paired scope boundary — text before the closing brace |
| `terminator` | Text after the closing brace: `;` for struct, `// namespace :::name:::` for namespace |

Dedicated fragments remain for genuinely irregular shapes: Bimap (methods, enum, singleton), Chars (isNumeric, special array), Generated (includes, instances). These are not decomposable into the three universal shapes without loss.

### 3. Manifest (`CAST.md`)
The manifest is the "brain" of the operation. Two tables are mandatory; further output-shaped tables are optional. The manifest is fully self-describing: every cell's role is readable from the cell and its column, with zero engine conventions to memorize.

*   `## index` — the **alias index**: `| alias | symbol |`, optional third column `format`. Every alias carries the `#` sigil as **part of its name** (`#lexicon`, `#id`) — the sigil is the reference marker: a `#`-sigiled cell anywhere in a file resolves against that file's own `## index`; a bare word is a literal. The two can never collide. A symbol is whatever the alias stands for — an input file path, an output file path, a type symbol (`juce::Identifier`). Every input file — every template, every table file, the manifest itself — is declared in the manifest's index exactly once; CAST parses exactly what it declares: no directory scanning, no glob. A `format` cell names a Transform Vocabulary op applied to the symbol at resolution. Referencing an undeclared alias is FATAL. Interior border lines (grid tables) and dash-only rows group the index visually without declaring anything.
*   `## output` — first-order generation, one row per generated construct: `| code | list | structure | lineBreak | file |`. `structure` cells are multi-line — their continuation lines carry an empty `code` key per the row law. `code` names the row's body template; `list` (and any further truthfully-named source columns) feeds the body's regions as `#alias:table`; `lineBreak` names the separator text joining consecutive rows sharing a file (`#alias:table:entry`); `file` is the output target. `structure` replaces the old token column and all wrapper columns:

    #### Structure Cell Grammar

    The structure cell is valid CommonMark + pandoc block markdown. Two constructs, nothing else:

    *   **Root-level bullets = the row's token bindings.** `- key: value` — each bullet binds one jack of the row's templates: `- keyword: static constexpr` fills `:::keyword:::`. Values follow the sigil law — `#`-sigiled resolves through `## index`, `#alias:table:entry` resolves to that table row's value, bare is literal. A value may contain jacks (`- terminator: // namespace :::name:::`) resolved against the same scope's bindings — SSOT for names. An explicit-empty binding (`- prologue:`) renders nothing. **A `default` token is never declared — the default is always the source table's first row.**
    *   **Quoted paragraphs = the wrap chain, inward → outward.** `> #scope: Name` wraps the row's built body through its `:::code:::` jack; its own tokens are bullets at the same depth; `> > ` is the next wrap outward. Blank line separates the bullet group from each quote block; every line inside a quote carries its `> ` prefix; bullets never nest — ownership is quote depth, never indentation.

    Rows sharing an output `file` must declare byte-identical outermost wraps — mismatch is FATAL, never first-row-wins.

    ```markdown
    +-------------+-------------------+-------------------------------------------+-----------+----------------+
    | code        | list              | structure                                 | lineBreak | file           |
    +=============+===================+===========================================+===========+================+
    | #definition | #xml:xmlOperators | - keyword: static constexpr               | #break    | #jam_Operators |
    |             |                   | - type: #cString                          |           |                |
    |             |                   |                                           |           |                |
    |             |                   | > #scope: XmlOperators                    |           |                |
    |             |                   | > - keyword: struct                       |           |                |
    |             |                   | > - terminator: ;                         |           |                |
    |             |                   |                                           |           |                |
    |             |                   | > > #scope: Id                            |           |                |
    |             |                   | > > - keyword: namespace                  |           |                |
    |             |                   | > > - terminator: // namespace :::name::: |           |                |
    +-------------+-------------------+-------------------------------------------+-----------+----------------+
    ```

    Reading the row: `#definition` iterates `## xmlOperators`, emitting one declaration per row; the result wraps into `struct XmlOperators { ... };`, which wraps into `namespace Id { ... }// namespace Id`. Six operator structs = six rows sharing `#jam_Operators`, joined by the `lineBreak` text, sharing one identical outermost namespace. Nesting depth never adds columns — it adds `> ` markers.
*   `## output index` — optional, **second-order**: an output-shaped table whose list source is `## output` itself (`#CAST:output`). The `file` column of `## output` IS the header list — no separate headers table exists. The master include is emitted after the outputs, beside them, from them. It never folds into `## output`: a `struct` wrapper is not a `namespace` wrapper — same bones, different flesh, different table.

**Junctions are named jacks.** A table column is an output jack; a template placeholder is an input jack. Same name = same circuit — no mapping syntax exists. Column names are honest labels, not engine vocabulary: rename the region and the column together and the circuit is intact.

**Matched-replace is the only fill rule.** A template jack fills by one uniform sequence — template token → token binding → matched cell → direct `## index` lookup → replace. The data row's own cell is consulted first, then the declaring row; a `#`-sigiled result resolves through the index; the resolved text replaces the jack. No fallbacks, no positional indices, no engine defaults — a jack with no match is FATAL (dead placeholder).

**Region membership is token-driven.** A region whose column declares its rows expands rows carrying a cell. A region fed by a token vocabulary — any row in the source table declares the token — expands only token-declaring rows. Otherwise every row expands.

**Parity is enforced.** Per output row, the placeholder union of its resolved templates must equal its non-reserved columns; list-region interiors validate against their source table's columns instead. No match, no cigar.

> **FATAL MANIFEST ERRORS:** CAST will immediately crash if it detects:
> *   An **undeclared alias** (a `#`-sigiled cell absent from the file's `## index`).
> *   A **duplicate alias** (two `## index` rows declaring the same alias).
> *   A **missing file** (an `## index` path that does not exist).
> *   An **unresolvable table** (a `#alias:table` cell whose heading is absent from the aliased file).
> *   An **orphan column** (a non-reserved column no template placeholder consumes).
> *   A **dead placeholder** (a template placeholder no column or source-table column feeds).

---

## Canon Files

Every CAST-driven project declares its generation inputs in the canon files:

| File | Role |
|------|------|
| `CAST.md` | Codegen Annotated Source of Truth — the manifest; also home to the identity and paths tables |
| `lexicon.md` | Every entity declared once: `\| name \| value \|` (registry) |
| `chars.md` | Single characters — framework-owned, generates `namespace chars` (registry) |
| `files.md` | Filenames with extensions — generates `namespace files` (registry; folds the former `extensions.md`) |
| `colours.md` | Colour entities (registry) |
| `localisation-lang.md` | The only long-text home, one file per language (e.g. `localisation-en.md`) — generates `namespace text::lang` (registry) |
| `xml.md`, `html.md`, `css.md`, `markdown.md`, `terminal.md`, `syntax.md`, `mermaid.md`, `gui.md`, `plugin.md`, `graphics.md` | Domain tables, one file per domain — every operator, bimap, lookup table, and hashmap belonging to that domain |

The reference registry is the union of the declaration tables — `## lexicon`, `## chars`, `## files`, `## colours`, and each language's own `localisation-lang.md` text table. A relation cell may reference an entity from any of them; the word is declared exactly once, in exactly one table.

**Declaration type follows dominant consumption, and type is data.** An entity consumed as an Identifier (tree keys, property keys, table lookups) declares an Identifier-type token; an entity consumed as a string (delimiters, map keys, emitted text, cell comparisons) declares a String-type token. The mechanism is the lexicon `type` column — explicit on every row, holding a `#`-sigiled alias (`#id`, `#string`) resolved through the lexicon file's own `## index` to the target language's type symbol (`juce::Identifier`, `juce::String`). CAST is language-agnostic: the engine never knows any type name — it delivers the resolved symbol to the `type` jack like any other cell, and the project-owned template composes the declaration around it. No engine default exists — a default would smuggle one language's type into the engine. A `.toString()` projection at nearly every call site is the violation signature — the declared type is wrong, not the call sites.

All manifest and table files compile into **one master state document**: every `##`-headed table, from every file, becomes a sibling in a single tree, and generation reads only that tree.

Every table is addressed by its file: a reference is `#alias:heading` (`#lexicon:lexicon`, `#template:template token type`, `#CAST:output`), where the alias resolves through `## index` and the heading inside that file. Two tables with the same heading inside one file are FATAL (reported as already-declared, with both locations); the same heading in two different files is two different tables — the qualifier is the namespace.

**Entity rules:**
*   An entity is unique, whole, and opaque. `UI`, `scale`, and `UI scale` are three independent declarations — CAST never decomposes or derives one from another.
*   Uniqueness is global and case-insensitive. First declaration wins and fixes the casing.
*   Word boundaries (spaces) and per-word casing are stored data — the declaration is the single source of every projected form. The declared casing is exactly what `:::entry:::` emits.
*   Casing at emission belongs to the template, via transform tags backed by the closed Transform Vocabulary's case family (`toTitle`/`toPascal`/`toCamel`/`toKebab`). An all-uppercase declared word is an abbreviation and passes through `toTitle`/`toPascal`/`toCamel` intact — see Transform Vocabulary below for the full rule and examples (`fail hazard URI` → `failHazardURI`).
*   A value wrapped in backticks is a byte-exact literal — every authored character, including leading/trailing spaces and raw non-ASCII bytes, survives untouched. This holds even though `lexicon.md` and the domain tables are themselves markdown: CAST reads a backtick-wrapped cell's pre-formatting source text, not markdown's rendered form, so authored whitespace is never trimmed and multi-byte glyphs are never corrupted by markdown's own inline rules.
*   Every use outside the declaration — relation cells, template tags — is a reference, resolved **byte-exactly against the declared canonical form** (case-insensitivity applies only to the uniqueness constraint, never to lookup). Referencing an undeclared entity — including a declared word in the wrong casing — is a **FATAL** generation error:
    > `tables/css.md: entity not declared in lexicon: myMissingEntity`
*   A row whose column-0 cell is solely dash characters (one or more `-`, nothing else) is a visual separator, not data. GFM parses it as an ordinary row, but CAST skips it everywhere a row is enumerated — the lexicon registry, patch-cable row-matching, and uniqueness/constraint validation:
    ```markdown
    | name    | value |
    | ------- | ----- |
    | alpha   | 1     |
    | ------- |       |
    | beta    | 2     |
    ```
    The `| ------- |` row above is invisible to CAST — `alpha` and `beta` are the only two entities declared.
*   A `name` column entry is validated at declaration: **FATAL** if it is a plain number, starts with a digit, contains any character outside `[a-z A-Z 0-9 space]`, or exceeds 40 characters.
*   A `value` that byte-equals a case-family projection of its own `name` (`toTitle`/`toPascal`/`toCamel`/`toKebab`/`toSnake`/`toScreamingSnake`) is **FATAL** — redundant data the template already projects. Delete the value and let the template project the name:
    > `tables/lexicon.md:12 (name): value byte-equals toPascal projection of name; delete it, let the template project it`

**Division of labor:** the table is the mapping. The template is a cookie cutter — placeholder substitution only. Logic belongs to the engine, never the table or the template.

**Generation pipeline — documents build and write themselves:**

1. The **Model** parses once — manifest + tables spliced into one master state document, the operational chain.
2. Each template file parses once into a **grammar tree** — the parsed shape of the template language (text, placeholders, regions), shared and immutable.
3. Generation iterates the **Model**: each output row **builds** its own output document — a state tree constructed from the grammar tree against the Model: placeholders resolved to cells, regions expanded (wrapper spliced at its `code` slot, list body replicated once per source row, joined by the separator). Resolution happens at build, into the tree — never at emission.
4. Emission iterates the **output documents**: each document **writes** its own target file. The document owns its serialization; no external walker re-inspects the tree, no per-node dispatch at write time.

Build and write are the only two operations. An engine component that scans a document's internals node-by-node to produce output is the violation signature — the document was asked, not told.

---

## Cookbook: Table + Template → Code

Every generated construct is plain text substitution — nothing more. These examples show each pairing end to end. In all of them an `## output` row wires the circuit: its list column holds the source table as `name:table`, and the region in the body template matching that column's name collects the expanded rows.

### 1. Identifier constants — declaration table + scalar placeholders

```markdown
## core
| key        | string     |
| ---------- | ---------- |
| id         |            |
| blockQuote | blockquote |
| dataWidth  | toKebab    |
```
```cpp
// Identifiers.cast (region body)
inline const juce::Identifier :::key::: { ":::string:::" };
```
```cpp
// generated — empty cell = row key; a cell naming a transform = transform of the row key
inline const juce::Identifier id { "id" };
inline const juce::Identifier blockQuote { "blockquote" };
inline const juce::Identifier dataWidth { "data-width" };
```

### 2. String constants — literals survive via backticks

```markdown
## localisationText
| key       | string   |
| --------- | -------- |
| buttonOk  | OK       |
| cliPrefix | `--`     |
```
```cpp
// Text.cast (region body)
inline const juce::String :::key::: { ":::string:::" };
```
```cpp
inline const juce::String buttonOk { "OK" };
inline const juce::String cliPrefix { "--" };
```

### 3. Operator struct — structure cell nesting

```markdown
## xmlOperators
| name            | value   |
| --------------- | ------- |
| declarationOpen | `"<!"` |
| tagClose        | `">"`   |
```
```cpp
// Definition.cast — #declaration is an inline fragment reference
#declaration { :::value::: };
```
```cpp
// Scope.cast — #declaration is an inline fragment reference
#declaration
{
:::prologue:::
:::code:::
:::epilogue:::
}:::terminator:::
```
```markdown
## output (structure cell of the row: code #definition, list #xml:xmlOperators, file #jam_Operators)
- keyword: static constexpr
- type: #cString

> #scope: XmlOperators
> - keyword: struct
> - terminator: ;

> > #scope: Id
> > - keyword: namespace
> > - terminator: // namespace :::name:::
```
```cpp
// generated — declarations from the source table, wrapped by the structure's wrap chain
namespace Id
{
struct XmlOperators
{
    static constexpr const char* const declarationOpen { "<!" };
    static constexpr const char* const tagClose { ">" };
};
}// namespace Id
```
One row per struct: six rows share `#jam_Operators`, joined by the `lineBreak` text, each with its own depth-1 struct scope and one byte-identical outermost namespace scope. Nesting depth is `> ` count, never a column.

### 4. Bimap — `key|value` map table

```markdown
## screen
| key  | value |
| ---- | ----- |
| main | 0     |
| alt  | 1     |
```
```cpp
// Bimap.cast (excerpt)
struct :::list:toPascal::: : public jam::Bimap<:::list:toPascal:::, juce::String, int>
{
    enum value : int
    {
:::list:begin:::
        :::key:toCamel::: = :::value:::,
:::list:end:::
    };
};
```
```cpp
struct Screen : public jam::Bimap<Screen, juce::String, int>
{
    enum value : int
    {
        main = 0,
        alt = 1,
    };
};
```
**Ordinals are arbitrary data.** The `value` column declares every ordinal explicitly — nothing is ever derived from row position. Nonzero starts, gaps, and bit flags are ordinary cells; reordering rows never changes a value.

### 5. Wiring it up — one `## output` row

```markdown
## index (excerpt)

| alias   | symbol                  |
| ------- | ----------------------- |
| #bimap  | template/Bimap.cast     |
| #scope  | template/Scope.cast     |
| #screen | tables/screen.md        |
| #Screen | ../gen/Screen.h         |

## output (excerpt)

+--------+----------------+-------------------------------------------+-----------+---------+
| code   | list           | structure                                 | lineBreak | file    |
+========+================+===========================================+===========+=========+
| #bimap | #screen:screen | > #scope: map                             |           | #Screen |
|        |                | > - keyword: namespace                    |           |         |
|        |                | > - terminator: // namespace :::name::: |           |         |
+--------+----------------+-------------------------------------------+-----------+---------+
```
Reading the row: body `#bimap`, region fed by the `## screen` table from `tables/screen.md`, wrapped by a namespace scope named `map`, written to `../gen/Screen.h`. Every cell's role is visible: `#`-sigiled values resolve through `## index`, `#alias:table` values are sources, bare values are jack literals. Nesting is `> ` depth, not column count.

### 6. Spec key table — component words + `format` override

```markdown
## Entity
| key              | value  | format        |
| ----------------- | ------ | ------------- |
| aacute             | U+00E1 |               |
| Aacute             | U+00C1 |               |
| measured angle     | U+2221 | join          |
| Long Left Arrow    | U+27F5 | join          |
| differential d     | U+2146 | DifferentialD |
```
```cpp
// HashMap.cast (region body) — from/to derived from the table's own shape:
// key column -> fromLiteral, `U+XXXX` value cells -> fromCodepoint
            { :::from:::, :::to::: },
```
```cpp
// generated — the from-transform reads the key source (format-else-key), the to-transform the value cell
            { juce::String::fromUTF8 ("aacute"), ... },
            { juce::String::fromUTF8 ("Aacute"), ... },
            { juce::String::fromUTF8 ("measuredangle"), ... },
            { juce::String::fromUTF8 ("LongLeftArrow"), ... },
            { juce::String::fromUTF8 ("DifferentialD"), ... },
```
`key` carries the wire token split into its component words, each word keeping the spec's exact casing — case is the differentiator for a spec pair (`aacute` / `Aacute`), never a baked-in transform. `format` resolves the wire form: empty leaves `key` verbatim for a single-word token; `join` removes the spaces for a multi-word token, reassembling it byte-exact because every word already carries its true casing; a literal wire token (`DifferentialD`) overrides both when the readable `key` deliberately diverges from mechanical reconstruction.

### 7. LookupTable — `entry` + fixed prefix in the template

```markdown
## cssCodePoints
| entry | string |
| ----- | ------ |
| colon | colon  |
| comma | comma  |
```
```cpp
// LookupTable.cast (region body) — the qualifying scope is plain template text
            { Id::CssTokenType:::::entry:::, chars:::::string::: },
```
```cpp
            { Id::CssTokenType::colon, chars::colon },
            { Id::CssTokenType::comma, chars::comma },
```

### 8. Lexicon + Relations — declare once, reference by `entry`

```markdown
## lexicon
| name             | value                |
| ----------------- | --------------------- |
| fail hazard URI   | contains URI scheme  |
| hex prefix        | 0x                    |
```
```markdown
## string
| entry            |
| ------------------ |
| fail hazard URI   |
```
```cpp
// Text.cast (region body)
inline const juce::String :::entry:toCamel::: { ":::value:::" };
```
```cpp
// generated — :::entry::: resolves the domain-table cell against the lexicon (byte-exact);
// :::entry:toCamel::: projects the declared name; :::value::: fills the lexicon's own value column
inline const juce::String failHazardURI { "contains URI scheme" };
```
`fail hazard URI`'s all-uppercase word (`URI`) survives `toCamel` intact — the abbreviation rule (Canon Files, above), not a special case for this entity. The relation table names its column `entry` — never `name` or `key` — because it is a pure reference into the lexicon; the lexicon is the only place a `name` column ever appears.

---

## Transform Vocabulary (Closed Set)
These are the only text transformations CAST can perform, applied via placeholder transform tags — `:::column:transform:::`. All six case-family transforms (`toTitle`, `toPascal`, `toCamel`, `toKebab`, `toSnake`, `toScreamingSnake`) share one rule: **an all-uppercase declared word is an abbreviation and is case-invariant — in every projection, every position** — `UI`, `URI`, `ID` never get title-cased, Pascal-cased, or lowercased into `Ui`/`Uri`/`Id`, whether they sit first, last, or anywhere between. Every other word normalizes strictly per each projection's own rule; position rules apply to normal words only. The entire vocabulary is owned by `jam::Format` — no transform is reimplemented cast-side; the engine registers the shared functions directly.

1.  `toUpper` — Converts the value to uppercase.
2.  `toTitle` — Converts to titlecase (useful for display names). Each word's first letter uppercases, the rest lowercases — except abbreviations, which stay exactly as declared: `ui scale` becomes `UI Scale`.
3.  `toKebab` — Converts to kebab-case (e.g., `my-variable`). Non-abbreviation words lowercase and hyphen-join; abbreviations pass through intact: `data width` becomes `data-width`; `fail hazard URI` becomes `fail-hazard-URI`. Non-spaced input uses the existing kebab conversion.
4.  `toLiteral` — Renders the value as string-literal text: a bare `"` becomes `\"`, non-ASCII bytes become `\xNN`. Cells carry escape semantics: an authored backslash and the character after it pass through as one untouched pair, so `\n` means a newline, `\\` means one literal backslash, and `\"` means one quote — the author writes escape sequences, the transform never re-escapes them.
5.  `toUTF8` — Converts `U+XXXX` codepoint tokens into UTF-8 `\xNN` byte escape sequences.
6.  `toHex` — Converts a glyph or `0xNN` cell into a minimal lowercase `0xNN` integer literal.
7.  `toCodepoint` — Converts a glyph or `0xNN` cell into zero-padded `U+XXXX` codepoint notation.
8.  `toSymbol` — Qualifies a two-part symbol with its owning namespace (e.g., `A::b` becomes `juce::A::b`). Leaves three-or-more part symbols verbatim.
9.  `toPascal` — Uppercases the first letter of each space-separated word and joins them, abbreviations passing through whole: `xml operators` becomes `XmlOperators`; `UI scale` becomes `UIScale`; `fail hazard URI` becomes `FailHazardURI`. Single-word input follows the same rule — a non-abbreviation word capitalizes its first letter, an abbreviation is already correct.
10. `toCamel` — Joins space-separated words into camelCase: a non-abbreviation first word lowercases in full; an abbreviation first word passes through intact — an abbreviation never lowercases, in any position. Every remaining word Pascal-joins per the `toPascal` rule above. `hex prefix` becomes `hexPrefix`; `UI scale` becomes `UIScale`; `fail hazard URI` becomes `failHazardURI`. Single non-abbreviation words lowercase in full: `Generated` becomes `generated`.
11. `toSnake` — Underscore-joins space-separated words: non-abbreviation words lowercase; abbreviations pass through intact: `clamp to border` becomes `clamp_to_border`; `scale X` becomes `scale_X`. A wire format that demands a lowercased abbreviation authors the literal value instead of projecting.
12. `toScreamingSnake` — Uppercases space-separated words and underscore-joins them: `clamp to border` becomes `CLAMP_TO_BORDER`.
13. `join` — Removes the spaces between words, case untouched: `Long left arrow` becomes `Longleftarrow`. Used by spec key tables (Cookbook, above), where each word already carries its own exact casing.

**Reference Derivation.** References never take a format op. A cell that references an entity declared elsewhere (lexicon, chars, files, extensions, a declared enum family) is written as a byte-exact literal, and emission derives the entity's **own generated symbol** from its declaration — `colon` emits `chars::colon`, `begin` emits `Id::begin`, `region open` under the Byte family emits `map::Byte::regionOpen`. One declaration, one symbol, every reference emits it — that derivation is why the canon registries are globally unique. Format ops exist only where a declaration table emits its own tokens; a transform tag applied to a reference is a conflation error (FATAL).

**Non-ASCII keys.** A table key may contain any character — accented colour names, subscripts, symbols. Two separate rules keep the generated source portable. The identifier-producing transforms (the case family) fold each non-ASCII character to its ASCII equivalent before joining, so `Blanc Cassé` projects `blancCasse`; a character with no known equivalent collapses to a single underscore. The string-producing path leaves the characters alone and lets an escaping transform render them as byte escapes, so the authored spelling survives verbatim in the emitted literal. Generated source therefore need never carry a raw byte above 127, whatever the tables hold.

### Per-Placeholder Transforms
A placeholder names a transform after its interior colon — `:::column:transform:::` — and fills with that transform applied to the column's resolved cell value for the row being expanded. A transform applies to its own placeholder only; the plain `:::column:::` emits the resolved cell verbatim. The transform name must be in the closed transform vocabulary above.

```cpp
inline constexpr juce::juce_wchar :::key::: { :::glyph:toHex::: };///< :::glyph:toCodepoint:::
```

---

##  Predicate Vocabulary (Closed Set)
These are the only validation rules CAST can enforce. The canon laws (uniqueness, name form, hazard, parity, manifest integrity) are hardcoded in the engine and always run — laws are never manifest data:

1.  `matches <regex>` — The cell must match the provided regular expression.
2.  `unique` — The cell value must be unique within the column (can span multiple tables if declared as a shared registry).
3.  `existsIn <table>.<column>` — **Foreign Key:** The cell value must match a row key (Column 0) in the specified target table.
4.  `oneOf a|b|c` — The cell value must be one of the pipe-separated values (an empty cell is permitted if it is explicitly listed in the set).
5.  `range` — A numeric cell must be between the row's declared `min` and `max` columns.
6.  `parity <table>.<column>` — Enforces key-set equality across tables (e.g., ensuring localization languages match perfectly).
7.  `fileExists <root>` — The cell value must resolve to an actual file under the declared root directory.
8.  `onePerGroup <column>` — Ensures exactly one row is marked per distinct value in the specified group column.

---

## Canonical Markdown (Formatter)

CAST formats its own inputs. After every successful generation (or on demand via `--format`), each declared markdown file is parsed, validated against the markdown spec, and rewritten in canonical form — write-if-different.

*   **Spec:** CAST's markdown parser/formatter implements the CommonMark (CMARK) spec plus Pandoc's grid-table extension. Canonical form: `-` bullets, sequential ordered lists, 2-space nested indent, ATX headings, `---` breaks, backtick fences, `**strong**` / `_emphasis_`, prose preserved as authored (no re-wrapping), single final newline, LF.
*   **Tables:** every column padded to its widest cell; alignment rows canonical (`:---`, `:---:`, `---:`); pipes escaped as `\|`. **Pandoc grid tables are formatted exactly like pipe tables** — same width rule, interior borders re-emitted in their authored positions, `+===+` at the header boundary.
*   **Spec gate:** a file failing markdown-spec validation is reported and **never rewritten** — the formatter refuses to touch what it cannot faithfully re-emit.
*   **Idempotent:** `format(format(x)) == format(x)`. A canonical file re-formats to itself, byte-identical.
*   **Lossless:** formatting is layout-only. Cell content, backtick literals, authored row order, and group borders survive untouched; generation output is byte-identical before and after formatting.

---

## Determinism & Failure Behavior

CAST is designed to be perfectly reproducible.

*   **Total Function:** Output bytes are strictly determined by your tables, templates, `CAST.md`, and the `cast` binary. 
*   **Clean Output:** Files always use LF line endings. There are no timestamps, no absolute paths, no hostnames, and no environment variables in the output.
*   **Write-If-Different:** CAST will only touch an output file if the newly generated bytes differ from the existing file. Running CAST twice will always result in an empty diff (fixpoint).

### How Failures Work
There are no warnings. Every failure is **FATAL**. 
*   CAST will exit with a non-zero code.
*   **No output files will be written** (it fails before the write phase begins).
*   The error message will explicitly name the file, row, column, and the violated rule, formatted like this:
    > `float row 1 (preamp gain): default outside [min, max]`
*   A template placeholder with no matching column reports where it happened, which output row was being built, and everything that *was* available:
    > ```
    > template/LookupTable.cast:13 (:::key:::): placeholder has no source: :::key:::
    >   output row: Screen.h (list source: screen:screen)
    >   available here: :::key::: :::value:::
    > ```
    The classic cause: the template names a placeholder the source table does not carry as a column — the *available here* list shows what the table actually provides.
*   A duplicate declaration names both the offender and the original, each at its true `file:row (column)` location:
    > `tables/cssCodePoints.md:9 (name): duplicate "closeParen" already declared at tables/cast.md:41 (name)`
*   A lexicon `name` failing declaration validation names the violated rule and the offending name:
    > `tables/lexicon.md:5 (name): name is a plain number: 123`
    > `tables/lexicon.md:6 (name): name starts with a digit: 1abc`
    > `tables/lexicon.md:7 (name): name contains a character outside [a-zA-Z0-9 space]: bad$name`
    > `tables/lexicon.md:8 (name): name exceeds 40 characters: aVeryLongNameThatOverflowsTheFortyCharacterLimit`
*   A redundant lexicon value names the matching transform:
    > `tables/lexicon.md:12 (name): value byte-equals toPascal projection of name; delete it, let the template project it`
*   Every `file:row` in a diagnostic is the row's true physical line in the source file — not its ordinal position among the table's rows. A table preceded by prose, blank lines, or separator rows still reports the line you'd land on in an editor.

*(Note: Because CAST fails during the configure phase, a failing CAST run will prevent your compilation toolchain from even starting.)*

---

## CMake Integration Contract

If you are integrating CAST into a CMake build system, adhere to these strict rules:

*   **No Discovery:** Frameworks must carry the exact, tracked per-platform `cast` binaries (macOS + Windows). Do not use `find_program()`. Do not assert versions.
*   **Internal Source Path:** The absolute source of truth for the binary is `~/Documents/Poems/dev/cast`.
*   **Invocation:** Use `codegen.cmake`. It must be the **very first include** before `project()`.
*   **Execution Flow:** `codegen.cmake` dispatches to the carried binary via `execute_process (cast CAST.md)`.
*   **Dependencies:** The list of inputs for `CMAKE_CONFIGURE_DEPENDS` is derived entirely from the manifest—never maintain this list by hand.
*   **Role of CMake:** CMake is strictly a consumer and dispatcher. It must never implement codegen logic itself.
*   **Output Structure:** Generated output is header-only, grouped by construct type. Root templates and manifest structure remain identical across platforms/frameworks.
