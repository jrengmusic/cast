# PLAN: CAST Template Language — TemplateDocument Engine (TW rework)

**RFC:** none — objective from ARCHITECT prompt; TD-arc rejection rulings are ground of truth
**Date:** 2026-08-14
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + jam framework

## Context

The TD arc (TD2a–TD5) landed the language data and a working engine, then ARCHITECT
condemned three dishonest concepts in it:

1. **Emission-time resolution** — Writer's `emit`/`emitNode`/`getPlaceholder`/`getRegion`/
   `getList`/`emitSelected` family walks a document's internals node-by-node at write time,
   threading `injectedCode` and a `RowContext` manual boolean through every signature.
   /truth ruling (NAMES.md Rule 9, HELP.md): *documents build and write themselves* — an
   engine component that scans a document's internals to produce output asked the document
   instead of telling it.
2. **Marker assembly in build()** — `insideMarker`/`segments` lexing state leaked into tree
   construction. A marker is ONE lexical unit; classification belongs to `getToken`.
3. **Validator scanning with analogy vocabulary** — `collectJacks`/`isBijection` re-walk
   trees with foreign names. "Bijection" and "jacks" were analogy for understanding, never
   code; the codebase semantics are **placeholders, columns, parity**.

This plan replaces the condemned parts (TW0–TW4), keeps everything ratified and clean, then
gates at TD6. After the fixpoint, grand-plan **[11] completes as jam conformance** (JC arc).

## Ratified decisions (closed — PP-5)

Decisions 1–9 from the approved TD plan stand (symbolic emission; consume jam never
re-declare — `Id::tripleColon` jam_Identifiers.h:1182, marker length derived, never `3` or
`":::"`; greedy state-split run rule; one parametrized cutter per type; format ops project
declarations never references; jam canon spliced read-only; derivation not parameters;
row-0 default + capacity column; `## template` → `map::Template::classes`). New rulings:

10. **/truth — generation pipeline: documents build and write themselves** (NAMES.md Rule 9
    + HELP.md, amended). Model parses once (operational chain); each template file parses
    once into a shared immutable grammar tree; each output row **builds** its own output
    document — a state tree resolved from the grammar tree against the Model at build,
    never at emission; each output document **writes** its own target file. Build and write
    are the only operations. No emission vocabulary exists beside the codebase's own verbs.
11. **Marker is one lexical unit.** `getToken` consumes each whole marker into one
    classified token (HtmlDocument `getStartTag`/`getEndTag` pattern, jam_Html.h:51-119);
    name and transform are token properties. `build()` is a trivial classified-token loop
    plus an open-element stack — zero lexing state.
12. **CAST token family — language-qualified.** `map::Tokens` rejected as too generic (jam
    hosts Documents of multiple languages). Table `## template token type` →
    `map::TemplateTokenType`, joining jam's per-language sibling family
    (`map::DocumentTokenType`, `map::HtmlTokenType` — Rule 5 nearest-sibling precedence).
    Bimap + nested enum, ordinal from row position: `text = 0`, `placeholder = 1`,
    `regionOpen = 2`, `regionClose = 3`. The byte-class LUT keeps `map::Byte` — genuine
    byte semantics; token types are CAST's own domain.
13. **`## rules` re-points at CAST's own tokens** and regenerates as a Bimap (codebase
    pattern for named ordinal families), not a HashMap: begin →
    `map::TemplateTokenType::regionOpen`, end → `regionClose`.
14. **Validator parity locus** — `Validator::isPlaceholders`, consuming
    `TemplateDocument::getPlaceholders()` (public API, computed at parse, stored —
    documents are immutable after construction). Pipeline: parse → validate → build → write.
15. **State layer** — `build (const Model&, row)` on TemplateDocument returns a NEW
    resolved TemplateDocument; banner resolves INTO the tree at build; `toFile` is pure
    serialization (self-write). Writer dissolves to orchestration + `getBanner` data only.
16. **jam::Format to its fullest** — marker-interior split via the colon family
    (`getPreColon` jam_Format.h:403, `getPostColon` :408; `from` :443 / `upTo` :447);
    no hand-rolled substring logic anywhere in the engine. getPreColon/getPostColon
    themselves rewired onto the zero-copy `upTo`/`from` pair (jam_Format.cpp:412-420).
17. **Delimiter-string lexer — the byte-class grammar dies** (supersedes the
    `map::Template::classes` half of decision 9). tripleColon is the language's ONLY
    lexical structure: `isMarker` = `":::"` at position not followed by `':'` (the greedy
    rule as a two-condition check); marker close = leftmost `":::"`. The `## template`
    byte table, the `map::Template` LUT oracle, and the LUT cutter are deleted; the
    vocabulary is the all-text default (`{ map::Byte::text, {} }`). Precedent: base
    contract string-delimiter detection (`Cursor::startsWith` jam_Document.h:433-436,
    HtmlDocument commentClose compare jam_Html.h:68); parse loop verified indifferent
    (jam_Document.h:849-893 — cursor advances by getToken's returned length).
18. **Region helper vocabulary** — TW2 helpers ratified: `getCell` (placeholder cell +
    transform + key fallback), `getCodeText` (region three-way; the name follows Rule 7 —
    the yield is code text; `expansion`/`getExpansion` rejected), `isSelected` (emergent
    selection), `getOrCreate` (template cache relocated from Model — HtmlDocument
    precedent jam_Html.h:26-43), public `build (model, row, banner)` + `toFile`.

## The Language (IDL — unchanged)

| Source | Token (TemplateTokenType) | Node |
|---|---|---|
| literal text (incl. `::` runs) | `text` | Text leaf, `Id::text` property |
| `:::name:::` | `placeholder` | leaf, `id = name` |
| `:::name:transform:::` | `placeholder` | leaf, + `Id::transform` property |
| `:::name:begin:::` … `:::name:end:::` | `regionOpen` / `regionClose` | Region, `id = name`, children = body |

Greedy state-split run rule (decision 3) is preserved — it now lives entirely in `getToken`.

## Grammar Data (SSOT — cast/tables/template.md)

```
## template
| entry | class     |
| ----- | --------- |
|       | text      |   ← row 0 = default (stays as landed)
| colon | operators |

## template token type
| entry        |
| ------------ |
| text         |
| placeholder  |
| region open  |
| region close |

## rules
| entry | class        |
| ----- | ------------ |
| begin | region open  |
| end   | region close |
```

`## rules` cells now resolve against CAST's own token family; emission derives
`map::TemplateTokenType::regionOpen` etc. (decision 5 — byte-exact references, symbols
derived at the declaration site).

Generated oracle (hand-written bootstrap; self-gen target after the new engine lands).
Shape mirrors jam's generated Bimap family VERBATIM (`map::Byte`,
jam/generated/jam_Bimaps.h:620-661 — verified):

```cpp
// Source/generated/TemplateTokenType.h
namespace map
{
struct TemplateTokenType : public jam::Bimap<TemplateTokenType, juce::String, int>
{
    TemplateTokenType()
    {
        map = {
            { text, juce::String::fromUTF8 ("text") },
            { placeholder, juce::String::fromUTF8 ("placeholder") },
            { regionOpen, juce::String::fromUTF8 ("regionOpen") },
            { regionClose, juce::String::fromUTF8 ("regionClose") },
        };
    }

    enum value : int
    {
        text = 0,
        placeholder = 1,
        regionOpen = 2,
        regionClose = 3,
    };

    // getDefault / get statics byte-identical to the map::Byte block
};
}

// Source/generated/Rules.h — HashMap dies; same Bimap family shape. Entry cells are
// REFERENCES: begin/end derive Id::begin/Id::end, classes derive TemplateTokenType symbols.
namespace map
{
struct Rules : public jam::Bimap<Rules, juce::String, int>
{
    Rules()
    {
        map = {
            { map::TemplateTokenType::regionOpen, Id::begin.toString() },
            { map::TemplateTokenType::regionClose, Id::end.toString() },
        };
    }
    // no zero-keyed row → relies on the key-agnostic, empty-safe base getDefault
    // (jam_Bimap.h:188-192 — verified)
};
}
```

`Source/generated/LookupTable.h` DELETED (decision 17 — no byte classes exist); the
`## template` byte table and `cast/template/LookupTable.h` cutter die with it.
`Source/generated/Generated.h` include list: `TemplateTokenType.h` added before `Rules.h`
(Rules references its symbols; generated headers carry zero includes); `LookupTable.h`
include removed.

**Reference resolution (verified):** enum-row cells all resolve — `text`, `region open`,
`region close` in jam lexicon (jam lexicon.md:1086, :898, :897, spliced read-only);
`placeholder` in cast lexicon (:36); `rules` in cast lexicon (:42).
**Gated names ratified by this plan's approval:** heading entity `template token type`
(one new cast lexicon row, Identifier type — struct name is its toPascal projection);
struct symbols `TemplateTokenType`, `Rules`. `getPlaceholders` / `isPlaceholders` /
`build` / `toFile` — existing codebase verbs. Nothing else.

## Dependency & API Inventory

- `jam::Document` contract: `Cursor` (jam_Document.h:400-437), `Vocabulary` (:376-392),
  pure virtuals `getToken`/`build`/`getVocabulary` (:901-907), `Token` with properties
  array, `addChild` returns `Element*`.
- Whole-marker consumption precedent: `HtmlDocument::getToken` markup dispatch +
  `getStartTag`/`getEndTag`/`getDoctype` (jam_Html.h:26-137) — one token per construct,
  properties carry the interior; `build` = classified loop + `buildTree` stack.
- Two-scanner text/operator shape: `MarkdownDocument::getToken` (jam_MarkdownDocument.h:
  433-453), `getOperatorToken`/`getTextToken` (:~936-967) — kept verbatim in
  TemplateDocument for the run scanners feeding the marker consumer.
- `jam::Bimap` nested-enum family: `struct Byte : public jam::Bimap<Byte, juce::String,
  int>` (jam/generated/jam_Bimaps.h:620-661) — the oracle's exact shape. Sibling
  `*TokenType` family verified: `XmlTokenType` :154, `MarkdownTokenType` :520,
  `DocumentTokenType` :576, `SyntaxTokenType` :904, `CssTokenType` :2308,
  `HtmlTokenType` :2442 — `TemplateTokenType` joins this family (Rule 5).
- `Token::type` is the family slot: `int type { map::DocumentTokenType::text }`
  (jam_Document.h:65-67); derived documents assign their own family constants —
  `token.type = map::HtmlTokenType::startTag` (jam_Html.h:554), compare at :983;
  `map::MarkdownTokenType::imageOpen` (jam_MarkdownDocument.h:4358). TemplateDocument
  assigning `map::TemplateTokenType::*` is this exact precedent.
- `jam::Format`: colon family `getPreColon` :403 / `getPostColon` :408, `from` :443 /
  `upTo` :447; case family :291-337; `onlyExtensionFromFilename` :256.
- cast engine kept: `runJobs` (Model.h:6-15), `Model::parse` splice (:23-60),
  `Model::getTemplate` cache (:62-82), `Transforms::getTransforms()` (Operators.h),
  `Writer::toFile` row loop (Writer.h:16-40), `Writer::getBanner` (:192-206).

## Validation Gate

Each step validated by COUNSELOR against MANIFESTO.md, NAMES.md (Rule 9 as amended),
CODING.md, and this locked plan before the next. @Auditor runs ONCE, after TD6.
Standing constraints: agents never build, never run cast, never touch git; zero name
latitude; no doxygen/comment authorship (deferred to [12]).

## Steps

### TW0 — Language data revision + oracle reshape
**Scope:** `cast/tables/template.md`, `cast/tables/lexicon.md` (gated additions only),
`Source/generated/TemplateTokenType.h` (new), `Source/generated/Rules.h` (reshape),
`Source/generated/Generated.h`.
**Action:** add `## template token type` table; oracle Bimap files copied byte-exact from
the jam_Bimaps.h family pattern; Rules HashMap dies. No engine code.
**Validation:** oracle compiles; shapes byte-match the jam Bimap family; every cell a
byte-exact reference; no format ops on references.

### TW1 — TemplateDocument grammar layer (redo of TD2b)
**Scope:** `Source/TemplateDocument.h`.
**Action:** `getToken` consumes each WHOLE marker as one token classified by
`map::TemplateTokenType` (greedy state-split rule inside the scanner; interior split via
jam::Format colon family; name/transform as token properties; reserved words via the Rules
Bimap — zero literal comparison). `build()` = classified-token loop + open-element stack
(HtmlDocument shape); `insideMarker`/`segments`/`regionStack` state machine dies.
`getPlaceholders()` computed at parse, stored, exposed as public API. Keep verbatim: parse
factory, vocabulary static, two-scanner dispatch, `addText`, token array member.
**Validation:** parses every cutter; tree matches IDL; no lexing state in build; no
`":::"`/`3` literals; no hand-rolled substring ops.

### TW2 — TemplateDocument state layer
**Scope:** `Source/TemplateDocument.h` (+ `Model.h` signature surface only if needed).
**Action:** public `build (const Model&, row)` returns a NEW resolved TemplateDocument:
placeholders → row cells (transforms via Transforms/jam::Format); regions resolve
three-way at build — cell bearing an extension → wrapper grammar spliced at its code slot;
cell naming a Model table → list replication (separator from `<column> lineBreak` cell
else `Id::lineBreak` cell; emergent selection — row skipped when a referenced column is
empty, `value` excepted; list-row resolution reads the source table's key column when a
cell is absent — a code path, never a boolean flag); else passthrough. Banner resolved
into the tree head at build. `toFile` = pure serialization self-write.
**Validation:** resolved document contains zero placeholders; no RowContext, no flags, no
emission-time Model access; immutability held (build returns new, mutates nothing).

### TW3 — Writer dissolves to orchestration
**Scope:** `Source/Writer.h`.
**Action:** delete-first — `RowContext`, `getCode`, `emit`, `emitNode`, `getPlaceholder`,
`getRegion`, `getList`, `emitSelected`, `hasPlaceholder`, `injectedCode` threading all die
(Writer.h:48-190). Remaining: `toFile` iterates `Id::output`/`Id::index` rows via
`runJobs` → `model.getTemplate (…)` grammar → `grammar.build (model, row)` →
`document.toFile (file)`; `getBanner` stays as the banner data feed into build.
**Validation:** Writer ≤ orchestration + banner; no tree walk, no string assembly, no
Model value-sniffing; grep clean for the dead names.

### TW4 — Validator parity (redo of TD5/[NO5])
**Scope:** `Source/Validator.h`.
**Action:** delete `collectJacks` (:401-435), `isBijection` (:437-476), their `isManifest`
wiring (:478-487). Add `isPlaceholders`: parity between
`TemplateDocument::getPlaceholders()` and the output row's columns, both directions,
reserved names excepted (first/key column, `file`, `* lineBreak`, `capacity`); wrapper and
separator templates validated per the same parity through their own placeholder sets.
**Validation:** a dead column and a dead placeholder are both caught; "bijection"/"jacks"
appear nowhere; predicates + hazard scan carry over untouched.

### TD6 — Bootstrap + self-gen double-fixpoint + Auditor (ARCHITECT gate)
Build → `cast cast/CAST.md` → `git diff Source/generated/` empty → rebuild → run again →
still empty → `cast --help` renders. @Auditor sweep after. ARCHITECT runs everything.

### [11] completion — jam conformance (grand plan, after TD6)
The new engine is the vehicle: jam tables migrate to canon shape (JC arc), jam
lexicon/topology conform, jam/generated converges byte-stable, ARCHITECT fixpoint,
consumers rewired. Grand-plan **[11] Protocol table SSOT** closes there. Planned in
detail as its own sprint once TD6 is green.

## BLESSED Alignment

- **Explicit/Deterministic:** classification is data (`TemplateTokenType`, Rules Bimap);
  build resolves everything once; write is byte-copy; no per-cell choice survives.
- **SSOT:** token family declared once in template.md; rules reference it; framework
  constants never re-declared; placeholders computed once at parse.
- **Stateless/Encapsulation:** documents build and write themselves — Writer tells, never
  asks; no manual booleans; no injected threading; lexing state confined to getToken.
- **Lean:** cursor discipline is the only authored algorithm; the emit family (~140 lines)
  and the validator scanners (~85 lines) die; YAGNI — four constructs, nothing more.

## Risks / Open Questions

- None blocking. Bimap family shape verified verbatim (jam_Bimaps.h:620-661); all
  reference cells resolve (jam + cast lexicons checked); the one new lexicon row is gated
  by this plan. jam's own `## Byte` table is pre-conformance (`entry|value|format`,
  `@row@`) — cast's token table uses the canon single-`entry` shape with row-position
  ordinals; that divergence converges in the [11] jam-conformance sprint, not here.

## Verification

ARCHITECT runs: build → `./…/cast ./cast/CAST.md` → `git diff Source/generated/` empty →
rebuild → rerun → still empty → `cast --help` renders. Then jam conformance sprint ([11]).
