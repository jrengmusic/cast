# CAST: Codegen Annotated Source of Truth
**Version 0.0.1**

Welcome to the `cast` help page. If you are seeing this, you either ran `cast --help`, or `cast` could not find a `CAST.md` file in your current directory. 

CAST is a strictly deterministic code generator. It has no domain knowledge, no interpreter, and no conditional logic inside its templates. It simply takes your data (Tables), your formatting (Templates), and your instructions (Manifest) to generate code.

---

## Quick Start: Command Line Interface

CAST is controlled entirely via the command line and the `CAST.md` manifest file. It never accepts generation rules via arguments—arguments are only for *selecting* what to run.

*   `cast` — Finds `CAST.md` in the current directory and regenerates **all** declared outputs.
*   `cast <path>/CAST.md` — Regenerates all outputs using a specific manifest file.
*   `cast CAST.md <output>` — Regenerates **only one** specific output file declared in the manifest.
*   `cast --version` — Prints the version and source commit (this same stamp is embedded in generated file banners).
*   `cast --help` — Prints this guide and exits successfully.
*   *No `CAST.md` found* — Prints this guide and exits with an error code.

---

## The 3 Artifact Types

CAST operates on exactly three types of files. Generated output files are considered untracked build artifacts and should not be edited by hand.

### 1. Relations (Input Tables)
Your data lives in standard Markdown files as GitHub Flavored Markdown (GFM) tables.
*   **Naming:** Any `## Heading` immediately followed by a GFM table creates a "Relation" named after that heading.
*   **Keys:** The **first column (Column 0)** is the row key. Looking up a row by value always checks this column.

#### Column 0: `key` vs `entry`

The column-0 **header** declares what type of table this is. Pick exactly one:

| Header | Meaning | Constraint |
|---|---|---|
| `key` | **Declares** a new identifier. This row is the identifier's single point of origin. | `unique` — the value may not be declared anywhere else, in any table. |
| `entry` | **References** an identifier declared elsewhere (in some `key` table). | Not subject to `unique` — references repeat freely. |

Rules of thumb:
*   Vocabulary tables (identifiers, chars, strings, operators) **declare** → `key`.
*   Relational tables (bimaps, classification maps, lookup tables, anything whose column 0 names things that already exist) **reference** → `entry`.
*   The fragment template may address column 0 by its authored header — a `key` table fills `@key@`, an `entry` table fills `@entry@` — or by the canonical `@entry@`, which fills with column 0 of *any* table whatever that column's header is. `@entry@` is what lets one fragment serve `key`-headed and `entry`-headed tables alike; the header still decides declaration versus reference, and lexicon resolution still applies only to `entry`/`name` headers.

Getting this wrong is loud. Declaring `closeParen` in an `entry`-style table under a `key` header fails like this:

> `tables/cssCodePoints.md: duplicate "closeParen" already declared at tables/cast.md`
*   **Cells:** All cells become plain strings. Text inside backticks (`` ` ``) unwraps to its plain content. Numbers are kept exactly as you typed them—CAST will never reformat or reparse numbers.
*   **Order:** Rows are always output in the exact order you authored them. CAST never sorts rows.

**Lingua law.** Column 0's header states its own semantics — three lanes, never mixed: `name` **declares** (registries only: `lexicon`, `chars`, `files`, `colours`); `entry` **refers** (lexicon-resolved, registry-validated, FATAL on a miss); `key` **indexes** (raw lookup data local to its own table, never resolved against the lexicon, emitted as-is).

#### Cell Resolution
Before a declared transform (from `## transforms`) is applied, each cell value is resolved against the row key:
*   An **empty cell** resolves to the row key (column-0 value).
*   A cell of exactly `@row@` resolves to the row's 0-based index.
*   A cell whose text **names a transform** (e.g. `toUpper`) resolves to that transform applied to the row key.
*   Any **other cell** is its own literal value.
*   A cell **wrapped in backticks** is a literal and is never resolved — no transform lookup, no empty-cell resolution.
*   The **first column (the row key itself)** is never resolved.
The resolved value is then passed through any declared column transform.

> **FATAL HAZARD RULE:** If a cell contains `<`, `>`, or a URI scheme (like `http://`) and it is *not* wrapped in backticks, CAST will crash. Always wrap special characters in backticks.

#### Lexicon Reference Resolution

When `lexicon.md` is present in the tables directory and a patched source table's column-0 header is `entry`, each entry cell is resolved as a reference into the lexicon registry:

*   The entry cell is looked up **byte-exactly** in the lexicon's `name` column — a reference must match its declaration's casing verbatim. A reference differing only in case is an undeclared entity (FATAL).
*   On hit: `@entry@` fills with the **canonical name** (as declared in the lexicon, spaces and casing preserved). `@value@` fills with the referenced entity's value, passed through standard cell resolution and any declared column transform.
*   On miss: **FATAL** — the entity is not declared.
*   When no `lexicon.md` is present, `entry`-headed tables expand without reference resolution (backward compatible).

### 2. Templates (Formatting)
Templates are plain text files containing exactly one special construct: `@placeholder@`. There are no loops, no `if` statements, and no expressions.
*   **Scalar Placeholder:** Replaced by a single cell value or a single manifest value.
*   **Aggregate Placeholder:** Replaced by an entire table, projected through fragment templates, in the order you authored the rows. Consecutive fragments are joined by the output's separator template (see `## generated`), or by nothing when that cell is empty.
*   **Root Templates:** Used to generate exactly one output file.
*   **Fragment Templates:** Used for per-row expansions inside Root Templates. Fragments never generate files themselves.
*   **Logic:** Any actual code logic (function bodies, framework calls) lives as plain text in the template. Templates do not execute logic.

#### Row-Region Templates
A fragment template containing `@row:begin@` and `@row:end@` is a *row-region template*. It expands once for its whole matched table rather than once per row:
*   The entire line containing `@row:begin@` and the entire line containing `@row:end@` are consumed (not emitted).
*   Text between those two marker lines repeats once per matched row, filled from that row's cells.
*   `@row:index@` — inside a row region, fills with the zero-based position of the row within the matched row set. It is not a table-level placeholder and is not available outside a row region.
*   Text outside the region sees *table-level placeholders* (see below) rather than per-row placeholders.
*   A fragment may contain **several regions**; each is expanded in turn against the same matched table.

**Column regions.** A region in a fragment may be named after a column instead of after `row`: `@win:begin@` … `@win:end@`. Such a region emits only the rows whose cell in that column is non-empty — selection is emergent from the data, never declared. Inside it, jacks are column names, exactly as everywhere else. A region whose rows are all empty emits nothing at all — which is how a platform section with no data yet still compiles. Three column regions over `mac`, `win` and `linux`, wrapped in the language's own conditional-compilation text, are how one fragment serves every platform without a line of engine logic. Naming a column that the matched table does not have is FATAL, and `@row:begin@` takes no column — writing `@row:begin:win@` is FATAL and tells you to use the column's own name.

**Table-level placeholders** (valid outside a row region in a row-region template):
*   `@table@` — the table's name (the heading text).
*   `@table:transformName@` — the table's name run through a transform. `@table:toPascal@` turns a `## xmlOperators` heading into `XmlOperators` — the standard way to derive a struct name from a table name.
*   `@brief@` — the prose paragraph authored between the table's `##` heading and the table itself; empty string if none.
*   `@columnName@` — for any column header, the column-0 key of the *first* row whose cell in that column is non-empty.
*   `@default@` / `@default:transformName@` — fills from **row 0's `value` cell**, verbatim or run through a transform; when the table has no `value` column, or row 0's `value` cell is empty, it fills with `0`. One rule for every map shape: a `Bimap` whose values are authored literals gets that literal; an ordinal `Bimap` whose values are `@row:index@` (no stored `value` column) and a `LookupTable`'s fill expression both get `0` the same way. There is no `default` marker column — row position replaces it.

#### Placeholder Regions
A placeholder in a root template may be written as a region — `@identifier:begin@ … @identifier:end@` — instead of the bare `@identifier@`. The text between the two marker lines is the body that a fragment file would otherwise hold, and it expands exactly as that fragment would: once per row, with every per-row placeholder available. A patch cable leaves its `fragment` cell **empty** to say "my body is the region in the root".

This is the answer to the one-line fragment. A file whose entire content is a single line of output is a file for the sake of a file: the manifest already says which table feeds which placeholder, so the line belongs where it is emitted. Objects with real structure — a struct with a map, an enum and methods — still earn a fragment file, because their body is the thing being maintained.

Both sources feed one placeholder: a placeholder may be filled by region rows and fragment-file rows at the same time, and their output concatenates in manifest order at the region's location.

**The region's name is read by the file's role.** The manifest already declares which files are roots (`## generated`) and which are fragments (`## patch`), so there is nothing to guess: in a **root**, a region name is a **placeholder**; in a **fragment**, a region name is a **column**, and `@row@` means every row. One spelling, one meaning per role.

**Enforcement.** All five are FATAL:
*   A region in a root names something that is not a declared placeholder.
*   A region in a fragment names something that is not a column of the matched table.
*   A patch cable has neither source — its `fragment` cell is empty and no root declares a region for its placeholder — or a placeholder's region exists but no patch cable ever feeds it.
*   `row` is used as a placeholder name. It is reserved for the all-rows region.
*   A placeholder appears in more than one root, or in none.

#### Chain Fragments
A `fragment` cell may hold two space-separated paths instead of one — a **chain**. The first path is the **base** fragment; the second is a **wrapper**. Depth caps at one base plus one wrapper — a chain never nests further.

For each `@row:begin@ … @row:end@` block in the base fragment, the engine expands that block against the matched table exactly as it always would, then substitutes the wrapper's own `@row@` hole with that expanded body. The wrapper supplies everything around the row body — struct shell, enum keyword, namespace braces — and the base supplies the row-by-row content; neither is rewritten to know about the other.

A wrapper may itself contain column-filtered regions (`@mac:begin@ … @mac:end@`), which filter and fill exactly as they would in any fragment (Column regions, above) — including the `@value@` → `@cell@` fallback when the matched table has no `value` column.

### 3. Manifest (`CAST.md`)
The manifest is the "brain" of the operation. It contains four specific GFM tables that tell CAST how to combine Relations and Templates:

*   `## generated` — Maps a **Root Template** → **Output File Path** → **Separator Template** (columns: `template | output | separator`). Relations are always the `.md` files directly inside the `tables` directory beside the manifest. The `separator` cell names a template whose expanded text is placed between consecutive fragments of every placeholder in that output; leave it empty for no separator. Because the separator is a template rather than an engine constant, each language supplies its own — a C++ manifest points at a rule comment, another language at whatever its convention is.
*   `## patch` — the **cord list** of the patch bay: `source | fragment | placeholder`. One cord, one row — every cord states all three cells; no cell inherits from the row above. Junctions are named jacks: a table column is an output jack, a template or fragment placeholder is an input jack, and the same name is the same circuit — no mapping syntax exists.
    *   **Source** is always a **bare table name**, resolved in the tables namespace (the spliced `tables/*.md` domain tables). Nothing else is legal in the cell — no dot-address, no bracket, no filter of any kind. A source row expands only when every jack its fragment references carries a signal (non-empty cell); an empty referenced cell drops the row (emergent selection). Extending a circuit is one column on the canon table plus one cord — never a new table, never a duplicated row.
    *   **Fragment** names a fragment template file, relative to the manifest. An empty `fragment` cell is region-fed — the cable's body lives in the root template's own placeholder region (Placeholder Regions, above), not in a file. A `fragment` cell may instead hold a chain — two space-separated paths (Chain Fragments, above).
    *   **Placeholder** names the aggregate region in a root template that collects this cable's expanded output.
    *   **Derivation is placeholder-driven — there is no `type` column and no type strings drive the engine.** A fragment's own placeholders declare what CAST derives from the matched table: `@type@` derives the table's value type from its `value` column's shape (`int` when there is no `value` column or its cells are not all numeric; `uint32_t` when every cell is numeric; a handful of recognized shapes — `juce::juce_wchar`, `const char*`, or an authored `std::array` type — for their exact patterns). `@from@`/`@to@` derive expression transforms from the table's own shape: `@from@` reads `fromId` when column 0 is headed `entry`, `fromLiteral` otherwise; `@to@` reads the `value` column's cell shape — `fromCodepoint` for a `U+XXXX` cell, `fromMap` for a `Name::member` cell. `@from:type@`/`@to:type@` publish each transform's implied C++ type. `@capacity@` derives the matched table's last row's key plus one — the emitted `LookupTable`'s entry count. `@default@` derives from row 0 (the row-0 `@default@` rule, above). `@symbol@` derives the table name, camelCased. None of these are authored on the cable; each appears in the generated output only because the fragment names it.
*   `## transforms` — Maps a **Column** to a specific text transformation (see Transform Vocabulary below).
*   `## constraints` — Maps a **Column** to a validation rule (see Predicate Vocabulary below), such as Foreign Keys.

**The master include header is manifest-derived.** Every scope's master (`(prefix)Generated.h`) is generated, never authored: no project and no module defines a Generated.h template — the cookie cutter is engine property, embedded in the engine's binary data. Its content derives entirely from the manifest: module includes and struct members from the canon table's module cells first, then one sibling include per `## generated` output row. The master itself is not a `## generated` row — it is emitted after them, beside them, from them.

> **FATAL MANIFEST ERRORS:** CAST will immediately crash if it detects:
> *   An **Orphan template** (a template file not referenced by the manifest).
> *   An **Undeclared output** (an output file generated outside of `## generated`).
> *   A **Missing separator template** (a `## generated` row naming a separator file that does not exist).
> *   An **Unmapped fragment** (a fragment missing from the patch table, or a cable naming a fragment file that does not exist). A patch cable with an **empty** `fragment` cell names no file by design — it is fed by its placeholder's region in the root (see Placeholder Regions).

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

**Declaration type follows dominant consumption.** An entity consumed as an Identifier (tree keys, property keys, table lookups) emits `juce::Identifier`; an entity consumed as a string (delimiters, map keys, emitted text, cell comparisons) emits `juce::String`. The mechanism is the `## string` selection table: membership there routes the entity through the string emission region; everything else emits Identifier. A `.toString()` projection at nearly every call site is the violation signature — the declaration type is wrong, not the call sites.

All manifest and table files compile into **one master state document**: every `##`-headed table, from every file, becomes a sibling in a single tree, and generation reads only that tree.

Table names live in two namespaces resolved by provenance: manifest sections (`## generated`, `## patch`, `## constraints`, `## transforms`, identity tables) and spliced `tables/*.md` domain tables. A patch cable's source resolves in the tables namespace; manifest reads resolve in the manifest namespace. Two tables with the same name inside one namespace are FATAL (reported as already-declared, with both locations).

**Entity rules:**
*   An entity is unique, whole, and opaque. `UI`, `scale`, and `UI scale` are three independent declarations — CAST never decomposes or derives one from another.
*   Uniqueness is global and case-insensitive. First declaration wins and fixes the casing.
*   Word boundaries (spaces) and per-word casing are stored data — the declaration is the single source of every projected form. The declared casing is exactly what `@entry@` emits.
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

---

## Cookbook: Table + Template → Code

Every generated construct is plain text substitution — nothing more. These examples show each pairing end to end. In all of them the patch cable's `source` names the table (always a bare table name), and the `placeholder` cell names the root template's aggregate region that collects the expanded fragments.

### 1. Identifier constants — `key` table + scalar fragment

```markdown
## core
| key        | string     |
| ---------- | ---------- |
| id         |            |
| blockQuote | blockquote |
| dataWidth  | toKebab    |
```
```cpp
// template/Identifier.h
inline const juce::Identifier @key@ { "@string@" };
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
// template/String.h
inline const juce::String @key@ { "@string@" };
```
```cpp
inline const juce::String buttonOk { "OK" };
inline const juce::String cliPrefix { "--" };
```

### 3. Operator struct — row region + `@table:toPascal@`

```markdown
## xmlOperators
| key             | string  |
| --------------- | ------- |
| declarationOpen | `<?xml` |
| tagClose        | `>`     |
```
```cpp
// template/Operator.h — @row:begin@/@row:end@ lines are consumed; the body repeats per row
namespace Id
{
struct @table:toPascal@
{
@row:begin@
    static constexpr const char* const @key@ { "@string@" };
@row:end@
};
}
```
```cpp
// generated — the table NAME becomes the struct name, PascalCased
namespace Id
{
struct XmlOperators
{
    static constexpr const char* const declarationOpen { "<?xml" };
    static constexpr const char* const tagClose { ">" };
};
}
```
One template, many structs: give each struct its own cable — `source: xmlOperators`, `source: cssOperators` — each with its own placeholder, both through the same `template/Operator.h`.

### 4. Bimap — `entry` table + table-level placeholders

```markdown
## Screen
| entry | value | format |
| ----- | ----- | ------ |
| main  | 0     |        |
| alt   | 1     |        |
```
```cpp
// template/Bimap.h (excerpt) — @default@ fills from row 0's `value` cell (the row-0 @default@ rule)
struct @table@ : public jam::Bimap<@table@, juce::String, @type@>
{
    enum value : @type@
    {
@row:begin@
        @entry@ = @value@,
@row:end@
    };

    const juce::String& getDefault() const noexcept override { return hashmap.at (@default@); }
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

    const juce::String& getDefault() const noexcept override { return hashmap.at (0); }
};
```
Column 0 is `entry` because `main` and `alt` are declared once in a `key` table elsewhere — the bimap only references them.

**Ordinal derivation:** an enum's ordinals derive from its relation row's authored position — never a stored column. The lexicon and relation tables carry no ordinal data at all; `@row:index@` (SPEC §3.2, zero-based, scoped to the matched row set of one patch cable) supplies the position directly, so a `value` column like example 4's above becomes unnecessary:

```markdown
## Screen
| entry |
| ----- |
| main  |
| alt   |
```
```cpp
// template/Bimap.h (excerpt) — @row:index@ replaces a stored `value` column
        @entry@ = @row:index@,
```
```cpp
        main = 0,
        alt = 1,
```
Before/after: dropping the stored `value` column and swapping `@value@` for `@row:index@` in the fragment produces byte-identical output — the ordinals were always just row position. `@default@` is unaffected by the swap: with no `value` column left, it was already `0` (the row-0 `@default@` rule's no-column case) before and after.

**One fragment, every shape.** Nothing above is specific to `Screen`. A table whose keys are declared elsewhere and a table whose keys are its own declarations both fill `@entry@`; tables of different value shapes both fill `@type@`, derived from the matched table itself; a key that is a symbol rather than a number is just another `value` cell. Where two tables of the same type differ, the difference belongs in the table or in a placeholder — never in a second copy of the template. A fragment legitimately forks only when it differs in *structure*: platform-conditional output, for instance, is one fragment whose body is three filtered row regions over `mac`/`win`/`linux` columns.

`@row:index@` is zero-based **per patch cable**, not per table: two cables can each address the same table through a different column via a dot-address — `platforms.mac` through one cable, `platforms.win` through another — and each matched row set gets its own independent 0-based sequence (`expandRowRegions()` resets `rowIndex` to 0 on every call, and `buildSlotResults()` calls it once per patch cable). Two column-addressed Bimaps each start at `0` — the intended shape for per-platform enums.

### 5. Relational HashMap — auto-derived `from`/`to`

```markdown
## atRuleType
| entry         | value                    |
| ------------- | ------------------------ |
| atRuleMedia   | CssRuleType::mediaRule   |
| atRuleCharset | CssRuleType::charsetRule |
```
```markdown
## patch (excerpt)
| source     | fragment           | placeholder    |
| ---------- | ------------------- | ------- |
| atRuleType | template/HashMap.h | hashmap |
```
```cpp
// template/HashMap.h (row body) — the one fragment for every map shape;
// @from@/@to@ are derived from the matched table: column 0 is `entry` -> fromId,
// value cells are `Name::member` -> fromMap
            { @from@, @to@ },
```
```cpp
// generated — jam::HashMap<juce::String, int>, both spellings and both types derived from the table
            { Id::atRuleMedia.toString(), map::CssRuleType::mediaRule },
            { Id::atRuleCharset.toString(), map::CssRuleType::charsetRule },
```

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
// patch cable: source Entity, fragment template/HashMap.h — auto-derived: column 0
// is `key` (not `entry`) -> fromLiteral, value cells are `U+XXXX` -> fromCodepoint
            { @from@, @to@ },
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
// template/CssCodePointsLookupTable.h (row body) — the qualifying scope is plain template text
            { Id::CssTokenType::@entry@, Chars::@string@ },
```
```cpp
            { Id::CssTokenType::colon, Chars::colon },
            { Id::CssTokenType::comma, Chars::comma },
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
// template/String.h
inline const juce::String @entry:toCamel@ { "@value@" };
```
```cpp
// generated — @entry@ resolves the domain-table cell against the lexicon (byte-exact);
// @entry:toCamel@ projects the declared name; @value@ fills the lexicon's own value column
inline const juce::String failHazardURI { "contains URI scheme" };
```
`fail hazard URI`'s all-uppercase word (`URI`) survives `toCamel` intact — the abbreviation rule (Canon Files, above), not a special case for this entity. The relation table names its column `entry` — never `name` or `key` — because it is a pure reference into the lexicon; the lexicon is the only place a `name` column ever appears.

---

## Transform Vocabulary (Closed Set)
These are the only text transformations CAST can perform, applied via the `## transforms` table. All six case-family transforms (`toTitle`, `toPascal`, `toCamel`, `toKebab`, `toSnake`, `toScreamingSnake`) share one rule: **an all-uppercase declared word is an abbreviation and is case-invariant — in every projection, every position** — `UI`, `URI`, `ID` never get title-cased, Pascal-cased, or lowercased into `Ui`/`Uri`/`Id`, whether they sit first, last, or anywhere between. Every other word normalizes strictly per each projection's own rule; position rules apply to normal words only. The entire vocabulary is owned by `jam::Format` — no transform is reimplemented cast-side; the engine registers the shared functions directly.

1.  `toUpper` — Converts the value to uppercase.
2.  `toTitle` — Converts to titlecase (useful for display names). Each word's first letter uppercases, the rest lowercases — except abbreviations, which stay exactly as declared: `ui scale` becomes `UI Scale`.
3.  `toKebab` — Converts to kebab-case (e.g., `my-variable`). Non-abbreviation words lowercase and hyphen-join; abbreviations pass through intact: `data width` becomes `data-width`; `fail hazard URI` becomes `fail-hazard-URI`. Non-spaced input uses the existing kebab conversion.
4.  `toLiteral` — Renders the value as string-literal text: a bare `"` becomes `\"`, non-ASCII bytes become `\xNN`. Cells carry escape semantics: an authored backslash and the character after it pass through as one untouched pair, so `\n` means a newline, `\\` means one literal backslash, and `\"` means one quote — the author writes escape sequences, the transform never re-escapes them.
5.  `toUTF8` — Converts `U+XXXX` codepoint tokens into UTF-8 `\xNN` byte escape sequences.
6.  `toHex` — Converts a glyph or `0xNN` cell into a minimal lowercase `0xNN` integer literal.
7.  `toCodepoint` — Converts a glyph or `0xNN` cell into zero-padded `U+XXXX` codepoint notation.
8.  `toSymbol` — Qualifies a two-part symbol with its owning namespace (e.g., `A::b` becomes `juce::A::b`). Leaves three-or-more part symbols verbatim.
9.  `toPascal` — Uppercases the first letter of each space-separated word and joins them, abbreviations passing through whole: `xml operators` becomes `XmlOperators`; `UI scale` becomes `UIScale`; `fail hazard URI` becomes `FailHazardURI`. Single-word input follows the same rule — a non-abbreviation word capitalizes its first letter, an abbreviation is already correct.
10. `fromId` — Expression transform: emits the referenced entity's **own declared symbol**, resolved by its declaring table — a lexicon-declared entity projects `Id::charset.toString()`; a files-declared entity projects `files::cpp`. One declaration, one generated constant, every reference emits it. Implied type `juce::String` in every case.
11. `toCamel` — Joins space-separated words into camelCase: a non-abbreviation first word lowercases in full; an abbreviation first word passes through intact — an abbreviation never lowercases, in any position. Every remaining word Pascal-joins per the `toPascal` rule above. `hex prefix` becomes `hexPrefix`; `UI scale` becomes `UIScale`; `fail hazard URI` becomes `failHazardURI`. Single non-abbreviation words lowercase in full: `Generated` becomes `generated`.
12. `toSnake` — Underscore-joins space-separated words: non-abbreviation words lowercase; abbreviations pass through intact: `clamp to border` becomes `clamp_to_border`; `scale X` becomes `scale_X`. A wire format that demands a lowercased abbreviation authors the literal value instead of projecting.
13. `toScreamingSnake` — Uppercases space-separated words and underscore-joins them: `clamp to border` becomes `CLAMP_TO_BORDER`.
14. `join` — Removes the spaces between words, case untouched: `Long left arrow` becomes `Longleftarrow`. Used by spec key tables (Cookbook, above), where each word already carries its own exact casing.
15. `fromMap` — Expression transform: prefixes the cell verbatim with `map::` — `BlockType::document` becomes `map::BlockType::document`. Implied type `int`.
16. `fromIdentifier` — Expression transform: `Id::` plus the cell's camel projection — `body` becomes `Id::body`. Implied type `juce::Identifier`.
17. `fromLiteral` — Expression transform: wraps the cell's `toLiteral` escaping in a `juce::String::fromUTF8 ("…")` constructor. Implied type `juce::String`.
18. `fromCodepoint` — Expression transform: wraps the cell's `toUTF8` conversion in a `juce::String::fromUTF8 ("…")` constructor. Implied type `juce::String`.

**Expression transforms.** The `from*` family emits C++ expressions rather than text: each member fixes both the spelling of the emitted expression and the C++ type it evaluates to. `@from@`/`@to@` derive them from the matched table's own shape (Manifest, above), and `@from:type@` / `@to:type@` publish the implied types — one declaration site for the emission-type pairing.

**Non-ASCII keys.** A table key may contain any character — accented colour names, subscripts, symbols. Two separate rules keep the generated source portable. The identifier-producing transforms (the case family) fold each non-ASCII character to its ASCII equivalent before joining, so `Blanc Cassé` projects `blancCasse`; a character with no known equivalent collapses to a single underscore. The string-producing path leaves the characters alone and lets an escaping transform render them as byte escapes, so the authored spelling survives verbatim in the emitted literal. Generated source therefore need never carry a raw byte above 127, whatever the tables hold.

### Per-Placeholder Transforms
A placeholder may name a transform after a colon — `@column:transformName@` — and fills with that transform applied to the column's resolved cell value for the row being expanded. A transform named in the placeholder applies to that placeholder only; the `## transforms` table continues to govern the plain `@column@` placeholder. The transform name must be in the closed transform vocabulary above.

```cpp
inline constexpr juce::juce_wchar @key@ { @glyph:toHex@ };///< @glyph:toCodepoint@
```

---

##  Predicate Vocabulary (Closed Set)
These are the only validation rules CAST can enforce, applied via the `## constraints` table:

1.  `matches <regex>` — The cell must match the provided regular expression.
2.  `unique` — The cell value must be unique within the column (can span multiple tables if declared as a shared registry).
3.  `existsIn <table>.<column>` — **Foreign Key:** The cell value must match a row key (Column 0) in the specified target table.
4.  `oneOf a|b|c` — The cell value must be one of the pipe-separated values (an empty cell is permitted if it is explicitly listed in the set).
5.  `range` — A numeric cell must be between the row's declared `min` and `max` columns.
6.  `parity <table>.<column>` — Enforces key-set equality across tables (e.g., ensuring localization languages match perfectly).
7.  `fileExists <root>` — The cell value must resolve to an actual file under the declared root directory.
8.  `onePerGroup <column>` — Ensures exactly one row is marked per distinct value in the specified group column.

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
*   A template placeholder with no matching table column reports where it happened, which patch cable was being expanded, and everything that *was* available:
    > ```
    > template/CssCodePointsLookupTable.h:13 (@key@): placeholder has no source: @key@
    >   dispatch row: cssCodePoints (source table: cssCodePoints)
    >   available here: @row:begin@ @row:end@ @row:index@ @entry@ @string@ @table@ @table:toPascal@ @brief@
    > ```
    The classic cause: the template says `@key@` but the table's column 0 is `entry` (or vice versa) — the *available here* list shows which one the table actually provides.
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
