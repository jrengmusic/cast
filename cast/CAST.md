## outputs

| output | template | tables |
|---|---|---|
| ../Source/generated/Lexicon.h | template/Lexicon.h | `tables/cast.md` |
| ../Source/generated/Banner.h | template/Banner.h | `tables/banner.md` |

## dispatch

| table | column | value | template | slot |
|---|---|---|---|---|
| section | word |  | template/IdentifierRow.h | section |
| predicate | word |  | template/IdentifierRow.h | predicate |
| transform | word |  | template/IdentifierRow.h | transform |
| file | word |  | template/IdentifierRow.h | file |
| column | word |  | template/IdentifierRow.h | column |
| banner | colour |  | template/BannerRow.h | banner |

## constraints

| column | predicate |
|---|---|
| word | unique |
