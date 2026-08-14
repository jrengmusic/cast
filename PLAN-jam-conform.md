# PLAN: jam/generated Oracle — Conform to Cast's Current Canon (closes [11])

**RFC:** none — objective from ARCHITECT ruling this session
**Date:** 2026-08-14
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + jam framework
**Status:** APPROVED — execution in progress

## Context

[11] "Protocol table SSOT" closes when `jam/diff` is byte-identical to a hand-authored,
compiling `jam/generated` — the same oracle/diff discipline cast already runs on itself
(`Source/generated/` = hand-verified oracle, `Source/diff/` = fresh self-gen output).
ARCHITECT's method: **write the oracle first** (Step 1), compile it standalone, and only
then derive exactly what `jam/cast/CAST.md` + `jam/cast/tables/*.md` must declare to
regenerate it. The cast engine itself is not the bottleneck — it already has every
template shape most of jam's outputs need; only jam's own manifest/data lags.

## Comprehensive file-by-file audit (read directly, not inferred)

12 files in `jam/generated/`, checked one by one against cast's own oracle pattern
(`cast/Source/generated/*.h`) and against the currently-correct jam templates
(`jam/cast/template/{Bimap,Struct,Namespace,HashMap,Identifiers,Text,Files,Generated}.cast`,
all `:::...:::` delimiter, all parametrized by a `list`/`type` column — none hardcode a
type name).

**Already correct shape AND complete — do not touch:**
- `jam_Bimaps.h` — every struct already `struct X : public jam::Bimap<X, juce::String, int>`,
  byte-identical pattern to cast's `Bimaps.h` (`TemplateTokenType`/`Rules`).
- `jam_Files.h` — `namespace files { inline const juce::String x {...}; }`, matches
  `Files.cast` exactly.
- `jam_Extensions.h` — `namespace extensions { inline const juce::String x {"x"}; }`,
  same flat-literal shape as `Identifiers.cast` output, already conformant.
- `jam_Identifiers.h` — `namespace Id { inline const juce::Identifier x {"x"}; }`
  (~1275 lines), matches `Identifiers.cast` exactly.
- `jam_Text.h` — `namespace text { namespace en { inline const juce::String x {...}; } }`,
  content matches `Text.cast`'s shape; only the namespace declaration syntax differs
  cosmetically from cast's single-line `namespace text::en { ... }` (C++17 nested
  namespace) — **trivial**, fold into Step 1 as a mechanical syntax normalization, not
  a re-author.

**Needs reshape (old engine shape → current canon), fold into existing templates:**
- `jam_Enums.h` — 21 structs (`HtmlBlockType`, `DEC`, `OSC`, `SGR`, `ColorMode`,
  `UnderlineStyle`, `CSI`, `WindowOps`, `DSR`, `TabClear`, `CursorShape`, `DECRQSS`,
  `CsiIntermediate`, `ESC`, `CharsetIntermediate`, `CharsetDesignator`,
  `DecEscIntermediate`, `DecEscFinal`, `ModeReport`, `ANSI`, `ShellIntegration`,
  `KeyboardAssignMode`), currently a bare `struct X { enum value : int {...}; };` with
  no runtime lookup. **Ratified**: reshape to `Bimap.cast`'s shape (same int-valued
  named-constant data, gains `get()`/`getDefault()` it didn't have) — same treatment as
  jam's own existing 60+ Bimaps. **File organization — ARCHITECT caution applied**:
  `jam_Bimaps.h` is already ~10,000 lines (`ColourNames` alone spans ~6,000 lines,
  lines 3202–9216) — a large-table domain already violating MANIFESTO L (300/file).
  The 21 terminal-protocol types do **not** merge into that file. `jam_Enums.h` stays
  its own dedicated `## output` row/target file — only the template changes (`code`
  and `file` are independent manifest columns; keeping the existing file name avoids
  inventing a new one under Rule -1). Delete `Enum.cast`, keep `jam_Enums.h` as the
  output target, reshaped in place.
  **Flagged, not fixed here**: `ColourNames`/`ColourIdMap`'s existing size is
  pre-existing debt, belongs to `[12]` MAJOR AUDIT (all jam files), not this plan.
- `jam_HashMaps.h` — `Entity`, `Diacritics`, `LanguageFamily`: currently
  `struct X { X() { table = {...}; } };` (custom container, not `jam::HashMap`).
  Root cause traced: `jam::HashMap` (jam_core, ankerl-fork) is single-value-per-key —
  fits fine — the *data* just has one real bug: `jam/cast/tables/html.md:165-166`
  correctly declares `aacute`/`Aacute` as distinct rows, but the current hand-built
  `jam_HashMaps.h` line 27 dropped the capital A, producing a duplicate key. That's an
  engine-emission artifact from the now-dead pre-`ER`/`TW`/`SM` engine, not a shape or
  data problem — once regenerated through the current `HashMap.cast` (verbatim
  `:::key:::`, no case transform), it's correct. Reshape to
  `inline const jam::HashMap<juce::String, juce::String> x {...};` free-variable form
  (matches cast's own `banner`), keep the three names, keep the `map::` namespace.
- `jam_Generated.h` — rewrite once the above lands: gains `jam::SharedInstance<map::X>`
  entries for the 21 folded Enums (Bimap-shaped types get `SharedInstance`, matching
  every existing jam Bimap and cast's `templateTokenType`/`rules`); loses nothing for
  `HashMaps` (cast's own `banner` HashMap is never `SharedInstance`-wrapped either —
  consistent, no change needed there); drops the `jam_Enums.h` include.

**Own shape, no cast precedent — keep the shape, migrate delimiter syntax only:**
- `jam_LookupTables.h` — `terminalColourSpace`, `cssCodePoints`, `markupOpen`,
  `markupClose`, `rawTextEnd`: `jam::LookupTable<int, Type, capacity>`, int-keyed,
  genuinely different container from Bimap/HashMap (byte/codepoint lookup, not
  string-keyed). Note: capacity expressions reference Bimap-typed enum members
  (`CssTokenType::closeBrace + 1`, `HtmlBlockType::cdata + 1`) — these survive the
  Enum→Bimap fold unchanged, since `Bimap.cast`'s nested `enum value : int {...}`
  preserves the same `X::member` access path the old plain-enum shape had.
- `jam_RomanNumerals.h` — `std::pair<const char*, int>[]`, unique shape, 13 rows,
  same treatment: keep shape, migrate `@...@` → `:::...:::` once its template exists.

## Doxygen is IN scope — template capability, not deferred authorship

`jam/cast/tables/terminal.md` already carries real, previously-authored `entry | value |
doc` tables (confirmed: lines 266, 284, 295, 316, 365, 374, 387... — real per-row prose,
e.g. `"Save cursor (DECSC variant — see file-doc's \"Collision resolutions\")"`), which
`Enum.cast` currently renders as `///< comment` per enum value, plus a struct-level
`/** @brief @brief@ */`. This is existing authored content, not something deferred to a
later doxygen pass — losing it on the Enum→Bimap reshape would be data loss, not a
scope cut. **`Bimap.cast` has no brief/doc emission today** (cast's own `Bimaps.h` has
none either, since cast's own doxygen pass hasn't run yet) — this plan must extend
`Bimap.cast` so the reshape preserves what already exists.

**Ratified design (ARCHITECT), supersedes the earlier `brief`/`doc`-column proposal:**
- **Per-row comment** — single-liner, stays a table column: `| entry | value | comment |`
  (renamed from the current `doc` column). Template placeholder `:::comment:::`, emits
  `///< text` per row, same convention as `:::key:::`/`:::value:::`.
- **Struct-level brief** — can be long, does NOT live in a table column. Lives in a
  fenced code block placed directly after the `## table name` heading, before the
  table itself:
  ````
  ## table name
  ```table name comment
  Long-form struct-level documentation goes here.
  ```
  | entry | value | comment |
  ```
  ````
  This is not a new engine capability — `jam::MarkdownDocument::getCodeBlock
  (const juce::Identifier&)` already exists and is already used exactly this way:
  `Source/Writer.h:130` reads `document.getCodeBlock (Id::banner)->getAllSubText()`
  for the banner fenced block. Same mechanism, keyed by an identifier built from the
  table name (e.g. `<table name> comment`) instead of a fixed `Id::banner`.
- Both are optional — tables without a `comment` column or a following fenced block
  render exactly as `Bimap.cast` does today. No new doxygen *authorship* starts here
  (still written last, per standing convention) — this only carries forward what
  `terminal.md` already has, through a table-column rename (`doc`→`comment`) and a
  fenced-block relocation for the struct-level brief, both mechanical.
- **Open engine question for Step 2**: `TemplateDocument::getCell` resolves values
  per-row from table columns; the struct-level brief is per-*table*, not per-row, so
  it cannot resolve the same way `:::comment:::` does. Needs a `Model` method (e.g.
  wrapping `getCodeBlock` with the `<table name> comment` naming convention) invoked
  once per dispatch row, analogous to how `banner` is already passed into
  `TemplateDocument::build (model, row, banner, code)` as a resolved string rather
  than a per-row placeholder. Exact wiring point is Step 2's job, not Step 1's —
  Step 1's oracle files hand-authored the `///<`/`@brief` text directly in C++, no
  table-driven resolution needed yet.

## Ratified: Operators shape (CORRECTED after build evidence)

`jam_Operators.h` (`XmlOperators`, `MarkdownOperators`, `CssOperators`, `HtmlOperators`,
`TerminalOperators`) is `struct X { static constexpr const char* const member {"v"}; };`
— no existing template produces this today. **First attempt (namespace-wrap reusing
`Identifiers.cast`'s `juce::String` body) was proven wrong by the ARCHITECT build**:
`jam_LookupTables.h` declares `inline constexpr jam::LookupTable<int, const char*, N>
markupOpen/markupClose/rawTextEnd`, populated from `Id::MarkdownOperators::*` —
`constexpr` requires a literal type, `juce::String` is not one, so this can never
compile for those 3 consumers regardless of any conversion fix. Also broke
`std::string_view` conversions in `jam_XML.h`/`jam_Css.h`/`jam_Html.h` (63+ call
sites total, swept by Pathfinder).

**Corrected, ARCHITECT-ratified**: keep the struct shape, keep the member type
`const char*` (zero call-site changes anywhere — `LookupTable`, `string_view`,
every existing consumer keeps compiling exactly as before), only make the C++17
implicit `inline` on `static constexpr` data members **explicit**:
```cpp
struct XmlOperators
{
    inline static constexpr const char* const declarationOpen { "<!" };
    ...
};
```
No new template needed for the *shape* (it's the original shape, explicit `inline`
added for clarity/robustness) — Step 2 still needs its own body template for this
shape since it doesn't match `Identifiers.cast`'s `juce::String` output, but the
oracle file itself required no reshape, only one keyword.

## Steps (testable, validated independently, nothing already-working touched)

### Step 1: Hand-author the oracle — `jam/generated/*.h`
**Scope:** `jam_Enums.h` (reshape to Bimap in place — no merge into `jam_Bimaps.h`),
`jam_HashMaps.h` (reshape + bug fix), `jam_LookupTables.h`/`jam_RomanNumerals.h`
(delimiter-only, no shape change), `jam_Text.h` (namespace syntax only),
`jam_Operators.h` (explicit `inline` on static constexpr members, shape unchanged —
see corrected ratification above), `jam_Generated.h`
(rewrite include list + SharedInstance set).
**Untouched:** `jam_Files.h`, `jam_Extensions.h`, `jam_Identifiers.h`, `jam_Bimaps.h`.
**Action:** Engineer hand-writes each target file in the exact shape cast's own oracle
proves compiles, preserving every existing struct/namespace/member name and value
(this is a reshape + one bug fix, not new content authorship — no new names).
**Validation:** ARCHITECT compiles `jam/generated/` standalone (no `cast` run needed —
this is hand-written, not yet generated). Compiler is the gate.

**Consumer sweep — corrected after first build (not zero-change as originally
assumed):** first build surfaced 2 categories of real consumer breakage, both fixed:
(1) `jam_Operators.h` — see corrected ratification above, reverted to zero-call-site-
change shape; (2) `jam_HashMaps.h`'s struct→free-variable reshape broke the 4 call
sites that did `map::Entity::get()`/`map::Diacritics::get()`/`map::LanguageFamily::get()`
— fixed to `map::entity`/`map::diacritics`/`map::languageFamily` (direct variable
reference, no accessor method needed once it's a value instead of a type) at:
`jam_web/html/jam_Html.h:250`, `jam_markdown/document/jam_MarkdownDocument.h:3577`,
`jam_markdown/layout/jam_MarkdownSyntax.h:256`, `jam_core/format/jam_Format.cpp:109`.
A literal artifact (`</content>` tag leaked into `jam_Operators.h:102` by an Engineer's
tool output) was also caught and fixed at this gate — proof the compiler catches what
structural review alone missed.

### Step 2: Derive `jam/cast/tables/*.md` + `jam/cast/CAST.md` from the oracle
**Scope:** `jam/cast/CAST.md` (`## index`/`## output`/`## output index`, replacing
`## generated`+`## patch`), the ~11 non-canon table files (rename `entry`→`name`,
resolve `@row@` to explicit `value`, drop/relocate the `format` column per what Step 1
actually needed — prose values stay as `value`, case-transform names move to the
template placeholder, matching `:::key:toCamel:::` already proven in `Bimap.cast`).
**Action:** For every struct/namespace that landed in Step 1, write the manifest row +
table data that reproduces it byte-for-byte through the existing templates. This is
mechanical once Step 1 is the fixed target — no guessing, diff against Step 1's oracle
at every row. Also extend `Bimap.cast` with `:::brief:::`/`:::doc:::` support per the
doxygen ratification above.
**Validation:** dry read — every `## output` row resolves through `## index`; zero
`entry`/`@row@`/`@...@` remaining in `jam/cast/tables/` or `jam/cast/template/`.

### Step 3: ARCHITECT gate — build, run, diff, fixpoint
**Scope:** none (verification).
**Action:** ARCHITECT runs `cast ./jam/cast/CAST.md`, diffs `jam/diff/` against Step 1's
`jam/generated/` oracle byte-for-byte, reruns until stable. Builds jam.
**Validation:** byte-identical fixpoint closes [11]. @Auditor sweeps once, then [12]
MAJOR AUDIT (all jam files) opens.

## BLESSED Alignment

- **B/S(SSOT)** — one oracle, one manifest grammar, one delimiter syntax, shared
  templates across cast and jam — no more fork between "what the template emits" and
  "what jam's manifest still asks for."
- **L** — `Enum.cast` dies entirely (fold into existing `Bimap.cast`); no net-new
  template.
- **E** — the `aacute`/`Aacute` corruption traced to its root (dead engine's emission
  path), fixed at the regeneration boundary, not patched in the generated file by hand.
- **D** — same template, same manifest grammar, both frameworks — the divergence
  (two incompatible calling conventions for one template set) was the non-determinism
  source this plan removes.

## Risks

1. Doxygen comments already authored in `terminal.md`'s `doc` columns (rendered today
   via `jam_Enums.h`'s `///<` per-value comments and `/** @brief @brief@ */`) must
   survive the Enum→Bimap reshape — see "Doxygen is IN scope" above. `jam_Chars.h`'s
   `///< U+00E6`-style comments are hand-authored directly in that file (not
   table-driven) and stay untouched, out of scope for this plan. New doxygen
   *authorship* beyond what already exists stays deferred to the later dedicated pass
   per standing convention — this plan only carries forward existing content.
2. `jam_Chars.h` carries two non-declarative extras — a `special` array and a static
   `isNumeric()` function — inside the otherwise flat `chars::` namespace. These are
   already correct and untouched by this plan; flagged only so Step 2's manifest
   derivation doesn't try to account for them as template output (they're jam-authored
   C++, not `.cast`-generated content).
3. `jam_Bimaps.h`'s existing 10,000-line size (`ColourNames`/`ColourIdMap`) is
   pre-existing debt, out of scope — flagged for `[12]` MAJOR AUDIT.
