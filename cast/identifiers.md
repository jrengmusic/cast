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
| type    | name             | value                | comment                                                     |
+=========+==================+======================+=============================================================+
| @id     | archive          | `archive`            | Pack table archive column.                                  |
| @id     | argument         | `argument`           | Toolchain table argument column.                            |
| @id     | assumeFilename   | `assume-filename`    | --assume-filename CLI flag word — stdin style search start. |
| @id     | banner           | `banner`             | Banner artwork key.                                         |
| @id     | bannerClose      | `bannerClose`        |                                                             |
| @id     | bannerOpen       | `bannerOpen`         |                                                             |
| @id     | begin            | `begin`              |                                                             |
| @id     | blockClose       | `blockClose`         |                                                             |
| @id     | blockLine        | `blockLine`          | Block-comment continuation-line glyph key.                  |
| @id     | blockOpen        | `blockOpen`          |                                                             |
| @id     | bundleColumn     | `bundleColumn`       | Pack layout key — item x position.                          |
| @id     | comment          | `comment`            |                                                             |
| @string | dmg              | `dmg`                | Dmg archive extension.                                      |
| @id     | doubleDash       | `--`                 | Double-dash delimiter.                                      |
| @string | dsStore          | `.DS_Store`          | Finder layout file name.                                    |
| @id     | firstRow         | `firstRow`           | Pack layout key — first row y position.                     |
| @string | hdiutil          | `hdiutil`            | Dmg image tool command.                                     |
| @id     | help             | `help`               |                                                             |
| @id     | host             | `host`               | Toolchain and pack table host column.                       |
| @id     | iconSize         | `iconSize`           | Pack layout key — icon size.                                |
| @id     | item             | `item`               | Pack table item column.                                     |
| @id     | link             | `link`               | Pack table link target column.                              |
| @id     | linkColumn       | `linkColumn`         | Pack layout key — link x position.                          |
| @id     | linkName         | `linkName`           | Pack table link name column.                                |
| @id     | linux            | `linux`              | Host word — Linux.                                          |
| @id     | mac              | `mac`                | Host word — macOS.                                          |
| @id     | p                | `p`                  |                                                             |
| @id     | pack             | `pack`               | Reserved pack table and toolchain command word.             |
| @id     | packLayout       | `pack_layout`        | Reserved pack layout table id, parsed from `pack layout`.   |
| @id     | rowSpacing       | `rowSpacing`         | Pack layout key — row pitch.                                |
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
| @id     | win              | `win`                | Host word — Windows.                                        |
| @id     | windowLeft       | `windowLeft`         | Pack layout key — window left edge.                         |
| @id     | windowTop        | `windowTop`          | Pack layout key — window top edge.                          |
| @id     | wiring           | `wiring`             | Manifest wiring-table classification.                       |
| @id     | word             | `word`               | Sync identity-row boundary keyword — whole-word matching.   |
| @string | zip              | `zip`                | Zip archive extension.                                      |
+---------+------------------+----------------------+-------------------------------------------------------------+
