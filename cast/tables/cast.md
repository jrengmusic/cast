████████████░░████████████░░████████████░░████████████░░
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░    
████░░        ████░░  ████░░████░░            ████░░    
████░░        ████████████░░████████████░░    ████░░    
████░░        ████░░  ████░░        ████░░    ████░░    
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░    
████████████░░████░░  ████░░████████████░░    ████░░    

## section

CAST.md manifest section-heading names — the four table kinds queried by Driver::run via getTableRowKeys.

| word | string |
|---|---|
| outputs | outputs |
| dispatch | dispatch |
| transforms | transforms |
| constraints | constraints |

## predicate

Constraint predicate names from the closed vocabulary, consumed by Constraints::buildPredicateMap and matched against `## constraints` rows.

| word | string |
|---|---|
| matches | matches |
| unique | unique |
| existsIn | existsIn |
| oneOf | oneOf |
| range | range |
| parity | parity |
| fileExists | fileExists |
| onePerGroup | onePerGroup |

## transform

Transform names from the closed vocabulary, consumed by Transforms::getTransformed and matched against `## transforms` rows.

| word | string |
|---|---|
| toUpper | toUpper |
| toTitle | toTitle |
| toKebab | toKebab |
| escapeCpp | escapeCpp |
| utf8Bytes | utf8Bytes |
| codepointHex | codepointHex |
| codepointLabel | codepointLabel |
| qualifySymbol | qualifySymbol |
| symbolFromFile | symbolFromFile |

## file

Well-known filename vocabulary.

| word | string |
|---|---|
| cast | CAST.md |
| specification | SPEC.md |

## column

CAST.md manifest table column-header vocabulary — the fixed schema columns of the outputs/dispatch/transforms/constraints tables, consumed by Driver's keyed table queries.

| word | string |
|---|---|
| column | column |
| slot | slot |
| transform | transform |
| predicate | predicate |
| tables | tables |
| templatePath | template |
