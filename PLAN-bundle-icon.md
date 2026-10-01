# PLAN: Bundle Icon in Every Pack Archive

**RFC:** none — objective from ARCHITECT prompt and DEBT-20260930T000000 (jfs)
**Date:** 2026-10-01
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 + JAM (cast); LANGUAGE.md C++ — single-header writers (LANGUAGE.md:32-34)

## Context
Finder draws the AU `.component` in the Release dmg as a blank "?" (DEBT-20260930T000000). ARCHITECT: *"must be consistent crossplatform. no hardcoded into any format"*; *"must be work exactly, identically with windows zip even mac zip"*.

One rule for every host: **the archive records the icon switch that is on the bundle on disk.**
- Windows: JUCE already writes `desktop.ini`, `Plugin.ico` and `attrib +s` on the `.vst3` / `.aaxplugin` folder (JUCEUtils.cmake:936-949, :1264, :1343). ZipWriter drops the attribute: host Unix, Unix mode only (ZipWriter.h:59, :304).
- macOS: signing clears attributes (jfs CMakeLists.txt:595). cast writes `Icon\r` + FinderInfo on a staged copy of each bundle, for zip and dmg.

## ARCHITECT Rulings
1. Icon source: *"Bundle's Info.plist"* — cast reads `CFBundleIconFile` from each bundle's generated Info.plist (juceaide, JUCEUtils.cmake:788; XML, AU Info.plist:1-9).
2. Zip attributes: *"Unix host + DOS byte"* — host Unix and Unix mode stay in the high 16 bits; MS-DOS attribute bits from disk go in the low byte.
3. *"why this is cast reponsibility? not cmake?"* — answered: the stage exists only inside cast (Pack.h:256-267). Sprint 98 ruling: *"DS_Store is cast reponsibilty"*.

## Dependency & API Inventory
- JUCE `juce::File`: no xattr / FinderInfo API (doxygen classjuce_1_1File.xml; juce_File.h:357-390, :1115). `juce::XmlDocument` parses the XML Info.plist. `juce::File::create`, `getChildFile`.
- JAM: no xattr / AppleDouble / plist API (jam docs/xml).
- macOS: `setxattr` / `getxattr` with `XATTR_FINDERINFO_NAME`, `XATTR_RESOURCEFORK_NAME` (`<sys/xattr.h>`), platform header under `#if JUCE_MAC` — sibling Pack.h:3-5.
- Windows: `GetFileAttributesW` (JUCE has `isHidden` only, juce_File.h:390; no System bit).
- Writer family (NAMES Rule 5): AliasWriter, BinaryWriter, PropertyListWriter, ZipWriter — header-only `struct`, static functions, private `static constexpr` format constants (AliasWriter.h:188-247), `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`.
- Diagnostics: `text::Diagnostics::failNotFound`, `failOutputWrite` reused (Pack.h:163, :355). No new text rows.
- Format constants stay private in the one writer that uses them (CODING.md Scope and Ownership 2). No identifiers.md rows.

## Validation Gate
COUNSELOR validates each step against MANIFESTO.md, NAMES.md, CODING.md and this PLAN before the next step. @Auditor runs once after the last step.

## Steps

### Step 1: Byte evidence (Pathfinder, read-only, scratchpad)
Dump, from `/Library/Audio/Plug-Ins/Components/UAD Softube Vocoder.component`: the `com.apple.FinderInfo` of the bundle and of `Icon\r`; the header and the map of the `Icon\r` resource fork. Copy the bundle to the scratchpad, `ditto -c -k --sequesterRsrc --keepParent` it, and dump the `__MACOSX/` entries. Every offset and constant in Steps 2 and 4 cites this dump.

### Step 2: IconWriter (new `Source/IconWriter.h`)
`static juce::Result toBundle (const juce::File& bundle)`. When `bundle/Contents/Info.plist` exists and its dict has `CFBundleIconFile`: read `Contents/Resources/<value>` verbatim; write `bundle/Icon\r` (empty data fork) with a resource fork of one `icns` resource, id -16455, those bytes; FinderInfo of `Icon\r` = type and creator zero (Step 1 dump: `00×8, 40 00, 00×22`), invisible flag 0x4000; FinderInfo of `bundle` = custom-icon flag. Icon file missing → `failNotFound`; write failure → `failOutputWrite`. No Info.plist or no key → ok. `#if JUCE_MAC`.

### Step 3: Pack stages on macOS for zip and dmg
On macOS both formats go through the stage: item clones + `IconWriter::toBundle` on each clone. Zip → `ZipWriter::toFile (archive, stagedItems, links)`; dmg → links, background, `.DS_Store`, `hdiutil` as now. Windows zip unchanged.

### Step 4: ZipWriter records the icon switch
- New `Source/AppleDoubleWriter.h`: `static std::optional<juce::MemoryBlock> getAppleDouble (const juce::File& entry)` — AppleDouble v2 from the entry's FinderInfo and resource fork; `nullopt` when it has neither. `#if JUCE_MAC`.
- `ZipWriter.h`: on macOS, each entry with an AppleDouble adds `__MACOSX/<parent>/._<name>` after the item entries, deflated, in walk order. On Windows, the low byte of the external attributes holds the ReadOnly, Hidden, System and Directory bits from `GetFileAttributesW`. No other xattrs recorded.

### Step 5: SPEC and HELP
`SPEC.md` §6.12 icon paragraph and zip additions; §10.1 row for a bundle icon that its Info.plist names and that does not exist. `HELP.md` pack section.

### Step 6: Audit, doc pass, log
@Auditor sweep; findings resolved per DCF; doxygen pass; jfs `carol/SPRINT-LOG.md` Sprint 99; `carol debt clear DEBT-20260930T000000`; commit messages in chat.

## BLESSED Alignment
- B: the stage owns every icon write; build artefacts untouched.
- L: two single-responsibility writers; functions ≤30 lines.
- E: every failure a §10.1 fatal; named constants.
- S: icon from the bundle's own Info.plist; disk is the one truth ZipWriter records.
- S (Stateless): writers keep no state.
- D: verbatim icns bytes; fixed entry order.

## Verification (ARCHITECT runs)
1. `cast` build; jfs Release build on macOS and Windows.
2. macOS dmg: each bundle shows the jfs icon in Finder; `codesign --verify` passes on each; `spctl` on the dmg; notarytool accepts; AAX loads in Pro Tools.
3. macOS zip: Archive Utility extract shows the icons.
4. Windows zip: Explorer extract shows the `.vst3` / `.aaxplugin` icons; two runs give identical bytes.
