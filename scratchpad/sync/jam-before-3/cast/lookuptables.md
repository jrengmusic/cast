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

+------------+-----------------------------+
| alias      | symbol                      |
+============+=============================+
| @black     | 0xff000000                  |
| @red       | 0xffff0000                  |
| @green     | 0xff00ff00                  |
| @yellow    | 0xffffff00                  |
| @magenta   | 0xffff00ff                  |
| @aqua      | 0xff00ffff                  |
| @white     | 0xffffffff                  |
| @operators | map::Byte::operators        |
| @quote     | map::Byte::quote            |
| @BK        | map::LineBreak::BK          |
| @CR        | map::LineBreak::CR          |
| @LF        | map::LineBreak::LF          |
| @CM        | map::LineBreak::CM          |
| @NL        | map::LineBreak::NL          |
| @SG        | map::LineBreak::SG          |
| @WJ        | map::LineBreak::WJ          |
| @ZW        | map::LineBreak::ZW          |
| @GL        | map::LineBreak::GL          |
| @SP        | map::LineBreak::SP          |
| @ZWJ       | map::LineBreak::ZWJ         |
| @B2        | map::LineBreak::B2          |
| @BA        | map::LineBreak::BA          |
| @BB        | map::LineBreak::BB          |
| @HY        | map::LineBreak::HY          |
| @HH        | map::LineBreak::HH          |
| @CB        | map::LineBreak::CB          |
| @CL        | map::LineBreak::CL          |
| @CP        | map::LineBreak::CP          |
| @EX        | map::LineBreak::EX          |
| @IN        | map::LineBreak::IN          |
| @NS        | map::LineBreak::NS          |
| @OP        | map::LineBreak::OP          |
| @QU        | map::LineBreak::QU          |
| @IS        | map::LineBreak::IS          |
| @NU        | map::LineBreak::NU          |
| @PO        | map::LineBreak::PO          |
| @PR        | map::LineBreak::PR          |
| @SY        | map::LineBreak::SY          |
| @AI        | map::LineBreak::AI          |
| @AK        | map::LineBreak::AK          |
| @AL        | map::LineBreak::AL          |
| @AP        | map::LineBreak::AP          |
| @AS        | map::LineBreak::AS          |
| @CJ        | map::LineBreak::CJ          |
| @EB        | map::LineBreak::EB          |
| @EM        | map::LineBreak::EM          |
| @H2        | map::LineBreak::H2          |
| @H3        | map::LineBreak::H3          |
| @HL        | map::LineBreak::HL          |
| @ID        | map::LineBreak::ID          |
| @JL        | map::LineBreak::JL          |
| @JV        | map::LineBreak::JV          |
| @JT        | map::LineBreak::JT          |
| @RI        | map::LineBreak::RI          |
| @SA        | map::LineBreak::SA          |
| @VF        | map::LineBreak::VF          |
| @VI        | map::LineBreak::VI          |
| @XX        | map::LineBreak::XX          |
| @und       | map::LineBreakLanguage::und |
| @de        | map::LineBreakLanguage::de  |
| @en        | map::LineBreakLanguage::en  |
| @es        | map::LineBreakLanguage::es  |
| @fr        | map::LineBreakLanguage::fr  |
| @ja        | map::LineBreakLanguage::ja  |
| @ko        | map::LineBreakLanguage::ko  |
| @ru        | map::LineBreakLanguage::ru  |
| @zh        | map::LineBreakLanguage::zh  |
+------------+-----------------------------+

## terminalColourSpace

```
@brief 256-entry xterm palette — terminal colour index to ARGB word.

Direct-indexes every terminal colour index (0–255) into its packed ARGB
word: the 16 base colours, the 6×6×6 colour cube, and the 24 greyscale
ramp. Consumed by the terminal renderer to resolve SGR colour parameters
into drawable `juce::Colour` values.
```

+-----+------------+
| key | value      |
+=====+============+
| 0   | @black     |
| 1   | 0xffcd0000 |
| 2   | 0xff00cd00 |
| 3   | 0xffcdcd00 |
| 4   | 0xff0000ee |
| 5   | 0xffcd00cd |
| 6   | 0xff00cdcd |
| 7   | 0xffe5e5e5 |
| 8   | 0xff7f7f7f |
| 9   | @red       |
| 10  | @green     |
| 11  | @yellow    |
| 12  | 0xff5c5cff |
| 13  | @magenta   |
| 14  | @aqua      |
| 15  | @white     |
| 16  | @black     |
| 17  | 0xff00005f |
| 18  | 0xff000087 |
| 19  | 0xff0000af |
| 20  | 0xff0000d7 |
| 21  | 0xff0000ff |
| 22  | 0xff005f00 |
| 23  | 0xff005f5f |
| 24  | 0xff005f87 |
| 25  | 0xff005faf |
| 26  | 0xff005fd7 |
| 27  | 0xff005fff |
| 28  | 0xff008700 |
| 29  | 0xff00875f |
| 30  | 0xff008787 |
| 31  | 0xff0087af |
| 32  | 0xff0087d7 |
| 33  | 0xff0087ff |
| 34  | 0xff00af00 |
| 35  | 0xff00af5f |
| 36  | 0xff00af87 |
| 37  | 0xff00afaf |
| 38  | 0xff00afd7 |
| 39  | 0xff00afff |
| 40  | 0xff00d700 |
| 41  | 0xff00d75f |
| 42  | 0xff00d787 |
| 43  | 0xff00d7af |
| 44  | 0xff00d7d7 |
| 45  | 0xff00d7ff |
| 46  | @green     |
| 47  | 0xff00ff5f |
| 48  | 0xff00ff87 |
| 49  | 0xff00ffaf |
| 50  | 0xff00ffd7 |
| 51  | @aqua      |
| 52  | 0xff5f0000 |
| 53  | 0xff5f005f |
| 54  | 0xff5f0087 |
| 55  | 0xff5f00af |
| 56  | 0xff5f00d7 |
| 57  | 0xff5f00ff |
| 58  | 0xff5f5f00 |
| 59  | 0xff5f5f5f |
| 60  | 0xff5f5f87 |
| 61  | 0xff5f5faf |
| 62  | 0xff5f5fd7 |
| 63  | 0xff5f5fff |
| 64  | 0xff5f8700 |
| 65  | 0xff5f875f |
| 66  | 0xff5f8787 |
| 67  | 0xff5f87af |
| 68  | 0xff5f87d7 |
| 69  | 0xff5f87ff |
| 70  | 0xff5faf00 |
| 71  | 0xff5faf5f |
| 72  | 0xff5faf87 |
| 73  | 0xff5fafaf |
| 74  | 0xff5fafd7 |
| 75  | 0xff5fafff |
| 76  | 0xff5fd700 |
| 77  | 0xff5fd75f |
| 78  | 0xff5fd787 |
| 79  | 0xff5fd7af |
| 80  | 0xff5fd7d7 |
| 81  | 0xff5fd7ff |
| 82  | 0xff5fff00 |
| 83  | 0xff5fff5f |
| 84  | 0xff5fff87 |
| 85  | 0xff5fffaf |
| 86  | 0xff5fffd7 |
| 87  | 0xff5fffff |
| 88  | 0xff870000 |
| 89  | 0xff87005f |
| 90  | 0xff870087 |
| 91  | 0xff8700af |
| 92  | 0xff8700d7 |
| 93  | 0xff8700ff |
| 94  | 0xff875f00 |
| 95  | 0xff875f5f |
| 96  | 0xff875f87 |
| 97  | 0xff875faf |
| 98  | 0xff875fd7 |
| 99  | 0xff875fff |
| 100 | 0xff878700 |
| 101 | 0xff87875f |
| 102 | 0xff878787 |
| 103 | 0xff8787af |
| 104 | 0xff8787d7 |
| 105 | 0xff8787ff |
| 106 | 0xff87af00 |
| 107 | 0xff87af5f |
| 108 | 0xff87af87 |
| 109 | 0xff87afaf |
| 110 | 0xff87afd7 |
| 111 | 0xff87afff |
| 112 | 0xff87d700 |
| 113 | 0xff87d75f |
| 114 | 0xff87d787 |
| 115 | 0xff87d7af |
| 116 | 0xff87d7d7 |
| 117 | 0xff87d7ff |
| 118 | 0xff87ff00 |
| 119 | 0xff87ff5f |
| 120 | 0xff87ff87 |
| 121 | 0xff87ffaf |
| 122 | 0xff87ffd7 |
| 123 | 0xff87ffff |
| 124 | 0xffaf0000 |
| 125 | 0xffaf005f |
| 126 | 0xffaf0087 |
| 127 | 0xffaf00af |
| 128 | 0xffaf00d7 |
| 129 | 0xffaf00ff |
| 130 | 0xffaf5f00 |
| 131 | 0xffaf5f5f |
| 132 | 0xffaf5f87 |
| 133 | 0xffaf5faf |
| 134 | 0xffaf5fd7 |
| 135 | 0xffaf5fff |
| 136 | 0xffaf8700 |
| 137 | 0xffaf875f |
| 138 | 0xffaf8787 |
| 139 | 0xffaf87af |
| 140 | 0xffaf87d7 |
| 141 | 0xffaf87ff |
| 142 | 0xffafaf00 |
| 143 | 0xffafaf5f |
| 144 | 0xffafaf87 |
| 145 | 0xffafafaf |
| 146 | 0xffafafd7 |
| 147 | 0xffafafff |
| 148 | 0xffafd700 |
| 149 | 0xffafd75f |
| 150 | 0xffafd787 |
| 151 | 0xffafd7af |
| 152 | 0xffafd7d7 |
| 153 | 0xffafd7ff |
| 154 | 0xffafff00 |
| 155 | 0xffafff5f |
| 156 | 0xffafff87 |
| 157 | 0xffafffaf |
| 158 | 0xffafffd7 |
| 159 | 0xffafffff |
| 160 | 0xffd70000 |
| 161 | 0xffd7005f |
| 162 | 0xffd70087 |
| 163 | 0xffd700af |
| 164 | 0xffd700d7 |
| 165 | 0xffd700ff |
| 166 | 0xffd75f00 |
| 167 | 0xffd75f5f |
| 168 | 0xffd75f87 |
| 169 | 0xffd75faf |
| 170 | 0xffd75fd7 |
| 171 | 0xffd75fff |
| 172 | 0xffd78700 |
| 173 | 0xffd7875f |
| 174 | 0xffd78787 |
| 175 | 0xffd787af |
| 176 | 0xffd787d7 |
| 177 | 0xffd787ff |
| 178 | 0xffd7af00 |
| 179 | 0xffd7af5f |
| 180 | 0xffd7af87 |
| 181 | 0xffd7afaf |
| 182 | 0xffd7afd7 |
| 183 | 0xffd7afff |
| 184 | 0xffd7d700 |
| 185 | 0xffd7d75f |
| 186 | 0xffd7d787 |
| 187 | 0xffd7d7af |
| 188 | 0xffd7d7d7 |
| 189 | 0xffd7d7ff |
| 190 | 0xffd7ff00 |
| 191 | 0xffd7ff5f |
| 192 | 0xffd7ff87 |
| 193 | 0xffd7ffaf |
| 194 | 0xffd7ffd7 |
| 195 | 0xffd7ffff |
| 196 | @red       |
| 197 | 0xffff005f |
| 198 | 0xffff0087 |
| 199 | 0xffff00af |
| 200 | 0xffff00d7 |
| 201 | @magenta   |
| 202 | 0xffff5f00 |
| 203 | 0xffff5f5f |
| 204 | 0xffff5f87 |
| 205 | 0xffff5faf |
| 206 | 0xffff5fd7 |
| 207 | 0xffff5fff |
| 208 | 0xffff8700 |
| 209 | 0xffff875f |
| 210 | 0xffff8787 |
| 211 | 0xffff87af |
| 212 | 0xffff87d7 |
| 213 | 0xffff87ff |
| 214 | 0xffffaf00 |
| 215 | 0xffffaf5f |
| 216 | 0xffffaf87 |
| 217 | 0xffffafaf |
| 218 | 0xffffafd7 |
| 219 | 0xffffafff |
| 220 | 0xffffd700 |
| 221 | 0xffffd75f |
| 222 | 0xffffd787 |
| 223 | 0xffffd7af |
| 224 | 0xffffd7d7 |
| 225 | 0xffffd7ff |
| 226 | @yellow    |
| 227 | 0xffffff5f |
| 228 | 0xffffff87 |
| 229 | 0xffffffaf |
| 230 | 0xffffffd7 |
| 231 | @white     |
| 232 | 0xff080808 |
| 233 | 0xff121212 |
| 234 | 0xff1c1c1c |
| 235 | 0xff262626 |
| 236 | 0xff303030 |
| 237 | 0xff3a3a3a |
| 238 | 0xff444444 |
| 239 | 0xff4e4e4e |
| 240 | 0xff585858 |
| 241 | 0xff626262 |
| 242 | 0xff6c6c6c |
| 243 | 0xff767676 |
| 244 | 0xff808080 |
| 245 | 0xff8a8a8a |
| 246 | 0xff949494 |
| 247 | 0xff9e9e9e |
| 248 | 0xffa8a8a8 |
| 249 | 0xffb2b2b2 |
| 250 | 0xffbcbcbc |
| 251 | 0xffc6c6c6 |
| 252 | 0xffd0d0d0 |
| 253 | 0xffdadada |
| 254 | 0xffe4e4e4 |
| 255 | 0xffeeeeee |
+-----+------------+

## cssCodePoints

```
@brief CSS token kind to its single-character delimiter codepoint.

Maps the punctuation CSS token kinds (colon, semicolon, comma, the bracket/
paren/brace pairs) to their single-character codepoint, and the ident/
whitespace kinds to their sentinel (null and space). Consumed by the CSS
tokeniser when it needs the literal for a punctuation token.
```

+----------------------------+----------------------+
| key                        | value                |
+============================+======================+
| CssTokenType::ident        | Chars::nullCharacter |
| CssTokenType::whitespace   | Chars::space         |
| CssTokenType::colon        | Chars::colon         |
| CssTokenType::semicolon    | Chars::semicolon     |
| CssTokenType::comma        | Chars::comma         |
| CssTokenType::openBracket  | Chars::openBracket   |
| CssTokenType::closeBracket | Chars::closeBracket  |
| CssTokenType::openParen    | Chars::openParen     |
| CssTokenType::closeParen   | Chars::closeParen    |
| CssTokenType::openBrace    | Chars::openBrace     |
| CssTokenType::closeBrace   | Chars::closeBrace    |
+----------------------------+----------------------+

## markupOpen

```
@brief HTML block start-condition to its opening delimiter text.

Maps each HTML block start condition (comment, processing instruction,
declaration, CDATA) to the opening delimiter that begins the raw block; the
`none` condition carries a null delimiter. Paired with `markupClose`.
```

+--------------------------------------+--------------------------+
| key                                  | value                    |
+======================================+==========================+
| HtmlBlockType::none                  | nullptr                  |
| HtmlBlockType::comment               | Chars::markupCommentOpen |
| HtmlBlockType::processingInstruction | Chars::processingOpen    |
| HtmlBlockType::declaration           | Chars::declarationOpen   |
| HtmlBlockType::cdata                 | Chars::cdataOpen         |
+--------------------------------------+--------------------------+

## markupClose

```
@brief HTML block start-condition to its closing delimiter text.

Maps each HTML block start condition to the closing delimiter that
terminates the raw block; the `none` condition carries a null delimiter.
Paired with `markupOpen`.
```

+--------------------------------------+-------------------------------+
| key                                  | value                         |
+======================================+===============================+
| HtmlBlockType::none                  | nullptr                       |
| HtmlBlockType::comment               | Chars::doubleDashChevronRight |
| HtmlBlockType::processingInstruction | Chars::processingClose        |
| HtmlBlockType::declaration           | Chars::declarationClose       |
| HtmlBlockType::cdata                 | Chars::cdataClose             |
+--------------------------------------+-------------------------------+

## rawTextEnd

```
@brief Raw-text tag kind to its closing-tag text.

Maps each raw-text HTML element (script, pre, textarea, style) to the exact
closing tag that ends its raw-text run. Consumed by the HTML tokeniser to
find the end of an element whose content is not parsed as markup.
```

+------------------------+-------------------------+
| key                    | value                   |
+========================+=========================+
| HtmlType1Tag::script   | Chars::scriptCloseTag   |
| HtmlType1Tag::pre      | Chars::preCloseTag      |
| HtmlType1Tag::textarea | Chars::textareaCloseTag |
| HtmlType1Tag::style    | Chars::styleCloseTag    |
+------------------------+-------------------------+

## markupLanguage

```
@brief Byte to its markup-language classification.

Direct-indexes every byte into a `map::Byte` class for markup scanning.
`<` and `>` are operator delimiters; all other bytes default to text.
```

+------+-----------------+
| key  | value           |
+======+=================+
| 0x0  | map::Byte::text |
| 0x3c | @operators      |
| 0x3e | @operators      |
+------+-----------------+

## css

```
@brief Byte to its CSS classification.

Direct-indexes every byte into a `map::Byte` class for CSS scanning: the
punctuation and structural bytes (braces, brackets, parens, and the
`; : , @ # % ! . + - * / = ~ | ^ $` operator set) are operator delimiters,
the quote bytes are quotes, and everything else defaults to text.
```

+------+-----------------+
| key  | value           |
+======+=================+
| 0x0  | map::Byte::text |
| 0x3c | @operators      |
| 0x3e | @operators      |
| 0x7b | @operators      |
| 0x7d | @operators      |
| 0x28 | @operators      |
| 0x29 | @operators      |
| 0x5b | @operators      |
| 0x5d | @operators      |
| 0x3b | @operators      |
| 0x3a | @operators      |
| 0x2c | @operators      |
| 0x40 | @operators      |
| 0x23 | @operators      |
| 0x25 | @operators      |
| 0x21 | @operators      |
| 0x2e | @operators      |
| 0x2b | @operators      |
| 0x2d | @operators      |
| 0x2a | @operators      |
| 0x2f | @operators      |
| 0x3d | @operators      |
| 0x7e | @operators      |
| 0x7c | @operators      |
| 0x5e | @operators      |
| 0x24 | @operators      |
| 0x22 | @quote          |
| 0x27 | @quote          |
+------+-----------------+

## markdown

```
@brief Byte to its Markdown classification.

Direct-indexes every byte into a `map::Byte` class for Markdown scanning:
the emphasis/list/link/heading/table punctuation bytes are operator
delimiters, and everything else defaults to text.
```

+------+-----------------+
| key  | value           |
+======+=================+
| 0x0  | map::Byte::text |
| 0x3c | @operators      |
| 0x3e | @operators      |
| 0x28 | @operators      |
| 0x29 | @operators      |
| 0x5b | @operators      |
| 0x5d | @operators      |
| 0x3a | @operators      |
| 0x23 | @operators      |
| 0x21 | @operators      |
| 0x2b | @operators      |
| 0x2d | @operators      |
| 0x2a | @operators      |
| 0x3d | @operators      |
| 0x7e | @operators      |
| 0x7c | @operators      |
| 0x5c | @operators      |
| 0x60 | @operators      |
+------+-----------------+

## code

```
@brief Byte to its code-language classification.

Direct-indexes every byte into a `map::Byte` class for code scanning: the
operator punctuation (braces, brackets, parens, and the
`; : , @ # % ! . + - * / = ~ | ^ & ?` operator set) is operator delimiters,
the quote bytes are quotes, and everything else defaults to text.
```

+------+-----------------+
| key  | value           |
+======+=================+
| 0x0  | map::Byte::text |
| 0x3c | @operators      |
| 0x3e | @operators      |
| 0x7b | @operators      |
| 0x7d | @operators      |
| 0x28 | @operators      |
| 0x29 | @operators      |
| 0x5b | @operators      |
| 0x5d | @operators      |
| 0x3b | @operators      |
| 0x3a | @operators      |
| 0x2c | @operators      |
| 0x40 | @operators      |
| 0x23 | @operators      |
| 0x25 | @operators      |
| 0x21 | @operators      |
| 0x2e | @operators      |
| 0x2b | @operators      |
| 0x2d | @operators      |
| 0x2a | @operators      |
| 0x2f | @operators      |
| 0x3d | @operators      |
| 0x7e | @operators      |
| 0x7c | @operators      |
| 0x5e | @operators      |
| 0x22 | @quote          |
| 0x27 | @quote          |
| 0x26 | @operators      |
| 0x3f | @operators      |
+------+-----------------+

## markup

```
@brief Byte to its markup-tag classification.

Direct-indexes every byte into a `map::Byte` class for markup-tag scanning:
`<`, `>`, `/`, and `=` are operator delimiters, the quote bytes are quotes,
and everything else defaults to text.
```

+------+-----------------+
| key  | value           |
+======+=================+
| 0x0  | map::Byte::text |
| 0x3c | @operators      |
| 0x3e | @operators      |
| 0x2f | @operators      |
| 0x3d | @operators      |
| 0x22 | @quote          |
| 0x27 | @quote          |
+------+-----------------+

## data

```
@brief Byte to its data-language classification.

Direct-indexes every byte into a `map::Byte` class for data-language (JSON/
YAML) scanning: the brace/bracket/colon/comma/equals bytes are operator
delimiters, the quote bytes are quotes, and everything else defaults to
text.
```

+------+-----------------+
| key  | value           |
+======+=================+
| 0x0  | map::Byte::text |
| 0x7b | @operators      |
| 0x7d | @operators      |
| 0x5b | @operators      |
| 0x5d | @operators      |
| 0x3a | @operators      |
| 0x2c | @operators      |
| 0x3d | @operators      |
| 0x22 | @quote          |
+------+-----------------+

## ansi

```
@brief Byte to its ANSI classification.

Direct-indexes every byte into a `map::Byte` class for ANSI scanning: the C0
control bytes (0x00-0x1f) and DEL (0x7f) are operator delimiters — ESC
introduces CSI/OSC sequences and the other C0 singles close lines or move
the cursor; every other byte, printable ASCII and UTF-8 lead/continuation
alike, defaults to text.
```

+------+-----------------+
| key  | value           |
+======+=================+
| 0x20 | map::Byte::text |
| 0x0  | @operators      |
| 0x1  | @operators      |
| 0x2  | @operators      |
| 0x3  | @operators      |
| 0x4  | @operators      |
| 0x5  | @operators      |
| 0x6  | @operators      |
| 0x7  | @operators      |
| 0x8  | @operators      |
| 0x9  | @operators      |
| 0xa  | @operators      |
| 0xb  | @operators      |
| 0xc  | @operators      |
| 0xd  | @operators      |
| 0xe  | @operators      |
| 0xf  | @operators      |
| 0x10 | @operators      |
| 0x11 | @operators      |
| 0x12 | @operators      |
| 0x13 | @operators      |
| 0x14 | @operators      |
| 0x15 | @operators      |
| 0x16 | @operators      |
| 0x17 | @operators      |
| 0x18 | @operators      |
| 0x19 | @operators      |
| 0x1a | @operators      |
| 0x1b | @operators      |
| 0x1c | @operators      |
| 0x1d | @operators      |
| 0x1e | @operators      |
| 0x1f | @operators      |
| 0x7f | @operators      |
+------+-----------------+

## reflow

```
@brief Byte class for the reflow vocabulary.

Every byte is text; the document decodes codepoints itself.
```

+-----+-----------------+
| key | value           |
+=====+=================+
| 0x0 | map::Byte::text |
+-----+-----------------+

## os2UnicodeRangeStarts

```
@brief `OS/2` ulUnicodeRange — first codepoint of each Unicode block range.

Row N of this table, `os2UnicodeRangeEnds` and `os2UnicodeRangeBits` together
describe one Unicode block range and the `ulUnicodeRange` bit it sets. The
font writer walks every mapped codepoint and sets the bit of each range the
codepoint falls inside.
```

+-----+----------+
| key | value    |
+=====+==========+
| 0   | 0x0      |
| 1   | 0x80     |
| 2   | 0x100    |
| 3   | 0x180    |
| 4   | 0x250    |
| 5   | 0x1d00   |
| 6   | 0x1d80   |
| 7   | 0x2b0    |
| 8   | 0xa700   |
| 9   | 0x300    |
| 10  | 0x1dc0   |
| 11  | 0x370    |
| 12  | 0x2c80   |
| 13  | 0x400    |
| 14  | 0x500    |
| 15  | 0x2de0   |
| 16  | 0xa640   |
| 17  | 0x530    |
| 18  | 0x590    |
| 19  | 0xa500   |
| 20  | 0x600    |
| 21  | 0x750    |
| 22  | 0x7c0    |
| 23  | 0x900    |
| 24  | 0x980    |
| 25  | 0xa00    |
| 26  | 0xa80    |
| 27  | 0xb00    |
| 28  | 0xb80    |
| 29  | 0xc00    |
| 30  | 0xc80    |
| 31  | 0xd00    |
| 32  | 0xe00    |
| 33  | 0xe80    |
| 34  | 0x10a0   |
| 35  | 0x2d00   |
| 36  | 0x1b00   |
| 37  | 0x1100   |
| 38  | 0x1e00   |
| 39  | 0x2c60   |
| 40  | 0xa720   |
| 41  | 0x1f00   |
| 42  | 0x2000   |
| 43  | 0x2e00   |
| 44  | 0x2070   |
| 45  | 0x20a0   |
| 46  | 0x20d0   |
| 47  | 0x2100   |
| 48  | 0x2150   |
| 49  | 0x2190   |
| 50  | 0x27f0   |
| 51  | 0x2900   |
| 52  | 0x2b00   |
| 53  | 0x2200   |
| 54  | 0x2a00   |
| 55  | 0x27c0   |
| 56  | 0x2980   |
| 57  | 0x2300   |
| 58  | 0x2400   |
| 59  | 0x2440   |
| 60  | 0x2460   |
| 61  | 0x2500   |
| 62  | 0x2580   |
| 63  | 0x25a0   |
| 64  | 0x2600   |
| 65  | 0x2700   |
| 66  | 0x3000   |
| 67  | 0x3040   |
| 68  | 0x30a0   |
| 69  | 0x31f0   |
| 70  | 0x3100   |
| 71  | 0x31a0   |
| 72  | 0x3130   |
| 73  | 0xa840   |
| 74  | 0x3200   |
| 75  | 0x3300   |
| 76  | 0xac00   |
| 77  | 0xd800   |
| 78  | 0x10900  |
| 79  | 0x4e00   |
| 80  | 0x2e80   |
| 81  | 0x2f00   |
| 82  | 0x2ff0   |
| 83  | 0x3400   |
| 84  | 0x20000  |
| 85  | 0x3190   |
| 86  | 0xe000   |
| 87  | 0x31c0   |
| 88  | 0xf900   |
| 89  | 0x2f800  |
| 90  | 0xfb00   |
| 91  | 0xfb50   |
| 92  | 0xfe20   |
| 93  | 0xfe10   |
| 94  | 0xfe30   |
| 95  | 0xfe50   |
| 96  | 0xfe70   |
| 97  | 0xff00   |
| 98  | 0xfff0   |
| 99  | 0xf00    |
| 100 | 0x700    |
| 101 | 0x780    |
| 102 | 0xd80    |
| 103 | 0x1000   |
| 104 | 0x1200   |
| 105 | 0x1380   |
| 106 | 0x2d80   |
| 107 | 0x13a0   |
| 108 | 0x1400   |
| 109 | 0x1680   |
| 110 | 0x16a0   |
| 111 | 0x1780   |
| 112 | 0x19e0   |
| 113 | 0x1800   |
| 114 | 0x2800   |
| 115 | 0xa000   |
| 116 | 0xa490   |
| 117 | 0x1700   |
| 118 | 0x1720   |
| 119 | 0x1740   |
| 120 | 0x1760   |
| 121 | 0x10300  |
| 122 | 0x10330  |
| 123 | 0x10400  |
| 124 | 0x1d000  |
| 125 | 0x1d100  |
| 126 | 0x1d200  |
| 127 | 0x1d400  |
| 128 | 0xf0000  |
| 129 | 0x100000 |
| 130 | 0xfe00   |
| 131 | 0xe0100  |
| 132 | 0xe0000  |
| 133 | 0x1900   |
| 134 | 0x1950   |
| 135 | 0x1980   |
| 136 | 0x1a00   |
| 137 | 0x2c00   |
| 138 | 0x2d30   |
| 139 | 0x4dc0   |
| 140 | 0xa800   |
| 141 | 0x10000  |
| 142 | 0x10080  |
| 143 | 0x10100  |
| 144 | 0x10140  |
| 145 | 0x10380  |
| 146 | 0x103a0  |
| 147 | 0x10450  |
| 148 | 0x10480  |
| 149 | 0x10800  |
| 150 | 0x10a00  |
| 151 | 0x1d300  |
| 152 | 0x12000  |
| 153 | 0x12400  |
| 154 | 0x1d360  |
| 155 | 0x1b80   |
| 156 | 0x1c00   |
| 157 | 0x1c50   |
| 158 | 0xa880   |
| 159 | 0xa900   |
| 160 | 0xa930   |
| 161 | 0xaa00   |
| 162 | 0x10190  |
| 163 | 0x101d0  |
| 164 | 0x102a0  |
| 165 | 0x10280  |
| 166 | 0x10920  |
| 167 | 0x1f030  |
| 168 | 0x1f000  |
+-----+----------+

## os2UnicodeRangeEnds

```
@brief `OS/2` ulUnicodeRange — last codepoint of each Unicode block range.

Row N pairs with the same row of `os2UnicodeRangeStarts`. The range is
inclusive at both ends.
```

+-----+----------+
| key | value    |
+=====+==========+
| 0   | 0x7f     |
| 1   | 0xff     |
| 2   | 0x17f    |
| 3   | 0x24f    |
| 4   | 0x2af    |
| 5   | 0x1d7f   |
| 6   | 0x1dbf   |
| 7   | 0x2ff    |
| 8   | 0xa71f   |
| 9   | 0x36f    |
| 10  | 0x1dff   |
| 11  | 0x3ff    |
| 12  | 0x2cff   |
| 13  | 0x4ff    |
| 14  | 0x52f    |
| 15  | 0x2dff   |
| 16  | 0xa69f   |
| 17  | 0x58f    |
| 18  | 0x5ff    |
| 19  | 0xa63f   |
| 20  | 0x6ff    |
| 21  | 0x77f    |
| 22  | 0x7ff    |
| 23  | 0x97f    |
| 24  | 0x9ff    |
| 25  | 0xa7f    |
| 26  | 0xaff    |
| 27  | 0xb7f    |
| 28  | 0xbff    |
| 29  | 0xc7f    |
| 30  | 0xcff    |
| 31  | 0xd7f    |
| 32  | 0xe7f    |
| 33  | 0xeff    |
| 34  | 0x10ff   |
| 35  | 0x2d2f   |
| 36  | 0x1b7f   |
| 37  | 0x11ff   |
| 38  | 0x1eff   |
| 39  | 0x2c7f   |
| 40  | 0xa7ff   |
| 41  | 0x1fff   |
| 42  | 0x206f   |
| 43  | 0x2e7f   |
| 44  | 0x209f   |
| 45  | 0x20cf   |
| 46  | 0x20ff   |
| 47  | 0x214f   |
| 48  | 0x218f   |
| 49  | 0x21ff   |
| 50  | 0x27ff   |
| 51  | 0x297f   |
| 52  | 0x2bff   |
| 53  | 0x22ff   |
| 54  | 0x2aff   |
| 55  | 0x27ef   |
| 56  | 0x29ff   |
| 57  | 0x23ff   |
| 58  | 0x243f   |
| 59  | 0x245f   |
| 60  | 0x24ff   |
| 61  | 0x257f   |
| 62  | 0x259f   |
| 63  | 0x25ff   |
| 64  | 0x26ff   |
| 65  | 0x27bf   |
| 66  | 0x303f   |
| 67  | 0x309f   |
| 68  | 0x30ff   |
| 69  | 0x31ff   |
| 70  | 0x312f   |
| 71  | 0x31bf   |
| 72  | 0x318f   |
| 73  | 0xa87f   |
| 74  | 0x32ff   |
| 75  | 0x33ff   |
| 76  | 0xd7af   |
| 77  | 0xdfff   |
| 78  | 0x1091f  |
| 79  | 0x9fff   |
| 80  | 0x2eff   |
| 81  | 0x2fdf   |
| 82  | 0x2fff   |
| 83  | 0x4dbf   |
| 84  | 0x2a6df  |
| 85  | 0x319f   |
| 86  | 0xf8ff   |
| 87  | 0x31ef   |
| 88  | 0xfaff   |
| 89  | 0x2fa1f  |
| 90  | 0xfb4f   |
| 91  | 0xfdff   |
| 92  | 0xfe2f   |
| 93  | 0xfe1f   |
| 94  | 0xfe4f   |
| 95  | 0xfe6f   |
| 96  | 0xfeff   |
| 97  | 0xffef   |
| 98  | 0xffff   |
| 99  | 0xfff    |
| 100 | 0x74f    |
| 101 | 0x7bf    |
| 102 | 0xdff    |
| 103 | 0x109f   |
| 104 | 0x137f   |
| 105 | 0x139f   |
| 106 | 0x2ddf   |
| 107 | 0x13ff   |
| 108 | 0x167f   |
| 109 | 0x169f   |
| 110 | 0x16ff   |
| 111 | 0x17ff   |
| 112 | 0x19ff   |
| 113 | 0x18af   |
| 114 | 0x28ff   |
| 115 | 0xa48f   |
| 116 | 0xa4cf   |
| 117 | 0x171f   |
| 118 | 0x173f   |
| 119 | 0x175f   |
| 120 | 0x177f   |
| 121 | 0x1032f  |
| 122 | 0x1034f  |
| 123 | 0x1044f  |
| 124 | 0x1d0ff  |
| 125 | 0x1d1ff  |
| 126 | 0x1d24f  |
| 127 | 0x1d7ff  |
| 128 | 0xffffd  |
| 129 | 0x10fffd |
| 130 | 0xfe0f   |
| 131 | 0xe01ef  |
| 132 | 0xe007f  |
| 133 | 0x194f   |
| 134 | 0x197f   |
| 135 | 0x19df   |
| 136 | 0x1a1f   |
| 137 | 0x2c5f   |
| 138 | 0x2d7f   |
| 139 | 0x4dff   |
| 140 | 0xa82f   |
| 141 | 0x1007f  |
| 142 | 0x100ff  |
| 143 | 0x1013f  |
| 144 | 0x1018f  |
| 145 | 0x1039f  |
| 146 | 0x103df  |
| 147 | 0x1047f  |
| 148 | 0x104af  |
| 149 | 0x1083f  |
| 150 | 0x10a5f  |
| 151 | 0x1d35f  |
| 152 | 0x123ff  |
| 153 | 0x1247f  |
| 154 | 0x1d37f  |
| 155 | 0x1bbf   |
| 156 | 0x1c4f   |
| 157 | 0x1c7f   |
| 158 | 0xa8df   |
| 159 | 0xa92f   |
| 160 | 0xa95f   |
| 161 | 0xaa5f   |
| 162 | 0x101cf  |
| 163 | 0x101ff  |
| 164 | 0x102df  |
| 165 | 0x1029f  |
| 166 | 0x1093f  |
| 167 | 0x1f09f  |
| 168 | 0x1f02f  |
+-----+----------+

## os2UnicodeRangeBits

```
@brief `OS/2` ulUnicodeRange — bit number each Unicode block range sets.

Bits 0 through 31 land in `ulUnicodeRange1`, 32 through 63 in
`ulUnicodeRange2`, 64 through 95 in `ulUnicodeRange3`, and 96 through 122 in
`ulUnicodeRange4`.
```

+-----+-------+
| key | value |
+=====+=======+
| 0   | 0     |
| 1   | 1     |
| 2   | 2     |
| 3   | 3     |
| 4   | 4     |
| 5   | 4     |
| 6   | 4     |
| 7   | 5     |
| 8   | 5     |
| 9   | 6     |
| 10  | 6     |
| 11  | 7     |
| 12  | 8     |
| 13  | 9     |
| 14  | 9     |
| 15  | 9     |
| 16  | 9     |
| 17  | 10    |
| 18  | 11    |
| 19  | 12    |
| 20  | 13    |
| 21  | 13    |
| 22  | 14    |
| 23  | 15    |
| 24  | 16    |
| 25  | 17    |
| 26  | 18    |
| 27  | 19    |
| 28  | 20    |
| 29  | 21    |
| 30  | 22    |
| 31  | 23    |
| 32  | 24    |
| 33  | 25    |
| 34  | 26    |
| 35  | 26    |
| 36  | 27    |
| 37  | 28    |
| 38  | 29    |
| 39  | 29    |
| 40  | 29    |
| 41  | 30    |
| 42  | 31    |
| 43  | 31    |
| 44  | 32    |
| 45  | 33    |
| 46  | 34    |
| 47  | 35    |
| 48  | 36    |
| 49  | 37    |
| 50  | 37    |
| 51  | 37    |
| 52  | 37    |
| 53  | 38    |
| 54  | 38    |
| 55  | 38    |
| 56  | 38    |
| 57  | 39    |
| 58  | 40    |
| 59  | 41    |
| 60  | 42    |
| 61  | 43    |
| 62  | 44    |
| 63  | 45    |
| 64  | 46    |
| 65  | 47    |
| 66  | 48    |
| 67  | 49    |
| 68  | 50    |
| 69  | 50    |
| 70  | 51    |
| 71  | 51    |
| 72  | 52    |
| 73  | 53    |
| 74  | 54    |
| 75  | 55    |
| 76  | 56    |
| 77  | 57    |
| 78  | 58    |
| 79  | 59    |
| 80  | 59    |
| 81  | 59    |
| 82  | 59    |
| 83  | 59    |
| 84  | 59    |
| 85  | 59    |
| 86  | 60    |
| 87  | 61    |
| 88  | 61    |
| 89  | 61    |
| 90  | 62    |
| 91  | 63    |
| 92  | 64    |
| 93  | 65    |
| 94  | 65    |
| 95  | 66    |
| 96  | 67    |
| 97  | 68    |
| 98  | 69    |
| 99  | 70    |
| 100 | 71    |
| 101 | 72    |
| 102 | 73    |
| 103 | 74    |
| 104 | 75    |
| 105 | 75    |
| 106 | 75    |
| 107 | 76    |
| 108 | 77    |
| 109 | 78    |
| 110 | 79    |
| 111 | 80    |
| 112 | 80    |
| 113 | 81    |
| 114 | 82    |
| 115 | 83    |
| 116 | 83    |
| 117 | 84    |
| 118 | 84    |
| 119 | 84    |
| 120 | 84    |
| 121 | 85    |
| 122 | 86    |
| 123 | 87    |
| 124 | 88    |
| 125 | 88    |
| 126 | 88    |
| 127 | 89    |
| 128 | 90    |
| 129 | 90    |
| 130 | 91    |
| 131 | 91    |
| 132 | 92    |
| 133 | 93    |
| 134 | 94    |
| 135 | 95    |
| 136 | 96    |
| 137 | 97    |
| 138 | 98    |
| 139 | 99    |
| 140 | 100   |
| 141 | 101   |
| 142 | 101   |
| 143 | 101   |
| 144 | 102   |
| 145 | 103   |
| 146 | 104   |
| 147 | 105   |
| 148 | 106   |
| 149 | 107   |
| 150 | 108   |
| 151 | 109   |
| 152 | 110   |
| 153 | 110   |
| 154 | 111   |
| 155 | 112   |
| 156 | 113   |
| 157 | 114   |
| 158 | 115   |
| 159 | 116   |
| 160 | 117   |
| 161 | 118   |
| 162 | 119   |
| 163 | 120   |
| 164 | 121   |
| 165 | 121   |
| 166 | 121   |
| 167 | 122   |
| 168 | 122   |
+-----+-------+

## lineBreakAmbiguousClasses

```
@brief AI tailoring per language (libunibreak linebreak.c resolve_lb_class).

AI resolves to ID for Chinese, Japanese and Korean, AL otherwise.
```

+------+-------+
| key  | value |
+======+=======+
| @und | @AL   |
| @de  | @AL   |
| @en  | @AL   |
| @es  | @AL   |
| @fr  | @AL   |
| @ja  | @ID   |
| @ko  | @ID   |
| @ru  | @AL   |
| @zh  | @ID   |
+------+-------+

## lineBreakTailoringLanguages

```
@brief Language of each per-language QU override row (libunibreak
linebreakdef.c). Row N of this table, `lineBreakTailoringCodepoints` and
`lineBreakTailoringClasses` together describe one override.
```

+-----+-------+
| key | value |
+=====+=======+
| 0   | @en   |
| 1   | @en   |
| 2   | @en   |
| 3   | @de   |
| 4   | @de   |
| 5   | @de   |
| 6   | @de   |
| 7   | @de   |
| 8   | @de   |
| 9   | @es   |
| 10  | @es   |
| 11  | @es   |
| 12  | @es   |
| 13  | @es   |
| 14  | @es   |
| 15  | @es   |
| 16  | @fr   |
| 17  | @fr   |
| 18  | @fr   |
| 19  | @fr   |
| 20  | @fr   |
| 21  | @fr   |
| 22  | @fr   |
| 23  | @ru   |
| 24  | @ru   |
| 25  | @ru   |
| 26  | @zh   |
| 27  | @zh   |
| 28  | @zh   |
| 29  | @zh   |
+-----+-------+

## lineBreakTailoringCodepoints

```
@brief Codepoint of each per-language QU override row (libunibreak
linebreakdef.c).
```

+-----+--------+
| key | value  |
+=====+========+
| 0   | 0x2018 |
| 1   | 0x201c |
| 2   | 0x201d |
| 3   | 0xab   |
| 4   | 0xbb   |
| 5   | 0x2018 |
| 6   | 0x201c |
| 7   | 0x2039 |
| 8   | 0x203a |
| 9   | 0xab   |
| 10  | 0xbb   |
| 11  | 0x2018 |
| 12  | 0x201c |
| 13  | 0x201d |
| 14  | 0x2039 |
| 15  | 0x203a |
| 16  | 0xab   |
| 17  | 0xbb   |
| 18  | 0x2018 |
| 19  | 0x201c |
| 20  | 0x201d |
| 21  | 0x2039 |
| 22  | 0x203a |
| 23  | 0xab   |
| 24  | 0xbb   |
| 25  | 0x201c |
| 26  | 0x2018 |
| 27  | 0x2019 |
| 28  | 0x201c |
| 29  | 0x201d |
+-----+--------+

## lineBreakTailoringClasses

```
@brief Resolved class of each per-language QU override row (libunibreak
linebreakdef.c).
```

+-----+-------+
| key | value |
+=====+=======+
| 0   | @OP   |
| 1   | @OP   |
| 2   | @CL   |
| 3   | @CL   |
| 4   | @OP   |
| 5   | @CL   |
| 6   | @CL   |
| 7   | @CL   |
| 8   | @OP   |
| 9   | @OP   |
| 10  | @CL   |
| 11  | @OP   |
| 12  | @OP   |
| 13  | @CL   |
| 14  | @OP   |
| 15  | @CL   |
| 16  | @OP   |
| 17  | @CL   |
| 18  | @OP   |
| 19  | @OP   |
| 20  | @CL   |
| 21  | @OP   |
| 22  | @CL   |
| 23  | @OP   |
| 24  | @CL   |
| 25  | @CL   |
| 26  | @OP   |
| 27  | @CL   |
| 28  | @OP   |
| 29  | @CL   |
+-----+-------+

## lineBreakAdjacentMasks

```
@brief Break mask for two adjacent codepoints (UAX #14 LB11-LB31, priority
order, tr14-55 :3943-4214). Bit n of the value set means the class
`map::LineBreak` value n directly after the key gets no break.
```

+-----+-----------------+
| key | value           |
+=====+=================+
| @WJ | 0x1ffffffffffff |
| @GL | 0x1ffffffffffff |
| @B2 | 0x113ed940      |
| @BA | 0x113ed040      |
| @BB | 0x1ffffffffffff |
| @HY | 0x133ed040      |
| @HH | 0x113ed040      |
| @CB | 0x113ed140      |
| @CL | 0x113ed140      |
| @CP | 0x113ed140      |
| @EX | 0x113ed140      |
| @IN | 0x113ed140      |
| @NS | 0x113ed140      |
| @OP | 0x1ffffffffffff |
| @QU | 0x113ed140      |
| @IS | 0x80933ed140    |
| @NU | 0x809f3ed140    |
| @PO | 0x80933ed140    |
| @PR | 0xff8933ed140   |
| @SY | 0x80113ed140    |
| @AK | 0xc000113ed140  |
| @AL | 0x809f3ed140    |
| @AP | 0x2513ed140     |
| @AS | 0xc000113ed140  |
| @EB | 0x10153ed140    |
| @EM | 0x153ed140      |
| @H2 | 0xc00153ed140   |
| @H3 | 0x800153ed140   |
| @HL | 0x809f3ed140    |
| @ID | 0x153ed140      |
| @JL | 0x660153ed140   |
| @JV | 0xc00153ed140   |
| @JT | 0x800153ed140   |
| @RI | 0x113ed140      |
| @VF | 0x113ed140      |
| @VI | 0x113ed140      |
+-----+-----------------+

## lineBreakSpacedMasks

```
@brief Break mask across a run of spaces (UAX #14 LB11, LB13, LB14, LB15d,
LB16, LB17, tr14-55 :3781-3785, :3943-4021, :4010-4012). Bit n of the value
set means the class `map::LineBreak` value n after the space run gets no
break. LB15c's `SP div IS NU` exception is coded in the reflow rules, not
in this table.
```

+-----+-----------------+
| key | value           |
+=====+=================+
| @WJ | 0x110e0040      |
| @GL | 0x110e0040      |
| @B2 | 0x110e0840      |
| @BA | 0x110e0040      |
| @BB | 0x110e0040      |
| @HY | 0x110e0040      |
| @HH | 0x110e0040      |
| @CB | 0x110e0040      |
| @CL | 0x112e0040      |
| @CP | 0x112e0040      |
| @EX | 0x110e0040      |
| @IN | 0x110e0040      |
| @NS | 0x110e0040      |
| @OP | 0x1ffffffffffff |
| @QU | 0x110e0040      |
| @IS | 0x110e0040      |
| @NU | 0x110e0040      |
| @PO | 0x110e0040      |
| @PR | 0x110e0040      |
| @SY | 0x110e0040      |
| @AK | 0x110e0040      |
| @AL | 0x110e0040      |
| @AP | 0x110e0040      |
| @AS | 0x110e0040      |
| @EB | 0x110e0040      |
| @EM | 0x110e0040      |
| @H2 | 0x110e0040      |
| @H3 | 0x110e0040      |
| @HL | 0x110e0040      |
| @ID | 0x110e0040      |
| @JL | 0x110e0040      |
| @JV | 0x110e0040      |
| @JT | 0x110e0040      |
| @RI | 0x110e0040      |
| @VF | 0x110e0040      |
| @VI | 0x110e0040      |
+-----+-----------------+
