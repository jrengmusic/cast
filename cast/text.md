## diagnostics

+--------------------------+----------------------------------------------------------------------+
| name                     | value                                                                |
+==========================+======================================================================+
| failNotFound             | not found                                                            |
| failUnknownPredicate     | unknown predicate                                                    |
| failHazardUri            | contains URI scheme                                                  |
| failHazardAngleBrackets  | `contains '<' or '>'`                                                |
| failUnknownTransform     | unknown transform                                                    |
| failTemplateMissing      | template not found                                                   |
| failFragmentMissing      | fragment not found                                                   |
| failOrphan               | orphan template                                                      |
| failTableMissing         | table not found                                                      |
| failAlreadyDeclared      | `" already declared at `                                             |
| failEntityMissing        | entity not declared in lexicon                                       |
| failOutputMissing        | output not found                                                     |
| failNameNumeric          | name is a plain number                                               |
| failNameLeadingDigit     | name starts with a digit                                             |
| failNameInvalidChar      | name contains an invalid character                                   |
| failTooLong              | `exceeds `                                                           |
| failTooLongSuffix        | ` characters`                                                        |
| failRedundantValue       | `value byte-equals `                                                 |
| failRedundantValueSuffix | ` projection of name; delete it, let the template project it`        |
| failPlaceholderUnknown   | region names a placeholder not declared in dispatch                  |
| failColumnUnknown        | column not in the matched table                                      |
| failRowReserved          | row is a reserved name                                               |
| failRowRegionColumn      | row region takes no column; use the column's name as the region name |
| failPlaceholderMissing   | placeholder has no placeholder or region in any root                 |
| failPlaceholderAmbiguous | placeholder declared in multiple roots                               |
| failRegionSourceMissing  | dispatch row has no template and its placeholder has no region       |
| failRegionUnfed          | placeholder region is declared but no dispatch row feeds it          |
| failTemplateChainDepth   | template chain exceeds maximum depth of 2                            |
| failAliasMissing         | alias not declared in index                                          |
| failNoMatch              | value does not match                                                 |
| failDuplicate            | `duplicate "`                                                        |
| failFormatAdjacent       | format column has no column to format                                |
| failColumnInvalid        | column name is not a valid identifier                                |
| failAmbiguous            | nested shape has more than one candidate                             |
| failBlockUnnamed         | code block has no id                                                 |
| failOutputWrite          | cannot write output                                                  |
| failBindingDuplicate     | duplicate binding                                                    |
| failMarkerUnterminated   | unterminated marker                                                  |
+--------------------------+----------------------------------------------------------------------+
