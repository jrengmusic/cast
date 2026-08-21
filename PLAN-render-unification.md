# PLAN: Placeholder-wired render engine; converge jam/ byte-identical

**RFC:** none — objective and design ratified in ARCHITECT session 2026-08-21
**Date:** 2026-08-21 (revision 4 — supersedes revision 3 entirely)
**SPEC:** `Source/HELP.md` (rewritten this revision — the governing document)
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE / JAM (`jam::Document`, `jam::MarkdownDocument`, `jam::Strings`, `jam::Format`, `jam::Function::Map`)

## Context

cast generates jam's headers. Correctness = **byte-identical** to `jam/generated/` (oracle, 13 files), proven by `jam/diff/` vs `jam/generated/` + **fixpoint** (2nd run = zero changes). The engine, templates, and manifest are rewritten from scratch to the ratified design — not patched.

## The Design

**Four reserved columns. Zero reserved placeholder names. One join.**

```
## output:  | placeholder | structure | separator | file |
```

The four column names are the engine's entire vocabulary — plus the grammar itself: `:::name:::` marker, `@` sigil, `- key: value` bullet, `> ` depth. Every placeholder name (`line`, `list`, `files`, `instance`, `keyword`, …) is data.

Per expansion key, each column answers one question:

| Column | Question | Cell form |
|---|---|---|
| `placeholder` | **what data** | `- line: @lexicon:lexicon` (table ref → row iteration); `- files: file` (column name → unique values, self excluded — second order); `- line: instance` (binding name → rows carrying that binding — second order) |
| `structure` | **what shape** | bindings `- key: value`; wrap chain `@scope: Name` / `> @scope: Name`; `> ` = one wrap, one tab |
| `separator` | **what join** | `- line: @text:break:line`, `- list: @text:break:comma`; empty = newline; flat `@ref` = row-sibling join |
| `file` | **what target** | output alias |

`> ` depth is **uniform across placeholder, structure, and separator columns** — same depth, same scope.

**One mechanism.** Every expansion — vertical rows, horizontal columns, sibling rows sharing a file — is build-items-then-join:

```cpp
auto line { items.joinIntoString (separator) };            // jam::Strings — jam_Strings.h:1006
scope = jam::Format::replaceholder (scope, name, line);    // jam_Format.h:438 — default delimiter Id::tripleColon
```

`jam::Format::hasPlaceholder` (jam_Format.h:443) is the existence predicate — matched-replace and parity checks use it; no hand-rolled `contains(":::")` scanning anywhere.

## Locked Decisions (ARCHITECT-ratified, this session)

| # | Decision |
|---|----------|
| 1 | Reserved columns: `placeholder \| structure \| separator \| file` — exactly these four; column name `placeholder` (not `list`, not `token`) |
| 2 | Placeholder names are data — engine hardcodes none; axis is decided by the wiring form, never the name |
| 3 | `> ` depth is the **raw `>` count** — no `>` = 0 tabs, `>` = 1 tab, `> >` = 2 tabs — uniformly across the three keyed columns. No headless-blockquote fold: a `>` block's bullets belong to its raw depth whether or not a `@scope:` head is present. Wiring, shape binding, and separator entry match by identical raw depth. Data misaligned with this (Chars row `> - line:` vs depth-0 wiring) is corrected in Step 3 |
| 4 | **ALL constructs use Scope/Definition/Specialised** — universal decomposition, no monolithic wraps. `Scope.cast` + `Definition.cast` are universal; each construct's irregular fixed text is its own Specialised fragment (`Methods.cast` for bimap methods, `Chars.cast` for Chars, `LookupTable.cast` reduced to only its irregular part). No parametrized method text, no code in table cells. Single-line fragments may live as `## index` backtick literals (`@include`, `@sharedInstance`, `@inPlace`) — established pattern |
| 5 | **First-row law** — map-type default = first source row, always; no `- default:` binding exists; `getDefault()` emits row 0's ordinal. Generalized engine-side: an unbound wrap placeholder naming a column of the row's wired source table resolves from that table's row 0 (fills LookupTable's `:::value:::`) |
| 6 | text.md: `## break` (line/comma/semicolon) + `## namespace` (prologue/epilogue banner literals — replaces `## banner`; all `@text:banner:*` refs become `@text:namespace:*`) |
| 7 | `## output` mandatory; `## output index` optional, second-order, source = `## output` itself. Includes = unique `file` values (authored order, self excluded). Master-index need is per-language — data, not engine |
| 8 | SharedInstance member selection = presence of `- instance:` binding on the row (already in data: `- instance: shared`, jam CAST.md:367 ff.). No new column — the earlier "instance column" idea is retracted; the binding-presence mechanism the old engine proved (TemplateDocument.h:108-109) is the contract |
| 9 | `jam_MermaidGeometry.h`: delete its output rows. **Unification (ARCHITECT): no jam_Enums — EVERYTHING is jam_Bimaps.** One model, one construct, ONE HEADER, one table shape: the 22 jam_Enums structs appended as a block into jam_Bimaps.h, jam_Enums.h deleted, jam_Generated.h include dropped. Six policy Bimaps ratified after NodeShape: ConstructionPolicy, HitTestClassification, SizeAdjustPolicy, MindmapSizePolicy, BlockSizePolicy, OverlayType (`none` = 0-row spelling; `default` never appears). Convergence target = the oracle's **12 files** |
| 10 | Bimap decomposes: Scope (struct) + two independently-named expansions of the same source (`entry` → map-init fragment, `value` → Definition with `- open: =`) + fragment for fixed method block. Two expansions of one source = two placeholder names — legal because names are data |
| 11 | HashMap: Scope + Definition; `:::list:::` = row columns joined by `- list:` separator; type aliases backtick-wrapped in index (`<`/`>` hazard) |
| 12 | Include.cast / Instance.cast / HashMap.cast die as engine concepts — `files` and `instance` are ordinary data-wired expansions |
| 13 | Engine rewrite from scratch, delete-first. Design by Contract. NO hand-rolled methods. get/set/is/has/apply verb semantics per NAMES.md — no foreign semantics. Framework API to its fullest extent: `jam::Strings::joinIntoString`, `jam::Format` transforms + `replaceholder`/`hasPlaceholder`, `jam::MarkdownDocument::parse` (only parser), `jam::Function::Map` dispatch |

## Engine Rewrite

### Class contract (ARCHITECT-pinned — no delegation without it)

| Class | Inherits | Contract |
|---|---|---|
| `TemplateDocument` | **`jam::MarkdownDocument`** | Templates parse through the framework parser — no custom tokenizer, no stub Vocabulary, no hand scanner. `:::name:transform:::` resolution operates on `Document::Element` trees. The framework pair pattern (MarkdownDocument overrides getVocabulary/getToken/build, jam_MarkdownDocument.h:436-471) is the model |
| `Writer` | **`jam::Document::Writer`** | Mirror of `MarkdownWriter : Document::Writer` (jam_MarkdownWriter.h:7-11): one `getText (const Document&)` override serializing the built output document; `toFile` INHERITED from the framework — no hand-rolled `replaceWithText` emission |
| `Model` | `jam::MarkdownDocument` (as-is) | Master state document; reserved-column accessors read `Id::placeholder` |
| `Validator` | `jam::MarkdownValidator` (as-is) | Reserved set = the four columns; parity + dead-wiring/dead-placeholder FATALs per SPEC |

Design by Contract throughout: build constructs `Document::Element` state trees against the Model; the document writes itself (`Document::Writer::toFile`, jam_Document.h:501) — never hand-rolled emission. Substitution: `jam::Format::replaceholder`/`hasPlaceholder` (jam_Format.h:438/443). Join: `jam::Strings::joinIntoString` (jam_Strings.h:1006). No other mechanism.

**The no-scanner inversion.** The engine never enumerates `:::` markers. Candidate placeholder names come from DATA — wiring keys, binding keys, source columns, transform-tagged variants over the closed Transforms map. `hasPlaceholder` tests each candidate, `replaceholder` fills it, and after all candidates apply, a single `contains (Id::tripleColon)` residue check is the FATAL dead-placeholder gate. Templates parse through `MarkdownDocument::parse` — zero custom tokenization anywhere in cast.

### Build algorithm (per output row — matched-replace only)

1. Resolve the row's templates/fragments from structure bindings (alias → grammar tree).
2. For each placeholder `:::name:::` at current depth:
   - wiring in **placeholder column** at this depth? → expansion:
     - table ref → for each source row: build through the structure's same-named shape binding → `joinIntoString` (separator column's same-named entry, default newline)
     - column name (second order) → unique column values, authored order, self excluded → build each through shape → join
     - binding name (second order) → rows carrying that binding → build each through shape → join
   - else **structure binding** at this depth? → resolve value (sigil law; interior placeholders resolve against same scope) → replace
   - else **source-row cell**? → cell resolution (+ transform tag) → replace
   - else → FATAL dead placeholder
3. `:::list:::`-form horizontal fill: current row's non-key columns joined by the separator's same-named entry.
4. Wrap chain inward→outward: each `> ` level builds its wrap template through matched-replace; the placeholder matching one of the row's expansion keys receives the assembled inner content (intersection of wrap-template placeholders × row expansion keys must be exactly one — FATAL otherwise); content indented one tab per depth, frame never indented.
5. Empty resolution → elide placeholder + one preceding whitespace (existing rule, kept).

Every step is get-semantics (`getBinding`, `getSource`, `getSeparator`, `getContent`) returning values; the caller replaces. No method both computes and stores. Preconditions assert once at the owner; downstream trusts (MANIFESTO D).

### Deleted with the rewrite

- All hardcoded token names: `Id::body` (Writer.h:89-122), `Id::line` prefix-match (TemplateDocument.h:705, Writer.h:155), `Id::list` as column (renamed)
- The three parallel resolution paths (`getCell` / `getText` ×2 / `getExpansion`, TemplateDocument.h:573/649/690/77) — one matched-replace sequence remains
- Old `getExpansion`'s inline column-vs-binding branching (TemplateDocument.h:91-122) — reborn as the second-order wiring forms, driven by the placeholder column

## Data Migration (jam/cast/)

- `CAST.md ## output`: rename column `list` → `placeholder`; move source refs into wiring form (`- line: @alias:table`, depth-scoped `> - line:` where the expansion is depth-1); delete `jam_MermaidGeometry` rows; `@text:banner:*` → `@text:namespace:*`; **fix the 6 inverted operator rows** (CAST.md:259 ff. use legacy outward quotes — deepest `> >` as namespace; migrate to inward form: depth 0 = outermost namespace, depth 1 = struct, matching the Identifiers/Text/Chars/Extensions rows); align Chars row's wiring depth (`> - line:`)
- `CAST.md ## output index`: 4-column form; `- files: file` + `- line: instance` wiring (Cookbook 6 in SPEC is the exact target)
- `tables/text.md`: `## banner` → `## namespace` (ARCHITECT's exact literals); `## break` already correct (text.md:22-30)
- `tables/mermaid.md`: dead census tables deleted (`## MermaidGeometry`, `## mermaidShapeVertices`, `## mermaidShapeCornerRadii`) — mermaid metrics/font/style defaults have their SSOT in resources/mermaid.css; CAST-driven CSS is future work authored only when needed (YAGNI)
- **Optimistic-deterministic ruling (ARCHITECT):** no fallback vocabulary or concept anywhere; no bare generated constants; LUT default value = FIRST ROW, like every map; membership never inferred by comparing against a default value — the caller establishes key-domain validity once at the owner. Oracle jam_MermaidTables.h amended accordingly (constants deleted, prose blocks trimmed); jam_MermaidDiagram call sites redesigned; policy vocabularies become declared tables, cells carry symbols only, pack ordinals derive. Cast's remaining mermaid constructs converge against the amended oracle
- `## index`: fragment aliases point to `.cast` files (Bimap body/methods, Chars, LookupTable wrapper); single-line fragments as backtick literals (`@include` added; `@sharedInstance`, `@inPlace` exist)
- Templates: `Scope.cast`, `Definition.cast` unchanged (already correct shape); author fragment `.cast` files; delete obsolete Bimap.cast/LookupTable.cast content in favor of fragment decomposition per decisions 10-11

## Steps

### Step 1: SPEC ✅ (this revision)
`Source/HELP.md` rewritten — governing document for everything below.

### Step 2: Engine rewrite
Full teardown against the pinned class contract. **Nothing from the old engine survives except `runJobs`** (the parallel job helper, Model.h:6-16). `TemplateDocument.h` + `Writer.h` written new; `Model.h`/`Validator.h` re-cut to match. No hand-rolled method where the framework API provides one — the API is read thoroughly first and is the authority.

### Step 3: Data migration
jam/cast/CAST.md + text.md + index + fragments per Data Migration above.

### Step 4: Converge
ARCHITECT builds + runs `./cast jam/cast/CAST.md`. `cmp -s jam/diff/<f> jam/generated/<f>` per file — 13 files, iterate residuals to byte-identical.

### Step 5: Fixpoint
2nd `./cast` = zero changes.

### Step 6: Auditor sweep
Full-sprint audit vs MANIFESTO/NAMES/CODING + this plan's locked decisions.

## Verification

Per step: ARCHITECT builds + tests — agents never build unrequested. Gate: all 13 files byte-identical AND fixpoint. Engine gate: zero hardcoded placeholder names (grep for `Id::line`, `Id::list`, `Id::body` in engine sources returns nothing outside the four reserved columns).
