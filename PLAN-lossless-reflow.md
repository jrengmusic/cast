# PLAN: Lossless Reflow — `cast --format` round trip is byte-exact

**RFC:** none — objective from ARCHITECT prompt (cast/RFC.md is the unrelated Windows MSVC activation RFC)
**Date:** 2026-09-26
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE / JAM (LANGUAGE.md §C++/JUCE — header-only preferred ≈300 LOC; 30-line / 3-branch rules unchanged)
**Destination after approval:** copied verbatim to `C:/Users/jreng/Documents/Poems/dev/cast/PLAN-lossless-reflow.md` (precedent: `PLAN-sync-unibreak-wrap.md` at the cast root, cast SPRINT-LOG.md:117, :191)

---

## Context

`cast --format project-info.md` at `## format` `value | 40` (jfs project-info.md:10) split 25 value cells across rows. The reader joined them back with a space (backtick span, jam_MarkdownInlineParser.cpp:1390), a newline (plain cell and fence, jam_MarkdownBlockParser.cpp:1485, :1431), or a cut (fence rows re-split per line, jam_MarkdownWriter.cpp:630-631). CMake now fails at `ICON_BIG "…/Source/ icons/aksara_512pt.png"` (CMakeLists.txt:251); seven `target_compile_definitions` values are cut mid-token (CMakeLists.txt:316-321); `ProjectInfo.h:44` `publicKey` carries eight `\n`.

ARCHITECT's rulings this session, verbatim:
- *"IT MUST BE DETERMINISTICALLY LOSSLESS."*
- *"reflow contract stands, always detect, always reflow CORRECTLY, not to fucking drop it because it cant handle single liner."*
- *"what if the fence carry whitespace? genuiene break opprotunity?"* — break opportunities are honoured everywhere, fence included.
- Fence: *"Authored newlines survive"* (AskUserQuestion). Plain cell: *"newline as data (lossless) only for fence. toLiteral only carry whitespace"*.
- Carrier: *"Backslash continuation, parity law"* (AskUserQuestion).
- Paragraphs: *"Never cut a paragraph token"* (AskUserQuestion).
- Data: *"long lines wrapped single liner should not be using backtick at all, but just use toLiteral"* — publicKey leaves the fence.
- SPEC: *"why the fuck you keep referring SPEC as the immutable law? what is our objectives?"* — SPEC.md and HELP.md are rewritten to the law below.

Approval, verbatim: *"i give you permission to build. fix it until jfs can built clean. that also means publicKey must be exactly LOSSLESS, otherwise the license would be potentially broken with lossy reflow. no gate until plan completion, with execption there is discrepancy ARCHITECT need to decide which is not answerable by READING THOROUGHLY. DESIGN BY CONTRACT."* and *"always run cast --sync in scratchpad, never in production repo until you 100% achieve the lossless roundtrip"*.

Consequences: Step 7 builds cast (Engineer, permission granted); Step 9 `--sync` runs against scratchpad copies of both roots until Step 7's proofs are byte-identical; the production `user_modules` sync happens only after that proof; Step 8 ends with a clean jfs CMake configure and build.

Invariant this plan delivers: for every width w, `read (format (doc, w)) == read (doc)`, and `format (format (doc, w1), w2) == format (doc, w2)`.

---

## The Law (locked)

**L1 — Reflow contract (unchanged).** A cell in a `## format`-named column reflows at that width: break at UAX #14 opportunities (jam_ReflowDocumentRules.cpp); a segment with no opportunity that is wider than the width is cut by codepoint display width (jam_ReflowDocument.cpp:48-74, :167-171).

**L2 — Row-end backslash run, by parity.** Every physical line of a grid cell ends (before fill) in a backslash run of written length `w`. With `a` = the authored trailing backslash count of the content on that row:
- writer boundary (cut or break): `w = 2a + 1` — odd. The break whitespace stays in the row, before the run.
- authored newline (fence only) whose content ends in whitespace or in a backslash: `w = 2a + 2` — even, ≥ 2.
- authored newline with nothing to protect: `w = 0`.

Reader inverse, per cell line: count the trailing run `w` after `trimEnd` (`trimEnd` stops at `\`, BlockParser.cpp:99-116, so protected whitespace survives): odd → drop `(w+1)/2` backslashes, keep `(w-1)/2`, join to the next line by nothing; even ≥ 2 → drop `w/2 + 1`, keep `(w-2)/2`, join by newline; 0 → join by newline. This is the pipe escape law (SPEC.md:199-201, jam_MarkdownWriter.h:127-130, `getTrailingBackslashCount` BlockParser.cpp:1292-1321) extended from "before a pipe" to "at a row end".

**L3 — Leading flank.** The first space after a cell's opening `|` is padding; every byte after it up to the content is content. A hand-authored `|this|` has no flank and loses nothing. Fence rows and code-span rows are rendered left-aligned; column alignment applies to prose cells only.

**L4 — Fence is verbatim.** Authored newlines, indentation, trailing whitespace and trailing backslashes in a fenced cell survive by L2 + L3. The fence content reaches `ReflowDocument` as one document whose mandatory breaks are the authored newlines only (never a prior run's rows).

**L5 — Plain cell.** A plain (non-fence) cell carries no newline as data. Its authored soft line breaks fold to a space before reflow (existing `getSourceText`, jam_MarkdownWriter.cpp:341-373). Every row boundary the writer makes is L2-odd. The reader never sees an unmarked boundary from the formatter in a plain cell; a hand-authored unmarked boundary reads as today (newline, SPEC.md:215 — `toComment` folds it, cast Transforms.h:32).

**L6 — Paragraph.** `--line-wrap` breaks at UAX #14 opportunities only. A token wider than the wrap stands whole on its own line. No cut, no marker; the CommonMark hard break (`\`+newline, jam_MarkdownInlineParser.cpp:376-388) keeps its meaning.

**L7 — Code span.** A code span in a cell never sees a writer boundary after L2 (the reader joins rows before the inline pass). `addCodeSpanElement`'s newline fold (:1390) then applies only to authored newlines, which SPEC.md:189 forbids — no change to that function.

---

## Dependency & API Inventory

**JAM (`C:/Users/jreng/Documents/Poems/dev/jam`)**
- `jam::ReflowDocument` — jam_document/document/jam_ReflowDocument.h/.cpp, jam_ReflowDocumentRules.cpp. Rows are `Id::row` source spans (h:49-50); `setRows` (cpp:161-193); `getSegment` trims `trailingClasses` (cpp:26-41; Rules.cpp:50-52); `getRows` codepoint cut (cpp:48-74); `Id::line`/`Id::row`/`Id::tokens` (jam_Identifiers.h:744, :1060, :1351).
- `jam::MarkdownWriter` — jam_markdown/document/jam_MarkdownWriter.h/.cpp. Cell source is `Id::rawText` (cpp:50-52); `getWrappedText (const ReflowDocument&)` (cpp:94-120); `getPaddedText` (cpp:181-191, `cellFlankSpaceWidth { 1 }` h:68); `getRowText`/`getGridRowText` (cpp:193-230); `getCellReflows` (cpp:615-639); `setCellRows` (cpp:641-652); `getClampedColumnWidths` (cpp:678-697); `getWrappedGridTableText` (cpp:788-802); `getParagraphText` (cpp:375-391); `getAlignmentPadding` (cpp:164-179).
- `jam::MarkdownDocument::BlockParser` — jam_MarkdownBlockParser.cpp. `trimStart`/`trimEnd` (:85-116); `getTrailingBackslashCount` two overloads (:1292-1321); `getTrimmedSpans` (:1437-1468); `getCellText (spans)` (:1470-1486); `addTableCell` (:1488-1520); `addCells` (:1549-1595); `addColumnSpans` (:1597-1625) — precedent for a rewritten cell line: `document.addSource (…)` appendix span (:1615-1620); `splitCellSpans` (:1627-1654); `addCellLines` (:1656-1676); `addTableRow` (:1678-1720); `joinIntoString` (:160-180); `closeCodeBlock` (:182-202).
- `jam::MarkdownDocument::InlineParser` — `addCodeSpanElement` (:1383-1402), `addLineBreakElement` (:1404-1434), lineBreak tokens (:283-293, :376-388). Unchanged.
- `Chars::backslash`, `Chars::space`, `Chars::tab`, `Chars::newline`, `Chars::pipe` (jam_Chars.h — used at Writer.cpp:53, :61; BlockParser.cpp:91-93, :1297).
- `jam::AttributedChar::width` (Writer.cpp:91, ReflowDocument.cpp:45, :59).
- No test harness exists in the jam repo (Pathfinder D1: none found). No doxygen XML index (D2: none found) — doxygen prose is written last per Code Hygiene; the header prose that changes is listed per step.

**cast (`C:/Users/jreng/Documents/Poems/dev/cast`)**
- `Processor::format` constructs `jam::MarkdownWriter { maxTableWidth, lineWrap, columnWidth }` (Processor.h:83-112, :95). No cast code change is required by L1–L7; cast changes are contract text, rebuild, sync.
- Contract text to rewrite: SPEC.md:184-201 (§3.2 literals, padding), :203-229 (§3.3 Formatter), :838-866 (§6.11 `## format`); Source/HELP.md:104-125 (backtick/fence/padding), :26-30 (`--line-wrap`).
- Manifest lane: `cast/spell.md:1-7` `## format` (`comment | 40`) — unchanged.
- Sync: `cast --sync <source-root> <target-root>` (HELP.md:36-40); last sprint synced jam → kuassa `user_modules` (cast SPRINT-LOG.md:152); paths must be `C:/…` under MSYS (:156).

**jfs (`C:/Users/jreng/Documents/Poems/kuassa/jreng-filter-strip`)**
- `project-info.md` `## format` `comment | 40`, `value | 40` (:3-11) — kept as authored (cast SPRINT-LOG.md:129).
- `## project info` has a `format` column (:34); `toLiteral` rows at :36-72. `## cmake`, `## include`, `## layout glob`, `## define`, `## source`, `## signing` have no `format` column — their literals are backtick spans (HELP.md:110 "A backtick **is** the `toLiteral` operation").
- Regenerate: `cast --format project-info.md` then `cast project-info.md` (jfs SPRINT-LOG.md:14; HELP.md:17-21).
- `publicKey` consumer: `Format::join (publicKey)` (kuassa_aquatic_prime/AquaticPrime/AquaticPrime.cpp:10).

---

## Validation Gate

Each step is validated by COUNSELOR before the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked law L1–L7 (no deviation, no scope drift). @Auditor runs ONCE, after Step 9, covering the whole sprint.

Every @Engineer prompt carries: Design by Contract per CODING.md CRITICAL RULES; the MANIFESTO **E** MVP data-flow contract verbatim; doxygen-first instruction; no comments/doxygen (Code Hygiene) except Step 10; the exact file set; zero names beyond those ratified in this plan.

**Names for ratification (NAMES.md Rule -1; all join the `get`/`add`/`is` families, Rule 5):**
- BlockParser: `getContinuedSpans` (returns the cell's per-line spans with L2-marked lines joined into appendix spans), `getRowEndBackslashCount` (the `w` of a trimmed line), `isContinuedLine` (odd `w`).
- MarkdownWriter: `getRowEndText` (a row's content plus its L2 run), `getContinuationText` (the odd run for a writer boundary), `getTerminationText` (the even run for a protected authored newline), `getFenceRows`.
- ReflowDocument: none new — the whitespace-carrying row span is read from the source between consecutive rows.

---

## Steps

### Step 1 — Reader: L2 detection and join (BlockParser)
**Scope:** `jam_markdown/document/jam_MarkdownDocument.h` (declarations only), `jam_markdown/document/jam_MarkdownBlockParser.cpp`.
**Action:**
1. Add `getRowEndBackslashCount (const std::string& source, size_t start, size_t end) noexcept` — `trimEnd` then `getTrailingBackslashCount (source, start, trimmedEnd)` (existing overload :1306-1321).
2. Add `getContinuedSpans (const jam::Array<Document::Span>& spans) const` — walks a cell's per-line spans in order; for each line computes `w`; odd → append content minus `(w+1)/2` backslashes to the pending join buffer, continue; even ≥ 2 → append content minus `w/2 + 1`, close the line; 0 → close the line. A closed line whose bytes are not one contiguous source span is appended via `document.addSource` and spanned over the appendix (pattern: `addColumnSpans` :1611-1621). Returns the per-logical-line spans.
3. `addTableRow` (:1678-1720) passes `getContinuedSpans (cellSpans.at (column))` to both `getCellText` and `addTableCell` in place of the raw per-line spans. `getCellText` (:1470-1486) and `addCellLines` (:1656-1676) are unchanged: they now receive logical lines.
4. L3: in `addCells` (:1549-1595) and the final-cell branch (:1587-1590), replace `trimStart` on the cell's leading edge with "advance past exactly one `Chars::space` if present" — the flank. `trimEnd` stays (fill), and stops at the L2 run by construction.
**Validation:** `getContinuedSpans` is a `get` that returns and does not store (NAMES Verb Contract); the appendix pattern is the existing one (MANIFESTO E — extend the established pattern); no bail-out guards; `.at()` on every index; no new struct; 30-line functions.

### Step 2 — Writer: whitespace-carrying rows (ReflowDocument consumer side)
**Scope:** `jam_markdown/document/jam_MarkdownWriter.cpp`, `.h` (declarations).
**Action:**
1. Add `getRowEndText (std::string_view source, const Document::Token& row, const Document::Token* nextRow, bool isWriterBoundary)` — the row text is `source.substr (row.offset(), (nextRow ? nextRow->offset() : row.offset() + row.length()) - row.offset())` so the break whitespace between two rows of one `Id::line` is kept (rows are source spans, jam_ReflowDocument.h:49-50; segments drop trailing spaces, cpp:32-33, so the gap between rows is exactly the consumed whitespace). Append `getContinuationText (a)` for a writer boundary (`\` × (2a+1)), `getTerminationText (a)` for an authored line end that ends in whitespace or backslash (`\` × (2a+2)), nothing otherwise; `a` = authored trailing backslash count of the row content (`MarkdownDocument::BlockParser::getTrailingBackslashCount (const juce::String&)` :1292-1304 is the existing counter — expose it or call the `std::string` overload).
2. `getWrappedText (const jam::ReflowDocument&)` (:94-120) builds cell text via `getRowEndText`: within one `Id::line`, every row but the last is a writer boundary; the last row of a line is an authored line end. Newlines between rows/lines stay as the row separators for `getGridRowText`.
3. Paragraph path (`getParagraphText` :375-391) does NOT use L2 — L6. It keeps `getWrappedText` behaviour for rows joined by newline with no markers; see Step 4 for the no-cut rule.
**Validation:** rows are read in place from the ReflowDocument source (no copy of Model data beyond the rendered string — rendering is the materialisation); `getRowEndText` returns, stores nothing; constants named (no literal `2`, `1` — `continuationRunOffset { 1 }`, `terminationRunOffset { 2 }` as `static constexpr` in the .h next to `cellFlankSpaceWidth`); 3-branch rule: the three L2 cases are a `Function::Map` or a two-branch decision + fallthrough — Engineer reports the shape, COUNSELOR checks.

### Step 3 — Writer: fence cell as one document, left-aligned
**Scope:** `jam_MarkdownWriter.cpp` `getCellReflows` (:615-639), `getPaddedText`/`getRowText` (:181-213), `setCellRows` (:641-652).
**Action:**
1. `getCellReflows`: fenced cell → `ReflowDocument::parse (getView (text), limit…)` stays; `text` is now `Id::rawText` whose newlines are authored only (Step 1 joins writer rows before rawText is stored, BlockParser.cpp:1500-1501). The fence delimiter lines (``` ``` ```) are rows of that document too — they carry no L2 run (they are authored lines with nothing to protect).
2. L3 left-alignment: a cell whose `Element` has a code-block child (`isFenced`, :627-628) or a `Id::code` child renders through the unaligned branch of `getPaddedText` regardless of column alignment. Thread `isFenced` per cell to `getRowText` (the existing `cells`/`alignment` arrays are parallel; add a parallel `jam::Array<bool>` — or pass alignment as empty string for such cells at the call site in `getGridRowsText` :757-786, which needs no new type). Engineer uses the empty-alignment route.
**Validation:** no new struct (CODING.md fake-carrier rule); alignment decision made once at the owner (`getGridRowsText`), not re-checked downstream (MANIFESTO D).

### Step 4 — Writer/Reflow: paragraphs never cut (L6)
**Scope:** `jam_markdown/document/jam_MarkdownWriter.cpp` `getParagraphText` (:375-391); `jam_document/document/jam_ReflowDocument.h/.cpp` only if a no-cut entry point is needed.
**Action:** `setRows (line, newColumns)` splits over-wide segments first (cpp:167-171). For paragraphs the first pass must not run. Read `setRows` for the smallest change: the split pass is conditional on `newColumns > 0`; a paragraph call passes its limit for the packing pass and must skip the split. Preferred shape (no new name): `ReflowDocument::setRows (Element&, int newColumns, int cutColumns)` where `cutColumns <= 0` disables the split pass and cells pass `cutColumns = newColumns`; `parse (text, newColumns, newLanguage)` keeps its current signature and `build()` calls `setRows (line, columns, columns)`. `getParagraphText` re-rows with `cutColumns = 0`. Engineer reads the header prose (h:40-55) before changing it.
**Validation:** signature change is exhaustively applied (compiler is the gatekeeper); no boolean flag parameter — an int width that is 0 is the existing idiom (`newColumns` of 0 or less, h:48); doxygen for the new parameter deferred to Step 10.

### Step 5 — ReflowDocument: no change unless Step 2/4 need it
**Scope:** `jam_ReflowDocument.h/.cpp`.
**Action:** COUNSELOR reads Step 2's final diff; if `getRowEndText` can read the whitespace gap from consecutive `Id::row` spans as written above, ReflowDocument is untouched apart from Step 4. If Engineer finds the gap is not the consumed whitespace in some case (e.g. a zero-width-space break), Engineer stops and reports the case with file:line — no guard, no fix without COUNSELOR.
**Validation:** report only.

### Step 6 — cast contract: SPEC.md and HELP.md
**Scope:** `dev/cast/SPEC.md`, `dev/cast/Source/HELP.md`.
**Action:** Rewrite to L1–L7:
- SPEC.md:184-186 stays ("verbatim data … no change") and becomes the governing sentence; :188-191 fence multi-line law stays.
- SPEC.md:193-197: padding is one flank space after `|` and the fill before the closing `|`; leading content whitespace after the flank is data; the "codepoint token" sentence for edge spaces is removed for fences and kept only where true (trailing whitespace before a plain-cell row end that carries no L2 run).
- SPEC.md:199-201: the pipe parity law is generalised: a backslash run at a row end is read by L2; before a pipe as today.
- SPEC.md:203-229 §3.3: replace :221-225 with L1, L2, L4, L5, L6; keep :207-213; add the two invariants (`read∘format` and `format∘format`).
- SPEC.md:838-866 §6.11: :855-860 replaced — a split never changes the value; the author's width is layout only.
- HELP.md:104-125: L2/L3/L4 stated in the author's terms with one example of a wrapped long literal and one of a fence row ending in `\`; HELP.md:26-30 `--line-wrap`: a token wider than the wrap stands whole.
**Validation:** ASD-STE100; every sentence traces to a law line; no behaviour claim without the jam file:line in this plan.

### Step 7 — Build cast and prove the invariant (ARCHITECT builds; Engineer runs proofs on ARCHITECT's word)
**Scope:** no source files. Scratch files under the session scratchpad only.
**Action:** After ARCHITECT builds `cast.exe`, Engineer runs, on a scratch copy of jfs `project-info.md` (post Step 8) and of `cast/spell.md`:
1. `cast --format` three times → md5 identical (fixpoint, as cast SPRINT-LOG.md:152).
2. Round trip: `cast <manifest>` output before and after `--format` at widths 20, 40, default → generated files byte-identical (`read∘format == read`).
3. Fence proof: a scratch table with a fenced cell holding indentation, a trailing space, a trailing `\`, and a 100-char spaceless line, at width 20 → formatted, read, regenerated: byte-identical.
**Validation:** Pathfinder verifies the md5 lines and diffs (read-only); COUNSELOR reports numbers only.

### Step 8 — jfs data repair
**Scope:** `jreng-filter-strip/project-info.md` only.
**Action:** Restore the 25 damaged value cells to their original bytes (the writer re-splits them under L2 on the next `--format`). Exact edits:
- :46-47 `productWebsite` → one line `` `https://www.kuassa.com/products/jreng-filter-strip` ``.
- :59-68 `publicKey` → plain cell, `format` = `toLiteral`, value = the eight hex rows joined by nothing (258 chars): `0xC9459050541DBEB172AABC2F173609D7E9931196D877D7DEEC5003DA7490519CC7B747C5D0D13181058138ACA514E5CDCEC36CBBCD1E9BA5FE980028190566922731C895CD1353BE03EEBC5EA38615B205735B0B02ABD7128A0E3E96A6BB3CCB153BB03A37AE76519AFC940A5314A6AD8699D52A4F61C536362E0391C2D5F13D` — ARCHITECT's ruling "just use toLiteral".
- :95-96 `companyCopyright` → `© 2010-2025. PT Kuassa Teknika. All rights reserved.` (one space at the join — the writer broke at a space; the run length before the break is not recoverable from the working tree; ARCHITECT confirms one space).
- :112-113 `icon` → `` `${CMAKE_CURRENT_SOURCE_DIR}/Source/icons/aksara_512pt.png` ``.
- :142-143 `qaDirectory` → `` `$ENV{HOME}/Documents/Poems/dev/___builds___` ``.
- :179-180 `identity` → `Developer ID Application: Bayu Ardianto \\\\(9BDSN9TDX3\\\\)` (one space at the join — ARCHITECT confirms).
- :429-431 `audioProcessorRegistration` → `Source/FilterStripAudioProcessorRegistration.cpp`.
- :483-499 six `## define` values → each generator expression on one line, joined by nothing (e.g. `` `$<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN=1>` ``).
- `## define` `jucePluginAaxDisableDefaultSettingsChunks` → `JucePlugin_AAXDisableDefaultSettingsChunks=1` on one line (found by the Step 7 round trip: CMakeLists.txt:307-308 carried `Chun` / `ks=1`; missed in the first inventory). 26 damaged cells in total.
- :509-516 three `## include` values; :524-552 nine `## layout glob` values → each path on one line, joined by nothing.
Then `cast --format project-info.md` (Engineer, after Step 7's binary), then `cast project-info.md` to regenerate `CMakeLists.txt` and `Source/generated/ProjectInfo.h`.
**Validation:** COUNSELOR reads CMakeLists.txt:251, :316-321, :330-332, :486-495 and ProjectInfo.h:44-45 — no `/ `, no mid-token space, `publicKey` one string with no `\n`. `## format` widths unchanged (:3-11).

### Step 9 — Sync jam → kuassa user_modules
**Scope:** `cast --sync C:/Users/jreng/Documents/Poems/dev/jam C:/Users/jreng/Documents/Poems/kuassa/user_modules` (paths as `C:/…`, cast SPRINT-LOG.md:156). Run on ARCHITECT's word after Step 8 proves.
**Validation:** rerun and reverse both report zero writes (precedent: cast SPRINT-LOG.md:152).

### Step 10 — Doxygen pass (post-audit)
**Scope:** headers touched in Steps 1–4 only: `jam_MarkdownDocument.h` (new BlockParser declarations; `getCellText`/`addCellLines` prose :906-915, :1008-1019), `jam_MarkdownWriter.h` (new declarations; `getWrappedText` prose :162-168), `jam_ReflowDocument.h` (`setRows` prose :40-55 if Step 4 changes the signature).
**Action:** header-only doxygen, zero warnings, `@param` matches signatures. No doxygen regeneration target exists on this machine (cast SPRINT-LOG.md:159) — prose is proofread by COUNSELOR.
**Validation:** CODING.md Doxygen Discipline.

---

## Execution Record (what landed, beyond the step text)

Reader — jam_MarkdownBlockParser.cpp / jam_MarkdownDocument.h / jam_MarkdownDocument.cpp / jam_MarkdownInlineParser.cpp:
- `getRowEndBackslashCount`, `isContinuedLine`, `getSpanContentEnd` (approved during execution, Rule 5), `getContinuedSpans`; constants `continuationRunOffset`/`terminationRunOffset` in the BlockParser section.
- `addCell` extracted from `addCells` (DCF §5: 49-line function + `continue` bail-out); L3 flank rule lives in `addCell`.
- `addColumnSpans` escaped-cell branch: `.trim()` → flank rule + `trimEnd()` (proof round 3/4 finding).
- `addTableCell`: whole-text `.trim()` before `Id::rawText` removed (same defect class).
- `getTrailingBackslashCount (const juce::String&)` moved to `MarkdownDocument` public static (definition in jam_MarkdownDocument.cpp beside `isParagraph`); writer's duplicate deleted.
- `MarkdownDocument::isTableCell` public static; `addParagraphLine` skips `trimStart` and `InlineParser::getLeafText` (new, `Element&` — the `leafText` map key is `Element*`) skips `trimEnd` when the paragraph's owner is a table cell (proof round 4 finding: CommonMark paragraph edge trim destroyed protected cell whitespace).

Writer — jam_MarkdownWriter.h/.cpp:
- `getContinuationText`, `getTerminationText`, `getRowEndText (source, contentStart, contentEnd, isContinued)`, `getLineRows` (per-line `[lineStart, lineEnd)` spans so leading indentation and trailing whitespace of an authored line are rendered), `getCellText (const ReflowDocument&)` overload, `getRowAlignment` (literal cells left-aligned; `Id::code` is a grandchild of the cell via its paragraph).
- `getRowEndText` REPLACES the authored trailing run (drops `a`, writes `2a+1` / `2a+2`) — proof round 2 found append gave a total of `3a+1`.
- `getWrappedWidth` measures rendered rows (run + carried whitespace).
- `setCellRows` deleted (call and function): rows pack at the named width; the clamp only widens the column (oscillation otherwise).

ReflowDocument — jam_ReflowDocument.h/.cpp: `setRows (Element&, int newColumns, int cutColumns)`; `build()` passes `columns, columns`; paragraphs pass `0` (L6).

Contract — cast SPEC.md §3.2, §3.3, §6.11 and Source/HELP.md (command line, cell law, `### format`) rewritten to L1–L7.

Proof (scratchpad, five rounds): fixpoint in one pass at widths 20/40/80/none; generated CMakeLists.txt and ProjectInfo.h byte-identical across widths; fence fixture (indent, protected trailing space, authored trailing backslash, 100-char run, spaced sentence, empty line, whitespace-only last line) round-trips byte-exact; `identity` with `\\\\` exact at width 20; `--sync` idempotent both directions on scratch copies. Data facts: 27 damaged cells (the `## layout glob` table has ten rows, not nine); `jucePluginAaxDisableDefaultSettingsChunks` added. Production `project-info.md` backed up at scratchpad `proof/production-project-info.backup.md` (md5 c603df4a…) before the repair.

## Audit Resolution (one Auditor pass, 54 findings; all code findings resolved this sprint)

Bugs: (1) pending join flushed at the cell's last line; (2/30) L2 and L3 scoped to grid tables via the table's `Id::format` property — pipe tables keep GFM trimming, `getPipeTableText` writes no runs; (3) alignment read on the reader: right/center columns strip all leading fill, left/none keep the flank; the writer pads by column only, `getRowAlignment` deleted; (4) pipes escaped per row after reflow (`getEscapedCellText`), so a cut never splits `\|`; (5) line bounds recorded by `ReflowDocument::build` on the `Id::line` Element under the new identifier `Id::span` (added to jam/cast/identifiers.md, regenerated), `getLineRows` reads it; (6) `Id::tokens` from the first logical span, untrimmed; (36) an unmarked soft break in a plain cell reads as a space (`addLineBreakElement`, cell-scoped by `blocks.root`).
Code: one constant set and one `getTrailingBackslashCount (std::string_view)` on `MarkdownDocument`; `getContinuedSpans` → `addContinuedSpans` (+ `getSpanStart`, `addAppendixSpan`, `hasEscapedChar`, `getTrimmedSpan`, `addTableCells`); `cellHasEscape` parallel array deleted (escape derived from the span); `leafText` keyed by `const Element*`, `getLeafText (const Element&)`; `BlockParser::root` public (the existing cross-class field pattern); `getRowEndRunText` replaces the two run functions; `getWrappedWidth`, dead `getWrappedText (String, int)`, unwrapped grid branch deleted — every grid cell (header too) renders through the one row renderer, measured after rendering; `getGridRowsText` emits the header once before the body loop; `ReflowDocument::parse (text, columns, cutColumns, language)`, `noCut { 0 }`, `getCutSegments`, `addLine` rows its own line; all listed functions ≤ 30 lines; stray `run1.log`/`sync1.log` removed from the jfs root.
Contract: SPEC §3.2/§3.3, HELP.md cell law, command line, `### format`, "Canonical Markdown" and the `fromUTF8` note rewritten (grid scope, alignment on read, unmarked boundary = space, width bounds content, both invariants, STE sentence length).

Residuals for ARCHITECT (pre-existing, outside the sprint's files, reported verbatim): manual flags `isLeafOpen`/`isGridTableHeaderDone` (BlockParser state machine); InlineParser reference members `Document& document`, `const BlockParser& blocks`; `trimEnd` loop `break`; grid tables never propagate real column alignment (`addAccumulatedHeaderRow` hardcodes empty alignments), so the reader's right/center branch is unreachable for grid tables today; `.project` in jfs shows modified before this sprint.

## BLESSED Alignment
- **B** — the writer owns the marker; the reader owns its inverse; nothing else touches row boundaries. No guard anywhere: the law makes every input decidable.
- **L** — three new reader functions, three new writer functions, each one responsibility; no function over 30 lines; L2's three cases are one decision.
- **E** — every row boundary is explicit in the file; no inference from geometry; `continuationRunOffset`/`terminationRunOffset` named constants; alignment decided once at `getGridRowsText`.
- **S (SSOT)** — the source bytes are the one truth; rows are spans over them; the appendix pattern (`addSource`) is the existing way a rewritten line enters the source.
- **S (Stateless)** — no flags; `w` and `a` are computed at the boundary and consumed there.
- **E (Encapsulation)** — cast changes no code; jam's writer/reader pair is the boundary; cast's Model reads `Id::rawText`/`getAllSubText` unchanged.
- **D** — `read∘format == read` and `format∘format == format` are the proofs (Step 7); a failure is a violation upstream, found by the diff.

## Risks / Open Questions
- **Whitespace run at two joins (Step 8):** `companyCopyright` and `identity` were broken at a space whose count is gone from the file. The plan writes one space. ARCHITECT confirms or corrects at approval.
- **Consumed-whitespace gap (Step 2/5):** the plan reads the break whitespace as the source gap between consecutive `Id::row` spans. `getSegment` drops `trailingClasses` = space + mandatory-break classes only (Rules.cpp:50-52); a zero-width space (`zeroWidthSpace`, in `nonAbsorbingClasses` :69-71) at a break is not in `trailingClasses` — it stays inside a segment. Engineer confirms against the rules file and reports any class where the gap is not the consumed bytes.
- **Signature change in Step 4** (`setRows` third parameter) is the smallest no-cut shape found from the header prose (h:40-55). If Engineer's read of `build()` (cpp:132-159) shows a smaller change, it is reported before writing.
- **Hand-authored plain cells with unmarked line breaks** keep today's reading (newline). The formatter will fold them to a space on the next `--format` (existing `getSourceText`), which is the same value cast's `toComment` produces. No data lane in jfs relies on a newline inside a plain value cell (all 25 damaged cells are single-value).
