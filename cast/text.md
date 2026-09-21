## diagnostics

```
@brief Engine failure-message texts, emitted verbatim when a gate fails.

Each constant is the exact diagnostic text CAST prints for one fatal in
SPEC §10.1. The Validator owns the decision; these strings are the words it
speaks. Prefix/affix-shaped entries pair with the offender's own text.
```

+------------------------+------------------------------------------------------+--------------------------------------------------------------------------------+
| name                   | value                                                | comment                                                                        |
+========================+======================================================+================================================================================+
| failNotFound           | `not found`                                          | Referenced table, column, or symbol does not exist.                            |
| failUnknownTransform   | `unknown transform`                                  | A format cell named an operation outside §8.                                   |
| failTemplateMissing    | `template not found`                                 | A shape address named a fence that does not exist in its template file.        |
| failStructureMissing   | `structure not declared`                             | An output row declares no structure column entry.                              |
| failOrphan             | `orphan template`                                    | A list or comment line with no partner at its own depth and ordinal.           |
| failTableMissing       | `table not found`                                    | An address named a table that does not exist.                                  |
| failOutputMissing      | `output not found`                                   | An index symbol names a file that does not exist.                              |
| failColumnUnknown      | `column not in the matched table`                    | A named token matched no column of the addressed table.                        |
| failDuplicate          | `duplicate "`                                        | Prefix — a column entry repeated byte-exactly.                                 |
| failFormatAdjacent     | `format column has no column to format`              | \| format \| format \| adjacency.                                              |
| failAmbiguous          | `nested shape has more than one candidate`           | A shape's arity does not match the sources supplied.                           |
| failOutputWrite        | `cannot write output`                                | An output file could not be written.                                           |
| failBindingDuplicate   | `duplicate binding`                                  | A shape declared one binding name twice.                                       |
| failMarkerUnterminated | `unterminated marker`                                | A ::: marker in a shape block never closed.                                    |
| failMapOrphan          | `map line has no shape paragraph`                    | A list line paired with no structure line and no shape paragraph at its depth. |
| failToolchain          | `toolchain command failed`                           | A toolchain row's process could not start or exited nonzero.                   |
| failToolchainArgument  | `toolchain argument not declared`                    | A --\<word\> CLI argument matched no toolchain row's argument column.          |
| failToolchainColumn    | `toolchain column not declared`                      | A ## toolchain table's header row declares no command or flag column.          |
| failFencePrefix        | `unknown fence prefix`                               | A fence's bracket word is not a comment-syntax extension or no-banner.         |
| failRegionFile         | `region file not found`                              | A region row's file does not exist.                                            |
| failRegionPair         | `[begin] and [end] must be declared together`        | A region row declared one delimiter binding without the other.                 |
| failRegionDelimiter    | `region delimiter not found or out of order`         | A region delimiter matched no line, or [end] matched at or before [begin].     |
| failRegionShared       | `file shared between region and whole-file rows`     | A file was declared by both a region row and a whole-file row.                 |
| failSyncInfo           | `user-modules-info.md not found`                     | A sync root carries no user-modules-info.md file.                              |
| failSyncIdentity       | `composed identity key missing`                      | A composed identity key is absent from either sync file.                       |
| failSyncAmbiguity      | `identity source value maps to more than one target` | Two identity rows share a source value but name different target values.       |
| failSyncModule         | `kernel module directory missing or undeclared`      | A kernel row named no directory, or a filePrefix directory was undeclared.     |
| failSyncCorrespondence | `kernel sets do not correspond`                      | The two sync files' kernel sets do not correspond one to one.                  |
| failSyncContamination  | `target token present in source`                     | A source text file already contained a pair's target value.                    |
| failSyncRoot           | `source and target roots are the same`               | The sync source root and target root resolved to the same directory.           |
| failSyncRead           | `source file cannot be read`                         | A sync source file could not be read.                                          |
| failSyncDelete         | `delete failed`                                      | A sync mirror-delete could not remove a target file.                           |
| failSyncArguments      | `--sync takes a source root and a target root`       | A --sync line did not carry exactly two roots.                                 |
+------------------------+------------------------------------------------------+--------------------------------------------------------------------------------+
