# CAST: Codegen Annotated Source of Truth
**Version 0.0.1**

Welcome to the `cast` help page. If you are seeing this, you either ran `cast --help`, or `cast` could not find a `CAST.md` file in your current directory. 

CAST is a strictly deterministic code generator. It has no domain knowledge, no interpreter, and no conditional logic inside its templates. It simply takes your data (Tables), your code shapes (Templates), and your instructions (Manifest) to generate code.

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

CAST operates on exactly three types of files, all flat in one data directory. Generated output files are considered untracked build artifacts and should not be edited by hand.

### 1. Relations (Input Tables)
Your data lives in standard Markdown files as **Pandoc grid tables** — one format everywhere for consistency (`+---+` frame, `+===+` header separator); the parser reads GFM pipe tables as the same relation.

**Row law — column 0 is the key, and the key delimits rows:** a `|` line with a non-empty column-0 cell starts a row; a line with an empty column-0 cell continues the previous row, its cells joining by newline. Border lines are always visual — group separators, frames, emphasis — never semantics; author them wherever they aid reading, or nowhere. Multi-line cells (the manifest `placeholder` and `structure` columns) need no borders — their continuation lines carry an empty key by construction. Cells may contain arbitrary block content.
*   **Naming:** Any `## Heading` immediately followed by a table creates a "Relation" named after that heading.
*   **Keys:** The **first column (Column 0)** is the row key. Looking up a row by value always checks this column.

#### Map tables: `key | value`

Every map-type table — bimaps, classification maps, anything consumed as a
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
```
*   **Cells:** All cells become plain strings. Text inside backticks (`` ` ``) unwraps to its plain content. Numbers are kept exactly as you typed them—CAST will never reformat or reparse numbers.
*   **Order:** Rows are always output in the exact order you authored them. CAST never sorts rows.
*   **First-row law:** a map-type table's **first row is its default** — always, with no declaration. A `- default:` binding never exists. The law is realized as template text, never engine logic: a bimap's `getDefault()` is the literal line `return map.at (0);` in the template block.

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

When `lexicon.md` is present in the data directory and a source table's column-0 header is `entry`, each entry cell is resolved as a reference into the lexicon registry:

*   The entry cell is looked up **byte-exactly** in the lexicon's `name` column — a reference must match its declaration's casing verbatim. A reference differing only in case is an undeclared entity (FATAL).
*   On hit: `:::entry:::` fills with the **canonical name** (as declared in the lexicon, spaces and casing preserved). `:::value:::` fills with the referenced entity's value, passed through standard cell resolution and any placeholder transform tag.
*   On miss: **FATAL** — the entity is not declared.
*   When no `lexicon.md` is present, `entry`-headed tables expand without reference resolution (backward compatible).
*   **References are literal; format ops are declaration-scoped.** A reference cell is written as a byte-exact literal and is never passed through a format op — projection belongs solely to a declaration table's own emission site. Emission of a reference derives the entity's generated symbol from its declaration (see Reference Derivation, Transform Vocabulary below).

### 2. Templates (`template.cast`)

All code shapes live in **one file per data directory**: `template.cast`. It is an ordinary markdown file of fenced code blocks — **the fence's info string is the template's id**:

````markdown
```char
static constexpr const char* const :::key::: { :::value::: };
```

```pair
{ :::key:quoted:::, :::value::: },
```

```separator

//==============================================================================

```
````

*   **Reference form:** `template:<id>` — anywhere a shape is named (manifest structure and separator cells), `template:char` resolves to the code block whose info string is `char`. Exactly one `.cast` file is declared per manifest; a `template:` reference resolves against it. A missing id is FATAL.
*   **Explicit code.** A template block is the literal output text — keywords, braces, indentation, method bodies, doc comments are all authored verbatim. There is no abstract vocabulary: no keyword/type/open/close/prologue/epilogue/terminator bindings, no universal shapes, no fragments. What you read in the block is what the file will contain.
*   **Placeholder:** `:::name:::` — replaced through matched-replace (below). `:::name:transform:::` applies a registered transform to the resolved value (Transform Vocabulary, below) — `:::key:quoted:::` wraps the resolved key in double quotes, `:::key:toCamel:::` projects it to camelCase. Cells stay symbol-only; quoting and casing are transform tags in the template.
*   **Jacks expand vertical — always, and the jack is universal.** Every expansion jack is `:::line:::`: the wired source table's rows each build through the jack's bound shape, and the results join by `jam::Strings::joinIntoString` with the jack's separator entry (default: newline). A jack's **identity is its raw `> ` depth** in the manifest — uniform across the placeholder, structure, and separator columns — and that same depth is the indent of its expanded rows. A template with multiple `:::line:::` occurrences pairs them with the row's `- line:` wiring bullets in **authored order** (Nth occurrence ↔ Nth bullet). There is no horizontal axis — a horizontal row shape is simply authored as one line in its template block (`pair`).
*   **The `separator` block** is a template like any other — its text (the `//===` section rule) joins whatever the manifest wires it to, most commonly sibling constructs sharing one output file.
*   **Empty-token rule:** a placeholder is always replaced — with its resolved text when the token is defined, with the empty string otherwise. When the value is empty, the replacement target widens to the marker plus one whitespace character immediately preceding it — still replacement, never removal machinery. A line that contained a placeholder and reduces to whitespace emits nothing and suppresses the adjacent blank line.
*   **Author responsibility.** Template structure and placeholder tokens are authored truth. The engine executes them — it never interprets, repairs, or normalizes a template.

#### Engine Stamps

The engine itself stamps exactly two things — everything else is template text:

*   **The banner + `#pragma once`**: every generated file opens with the CAST banner (language-aware comment fencing) followed by the language's pragma. The banner art has one source, owned by the `cast` binary.
*   **Per-language comment syntax**: the comment transforms (`toComment`, `toCommentBlock`, `brief`) format language-aware by target-extension lookup against the `comments` relation — data holds plain literal strings, never comment markers.

#### Placeholder Names Are Data

The engine holds **no hardcoded placeholder names**. `line`, `files`, `instance`, `key`, `name` — every name in this guide is a data convention, never an engine symbol; even the universal jack name `line` is convention, matched by name + depth + authored order, never assumed. The engine's entire knowledge is the four reserved manifest columns (`placeholder | structure | separator | file`), the `:::` marker, the `@` sigil, the `template:` reference form, the `- key: value` bullet, and the `> ` depth marker.

**One mechanism: `jam::Strings::joinIntoString (separator)`.** Every expansion is the same operation: build the items, join by the resolved separator.

### 3. Manifest (`CAST.md`)
The manifest is the "brain" of the operation. Two tables are mandatory (`## index`, `## output`); `## output index` is optional. The manifest is fully self-describing: every cell's role is readable from the cell and its column, with zero engine conventions to memorize.

*   `## index` — the **alias index**: `| alias | symbol |`, optional third column `format`. Every alias carries the `@` sigil as **part of its name** (`@lexicon`, `@jam_Bimaps`) — the sigil is the reference marker: a `@`-sigiled cell anywhere in a file resolves against that file's own `## index`; a bare word is a literal. The two can never collide. A symbol is whatever the alias stands for — an input file path, an output file path, a type symbol. Every input file — the template file, every table file, the manifest itself — is declared in the manifest's index exactly once; CAST parses exactly what it declares: no directory scanning, no glob. A `format` cell names a Transform Vocabulary op applied to the symbol at resolution. Referencing an undeclared alias is FATAL.
*   `## output` — first-order generation, one row per generated construct: `| placeholder | structure | separator | file |`. These four column names are the **only** names the engine reserves. `placeholder` and `structure` cells are multi-line — continuation lines carry an empty key per the row law.

    #### Placeholder Cell Grammar — what data

    The placeholder cell **wires** the row's expansion jacks: each bullet `- line: value` (at its jack's depth) declares which data feeds that jack. The value's own form states the wiring — three forms, decided by construction, never by engine special-cases:

    *   **Table reference** (`> > > - line: @xml:XmlTokenType`) — the referenced table's rows are iterated, each row built through the jack's shape binding (structure column, same depth), the results joined by the jack's separator entry (default: newline).
    *   **Column name** (`- files: file`, second-order only) — the value names a column of the source table (`## output`). The jack expands over the column's **unique values** in authored row order, the emitting row's own file excluded.
    *   **Binding name** (`- line: instance`, second-order only) — the value matches no column; it names a structure **binding**. The jack expands over exactly the rows whose structure cell carries that binding at any depth — presence is the selection, pure data, no engine filter.

    **Depth law — `> ` count is the jack's identity AND its indent.** A wiring bullet's raw `> ` count identifies the jack — the same count addresses the same jack's entries in the structure and separator columns — and is the tab indent applied to **each expanded row**: no `> ` = column 0, `> > ` = two tabs (8 spaces), `> > > ` = three tabs (12 spaces). Template frames carry their own literal indentation; jack markers sit at column 0 in the template, and the wiring depth supplies the per-row indent. Multiple jacks in one template pair with the wiring bullets in authored order.

    #### Structure Cell Grammar — what shape

    The structure cell states the row's shape — containment chain plus bindings, nothing else:

    *   **A head line is a `template:<id>` reference** — the shape at its containment depth. Depth 0 is the outermost shape (`template:namespace`); a deeper head (`> template:bimap`) is a shape whose built output fills the parent shape's jack. A head's depth is pure containment — it carries no indent arithmetic (Depth law, above); only bullet depth does.
    *   **Bullets = bindings.** A bullet whose name+depth matches a wiring bullet is that jack's **shape binding** (`> > > - line: template:pair` — the shape each source row builds through). Any other bullet binds one placeholder of its containment depth's template: `- name: jam` fills the namespace block's `:::name:::`. Values follow the sigil law — `template:<id>` resolves a template block, `@`-sigiled resolves through `## index`, bare is literal. A value may contain placeholders resolved against the same scope's bindings — SSOT for names.

    **Parent fill.** A containing template has exactly one `:::line:::`; the child depth's assembled output fills it. A containing template with zero or multiple unfilled jacks after bindings apply is FATAL. Rows sharing an output `file` must declare byte-identical depth-0 shapes and bindings — mismatch is FATAL, never first-row-wins.

    #### Separator Cell Grammar — what join

    The separator cell names the join per jack, keyed identically to the wiring:

    *   `- line: template:separator` — the jack's rows joined by newline + the separator block's text + newline (the `//===` rule). Only scoped codeblock constructs use it — bimap, hashMap, lookupTable and kin; single-liner expansions (chars, identifiers, text) never do.
    *   Empty, or a jack with no same-named entry — joined by a plain newline (the default).
    *   A flat `template:separator` (no bullet) — the block's text joins **consecutive output rows sharing this row's file** (the row-sibling join — same mechanism, applied at file assembly).

*   `## output index` — optional, **second-order**: an output-shaped table whose expansion source is `## output` itself. Its wiring uses the second-order forms: a **column name** expands unique column values (`- files: file` — the `file` column of `## output` IS the include list, no separate headers table exists); a **binding name** expands the rows carrying that binding (`- line: instance` — every map row already declares `- instance:`, and that presence is the selection). The master include is emitted after the outputs, beside them, from them.

**Matched-replace is the only fill rule.** A placeholder fills by one uniform sequence — wiring declaration (placeholder column) → binding (structure column) → matched cell (source row) → direct `## index` lookup → replace. The data row's own cell is consulted first among cells, then the declaring row; a `@`-sigiled result resolves through the index; a `template:` result resolves a block; the resolved text replaces the placeholder. No fallbacks, no positional indices, no engine defaults — a placeholder with no match is FATAL (dead placeholder).

**Parity is enforced.** Per output row, every placeholder of its resolved templates must fill through matched-replace, and every wiring declaration and binding must be consumed by some placeholder. Expansion interiors validate against their source table's columns. No match, no cigar.

> **FATAL MANIFEST ERRORS:** CAST will immediately crash if it detects:
> *   An **undeclared alias** (a `@`-sigiled cell absent from the file's `## index`).
> *   A **duplicate alias** (two `## index` rows declaring the same alias).
> *   A **missing file** (an `## index` path that does not exist).
> *   A **missing template** (a `template:<id>` naming no code block in the declared template file).
> *   An **unresolvable table** (a `@alias:table` cell whose heading is absent from the aliased file).
> *   A **dead wiring** (a placeholder-column or structure-cell declaration no template placeholder consumes).
> *   A **dead placeholder** (a template placeholder no wiring, binding, or source-table column feeds).
> *   A **wrap mismatch** (rows sharing a file with non-identical depth-0 shapes).

---

## Canon Files

Every CAST-driven project declares its generation inputs in the canon files — all flat in one data directory beside the manifest:

| File | Role |
|------|------|
| `CAST.md` | Codegen Annotated Source of Truth — the manifest |
| `template.cast` | Every code shape, one fenced block per template id |
| `lexicon.md` | Every entity declared once: `\| name \| value \|` (registry) |
| `chars.md` | Single characters — generates the character constants (registry) |
| `files.md` | Filenames with extensions — generates the file constants (registry) |
| `colours.md` | Colour entities (registry) |
| `text.md` / `localisation-lang.md` | The long-text home — generates the text constants |
| `comments.md` | Per-language comment syntax — drives the engine's comment stamps |
| `xml.md`, `html.md`, `css.md`, `markdown.md`, `terminal.md`, `syntax.md`, `mermaid.md`, `gui.md`, `plugin.md`, `graphics.md` | Domain tables, one file per domain — every operator, bimap, lookup table, and hashmap belonging to that domain |

The reference registry is the union of the declaration tables — `## lexicon`, `## chars`, `## files`, `## colours`, and each language's own text table. A relation cell may reference an entity from any of them; the word is declared exactly once, in exactly one table.

**Declaration type follows dominant consumption, and type is data.** An entity consumed as an Identifier (tree keys, property keys, table lookups) declares an Identifier-type token; an entity consumed as a string (delimiters, map keys, emitted text, cell comparisons) declares a String-type token. The mechanism is the lexicon `type` column — explicit on every row, holding a `@`-sigiled alias (`@id`, `@string`) resolved through the lexicon file's own `## index` to the target language's type symbol (`juce::Identifier`, `juce::String`). CAST is language-agnostic: the engine never knows any type name — it delivers the resolved symbol like any other cell, and the project-owned template composes the declaration around it. No engine default exists — a default would smuggle one language's type into the engine.

All manifest and table files compile into **one master state document**: every `##`-headed table, from every file, becomes a sibling in a single tree, and generation reads only that tree.

Every table is addressed by its file: a reference is `@alias:heading` (`@lexicon:lexicon`, `@xml:XmlTokenType`, `@CAST:output`), where the alias resolves through `## index` and the heading inside that file. Two tables with the same heading inside one file are FATAL (reported as already-declared, with both locations); the same heading in two different files is two different tables — the qualifier is the namespace.

**Entity rules:**
*   An entity is unique, whole, and opaque. `UI`, `scale`, and `UI scale` are three independent declarations — CAST never decomposes or derives one from another.
*   Uniqueness is global and case-insensitive. First declaration wins and fixes the casing.
*   Word boundaries (spaces) and per-word casing are stored data — the declaration is the single source of every projected form. The declared casing is exactly what `:::entry:::` emits.
*   Casing at emission belongs to the template, via transform tags backed by the closed Transform Vocabulary's case family (`toTitle`/`toPascal`/`toCamel`/`toKebab`). An all-uppercase declared word is an abbreviation and passes through `toTitle`/`toPascal`/`toCamel` intact — see Transform Vocabulary below for the full rule and examples (`fail hazard URI` → `failHazardURI`).
*   A value wrapped in backticks is a byte-exact literal — every authored character, including leading/trailing spaces and raw non-ASCII bytes, survives untouched. This holds even though `lexicon.md` and the domain tables are themselves markdown: CAST reads a backtick-wrapped cell's pre-formatting source text, not markdown's rendered form, so authored whitespace is never trimmed and multi-byte glyphs are never corrupted by markdown's own inline rules.
*   Every use outside the declaration — relation cells, template tags — is a reference, resolved **byte-exactly against the declared canonical form** (case-insensitivity applies only to the uniqueness constraint, never to lookup). Referencing an undeclared entity — including a declared word in the wrong casing — is a **FATAL** generation error:
    > `css.md: entity not declared in lexicon: myMissingEntity`
*   A row whose column-0 cell is solely dash characters (one or more `-`, nothing else) is a visual separator, not data. GFM parses it as an ordinary row, but CAST skips it everywhere a row is enumerated — the lexicon registry, row-matching, and uniqueness/constraint validation.
*   A `name` column entry is validated at declaration: **FATAL** if it is a plain number, starts with a digit, contains any character outside `[a-z A-Z 0-9 space]`, or exceeds 40 characters.
*   A `value` that byte-equals a case-family projection of its own `name` (`toTitle`/`toPascal`/`toCamel`/`toKebab`/`toSnake`/`toScreamingSnake`) is **FATAL** — redundant data the template already projects. Delete the value and let the template project the name.

**Division of labor:** the table is the mapping. The template is a cookie cutter — placeholder substitution only. Logic belongs to the engine, never the table or the template.

**Generation pipeline — documents build and write themselves:**

1. The **Model** parses once — manifest + tables spliced into one master state document, the operational chain.
2. The **template file** parses once through the markdown parser — each fenced block addressable by its info string, shared and immutable.
3. Generation iterates the **Model**: each output row **builds** its own output document — placeholders resolved through matched-replace, jacks resolved (rows built per source row, joined by `joinIntoString (separator)`, indented per wiring depth). Resolution happens at build, into the tree — never at emission.
4. Emission iterates the **output documents**: each document **writes** its own target file. The document owns its serialization; no external walker re-inspects the tree, no per-node dispatch at write time.

Build and write are the only two operations. An engine component that scans a document's internals node-by-node to produce output is the violation signature — the document was asked, not told.

---

## Cookbook: Table + Template → Code

Every generated construct is plain text substitution — nothing more. These examples show each pairing end to end. In all of them an `## output` row wires the circuit through its placeholder column.

### 1. Identifier constants — declaration table + scalar placeholders

```markdown
## core
| key        | string     |
| ---------- | ---------- |
| id         |            |
| blockQuote | blockquote |
| dataWidth  | toKebab    |
```
````markdown
```identifier
inline const juce::Identifier :::name::: { juce::String::fromUTF8 (":::value:::") };
```
````
```cpp
// generated — empty cell = row key; a cell naming a transform = transform of the row key
inline const juce::Identifier id { "id" };
inline const juce::Identifier blockQuote { "blockquote" };
inline const juce::Identifier dataWidth { "data-width" };
```

### 2. Char constants inside a specialised struct — jack at column 0, wiring depth = tabs

````markdown
```chars
struct Chars
{
    :::line:::

    static constexpr const char* const special { "..." };

    static bool isNumeric (juce::juce_wchar c) noexcept
    {
        return (c >= zero and c <= nine) or c == dot or c == dash;
    }
};
```

```wchar
static constexpr juce::juce_wchar :::name::: { :::value::: };///< :::comment:::
```
````
The `chars` block is **explicit code** — the fixed members and the method are literal template text, not bindings. `:::line:::` sits at column 0; the manifest wires it at depth 1 (`> - line: @chars:chars`), so each expanded `wchar` row lands one tab in. The frame's own indentation is authored in the block.

### 3. Namespace wrap — containment, not columns

```markdown
+---------------------------+---------------------------+-----------+---------------+
| placeholder               | structure                 | separator | file          |
+===========================+===========================+===========+===============+
| > - line: @chars:chars    | template:namespace        |           | @jam_Chars    |
|                           | - name: jam               |           |               |
|                           |                           |           |               |
|                           | > template:chars          |           |               |
|                           | > - line: template:wchar  |           |               |
+---------------------------+---------------------------+-----------+---------------+
```
Depth 0 = the `namespace` block (`- name: jam` fills its `:::name:::`); depth 1 = the `chars` block, whose built output fills the namespace's single `:::line:::` (parent fill). The depth-1 jack wires `@chars:chars` rows through the `wchar` shape at one tab. The separator column stays **empty** — single-liner expansions join by plain newline; the `//===` separator belongs only to scoped codeblock constructs (Cookbook 5). Head depth is containment only; bullet depth is jack identity + indent.

### 4. HashMap — pair rows, quotes from the transform tag

````markdown
```hashMap
inline const jam::HashMap<:::keyType:::, :::valueType:::> :::name:::
{
:::line:::
};
```

```pair
{ :::key:quoted:::, :::value::: },
```
````
```cpp
inline const jam::HashMap<juce::String, int> tokens
{
    { "text", 0 },
    { "placeholder", 1 },
};
```
The `pair` block serves every map-entry shape; `:::key:quoted:::` wraps the symbol-only cell in double quotes at emission — quotes are never authored in cells. Type symbols with `<`/`>` are backtick-wrapped in `## index` to clear the hazard.

### 5. Bimap — two jacks over one source

````markdown
```bimap
/** :::comment::: */
struct :::name::: : public jam::Bimap<:::name:::>
{
    :::name:::()
    {
        map = {
:::line:::
        };
    }

    enum value : int
    {
:::line:::
    };

    const juce::String& getDefault() const noexcept override
    {
        return map.at (0);
    }
    ...
};
```

```enum
:::name::: = :::value:::,///< :::comment:::
```
````
```markdown
| placeholder                    | structure                      | separator          | file        |
| ------------------------------ | ------------------------------ | ------------------ | ----------- |
| > > > - line: @xml:            | template:namespace             | template:separator | @jam_Bimaps |
|   XmlTokenType                 | - name: jam                    |                    |             |
| > > - line: @xml:XmlTokenType  |                                |                    |             |
|                                | > template:bimap               |                    |             |
|                                | > - name: XmlTokenType         |                    |             |
|                                | > > > - line: template:pair    |                    |             |
|                                | > > - line: template:enum      |                    |             |
```
One source table, two jacks, two shapes — the depth-3 jack builds each row through `pair` at three tabs, the depth-2 jack through `enum` at two tabs; the bimap block's two `:::line:::` occurrences pair with the wiring bullets in authored order. The method block, the CRTP base, `map.at (0)` (First-row law) — all literal template text. **Ordinals are arbitrary data** — the `value` column declares every ordinal explicitly; reordering rows never changes a value.

### 6. Second order — the master include, from the outputs themselves

The master-index file has its own dedicated block — literal code, include jack at the top (before the namespace opens), member jack inside the struct:

````markdown
```generated
:::files:::

namespace jam::map
{

struct Generated
{
:::line:::
};

}
```
````
```markdown
## output index

| placeholder        | structure                          | separator | file           |
| ------------------ | ---------------------------------- | --------- | -------------- |
| - files: file      | template:generated                 |           | @jam_Generated |
| > - line: instance | - files: template:include          |           |                |
|                    | > - line: template:sharedInstance  |           |                |
```
`- files: file` names a **column** of `## output` → unique file values, authored order, self excluded → one `#include` per file via the `include` block. `- line: instance` names a **binding** → exactly the rows declaring `- instance:` emit a member via the `sharedInstance` block. No column is added anywhere — presence of the binding is the selection. (The block sketch above is illustrative — the authored block reproduces the oracle's exact bytes.)

### 7. Spec key table — component words + `format` override

```markdown
## Entity
| key              | value  | format        |
| ----------------- | ------ | ------------- |
| aacute             | U+00E1 |               |
| measured angle     | U+2221 | join          |
| differential d     | U+2146 | DifferentialD |
```
`key` carries the wire token split into its component words, each word keeping the spec's exact casing — case is the differentiator for a spec pair, never a baked-in transform. `format` resolves the wire form: empty leaves `key` verbatim for a single-word token; `join` removes the spaces for a multi-word token, reassembling it byte-exact because every word already carries its true casing; a literal wire token (`DifferentialD`) overrides both when the readable `key` deliberately diverges from mechanical reconstruction.

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
5.  `quoted` — Wraps the value in double quotes, content untouched. The map-key transform: cells stay symbol-only, the template tag supplies the quotes.
6.  `toUTF8` — Converts `U+XXXX` codepoint tokens into UTF-8 `\xNN` byte escape sequences.
7.  `toHex` — Converts a glyph or `0xNN` cell into a minimal lowercase `0xNN` integer literal.
8.  `toCodepoint` — Converts a glyph or `0xNN` cell into zero-padded `U+XXXX` codepoint notation.
9.  `toSymbol` — Qualifies a two-part symbol with its owning namespace (e.g., `A::b` becomes `juce::A::b`). Leaves three-or-more part symbols verbatim.
10. `toPascal` — Uppercases the first letter of each space-separated word and joins them, abbreviations passing through whole: `xml operators` becomes `XmlOperators`; `UI scale` becomes `UIScale`.
11. `toCamel` — Joins space-separated words into camelCase: a non-abbreviation first word lowercases in full; an abbreviation first word passes through intact. `hex prefix` becomes `hexPrefix`; `UI scale` becomes `UIScale`; `fail hazard URI` becomes `failHazardURI`. Single non-abbreviation words lowercase in full: `Generated` becomes `generated`.
12. `toSnake` — Underscore-joins space-separated words: non-abbreviation words lowercase; abbreviations pass through intact: `clamp to border` becomes `clamp_to_border`; `scale X` becomes `scale_X`.
13. `toScreamingSnake` — Uppercases space-separated words and underscore-joins them: `clamp to border` becomes `CLAMP_TO_BORDER`.
14. `join` — Removes the spaces between words, case untouched: `Long left arrow` becomes `Longleftarrow`. Used by spec key tables (Cookbook, above).
15. `toComment` / `toCommentBlock` / `brief` — the comment family: format plain literal text as the target language's comment, block comment, or doc-brief — language chosen by the output file's extension against the `comments` relation (Engine Stamps, above).

**Reference Derivation.** References never take a format op. A cell that references an entity declared elsewhere (lexicon, chars, files, a declared enum family) is written as a byte-exact literal, and emission derives the entity's **own generated symbol** from its declaration — `colon` emits `Chars::colon`, `begin` emits `Id::begin`. One declaration, one symbol, every reference emits it — that derivation is why the canon registries are globally unique. Format ops exist only where a declaration table emits its own tokens; a transform tag applied to a reference is a conflation error (FATAL).

**Non-ASCII keys.** A table key may contain any character — accented colour names, subscripts, symbols. Two separate rules keep the generated source portable. The identifier-producing transforms (the case family) fold each non-ASCII character to its ASCII equivalent before joining, so `Blanc Cassé` projects `blancCasse`; a character with no known equivalent collapses to a single underscore. The string-producing path leaves the characters alone and lets an escaping transform render them as byte escapes, so the authored spelling survives verbatim in the emitted literal. Generated source therefore need never carry a raw byte above 127, whatever the tables hold.

### Per-Placeholder Transforms
A placeholder names a transform after its interior colon — `:::column:transform:::` — and fills with that transform applied to the column's resolved cell value for the row being expanded. A transform applies to its own placeholder only; the plain `:::column:::` emits the resolved cell verbatim. The transform name must be in the closed transform vocabulary above.

```cpp
static constexpr juce::juce_wchar :::name::: { :::value:toHex::: };///< :::value:toCodepoint:::
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
*   **Tables:** every column padded to its widest cell; alignment rows canonical (`:---`, `:---:`, `---:`); pipes escaped as `\|`. **Pandoc grid tables are formatted exactly like pipe tables** — same width rule, interior borders re-emitted in their authored positions, `+===+` at the header boundary, and **the closing bottom border always emitted** — a table authored without one gains it in canonical form.
*   **Malformed rows fail fast:** a grid-table row whose authored cell count disagrees with the header's column count is malformed data — the file is reported with its `path:line` and **never rewritten**. The formatter never pads, truncates, or guesses a cell boundary.
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
*   A template placeholder with no matching source reports where it happened, which output row was being built, and everything that *was* available:
    > ```
    > template.cast (identifier, :::key:::): placeholder has no source
    >   output row: jam_Identifiers.h (source: lexicon:lexicon)
    >   available here: :::name::: :::value:::
    > ```
    The classic cause: the template names a placeholder the source table does not carry as a column — the *available here* list shows what the table actually provides.
*   A duplicate declaration names both the offender and the original, each at its true `file:row (column)` location:
    > `css.md:9 (name): duplicate "closeParen" already declared at lexicon.md:41 (name)`
*   A lexicon `name` failing declaration validation names the violated rule and the offending name:
    > `lexicon.md:5 (name): name is a plain number: 123`
    > `lexicon.md:6 (name): name starts with a digit: 1abc`
    > `lexicon.md:7 (name): name contains a character outside [a-zA-Z0-9 space]: bad$name`
    > `lexicon.md:8 (name): name exceeds 40 characters: aVeryLongNameThatOverflowsTheFortyCharacterLimit`
*   A redundant lexicon value names the matching transform:
    > `lexicon.md:12 (name): value byte-equals toPascal projection of name; delete it, let the template project it`
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
