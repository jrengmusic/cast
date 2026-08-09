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

CAST operates on exactly three kinds of files. Generated output files are considered untracked build artifacts and should not be edited by hand.

### 1. Relations (Input Tables)
Your data lives in standard Markdown files as GitHub Flavored Markdown (GFM) tables.
*   **Naming:** Any `## Heading` immediately followed by a GFM table creates a "Relation" named after that heading.
*   **Keys:** The **first column (Column 0)** is the row key. Looking up a row by value always checks this column.

#### Column 0: `key` vs `entry`

The column-0 **header** declares what kind of table this is. Pick exactly one:

| Header | Meaning | Constraint |
|---|---|---|
| `key` | **Declares** a new identifier. This row is the identifier's single point of origin. | `unique` — the value may not be declared anywhere else, in any table. |
| `entry` | **References** an identifier declared elsewhere (in some `key` table). | Not subject to `unique` — references repeat freely. |

Rules of thumb:
*   Vocabulary tables (identifiers, chars, strings, operators) **declare** → `key`.
*   Relational tables (bimaps, classification maps, lookup tables, anything whose column 0 names things that already exist) **reference** → `entry`.
*   The fragment template must address column 0 by the same name: a `key` table fills `@key@`, an `entry` table fills `@entry@`. A mismatch fails with the *placeholder has no source* diagnostic below.

Getting this wrong is loud. Declaring `closeParen` in an `entry`-style table under a `key` header fails like this:

> `tables/cssCodePoints.md: duplicate "closeParen" already declared at tables/cast.md`
*   **Cells:** All cells become plain strings. Text inside backticks (`` ` ``) unwraps to its plain content. Numbers are kept exactly as you typed them—CAST will never reformat or reparse numbers.
*   **Order:** Rows are always output in the exact order you authored them. CAST never sorts rows.

#### Cell Resolution
Before a declared transform (from `## transforms`) is applied, each cell value is resolved against the row key:
*   An **empty cell** resolves to the row key (column-0 value).
*   A cell whose text **names a transform** (e.g. `toUpper`) resolves to that transform applied to the row key.
*   Any **other cell** is its own literal value.
*   A cell **wrapped in backticks** is a literal and is never resolved — no transform lookup, no empty-cell fallback.
*   The **first column (the row key itself)** is never resolved.
The resolved value is then passed through any declared column transform.

> **FATAL HAZARD RULE:** If a cell contains `<`, `>`, or a URI scheme (like `http://`) and it is *not* wrapped in backticks, CAST will crash. Always wrap special characters in backticks.

#### Lexicon Reference Resolution

When `lexicon.md` is present in the tables directory and a dispatched source table's column-0 header is `entry`, each entry cell is resolved as a reference into the lexicon registry:

*   The entry cell is looked up **case-insensitively** in the lexicon's `name` column.
*   On hit: `@entry@` fills with the **canonical name** (as declared in the lexicon, spaces and casing preserved). `@value@` fills with the referenced entity's value, passed through standard cell resolution and any declared column transform.
*   On miss: **FATAL** — the entity is not declared.
*   When no `lexicon.md` is present, `entry`-headed tables expand without reference resolution (backward compatible).

### 2. Templates (Formatting)
Templates are plain text files containing exactly one special construct: `@placeholder@`. There are no loops, no `if` statements, and no expressions.
*   **Scalar Placeholder:** Replaced by a single cell value or a single manifest value.
*   **Aggregate Placeholder:** Replaced by an entire table, projected through fragment templates, in the order you authored the rows.
*   **Root Templates:** Used to generate exactly one output file.
*   **Fragment Templates:** Used for per-row expansions inside Root Templates. Fragments never generate files themselves.
*   **Logic:** Any actual code logic (function bodies, framework calls) lives as plain text in the template. Templates do not execute logic.

#### Row-Region Templates
A fragment template containing `@row:begin@` and `@row:end@` is a *row-region template*. It expands once for its whole matched table rather than once per row:
*   The entire line containing `@row:begin@` and the entire line containing `@row:end@` are consumed (not emitted).
*   Text between those two marker lines repeats once per matched row, filled from that row's cells.
*   `@row:index@` — inside a row region, fills with the zero-based position of the row within the matched row set. It is not a table-level placeholder and is not available outside a row region.
*   `@cell@` — the current row's value in the dispatch row's filter column (per-row; transforms apply, e.g. `@cell:toPascal@`). Available in both row-region and per-row fragment expansion.
*   Text outside the region sees *table-level placeholders* (see below) rather than per-row placeholders.

**Table-level placeholders** (valid outside a row region in a row-region template):
*   `@table@` — the table's name (the heading text).
*   `@table:transformName@` — the table's name run through a transform. `@table:toPascal@` turns a `## xmlOperators` heading into `XmlOperators` — the standard way to derive a struct name from a table name.
*   `@brief@` — the prose paragraph authored between the table's `##` heading and the table itself; empty string if none.
*   `@columnName@` — for any column header, the column-0 key of the *first* row whose cell in that column is non-empty. Useful for a `default` marker column: if one row has `yes` in that column, `@default@` fills with that row's key.
*   `@column@` — the dispatch row's filter column name (transforms apply, e.g. `@column:toPascal@`). Available in both row-region and per-row fragment expansion.

### 3. Manifest (`CAST.md`)
The manifest is the "brain" of the operation. It contains four specific GFM tables that tell CAST how to combine Relations and Templates:

*   `## outputs` — Maps a **Root Template** → **Output File Path** (columns: `template | output`, output right-most). Relations are always the `.md` files directly inside the `tables` directory beside the manifest.
*   `## dispatch` — Maps a **(Table, Column, Value or Presence)** → **Fragment Template** → **Slot Name**. This is how you tell CAST to use different fragments based on a cell's value (or simply if a cell exists).
*   `## transforms` — Maps a **Column** to a specific text transformation (see Transform Vocabulary below).
*   `## constraints` — Maps a **Column** to a validation rule (see Predicate Vocabulary below), such as Foreign Keys.

> **FATAL MANIFEST ERRORS:** CAST will immediately crash if it detects:
> *   An **Orphan template** (a template file not referenced by the manifest).
> *   An **Undeclared output** (an output file generated outside of `## outputs`).
> *   An **Unmapped fragment** (a fragment missing from dispatch, or a dispatch pointing to a non-existent fragment).

---

## Canon Files

Every CAST-driven project declares its generation inputs in three canon files:

| File | Role |
|------|------|
| `CAST.md` | Codegen Annotated Source of Truth — the manifest |
| `lexicon.md` | Every entity declared once: `\| name \| value \|` |
| `relations.md` | m:n mappings between lexicon entities |

**Entity rules:**
*   An entity is unique, whole, and opaque. `UI`, `scale`, and `UI scale` are three independent declarations — CAST never decomposes or derives one from another.
*   Uniqueness is global and case-insensitive. First declaration wins and fixes the casing.
*   Word boundaries (spaces) and per-word casing are stored data — the declaration is the single source of every projected form.
*   Every use outside the declaration — relation cells, template tags — is a reference, resolved case-insensitively. Referencing an undeclared entity is a **FATAL** generation error:
    > `tables/relations.md: entity not declared in lexicon: myMissingEntity`

---

## Cookbook: Table + Template → Code

Every generated construct is plain text substitution — nothing more. These examples show each pairing end to end. In all of them the dispatch row's `column` cell names the table's column-0 header, and the `slot` collects the expanded fragments into the root template's aggregate placeholder.

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

### 2. Char constants — per-placeholder transforms

```markdown
## characters
| key   | glyph |
| ----- | ----- |
| at    | @     |
| space | 0x20  |
```
```cpp
// template/Char.h
inline constexpr juce::juce_wchar @key@ { @glyph:codepointHex@ };///< @glyph:codepointLabel@
```
```cpp
// generated — one column, two projections via @column:transform@
inline constexpr juce::juce_wchar at { 0x40 };///< U+0040
inline constexpr juce::juce_wchar space { 0x20 };///< U+0020
```

### 3. String constants — literals survive via backticks

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

### 4. Operator struct — row region + `@table:toPascal@`

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
One template, many structs: give each dispatch row a unique key and point its `source` column at a different table — `xmlOperatorStruct → xmlOperators`, `cssOperatorStruct → cssOperators` — all through the same `template/Operator.h`.

### 5. Bimap — `entry` table + table-level placeholders

```markdown
## Screen
| entry | value | views | default |
| ----- | ----- | ----- | ------- |
| main  | 0     |       | yes     |
| alt   | 1     |       |         |
```
```cpp
// template/Bimap.h (excerpt) — @default@ fills with the key of the first row marked in that column
struct @table@ : public jam::Bimap<@table@>
{
    enum value
    {
@row:begin@
        @entry@ = @value@,
@row:end@
    };

    const juce::String& getDefault() const noexcept override { return hashmap.at (@default@); }
};
```
```cpp
struct Screen : public jam::Bimap<Screen>
{
    enum value
    {
        main = 0,
        alt = 1,
    };

    const juce::String& getDefault() const noexcept override { return hashmap.at (main); }
};
```
Column 0 is `entry` because `main` and `alt` are declared once in a `key` table elsewhere — the bimap only references them.

### 6. Relational HashMap — `entry` + `toString`

```markdown
## atRuleType
| entry         | value                    |
| ------------- | ------------------------ |
| atRuleMedia   | CssRuleType::mediaRule   |
| atRuleCharset | CssRuleType::charsetRule |
```
```cpp
// template/IdToIntDefaultHashMap.h (row body)
            { @entry:toString@, Id::@value@ },
```
```cpp
// generated — runtime map keyed by identifier strings
            { Id::atRuleMedia.toString(), Id::CssRuleType::mediaRule },
            { Id::atRuleCharset.toString(), Id::CssRuleType::charsetRule },
```

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

---

## Transform Vocabulary (Closed Set)
These are the only text transformations CAST can perform, applied via the `## transforms` table:

1.  `toUpper` — Converts the value to uppercase.
2.  `toTitle` — Converts to titlecase (useful for display names).
3.  `toKebab` — Converts to kebab-case (e.g., `my-variable`). Space-separated words are lowercased and hyphen-joined: `data width` becomes `data-width`. Non-spaced input uses the existing kebab conversion.
4.  `escapeCpp` — Escapes for C string literals: `"` becomes `\"`, `\` becomes `\\`, non-ASCII bytes become `\xNN`.
5.  `utf8Bytes` — Converts `U+XXXX` codepoint tokens into UTF-8 `\xNN` byte escape sequences.
6.  `codepointHex` — Converts a glyph or `0xNN` cell into a minimal lowercase `0xNN` integer literal.
7.  `codepointLabel` — Converts a codepoint into a zero-padded `U+XXXX` notation.
8.  `qualifySymbol` — Adds a namespace to a two-part symbol (e.g., `A::b` becomes `juce::A::b`). Leaves three-or-more part symbols verbatim.
9.  `symbolFromFile` — Replaces dots with underscores to create a `BinaryData` symbol from a filename.
10. `toPascal` — Uppercases the first character of each space-separated word and joins them: `xml operators` becomes `XmlOperators`; `UI scale` becomes `UIScale`. Single-word input uppercases the first character only, preserving the rest.
11. `toString` — Wraps a bare identifier word as an identifier-string expression: `charset` becomes `Id::charset.toString()`. Used by relational templates whose runtime maps are keyed by identifier strings.
12. `toCamel` — Joins space-separated words into camelCase: first word entirely lowercased, each remaining word's first character uppercased. `hex prefix` becomes `hexPrefix`; `UI scale` becomes `uiScale`. Single words are lowercased: `Generated` becomes `generated`.

### Per-Placeholder Transforms
A placeholder may name a transform after a colon — `@column:transformName@` — and fills with that transform applied to the column's resolved cell value for the row being expanded. A transform named in the placeholder applies to that placeholder only; the `## transforms` table continues to govern the plain `@column@` placeholder. The transform name must be in the closed transform vocabulary above.

```cpp
inline constexpr juce::juce_wchar @key@ { @glyph:codepointHex@ };///< @glyph:codepointLabel@
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
*   A template placeholder with no matching table column reports where it happened, which dispatch row was being expanded, and everything that *was* available:
    > ```
    > template/CssCodePointsLookupTable.h:13 (@key@): placeholder has no source: @key@
    >   dispatch row: cssCodePoints (source table: cssCodePoints)
    >   available here: @row:begin@ @row:end@ @row:index@ @entry@ @string@ @table@ @table:toPascal@ @brief@
    > ```
    The classic cause: the template says `@key@` but the table's column 0 is `entry` (or vice versa) — the *available here* list shows which one the table actually provides.
*   A duplicate declaration names both the offender and the original:
    > `tables/cssCodePoints.md: duplicate "closeParen" already declared at tables/cast.md`

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
*   **Output Structure:** Generated output is header-only, grouped by construct kind. Root templates and manifest structure remain identical across platforms/frameworks.
