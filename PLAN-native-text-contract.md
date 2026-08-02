# PLAN: Native Text Contract (jam_markdown → juce::AttributedString/TextLayout)

**RFC:** none — objective from ARCHITECT prompt (session-ratified decisions)
**Date:** 2026-08-02
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE (LANGUAGE.md reference implementation; single-header preferred ~300 LOC)

## Context

jam_markdown hand-rolls text wrapping (`wrapIntoRows`/`getRowBreak`, jam_MarkdownLayout.cpp:341-389/292-339) on `jam::GlyphArrangement`, which lives in jam_vulkan — dragging jam_vulkan (and its generated `JamVulkanShaderData.h`) into every consumer, including headless cast. ARCHITECT ruled: the rendering contract is 100% juce-native — VulkanEngine accelerates *below* the LLGC line (`externalContextFactory`, jam_VulkanEngine.h:74; `drawGlyphs`→GlyphAtlas, jam_VulkanLowLevelGraphicsContext.h:318-320), so nothing above it may name the backend. Wrapping/shaping/drawing move onto `juce::AttributedString` → `juce::TextLayout` (both sit on `detail::ShapedText`, juce_TextLayout.cpp:459 — shaping comes free). jam_vulkan and jam_mermaid leave jam_markdown's dependency list; cast's vulkan pull disappears (supersedes SPIR-V generation).

## Dependency & API Inventory

- **JUCE:** `juce::AttributedString` (Attribute = range+font+colour, juce_AttributedString.h:175-181; not final, :55), `juce::TextLayout` (`createLayout`, Line/Run/Glyph, `Line::stringRange`, `draw`→LLGC, juce_TextLayout.h:122/153-206), `juce::Font::FontStyleFlags` bold/italic/underlined (juce_Font.h:56-62 — **no strikethrough anywhere in juce_graphics**; no per-range payload).
- **jam parser model (untouched SSOT):** runs = var triplets [start,end,flags] with `Stamp::bold/italic/strike` (jam_Markdown.h:2395, :3391); links = [start,end,url] (:3394); tokens = `SyntaxToken{start,end,tokenType}` (jam_MarkdownSyntax.cpp); lexicon `Id::runs/links/tokens/text/type/checked` (jam_Lexicon.h:777-779 etc.).
- **Descriptor model (kept):** `jam::Descriptor<float>` = NamedValueSet + bounds (jam_Descriptor.h:19-23); per-row slicing helpers `sliceRuns/sliceLinks/sliceTokens` (jam_MarkdownLayout.cpp:157-213) — reused against `Line::stringRange`.
- **jam_graphics:** `fonts/` dir exists (jam_SimdBlend.h; include pattern jam_graphics.h:128). Containers: `jam::Array`, `jam::Function::Map`, `jam::LookupTable/LookupEntry` (jam_core/utils, function_map).
- **Consumers of MarkdownDocument:** cast Help.cpp:14; jam_tui toAnsiString (jam_tui_markdown.cpp:304-317); jam sandbox smoke (main.cpp:332-615). Public signatures (`ctor/layout/draw/toSvgString/getDescriptors`) unchanged — consumers survive.

## New Names (Rule -1 — ratified at plan approval)

- `jam::AttributedString` — placement mirrors juce: jam_graphics/fonts/jam_AttributedString.h
- `addLink (juce::Range<int>, const juce::String& url)` / `addSyntaxToken (juce::Range<int>, int type)` / `addStrikethrough (juce::Range<int>)`
- Readers (proven callers: draw post-pass, toSvgString): `getLinks()`, `getSyntaxTokens()`, `getStrikethroughs()`
- Sidecar entry types at class scope, family-shaped after `juce::AttributedString::Attribute` (range + payload): `Link { juce::Range<int> range; juce::String url; }`, `SyntaxToken { juce::Range<int> range; int type; }`, strikethroughs as `juce::Array<juce::Range<int>>` (payload-less — no struct)
- `MarkdownLayout::getAttributedString (text, runs, links, tokens, font, codeFont)` — SSOT builder, TextLine/runs → jam::AttributedString (Stamp flags → Font styles + sidecars)

## Validation Gate

Each step validated by @Auditor against MANIFESTO.md, NAMES.md, JRENG-CODING-STANDARD.md, and this locked plan before the next step. NO builds by agents — ARCHITECT compiles.

## Steps

### Step 1: jam::AttributedString
**Scope:** jam_graphics/fonts/jam_AttributedString.h (new, header-only), jam_graphics.h (include + module deps)
**Action:** `class AttributedString : public juce::AttributedString` with the three sidecars and readers above; entry structs at class scope; brace init; no bail-outs. jam_graphics module declaration gains `juce_graphics` dependency; include added at jam_graphics.h fonts block (:128 pattern). Zero includes in the submodule header.
**Validation:** name set exactly as ratified; struct discipline; header-only ≤300 LOC.

### Step 2: MarkdownLayout wrap engine replacement (delete first)
**Scope:** jam_markdown/layout/jam_MarkdownLayout.cpp/.h
**Action:** Delete `configureArrangementSize`, `getEntryCursor`, `getRowBreak`, `wrapIntoRows`, and all `jam::GlyphArrangement&` parameters (layout signature → `layout (root, font, codeFont, width, descriptors, bounds)`). Add `getAttributedString` SSOT builder. Flow blocks (paragraph :409, heading :416, codeBlock :536, htmlBlock :570, image alt :634, table cells) build jam::AttributedString → `juce::TextLayout::createLayout (as, width)` → one row Descriptor per `Line` via `Line::stringRange` + existing `sliceRuns/sliceLinks/sliceTokens`; row height/baseline from Line metrics. Geometry blocks (thematicBreak, blockquote, list, listItem, mermaid) untouched. Dispatch stays `jam::Function::Map`.
**Validation:** zero GlyphArrangement references in jam_markdown/layout; descriptor row model byte-compatible in properties keys; no new names beyond ratified set.

### Step 3: MarkdownDocument on native draw
**Scope:** jam_markdown/document/jam_MarkdownDocument.h/.cpp
**Action:** Delete members `arrangement`, `codeArrangement`, per-variant `Typeface::Ptr`s and `jam::Typeface::getInstance()` registration (:29-55 — atlas typeface registration is VulkanEngine's job, jam_VulkanEngine.h:218). Keep `font`/`codeFont` + ctor signature. `drawTextRow`: row AttributedString (via `getAttributedString` + LookAndFeel colours) → single-line `juce::TextLayout::draw (g, bounds)`; code-run tint and strikethrough drawn as post-pass from sidecar ranges using TextLayout Run/Glyph positions (replaces `drawCodeRunTint` arrangement walk :76-129). `toSvgString`: glyph positions from `line.lineOrigin + glyph.anchor` instead of arrangement entries. `Function::Map` dispatch and public API unchanged.
**Validation:** public surface identical; zero vulkan symbols; restamp/colour LookupTables (MarkdownLayout statics) remain SSOT.

### Step 4: Module decoupling
**Scope:** jam_markdown/jam_markdown.h
**Action:** Dependencies → `juce_core, juce_graphics, juce_gui_basics, jam_core, jam_graphics` (LookAndFeel in draw requires juce_gui_basics). Remove `jam_vulkan` include/dep entirely. `jam_mermaid` include and `layoutMermaid`/diagram dispatch gated `#if JUCE_MODULE_AVAILABLE_jam_mermaid && JAM_MARKDOWN_MERMAID`.
**Validation:** no vulkan reference anywhere in jam_markdown; gate macro correct per JUCE module macro semantics.

### Step 5: jam_tui adaptation
**Scope:** jam_tui/markdown/jam_tui_markdown.cpp, jam_tui.h deps
**Action:** `toAnsiString` layout width becomes pixels: cols × mono advance (from the document's mono font metrics); `getCellHeight` reads TextLayout-derived line heights; cell/SGR walk (`getStyledLine`, restamps) unchanged. jam_tui module deps already juce_gui_basics-complete — verify only.
**Validation:** no hand-rolled wrap logic re-introduced; ANSI path facts unchanged.

### Step 6: cast cleanup
**Scope:** cast/CMakeLists.txt
**Action:** Remove harfbuzz include dir wired for the vulkan chain if now unreferenced; JAM_MODULES stays `jam_core jam_tui`. No source changes.
**Validation:** no dangling vulkan/harfbuzz references in cast configure inputs.

### Step 7: Final audit + ARCHITECT build
**Scope:** all touched files
**Action:** @Auditor full sweep (contracts + locked plan); report residuals verbatim. ARCHITECT compiles cast and jam smoke sandbox.
**Validation:** zero unresolved findings.

## BLESSED Alignment

- **B:** typeface registration removed from MarkdownDocument — atlas ownership stays with VulkanEngine; no floating vulkan coupling.
- **L:** deletes the ~250-line hand-rolled wrap engine; YAGNI — no abstract measurer seam (rejected), no engine choice.
- **E:** no bail-outs; dispatch via Function::Map; module arrow strictly downward (markdown never names the backend).
- **S (SSOT):** parser var-triplet model untouched; one AttributedString builder; wrapping logic exists once — in JUCE.
- **S (Stateless):** MarkdownDocument keeps only fonts + descriptors; per-paint row layout is transient, matching current per-paint shape behavior.
- **D:** same input → same layout via juce's deterministic shaping.

## Risks / Open Questions

- `toSvgString` glyph-position rework is the most intricate step (Run/Glyph anchors vs arrangement entries) — behavior verified against smoke SVG fixtures by ARCHITECT build.
- None open — all names and placements ratified in-session.

## Verification

ARCHITECT builds: cast (`JamVulkanShaderData.h` error must vanish), jam sandbox smoke (draw/toSvgString/layout call sites), then `./cast.sh --help` renders the CMY help page through the new pipeline.
