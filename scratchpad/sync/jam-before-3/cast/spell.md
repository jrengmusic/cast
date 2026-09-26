```
████████████░░████████████░░████████████░░████████████░░
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░
████░░        ████░░  ████░░████░░            ████░░
████░░        ████████████░░████████████░░    ████░░
████░░        ████░░  ████░░        ████░░    ████░░
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░
████████████░░████░░  ████░░████████████░░    ████░░
```

## index

+-----------------------------+----------------------------------------------------------------------------------------+
| alias                       | symbol                                                                                 |
+=============================+========================================================================================+
| @code                       | code.cast                                                                              |
| @identifiers                | identifiers.md                                                                         |
| @text                       | text.md                                                                                |
| @chars                      | chars.md                                                                               |
| @jam_Identifiers            | ../generated/jam_Identifiers.h                                                         |
| @jam_Text                   | ../generated/jam_Text.h                                                                |
| @jam_Chars                  | ../generated/jam_Chars.h                                                               |
| @jam_Files                  | ../generated/jam_Files.h                                                               |
| @jam_Bimaps                 | ../generated/jam_Bimaps.h                                                              |
| @jam_OpenType               | ../generated/jam_OpenType.h                                                            |
| @jam_Terminal               | ../generated/jam_Terminal.h                                                            |
| @jam_Colours                | ../generated/jam_Colours.h                                                             |
| @jam_HashMaps               | ../generated/jam_HashMaps.h                                                            |
| @jam_LookupTables           | ../generated/jam_LookupTables.h                                                        |
| @jam_Mermaid                | ../generated/jam_Mermaid.h                                                             |
| @jam_Generated              | ../generated/jam_Generated.h                                                           |
| @syntax                     | syntax.md                                                                              |
| @entities                   | entities.md                                                                            |
| @files                      | files.md                                                                               |
| @bimaps                     | bimaps.md                                                                              |
| @opentype                   | opentype.md                                                                            |
| @terminal                   | terminal.md                                                                            |
| @colours                    | colours.md                                                                             |
| @lookuptables               | lookuptables.md                                                                        |
| @mermaid                    | mermaid.md                                                                             |
| @bimap                      | jam::Bimap<int>                                                                        |
| @bimapIdentifier            | jam::Bimap<int, juce::Identifier>                                                      |
| @bimapUint32                | jam::Bimap<uint32_t>                                                                   |
| @colourSpace                | jam::LookupTable<int, uint32_t, 256>                                                   |
| @codePoints                 | jam::LookupTable<int, juce::juce_wchar, CssTokenType::closeBrace + 1>                  |
| @markupTags                 | jam::LookupTable<int, const char*, HtmlBlockType::cdata + 1>                           |
| @rawTextTags                | jam::LookupTable<int, const char*, HtmlType1Tag::style + 1>                            |
| @byteClass                  | jam::LookupTable<int, int, 256>                                                        |
| @os2UnicodeRange            | jam::LookupTable<int, int, 169>                                                        |
| @lineBreakAmbiguous         | jam::LookupTable<int, int, map::LineBreakLanguage::zh + 1>                             |
| @lineBreakTailoring         | jam::LookupTable<int, int, 30>                                                         |
| @lineBreakMask              | jam::LookupTable<int, uint64_t, map::LineBreak::XX + 1>                                |
| @blockLink                  | jam::LookupTable<int, jam::Union<uint8_t, uint8_t>, Operator::invisible + 1>           |
| @cornerRadius               | jam::LookupTable<int, int, 28>                                                         |
| @cardinality                | jam::LookupTable<int, int, Operator::erZeroOrMoreAlt + 1>                              |
| @ganttRole                  | jam::LookupTable<int, int, Keyword::milestone + 1>                                     |
| @eventModelingSwimlane      | jam::LookupTable<int, int, Keyword::rmo + 1>                                           |
| @eventModelingSwimlaneLabel | jam::LookupTable<int, const char*, 3>                                                  |
| @eventModelingColourId      | jam::LookupTable<int, int, Keyword::rmo + 1>                                           |
| @durationUnit               | jam::LookupTable<int, float, 128>                                                      |
| @statusColourId             | jam::LookupTable<int, int, Keyword::milestone + 1>                                     |
| @sectionColourId            | jam::LookupTable<int, int, 2>                                                          |
| @priorityColourId           | jam::LookupTable<int, int, Keyword::veryHigh + 1>                                      |
| @quadrantCorner             | jam::LookupTable<int, jam::Union<uint8_t, uint8_t>, 4>                                 |
| @axisTickDay                | jam::LookupTable<int, double, 5>                                                       |
| @svgPath                    | jam::LookupTable<int, const char*, 22>                                                 |
| @shapePathId                | jam::LookupTable<int, int, 28>                                                         |
| @markerPathId               | jam::LookupTable<int, int, 14>                                                         |
| @markerAnchor               | jam::LookupTable<int, jam::Union<float, float>, 14>                                    |
| @markerStroke               | jam::LookupTable<int, jam::Union<uint8_t, float>, 14>                                  |
| @decorationGeometry         | jam::LookupTable<int, jam::Union<uint8_t, float, uint8_t>, 14>                         |
| @arrowheadPathId            | jam::LookupTable<int, int, 46>                                                         |
| @arrowheadAnchor            | jam::LookupTable<int, jam::Union<float, float>, 46>                                    |
| @classRelation              | jam::LookupTable<int, RelationValue, 16>                                               |
| @sequenceArrow              | jam::LookupTable<int, jam::Union<uint8_t, uint8_t, uint8_t, uint8_t>, 8>               |
| @stateStereotype            | jam::LookupTable<int, jam::Union<uint8_t, uint8_t>, 3>                                 |
| @mindmapShape               | jam::LookupTable<int, jam::Union<uint8_t, uint8_t, uint8_t>, 6>                        |
| @c4Role                     | jam::LookupTable<int, int, Keyword::title + 1>                                         |
| @flowchartLink              | jam::LookupTable<int, jam::Union<uint8_t, uint8_t, uint8_t>, Operator::dashedLink + 1> |
| @shape                      | jam::LookupTable<int, jam::Union<uint8_t, uint8_t, uint8_t>, 13>                       |
| @padding                    | jam::LookupTable<int, jam::Union<float, float>, 28>                                    |
| @geometry                   | jam::LookupTable<int, GeometryValue, 28>                                               |
| @groupLayout                | jam::LookupTable<int, int, GroupType::timelineSection + 1>                             |
| @architectureRole           | jam::LookupTable<int, int, Keyword::blank + 1>                                         |
| @architectureLink           | jam::LookupTable<int, jam::Union<uint8_t, uint8_t, uint8_t>, 5>                        |
| @architectureAnchor         | jam::LookupTable<int, jam::Union<float, float>, Keyword::sideB + 1>                    |
| @architecturePathId         | jam::LookupTable<int, int, Keyword::blank + 1>                                         |
| @iconPathId                 | jam::LookupTable<int, int, Keyword::car + 1>                                           |
| @architectureAxis           | jam::LookupTable<int, int, Keyword::sideB + 1>                                         |
| @directionMask              | jam::LookupTable<int, int, Direction::y + 1>                                           |
| @c4Trait                    | jam::LookupTable<int, jam::Union<uint8_t, uint8_t, uint8_t>, 20>                       |
| @c4Boundary                 | jam::LookupTable<int, jam::Union<uint8_t, uint8_t>, Keyword::nodeR + 1>                |
| @c4Stereotype               | jam::LookupTable<int, const char*, 8>                                                  |
| @c4Relation                 | jam::LookupTable<int, jam::Union<uint8_t, uint8_t, uint8_t>, Keyword::title + 1>       |
| @gitRole                    | jam::LookupTable<int, int, Keyword::parent + 1>                                        |
| @requirementRole            | jam::LookupTable<int, int, Keyword::test + 1>                                          |
| @requirementStereotype      | jam::LookupTable<int, const char*, 7>                                                  |
| @attributeLabel             | jam::LookupTable<int, const char*, Keyword::test + 1>                                  |
| @cynefinDecisionModel       | jam::LookupTable<int, const char*, Keyword::confusion + 1>                             |
| @cynefinPractice            | jam::LookupTable<int, const char*, Keyword::confusion + 1>                             |
| @railroadLeafFillColourId   | jam::LookupTable<int, int, TermType::special + 1>                                      |
| @railroadLeafBorderColourId | jam::LookupTable<int, int, TermType::special + 1>                                      |
| @railroadLeafTextColourId   | jam::LookupTable<int, int, TermType::special + 1>                                      |
| @modules                    | ../user-modules-info.md                                                                |
| @jam_animation              | ../jam_animation/jam_animation.h                                                       |
| @jam_audio_devices          | ../jam_audio_devices/jam_audio_devices.h                                               |
| @jam_clap                   | ../jam_clap/jam_clap.h                                                                 |
| @jam_core                   | ../jam_core/jam_core.h                                                                 |
| @jam_data_structures        | ../jam_data_structures/jam_data_structures.h                                           |
| @jam_debug                  | ../jam_debug/jam_debug.h                                                               |
| @jam_document               | ../jam_document/jam_document.h                                                         |
| @jam_dsp                    | ../jam_dsp/jam_dsp.h                                                                   |
| @jam_freetype               | ../jam_freetype/jam_freetype.h                                                         |
| @jam_fuzzy                  | ../jam_fuzzy/jam_fuzzy.h                                                               |
| @jam_graphics               | ../jam_graphics/jam_graphics.h                                                         |
| @jam_gui                    | ../jam_gui/jam_gui.h                                                                   |
| @jam_markdown               | ../jam_markdown/jam_markdown.h                                                         |
| @jam_mermaid_diagram        | ../jam_mermaid_diagram/jam_mermaid_diagram.h                                           |
| @jam_opentype               | ../jam_opentype/jam_opentype.h                                                         |
| @jam_plugin_bootstrap       | ../jam_plugin_bootstrap/jam_plugin_bootstrap.h                                         |
| @jam_style                  | ../jam_style/jam_style.h                                                               |
| @jam_subprocess             | ../jam_subprocess/jam_subprocess.h                                                     |
| @jam_terminal               | ../jam_terminal/jam_terminal.h                                                         |
| @jam_vulkan                 | ../jam_vulkan/jam_vulkan.h                                                             |
| @jam_web                    | ../jam_web/jam_web.h                                                                   |
| @jam_whelmed                | ../jam_whelmed/jam_whelmed.h                                                           |
+-----------------------------+----------------------------------------------------------------------------------------+

## headers

+--------------------+-----------------------------------------------------------------------------------------+---------+
| file               | brief                                                                                   | comment |
+====================+=========================================================================================+=========+
| jam_Identifiers.h  | ```                                                                                     |         |
|                    | @file jam_Identifiers.h                                                                 |         |
|                    | @brief Identifier and name-string constants for the jam framework vocabulary.           |         |
|                    | ```                                                                                     |         |
+--------------------+-----------------------------------------------------------------------------------------+---------+
| jam_Text.h         | ```                                                                                     |         |
|                    | @file jam_Text.h                                                                        |         |
|                    | @brief Framework-localised English UI and diagnostic strings.                           |         |
|                    | ```                                                                                     |         |
+--------------------+-----------------------------------------------------------------------------------------+---------+
| jam_Chars.h        | ```                                                                                     |         |
|                    | @file jam_Chars.h                                                                       |         |
|                    | @brief Character codepoints, token literals, and the character relation maps.           |         |
|                    | ```                                                                                     |         |
+--------------------+-----------------------------------------------------------------------------------------+---------+
| jam_Files.h        | ```                                                                                     |         |
|                    | @file jam_Files.h                                                                       |         |
|                    | @brief Framework asset file names and recognised file extensions.                       |         |
|                    | ```                                                                                     |         |
+--------------------+-----------------------------------------------------------------------------------------+---------+
| jam_Bimaps.h       | ```                                                                                     |         |
|                    | @file jam_Bimaps.h                                                                      |         |
|                    | @brief Bidirectional name-to-key registries for the framework vocabulary.               |         |
|                    | ```                                                                                     |         |
+--------------------+-----------------------------------------------------------------------------------------+---------+
| jam_OpenType.h     | ```                                                                                     |         |
|                    | @file jam_OpenType.h                                                                    |         |
|                    | @brief OpenType binary-format vocabulary — sfnt tags, cmap platform and                 |         |
|                    | encoding IDs, name IDs, OS/2 bits, PANOSE fields, GSUB lookup types,                    |         |
|                    | standard glyph names, and RIBBI styles.                                                 |         |
|                    | ```                                                                                     |         |
+--------------------+-----------------------------------------------------------------------------------------+---------+
| jam_Terminal.h     | ```                                                                                     |         |
|                    | @file jam_Terminal.h                                                                    |         |
|                    | @brief Terminal control-sequence vocabulary — DEC/ANSI modes, CSI/ESC/OSC/SGR           |         |
|                    | codes, and the bidirectional registries mapping them to their names.                    |         |
|                    | ```                                                                                     |         |
+--------------------+-----------------------------------------------------------------------------------------+---------+
| jam_Colours.h      | ```                                                                                     |         |
|                    | @file jam_Colours.h                                                                     |         |
|                    | @brief Named colour palette and component colour-id registries.                         |         |
|                    | ```                                                                                     |         |
+--------------------+-----------------------------------------------------------------------------------------+---------+
| jam_HashMaps.h     | ```                                                                                     |         |
|                    | @file jam_HashMaps.h                                                                    |         |
|                    | @brief Static lookup maps for language families, roman numerals, entities, and escapes. |         |
|                    | ```                                                                                     |         |
+--------------------+-----------------------------------------------------------------------------------------+---------+
| jam_LookupTables.h | ```                                                                                     |         |
|                    | @file jam_LookupTables.h                                                                |         |
|                    | @brief Direct-indexed lookup tables for terminal colour, markup, and syntax dispatch.   |         |
|                    | ```                                                                                     |         |
+--------------------+-----------------------------------------------------------------------------------------+---------+
| jam_Mermaid.h      | ```                                                                                     |         |
|                    | @file jam_Mermaid.h                                                                     |         |
|                    | @brief Mermaid diagram vocabulary — keywords, shapes, styles, operators, and            |         |
|                    | the packed geometry/marker tables that map them onto render primitives.                 |         |
|                    |                                                                                         |         |
|                    | The Mermaid struct is the single generated source of truth for every                    |         |
|                    | diagram dialect the renderer accepts: front-matter diagram types, node                  |         |
|                    | shapes and their construction/hit-test/size policies, edge strokes and                  |         |
|                    | decorations, stylesheet colour-id and metric keys, and the full operator                |         |
|                    | alphabet (delimiters, messages, class relations, ER cardinalities). Each                |         |
|                    | `jam::Bimap` exposes a bidirectional key↔literal registry via getInstance();            |         |
|                    | each `jam::LookupTable` is a `static constexpr` direct-indexed conversion               |         |
|                    | consumed by the layout and draw passes.                                                 |         |
|                    | ```                                                                                     |         |
+--------------------+-----------------------------------------------------------------------------------------+---------+
| jam_Generated.h    | ```                                                                                     |         |
|                    | @file jam_Generated.h                                                                   |         |
|                    | @brief Generated-header umbrella — re-exports and instantiates every registry.          |         |
|                    |                                                                                         |         |
|                    | The Generated struct aggregates the framework's generated shared-instance               |         |
|                    | registries: constructing it constructs every generated bimap exactly once,              |         |
|                    | giving a single point of ownership for the whole generated vocabulary.                  |         |
|                    | ```                                                                                     |         |
+--------------------+-----------------------------------------------------------------------------------------+---------+

## output

+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| list                                                           | separator                     | structure                                                          | file                  |
+================================================================+===============================+====================================================================+=======================+
| - [list]: @identifiers                                         |                               | @code:namespace                                                    | @jam_Identifiers      |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - generated:                                                       |                       |
|                                                                |                               | - name: Id                                                         |                       |
|                                                                |                               | - [comment]: @headers:brief                                        |                       |
|                                                                |                               | - [list]: @code:identifier                                         |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > - [list]: @text:english                                      |                               | @code:namespace                                                    | @jam_Text             |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - generated:                                                       |                       |
|                                                                |                               | - name: text                                                       |                       |
|                                                                |                               | - [comment]: @headers:brief                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:struct                                                       |                       |
|                                                                |                               | - name: English                                                    |                       |
|                                                                |                               | > - [list]: @code:char                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > - [list]: @chars:chars                                       |                               | @code:chars                                                        | @jam_Chars            |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - generated:                                                       |                       |
|                                                                |                               | - [comment]: @headers:brief                                        |                       |
| > - [list]: @chars:tokens                                      |                               | > - [list]: @code:wchar                                            |                       |
| > > - [list]: @chars:escape                                    |                               | > - [list]: @code:char                                             |                       |
|                                                                |                               | > > - [list]: @code:map-entry                                      |                       |
| > > - [list]: @chars:enclosure                                 |                               | > > - [list]: @code:map-entry                                      |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > - [list]: @files:extensions                                  | - [list]: @code:linebreak     | @code:line                                                         | @jam_Files            |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - generated:                                                       |                       |
|                                                                |                               | - [comment]: @headers:brief                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:struct                                                       |                       |
|                                                                |                               | - name: Extensions                                                 |                       |
|                                                                |                               | > - [list]: @code:char                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > - [list]: @files:files                                       | - [list]: @code:linebreak     | @code:line                                                         | @jam_Files            |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:namespace                                                    |                       |
|                                                                |                               | - name: files                                                      |                       |
|                                                                |                               | > - [list]: @code:identifier                                       |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:TaperMap                               | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:TaperMap                                 |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: TaperMap                                                   |                       |
|                                                                |                               | - type: TaperMap                                                   |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Curve taper values.                                   |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:XmlTokenType                           | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:XmlTokenType                             |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: XmlTokenType                                               |                       |
|                                                                |                               | - type: XmlTokenType                                               |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: XML token kinds.                                      |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:AnsiTokenType                          | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:AnsiTokenType                            |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: AnsiTokenType                                              |                       |
|                                                                |                               | - type: AnsiTokenType                                              |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: ANSI token kinds.                                     |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:Segment                                | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:Segment                                  |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: Segment                                                    |                       |
|                                                                |                               | - type: Segment                                                    |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: UI nine-segment anchors.                              |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:ButtonState                            | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:ButtonState                              |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: ButtonState                                                |                       |
|                                                                |                               | - type: ButtonState                                                |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Button visual states.                                 |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:Position                               | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:Position                                 |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: Position                                                   |                       |
|                                                                |                               | - type: Position                                                   |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Layout position anchors.                              |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:Orientation                            | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:Orientation                              |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: Orientation                                                |                       |
|                                                                |                               | - type: Orientation                                                |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Axis orientation.                                     |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:windowFX mac                           | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @bimaps:windowFX windows                       |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
| > > - [list]: @bimaps:windowFX mac                             |                               | @code:window-fx                                                    |                       |
|                                                                |                               | - name: WindowFX                                                   |                       |
|                                                                |                               | - type: WindowFX                                                   |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Native window effects.                                |                       |
| > > - [list]: @bimaps:windowFX windows                         |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:BlockType                              | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:BlockType                                |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: BlockType                                                  |                       |
|                                                                |                               | - type: BlockType                                                  |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Markdown block kinds.                                 |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:MarkdownTokenType                      | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:MarkdownTokenType                        |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: MarkdownTokenType                                          |                       |
|                                                                |                               | - type: MarkdownTokenType                                          |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Markdown inline-token kinds.                          |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:DocumentTokenType                      | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:DocumentTokenType                        |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: DocumentTokenType                                          |                       |
|                                                                |                               | - type: DocumentTokenType                                          |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Document token kinds.                                 |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:Byte                                   | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:Byte                                     |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: Byte                                                       |                       |
|                                                                |                               | - type: Byte                                                       |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Byte classification.                                  |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:LineBreak                              | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:LineBreak                                |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: LineBreak                                                  |                       |
|                                                                |                               | - type: LineBreak                                                  |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Unicode Line_Break classes (UAX #14).                 |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:LineBreakLanguage                      | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:LineBreakLanguage                        |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: LineBreakLanguage                                          |                       |
|                                                                |                               | - type: LineBreakLanguage                                          |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Language input for UAX #14 tailoring.                 |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:HeadingLevel                           | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:HeadingLevel                             |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: HeadingLevel                                               |                       |
|                                                                |                               | - type: HeadingLevel                                               |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Heading levels one to six.                            |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:HtmlType1Tag                           | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:HtmlType1Tag                             |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: HtmlType1Tag                                               |                       |
|                                                                |                               | - type: HtmlType1Tag                                               |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Raw-text HTML tag kinds.                              |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:HtmlBlockTag                           | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:HtmlBlockTag                             |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: HtmlBlockTag                                               |                       |
|                                                                |                               | - type: HtmlBlockTag                                               |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Block-level HTML tags.                                |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:SyntaxTokenType                        | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:SyntaxTokenType                          |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: SyntaxTokenType                                            |                       |
|                                                                |                               | - type: SyntaxTokenType                                            |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Syntax-highlight token kinds.                         |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:Family                                 | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:Family                                   |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: Family                                                     |                       |
|                                                                |                               | - type: Family                                                     |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Language family classification.                       |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:OpenBlock                              | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:OpenBlock                                |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: OpenBlock                                                  |                       |
|                                                                |                               | - type: OpenBlock                                                  |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Markdown block-open constructs.                       |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:CppKeyword                             | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:CppKeyword                               |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: CppKeyword                                                 |                       |
|                                                                |                               | - type: CppKeyword                                                 |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: C++ keyword set.                                      |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:JsKeyword                              | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:JsKeyword                                |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: JsKeyword                                                  |                       |
|                                                                |                               | - type: JsKeyword                                                  |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: JavaScript keyword set.                               |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:PythonKeyword                          | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:PythonKeyword                            |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: PythonKeyword                                              |                       |
|                                                                |                               | - type: PythonKeyword                                              |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Python keyword set.                                   |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:CssTokenType                           | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:CssTokenType                             |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: CssTokenType                                               |                       |
|                                                                |                               | - type: CssTokenType                                               |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: CSS token kinds.                                      |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:CssRuleType                            | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:CssRuleType                              |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: CssRuleType                                                |                       |
|                                                                |                               | - type: CssRuleType                                                |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: CSS at-rule kinds.                                    |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:HtmlTokenType                          | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:HtmlTokenType                            |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: HtmlTokenType                                              |                       |
|                                                                |                               | - type: HtmlTokenType                                              |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: HTML token kinds.                                     |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:HtmlVoidTag                            | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:HtmlVoidTag                              |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: HtmlVoidTag                                                |                       |
|                                                                |                               | - type: HtmlVoidTag                                                |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: HTML void-element tags.                               |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:VariDisplayMode                        | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:VariDisplayMode                          |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: VariDisplayMode                                            |                       |
|                                                                |                               | - type: VariDisplayMode                                            |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Vari display modes.                                   |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:PluginWrapper                          | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:PluginWrapper                            |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: PluginWrapper                                              |                       |
|                                                                |                               | - type: PluginWrapper                                              |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Plugin wrapper formats.                               |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:AnalyzerMode                           | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:AnalyzerMode                             |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: AnalyzerMode                                               |                       |
|                                                                |                               | - type: AnalyzerMode                                               |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Spectrum analyzer modes.                              |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:Appearance                             | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:Appearance                               |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:appearance                                                   |                       |
|                                                                |                               | - name: Appearance                                                 |                       |
|                                                                |                               | - type: Appearance                                                 |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Application appearance themes.                        |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:AudioParameter                         | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:AudioParameter                           |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: AudioParameter                                             |                       |
|                                                                |                               | - type: AudioParameter                                             |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Audio parameter kinds.                                |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:Display                                | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:Display                                  |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: Display                                                    |                       |
|                                                                |                               | - type: Display                                                    |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Parameter display modes.                              |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:FontRasterizerBackend                  | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:FontRasterizerBackend                    |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: FontRasterizerBackend                                      |                       |
|                                                                |                               | - type: FontRasterizerBackend                                      |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Font rasterizer backends.                             |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:ImageResample                          | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:ImageResample                            |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: ImageResample                                              |                       |
|                                                                |                               | - type: ImageResample                                              |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Image resample filters.                               |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:MouseButton                            | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:MouseButton                              |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: MouseButton                                                |                       |
|                                                                |                               | - type: MouseButton                                                |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Mouse buttons.                                        |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:Oversampling                           | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:Oversampling                             |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: Oversampling                                               |                       |
|                                                                |                               | - type: Oversampling                                               |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Oversampling factors.                                 |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:UIScaleMap                             | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:UIScaleMap                               |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: UIScaleMap                                                 |                       |
|                                                                |                               | - type: UIScaleMap                                                 |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: UI scale presets.                                     |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:HtmlStandardTag                        | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:HtmlStandardTag                          |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: HtmlStandardTag                                            |                       |
|                                                                |                               | - type: HtmlStandardTag                                            |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Standard HTML element tags.                           |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:ParameterPage                          | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:ParameterPage                            |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: ParameterPage                                              |                       |
|                                                                |                               | - type: ParameterPage                                              |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Parameter page labels.                                |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:ViewOrientation                        | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:ViewOrientation                          |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: ViewOrientation                                            |                       |
|                                                                |                               | - type: ViewOrientation                                            |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Device view orientations.                             |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:AtRuleType                             | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:AtRuleType                               |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: AtRuleType                                                 |                       |
|                                                                |                               | - type: AtRuleType                                                 |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: CSS at-rule discriminators.                           |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:BlockTag                               | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:BlockTag                                 |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: BlockTag                                                   |                       |
|                                                                |                               | - type: BlockTag                                                   |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Markdown block to HTML tag map.                       |                       |
|                                                                |                               | - base: @bimapIdentifier                                           |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::Identifier                                      |                       |
|                                                                |                               | > > > - [list]: @code:identifier-entry                             |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:HeadingTag                             | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:HeadingTag                               |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: HeadingTag                                                 |                       |
|                                                                |                               | - type: HeadingTag                                                 |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Heading level to HTML tag map.                        |                       |
|                                                                |                               | - base: @bimapIdentifier                                           |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::Identifier                                      |                       |
|                                                                |                               | > > > - [list]: @code:identifier-entry                             |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:Sharps                                 | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:Sharps                                   |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: Sharps                                                     |                       |
|                                                                |                               | - type: Sharps                                                     |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Sharp key names.                                      |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:Flats                                  | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:Flats                                    |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: Flats                                                      |                       |
|                                                                |                               | - type: Flats                                                      |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Flat key names.                                       |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:HtmlBlockType                          | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:HtmlBlockType                            |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: HtmlBlockType                                              |                       |
|                                                                |                               | - type: HtmlBlockType                                              |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: HTML block start-condition kinds.                     |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:OverlayAxisLine                        | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:OverlayAxisLine                          |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: OverlayAxisLine                                            |                       |
|                                                                |                               | - type: OverlayAxisLine                                            |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Message-overlay axis line styles.                     |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:EditorMode                             | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:EditorMode                               |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: EditorMode                                                 |                       |
|                                                                |                               | - type: EditorMode                                                 |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Text editor modes.                                    |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:EditorState                            | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:EditorState                              |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: EditorState                                                |                       |
|                                                                |                               | - type: EditorState                                                |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Text editor state properties.                         |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @bimaps:VisualMode                             | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Bimaps           |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @bimaps:VisualMode                               |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: VisualMode                                                 |                       |
|                                                                |                               | - type: VisualMode                                                 |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Visual selection shapes.                              |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @opentype:SfntTableTag                         | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_OpenType         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @opentype:SfntTableTag                           |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: SfntTableTag                                               |                       |
|                                                                |                               | - type: SfntTableTag                                               |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: sfnt table directory tags.                            |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @opentype:CmapPlatformId                       | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_OpenType         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @opentype:CmapPlatformId                         |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: CmapPlatformId                                             |                       |
|                                                                |                               | - type: CmapPlatformId                                             |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: cmap platform IDs.                                    |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @opentype:CmapEncodingId                       | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_OpenType         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @opentype:CmapEncodingId                         |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: CmapEncodingId                                             |                       |
|                                                                |                               | - type: CmapEncodingId                                             |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: cmap encoding IDs, platform-prefixed.                 |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @opentype:NameId                               | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_OpenType         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @opentype:NameId                                 |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: NameId                                                     |                       |
|                                                                |                               | - type: NameId                                                     |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: `name` table nameID vocabulary.                       |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @opentype:Os2FsSelection                       | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_OpenType         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @opentype:Os2FsSelection                         |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: Os2FsSelection                                             |                       |
|                                                                |                               | - type: Os2FsSelection                                             |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: `OS/2.fsSelection` bit flags.                         |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @opentype:PanoseField                          | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_OpenType         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @opentype:PanoseField                            |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: PanoseField                                                |                       |
|                                                                |                               | - type: PanoseField                                                |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: PANOSE byte field positions.                          |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @opentype:GsubLookupType                       | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_OpenType         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @opentype:GsubLookupType                         |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: GsubLookupType                                             |                       |
|                                                                |                               | - type: GsubLookupType                                             |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: GSUB LookupType constants.                            |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @opentype:StandardGlyphName                    | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_OpenType         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @opentype:StandardGlyphName                      |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: StandardGlyphName                                          |                       |
|                                                                |                               | - type: StandardGlyphName                                          |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Macintosh standard glyph names this module emits.     |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @opentype:RibbiStyle                           | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_OpenType         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @opentype:RibbiStyle                             |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: RibbiStyle                                                 |                       |
|                                                                |                               | - type: RibbiStyle                                                 |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: RIBBI style names.                                    |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:Screen                               | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - generated:                                                       |                       |
| > > - [list]: @terminal:Screen                                 |                               | - name: map                                                        |                       |
|                                                                |                               | - [comment]: @headers:brief                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: Screen                                                     |                       |
|                                                                |                               | - type: Screen                                                     |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Terminal screen buffer mode.                          |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:MouseTracking                        | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:MouseTracking                          |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: MouseTracking                                              |                       |
|                                                                |                               | - type: MouseTracking                                              |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Terminal mouse-tracking modes.                        |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:DEC                                  | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:DEC                                    |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: DEC                                                        |                       |
|                                                                |                               | - type: DEC                                                        |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: DEC private mode numbers.                             |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:OSC                                  | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:OSC                                    |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: OSC                                                        |                       |
|                                                                |                               | - type: OSC                                                        |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: OSC command codes.                                    |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:SGR                                  | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:SGR                                    |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: SGR                                                        |                       |
|                                                                |                               | - type: SGR                                                        |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: SGR rendition parameters.                             |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:ColorMode                            | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:ColorMode                              |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: ColorMode                                                  |                       |
|                                                                |                               | - type: ColorMode                                                  |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: SGR extended-colour modes.                            |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:UnderlineStyle                       | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:UnderlineStyle                         |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: UnderlineStyle                                             |                       |
|                                                                |                               | - type: UnderlineStyle                                             |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: SGR underline styles.                                 |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:CSI                                  | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:CSI                                    |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: CSI                                                        |                       |
|                                                                |                               | - type: CSI                                                        |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: CSI final-byte commands.                              |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:WindowOps                            | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:WindowOps                              |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: WindowOps                                                  |                       |
|                                                                |                               | - type: WindowOps                                                  |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: CSI window-operation codes.                           |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:DSR                                  | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:DSR                                    |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: DSR                                                        |                       |
|                                                                |                               | - type: DSR                                                        |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: DSR sub-command codes.                                |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:TabClear                             | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:TabClear                               |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: TabClear                                                   |                       |
|                                                                |                               | - type: TabClear                                                   |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Tab-clear sub-mode codes.                             |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:CursorShape                          | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:CursorShape                            |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: CursorShape                                                |                       |
|                                                                |                               | - type: CursorShape                                                |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Cursor shape codes.                                   |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:DECRQSS                              | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:DECRQSS                                |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: DECRQSS                                                    |                       |
|                                                                |                               | - type: DECRQSS                                                    |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: DECRQSS setting codes.                                |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:CsiIntermediate                      | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:CsiIntermediate                        |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: CsiIntermediate                                            |                       |
|                                                                |                               | - type: CsiIntermediate                                            |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: CSI intermediate bytes.                               |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:ESC                                  | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:ESC                                    |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: ESC                                                        |                       |
|                                                                |                               | - type: ESC                                                        |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: ESC final-byte sequences.                             |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:CharsetIntermediate                  | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:CharsetIntermediate                    |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: CharsetIntermediate                                        |                       |
|                                                                |                               | - type: CharsetIntermediate                                        |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Charset-select intermediate bytes.                    |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:CharsetDesignator                    | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:CharsetDesignator                      |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: CharsetDesignator                                          |                       |
|                                                                |                               | - type: CharsetDesignator                                          |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Charset-designator final bytes.                       |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:DecEscIntermediate                   | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:DecEscIntermediate                     |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: DecEscIntermediate                                         |                       |
|                                                                |                               | - type: DecEscIntermediate                                         |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: DEC ESC intermediate byte.                            |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:DecEscFinal                          | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:DecEscFinal                            |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: DecEscFinal                                                |                       |
|                                                                |                               | - type: DecEscFinal                                                |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: DEC ESC final byte.                                   |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:ModeReport                           | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:ModeReport                             |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: ModeReport                                                 |                       |
|                                                                |                               | - type: ModeReport                                                 |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: DECRQM response states.                               |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:ANSI                                 | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:ANSI                                   |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: ANSI                                                       |                       |
|                                                                |                               | - type: ANSI                                                       |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: ANSI mode numbers.                                    |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:ShellIntegration                     | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:ShellIntegration                       |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: ShellIntegration                                           |                       |
|                                                                |                               | - type: ShellIntegration                                           |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: OSC 133 sub-commands.                                 |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @terminal:KeyboardAssignMode                   | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Terminal         |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @terminal:KeyboardAssignMode                     |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: KeyboardAssignMode                                         |                       |
|                                                                |                               | - type: KeyboardAssignMode                                         |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Keyboard flag-assignment modes.                       |                       |
|                                                                |                               | - base: @bimap                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:name-entry                                   |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @colours:ColourNames                           | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Colours          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @colours:ColourNames                             |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: ColourNames                                                |                       |
|                                                                |                               | - type: ColourNames                                                |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Named colour palette.                                 |                       |
|                                                                |                               | - base: @bimapUint32                                               |                       |
|                                                                |                               | - keyType: uint32_t                                                |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | > > > - [list]: @code:bimap-entry                                  |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @colours:ColourId                              | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_Colours          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @colours:ColourId                                |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:bimap                                                        |                       |
|                                                                |                               | - name: ColourId                                                   |                       |
|                                                                |                               | - type: ColourId                                                   |                       |
|                                                                |                               | - instance:                                                        |                       |
|                                                                |                               | - [comment]: Component colour-id to CSS variable-name map.         |                       |
|                                                                |                               | - base: @bimapIdentifier                                           |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: juce::Identifier                                      |                       |
|                                                                |                               | > > > - [list]: @code:identifier-entry                             |                       |
|                                                                |                               | > > - [list]: @code:enum-entry                                     |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:terminalColourSpace                | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - generated:                                                       |                       |
| > > - [list]: @lookuptables:terminalColourSpace:key            | > > - [list]: @code:comma     | - name: map                                                        |                       |
|                                                                |                               | - [comment]: @headers:brief                                        |                       |
| > > - [list]: @lookuptables:terminalColourSpace:value          |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @colourSpace                                               |                       |
|                                                                |                               | - name: terminalColourSpace                                        |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:cssCodePoints                      | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:cssCodePoints:key                  | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:cssCodePoints:value                |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @codePoints                                                |                       |
|                                                                |                               | - name: cssCodePoints                                              |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:markupOpen                         | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:markupOpen:key                     | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:markupOpen:value                   |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @markupTags                                                |                       |
|                                                                |                               | - name: markupOpen                                                 |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:markupClose                        | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:markupClose:key                    | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:markupClose:value                  |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @markupTags                                                |                       |
|                                                                |                               | - name: markupClose                                                |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:rawTextEnd                         | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:rawTextEnd:key                     | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:rawTextEnd:value                   |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @rawTextTags                                               |                       |
|                                                                |                               | - name: rawTextEnd                                                 |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:markupLanguage                     | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:markupLanguage:key                 | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:markupLanguage:value               |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @byteClass                                                 |                       |
|                                                                |                               | - name: markupLanguage                                             |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:css                                | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:css:key                            | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:css:value                          |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @byteClass                                                 |                       |
|                                                                |                               | - name: css                                                        |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:markdown                           | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:markdown:key                       | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:markdown:value                     |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @byteClass                                                 |                       |
|                                                                |                               | - name: markdown                                                   |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:code                               | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:code:key                           | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:code:value                         |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @byteClass                                                 |                       |
|                                                                |                               | - name: code                                                       |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:markup                             | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:markup:key                         | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:markup:value                       |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @byteClass                                                 |                       |
|                                                                |                               | - name: markup                                                     |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:data                               | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:data:key                           | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:data:value                         |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @byteClass                                                 |                       |
|                                                                |                               | - name: data                                                       |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:ansi                               | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:ansi:key                           | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:ansi:value                         |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @byteClass                                                 |                       |
|                                                                |                               | - name: ansi                                                       |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:reflow                             | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:reflow:key                         | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:reflow:value                       |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @byteClass                                                 |                       |
|                                                                |                               | - name: reflow                                                     |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:os2UnicodeRangeStarts              | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:os2UnicodeRangeStarts:key          | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:os2UnicodeRangeStarts:value        |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @os2UnicodeRange                                           |                       |
|                                                                |                               | - name: os2UnicodeRangeStarts                                      |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:os2UnicodeRangeEnds                | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:os2UnicodeRangeEnds:key            | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:os2UnicodeRangeEnds:value          |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @os2UnicodeRange                                           |                       |
|                                                                |                               | - name: os2UnicodeRangeEnds                                        |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:os2UnicodeRangeBits                | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:os2UnicodeRangeBits:key            | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:os2UnicodeRangeBits:value          |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @os2UnicodeRange                                           |                       |
|                                                                |                               | - name: os2UnicodeRangeBits                                        |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:lineBreakAmbiguousClasses          | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:lineBreakAmbiguousClasses:key      | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:lineBreakAmbiguousClasses:value    |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @lineBreakAmbiguous                                        |                       |
|                                                                |                               | - name: lineBreakAmbiguousClasses                                  |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:lineBreakTailoringLanguages        | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:lineBreakTailoringLanguages:key    | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:lineBreakTailoringLanguages:value  |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @lineBreakTailoring                                        |                       |
|                                                                |                               | - name: lineBreakTailoringLanguages                                |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:lineBreakTailoringCodepoints       | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:lineBreakTailoringCodepoints:key   | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:lineBreakTailoringCodepoints:value |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @lineBreakTailoring                                        |                       |
|                                                                |                               | - name: lineBreakTailoringCodepoints                               |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:lineBreakTailoringClasses          | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:lineBreakTailoringClasses:key      | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:lineBreakTailoringClasses:value    |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @lineBreakTailoring                                        |                       |
|                                                                |                               | - name: lineBreakTailoringClasses                                  |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:lineBreakAdjacentMasks             | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:lineBreakAdjacentMasks:key         | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:lineBreakAdjacentMasks:value       |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @lineBreakMask                                             |                       |
|                                                                |                               | - name: lineBreakAdjacentMasks                                     |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @lookuptables:lineBreakSpacedMasks               | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_LookupTables     |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > - [list]: @lookuptables:lineBreakSpacedMasks:key           | > > - [list]: @code:comma     | - name: map                                                        |                       |
| > > - [list]: @lookuptables:lineBreakSpacedMasks:value         |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:lookup-table                                                 |                       |
|                                                                |                               | - type: @lineBreakMask                                             |                       |
|                                                                |                               | - name: lineBreakSpacedMasks                                       |                       |
|                                                                |                               | > > - [list]: @code:entry                                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > - [list]: @syntax:languageFamily                             | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_HashMaps         |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - generated:                                                       |                       |
|                                                                |                               | - name: map                                                        |                       |
|                                                                |                               | - [comment]: @headers:brief                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:hash-map                                                     |                       |
|                                                                |                               | - keyType: juce::String                                            |                       |
|                                                                |                               | - valueType: int                                                   |                       |
|                                                                |                               | - name: languageFamily                                             |                       |
|                                                                |                               | > - [list]: @code:map-entry                                        |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > - [list]: @text:romanNumerals                                | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_HashMaps         |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:hash-map                                                     |                       |
|                                                                |                               | - keyType: juce::String                                            |                       |
|                                                                |                               | - valueType: int                                                   |                       |
|                                                                |                               | - name: romanNumerals                                              |                       |
|                                                                |                               | > - [list]: @code:map-entry                                        |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > - [list]: @entities                                          | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_HashMaps         |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:hash-map                                                     |                       |
|                                                                |                               | - keyType: juce::String                                            |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | - name: entities                                                   |                       |
|                                                                |                               | > - [list]: @code:utf8-entry                                       |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > - [list]: @chars:Diacritics                                  | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_HashMaps         |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:hash-map                                                     |                       |
|                                                                |                               | - keyType: juce::String                                            |                       |
|                                                                |                               | - valueType: juce::String                                          |                       |
|                                                                |                               | - name: diacritics                                                 |                       |
|                                                                |                               | > - [list]: @code:utf8-entry                                       |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > - [list]: @opentype:macRomanBytes                            | - [list]: @code:linebreak     | @code:namespace                                                    | @jam_HashMaps         |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: map                                                        |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | @code:hash-map                                                     |                       |
|                                                                |                               | - keyType: int                                                     |                       |
|                                                                |                               | - valueType: int                                                   |                       |
|                                                                |                               | - name: macRomanBytes                                              |                       |
|                                                                |                               | > - [list]: @code:map-entry                                        |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:DiagramType                         | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - generated:                                                       |                       |
| > > > - [list]: @mermaid:DiagramType                           |                               | - name: Mermaid                                                    |                       |
|                                                                |                               | - [comment]: @headers:brief                                        |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:bimap                                                      |                       |
|                                                                |                               | > - name: DiagramType                                              |                       |
|                                                                |                               | > - type: Mermaid::DiagramType                                     |                       |
|                                                                |                               | > - instance: diagramType                                          |                       |
|                                                                |                               | > - [comment]: Mermaid diagram kinds.                              |                       |
|                                                                |                               | > - base: @bimapIdentifier                                         |                       |
|                                                                |                               | > - keyType: int                                                   |                       |
|                                                                |                               | > - valueType: juce::Identifier                                    |                       |
|                                                                |                               | > > > > - [list]: @code:identifier-entry                           |                       |
|                                                                |                               | > > > - [list]: @code:enum-entry                                   |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:NodeShape                           | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @mermaid:NodeShape                             |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:bimap                                                      |                       |
|                                                                |                               | > - name: NodeShape                                                |                       |
|                                                                |                               | > - type: Mermaid::NodeShape                                       |                       |
|                                                                |                               | > - instance: nodeShape                                            |                       |
|                                                                |                               | > - [comment]: Mermaid node shapes.                                |                       |
|                                                                |                               | > - base: @bimap                                                   |                       |
|                                                                |                               | > - keyType: int                                                   |                       |
|                                                                |                               | > - valueType: juce::String                                        |                       |
|                                                                |                               | > > > > - [list]: @code:name-entry                                 |                       |
|                                                                |                               | > > > - [list]: @code:enum-entry                                   |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:Direction                           | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @mermaid:Direction                             |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:bimap                                                      |                       |
|                                                                |                               | > - name: Direction                                                |                       |
|                                                                |                               | > - type: Mermaid::Direction                                       |                       |
|                                                                |                               | > - instance: direction                                            |                       |
|                                                                |                               | > - [comment]: Mermaid direction keywords.                         |                       |
|                                                                |                               | > - base: @bimap                                                   |                       |
|                                                                |                               | > - keyType: int                                                   |                       |
|                                                                |                               | > - valueType: juce::String                                        |                       |
|                                                                |                               | > > > > - [list]: @code:bimap-entry                                |                       |
|                                                                |                               | > > > - [list]: @code:enum-entry                                   |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @mermaid:directionMasks                        | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | - name: Mermaid                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > @code:static-lookup-table                                        |                       |
|                                                                |                               | > - type: @directionMask                                           |                       |
|                                                                |                               | > - name: directionMasks                                           |                       |
|                                                                |                               | > > > - [list]: @code:map-entry                                    |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:EdgeStroke                          | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @mermaid:EdgeStroke                            |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:bimap                                                      |                       |
|                                                                |                               | > - name: EdgeStroke                                               |                       |
|                                                                |                               | > - type: Mermaid::EdgeStroke                                      |                       |
|                                                                |                               | > - instance: edgeStroke                                           |                       |
|                                                                |                               | > - [comment]: Mermaid edge stroke styles.                         |                       |
|                                                                |                               | > - base: @bimap                                                   |                       |
|                                                                |                               | > - keyType: int                                                   |                       |
|                                                                |                               | > - valueType: juce::String                                        |                       |
|                                                                |                               | > > > > - [list]: @code:name-entry                                 |                       |
|                                                                |                               | > > > - [list]: @code:enum-entry                                   |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:EdgeDecoration                      | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @mermaid:EdgeDecoration                        |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:bimap                                                      |                       |
|                                                                |                               | > - name: EdgeDecoration                                           |                       |
|                                                                |                               | > - type: Mermaid::EdgeDecoration                                  |                       |
|                                                                |                               | > - instance: edgeDecoration                                       |                       |
|                                                                |                               | > - [comment]: Mermaid edge decorations.                           |                       |
|                                                                |                               | > - base: @bimap                                                   |                       |
|                                                                |                               | > - keyType: int                                                   |                       |
|                                                                |                               | > - valueType: juce::String                                        |                       |
|                                                                |                               | > > > > - [list]: @code:name-entry                                 |                       |
|                                                                |                               | > > > - [list]: @code:enum-entry                                   |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:MarkerPaint                         | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @mermaid:MarkerPaint                           |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:bimap                                                      |                       |
|                                                                |                               | > - name: MarkerPaint                                              |                       |
|                                                                |                               | > - type: Mermaid::MarkerPaint                                     |                       |
|                                                                |                               | > - instance: markerPaint                                          |                       |
|                                                                |                               | > - [comment]: Mermaid marker paints.                              |                       |
|                                                                |                               | > - base: @bimap                                                   |                       |
|                                                                |                               | > - keyType: int                                                   |                       |
|                                                                |                               | > - valueType: juce::String                                        |                       |
|                                                                |                               | > > > > - [list]: @code:name-entry                                 |                       |
|                                                                |                               | > > > - [list]: @code:enum-entry                                   |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:StyleSheet                          | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @mermaid:StyleSheet                            |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:bimap                                                      |                       |
|                                                                |                               | > - name: StyleSheet                                               |                       |
|                                                                |                               | > - type: Mermaid::StyleSheet                                      |                       |
|                                                                |                               | > - instance: mermaidStyleSheet                                    |                       |
|                                                                |                               | > - [comment]: Mermaid stylesheet keys.                            |                       |
|                                                                |                               | > - base: @bimapIdentifier                                         |                       |
|                                                                |                               | > - keyType: int                                                   |                       |
|                                                                |                               | > - valueType: juce::Identifier                                    |                       |
|                                                                |                               | > > > > - [list]: @code:identifier-entry                           |                       |
|                                                                |                               | > > > - [list]: @code:enum-entry                                   |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:Operator                            | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @mermaid:Operator                              |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:bimap                                                      |                       |
|                                                                |                               | > - name: Operator                                                 |                       |
|                                                                |                               | > - type: Mermaid::Operator                                        |                       |
|                                                                |                               | > - instance: mermaidOperator                                      |                       |
|                                                                |                               | > - [comment]: Mermaid operators.                                  |                       |
|                                                                |                               | > - base: @bimap                                                   |                       |
|                                                                |                               | > - keyType: int                                                   |                       |
|                                                                |                               | > - valueType: juce::String                                        |                       |
|                                                                |                               | > > > > - [list]: @code:bimap-entry                                |                       |
|                                                                |                               | > > > - [list]: @code:enum-entry                                   |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:TokenType                           | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @mermaid:TokenType                             |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:bimap                                                      |                       |
|                                                                |                               | > - name: TokenType                                                |                       |
|                                                                |                               | > - type: Mermaid::TokenType                                       |                       |
|                                                                |                               | > - instance: mermaidTokenType                                     |                       |
|                                                                |                               | > - [comment]: Mermaid token kinds.                                |                       |
|                                                                |                               | > - base: @bimap                                                   |                       |
|                                                                |                               | > - keyType: int                                                   |                       |
|                                                                |                               | > - valueType: juce::String                                        |                       |
|                                                                |                               | > > > > - [list]: @code:name-entry                                 |                       |
|                                                                |                               | > > > - [list]: @code:enum-entry                                   |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @mermaid:characters                            | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @mermaid:characters:key                        | > > > - [list]: @code:comma   | - name: Mermaid                                                    |                       |
| > > > - [list]: @mermaid:characters:value                      |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:static-lookup-table                                        |                       |
|                                                                |                               | > - type: @byteClass                                               |                       |
|                                                                |                               | > - name: characters                                               |                       |
|                                                                |                               | > > > - [list]: @code:entry                                        |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @mermaid:blockLinks                            | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @mermaid:blockLinks:first                      | > > > - [list]: @code:comma   | - name: Mermaid                                                    |                       |
| > > > - [list]: @mermaid:blockLinks:second                     |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:static-lookup-table                                        |                       |
|                                                                |                               | > - type: @blockLink                                               |                       |
|                                                                |                               | > - name: blockLinks                                               |                       |
|                                                                |                               | > > > - [list]: @code:union-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @mermaid:flowchartLinks                        | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @mermaid:flowchartLinks:first                  | > > > - [list]: @code:comma   | - name: Mermaid                                                    |                       |
| > > > - [list]: @mermaid:flowchartLinks:second                 |                               |                                                                    |                       |
| > > > - [list]: @mermaid:flowchartLinks:third                  |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:static-lookup-table                                        |                       |
|                                                                |                               | > - type: @flowchartLink                                           |                       |
|                                                                |                               | > - name: flowchartLinks                                           |                       |
|                                                                |                               | > > > - [list]: @code:union-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:GroupType                           | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @mermaid:GroupType                             |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:bimap                                                      |                       |
|                                                                |                               | > - name: GroupType                                                |                       |
|                                                                |                               | > - type: Mermaid::GroupType                                       |                       |
|                                                                |                               | > - instance: groupType                                            |                       |
|                                                                |                               | > - [comment]: Mermaid group kinds across diagram families.        |                       |
|                                                                |                               | > - base: @bimap                                                   |                       |
|                                                                |                               | > - keyType: int                                                   |                       |
|                                                                |                               | > - valueType: juce::String                                        |                       |
|                                                                |                               | > > > > - [list]: @code:name-entry                                 |                       |
|                                                                |                               | > > > - [list]: @code:enum-entry                                   |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:GroupLayoutPolicy                   | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @mermaid:GroupLayoutPolicy                     |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:bimap                                                      |                       |
|                                                                |                               | > - name: GroupLayoutPolicy                                        |                       |
|                                                                |                               | > - type: Mermaid::GroupLayoutPolicy                               |                       |
|                                                                |                               | > - instance: groupLayoutPolicy                                    |                       |
|                                                                |                               | > - [comment]: Mermaid group layout-ownership policies.            |                       |
|                                                                |                               | > - base: @bimap                                                   |                       |
|                                                                |                               | > - keyType: int                                                   |                       |
|                                                                |                               | > - valueType: juce::String                                        |                       |
|                                                                |                               | > > > > - [list]: @code:name-entry                                 |                       |
|                                                                |                               | > > > - [list]: @code:enum-entry                                   |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:ConstructionPolicy                | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:ConstructionPolicy                  |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Shape                                  |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | > @code:shape                                                      |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: ConstructionPolicy                                     |                       |
|                                                                |                               | > > - type: Mermaid::Shape::ConstructionPolicy                     |                       |
|                                                                |                               | > > - instance: shapeConstructionPolicy                            |                       |
|                                                                |                               | > > - [comment]: Mermaid node-shape outline construction policies. |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:name-entry                               |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:HitTestClassification             | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:HitTestClassification               |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Shape                                  |                               | > @code:shape                                                      |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: HitTestClassification                                  |                       |
|                                                                |                               | > > - type: Mermaid::Shape::HitTestClassification                  |                       |
|                                                                |                               | > > - instance: shapeHitTestClassification                         |                       |
|                                                                |                               | > > - [comment]: Mermaid node-shape hit-test predicates.           |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:name-entry                               |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:SizeAdjustPolicy                  | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:SizeAdjustPolicy                    |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Shape                                  |                               | > @code:shape                                                      |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: SizeAdjustPolicy                                       |                       |
|                                                                |                               | > > - type: Mermaid::Shape::SizeAdjustPolicy                       |                       |
|                                                                |                               | > > - instance: shapeSizeAdjustPolicy                              |                       |
|                                                                |                               | > > - [comment]: Mermaid node-shape size-adjust policies.          |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:name-entry                               |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:MindmapSizePolicy                 | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:MindmapSizePolicy                   |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Shape                                  |                               | > @code:shape                                                      |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: MindmapSizePolicy                                      |                       |
|                                                                |                               | > > - type: Mermaid::Shape::MindmapSizePolicy                      |                       |
|                                                                |                               | > > - instance: shapeMindmapSizePolicy                             |                       |
|                                                                |                               | > > - [comment]: Mermaid mindmap node-size policies.               |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:name-entry                               |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:BlockSizePolicy                   | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:BlockSizePolicy                     |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Shape                                  |                               | > @code:shape                                                      |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: BlockSizePolicy                                        |                       |
|                                                                |                               | > > - type: Mermaid::Shape::BlockSizePolicy                        |                       |
|                                                                |                               | > > - instance: shapeBlockSizePolicy                               |                       |
|                                                                |                               | > > - [comment]: Mermaid block-diagram node-size policies.         |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:name-entry                               |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:shapes                              | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:shapes:first                        | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:shapes:second                       |                               |                                                                    |                       |
| > > > > - [list]: @mermaid:shapes:third                        |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Shape                                  |                               | > @code:shape                                                      |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-value-lookup-table                                |                       |
|                                                                |                               | > > - type: @shape                                                 |                       |
|                                                                |                               | > > - name: shapes                                                 |                       |
|                                                                |                               | > > > > - [list]: @code:value-union-entry                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:cornerRadii                         | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:cornerRadii:key                     | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:cornerRadii:value                   |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Shape                                  |                               | > @code:shape                                                      |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @cornerRadius                                          |                       |
|                                                                |                               | > > - name: cornerRadii                                            |                       |
|                                                                |                               | > > > > - [list]: @code:entry                                      |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:shapePathIds                        | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:shapePathIds:key                    | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:shapePathIds:value                  |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Shape                                  |                               | > @code:shape                                                      |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @shapePathId                                           |                       |
|                                                                |                               | > > - name: pathIds                                                |                       |
|                                                                |                               | > > > > - [list]: @code:entry                                      |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:paddings                            | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:paddings:first                      | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:paddings:second                     |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Shape                                  |                               | > @code:shape                                                      |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @padding                                               |                       |
|                                                                |                               | > > - name: paddings                                               |                       |
|                                                                |                               | > > > > - [list]: @code:union-entry                                |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:geometry                            | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Shape                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:shape                                                      |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @geometry                                              |                       |
|                                                                |                               | > > - name: geometry                                               |                       |
|                                                                |                               | > > > > - [list]: @code:geometry-entry                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:layout                              | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:layout:key                          | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:layout:value                        |                               |                                                                    |                       |
| > - [comment]: @mermaid:Group                                  |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Group                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @groupLayout                                           |                       |
|                                                                |                               | > > - name: layout                                                 |                       |
|                                                                |                               | > > > > - [list]: @code:entry                                      |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > - [list]: @mermaid:svgPaths                              | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | - name: Mermaid                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > @code:static-lookup-table                                        |                       |
|                                                                |                               | > - type: @svgPath                                                 |                       |
|                                                                |                               | > - name: svgPaths                                                 |                       |
|                                                                |                               | > > > - [list]: @code:map-entry                                    |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:markerPathIds                       | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > - [comment]: @mermaid:Marker                                 |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Marker                                                   |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @markerPathId                                          |                       |
|                                                                |                               | > > - name: pathIds                                                |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:markerAnchors                       | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:markerAnchors:first                 | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:markerAnchors:second                |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Marker                                 |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Marker                                                   |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @markerAnchor                                          |                       |
|                                                                |                               | > > - name: anchors                                                |                       |
|                                                                |                               | > > > > - [list]: @code:union-entry                                |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:markerStrokes                       | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:markerStrokes:paint                 | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:markerStrokes:width                 |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Marker                                 |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Marker                                                   |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @markerStroke                                          |                       |
|                                                                |                               | > > - name: strokes                                                |                       |
|                                                                |                               | > > > > - [list]: @code:union-entry                                |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:decorationGeometry                  | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:decorationGeometry:shape            | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:decorationGeometry:scale            |                               |                                                                    |                       |
| > > > > - [list]: @mermaid:decorationGeometry:filled           |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Marker                                 |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Marker                                                   |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @decorationGeometry                                    |                       |
|                                                                |                               | > > - name: decorationGeometry                                     |                       |
|                                                                |                               | > > > > - [list]: @code:union-entry                                |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:arrowheadPathIds                    | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > - [comment]: @mermaid:Arrowhead                              |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Arrowhead                                                |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @arrowheadPathId                                       |                       |
|                                                                |                               | > > - name: pathIds                                                |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:arrowheadAnchors                    | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:arrowheadAnchors:first              | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:arrowheadAnchors:second             |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Arrowhead                              |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Arrowhead                                                |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @arrowheadAnchor                                       |                       |
|                                                                |                               | > > - name: anchors                                                |                       |
|                                                                |                               | > > > > - [list]: @code:union-entry                                |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:C4Keyword                         | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:C4Keyword                           |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:C4                                     |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: C4                                                       |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Keyword                                                |                       |
|                                                                |                               | > > - type: Mermaid::C4::Keyword                                   |                       |
|                                                                |                               | > > - instance: C4Keyword                                          |                       |
|                                                                |                               | > > - [comment]: Mermaid C4 element/relation/boundary keywords.    |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:bimap-entry                              |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:c4Roles                             | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:C4                                     |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: C4                                                       |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @c4Role                                                |                       |
|                                                                |                               | > > - name: roles                                                  |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:c4Traits                            | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:c4Traits:first                      | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:c4Traits:second                     |                               |                                                                    |                       |
| > > > > - [list]: @mermaid:c4Traits:third                      |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:C4                                     |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: C4                                                       |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-value-lookup-table                                |                       |
|                                                                |                               | > > - type: @c4Trait                                               |                       |
|                                                                |                               | > > - name: traits                                                 |                       |
|                                                                |                               | > > > > - [list]: @code:value-union-entry                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:c4Boundaries                        | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:c4Boundaries:first                  | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:c4Boundaries:second                 |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:C4                                     |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: C4                                                       |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @c4Boundary                                            |                       |
|                                                                |                               | > > - name: boundaries                                             |                       |
|                                                                |                               | > > > > - [list]: @code:union-entry                                |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:c4Stereotypes                       | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:C4                                     |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: C4                                                       |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-value-lookup-table                                |                       |
|                                                                |                               | > > - type: @c4Stereotype                                          |                       |
|                                                                |                               | > > - name: stereotypes                                            |                       |
|                                                                |                               | > > > > - [list]: @code:value-entry                                |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:c4Relations                         | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:c4Relations:first                   | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:c4Relations:second                  |                               |                                                                    |                       |
| > > > > - [list]: @mermaid:c4Relations:third                   |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:C4                                     |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: C4                                                       |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @c4Relation                                            |                       |
|                                                                |                               | > > - name: relations                                              |                       |
|                                                                |                               | > > > > - [list]: @code:union-entry                                |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:classRelations                      | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:classRelations:link                 | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:classRelations:sourceDecoration     |                               |                                                                    |                       |
| > > > > - [list]: @mermaid:classRelations:targetDecoration     |                               |                                                                    |                       |
| > > > > - [list]: @mermaid:classRelations:open                 |                               |                                                                    |                       |
| > - [comment]: @mermaid:Class                                  |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:class                                                      |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-value-lookup-table                                |                       |
|                                                                |                               | > > - type: @classRelation                                         |                       |
|                                                                |                               | > > - name: relations                                              |                       |
|                                                                |                               | > > > > - [list]: @code:value-relation-entry                       |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:erCardinalities                     | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:erCardinalities:key                 | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:erCardinalities:value               |                               |                                                                    |                       |
| > - [comment]: @mermaid:ER                                     |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: ER                                                       |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @cardinality                                           |                       |
|                                                                |                               | > > - name: cardinalities                                          |                       |
|                                                                |                               | > > > > - [list]: @code:entry                                      |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:SequenceKeyword                   | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:SequenceKeyword                     |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Sequence                               |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Sequence                                                 |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Keyword                                                |                       |
|                                                                |                               | > > - type: Mermaid::Sequence::Keyword                             |                       |
|                                                                |                               | > > - instance: sequenceKeyword                                    |                       |
|                                                                |                               | > > - [comment]: Mermaid sequence frame keywords.                  |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:bimap-entry                              |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:sequenceArrows                      | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:sequenceArrows:symbol               | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:sequenceArrows:stroke               |                               |                                                                    |                       |
| > > > > - [list]: @mermaid:sequenceArrows:reverse              |                               |                                                                    |                       |
| > > > > - [list]: @mermaid:sequenceArrows:decoration           |                               |                                                                    |                       |
| > - [comment]: @mermaid:Sequence                               |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Sequence                                                 |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-value-lookup-table                                |                       |
|                                                                |                               | > > - type: @sequenceArrow                                         |                       |
|                                                                |                               | > > - name: arrows                                                 |                       |
|                                                                |                               | > > > > - [list]: @code:value-union-entry                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:stateStereotypes                    | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:stateStereotypes:first              | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:stateStereotypes:second             |                               |                                                                    |                       |
| > - [comment]: @mermaid:State                                  |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: State                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-value-lookup-table                                |                       |
|                                                                |                               | > > - type: @stateStereotype                                       |                       |
|                                                                |                               | > > - name: stereotypes                                            |                       |
|                                                                |                               | > > > > - [list]: @code:value-union-entry                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:quadrantCorners                     | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:quadrantCorners:first               | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:quadrantCorners:second              |                               |                                                                    |                       |
| > - [comment]: @mermaid:Quadrant                               |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Quadrant                                                 |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-value-lookup-table                                |                       |
|                                                                |                               | > > - type: @quadrantCorner                                        |                       |
|                                                                |                               | > > - name: corners                                                |                       |
|                                                                |                               | > > > > - [list]: @code:value-union-entry                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:GitKeyword                        | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:GitKeyword                          |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Git                                    |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Git                                                      |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Keyword                                                |                       |
|                                                                |                               | > > - type: Mermaid::Git::Keyword                                  |                       |
|                                                                |                               | > > - instance: gitKeyword                                         |                       |
|                                                                |                               | > > - [comment]: Mermaid git commit kinds.                         |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:bimap-entry                              |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:gitRoles                            | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Git                                    |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Git                                                      |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @gitRole                                               |                       |
|                                                                |                               | > > - name: roles                                                  |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:GanttKeyword                      | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:GanttKeyword                        |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Gantt                                  |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Gantt                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Keyword                                                |                       |
|                                                                |                               | > > - type: Mermaid::Gantt::Keyword                                |                       |
|                                                                |                               | > > - instance: ganttKeyword                                       |                       |
|                                                                |                               | > > - [comment]: Mermaid gantt task status and directive keywords. |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:name-entry                               |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:ganttRoles                          | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Gantt                                  |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Gantt                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @ganttRole                                             |                       |
|                                                                |                               | > > - name: roles                                                  |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:ganttRoles                          | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Gantt                                  |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Gantt                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @ganttRole                                             |                       |
|                                                                |                               | > > - name: roles                                                  |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:ganttDurationUnits                  | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:ganttDurationUnits:key              | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:ganttDurationUnits:value            |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Gantt                                  |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Gantt                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @durationUnit                                          |                       |
|                                                                |                               | > > - name: durationUnits                                          |                       |
|                                                                |                               | > > > > - [list]: @code:entry                                      |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:ganttStatusColourIds                | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:ganttStatusColourIds:key            | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:ganttStatusColourIds:value          |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Gantt                                  |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Gantt                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @statusColourId                                        |                       |
|                                                                |                               | > > - name: statusColourIds                                        |                       |
|                                                                |                               | > > > > - [list]: @code:entry                                      |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:ganttSectionBandColourIds           | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
|                                                                |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Gantt                                  |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Gantt                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-value-lookup-table                                |                       |
|                                                                |                               | > > - type: @sectionColourId                                       |                       |
|                                                                |                               | > > - name: sectionBandColourIds                                   |                       |
|                                                                |                               | > > > > - [list]: @code:value-entry                                |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:ganttAxisTickDays                   | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
|                                                                |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Gantt                                  |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Gantt                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-value-lookup-table                                |                       |
|                                                                |                               | > > - type: @axisTickDay                                           |                       |
|                                                                |                               | > > - name: axisTickDays                                           |                       |
|                                                                |                               | > > > > - [list]: @code:value-entry                                |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @mermaid:ganttConstants                          | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Gantt                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Gantt                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > - [list]: @code:constant                                       |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:KanbanKeyword                     | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:KanbanKeyword                       |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Kanban                                 |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Kanban                                                   |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Keyword                                                |                       |
|                                                                |                               | > > - type: Mermaid::Kanban::Keyword                               |                       |
|                                                                |                               | > > - instance: kanbanKeyword                                      |                       |
|                                                                |                               | > > - [comment]: Mermaid kanban priorities.                        |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:bimap-entry                              |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:kanbanPriorityColourIds             | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:kanbanPriorityColourIds:key         | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:kanbanPriorityColourIds:value       |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Kanban                                 |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Kanban                                                   |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @priorityColourId                                      |                       |
|                                                                |                               | > > - name: priorityColourIds                                      |                       |
|                                                                |                               | > > > > - [list]: @code:entry                                      |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @mermaid:journeyConstants                        | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Journey                                |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Journey                                                  |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > - [list]: @code:constant                                       |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:mindmapShapes                       | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:mindmapShapes:first                 | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:mindmapShapes:second                |                               |                                                                    |                       |
| > > > > - [list]: @mermaid:mindmapShapes:third                 |                               |                                                                    |                       |
| > - [comment]: @mermaid:Mindmap                                |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Mindmap                                                  |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-value-lookup-table                                |                       |
|                                                                |                               | > > - type: @mindmapShape                                          |                       |
|                                                                |                               | > > - name: shapes                                                 |                       |
|                                                                |                               | > > > > - [list]: @code:value-union-entry                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:ArchitectureKeyword               | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:ArchitectureKeyword                 |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Architecture                           |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Architecture                                             |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Keyword                                                |                       |
|                                                                |                               | > > - type: Mermaid::Architecture::Keyword                         |                       |
|                                                                |                               | > > - instance: architectureKeyword                                |                       |
|                                                                |                               | > > - [comment]: Mermaid architecture keywords.                    |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:bimap-entry                              |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:IconKeyword                       | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:IconKeyword                         |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Icon                                   |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Icon                                                     |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Keyword                                                |                       |
|                                                                |                               | > > - type: Mermaid::Icon::Keyword                                 |                       |
|                                                                |                               | > > - instance: iconKeyword                                        |                       |
|                                                                |                               | > > - [comment]: Mermaid label icon keywords.                      |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:bimap-entry                              |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:architectureRoles                   | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Architecture                           |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Architecture                                             |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @architectureRole                                      |                       |
|                                                                |                               | > > - name: roles                                                  |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:architectureAxes                    | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Architecture                           |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Architecture                                             |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @architectureAxis                                      |                       |
|                                                                |                               | > > - name: axes                                                   |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:architectureLinks                   | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:architectureLinks:first             | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:architectureLinks:second            |                               |                                                                    |                       |
| > > > > - [list]: @mermaid:architectureLinks:third             |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Architecture                           |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Architecture                                             |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-value-lookup-table                                |                       |
|                                                                |                               | > > - type: @architectureLink                                      |                       |
|                                                                |                               | > > - name: links                                                  |                       |
|                                                                |                               | > > > > - [list]: @code:value-union-entry                          |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:architectureAnchors                 | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:architectureAnchors:first           | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:architectureAnchors:second          |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Architecture                           |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Architecture                                             |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @architectureAnchor                                    |                       |
|                                                                |                               | > > - name: anchors                                                |                       |
|                                                                |                               | > > > > - [list]: @code:union-entry                                |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:architecturePathIds                 | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Architecture                           |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Architecture                                             |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @architecturePathId                                    |                       |
|                                                                |                               | > > - name: pathIds                                                |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:iconPathIds                         | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Icon                                   |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Icon                                                     |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @iconPathId                                            |                       |
|                                                                |                               | > > - name: pathIds                                                |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:XyKeyword                         | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:XyKeyword                           |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Xy                                     |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Xy                                                       |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Keyword                                                |                       |
|                                                                |                               | > > - type: Mermaid::Xy::Keyword                                   |                       |
|                                                                |                               | > > - instance: xyKeyword                                          |                       |
|                                                                |                               | > > - [comment]: Mermaid XY-chart series kinds.                    |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:name-entry                               |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:QuadrantKeyword                   | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:QuadrantKeyword                     |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Quadrant                               |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Quadrant                                                 |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Keyword                                                |                       |
|                                                                |                               | > > - type: Mermaid::Quadrant::Keyword                             |                       |
|                                                                |                               | > > - instance: quadrantKeyword                                    |                       |
|                                                                |                               | > > - [comment]: Mermaid quadrant label keywords.                  |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:bimap-entry                              |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:VennKeyword                       | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:VennKeyword                         |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Venn                                   |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Venn                                                     |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Keyword                                                |                       |
|                                                                |                               | > > - type: Mermaid::Venn::Keyword                                 |                       |
|                                                                |                               | > > - instance: vennKeyword                                        |                       |
|                                                                |                               | > > - [comment]: Mermaid venn statement keywords.                  |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:bimap-entry                              |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:RadarKeyword                      | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:RadarKeyword                        |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Radar                                  |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Radar                                                    |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Keyword                                                |                       |
|                                                                |                               | > > - type: Mermaid::Radar::Keyword                                |                       |
|                                                                |                               | > > - instance: radarKeyword                                       |                       |
|                                                                |                               | > > - [comment]: Mermaid radar statement keywords.                 |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:bimap-entry                              |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:Info                                | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @mermaid:Info                                  |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:bimap                                                      |                       |
|                                                                |                               | > - name: Info                                                     |                       |
|                                                                |                               | > - type: Mermaid::Info                                            |                       |
|                                                                |                               | > - instance: info                                                 |                       |
|                                                                |                               | > - [comment]: Mermaid pinned renderer version.                    |                       |
|                                                                |                               | > - base: @bimap                                                   |                       |
|                                                                |                               | > - keyType: int                                                   |                       |
|                                                                |                               | > - valueType: juce::String                                        |                       |
|                                                                |                               | > > > > - [list]: @code:bimap-entry                                |                       |
|                                                                |                               | > > > - [list]: @code:enum-entry                                   |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:WardleyAxis                         | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > - [list]: @mermaid:WardleyAxis                           |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:bimap                                                      |                       |
|                                                                |                               | > - name: WardleyAxis                                              |                       |
|                                                                |                               | > - type: Mermaid::WardleyAxis                                     |                       |
|                                                                |                               | > - instance: wardleyAxis                                          |                       |
|                                                                |                               | > - [comment]: Mermaid wardley axis titles.                        |                       |
|                                                                |                               | > - base: @bimap                                                   |                       |
|                                                                |                               | > - keyType: int                                                   |                       |
|                                                                |                               | > - valueType: juce::String                                        |                       |
|                                                                |                               | > > > > - [list]: @code:bimap-entry                                |                       |
|                                                                |                               | > > > - [list]: @code:enum-entry                                   |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:CynefinKeyword                    | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:CynefinKeyword                      |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Cynefin                                |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Cynefin                                                  |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Keyword                                                |                       |
|                                                                |                               | > > - type: Mermaid::Cynefin::Keyword                              |                       |
|                                                                |                               | > > - instance: cynefinKeyword                                     |                       |
|                                                                |                               | > > - [comment]: Mermaid cynefin domain keywords.                  |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:bimap-entry                              |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:cynefinDecisionModels               | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Cynefin                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Cynefin                                                  |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @cynefinDecisionModel                                  |                       |
|                                                                |                               | > > - name: decisionModels                                         |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:cynefinPractices                    | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Cynefin                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Cynefin                                                  |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @cynefinPractice                                       |                       |
|                                                                |                               | > > - name: practices                                              |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:TreeViewNodeType                  | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:TreeViewNodeType                    |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:TreeView                               |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: TreeView                                                 |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: NodeType                                               |                       |
|                                                                |                               | > > - type: Mermaid::TreeView::NodeType                            |                       |
|                                                                |                               | > > - instance: treeViewNodeType                                   |                       |
|                                                                |                               | > > - [comment]: Mermaid treeView row concept ordinals.            |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:name-entry                               |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:TreeViewClass                     | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:TreeViewClass                       |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:TreeView                               |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: TreeView                                                 |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Class                                                  |                       |
|                                                                |                               | > > - type: Mermaid::TreeView::Class                               |                       |
|                                                                |                               | > > - instance: treeViewClass                                      |                       |
|                                                                |                               | > > - [comment]: Mermaid treeView class-annotation keywords.       |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:bimap-entry                              |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:WardleyKeyword                    | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:WardleyKeyword                      |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Wardley                                |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Wardley                                                  |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Keyword                                                |                       |
|                                                                |                               | > > - type: Mermaid::Wardley::Keyword                              |                       |
|                                                                |                               | > > - instance: wardleyKeyword                                     |                       |
|                                                                |                               | > > - [comment]: Mermaid wardley statement keywords.               |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:bimap-entry                              |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:WardleyStrategy                   | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:WardleyStrategy                     |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Wardley                                |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Wardley                                                  |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Strategy                                               |                       |
|                                                                |                               | > > - type: Mermaid::Wardley::Strategy                             |                       |
|                                                                |                               | > > - instance: wardleyStrategy                                    |                       |
|                                                                |                               | > > - [comment]: Mermaid wardley sourcing strategies.              |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:bimap-entry                              |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:RequirementKeyword                | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:RequirementKeyword                  |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Requirement                            |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Requirement                                              |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Keyword                                                |                       |
|                                                                |                               | > > - type: Mermaid::Requirement::Keyword                          |                       |
|                                                                |                               | > > - instance: requirementKeyword                                 |                       |
|                                                                |                               | > > - [comment]: Mermaid requirement keywords.                     |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:name-entry                               |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:requirementRoles                    | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Requirement                            |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Requirement                                              |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @requirementRole                                       |                       |
|                                                                |                               | > > - name: roles                                                  |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:stereotypes                         | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Requirement                            |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Requirement                                              |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @requirementStereotype                                 |                       |
|                                                                |                               | > > - name: stereotypes                                            |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:attributeLabels                     | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Requirement                            |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Requirement                                              |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @attributeLabel                                        |                       |
|                                                                |                               | > > - name: attributeLabels                                        |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:TermType                          | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:TermType                            |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Railroad                               |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Railroad                                                 |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: TermType                                               |                       |
|                                                                |                               | > > - type: Mermaid::Railroad::TermType                            |                       |
|                                                                |                               | > > - instance: railroadTermType                                   |                       |
|                                                                |                               | > > - [comment]: Mermaid railroad diagram term kinds.              |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:name-entry                               |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:railroadLeafFillColourIds           | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:railroadLeafFillColourIds:key       | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:railroadLeafFillColourIds:value     |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Railroad                               |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Railroad                                                 |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @railroadLeafFillColourId                              |                       |
|                                                                |                               | > > - name: leafFillColourIds                                      |                       |
|                                                                |                               | > > > > - [list]: @code:entry                                      |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:railroadLeafBorderColourIds         | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:railroadLeafBorderColourIds:key     | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:railroadLeafBorderColourIds:value   |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Railroad                               |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Railroad                                                 |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @railroadLeafBorderColourId                            |                       |
|                                                                |                               | > > - name: leafBorderColourIds                                    |                       |
|                                                                |                               | > > > > - [list]: @code:entry                                      |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:railroadLeafTextColourIds           | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:railroadLeafTextColourIds:key       | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:railroadLeafTextColourIds:value     |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
| > - [comment]: @mermaid:Railroad                               |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Railroad                                                 |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @railroadLeafTextColourId                              |                       |
|                                                                |                               | > > - name: leafTextColourIds                                      |                       |
|                                                                |                               | > > > > - [list]: @code:entry                                      |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > - [list]: @mermaid:railroadConstants                       | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | - name: Mermaid                                                    |                       |
| > - [comment]: @mermaid:Railroad                               |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: Railroad                                                 |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > - [list]: @code:constant                                       |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > > - [list]: @mermaid:EventModelingKeyword              | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:EventModelingKeyword                |                               | - name: Mermaid                                                    |                       |
|                                                                |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: EventModeling                                            |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:bimap                                                    |                       |
|                                                                |                               | > > - name: Keyword                                                |                       |
|                                                                |                               | > > - type: Mermaid::EventModeling::Keyword                        |                       |
|                                                                |                               | > > - instance: eventModelingKeyword                               |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > - base: @bimap                                                 |                       |
|                                                                |                               | > > - keyType: int                                                 |                       |
|                                                                |                               | > > - valueType: juce::String                                      |                       |
|                                                                |                               | > > > > > - [list]: @code:name-entry                               |                       |
|                                                                |                               | > > > > - [list]: @code:enum-entry                                 |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:swimlanes                           | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: EventModeling                                            |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @eventModelingSwimlane                                 |                       |
|                                                                |                               | > > - name: swimlanes                                              |                       |
|                                                                |                               | > > > > - [list]: @code:map-entry                                  |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:swimlaneLabels                      | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: EventModeling                                            |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-value-lookup-table                                |                       |
|                                                                |                               | > > - type: @eventModelingSwimlaneLabel                            |                       |
|                                                                |                               | > > - name: swimlaneLabels                                         |                       |
|                                                                |                               | > > > > - [list]: @code:value-entry                                |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:swimlaneLabelPrefixes               | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
|                                                                |                               | - name: Mermaid                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: EventModeling                                            |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-value-lookup-table                                |                       |
|                                                                |                               | > > - type: @eventModelingSwimlaneLabel                            |                       |
|                                                                |                               | > > - name: swimlaneLabelPrefixes                                  |                       |
|                                                                |                               | > > > > - [list]: @code:value-entry                                |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:eventModelingFillColourIds          | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:eventModelingFillColourIds:key      | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:eventModelingFillColourIds:value    |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: EventModeling                                            |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @eventModelingColourId                                 |                       |
|                                                                |                               | > > - name: fillColourIds                                          |                       |
|                                                                |                               | > > > > - [list]: @code:entry                                      |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| > > > > - [list]: @mermaid:eventModelingBorderColourIds        | - [list]: @code:linebreak     | @code:struct                                                       | @jam_Mermaid          |
|                                                                |                               | - macro: #pragma once                                              |                       |
| > > > > - [list]: @mermaid:eventModelingBorderColourIds:key    | > > > > - [list]: @code:comma | - name: Mermaid                                                    |                       |
| > > > > - [list]: @mermaid:eventModelingBorderColourIds:value  |                               |                                                                    |                       |
| - [comment]: @mermaid:Mermaid                                  |                               |                                                                    |                       |
|                                                                |                               | > @code:struct                                                     |                       |
|                                                                |                               | > - name: EventModeling                                            |                       |
|                                                                |                               |                                                                    |                       |
|                                                                |                               | > > @code:static-lookup-table                                      |                       |
|                                                                |                               | > > - type: @eventModelingColourId                                 |                       |
|                                                                |                               | > > - name: borderColourIds                                        |                       |
|                                                                |                               | > > > > - [list]: @code:entry                                      |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_animation        |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_animation                   |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_audio_devices    |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_audio_devices               |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_clap             |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_clap                        |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_core             |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_core                        |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_data_structures  |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_data_structures             |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_debug            |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_debug                       |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_document         |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_document                    |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_dsp              |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_dsp                         |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_freetype         |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_freetype                    |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_fuzzy            |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_fuzzy                       |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_graphics         |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_graphics                    |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_gui              |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_gui                         |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_markdown         |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_markdown                    |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_mermaid_diagram  |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_mermaid_diagram             |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_opentype         |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_opentype                    |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_plugin_bootstrap |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_plugin_bootstrap            |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_style            |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_style                       |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_subprocess       |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_subprocess                  |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_terminal         |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_terminal                    |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_vulkan           |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_vulkan                      |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_web              |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_web                         |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+
| - [list]: @modules:identity                                    |                               | @code:moduleDeclaration                                            | @jam_whelmed          |
| - [list]: @modules:framework                                   |                               | - [begin]: BEGIN_JUCE_MODULE_DECLARATION                           |                       |
| - [list]: @modules:module:name=jam_whelmed                     |                               | - [end]: END_JUCE_MODULE_DECLARATION                               |                       |
|                                                                |                               | - [list]: @code:module                                             |                       |
+----------------------------------------------------------------+-------------------------------+--------------------------------------------------------------------+-----------------------+

## output index

+----------------------+-----------+-----------------------------------+----------------+
| list                 | separator | structure                         | file           |
+======================+===========+===================================+================+
| - [list]: @headers   |           | @code:generated                   | @jam_Generated |
| > - [list]: instance |           | - macro: #pragma once             |                |
|                      |           | - [comment]: @headers:brief       |                |
|                      |           | - [list]: @code:include           |                |
|                      |           | > - [list]: @code:shared-instance |                |
+----------------------+-----------+-----------------------------------+----------------+
