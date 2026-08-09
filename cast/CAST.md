## generated

| template               | output                            |
| ---------------------- | --------------------------------- |
| template/Identifiers.h | ../Source/generated/Identifiers.h |
| template/HashMaps.h    | ../Source/generated/HashMaps.h    |
| template/CAST.h        | ../Source/generated/CAST.h        |

## dispatch

| table     | column | value | template              | slot      |
| --------- | ------ | ----- | --------------------- | --------- |
| section   | entry  |       | template/Identifier.h | section   |
| predicate | entry  |       | template/Identifier.h | predicate |
| transform | entry  |       | template/Identifier.h | transform |
| file      | entry  |       | template/Identifier.h | file      |
| column    | entry  |       | template/Identifier.h | column    |
| char      | entry  |       | template/Char.h       | char      |
| string    | entry  |       | template/String.h     | string    |
| banner    | entry  |       | template/HashMap.h    | banner    |

## constraints

| column | predicate |
| ------ | --------- |
| name   | unique    |

## transforms

| column | transform |
| ------ | --------- |
| value  | escapeCpp |
