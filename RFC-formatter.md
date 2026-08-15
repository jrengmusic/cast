# RFC — CAST Formatter: Canonical Markdown Write-Back (Pandoc Grid Tables + GFM + CommonMark)

Date: 2026-08-16
Status: Research complete, design ratified by ARCHITECT this session — ready for COUNSELOR to convert to PLAN
Language Constraints: C++17 / JUCE + jam framework

---

## Context

ARCHITECT's nvim markdown format-on-save (`prettier`, via conform.nvim) does not understand
Pandoc grid tables (`+---+---+` box-drawing syntax) — it leaves them untouched, which is why
they've never been auto-corrected and drift out of alignment by hand.

The stated requirement (ARCHITECT, this session): a formatter that behaves for grid tables the
way `prettier` already behaves for GFM pipe tables — type loosely, save, it aligns itself, every
time, with zero manual alignment ever required. Scope was explicitly widened mid-session from
"just fix grid tables" to full CommonMark + Pandoc grid table parity, no usage-based limiting
("no scope, i write and read all LEGIT SPEC: CommonMark + Pandoc grid tables").

## Research Summary — why off-the-shelf tools were rejected (evidence, not assumption)

Every candidate was installed and empirically tested against the real file
`~/Documents/Poems/dev/cast/cast/CAST.md`, not assumed from documentation.

1. **`prettier`** — remark/micromark-based. Has zero grid-table awareness; leaves the syntax as
   opaque text. Confirmed via source inspection of its markdown parser scope.
2. **`pandoc` (CLI, markdown→markdown round-trip)** — installed (v3.10.2), tested directly
   against `CAST.md`. Corrupted the file: rows without an explicit `+---+---+` separator between
   them were merged into a single multi-line cell (10 aliases collapsed into one cell), and rows
   whose cell content didn't exactly fill the column width declared by the header separator had
   their `|` characters land at the wrong character column, misparsing into adjacent cells.
   Root-caused by reading `man pandoc`'s own grid-table grammar: it requires (a) a separator
   line between every row and (b) every row's `|` characters landing at the exact same character
   column as the header separator's `+` marks — a parsing precondition, not a formatting output
   choice, unaffected by any CLI flag. Confirmed with a second, independent test: even after
   manually inserting row separators, rows with drifted column padding still corrupted — column
   alignment is required *in addition to* row separators.
3. **Panache** (Rust, `panache.bz`, purpose-built Pandoc/Quarto formatter, not a converter) —
   installed (v3.4.0), tested against the identical `CAST.md` content. Produced byte-for-byte
   the same class of corruption as pandoc (missing-separator rows merged, even word-wrapped
   across cells). Confirms the strict-alignment requirement is inherent to Pandoc grid-table
   grammar itself, not a defect specific to one implementation.
4. **`dprint`** (pulldown-cmark backend) — GFM pipe tables only, no grid-table support at all.
5. **`@adobe/remark-gridtables`** — exists and is maintained, but Prettier 3.9 (June 2026)
   migrated its markdown engine from remark to micromark, an incompatible plugin API. Using it
   means abandoning prettier's printer/house-style entirely — fails the parity requirement.
6. **Pandoc Lua filters** — confirmed (via `pandoc.org/lua-filters.html`) to run *after* parsing
   completes. A filter cannot un-corrupt content the reader already misparsed — ruling out a
   pandoc-filter-based fix.

**Verdict:** no existing tool tolerates loosely-typed grid tables (missing separators,
inconsistent column padding) the way GFM pipe-table parsers tolerate loose pipe spacing. This is
because grid-table grammar (as every spec-compliant implementation enforces it) is
position-based, while pipe-table grammar is pipe-count-based — the latter is inherently
tolerant, the former inherently is not, in any implementation that follows the published spec.

## Discovery — the tolerant parser already exists, in this codebase

`~/Documents/Poems/dev/jam/jam_markdown/document/jam_MarkdownDocument.h` — module description:
"Clean-room native CommonMark + GFM markdown parsing and rendering." Verified by direct code
read (not assumed):

- Spec-section-cited block parser covering CommonMark §4.1–§5.3 (thematic breaks, ATX/setext
  headings, blockquotes, list items, fenced/indented code, HTML blocks) plus a dedicated
  **"Grid tables (Pandoc grid table extension)"** section (`jam_MarkdownDocument.h:2244`).
- `addGridTableLine` (`jam_MarkdownDocument.h:2325–2377`): every line starting with `|` becomes
  its own new row via `addTableRowAtLeaf`, regardless of whether a `+---+---+` separator preceded
  it. Border lines are only consulted to detect the header boundary (`====`). **No row-separator
  requirement — solves the defect that broke both pandoc and Panache.**
- `splitTableRow` (`jam_MarkdownDocument.h:1970–1989`): splits a row by scanning `|` characters
  in the row's actual text content — never checks character-column position against the header
  separator. **No column-alignment requirement — solves the second defect.**

This parser is already wired into CAST's own pipeline: `Model : public jam::MarkdownDocument`
(`Source/Model.h:17`), and `Model::parse` (`Source/Model.h:24–79`) already parses `CAST.md` and
every table file its `## index` references, using exactly this tolerant grid-table logic. This
is the live codebase already correctly reading the files that broke every external tool tested.

**What's missing, confirmed by search** — no writer. `grep -rln "toMarkdown|serialize|
MarkdownWriter|MarkdownFormatter"` across all of `~/Documents/Poems/dev/jam/` returns nothing.
`Source/Writer.h` (CAST's only writer) drives `TemplateDocument` to emit C++ code
(`Writer.h:15–116`) — it has no markdown-text output path. The `jam_markdown` module builds a
`Document`/`Element` AST for **rendering** (`jam_MarkdownComponent.h`, `jam_MarkdownProjection.h`,
`jam_MarkdownSyntax.h` are all display-oriented), never for re-emitting canonical markdown text.

## Ratified Design (ARCHITECT, this session)

Add a `Formatter` to CAST that walks the `Document`/`Element` tree `Model` already builds and
re-emits canonical markdown text, covering every block type the parser already parses (full
CommonMark parity — headings, paragraphs, lists, blockquotes, thematic breaks, code blocks, HTML
blocks — plus GFM pipe tables and Pandoc grid tables), written back to the source `.md` files.

**Shape precedent — `Validator`, not `Writer`.** `Source/Validator.h` is an all-`static`-method
struct: no instance state, every function takes `const Model&` (or `Element&`) as an explicit
parameter, returns a `juce::Result`. This is the closer nearest-sibling match (NAMES.md Rule 5)
for a stateless formatter than `Writer`, which is a small struct holding a `const Model&` member
for its one real method. `Formatter` should follow `Validator`'s shape: static methods, no
constructor-injected state, explicit `Model`/`Element` parameters.

**Pipeline wiring** (`Source/Processor.h:17–26`, `generate()`):
1. **Default behavior:** `generate()` runs `Formatter` after `Validator::isValid` succeeds, before
   or alongside `Writer::toFile` — every `cast` run leaves the source manifest and table files in
   canonical form. Tables are always clean going forward; no manual alignment, ever.
2. **Opt-out:** a `--no-format` flag (extends `main.cpp:105–122`'s existing flag-dispatch pattern,
   same `Id::doubleDash`-prefixed style as `--version`/`--help`) skips the formatting step for a
   single run.
3. **Formatter-only mode:** `cast --format` runs parse → validate → format → write-back and stops
   — skips codegen entirely. This is the standalone markdown-beautifier entry point.

## Open Questions for COUNSELOR/Engineer to ratify during implementation

CommonMark's spec (verified via its own official test suite, `spec.commonmark.org/0.31.2/
spec.json`, 652 cases) defines **parsing** — markdown text → semantic structure — unambiguously.
It does **not** define a canonical markdown-to-markdown output style; that's why prettier,
pandoc's writer, and remark-stringify each produce different-looking output from identical input
despite all being spec-compliant readers. `Formatter`'s writer therefore needs its own explicit
house-style decisions, not spec lookups, for at minimum:
- Bullet list marker (`-` / `*` / `+`) and ordered-list delimiter (`.` / `)`) normalization.
- Emphasis/strong marker choice (`*`/`_`, `**`/`__`).
- Indent width for nested list content / block quotes.
- Fenced code block style (backtick vs tilde, spacing before/after the info string).
- Blank-line normalization between blocks.
- Table column-width computation (recommend: widest cell content per column, consistent with the
  zero-data-loss repair logic already prototyped and verified in this session's sandbox testing
  against the real `CAST.md`, 45/45 rows preserved).

## Validation Strategy

- **Round-trip fidelity** against the real corpus (`CAST.md` + every referenced `tables/*.md`):
  cell content extracted before/after must match exactly — zero data loss, same discipline
  already proven in this session's sandbox prototype (`/tmp/cast-sandbox/`, not committed).
- **Idempotency**: `format(format(x))` must equal `format(x)` — the correctness bar for any
  formatter.
- **Parser regression**: the existing tolerant parser is unchanged by this RFC — only a writer is
  added — so no new parser test burden; CommonMark's 652-case suite is relevant only if the
  parser itself is ever touched.

## Non-Goal / Future Work (explicitly out of this RFC's scope)

Once `cast --format` exists as a working binary, wiring it into nvim's `conform.nvim` as the
markdown formatter (`~/.config/nvim/lua/plugins/formatting.lua`, replacing the currently-reverted
`prettier` entry for markdown) is machine/toolchain integration — MACHINIST's domain, not CAST's.
Same integration pattern already used for `clang-format` in that config (external CLI binary,
stdin/stdout or file-mode). Not part of this RFC or the resulting PLAN.

## Risks

1. Full CommonMark writer coverage (not table-only) is a substantial implementation — every
   block type the parser already handles needs a matching writer case for round-trip fidelity.
   Underestimating this as "just add a class" (ARCHITECT's own framing, corrected this session)
   risks a partial implementation that silently drops content the parser understands but the
   writer doesn't yet emit.
2. House-style decisions (see Open Questions) are unratified — implementation should not
   improvise these per NAMES.md Rule -1; each needs explicit ARCHITECT sign-off before the writer
   hard-codes a choice.
3. Write-back is destructive by nature (formats the source file in place) — first run against
   real files should be diff-reviewed before trusting the default-always-format pipeline
   behavior, despite the zero-data-loss verification already done against a sandbox copy.
