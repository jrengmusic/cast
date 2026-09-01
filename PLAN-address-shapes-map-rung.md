# PLAN: Address-Form Shapes + Positional Map Rung — CAST as Straight String Replacement

**RFC:** none — objective from ARCHITECT's rulings this session
**Date:** 2026-08-31
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + JAM (LANGUAGE.md §C++: single-header default; 30/3 unchanged)

## CONTRACT (ARCHITECT, verbatim)

0 copy · 0 allocation · 0 temp containers · 0 mutation · 0 shadow state · 0 shadow copy ·
0 hand-rolled string parsing · 0 hand-rolled arithmetic · 0 new pattern · 0 foreign
semantics. NAMES.md is the contract — no ratification round-trips. Framework API OOTB to
its fullest extent. No gates: agents build, run `./cast`, diff, configure; every
residual resolves in-sprint. Read-only git remains forbidden.

## Context

CAST could not express `:::token1::: :::token2:::` from a `key | value` table. Root cause:
the engine's only token supplier is a column header — Shapes.h:218, Items.h:261/329 — and
SPEC §6.5 / §2:46 wrote that limitation plus "one `.cast` per directory" as law. EVE's
CMakeLists.txt was authored around it: transposed one-row tables (project-info.md:148-152),
nine cmake item blocks baking structure (template.cast:588-664), `## modules`/`## libraries`
restating 13 names.

## Rulings

1. `template` is not a reserved word. A shape is `@<alias>:<fence>`; the alias's index
   symbol is a `.cast` file. N templates. jam `template.cast` → `code.cast`; EVE's cmake
   blocks → `eve/cast/cmake.cast`.
2. Map = a table read by row key: jam keys every data row by its **first column**
   (jam_MarkdownBlockParser.cpp:1598-1604) → lookup is the O(1) probe
   `getTableCell (table, Id::value, token)`. `key | value`, `name | type | value | …`.
3. Map pairing is positional: list-column `- list:` lines in excess of the structure
   column's at a `>` count are map lines, grouped to that count's paragraphs by order;
   blank `- list:` closes a group; fewer groups fill trailing paragraphs; a different `>`
   count is a different scope.
4. Expansion, arity, `>`, separators, bindings, record rows: untouched.
5. EVE: everything in `project-info.md`, the toolchain manifest — every list table
   `name | value | comment` (path lists `root | name | comment`), comments rendered inline
   in cmake syntax (comments.md `## cmake comment` via `manifestSyntax`), padded by §7.2.
   `## project info` reordered `name | type | value | format | comment` (first column =
   key), gains `companyCopyright`, `companyEmail`; `format`/`architecture` are lists; join
   datums `@space` (`U+0020`, `fromUTF8`) / `@semicolon` in the manifest index.
6. `toLiteral` output is the correct literal in every target language (SPEC §9);
   `project("EVE" VERSION "0.1.0")` is right.

Gate (self-run): jam + cast generated outputs byte-identical; EVE CMakeLists.txt fixpoint +
`cmake -G Ninja` configures.

## Engine Design (Step 2 + Step 4)

- `TemplateDocument` keeps its name, stops deriving from `jam::MarkdownDocument`: owns
  `jam::HashMap<juce::String, jam::MarkdownDocument>` keyed by the index symbol as authored;
  built from `const Model&` — every index row with `Extensions::cast` parsed with its symbol
  as origin, blocks stamped `Id::value` + `Id::placeholder` (existing ctor loop).
  `getCodeBlock (Element& line)` reads `Id::templatePath` (template path) + `Id::info`
  (fence) → `documents.at (path).getCodeBlock (fence)`. `getBlockValue (model, row, address)`,
  `getBinding (model, row, value)` resolve `@alias:fence` at read for separators/bindings.
- `Model::isShape (row, value)`: `@`-value whose alias symbol has `Extensions::cast`.
  Replaces every `getPreColon == Id::templatePath` test (Model.h:538/553/583/618,
  TemplateDocument.h:86). Stamp at parse: `Id::templatePath` = resolved template path,
  `Id::info` = fence.
- `Model::getValue (file, alias)` reads the symbol cell's `Id::value` stamp (formatted per
  §4.1), not raw subtext.
- Delete `Model::getTemplateFile`, the unused `templateDocument` parameter of `addLines`
  and its parse (Model.h:450-451). Writer.h:152 output buffer → `jam::MarkdownDocument`.
- Map stamp (parse, list column, before ordinal stamping): per `>` count, the first
  `listCount − structureListCount` `- list:` bullets are map lines, stamped `Id::shape` =
  owning paragraph's line index (binding precedent, Model.h:613) and no `Id::level`/
  `Id::line`; expansions' ordinals unshifted. `getSource`/`getSeparator` read only bullets
  carrying `Id::line`. `Model::getMap (row, line, occurrence)`: the occurrence-th non-blank
  map bullet whose `Id::shape` equals the paragraph's, through `getTable (row, value)`.
- `Shapes::getTokenValue`: binding → maps (`getTableCell (*map, Id::value, name)`) →
  comment → column.
- Validator: `isReference` treats a shape address as resolved by `isShape`; `hasTemplate`/
  `isStructure`/`isMarkerCountValid` take the line; new `isMap`: map bullet with no
  paragraph at its count → `failMapOrphan`; map table without `value` header →
  `failColumnUnknown`.

## Steps

1. SPEC §1/§2/§4.2/§6.1/§6.3/§6.5/§6.6/§7/§10.1 + HELP sync — done.
2. Engine: TemplateDocument pool + `isShape` + address stamps + `getValue` stamp read.
3. Data: `template.cast` → `code.cast`; jam/cast/eve manifests `template:` → `@code:`,
   index `@template` → `@code`. Build, run cast on jam + cast, diff empty.
4. Engine: map rung + `isMap` + `failMapOrphan` (text.md) — build, regen cast, diff.
5. `eve/cast/cmake.cast` — one `cmake` block, 25 inline tokens, 12 list slots, single-liners
   `entry` (`:::value::::::comment:::`), `value` (`:::value:::`), `link` (`:::name:::`),
   `patch` (`":::root:::/:::name:::":::comment:::`), `module`
   (`juce_add_module(":::root:::/:::name:::"):::comment:::`); cmake family deleted from
   `code.cast`.
6. Librarian: flag/define meanings, cited. `eve/project-info.md` + `eve/cast/CAST.md`
   rewritten. Run cast on eve twice; `cmake -G Ninja` scratch configure.
7. Auditor sweep; all findings + prior residuals #6/#11/#19/#20/#26 resolved.
