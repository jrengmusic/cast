## diagnostics

```
@brief Engine failure-message texts, emitted verbatim when a gate fails.

Each constant is the exact diagnostic text CAST prints for one fatal in
SPEC §10.1. The Validator owns the decision; these strings are the words it
speaks. Prefix/affix-shaped entries pair with the offender's own text.
```

+------------------------+--------------------------------------------+---------------------------------------------------------+
| name                   | value                                      | comment                                                 |
+========================+============================================+=========================================================+
| failNotFound           | `not found`                                | Referenced table, column, or symbol does not exist.     |
| failUnknownTransform   | `unknown transform`                        | A format cell named an operation outside §8.            |
| failTemplateMissing    | `template not found`                       | template:\<id\> named a block that does not exist.      |
| failOrphan             | `orphan template`                          | A template block no manifest row addresses.             |
| failTableMissing       | `table not found`                          | An address named a table that does not exist.           |
| failOutputMissing      | `output not found`                         | An output row declared no file to write.                |
| failColumnUnknown      | `column not in the matched table`          | A named token matched no column of the addressed table. |
| failAliasMissing       | `alias not declared in index`              | An alias is absent from the writing file's index.       |
| failDuplicate          | `duplicate "`                              | Prefix — a column entry repeated byte-exactly.          |
| failFormatAdjacent     | `format column has no column to format`    | \| format \| format \| adjacency.                       |
| failColumnInvalid      | `column name is not a valid identifier`    | A column name failed identifier validation.             |
| failAmbiguous          | `nested shape has more than one candidate` | A nested shape resolved to more than one candidate.     |
| failBlockUnnamed       | `code block has no id`                     | A template fenced block carries no info string.         |
| failOutputWrite        | `cannot write output`                      | An output file could not be written.                    |
| failBindingDuplicate   | `duplicate binding`                        | A shape declared one binding name twice.                |
| failMarkerUnterminated | `unterminated marker`                      | A ::: marker in a shape block never closed.             |
+------------------------+--------------------------------------------+---------------------------------------------------------+
