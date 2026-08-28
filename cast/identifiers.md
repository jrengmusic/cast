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

+---------+------------------+----------------------+-----------+----------------------------------------+
| type    | name             | value                | format    | comment                                |
+=========+==================+======================+===========+========================================+
| @id     | banner           |                      | toLiteral | Banner artwork key.                    |
| @id     | blockLine        |                      | toLiteral |                                        |
| @id     | brief            |                      | toLiteral | Brief documentation key.               |
| @string | fromCodepoint    | `from codepoint`     |           | Codepoint decode operation.            |
| @string | fromUTF8         | `from UTF8`          |           | UTF-8 decode operation.                |
| @id     | indexComment     | `index comment`      |           |                                        |
| @string | join             |                      | toLiteral | Join text operation.                   |
| @id     | key              |                      | toLiteral |                                        |
| @id     | list             |                      | toLiteral | Reserved expansion token name.         |
| @id     | noFormat         | `no-format`          |           | Formatless-column marker.              |
| @id     | placeholder      |                      | toLiteral | Placeholder token name.                |
| @id     | separator        |                      | toLiteral | Separator column key.                  |
| @id     | structure        |                      | toLiteral | Structure column key.                  |
| @id     | symbol           |                      | toLiteral | Index symbol column key.               |
| @id     | templatePath     | `template`           |           | Template block id stamp.               |
| @string | toCamel          | `to camel`           |           | camelCase operation.                   |
| @string | toCodepoint      | `to codepoint`       |           | Codepoint encode operation.            |
| @string | toFileName       | `to file name`       |           | File-name transform operation.         |
| @string | toHex            | `to hex`             |           | Hex encode operation.                  |
| @string | toKebab          | `to kebab`           |           | kebab-case operation.                  |
| @string | toLiteral        | `to literal`         |           | Literal delimiting/escaping operation. |
| @string | toPascal         | `to pascal`          |           | PascalCase operation.                  |
| @string | toScreamingSnake | `to screaming snake` |           | SCREAMING_SNAKE_CASE operation.        |
| @string | toSnake          | `to snake`           |           | snake_case operation.                  |
| @string | toTitle          | `to title`           |           | Title Case operation.                  |
| @string | toUpper          | `to upper`           |           | UPPERCASE operation.                   |
| @string | toUTF8           | `to UTF8`            |           | UTF-8 encode operation.                |
+---------+------------------+----------------------+-----------+----------------------------------------+
