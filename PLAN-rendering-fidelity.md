# PLAN: Rendering Fidelity + ColourScheme Formalization

**RFC:** none — objective from ARCHITECT prompt (session-ratified decisions)
**Date:** 2026-08-02
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE (LANGUAGE.md reference implementation; single-header preferred; 30/3 unchanged)

## Context

cast's `--help` render exposed defects (list collapse, dropped bullets, lost indent, no block spacing) and the glow/glamour survey confirmed the missing pieces already exist in jam as building blocks (TerminalPalette cube math, Char::fromCodepoint width tables, TerminalParser). ARCHITECT ratified four work items, then widened one: the colour seam — today two coexisting mechanisms (KANJUT deployed: CSS → StyleManager tables → StyleTheme via generated ColourIdMap; JAM deployed: config ValueTree → ColourMap registry → StyleMethods walk → LAF setColour) — is formalized into **one** `jam::ColourScheme` (juce isomorph: `LookAndFeel_V4::ColourScheme`, whose internal store is literally `Colour palette[]`), absorbing **both** mechanisms plus the ANSI needs (xterm-256 indexed palette, truecolor→256 quantization, depth degradation), wired through `StyleCustom`. `jam::Link` → `jam::Hyperlink` rename ratified. KANJUT is CANON — every kernel change echoes to kuassa.

## Dependency & API Inventory

- **Parser bug:** `getListItemLead` interrupt check (jam_Markdown.h:794-806) fires on any open paragraph; cmark applies it only when the full container chain matched. Driver: addLine :2177-2205, openNewBlocks :2138, openContainer :1984 (OpenFunction array must unroll), interruptTable :1996.
- **TUI emit:** `fgEscape`/`bgEscape`/`serializeRow` (jam_tui_graphics_serialize.cpp:43/104/135, 24-bit only), ANSI constants (jam_tui_escapes.h:3-35), `appendCodepointAsUTF8` (:13), `toAnsiString` (jam_tui_markdown.cpp:278-373), margin marks machinery. No isatty anywhere yet.
- **Markers/geometry:** bullet = textless listItem descriptor (jam_MarkdownLayout.cpp:488-491); ordered marker = texted (:494-502); GUI bullet = fillEllipse (jam_MarkdownDocument.cpp:259-264); descriptors carry pixel x/y (TUI discards both). Metrics: `defaultMetrics()` (jam_MarkdownLayout.cpp:26-41; listIndent 24, paragraphSpacing 0.5, quoteIndent 16).
- **Hyperlink ripple (exhaustive, Pathfinder):** jam_Link.h (all), jam_graphics.h:88, jam_Char.h:59,61,180-183,233-235, jam_MarkdownLayout.cpp:243,250,63-105,253, jam_TerminalVideo.h:877-879,1019-1034,1935-36, jam_TerminalVideo.cpp:314-435,655, jam_TerminalVideoEdit.cpp:515, jam_TerminalVideoOSCExt.cpp:92-127; kuassa: kuassa_Link.h, kuassa_Char.h:59-235, kuassa_graphics.h:88.
- **Palette:** `TerminalPalette` (jam_terminal/cell/jam_TerminalPalette.h) — 256 slots, `cubeComponent` {0,95,135,175,215,255}, cube/gray constants; consumers in jam_terminal video SGR path. kuassa has NO terminal module — palette section is new there.
- **Colour seam JAM:** ColourMap (jam_graphics/colour_map/jam_ColourMap.h, `: AnyMap`, fromValueTree/getChildWithName); StyleMethods<Derived> : StyleCustom (jam_style/jam_StyleMethods.h:19-112 — get/setColourId/setColours/contains + protected colourMap); consumers: Svg::getStyledGraphics (jam_Svg.h:516-529, .cpp:127-203), Svg::Flex::getSegments (jam_StyledGraphics.h:157, .cpp:64-93), StyleWhelmed (jam_whelmed/style/jam_StyleWhelmed.h:15). Downstream (out of repo, breaks expectedly per Refactor-Rewrite): END EventRegistration.cpp:85-120.
- **Colour seam KANJUT:** kuassa_ColourMap.h + kuassa_StyleMethods.h (dormant twins); deployed: StyleManager + StyleTheme<ManagerType> (kuassa_plugin_bootstrap/style_manager/, CSS parse → fonts/colours/appearance tables; CSS SSOT e.g. jreng-filter-strip/Source/layout/style.css), generated kuassa_ColourIdMap.h (lexicon bimap `--Component--colourId` → id symbol; stays as the CSS-name→id source).
- **Width:** `Char::fromCodepoint` (jam_Charset.cpp:32), `Char::width` (jam_CharProps.cpp:2613, 3-level tables); TerminalVideo usage jam_TerminalVideo.cpp:339-340. `getStyledLine` (jam_MarkdownLayout.cpp:219-257) bypasses it.
- **Harness:** `TerminalParser (TerminalVideo&)` + `process(bytes,len)` (jam_TerminalParser.h:149); `TerminalVideo (Cell::Rectangle, TerminalEvents&)` (jam_TerminalVideo.h:155) — headless-capable. Smoke target: sandbox/smoke (CMakeLists:26-60, corpus/, command list main.cpp:1250+).
- **Stamp flags:** bold 0x01; serializeRow already emits bg SGR; `Stamp::Entry.bg` unused by markdown restamps (code bg tint lands here).

## New Names (Rule -1 — ratified at plan approval)

- `jam::Hyperlink` (file jam_Hyperlink.h), `Char::hyperlinkId()`, `Char::withHyperlinkId()`, `TerminalVideo::activeHyperlinkId`, `stampHyperlink()` — ratified in-session
- `jam::ColourScheme` (file jam_graphics/colour_scheme/jam_ColourScheme.h) — ratified in-session; `: AnyMap`, keeps `fromValueTree` / `getChildWithName` contract
- `ColourScheme::Palette` — nested indexed store type (NO member on ColourScheme — ratified amendment: nested registry nodes carry no palette; the one mutable instance is owned by jam_terminal at Step 6): `getColour (int index)`, `setColour (int index, juce::Colour)`, static `getNearestIndex (juce::Colour)` (convention-based, deterministic) + absorbed cube/gray constants, `cubeComponent`, and private `getAnsiColours` / `getSquaredDistance` helpers + `ansiSize` constant (ratified in-session)
- `ColourScheme::addColourId (treeType, property, colourId)` — registration (verb contract: add inserts; replaces StyleMethods::setColourId)
- `ColourScheme::applyColours (juce::LookAndFeel&, const juce::ValueTree&)` — the lockstep walk (apply = push onto caller-supplied target; replaces StyleMethods::setColours)
- `ColourScheme::contains (const juce::ValueTree&, const juce::Identifier&)` — absorbed unchanged
- `jam::tui::ColourDepth` enum: `truecolor`, `palette256`, `plain`
- `jam::tui::Metrics::getColourDepth()` — platform query (isatty + COLORTERM/TERM), same shape as Metrics::getBounds
- `bullet { 0x2022 }` constant (jam_tui_markdown.cpp, box*/checkbox* family)
- parser param `allContainersMatched`

## Validation Gate

Each step validated by @Auditor against MANIFESTO.md, NAMES.md, JRENG-CODING-STANDARD.md, and this locked plan before the next step. Agents never build — ARCHITECT compiles. Engineer prompts carry: no doxygen, no PLAN/ARCHITECT/sprint-citing comments, zero identifier latitude.

## Steps

### Step 1: Parser — list interrupt scoped to matched chains
**Scope:** jam_markdown/parser/jam_Markdown.h
**Action:** `addLine` computes `allContainersMatched { continuation.depth == containers.containerLevels.size() }` before `applyContinuationResult`; threads through `openNewBlocks` → `openContainer` (unroll the 2-entry OpenFunction array into direct openBlockquote/openListItem calls) → `openListItem` → `getListItemLead`, whose check becomes `allContainersMatched and leafOpen and paragraph`. Other openContainer/openNewBlocks callers (interruptTable) pass `true`. Nothing else changes.
**Validation:** `1. a\n2. b\n3. c` shape parses as 3 items; blockquote/leaf opens untouched; no new names beyond ratified param.

### Step 2: TUI rendering fixes
**Scope:** jam_tui/markdown/jam_tui_markdown.cpp
**Action:** (a) `bullet` constant; (b) row left-pad from `toCharColumn (bounds.getX(), advance)` minus margin-mark cells; (c) pending-marker merge — marker descriptors (ordered text / textless bullet) prefix the first content row sharing their Y instead of emitting own rows (bullet colour via `elementColourId[listItem]`), flush pending marker on Y mismatch and at loop end; (d) block spacing — one blank body row when `rowY - previousRowBottom >= rowHeight * 0.5f`; (e) heading rows stamped `Stamp::bold` (+ existing headingColourId path); (f) inline-code cells gain `bg = codeBackgroundColourId` (serializeRow already emits bg SGR).
**Amendment (ARCHITECT directive, ratified in-session):** restamp passes are SSOT violations — `restampCodeAndLinkCells` and `restampSyntaxTokens` DELETED from MarkdownLayout; `getAnsiRow` resolves colour/flags in exactly one per-cell pass (precedence token > code > link > element colour, code wins over link, no Stamp-table interning), mirroring GUI `applyRowColours`.
**Validation:** no hand-rolled wrap; geometry read from descriptors only (layout stays SSOT); glyph choices match GUI isomorphs (fillEllipse→•, drawCodeTint→bg); zero `restamp` references in code.

### Step 3: Hyperlink rename
**Scope:** jam_graphics (jam_Link.h→jam_Hyperlink.h, jam_Char.h, jam_graphics.h), jam_markdown/layout, jam_terminal/video (Video.h/.cpp/Edit/OSCExt); kuassa echoes (kuassa_Link.h→kuassa_Hyperlink.h, kuassa_Char.h, kuassa_graphics.h)
**Action:** `jam::Link`→`jam::Hyperlink`, `linkId()`→`hyperlinkId()`, `withLinkId()`→`withHyperlinkId()`, `activeLinkId`→`activeHyperlinkId`, `stampLink`→`stampHyperlink`; file renames via Engineer (`rm` old after new in place); include lines updated; header prose updated. Exhaustive site list in inventory.
**Validation:** zero remaining `jam::Link`/`linkId` (grep; markdown `Id::links`/`linkColourId` vocabulary stays); kuassa byte-parallel.

### Step 4: jam::ColourScheme core (delete first)
**Scope:** NEW jam_graphics/colour_scheme/jam_ColourScheme.h; DELETE jam_graphics/colour_map/jam_ColourMap.h and jam_terminal/cell/jam_TerminalPalette.h (Engineer `rm`); jam_graphics.h + jam_terminal.h include lists; kuassa: NEW kuassa_graphics/colour_scheme/kuassa_ColourScheme.h, DELETE kuassa_ColourMap.h, kuassa_graphics.h
**Action:** `struct ColourScheme : AnyMap` — absorbs ColourMap verbatim (fromValueTree, getChildWithName) + absorbed-from-StyleMethods `addColourId`/`applyColours (juce::LookAndFeel&, const juce::ValueTree&)`/`contains` + nested `struct Palette`, public member `palette` (absorbs TerminalPalette storage/constants/`cubeComponent`; surface renamed `getColour`/`setColour`; adds deterministic `getNearestIndex (juce::Colour)` — inverse cube walk + ANSI-16/gray comparison).
**Validation:** single header per LANGUAGE.md; absorbed code behavior-identical beyond names; kuassa parallel identical.

### Step 5: JAM consumer re-seam
**Scope:** jam_Svg.h/.cpp, jam_StyledGraphics.h/.cpp, jam_style/jam_StyleMethods.h (DELETE), jam_style/jam_StyleCustom.h, jam_style.h, jam_whelmed/style/jam_StyleWhelmed.h
**Action:** `const ColourMap&` params → `const ColourScheme&` (Svg::getStyledGraphics ×3, Svg::Flex::getSegments); StyleMethods deleted — StyleCustom gains protected member `ColourScheme colourScheme`; StyleWhelmed derives StyleCustom directly, registration becomes `colourScheme.addColourId (...)` + `colourScheme.applyColours (*this, tree)`. END breakage expected downstream (ARCHITECT migrates END separately — Refactor-Rewrite).
**Validation:** zero ColourMap references in jam; Svg flow behavior identical (same int-entry lookups).

### Step 6: jam_terminal palette re-seam
**Scope:** jam_terminal video SGR/palette consumers (former TerminalPalette sites)
**Action:** SGR 256 resolution reads `ColourScheme::Palette::getColour (index)`; palette mutation (OSC) via `setColour`. If the old surface was process-static, keep a process-static `ColourScheme::Palette` in jam_terminal with identical linkage (no behavior change; Auditor flags the exact shape found for ARCHITECT visibility).
**Validation:** SGR 256/palette behavior byte-identical; no jam_terminal → jam_style dependency introduced.

### Step 7: KANJUT StyleManager re-seam
**Scope:** kuassa_plugin_bootstrap/style_manager/kuassa_StyleManager.h, kuassa_StyleTheme.h, kuassa_StyleMethods.h (DELETE), kuassa_style.h, kuassa_StyleCustom.h
**Action:** StyleManager keeps CSS parsing/fonts/metrics/window; its colour + appearance tables become a `kuassa::ColourScheme` it owns; StyleTheme `setAppearance` dispatch routes through the scheme; generated ColourIdMap remains the CSS-name→colourId source feeding `addColourId`. kuassa_StyleCustom gains `colourScheme` member (parity with Step 5).
**Validation:** style.css dual-render SSOT untouched; plugin colour resolution equivalent; kuassa_StyleMethods gone.

### Step 8: ANSI degradation wiring
**Scope:** jam_tui (jam_tui_metrics.h, jam_tui_graphics_serialize.cpp, jam_tui_markdown.cpp), jam_tui.h
**Action:** `ColourDepth` enum + `Metrics::getColourDepth()` (not isatty(STDOUT_FILENO) → `plain`; COLORTERM contains "truecolor"/"24bit" → `truecolor`; TERM contains "256" → `palette256`; else `truecolor` on tty — current behavior preserved); `fgEscape`/`bgEscape` gain depth param — `palette256` emits `38;5;N`/`48;5;N` via `Palette::getNearestIndex`, `plain` emits nothing (serializeRow skips style SGR entirely); depth resolved once per `getLines()`/`toAnsiString` call (calculation input, never stored).
**Validation:** truecolor output byte-identical to today; `plain` output contains zero ESC bytes.

### Step 9: Codepoint-width wiring
**Scope:** jam_markdown/layout/jam_MarkdownLayout.cpp (getStyledLine)
**Action:** cell stamping consults `Char::width (codepoint)` — width-2 codepoints stamp `Char::doubleWidth` (narrow/TUI path only; the `wide` param's proportional GUI semantics unchanged).
**Validation:** ASCII output unchanged; wide-rune cells stamped wide (asserted by Step 11).

### Step 10: cast width policy
**Scope:** cast/Source/Help.cpp
**Action:** named constants — width cap 120 cols, document margin 2 cols; `widthCols = jmin (terminal cols, cap) - margin * 2`; rows padded by margin (glow policy translation).
**Validation:** constants named, no magic numbers; SPEC.md §9 behavior unchanged.

### Step 11: Round-trip harness
**Scope:** jam sandbox/smoke (main.cpp new command, corpus fixture)
**Action:** smoke command `ansi`: markdown fixture → parse → MarkdownDocument → `toAnsiString` (forced truecolor depth) → `TerminalVideo { {80,24}, events } + TerminalParser::process` → assert cell-grid facts (bullet glyph cells, bold flag on heading cells, indent columns, wide-rune double cells, code bg styleId). TerminalEvents wired as existing smoke terminal usage does — Engineer verifies constructibility and reports discrepancy, no improvisation.
**Validation:** assertions semantic (cell grid), not byte-compare; fixture in corpus/ per existing shape.

### Step 12: Final audit + ARCHITECT build
**Scope:** all touched files, both repos
**Action:** @Auditor full sweep (contracts + this plan + KANJUT parity file-by-file); residuals verbatim. ARCHITECT builds: cast, jam smoke, one kuassa plugin (StyleManager path). `./cast` and `./cast --help` visual check.
**Validation:** zero unresolved findings; [N7] closes with this.

## BLESSED Alignment

- **B:** palette ownership explicit (scheme on StyleCustom; terminal static only where it already was); Hyperlink table ownership unchanged (Instance chain).
- **L:** deletes ColourMap, StyleMethods, TerminalPalette as separate surfaces — one ColourScheme; no speculative slots (no juce semantic-role enum we don't need).
- **E:** depth is a visible parameter, not hidden global; degradation named per level; no bail-outs; marker merge reads descriptor geometry, no re-derived state.
- **S (SSOT):** one colour store for GUI/SVG/ANSI; cube math exists once; layout geometry remains the single wrap truth; CSS file remains the single design truth feeding the scheme.
- **S (Stateless):** ColourDepth resolved per call as calculation input; pending-marker is loop-transient.
- **D:** quantization deterministic (fixed cube walk); parser fix converges on cmark reference behavior.

## Risks / Open Questions

- Heading TUI presentation set to bold + headingColourId (no hash-prefix re-emission) — amendable at approval.
- END (out-of-repo consumer) breaks at Step 5 by design; its migration is ARCHITECT-scheduled.
- Step 6 static-vs-member palette in jam_terminal preserves current linkage; Auditor reports the exact shape found.

## Verification

ARCHITECT builds cast + jam smoke + one kuassa plugin. `./cast` (no args) renders banner + spec: 3-item lists with markers, bullets, indent, block spacing, bold headings, code bg tint. `./cast --help | cat` emits zero ESC bytes (plain depth). Smoke `ansi` passes cell-grid assertions. KANJUT plugin renders unchanged.
