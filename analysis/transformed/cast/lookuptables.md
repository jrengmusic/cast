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

+------------+----------------------+
| alias      | symbol               |
+============+======================+
| @black     | 0xff000000           |
| @red       | 0xffff0000           |
| @green     | 0xff00ff00           |
| @yellow    | 0xffffff00           |
| @magenta   | 0xffff00ff           |
| @aqua      | 0xff00ffff           |
| @white     | 0xffffffff           |
| @operators | map::Byte::operators |
| @quote     | map::Byte::quote     |
+------------+----------------------+

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
