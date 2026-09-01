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

+---------+------------------+----------------------+--------------------------------------------+
| type    | name             | value                | comment                                    |
+=========+==================+======================+============================================+
| @id     | argument         | `argument`           | Toolchain table argument column.           |
| @id     | banner           | `banner`             | Banner artwork key.                        |
| @id     | blockLine        | `blockLine`          | Block-comment continuation-line glyph key. |
| @id     | brief            | `brief`              | Brief documentation key.                   |
| @id     | command          | `command`            | Toolchain table command column.            |
| @id     | flag             | `flag`               | Toolchain table flag column.               |
| @string | fromCodepoint    | `from codepoint`     | Codepoint decode operation.                |
| @string | fromUTF8         | `from UTF8`          | UTF-8 decode operation.                    |
| @string | join             | `join`               | Join text operation.                       |
| @id     | key              | `key`                | Identity column key.                       |
| @id     | list             | `list`               | Reserved expansion token name.             |
| @id     | noFormat         | `no-format`          | Formatless-column marker.                  |
| @id     | placeholder      | `placeholder`        | Placeholder token name.                    |
| @id     | separator        | `separator`          | Separator column key.                      |
| @id     | structure        | `structure`          | Structure column key.                      |
| @id     | symbol           | `symbol`             | Index symbol column key.                   |
| @id     | templatePath     | `template`           | Template file path stamp.                  |
| @string | toCamel          | `to camel`           | camelCase operation.                       |
| @string | toCodepoint      | `to codepoint`       | Codepoint encode operation.                |
| @string | toFileName       | `to file name`       | File-name transform operation.             |
| @string | toHex            | `to hex`             | Hex encode operation.                      |
| @string | toKebab          | `to kebab`           | kebab-case operation.                      |
| @string | toLiteral        | `to literal`         | Literal delimiting/escaping operation.     |
| @id     | toolchain        | `toolchain`          | Reserved toolchain manifest table.         |
| @string | toPascal         | `to pascal`          | PascalCase operation.                      |
| @string | toScreamingSnake | `to screaming snake` | SCREAMING_SNAKE_CASE operation.            |
| @string | toSnake          | `to snake`           | snake_case operation.                      |
| @string | toTitle          | `to title`           | Title Case operation.                      |
| @string | toUpper          | `to upper`           | UPPERCASE operation.                       |
| @string | toUTF8           | `to UTF8`            | UTF-8 encode operation.                    |
| @id     | wiring           | `wiring`             | Manifest wiring-table classification.      |
+---------+------------------+----------------------+--------------------------------------------+
