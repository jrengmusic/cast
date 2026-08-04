## outputs

| output | template | tables |
|---|---|---|
| ../Source/generated/Identifiers.h | template/Identifiers.h | `tables/cast.md` |
| ../Source/generated/HashMaps.h | template/HashMaps.h | `tables/banner.md` |
| ../Source/generated/CAST.h | template/CAST.h |  |

## dispatch

| table | column | value | template | slot |
|---|---|---|---|---|
| section | word |  | template/Identifier.h | section |
| predicate | word |  | template/Identifier.h | predicate |
| transform | word |  | template/Identifier.h | transform |
| file | word |  | template/Identifier.h | file |
| column | word |  | template/Identifier.h | column |
| char | word |  | template/Char.h | char |
| string | word |  | template/String.h | string |
| banner | colour |  | template/HashMap.h | banner |

## constraints

| column | predicate |
|---|---|
| word | unique |

## transforms

| column | transform |
|---|---|
| string | escapeCpp |
