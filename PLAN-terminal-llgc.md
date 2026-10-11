# PLAN: Terminal LLGC follows the Vulkan context — cast --help

**RFC:** none — objective from ARCHITECT, this session
**Date:** 2026-10-11
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + JAM (LANGUAGE.md C++/JUCE: single header preferred, split only for a real responsibility or platform)

## Context

`cast --help` renders through MarkdownComponent and the Terminal LLGC (jam Sprint 159). The render is wrong:
- a 44-cell column, centred in a fixed 100 cells;
- backgrounds on rows other than their text;
- uneven blank rows.

**Root cause, by read.** The terminal context does not follow the Vulkan context:
- It rounds every translation into a `juce::Point<int> origin` (`jam_TerminalGraphicsContext.h:77-83`, `:1006`).
  - Text gets `round(top) + round(local)` (`jam_AttributedGraphics.cpp:246`, `jam_TerminalGraphicsContextText.cpp:51`).
  - Shapes get `round(top + local)` (`jam_AttributedGraphics.cpp:224`, `jam_TerminalGraphicsContextPath.cpp:65`).
- It rounds .5 by parity (`juce_MathsFunctions.h:592-594`).
- It has no device scale (`:86`).

**The Vulkan pattern this plan follows:**
- `VulkanTransformState`, which mirrors `juce::RenderingHelpers::TranslationOrTransform` (`jam_VulkanTransformState.h:12-15`; `juce_RenderingHelpers.h:49-169`);
- the device scale, composed once at context construction (`jam_VulkanLowLevelGraphicsContext.h:49-50`);
- `VulkanGraphics::resize (width, height, float scale = 1.0f)` (`jam_VulkanGraphics.h:393`).

## Rulings (ARCHITECT, verbatim)

- *"MarkdownComponent WITH StyleMarkdown EXACTLY AS IS. LLGC translate into its own domain … only diverge at UNIT, PIXEL, BYTES, FONT"* (Sprint 159).
- *"css always em"*. *"the ISOMORPHISM stands. we never invent new pattern."*
- *"it should be specific terminal protocol agnostic"*.
- *"if you need more static table, follow the established pattern"*.
- *"24 bit and 256 colour DOES NOT MATTER"*.
- Width: *"Terminal window width"*; *"terminal column count yes"*.
  - *"use both then. column wrap IS the number to reflow. like threshold, if terminal column width wider than 100. render centered, otherwise wrap to whatever terminal column width. and define at css"*.
  - *"we still need baseline padding … at least one cell for each"*. `margin: 2em` (`markdown.css:121`) already gives this.
- Line pitch: *"Engine tells a scale"*.
- Path fills: *"Keep half-block fills"*.
- Transform layer: *"extract the abstract. check how juce organize, conform to our siblings"*.
  - Name per NAMES.md: `jam::TransformState`, nearest sibling `VulkanTransformState` without the domain prefix (Rule 5).
  - It goes to `jam_graphics/geometry/`, the module that `jam_vulkan.h:37`, `jam_pdf.h:34` and `jam_terminal.h:54` all include.
- KANJUT: *"Sync KANJUT too"*.
- Cell edges: *"Smallest container"*, which is `juce::Rectangle<float>::getSmallestIntegerContainer` (`juce_Rectangle.h:851-857`).

## Dependency & API Inventory

- **JUCE:**
  - `AffineTransform` (`scale`, `followedBy`, `transformedBy`).
  - `Rectangle<float>::getSmallestIntegerContainer`.
  - `EdgeTable`, already used by the context (`jam_TerminalGraphicsContextPath.cpp:7-14`).
  - `Font::getHorizontalScale` (`juce_Font.h:426`) is the family precedent for the name `verticalScale`.
- **jam:**
  - `jam::TransformState`: moved from `jam_VulkanTransformState.h`.
  - `jam::TerminalWinsize` (`jam_TerminalWinsize.h:34-72`; `isValid` `:60`, `getCols` `:69`).
  - `StyleMarkdown::getMetrics<float>` is public (`jam_StyleMarkdown.h:155-158`).
  - `map::MarkdownStyleSheet::lineHeight`, `margin`, `maxLineWidth`.
- **Include generation:**
  - `jam_graphics.h` and `jam_vulkan.h` include lists are generated from `jam/user-modules-info.md` `## submodule` (`jam/cast/spell.md:3230`, `:3345`).
  - Rows: Rectangle `:909`, VulkanTransformState `:1151`.
  - cast's toolchain runs jam's manifest (`cast/project-info.md:215`, `:223`).
  - `jam_terminal.cpp` includes are hand-written. Platform split: `:30-34`.
- **Name families:**
  - `getWinsize` pairs with `setWinsize` (`jam_TerminalTTY.h:312`).
  - `currentTransform` is the member name in `VulkanLowLevelGraphicsContext::State` (`:951`).
  - Platform files follow `tty/native/jam_TerminalTTY_{posix,windows}.cpp`.
- **External callers:**
  - `cast/Source/Help.h:28-44`.
  - `eve/tests/{llgc_parity,ansi_fixpoint}/main.cpp`: `resize (cols, rows)` and `drawCells`. Both stay source-compatible.

## Execution Rulings (ARCHITECT, verbatim)

- *"always watchdog your engineer. max 10m. more than 10m must be something wrong. dont let it run in endless cycle."*
  - Each Engineer runs in the background under a 10-minute limit.
  - An Engineer still running at 10 minutes is stopped (TaskStop) and its partial output is read.
  - The correction is re-delegated from this PLAN.
- *"always build with cast toolchain."* Every build is `cast cast/spell.md --debug` in `dev/cast`.
- *"no gate until /log"*. Steps 1–7 run to the logged endpoint without stopping.

## Validation Gate

COUNSELOR validates each step against MANIFESTO.md, NAMES.md, CODING.md and this PLAN before the next step: read every changed line, check the file set, names, Rulings and DCF. @Auditor runs once, after Step 7.

## Steps

### Step 1 — Extract `jam::TransformState`
**Scope:**
- `jam_vulkan/context/jam_VulkanTransformState.h` moves to `jam_graphics/geometry/jam_TransformState.h`.
- `jam/user-modules-info.md`: delete row `:1151`; add the jam_graphics row after `:909`.
- The `jam_vulkan` users: the LLGC `.h` `:951`; `.cpp` `:156`, `:280`, `:385`; `…Image.cpp:13`; `…Path.cpp:189`, `:261`, `:435`; `…PathCache.cpp:61`, `:94-101`; `…Glyph.cpp:94`. The type name changes; the member `currentTransform` stays.

**Action:** delete first, then move the body unchanged. The struct becomes `TransformState`. The compiler lists what remains.

**Validation:**
- No `VulkanTransformState` remains in jam.
- The body is byte-identical apart from the name.
- Zero includes in the submodule file.

### Step 2 — Terminal context: `TransformState` in place of `origin`
**Scope:** `jam_TerminalGraphicsContext.h`.

**Action:**
- **Members.** Delete `juce::Point<int> origin`, both the member and the `SavedState` field. Add `jam::TransformState currentTransform` in both places.
- **Constructor.** It composes `AffineTransform::scale (1.0f, engine.verticalScale)` exactly as `jam_VulkanLowLevelGraphicsContext.h:49-50` does.
- **Transform overrides.** `setOrigin`, `addTransform` and `getPhysicalPixelScaleFactor` forward to `currentTransform`, as `:66-80` does. The translation-only `jassert` is deleted.
- **`quantizeToCells`.**
  - It takes a user-space rect and maps it to device space through `currentTransform`, as the Vulkan `clipToRectangle` does at `:88`.
  - It returns `getSmallestIntegerContainer()`.
  - Every hand `translated (origin.x, origin.y)` is deleted.
- **`getClipBounds`.** It uses `deviceSpaceToUserSpace`.
- **`getCentreCell`.** It returns the cell that contains the centre: the smallest container of the device centre point.
- **`isBelowCell`.** It reads device extents.
- **`drawCells`.** It writes at device cells, with no transform. Its callers `drawGlyphs` and `drawAttributedText` pass cells they have already quantized (`jam_TerminalGraphicsContextText.cpp:61`, `:114`). A second transform would quantize twice, which breaks MANIFESTO S and D. This is a correction made during execution.

**Validation:**
- No `origin` member remains, and no `roundToInt` or `toNearestIntEdges` remains in the context.
- One quantization function exists.
- CODING CRITICAL RULES hold, and no names are added beyond this PLAN.

### Step 3 — Terminal context `.cpp` lanes
**Scope:** `jam_TerminalGraphicsContext{Path,Image,Text,Transparency}.cpp`.

**Action:**
- **Paths and images.** `fillPath`, `clipToPath`, `drawImage` and `clipToImageAlpha` compose `currentTransform.getTransformWith (t)`, as `jam_VulkanLowLevelGraphicsContextPathCache.cpp:61` does. The half-block lane stays (*"Keep half-block fills"*).
- **`drawGlyphs`.**
  - The `jassert (transform.isOnlyTranslation())` is deleted.
  - A glyph's cell is `quantizeToCells ({ position.x, position.y − baseline, advance, cellHeight })`. Its row is that rect's top and its column is that rect's left.
  - `getSegmentEnd` uses the same function.
- **`addLink` and `drawAttributedText`.** Both use `quantizeToCells`.

**Validation:**
- Every primitive goes through `quantizeToCells` or `currentTransform.getTransformWith`.
- The `jam_TerminalGraphicsEngine.h` byte writer is unchanged.

### Step 4 — Terminal engine: scale and device size
**Scope:**
- `jam_TerminalGraphicsEngine.h`.
- New `graphics/native/jam_TerminalGraphicsEngine_posix.cpp` and `…_windows.cpp`.
- `jam_terminal.cpp`, which includes them under the `:30-34` guard.

**Action:**
- **Scale.** `resize (int cols, int rows, float scale = 1.0f)` mirrors `VulkanGraphics::resize (int width, int height, float scale = 1.0f)`, including the sibling's parameter name. It stores the value in the public member `float verticalScale { 1.0f }`, beside `widthCols` and `heightRows`.
- **Device size.** Add `jam::TerminalWinsize getWinsize() const`:
  - POSIX: `TIOCGWINSZ` on the controlling terminal `/dev/tty`, with RAII ownership of the descriptor;
  - Windows: `GetConsoleScreenBufferInfo` on `CONOUT$`;
  - no terminal device: `TerminalWinsize {}`, whose `isValid()` is false.
- `eve/tests` stay source-compatible.

**Validation:**
- B: the descriptor or handle has one owner.
- Platform headers are only in the platform files.
- Names come only from this PLAN.

### Step 5 — cast
**Scope:** `cast/Source/Help.h`, `cast/Source/main.cpp`, `cast/Source/markdown.css`.

**Action:**
- **CSS.** `markdown.css:120` becomes `maxLineWidth: 100em`.
- **`printHelp (helpText)`:**
  1. style sheets, `StyleManager`, `StyleMarkdown` and the LookAndFeel, as today;
  2. the engine `{ stdout }`;
  3. the width is the `cols` field of `getWinsize()` when valid; `getCols()` returns the `cell` unit type and the component takes an `int`. Otherwise it is the smallest container of `maxLineWidth + 2·margin`, read through `StyleMarkdown::getMetrics<float>`;
  4. `verticalScale` is `1 / getMetrics<float> (lineHeight)`;
  5. `setBounds (0, 0, width, 0)`, then `setDocument`;
  6. rows are the smallest container of the content height × `verticalScale`;
  7. `engine.resize (width, rows, verticalScale)`;
  8. the `GraphicsContext`, then the paint.
- **main.cpp.** Delete the `lineWrap` chain that `--help` no longer uses (*"--line-wrap no longer affects --help"*):
  - `printBannerAndHelp (lineWrap)` and its style parse (`main.cpp:784-793`);
  - the `runDocumentNotFound` parameter (`:849-851`);
  - the `:1397` call.

**Validation:**
- `--line-wrap` still governs `--format` and generate.
- No hardcoded number: every value is read from CSS or from the device.

### Step 6 — Build validation (cast only) and KANJUT sync
**Action:**
1. Run `cast cast/spell.md --debug` in `dev/cast`. This runs jam's codegen, so the include lists regenerate. Expect 0 errors and 0 warnings.
2. Mirror to KANJUT. Run `cast --sync ~/Documents/Poems/dev/jam ~/Documents/Poems/kuassa/user_modules` in a scratchpad copy first, then in production. The second run must write 0. No KANJUT build.
   - Hand-mirror the kuassa `user-modules-info.md` rows `:1100-1101` to the `kuassa_graphics` row (precedent: jam log `:419`).
   - Regenerate kuassa `generated/` with its manifest.

**Validation:**
- The build output is clean.
- The sync reaches fixpoint.
- No `VulkanTransformState` remains in kuassa.

### Step 7 — Auditor, once; then doxygen; then log
- @Auditor sweeps Steps 1–6 against CONTRACT and this PLAN. COUNSELOR resolves every finding.
- Doxygen pass: `TransformState`, the terminal context and engine, `Help.h`, `main.cpp`, with 0 warnings. This includes the stale prose at `Help.h:4-18` and `jam_TerminalGraphicsContext.h:10-22`.
- The sprint log goes in `cast/carol/SPRINT-LOG.md` and jam's log. ARCHITECT runs `cast --help` at runtime.

## Amendment 1 — the first prompt in full; the cell law (execution)

**Scope.** The first prompt names six defects:
1. the narrow column;
2. misaligned background blocks;
3. GFM table garbage;
4. pandoc table garbage;
5. code-block garbage;
6. indent, quote and list not rendered.

Also: *"no magenta, no yellow"*. Steps 1–6 fixed only item 1. ARCHITECT: *"READ MY FUCKING FIRST PROMPT"*, *"NO GATE"*, *"so what"* (on the fractional metrics).

**Contract.**
- MANIFESTO: *"If the data does not fit the implementation, the parse is wrong or the implementation is wrong."* The em metrics are data, so the LLGC is wrong.
- The class's own cell law: *"The character is the cell's primary content; geometry … contributes colour only"* (`jam_TerminalGraphicsContext.h:10-13`).

**Corrections:**
- **Glyph cell.** A glyph goes to the cell that holds the centre of its device box (`getCentreCell`). The box is one narrow column wide (`jam::AttributedChar::narrowColumns`) and `cellHeight` tall, so a wide glyph keeps its left cell. A path fill owns a cell when its two samples cover the majority of the cell (`fillPathCells`). A rect fill takes the smallest container (`quantizeToCells`, ruling *"Smallest container"*). With cell-snapped metrics (Amendment 2), every fill and text box lands on whole cells, so the two rules give the same cells. This supersedes the earlier "column is the box's left edge", which produced the `c` and `▀` mismatch in help5.
- **Geometry yields to text.** A geometry write never replaces a character.
  - A half-block onto a half-block becomes one cell, with the foreground for one half and the background for the other.
  - Any other geometry write onto a character writes nothing.
- **Text keeps what lies under it.** Text written onto a cell keeps the whole-cell background already there (`compositeOntoCell`). Amendment 2 removed the half-block colour and the overline/underline inheritance.
- **Hairlines.** Superseded by Amendment 2: END does not draw overline or underline on blank cells, so hairlines are box-drawing glyphs.

## Amendment 2: cell snap, joined lines, fixed palette (execution)

**Rulings, verbatim:**
- *"why cant we have half cell drawing for anything"*;
- *"why keep patching instead of impelementing correctly per DESIGN BY CONTRACT"*;
- *"StyleMarkdown should have deterministic calculation to SNAP against cell"*;
- *"rounding padding? ceil"*;
- *"Block paddings only"*;
- *"A SOLID RECTANGLE with corner MEET DETERMINISTICALLY"*.

**Changes:**
- **StyleMarkdown snaps lengths to a told cell.** `setCellSize` belongs to the family of `GlyphArrangement::setCellSize`.
  - Line height, gap, indent and the metadata label column (a width) go to the nearest cell.
  - Block padding goes up (`ceil`).
  - List item padding goes to the nearest cell.
  - `Help.h` tells `(narrowColumns, lineHeight)`.
- **One transform path.** `AttributedGraphics` passes the unit transform through `g.addTransform` for shapes too. The terminal context snaps a translation-only unit origin to a whole cell, rounding half up.
- **A cell's owner is the fill that covers its majority.** A half-block marks minority coverage only. Text inherits only a whole-cell background.
- **Hairlines are box-drawing glyphs.** Each cell takes the union of the arms that meet in it (`BoxDrawing::getLines` / `getCodepoint`). A run joins the line in the cell just before its first cell.
- **Fixed palette only.** The terminal maps colours to palette indices 16–255 only, never the themed 0–15. MANIFESTO D: the output must not depend on the terminal theme.

## BLESSED Alignment

- **B:** the device descriptor is RAII. The engine owns the device facts.
- **L:** one quantization function. The shared transform layer has no duplicate.
- **E:** the scale and the width are told explicitly. No magic numbers.
- **S (SSOT):** one transform per context, one rounding rule, and one `TransformState` for every LLGC that uses it.
- **S (Stateless):** the context keeps only per-frame state.
- **E (Encapsulation):** MarkdownComponent is untouched. StyleMarkdown gains only the told cell snap (Amendment 2). `TransformState` lives in jam_graphics, below both jam_vulkan and jam_terminal, so no layer includes upward.
- **D:** each user coordinate maps to one cell, and the result is the same under any whole-cell shift.

## Risks / Open Questions

None — closed by the rulings above.

**Known effects:**
- Not built, by the standing *"you only allowed to build cast for build validation"*: EVE, END, stamp, jfs and the KANJUT tree compile against the `TransformState` rename.
- The `eve/tests` callers keep their signatures.
