## text

+-----------------------------+----------------------------------------------------------------------+
| key                         | value                                                                |
+=============================+======================================================================+
| fail no match               | value does not match                                                 |
| fail duplicate              | `duplicate "`                                                        |
| fail foreign key missing    | `value not found in `                                                |
| fail not in set             | `value not in {`                                                     |
| fail out of range           | ` outside [`                                                         |
| fail local missing          | local key not in ref                                                 |
| fail ref missing            | ref key not in local                                                 |
| fail not found              | not found                                                            |
| fail group open             | `group '`                                                            |
| fail group close            | `' does not have exactly one marked row`                             |
| fail unknown predicate      | unknown predicate                                                    |
| fail hazard Uri             | contains URI scheme                                                  |
| fail hazard angle brackets  | `contains '<' or '>'`                                                |
| fail unknown transform      | unknown transform                                                    |
| fail template missing       | template not found                                                   |
| fail fragment missing       | fragment not found                                                   |
| fail orphan                 | orphan template                                                      |
| fail no source              | placeholder has no source                                            |
| fail table missing          | table not found                                                      |
| fail already declared       | `" already declared at `                                             |
| fail entity missing         | entity not declared in lexicon                                       |
| fail output missing         | output not found                                                     |
| fail name numeric           | name is a plain number                                               |
| fail name leading digit     | name starts with a digit                                             |
| fail name invalid char      | name contains an invalid character                                   |
| fail too long               | `exceeds `                                                           |
| fail too long suffix        | ` characters`                                                        |
| fail redundant value        | `value byte-equals `                                                 |
| fail redundant value suffix | ` projection of name; delete it, let the template project it`        |
| fail placeholder unknown    | region names a placeholder not declared in dispatch                  |
| fail column unknown         | region names a column not in the matched table                       |
| fail row reserved           | row is a reserved name                                               |
| fail row region column      | row region takes no column; use the column's name as the region name |
| fail placeholder missing    | placeholder has no placeholder or region in any root                 |
| fail placeholder ambiguous  | placeholder declared in multiple roots                               |
| fail region source missing  | dispatch row has no template and its placeholder has no region       |
| fail region unfed           | placeholder region is declared but no dispatch row feeds it          |
| fail template chain depth   | template chain exceeds maximum depth of 2                            |
+-----------------------------+----------------------------------------------------------------------+
