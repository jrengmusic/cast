# PLAN: Cast-Local Generated Output Fix

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-08-05
**BLESSED Compliance:** verified
**Language Constraints:** C++ / JUCE — header-only inline implementation preferred (LANGUAGE.md L adaptation)

## Overview

Align cast's own generated output with the convention JAM landed in Sprint 60: one header per type, a master include, engine-injected output banner, and formatted namespace blocks. This is the prerequisite before PLAN-cast.md tasks #8 (extract JAM templates) and #9 (author JAM CAST.md).

## Language / Framework Constraints

- C++/JUCE: header-only inline implementation for engine functions (LANGUAGE.md L adaptation — single-responsibility files under ~300 LOC stay single headers)
- jam::Markdown::parse() is the data model — no manual string parsing (ARCHITECT directive)
- jam::Document tree-walk API for code block content extraction (Pathfinder verified: applyFunctionRecursively, getAllSubText, Id::BlockType::codeBlock)

## Dependency & API Inventory

- **jam_markdown**: `jam::Markdown::parse()` → `jam::Document`; `applyFunctionRecursively()` for tree walk; `getAllSubText()` for code block content; `Id::BlockType::codeBlock` for node type check (`jam_Markdown.h` verified by Pathfinder)
- **jam_core**: `jam::Format::replaceholder()` already used in Template.h for hole substitution
- **juce_core**: `juce::StringArray::addLines()`, `joinIntoString()` for banner line processing
- **Established pattern**: static functions in `cast` namespace, header-only (Driver.h, Validation.h, Constraints.h, Template.h, Transforms.h, Help.h)
- **Engine flow**: `Driver::run()` → `processOutput()` (two-pass: validate then write) → `getOutput()` (template expansion) → write-if-different

## Validation Gate

Each step validated by Auditor via Task tool:
- MANIFESTO.md (BLESSED): no bail-out guards, no magic numbers, no shadow state, SSOT (banner read once), Deterministic (fixpoint holds)
- NAMES.md: no new names without ARCHITECT approval (all names in this plan are ARCHITECT-directed)
- JRENG-CODING-STANDARD.md: brace style, alternative tokens, no anonymous namespaces, static linkage
- Locked decisions: master = `generated/CAST.h` (engine-generated output row), fragment = singular of root type, namespace format with separators, banner from `cast_output.md` parsed by jam::Markdown

## Steps

### Step 1: Convert cast_output.txt to cast_output.md

**Scope:** `cast/cast/cast_output.md` (new), `cast_output.txt` (delete)

**Action:** Create `cast/cast/cast_output.md` containing the cast_output.txt banner content wrapped in a fenced code block (no language tag). Delete `cast_output.txt`.

The file content:
```
```
░░████████████░░████████████░░████████████░░████████████
░░████  ░░████░░████  ░░████░░████  ░░████    ░░████    
░░████        ░░████  ░░████░░████            ░░████    
░░████        ░░████████████░░████████████    ░░████    
░░████        ░░████  ░░████        ░░████    ░░████    
░░████  ░░████░░████  ░░████░░████  ░░████    ░░████    
░░████████████░░████  ░░████░░████████████    ░░████    
```
```

**Validation:** Auditor confirms: file exists, content is a single fenced code block, `cast_output.txt` deleted, jam::Markdown::parse() on the file yields a codeBlock node with the correct banner text via getAllSubText().

### Step 2: Engine — output banner injection + configure-depends

**Scope:** `Source/Driver.h`, `Source/CastCLI.cpp`

**Action for Driver.h:**

Add a static function `getOutputBanner` in the `cast` namespace (before `processOutput`):
1. Takes `const juce::File& dir` (manifest directory)
2. Looks for `cast_output.md` in dir; if absent, returns empty string (no banner — graceful)
3. Parses with `jam::Markdown::parse()`
4. Walks the parsed tree via `applyFunctionRecursively` to find the first node where `Id::type == Id::BlockType::codeBlock`; extracts text via `getAllSubText()`
5. Splits into lines via `juce::StringArray::addLines()`, prefixes each line with `"// "`, joins with `"\n"`
6. Returns the formatted banner string

Modify `processOutput` signature: add `const juce::String& outputBanner` parameter (after `writeOutputs`).

Modify `processOutput` body: after the `validateHoles` check on the root template output, prepend the banner:
- If `outputBanner.isNotEmpty()`: `outputBanner + "\n" + outputText` → use as `finalOutput`
- If empty: `outputText` is `finalOutput`
- The write-if-different comparison and write use `finalOutput`

Modify `Driver::run`: after parsing the manifest and before the two-pass loop, call `getOutputBanner(dir)` once. Pass the result to both `processOutput` calls (validate pass and write pass).

**Action for CastCLI.cpp:**

In `getConfigureDepends()`, after adding the manifest file, add:
- Check if `dir.getChildFile("cast_output.md")` exists; if so, add to depends list.

**Validation:** Auditor confirms: `getOutputBanner` uses jam::Markdown::parse() (no manual string parsing), returns `// `-prefixed lines, empty string when file absent. `processOutput` prepends banner after hole check, before write. `Driver::run` reads banner once. `getConfigureDepends` includes `cast_output.md` when present. No bail-out guards. No magic strings (banner filename is a named constant or direct literal at the single call site).

### Step 3: Rename templates + namespace format + create master template

**Scope:** `cast/template/` — rename 4 files, create 1 new file

**Action:**

1. **Delete** `cast/template/Lexicon.h`, `cast/template/Banner.h`, `cast/template/IdentifierRow.h`, `cast/template/BannerRow.h`

2. **Create** `cast/template/Identifiers.h` (root — formerly Lexicon.h):
   - Remove hardcoded banner (engine injects it)
   - Add namespace separator and END marker per ARCHITECT format:
   ```
   #pragma once

   namespace Id
   {
   /*____________________________________________________________________________*/

   @section@

   @predicate@

   @transform@

   @file@

   @column@
   /**______________________________END OF NAMESPACE______________________________*/
   }// namespace Id
   ```

3. **Create** `cast/template/HashMaps.h` (root — formerly Banner.h):
   - Add namespace separator and END marker:
   ```
   #pragma once

   namespace cast
   {
   /*____________________________________________________________________________*/

   @banner@
   /**______________________________END OF NAMESPACE______________________________*/
   }// namespace cast
   ```

4. **Create** `cast/template/Identifier.h` (fragment — formerly IdentifierRow.h):
   - Content unchanged:
   ```
   inline const juce::Identifier @word@ { "@string@" };
   ```

5. **Create** `cast/template/HashMap.h` (fragment — formerly BannerRow.h):
   - Content unchanged:
   ```
       { "@colour@", juce::String::fromUTF8 ("@text@") },
   ```

6. **Create** `cast/template/CAST.h` (root — master include, no holes, no namespace):
   ```
   #pragma once

   #include "Identifiers.h"
   #include "HashMaps.h"
   ```

**Validation:** Auditor confirms: old template files deleted (orphan scan would FATAL on leftovers). New templates have correct namespace format (separator + END marker). CAST.h master has no namespace (just includes). Fragment templates content unchanged. No hardcoded banner in any template.

### Step 4: Update manifest

**Scope:** `cast/CAST.md`

**Action:**

Update `## outputs` table:
```
| output | template | tables |
|---|---|---|
| ../Source/generated/Identifiers.h | template/Identifiers.h | `tables/cast.md` |
| ../Source/generated/HashMaps.h | template/HashMaps.h | `tables/banner.md` |
| ../Source/generated/CAST.h | template/CAST.h |  |
```

Update `## dispatch` table template paths:
```
| table | column | value | template | slot |
|---|---|---|---|---|
| section | word |  | template/Identifier.h | section |
| predicate | word |  | template/Identifier.h | predicate |
| transform | word |  | template/Identifier.h | transform |
| file | word |  | template/Identifier.h | file |
| column | word |  | template/Identifier.h | column |
| banner | colour |  | template/HashMap.h | banner |
```

`## constraints` unchanged (`word unique`).

**Validation:** Auditor confirms: 3 output rows (Identifiers, HashMaps, CAST). CAST.h row has empty tables cell. 6 dispatch rows with new fragment paths. All referenced template files exist on disk. No orphan templates in `cast/template/`.

### Step 5: Update CastCLI.cpp includes

**Scope:** `Source/CastCLI.cpp`

**Action:**

Replace lines 8-9:
```cpp
#include "generated/Lexicon.h"
#include "generated/Banner.h"
```
with:
```cpp
#include "generated/CAST.h"
```

**Validation:** Auditor confirms: single include of `generated/CAST.h`. No references to old names (Lexicon.h, Banner.h) anywhere in Source/.

### Step 6: Delete old generated files

**Scope:** `Source/generated/Lexicon.h`, `Source/generated/Banner.h` (delete)

**Action:** Delete both files. They are untracked (gitignored under `Source/generated/`). New files (`Identifiers.h`, `HashMaps.h`, `CAST.h`) will be generated by running cast.

**Validation:** Auditor confirms: old files deleted. `Source/generated/` contains no stale files.

### Step 7: ARCHITECT builds and runs fixpoint

**Scope:** Build + run

**Action:** ARCHITECT builds cast, then runs:
1. `./cast cast/CAST.md` — generates Identifiers.h, HashMaps.h, CAST.h
2. `./cast cast/CAST.md` — second run, verify empty diff (fixpoint)

Verify generated output:
- All three files have `// `-prefixed banner at top
- Identifiers.h and HashMaps.h have namespace separator + END marker
- CAST.h has no namespace, just includes
- CastCLI.cpp compiles with the new include

**Validation:** ARCHITECT confirms fixpoint (stage1 == stage2, empty diff). Compiler output is ground of truth.

## BLESSED Alignment

- **B (Bound):** Banner read once in `run()`, passed by const ref — single owner, no floating state
- **L (Lean):** `getOutputBanner` ~25 LOC in Driver.h (stays under 300); no new files; template files are minimal
- **E (Explicit):** All names ARCHITECT-directed (CAST.h, Identifiers.h, HashMaps.h, Identifier.h, HashMap.h); no magic values; banner filename is explicit
- **S (SSOT):** Banner source is `cast_output.md` — one file, parsed by jam::Markdown, no hardcoded duplicates in templates; master include is one generated file
- **S (Stateless):** `getOutputBanner` is a pure function — reads file, returns string, no cached state
- **E (Encapsulation):** Engine owns banner injection; templates own namespace structure; manifest owns wiring; no layer crossing
- **D (Deterministic):** Banner is a total function of (cast_output.md, binary); fixpoint holds; write-if-different preserved

## Risks / Open Questions

- **`Id::BlockType::codeBlock` accessibility**: Need to confirm jam_markdown header exposes this constant. Pathfinder verified it exists in `jam_Markdown.h` — Engineer reads the header before implementing.
- **Empty tables cell for CAST.h output**: Engine must handle empty `getTableValues` result (no roots parsed, no dispatch, template expanded as-is). Verified in code analysis — the for loops simply don't execute. Low risk.
