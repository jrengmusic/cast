```
████████████░░████████████░░████████████░░████████████░░
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░
████░░        ████░░  ████░░████░░            ████░░
████░░        ████████████░░████████████░░    ████░░
████░░        ████░░  ████░░        ████░░    ████░░
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░
████████████░░████░░  ████░░████████████░░    ████░░
```

## chars

```
@brief Character codepoints, token literals, and the character relation maps.

Two groups of constants. The juce::juce_wchar constants name Unicode
codepoints the parsers branch on; the const char* constants name
multi-character token literals. Two maps and one predicate read those
constants: `escape` maps each control character to its named escape
letter, `enclosure` maps each opening character to the closing character
that pairs with it, and `isNumeric` answers whether a codepoint belongs in a numeric literal.
```

+-----------------------------+---------+----------+----------+
| name                        | format  | value    | comment  |
+=============================+=========+==========+==========+
| ampersand                   | toCamel | 0x26     | U+0026   |
| application program command | toCamel | 0x9f     | U+009F   |
| asterisk                    | toCamel | 0x2a     | U+002A   |
| at                          | toCamel | 0x40     | U+0040   |
| backslash                   | toCamel | 0x5c     | U+005C   |
| backspace                   | toCamel | 0x8      | U+0008   |
| backtick                    | toCamel | 0x60     | U+0060   |
| caret                       | toCamel | 0x5e     | U+005E   |
| carriage return             | toCamel | 0xd      | U+000D   |
| close brace                 | toCamel | 0x7d     | U+007D   |
| close bracket               | toCamel | 0x5d     | U+005D   |
| close paren                 | toCamel | 0x29     | U+0029   |
| colon                       | toCamel | 0x3a     | U+003A   |
| comma                       | toCamel | 0x2c     | U+002C   |
| dash                        | toCamel | 0x2d     | U+002D   |
| delete character            | toCamel | 0x7f     | U+007F   |
| dollar sign                 | toCamel | 0x24     | U+0024   |
| dot                         | toCamel | 0x2e     | U+002E   |
| double quote                | toCamel | 0x22     | U+0022   |
| eight                       | toCamel | 0x38     | U+0038   |
| equals                      | toCamel | 0x3d     | U+003D   |
| exclamation                 | toCamel | 0x21     | U+0021   |
| five                        | toCamel | 0x35     | U+0035   |
| form feed                   | toCamel | 0xc      | U+000C   |
| four                        | toCamel | 0x34     | U+0034   |
| greater than                | toCamel | 0x3e     | U+003E   |
| hash                        | toCamel | 0x23     | U+0023   |
| less than                   | toCamel | 0x3c     | U+003C   |
| lower a                     | toCamel | 0x61     | U+0061   |
| lower b                     | toCamel | 0x62     | U+0062   |
| lower c                     | toCamel | 0x63     | U+0063   |
| lower d                     | toCamel | 0x64     | U+0064   |
| lower e                     | toCamel | 0x65     | U+0065   |
| lower f                     | toCamel | 0x66     | U+0066   |
| lower g                     | toCamel | 0x67     | U+0067   |
| lower h                     | toCamel | 0x68     | U+0068   |
| lower i                     | toCamel | 0x69     | U+0069   |
| lower j                     | toCamel | 0x6a     | U+006A   |
| lower k                     | toCamel | 0x6b     | U+006B   |
| lower l                     | toCamel | 0x6c     | U+006C   |
| lower m                     | toCamel | 0x6d     | U+006D   |
| lower n                     | toCamel | 0x6e     | U+006E   |
| lower o                     | toCamel | 0x6f     | U+006F   |
| lower p                     | toCamel | 0x70     | U+0070   |
| lower q                     | toCamel | 0x71     | U+0071   |
| lower r                     | toCamel | 0x72     | U+0072   |
| lower s                     | toCamel | 0x73     | U+0073   |
| lower t                     | toCamel | 0x74     | U+0074   |
| lower u                     | toCamel | 0x75     | U+0075   |
| lower v                     | toCamel | 0x76     | U+0076   |
| lower w                     | toCamel | 0x77     | U+0077   |
| lower x                     | toCamel | 0x78     | U+0078   |
| lower y                     | toCamel | 0x79     | U+0079   |
| lower z                     | toCamel | 0x7a     | U+007A   |
| maximum code point          | toCamel | 0x10ffff | U+10FFFF |
| newline                     | toCamel | 0xa      | U+000A   |
| nine                        | toCamel | 0x39     | U+0039   |
| non ascii start             | toCamel | 0x80     | U+0080   |
| null character              | toCamel | 0x0      | U+0000   |
| one                         | toCamel | 0x31     | U+0031   |
| open brace                  | toCamel | 0x7b     | U+007B   |
| open bracket                | toCamel | 0x5b     | U+005B   |
| open paren                  | toCamel | 0x28     | U+0028   |
| percent                     | toCamel | 0x25     | U+0025   |
| pipe                        | toCamel | 0x7c     | U+007C   |
| plus                        | toCamel | 0x2b     | U+002B   |
| question                    | toCamel | 0x3f     | U+003F   |
| replacement character       | toCamel | 0xfffd   | U+FFFD   |
| semicolon                   | toCamel | 0x3b     | U+003B   |
| seven                       | toCamel | 0x37     | U+0037   |
| shift out                   | toCamel | 0xe      | U+000E   |
| single quote                | toCamel | 0x27     | U+0027   |
| six                         | toCamel | 0x36     | U+0036   |
| slash                       | toCamel | 0x2f     | U+002F   |
| space                       | toCamel | 0x20     | U+0020   |
| surrogate range end         | toCamel | 0xdfff   | U+DFFF   |
| surrogate range start       | toCamel | 0xd800   | U+D800   |
| tab                         | toCamel | 0x9      | U+0009   |
| three                       | toCamel | 0x33     | U+0033   |
| tilde                       | toCamel | 0x7e     | U+007E   |
| two                         | toCamel | 0x32     | U+0032   |
| underscore                  | toCamel | 0x5f     | U+005F   |
| unit separator              | toCamel | 0x1f     | U+001F   |
| upper A                     | toCamel | 0x41     | U+0041   |
| upper B                     | toCamel | 0x42     | U+0042   |
| upper C                     | toCamel | 0x43     | U+0043   |
| upper D                     | toCamel | 0x44     | U+0044   |
| upper E                     | toCamel | 0x45     | U+0045   |
| upper F                     | toCamel | 0x46     | U+0046   |
| upper G                     | toCamel | 0x47     | U+0047   |
| upper H                     | toCamel | 0x48     | U+0048   |
| upper I                     | toCamel | 0x49     | U+0049   |
| upper J                     | toCamel | 0x4a     | U+004A   |
| upper K                     | toCamel | 0x4b     | U+004B   |
| upper L                     | toCamel | 0x4c     | U+004C   |
| upper M                     | toCamel | 0x4d     | U+004D   |
| upper N                     | toCamel | 0x4e     | U+004E   |
| upper O                     | toCamel | 0x4f     | U+004F   |
| upper P                     | toCamel | 0x50     | U+0050   |
| upper Q                     | toCamel | 0x51     | U+0051   |
| upper R                     | toCamel | 0x52     | U+0052   |
| upper S                     | toCamel | 0x53     | U+0053   |
| upper T                     | toCamel | 0x54     | U+0054   |
| upper U                     | toCamel | 0x55     | U+0055   |
| upper V                     | toCamel | 0x56     | U+0056   |
| upper W                     | toCamel | 0x57     | U+0057   |
| upper X                     | toCamel | 0x58     | U+0058   |
| upper Y                     | toCamel | 0x59     | U+0059   |
| upper Z                     | toCamel | 0x5a     | U+005A   |
| vertical tab                | toCamel | 0xb      | U+000B   |
| zero                        | toCamel | 0x30     | U+0030   |
+-----------------------------+---------+----------+----------+

## escape

```
@brief Control character to its named escape letter.

Keyed by the character's codepoint constant, valued by the lowercase
letter used after a backslash in a literal escape sequence.
```

+-----------------+---------+--------+
| key             | format  | value  |
+=================+=========+========+
| backspace       | toCamel | lowerB |
| tab             | toCamel | lowerT |
| newline         | toCamel | lowerN |
| vertical tab    | toCamel | lowerV |
| form feed       | toCamel | lowerF |
| carriage return | toCamel | lowerR |
+-----------------+---------+--------+

## enclosure

```
@brief Opening character to its matching closing character.

Keyed by the character's codepoint constant, valued by the codepoint
constant of the character that closes the enclosure it opens.
```

+--------------+---------+--------------+
| key          | format  | value        |
+==============+=========+==============+
| open bracket | toCamel | closeBracket |
| open brace   | toCamel | closeBrace   |
| open paren   | toCamel | closeParen   |
| double quote | toCamel | doubleQuote  |
| single quote | toCamel | singleQuote  |
| backtick     | toCamel | backtick     |
+--------------+---------+--------------+

## Byte

+--------------+-------+
| key          | value |
+==============+=======+
| text         | 0     |
| operators    | 1     |
| region open  | 2     |
| region close | 3     |
| quote        | 4     |
+--------------+-------+

## Diacritics

```
@brief Base letter to its accented diacritic variants.

Maps each base letter (and a few digraphs/ligatures like `AE`, `ss`, `TH`)
to the concatenated string of its accented variants. Consumed by
case/diacritic folding so an unaccented search can match accented text.
```

+------+-----------+
| key  | value     |
+======+===========+
| `A`  | `ÀÁÂÃÄÅĀ` |
| `AE` | `Æ`       |
| `C`  | `ÇĆČ`     |
| `E`  | `ÈÉÊËĒ`   |
| `I`  | `ÌÍÎÏĪ`   |
| `D`  | `Ð`       |
| `N`  | `Ñ`       |
| `O`  | `ÒÓÔÕÖØŌ` |
| `U`  | `ÙÚÛÜŪ`   |
| `Y`  | `Ý`       |
| `TH` | `Þ`       |
| `ss` | `ß`       |
| `a`  | `àáâãäåā` |
| `ae` | `æ`       |
| `c`  | `çćč`     |
| `e`  | `èéêëē`   |
| `i`  | `ìíîïī`   |
| `d`  | `ð`       |
| `n`  | `ñ`       |
| `o`  | `òóôõöøō` |
| `u`  | `ùúûüū`   |
| `y`  | `ýÿ`      |
| `th` | `þ`       |
| `s`  | `šş`      |
| `S`  | `ŠŞ`      |
| `z`  | `ž`       |
| `Z`  | `Ž`       |
| `l`  | `ł`       |
| `L`  | `Ł`       |
| `2`  | `₂`       |
+------+-----------+

## xmlEscapes

```
@brief XML special character to its escaped literal form.

Maps the XML-significant characters (`&`, `<`, `>`) to their `&amp;`-style
escape sequences. Consumed by the XML writer to escape character data so
the output round-trips as well-formed XML.
```

+-----+-------------+
| key | value       |
+=====+=============+
| '&' | "&amp;amp;" |
| '<' | "&amp;lt;"  |
| '>' | "&amp;gt;"  |
+-----+-------------+

## tokens

+------------------------+--------------------------------------------+----------+-------------------------------------------+
| name                   | value                                      | format   | comment                                   |
+========================+============================================+==========+===========================================+
| annotationClose        | `>>`                                       |          | Annotation close marker.                  |
| annotationOpen         | `<<`                                       |          | Annotation open marker.                   |
| attributeBlock         | `@{`                                       |          | Attribute block open.                     |
| block                  | `block`                                    |          | Block keyword.                            |
| cdataClose             | `]]>`                                      |          | CDATA close.                              |
| cdataOpen              | `<![CDATA[`                                |          | CDATA open.                               |
| classDef               | `:::`                                      |          | CAST class/token delimiter.               |
| classToken             | `class`                                    |          | Class keyword.                            |
| csiIntroducer          | `U+001B[`                                  | fromUTF8 | CSI escape introducer.                    |
| cssCommentClose        | `*/`                                       |          | CSS comment close.                        |
| cssCommentOpen         | `/*`                                       |          | CSS comment open.                         |
| declarationClose       | `>`                                        |          | HTML declaration close.                   |
| declarationOpen        | `<!`                                       |          | HTML declaration open.                    |
| doubleDash             | `--`                                       |          | Double dash.                              |
| doubleDashChevronRight | `-->`                                      |          | HTML comment close.                       |
| doubleDot              | `..`                                       |          | Double dot.                               |
| doublePercent          | `%%`                                       |          | Mermaid comment marker.                   |
| elseToken              | `else`                                     |          | Else keyword.                             |
| encoding               | `encoding`                                 |          | Encoding keyword.                         |
| endTagOpen             | `</`                                       |          | HTML end-tag open.                        |
| escapedPipe            | `\\\|`                                     |          | Escaped pipe in a table cell.             |
| ftpAutolinkPrefix      | `ftp://`                                   |          | FTP autolink prefix.                      |
| groupModifier          | `{group}`                                  |          | Group modifier token.                     |
| httpAutolinkPrefix     | `http://`                                  |          | HTTP autolink prefix.                     |
| httpsAutolinkPrefix    | `https://`                                 |          | HTTPS autolink prefix.                    |
| important              | `important`                                |          | CSS important keyword.                    |
| markupCommentOpen      | `<!--`                                     |          | HTML comment open.                        |
| messageReverseSolid    | `<-`                                       |          | Mermaid reverse message arrow.            |
| messageSolid           | `->`                                       |          | Mermaid message arrow.                    |
| preCloseTag            | `</pre>`                                   |          | Pre close tag.                            |
| processingClose        | `?>`                                       |          | Processing-instruction close.             |
| processingOpen         | `<?`                                       |          | Processing-instruction open.              |
| quotes                 | `"'`                                       |          | Both quote characters.                    |
| scriptCloseTag         | `</script>`                                |          | Script close tag.                         |
| selfCloseTag           | `/>`                                       |          | HTML self-close tag.                      |
| special                | `., U+0060~!@#$%^&*()-=+[]{}\\\|;:'",<>/?` | fromUTF8 | Punctuation set for token classification. |
| state                  | `state`                                    |          | State keyword.                            |
| styleCloseTag          | `</style>`                                 |          | Style close tag.                          |
| textareaCloseTag       | `</textarea>`                              |          | Textarea close tag.                       |
| urlFunction            | `url`                                      |          | CSS url function name.                    |
| urlOpen                | `url(`                                     |          | CSS url function open.                    |
| utf8                   | `UTF-8`                                    |          | UTF-8 charset name.                       |
| varFunction            | `var`                                      |          | CSS var function name.                    |
| wwwAutolinkPrefix      | `www.`                                     |          | WWW autolink prefix.                      |
| xml                    | `xml`                                      |          | XML keyword.                              |
+------------------------+--------------------------------------------+----------+-------------------------------------------+
