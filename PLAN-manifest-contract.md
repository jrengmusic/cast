# PLAN: CAST Self-Describing Manifest (SM arc)

**RFC:** none — objective from ARCHITECT rulings this session
**Date:** 2026-08-14
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + jam framework

## Context

The TW-arc engine works but is garbage by design: role resolution happens by
*sniffing cell shapes* instead of reading declarations. ARCHITECT condemned four
faults, each verified in code:

1. **Wrapper wiring is extension-sniffing** — Writer.h:40-42, TemplateDocument.h:224,
   :337. "Cell happens to parse as a filename" is the type system.
2. **row:index is smuggled state** — TemplateDocument.h:176 threaded 8th parameter,
   magic `Id::row` placeholder + fake `index` transform (:197-199), selection-dependent
   counter (:270-285). Root cause: the source table declared no values. **Enum is
   ARBITRARY** — ordinals are data, never derived from row position.
3. **Manifest is not self-describing** — nine engine-known column conventions
   (`{Id::output, Id::index}`, `*columns.begin()`, `Id::file`, `endsWith lineBreak`,
   `Id::code`, `"<region> lineBreak"` composition, `Id::value` exemption, `Id::row`,
   `Id::index`) with zero declared in the manifest.
4. **Source-table wiring is name-coincidence** — blind `tables/*.md` glob splices
   anonymous files; `model.getTables (cell)` resolves headings by luck; miss = silent
   passthrough emitting garbage (TemplateDocument.h:242-246). "How do I know the
   table lives in template.md?" — you can't; the engine threw the filename away.

## Ratified decisions (closed — PP-5)

1. **Map-type tables are `key|value`** — all of them. `## template token type` and
   `## rules` reshape; ordinals are explicit data (`text|0` … `region close|3`).
   Bimap.cast jacks: `:::key:toCamel:::` / `:::value:::`. `:::row:index:::` dies.
2. **`## index` is a mandatory CAST.md table** — `| name | path |`. Every referenced
   file declared once — the file index. The directory glob dies: the Model parses
   exactly what `## index` declares. Column `name` (Rule 5 nearest-sibling:
   lexicon's key column is `name`). Files.cast takes the name `files`
   (ARCHITECT's draft).
3. **Table references are `name:table` qualified** — `lexicon:lexicon`,
   `template:template token type`, `CAST:output`. Filename-qualified
   namespace kills alias collisions; the colon makes list cells pure syntax.
   `## index` carries no `table` column.
4. **Output-shaped table contract** — `| <body> | <wrapper>... | file |` are the
   mandatory bones; `file` is the only reserved column name. Wrapper columns layer
   left→right (inner→outer). All other columns (data jacks, list sources,
   separators) are optional, arbitrarily and *truthfully* named — `struct` is not
   `namespace`, which is why `## output index` never folds into `## output`: same
   bones, different flesh = different table. Writer iterates every output-shaped
   table.
5. **`## output index` is second-order and optional** — its list source is
   `## output` itself (`CAST:output`); the master is emitted "after them, beside
   them, from them" (NAMES.md Rule 9). Different nest, own truthful column set.
6. **Jack↔column parity is the Validator's job** — per row, the placeholder union of
   the row's resolved `.cast` cells must match the row's non-reserved columns, and
   list-region interiors validate against their source table's columns. No match,
   no cigar (FATAL).
7. **Separator wiring** — `<region> lineBreak` suffix column stays, validated by
   jack parity. One reserved suffix word.
8. **Undeclared reference = FATAL** — `## index` name misses, unresolvable
   `name:table`, silent passthrough: all die. Fail fast at validation.
9. **Manifest tables renamed** — `## index` = file declarations (the file index);
   `## output` = first-order outputs (mandatory); `## output index` = second-order
   master generation (optional).
10. **files.md → binary-files.md**; declared name `binary files`; references read
    `binary files:files`. The `headers` table dies — `## output index` sources
    `CAST:output`; the `file` column is the header list (SSOT).
11. **Output root** — full relative path per `file` cell
    (`../Source/diff/Identifiers.h`). No reserved output-root name — `## index`
    declares inputs only.

## Dependency & API Inventory

*(verified: session reads + Pathfinder BRIEF)*

- **cast/Source/Model.h** — `parse` (:22) globs `cast/tables/*.md` via
  `juce::File::findChildFiles` (:38-39) — this glob dies; `getOutput` (:61),
  `path` member (:67). Rework keys tables by (file name, heading) via jam scoped
  table accessors.
- **jam::MarkdownDocument** (jam_MarkdownDocument.h) — `getTables` (:116),
  `getTableRows` (:127), `getTableHeaders` (:139), `getTableCell` (:167),
  `getTableValue` (:179), `hasTableValue` (:236) — scoped access exists, no jam
  API change needed.
- **cast/Source/TemplateDocument.h** — parse/getOrCreate/build/toFile pipeline
  stands; only cell-resolution internals (getCell, getCodeText, isSelected,
  getPlaceholders) and the rowIndex thread change.
- **cast/Source/Writer.h** — orchestration only; loses `{Id::output, Id::index}`,
  `*columns.begin()`, extension sniff.
- **cast/Source/Validator.h** — isManifest (:443) = isTemplates → unique
  (`Id::name`) → isPlaceholders (:387); unique predicate (:84). Chain gains isIndex;
  isPlaceholders sharpens to decision 6.
- **jam::Format::getPreColon (:403) / getPostColon (:408)** (zero-copy upTo/from) —
  split `name:table` references; already the marker-interior splitter
  (TemplateDocument.h:372-373).
- **Lexicon facts** — jam declares `name` (:733), `path` (:827), `value` (:1251);
  `key` undeclared in both lexicons → one jam free-lexicon declaration. cast's
  `entry` (cast lexicon.md:16) dies if unreferenced after reshape. `Id::key` exists
  in neither generated Identifiers — bootstrap addition needed.
- **Data files** (cast/tables/): banner.md, files.md, lexicon.md,
  localisation-en.md, template.md — localisation is language-qualified.

## Validation Gate

Each step validated by COUNSELOR against MANIFESTO.md (BLESSED), NAMES.md,
~/.carol/CODING.md, and the locked decisions above. @Auditor runs ONCE, after the
final step. ARCHITECT alone builds/runs cast/git.

## Steps

### Step SM1: /truth — canon amendment (NAMES.md Rule 9 + HELP.md)
**Scope:** ~/.carol/NAMES.md, cast/Source/HELP.md
**Action:** Amend Rule 9: (a) golden rule 5 (ordinals from `@row:index@`) is
REPLACED — ordinals are arbitrary data in `key|value` map tables; the lexicon still
stores no ordinals, relation/map tables do. (b) The `## patch` cord-list section is
REPLACED by the manifest contract: mandatory `## index` (`name|path`) + output-shaped
tables (`|<body>|<wrapper>...|file|`), `name:table` qualified table references,
jack↔column parity, `<region> lineBreak` separator suffix, undeclared reference =
FATAL. HELP.md mirrors the same contract (spec + cookbook examples).
**Validation:** amended text states exactly the ratified decisions; no residual
`## patch` / `@row:index@` vocabulary.

### Step SM2: Language data — key|value maps + lexicon
**Scope:** cast/cast/tables/template.md, cast/cast/tables/lexicon.md,
jam/cast/tables/lexicon.md, jam/cast/template/Bimap.cast,
cast/Source/generated/TemplateTokenType.h (oracle)
**Action:** `## template token type` → `key|value` with explicit ordinals 0-3.
`## rules` → `key|value` (`begin|region open`, `end|region close`). Bimap.cast:
`:::entry:toCamel:::` → `:::key:toCamel:::` (lines 7, 15), `:::row:index:::` →
`:::value:::`. Declare `key` in jam free lexicon; delete cast's `entry` row iff
grep-zero references remain. Oracle TemplateTokenType.h is already value-correct —
byte-identical target.
**Validation:** tables read `key|value`; Bimap.cast carries no `entry`/`row:index`;
lexicon diffs are exactly one add (+ one delete if unreferenced).

### Step SM3: Manifest + data rewrite — CAST.md
**Scope:** cast/cast/CAST.md, cast/cast/tables/files.md → binary-files.md,
the `headers` table's host file
**Action:** Rename files.md → binary-files.md (declared name `binary files`).
Delete the `headers` table — `## output index` sources `CAST:output`; the `file`
column is the header list. Write `## index` (`name|path`) declaring: the jam
templates (Files.cast = `files`), the data files (banner host, binary-files.md,
lexicon.md, localisation-en.md, template.md), and CAST.md itself — inputs only, no
output root. Rewrite `## output` (5 rows) and `## output index` (1 row): cells
reference `## index` names; list cells take `name:table` form; `file` cells keep
full relative paths (`../Source/diff/Identifiers.h`); `list lineBreak` keeps the
Break.cast reference by name. All `## index` names ARCHITECT-ratified via the full
manifest draft before this step executes.
**Validation:** zero input paths outside `## index`; every reference resolves;
no `headers` table remains; manifest readable end-to-end without engine knowledge.

### Step SM4: Model — declared parsing, scoped tables
**Scope:** cast/Source/Model.h
**Action:** `parse` reads CAST.md first, then parses exactly the `## index`-declared
`.md` files (parallel, existing runJobs pattern). Tables keyed by (file name,
heading) via jam scoped accessors; `getTables`/`getTableRows`/`getTableHeaders`
overloads take the qualified reference; `getOutput` resolves a `## index` name to
its path. Directory glob (Model.h:38-39) dies. `name:table` split via
jam::Format::getPreColon/getPostColon.
**Validation:** no directory iteration remains; unresolvable name → juce::Result
fail path (no assert-only), byte-exact scoped lookups.

### Step SM5: TemplateDocument — declared resolution
**Scope:** cast/Source/TemplateDocument.h
**Action:** Delete the rowIndex thread (8th param, `Id::row`/`Id::index` case,
counter) and the `Id::value` exemption. getCell/getCodeText/isSelected/
getPlaceholders resolve cells through the Model's `## index` view: `.cast`-named
cell = fragment (getOrCreate via resolved path); colon-qualified cell = list source
(scoped table); anything else = literal jack data. The extension sniff
(onlyExtensionFromFilename ×3) dies; the silent passthrough (:242-246) dies —
unresolvable = FATAL surfaced via Validator, never emitted.
**Validation:** zero onlyExtensionFromFilename calls; zero default parameters; build
walker signature shrinks; no `Id::row`/`Id::index` references.

### Step SM6: Writer — generic output-shaped iteration
**Scope:** cast/Source/Writer.h
**Action:** Iterate every output-shaped table (has `file` column; not `## index`),
replacing `{Id::output, Id::index}`. Per row: body = first column's fragment,
wrappers = subsequent `.cast`-name cells left→right through `:::code:::`, target =
the `file` cell path. `*columns.begin()` shape-privilege and the extension sniff
die.
**Validation:** no hardcoded table identifiers; wrapper order = column order;
getBanner untouched.

### Step SM7: Validator — isIndex + sharpened parity
**Scope:** cast/Source/Validator.h
**Action:** New `isIndex`: names unique, paths exist, every manifest reference
(body/wrapper/list/separator cells) resolves through `## index`. `isPlaceholders`
sharpens to decision 6: per row, placeholder union of resolved fragments ↔
non-reserved columns, both directions FATAL; list-region interiors validate against
the source table's columns (reserved: `file`, `code`, `<region> lineBreak`).
isManifest chain: isTemplates → unique → isIndex → isPlaceholders.
**Validation:** predicate returns juce::Result; no precomputed name-lists; every
FATAL carries file:line provenance from the master document.

### Step SM8: TD6 gate — bootstrap fixpoint (ARCHITECT)
**Scope:** none (verification)
**Action:** ARCHITECT: build → run `cast ./cast/CAST.md` → `git diff Source/diff/`
clean → rerun → still clean → `--help` renders. Then @Auditor sweeps ONCE over
SM1-SM7. Doxygen prose stays deferred to [12].

## BLESSED Alignment

- **B** — resources owned by declaration: one `## index` row per file, one parse per
  file, template cache keyed by resolved path.
- **L** — engine sheds nine conventions + rowIndex thread + glob; net deletion.
- **E** — every reference explicit and readable in the manifest; FATAL over silent
  passthrough; no magic words beyond `file`/`code`/`lineBreak`-suffix, all documented.
- **S (SSOT)** — every path declared once; ordinals declared once as data; no
  position-derived shadow values; `headers` duplicate truth dies.
- **S (Stateless)** — build threads no counters; documents stay immutable.
- **E (Encapsulation)** — Model owns file resolution; TemplateDocument owns building;
  Writer only orchestrates; Validator only judges.
- **D** — role by declaration, not by sniffing: same manifest, same output, no
  coincidence-dependent behavior.

## Risks / Open Questions

1. **Remaining `## index` row names** (bimap, break, files, namespace, struct,
   hash map, identifiers, text, generated, banner/cast handle, lexicon, template,
   CAST, localisation handle) — presented as the full manifest draft for one-shot
   ratification at SM3 intake, before the Engineer dispatch.
