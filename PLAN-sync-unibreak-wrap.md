# PLAN: sync-unibreak-wrap

**RFC:** none. The objective comes from ARCHITECT's prompts.
**Date:** 2026-09-24 (revision 4)
**BLESSED Compliance:** verified against the reads cited in each step
**Language Constraints:** C++17 / JUCE + JAM (LANGUAGE.md C++/JUCE §L)

## Overview

Revision 3's Step 6 made a hidden state machine: `UniBreak::Result`, `init`/`step`/`end` and a callback walk. Its state lived outside the Model. Revision 4 rebuilds the text layer from jam's own owners:
- `Unibreak` derives from `Document`.
- `Element` holds lines, segments and rows.
- `Document::Index` counts lines and rows only.
- `AttributedChar` owns width and the cluster walk.
- AnsiDocument owns the cell contract.

## ARCHITECT rulings (verbatim, this revision)

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

## Dependency & API Inventory (read this session)

- **Document engine.**
  - `parse (const char*, int)` normalises the input with `preprocess` and runs `getTokens`. It walks the `Cursor`, calls `getToken` until the source is consumed, then calls `build()` (jam_Document.cpp:586–631).
  - `getToken` can return any length above 0 (cpp:616–627).
  - The hooks are declared at jam_Document.h:999–1013.
- **Nearest sibling: `CodeDocument`** (jam_CodeDocument.h:25–138).
  - `static parse (text, info)` sets a member, then calls `Document::parse` (:40–54).
  - `getToken` appends to `tokens` (:79–122).
  - `build()` stores the tokens on the root (:125–128).
  - Members: `jam::Array<Document::Token> tokens`, `int family` (:137–138).
- **Model value types.**
  - `Element::Value` holds `Tokens = jam::Array<Token>` (h:166, :173–187).
  - A `Token` is a `Span` (`jam::Union<uint32_t, uint32_t>`) plus a type (h:50–87). `Token::setSpan` is at h:94.
  - A property is replaced through its `get` pointer. This is established at DocumentIndex.cpp:469 and TextEditor.cpp:912.
  - The root children are iterated with `Document::begin`/`end` (h:746–750).
- **Identifiers that exist:** `Id::line` (jam_Identifiers.h:744), `Id::row` (:1060), `Id::tokens` (:1351), `Id::cells` (:211).
- **Byte-class tables.** `## ansi` in jam/cast/lookuptables.md:629–678 has one default row (`0x20 | map::Byte::text`) plus operators. `## data` at :604–627 uses a `0x0` default row.
- **AttributedChar (jam_core).**
  - `width (uint32_t)`: the per-codepoint monospace display width (jam_AttributedChar.h:269–278).
  - The width clamp `rawWidth < 1 ? 1 : rawWidth` → `widthClass` (jam_Charset.cpp:41–45).
  - `graphemeSegmentationInit`/`graphemeSegmentationStep` (h:327–341).
  - A cluster atom is `contentGrapheme` (h:34–37, :76–77).
  - `narrow`/`wide` are one and two display columns (h:85–89).
- **Index.**
  - It is used only by `jam::TextEditor` (TextEditor.cpp:77, :247–254, :478–489, :824–832, :913–934, :1080–1090).
  - The cell reads are at DocumentIndex.cpp:26, :41, :68, :79–86, :93, :396, :411, :416–419, :457–469, :495–497, :504–513, :559–586.
  - The Codec doc names `Id::cells` (h:471–481).
- **Module graph** (module headers, line 10):
  - jam_terminal depends on jam_core, jam_graphics and jam_data_structures, with no jam_gui and no jam_markdown.
  - jam_gui does not list jam_terminal.
  - So adding jam_gui → jam_terminal makes no cycle.
- **UAX #14 rules:**
  - `contextRules`, `precedingRules` and the class masks (jam_UniBreakRules.cpp:54–141).
  - The decision functions (:143–342).
  - Their only state input is `Result`, and each field maps to a neighbour token (Step 7).

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

## Steps

### Steps 1–5 — DONE
These steps are validated and do not change:
- the sync tie-break
- SPEC and HELP for sync
- the jam cast tables (`map::LineBreak`, masks, tailoring)
- the property word

The Step 5 state-machine form is replaced by Step 7. Its rule content carries over.

### Step 6: AttributedChar — the one cluster walk and text width
**Scope:** `jam_core/text/jam_AttributedChar.h`, `jam_core/text/jam_Charset.cpp`.

1. **`static uint8_t getWidthClass (uint32_t codepoint) noexcept`.**
   - Move the width clamp out of `fromCodepoint` (Charset.cpp:42–45) into it: `rawWidth < 1 ? narrow : (rawWidth == 2 ? wide : narrow)`, with the same arithmetic as today.
   - `fromCodepoint` calls it.
2. **`static constexpr int narrowColumns { 1 }` and `static constexpr int wideColumns { 2 }`.** These are the column counts that the `narrow`/`wide` docs state (h:85–89).
3. **`template <typename Function> static void applyToClusters (std::string_view text, Function&& function)`.**
   - It decodes each codepoint in place with `juce::CharPointer_UTF8`.
   - The cluster rule is `graphemeSegmentationStep (...).addToCurrentCell()` while `count < codepoints.size()`. This is the rule of AnsiDocument.cpp:271–285.
   - It calls `function (const Grapheme::Entry& cluster, int codepointIndex)` once per cluster, where `codepointIndex` is the index of the cluster's first codepoint.
   - Positive if/else only: no `continue` and no early return.
4. **`static int width (std::string_view text) noexcept`.**
   - It is the sum over `applyToClusters` of `getWidthClass (base) == wide ? wideColumns : narrowColumns`.
   - It is in the `width (uint32_t)` family and allocates nothing.

**Validation:**
- The clamp exists once.
- There is one cluster walk.
- No allocation.
- No bail-out.

### Step 7: Unibreak — a Document
**Scope:**
- **Delete:** `jam_core/text/jam_UniBreak.h`, `jam_UniBreak.cpp` and `jam_UniBreakRules.cpp`.
- **Add:** `jam_core/text/jam_Unibreak.h`, `jam_Unibreak.cpp` and `jam_UnibreakRules.cpp`.
- **Edit:**
  - `jam_core/jam_core.h`: move the include after `document/jam_Document.h` (:113), because Unibreak derives from Document.
  - `jam_core/jam_core.cpp:5–6`.
  - `jam/cast/lookuptables.md`: add `## unibreak`.

The Engineer deletes the three old files. If the permission layer denies an `rm`, the Engineer reports it, and COUNSELOR brings it to ARCHITECT at that point.

1. **Vocabulary.**
   - `## unibreak` in lookuptables.md has a brief and one row, `| 0x0 | map::Byte::text |`, in the `## data`/`## ansi` shape.
   - `getVocabulary()` returns `static const Vocabulary { map::unibreak, {}, {} }`, the AnsiDocument.cpp:36–40 shape.
2. **`struct Unibreak : Document`**, in the CodeDocument shape.
   - `static Unibreak parse (const juce::String& text, int language)` sets `language`, then calls `Document::parse (text.toRawUTF8(), getNumBytesAsUTF8())`.
   - Members: `Document::Tokens tokens`, `int language`.
   - The decision constants `noBreak`, `allowedBreak`, `mandatoryBreak` and `undecided` move from `Result` to `Unibreak`, with the same names and values.
3. **`getToken (Cursor&, int)`.**
   - It decodes one codepoint at the cursor.
   - It resolves the class with `getResolvedClass (codepoint, language)`, dropping `isStrict`: the only caller passes `false`, per jam_UniBreak.h:120.
   - UAX #14 LB9: a CM or ZWJ codepoint after a token whose class is not in `nonAbsorbingClasses` extends that token with `setSpan`. This is creation.
   - UAX #14 LB10: a CM or ZWJ codepoint in any other position appends a token with class AL.
   - Otherwise it appends `Token { start, end, class }`.
   - It returns the byte length.
4. **`build()`.**
   - Each boundary `i` (the token index after the candidate break) gets `getBoundaryDecision (tokens, source, i)`.
   - The model has these parts:
     - An allowed break ends a segment. A segment is a `Token` whose span is the segment content, with trailing SP/BK/CR/LF/NL excluded. The class of the codepoint tokens gives the trim, so nothing is decoded again.
     - A mandatory break ends the line. Each line is one `Id::line` child of root. It carries `Id::tokens` (its segments) and `Id::row`, which is one row spanning the whole trimmed line. That is the unwrapped state.
     - The end of the text ends the last line (LB3).
   - `tokens` is consumed here, in the AnsiDocument.cpp:167–173 shape.
5. **The rules rewrite (`jam_UnibreakRules.cpp`).**
   - `using Rule = int (*) (const Document::Tokens& tokens, const std::string& source, int boundary);`
   - The masks and the two `LookupTable`s stay with the same entries.
   - Each `Result` field becomes a read in place:

     | `Result` field | Read in place |
     |---|---|
     | `earlierClass` | `tokens.at (i - 2).type`, or `noClass` |
     | `previousClass` | `tokens.at (i - 1).type` |
     | `pendingClass` / `afterClass` | `tokens.at (i).type` |
     | `followingClass` | `tokens.at (i + 1).type`, or `noClass` |
     | lookahead | `tokens.at (i + 2).type`, or `noClass` |

   - The flags come from `AttributedChar::isEastAsian`, `isInitialQuotation`, `isFinalQuotation`, `isPotentialEmoji` and the dotted circle. They read the token's first codepoint, decoded at `token.offset()` from `source`.
   - The joiner flag is "the token's last codepoint is ZWJ".
   - The run states come from backward scans over `tokens` (Rules.cpp:56–88 semantics), each as a file-static predicate:

     | Predicate | Replaces |
     |---|---|
     | `getSpacedClass` | `hasSpaces` / `spacedClass` |
     | `isRegionalOdd` | `regionalOdd` |
     | `isNumericRun` | `numericRun` |
     | `isNumericClosed` | `numericClosed` |
     | `isQuoteOpen` | `quoteOpen` |

   - `startOfText` is `boundary == 0`.
   - The reference implementation is the current jam_UniBreakRules.cpp. libunibreak stays the external reference.
6. **Delete:**
   - `Result` and its static_asserts
   - `init`, `step` and `end`
   - `getShiftedState`, `getContextState`, `getAbsorbedState` and `getFilledState`
   - `applyToLines`, and `applyToClusters` (moved to Step 6)
   - both `getCellWidth` overloads
   - `getClusterHeadCells`, the `unprintable*` constants and `getTrimmedEnd`

**Validation:**
- No state outside the Model.
- No out-parameter, no bail-out, and at most 30 lines and 3 branches per function.
- One token array.
- The rules read in place.

### Step 8: Unibreak rows — reflow as a state update
**Scope:** `jam_Unibreak.h/.cpp`.

1. **`template <typename Measure, typename LineTest> void setRows (float limit, Measure&& measure, LineTest&& isAllowedLine)`.**
   - For each `Id::line`, it replaces `Id::row` in full (complete replaces complete).
   - `measure (Document::Span row)` returns the width of a row span.
   - A row runs from its first segment's start to its last segment's trimmed end.
   - Rows fill greedily with whole segments while the width is at or below `limit`.
   - A segment wider than `limit` is a row of its own.
2. **The line-start rule.**
   - After a greedy fill, the fit checks each row after the first with `isAllowedLine (Document::Span row)`.
   - If a row fails, the break before it goes into a local forbidden set, and the fill runs again from the previous row's start.
   - This is creation of the new rows, so multiple passes and a local work buffer are permitted (MANIFESTO E).
   - It ends because each pass adds one forbidden break.
   - A mandatory break is never tested, because it is a line boundary.

**Validation:**
- The output is identical for identical inputs.
- The Model is replaced, never patched.

### Step 9: Document::Index — contract fix (lines and rows only)
**Scope:** `jam_core/document/jam_Document.h` (the Index section, h:433–632), `jam_DocumentIndex.cpp`.

1. **Leaf rows come from `Id::row`.**
   - A line's row count is `jmax (minNumRows, line->get<Tokens> (Id::row)->size())`. It is read in `addLeaf` (cpp:89–96), in the same way `Id::cells` was read.
   - Node `numCells` becomes a leaf row count read at `addLeaf` and in `setNumRows`.
   - `appendLine`/`insertLine` assert `Id::row`, not `Id::cells` (cpp:26, :41).
2. **`setNumCells (int lineNumber)` becomes `setNumRows (int lineNumber)`.**
   - It re-reads `Id::row` and applies the delta with `addCounts`, as cpp:71–87 does today.
   - The private `setNumRows (Node*)` recursion (cpp:368–378) is deleted. Its only caller is `setColumns`.
3. **Delete:** `columns`, `unwrappedColumns`, `setColumns`, `getColumns`, `getRowNumber (int, int column)` and `getCellRange`.
4. **The swap tier leaves cells.**
   - Codec gains `std::function<juce::int64 (const Element&)> getNumBytes` and `std::function<void (Element&)> clear`. They are all-or-none with `encode`/`decode` (cpp:15).
   - Byte accounting: a leaf stores its `numBytes`. Every `getNumBytes (Cells)` read becomes `codec.getNumBytes (line)` (cpp:68, :79, :86, :411, :468, :497).
   - Release: `*cells = Cells {}` becomes `codec.clear (line)` (cpp:469).
   - Page-in check: `jassert (codec.getNumBytes (line) == leaf numBytes)` (cpp:495).
   - `static getNumBytes (const Cells&)` (h:613, cpp:416–419) moves to AnsiDocument (Step 10).
5. **Doxygen contract.** The prose that says Index "indexes only Id::line elements that carry an Id::cells property" (h:441–443, :472–475) is corrected in the post-audit doc pass.

**Validation:**
- There is no `Cells`, `AttributedChar` or `Id::cells` in the Index section or in jam_DocumentIndex.cpp.
- The B-tree functions do not change.

### Step 10: AnsiDocument — owns the cell contract; one cluster walk
**Scope:** `jam_terminal/document/jam_AnsiDocument.h/.cpp`.

1. **Moved from Index as working code.** These are public statics, and AnsiDocument holds no state for them.
   - `static void setRows (Document::Element& line, int columns)`: `Id::row` becomes `[k × columns, min ((k + 1) × columns, numCells))`. This is the cpp:396 and :583 arithmetic.
   - `static void setRows (Document::Index& index, int columns)`: for each line, call `setRows (*index.getElement (line), columns)`, then `index.setNumRows (line)`.
   - `static int getRowNumber (Document::Index& index, int lineNumber, int column)`: the first row of the line whose span holds `column` (cpp:559–571).
   - `static juce::Range<int> getCellRange (Document::Index& index, int rowNumber)`: the line's `Id::row` token at `rowNumber - getRowNumber (line)` (cpp:573–586).
   - `static juce::int64 getNumBytes (const Document::Element& line)`: `numCells × sizeof (AttributedChar)` (cpp:416–419).
   - `static void clear (Document::Element& line)`: `Id::cells` becomes `Cells {}` (cpp:469).
2. **`addLine` (cpp:176–180)** also adds `Id::row` as one row spanning the cells.
3. **Revision 3 Step 9.1, with the host now `AttributedChar`:**
   - `addTextRun` reads the token bytes in place: `std::string_view { document.getSource() }.substr (token.offset(), token.length())`. It calls `AttributedChar::applyToClusters` → `addCluster`.
   - Delete the loop and `addPendingCluster` (cpp:248–289, h:158–163).
4. **`addCluster`:** `isWideCluster` becomes `AttributedChar::getWidthClass (base) == AttributedChar::wide` (cpp:234). This is one clamp rule.

**Validation:**
- No manual boolean.
- No `getText` copy in `addTextRun`.
- The arithmetic is identical to the Index lines it replaces.

### Step 11: TextEditor — compile against the moved code
**Scope:** `jam_gui/jam_gui.h` (dependencies: add `jam_terminal`), `jam_gui/widgets/jam_TextEditor.cpp`. There is no design change.

| TextEditor.cpp | Today | Change |
|---|---|---|
| :253 | `index.setColumns (w / cellWidth)` | `AnsiDocument::setRows (index, w / cellWidth)` |
| :478, :487, :824 | `index.getRowNumber (line, column)` | `AnsiDocument::getRowNumber (index, line, column)` |
| :825, :1089 | `index.getCellRange (row)` | `AnsiDocument::getCellRange (index, row)` |
| :913 | `index.setNumCells (line)` | delete. `setContentSize()` (:914) sets every row again through :253. |
| :922 | `add<Document::Cells> (Id::cells)` | also `add<Document::Tokens> (Id::row)` before `insertLine` |

The Codec at :74–77 does not change.

**Validation:** TextEditor has no other diff.

### Step 12: Terminal GraphicsContext — revision 3 Step 9.2, with host `AttributedChar`
**Scope:** `jam_terminal/graphics/jam_TerminalGraphicsContext.h`, `jam_TerminalGraphicsContextText.cpp`.
- Delete `appendSegmentedCodepoint`, `addPendingClusterCells` and the `Pen` struct (h:543–555, :929–958).
- Make one `AttributedChar::applyToClusters` call over the `juce::AttributedString` text's UTF-8 view. That text is JUCE input (h:366).
- Each cluster takes the style of the attribute run that holds its `codepointIndex`. The run index advances in one scalar local, which a lambda with explicit captures reads.

**Validation:**
- There is one cluster walk in jam.
- No `substring`.

### Step 13: MarkdownWriter — `maxTableWidth` and `lineWrap` as materialisation
**Scope:**
- `jam_markdown/document/jam_MarkdownWriter.h/.cpp`
- `jam_MarkdownDocument.h/.cpp`
- `jam_MarkdownBlockParser.h/.cpp`
- `jam/cast/bimaps.md` (`## OpenBlock` rows only)

1. **Delete:**
   - the old body of `getWrappedText`. The name stays, and it is the one shared "parse → setRows → join rows" function for the paragraph and the grid cell (item 5). Its `limit` parameter is `float`.
   - `MarkdownDocument::isParagraphInterrupt` (h:88, cpp:88–93)
   - `BlockParser::isParagraphInterrupt` (h:575, cpp:236–249)
2. **`static std::string_view getView (const juce::String& text)`** is a private MarkdownWriter member, with `toRawUTF8`/`getNumBytesAsUTF8` (the jam_Strings.h:696 shape). These call `AttributedChar::width (getView (…))`:
   - `getFillWidth` (cpp:86)
   - `getPrefixWidth` (:251)
   - the table widths (:446, :480)
3. **Line-start rule: revision 3 Step 7.2, unchanged.**
   - The `## OpenBlock` rows `blockquote`, `listItem`, `setextHeading` and `table` are appended after `gridTable`.
   - `BlockParser::OpenBlockCondition` and `openBlockConditions` are added.
   - `BlockParser::isOpenBlock (const std::string&, size_t, size_t)`, in the `getHtmlBlockCondition` shape (BlockParser.cpp:1029–1044).
   - A public `MarkdownDocument::isOpenBlock` forwards to it.
4. **Paragraph (`lineWrap`, reflow)** — `getParagraphText (block) const`:
   1. `getJoinedText (block)` (cpp:89–99) stays as the one-time creation of the paragraph source. A soft break inside `Id::text` becomes `Chars::space` (InlineParser.cpp:1367–1381).
   2. `auto unibreak { Unibreak::parse (source, map::LineBreakLanguage::und) }`.
   3. Call `unibreak.setRows (lineWrap - getPrefixWidth (block), measure, isAllowedLine)`:
      - `measure` = `AttributedChar::width` of the row's byte view of `unibreak.getSource()`.
      - `isAllowedLine` = `not MarkdownDocument::isOpenBlock (unibreak.getSource(), start, start + length)`, with `start` from `unpack<0>` and `length` from `unpack<1>`.
   4. The output walks `for (auto* line : unibreak)` and each line's `Id::row` tokens. It appends each row's bytes to the `juce::String` output, which is the final output, with `Chars::newline` between rows.
   5. Index is not built here. A sequential writer reads Elements in order, so an Index would be a structure with no reader (MANIFESTO L, YAGNI).
5. **Grid cell (`maxTableWidth`, break only)** — `getGridTableText` (cpp:514–542), body cells only, and only when `Id::format` is `gridTable`:
   - Delete the `getWrappedText (text, columnWidths.at (...))` call.
   - The cell text goes through `Unibreak::parse` → `setRows (columnWidth, measure, isAllowedLine)`, and the rows are written as the cell's lines. Authored line breaks are mandatory breaks, so they stay.
   - Header cells never split. Pipe tables do not change.

**Validation:**
- The Model is only read.
- No row starts an `isOpenBlock` line.
- `format (format (x)) == format (x)`.

### Step 14: cast flags — contract fix (revision 3 Step 8, unchanged)
**Scope:** `Source/main.cpp`, `SPEC.md` §2.1 and §3.3, `Source/HELP.md`.
1. Delete `getArgumentsWithoutFlag` and the two `jam::Array<char*>` copies (main.cpp:93–110, :484–488).
2. Add `static int getArgumentIndex (int argc, char* argv[], int position)`, which reads argv in place and skips the value-flag pairs. `getFlagArgument`, `getPostFlagArgument` and `getManifestIndex` read through it.
3. SPEC §2.1 and §3.3 get the rules for grid body split, paragraph reflow and "no wrapped line starts a block". HELP follows SPEC.

### Step 15: Pixel sites — no change (revision 3 Step 10)
JUCE's own wrap and fit stay JUCE's (juce_TextLayout.cpp:447–459).

### Step 16: Deletion, data and proofs
1. The Engineer deletes `jam_graphics/text/jam_ShapedTextOptions.h`. Its include is already removed. A denied `rm` is reported, not routed around.
2. Module table shape: 269/273 columns. This decision is open (revision 3 Step 12).
3. Regenerate jam, then cast (this generates `map::unibreak` and the `OpenBlock` rows). The second run is byte-identical.
4. Run the LineBreakTest.txt 17.0.0 harness through `Unibreak::parse`. The harness maps byte offsets to codepoint indices.
5. Run the sync proof (SPRINT-LOG :394, :405).
6. `cast --format` two times on a file that has long paragraphs and grid cells. The second run writes zero bytes.

## BLESSED Alignment
- **B:** Each Document owns its tokens and Elements. Index owns only its B-tree. The Codec is the owner's capability, injected into Index.
- **L:** Lookup tables replace the rule chains (`contextRules`, `precedingRules`, `openBlockConditions`). The run-state predicates split the rule reads by responsibility.
- **E:** No magic values. `narrowColumns`/`wideColumns` name the documented column counts. The vocabulary is generated (`map::unibreak`, `map::OpenBlock`).
- **SSOT:**
  - one width clamp (`getWidthClass`)
  - one cluster walk (`AttributedChar::applyToClusters`)
  - one token array per Document
  - rows are only in `Id::row`
  - cell arithmetic is only in AnsiDocument
- **Stateless:** No carrier outside the Model. `Result` is deleted. Build-time locals live only within `build()` or `setRows`.
- **Encapsulation:** Unidirectional layers: jam_core ← jam_terminal ← jam_gui, and jam_core ← jam_markdown. Index does not know any payload.
- **D:** The Step 16 proofs.

## Risks / Open Questions
- `Document::parse` runs `preprocess`, which maps CRLF/CR/FF to LF and NUL/lone surrogates to U+FFFD (h:792–803). The Step 16 harness applies the same mapping to the expected breaks in LineBreakTest cases that hold those codepoints.
- Step 16: the module table shape.
