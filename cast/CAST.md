## generated

| template               | output                       | separator        |
| ---------------------- | ---------------------------- | ---------------- |
| template/Identifiers.h | ../Source/diff/Identifiers.h | template/Break.h |
| template/Text.h        | ../Source/diff/Text.h        | template/Break.h |
| template/Files.h       | ../Source/diff/Files.h       | template/Break.h |
| template/HashMaps.h    | ../Source/diff/HashMaps.h    | template/Break.h |
| template/CAST.h        | ../Source/diff/CAST.h        |                  |
| template/Generated.h   | ../Source/diff/Generated.h   |                  |

## patch

| source       | fragment           | placeholder   |
| ------------ | ------------------ | ------------- |
| section      |                    | section       |
| predicate    |                    | predicate     |
| transform    |                    | transform     |
| column       |                    | column        |
| string       |                    | string        |
| files        |                    | file          |
| files.module |                    | moduleInclude |
| files.module |                    | module        |
| text         |                    | localisation  |
| banner       | template/HashMap.h | banner        |

## constraints

| column | predicate |
| ------ | --------- |
| name   | unique    |

## transforms

| column | transform |
| ------ | --------- |
| value  | toLiteral |
