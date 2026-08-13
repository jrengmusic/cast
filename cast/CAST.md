## generated

| template               | output                            | separator        |
| ---------------------- | --------------------------------- | ---------------- |
| template/Identifiers.h | ../Source/generated/Identifiers.h | template/Break.h |
| template/Text.h        | ../Source/generated/Text.h        | template/Break.h |
| template/Files.h       | ../Source/generated/Files.h       | template/Break.h |
| template/HashMap.h     | ../Source/generated/HashMap.h     |                  |

## patch

| source    | fragment | placeholder   |
| --------- | -------- | ------------- |
| lexicon   |          | lexicon       |
| files     |          | file          |
| files     |          | moduleInclude |
| files     |          | module        |
| text      |          | localisation  |
| banner    |          | banner        |
| generated |          | generated     |

## constraints

| column | predicate |
| ------ | --------- |
| name   | unique    |

## transforms

| column | transform |
| ------ | --------- |
| value  | toLiteral |
