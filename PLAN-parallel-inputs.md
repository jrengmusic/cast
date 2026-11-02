# PLAN: parallel-inputs

**RFC:** none. The objective comes from ARCHITECT's prompt: *"why not parse its file arguments in parallel? we already have Jobs::run"*
**Date:** 2026-11-02
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 + JAM (CODING.md, LANGUAGE.md C++/JUCE)

## Overview

`getProcessedTexts` (cast `Source/main.cpp:1138-1166`) runs its per-file work through `Jobs::run`, not through a serial `for` loop. Before that can work, two problems need a fix. `jam::Strings` const reads write, so concurrent reads race (JAM). A repeated `-` argument reads stdin concurrently (cast).

## ARCHITECT rulings (verbatim, this session)

- Scope: *"Shared loop, both lines"*. `--tables` and `--format` stdout both go parallel through `getProcessedTexts` (main.cpp:1185, 1251).
- Race: *"Fix jam::Strings"*. A const read of `jam::Strings` does not write.
- Stdin: *"Read stdin once"*. Stdin is read once on the main thread, and each `-` gets that text.
- Audit finding 14 (stored non-owning pointers in `jam::Strings::ConstIterator` and the hash functors): *"exception"*.
- Audit finding 28 (SPEC rule 1 against rule 6): *"- is wrong argument"*. `-` is an unknown flag on the `--format` and `--tables` lines, and stdin is read only when no file argument exists.
- Transport (amends Step 5, after the audit): *"jam::Strings stays as is. but if you need to contain string as transport in parallel, use jam::Array<std::string, with lock>"* and *"unless we need Input or final Output to communicate to juce API, there is no reason to use juce::String at all"*.

## Language / Framework Constraints

- CODING.md CRITICAL RULES: `not/and/or`, brace init, `.at()`, explicit lambda capture lists, no bail-out guards (a result return is permitted), structured bindings with descriptive names, no comments and no doxygen before the audit.
- CODING.md *One object, one translation unit*: `jam::Strings` bodies stay in `jam_Strings.cpp`. The template `addArray` stays in the header.
- `juce::Result` cannot be default-constructed (`juce_Result.h:115-120`, private), and `jam::Array::resize` value-initializes each element (`jam_Array.h:214`). Thus a per-index slot cannot hold a `juce::Result`.

## Dependency & API Inventory

- **cast `Jobs::run`** (`Source/Jobs.h:28-38`). This is a transient `juce::ThreadPool` (default `numberOfThreads { SystemStats::getNumCpus() }`, `juce_ThreadPool.h:195`). It blocks until every job is done.
- **Established parallel-collect pattern.** `resize` the slot array, write `slots.at (index) =` in the worker, then collect serially:
  - `Sync::getSyncResults` (`Sync.cpp:164-177`): `jam::Array<std::pair<juce::String, juce::String>>`
  - `Sync::runContamination` (`Sync.cpp:140-155`): the first non-empty error string returns `juce::Result::fail (error)`
  - `Processor::format` (`Processor.cpp:30-39`)
  - `Model::parse` (`Model.cpp:628-642`)
- **`jam::Array`** (`jam_Array.h:45-46`, default `juce::DummyCriticalSection`). `at()` reads with no write (`jam_Array.h:336-355`).
- **`jam::Strings`** (`jam_core/text/jam_Strings.{h,cpp}`). These const members write mutable state:
  - `size`, `at` (h:586-602)
  - `end`, `ConstIterator::operator*` (h:627, 655)
  - `contains` (cpp:311-334), `indexOf` (cpp:336-359)
  - `joinIntoString` (cpp:575-608), `operator==` (cpp:187-200), `findPosition` (cpp:739-761)
  - The writes go through `flushIntake` (cpp:763-770), `cachePositions` and `rebuildPositions` (cpp:817-836), `cacheOccurrences` and `incrementExactOccurrence` (cpp:781-803), and `addToStorage` (cpp:947-961).
  - The mutable members are `spans`, `intake`, `exactPositions`, `foldedPositions`, `exactOccurrences`, `positionsDirty`, `intakeQueried` and `occurrenceWatermark` (h:918-948).
  - The same race already exists in `Processor::format` (`Processor.cpp:29, 36`). This fix also covers that site.
- **Stdin.** `getStandardInputText` (main.cpp:697-717) has one caller, `getInputText` (main.cpp:731). `getInputText` (main.cpp:728-735) has one caller, `getProcessedText` (main.cpp:1088), checked with LSP findReferences.
- **Thread affinity.** On the getText path, no call needs the message thread (Pathfinder trace: main.cpp, Processor, Model, Validator, JsonWriter, TemplateDocument). `MarkdownDocument::parse` already runs on `Jobs::run` workers (Model.cpp:637). `ScopedJuceInitialiser_GUI` (main.cpp:1141) stays on the main thread, alive across `Jobs::run`.

## Validation Gate

COUNSELOR validates each step before the next one. It checks against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md and the locked PLAN decisions, with no deviation and no scope drift. @Auditor runs ONCE, after the final code step, and covers the whole sprint. It never runs per step.

## Steps

### Step 0: Baseline
**Scope:** session scratchpad only.
**Action:** COUNSELOR copies this plan to the project root. Pathfinder then records the current output of the installed `~/.local/bin/cast` into the scratchpad. It runs `cast --tables` on every `cast/*.md` file in one call, then `cast --format` on the same files in one call. Each stdout goes to its own file.
**Validation:** The two baseline files exist and are not empty. The exit code of each run is 0.

### Step 1: JAM, delete the staging tier
**Scope:** `jam/jam_core/text/jam_Strings.h`, `jam_Strings.cpp`.
**Action:**
- Delete the members `intake` and `intakeQueried`, and the functions `flushIntake` and `intakeContains` (declarations and definitions).
- Delete every `flushIntake();` / `other.flushIntake();` call.
- `add (juce::String)` becomes `addToStorage (getView (stringToAdd))`. `add (std::string_view)` becomes `addToStorage (textToAdd)`.
- `addTokens` (both overloads) and `addLines` call `addToStorage (getView (token))` / `addToStorage (getView (line))` per element.
- In `addIfNotAlreadyThere` and `contains`, delete each staging branch (`intakeQueried`, `intake.isEmpty()`, `intakeContains`, `intake.add`).
- Delete the `intake` lines in the copy and move constructors, `swapWith`, `clear` and `ensureStorageAllocated`.
**Validation:** No `intake`, `intakeQueried`, `flushIntake` or `intakeContains` token remains (Read of both files). The element order is unchanged: every add appends one span. The Step 1-3 names are deletions or names that exist already; nothing new.

### Step 2: JAM, occurrence index kept current by each mutator
**Scope:** `jam_Strings.h`, `jam_Strings.cpp`.
**Action:**
- Delete `occurrenceWatermark` and `cacheOccurrences`.
- `addToStorage` calls `incrementExactOccurrence` for the span it appends.
- Delete each watermark condition and each `cacheOccurrences();` call (`insert`, `addIfNotAlreadyThere`, `remove`, `removeRange`, `removeDuplicates`, `move`, `sort`, `sortNatural`). The decrements become unconditional.
- In `addIfNotAlreadyThere`, delete the explicit `incrementExactOccurrence` call and `++occurrenceWatermark`, because `addToStorage` now counts.
- In `replaceSpan`, delete `incrementExactOccurrence (newSpan)` (counted by `addToStorage`) and keep the decrement of `oldSpan`.
- The copy constructor fills `exactOccurrences` with `for (const auto span : spans) incrementExactOccurrence (span);`.
**Validation:** Each span in `spans` is counted exactly once in `exactOccurrences`:
- Every append path goes through `addToStorage`.
- Every removal path decrements.
- `replaceSpan` gives a net count of zero for the popped tail.

### Step 3: JAM, position maps kept current; no const member writes
**Scope:** `jam_Strings.h`, `jam_Strings.cpp`.
**Action:**
- Delete `positionsDirty` and `cachePositions`.
- `addToStorage` calls `exactPositions.try_emplace (span, index)` and `foldedPositions.try_emplace (span, index)` for the appended index. `try_emplace` does not overwrite, so the first occurrence wins, the same as `rebuildPositions` (cpp:823-836).
- Each public mutator that reorders, replaces or removes spans calls `rebuildPositions()` once at its end: `insert`, `set`, `remove`, `removeRange`, `removeDuplicates`, `move`, `trim`, `sort`, `sortNatural`. The copy constructor also calls `rebuildPositions()`.
- `removeString`, `removeEmptyStrings` and `appendNumbersToDuplicates` reach the maps through `remove`/`set`, so they stay correct with no change.
- Delete `positionsDirty` from the move constructor, `swapWith` and `clear`.
- Remove `mutable` from `spans`, `exactPositions`, `foldedPositions` and `exactOccurrences`. `addToStorage`, `incrementExactOccurrence` and `rebuildPositions` become non-const.
**Validation:**
- No `mutable` remains in `jam::Strings`. The compiler proves that no const member writes.
- Read each const member body (`size`, `at`, `contains`, `indexOf`, `findPosition`, `joinIntoString`, `operator==`, `getString`, `getView`). None calls `arena->insertArray` (Pathfinder obs. 3: a write through `unique_ptr` escapes `mutable`).

### Step 4: cast, stdin read once
**Scope:** `cast/Source/main.cpp` (`getInputText`, `getProcessedText`, `getProcessedTexts`).
**Action:**
- `getInputText (const juce::String& input, const std::pair<juce::Result, juce::String>& standardInput)` returns `standardInput` when `isStandardInput (input)`. Otherwise it returns the file text, as today.
- `getProcessedText` gains the parameter `const std::pair<juce::Result, juce::String>& standardInput` after `input`. It passes the parameter to `getInputText`, and its lambda captures `&standardInput`.
- In `getProcessedTexts`, inside the `filesFound and styleFound` block, add: `const auto standardInput { files.contains (getStandardInputArgument()) ? getStandardInputText() : std::pair { juce::Result::ok(), juce::String() } };`, then pass it on.
**Validation:**
- `getStandardInputText` has one call site, on the main thread, before any dispatch.
- The only new identifier is `standardInput`, which joins the `getStandardInputText` / `isStandardInput` / `getStandardInputArgument` family.

### Step 5: cast, Jobs::run in getProcessedTexts
**Scope:** `cast/Source/main.cpp` (`getProcessedTexts`).
**Action:** Replace the serial loop (main.cpp:1150-1162) with the Sync pattern:
```cpp
jam::Array<std::pair<juce::String, juce::String>> processedTexts;
processedTexts.resize (files.size());

Jobs::run (files.size(),
    [&files, &stylePath, &assumeFilename, &standardInput, &processedTexts, &getText, lineWrap] (int index)
    {
        const auto [processResult, text] { getProcessedText (lineWrap, stylePath, assumeFilename, files.at (index), standardInput, getText) };
        processedTexts.at (index) = { processResult.getErrorMessage(), text };
    });

jam::Strings texts;

for (const auto& [errorMessage, text] : processedTexts)
{
    if (errorMessage.isNotEmpty())
        return { juce::Result::fail (errorMessage), {} };

    texts.add (text);
}

return { juce::Result::ok(), std::move (texts) };
```
**Validation:**
- Argument order is kept by index.
- The first failure by index is reported. This is the same as the serial loop: a failed `juce::Result` always carries a non-empty message (`juce_Result.h:75-79`).
- The new identifiers are `processedTexts` (from the `getProcessedText` family) and `errorMessage`.
- The worker writes only its own slot.

### Step 5b: cast, transport in jam::Array<std::string, juce::CriticalSection> (audit findings 17, 18, 21)
**Scope:** `cast/Source/main.cpp` (`getInputText`, `getProcessedText`, `getProcessedTexts`, `writeTexts`, `runFormatOutput`, `runTablesOutput`).
**Action:**
- The slots are `jam::Array<std::string, juce::CriticalSection> processedTexts` and `processFailures`.
- `getProcessedTexts` returns `std::pair<juce::Result, jam::Array<std::string, juce::CriticalSection>>`: the first failure by index, or the text slots.
- `writeTexts (texts, separator)` writes each `std::string` in place.
- `--tables` frames the output with `fputc` of `Chars::openBracket`, `Chars::closeBracket` and `Chars::newline`, and passes `Chars::comma` as the separator.
- `standardInput` becomes `standardInputText` (NAMES Rule 5, family `getStandardInputText`).
**Validation:** No `jam::Strings` and no join on the output path. The worker writes only its own slots. Output is byte-identical to the Step 0 baseline.

### Step 6: SPEC and HELP, stdin read once
**Scope:** `cast/SPEC.md` §2.1 (`--format` rule 1, `--tables` rule 2), `cast/Source/HELP.md` lines 48 and 67.
**Action:** In each rule, after "A file argument `-` also reads stdin", add: "The engine reads stdin once, and each `-` argument gets the same text." Add the same text to HELP.md lines 48 and 67, which are derived.
**Validation:** SPEC and HELP state one rule with the same words. ARCHITECT's ruling: *"Read stdin once"*.

### Step 7: Build and compare
**Scope:** build only. ARCHITECT: *"always build with cast toolchain"*.
**Action:**
- Engineer runs `cast cast/spell.md --debug`, with a watchdog of 10 min. The build compiles JAM and cast and installs the Debug binary.
- Pathfinder runs the Step 0 commands again with the new binary and diffs the result against the baseline.
- Pathfinder runs `printf '| a |\n| - |\n| 1 |\n' | cast --tables - -`.
- Pathfinder runs `git status` in cast to check the regeneration.
**Validation:**
- The build exits 0 with zero warnings.
- Both outputs are byte-identical to the baseline.
- The `- -` run prints two identical objects.
- The `Source/generated/` files show no diff.

### Step 8: Audit
@Auditor runs once over Steps 1-6 against CONTRACT and this PLAN. COUNSELOR resolves each finding that it validates (DCF §5).

### Step 9: Doxygen pass
@Pathfinder writes the pass (doxygen-writer skill) with COUNSELOR's exact targets:
- `jam::Strings`: class prose, `add`, `addIfNotAlreadyThere`, `contains`, `addToStorage`, `rebuildPositions`, `incrementExactOccurrence`, the member briefs, and the copy constructor. The staging tier, watermark and dirty flag text is removed.
- cast: `getInputText`, `getProcessedText`, `getProcessedTexts`.
COUNSELOR reads each block against the code.

## BLESSED Alignment

- **D, Deterministic.** No const read writes, so concurrent reads do not race (MANIFESTO anti-pattern: "A plain (non-atomic) read that races a write"). Stdin is read once, so the result does not depend on thread order.
- **S, Stateless.** The flags `positionsDirty` and `intakeQueried`, the watermark and the staging tier go. The container is complete after each mutation (MANIFESTO E, "Complete at Creation"; CAROL Code Contract, "No manual boolean flags").
- **S, SSOT.** One insertion primitive (`addToStorage`) keeps all three indexes. One per-file loop serves both lines.
- **E, Encapsulation.** Each worker writes only its slot, and the main thread collects. This is the established pattern; no new pattern.
- **L, Lean.** `getProcessedTexts` stays under 30 lines. `jam::Strings` loses members.
- **B, Bound.** The pool lives inside `Jobs::run`. The `standardInput` and `processedTexts` locals outlive every job.

## Risks / Open Questions

- Fact, not a question: each worker's `Processor` builds a Model, and each Model parse starts its own `Jobs::run` pool (Model.cpp:631, Jobs.h:31). So pools nest, one per input. This is correct but has a cost. ARCHITECT measures the speed.
- Fact: an add now writes the bytes and the three indexes at once. Before, this work was deferred until the first read. `removeString` and `removeEmptyStrings` rebuild the position maps once per removed element.
- Fact: the JAM change applies to every JAM consumer (END, TIT, CAKE, WHATDBG, CAROLINE). The behaviour stays the same except for the cost above.
