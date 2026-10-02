## index

+---------+------------------+
| alias   | symbol           |
+=========+==================+
| @id     | juce::Identifier |
| @string | juce::String     |
+---------+------------------+

## identifiers

```
@brief Identifier and transform-name constants CAST stamps and reads.

juce::Identifier entries are the provenance and state keys the engine
stamps at parse and reads by name (§11.1). juce::String entries name the
transform operations a format cell may declare (§8).
```

+---------+------------------+----------------------+-------------------------------------------------------------+
| type    | name             | value                | description                                                 |
+=========+==================+======================+=============================================================+
| @id     | argument         | `argument`           | Toolchain table argument column.                            |
| @id     | assumeFilename   | `assume-filename`    | --assume-filename CLI flag word — stdin style search start. |
| @id     | background       | `background`         | --pack background option word.                              |
| @string | backgroundPrefix | `.background`        | Dmg background image file-name prefix.                      |
| @id     | banner           | `banner`             | Banner artwork key.                                         |
| @id     | bannerClose      | `bannerClose`        |                                                             |
| @id     | bannerOpen       | `bannerOpen`         |                                                             |
| @id     | begin            | `begin`              |                                                             |
| @id     | blockClose       | `blockClose`         |                                                             |
| @id     | blockLine        | `blockLine`          | Block-comment continuation-line glyph key.                  |
| @id     | blockOpen        | `blockOpen`          |                                                             |
| @id     | bundleColumn     | `bundle-column`      | --pack layout option word.                                  |
| @id     | comment          | `comment`            |                                                             |
| @id     | date             | `date`               | Reserved date token name.                                   |
| @id     | description      | `description`        |                                                             |
| @string | dmg              | `dmg`                | Dmg archive extension.                                      |
| @id     | doubleDash       | `--`                 | Double-dash delimiter.                                      |
| @string | dsStore          | `.DS_Store`          | Finder layout file name.                                    |
| @id     | firstRow         | `first-row`          | --pack layout option word.                                  |
| @string | hdiutil          | `hdiutil`            | Dmg image tool command.                                     |
| @id     | help             | `help`               |                                                             |
| @id     | iconSize         | `icon-size`          | --pack layout option word.                                  |
| @id     | link             | `link`               | --pack link option word.                                    |
| @id     | linkColumn       | `link-column`        | --pack layout option word.                                  |
| @id     | p                | `p`                  |                                                             |
| @id     | pack             | `pack`               | --pack CLI flag word.                                       |
| @id     | rowSpacing       | `row-spacing`        | --pack layout option word.                                  |
| @id     | syncBoundary     | `boundary`           | Sync identity-row boundary column key.                      |
| @id     | brief            | `brief`              | Brief documentation key.                                    |
| @id     | command          | `command`            | Toolchain table command column.                             |
| @id     | filePrefix       | `filePrefix`         | Sync composed identity key — file-name prefix.              |
| @id     | flag             | `flag`               | Toolchain table flag column.                                |
| @string | fromCodepoint    | `from codepoint`     | Codepoint decode operation.                                 |
| @string | fromUTF8         | `from UTF8`          | UTF-8 decode operation.                                     |
| @id     | identity         | `identity`           | Sync info-file identity table name.                         |
| @id     | ignore           | `ignore`             | Sync info-file ignore table name.                           |
| @id     | inPlace          | `i`                  | -i CLI flag word — format in place.                         |
| @string | join             | `join`               | Join text operation.                                        |
| @id     | kernel           | `kernel`             | Sync module-row class keyword — a walked directory.         |
| @id     | lineWrap         | `line-wrap`          | Prose reflow width CLI flag word.                           |
| @id     | list             | `list`               | Reserved expansion token name.                              |
| @id     | macroPrefix      | `macroPrefix`        | Sync composed identity key — macro-name prefix.             |
| @id     | noBanner         | `no-banner`          | Banner-suppressing fence-prefix marker.                     |
| @id     | noFormat         | `no-format`          | Formatless-column marker.                                   |
| @id     | placeholder      | `placeholder`        | Placeholder token name.                                     |
| @id     | separator        | `separator`          | Separator column key.                                       |
| @id     | structure        | `structure`          | Structure column key.                                       |
| @id     | symbol           | `symbol`             | Index symbol column key.                                    |
| @id     | sync             | `sync`               | --sync CLI flag word.                                       |
| @id     | templatePath     | `template`           | Template file path stamp.                                   |
| @string | toCamel          | `to camel`           | camelCase operation.                                        |
| @string | toCodepoint      | `to codepoint`       | Codepoint encode operation.                                 |
| @id     | toComment        | `toComment`          |                                                             |
| @id     | toCommentBlock   | `toCommentBlock`     |                                                             |
| @string | toFileName       | `to file name`       | File-name transform operation.                              |
| @string | toHex            | `to hex`             | Hex encode operation.                                       |
| @string | toKebab          | `to kebab`           | kebab-case operation.                                       |
| @id     | tokenModule      | `module`             |                                                             |
| @string | toLiteral        | `to literal`         | Literal delimiting/escaping operation.                      |
| @id     | toolchain        | `toolchain`          | Reserved toolchain manifest table.                          |
| @string | toPascal         | `to pascal`          | PascalCase operation.                                       |
| @string | toScreamingSnake | `to screaming snake` | SCREAMING_SNAKE_CASE operation.                             |
| @string | toSnake          | `to snake`           | snake_case operation.                                       |
| @string | toTitle          | `to title`           | Title Case operation.                                       |
| @string | toUpper          | `to upper`           | UPPERCASE operation.                                        |
| @string | toUTF8           | `to UTF8`            | UTF-8 encode operation.                                     |
| @id     | windowLeft       | `window-left`        | --pack layout option word.                                  |
| @id     | windowTop        | `window-top`         | --pack layout option word.                                  |
| @id     | wiring           | `wiring`             | Manifest wiring-table classification.                       |
| @id     | word             | `word`               | Sync identity-row boundary keyword — whole-word matching.   |
| @string | zip              | `zip`                | Zip archive extension.                                      |
+---------+------------------+----------------------+-------------------------------------------------------------+
