# PLAN: sync-unibreak-wrap

**RFC:** none. The objective comes from ARCHITECT's prompts.
**Date:** 2026-09-25 (revision 6)
**BLESSED Compliance:** verified against the reads cited in each step
**Language Constraints:** C++17 / JUCE + JAM (LANGUAGE.md C++/JUCE §L)

## Overview

Revision 3's Step 6 made a hidden state machine: `UniBreak::Result`, `init`/`step`/`end` and a callback walk. Its state lived outside the Model. Revision 4 rebuilt the text layer from jam's own owners:
- `Unibreak` derives from `Document`.
- `Element` holds lines, segments and rows.
- `Document::Index` counts lines and rows only.
- `AttributedChar` owns width and the cluster walk.
- AnsiDocument owns the cell contract.

Revision 5 corrected revision 4's Unibreak API: break opportunities only, rows as the renderer's materialisation, MarkdownDocument as the oracle, `jam::Function::Map` for every keyed rule set.

Revision 6 fixes the shape, the home and the packer, from the sandbox measurement and ARCHITECT's rulings:
- The type is `ReflowDocument`. It is universal: any text, any consumer. Its use is `ReflowDocument::parse (text, columns)`; the consumer reads rows (`Id::row`) and renders each with `Document::getText (const Token&)`.
- Its home is a new kernel module, `jam_document`. It depends on `jam_core` only. It is not core.
- The three layers are: **opportunities** (`parse`, creation), **packing** (`setRows (line, columns)`, greedy, a state update on the Model, sibling of `AnsiDocument::setRows`), and **host validity** (the consumer's oracle; retract is `setGlue (line, offset)`, a state update on the Model, then `setRows` again).
- The packer is greedy. ARCHITECT: "use greedy then" (after the scale numbers below).
- Rows live on the Model as `Id::row`, the same property `AnsiDocument` writes and `Document::Index` counts.
- Step 0 records the KANJUT → JAM hand fork that landed this session. The reflow work is an addition on top of it.
- The module-table schema change (identity `vendor`, `## framework`, ten-column `## module`) is done and recorded in Step 16.2.
- Steps 6, 9, 10, 11, 12 and 14 are on disk. Steps 7 and 13 remain. Step 16 is the proof set.

## ARCHITECT rulings (verbatim, revision 6)

- "Unibreak is NOT only for markdown, not only for cell, not only for table, etc. Unibreak is universal line break, text wrapping, reflow, for any text, any document"
- "i'm 100% sure Document as the base is the correct isomorphic abstraction"
- "i want the most deterministic, but also fastest for any size of Document"
- "is Unibreak : Document, is wrong shape? should it be just Reflow : Document with unibreak implementation absorbed?"
- "i wonder why cant we eventually, simply have something like … auto reflowed = ReflowDocument::reflow (text, colWidth); isnt the whole idea … as simple as that?"
- "then dont add ReflowDocument at jam_core, it's not core at all"
- "should we just have jam_document module, and bring all document based module there including reflow, the only deps should be just jam_core?"
- "why cant all Document based put into jam_Document? while the consumer kept at their own domain?"
- "handroll KANJUT to JAM conformance first to be safe, hence JAM stay ahead and get latest windows runtime hardening from KANJUT"
- "all KANJUT latest changes (EVERYTHING) must be forked back to JAM. then our current sprint with Reflow is just addition on top of it"
- "i dont want to risk to run cast sync now, our sprint havent yet finished, it could break. because of divergence."
- "HANDROLL FORK EVERYTHING NEW FROM KANJUT"
- "comparing ns is stupid, in real usage sub ms is negligible"
- "i think the only number matters are above >= 500 ms. anything below that is negligible especially for huge line numbers"
- "so you cant just blindly said greedy always win, when the gap 1-1000 ns, 1 ms nothing"
- "finish the sandbox first and get accurate measurement, then update the plan"
- "use greedy then"
- Module table: "website and license are not belong to module table, they all constants" / "freetype attribution could be moved into its header" / "version should not be sync no? it could be different for each framework" / "why redundant vendor and moduleVendor?"

## ARCHITECT rulings (verbatim, revision 5)

- "why handroll static tables? CONTRACT VIOLATIONS"
- "generated only included at core, no submodules should include anything"
- "Unibreak is just another document using the exact shape of Vocabulary, Rules, Token … there must be universal isomorphic abstraction to use Document as the SSOT design by contract"
- Line rule SSOT: "MarkdownDocument as oracle" — no predicate table anywhere; the writer parses the candidate rows and reads the block type.
- "~/Documents/Poems/dev/libunibreak/ why reinventing the wheel? your task only to clean-room rewrite from scratch BLESSED compliance design by contract into our domain"

## ARCHITECT rulings (verbatim, revision 4)

- "rename UniBreak to Unibreak as derivation of Document. we already have many working pattern as example, isomorphic abstraction holds. do not create new pattern."
- "Document::Index is not cell operation. cell monospace is AnsiDocument contract"
- "monospace, cell, etc is AnsiDocument contract, has nothing to do with Index, B-Tree, line, wrapping, reflow"
- "fixing AnsiDocument is scope of this plan … you only move working implementation to AnsiDocument from Index"
- "then fix it so it compiles" (TextEditor's Index call sites)
- "TextEditor is unfinished work. will be continued later when working with EVE."
- Width: "isn't this already carried by AttributedChar?"
- These rulings from revision 3 still hold:
  - "juce::String only being used as I/O, input and final output"
  - "NO COPY, NO ALLOC, NO MUTATION ALLOWED"
  - reflowed paragraph → "as materialisation"
  - "NEVER FIGHT, NEVER BREAK JUCE NATIVE RENDERING CONTRACT"
  - byte range → "jam::Union"

## Sandbox measurement (dev/sandbox, `sandbox.exe --scale`, Windows Release)

The harness parses with `jam::Unibreak::parse`, packs with `RowPacker::packGreedy` (RowPacker.cpp:21–43) and `RowPacker::packKnuthPlass` (:119–138), and validates rows with `Paragraph::isValidParagraph` = one root child of type `map::BlockType::paragraph` (Paragraph.cpp:35–42). Widths come from `AttributedChar::width (std::string_view)` (RowPacker.cpp:16–19). Cells at or above 500 ms are marked ◆.

**A. One line, N units, limit = 100**

| units | parse s | greedy pack s | greedy rows | knuth-plass pack s | knuth-plass rows | peak MB |
|---|---|---|---|---|---|---|
| 10000 | 0.006 | 0.004 | 777 | ◆ 2.365 | 784 | 18.5 |
| 100000 | 0.048 | 0.038 | 7731 | ◆ 250.181 | 7805 | 49.1 |
| 1000000 | 0.456 | 0.382 | 77367 | skipped, >60 s at previous N | - | 303.0 |

**B. Many lines, L lines, limit = 100**

| lines | parse s | greedy pack s | greedy rows | knuth-plass pack s | knuth-plass rows | peak MB |
|---|---|---|---|---|---|---|
| 10000 | 0.123 | 0.116 | 26048 | 0.469 | 26100 | 303.0 |
| 100000 | ◆ 1.632 | ◆ 1.240 | 260991 | ◆ 4.711 | 261514 | 1133.0 |
| 1000000 | ◆ 60.715 | ◆ 15.127 | 2609957 | ◆ 49.768 | 2615193 | 3172.4 |

Oracle loop at L = 10000 (greedy): 0.527 s, 16595 `MarkdownDocument::parse` calls.

**C. Re-row L = 100000 at a new width, greedy, no re-parse**

| limit | pack s | rows |
|---|---|---|
| 72 | ◆ 0.928 | 339787 |
| 40 | ◆ 0.637 | 585849 |

Determinism: A (N = 10000) pass; B (L = 10000) pass.

Facts the numbers give:
- Greedy is under parse time at every size. It crosses 500 ms only at 100k lines and above.
- Knuth–Plass is quadratic in units per line: 2.4 s at 10k units, 250 s at 100k units. At 1M lines it is 49.8 s against greedy's 15.1 s. Row counts differ by under 1%.
- Parse is the dominant cost at 1M lines (60.7 s).
- Ruling: "use greedy then".

## Dependency & API Inventory (read this session)

- **Document engine.**
  - `parse (const char*, int)` normalises the input with `preprocess` and runs `getTokens`. It walks the `Cursor`, calls `getToken` until the source is consumed, then calls `build()` (jam_Document.cpp:586–631).
  - `getToken` can return any length above 0 (cpp:616–627).
  - The hooks are declared at jam_Document.h:999–1013.
  - `Document::Writer` is the domain text-renderer seat: `virtual juce::String getText (const Document&) const = 0` plus `toFile` (jam_Document.h:704–713).
  - `getSource()` returns the preprocessed UTF-8 bytes (h:758). `juce::String getText (const Token&) const` materialises a token's span as a String (h:774). This is the established "text out" of a span; no consumer writes its own `fromUTF8` copy.
  - `Token` is `Span` (`jam::Union<uint32_t, uint32_t>`) plus `int type`; `offset()`, `length()`, `setSpan (start, end)` (h:50–94).
- **Nearest sibling: `CodeDocument`** (jam_CodeDocument.h:25–138).
  - `static parse (text, info)` sets a member, then calls `Document::parse` (:40–54).
  - `getToken` appends to `tokens` (:79–122).
  - `build()` stores the tokens on the root (:125–128).
  - Members: `jam::Array<Document::Token> tokens`, `int family` (:137–138).
- **Row sibling: `AnsiDocument`** (jam_AnsiDocument.h:78–84, .cpp:176–241).
  - `static void setRows (Document::Element& line, int columns)` replaces the line's `Id::row` tokens in place: `*line.get<Document::Tokens> (Id::row) = std::move (rows)` (cpp:176–193).
  - `static void setRows (Document::Index&, int columns)` re-rows every line and calls `index.setNumRows (line)` (cpp:195–202).
  - `addLine` adds `Id::row` on the line then calls `setRows` (cpp:235–241).
- **Index counts `Id::row`.** `appendLine`/`insertLine` assert `Id::row` (jam_DocumentIndex.cpp:28, :45); a leaf's row count is `jmax (minNumRows, line->get<Tokens> (Id::row)->size())` (:87, :103); `setNumRows (int lineNumber)` (:77). Any Document whose lines carry `Id::row` is indexable. No Index work is in this sprint.
- **Model value types.**
  - `Element::Value` holds `Tokens = jam::Array<Token>` (h:166, :173–187).
  - A property is replaced through its `get` pointer. This is established at DocumentIndex.cpp:469 and TextEditor.cpp:912.
  - The root children are iterated with `Document::begin`/`end` (h:741–745).
- **Identifiers that exist:** `Id::line` (jam_Identifiers.h:744), `Id::row` (:1060), `Id::tokens` (:1351), `Id::cells` (:211).
- **Byte-class tables.** `## unibreak` in jam/cast/lookuptables.md:680–692 has one row `| 0x0 | map::Byte::text |`; its brief names Unibreak (:683–685). Wiring in jam/cast/spell.md:1655–1662 (`@lookuptables:unibreak`, `- name: unibreak`).
- **AttributedChar (jam_core, frozen through the fork).**
  - `narrowColumns { 1 }`, `wideColumns { 2 }` (jam_AttributedChar.h:97–98).
  - `static uint8_t getWidthClass (uint32_t)` (:271); `static int width (uint32_t)` (:283); `static int width (std::string_view)` (:285); `applyToClusters` (:351).
  - `lineBreakClass`, `isEastAsian`, `isInitialQuotation`, `isFinalQuotation`, `isPotentialEmoji` definitions at jam_CharProps.cpp:2661–2685 (restored this session from the pre-fork backup; SHA-256 matches).
- **Current Unibreak files (jam_core/text).**
  - `jam_Unibreak.h:1–74`: constants (:7–10), `parse (text, language)` (:12), the revision-4 `setRows` template with two lambdas (:14–24), `addLine (Document&, Tokens segments, Token row)` (:32), `getRow (segments, first, last)` (:33), the `getRows` template with a `forbidden` array (:35–66), members `tokens`, `language` (:68–69).
  - `jam_Unibreak.cpp:1–121`: `getTailoredClass` (:4–11), `getResolvedClass` (:13–22), `trailingClasses` (:24–25), `getSegment` (:27–42), `parse` (:44–50), `getVocabulary` with `map::unibreak` (:52–56), `getToken` (:58–75), `addLine` writing one `Id::row` (:77–82), `getRow` (:84–88), `build()` (:90–117).
  - `jam_UnibreakRules.cpp`: decision functions (:8–20 declarations, :188–381 bodies), class masks (:52–88), run-state predicates `getSpacedClass`/`isRegionalOdd`/`isNumericRun`/`isNumericClosed`/`isQuoteOpen` (:91–146), `getPrologueDecision` (:148–155), **hand-written `jam::LookupTable` tables `contextRules` (:157–175) and `precedingRules` (:177–186)** dispatched at :202 and :207.
  - Includes: jam_core.h:118 (`text/jam_Unibreak.h`, after `document/jam_Document.h` :117); jam_core.cpp:5–6.
  - References outside jam_core/text: jam_MarkdownWriter.cpp:108–121 (`jam::Unibreak::parse`, `setRows`), generated/jam_LookupTables.h (`map::unibreak`), cast/lookuptables.md, jam_core.h, jam_core.cpp. No other file in jam names `Unibreak`.
- **New-module precedent: `jam_web`.**
  - Umbrella jam_web/jam_web.h:1–34: declaration block (:1–18, `dependencies: juce_core, jam_core` :10), `#pragma once`, `<juce_core/juce_core.h>`, `<jam_core/jam_core.h>`, then its submodule headers (:25–33).
  - Module row user-modules-info.md:150–154 (`kernel`, dependencies `juce_core`, `jam_core`). Rows are alphabetical: `jam_debug` :63, `jam_dsp` :70.
  - spell.md index rows `@modules` … `@jam_whelmed` (:104–124, alphabetical; `@jam_debug` :110, `@jam_dsp` :111); wiring block per module (:3240–3242 for jam_web: `@modules:identity`, `@modules:framework`, `@modules:module:name=jam_web` against `@code:moduleDeclaration`).
- **jam_markdown dependencies.** jam_markdown.h:10 `dependencies: juce_core, jam_core, juce_gui_basics`; includes :20–22. user-modules-info.md:100–103 (`juce_core`, `jam_core`, `juce_gui_basics`). cast/project-info.md:139–141 lists user modules `jam_core`, `jam_subprocess`, `jam_markdown`.
- **UAX #14 rules:** see the Unibreak files above. libunibreak (`~/Documents/Poems/dev/libunibreak`) stays the external reference: `set_linebreaks_utf8` fills one decision per code point — MUSTBREAK / ALLOWBREAK / NOBREAK / INSIDEACHAR (linebreak.h:59–76); no width, no wrapping (linebreak.h:5).
- **Rule dispatch shape in jam.** Every Document domain dispatches keyed rules through `jam::Function::Map` built once in a static-local lambda: `segments` (jam_CodeDocument.h:81–97, jam_Css.cpp:99–131), `markupOperators` (jam_Xml.h:801–810, jam_Html.cpp:94–99), `leafBlocks` (jam_MarkdownBlockParser.cpp:2334–2363), `sgrParameters` (jam_AnsiDocument.h:203). A key with a default falls through `contains (key) ? get (…) : default` (jam_CodeDocument.h:358–365). `get` deduces `T&` for a named lvalue (jam_Function.h:252–256).
- **`jam::LookupTable`** is the generated lane's type (`@lookuptables` → generated/jam_LookupTables.h, spell.md:44–57). Generated headers are included only by jam_core (jam_core.h:82 ← generated/jam_Generated.h); a submodule includes nothing (CODING.md Header Discipline).
- **MarkdownDocument as oracle.** `static MarkdownDocument parse (const juce::String&)` (jam_MarkdownDocument.h:45). Every root child block carries `Id::type` (jam_MarkdownBlockParser.cpp:67); `map::BlockType::paragraph` is the paragraph type (:2309). A grid cell holds `Id::rawText` (:1495), not blocks, and every cell row re-enters the grid parser inside `| … |`, so no cell row can open a block: the line rule binds the paragraph only.
- **Blocks that need two rows.** A setext heading is a paragraph line plus an underline (`promoteSetextHeading`, :2397); a pipe table is a header row plus a delimiter row. A lone `===` or `| a |` is a paragraph. The oracle therefore tests the rows cumulatively.
- **The rev-4 line rule still on disk.** `MarkdownDocument::isOpenBlock` (h:88, cpp:88–91) → `BlockParser::isOpenBlock` (h:575, BlockParser.cpp:236–247) → `openBlockConditions` (`OpenBlockCondition` h:1083, table h:1085–1086, after `isGridTableHeaderSeparator` h:1081). bimaps.md `## OpenBlock` rows `blockquote | 6`, `listItem | 7`, `setextHeading | 8`, `table | 9` (:488–491); `leafBlocks` keys only rows 0–5 (BlockParser.cpp:2357–2363).
- **`htmlBlockConditions`** hand-written `jam::LookupTable` (h:759–762) read at BlockParser.cpp:1035 inside `getHtmlBlockCondition` (:1027); callers :1112, :1129.
- **MarkdownWriter today.** `getWrappedText (const juce::String&, float limit)` (h:146, cpp:106–131) builds two lambdas and calls `unibreak.setRows`, then copies rows with `juce::String::fromUTF8` (:127–128). `getParagraphText` (cpp:265–271) casts the limit to `float`. The grid-cell caller is cpp:543. `getJoinedText` (cpp:94–104) maps newlines to spaces (:100). `MarkdownWriter::getRowText` already exists and renders a table row (h:209, cpp:175) — revision 5's `getRowText` name collided with it and is withdrawn.
- **Token is move-only** (`jam::Array` member, jam_Document.h:105; jam_Array.h:895). `Element::add<ValueType> (key, args…)` constructs in place (jam_Document.h:424–430).
- **Fork state.** Freeze manifest: 62 files with SHA-256 (scratchpad `jam-freeze/MANIFEST.md`). jam_graphics.h:46–47 includes `geometry/jam_Cell.h` then `text/jam_ShapedTextOptions.h`. `jam_core/jam_core.cpp:1–15` is the frozen TU list.

## Validation Gate

- COUNSELOR validates each step against MANIFESTO, NAMES, CODING and this PLAN.
- @Auditor runs one time, after the final step.
- Every @Engineer prompt restates these items:
  - Design by Contract per CODING CRITICAL RULES.
  - The MANIFESTO **E** MVP contract, word for word.
  - The facts that the step cites.
  - Doxygen first. No comments.
  - The explicit file set, and "touch nothing outside it".
  - The names in this PLAN, and zero other names. Local variable names are free. If a step needs another member name, the Engineer stops and reports.
- Agents do not build or run cast. ARCHITECT builds (Step 16).
- Agents never run git and never delete files. ARCHITECT deletes.

## Names ratified by revision 6

- `ReflowDocument` (type), `jam_document` (module), files `jam_document/jam_document.h`, `jam_document/jam_document.cpp`, `jam_document/document/jam_ReflowDocument.h`, `jam_document/document/jam_ReflowDocument.cpp`, `jam_document/document/jam_ReflowDocumentRules.cpp`.
- `ReflowDocument::parse (const juce::String& text, int columns, int language)`.
- `ReflowDocument::setRows (Element& line, int columns)` — the `AnsiDocument::setRows (line, columns)` family.
- `ReflowDocument::setGlue (Element& line, uint32_t offset)` — the retract state update.
- ~~`ReflowDocument::getText() const`~~ — **withdrawn** (ARCHITECT: "the name is garbage"). A derived `getText` hid `Document::getText (const Token&)` (jam_Document.h:774) by C++ name lookup; the `using Document::getText;` patch was the symptom. No jam Document has a whole-document `getText()`; the consumer joins rows: `MarkdownWriter::getWrappedText (const jam::ReflowDocument&)`, an overload of the existing `getWrappedText (const juce::String&, int)` (MarkdownWriter.h:146), named for its product (Rules 3, 7; the writer's `get<Product>Text` family). An interim `getJoinedText (const ReflowDocument&)` overload was rejected: "joined" names the mechanism.
- Member `columns` beside `language`.
- Lookup table `## reflow` / `map::reflow` (renamed from `## unibreak`).
- `MarkdownDocument::isParagraph (const juce::String&)`.
- `MarkdownWriter::getOpenBlockRow (const jam::ReflowDocument&, const Element& line)`.
- Revision 5 names withdrawn: `MarkdownWriter::getRows`, `MarkdownWriter::getRowText`, `forbiddenStarts`.
- Family-shaped names that landed in execution (NAMES Rule 5, no ratification needed): file-statics `getRowWidth` (the sandbox's RowPacker.h:25 name) and `getSegmentIndex` in jam_ReflowDocument.cpp:44–55; table accessors `getContextRules()`/`getPrecedingRules()` (jam_ReflowDocumentRules.cpp:155, :194) and `BlockParser::getHtmlBlockConditions()` (jam_MarkdownBlockParser.cpp:1014), the jam_CodeDocument.h:331–350 accessor shape.
- Fence `module` in both `code.cast` (ARCHITECT ruling, Step 16.2).

## Steps

### Step 0 — KANJUT → JAM hand fork (DONE this session)
- Every KANJUT kernel scope (13 modules, `cast/`, `resources/`, `patch/`) was transformed through the identity pairs and written into JAM by script (`fork/fork.py`, `fork/apply.py`), with dry-run, `backup/` (1,086 pre-fork files), and SHA-256 verification.
- 62 files were frozen and never overwritten (scratchpad `jam-freeze/MANIFEST.md`): the jam cast tables and generated headers, jam_core's `jam_Document.h`, `jam_DocumentIndex.cpp`, `jam_core.h/.cpp`, `jam_AttributedChar.h`, `jam_Charset.cpp`, the three Unibreak files, `jam_graphics.h`, `jam_gui.h`, `jam_TextEditor.cpp`, the five jam_markdown document files, the four jam_terminal files, the six jam_vulkan TUs that were then deleted, six patches, `user-modules-info.md`.
- Deleted (ARCHITECT, by hand): `jam_vulkan/context/jam_VulkanGraphicsFrame.cpp`, `…ImageCache.cpp`, `…SetupPresentationTarget.cpp`, `…SetupRenderTargets.cpp`, `jam_vulkan/engine/jam_VulkanImageProcessor.h/.cpp`, `jam_graphics/geometry/jam_CellPoint.h`, `jam_CellRectangle.h` (KANJUT's `jam_Cell.h` defines `Cell::Point`/`Cell::Rectangle` inline).
- `jam_graphics/jam_graphics.h` is KANJUT's (`geometry/jam_Cell.h` :46, `text/jam_ShapedTextOptions.h` :47). `ShapedTextOptions` stays: Step 16.1 is void.
- New in JAM from KANJUT: `patch/juce-module-lifetime-hook.patch`; `patch/juce-vulkan-engine-hook.patch` and `juce-paint-update-rect-hook.patch` are KANJUT's; the Sprint 125 VulkanEngine model (`juce::Thread`, `create()`/`destroy()`, `moduleHooks`).
- `jam_core/text/jam_CharProps.cpp` was overwritten by the fork and lost five `AttributedChar` statics; restored from the pre-fork backup (SHA-256 `621ae515…` = backup). No other jam_core TU lost a definition (`jam_Format.cpp:407` differs only by KANJUT's added `removeCharacters` arguments).
- Proof: `dev/sandbox` links and runs against the forked tree (Sandbox measurement above).

### Steps 1–5 — DONE
These steps are validated and do not change:
- the sync tie-break
- SPEC and HELP for sync
- the jam cast tables (`map::LineBreak`, masks, tailoring)
- the property word

### Step 6 — DONE
`AttributedChar::getWidthClass` (h:271), `narrowColumns`/`wideColumns` (h:97–98), `applyToClusters` (h:351), `width (std::string_view)` (h:285).

### Step 7: ReflowDocument — a Document in `jam_document` (revision 6)
**Scope:**
- **Add:** `jam_document/jam_document.h`, `jam_document/jam_document.cpp`, `jam_document/document/jam_ReflowDocument.h`, `jam_document/document/jam_ReflowDocument.cpp`, `jam_document/document/jam_ReflowDocumentRules.cpp`.
- **Edit:** `jam_core/jam_core.h` (delete :118), `jam_core/jam_core.cpp` (delete :5–6), `jam/cast/lookuptables.md:680–692`, `jam/cast/spell.md` (:1655–1662 wiring; index row after :110; moduleDeclaration block after `jam_debug`'s), `jam/user-modules-info.md` (module row between :63 and :70; jam_markdown dependencies :100–103), `jam_markdown/jam_markdown.h` (:10, :21), `cast/project-info.md` (user-module row after :141).
- **Delete (ARCHITECT, by hand, after the new files compile):** `jam_core/text/jam_Unibreak.h`, `jam_Unibreak.cpp`, `jam_UnibreakRules.cpp`.

**Contract.** ReflowDocument is the clean-room map of libunibreak's public contract (linebreak.h:59–76) plus greedy packing on the Model. MUSTBREAK ends an `Id::line`. ALLOWBREAK ends a segment inside the line's `Id::tokens`. NOBREAK is inside a segment. Rows are `Id::row` on the line: greedy, at `columns`, measured by `AttributedChar::width`. ReflowDocument knows no consumer grammar: no callback, no predicate, no oracle. Host validity is the consumer's (Step 13), through two state updates: `setGlue`, then `setRows`.

1. **Module `jam_document`.**
   - `jam_document.h` in the jam_web.h:1–34 shape: `ID: jam_document`, `dependencies: juce_core, jam_core`, `#include <juce_core/juce_core.h>`, `#include <jam_core/jam_core.h>`, `#include "document/jam_ReflowDocument.h"`. No other include; submodule files include nothing.
   - `jam_document.cpp`: `#include "jam_document.h"`, `#include "document/jam_ReflowDocumentRules.cpp"`, `#include "document/jam_ReflowDocument.cpp"` (jam_core.cpp:5–6 order: rules first).
   - `user-modules-info.md`: row `jam_document | kernel | JAM Document | Universal line break and reflow over jam::Document (UAX #14) | juce_core, jam_core` in the jam_markdown:100–103 shape (empty frameworks, libs, standard, searchpaths); alphabetical between `jam_debug` and `jam_dsp`. `jam_markdown`'s dependencies gain `jam_document`.
   - `spell.md`: index row `| @jam_document | ../jam_document/jam_document.h |` after `@jam_debug` (:110); moduleDeclaration block after `jam_debug`'s with the three list lines `@modules:identity`, `@modules:framework`, `@modules:module:name=jam_document` (the :3240–3242 shape).
   - `jam_markdown.h`: `dependencies: juce_core, jam_core, jam_document, juce_gui_basics` (:10); `#include <jam_document/jam_document.h>` after `<jam_core/jam_core.h>` (:21).
   - `cast/project-info.md`: `| @user-module | jam_document | Universal line break and reflow over jam::Document (UAX #14) |` after :141.
   - `jam_core.h:118` and `jam_core.cpp:5–6` are deleted.
2. **Vocabulary.**
   - `## unibreak` (lookuptables.md:680) becomes `## reflow`; brief: "Byte to its ReflowDocument classification. Every byte is text; ReflowDocument decodes codepoints itself." One row `| 0x0 | map::Byte::text |`.
   - spell.md:1655–1662: `@lookuptables:unibreak` → `@lookuptables:reflow` (three lines) and `- name: unibreak` → `- name: reflow`.
   - `getVocabulary()` returns `static const Vocabulary { map::reflow, {}, {} }` (jam_Unibreak.cpp:52–56 shape). `map::reflow` reaches jam_document through `<jam_core/jam_core.h>` (jam_core.h:82); jam_document includes no generated header.
3. **`struct ReflowDocument : Document`**, in the CodeDocument shape.
   - `static constexpr int noBreak { 0 }`, `allowedBreak { 1 }`, `mandatoryBreak { 2 }`, `undecided { 3 }` (jam_Unibreak.h:7–10, same names and values).
   - `static ReflowDocument parse (const juce::String& text, int columns, int language)`: sets `columns` and `language`, then `Document::parse (text.toRawUTF8(), getNumBytesAsUTF8())` (jam_Unibreak.cpp:44–50 shape).
   - `void setRows (Element& line, int columns)` — public. Greedy, the measured `RowPacker::packGreedy` (RowPacker.cpp:21–43) without the forbidden set:
     - `segments` = `*line.get<Tokens> (Id::tokens)`; `rows`; `start = 0`.
     - While `start < segments.size()`: `end = start + 1`; while `end < segments.size()` and `AttributedChar::width (std::string_view { source }.substr (row.offset(), row.length())) <= columns` for `row = getRow (segments, start, end + 1)`: `++end`. Then `rows.add (getRow (segments, start, end))`; `start = end`.
     - `*line.get<Tokens> (Id::row) = std::move (rows)` (jam_AnsiDocument.cpp:192 shape).
     - A unit wider than `columns` is a row of its own. An empty line has zero rows.
   - `void setGlue (Element& line, uint32_t offset)` — public. The segment in `Id::tokens` whose `offset()` equals `offset` (asserted present, index ≥ 1) is joined into its predecessor: `predecessor.setSpan (predecessor.offset(), segment.offset() + segment.length())`, then the segment is removed through `jam::Array`'s removal API (jam_Array.h — read before use). Rows are not touched here; the caller calls `setRows` next.
   - No whole-document text member (withdrawn, see Names). Row text is `Document::getText (const Token&)` (h:774); the join is the consumer's (Step 13.4a).
   - Members: `Document::Tokens tokens`, `int language { map::LineBreakLanguage::und }`, `int columns`.
   - Private statics: `getRow (const Tokens& segments, int first, int last)` (jam_Unibreak.cpp:84–88, kept — revision 5's deletion of it is superseded), `addLine (Document&, Tokens segments)`: adds the `Id::line` child, `add<Tokens> (Id::tokens, std::move (segments))`, `add<Tokens> (Id::row)` (jam_AnsiDocument.cpp:237–239 shape).
   - **Delete:** the `setRows` template (jam_Unibreak.h:14–24), the `getRows` template and its `forbidden` array (:35–66), `addLine`'s `Token row` parameter (:32, cpp:77–82), the third `getSegment` call in `build()` (cpp:112).
4. **`getToken (Cursor&, int)`** — jam_Unibreak.cpp:58–75 unchanged in behaviour:
   - decodes one codepoint at the cursor; resolves the class with `getResolvedClass (codepoint, language)`;
   - UAX #14 LB9: a CM or ZWJ codepoint after a token whose class is not in `nonAbsorbingClasses` extends that token with `setSpan` (creation);
   - UAX #14 LB10: a CM or ZWJ codepoint in any other position appends a token with class AL;
   - otherwise appends `Token { start, end, class }`; returns the byte length.
5. **`build()`** — jam_Unibreak.cpp:90–117 with one change: at a mandatory break, `addLine (*this, std::move (segments))` then `setRows (*line, columns)` on the line just added (the `AnsiDocument::addLine` :240 shape). `tokens` is consumed here; a `Token` is move-only: `segments.add (std::move (segment))`, never a copy.
6. **The rules TU (`jam_ReflowDocumentRules.cpp`)** — jam_UnibreakRules.cpp with `Unibreak::` → `ReflowDocument::` and one structural change:
   - `contextRules` (:157–175) and `precedingRules` (:177–186) become `jam::Function::Map<int, int>`, each built once in a static-local immediately-invoked lambda inside its one consumer — `contextRules` inside `getContextDecision` (:205–208), `precedingRules` inside `getBoundaryDecision` (:188–203) — in the jam_CodeDocument.h:81–97 shape. Registration is `add<const Document::Tokens&, const std::string&, int&> (map::LineBreak::X, getXDecision)`; the call is `get (lineBreakClass, tokens, source, boundary)` with named lvalues (jam_Function.h:252–256). The entries are the same as today's tables. An unlisted class takes the former default: `getMaskDecision` for `contextRules`, `getContextDecision` for `precedingRules`, through `contains (…) ? get (…) : default (…)` (jam_CodeDocument.h:358–365 shape). Delete `using Rule`. No `jam::LookupTable` is declared in jam_document; the generated `map::lineBreak*` tables are read as today (jam_Unibreak.cpp:4–22).
   - The masks (:24–25, :52–88), the run-state predicates (:91–146), `getPrologueDecision` (:148–155) and the decision functions (:210–381) do not change.

**Validation:**
- No state outside the Model; `columns` and `language` are parse inputs (CodeDocument's `family` shape).
- No out-parameter, no bail-out, at most 30 lines and 3 branches per function.
- One token array. The rules read in place.
- No `jam::LookupTable` declaration, no template member, no callback, no predicate in jam_document.
- `jam_document` includes `jam_core` only. jam_core has no `Unibreak`/`Reflow` reference.
- **ARCHITECT exception (L, LANGUAGE.md domain-complex single-use):** `jam_ReflowDocumentRules.cpp` stays one TU at 418 lines — the UAX #14 rule set is kept whole. "Keep one TU (LANGUAGE.md exception)". The Auditor is told; it is not a finding.
- `setRows` on a `Document::Index`-held ReflowDocument line would keep DocumentIndex.cpp:28/:45/:87 valid (`Id::row` present). Not exercised this sprint.

### Step 8: deleted in revision 5
Rows are `Id::row` on the ReflowDocument line (Step 7.3). No separate step.

### Step 9 — DONE
`Document::Index::setNumRows` (jam_DocumentIndex.cpp:77); `Id::row` asserted at :28, :45; row count at :87, :103; Codec `getNumBytes`/`clear` (:16–17, :73, :91–93, :406–408, :460, :486).

### Step 10 — DONE
`AnsiDocument::setRows` ×2, `getRowNumber`, `getCellRange` (jam_AnsiDocument.h:78–84, .cpp:176–213), `addLine` adds `Id::row` (cpp:235–241).

### Step 11 — DONE
jam_TextEditor.cpp:253 (`AnsiDocument::setRows`), :478, :487, :824 (`getRowNumber`), :825, :1089 (`getCellRange`), :923 (`add<Document::Tokens> (Id::row)`).

### Step 12 — DONE
`jam::AttributedChar::applyToClusters` at jam_TerminalGraphicsContextText.cpp:82; no `appendSegmentedCodepoint`, `addPendingClusterCells` or `Pen` in jam_terminal/graphics.

### Step 13: MarkdownWriter — ReflowDocument in, MarkdownDocument as the oracle (revision 6)
**Scope:**
- `jam_markdown/document/jam_MarkdownWriter.h/.cpp`
- `jam_MarkdownDocument.h/.cpp`
- `jam_MarkdownBlockParser.cpp`
- `jam/cast/bimaps.md` (`## OpenBlock` rows :488–491 only)

`limit` is `int` everywhere: `AttributedChar::width` returns `int` (h:285), `lineWrap` (MarkdownWriter.h:53) and the column widths are `int`.

1. **Delete:**
   - `MarkdownDocument::isOpenBlock` (h:88, cpp:88–91).
   - `BlockParser::isOpenBlock` (h:575, BlockParser.cpp:236–247).
   - `BlockParser::OpenBlockCondition` and `openBlockConditions` (h:1083–1086 and the table body that follows).
   - bimaps.md `## OpenBlock` rows `blockquote | 6`, `listItem | 7`, `setextHeading | 8`, `table | 9` (:488–491). `leafBlocks` (BlockParser.cpp:2357–2363) keys rows 0–5 only. ARCHITECT regenerates jam in Step 16.
   - The two lambdas and the `fromUTF8` copy in `getWrappedText` (cpp:110–130).
2. **`htmlBlockConditions` (h:759–762) becomes a `jam::Function::Map<int, bool>`** built once in a static-local lambda inside `getHtmlBlockCondition` (BlockParser.cpp:1027–1042), in the `leafBlocks` shape (:2334–2353). Keys: `map::HtmlBlockType::rawText` … `anyTag`, each bound to its `isHtmlBlockTypeNStart`; `none` has no entry. The loop over `rawText..anyTag` calls `get (type, source, start, end)` and returns the first `type` that answers true. Registration `add<const std::string&, size_t&, size_t&>`; the call passes the named lvalues `source`, `start`, `end`. Delete `HtmlBlockCondition` (h:759).
3. **`static bool MarkdownDocument::isParagraph (const juce::String& text)`** — public static, in the slot `isOpenBlock` leaves (h:88). Body, the measured sandbox oracle (Paragraph.cpp:35–42): `const auto document { parse (text) }; const auto* firstChild { document.getRoot()->firstChild }; return firstChild != nullptr and firstChild->nextSibling == nullptr and *firstChild->get<int> (Id::type) == map::BlockType::paragraph;`. The parser is its only rule set.
4a. **`static juce::String MarkdownWriter::getWrappedText (const jam::ReflowDocument& reflow)`** — overload beside `getWrappedText (const juce::String&, int)` (h:146). For every line: every `Id::row` token through `reflow.getText (row)`, joined with `Chars::newline`; lines joined with `Chars::newline` (`jam::Strings`, the cpp:130 join).
4. **`static juce::String MarkdownWriter::getWrappedText (const juce::String& text, int limit)`** (h:146 `float` → `int`; cpp:106): `return getWrappedText (jam::ReflowDocument::parse (text, limit, map::LineBreakLanguage::und));`. The grid-cell caller (cpp:543) drops its `static_cast<float>`. A cell holds `Id::rawText` and re-enters the grid parser inside `| … |`, so no cell row can open a block: no oracle here. Header cells never split (cpp:541–542). Pipe tables do not change.
5. **`static int MarkdownWriter::getOpenBlockRow (const jam::ReflowDocument& reflow, const Element& line)`** — private static. For `rows = *line.get<Document::Tokens> (Id::row)` and `k` from 1 to `rows.size() - 1`: the candidate is rows `0..k`, each `reflow.getText (row)`, joined with `Chars::newline` (a `jam::Strings` local); the first `k` for which `not MarkdownDocument::isParagraph (candidate)` is returned. When every prefix is a paragraph, return `-1` (the MANIFESTO `findFirst` result-return shape). The cumulative test covers the two-row blocks (setext heading, pipe table).
6. **`getParagraphText (block) const`** (cpp:265–271):
   1. `const int limit { lineWrap - getPrefixWidth (block) }; jassert (limit > 0);`
   2. `auto reflow { jam::ReflowDocument::parse (getJoinedText (block), limit, map::LineBreakLanguage::und) };` — `getJoinedText` (cpp:94–104) stays the one-time creation of the paragraph source.
   3. For every `line` in `reflow`: `for (int row { getOpenBlockRow (reflow, *line) }; row >= 0; row = getOpenBlockRow (reflow, *line)) { reflow.setGlue (*line, line->get<Document::Tokens> (Id::row)->at (row).offset()); reflow.setRows (*line, limit); }`. Each pass removes one segment, so the loop ends. Both calls are state updates on the Model (MANIFESTO E: "A complete state replaces a complete state").
   4. `return getWrappedText (reflow);`
   5. The candidate strings are transient creation state inside `getOpenBlockRow`. No Index is built.
7. **`getView`, `getFillWidth`, `getPrefixWidth`, the table widths:** unchanged (revision 4 item 2, done).

**Validation:**
- The writer writes nothing into the markdown Document; it updates only its own ReflowDocument's state through `setGlue`/`setRows`.
- No `isOpenBlock`, no `openBlockConditions`, no hand-written `jam::LookupTable`, no `float` limit, no `fromUTF8` copy in jam_markdown's writer.
- `MarkdownDocument::parse` is the only grammar the line rule consults.
- Every paragraph row set satisfies `isParagraph` cumulatively.
- `format (format (x)) == format (x)`.

### Step 14 — DONE
`getArgumentIndex` (main.cpp:93) reads argv in place; `getArgumentsWithoutFlag` and the `jam::Array<char*>` copies are gone. SPEC §2.1/§3.3 and HELP carry the wrap rules.

### Step 15: Pixel sites — no change (revision 3 Step 10)
JUCE's own wrap and fit stay JUCE's (juce_TextLayout.cpp:447–459).

### Step 16: Deletion, data and proofs
1. **Void.** `jam_graphics/text/jam_ShapedTextOptions.h` stays; jam_graphics.h:47 includes it (KANJUT's umbrella).
2. **DONE — module table.** Both `user-modules-info.md`: `## identity` key `moduleVendor` → `vendor`; new `## framework` (`version`, `website`, `license`) between `## identity` and `## module` (jam :22–28); `## module` reduced to `name | class | title | description | dependencies | OSXFrameworks | LinuxLibs | windowsLibs | minimumCppStandard | searchpaths` (jam :32–35). Both `code.cast`: `iOSFrameworks:` line deleted. **Map ownership (ARCHITECT ruling after `spell.md:3169 (list): map line has no shape paragraph`):** the engine binds maps to shape paragraphs only (SPEC §6.5 :604–615; Model.h:1345–1357 assigns a map's `Id::shape` from `getParagraph`; `getMap` matches that ordinal, Model.h:323); an item shape line owns neither maps nor bindings (Model.h:1196 vs :1213). The wiring therefore follows the `dev/cast/cast/spell.md:29–51` pattern: the `moduleDeclaration` fence is a shape paragraph holding `vendor`/`version`/`website`/`license` plus `:::[list]:::` (jam code.cast:333–339); a new fence `module` holds `ID`/`name`/`description`/`dependencies`/`OSXFrameworks`/`LinuxLibs`/`windowsLibs`/`minimumCppStandard`/`searchpaths` (:341–350). Every block's structure column is `@code:moduleDeclaration`, `- [begin]: …`, `- [end]: …`, `- [list]: @code:module`; its list column is `- [list]: @modules:identity`, `- [list]: @modules:framework`, `- [list]: @modules:module:name=<module>` (jam :3169–3252, 21 blocks; kuassa :1107–1163, 14 blocks). Arity: the paragraph demands 1; two list lines are in excess → maps; the third is the item source. JUCE reads declaration keys into a dictionary regardless of order (JUCEModuleSupport.cmake:153–168), so the constants render first. Both freetype umbrellas: `// FreeType 2.13.3 — https://freetype.org — FreeType License (BSD-like)` outside the declaration block. cast SPEC §2.2: sync reads exactly `identity`, `module`, `ignore`; any other table belongs to the manifest. Sync never reads `## framework`, so each framework's version is its own.
3. **ARCHITECT regenerates** jam (`map::reflow` replaces `map::unibreak`; the four `OpenBlock` rows drop; `jam_document`'s declaration renders), then kuassa (module-table change), then cast. The second run of each is byte-identical.
4. Run the LineBreakTest.txt 17.0.0 harness through `ReflowDocument::parse`. The harness maps byte offsets to codepoint indices.
5. `cast --format` two times on a file that has long paragraphs and grid cells. The second run writes zero bytes.
6. **After "log sprint" only** (ARCHITECT: "i dont want to risk to run cast sync now"): `cast --sync jam → kuassa` (SPRINT-LOG :394, :405 proof); `jam_document` reaches KANJUT as `kuassa_document` through the identity pairs. Then the jfs Windows build.
7. Build cast on the forked jam tree (ARCHITECT).

## Inherited residuals — KANJUT Sprint 125 (now in JAM through Step 0)

Verbatim from kuassa/user_modules/carol/SPRINT-LOG.md:55–80 (file:line as reported there; `kuassa_` names are now `jam_`). ARCHITECT dispositions each. None is deferred by this PLAN.

1. (fixed) Measurement.cpp:60 message-thread assert on the build thread
2. Module-lifetime engine vs JUCE GUI lifetime: `create()` builds `juce::Desktop` in InitDll/bundleEntry before any `ScopedJuceInitialiser_GUI` (VST3.cpp:800/:819); host factory release runs `shutdownJuce_GUI` (`DeletedAtShutdown::deleteAll`, `MessageManager::deleteInstance`, juce_MessageManager.cpp:465-477) before ExitDll → `destroy()`, so the engine outlives the subsystem it depends on; factory re-acquire re-inits JUCE under a live engine
3. AU/LV2 (no shutdown hook): engine dies in static destruction; leak-detector counter (:1168) destroyed first → spurious leak report; `weak()`/`createMutex()` destruction order vs the holder; LV2 on Windows joins threads under the loader lock
4. (fixed) SharedInstance::create unlocked read; residual: create() at VulkanEngine.h:385 reads Desktop/NSScreen off the main thread when an AU/LV2 host calls the entry from a background thread
5. (fixed in the sprint file set) getInstance null contract; pessimistic re-checks remain outside the set: StyleDebug.h:82, ImageMatte.cpp:32
6. Step 1 mutex on every `SharedInstance<T>::getInstance()` (all generated bimaps, parser hot loops; per glyph in the D2D lane via `getGlyphAtlas()`, LowLevelGraphicsGlyphRenderer.h:246/254/262); a T whose ctor calls its own getInstance() now deadlocks instead of asserting
7. kuassa_Window.cpp:26 — `juce::DocumentWindow` base defaults `addToDesktop = true` (juce_DocumentWindow.h:105): peer exists before the Window.h:168 member join; generic-app lane can create a peer while the build thread writes `externalContextFactory`
8. VulkanEngine.h:542/:548 — the build thread writes process-global `juce::ComponentPeer::externalContextFactory`, read on the message thread at peer creation
9. (fixed) uploadDirtyAtlasSlots items; residual: `VulkanGraphics` calls up into `VulkanEngine::getInstance()` (layer reversal, :1891 shape)
10. PipelineCache.h — writes VulkanDevice MSAA state (domain); extent/budget mismatch discards a valid driver blob; restored sample count not validated; create failure silent; non-template bodies in the header; MsaaCalibration.cpp:42-43 bail-out now load-bearing
11. Calibration now measures at primary-display physical extent (e.g. 3840×2160) instead of the design editor extent; a display change invalidates the key
12. (fixed) reinitialiseDevice device name / `initialiseDevice` parameter
13. Non-opaque peer without external_memory_win32 (or failed composition create) fails `create` and retries every paint with a log line; VulkanDevice.cpp retries composition creation on every call after a failure
14. AquaticPrime.cpp:10 + Format.cpp:404-411 — consumer-side sanitise changes data shape in transit (MANIFESTO E verdict: parse or implementation); re-runs per licence check (:101/:133); AquaticPrime.cpp:31 local `"\r\n\t "` duplicates `Format::join`; `.trim()` redundant
15. Every VST3 InitDll builds the full engine (~250 ms), including in-process host scans
16. VulkanDevice.cpp — extension-present loop copied 5× (:90-101, :119-128, :134-143, :272-281, :299-308); `createLogicalDevice` ~95 lines; redundant local copied to member (:268/:283); indented `#if` (:324/:343)
17. VulkanEngine.h — 1172 lines, many responsibilities; non-template bodies in header; `createContext` > 30 lines; `getOrCreateGraphics (peer)` recomputes what `createContext` computed; dead locals on Windows (:910-911)
18. PluginEditor.h:282, Window.h:168 — `VulkanEngine&` reference members (PLAN-approved)
19. kuassa_File.h:255-262 — kuassa_core references `juce::AudioProcessor`/`PluginHostType` under a module-availability guard
20. patch: AAX include at AAX_utils:46 vs PLAN Step 13.3 "before :111" (moved to fix the `_WIN32_WINNT` redefinition warning); AU calls init twice (idempotent)
21. `moduleHooks` runs only if the static SharedCode archive links kuassa_plugin_bootstrap.cpp's object (it does today)
22. VulkanEngine not `final` while `startThread()` runs in the ctor
23. Style residue: FilterStripView.cpp:64 magic `result == 1`; VulkanTextureCache.cpp:9 `retiredTextures.clear()` in a dtor; Glyph.cpp:33 out-parameter `int& recordCursor`; VulkanGraphics.cpp:1169-1201 bail-out returns in a void function; generated CMakeLists.txt:130 spacing
24. Pre-existing risks: FilterStripProcessor.cpp:146 `setLatencySamples` on the audio thread; :199-203 callAsync captures raw `this`; FilterStripView.cpp:22 null `view` when `isReady` fails, dereferenced in PluginEditor.h:116; PluginEditorLayout.h:53 `isReady` writes the settings file; PluginEditorLayout.h 630 lines
25. (fixed in docs pass) PLAN/history leaks in comments
26. Names not in the ratified list (unverifiable without git): `compositionInteropSupported`, `interopExtensionsResult`, `externalMemoryWin32Supported`, `keyStream`, `priorCacheData`, `headerSize`, `hasPriorCache`, `priorStream`, `created`, `compositionFormat`, `compositionExtent`

## BLESSED Alignment
- **B:** Each Document owns its tokens and Elements. ReflowDocument owns its lines, segments and rows. Index owns only its B-tree.
- **L:** `jam::Function::Map` replaces the rule chains (`contextRules`, `precedingRules`, `htmlBlockConditions`). `setRows`, `setGlue`, `getText`, `getOpenBlockRow` are each one responsibility under 30 lines.
- **E:** No magic values. The vocabulary is generated (`map::reflow`). ReflowDocument knows no consumer: no callback, no predicate. The markdown grammar exists once, in the parser; the writer asks it through `isParagraph`. `jam_document` includes `jam_core` only; jam_core does not know jam_document.
- **SSOT:**
  - one width clamp (`getWidthClass`), one cluster walk (`applyToClusters`)
  - one token array per Document; one row property (`Id::row`) written by `AnsiDocument::setRows` and `ReflowDocument::setRows`
  - one span-to-text (`Document::getText (const Token&)`); no `fromUTF8` copy in a consumer
  - one markdown grammar (`MarkdownDocument::parse`); no predicate table
- **Stateless:** No carrier outside the Model. `columns`/`language` are parse inputs. The oracle's candidate strings live only inside `getOpenBlockRow`.
- **Encapsulation:** Unidirectional layers: jam_core ← jam_document ← jam_markdown; jam_core ← jam_terminal ← jam_gui. Retract is a state update the consumer tells the Model (`setGlue`, `setRows`), never an inspection the Model performs on the consumer's behalf.
- **D:** The Step 16 proofs; sandbox determinism passes at A/B.

## Risks / Open Questions
- `Document::parse` runs `preprocess`, which maps CRLF/CR/FF to LF and NUL/lone surrogates to U+FFFD (h:788–803). The Step 16.4 harness applies the same mapping to the expected breaks in LineBreakTest cases that hold those codepoints.
- Every project that adds `jam_markdown` as a JUCE module must also add `jam_document` (a JUCE module's `dependencies:` line names a target the project must provide). This PLAN edits cast's `project-info.md` only; END, TIT, CAKE, WHATDBG and CAROLINE are outside the named scope — ARCHITECT's call.
- The harness's 170/382 fixpoint mismatches from the first sandbox run, its `%s` mojibake and REPORT path bug are sandbox-only; the sandbox is not a deliverable.
- Later, per ARCHITECT's ruling type by type: moving other Document types into `jam_document` (survey: CodeDocument, XmlDocument movable now; Mermaid, OpenType, Ansi carry jam_graphics/jam_style/jam_markdown dependencies).
