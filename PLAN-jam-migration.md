# PLAN: jam Migration to CAST Canon (sigil + token contract)

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-08-16
**BLESSED Compliance:** verified
**Language Constraints:** Markdown table data + C++17/JUCE oracle promotion (LANGUAGE.md: header-only, grid tables canon)

## Context

CAST is self-hosting on the new canon: `#`-sigiled `## index` (`|alias|symbol|` + optional `format`), matched-replace fill rule, token column (`instance, default: shared, text`), CMARK+Pandoc formatter (HELP.md is the updated SPEC). jam's cast inputs predate all of it: un-sigiled `|alias|path|` index, bare reference cells, bare `id`/`string` type cells, no `default` tokens. Objective: migrate jam/cast to the SPEC and iterate `./cast jam/cast/CAST.md` until `jam/diff/` is **100% byte-identical** to the checked-in `jam/generated/` oracle — except the ratified getDefault change below, which re-promotes the oracle.

## Locked Decisions (ARCHITECT, this session)

1. **Mermaid is sandboxed** — jam_mermaid is not a module dep. `tables/mermaid.md` and mermaid vocabulary are never edited. The 15 mermaid rows in `## output` receive only the mechanical sigil/token treatment every row gets (required for the manifest to parse); their generated output must remain byte-identical. Gate: CAST + jam compile clean.
2. **No numbers — TOKEN.** Every bimap `default` token is the default row's **key name** (like cast's `map.at (text)`), uniformly across all 62 structs, including the five symbol-literal cases (ColourNames, ColourIdMap, AtRuleType, BlockTag, HeadingTag → token = the row whose value the oracle argument names). Oracle `jam_Bimaps.h`/`jam_Enums.h` getDefault lines change from `map.at (0)`-style literals to `map.at (<keyToken>)` — diff-reviewed, then ARCHITECT re-promotes.
3. Formatter spec is CMARK + Pandoc grid tables (HELP.md Canonical Markdown) — jam corpus is formatted by the default generate-then-format pass; grid group borders preserved.
4. Oracle-first discipline continues: outputs write to `jam/diff/` (already wired in jam's index); `jam/generated/` is the oracle; promotion is ARCHITECT's `cp`/git action.

## Evidence Inventory (Pathfinder, verbatim reads this session)

- jam/cast/CAST.md: `## index` lines 107–153 (`|alias|path|`, un-sigiled, file cells → `../diff/*.h`); `## output` lines 155–263 (109 rows, 7 cols `code|namespace|namespace name|list|token|lineBreak|file`, token cells already `instance: shared` style); `## output index` lines 265–271 (1 row, 9 cols). Infra tables (Source/Product Identity, Modules, Paths, …) lines 1–106 are not codegen tables — formatter-only.
- Cross-project reference: index row `../../cast/cast/tables/comments.md` (jam consumes cast's comments table) — stays, alias sigiled like the rest.
- jam/cast/tables/lexicon.md: 1,295 lines; `|name|value|type|` with `id`×1259, `string`×20 type cells; `## index` already exists at :1288 (`id → juce::Identifier`, `string → juce::String`) — un-sigiled.
- jam/cast/tables/files.md: `## files` + `## extensions`, both `|name|value|type|` — has type cells, **no `## index`**.
- localisation-en.md: `## text` + `## Sharps`/`## Flats`/`## romanNumerals`, key|value shapes, no type cells — data untouched.
- template/Bimap.cast already carries `:::default:::` (line 21 `return map.at (:::default:::);`) — templates are shared with cast and already canon; zero template edits expected.
- Oracle getDefault map (all 62 structs → argument) captured verbatim this session (e.g. `Screen→0`, `HeadingLevel→1`, `VariDisplayMode→2`, `UIScaleMap→3`, `AnalyzerMode→1`, `ColourNames→0xffff00ff`, `ColourIdMap→juce::DocumentWindow::textColourId`, `AtRuleType→CssRuleType::fontFaceRule`, `BlockTag→BlockType::document`, `HeadingTag→HeadingLevel::level1`) — Step 3 translates each argument to its row-key token by reading the source table row whose value matches.
- Engine contract (this sprint, cast side): `Model::getValue(file, alias)` resolves `#`-sigiled cells against the containing file's `## index` (`|alias|symbol|` + optional `format`); matched-replace fill; token-vocabulary region membership; `Id::symbol` is the index value column name — jam's `path` header must rename to `symbol`.

## Validation Gate

Each step validated by COUNSELOR against MANIFESTO.md (BLESSED), NAMES.md, CODING.md, HELP.md (SPEC), and the locked decisions above before the next step. @Auditor only on explicit ARCHITECT command. Kill-and-correct triggers: any mermaid.md edit, any template edit, any new name not in the delegation prompt, any hand-invented alias.

## Steps

### Step 1: Manifest index — sigil + symbol
**Scope:** jam/cast/CAST.md `## index` only
**Action:** @Engineer: rename column header `path` → `symbol`; prefix every alias in column 0 with `#` (alias text otherwise unchanged). No row added, removed, or reordered; symbols untouched.
**Validation:** 47 rows intact; every alias sigiled; header `| alias | symbol |`; diff shows only column-0 `#` prefixes + one header word.

### Step 2: Manifest reference cells — sigil
**Scope:** jam/cast/CAST.md `## output`, `## output index`
**Action:** @Engineer: in both tables, sigil every index-referencing cell: `code`, wrapper columns (`namespace`, `struct`), `lineBreak`, `file`, `output` cells → `#alias`; `list` and `output`-source cells `alias:heading` → `#alias:heading`. Bare literals (`Id`, `map`, `shared`, struct names, namespace names) stay bare. No row/column changes.
**Validation:** every sigiled cell's alias exists in Step 1's index; zero literal cells altered; row count 109 + 1 unchanged; mermaid rows touched only by sigils.

### Step 3: Bimap default tokens
**Scope:** jam/cast/CAST.md `## output` bimap rows (jam_Bimaps + jam_Enums targets, mermaid rows included mechanically)
**Action:** COUNSELOR derives the token per struct: read the source table, find the row whose `value` equals the oracle getDefault argument, take that row's key projected exactly as the enum member is emitted (`:::key:toCamel:::`). @Engineer extends each bimap row's token cell to `instance, default: shared, <keyToken>` per the supplied table — zero self-derived names.
**Validation:** 62 tokens supplied, each traced to a source-table row; ambiguous or missing matches stop for ARCHITECT; no numbers anywhere in a default token.

### Step 4: Table files — sigil type vocabulary
**Scope:** jam/cast/tables/lexicon.md, files.md
**Action:** @Engineer: lexicon.md — sigil the `## index` aliases (`#id`, `#string`) and all 1,279 type cells (`id`→`#id`, `string`→`#string`). files.md — add the same two-row `## index` (`#id`, `#string` — matching cast/tables canon shape) and sigil its type cells. No other file touched; mermaid.md untouched.
**Validation:** type-cell counts match pre-migration counts exactly (1259+20 in lexicon); no data cell altered beyond the sigil; localisation-en.md and every domain table byte-unchanged.

### Step 5: Generate → diff loop
**Scope:** execution + read-only verification, no source edits without a named discrepancy
**Action:** ARCHITECT (or COUNSELOR on go) runs `./cast jam/cast/CAST.md`. @Pathfinder diffs `jam/diff/` vs `jam/generated/` file by file. Expected sole diff class: getDefault arguments (decision 2). Every other diff hunk is a discrepancy — root-caused (table data vs engine) and fixed at its owner; loop until the only remaining diffs are the ratified getDefault lines.
**Validation:** `diff -r` output contains exclusively getDefault hunks; mermaid outputs byte-identical; FATAL diagnostics zero.

### Step 6: Oracle promotion + build gate
**Scope:** jam/generated/ (ARCHITECT), builds
**Action:** ARCHITECT reviews the getDefault diff and promotes `jam/diff/` → `jam/generated/`. Build jam-dependent project + cast; both compile clean (mermaid gate).
**Validation:** clean builds; post-promotion rerun of `./cast jam/cast/CAST.md` yields empty diff — jam reaches fixpoint.

### Step 7: Formatter pass over jam corpus
**Scope:** jam/cast/*.md via the default format pass
**Action:** The Step 5/6 runs already format (default generate-then-format). Verify: second run leaves every jam .md byte-identical (idempotency); grid group borders and `## Sync Ignore` fence preserved; regeneration after formatting still fixpoint.
**Validation:** hash-stable .md corpus across two runs; generated outputs unchanged by formatting.

## BLESSED Alignment

- **S (SSOT):** default lives once, in the manifest token, named by the row key — the oracle stops carrying magic numbers
- **E (Explicit):** sigil law makes every reference visibly a reference; bare = literal, collision-free
- **D:** byte-identical diff loop + fixpoint + formatter idempotency are the determinism proofs
- **B/E (Encapsulation):** mermaid sandbox boundary respected — mechanical manifest treatment only, vocabulary untouched

## Risks / Open Questions

- 109-row sigil edit is mechanical but wide — Steps 1–3 are separate Engineer tasks to keep each diff reviewable
- Unknown engine/spec gaps may surface in Step 5 (jam corpus is 40× cast's); each is a named discrepancy back to ARCHITECT, never a silent workaround
- jam_Enums.h terminal bimaps (54) share Bimap.cast — default tokens required there too; same rule, same derivation
