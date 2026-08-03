# PLAN: jam_Markdown Document Isomorphism — tree SSOT, canon Descriptor IR, dual-surface rendering

**RFC:** none — objective from ARCHITECT prompts (this session, ratified point-by-point)
**Date:** 2026-08-03
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE — LANGUAGE.md reference implementation; single-header preferred; 30/3 unchanged; 300 as responsibility smell only

## Context

The endless-cycle root cause was structure-as-offsets: the parser flattened inline structure into `Id::runs`/`Id::links` var tuples, every consumer re-derived it, and the layout→render chain shoved content through Descriptor var properties. Steps 1-2 fixed the parse product. This update records the ratified rendering architecture that completes the fix — discovered and ruled in-session while executing Step 3.

## THE CORRECT COMPREHENSIVE UNDERSTANDING (ratified)

### 1. Two products, cleanly split
- **Parse product** = the Document tree. Semantic SSOT: content + inline style structure (em/strong/del/code/a + untagged text child nodes). One engine (`Document::parse`/`getOrCreate`), markdown is the third Vocabulary+Rules supplier beside Html/Css. **DONE (Step 2).**
- **Layout product** = Descriptors. **Descriptor is canon** (jam_Descriptor.h): NVS properties + bounds, target-agnostic metadata IR. It carries METADATA ONLY — kind, geometry, metrics, style hints. Content never rides it (the `Id::text`/`Id::tokens` var-content channel dies).

### 2. The plugin_bootstrap isomorphism is the canon pattern
Tree (semantic SSOT) + Descriptor (metadata) + a builder per target producing that target's concrete objects (ViewManager → juce::Component; markdown gui → paint objects; markdown tui → cells). Builders read content from the TREE at build time (jam_ViewManager.cpp:17-59 walks the tree, consults metadata per node). The markdown widget is SINGULAR — one Component composing plural graphics objects in one paint()/draw().

### 3. One styled-text carrier, two surface-native layout engines
- **Carrier:** `jam::AttributedString` (jam_AttributedString.h) — already semantic: `Link{range,url}`, `SyntaxToken{range,type}`, strikethrough ranges as typed arrays; bold/italic readable from per-range `juce::Font` (`isBold()`/`isItalic()` — no inversion). Gains **`addCodeSpan`/`getCodeSpans`** (ratified) following the addStrikethrough family shape — closes the last sub-semantic gap. Built ONCE per block by the shared tree walk **`addStyledText`** (exists, jam_MarkdownLayout.cpp).
- **gui:** `juce::TextLayout` consumes the carrier → pixel glyphs → `juce::Graphics`. Juce-native surface contract; the Vulkan LLGC + shared GlyphAtlas do the heavy lifting behind it (jam_ClapServices.h:29-31). No custom pipeline at the surface — the OpenGL-era GlyphAtlas shortcut is obsolete for gui text.
- **tui:** NEW **`tui::TextLayout`** (ratified; jam_tui) — the cell-domain mirror of juce::TextLayout, exactly as tui::Graphics/tui::Component mirror juce. Consumes the same carrier, wraps at codepoint cell widths, emits stamped cell rows: Font.isBold/isItalic + code-span ranges → `Stamp` flags (interned styleId), links → `Hyperlink` ids (interned, per-cell `Char::withHyperlinkId`).
- Divergence exists only inside each projection; the model never forks. A terminal is cells — cells are its native contract, not old plumbing.

### 4. What dies
1. **GraphicDescriptor** → plain `juce::NamedValueSet` (near-duplicate metadata-bag type; NVS is juce-canonical, Component-native, typed). Markdown's `defaultMetrics` migrates NOW; the type itself survives until the mermaid sweep (ARCHITECT-commanded timing).
2. **Descriptor var-content channel** — `Id::text`/`Id::tokens` vars in properties.
3. **`drawTextRow`'s rebuild** — vars → AttributedString → TextLayout per draw (jam_MarkdownDocument.cpp:108-126).
4. **Per-row text descriptors** — wrap lives where each engine owns it (juce::TextLayout internally for gui; tui::TextLayout for cells). Text blocks get ONE descriptor (block bounds) + one carrier.
5. **TextLine outside the editor domain** — its own header scopes it to the text editor's document model (jam_TextLine.h:5-8); its `fromText` proportional mode fed only the GlyphArrangement shortcut (jam_TextLine.h:80-86). Exits the markdown/mermaid IR.
6. `styledText`-persistence question, TextRow, GraphicBlock, span-tuple encodings — all dissolved; nothing new is invented beyond the two ratified names.

### 5. Scope rulings
- **Mermaid: on ARCHITECT's command, not this sprint.** Markdown proves the architecture end-to-end (gui+tui+cast). GraphicDescriptor stays alive for mermaid until then.
- Heading `Id::id` = raw heading text (anchor key; conforms to plan wording).
- `Id::tokens` code-highlight flow: SyntaxToken ranges ride the carrier (`addSyntaxToken` exists).

## Dependency & API Inventory (verified file:line this session)
- Engine: jam_Document.h — Vocabulary :68-76, Rules :82-104, getTokens :235-314, parse :338-390, getOrCreate :409/:571-589; queries getChildByName :433, getChildByID :453, getAllSubText :471 (new), getTableValue :484 (new).
- Producer: jam_Markdown.h — supplier seams :12-27; buildInlineTree :3246; addInlines :3443; addLeafText :3473 (bottom-up); inlineConstruction Function::Map.
- Carrier: jam_AttributedString.h :9-54; shared walk addStyledText (jam_MarkdownLayout.cpp, static, ratified).
- IR: jam_Descriptor.h :19-23 (NVS + bounds, canon); GraphicDescriptor (jam_GraphicDescriptor.h:20, dying).
- tui: tui::Graphics (jam_tui_graphics.h:29), tui::Component (jam_tui_component.h:25), Stamp (jam_Stamp.h:68, flags incl. code bit :179), Hyperlink (jam_Hyperlink.h:45, per-cell via Char::withHyperlinkId :41-43), codepoint-width wiring (R9 sprint).
- gui backend: Vulkan LLGC factory + GlyphAtlas shared tables (jam_ClapServices.h:29-31,42-47).

## State: completed this sprint
- Step 1 Vocabulary: strong/code/href added; pyDel→del, pageA→a; authored tables reconciled + deduped (## tag section; membership prose convention); mermaid direction rows deleted (uppercase-derivation rule recorded); td collision resolved. Audited.
- Step 2 Producer: tree product (element nodes + untagged text children everywhere, incl. codeBlock/mermaid/htmlBlock payload as text child; image description as inlines); runs/links/flattenCell dead; addLeafText bottom-up; getAllSubText/getTableValue on Document (findTable capture fixed). Audited.
- Step 3 partial: module consumers on tree walk; addStyledText shared static; sliceTokens rename; zero Id::runs/Id::links module-wide. Gui draw styling gap OPEN — resolved by this architecture.

## Validation Gate
Each step Auditor-gated against MANIFESTO.md, NAMES.md, JRENG-CODING-STANDARD.md, and this locked plan. Agents never build (ARCHITECT compiles), never run git. Engineer prompts restate: no doxygen, no plan/sprint-citing comments, zero identifier latitude.

## Steps

### Step 3R: gui completion — carrier + canon Descriptor purge (jam_graphics, jam_markdown)
**Scope:** jam_AttributedString.h; jam_MarkdownLayout.cpp/.h; layout/jam_MarkdownTable.cpp; document/jam_MarkdownDocument.cpp/.h; document/jam_MarkdownTable.cpp
**Action:**
- `jam::AttributedString` gains `addCodeSpan (juce::Range<int>)` / `getCodeSpans()` — exact addStrikethrough family shape. `addStyledText` records code spans via it (fonts stay as-is for gui measure).
- MarkdownLayout: text blocks (paragraph/heading/cells/codeBlock/htmlBlock) emit ONE descriptor each — bounds = block box, NVS metadata only (type, alignment, metrics; ZERO `Id::text`/`Id::tokens` vars) — plus one built `jam::AttributedString` (code blocks carry SyntaxToken ranges via addSyntaxToken). MarkdownDocument owns the built carriers; descriptor↔carrier pairing via an index property under an EXISTING Id word (interning-reference idiom, Stamp/Hyperlink precedent) — Engineer verifies word availability, STOPs if none fits. Non-text descriptors (thematicBreak/blockquote/table grid) unchanged.
- `defaultMetrics`: GraphicDescriptor → `juce::NamedValueSet` (getWithDefault/operator[]); markdown module has zero GraphicDescriptor references after.
- draw/toSvgString: per text block — carrier → `juce::TextLayout` (block width) → draw at block origin; strikethrough/link/code decoration from carrier ranges via TextLayout glyph geometry (drawStrikethroughs precedent); svg emits per-line tspans from TextLayout lines. `drawTextRow` var-rebuild and `getAttributedString (text, tokens, font)` reconstruction die.
**Validation:** zero `Id::text`/`Id::tokens` reads on Descriptor properties in jam_markdown; zero GraphicDescriptor references in jam_markdown; styling (bold/italic/code/strike/link + syntax colours) restored at draw/svg; one carrier per text block; no parallel-array shapes (index-property pairing only); coding standard.

### Step 4R: tui::TextLayout + jam_tui_markdown
**Scope:** NEW jam_tui/ansi/jam_tui_textlayout.h (single header, module include at jam_tui.h topmost); jam_tui/markdown/jam_tui_markdown.cpp
**Action:**
- `tui::TextLayout`: consumes `const jam::AttributedString&` + cell width → wrapped rows of stamped `Char` cells. Codepoint walk with existing width-class wiring; per-range: Font.isBold/isItalic → Stamp::bold/italic, getCodeSpans → Stamp::code, getStrikethroughs → Stamp::strike, getLinks → Hyperlink intern → `Char::withHyperlinkId`; colours from the tui colour-id resolution already in jam_tui_markdown (LookAndFeel → Stamp fg). API mirrors juce::TextLayout's shape (createLayout + line access) at the cell domain.
- jam_tui_markdown: toAnsiString consumes MarkdownDocument descriptors + carriers → tui::TextLayout rows → tui::Graphics/ANSI. All offset arithmetic and getStyleFlags/getStyledLine descendants die; TextLine::fromText proportional usage in the markdown path dies.
**Validation:** tui renders bold/italic/code/strike/links (OSC 8) correctly from the same carriers gui consumes; zero re-derivation; ANSI fixtures re-baselined; family naming (tui::TextLayout mirrors juce) verified.

### Step 5: cast collapse (unchanged intent)
**Scope:** cast Source/
**Action:** `rm Source/Cells.h`; getCellHazard/findColumnIndex/extractSections and every process*/output-arg/index-loop shape die by name; engine = select→project→join exclusively via Document queries (`getChildByID`, `getTableValue`, `getAllSubText`) + `Format::replaceholder`; hazard from structure (cell has `a`/html child outside `code` → FATAL; text payload `containsAnyOf ("<>")`); single headers per LANGUAGE.md.
**Validation:** zero cursor/offset/reparse/manual-parse in cast; fixtures pass; fixpoint holds.

### Step 5b: cast self-validation — own generated table, zero magic strings (ratified rulings, this session)
**Scope:** cast repo only; jam read-only; KANJUT untouched.
**Rulings (ARCHITECT, verbatim intent):**
- All identifiers global `namespace Id` — always, project and modules alike. No new pattern. Cast's words extend the same global `Id::` (zero overlap with jam's lexicon, verified).
- `generated/` is UNTRACKED (gitignored). Only the tables (+ templates + CAST.md) are the tracked SSOT. Byte-diff vs the hand-written target is a transient bootstrap check this sprint; fixpoint is the durable gate.
- `Id::cast` → `"CAST.md"` (word≠string row, applicationSupport precedent).
- Templates are opaque `.h`-shaped text with `@hole@` (SPEC §3) — never markdown. Layout is A1: `cast/tables/`, `cast/template/`, `cast/CAST.md` — three siblings.
**Action (target first, in order):**
1. Hand-write target `generated/Identifiers.h`: jam generated shape exactly (7-line art banner from jam/generated/Identifiers.h:1-7, `#pragma once`, global `namespace Id`, `inline const juce::Identifier` rows, blank line between groups). Words: sections outputs/dispatch/transforms/constraints; predicates matches/unique/existsIn/oneOf/range/parity/fileExists/onePerGroup; transforms toUpper/toTitle/toKebab/escapeCpp/utf8Bytes/codepointHex/codepointLabel/qualifySymbol/symbolFromFile; plus `cast|CAST.md`.
2. Author `cast/tables/cast.md` — vocabulary SSOT: `## section`, `## predicate`, `## transform`, `## file` (holds `| cast | CAST.md |`); |word|string| rows, markdown.md `## tag` membership convention.
3. Author `cast/template/` (root + fragment per output, extracted from generated/Identifiers.h and generated/Banner.h) + `cast/CAST.md` (outputs: both headers; dispatch: fragment per row shape → slot; transforms: none; constraints: unique on word columns).
4. Source sweep — zero magic strings: vocabulary literals → `Id::` (`.toString()` where String keys needed); Cast.cpp "CAST.md" → `Id::cast.toString()`, "--version" from existing `Id::version` (jam:941); cast::toKebab/cast::toTitle bodies delegate to `jam::Format::toKebab`/`toTitleCase` (verify semantics, STOP on divergence); indexOf/substring splits → `upToFirstOccurrenceOf`/`fromFirstOccurrenceOf`; manual U+ hex + char-buf UTF-8 → juce API (`getHexValue32`, `charToString().toRawUTF8()`); file-handle Strings → `juce::File` (RelationTables::sources, configureDepends); index++ → range-for except where the index is the reported row number or a positional column read.
5. `.gitignore`: `generated/`.
**Validation:** cast runs its own `cast/CAST.md` → regenerated bytes match the hand-written targets (transient), fixpoint holds (durable); grep gates: zero vocabulary literals in Source/ outside generated/, zero indexOf-split parsing, zero manual hex/UTF-8, remaining indexed loops each justified.

### Step 5c: engine rewrite — jam::Document IS the data model (ratified, supersedes Step 5/5b engine internals; table/template/vocabulary products stand)

**Rulings (ARCHITECT, verbatim intent):**
- jam::Document is THE data model. A new struct, container, or data structure in the engine = failure.
- No circus — no temporary arrays/maps built from the Document to re-iterate and mutate. The Document is the IR SSOT; the engine QUERIES it at use.
- Table queries: internals are direct element lookups — never string manipulation, never char reads, never substring.
- No int rows/cols — rows address by column-0 KEY (SPEC §2), columns by header name. All Identifier-addressed.

**Step 5c-1 — jam::Document table query family (jam_core/document/jam_Document.h):**
- `getTableRowKeys (tableId)` → StringArray — column-0 cells of data rows, authored order; key position = SPEC §8 row ordinal.
- `getTableHeaders (tableId)` → StringArray — header cells, authored order.
- `getTableValue (tableId, colId, rowId)` — exists (:484), unchanged, the one cell accessor.
- `getTableValues (tableId, colId, rowId)` → StringArray — the cell's Id::code children texts in order (list cell); no code children → whole cell text as one element.
- `getTableCell (tableId, colId, rowId)` → const Document* — the cell element (XmlElement::getChildElement precedent; hazard inspection consumes it).
- No getNum*, no int overloads. One shared private structural resolver (factored from getTableValue's anchor walk).
- Validation: internals zero substring/indexOf/char arithmetic; no doxygen (sprint-end discipline).

**Step 5c-2 — cast engine delete-first rewrite (cast Source/):**
- `rm Source/Manifest.h Source/Relations.h` — dissolved. Driver.h + Constraints.h rewritten fresh from this model; old bytes never read back. Transforms.h, Template.h, Help.h survive.
- OutputEntry, DispatchEntry, TransformEntry, ConstraintEntry, PredicateContext, RelationTables, Manifest struct, getRelationTables, getCellText, getColumnIndex, getInputTables — ALL DEAD BY NAME.
- Driver::run: parse manifest once; range-for over `getTableRowKeys (Id::outputs)`; cells via keyed `getTableValue`; tables list via `getTableValues` (outputs cell re-authored as code-span list — `fromTokens` dies); per output: roots parsed into ONE transient local `jam::Array<jam::Document>` (the only permitted container — owns the parse products, scoped to one output); table→root lookup = `getChildByID (tableName)` query over roots, source file = the path in hand at the same loop level; hazard per root (applyFunctionRecursively + getTableCell); constraints; dispatch range-for over `getTableRowKeys (Id::dispatch)`; projection = range-for `getTableHeaders` × keyed `getTableValue` → SubstitutionMap (existing alias, transient) → replaceholder → validateHoles → write-if-different. Manifest validation (template/fragment existence, transform names, unmapped fragment) = manifest queries before generation.
- Constraints: predicates = free functions, explicit args via Function::Map (no context struct); enumeration predicates range-for over getTableRowKeys; arg grammar splits (existsIn t.c, range a b) via upTo/fromFirstOccurrenceOf — SPEC §7 authored DSL, the one sanctioned split.
- configureDepends: queried from the manifest Document at print time in Cast.cpp — no Driver member (Stateless).
- `specification | SPEC.md` row → cast/tables/cast.md `## file` + generated/Identifiers.h target; Cast.cpp reads `Id::specification`.
- Validation (structural absence, grep-verified): zero `struct` in Driver.h/Constraints.h; zero `Id::children` in cast Source/; zero `for (int` in engine paths; zero `fromTokens`; zero numeric column ordinals; zero out-ref params; CONTRACT sweep.

**Step 5c-3 — self-validation:** ARCHITECT builds jam + cast; cast regenerates its own generated/ (untracked), fixpoint holds; Auditor sweep on the 5c gates + SPEC fidelity; then Step 6.

**Risks:** new query names ratified by plan approval (Rule -1); duplicate/empty col-0 keys resolve first match — key uniqueness is author's contract (SPEC §2); KANJUT jam_Document divergence widens (HANDOFF).

### Step 5d: parser shadow-struct teardown + tui fold (ARCHITECT contract ruling)

**Ruling:** No shadow copy allowed — the Document IS the data structure, in the parser too. Not optional.
- jam_Markdown.h "product structs" die: ListMarker, AtxHeading, ListIndent, TaskListMarker, SoleImage, ReferenceToken, FenceInfo, Continuation, LinkSpan, LinkToken, OpenLeaf, Containers. Element-shaped bundles → detectors build/annotate the Document element directly (properties + children, add*/build* verbs). Containers (four parallel Array<int> shadowing the open-block stack) dissolves — open-block state IS the partially built tree; walk its open tail. Scan-state residue (consumed lengths, indents) survives only as plain locals/ints at call sites, never bundled types.
- jam_tui_markdown.cpp folds into its header (LANGUAGE.md single-header default).
**Validation:** zero struct definitions in jam_Markdown.h parser internals; zero parallel arrays; parse product byte-identical (same tree for same input — smoke/cast render unchanged); grep gates + CONTRACT sweep.

### Step 6: Harness + audit + ARCHITECT build + close
**Scope:** test fixtures; full sweep; D9/D10
**Action:** smoke `ansi` fixtures updated; full Auditor sweep (acceptance bar: zero cursor/offset/reparse over the product anywhere — grep-verified; zero var-content on Descriptors); ARCHITECT builds jam + cast, runs `./cast` — list structure, styling, tables, banner correct, zero asserts; then /log (cast, jam, KANJUT — separate commits) + drain 190025/190852/190431.
**Validation:** clean render both surfaces; all Auditor findings resolved in-sprint; ledger drained.

## BLESSED Alignment
- **S (SSOT):** one tree carries content+style; one carrier per block; one metadata IR; NVS the one metadata-bag type; style interned once (Stamp/Hyperlink).
- **E:** builders tell/consume; content delivered, never reconstructed; surfaces honor native contracts (juce at gui, cells at tui).
- **L:** GraphicDescriptor, TextRow, per-row text descriptors, rebuild paths — all deleted, nothing new beyond two ratified names.
- **B:** MarkdownDocument owns tree + descriptors + carriers; interning tables own styles/links.
- **D:** one parse, one build, deterministic projections.

## Risks / Open Questions
- Pairing-index Id word: Engineer STOPs if no existing word fits (Rule -1).
- `getTableValue` ~80-line decomposition: held Auditor flag, ARCHITECT ruling pending — not blocking.
- Mermaid sweep (GraphicDescriptor kill completion, label rendering off TextLine-proportional): awaits ARCHITECT command — recorded, not scheduled.
- KANJUT kernel divergence (jam_Document.h new queries have no kuassa twin; no markdown module there): flagged for the KANJUT session (HANDOFF.md exists).

## Verification
ARCHITECT builds jam + cast. `./cast --help` and gui markdown render styled correctly on both surfaces; cast fixtures + fixpoint pass; grep gates: zero manual parsing in cast, zero var-content descriptors, zero runs/links anywhere.
