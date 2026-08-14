## index

| alias        | path                                     |
| ------------ | ---------------------------------------- |
| bimap        | ../../jam/cast/template/Bimap.cast       |
| break        | ../../jam/cast/template/Break.cast       |
| char         | ../../jam/cast/template/Chars.cast       |
| files        | ../../jam/cast/template/Files.cast       |
| generated    | ../../jam/cast/template/Generated.cast   |
| hash map     | ../../jam/cast/template/HashMap.cast     |
| identifiers  | ../../jam/cast/template/Identifiers.cast |
| namespace    | ../../jam/cast/template/Namespace.cast   |
| struct       | ../../jam/cast/template/Struct.cast      |
| text         | ../../jam/cast/template/Text.cast        |
| banner       | tables/banner.md                         |
| comments     | tables/comments.md                       |
| binary files | tables/binary-files.md                   |
| lexicon      | tables/lexicon.md                        |
| localisation | tables/localisation-en.md                |
| template     | tables/template.md                       |
| CAST         | CAST.md                                  |
| Identifiers  | ../Source/generated/Identifiers.h        |
| Text         | ../Source/generated/Text.h               |
| Files        | ../Source/generated/Files.h              |
| HashMap      | ../Source/generated/HashMap.h            |
| Bimaps       | ../Source/generated/Bimaps.h             |
| Generated    | ../Source/generated/Generated.h          |

## output

| code        | namespace | namespace name | list                         | token                                          | lineBreak | instance | file        |
| ----------- | --------- | -------------- | ---------------------------- | ---------------------------------------------- | --------- | -------- | ----------- |
| identifiers | namespace | Id             | lexicon:lexicon              |                                                |           |          | Identifiers |
| text        | namespace | text::en       | localisation:text            |                                                |           |          | Text        |
| files       | namespace | files          | binary files:files           |                                                |           |          | Files       |
| hash map    | namespace | map            | banner:banner                | keyType, valueType: juce::String, juce::String |           |          | HashMap     |
| hash map    | namespace | map            | comments:banner open         | keyType, valueType: juce::String, juce::String |           |          | HashMap     |
| hash map    | namespace | map            | comments:banner close        | keyType, valueType: juce::String, juce::String |           |          | HashMap     |
| hash map    | namespace | map            | comments:pragma              | keyType, valueType: juce::String, juce::String |           |          | HashMap     |
| bimap       | namespace | map            | template:template token type |                                                | break     | shared   | Bimaps      |
| bimap       | namespace | map            | template:rules               |                                                | break     | shared   | Bimaps      |

## output index

| code      | struct | struct name | output      | list                 | instance    | file      |
| --------- | ------ | ----------- | ----------- | -------------------- | ----------- | --------- |
| generated | struct | Generated   | CAST:output | binary files:headers | CAST:output | Generated |
