# RFC — Frontmatter Pass-Through in `cast --format`

**Status:** Draft, for ARCHITECT decision, then COUNSELOR
**Author:** MACHINIST
**Date:** 27 Sep 2026
**Requested by:** ARCHITECT

---

## 1. Objective

ARCHITECT's direction: *"write RFC-frontmatter.md at cast repo"*, after the decision
*"3"*. Decision 3 is: prevent a repeat in cast, and also in nvim. Then ARCHITECT
directed: *"dont change lua formatting"*. Thus this RFC covers only cast.

The objective:

1. `cast --format` keeps a leading frontmatter block byte-for-byte.
2. Cast formats the markdown body after the block, as now.
3. The output is idempotent, like all other `cast --format` output
   (`RFC-clang-format-parity.md` §7, invariant 2).

---

## 2. Problem

### 2.1 The incident

1. ARCHITECT edited `~/.carol/agents/counselor.md` in nvim. The only change was the
   `model` value, set to `opus`.
2. nvim now formats each markdown buffer with `cast --format` on Esc, on `c:n`, and on
   save. The commit is `~/.config` `4f3d939` (`nvim/lua/core/formatting.lua`,
   `formatMarkdown`).
3. Cast rewrote the frontmatter. The file then started with:

   ```
   ---

   ## name: COUNSELOR
   description: …
   model: opus
   effort: medium
   tools: …
   color: cyan

   ## Role: COUNSELOR
   ```

4. The closing `---` was gone. Claude Code did not load the agent:
   `--agent 'COUNSELOR' not found. Available agents: Auditor, claude, …`.
5. MACHINIST restored the frontmatter by hand. No other file on the machine had the
   damage. The check was a search for `^## (name|description|model|title|version):`
   in `~/.carol`, `~/.claude`, and `~/.config`.

### 2.2 Root cause

Cast is correct for CommonMark. `SPEC.md:221`: *"CAST reads CommonMark plus the
pandoc grid-table extension."* CommonMark has no frontmatter. The JAM parser reads the
block like this:

1. Line 1 `---` is a thematic break (`jam_MarkdownBlockParser.cpp:352`,
   `isThematicBreak`).
2. The `key: value` lines are one paragraph.
3. The closing `---` under a paragraph is a setext heading underline, level 2
   (`jam_MarkdownBlockParser.cpp:2443`, `promoteSetextHeading`). The paragraph becomes
   an H2 heading.
4. The writer emits the H2 in ATX form, `## …`. The underline is consumed.

Thus the closing delimiter disappears and the block becomes a heading.

### 2.3 Observed behavior

MACHINIST ran these tests on Windows with `cast 0.1.0 (dada03c)`.

| Case | Input                                                   | Output                                                              |
|------|---------------------------------------------------------|---------------------------------------------------------------------|
| 1    | `---`, 3 `key: value` lines, `---`, blank, `## Role`, text | `---`, blank, `## name: X`, the other 2 lines, blank, `## Role`, text |
| 2    | the output of case 1, formatted again                   | `---`, blank, `## name: X`, blank, `description: a b model: opus` (lines joined) |
| 3    | `---`, `title: T`, `---` (frontmatter only)             | `---`, blank, `## title: T`                                          |
| 4    | `---`, `key: v`, blank, `other: w`, `---`, blank, body  | `---`, blank, `key: v`, blank, `## other: w`, blank, body            |
| 5    | `Intro.`, blank, `---`, `name: X`, `---`                | `Intro.`, blank, `---`, blank, `## name: X`                          |

Results:

1. Each case loses the closing `---`.
2. Case 2 shows that the output is **not idempotent**. The second pass joins the
   remaining lines into one paragraph. This breaks invariant 2 of
   `RFC-clang-format-parity.md` §7.
3. Case 5 is not frontmatter. Its output is correct CommonMark, and it must not
   change.

### 2.4 Exposure

Every file below starts with a frontmatter block. Each one is damaged when it is
formatted, and nvim formats on save.

- `~/.carol/agents/*.md`: 7 files.
- `~/.carol/commands/*.md`: 14 files.
- `~/.carol/output-styles.md`.
- `~/.carol/skills/doxygen-protocol/SKILL.md`.
- `~/.claude/skills/synced/**/SKILL.md`: 9 files.

The nvim side is not guarded. ARCHITECT rejected an interim guard in
`formatting.lua`. Until cast ships this change, a save of any of these files in nvim
damages the file.

---

## 3. Current Code Paths

Each path below parses the text and renders it through `jam::MarkdownWriter`. A
frontmatter block goes through each one.

1. `Processor::getText` (`Source/Processor.h:107-113`) — the stdout mode of
   `cast --format <file>` and stdin.
2. `Processor::format` and `writeOriginIfChanged` (`Source/Processor.h:80`,
   `Processor.h:207`) — `cast --format -i` and the format step of `cast <manifest>`.
3. `Sync::getCanonicalMarkdown` (`Source/Sync.h:880-884`) — `--sync`.
4. The parse entry points: `Model.h:934` and `Model.h:1134`, both
   `jam::MarkdownDocument::parse`.

`RFC-clang-format-parity.md` §7, invariant 6: *"One formatter path."* The fix must
apply to all three render paths through one point.

---

## 4. Target Behavior

### 4.1 Recognition

A frontmatter block exists only when all of these are true:

1. Line 1 of the file is `---`. Line 1 means the first byte of the file.
2. A later line is the closing delimiter.
3. The block ends at the first closing delimiter.

Case 5 (§2.3) has `---` at line 3. It is not frontmatter, and its output does not
change.

Open points for ARCHITECT (§6): the closing delimiter set, and the rule for trailing
spaces on the delimiter lines.

### 4.2 Content

1. Cast writes the block verbatim: the opening line, each content line, and the
   closing line.
2. Cast does not parse the block as YAML. Cast does not validate it, and it does not
   reorder or reflow it.
3. The line ends are LF, the same as all cast output
   (`RFC-clang-format-parity.md` §7, invariant 3).
4. Cast formats the text after the block as a markdown document, as it does now.
5. The separator between the block and the body is an open point (§6).

### 4.3 Meaning in cast's own data

1. A frontmatter block has no meaning to cast. It is opaque text.
2. Cast does not read tables, headings, or index rows from the block.
3. A provenance line number (`Id::line`) in the body counts the block's lines. A
   diagnostic then names the real line in the file.

### 4.4 No block

A file with no frontmatter gives byte-identical output to the current cast. This
includes a file with a thematic break at line 1 and no closing delimiter.

---

## 5. Where the Change Goes — Options

The layer is ARCHITECT's decision. Each option is a correct solution.

1. **JAM parser.** `jam::MarkdownDocument` gains a frontmatter block type, and
   `jam::MarkdownWriter` emits it verbatim. Each JAM consumer gets it.
   `RFC-clang-format-parity.md` §8 put JAM out of scope for that RFC. This RFC needs
   ARCHITECT's word to change JAM.
2. **cast only.** Cast splits the block from the text before the parse, and puts it
   back after the render. The split lives at one point that all three render paths
   in §3 use. JAM does not change. Provenance line numbers need the block's line
   count as an offset.

Evidence to decide with:

- `SPEC.md:221` defines the substrate as CommonMark plus one pandoc extension. Option 1
  adds a second extension at the parser layer. Option 2 adds it at the tool layer.
- Other JAM consumers of `MarkdownDocument` exist, for example `jam::ConfigDocument`
  (`jam_ConfigDocument.h`). With option 2, those consumers do not get the rule.

---

## 6. Open Questions for ARCHITECT

1. **Layer.** Option 1 (JAM) or option 2 (cast only), §5.
2. **Closing delimiter.** `---` only, or `---` and `...`. Librarian research on
   pandoc `yaml_metadata_block` and on the Claude Code frontmatter parser can supply
   the facts first.
3. **Delimiter whitespace.** Does a delimiter line with trailing spaces count?
4. **Separator.** After the closing `---`, does cast normalize the body separator to
   one blank line, or keep the input as it is?
5. **Unclosed block.** Line 1 is `---` and no closing delimiter exists. Does cast
   read it as CommonMark (current behavior, §4.4), or is it fatal?
6. **SPEC wording.** `SPEC.md:221` changes to name frontmatter. The exact name of the
   extension follows question 2.

---

## 7. Acceptance Criteria

1. Cases 1, 3, and 4 of §2.3: the block is byte-identical in the output. The body is
   formatted.
2. Case 2: a second `cast --format` run on the output gives the same bytes.
3. Case 5: the output is byte-identical to the current output.
4. Each of the 32 files in §2.4: `cast --format <file>` keeps the frontmatter
   byte-identical. Claude Code loads each agent after a format. The test is
   `carol` with `--agent COUNSELOR`.
5. A file with no frontmatter: the output is byte-identical to the current output,
   for each markdown file that `cast/cast/spell.md` declares.
6. A diagnostic in the body of a file with frontmatter names the real file line.
7. The three render paths in §3 give the same text for the same input.
8. Both platforms: Windows (clang-cl build) and macOS.

---

## 8. Consumer Side (nvim)

No nvim change is in this RFC. ARCHITECT's direction: *"dont change lua
formatting"*. When cast ships, the nvim wiring in `~/.config` `4f3d939` works for
frontmatter files with no change.

---

**End of RFC**
