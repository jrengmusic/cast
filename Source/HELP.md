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

**Row law — column 0 is the key, and the key delimits rows:** a `|` line with a non-empty column-0 cell starts a row; a line with an empty column-0 cell continues the previous row, its cells joining by newline. Border lines are always visual — group separators, frames, emphasis — never semantics; author them wherever they aid reading, or nowhere. Multi-line cells (the manifest `placeholder` and `structure` columns) need no borders — their continuation lines carry an empty key by construction. Cells may contain arbitrary block content.
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
```
*   **Cells:** All cells become plain strings. Text inside backticks (`` ` ``) unwraps to its plain content. Numbers are kept exactly as you typed them—CAST will never reformat or reparse numbers.
*   **Order:** Rows are always output in the exact order you authored them. CAST never sorts rows.
*   **First-row law:** a map-type table's **first row is its default** — always, with no declaration. A `- default:` binding never exists; wherever a generated construct needs a default (a bimap's `getDefault()`), it is derived from row 0.

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

*   **Placeholder:** `:::name:::` — replaced through matched-replace (below). `:::name:transform:::` applies a registered transform to the resolved value (Transform Vocabulary, below).
*   **Empty-token rule:** a placeholder is always replaced — with its resolved text when the token is defined, with the empty string otherwise. When the value is empty, the replacement target widens to the marker plus one whitespace character immediately preceding it — still replacement, never removal machinery. `:::keyword::: :::type::: :::name:::` with an empty `type` emits `keyword name`, not `keyword  name`. A line that contained a placeholder and reduces to whitespace emits nothing and suppresses the adjacent blank line — an empty `:::prologue:::` followed by a newline emits no blank line.
*   **Logic:** any actual code logic (function bodies, framework calls) lives as plain text in the template. Templates do not execute logic.

#### The Two Universal Templates

Two shapes cover every regular output construct. Templates contain zero hardcoded keywords — `namespace`, `struct`, `static constexpr`, `inline const` are all binding data, never template text.

`Definition.cast` — one horizontal declaration, repeated per source row. `:::list:::` is the row's columns joined by the `list` separator; `:::open:::`/`:::close:::` are data-driven delimiters — `{`/`}` for declarations, `=`/(empty) for enum entries; empty = elided:
```
:::keyword::: :::type::: :::name:toCamel::: :::open::: :::list::: :::close::::::terminator::: :::comment:toComment:::
```

`Scope.cast` — a named vertical scope. `:::line:::` is the vertical expansion slot — source rows expanded one-per-row, joined by the `line` separator (default: newline):
```
:::files:::

:::keyword::: :::type::: :::name:::
{
:::prologue:::

:::line:::

:::epilogue:::
}:::terminator:::
```

`namespace Id` = `@scope` with keyword `namespace`, name `Id`, terminator `// namespace :::name:::`. `struct XmlOperators` = `@scope` with keyword `struct`, terminator `;`. `static constexpr const char* const x { "value" };` = `@definition` with keyword `static constexpr`, type `const char* const`, open `{`, close `}`. One grammar, any language — the engine knows zero syntax.

#### Fragments

Genuinely irregular fixed text — a bimap's method block, a chars epilogue — lives in a **fragment**: an ordinary `.cast` file declared in `## index` and referenced by alias, exactly like the two universal templates. A fragment may contain placeholders; they resolve through matched-replace against the owning scope's bindings and source row. A single-line fragment may alternatively live directly in its `## index` symbol cell as a backtick literal (`@include` → `` `#include ":::file:toFileName:::"` ``, `@sharedInstance` → `` `SharedInstance<map:::::name:::>` ``) — the alias is the routing either way. New fragments exist only for genuinely new *shapes*; never for new keywords.

#### Placeholder Names Are Data

The engine holds **no hardcoded placeholder names**. `line`, `list`, `files`, `instance`, `keyword` — every name below is a data convention, never an engine symbol. The engine's entire knowledge is the four reserved manifest columns (`placeholder | structure | separator | file`), the `:::` marker, the `@` sigil, the `- key: value` bullet, and the `> ` depth marker.

**One mechanism: `jam::Strings::joinIntoString (separator)`.** Every expansion — vertical rows, horizontal columns, sibling rows sharing a file — is the same operation: build the items, join by the resolved separator.

#### Conventional Placeholder Vocabulary

| Placeholder | Meaning (by convention — all data) |
|-------------|------------------------------------|
| `keyword` | The leading keyword(s): `namespace`, `struct`, `static constexpr`, `inline const` |
| `type` | The type token: `const char* const`, `juce::Identifier`, or empty (namespaces have no type) |
| `name` | The declared name: `Id`, `XmlOperators`, `declarationOpen` |
| `open` | Opening delimiter before the list/value: `{` for declarations, `=` for enum entries, empty = elided |
| `close` | Closing delimiter after the list/value: `}` for declarations, empty = elided |
| `line` | Vertical expansion: source rows expanded through the structure's `- line:` shape, joined by the separator's `- line:` text |
| `list` | Horizontal expansion: the current row's column values joined by the separator's `- list:` text |
| `files` | Include directives — second-order expansion over unique `file` values (Manifest, below) |
| `instance` | Second-order member expansion — one per output row carrying an `- instance:` binding (Manifest, below) |
| `comment` | Comment text, applied via a comment transform (e.g. `:::comment:toComment:::`, `:::comment:brief:::`) — plain literal strings in data; the engine formats language-aware by target-extension lookup |
| `prologue` | Paired scope boundary — text after the opening brace (e.g. decorative banner) |
| `epilogue` | Paired scope boundary — text before the closing brace |
| `terminator` | Text after the closing brace: `;` for struct, `// namespace :::name:::` for namespace |

Maps, bimaps, lookup tables, hashmaps, and enums all decompose to Scope + Definition + fragments. Two expansions of the same source in one scope are two differently-named placeholders (`entry` for a bimap's map-init rows, `value` for its enum rows), each wired independently — names are free because names are data.

### 3. Manifest (`CAST.md`)
The manifest is the "brain" of the operation. Two tables are mandatory (`## index`, `## output`); `## output index` is optional. The manifest is fully self-describing: every cell's role is readable from the cell and its column, with zero engine conventions to memorize.

*   `## index` — the **alias index**: `| alias | symbol |`, optional third column `format`. Every alias carries the `@` sigil as **part of its name** (`@lexicon`, `@id`) — the sigil is the reference marker: a `@`-sigiled cell anywhere in a file resolves against that file's own `## index`; a bare word is a literal. The two can never collide. A symbol is whatever the alias stands for — an input file path, an output file path, a type symbol (`juce::Identifier`), a single-line fragment (backtick literal, may contain placeholders). Every input file — every template, every fragment, every table file, the manifest itself — is declared in the manifest's index exactly once; CAST parses exactly what it declares: no directory scanning, no glob. A `format` cell names a Transform Vocabulary op applied to the symbol at resolution. Referencing an undeclared alias is FATAL. Interior border lines (grid tables) and dash-only rows group the index visually without declaring anything.
*   `## output` — first-order generation, one row per generated construct: `| placeholder | structure | separator | file |`. These four column names are the **only** names the engine reserves. `placeholder` and `structure` cells are multi-line — continuation lines carry an empty key per the row law.

    #### Placeholder Cell Grammar — what data

    The placeholder cell **wires** the row's expansions: each bullet `- name: value` declares which data feeds the placeholder `:::name:::`. The value's own form states the wiring — three forms, decided by construction, never by engine special-cases:

    *   **Table reference** (`- line: @lexicon:lexicon`) — the placeholder is a **vertical expansion**: the referenced table's rows are iterated, each row built through the structure's same-named shape binding, the results joined by the separator's same-named entry (default: newline).
    *   **Column name** (`- files: file`, second-order only) — the value names a column of the source table (`## output`). The placeholder expands over the column's **unique values** in authored row order, the emitting row's own file excluded.
    *   **Binding name** (`- line: instance`, second-order only) — the value matches no column; it names a structure **binding**. The placeholder expands over exactly the rows whose structure cell carries that binding — presence is the selection, pure data, no engine filter.

    A `> ` prefix scopes a wiring bullet to its depth — **depth is the raw `> ` count**, identically in all three keyed columns: no `> ` = depth 0, `> ` = depth 1, `> > ` = depth 2. `> - line: @screen:screen` feeds the depth-1 `:::line:::`. Wiring, shape binding, and separator entry for one expansion always sit at the same raw depth.

    #### Structure Cell Grammar — what shape

    The structure cell is valid CommonMark + pandoc block markdown. Two constructs, nothing else:

    *   **Bullets = the row's bindings.** `- key: value` — each bullet binds one placeholder of the row's templates: `- keyword: static constexpr` fills `:::keyword:::`; `- line: @definition` names the shape a vertical expansion builds each row through. **Within a depth, ownership is positional:** bullets *before* the depth's `@scope:` head bind the content shape built inside that scope; bullets *after* the head bind the scope itself — which is how one depth carries both `- keyword: static constexpr` (the definition's) and `- keyword: namespace` (the scope's) without collision. Values follow the sigil law — `@`-sigiled resolves through `## index` (a `.cast` path expands as a template/fragment; `@alias:table:entry` resolves to that table row's value), bare is literal. A value may contain placeholders (`- terminator: // namespace :::name:::`) resolved against the same scope's bindings — SSOT for names. An explicit-empty binding (`- prologue:`) renders nothing. **A `default` binding is never declared — the default is always the source table's first row** (First-row law, above).
    *   **`> ` = one tab; depth is the raw `> ` count.** No `> ` = depth 0, no indent (the top-level scope); `> ` = depth 1, one tab; `> > ` = depth 2, two tabs. `@scope: Name` at a depth wraps the content assembled at that depth and indents it per the depth's tab count. A bullet belongs to exactly its own raw depth — with or without a `@scope:` head at that depth; there is no fold into the parent. Blank line separates each depth group; every line inside a quote carries its `> ` prefix; bullets never nest — ownership is quote depth, never indentation. Depth marking is **uniform across the placeholder, structure, and separator columns** — the same `> ` count addresses the same depth in all three.

    Rows sharing an output `file` must declare byte-identical outermost wraps — mismatch is FATAL, never first-row-wins.

    #### Separator Cell Grammar — what join

    The separator cell names the join text per expansion, keyed identically:

    *   `- line: @text:break:line` — vertical: rows joined by newline + resolved text + newline (the `//===` section rule between siblings).
    *   `- list: @text:break:comma` — horizontal: the row's column values joined inline by the resolved text.
    *   Empty, or an expansion with no same-named entry — joined by a plain newline (vertical default).
    *   A flat `@ref` — the resolved text joins **consecutive output rows sharing this row's file** (the row-sibling join — same mechanism, applied by the writer).

    Separator texts are ordinary table data — the `## break` table in `text.md` declares the canon: `line` = `//==============================================================================`, `comma` = `, `, `semicolon` = `; `.

    ```markdown
    +--------------------+---------------------------------------------+-------------------+----------------+
    | placeholder        | structure                                   | separator         | file           |
    +====================+=============================================+===================+================+
    | > - line: @xml:    | - keyword: static constexpr                 | @text:break:line  | @jam_Operators |
    |   xmlOperators     | - type: @cString                            |                   |                |
    |                    | - open: {                                   |                   |                |
    |                    | - close: }                                  |                   |                |
    |                    |                                             |                   |                |
    |                    | @scope: Id                                  |                   |                |
    |                    |                                             |                   |                |
    |                    | - keyword: namespace                        |                   |                |
    |                    | - terminator: // namespace :::name:::       |                   |                |
    |                    |                                             |                   |                |
    |                    | > @scope: XmlOperators                      |                   |                |
    |                    | >                                           |                   |                |
    |                    | > - keyword: struct                         |                   |                |
    |                    | > - terminator: ;                           |                   |                |
    |                    | > - line: @definition                       |                   |                |
    +--------------------+---------------------------------------------+-------------------+----------------+
    ```

    Reading the row: depth 0 declares the outermost scope (`namespace Id`); depth 1 declares the struct, whose content sits at one tab; the wiring `> - line: @xml:xmlOperators` feeds the depth-1 `:::line:::` through the `@definition` shape. **Binding ownership within a depth is positional:** bullets *before* the `@scope:` head bind the content shape built inside it (`static constexpr`, `@cString`, `{`, `}` — the definition's placeholders); bullets *after* the head bind the scope itself (`namespace`, its terminator). The separator column joins this row with its file-siblings by the `//===` rule. Six operator structs = six rows sharing `@jam_Operators`, sharing one byte-identical depth-0 namespace. Nesting depth never adds columns — it adds `> ` markers.
*   `## output index` — optional, **second-order**: an output-shaped table whose expansion source is `## output` itself. Not every language needs a master index — `## output` is mandatory, `## output index` is optional: the output index of the outputs. Its placeholder wiring uses the second-order forms: a **column name** expands unique column values (`- files: file` — the `file` column of `## output` IS the include list, no separate headers table exists); a **binding name** expands the rows carrying that binding (`- line: instance` — every map row already declares `- instance: shared`, and that presence is the selection). The master include is emitted after the outputs, beside them, from them. It never folds into `## output`: a `struct` wrapper is not a `namespace` wrapper — same bones, different flesh, different table.

**Matched-replace is the only fill rule.** A placeholder fills by one uniform sequence — template token → wiring declaration (placeholder column) → binding (structure column) → matched cell (source row) → direct `## index` lookup → replace. The data row's own cell is consulted first, then the declaring row; a `@`-sigiled result resolves through the index; the resolved text replaces the placeholder. No fallbacks, no positional indices, no engine defaults — a placeholder with no match is FATAL (dead placeholder).

**Wraps fill the same way.** A wrap template is built through matched-replace like any shape; a placeholder in it whose name matches one of the row's expansion keys fills with that expansion's assembled content. Per wrap application, the intersection of the wrap template's placeholder set with the row's expansion keys must be **exactly one** — zero or multiple is FATAL. That intersection is how `struct Generated`'s content reaches `:::line:::` at every depth without the engine knowing the name `line`.

**Parity is enforced.** Per output row, every placeholder of its resolved templates must fill through matched-replace, and every wiring declaration and binding must be consumed by some placeholder. Expansion interiors validate against their source table's columns. No match, no cigar.

> **FATAL MANIFEST ERRORS:** CAST will immediately crash if it detects:
> *   An **undeclared alias** (a `@`-sigiled cell absent from the file's `## index`).
> *   A **duplicate alias** (two `## index` rows declaring the same alias).
> *   A **missing file** (an `## index` path that does not exist).
> *   An **unresolvable table** (a `@alias:table` cell whose heading is absent from the aliased file).
> *   A **dead wiring** (a placeholder-column or structure-cell declaration no template placeholder consumes).
> *   A **dead placeholder** (a template placeholder no wiring, binding, or source-table column feeds).
> *   A **wrap mismatch** (rows sharing a file with non-identical outermost wraps).

---

## Canon Files

Every CAST-driven project declares its generation inputs in the canon files:

| File | Role |
|------|------|
| `CAST.md` | Codegen Annotated Source of Truth — the manifest; also home to the identity and paths tables |
| `lexicon.md` | Every entity declared once: `\| name \| value \|` (registry) |
| `chars.md` | Single characters — framework-owned, generates the character constants (registry) |
| `files.md` | Filenames with extensions — generates the file constants (registry; folds the former `extensions.md`) |
| `colours.md` | Colour entities (registry) |
| `text.md` / `localisation-lang.md` | The long-text home — generates the text constants; also carries `## break` (separator texts) and `## namespace` (scope banner literals) |
| `xml.md`, `html.md`, `css.md`, `markdown.md`, `terminal.md`, `syntax.md`, `mermaid.md`, `gui.md`, `plugin.md`, `graphics.md` | Domain tables, one file per domain — every operator, bimap, lookup table, and hashmap belonging to that domain |

The `## namespace` table declares the paired scope-boundary banners referenced as `@text:namespace:prologue` / `@text:namespace:epilogue`:

```markdown
## namespace

+----------+-------------------------------------------------------------------------------------+
| name     | value                                                                               |
+==========+=====================================================================================+
| prologue | `/*_____________________________________________________________________________*/` |
| epilogue | `/**______________________________END OF NAMESPACE______________________________*/` |
+----------+-------------------------------------------------------------------------------------+
```

The reference registry is the union of the declaration tables — `## lexicon`, `## chars`, `## files`, `## colours`, and each language's own text table. A relation cell may reference an entity from any of them; the word is declared exactly once, in exactly one table.

**Declaration type follows dominant consumption, and type is data.** An entity consumed as an Identifier (tree keys, property keys, table lookups) declares an Identifier-type token; an entity consumed as a string (delimiters, map keys, emitted text, cell comparisons) declares a String-type token. The mechanism is the lexicon `type` column — explicit on every row, holding a `@`-sigiled alias (`@id`, `@string`) resolved through the lexicon file's own `## index` to the target language's type symbol (`juce::Identifier`, `juce::String`). CAST is language-agnostic: the engine never knows any type name — it delivers the resolved symbol to the `type` placeholder like any other cell, and the project-owned template composes the declaration around it. No engine default exists — a default would smuggle one language's type into the engine. A `.toString()` projection at nearly every call site is the violation signature — the declared type is wrong, not the call sites.

All manifest and table files compile into **one master state document**: every `##`-headed table, from every file, becomes a sibling in a single tree, and generation reads only that tree.

Every table is addressed by its file: a reference is `@alias:heading` (`@lexicon:lexicon`, `@template:template token type`, `@CAST:output`), where the alias resolves through `## index` and the heading inside that file. Two tables with the same heading inside one file are FATAL (reported as already-declared, with both locations); the same heading in two different files is two different tables — the qualifier is the namespace.

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
2. Each template and fragment file parses once into a **grammar tree** — the parsed shape of the template language (text, placeholders), shared and immutable.
3. Generation iterates the **Model**: each output row **builds** its own output document — a state tree constructed from the grammar tree against the Model: placeholders resolved through matched-replace, expansions resolved (rows built per source row, joined by `joinIntoString (separator)`). Resolution happens at build, into the tree — never at emission.
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
```cpp
// Definition.cast shaped by: - keyword: inline const / - type: juce::Identifier
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
inline const juce::String buttonOk { "OK" };
inline const juce::String cliPrefix { "--" };
```

### 3. Operator struct — one row, full wiring

```markdown
## xmlOperators
| name            | value   |
| --------------- | ------- |
| declarationOpen | `"<!"` |
| tagClose        | `">"`   |
```
```markdown
## output (the full row)

placeholder            structure                                  separator          file
> - line: @xml:        - keyword: static constexpr                @text:break:line   @jam_Operators
  xmlOperators         - type: @cString
                       - open: {
                       - close: }

                       @scope: Id

                       - keyword: namespace
                       - terminator: // namespace :::name:::

                       > @scope: XmlOperators
                       >
                       > - keyword: struct
                       > - terminator: ;
                       > - line: @definition
```
```cpp
// generated
namespace Id
{
struct XmlOperators
{
    static constexpr const char* const declarationOpen { "<!" };
    static constexpr const char* const tagClose { ">" };
};
}// namespace Id
```
One row per struct: six rows share `@jam_Operators`, joined by the flat `separator` text, each with its own depth-1 struct scope and one byte-identical outermost namespace scope. Nesting depth is `> ` count, never a column.

### 4. HashMap — Definition entries inside a Scope, horizontal `list`

```markdown
## output row (structure + separator interplay)

placeholder                   structure                       separator
> - line: @entities:Entities  @scope: `:::list:toCamel:::`    @text:break:line
                              - keyword: inline const
                              - type: @hashMapStrStr
                              - terminator: ;
                              > - line: @definition           > - list: @text:break:comma
                              > - open: {
                              > - close: }
                              > - terminator: ,
```
```cpp
inline const jam::HashMap<juce::String, juce::String> diacritics
{
    { juce::String::fromUTF8 ("\xc3\x80"), juce::String::fromUTF8 ("\x41") },
    ...
};
```
The entry is Definition.cast with keyword/type/name elided; `:::list:::` is the row's key and value columns joined by the `- list:` separator (`, `). Sibling hashmaps share the file, joined by the `//===` rule. Type aliases (`@hashMapStrStr`) are backtick-wrapped in `## index` to clear the `<`/`>` hazard.

### 5. Bimap — two expansions of one source, names are data

A bimap struct needs the same source table twice: once as map-init entries, once as enum entries. Two placeholders, wired independently:

```markdown
placeholder                   structure
> - entry: @terminal:screen   > - entry: @mapEntry     (fragment: { :::key:::, ... },)
> - value: @terminal:screen   > - value: @definition   (with - open: = and - terminator: ,)
```
```cpp
struct Screen : public jam::Bimap<Screen, juce::String, int>
{
    Screen()
    {
        map = {
            { normal, juce::String::fromUTF8 ("normal") },
        };
    }

    enum value : int
    {
        normal = 0,
    };

    const juce::String& getDefault() const noexcept override
    {
        return map.at (0);
    }
};
```
**Ordinals are arbitrary data** — the `value` column declares every ordinal explicitly; reordering rows never changes a value. `getDefault()` derives from the **first row** (First-row law) — never from a declared binding. The fixed method block is a fragment (`.cast` file), alias-routed. Every map row also carries `- instance: shared` — its fill for `:::instance:::`, and its selection marker for the second order.

### 6. Second order — the master include, from the outputs themselves

```markdown
## output index

+-------------------+---------------------------------------+-----------+----------------+
| placeholder        | structure                             | separator | file           |
+====================+=======================================+===========+================+
| - files: file      | @scope: jam                           |           | @jam_Generated |
| > - line: instance |                                       |           |                |
|                   | - keyword: namespace                  |           |                |
|                   | - files: @include                     |           |                |
|                   | - prologue: @text:namespace:prologue  |           |                |
|                   | - epilogue: @text:namespace:epilogue  |           |                |
|                   | - terminator: // namespace :::name::: |           |                |
|                   |                                       |           |                |
|                   | > @scope: Generated                   |           |                |
|                   | >                                     |           |                |
|                   | > - keyword: struct                   |           |                |
|                   | > - line: @instance                   |           |                |
|                   | > - terminator: ;                     |           |                |
+-------------------+---------------------------------------+-----------+----------------+
```
`- files: file` names a **column** of `## output` → unique file values, authored order, self excluded → one `#include` per file via `@include`. `- line: instance` names a **binding** → exactly the rows declaring `- instance:` emit a member via `@instance` (`SharedInstance<map:::::name:::> :::name:toCamel::: { std::in_place };`). No column is added anywhere — presence of the binding is the selection.

### 7. Spec key table — component words + `format` override

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
// generated — the from-transform reads the key source (format-else-key), the to-transform the value cell
            { juce::String::fromUTF8 ("aacute"), ... },
            { juce::String::fromUTF8 ("Aacute"), ... },
            { juce::String::fromUTF8 ("measuredangle"), ... },
            { juce::String::fromUTF8 ("LongLeftArrow"), ... },
            { juce::String::fromUTF8 ("DifferentialD"), ... },
```
`key` carries the wire token split into its component words, each word keeping the spec's exact casing — case is the differentiator for a spec pair (`aacute` / `Aacute`), never a baked-in transform. `format` resolves the wire form: empty leaves `key` verbatim for a single-word token; `join` removes the spaces for a multi-word token, reassembling it byte-exact because every word already carries its true casing; a literal wire token (`DifferentialD`) overrides both when the readable `key` deliberately diverges from mechanical reconstruction.

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
5.  `toUTF8` — Converts `U+XXXX` codepoint tokens into UTF-8 `\xNN` byte escape sequences.
6.  `toHex` — Converts a glyph or `0xNN` cell into a minimal lowercase `0xNN` integer literal.
7.  `toCodepoint` — Converts a glyph or `0xNN` cell into zero-padded `U+XXXX` codepoint notation.
8.  `toSymbol` — Qualifies a two-part symbol with its owning namespace (e.g., `A::b` becomes `juce::A::b`). Leaves three-or-more part symbols verbatim.
9.  `toPascal` — Uppercases the first letter of each space-separated word and joins them, abbreviations passing through whole: `xml operators` becomes `XmlOperators`; `UI scale` becomes `UIScale`; `fail hazard URI` becomes `FailHazardURI`. Single-word input follows the same rule — a non-abbreviation word capitalizes its first letter, an abbreviation is already correct.
10. `toCamel` — Joins space-separated words into camelCase: a non-abbreviation first word lowercases in full; an abbreviation first word passes through intact — an abbreviation never lowercases, in any position. Every remaining word Pascal-joins per the `toPascal` rule above. `hex prefix` becomes `hexPrefix`; `UI scale` becomes `UIScale`; `fail hazard URI` becomes `failHazardURI`. Single non-abbreviation words lowercase in full: `Generated` becomes `generated`.
11. `toSnake` — Underscore-joins space-separated words: non-abbreviation words lowercase; abbreviations pass through intact: `clamp to border` becomes `clamp_to_border`; `scale X` becomes `scale_X`. A wire format that demands a lowercased abbreviation authors the literal value instead of projecting.
12. `toScreamingSnake` — Uppercases space-separated words and underscore-joins them: `clamp to border` becomes `CLAMP_TO_BORDER`.
13. `join` — Removes the spaces between words, case untouched: `Long left arrow` becomes `Longleftarrow`. Used by spec key tables (Cookbook, above), where each word already carries its own exact casing.

**Reference Derivation.** References never take a format op. A cell that references an entity declared elsewhere (lexicon, chars, files, extensions, a declared enum family) is written as a byte-exact literal, and emission derives the entity's **own generated symbol** from its declaration — `colon` emits `Chars::colon`, `begin` emits `Id::begin`, `region open` under the Byte family emits `map::Byte::regionOpen`. One declaration, one symbol, every reference emits it — that derivation is why the canon registries are globally unique. Format ops exist only where a declaration table emits its own tokens; a transform tag applied to a reference is a conflation error (FATAL).

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
*   A template placeholder with no matching source reports where it happened, which output row was being built, and everything that *was* available:
    > ```
    > template/Definition.cast:1 (:::key:::): placeholder has no source: :::key:::
    >   output row: Screen.h (source: terminal:screen)
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
