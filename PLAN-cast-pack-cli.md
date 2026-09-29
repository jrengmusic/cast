# PLAN: cast pack by CMake — `cast --pack`, dmg with default layout and background

**RFC:** none — objective from ARCHITECT's prompts this session
**Date:** 2026-09-29
**BLESSED Compliance:** verified against the rulings below
**Language Constraints:** C++17 / JUCE / jam (cast, header-only Source/); CMake template (jfs cmake.cast); LANGUAGE.md C++ L-adaptation

## Context

Sprint 97 made cast read a `## pack` table and run the pack as a cast `## toolchain` row. ARCHITECT ruled that design wrong: the pack table repeats data that CMake always knows, and cast is an ignitor that CMake orchestrates. This plan moves the pack into the CMake post-build: CMake calls `cast --pack` with the values it knows, cast packs.

## Rulings (ARCHITECT, verbatim)

| # | Ruling |
|---|---|
| R1 | *"pack source is ALREADY DETERMINISTICALLY KNOWN … pack table only need to define the target, the format is it dmg, is it zip, hence the table should be win/mac/linux(when applicable) … the rest are cmake logic"* |
| R2 | *"IT IS part of toolchain. but always orchestrate by cmake. cast is dumb, an ignitor"*; *"cmake can call cast and providing the known values as args"* |
| R3 | CLI item form: **Triples** — `cast --pack <archive> <item> <linkName> <linkTarget> …`, blank link as an empty argument |
| R4 | *"DS_Store is cast reponsibilty, part of dmg packer … deterministically … calulated by arithmetic, then we should just provide default value … project can override with custom layout, image (providing its path…), told by cmake"* |
| R5 | *"project-info.md is the manifest. it can provide EVERYTHING including layout. so it's just another "args" … we dont even need or want dedicated file"* |
| R6 | Background image: **This sprint** |
| R7 | *"ALWAYS START WITH DATA"*; *"DESIGN BY CONTRACT"* |
| R8 | Earlier rulings kept: own zip writer; `.DS_Store` written by cast (BinaryWriter); *"should be NO MORE opening dmg at building chain"*; drop the mermaid sandbox |

## Facts carried (read this session)

- JUCE gives every bundle's path: `$<TARGET_BUNDLE_DIR:…>` (cmake.cast `codesign` fence); Windows: `TARGET_FILE_DIR/../..` for bundles, `TARGET_FILE` for VST and Standalone (old cmake.cast release-build-copy-win fences).
- `CAST_FORMATS` order = `## plugin format` row order = the fixture row order (Standalone, VST, VST3, AU, AAX).
- cast CLI sibling: `--sync` — `isSyncFlag` + `runSyncArguments` over `juce::ArgumentList` (main.cpp:1137-1153, :1217-1218). `ArgumentList::getValueForOption` reads long options as `--name=value`.
- Toolchain quote bug: `addTokens (flag, true)` keeps quote marks (juce_StringArray.cpp:343, :357) against SPEC §6.9:924-925.
- Background: `icvp` `backgroundType` 2 + `backgroundImageAlias` = alias record v2 (mac_alias_alias.py:566-750: fixed record `>4shh` + `>h28pI2shI64pII4s4shhI2s10s`, tags 0,16,17,1,2,14,15,18,19, terminator `>hh` -1 0, length patched at offset 4). Pre-mount values: volume name = archive stem, POSIX path `/.background.tiff`, carbon path `<vol>:.background.tiff`, CNIDs `0xFFFFFFFF` (`ALIAS_NO_CNID`, :35), dates 0, fs `H+`, disk type 0, levels -1/-1. dmgbuild puts the image at `.background.<ext>` on the volume root (dmgbuild_core_NEW.py:641, :649).
- Background asset: `jreng-filter-strip/Source/icons/dmg_background_600x680pt.tiff` (600×680 @1x + 1200×1360 @2x).

## Data (Step 2 — first, per R7)

jfs `project-info.md`:
- `## pack` — `| name | mac | win |` (sibling shape: `## release`, :266-269):
  - `folder | ../Release | ../Release`
  - `format | dmg | zip`
  - `platform | macOS | Windows`
- `## pack layout` — `| key | value |`, keys are the CLI words (family `line-wrap`, `assume-filename`): `icon-size 64`, `bundle-column 150`, `link-column 450`, `first-row 80`, `row-spacing 120`, `window-left 100`, `window-top 100`, `background ${CMAKE_SOURCE_DIR}/Source/icons/dmg_background_600x680pt.tiff`.
- `## plugin format` — `linkDirectory` column restored (mac drag targets).
- `## signing` — `notaryProfile | notary` restored.
- `## toolchain` — back to `| argument | command | flag |`; the `pack`/codesign/notarytool/stapler rows removed.
- `## index` — `@release-dmg`, `@release-zip` removed; old `## pack` rows removed.

Archive path (CMake, from data): `<folder>/<productName> v${PROJECT_VERSION} <platform>.<format>`.

## Steps

### Step 1: SPEC + HELP (SPEC governs)
- §1: remove `## pack`, `## pack layout` tables, host words, the `pack` command word; add the `--pack` invocation, its option words, `.background.<ext>`.
- §2.1 Invocation: `cast --pack <archive> [--<layout-key>=<n>]… [--background=<path>] (<item> <linkName> <linkTarget>)…` — arity 1 + 3n, n ≥ 1; empty `linkName`/`linkTarget` = no link; layout defaults 64/150/450/80/120/100/100; layout and background options with a `.zip` archive are fatal; paths resolve against the working directory.
- §6.9: remove `host` and the `pack` row; keep the quote rule.
- §6.12 Pack: rewrite as the `--pack` behaviour (zip rules kept; dmg stage/clone/links/`.DS_Store`/hdiutil/no mount kept; background: cloned to `<stage>/.background.<ext>`, `icvp` type 2 + alias to `/.background.<ext>` on the volume named `<name>`).
- §10.1: remove host, pack-table, pack-layout-table fatals; add `--pack` arity, non-integer layout value, layout/background with zip, missing item or background file.
- HELP.md follows SPEC (toolchain section back; pack section as CLI; example = the jfs CMake call).
**Validation:** every behaviour sentence maps to R1–R8; no table-driven pack text remains.

### Step 2: jfs data — as "Data" above.
**Validation:** grid tables; no value repeated; each pack value appears once.

### Step 3: jfs template
- `cast/cmake.cast`:
  - `format-directory` fence: restore `set(CAST_LINK_DIRECTORY_:::value::: ":::linkDirectory:::")` under APPLE.
  - new fence `pack-value` (sibling of `format-directory`): `if(APPLE) set(CAST_PACK_:::name::: ":::mac:::") elseif(WIN32) set(CAST_PACK_:::name::: ":::win:::") endif()`.
  - new fence `pack-layout`: `--:::key:::=:::value:::` list item.
  - post-build: restore `add_custom_target(post-build ALL)` + dependencies on every format target; build `CAST_PACK_ITEMS` per format (mac: `$<TARGET_BUNDLE_DIR:…>` `${CAST_FORMAT}` `${CAST_LINK_DIRECTORY_${CAST_FORMAT}}`; win: bundle dir for VST3/AAX, `TARGET_FILE` for VST/Standalone, `""` `""`); `add_custom_command(TARGET post-build POST_BUILD COMMAND cast --pack "<archive>" ${CAST_PACK_LAYOUT (APPLE only)} ${CAST_PACK_ITEMS} VERBATIM USES_TERMINAL)` for Release; then, APPLE and `CAST_SIGN`: `codesign --sign ":::identity:::" "<archive>"`, `xcrun notarytool submit "<archive>" --keychain-profile ":::notaryProfile:::" --wait`, `xcrun stapler staple "<archive>"`.
- `cast/spell.md`: bindings for `pack-value` (`@project-info:pack`) and `pack-layout` (`@project-info:pack layout`), in the @CMakeLists row, mirroring the `format-directory` row (:100).
**Validation:** read the rendered shape against the sibling; `if`/`endif` balanced; no literal the data holds.

### Step 4: cast vocabulary
- identifiers.md: remove `archive`, `host`, `item`, `link`, `linkName`, `mac`, `win`, `linux`, `packLayout`; keep `dmg`, `zip` (extension keys), `dsStore`, `hdiutil`, `pack` (now the `--pack` flag word); layout Id values become the CLI words (`iconSize` → `icon-size`, …); add `background`.
- text.md: remove `failHost`, `failPackLayout`; add `failPackArguments` (sibling `failSyncArguments`); keep `failArchiveExtension`, `failArchiveHost`.
**Validation:** each removed row has no remaining reference (compiler is the check at ARCHITECT's build).

### Step 5: cast — remove the table path, fix quotes
- Validator.h: delete `isHost`, `isArchive`, `getRootNames`, `isUniqueArchive`, `isPackLayout` and their `isManifest` calls.
- Processor.h: `run()` and `runToolchainRow()` back to the pre-Sprint-97 shape (no host, no pack dispatch).
- Toolchain.h: delete `getHost`, `isHostSelected`, `isRowSelected`, `getWorkingFile`; `getToolchainArguments` unquotes each token (`juce::String::unquoted()`), per SPEC §6.9.
**Validation:** no reference to the removed names; §6.9 quote rule holds.

### Step 6: writers
- PropertyListWriter: add the data object (`0x4n` marker) for `juce::var` binary data (`isBinaryData`).
- New `AliasWriter.h` (Writer family): `static juce::MemoryBlock getAlias (const juce::String& volumeName, const juce::String& fileName)` — alias v2 per the facts above, fixed pre-mount values, named constants.
- BinaryWriter: `getStore` reads layout from `const juce::ArgumentList&` (`getValueForOption`, defaults as named constants) and item/link names from the triples; with `--background`, `icvp` gets `backgroundType` 2 and `backgroundImageAlias`.
**Validation:** layout default bytes equal the Sprint 97 fixture layout; alias bytes decode with the mac_alias layout (Python cross-check in the scratchpad).

### Step 7: Pack.h — CLI-driven
- `static juce::Result toArchive (const juce::ArgumentList& arguments)`: archive = first positional; triples read in place; extension selects zip or dmg (`Function::Map`, as now); zip = `ZipWriter::toFile` with items and links; dmg = new stage (`getNonexistentSibling`), clone items, links, background clone to `.background.<ext>`, `.DS_Store`, `hdiutil create … -format UDZO`, stage removed.
**Validation:** no Model read; every failure is a §10.1 fatal.

### Step 8: main.cpp — `--pack` entry
- `getPackFlag`, `isPackFlag`, `runPackArguments` (siblings of the sync trio); arity check → `failPackArguments`; non-integer layout value → `failFlagValue`; dispatch before `runCommandLine`; no terminal clear on `--pack` (main.cpp:1206).
**Validation:** sibling shape; no new pattern.

### Step 9: Auditor once; doc pass (doxygen, HELP); /log; commit messages.

## Verification (ARCHITECT's runs)
1. `cast` regenerates cast's own `Source/generated/*`; build cast.
2. jfs: regenerate `CMakeLists.txt`; Release build with `CAST_SIGN=ON`.
3. dmg: Finder shows the icon layout **and the background** (the no-mount alias proof); `spctl -a -t open --context context:primary-signature -v`; `xcrun stapler validate`.
4. Windows: Release build writes the zip; two runs give identical bytes.
5. CMake passes the empty `""` link arguments on Windows (VERBATIM); if the generator drops them, cast's arity fatal names it.

## BLESSED Alignment
- **B:** the stage is owned and removed by the pack call.
- **L:** Pack, BinaryWriter, AliasWriter, PropertyListWriter, ZipWriter — one job each.
- **E:** every failure is a named §10.1 fatal; defaults are named constants.
- **S:** each pack value lives once in project-info.md; paths come from CMake.
- **D:** zip bytes fixed by input; `.DS_Store` bytes fixed by arguments.
