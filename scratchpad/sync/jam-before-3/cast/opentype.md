```
████████████░░████████████░░████████████░░████████████░░
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░
████░░        ████░░  ████░░████░░            ████░░
████░░        ████████████░░████████████░░    ████░░
████░░        ████░░  ████░░        ████░░    ████░░
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░
████████████░░████░░  ████░░████████████░░    ████░░
```

## SfntTableTag

```
@brief sfnt table directory tags.

Maps each sfnt table this module reads or writes to its directory index and
its four-character tag string. `os2` is the identifier for the `OS/2` table —
the tag string keeps the slash the C++ identifier cannot carry. The key is
the table's position in the file the font builder assembles. The order is the
one the OpenType specification recommends for TrueType outlines.
```

+------+-----+--------+
| name | key | value  |
+======+=====+========+
| head | 0   | `head` |
| hhea | 1   | `hhea` |
| maxp | 2   | `maxp` |
| os2  | 3   | `OS/2` |
| hmtx | 4   | `hmtx` |
| cmap | 5   | `cmap` |
| loca | 6   | `loca` |
| glyf | 7   | `glyf` |
| name | 8   | `name` |
| post | 9   | `post` |
| GSUB | 10  | `GSUB` |
+------+-----+--------+

## CmapPlatformId

```
@brief cmap platform IDs.

Maps each cmap platform this module builds subtables for to its platform ID.
The key is the `platformID` field the cmap subtable directory stores per
subtable.
```

+---------+-----+
| name    | key |
+=========+=====+
| unicode | 0   |
| windows | 3   |
+---------+-----+

## CmapEncodingId

```
@brief cmap encoding IDs, platform-prefixed.

Maps each cmap encoding this module builds subtables for to its encoding ID.
Encoding IDs are only meaningful relative to a platform, so each name carries
its platform even though `windowsSymbol`, `windowsUnicodeBmp`, and
`windowsUnicodeFull` share the Windows platform's one encoding-ID space with
the Unicode-platform space of `unicodeBmp` and `unicodeFull`. The key is the
`encodingID` field the cmap subtable directory stores per subtable.
```

+--------------------+-----+
| name               | key |
+====================+=====+
| windowsSymbol      | 0   |
| windowsUnicodeBmp  | 1   |
| windowsUnicodeFull | 10  |
| unicodeBmp         | 3   |
| unicodeFull        | 6   |
+--------------------+-----+

## NameId

```
@brief `name` table nameID vocabulary.

Maps each `name` table entry this module writes to its nameID. nameID 15 is
reserved by the OpenType spec and carries no row. The key is the `nameID`
field the `name` table record stores per string.
```

+--------------------------------+-----+
| name                           | key |
+================================+=====+
| copyright                      | 0   |
| familyName                     | 1   |
| styleName                      | 2   |
| uniqueFontIdentifier           | 3   |
| fullName                       | 4   |
| version                        | 5   |
| psName                         | 6   |
| trademark                      | 7   |
| manufacturer                   | 8   |
| designer                       | 9   |
| description                    | 10  |
| vendorURL                      | 11  |
| designerURL                    | 12  |
| licenseDescription             | 13  |
| licenseInfoURL                 | 14  |
| typographicFamily              | 16  |
| typographicSubfamily           | 17  |
| compatibleFullName             | 18  |
| sampleText                     | 19  |
| postScriptCidFindfontName      | 20  |
| wwsFamilyName                  | 21  |
| wwsSubfamilyName               | 22  |
| lightBackgroundPalette         | 23  |
| darkBackgroundPalette          | 24  |
| variationsPostScriptNamePrefix | 25  |
+--------------------------------+-----+

## Os2FsSelection

```
@brief `OS/2.fsSelection` bit flags.

Maps each `OS/2.fsSelection` flag this module sets to its bit value. Bits 7
through 9 (`useTypoMetrics`, `wws`, `oblique`) apply only to `OS/2` version 4
and later. The key is the bit the font builder ORs into `fsSelection`.
```

+----------------+--------+
| name           | key    |
+================+========+
| italic         | 0x0001 |
| bold           | 0x0020 |
| regular        | 0x0040 |
| useTypoMetrics | 0x0080 |
| wws            | 0x0100 |
| oblique        | 0x0200 |
+----------------+--------+

## PanoseField

```
@brief PANOSE byte field positions.

Maps each PANOSE classification byte to its position in the ten-byte PANOSE
struct the `OS/2` table stores. The key is the byte offset the font builder
writes each classification value into.
```

+------------------+-----+
| name             | key |
+==================+=====+
| bFamilyType      | 0   |
| bSerifStyle      | 1   |
| bWeight          | 2   |
| bProportion      | 3   |
| bContrast        | 4   |
| bStrokeVariation | 5   |
| bArmStyle        | 6   |
| bLetterForm      | 7   |
| bMidline         | 8   |
| bXHeight         | 9   |
+------------------+-----+

## GsubLookupType

```
@brief GSUB LookupType constants.

Maps each GSUB lookup subtable type to its LookupType. GPOS numbers its own
lookup types 1-9 independently — this table covers GSUB only. The key is the
`LookupType` field the GSUB lookup list stores per lookup.
```

+-------------------------+-----+
| name                    | key |
+=========================+=====+
| singleSubst             | 1   |
| multipleSubst           | 2   |
| alternateSubst          | 3   |
| ligatureSubst           | 4   |
| contextSubst            | 5   |
| chainContextSubst       | 6   |
| extensionSubst          | 7   |
| reverseChainSingleSubst | 8   |
+-------------------------+-----+

## StandardGlyphName

```
@brief Macintosh standard glyph names this module emits.

Maps each standard glyph name the `post` table can reference by index to
that index. A glyph whose name is absent here gets a custom name index and
a Pascal string in the `post` table's name storage. The key is the glyph
name's position in the Macintosh standard order.
```

+------------------+-----+--------------------+
| name             | key | value              |
+==================+=====+====================+
| notdef           | 0   | `.notdef`          |
| null             | 1   | `.null`            |
| nonmarkingreturn | 2   | `nonmarkingreturn` |
| space            | 3   | `space`            |
+------------------+-----+--------------------+

## RibbiStyle

```
@brief RIBBI style names.

Maps each style name the `name` table can carry in the Windows family and
subfamily records without a typographic split to its position. A style
absent here forces the family name to absorb the style and the subfamily to
read `Regular`.
```

+------------+-----+---------------+
| name       | key | value         |
+============+=====+===============+
| regular    | 0   | `Regular`     |
| bold       | 1   | `Bold`        |
| italic     | 2   | `Italic`      |
| boldItalic | 3   | `Bold Italic` |
+------------+-----+---------------+

## macRomanBytes

```
@brief Mac Roman byte of each Unicode codepoint the encoding covers.

Maps every codepoint Mac Roman represents above ASCII to the byte that
encodes it. The `name` table writer looks a character up here and writes
the byte it finds. Codepoints below `0x80` encode as themselves and carry
no row.
```

+--------+-------+
| key    | value |
+========+=======+
| 0xc4   | 0x80  |
| 0xc5   | 0x81  |
| 0xc7   | 0x82  |
| 0xc9   | 0x83  |
| 0xd1   | 0x84  |
| 0xd6   | 0x85  |
| 0xdc   | 0x86  |
| 0xe1   | 0x87  |
| 0xe0   | 0x88  |
| 0xe2   | 0x89  |
| 0xe4   | 0x8a  |
| 0xe3   | 0x8b  |
| 0xe5   | 0x8c  |
| 0xe7   | 0x8d  |
| 0xe9   | 0x8e  |
| 0xe8   | 0x8f  |
| 0xea   | 0x90  |
| 0xeb   | 0x91  |
| 0xed   | 0x92  |
| 0xec   | 0x93  |
| 0xee   | 0x94  |
| 0xef   | 0x95  |
| 0xf1   | 0x96  |
| 0xf3   | 0x97  |
| 0xf2   | 0x98  |
| 0xf4   | 0x99  |
| 0xf6   | 0x9a  |
| 0xf5   | 0x9b  |
| 0xfa   | 0x9c  |
| 0xf9   | 0x9d  |
| 0xfb   | 0x9e  |
| 0xfc   | 0x9f  |
| 0x2020 | 0xa0  |
| 0xb0   | 0xa1  |
| 0xa2   | 0xa2  |
| 0xa3   | 0xa3  |
| 0xa7   | 0xa4  |
| 0x2022 | 0xa5  |
| 0xb6   | 0xa6  |
| 0xdf   | 0xa7  |
| 0xae   | 0xa8  |
| 0xa9   | 0xa9  |
| 0x2122 | 0xaa  |
| 0xb4   | 0xab  |
| 0xa8   | 0xac  |
| 0x2260 | 0xad  |
| 0xc6   | 0xae  |
| 0xd8   | 0xaf  |
| 0x221e | 0xb0  |
| 0xb1   | 0xb1  |
| 0x2264 | 0xb2  |
| 0x2265 | 0xb3  |
| 0xa5   | 0xb4  |
| 0xb5   | 0xb5  |
| 0x2202 | 0xb6  |
| 0x2211 | 0xb7  |
| 0x220f | 0xb8  |
| 0x3c0  | 0xb9  |
| 0x222b | 0xba  |
| 0xaa   | 0xbb  |
| 0xba   | 0xbc  |
| 0x3a9  | 0xbd  |
| 0xe6   | 0xbe  |
| 0xf8   | 0xbf  |
| 0xbf   | 0xc0  |
| 0xa1   | 0xc1  |
| 0xac   | 0xc2  |
| 0x221a | 0xc3  |
| 0x192  | 0xc4  |
| 0x2248 | 0xc5  |
| 0x2206 | 0xc6  |
| 0xab   | 0xc7  |
| 0xbb   | 0xc8  |
| 0x2026 | 0xc9  |
| 0xa0   | 0xca  |
| 0xc0   | 0xcb  |
| 0xc3   | 0xcc  |
| 0xd5   | 0xcd  |
| 0x152  | 0xce  |
| 0x153  | 0xcf  |
| 0x2013 | 0xd0  |
| 0x2014 | 0xd1  |
| 0x201c | 0xd2  |
| 0x201d | 0xd3  |
| 0x2018 | 0xd4  |
| 0x2019 | 0xd5  |
| 0xf7   | 0xd6  |
| 0x25ca | 0xd7  |
| 0xff   | 0xd8  |
| 0x178  | 0xd9  |
| 0x2044 | 0xda  |
| 0x20ac | 0xdb  |
| 0x2039 | 0xdc  |
| 0x203a | 0xdd  |
| 0xfb01 | 0xde  |
| 0xfb02 | 0xdf  |
| 0x2021 | 0xe0  |
| 0xb7   | 0xe1  |
| 0x201a | 0xe2  |
| 0x201e | 0xe3  |
| 0x2030 | 0xe4  |
| 0xc2   | 0xe5  |
| 0xca   | 0xe6  |
| 0xc1   | 0xe7  |
| 0xcb   | 0xe8  |
| 0xc8   | 0xe9  |
| 0xcd   | 0xea  |
| 0xce   | 0xeb  |
| 0xcf   | 0xec  |
| 0xcc   | 0xed  |
| 0xd3   | 0xee  |
| 0xd4   | 0xef  |
| 0xf8ff | 0xf0  |
| 0xd2   | 0xf1  |
| 0xda   | 0xf2  |
| 0xdb   | 0xf3  |
| 0xd9   | 0xf4  |
| 0x131  | 0xf5  |
| 0x2c6  | 0xf6  |
| 0x2dc  | 0xf7  |
| 0xaf   | 0xf8  |
| 0x2d8  | 0xf9  |
| 0x2d9  | 0xfa  |
| 0x2da  | 0xfb  |
| 0xb8   | 0xfc  |
| 0x2dd  | 0xfd  |
| 0x2db  | 0xfe  |
| 0x2c7  | 0xff  |
+--------+-------+
