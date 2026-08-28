# PLAN: Comment Wiring — Plain-Replacement Documentation Channel

**RFC:** none — objective from ARCHITECT prompt (this session's ruling)
**Date:** 2026-08-28
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + JAM; cast engine reads via jam::MarkdownDocument only

## Context

jam/generated (target oracle, hand-authored doxygen) vs jam/diff residual is now
**documentation-only across all 9 headers** (code layer converged this session after the
c4 `type`-column fix). ARCHITECT's ratified design: `:::comment:::` is an ordinary
token — plain table-data-to-template string replacement. CAST knows nothing about
doxygen or any comment syntax. The frame (`/** … */`, `///<`, `@brief`) is authored
text: it arrives verbatim from the data (or wherever the template author puts it).
Resolution scope is already correct in the engine: column match = inline (per row);
fenced block/paragraph above the table = block (per table); the template decides
placement. Nothing is expressed in the manifest.

What dies: the engine's framing special-case — branching on the token *name*
`comment` and applying language-aware `toBrief`/`toComment` after resolution.

## Dependency & API Inventory (Pathfinder + direct reads)

- Resolution ladder already exists and stays: binding → row cell → table block —
  Shapes.h:83-92 (`getTokenValue`), commentTable via Shapes.h:64-78; block stamped at
  parse Model.h:339-363 (`addComments`, paragraph or fenced block, `getAllSubText`).
- Framing branches to delete: Shapes.h:117-120, Items.h:223-224, Items.h:291-292.
- Elision already works: empty token → lone line dropped (Shapes.h:145-155), inline
  appends nothing; `comment` exempt from supply gate (Validator.h:444) and from §5.3
  uniqueness (SPEC:220-226).
- `:::comment:::` in jam/cast/template.cast today: wchar:53 (inline), bimap:91
  (standalone), enumEntry:123 (inline), lookupTable:156, staticLookupTable:165
  (standalone). cast/cast/template.cast: bimap:47 (standalone).
- Existing comment DATA (all in jam/cast/bimaps.md): 22 tables with `comment` column
  (~189 non-empty cells, bimaps.md:3945-4320) currently emitted as `///<` via
  `Transforms::toComment`; 5 table paragraphs (bimaps.md:3959, 3990, 4013, 4064, 4075)
  currently emitted via `toBrief`. These converge today and MUST be migrated in the
  same step as the engine deletion or jam_Bimaps.h/jam_Chars.h regress.
- Width/padding law unaffected: Items measures the framed value today
  (Items.h:291-294); a verbatim `///< text` cell measures identically.
- Transforms::toComment/toCommentBlock/toBrief remain registered named transforms
  (Transforms.h:226-230) reachable from `format` cells and the Writer banner stamp
  (Writer.h:206 uses map::commentSyntax) — NOT deleted in this sprint; only the
  hardcoded `Id::comment` branches die. Dead-code verdict goes to Auditor.
- Oracle inventory (jam/generated): HashMaps 6 blocks/0 inline; LookupTables 12/0;
  Chars 4/156; Files 3/84; Text 2/47; Generated 2/93; Bimaps 70/189; Identifiers
  2/252; Mermaid 76/3.

## Validation Gate

Each step validated by COUNSELOR against MANIFESTO.md, NAMES.md, CODING.md, and this
plan before the next. ARCHITECT builds and runs `./cast jam/cast/CAST.md` at each
convergence gate; `cmp` per target header is the only completion check. @Auditor runs
ONCE, after the final step.

## Steps

### Step 1: Engine — delete the framing branches
**Scope:** Source/Shapes.h, Source/Items.h
**Action (@Engineer):** Delete the `if (name == Id::comment …) value = Transforms::…`
branches at Shapes.h:117-120, Items.h:223-224, Items.h:291-292. `comment` becomes an
ordinary token: resolved value replaces the marker verbatim. No other change; no new
names; Transforms.h untouched.
**Validation:** No `Id::comment` special-case remains in the substitution path; the
resolution ladder (Shapes.h:83-92) untouched; grep confirms remaining `Id::comment`
references are resolution/exemption only (Shapes.h:89, Validator.h:444, Items.h:575).

### Step 2: Data migration — bimaps.md carries its frames verbatim
**Scope:** jam/cast/bimaps.md only
**Action (@Engineer, Destructive-Edit Discipline — dry-run, backup, verify counts):**
1. Prepend `///< ` to every non-empty `comment` cell across the 22 tables
   (bimaps.md:3945-4320, ~189 cells). Resulting emission must be byte-identical to
   what `toComment` produced (`///<` + space + text).
2. Replace the 5 table paragraphs (bimaps.md:3959, 3990, 4013, 4064, 4075) with text
   carrying the frame verbatim, matched byte-for-byte against the corresponding
   oracle block in jam/generated/jam_Bimaps.h (single-line `/** @brief text */` where
   the oracle has single-line; fenced code block with the full multi-line frame where
   the oracle is multi-line).
**Validation:** cell count edited == dry-run prediction; no other column touched.
**Gate (ARCHITECT):** build + run; jam_Bimaps.h `///<` lines and the 5 briefs do not
regress vs pre-change output; `--format` fixpoint clean.

### Step 3: SPEC.md §5.4 amendment
**Scope:** SPEC.md (cast root)
**Action:** Rewrite SPEC:234-251: the marker is plain replacement — the engine formats
nothing; comment text carries its own frame, authored in the data (or the template,
wherever the author puts it); position and scope rules (inline = column, block = text
above table, template decides) stay as written. Amend §9 comment-family prose
(SPEC:454-455): comment transforms serve the banner stamp and `format` cells only.
**Validation:** SPEC states no engine formatting; no contradiction with §5.3/§6.5
exemptions.

### Step 4: Increment 1 — jam_HashMaps.h construct briefs (pilot)
**Scope:** jam/cast/template.cast (hashMap block), jam/cast/syntax.md, text.md,
entities.md, chars.md
**Action (@Engineer):**
1. template.cast `hashMap` block gains a lone `:::comment:::` first line (same shape
   as lookupTable:156).
2. Author 5 fenced code blocks, one above each source table — `## languageFamily`
   (syntax.md:235), `## romanNumerals` (text.md:151), `## entities` (entities.md:11),
   `## Diacritics` (chars.md), `## xmlEscapes` (chars.md:191) — text lifted VERBATIM
   from the oracle blocks in jam/generated/jam_HashMaps.h (full `/** … */` frame,
   hand-wrapped lines preserved byte-exact). Zero invented prose.
**Validation:** blocks byte-match the oracle; no other template block touched.
**Gate (ARCHITECT):** build + run; `diff jam/diff/jam_HashMaps.h
jam/generated/jam_HashMaps.h` residual == the `@file` header block only.

### Steps 5–11: Remaining increments, same pattern, one file per gate
Order (block-only first, inline-heavy after): jam_LookupTables.h (11 construct
blocks) → jam_Bimaps.h (remaining struct briefs → fenced blocks above bimaps.md
tables, verbatim from oracle) → jam_Text.h (1 block + `comment` column, 47 cells) →
jam_Files.h (2 + 84) → jam_Chars.h (3 + 156; wchar template already carries inline
marker at :53) → jam_Generated.h (1 + 93; `sharedInstance`/`include` template blocks
gain inline `:::comment:::`; cells on the output-index rows) → jam_Identifiers.h
(1 + 252) → jam_Mermaid.h (76 blocks + 3 inline; nested-struct brief sourcing ruled
by ARCHITECT when reached — see Open Questions).
Each increment: template marker placement (if absent) + data authored verbatim from
oracle + ARCHITECT build/run + per-file diff gate. Inline-column additions to tables
follow the c4 `type`-column mechanics (column insert, grid re-widened, --format
canonical).

### Step 12: Fixpoint + Auditor + doxygen pass
Second `./cast` run = zero changes; second `--format` = zero changes. @Auditor sweeps
once (includes dead-code verdict on the comment transform family). Doxygen prose on
engine changes last, per Code Hygiene.

## BLESSED Alignment

- **E (Explicit):** engine stops inspecting a token's name to alter its value — no
  magic token semantics; data is what lands.
- **S (SSOT):** each doc text declared once, at its table; frames live with the text
  they frame; no engine copy of language syntax in the substitution path.
- **S (Stateless) / D:** substitution is pure replacement; removing the branch removes
  the only value-dependent formatting in the render path.
- **L (Lean/YAGNI):** net deletion in the engine; no new engine code, no new names
  (the `continuation` proposal is withdrawn).

## Risks / Open Questions

- **`@file` header block channel** — per-output-file doc has no data slot today; every
  file keeps a uniform ~5-line residual until ARCHITECT rules where it lives (e.g.
  comment cell on the output/index row, Writer-adjacent verbatim placement). Raised at
  the jam_HashMaps.h gate.
- **jam_Mermaid.h nested-struct briefs** — three nested shapes (Mermaid → C4 →
  Keyword) would all resolve shape-level `:::comment:::` to the same first source
  table's block (SPEC:247-249). Needs a ruling when that increment starts.
- **Comment transform family** (toComment/toCommentBlock/toBrief + comments.md) —
  retained for banner + `format` cells this sprint; Auditor flags if any is dead.
- cast's own tree (cast/cast + Source/generated) has the same doxygen backlog — out of
  this plan's scope (ARCHITECT: focus is jam).
