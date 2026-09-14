# RFC — `cast --sync`: framework kernel sync by identity-token transform

**Author:** COUNSELOR (jfs session, 2026-09-08)
**Date:** 2026-09-08
**Status:** Proposed — awaiting ARCHITECT ruling on §8 decisions; no SPEC change, no code
**Consumer:** `~/Documents/Poems/kuassa/___lib___/HANDOFF.md` — this verb is its Step 0

Every claim carries a file:line read on 2026-09-08. Names in this document are proposals
unless marked *ratified*; NAMES.md Rule -1 applies.

---

## 1. Objective (ARCHITECT, verbatim)

- "dont use ECHO, echo is using dead lexicon. but now toolchain is using CAST. probably
  better just implement CAST --sync"
- "ECHO is untested. and it also just handrolling md parser. which i believe
  jam::MarkdownDocument is WAY more reliable and DETERMINISTIC."
- "when both framework provide identical framework-info.md isnt just extracting namespace
  name, file name prefix, etc as token to replace after copying? just like we're using
  template? then diff?"
- "most accurate semantics probably user-modules-info.md — both jam and kanjut must
  provide the identical table at modules root — update CAST engine — test sandbox jam to
  kuassa, in temp directory — replace jreng-filter-strip with jfs, with kuassa modules —
  CAST --sync jam to KANJUT"

Target pair: jam (`~/Documents/Poems/dev/jam`) → KANJUT (`~/Documents/Poems/kuassa/___lib___`).
CIUM (`~/Documents/Poems/iqala/___cium___`) is a later consumer of the same verb.

---

## 2. Prior art being retired — ECHO

- `~/.config/echo/` — 2,963 lines of CMake in 7 files: `echo.cmake` 766 (engine),
  `verify.cmake` 665, `lint.cmake` 426, `render.cmake` 385, `apply.cmake` 346
  (README:201 "not yet implemented"), `self-apply.cmake` 330, `diff.cmake` 45.
- Hand-rolled markdown table parser: `echo.cmake:162-244` (`echo_table_parse`),
  `:264-365` (`echo_table_section`).
- Reads `<root>/lexicon/identity.md` (`render.cmake:319-320`, `lint.cmake:42-46`).
  Neither jam nor `___lib___` has a `lexicon/` directory today (glob empty, both) —
  every verb fatals at identity load. KANJUT's copy sits at
  `___lib___/cast/tables/identity.md`; jam dropped its ECHO tables from `cast/CAST.md`
  at jam Sprint 91 (`jam/carol/SPRINT-LOG.md:139, :153`).
- Last successful run: KANJUT Sprint 88, 2026-07-21 — 11 kernel modules, 1,041 files
  (`___lib___/carol/SPRINT-LOG.md:1281`); jfs built clean after (`:1302`).
- Git dependency: scope = `git ls-files` (`verify.cmake:343-405`), diff =
  `git diff --no-index` (`:580-665`).

What survives from ECHO is its *semantics* (§3), not its code.

---

## 3. The transform — exactly what must be preserved

ECHO's whole transform is one ordered pass of literal replacements over every text file
and every path (`echo.cmake:635-678`), most-specific first:

1. Generated-header whole names (`JamVulkanShaderData.h` → `KuassaVulkanShaderData.h`) —
   `:638-647`; longest literals first so a shorter token never partial-matches inside one
   (`:623-627`).
2. `namespace jam` → `namespace kuassa` — `:649`
3. `jam::` → `kuassa::` — `:650`
4. macroPrefix `JAM_` → `KUASSA_` — `:651` (this is what carries
   `JAM_USING_AQUATIC_PRIME` → `KUASSA_USING_AQUATIC_PRIME`)
5. filePrefix `jam_` → `kuassa_` — `:652` (contents and file names)
6. `vendor:` / `website:` values inside JUCE module declaration blocks only,
   whitespace-tolerant — `:657-662`; `applicationName` is spliced the same way — a
   whole-token identity value, per-framework, not a substring transform
7. Module Pairs (module names that differ beyond the prefix) — `:664-673`
8. Bare word `jam` → `kuassa` at non-word boundaries, repeated to fixpoint — `:675`,
   `:599-616`

Plus two invariants:

- **Bijectivity** — `inverse (forward (file)) == file` per text file, fatal naming the
  file and offset on failure (`:756-766`). Guarantees no source file already carried a
  target token.
- **Binary passthrough** — `.spv`, fonts, images copied byte-for-byte
  (`README.md:49-50`; detection `verify.cmake:490-527`).

Scope rules (data, not code):

- Modules are classed `kernel` (synced) or `provision` (never touched)
  (`identity.md:26-41`; `README.md:101-110`).
- Sync Ignore, gitignore grammar, excludes paths inside kernel modules
  (`identity.md:53-59`; matcher `verify.cmake:19-252`).
- Ignored files already on the target side survive a module replace by stash/restore
  (`render.cmake:229-271`).
- Dependency-closure gate: a kernel file that `#include`s a provision module aborts the
  render with zero files written (`render.cmake:181-218`).
- Provision Hooks: guard macros that must remain present in named kernel files after
  render (`identity.md:61-71`; `lint.cmake:378-426`).

---

## 4. Data — `user-modules-info.md` at each framework root

Name is ARCHITECT's ("most accurate semantics probably user-modules-info.md"). Shape
follows cast's own `project-info.md` family — `## project info` `name | type | value |
format | comment` (`dev/cast/project-info.md:23-35`), `## user module`
`root | name | comment` (`:130-138`). Both roots carry the same tables; values differ.
Read by `Model::parse` exactly like a manifest — `jam::MarkdownDocument` only (cast
CLAUDE.md principle 6).

Proposed tables (column names reuse SPEC §5.3 identity columns where they fit —
`name`, `key`, `value`):

```
## identity

| key            | value  |
| namespace      | jam    |
| filePrefix     | jam_   |
| macroPrefix    | JAM_   |
| moduleVendor   | JRENG  |
| companyWebsite | …      |
| applicationName | JAM   |   (KANJUT's value is KANJUT)
| namespaceShort |        |   (blank on jam; `ku` on KANJUT — target-only, never a source token)

## module

| name              | class     |
| jam_core          | kernel    |
| jam_markdown      | kernel    |
| jam_fuzzy         | provision |
| …                 |           |

## header

| name             | value                   |
| vulkanShaderData | JamVulkanShaderData.h   |   (whole-token file names, KANJUT: identity.md:47-51)

## ignore

| value                    |
| generated/**             |
| jam_vulkan/spv/*.spv.d   |
```

Facts that bound the tables:

- The kernel sets of both roots must correspond one-to-one after transform, or the
  diff is meaningless (`README.md:109-110`). `kuassa_markdown` does not exist yet —
  the first sync creates it. jam's 7 non-jfs modules (`jam_audio_devices`, `jam_clap`,
  `jam_fuzzy`, `jam_mermaid_diagram`, `jam_subprocess`, `jam_terminal`, `jam_whelmed`)
  need a class; every `jam_*` directory on disk must be declared (ECHO lint CHECK 2,
  `lint.cmake:82-91` — worth keeping as a fatal).
- KANJUT's current `## Modules`, `## Generated Headers`, `## Sync Ignore`,
  `## Provision Hooks` are the seed values (`___lib___/cast/tables/identity.md`).
- Module Pairs table exists in ECHO for `aquatic_prime ↔ juwita_malam`-style renames
  (`README.md:112-114`); KANJUT's is empty (`identity.md:43-45`). Omit until a pair exists
  (YAGNI).

---

## 5. CLI

Current grammar (SPEC §2.1:60-64):

```
cast [<manifest>] [<directory> | --format | --no-format | --<word>]
```

- `--<word>` is the toolchain-row selector (SPEC §2.1:74-75, §6.9:649-653;
  `main.cpp:141-165`). Today `cast X --sync` runs format + generate, then fails
  `sync: <failToolchainArgument>` (`Processor.h:129-131`) — the name is unclaimed.
- Proposed: `sync` joins the reserved flags (`format`, `no-format`, `version`, `help` —
  `main.cpp:38-41`, `:144-145`).

Proposed invocation:

```
cast --sync <source-root> <target-root>
```

- Both roots must contain `user-modules-info.md`; missing → fatal naming the path.
- `<target-root>` may be any directory — a temp copy of `___lib___` is the sandbox
  ("test sandbox jam to kuassa, in temp directory"). No separate dry-run flag: the
  sandbox *is* the dry run, and the diff between sandbox and live target is the exact
  change a live run makes (ECHO's own recipe, `README.md:216-223`).
- Output: the list of paths written (write-if-different), the list of paths deleted
  (target files inside a kernel module with no source counterpart and not ignored), and
  nothing else. Zero lines written = in sync. That is the diff ARCHITECT asked for.

SPEC edits: §2.1 grammar + flag prose; §1 hardcode list gains the four table names
(§1:15-29 — "the engine hardcodes these and nothing else"); §10.1 fatal set gains the
rows in §7.

---

## 6. Engine placement

New file `Source/Sync.h` beside `Processor.h`; `main.cpp` dispatches `--sync` before the
manifest path (`main.cpp:319-337` pattern). No change to `Processor`, `Writer`,
`Validator`, `Shapes`, `Items` — sync reads no manifest and renders no template.

Components, each with its existing sibling:

| Component | Sibling in cast / jam / JUCE | Notes |
|---|---|---|
| Parse both info files | `Model::parse (const juce::File&)` (`Model.h:38`); `getTables (Id)`, `getTableRows`, `getTableCell` (`Processor.h:113-117`, `:145-150`) | tables only — no manifest semantics |
| Token map, ordered | `juce::String::replace` (`juce_String.h:802`) applied in §3 order | 8 steps, one function |
| Bare-word boundary replace | none — `containsWholeWord` exists (`juce_String.h:452`), no `replaceWholeWord` | the one new string primitive (≈20 lines, `jam::Format` sibling) |
| Contamination check | `juce::String::contains` (`:434`) for each target token in each source file | replaces ECHO's inverse(forward()) — equivalent and simpler: a source file with no target token is trivially invertible |
| Walk source kernel modules | `juce::File::findChildFiles` (`juce_File.h:622`) | recursive, per declared kernel module |
| Ignore matcher | none in jam/JUCE for gitignore grammar | decision §8.3 — `*`/`**` glob only, or path-prefix rows |
| Binary detection | `juce::File::loadFileAsData` (`:725`) + null-byte scan | copy via `copyFileTo` (`:548`) |
| Path transform | filePrefix + header-name steps on `getRelativePathFrom` (`:199`) | directories included (`jam_vulkan/` → `kuassa_vulkan/`) |
| Write-if-different | `Processor::writeOriginIfChanged` shape (`Processor.h:185-198`); `replaceWithData` (`juce_File.h:765`) | report path when changed |
| Parallel per file | `Jobs::run (count, perIndex)` (`Jobs.h`; `Processor.h:84-88`) | one job per source file |
| Deletions | target walk vs transformed source set; `deleteFile` | never inside ignored paths or provision modules |
| Include gate | scan `#include` lines for a declared module prefix; class lookup | decision §8.4 |
| Hook check | `contains ("#if " + guard)` per row after write | decision §8.4 |

No git. No CMake. No shell.

---

## 7. Fatal set additions (SPEC §10.1)

- `user-modules-info.md` missing at either root.
- A required `## identity` key missing (`namespace`, `filePrefix`, `macroPrefix`).
- A kernel module declared with no directory; a `<filePrefix>*` directory not declared.
- Kernel module sets of the two roots do not correspond after transform.
- A source text file contains a target token (contamination) — names file and token.
- A target root path equals the source root path.
- (if §8.4 yes) a kernel file includes a provision module; a Provision Hook guard absent
  after write.

Everything else is data (SPEC §1.1) — an ignore row that matches nothing is not a
diagnostic.

---

## 8. Decisions for ARCHITECT

1. **Table and column names** — `user-modules-info.md` (given); `## identity`,
   `## module`, `## header`, `## ignore`, column `class` — proposals.
2. **Invocation shape** — `cast --sync <source-root> <target-root>` vs. two info-file
   paths vs. names via a registry table (ECHO's `frameworks.md:3-8`). Root paths need no
   registry.
3. **Ignore grammar** — full gitignore (ECHO, ≈230 lines) vs. `*`/`**` globs only vs.
   literal path-prefix rows. KANJUT's four patterns (`identity.md:55-58`) are all
   expressible as prefix + extension.
4. **Keep the include gate and the hook check** — both are ECHO lint/render features;
   the hook check is what caught the Sprint 88 Aquatic strip
   (`___lib___/carol/SPRINT-LOG.md:1299-1300`). Without it the guard surface is
   verified only by the KANJUT build.
5. **Deletions** — sync deletes target kernel files with no source counterpart (a true
   mirror, ECHO's whole-directory replace `render.cmake:370-372`) vs. write-only with a
   report of orphans.
6. **Product-code sync** — `jreng-filter-strip/Source` is not a module; the same
   transform applies to it (`jam::` → `kuassa::`, project `generated/` re-cast) but it
   uses `ku::` (`generated/Identity.h:20`) which is a target-only token. Same verb with a
   project info file, or a manual step in the HANDOFF plan.

---

## 9. Complexity assessment

| Piece | Est. lines (C++17/JUCE) | Reuse | Risk |
|---|---|---|---|
| Info-file read + identity/module/header/ignore tables | 60 | `Model::parse`, table accessors | none |
| Ordered token transform (8 steps) + bare-word primitive | 60 | `String::replace` | ordering — covered by a fixed step list and the contamination fatal |
| Contamination + module-set correspondence fatals | 40 | `Result::fail` pattern | none |
| Source walk, path transform, binary passthrough | 60 | `findChildFiles`, `loadFileAsData`, `copyFileTo` | none |
| Ignore matcher | 20 (prefix) / 80 (glob) / 230 (gitignore) | — | grows with grammar; §8.3 |
| Write-if-different + report | 40 | `writeOriginIfChanged` shape | none |
| Deletions | 30 | target walk | §8.5 |
| Include gate + hook check | 60 | line scan | §8.4 |
| `main.cpp` dispatch + reserved flag | 20 | existing flag statics | none |
| SPEC §1/§2.1/§10.1 + HELP.md sync | prose | — | HELP derives from SPEC (SPEC §1:9-10) |
| **Total** | **≈390 (prefix ignore) to ≈600 (gitignore)** | | one new string primitive; zero new dependencies |

Reference: ECHO is 2,963 lines of CMake for the same semantics plus lint/self-apply; the
transform core is 45 of them. The C++ estimate is one `Sync.h` under the 300-line smell
detector only if the ignore matcher stays small (§8.3) or lives in a sibling — Lean is
served by responsibility split (transform / walk / report), not by relocation.

Not in this estimate, on purpose (HANDOFF §6-§7): the Aquatic hook surface in jam under
`JAM_USING_AQUATIC_PRIME`, `kuassa_markdown` creation, `generated/` ownership. Those are
data and framework work the verb consumes, not verb code.

---

## 10. Sources read

- `~/.config/echo/README.md`, `echo.cmake`, `render.cmake`, `lint.cmake`,
  `verify.cmake` (function index), `frameworks.md`
- `dev/cast/SPEC.md` §1, §2.1, §5.3, §6.9, §10.1 headings; `CLAUDE.md`; `project-info.md`;
  `Source/main.cpp`, `Processor.h`, `Writer.h`, `Transforms.h`, `Model.h` (API index)
- `___lib___/cast/tables/identity.md`; `___lib___/CLAUDE.md`;
  `___lib___/carol/SPRINT-LOG.md:1271-1302`
- `jam/carol/SPRINT-LOG.md:128-156`, `:300-310`
- `JUCE/modules/juce_core/files/juce_File.h`, `text/juce_String.h` (cited lines)
- `jam/jam_core/file/jam_File.h:13`
