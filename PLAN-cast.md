# PLAN — CAST: Universal Headless Codegen

Date: 2026-08-01
Status: Ready for COUNSELOR execution
Supersedes: `RFC-cast-codegen.md` in the parts listed under **Supersessions** below. Where this
PLAN conflicts with that RFC, this PLAN wins. The RFC's Research Summary remains valid evidence.

---

## Locked Decisions (ARCHITECT, this session)

1. **No Lua.** CAST carries no interpreter. Domain lives in data; the engine is dumb.
2. **Three tracked artifacts are the SSOT:** GFM tables (relations), templates, `CAST.md`
   (manifest). **Generated files are untracked** — build artifacts, same standing as `.o`.
3. **Templates are opaque text with one construct: `${hole}`.** Runtime logic (function bodies,
   framework calls) lives verbatim in templates. Templates never contain generate-time logic.
4. **Generate-time residue moves out of the generator:** cross-row ordering and derived
   defaults become explicit authored table data; validation and string transforms become
   declared rules. (Same doctrine as the explicit `order` column replacing trailing-digit
   z-order.)
5. **Manifest name: `CAST.md`.** One per framework, one per product project. The manifest is
   the invocation: argv selects, never defines.
6. **Engine semantics: select → project → join.** Dispatch (value/presence-keyed template
   selection), substitution + named transforms, FK-constraint validation. Authored row order.
   Write-if-different. Failures are FATAL naming `file:row:col`.
7. **Generated output is header-only, grouped by construct kind** (Identifier, Bimap, Chars,
   Localisation, Identity, Resource/Paths, ColourIds, Shader, …). Root templates and manifest
   structure are framework-invariant across JAM/KANJUT/CIUM.
8. **Sequencing: JAM first — restructure already WIP.** KANJUT next, CIUM last. The old
   KANJUT-first milestone is superseded.
9. **CAST binaries are carried per framework** — tracked, per-platform (macOS + Windows),
   ~3–4 MB each (cake dist precedent). No `find_program`, no version assert: the framework
   carries the exact binary that produces its bytes. Source SSOT: `~/Documents/Poems/dev/cast`.
10. **Invocation: `codegen.cmake`, first include before `project()`.**
    `CMAKE_HOST_APPLE`/`CMAKE_HOST_WIN32` dispatch to the carried binary,
    `execute_process (cast CAST.md)`, inputs registered as `CMAKE_CONFIGURE_DEPENDS`.
    Supersedes build-graph custom commands. Dissolves the `Metadata.cmake`
    before-`project()` chicken-and-egg — zero tracked generated files, no exceptions.
11. **Templates and `codegen.cmake` live per framework, echo-synced** (ECHO kernel-sync
    doctrine) — each framework is self-contained.
12. **CLI contract:**
    - `cast` — finds `CAST.md` in cwd, regenerates all outputs
    - `cast <path>/CAST.md` — explicit manifest
    - `cast CAST.md <output>` — regenerate one declared output row (selection only)
    - `cast --version` — version + source commit; stamped into generated banners
    - no `CAST.md` found → help page: the full rules spec (table grammar, template grammar,
      manifest tables, transform + predicate vocabulary)
13. **CMake is a consumer.** It never implements codegen. `lexicon.cmake`, `table.cmake`,
    `shaders.cmake`, `ParseParameters.cmake` are deleted per framework at its switchover.

---

## Design Contract (spec summary — Step 1 freezes the full text)

**Relations.** Any `## section` GFM table. Rows = tuples, columns = attributes, free shape.
Cells flatten to plain strings; code spans unwrap to content. Any cell containing `<`, `>`, or
a URI scheme outside a code span is FATAL (autolink hazard is live in the current parser:
`jam_Markdown.h:2910-2933` accepts any ≥2-char scheme; verified this session).

**Templates.** Opaque bytes + `${hole}`. A hole is *scalar* (one cell / one manifest value) or
*aggregate* (a table projected through fragment templates in row order) — the manifest decides;
template grammar never grows a second construct. Two tiers: **root** templates (one per output
file) and **fragment** templates (per-row, fill slots in roots). Fragments never produce files.

**CAST.md.** Four table kinds:

| section | maps |
|---|---|
| `## outputs` | output path ← root template ← input table list (1:1 output↔root per invocation) |
| `## dispatch` | (table, column, value/presence) → fragment → slot |
| `## transforms` | column → named transform (closed vocabulary) |
| `## constraints` | column → predicate / FK (`existsIn table.column`, range, regex, unique, parity) |

Orphan template, undeclared output, unmapped fragment → generation error. `CONFIGURE_DEPENDS`
list is derived from the manifest, never hand-maintained.

**Determinism.** Bytes are a total function of (tables, templates, CAST.md, binary). Numbers
pass through as authored strings. Fixed LF. No timestamps/paths/hostnames. Write-if-different.
Configure environment contributes nothing.

**Failure.** `float row 1 (preamp gain): default outside [min, max]` — file, row, column,
non-zero exit → configure FATAL before any TU compiles.

---

## Steps

### Phase 1 — Spec freeze + tool (cast repo)

**[1] Freeze the spec.** Write `SPEC.md` in the cast repo: manifest grammar, template grammar,
hazard rule, determinism rules, and the **complete transform + predicate vocabulary** enumerated
from the two discovery passes this session (all nine lexicon constructs + shaders.cmake + JFS
hand-authored targets — the enumeration exists in session context; nothing outside it is
needed). Vocabulary names pass the Decision Gate: propose to ARCHITECT before freezing
(NAMES.md Rule -1). The spec text is also the help-page content — the binary ships its own
contract.

**[2] Create the cast repo.** `~/Documents/Poems/dev/cast`. Console target via
`configure_app()` with no `TARGET_TYPE` (whatdbg pattern, `whatdbg/CMakeLists.txt:108-117`).
Links: juce_core + jam_core + jam_lexicon + jam_markdown parser header only
(`jam::Markdown::parse` → `jam::Document` — verified zero-include, no graphics/vulkan/mermaid).
`--version` embeds version + source commit hash.

**[3] Engine.** In order: cell flatten + hazard rule (~150 LOC) → CAST.md reader (~250) →
template engine (~150) → transforms (~150; reuse `jam::Format` where it already exists, e.g.
`toKebab`) → predicates + FK joins (~250) → driver (~200): select/project/join, row order,
write-if-different, `file:row:col` errors. Total ~1,300–1,800 LOC — smaller than one copy of
`lexicon.cmake` (2,410).

**[4] CLI + help** per the locked contract (Decision 12).

**[5] Fixture harness.** Minimal fixture manifest + tables + templates; golden-file expected
outputs; fixpoint check = run twice, diff empty. This harness is the durable CI check.

### Phase 2 — JAM conformance (restructure WIP is the stage-0 target)

**[6] SIOF verification (gate for header-only Identifier).** @Pathfinder pass: confirm no
namespace-scope static anywhere in JAM or its downstream projects consumes `Id::` symbols
during its own dynamic initialisation. 1,059 externs (`jam_Lexicon.h`) become C++17 inline
variables with dynamic init in every including TU — safe only if that holds. If violated:
STOP, surface to ARCHITECT.

**[7] Complete the JAM hand-restructure** (WIP, ARCHITECT-started): construct-grouped
header-only generated files. Bimaps are already header-only (instance-scoped map,
`jam_Lexicon.h:213-231`); Chars are constexpr; Identifier/Localisation/Identity/Resource/
ColourIds/Shader move per the same shape. Move generate-time residue into authored data:
explicit ordering wherever `lexicon.cmake` derived it; explicit `licenseFileExtension` row
(replaces `lexicon.cmake:1819-1838` derivation). Survey JAM `lexicon/*.md` for hazard cells
(`<`, `>`, bare URLs — `identity.md` URLs known) — backtick them **atomically with the
switchover commit** ([11]); `table.cmake` cannot read backticks, so earlier breaks the
current build.

**[8] Extract templates** from the restructured headers by parameterisation — root per
construct kind, fragments per row shape. Placeholder syntax `${...}` (settled by extraction).

**[9] Author JAM `CAST.md`** — outputs/dispatch/transforms/constraints for the full lexicon
surface, aggregating the per-module `lexicon/*.md` inputs (authoring split stays; wiring is
the manifest's).

**[10] Conformance loop.** cast run → byte-diff against the hand-restructured headers →
iterate to zero → fixpoint stage1==stage2. The byte-diff gate expires at switchover; the
fixpoint is durable CI.

**[11] JAM switchover — one atomic commit** (Refactor-Rewrite: delete first, no coexistence):
- gitignore `jam_lexicon/generated/`
- carry cast binaries (macOS arm64 + x86_64 or universal, Windows x64) as tracked toolchain
  deps
- add `codegen.cmake` per Decision 10
- backtick hazard cells
- **delete** `lexicon.cmake`, `table.cmake`, `shaders.cmake` and their include wiring
- update `.gitignore`, `BuildSetup.cmake` references

**[12] Build validation — ARCHITECT builds** (agents never build): JAM consumers END, TIT,
CAKE, WHATDBG. Compiler output is the ground of truth for the flip.

### Phase 3 — KANJUT conformance

**[13] Echo-sync** `codegen.cmake` + templates into `___lib___`; carry binaries.

**[14] Hand-restructure KANJUT lexicon** (nine constructs) to the identical construct-grouped
header-only shape. Remaining hazard cells: `lexicon/identity.md:17,23` URLs (the four
StyleTheme cells were already resolved — RFC revision 2026-07-26). Backtick atomically with
[15].

**[15] KANJUT `CAST.md`** → conformance loop → **switchover commit**: gitignore generated,
delete KANJUT `lexicon.cmake`/`table.cmake`, `ParseParameters.cmake`. ARCHITECT builds JFS.

**[16] Product milestone (JFS).** Product `CAST.md` + existing authored tables
(`Source/lexicon.md`, `layout/parameters.md`, `component.md`, `metadata.md`). Product output
templates extracted from the committed Step-5 files, reshaped header-only. `Metadata.cmake`
becomes an untracked configure-time output of `codegen.cmake` (Decision 10 — generated before
`project()` reads it). `EditorLayout.svg` × `component.md` join and all cross-source
references (parameters ↔ component ↔ lexicon ↔ resources) declared as `## constraints` FK
rows. Carried-over old-RFC scope, unaffected by the redesign: CAST validates `style.css`
(bounded grammar, never emits) — confirm scope with ARCHITECT when this step starts. Generated
product files gitignored; runtime XML remnants deleted per the bootstrap RFC.

### Phase 4 — CIUM conformance

**[17] Echo-sync, restructure, conform, switch** — identical shape to Phase 3.

---

## Verification Gates

| Gate | When | Durable? |
|---|---|---|
| Byte-diff vs hand-restructured stage-0 headers | per framework, pre-switchover | expires at switchover |
| Fixpoint: stage1 == stage2 (run twice, diff empty) | every framework, CI | **yes** |
| SIOF pass clean | before [7] completes | one-time |
| ARCHITECT builds all downstream consumers | every switchover | per flip |

## Supersessions of RFC-cast-codegen.md

Dead: the Lua contract and script layer (§Lua contract, §Responsibility split); environment-level
`find_program` tool (Handoff: "Environment-level tool"); committed generated files and the
optional-dependency model (§CMake wiring); the minimum-version assert (Open Question 4 —
dissolved by carried binaries); `lexicon.cmake`-byte stage-0 gate (re-targeted to restructured
headers); KANJUT-first milestone 1 (JAM first, WIP); "jam::lua::State is single-threaded"
constraint (moot). Still valid: all Research Summary evidence; the hazard/backtick rule (re-cited
to `jam_Markdown.h`, old parser files deleted); the `style.css` validate-never-emit decision;
`Whelmed` authoring rationale; rejected-alternatives list (now including Lua itself).

## Amendments (ARCHITECT, 2026-08-01 session)

A1. **Centralized authoring.** Per-module `lexicon/*.md` authoring dies. All authored relations
    live at `<framework>/cast/tables/`. Layout per framework:
    `cast/{tables/, template/, cast, cast.exe, CAST.md}` — manifest name stays `CAST.md`.
A2. **Binaries: two.** macOS universal (arm64+x86_64, ~6–8 MB) + Windows x64. Carried directly
    in `<framework>/cast/`, no `bin/` subdir. Amends Decision 9's per-arch option.
A3. **Bootstrap without lexicon.** JAM generated lexicon is removed from tracking now (JAM is
    already broken); old files relocate to a reference location. An interim hand-authored
    minimal single header (~30 scalar words + BlockType, MarkdownTokenType, MarkdownOperators,
    HtmlType, HtmlBlockTag bimaps + Entity) keeps jam_core + jam_markdown + CAST compiling.
    CAST regens the real headers → byte-diff vs reference → reference deleted.
A4. **SIOF gate closed.** Sole finding: `jam_Html.h:71-74` (4 static inline members consuming
    `Id::HtmlOperators::*`). In the all-inline restructure, inline-variable dynamic init is
    partially ordered — lexicon definitions precede those members in every including TU.
    No code change required; [6] resolved.
A5. **Generated output stays centralized in `jam_lexicon/generated/`** — already the bottom
    module every other module depends on (`jam_lexicon.h:22-23`). Single header per construct.
A6. **Live scope:** END, whelmed (in-framework), CAST. [12] build validation targets END;
    TIT/CAKE/WHATDBG flips happen when ARCHITECT builds them.

## Open Items Carried

1. SIOF result ([6]) — decision needed only if it fails.
2. Transform/predicate vocabulary naming — Decision Gate at [1].
3. `style.css` validation scope confirmation at [16].
