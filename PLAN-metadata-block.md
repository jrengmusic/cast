# PLAN: metadata-block — pandoc metadata block in the JAM markdown parser

**RFC:** RFC-frontmatter.md (recommendation only — ARCHITECT: *"don't just blindly take RFC as the binding law. it's a fucking recommendations"*)
**Date:** 2026-09-27
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + JAM; jam_markdown data from jam `cast/` tables (generated)
**Status:** approved by ARCHITECT (ExitPlanMode), locked

## Context

`cast --format` breaks a leading `---` … `---` block. The JAM parser reads the first `---` as a
thematic break (`jam_MarkdownBlockParser.cpp:352`). The closing `---` becomes a setext underline
(`:2443`), and the writer emits an ATX `## ` heading (`jam_MarkdownWriter.cpp:543-549`). Pathfinder
reproduced RFC cases 1–5 on `cast 0.1.0 (dada03c)`. Case 2 shows that the output is not idempotent.
nvim formats every markdown buffer through `cast --format --assume-filename=<buf>` on stdin
(`~/.config/nvim/lua/core/formatting.lua:78-105`).

The decisions:
- Layer: JAM parser. ARCHITECT: *"our SPEC is cmark+gfm+pandoc"*. The one pandoc extension in the
  substrate, the grid table, lives in the JAM parser (`addGridTable`, `jam_MarkdownBlockParser.cpp:2004`).
- Rule: pandoc's `yaml_metadata_block`, with the content kept opaque and verbatim. The solution was
  approved by ARCHITECT's `/go`.
- Name: ARCHITECT: *"metadata then"*. The names follow the `htmlBlock` family (NAMES Rule 5).
- Pattern: ARCHITECT: *"follow the established pattern verbatim. no reinvention, no new pattern, use
  framework API to its fullest extent. no handroll anything. no magic strings, no manual string parsing"*.

The recognition rule comes from the pandoc source (Librarian, `src/Text/Pandoc/Readers/Metadata.hs`,
`yamlMetaBlock` and `stopLine`):
1. Opening line: `---` at the line start, then only spaces or tabs (`string "---"` then `blankline`).
2. The next line must not be blank (`notFollowedBy blankline`).
3. The block closes at the first line that is `---` or `...`, then only spaces or tabs
   (`stopLine`, `manyTill anyLine stopLine`).
4. No closing line means no block. The lines read as ordinary markdown (`try`, then backtrack).
5. The parser tries the block at a block position. A `---` under an open paragraph is a setext
   underline first (`promoteSetextHeading` runs before `addBlocks`, `jam_MarkdownBlockParser.cpp:2477-2479`).
   Thus "a blank line before" falls out of the existing line order, as it does in pandoc.
6. Top level only (`containerDepth == 0`), per the approved solution text *"anywhere at top level after a blank line"*.
7. JAM keeps the content opaque. It does no YAML parse (RFC §4.2, approved: *"emit it verbatim as an opaque block"*).

The fixpoint follows from the rule. The writer joins root blocks with a blank line, and a thematic
break emits `---` (`jam_MarkdownWriter.cpp:591-595`). Thus each emitted break has a blank line after
it and never opens a block (rule 2). A real block emits byte for byte, thus
`format (format (x)) == format (x)` and `read (format (x)) == read (x)` (SPEC §3.3).

## Language / Framework Constraints

- JAM data is generated. Tables change in `jam/cast/*.md`, and cast regenerates `jam/generated/*`.
  A generated file is never edited by hand.
- CODING.md: `not`/`and`/`or`, brace init, `.at()`, no new comments or doxygen before the audit.
- Result returns (`return false` when a predicate answers) follow the sibling `addHtmlBlockLine`
  (`jam_MarkdownBlockParser.cpp:1088-1111`). This is the established shape.

## Dependency & API Inventory

| Concern | Established API (read this session) |
|---|---|
| Block kinds | `map::BlockType` ← `jam/cast/bimaps.md:230-248` (`htmlBlock | 8` … `tableBorder | 14`) |
| Leaf openers | `map::OpenBlock` ← `jam/cast/bimaps.md:479-488` (`thematicBreak | 0` … `gridTable | 5`) |
| Element id | `map::BlockTag` ← `jam/cast/bimaps.md:1103-1117` (`html | BlockType::htmlBlock | \`html\``); `appendBlock` reads it (`jam_MarkdownBlockParser.cpp:64-69`) |
| Delimiter text | `Chars::*` ← `jam/cast/chars.md:258-260` (`doubleDash | \`--\``, `doubleDot | \`..\``) → `jam_Chars.h:164` `static constexpr const char* const` |
| Prefix match | `Document::Cursor::startsWith (std::string_view)` (`jam_Document.h:678`); used with Chars at `jam_Xml.h:362` |
| Marker length | `std::string_view (Chars::x).size()` (`jam_Xml.h:455`) with `advance (start, end, count)` (`jam_MarkdownDocument.h:1472`) |
| Rest of line blank | `isBlankLine (source, start, end)` (`jam_MarkdownBlockParser.cpp:71-83`) |
| Look-ahead scan | `isGridTableFormat` loop: `for (auto lineStart { end + 1 }; lineStart <= source.size();)` + `getLineEnd` (`jam_MarkdownBlockParser.cpp:1974-2002`) |
| Leaf open / lines | `addHtmlBlock`, `addHtmlBlockLine`, `isHtmlBlockEnd`, `isHtmlBlock` (`jam_MarkdownBlockParser.cpp:1063-1143`; decl `jam_MarkdownDocument.h:836-851`) |
| Leaf dispatch | `addLeaf` `leafBlocks` Function::Map + `or` chain (`jam_MarkdownBlockParser.cpp:2378-2410`) |
| Line dispatch | `addLine` `or` chain (`jam_MarkdownBlockParser.cpp:2477-2480`) |
| Leaf close | `closeLeaf` htmlBlock branch (`jam_MarkdownBlockParser.cpp:213-214`) |
| Raw leaf text | `isRawTextLeaf` (`jam_MarkdownInlineParser.cpp:29-31`) |
| Writer | `getBlocks` map; `mermaid` reuses `getFencedBlockText` (`jam_MarkdownWriter.cpp:1033-1036`); `getHtmlBlockText` (`:597-600`) |

Names (all from ARCHITECT's *"metadata then"* plus Rule 5 families):
`BlockType::metadataBlock`, `OpenBlock::metadataBlock`, BlockTag `meta`, `Chars::tripleDash` (`---`),
`Chars::tripleDot` (`...`) (family `doubleDash`, `doubleDot`), `isMetadataBlock`, `isMetadataBlockEnd`,
`addMetadataBlock`, `addMetadataBlockLine` (family `isHtmlBlock`/`isHtmlBlockEnd`/`addHtmlBlock`/`addHtmlBlockLine`).

## Validation Gate

COUNSELOR validates each step before the next one. The checks are MANIFESTO.md (BLESSED), NAMES.md,
~/.carol/CODING.md, and this locked PLAN (no deviation, no scope drift). @Auditor runs one time,
after the final step, over the whole sprint.

## Steps

### Step 1: JAM data rows, regenerate
**Scope:** `jam/cast/bimaps.md`, `jam/cast/chars.md`, then `jam/generated/*` by regeneration only.
**Action:**
1. `bimaps.md` `## BlockType`: after `| tableBorder   | 14  |`, add `| metadataBlock | 15  |`.
2. `bimaps.md` `## OpenBlock`: after `| gridTable           | 5   |`, add `| metadataBlock       | 6   |`.
3. `bimaps.md` `## BlockTag`: after `| img        | BlockType::image         | \`img\`        |`, add
   `| meta       | BlockType::metadataBlock | \`meta\`       |`.
4. `chars.md` `## tokens`: add `tripleDash` = `` `---` `` and `tripleDot` = `` `...` `` in name order,
   with the comment cell blank (the doxygen pass fills it, Step 7).
5. Read jam's `## toolchain` rows first, and report them. Then run `cast cast/spell.md` from the jam
   root. Do not add a `--<word>`. The formatter realigns the tables.
**Validation:** the generated diff holds only the new enum values, map entries, and Chars constants.
A second run writes nothing.

### Step 2: JAM parser — recognition and leaf
**Scope:** `jam_markdown/document/jam_MarkdownDocument.h` (BlockParser declarations beside `:836-851`),
`jam_markdown/document/jam_MarkdownBlockParser.cpp`.
**Action:**
1. `static bool isMetadataBlockEnd (const std::string& source, size_t start, size_t end) noexcept`.
   It is true when `Cursor { source, start }.startsWith (Chars::tripleDash)` or `startsWith (Chars::tripleDot)`,
   and `isBlankLine (source, advance (start, end, static_cast<int> (std::string_view (<marker>).size())), end)`.
2. `static bool isMetadataBlock (const std::string& source, size_t start, size_t end) noexcept`.
   - The opening line starts with `Chars::tripleDash` and has a blank rest (the same test, one marker).
   - Then the `isGridTableFormat` loop from `end + 1`:
     1. The first line is blank → false.
     2. A line where `isMetadataBlockEnd` is true → true.
     3. The end of the source → false.
3. `bool addMetadataBlock (size_t start, size_t end, size_t& remainderStart)`. When `containerDepth == 0`
   and `isMetadataBlock (source, start, end)`:
   - `closeLeaf()`.
   - `appendBlock (getParent(), map::BlockType::metadataBlock)`.
   - `isLeafOpen = true`, `leafLines.clear()`, `leafLines.add (Document::Token (start, end))`.
   - `remainderStart = end`, then return true.
   Otherwise it returns false. The shape is `addHtmlBlock` (`:1080-1086`, `:1125-1143`).
4. `bool addMetadataBlockLine (size_t start, size_t end)`. The shape is `addHtmlBlockLine`:
   - When the open leaf is not `metadataBlock`, return false.
   - Otherwise `leafLines.add (Document::Token (start, end))`.
   - When `isMetadataBlockEnd (source, start, end)`, call `closeLeaf()`.
   - Return true.
5. `addLeaf`: add the `leafBlocks.add<…> (map::OpenBlock::metadataBlock, &BlockParser::addMetadataBlock)`
   row. Put `leafBlocks.get (map::OpenBlock::metadataBlock, …)` first in the `or` chain, before `thematicBreak`.
6. `addLine`: `addFencedCodeLine (start, lineEnd) or addMetadataBlockLine (start, lineEnd) or addHtmlBlockLine (…)`.
7. `closeLeaf`: `else if (type == map::BlockType::htmlBlock or type == map::BlockType::metadataBlock)`.
   This adds no new branch.
**Validation:** declarations sit beside the htmlBlock family. No literal string or char. No new helper.
No comment. Only prompt names. Branch counts do not grow.

### Step 3: JAM inline parser and writer
**Scope:** `jam_MarkdownInlineParser.cpp:29-31`, `jam_MarkdownWriter.cpp:1039-1040`.
**Action:**
1. `isRawTextLeaf`: add `or type == map::BlockType::metadataBlock`.
2. `getBlocks`: add `blocks.add<const MarkdownWriter&, const Element&> (map::BlockType::metadataBlock,
   [] (const MarkdownWriter&, const Element& block) { return getHtmlBlockText (block); });`. This is
   the `mermaid` reuse of `getFencedBlockText`.
**Validation:** no new writer function. The text goes out verbatim through the existing raw path.

### Step 4: Build cast, prove the behaviour
**Scope:** build only (`build.bat`, as last sprint). Probes run in the session scratchpad only,
with `cast --format <file>` or stdin. Never `-i`, never a manifest in a probe.
**Action:** build cast Release and install it as last sprint did. Then run these probes. Paste each
input and output in `cat -A` form.
1. RFC cases 1, 3, 4: the block is byte-identical, and the body is formatted.
2. RFC case 2 (format twice): the bytes are identical.
3. RFC case 5 (`Intro.`, blank, `---`, `name: X`, `---`): a metadata block (pandoc rule 5), verbatim.
4. `...` close: verbatim.
5. Unclosed `---`: the output is identical to the current output.
6. `---`, blank, `x`, `---` (a blank line after the opening line): the output is identical to the current output.
7. Unclosed `---` then `***`: format twice. The bytes are identical, and `read` does not change.
8. `> ---` in a blockquote: the output is identical to the current output.
9. `# H`, then `---`, `k: v`, `---` with no blank line: a metadata block (pandoc).
10. A metadata block, then a malformed grid table: the diagnostic names the real file line.
11. The 35 files that Pathfinder listed (`~/.carol/agents`, `commands`, `output-styles.md`,
    `skills/doxygen-protocol/SKILL.md`, `~/.claude/skills/synced/**/SKILL.md`): format each one to
    stdout. The leading block is byte-identical, and a second pass gives identical bytes. Do not
    write to these files.
**Validation:** every probe matches. ARCHITECT runs the Claude Code agent load test (RFC §7.4).

### Step 5: Fixpoints and KANJUT sync
**Action:**
1. cast: run `cast cast/spell.md --no-sign` two times. Run 2 changes no file.
2. jam: the second regeneration writes nothing (Step 1).
3. `cast --sync <jam> <kuassa/user_modules>` into a scratch copy first, then into production, as last
   sprint did. Then run the kuassa regeneration. A second sync and a second regeneration change nothing.
**Validation:** zero-diff second runs. The sync report lists only the files from Steps 1–3 and the
regenerated files.

### Step 6: SPEC and HELP
**Scope:** `SPEC.md` §3, `Source/HELP.md`.
**Action:**
1. `SPEC.md:221` becomes: `CAST reads CommonMark, GFM, and two pandoc extensions: grid tables and
   metadata blocks.` ARCHITECT: *"our SPEC is cmark+gfm+pandoc"*.
2. New `### 3.4 Metadata Block` states rules 1–7 above. The formatter writes the block byte for byte,
   and one blank line separates it from the next block.
3. HELP.md gets the derived one-paragraph form.
**Validation:** ASD-STE100. Each rule traces to the pandoc source or an ARCHITECT quote above.

### Step 7: Audit, then doxygen pass
1. @Auditor runs one sweep over Steps 1–6. COUNSELOR validates each finding against a CONTRACT
   clause, and each validated finding is resolved in this sprint.
2. Doxygen pass (post-audit):
   - the new BlockParser members;
   - the `bimaps.md` `BlockType`, `OpenBlock` and `BlockTag` brief fences (they list the kinds);
   - the `chars.md` comment cells;
   - `jam_MarkdownDocument.h:1563` (the deferred-text list).
3. Regenerate jam and cast docs to zero warnings. Sync to KANJUT again.

## BLESSED Alignment
- **B:** the parser owns the block. The Element owns its text. Nothing floats.
- **L:** four small members. `addLeaf`/`addLine` grow by one link each. `closeLeaf` grows no branch.
- **E:** delimiters are `Chars::` constants (no magic strings). Dispatch goes through the existing maps.
- **S (SSOT):** one rule at the parser. cast's five parse sites and three render sites inherit it,
  and cast needs no split.
- **S (Stateless):** the predicates are static and pure. Leaf state is the existing `isLeafOpen`/`leafLines`.
- **E (Encapsulation):** cast `Source/` does not change.
- **D:** recognition is a pure function of the bytes. The block emits verbatim. The fixpoint is proven in Step 4.

## Risks / Open Questions
- None open. The probe outcomes in Step 4 are the acceptance record.

## Execution Record
- Step 2.2 correction: the look-ahead returns false only when the line right after the opener is blank (`lineStart == end + 1`). Blank lines inside the block are content (pandoc `manyTill anyLine stopLine`).
- Step 2.1 correction: each marker advances by its own size. `isMetadataBlock` tests `startsWith (Chars::tripleDash)` and `isMetadataBlockEnd`.
- Audit finding 1: `containerDepth == 0` admitted table-cell parsers, because their root is a `tableCell` at depth 0. `addMetadataBlock` now tests `*getParent().get<int> (Id::type) == map::BlockType::document`. The SSOT line above applies to the document parser only. Cell parsers (`addCellLines`, `getCellText`) never open the block.
- Audit finding 8: `closeLeaf` dispatches through `leafClosing` (`Function::Map<int, void>`). This replaces a 4-arm chain.
- Step 4 probe 11: the leading block is byte-identical in all 32 files. The second pass is identical in 28 files. In 4 files the body changes on the second pass, and the change reproduces without the metadata block (SPRINT-LOG State for Continuation).
