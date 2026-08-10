## generated

| template               | output                            | separator        |
| ---------------------- | --------------------------------- | ---------------- |
| template/Identifiers.h | ../Source/generated/Identifiers.h | template/Break.h |
| template/Text.h        | ../Source/generated/Text.h        | template/Break.h |
| template/Files.h       | ../Source/generated/Files.h       | template/Break.h |
| template/HashMaps.h    | ../Source/generated/HashMaps.h    | template/Break.h |
| template/CAST.h        | ../Source/generated/CAST.h        |                  |
| template/Generated.h   | ../Source/generated/Generated.h   |                  |

## dispatch

| table     | column | value | template                             | placeholder   |
| --------- | ------ | ----- | ------------------------------------ | ------------- |
| section   | entry  |       |                                      | section       |
| predicate | entry  |       |                                      | predicate     |
| transform | entry  |       |                                      | transform     |
| files     | name   |       |                                      | file          |
| column    | entry  |       |                                      | column        |
| string    | entry  |       |                                      | string        |
| text      | key    |       |                                      | localisation  |
| banner    | key    |       | template/HashMap.h                   | banner        |
| files     | module |       |                                      | moduleInclude |
| files     | module |       |                                      | module        |

## constraints

| column | predicate |
| ------ | --------- |
| name   | unique    |

## transforms

| column | transform |
| ------ | --------- |
| value  | toLiteral |
