## index

| alias        | path                                     |
| ------------ | ---------------------------------------- |
| bimap        | ../../jam/cast/template/Bimap.cast       |
| break        | ../../jam/cast/template/Break.cast       |
| files        | ../../jam/cast/template/Files.cast       |
| generated    | ../../jam/cast/template/Generated.cast   |
| hash map     | ../../jam/cast/template/HashMap.cast     |
| identifiers  | ../../jam/cast/template/Identifiers.cast |
| namespace    | ../../jam/cast/template/Namespace.cast   |
| struct       | ../../jam/cast/template/Struct.cast      |
| text         | ../../jam/cast/template/Text.cast        |
| banner       | tables/banner.md                         |
| binary files | tables/binary-files.md                   |
| lexicon      | tables/lexicon.md                        |
| localisation | tables/localisation-en.md                |
| template     | tables/template.md                       |
| CAST         | CAST.md                                  |

## output

| code        | namespace | namespace name | list                         | lineBreak | instance | file                              |
| ----------- | --------- | -------------- | ---------------------------- | --------- | -------- | --------------------------------- |
| identifiers | namespace | Id             | lexicon:lexicon              |           |          | ../Source/generated/Identifiers.h |
| text        | namespace | text::en       | localisation:text            |           |          | ../Source/generated/Text.h        |
| files       | namespace | files          | binary files:files           |           |          | ../Source/generated/Files.h       |
| hash map    | namespace | map            | banner:banner                |           |          | ../Source/generated/HashMap.h     |
| bimap       | namespace | map            | template:template token type | break     | shared   | ../Source/generated/Bimaps.h      |
| bimap       | namespace | map            | template:rules               | break     | shared   | ../Source/generated/Bimaps.h      |

## output index

| code      | struct | struct name | output      | list                 | instance    | file                            |
| --------- | ------ | ----------- | ----------- | -------------------- | ----------- | ------------------------------- |
| generated | struct | Generated   | CAST:output | binary files:headers | CAST:output | ../Source/generated/Generated.h |
