```
████████████░░████████████░░████████████░░████████████░░
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░
████░░        ████░░  ████░░████░░            ████░░
████░░        ████████████░░████████████░░    ████░░
████░░        ████░░  ████░░        ████░░    ████░░
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░
████████████░░████░░  ████░░████████████░░    ████░░
```

## TaperMap

```
@brief Curve taper values — the log/alog series plus linear and skew.

Maps each taper preset to the exponent that shapes the parameter curve.
Positive keys are logarithmic (log10 = 10), negative keys are their
anti-log mirrors (alog10 = -10), and `skew` is the 1000-slot out-of-band
sentinel. Consumed by the taper evaluator to shape a normalized input.
```

+--------+------+
| name   | key  |
+========+======+
| linear | 0    |
| skew   | 1000 |
| log45  | 45   |
| log40  | 40   |
| log35  | 35   |
| log30  | 30   |
| log25  | 25   |
| log20  | 20   |
| log15  | 15   |
| log10  | 10   |
| log5   | 5    |
| log4   | 4    |
| log3   | 3    |
| log2   | 2    |
| log1   | 1    |
| alog1  | -1   |
| alog2  | -2   |
| alog3  | -3   |
| alog4  | -4   |
| alog5  | -5   |
| alog10 | -10  |
| alog15 | -15  |
| alog20 | -20  |
| alog25 | -25  |
| alog30 | -30  |
| alog35 | -35  |
| alog40 | -40  |
| alog45 | -45  |
+--------+------+

## XmlTokenType

```
@brief XML token kinds — start tag, end tag, text, processing instruction, declaration, and comment.

Classifies each token the XML tokeniser emits. `startTag` and `endTag`
bracket an element; `text` is the character data between them.
`processingInstruction` and `declaration` are markup instructions discarded
during tree construction; `comment` is a `<!-- -->` span likewise discarded.
The key is the discriminator the parser dispatches on when building the
element tree.
```

+-----------------------+-----+
| name                  | key |
+=======================+=====+
| startTag              | 0   |
| endTag                | 1   |
| text                  | 2   |
| processingInstruction | 3   |
| declaration           | 4   |
| comment               | 5   |
+-----------------------+-----+

## Segment

```
@brief UI nine-segment anchor positions.

Maps each of the nine slice anchors (centre plus the four corners and four
edges) to its key. Consumed by nine-slice resizing to pick which regions
stretch and which stay fixed when a component is scaled.
```

+-------------+-----+
| name        | key |
+=============+=====+
| centre      | 0   |
| topLeft     | 1   |
| top         | 2   |
| topRight    | 3   |
| left        | 4   |
| right       | 5   |
| bottomLeft  | 6   |
| bottom      | 7   |
| bottomRight | 8   |
+-------------+-----+

## ButtonState

```
@brief Button visual states — base states and their toggled-on combinations.

Maps each of the eight button states (normal, over, down, disabled, and the
`…On` variants of each) to its key. The key indexes the per-state image or
style the button draws for a given mouse/toggle combination.
```

+------------+-----+
| name       | key |
+============+=====+
| normal     | 0   |
| over       | 1   |
| down       | 2   |
| disabled   | 3   |
| normalOn   | 4   |
| overOn     | 5   |
| downOn     | 6   |
| disabledOn | 7   |
+------------+-----+

## Position

```
@brief Layout position anchors — top/bottom/left/right/centre.

Maps each anchor name to its key, consumed by layout code to align a child
within its parent's bounds.
```

+--------+-----+
| name   | key |
+========+=====+
| bottom | 0   |
| top    | 1   |
| right  | 2   |
| left   | 3   |
| center | 4   |
+--------+-----+

## Orientation

```
@brief Axis orientation — vertical or horizontal.

Maps each orientation name to its key, consumed by layout and draw code to
select the primary axis along which a list or control flows.
```

+------------+-----+
| name       | key |
+============+=====+
| vertical   | 0   |
| horizontal | 1   |
+------------+-----+

## windowFX mac

```
@brief Native window effects, per platform.

Maps each window-effect name to its key. The entries are compiled
conditionally: macOS exposes background-blur and glass effects, Windows
exposes blur-behind and acrylic. Consumed by the native windowing layer to
request the matching platform effect.
```

+--------------------------+-----+
| name                     | key |
+==========================+=====+
| backgroundBlur           | 0   |
| visualFXWindowBackground | 1   |
| glassFXRegular           | 2   |
| glassFXClear             | 3   |
+--------------------------+-----+

## windowFX windows

+------------+-----+
| name       | key |
+============+=====+
| blurBehind | 0   |
| acrylic    | 1   |
+------------+-----+

## BlockType

```
@brief Markdown block element kinds.

Maps each block-level construct the Markdown parser produces — paragraph,
heading, blockquote, code block, list and its items, thematic break, HTML
block, table and its rows/cells, mermaid diagram, and image — to its key.
The key is the block discriminator the document model stores per node.
```

+---------------+-----+
| name          | key |
+===============+=====+
| paragraph     | 0   |
| document      | 1   |
| heading       | 2   |
| blockquote    | 3   |
| codeBlock     | 4   |
| list          | 5   |
| listItem      | 6   |
| thematicBreak | 7   |
| htmlBlock     | 8   |
| table         | 9   |
| tableRow      | 10  |
| tableCell     | 11  |
| mermaid       | 12  |
| image         | 13  |
| tableBorder   | 14  |
+---------------+-----+

## MarkdownTokenType

```
@brief Markdown inline-token kinds.

Maps each inline construct the Markdown parser emits — text, character
reference, code span, emphasis delimiter, link open/close, image open,
autolink, raw HTML, and line break — to its key. The key is the token
discriminator consumed when building inline spans.
```

+--------------------+-----+
| name               | key |
+====================+=====+
| text               | 0   |
| characterReference | 1   |
| codeSpan           | 2   |
| emphasisDelimiter  | 3   |
| linkOpen           | 4   |
| imageOpen          | 5   |
| linkClose          | 6   |
| autolink           | 7   |
| rawHtml            | 8   |
| lineBreak          | 9   |
+--------------------+-----+

## DocumentTokenType

```
@brief Document token kinds — text, operators, region, comment.

Classifies each token the generic document scanner emits. `operators` covers
punctuation and structural delimiters; `region` marks region open/close
bracketing; `comment` marks comment content. The key drives syntax
highlighting and token dispatch.
```

+-----------+-----+
| name      | key |
+===========+=====+
| text      | 0   |
| operators | 1   |
| region    | 2   |
| comment   | 3   |
+-----------+-----+

## Byte

```
@brief Byte classification for syntax scanning.

Buckets each byte the scanners look at into text, operator, region-open,
region-close, or quote. The key is the classification the byte-to-class
lookup tables store per codepoint, letting the tokeniser skip re-testing a
byte on every pass.
```

+-------------+-----+
| name        | key |
+=============+=====+
| text        | 0   |
| operators   | 1   |
| regionOpen  | 2   |
| regionClose | 3   |
| quote       | 4   |
+-------------+-----+

## HeadingLevel

```
@brief Heading levels one through six.

Maps each heading level to its numeric key, which doubles as the heading
depth the document model records and the renderer uses for font sizing.
```

+--------+-----+
| name   | key |
+========+=====+
| level1 | 1   |
| level2 | 2   |
| level3 | 3   |
| level4 | 4   |
| level5 | 5   |
| level6 | 6   |
+--------+-----+

## HtmlType1Tag

```
@brief Raw-text HTML tag kinds (script/pre/textarea/style).

Maps the four HTML elements whose content is treated as raw text rather
than parsed markup to their keys. The key indexes the closing-tag table
(`map::rawTextEnd`) that terminates the raw-text run.
```

+----------+-----+
| name     | key |
+==========+=====+
| script   | 0   |
| pre      | 1   |
| textarea | 2   |
| style    | 3   |
+----------+-----+

## HtmlBlockTag

```
@brief Block-level HTML element tags.

Maps each block-level HTML tag name to its key. The key is the tag
discriminator used when the HTML parser classifies a start tag as a block
element; the ordinal values match the CommonMark block-tag table.
```

+------------+-----+--------------+
| name       | key | value        |
+============+=====+==============+
| address    | 0   | `address`    |
| article    | 1   | `article`    |
| aside      | 2   | `aside`      |
| basefont   | 4   | `basefont`   |
| blockquote | 5   | `blockquote` |
| body       | 6   | `body`       |
| caption    | 7   | `caption`    |
| center     | 8   | `center`     |
| colgroup   | 10  | `colgroup`   |
| dd         | 11  | `dd`         |
| details    | 12  | `details`    |
| dialog     | 13  | `dialog`     |
| dir        | 14  | `dir`        |
| dl         | 16  | `dl`         |
| dt         | 17  | `dt`         |
| fieldset   | 18  | `fieldset`   |
| figcaption | 19  | `figcaption` |
| figure     | 20  | `figure`     |
| footer     | 21  | `footer`     |
| form       | 22  | `form`       |
| frame      | 23  | `frame`      |
| frameset   | 24  | `frameset`   |
| h1         | 25  | `h1`         |
| h2         | 26  | `h2`         |
| h3         | 27  | `h3`         |
| h4         | 28  | `h4`         |
| h5         | 29  | `h5`         |
| h6         | 30  | `h6`         |
| head       | 31  | `head`       |
| header     | 32  | `header`     |
| html       | 34  | `html`       |
| iframe     | 35  | `iframe`     |
| legend     | 36  | `legend`     |
| li         | 37  | `li`         |
| menu       | 40  | `menu`       |
| menuitem   | 41  | `menuitem`   |
| nav        | 42  | `nav`        |
| noframes   | 43  | `noframes`   |
| ol         | 44  | `ol`         |
| optgroup   | 45  | `optgroup`   |
| option     | 46  | `option`     |
| lowerP     | 47  | `p`          |
| param      | 48  | `param`      |
| section    | 49  | `section`    |
| search     | 50  | `search`     |
| title      | 51  | `title`      |
| summary    | 52  | `summary`    |
| table      | 53  | `table`      |
| tbody      | 54  | `tbody`      |
| td         | 55  | `td`         |
| tfoot      | 56  | `tfoot`      |
| th         | 57  | `th`         |
| thead      | 58  | `thead`      |
| tr         | 59  | `tr`         |
| ul         | 61  | `ul`         |
+------------+-----+--------------+

## SyntaxTokenType

```
@brief Syntax-highlight token kinds.

Maps each highlight category — keyword, string, comment, number, and
punctuation — to its key. The key is the token class the syntax highlighter
maps to a colour in the active theme.
```

+-------------+-----+
| name        | key |
+=============+=====+
| keyword     | 0   |
| string      | 1   |
| comment     | 2   |
| number      | 3   |
| punctuation | 4   |
+-------------+-----+

## Family

```
@brief Language family classification.

Maps each language family — C-like, Python-like, shell, markup, data, and
JavaScript — to its key. The key is the family discriminator a file
extension resolves to via `map::languageFamily`, selecting which scanner
the code view runs.
```

+--------+-----+
| name   | key |
+========+=====+
| cLang  | 0   |
| python | 1   |
| shell  | 2   |
| markup | 3   |
| data   | 4   |
| js     | 5   |
+--------+-----+

## OpenBlock

```
@brief Markdown block-open constructs.

Maps the constructs that can open a block at the start of a line — thematic
break, ATX heading, fenced code, HTML block, reference definition, and grid
table — to their keys. The key is the opener discriminator the block parser
dispatches on before reading the block body.
```

+---------------------+-----+
| name                | key |
+=====================+=====+
| thematicBreak       | 0   |
| atxHeading          | 1   |
| fencedCode          | 2   |
| htmlBlock           | 3   |
| referenceDefinition | 4   |
| gridTable           | 5   |
+---------------------+-----+

## CppKeyword

```
@brief C++ keyword set.

Maps every C++ keyword (including alternative-token spellings and the C++20
`co_*`/`concept`/`char8_t` additions) to its key. The key is the token class
the C++ scanner marks as a keyword for highlighting.
```

+-----------------------+-----+--------------------+
| name                  | key | value              |
+=======================+=====+====================+
| tokenAlignas          | 0   | `alignas`          |
| tokenAlignof          | 1   | `alignof`          |
| tokenAnd              | 2   | `and`              |
| tokenAnd_eq           | 3   | `and_eq`           |
| tokenAsm              | 4   | `asm`              |
| tokenAuto             | 5   | `auto`             |
| tokenBitand           | 6   | `bitand`           |
| tokenBitor            | 7   | `bitor`            |
| tokenBool             | 8   | `bool`             |
| tokenBreak            | 9   | `break`            |
| tokenCase             | 10  | `case`             |
| tokenCatch            | 11  | `catch`            |
| tokenChar             | 12  | `char`             |
| tokenClass            | 13  | `class`            |
| tokenCompl            | 14  | `compl`            |
| tokenConst            | 15  | `const`            |
| tokenConst_cast       | 16  | `const_cast`       |
| tokenConstexpr        | 17  | `constexpr`        |
| tokenConsteval        | 18  | `consteval`        |
| tokenConstinit        | 19  | `constinit`        |
| tokenContinue         | 20  | `continue`         |
| tokenCo_await         | 21  | `co_await`         |
| tokenCo_return        | 22  | `co_return`        |
| tokenCo_yield         | 23  | `co_yield`         |
| tokenDecltype         | 24  | `decltype`         |
| tokenDefault          | 25  | `default`          |
| tokenDelete           | 26  | `delete`           |
| tokenDo               | 27  | `do`               |
| tokenDouble           | 28  | `double`           |
| tokenDynamic_cast     | 29  | `dynamic_cast`     |
| tokenElse             | 30  | `else`             |
| tokenEnum             | 31  | `enum`             |
| tokenExplicit         | 32  | `explicit`         |
| tokenExport           | 33  | `export`           |
| tokenExtern           | 34  | `extern`           |
| tokenFalse            | 35  | `false`            |
| tokenFloat            | 36  | `float`            |
| tokenFor              | 37  | `for`              |
| tokenFriend           | 38  | `friend`           |
| tokenGoto             | 39  | `goto`             |
| tokenIf               | 40  | `if`               |
| tokenInline           | 41  | `inline`           |
| tokenInt              | 42  | `int`              |
| tokenLong             | 43  | `long`             |
| tokenMutable          | 44  | `mutable`          |
| tokenNamespace        | 45  | `namespace`        |
| tokenNew              | 46  | `new`              |
| tokenNoexcept         | 47  | `noexcept`         |
| tokenNot              | 48  | `not`              |
| tokenNot_eq           | 49  | `not_eq`           |
| tokenNullptr          | 50  | `nullptr`          |
| tokenOperator         | 51  | `operator`         |
| tokenOr               | 52  | `or`               |
| tokenOr_eq            | 53  | `or_eq`            |
| tokenPrivate          | 54  | `private`          |
| tokenProtected        | 55  | `protected`        |
| tokenPublic           | 56  | `public`           |
| tokenRegister         | 57  | `register`         |
| tokenReinterpret_cast | 58  | `reinterpret_cast` |
| tokenRequires         | 59  | `requires`         |
| tokenReturn           | 60  | `return`           |
| tokenShort            | 61  | `short`            |
| tokenSigned           | 62  | `signed`           |
| tokenSizeof           | 63  | `sizeof`           |
| tokenStatic           | 64  | `static`           |
| tokenStatic_assert    | 65  | `static_assert`    |
| tokenStatic_cast      | 66  | `static_cast`      |
| tokenStruct           | 67  | `struct`           |
| tokenSwitch           | 68  | `switch`           |
| tokenTemplate         | 69  | `template`         |
| tokenThis             | 70  | `this`             |
| tokenThread_local     | 71  | `thread_local`     |
| tokenThrow            | 72  | `throw`            |
| tokenTrue             | 73  | `true`             |
| tokenTry              | 74  | `try`              |
| tokenTypedef          | 75  | `typedef`          |
| tokenTypeid           | 76  | `typeid`           |
| tokenTypename         | 77  | `typename`         |
| tokenUnion            | 78  | `union`            |
| tokenUnsigned         | 79  | `unsigned`         |
| tokenUsing            | 80  | `using`            |
| tokenVirtual          | 81  | `virtual`          |
| tokenVoid             | 82  | `void`             |
| tokenVolatile         | 83  | `volatile`         |
| tokenWchar_t          | 84  | `wchar_t`          |
| tokenWhile            | 85  | `while`            |
| tokenXor              | 86  | `xor`              |
| tokenXor_eq           | 87  | `xor_eq`           |
| tokenOverride         | 88  | `override`         |
| tokenFinal            | 89  | `final`            |
| import                | 90  | `import`           |
| tokenModule           | 91  | `module`           |
| tokenConcept          | 92  | `concept`          |
| tokenChar8_t          | 93  | `char8_t`          |
| tokenChar16_t         | 94  | `char16_t`         |
| tokenChar32_t         | 95  | `char32_t`         |
+-----------------------+-----+--------------------+

## JsKeyword

```
@brief JavaScript keyword set.

Maps every JavaScript reserved word and contextual keyword (including the
module-syntax and TypeScript additions `import`, `from`, `type`,
`interface`, `enum`, `implements`) to its key. The key is the token class
the JS scanner marks as a keyword for highlighting.
```

+----------------+-----+--------------+
| name           | key | value        |
+================+=====+==============+
| await          | 0   | `await`      |
| tokenBreak     | 1   | `break`      |
| tokenCase      | 2   | `case`       |
| tokenCatch     | 3   | `catch`      |
| tokenClass     | 4   | `class`      |
| tokenConst     | 5   | `const`      |
| tokenContinue  | 6   | `continue`   |
| debugger       | 7   | `debugger`   |
| tokenDefault   | 8   | `default`    |
| tokenDelete    | 9   | `delete`     |
| tokenDo        | 10  | `do`         |
| tokenElse      | 11  | `else`       |
| tokenExport    | 12  | `export`     |
| extends        | 13  | `extends`    |
| tokenFalse     | 14  | `false`      |
| finally        | 15  | `finally`    |
| tokenFor       | 16  | `for`        |
| function       | 17  | `function`   |
| tokenIf        | 18  | `if`         |
| import         | 19  | `import`     |
| in             | 20  | `in`         |
| instanceof     | 21  | `instanceof` |
| let            | 22  | `let`        |
| tokenNew       | 23  | `new`        |
| null           | 24  | `null`       |
| of             | 25  | `of`         |
| tokenReturn    | 26  | `return`     |
| super          | 27  | `super`      |
| tokenSwitch    | 28  | `switch`     |
| tokenThis      | 29  | `this`       |
| tokenThrow     | 30  | `throw`      |
| tokenTrue      | 31  | `true`       |
| tokenTry       | 32  | `try`        |
| typeof         | 33  | `typeof`     |
| undefined      | 34  | `undefined`  |
| var            | 35  | `var`        |
| tokenVoid      | 36  | `void`       |
| tokenWhile     | 37  | `while`      |
| yield          | 38  | `yield`      |
| async          | 39  | `async`      |
| from           | 40  | `from`       |
| as             | 41  | `as`         |
| type           | 42  | `type`       |
| tokenInterface | 43  | `interface`  |
| tokenEnum      | 44  | `enum`       |
| implements     | 45  | `implements` |
+----------------+-----+--------------+

## PythonKeyword

```
@brief Python keyword set.

Maps every Python keyword to its key. The key is the token class the Python
scanner marks as a keyword for highlighting.
```

+---------------+-----+------------+
| name          | key | value      |
+===============+=====+============+
| tokenFalse    | 0   | `false`    |
| none          | 1   | `none`     |
| tokenTrue     | 2   | `true`     |
| tokenAnd      | 3   | `and`      |
| as            | 4   | `as`       |
| tokenAssert   | 5   | `assert`   |
| async         | 6   | `async`    |
| await         | 7   | `await`    |
| tokenBreak    | 8   | `break`    |
| tokenClass    | 9   | `class`    |
| tokenContinue | 10  | `continue` |
| def           | 11  | `def`      |
| del           | 12  | `del`      |
| elif          | 13  | `elif`     |
| tokenElse     | 14  | `else`     |
| except        | 15  | `except`   |
| finally       | 16  | `finally`  |
| tokenFor      | 17  | `for`      |
| from          | 18  | `from`     |
| global        | 19  | `global`   |
| tokenIf       | 20  | `if`       |
| import        | 21  | `import`   |
| in            | 22  | `in`       |
| is            | 23  | `is`       |
| lambda        | 24  | `lambda`   |
| nonlocal      | 25  | `nonlocal` |
| tokenNot      | 26  | `not`      |
| tokenOr       | 27  | `or`       |
| pass          | 28  | `pass`     |
| raise         | 29  | `raise`    |
| tokenReturn   | 30  | `return`   |
| tokenTry      | 31  | `try`      |
| tokenWhile    | 32  | `while`    |
| with          | 33  | `with`     |
| yield         | 34  | `yield`    |
+---------------+-----+------------+

## CssTokenType

```
@brief CSS token kinds.

Maps each token kind the CSS tokeniser emits — ident, function, at-keyword,
hash, string (and bad string), url (and bad url), delim, number, percentage,
dimension, whitespace, the bracket/paren/brace pairs, and end-of-file — to
its key. The key is the discriminator the CSS parser dispatches on.
```

+--------------+-----+
| name         | key |
+==============+=====+
| ident        | 0   |
| function     | 1   |
| atKeyword    | 2   |
| hash         | 3   |
| string       | 4   |
| badString    | 5   |
| url          | 6   |
| badUrl       | 7   |
| delim        | 8   |
| number       | 9   |
| percentage   | 10  |
| dimension    | 11  |
| whitespace   | 12  |
| colon        | 13  |
| semicolon    | 14  |
| comma        | 15  |
| openBracket  | 16  |
| closeBracket | 17  |
| openParen    | 18  |
| closeParen   | 19  |
| openBrace    | 20  |
| closeBrace   | 21  |
| endOfFile    | 22  |
+--------------+-----+

## CssRuleType

```
@brief CSS at-rule kinds.

Maps each at-rule the CSS parser recognises — style, charset, import, media,
font-face, page, and namespace (plus the unknown-rule fallback) — to its
key. The key is the rule discriminator used to build the stylesheet model.
```

+---------------+-----+
| name          | key |
+===============+=====+
| styleRule     | 1   |
| unknownRule   | 0   |
| charsetRule   | 2   |
| importRule    | 3   |
| mediaRule     | 4   |
| fontFaceRule  | 5   |
| pageRule      | 6   |
| namespaceRule | 10  |
+---------------+-----+

## HtmlTokenType

```
@brief HTML token kinds.

Maps each token kind the HTML tokeniser emits — start tag, end tag, comment,
character, doctype, and end-of-file — to its key. The key is the
discriminator the HTML parser dispatches on when building the DOM.
```

+-----------+-----+
| name      | key |
+===========+=====+
| startTag  | 0   |
| endTag    | 1   |
| comment   | 2   |
| character | 3   |
| doctype   | 4   |
| endOfFile | 5   |
+-----------+-----+

## HtmlVoidTag

```
@brief HTML void-element tags.

Maps each void HTML element (the tags that cannot have children and must not
be closed) to its key. The key is the discriminator the HTML parser uses to
emit a self-closing node without waiting for an end tag.
```

+--------+-----+
| name   | key |
+========+=====+
| area   | 0   |
| base   | 1   |
| br     | 2   |
| col    | 3   |
| embed  | 4   |
| hr     | 5   |
| img    | 6   |
| input  | 7   |
| link   | 8   |
| meta   | 9   |
| source | 10  |
| track  | 11  |
| wbr    | 12  |
+--------+-----+

## VariDisplayMode

```
@brief Vari display modes.

Maps each Vari knob readout mode — Hertz, note, or kilohertz — to its key.
The key selects the format the parameter display uses for frequency values.
```

+------+-----+
| name | key |
+======+=====+
| hz   | 2   |
| note | 1   |
| khz  | 3   |
+------+-----+

## PluginWrapper

```
@brief Plugin wrapper formats.

Maps each plugin wrapper format — VST, VST3, AU, AUv3, AAX, Standalone,
Unity, and LV2 (plus the undefined fallback) — to its key. The value is the
full attribution string shown in the host's plugin metadata.
```

+------------+-----+------------------------------------------------------------------------------+
| name       | key | value                                                                        |
+============+=====+==============================================================================+
| undefined  | 0   | `Undefined`                                                                  |
| vst        | 1   | `VST by Steinberg Media Technologies, GmbH.`                                 |
| vst3       | 2   | `VST3 by Steinberg Media Technologies, GmbH.`                                |
| au         | 3   | `AU by Apple Computer, Inc.`                                                 |
| auv3       | 4   | `AUv3 by Apple Computer, Inc.`                                               |
| aax        | 5   | `AAX by Avid Technology, Inc.`                                               |
| standalone | 6   | `Standalone`                                                                 |
| unity      | 7   | `Unity Native Audio Plugin by Unity Technologies`                            |
| lv2        | 8   | `LV2 by Steve Harris, David Robillard, and other members of linux-audio-dev` |
+------------+-----+------------------------------------------------------------------------------+

## AnalyzerMode

```
@brief Spectrum analyzer display modes.

Maps each analyzer readout mode — polygon trace or bar graph — to its key.
The key selects the geometry the analyzer draws for its spectrum data.
```

+---------+-----+-----------+
| name    | key | value     |
+=========+=====+===========+
| polygon | 1   | `POLYGON` |
| bars    | 2   | `BARS`    |
+---------+-----+-----------+

## Appearance

```
@brief Application appearance themes.

Maps each appearance theme — light, dark, or automatic OS-following — to its
key. The key selects the colour scheme the UI applies across all components.
```

+-----------+-----+---------+
| name      | key | value   |
+===========+=====+=========+
| automatic | 3   | `AUTO`  |
| light     | 1   | `LIGHT` |
| dark      | 2   | `DARK`  |
+-----------+-----+---------+

## AudioParameter

```
@brief Audio parameter kinds.

Maps each parameter type — floating-point, boolean, choice, and integer — to
its key. The key is the discriminator used when constructing the matching
`juce::AudioProcessorParameter` from a parameter descriptor.
```

+---------------+-----+
| name          | key |
+===============+=====+
| floatingPoint | 0   |
| boolean       | 1   |
| choice        | 2   |
| integer       | 3   |
+---------------+-----+

## Display

```
@brief Parameter display modes.

Maps each parameter display mode — numeric readout or spectrum analyzer — to
its key. The key selects which visualisation a parameter panel shows.
```

+----------+-----+------------+
| name     | key | value      |
+==========+=====+============+
| numbers  | 1   | `NUMBERS`  |
| analyzer | 2   | `ANALYZER` |
+----------+-----+------------+

## FontRasterizerBackend

```
@brief Font rasterizer backends.

Maps each glyph rasterizer — FreeType, edge-table, or the native platform
rasterizer — to its key. The key selects the backend the text engine uses to
rasterize glyph outlines.
```

+-----------+-----+
| name      | key |
+===========+=====+
| freetype  | 0   |
| edgeTable | 1   |
| native    | 2   |
+-----------+-----+

## ImageResample

```
@brief Image resample filters.

Maps each resampling filter — linear or nearest-neighbour — to its key. The
key selects the interpolation the image pipeline uses when resizing.
```

+---------+-----+
| name    | key |
+=========+=====+
| linear  | 0   |
| nearest | 1   |
+---------+-----+

## MouseButton

```
@brief Mouse buttons.

Maps each mouse button — none, left, middle, and right — to its key. The key
is the discriminator a component's mouse handler switches on.
```

+--------+-----+
| name   | key |
+========+=====+
| none   | 0   |
| left   | 1   |
| middle | 2   |
| right  | 3   |
+--------+-----+

## Oversampling

```
@brief Oversampling factors.

Maps each oversampling factor — off, 2x, 4x, and 8x — to its key. The key is
the factor the DSP chain uses to configure its resampling stage.
```

+------+-----+-------+
| name | key | value |
+======+=====+=======+
| off  | 1   | `OFF` |
| x2   | 2   | `x2`  |
| x4   | 3   | `x4`  |
| x8   | 4   | `x8`  |
+------+-----+-------+

## UIScaleMap

```
@brief UI scale presets.

Maps each UI scale preset — mini, small, medium, large, and huge — to its
key. The key is the multiplier index the UI applies to size all components.
```

+------------+-----+----------+
| name       | key | value    |
+============+=====+==========+
| medium     | 3   | `MEDIUM` |
| mini       | 1   | `MINI`   |
| tokenSmall | 2   | `SMALL`  |
| large      | 4   | `LARGE`  |
| huge       | 5   | `HUGE`   |
+------------+-----+----------+

## HtmlStandardTag

```
@brief Standard HTML element tags.

Maps each standard (non-void, non-block) HTML element the HTML parser builds
a normal node for — div, label, button, select, and input — to its key.
```

+--------+-----+
| name   | key |
+========+=====+
| div    | 0   |
| label  | 1   |
| button | 2   |
| select | 3   |
| input  | 4   |
+--------+-----+

## ParameterPage

```
@brief Parameter page labels.

Maps each parameter page (A, B, C) to its key. The key selects which page of
parameters a panel displays.
```

+-------+-----+-------+
| name  | key | value |
+=======+=====+=======+
| pageA | 0   | `A`   |
| pageB | 1   | `B`   |
| pageC | 2   | `C`   |
+-------+-----+-------+

## ViewOrientation

```
@brief Device view orientations.

Maps each device orientation — landscape or portrait — to its key. The key
selects the layout the UI uses for the active device rotation.
```

+-----------+-----+-------------+
| name      | key | value       |
+===========+=====+=============+
| landscape | 1   | `LANDSCAPE` |
| portrait  | 2   | `PORTRAIT`  |
+-----------+-----+-------------+

## AtRuleType

```
@brief CSS at-rule discriminators.

Maps each at-rule discriminator to its key. The key is the rule category the
CSS parser assigns after reading the at-keyword, driving which rule builder
consumes the prelude.
```

+----------------+----------------------------+-------------+
| name           | key                        | value       |
+================+============================+=============+
| fontFace       | CssRuleType::fontFaceRule  | `font-face` |
| media          | CssRuleType::mediaRule     | `media`     |
| import         | CssRuleType::importRule    | `import`    |
| charset        | CssRuleType::charsetRule   | `charset`   |
| page           | CssRuleType::pageRule      | `page`      |
| tokenNamespace | CssRuleType::namespaceRule | `namespace` |
+----------------+----------------------------+-------------+

## BlockTag

```
@brief Markdown block kind to HTML tag map.

Maps each Markdown block kind to the `juce::Identifier` of the HTML element
it serializes to when the document is written out as HTML.
```

+------------+--------------------------+--------------+
| name       | key                      | value        |
+============+==========================+==============+
| body       | BlockType::document      | `body`       |
| p          | BlockType::paragraph     | `p`          |
| blockquote | BlockType::blockquote    | `blockquote` |
| pre        | BlockType::codeBlock     | `pre`        |
| hr         | BlockType::thematicBreak | `hr`         |
| html       | BlockType::htmlBlock     | `html`       |
| table      | BlockType::table         | `table`      |
| tr         | BlockType::tableRow      | `tr`         |
| li         | BlockType::listItem      | `li`         |
| mermaid    | BlockType::mermaid       | `mermaid`    |
| img        | BlockType::image         | `img`        |
+------------+--------------------------+--------------+

## HeadingTag

```
@brief Heading level to HTML tag map.

Maps each heading level to the `juce::Identifier` of the HTML heading
element (`h1` through `h6`) it serializes to.
```

+------+----------------------+-------+
| name | key                  | value |
+======+======================+=======+
| h1   | HeadingLevel::level1 | `h1`  |
| h2   | HeadingLevel::level2 | `h2`  |
| h3   | HeadingLevel::level3 | `h3`  |
| h4   | HeadingLevel::level4 | `h4`  |
| h5   | HeadingLevel::level5 | `h5`  |
| h6   | HeadingLevel::level6 | `h6`  |
+------+----------------------+-------+

## Sharps

```
@brief Sharp key names.

Maps each sharp musical key name (C, C#, D, … B) to its key. The key is the
pitch-class ordinal used by the note-name conversion tables.
```

+--------+-----+-------+
| name   | key | value |
+========+=====+=======+
| c      | 0   | `C`   |
| cSharp | 1   | `C#`  |
| d      | 2   | `D`   |
| dSharp | 3   | `D#`  |
| e      | 4   | `E`   |
| f      | 5   | `F`   |
| fSharp | 6   | `F#`  |
| g      | 7   | `G`   |
| gSharp | 8   | `G#`  |
| a      | 9   | `A`   |
| aSharp | 10  | `A#`  |
| b      | 11  | `B`   |
+--------+-----+-------+

## Flats

```
@brief Flat key names.

Maps each flat musical key name (C, Db, D, … B) to its key. The key is the
pitch-class ordinal used by the note-name conversion tables.
```

+-------+-----+-------+
| name  | key | value |
+=======+=====+=======+
| c     | 0   | `C`   |
| dFlat | 1   | `Db`  |
| d     | 2   | `D`   |
| eFlat | 3   | `Eb`  |
| e     | 4   | `E`   |
| f     | 5   | `F`   |
| gFlat | 6   | `Gb`  |
| g     | 7   | `G`   |
| aFlat | 8   | `Ab`  |
| a     | 9   | `A`   |
| bFlat | 10  | `Bb`  |
| b     | 11  | `B`   |
+-------+-----+-------+

## HtmlBlockType

```
@brief HTML block start-condition kinds (§4.6).

Maps each HTML block start condition — comment, processing instruction,
declaration, and CDATA (plus the none fallback) — to its key. The key
indexes the opening/closing delimiter tables (`map::markupOpen` /
`map::markupClose`) that bracket the raw block.
```

+-----------------------+-----+-------------------------------------------------------------------------+
| name                  | key | comment                                                                 |
+=======================+=====+=========================================================================+
| none                  | 0   | no §4.6 start condition matched                                         |
| rawText               | 1   | §4.6 type 1 — script/pre/textarea/style raw text, ends on its close tag |
| comment               | 2   | §4.6 type 2 — <!-- … -->                                                |
| processingInstruction | 3   | §4.6 type 3 — <? … ?>                                                   |
| declaration           | 4   | §4.6 type 4 — <! + ASCII letter … >                                     |
| cdata                 | 5   | §4.6 type 5 — <![CDATA[ … ]]>                                           |
| blockTag              | 6   | §4.6 type 6 — known block-level tag, ends on blank line                 |
| anyTag                | 7   | §4.6 type 7 — any complete open/closing tag, ends on blank line         |
+-----------------------+-----+-------------------------------------------------------------------------+
