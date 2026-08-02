# DEBT.md

**Purpose:** Inter-sprint ledger of debts — bugs, nitpicks, friction observed during usage. Drained by sprints via `/pay` (COUNSELOR planning) and `/log` (hygiene drain). **JRENG = paid in full, cash. No triage.**

**Format:** Each entry uses **O / D / E** articulation — Observation, Divergence, Expectation. IDs are UTC timestamps (`DEBT-YYYYMMDDTHHMMSS`). Newest entries at top. Add via `carol debt add`.

**Lifecycle:** Created lazily on first `carol debt add`. Entries appended via interactive prompt. Entries removed by `carol debt clear <id>` (called by `/log` hygiene step after SPRINT-LOG receipt is written). Survives `carol reset` — debts persist across protocol resets.

---

## DEBT-20260802T190852

**Observation:** `./cast` (--help/spec render) floods hundreds of "JUCE Assertion failure in juce_LookAndFeel.cpp:94" — the tui markdown renderer findColour()s ids the app's style.css appearance never registers (e.g. jam::codeBackgroundColourId at jam_tui_markdown.cpp getAnsiRow, and the MarkdownLayout::tokenColourId table lookups), each unregistered id asserting per cell; the rendered spec layout is wrong — nested list indentation escalates per item and ordered/bullet marker structure misgroups.
**Divergence:** the colour contract between an app's stylesheet and the renderer's consumed id set is unchecked (renderer reads ids no css row declares), and the list-structure defects trace to the markdown Document product consumed through project-level manual walking rather than the declarative tag/id surface.
**Expectation:** resolved with the markdown-Document isomorphism debt (DEBT-20260802T190025): renderer-consumed ids fully declared (css rows or renderer defaults — one contract, no silent findColour asserts), and list layout correct once consumers read the tagged Document product; `./cast --help` renders clean with zero assertion output.

---

## DEBT-20260802T190431

**Observation:** cast's banner (Source/Cast.cpp) hand-rolls presentation as three parallel structures — a juce::StringArray of glyph rows, a std::array of row colours, and paintBanner's per-glyph if/else chain dispatching on the two glyph codepoints with a special prior-row colour rule for the shade glyph.
**Divergence:** glyph→colour/rule dispatch encoded as imperative branching over parallel containers instead of one data-driven jam::HashMap/table shape (MANIFESTO L 3-branch rule: decision table in imperative code); banner art and its colour policy are not a single declared structure.
**Expectation:** banner expressed as one table-driven structure (jam::HashMap or equivalent canon container keying glyph→colour rule), paintBanner reduced to a lookup-driven loop with no per-glyph branching.

---

## DEBT-20260802T190025

**Observation:** jam::Document already provides the XmlElement-isomorphic query surface (getChildByName over Id::tag, getChildByID over Id::id — jam_Document.h:433-463), populated by the html/css producers; the jam_Markdown producer emits positional children with Id::type ints only — no Id::tag, no Id::id — so markdown consumers (cast Cells.h extractSections/findColumnIndex, Constraints.cpp ref splits, getCellHazard) hand-walk and hand-parse what should be declarative queries; cast Template.cpp hand-scans a reinvented ${hole} grammar although Format::replaceholder (%%name%% tokens) is the canon substitution API.
**Divergence:** markdown Document product is not isomorphic to juce::XmlElement/html semantics — blocks lack tags, headings lack id anchors; project code carries module-grade table-navigation and a parallel template grammar.
**Expectation:** jam_Markdown stamps Id::tag per block (heading/table/row/cell — the h2/table/tr/td mapping) and heading text as Id::id (html anchor semantic), kuassa echo; cast consumers rewritten onto getChildByID/getChildByName; template holes become %%name%% via Format::replaceholder; Cells.h helpers and manual splits deleted.

---

