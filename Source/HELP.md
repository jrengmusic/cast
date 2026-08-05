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
*   **Keys:** The **first column (Column 0)** is the row key. Looking up a row by value always checks this column. (Note: It is your responsibility to ensure keys are unique if you use them for lookups).
*   **Cells:** All cells become plain strings. Text inside backticks (`` ` ``) unwraps to its plain content. Numbers are kept exactly as you typed them—CAST will never reformat or reparse numbers.
*   **Order:** Rows are always output in the exact order you authored them. CAST never sorts rows.

> **FATAL HAZARD RULE:** If a cell contains `<`, `>`, or a URI scheme (like `http://`) and it is *not* wrapped in backticks, CAST will crash. Always wrap special characters in backticks.

### 2. Templates (Formatting)
Templates are plain text files containing exactly one special construct: `@hole@`. There are no loops, no `if` statements, and no expressions.
*   **Scalar Hole:** Replaced by a single cell value or a single manifest value.
*   **Aggregate Hole:** Replaced by an entire table, projected through fragment templates, in the order you authored the rows.
*   **Root Templates:** Used to generate exactly one output file.
*   **Fragment Templates:** Used for per-row expansions inside Root Templates. Fragments never generate files themselves.
*   **Logic:** Any actual code logic (function bodies, framework calls) lives as plain text in the template. Templates do not execute logic.

### 3. Manifest (`CAST.md`)
The manifest is the "brain" of the operation. It contains four specific GFM tables that tell CAST how to combine Relations and Templates:

*   `## outputs` — Maps an **Output File Path** ← **Root Template** ← **Input Table(s)**.
*   `## dispatch` — Maps a **(Table, Column, Value or Presence)** → **Fragment Template** → **Slot Name**. This is how you tell CAST to use different fragments based on a cell's value (or simply if a cell exists).
*   `## transforms` — Maps a **Column** to a specific text transformation (see Transform Vocabulary below).
*   `## constraints` — Maps a **Column** to a validation rule (see Predicate Vocabulary below), such as Foreign Keys.

> **FATAL MANIFEST ERRORS:** CAST will immediately crash if it detects:
> *   An **Orphan template** (a template file not referenced by the manifest).
> *   An **Undeclared output** (an output file generated outside of `## outputs`).
> *   An **Unmapped fragment** (a fragment missing from dispatch, or a dispatch pointing to a non-existent fragment).

---

## 🔧 Transform Vocabulary (Closed Set)
These are the only text transformations CAST can perform, applied via the `## transforms` table:

1.  `toUpper` — Converts the value to uppercase.
2.  `toTitle` — Converts to titlecase (useful for display names).
3.  `toKebab` — Converts to kebab-case (e.g., `my-variable`).
4.  `escapeCpp` — Escapes for C string literals: `"` becomes `\"`, `\` becomes `\\`, non-ASCII bytes become `\xNN`.
5.  `utf8Bytes` — Converts `U+XXXX` codepoint tokens into UTF-8 `\xNN` byte escape sequences.
6.  `codepointHex` — Converts a glyph or `0xNN` cell into a `0xNNNN` integer literal.
7.  `codepointLabel` — Converts a codepoint into a zero-padded `U+XXXX` notation.
8.  `qualifySymbol` — Adds a namespace to a two-part symbol (e.g., `A::b` becomes `juce::A::b`). Leaves three-or-more part symbols verbatim.
9.  `symbolFromFile` — Replaces dots with underscores to create a `BinaryData` symbol from a filename.

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

## 🛡️ Determinism & Failure Behavior

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
