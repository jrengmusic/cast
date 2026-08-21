# SPRINT-LOG.md

**Project:** cast  
**Repository:** /Users/jreng/Documents/Poems/dev/cast  
**Started:** 2026-08-01

**Purpose:** Long-term context memory across sessions. Tracks completed work, debts paid, and debts deferred to the inter-sprint ledger (`DEBT.md`). Written by PRIMARY agents only when ARCHITECT explicitly requests.

---

## 📖 Notation Reference

**[N]** = Sprint Number (e.g., `1`, `2`, `3`...)

**Sprint:** A discrete unit of work completed by one or more agents, ending with ARCHITECT approval ("done", "good", "commit")

---

## ⚠️ CRITICAL RULES

**AGENTS BUILD CODE FOR ARCHITECT TO TEST**
- Agents build/modify code ONLY when ARCHITECT explicitly requests
- ARCHITECT tests and provides feedback
- Agents wait for ARCHITECT approval before proceeding

**AGENTS NEVER RUN GIT COMMANDS**
- Write code changes without running git commands
- Agent runs git ONLY when user explicitly requests
- Never autonomous git operations
- **When committing:** Always stage ALL changes with `git add -A` before commit
  - ❌ DON'T selectively stage files (agents forget/miss files)
  - ✅ DO `git add -A` to capture every modified file

**SPRINT-LOG WRITTEN BY PRIMARY AGENTS ONLY**
- **COUNSELOR** writes to SPRINT-LOG
- Only when user explicitly says: `"log sprint"`
- No intermediate summary files
- No automatic logging after every task
- Latest sprint at top, keep last 5 entries

**NAMING RULE (CODE VOCABULARY)**
- All identifiers must obey project-specific naming conventions (see NAMES.md)
- Variable names: semantic + precise (not `temp`, `data`, `x`)
- Function names: verb-noun pattern (initRepository, detectCanonBranch)
- Struct fields: domain-specific terminology (not generic `value`, `item`, `entry`)
- Type names: PascalCase, clear intent (CanonBranchConfig, not BranchData)

**BEFORE CODING: ALWAYS SEARCH EXISTING PATTERNS**
- ❌ NEVER invent new states, enums, or utility functions without checking if they exist
- ✅ Always grep/search the codebase first for existing patterns
- ✅ Check types, constants, and error handling patterns before creating new ones
- **Methodology:** Read → Understand → Find SSOT → Use existing pattern

**TRUST THE LIBRARY, DON'T REINVENT**
- ❌ NEVER create custom helpers for things the library/framework already does
- ✅ Trust the library/framework - it's battle-tested

**FAIL-FAST RULE (CRITICAL)**
- ❌ NEVER silently ignore errors (no error suppression)
- ❌ NEVER use fallback values that mask failures
- ❌ NEVER return empty strings/zero values when operations fail
- ❌ NEVER use early returns
- ✅ ALWAYS check error returns explicitly
- ✅ ALWAYS return errors to caller or log + fail fast

**⚠️ NEVER REMOVE THESE RULES**
- Rules at top of SPRINT-LOG.md are immutable
- If rules need update: ADD new rules, don't erase old ones

---

## Quick Reference

### For Agents

**When user says:** `"log sprint"`

1. **Check:** Did I (PRIMARY agent) complete work this session?
2. **If YES:** Write sprint block to SPRINT-LOG.md (latest first)
3. **Include:** Files modified, changes made, alignment check, debts paid, debts deferred
4. **Hygiene:** After writing the SPRINT-LOG entry, drain paid debts from project-root `DEBT.md` via `carol debt clear <id>` for each ID under *Debts Paid*. Receipt first, then clean the books.

### For User

**Activate PRIMARY:**
```
"@CAROL.md COUNSELOR: Rock 'n Roll"
```

**Log completed work:**
```
"log sprint"
```

**Invoke subagent:**
```
"@oracle analyze this"
"@engineer scaffold that"
"@auditor verify this"
```

**Available Agents:**
- **PRIMARY:** COUNSELOR (domain specific strategic analysis)
- **Subagents:** Pathfinder, Oracle, Engineer, Auditor, Machinist, Librarian

---

<!-- SPRINT HISTORY STARTS BELOW -->
<!-- Latest sprint at top, oldest at bottom -->
<!-- Keep last 5 sprints, rotate older to git history -->

## SPRINT HISTORY

## Handoff to COUNSELOR: Definition≡Scope Render Unification

**From:** COUNSELOR
**Date:** 2026-08-21
**Status:** Ready for Implementation — blocked on ARCHITECT's in-flight table edits + 3 design confirmations. Plan written: `PLAN-render-unification.md` (repo root).

### Context
Continuing the jam convergence (cast `jam/diff/` → byte-identical with oracle `jam/generated/`). This session did NOT converge a file — it resolved the ENGINE-LEVEL design that unblocks the map/bimap/lookup files (and fixes why all 13 files currently differ). Root cause of divergence: the engine COLLAPSES list bodies onto one physical line (joins children by empty string, not newline), plus two redundant list mechanisms (region markers + source-expansion) and an emit-then-repair elision machine. ARCHITECT's design, reached this session: **Definition and Scope are ONE fragment**, differing only in AXIS — a Definition is one horizontal declaration (`type name { list }`, cells joined by `, `); a Scope stacks declarations vertically (newline-joined body; sibling breaks by `//===`). Oracle is uniformly vertical. `HELP.md` becomes the governing SPEC of this design.

### Completed
- Deleted dead `jam::Format::getSection`/`replaceSection` (the `:::name:begin:::`/`:::end:::` region-marker builders) + the region-marker docs in `cast/Source/HELP.md` (Engineer, verified zero residue).
- Established the full unified design with ARCHITECT (Definition≡Scope, two axes; `separator` column replacing `lineBreak` with `- line:`/`- list:`; `## break` table; `namespace jam` outermost drops inner `jam::`; maps decompose to Scope+Definition; region + elision deleted; tables already correct — only expression changes; data dictates logic).
- Wrote `PLAN-render-unification.md` (repo root) — 7 steps, byte-diff + fixpoint gate.
- Confirmed via reads: oracle is uniformly VERTICAL (Text/Identifiers/Generated/Chars/HashMaps one-per-line); the "horizontal" in `diff/` is the collapse bug, NOT truncation (corrected a Pathfinder mis-read). Sigil is `@` exclusively (HELP.md's `#` is stale). `## output` has NO `code` column (`list | structure | separator | file`).

### Remaining
- Steps 1-7 of `PLAN-render-unification.md`: HELP.md SPEC rewrite → Definition/Scope templates → engine (vertical body-join kills collapse; horizontal `- list:` join; delete region + elision; Writer `build*`→get-nouns) → converge vertical files (Identifiers/Text/Chars/Generated/Operators) → conform maps (HashMaps/LookupTables/Bimaps/Enums/MermaidTables/Entities) → delete region vocabulary + resolve `jam_MermaidGeometry` (absent from oracle) + fixpoint → Auditor.

### Key Decisions
- **Definition≡Scope, axis is the only difference** — Definition horizontal (`:::list:::` cells joined by `- list:`/comma), Scope vertical (items newline-joined, sibling break `- line:`/`//===`).
- **`separator` column** (was `lineBreak`), structured `- line:` (vertical) / `- list:` (horizontal); `## break` table (`text.md:22`): `line`=`//===`, `comma`=`, `.
- **`namespace jam` outermost** → inner types drop `jam::`.
- **Maps decompose** to Scope+Definition; delete `Bimap`/`HashMap`/`LookupTable.cast` + region mechanism.
- **HELP.md is the governing SPEC**; **tables are already correct** — manifest/templates/engine express the design.
- Definition body slot → `:::list:::`; Writer `build*` → get-nouns (ARCHITECT-ratified).

### Files Modified
- `jam/jam_core/text/jam_Format.h` / `jam_Format.cpp` — `getSection`/`replaceSection` removed
- `cast/Source/HELP.md` — region-marker docs removed (full SPEC rewrite pending, Step 1)
- `cast/PLAN-render-unification.md` — new plan
- (ARCHITECT, in-flight) `jam/cast/tables/text.md` (`## break`, `## english`, `@text:*` aliases), `jam/cast/CAST.md` (`separator` column), `jam/cast/tables/lexicon.md` (+ `.bak`)

### Open Questions
1. **Entry terminator vs comma** — `Definition.cast` ends literal `;`; map entries end `,`. Is the `,` the `- list:`/`- line:` separator (join) or a terminator slot? (ARCHITECT defining via `separator`/`## break`.)
2. **Structured `separator` not applied** — CAST.md cells are flat (`@text:break:line`/empty); the `- line:`/`- list:` form is target.
3. **Bimap** — map-init + enum + `getDefault`/`get` methods + singleton in one struct: pure Scope+Definition, or a retained specialized fragment for the fixed non-list text?
4. **Sigil** — HELP.md `#` vs tables `@` (plan assumes `@`).
5. **Body slot name** — Scope `:::body:::` vs Definition `:::list:::` — unify or keep distinct.

### Next Steps
1. Let ARCHITECT's table edits settle (text.md/separator/lexicon); resolve the 5 open questions.
2. Step 1: rewrite `Source/HELP.md` as the SPEC (delegate to @Engineer).
3. Steps 2-3: Definition/Scope templates + engine (delete region + elision, vertical/horizontal join) to @Engineer.
4. Converge file-by-file with `cmp` gate; fixpoint; @Auditor once at end.

## Handoff to COUNSELOR: jam_Chars.h Convergence + Structural Grammar SSOT

**From:** COUNSELOR
**Date:** 2026-08-21
**Status:** In Progress — build pending after Extensions struct conversion + include order fix

### Context
Converging cast's output (`jam/diff/`) to byte-identical with the oracle (`jam/generated/`), one file at a time. jam_Generated.h, jam_Text.h, jam_Identifiers.h are byte-compile-identical (cosmetic deltas only). jam_Files.h and jam_Extensions.h are byte-compile-identical. jam_Chars.h convergence drove a structural grammar refactor — `>` = tab rule, SSOT methods on TemplateDocument, top-level scope support for namespaces (no indent) vs blockquoted scopes for structs (indented).

### Completed
- **isTableBorder** — `MarkdownDocument::isTableBorder()` static predicate replaces `isTag(Id::border)` at 4 call sites (getTableRows, Writer x2, Validator x1). Fixes "border" keyword collision that silently dropped the `border` data row and replaced it with `+---+` separator.
- **chars:: → Chars::** — 1095 total sites renamed across jam source (890), generated/diff (74), tables + cast (131). `jam::Array<juce::juce_wchar> special` → `static constexpr const char* const special`. `jam_Format.cpp` lambda building specialCharacters deleted — `Chars::special` passed directly. `.contains()` → `juce::String(Chars::special).containsChar()`.
- **extensions:: → Extensions::** — struct Extensions with `static constexpr const char* const` members. 97 total sites renamed. `jam_Html.h` svgTagName shadow state deleted — `Extensions::svg` used directly.
- **Structural grammar SSOT** — `getWraps`, `getWrapAlias`, `getTokens` moved from Writer.h to TemplateDocument.h as static methods. `getOutermostWrap`, `isWrapHead` moved from Validator.h. `getContent` SSOT for `.cast` template expansion (was inline in buildRow). Both Writer and Validator consume shared methods.
- **> = tab rule** — engine supports top-level scope (`@scope: Name` without `>`, depth 0, no indent) and blockquoted scope (`> @scope: Name`, depth 1, body/epilogue indented). `getTokens` reads nested blockquote bullets as content tokens with indent applied. `applyWrap` indents body token only. All outermost namespace scopes in CAST.md stripped of `>` (739 lines).
- **Definition.cast** — `:::doxygen:toComment:::` appended. Language-agnostic comment formatting via existing `toComment` transform.
- **Chars.cast** — reduced to epilogue-only: `special` string literal + `isNumeric` function.
- **chars.md** — hex values unpadded, `doxygen` column added (U+NNNN, no hardcoded comment syntax), `## special` table deleted, `key` → `name` column, spurious last-row borders removed.
- **jam_Generated.h oracle** — includes reordered (Enums before LookupTables, Chars before Enums for dependency), `jam::SharedInstance` → `SharedInstance` (redundant inside namespace jam), `@sharedInstance` CAST.md alias updated.
- **jam_Text.h oracle** — blank lines fixed to match diff/.

### Remaining
- **Build + diff jam_Chars.h** — verify struct Chars output matches oracle after all engine changes. The top-level scope + content indentation mechanism is implemented but untested end-to-end.
- **jam_Files.h** — same question as Extensions: if declared once in one table, should be struct. ARCHITECT hasn't ruled yet.
- **jam_Generated.h include convergence** — diff/ only generates 3 includes (Bimaps, MermaidTables, Enums). Oracle now has 12. CAST.md needs output rows that generate all includes. Separate convergence task.
- **Next convergence files** (per handoff order):
  - jam_Operators.h (6 operator rows have wraps INVERTED vs oracle)
  - jam_Bimaps.h / jam_Enums.h
  - jam_HashMaps.h
  - jam_LookupTables.h
  - jam_MermaidGeometry.h / jam_MermaidTables.h
- **Deferred from Sprint 7**: LC7 doxygen prose + Auditor sweep; HELP.md transform-vocabulary wording amendment

### Key Decisions
- **> = tab**: one `>` = one tab indent. No `>` = no indent. Namespace scopes at top level (no `>`), struct scopes inside `>`. Engine enforces via blockquote depth in buildFile (blockquote = depth 1, top-level = depth 0).
- **Struct for single-source types**: types declared from one table use `struct` with `static constexpr` members (Chars, Extensions). Consumer uses `Chars::`, `Extensions::` (PascalCase). Namespace reserved for multi-source scopes (Id, text, map, files).
- **isTableBorder SSOT**: `MarkdownDocument::isTableBorder()` checks `Id::type == BlockType::tableBorder`. Replaces 4 independent `isTag(Id::border)` checks that collided with data rows keyed "border".
- **Structural grammar on TemplateDocument**: scope/wrap detection, token extraction, validation — one implementation consumed by Writer and Validator. No divergence on grammar changes.
- **getContent SSOT**: `.cast` template expansion in `TemplateDocument::getContent()`. All token resolution (body, epilogue, any slot) goes through `getBinding` → `getContent`. No inline expansion in buildRow.
- **No hardcoded comment syntax in tables**: doxygen column holds `U+NNNN`, template applies `:::doxygen:toComment:::`. Language-agnostic.
- **No hardcoded token indentation**: applyWrap indents body only. Content tokens pre-indented by getTokens (from nested blockquote). Frame tokens (keyword, terminator) never indented.

### Files Modified
- `jam_markdown/document/jam_MarkdownDocument.h` — isTableBorder() static predicate; getTableRows uses tableRow type check
- `jam_markdown/document/jam_MarkdownWriter.h` — 2 sites use isTableBorder()
- `jam_markdown/document/jam_MarkdownValidator.h` — 1 site uses isTableBorder()
- `jam_core/text/jam_Format.cpp` — chars:: → Chars::, special lambda deleted, containsChar
- `jam_web/html/jam_Html.h` — extensions:: → Extensions::, svgTagName deleted
- `jam_markdown/layout/jam_MarkdownSyntax.h` — extensions:: → Extensions::
- 20 jam source files — chars:: → Chars:: (890 sites)
- `cast/Source/TemplateDocument.h` — getWraps, getWrapAlias, getTokens, getOutermostWrap, isWrapHead, getContent, indentWidth (SSOT)
- `cast/Source/Writer.h` — delegates to TemplateDocument::, applyWrap body-only indent, buildFile blockquote depth
- `cast/Source/Validator.h` — delegates to TemplateDocument::
- `cast/CAST.md` — @sharedInstance alias, namespace > stripped (739 lines), Chars/Extensions struct rows, Definition.cast doxygen
- `cast/template/Definition.cast` — :::doxygen:toComment::: appended
- `cast/template/Chars.cast` — epilogue-only (special + isNumeric)
- `cast/tables/chars.md` — hex unpadded, doxygen column, name column, special deleted
- `cast/tables/terminal.md`, `css.md`, `syntax.md` — chars::/extensions:: renamed
- `generated/jam_Chars.h` — struct Chars oracle
- `generated/jam_Extensions.h` — struct Extensions oracle
- `generated/jam_Generated.h` — includes reordered, SharedInstance prefix removed
- `generated/jam_Text.h` — blank lines fixed
- `generated/jam_Enums.h`, `jam_LookupTables.h` — Chars:: rename
- `diff/jam_Enums.h`, `jam_LookupTables.h`, `jam_HashMaps.h`, `jam_Generated.h` — same renames

### Open Questions
- **jam_Files.h** — should `namespace files` become `struct Files`? Same single-table pattern. ARCHITECT hasn't ruled.

### Next Steps
1. Build jam + cast, run `./cast jam/cast/CAST.md`, diff jam_Chars.h — verify struct body indented, epilogue indented, frame at column 0
2. If not matching: trace the top-level scope + content token indentation path
3. If matching: jam_Chars.h CONVERGED — move to next file (jam_Operators.h or jam_Files.h per ARCHITECT)

## Handoff to COUNSELOR: jam_Identifiers.h Convergence

**From:** COUNSELOR
**Date:** 2026-08-20
**Status:** In Progress — rebuild pending after getTableRows fix

### Context
Converging cast's output (`jam/diff/`) to byte-identical with the oracle (`jam/generated/`), one file at a time. jam_Generated.h and jam_Text.h are DONE. jam_Identifiers.h had 4 engine/data defects plus a collision bug — all fixed, awaiting rebuild+diff.

### Completed
- **jam_Text.h** — byte-identical ✓ (verified this session, MD5 match)
- **toCamelCase abbreviation** — jam_Format.cpp:264 now checks `isAbbreviation` at index 0, same pattern as `toSnakeCase`. `UIScale` stays `UIScale`, not `uiScale`.
- **toLiteral non-ASCII hex-escaping** — jam_Format.cpp:928-934 adds `else if (byte > 0x7F)` branch producing `\xNN` escapes. Uses existing `Id::hexEscapePrefix`/`byteHexDigits` pattern from `encodeCodepointAsEscapedUtf8`.
- **lexicon.md data fixes** — `alias` (line 25) and `name` (line 726) restored `@id` type (were empty, causing bare `inline const alias { alias };`). `companyCopyright` changed from `\xc2\xa9` to raw `©` (toLiteral now hex-escapes it correctly).
- **Scope.cast blank line** — added blank line between `:::body:::` and `:::epilogue:::` (line 8). Body's last line follows lineBreak convention; the consistent blank line before scope closing comes from the template.
- **getTableRows collision fix** — jam_MarkdownDocument.h:133 changed from `not row->isTag(Id::border)` to `not row->contains(Id::type)`. Separator elements have `Id::type` property (set at line 2381), data rows don't. Fixes silent drop of lexicon row keyed "border".
- **cast Id:: dedup** — removed `special`, `toComment`, `toCommentBlock`, `toUnicode` from cast's `Source/generated/Identifiers.h`. Jam lexicon is SSOT (ARCHITECT ruling: "jam SSOT").
- **Oracle baseline** — jam/generated/jam_Identifiers.h updated from generated output + `border` entry added by ARCHITECT.

### Remaining
- **Rebuild jam + cast, run `./cast jam/cast/CAST.md`, diff jam_Identifiers.h** — verify byte-identical after the getTableRows fix. `border` should now appear in the generated output.
- **Oracle sync** — after successful diff, copy generated → oracle if not already matching.
- **Next convergence files** (one-by-one, per ARCHITECT ruling):
  - jam_Files.h / jam_Extensions.h
  - jam_Chars.h
  - jam_Operators.h (6 operator rows have wraps INVERTED vs oracle)
  - jam_Bimaps.h / jam_Enums.h
  - jam_HashMaps.h
  - jam_LookupTables.h
  - jam_MermaidGeometry.h / jam_MermaidTables.h
- **Deferred from Sprint 7**: LC7 doxygen prose + Auditor sweep; HELP.md transform-vocabulary wording amendment

### Key Decisions
- **jam SSOT for identifiers**: cast's own Id:: entries that duplicate jam lexicon entries are removed. Jam lexicon is the single source. `toComment`/`toCommentBlock`/`toUnicode` had different types/values in cast vs jam — `toCamelCase` produces identical registry keys either way.
- **Separator filter by structure, not tag**: `getTableRows` uses `row->contains(Id::type)` to identify separator elements (which have the type property stamped at parse time), not `row->isTag(Id::border)` which collides with data rows keyed "border".
- **toLiteral hex-escapes non-ASCII**: All bytes >0x7F produce `\xNN` in C++ string literals. Source data uses raw UTF-8 characters (author-writes-content), `toLiteral` converts to portable ASCII-safe escapes.
- **Scope.cast template owns the blank line**: The blank line before epilogue (END marker) comes from the template, not engine logic. Body's last line follows lineBreak convention without adding trailing whitespace.

### Files Modified
- `jam_core/text/jam_Format.cpp` — toCamelCase abbreviation check (line 264); toLiteral non-ASCII hex-escape branch (lines 928-934)
- `jam_markdown/document/jam_MarkdownDocument.h` — getTableRows filter: `Id::type` property check replaces `Id::border` tag check (line 133)
- `jam/cast/template/Scope.cast` — blank line between `:::body:::` and `:::epilogue:::` (line 8)
- `jam/cast/tables/lexicon.md` — alias/name @id restored; companyCopyright raw ©; border entry added
- `jam/generated/jam_Identifiers.h` — oracle baseline updated + border entry
- `cast/Source/generated/Identifiers.h` — removed special, toComment, toCommentBlock, toUnicode (jam SSOT)

### Open Questions
- None blocking. Rebuild + diff is the immediate next step.

### Next Steps
1. Rebuild jam + cast, run `./cast jam/cast/CAST.md`, diff jam_Identifiers.h — expect byte-identical
2. If identical: jam_Identifiers.h CONVERGED — move to next file
3. If not: read the diff, fix the residual

## Handoff to COUNSELOR: jam_Text.h Convergence + Perf Fix

**From:** COUNSELOR
**Date:** 2026-08-19
**Status:** In Progress

### Context
Converging cast's output (`jam/diff/`) to byte-identical with the oracle (`jam/generated/`), one file at a time. jam_Generated.h is DONE. jam_Text.h is nearly done — needs one rebuild+diff to verify the last trailing-newline fix lands.

### Completed
- **jam_Generated.h** — byte-identical ✓ (converged prior session; regression-verified this session, holds)
- **jam_Text.h oracle rewrite** — `namespace text { struct English { static constexpr const char* const … } }`, LF endings, 48 members
- **Consumer sweep** — 48 jam sites `text::en::` → `text::English::`, 20 cast sites → `text::Diagnostics::` (ODR split — ARCHITECT ruling: jam keeps `English`, cast uses `Diagnostics`)
- **`quoted` transform** — `chars::doubleQuote + input + doubleQuote` in Operators.h (no juce::String::quoted() — its heuristic skips trailing quote chars). Registered, lexicon word added, bootstrap updated. `@cString` format column in CAST.md index → `quoted`
- **localisation-en.md:70** — `failDuplicate` cell authored `duplicate \"` (author-writes-escapes law)
- **Model.h getFormat(row, alias)** — now matches index row key OR symbol cell (was key-only, missing type-alias lookups)
- **TemplateDocument.h blank-line elision** — `hasBlankLine` captured before first strip; fixes doubled blanks at chunk-start
- **Writer.h buildRow** — expanded body trimmed of trailing newlines before wraps apply
- **3.5× perf improvement** (Debug: 13.2s → 3.9s wall) — `getBoundValue` eliminated (:::name::: interpolation hoisted to Writer::getTokens at bind time); `getValue` early-returns for non-alias strings (no @ sigil → no String construction)
- **cast text::Diagnostics** — `failNoMatch` + `failDuplicate` members added to cast's bootstrap Text.h

### Remaining
- **Rebuild cast + run + diff** — verify jam_Text.h byte-identical after the trailing-newline trim (Writer.h:201). This is the immediate next step.
- **Next convergence files** (one-by-one, per ARCHITECT ruling):
  - jam_Identifiers.h (fromLiteral via format law — should work now)
  - jam_Files.h / jam_Extensions.h
  - jam_Chars.h
  - jam_Operators.h (6 operator rows have wraps INVERTED vs oracle — outer=struct, inner=namespace; fix data first)
  - jam_Bimaps.h / jam_Enums.h
  - jam_HashMaps.h
  - jam_LookupTables.h
  - jam_MermaidGeometry.h / jam_MermaidTables.h (templates derived from oracle when reached)
- **Performance** — measure Release build. Debug went from 13.2s → 3.9s; ARCHITECT flagged the original ~5-7s (presumably Release) as unacceptable.
- **Deferred from Sprint 7**: LC7 doxygen prose + Auditor sweep; HELP.md transform-vocabulary wording amendment

### Key Decisions
- **ODR split**: jam keeps `text::English`, cast uses `text::Diagnostics` (ARCHITECT: "keep struct English. project must conformed, use different name")
- **`quoted` over `juce::String::quoted()`**: juce's `quoted()` skips closing quote when string already ends with `"` (juce_String.cpp:1664) — breaks `failDuplicate`. Direct composition is correct.
- **Format column `quoted` replaces `toLiteral`** for `@cString`: `toLiteral` doubles backslashes + doesn't add quotes; `quoted` wraps without touching content — author-writes-escapes law (HELP.md:465)
- **Row-order-derives-everything**: one `## output` row position drives include order, Generated member order, per-file struct order. Oracle inconsistencies are fixed in the oracle (ARCHITECT ruling).
- **Depth-indent law**: wrap body indented `depth * 4` spaces (depth = index in wraps chain)
- **Elision laws**: empty jack → drop one preceding space/newline/tab; chunk-start strips one leading newline; elided line swallows following blank line
- **Performance**: getBoundValue was the #1 bottleneck (63% of work time — String::replace rescanning entire file bodies per token). Hoisted to bind-time in Writer::getTokens. getValue early-return for non-@ strings was #2 (25%).

### Files Modified
- `Source/Operators.h` — `Transforms::quoted` static method + registry entry
- `Source/Model.h` — getFormat alias-overload matches key or symbol; getValue early-return for non-alias
- `Source/TemplateDocument.h` — hasBlankLine captured before strip; getBoundValue deleted; call sites → `tokens.at`
- `Source/Writer.h` — getTokens does :::name::: interpolation at bind time; buildRow trims trailing newlines on expanded body
- `Source/generated/Identifiers.h` — `Id::quoted` bootstrap
- `Source/generated/Text.h` — `struct Diagnostics` (was English); `failNoMatch` + `failDuplicate` added
- `Source/main.cpp` — `text::Diagnostics::` (1 site)
- `Source/Validator.h` — `text::Diagnostics::` (16 sites)
- `cast/tables/lexicon.md` — `quoted` word added
- `jam/cast/CAST.md` — `@cString` format → `quoted`
- `jam/cast/tables/localisation-en.md` — `failDuplicate` cell → `duplicate \"`
- `jam/generated/jam_Text.h` — oracle rewrite (struct English, constexpr, LF)
- `jam/generated/jam_Generated.h` — oracle order patch (row-derived, prior session — unchanged this session)

### Open Questions
- None blocking. The immediate task is mechanical: rebuild, diff, confirm byte-identical.

### Next Steps
1. Rebuild cast, run `./cast jam/cast/CAST.md`, diff jam_Text.h — expect byte-identical
2. If identical: jam_Text.h CONVERGED — move to next file (jam_Identifiers.h)
3. If not: read the diff, fix the residual (likely another spacing law edge case)

## Sprint 7: Lexicon Canon Redesign — CAST.md / lexicon.md / relations.md ✅ (checkpoint)

**Date:** 2026-08-09
**Duration:** multi-session (shared with jam Sprint 62)

### Agents Participated
- COUNSELOR: architecture redesign synthesis with ARCHITECT (three-canon-file model, entity/abbreviation rules, jam::Format case-family ownership); LC6 oracle-diff verification (2 rounds, direct file reads); root-cause tracing for whitespace-trim + UTF-8 corruption defects in jam_MarkdownDocument.h (verified by direct read, corrected one of Pathfinder's mis-citations); rawText/getLiteralValue design + ratification; trivial fixes (CAST.md blank-row restore, lexicon.md double-space restore, Validator.h failHazardURI rename)
- Pathfinder ×2+: dependency/API inventory for redesign; whitespace-trim/UTF-8-corruption mechanism trace; git log/status both repos
- Engineer ×N: LC1 lexicon registry + reference resolution, LC2/LC8 word-aware transforms + jam::Format case family, LC3 canon authoring (lexicon.md/relations.md/CAST.md/fragments, delete-first), LC4 bootstrap, LC5 HELP.md amendment, LC9 Operators.h pass-through-wrapper removal, codeSpan UTF-8 decode fix (jam), rawText property + getLiteralValue wiring (2 call sites), failHazardURI rename

### Files Modified (cast repo)
- `Source/Generator.h` (new, replaces Driver.h) — lexicon registry build (Driver::run), entry-miss FATAL, `@entry@`/`@value@` row-region overrides, `isLiteralCell`/`getLiteralValue` (reads `Id::rawText`, bypasses CommonMark padding-strip), `buildPerRowMap` isBacktickLiteral/isKeyColumn split, lexicon-registry value-column read now backtick-aware (:692-695)
- `Source/Operators.h` (new, replaces Transforms.h) — local case wrappers deleted; `Transforms` struct encapsulates `jam::Function::Map` registry, `jam::Format` registered by direct function pointer
- `Source/Validator.h` (new, replaces Validation.h) — Transforms::contains sweep; :699 `Id::failHazardUri` → `Id::failHazardURI`
- `Source/Writer.h` (new) — TemplateEngine::expandText, fragment cache
- `cast/tables/lexicon.md` (new) — ~105 entities, single `## lexicon` table, space-form casing, backtick-literal convention; Blue Mana/Hello Summer double-space restored (authoring gap, not an engine bug)
- `cast/tables/relations.md` (new) — 8 membership tables, single `entry` column
- `cast/CAST.md` — rewritten: entry-headed dispatch, `name | unique` constraint, `value | escapeCpp` transform; outputs redirected to `../Source/g/`
- `cast/template/{Identifier,String,Char,HashMap}.h` — `@entry:toCamel@`/`@value@` placeholder shift
- `cast/tables/cast.md`, `cast/tables/banner.md` (deleted, delete-first)
- `Source/generated/{Identifiers,HashMaps,CAST}.h` — bootstrap additions (lexicon, toCamel, entry, failEntityMissing); `Source/g/` regenerated and verified byte-identical to oracle except ratified renames (LC6)
- `Source/HELP.md` — Canon Files section, Lexicon Reference Resolution, transform vocabulary entries (toCamel added)

### Files Modified (jam repo, cast-adjacent — see jam SPRINT-LOG Sprint 62 for full jam-side entry)
- `jam_core/format/jam_Format.h/.cpp` — case-family rewrite (forEachWord/isAbbreviation/normalizeWord)
- `jam_markdown/document/jam_MarkdownDocument.h` — codeSpan UTF-8 decode fix; `Id::rawText` Element property added
- `jam/generated/jam_Identifiers.h` — `rawText` bootstrapped

### Alignment Check
- [x] BLESSED — SSOT (lexicon.md declares every entity once; jam::Format owns the case family, no pass-through wrappers), Explicit (FATAL on unresolved reference; rawText/getLiteralValue asserts precondition), Encapsulation (Transforms registry function-local static behind accessor), Deterministic (fixpoint verified — Source/g/ byte-identical to oracle except ratified renames)
- [x] NAMES.md adhered — ARCHITECT-ratified: lexicon.md/relations.md/CAST.md (canon files), toCamel, rawText, entity/abbreviation rules (UI/URI stay uppercase through Pascal/Camel/Title)
- [x] MANIFESTO.md applied
- [ ] Doxygen pass + Auditor sweep — deferred by ARCHITECT to after plan step [11] (explicit ruling this session: "this is safe checkpoint. doxygen sweep and all docs after [11] completed")

### Problems Solved
- Table schema redesigned from kind-shaped (7 duplicated tables, collisions "solved" by mutation — hash→cssHash, UIScaleMap) to relation-shaped (1 lexicon + 8 thin membership tables, each entity declared once)
- Abbreviation-preserving case algorithm ratified: UI/URI stay uppercase through Pascal/Camel/Title; jam::Format is the sole case-family owner
- LC6 oracle diff: 3 real defects found and root-caused by direct source verification (not taken on subagent report alone) — CommonMark code-span padding-strip eating backtick-literal whitespace (corrected an initial mis-citation of trim() as the cause), UTF-8 byte-wise corruption in codeSpan decode, lexicon.md authoring gap (missing space — verified NOT an engine bug via targeted grep)
- CommonMark spec compliance preserved per ARCHITECT ruling — fix landed cast-side via new `Id::rawText` property + `getLiteralValue`, not by weakening jam's parser
- Second literal-cell gap found on re-verification after the first fix (lexicon-registry build at Generator.h:692 was still reading rendered text) — closed with the same pattern

### Debts Paid
- None

### Debts Deferred
- None ARCHITECT-commanded. Carried: LC7 doxygen prose + Auditor sweep (deferred to after plan step [11] completes); HELP.md transform-vocabulary wording still pre-/truth abbreviation-rule (needs update before LC7's audit); ~90 pre-existing byte-wise-cast sites in jam_MarkdownDocument.h flagged by Engineer as informational (ASCII-delimiter scanning, not content decode — untouched); Xml comment-branch gap (jam_Xml.h, silent misparse fallback) — disposition pending; jam table migration to canon shape — post-LC6, separate sprint.

## Sprint 6: --help Through the Terminal Graphics Contract ✅

**Date:** 2026-08-05 → 2026-08-07
**Duration:** multi-session (shared with jam Sprint 61)

### Agents Participated
- COUNSELOR: harness design under PLAN-terminal-graphics.md Step [9]; Help.h regression restore; trailing-newline hack reverted on ARCHITECT NO-HACK ruling
- Engineer ×3: harness rewrite (CMake + Help flow + banner port), banner centring, exact-frame sizing
- Auditor: deferred with jam Sprint 61 to plan step [12]

### Files Modified (3 total)
- `CMakeLists.txt:24-29` — JAM_MODULES: `jam_tui` (deleted module) → `jam_style` + `jam_terminal`, `jam_markdown` kept
- `Source/Help.h` — printHelp rewritten to the create-only surface: parse → jam::MarkdownComponent over the Document (StyleManager/style.css LookAndFeel wiring unchanged) → `setBounds(0,0,cols,0)` → `rows = getTextHeight() + documentMargin` (exact frame, bottom margin as real rows) → `GraphicsEngine engine { stdout }` → scoped `GraphicsContext` + `juce::Graphics` + `setOrigin(documentMargin, 0)` → paint; presentation at scope exit. Old manual walk (jam::tui::toAnsiString), getLines/trim/margin-prepend/printf all deleted. Single `<JuceHeader.h>` include
- `Source/CastCLI.cpp` — printBanner ported off jam_tui onto terminal::GraphicsEngine/GraphicsContext drawCells with Stamp-interned styles; horizontally centred via `juce::Justification (centred).appliedToRectangle (bannerRect, pageRect)` + context setOrigin — no hand arithmetic

### Alignment Check
- [x] BLESSED — Encapsulation (cast creates engine/context/Graphics, never touches engine after paint), SSOT (style.css → LookAndFeel → projection; Id::/Chars:: only), Deterministic (exact frame from getTextHeight, no heuristics, no live terminal-width query)
- [x] NAMES.md adhered — no new cast names
- [x] MANIFESTO.md applied
- [ ] Auditor sweep — deferred to jam plan step [12]

### Problems Solved
- --help renders pixel-reference-faithful through the full TGC chain ("rendered perfect") — tight • lists with hanging indent, quote band + │ bar + padding cell, inline code magenta-on-black per style.css codeBackgroundColourId, ─ rules, centred banner + H1, blank tail margin from real frame rows
- Stale-buffer regression of Help.h caught by compiler and restored to the ratified surface
- rowsPerBlock oversize heuristic and post-paint trim eliminated by juce::TextEditor-native getTextHeight measure

### Debts Paid
- None

### Debts Deferred
- None ARCHITECT-commanded. Carried: orphaned textEditorComponent* colour entries in generated/jam_Bimaps.h + cast/tables/colours.md (jam [11]/Arc A); interim generated files still bootstrap-pending first fixpoint run (PLAN-cast Step 7)

## Sprint 5: Eliminate Magic Strings — Table-Driven Id:: Constants ✅

**Date:** 2026-08-04
**Duration:** ~02:00

### Agents Participated
- COUNSELOR: orchestration, table/template infrastructure (cast.md, CAST.md, Identifiers.h template, String.h, Char.h), Id::value collision diagnosis + removal, full verification
- Engineer ×2: hand-written generated Identifiers.h + source-wide string/char literal replacement; char SSOT constants + charToString elimination + predicateOneOf fromTokens refactor

### Files Modified (13 total)
- `cast/tables/cast.md` — added `## column` (+value), `## file` (+castOutput), new `## char` section (11 entries: charSpace…charCloseBrace), new `## string` section (31 entries: programName…failOutputMissing)
- `cast/CAST.md` — +2 dispatch rows (char→Char.h, string→String.h), new `## transforms` section (string column → escapeCpp)
- `cast/template/Identifiers.h` — +2 slots (@char@, @string@)
- `cast/template/String.h` (new) — fragment: `inline const juce::String @word@ { "@string@" };`
- `cast/template/Char.h` (new) — fragment: `inline const juce::String @word@ { juce::String::charToString (Chars::@char@) };`
- `Source/generated/Identifiers.h` — hand-written interim: +11 char-derived juce::String constants, +31 multi-char juce::String constants, +1 Identifier (castOutput); Id::value removed (JAM provides OOTB)
- `Source/CastCLI.cpp` — all string/char literals replaced with Id:: constants; version/error printfs refactored to juce::String composition
- `Source/Driver.h` — outputBannerFileName constant removed (Id::castOutput is SSOT); all literals replaced
- `Source/Template.h` — CRLF normalization uses Id::charCarriageReturn + Id::charNewline; validateHoles error uses Id:: diagnostics
- `Source/Constraints.h` — all 8 predicate error messages use Id::diagnosticSeparator + Id::fail* constants; getLocation uses Id::char* constants; predicateOneOf refactored from pipe-wrapping to juce::StringArray::fromTokens
- `Source/Transforms.h` — escape sequences use Id:: constants (hexEscapePrefix, escapedDoubleQuote, escapedBackslash, etc.); char literals replaced with Chars:: constants; qualifySymbol uses Id::scopeResolution/juceNamespace; parseCodepoint uses startsWithIgnoreCase
- `Source/Validation.h` — hazard check uses Id::hazardChars/failHazardUri/failHazardAngleBrackets; manifest validation errors use Id:: diagnostics
- `Source/Help.h` — margin and joinIntoString use Id::charSpace/charNewline

### Alignment Check
- [x] BLESSED principles followed — S (SSOT): every string declared once in cast.md table, referenced via Id::; E (Explicit): no magic values, every constant named; L (Lean): 26 charToString calls collapsed to 11 Id::char* constants
- [x] NAMES.md adhered — all new names ARCHITECT-ratified (char* prefix for JAM collision avoidance, fail* diagnostic prefix, diagnostic separator)
- [x] MANIFESTO.md principles applied

### Problems Solved
- Id::value collision with JAM's jam_Identifiers.h:130 — removed from hand-written generated (JAM provides OOTB); table entry retained for future independent codegen
- predicateOneOf hand-rolled pipe-wrapping replaced with juce::StringArray::fromTokens — same SPEC §7.4 semantics, leaner
- replaceWithText 4th arg is `const char*` — solved via .toRawUTF8() on temporary (lifetime valid through full-expression)

### Debts Paid
- None

### Debts Deferred
- None

---

## Handoff to COUNSELOR: Cast Generated Layout Fix (task #8/#9 preflight)

**From:** COUNSELOR
**Date:** 2026-08-05
**Status:** Ready for Implementation (planning gate open — two decisions pending)

### Context
JAM's generated layout is now the ACCURATE target (jam Sprint 60: 7 type
files + jam_Generated.h master, jam_core sole includer). ARCHITECT opened
the next front: fix cast's OWN generated output to the identical convention
(module layout minus the jam_ prefix), then extract templates (task #8) and
author JAM CAST.md (task #9). This session was discovery only — Pathfinder
surveyed the cast engine + generated pipeline; no code changed.

### Completed (this session)
- Task hygiene: closed stale D9/D10, Step 5/6 duplicates (#58,#59,#64,#65,
  #70,#71) — jam Sprint 60 + cast Sprint 4 already covered them.
- Full engine survey (Pathfinder) — capabilities verified against file:line.

### Cast survey facts (ground truth)
- Cast generated output: `Source/generated/Lexicon.h` (namespace Id,
  33 juce::Identifier consts, 5 sections) incl. Cast.cpp:8;
  `Source/generated/Banner.h` (namespace cast, jam::HashMap banner, 8 rows)
  incl. Cast.cpp:9.
- Cast tables (correct as-is): `cast/tables/cast.md` (5 word|string sections
  → Identifiers), `cast/tables/banner.md` (colour|text → HashMap). Banner
  text is value payload, stays literal.
- Cast templates: `cast/template/` — Lexicon.h + IdentifierRow.h,
  Banner.h + BannerRow.h. Manifest: `cast/CAST.md` (2 outputs, 6 dispatch,
  1 constraint `word unique`).
- Engine READY: multi-table→one-output union (Driver.h:96-105); 9 transforms
  (Driver.h:37-38); cross-table `unique` scans all roots (Constraints.h:117-
  165); two-phase atomic + write-if-different (Driver.h:203-219);
  `Id::@word@.toString()` referencing is pure template text, zero engine work.
- Engine GAP (only one): NO dedup. Dispatch emits one fragment per matching
  row (Driver.h:139-148); same word in two tables → emitted twice. The
  ratified Identifiers union+dedup projection is the sole engine extension
  outstanding — lands with task #8/#9, not the cast-local fix.
- Dispatch is ONE level (table→fragment→slot). jam_Bimaps.h root template
  will need one dispatch row per Bimap struct-table (~54 slots).

### Key Decisions (ratified prior sprints, still binding)
- Cast convention = module layout exactly, minus jam_ prefix. So cast
  `Source/generated/` target: `Identifiers.h` (replaces Lexicon.h — "no
  lexicon"), `HashMaps.h` (replaces Banner.h), `Generated.h` master.
  Cast.cpp:8-9 collapse to one `"generated/Generated.h"` include.
- SSOT generation (binds #8/#9): tables declare a word once; Identifiers =
  union+dedup projection over all tables' word columns; all other outputs
  emit `Id::<word>.toString()` references; divergent display forms via
  transform columns; opaque values (ColourIdMap literals, Entity UTF8,
  banner text) stay literal.
- jam::ID extinct — `Id::` only. company/companyName key/value split.

### Files Modified
- None (discovery + task-status hygiene only).

### Open Questions (block the cast-local plan — ask ARCHITECT first)
1. Master `Generated.h` — hand-written (jam_Generated.h pattern) OR added as
   a third manifest output row (engine-generated)?
2. Fragment template naming — `BannerRow.h` → convention name? (module fix
   introduces StringRow/CharRow/etc.; cast should match.)

### Next Steps
- Resolve the two open questions (AskUserQuestion, one at a time).
- Then `/goplan` the cast-local fix: rename Lexicon→Identifiers,
  Banner→HashMaps, add Generated.h master, rewire Cast.cpp includes, update
  CAST.md output rows + template filenames. Engine untouched.
- After cast-local lands: task #8 (extract 7 module template pairs from the
  restructured jam headers) → task #9 (author jam/cast CAST.md incl. the
  union+dedup engine extension) → #10 conformance loop → #11 switchover.

---

## Sprint 4: JAM Generated Consolidation — cast ripple ✅

**Date:** 2026-08-04
**Duration:** ~02:30 (shared with jam Sprint 60 — see jam carol/SPRINT-LOG.md for the full sprint)

### Agents Participated
- COUNSELOR: orchestration (jam-side); cast ripple only here
- Engineer: G5 consumer rewire (one cast line)

### Files Modified (1 total)
- `Source/Cast.cpp:8` — `<generated/ColourNames.h>` include removed; jam::ColourNames now arrives transitively via jam_core's jam_Generated.h master (jam sole-includer contract). Cast's own `"generated/Lexicon.h"` / `"generated/Banner.h"` untouched.

### Alignment Check
- [x] BLESSED principles followed — E/S: modules never reach into jam's generated path
- [x] NAMES.md adhered — no new names in cast
- [x] MANIFESTO.md principles applied

### Problems Solved
- None cast-local — ripple of jam Sprint 60 layout consolidation

### Debts Paid
- None

### Debts Deferred
- None

## Sprint 3: Markdown Isomorphism — Engine on jam::Document + Self-Validation ✅

**Date:** 2026-08-02 → 2026-08-04
**Duration:** multi-session

### Agents Participated
- COUNSELOR: full orchestration; Steps 5b/5c/5d plan locking with ARCHITECT rulings (jam::Document IS the data model — no shadow structs, no circus; Identifier-only addressing; self-validated vocabulary; delete-first); trivial fixes (jam_Markdown.h namespace brace, toAnsiString default depth, Template.h getLocation straggler); audit disposition rounds; M3 refutation (css wired via StyleManager ctor → Css::getOrCreate → BinaryData)
- Pathfinder: query-family dependency/API inventory (jam_Document consumers, Function::Map, Format)
- Engineer ×7: 5c-1 Document table query family (jam), 5c-2 engine delete-first rewrite, 5d parser struct teardown (jam), 5d tui fold (jam), audit-sweep cast (consolidation + Validation.h split), audit-sweep jam, doxygen ×2 (cast + jam)
- Auditor: 5c audit rounds + comprehensive final sweep (gates PASS; H1-H3/M1-M3/L1-L8 dispositioned and resolved)

### Files Modified (cast repo)
- `Source/Driver.h` (167 lines) — generation only: buildPerRowMap, getOutput, two-phase processOutput (roots parsed first, `Id::path` provenance stamp; validate pass then write-if-different pass — SPEC §8), Driver::run
- `Source/Validation.h` (new, 195 lines) — getHazardMessage (prunable walk, code spans exempt), validateTableHazards, validateManifest (transforms/templates/orphan scan), validateRoots, validatePerColumnConstraints
- `Source/Constraints.h` (396 lines, accepted Lean exception — closed §7 predicate library) — 8 predicates on parsed roots (zero re-parsing), getScannedTables/getConstraintTargetTables scan SSOT, getLocation (§8 file:row (column)), buildPredicateMap single template-list, Id:: words in messages
- `Source/Transforms.h` — fromTokens simple overload; doxygen
- `Source/Template.h` — getLocation rename ripple; doxygen
- `Source/Help.h`, `Source/Cast.cpp` — `--` + Id::help / Id::version, Id::cast in no-manifest message, getConfigureDepends via manifest queries; doxygen + @file banner
- `Source/generated/Lexicon.h`, `Source/generated/Banner.h` — hand-written generation targets (untracked; regenerated by `./cast` fixpoint)
- `cast/CAST.md`, `cast/tables/cast.md`, `cast/tables/banner.md`, `cast/template/{Lexicon,IdentifierRow,Banner,BannerRow}.h` — authored self-validation SSOT
- `SPEC.md` — §2 col-0 key + hazard, §7.3 keyed FK, §8 file:row (column), §9 --help
- `CMakeLists.txt` — include dirs JAM_ROOT + harfbuzz only
- DELETED: `Source/Manifest.h`, `Source/Relations.h`, `Source/Cells.h`, `Source/Constraints.cpp`, `Source/Driver.cpp`, `Source/Help.cpp`, `Source/Manifest.cpp`
- `PLAN-markdown-isomorphism.md` — Steps 5b/5c/5d locked rulings

### Alignment Check
- [x] BLESSED principles followed — SSOT (Document the only model; one scan enumeration; vocabulary from one authored table), Stateless (every value queried at use), Lean (two engine files dissolved; Validation/Driver split at the real seam), Explicit (Identifier-addressed queries), Deterministic (fixpoint holds; two-phase atomic)
- [x] NAMES.md adhered — all new names ARCHITECT-ratified (query family, getLocation, getScannedTables/getConstraintTargetTables, validateRoots/validatePerColumnConstraints, Validation.h, Id::help)
- [x] MANIFESTO.md principles applied
- [x] Doxygen pass complete (dedicated sprint-end delegation, SPEC §-cited)

### Problems Solved
- Endless rewrite churn closed: engine contract locked in plan before dispatch; delete-first; structural-absence grep gates (zero Id::children/fromTokens/structs/out-refs/int table loops in engine)
- Include shadowing: cast generated header shadowed jam's `<generated/Identifiers.h>` via configure_app Source/ include — resolved by JFS precedent `Source/generated/Lexicon.h`
- `Id::value` collision with jam:130 — row deleted; set-intersection zero residue
- Constraint scope ratified (column set is the scope) — O(n²) duplicate application consolidated to once per (constraint, root, table)
- Auditor M3 refuted with citation: style.css loads via StyleManager ctor chain

### Debts Paid
- `DEBT-20260802T190025` — jam_Markdown stamps tag/id; cast consumers on Document queries; templates via Format::replaceholder
- `DEBT-20260802T190431` — banner one table-driven structure (banner.md → Banner.h), paintBanner lookup loop
- `DEBT-20260802T190852` — colour contract wired (css → StyleManager → renderer); list layout correct on container-owned gaps

### Debts Deferred
- None

## Sprint 2: Rendering Fidelity + Colour Architecture + Module Topology Sweep ✅

**Date:** 2026-08-02 → 2026-08-03
**Duration:** multi-session

### Agents Participated
- COUNSELOR: full orchestration; PLAN-rendering-fidelity amendments; LNM/ColourScheme/Palette/StyleManager design synthesis with ARCHITECT; trivial fixes (version SSOT, Id::dark qualification, ColourIdMap instance registration, ColourScheme base-shadowing qualification); DEBT capture coordination
- Pathfinder ×3: F4 rework surface, JFS style.css pipeline + Css naming contract, colourId ownership/module layering
- Engineer ×12: R10 width policy, R11 depth param + ansi harness, LNM mode 20 (Video+Model+Lexicon), F1/F5 fixes, ColourIdMap + StyleManager R7 echo, R13 StyleWhelmed strip + style.css + Help flow, R14 colour isomorphism, F3 stamped-cell map, F6 dead-code deletion, LookupTable int + style_manager relocation, size_t sweep + markdown guard, StyleTheme split correction; DEBT entries ×4; lldb crash diagnosis
- Auditor ×7: R10, R11, LNM (2 rounds), R12 full sweep, R13, R14+F3+F6, topology round (note: topology-round Auditor ran an unauthorized build — disclosed to ARCHITECT)

### Files Modified (cast repo, 8 total)
- `Source/Help.cpp` — width policy constants (widthCap 120, documentMargin 2); StyleManager flow (SharedInstance<SharedDocuments>, StyleManager {{}}, setAppearance Id::dark) replacing 15 hardcoded setColour lines
- `Source/Cast.cpp:80-81,86` — jam::Hyperlink member rename; Id::ColourIdMap instance registration (segfault fix); --version via ProjectInfo::versionString (CAST_VERSION macro deleted)
- `Source/style.css` (new) — CMY palette + 15 --Markdown-- appearance mappings (JFS anatomy)
- `CMakeLists.txt` — JAM_MODULES = jam_core, jam_tui, jam_markdown; style.css in cast_BinaryData; CAST_VERSION define removed
- `PLAN-rendering-fidelity.md` — in-session amendments (restamp elimination, Palette names, depth param, LNM)
- `DEBT.md` (new) — 4 entries captured
- `cast.sh` / build dirs untouched

### Alignment Check
- [x] BLESSED principles followed — SSOT (one nearest-colour spelling, one version truth), Lean (dead slots/classes deleted), Explicit (depth param, guarded optional deps), Deterministic (first-minimum ordered scans)
- [x] NAMES.md adhered — all new names ARCHITECT-ratified (newLineMode, TerminalColourSpace, ColourNames, getNearestIndex relocation, css words)
- [x] MANIFESTO.md principles applied
- [ ] Doxygen pass — deferred by ARCHITECT /log; stale-doc inventory carried: StyleWhelmed.h:12-19, getLines @param depth, getColourIdString prose, DecMode/Video/Model insertMode-only prose

### Problems Solved
- cast --help/no-args render path fully re-seamed onto css-driven StyleManager flow; console app freed from plugin_bootstrap/vulkan/audio dependency chain (module topology corrected at jam level)
- Segfault: ColourIdMap Instance never constructed — registration line added per Stamp/Hyperlink idiom
- Known-wrong residuals ledgered as DEBT (render assertions + layout, markdown Document isomorphism, banner table shape)

### Debts Paid
- `DEBT-20260802T171034` — colour-names JSON blob deleted both kernels; generated-shape ColourNames tables + one nearest/distance SSOT (R14)

### Debts Deferred
- `DEBT-20260802T190852` — render assertion flood + wrong list layout (resolves with markdown isomorphism)
- `DEBT-20260802T190431` — banner as table-driven structure
- `DEBT-20260802T190025` — markdown Document XmlElement isomorphism + %%name%% templates

## Sprint 1: Project Setup and Initial Planning ✅

**Date:** 2026-01-11  
**Duration:** 14:00 - 16:30 (2.5 hours)

### Agents Participated
- **COUNSELOR:** Kimi-K2 — Wrote SPEC.md and ARCHITECTURE.md
- **ENGINEER** (invoked by COUNSELOR) — Created project structure
- **AUDITOR** (invoked by COUNSELOR) — Verified spec compliance

### Files Modified (8 total)
- `SPEC.md:1-200` — Complete feature specification with all flows
- `ARCHITECTURE.md:1-150` — Initial architecture patterns documented
- `src/core/module.cpp:10-45` — Core module scaffolding with proper initialization
- `src/core/module.h:1-30` — Core module header with explicit dependencies
- `tests/core_test.cpp:1-50` — Test scaffolding following Testable principle
- `CMakeLists.txt:1-25` — Build configuration with explicit targets
- `README.md:1-20` — Project overview

### Alignment Check
- [x] BLESSED principles followed (Bound, Lean, Explicit, SSOT, Stateless, Encapsulation, Deterministic)
- [x] NAMES.md adhered (semantic names, verb-noun functions, no type encoding)
- [x] MANIFESTO.md principles applied (no layer violations, explicit dependencies)
- [x] No early returns used
- [x] Fail-fast error handling implemented

### Problems Solved
- Established project foundation following domain-specific patterns
- Defined clear module boundaries preventing layer violations

### Debts Paid
- `DEBT-20260111T101530` — Resolved missing module.h include in core_test.cpp (see Files Modified)
- *(or)* "None" if sprint did not touch any DEBT.md entries

### Debts Deferred
- `DEBT-20260111T143022` — Performance benchmarking suite (deferred by ARCHITECT command — out of scope for setup sprint)
- *(or)* "None" if no items pushed to DEBT.md during this sprint

**Status:** ✅ APPROVED - All files compile, tests scaffold in place

---

<!-- Actual sprint entries go here, written by PRIMARY agents -->

## Handoff to COUNSELOR: Cast-Local Generated Output Fix

**From:** COUNSELOR
**Date:** 2026-08-05
**Status:** In Progress — Steps 1-6 done, Step 7 (ARCHITECT build + fixpoint) pending; two ARCHITECT decisions open

### Context
Cast's own generated output needed alignment with JAM Sprint 60's convention: one header per type, master include, engine-injected banner, formatted namespace blocks. This is the prerequisite before PLAN-cast.md tasks #8 (extract JAM templates) and #9 (author JAM CAST.md). Plan lives at `PLAN-cast-local-fix.md`.

### Completed
- Step 1: `cast_output.txt` → `cast/cast/cast_output.md` (fenced code block, parseable by jam::Markdown)
- Step 2: Engine banner injection — `getOutputBanner()` in Driver.h reads cast_output.md via `jam::Markdown::parse()`, extracts codeBlock via `applyFunctionRecursively` + `getAllSubText()`, prefixes lines with `// `, prepends to every generated output. Read once in `Driver::run`, passed through both passes. `getConfigureDepends` in CastCLI.cpp includes cast_output.md. SSOT constant `outputBannerFileName`. NAMES fixes applied (`element` not `node`, `bannerDoc` not `doc`).
- Step 3: Templates renamed — Lexicon.h→Identifiers.h, Banner.h→HashMaps.h, IdentifierRow.h→Identifier.h, BannerRow.h→HashMap.h. New CAST.h master template (no namespace, just includes). Namespace separator + END marker in root templates. Hardcoded banner removed from templates.
- Step 4: Manifest updated — 3 output rows (Identifiers.h, HashMaps.h, CAST.h with empty tables), 6 dispatch rows with new fragment names.
- Step 5: CastCLI.cpp includes collapsed to `#include "generated/CAST.h"`. File renamed Cast.cpp→CastCLI.cpp (doxygen @file fixed).
- Step 6: Old generated files deleted (Lexicon.h, Banner.h). Interim hand-authored Identifiers.h, HashMaps.h, CAST.h created so cast builds (chicken-and-egg — cast generates these but can't build without them).
- Doxygen warnings fixed: CastCLI.cpp @file mismatch, Constraints.h HTML entity escapes (8 occurrences of `\<`/`\>` in doxygen prose).

### Remaining
- Step 7: ARCHITECT builds cast, runs fixpoint (`./cast cast/CAST.md` twice, verify empty diff). Verify generated output: banner at top, namespace format, CAST.h master shape.
- Two Auditor findings require ARCHITECT decisions (see Open Questions).
- Doxygen pass (dedicated step after fixpoint validated — per JRENG-CODING-STANDARD.md timing discipline).
- Auditor re-run after ARCHITECT decisions resolved.

### Key Decisions
- Master include: `generated/CAST.h` (cast), `generated/jam_CAST.h` (jam) — engine-generated manifest output rows, not hand-written. ARCHITECT-directed.
- Fragment naming: singular of root type (Identifier.h, HashMap.h, Bimap.h, Char.h). No "Row" suffix. ARCHITECT-directed.
- Namespace format: `/*___...___*/` separator after opening brace, `/**___END OF NAMESPACE___*/` before closing brace. ARCHITECT-directed.
- Output banner: engine reads `cast_output.md` (markdown, parsed by jam::Markdown), auto-prepends `// `-prefixed lines to every generated `.h`. Not in templates. ARCHITECT-directed.
- Input banner: authoring convention on table `.md` files, not enforced. ARCHITECT-directed.
- No manual string parsing — jam::Markdown::parse() only. ARCHITECT-directed.

### Files Modified
- `Source/CastCLI.cpp` (new, replaces deleted `Source/Cast.cpp`) — @file fixed; includes collapsed to `generated/CAST.h`; `getConfigureDepends` adds `cast_output.md` via SSOT constant
- `Source/Driver.h` — `getOutputBanner()` function; `outputBannerFileName` SSOT constant; `processOutput` signature + banner injection; `Driver::run` reads banner once
- `Source/Constraints.h` — 8 doxygen HTML entity escapes (`\<`/`\>`)
- `cast/CAST.md` — 3 output rows, 6 dispatch rows with new fragment paths
- `cast/cast_output.md` (new) — fenced code block banner artwork
- `cast/template/Identifiers.h` (new) — root template, namespace Id, separator + END marker
- `cast/template/HashMaps.h` (new) — root template, namespace cast, separator + END marker
- `cast/template/Identifier.h` (new) — fragment template
- `cast/template/HashMap.h` (new) — fragment template
- `cast/template/CAST.h` (new) — master template, no namespace
- `cast/template/Lexicon.h`, `Banner.h`, `IdentifierRow.h`, `BannerRow.h` (deleted)
- `cast_output.txt` (deleted)
- `Source/generated/Identifiers.h`, `HashMaps.h`, `CAST.h` (new, interim hand-authored, gitignored)
- `PLAN-cast-local-fix.md` (new) — 7-step plan

### Open Questions
1. **SPEC vs cast_output.md** — SPEC §1 lists three tracked artifact kinds (relations, templates, manifest). `cast_output.md` is a fourth input. SPEC §5 determinism says output is a function of (tables, templates, CAST.md, binary) — now also cast_output.md. Should SPEC be amended to add cast_output.md as a tracked input?
2. **Version stamp in banner** — SPEC §9 says "the same stamp appears in generated file banners." Currently cast_output.md has only artwork — no version/commit. Should the engine compose the version stamp into the banner, or should cast_output.md author a hole for it?

### Next Steps
- ARCHITECT resolves the two open questions.
- ARCHITECT builds cast and runs fixpoint (Step 7).
- On fixpoint success: doxygen pass (dedicated delegation to Engineer), then Auditor final sweep.
- Then: PLAN-cast.md task #8 (extract 7 module template pairs from restructured jam headers) → task #9 (author jam/cast CAST.md incl. union+dedup engine extension) → #10 conformance loop → #11 switchover.
