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
| castOutput | cast_output.md |

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
| value | value |

## char

Single-char juce::String constants derived from Chars:: — eliminates repeated juce::String::charToString boilerplate.

| word | char |
|---|---|
| charSpace | space |
| charNewline | newline |
| charDot | dot |
| charPipe | pipe |
| charOpenParen | openParen |
| charCloseParen | closeParen |
| charCarriageReturn | carriageReturn |
| charColon | colon |
| charComma | comma |
| charCloseBracket | closeBracket |
| charCloseBrace | closeBrace |

## string

Cast-specific multi-char string constants.

| word | string |
|---|---|
| programName | cast |
| commentPrefix | `// ` |
| cliPrefix | -- |
| scopeResolution | :: |
| juceNamespace | `juce::` |
| hazardChars | `<>` |
| hexPrefix | 0x |
| hexEscapePrefix | `\x` |
| escapedDoubleQuote | `\"` |
| escapedBackslash | `\\` |
| codepointPrefix | U+ |
| diagnosticSeparator | `: ` |
| failNoMatch | value does not match |
| failDuplicate | duplicate |
| failForeignKeyMissing | `value not found in ` |
| failNotInSet | `value not in {` |
| failOutOfRange | ` outside [` |
| failLocalMissing | local key not in ref |
| failRefMissing | ref key not in local |
| failNotFound | not found |
| failGroupOpen | `group '` |
| failGroupClose | `' does not have exactly one marked row` |
| failUnknownPredicate | unknown predicate |
| failHazardUri | contains URI scheme |
| failHazardAngleBrackets | `contains '<' or '>'` |
| failUnknownTransform | unknown transform |
| failTemplateMissing | template not found |
| failFragmentMissing | fragment not found |
| failOrphan | orphan template |
| failUnresolvedHole | unresolved template hole |
| failOutputMissing | output not found |
