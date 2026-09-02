## diagnostics

```
@brief Engine failure-message texts, emitted verbatim when a gate fails.

Each constant is the exact diagnostic text CAST prints for one fatal in
SPEC §10.1. The Validator owns the decision; these strings are the words it
speaks. Prefix/affix-shaped entries pair with the offender's own text.
```

+------------------------+--------------------------------------------+--------------------------------------------------------------------------------+
| name                   | value                                      | comment                                                                        |
+========================+============================================+================================================================================+
| failNotFound           | `not found`                                | Referenced table, column, or symbol does not exist.                            |
| failUnknownTransform   | `unknown transform`                        | A format cell named an operation outside §8.                                   |
| failTemplateMissing    | `template not found`                       | A shape address named a fence that does not exist in its template file.        |
| failStructureMissing   | `structure not declared`                   | An output row declares no structure column entry.                              |
| failOrphan             | `orphan template`                          | A list or comment line with no partner at its own depth and ordinal.           |
| failTableMissing       | `table not found`                          | An address named a table that does not exist.                                  |
| failOutputMissing      | `output not found`                         | An index symbol names a file that does not exist.                              |
| failColumnUnknown      | `column not in the matched table`          | A named token matched no column of the addressed table.                        |
| failDuplicate          | `duplicate "`                              | Prefix — a column entry repeated byte-exactly.                                 |
| failFormatAdjacent     | `format column has no column to format`    | \| format \| format \| adjacency.                                              |
| failAmbiguous          | `nested shape has more than one candidate` | A shape's arity does not match the sources supplied.                           |
| failOutputWrite        | `cannot write output`                      | An output file could not be written.                                           |
| failBindingDuplicate   | `duplicate binding`                        | A shape declared one binding name twice.                                       |
| failMarkerUnterminated | `unterminated marker`                      | A ::: marker in a shape block never closed.                                    |
| failMapOrphan          | `map line has no shape paragraph`          | A list line paired with no structure line and no shape paragraph at its depth. |
| failToolchain          | `toolchain command failed`                 | A toolchain row's process could not start or exited nonzero.                   |
| failToolchainArgument  | `toolchain argument not declared`          | A --\<word\> CLI argument matched no toolchain row's argument column.          |
| failToolchainColumn    | `toolchain column not declared`            | A ## toolchain table's header row declares no command or flag column.          |
+------------------------+--------------------------------------------+--------------------------------------------------------------------------------+
