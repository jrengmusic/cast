## index

+--------------+------------------------------------------------------+
| alias        | symbol                                               |
+==============+======================================================+
| bimap        | ../../jam/cast/template/Bimap.cast                   |
| break        | ../../jam/cast/template/Break.cast                   |
| char         | ../../jam/cast/template/Chars.cast                   |
| diff         | ../../jam/cast/template/Generated.cast               |
| files        | ../../jam/cast/template/Identifiers.cast             |
| hash map     | ../../jam/cast/template/HashMap.cast                 |
| identifiers  | ../../jam/cast/template/Identifiers.cast             |
| namespace    | ../../jam/cast/template/Namespace.cast               |
| struct       | ../../jam/cast/template/Struct.cast                  |
| text         | ../../jam/cast/template/Text.cast                    |
+--------------+------------------------------------------------------+
| banner       | tables/banner.md                                     |
| binary files | tables/binary-files.md                               |
| CAST         | CAST.md                                              |
| comments     | tables/comments.md                                   |
| lexicon      | tables/lexicon.md                                    |
| localisation | tables/localisation-en.md                            |
| template     | tables/template.md                                   |
+--------------+------------------------------------------------------+
| Bimaps       | ../Source/diff/Bimaps.h                              |
| Files        | ../Source/diff/Files.h                               |
| Generated    | ../Source/diff/Generated.h                           |
| HashMaps     | ../Source/diff/HashMaps.h                            |
| Identifiers  | ../Source/diff/Identifiers.h                         |
| Text         | ../Source/diff/Text.h                                |
+--------------+------------------------------------------------------+
| commentMap   | `jam::HashMap<juce::Identifier, juce::String>`       |
| id           | juce::Identifier                                     |
| string       | juce::String                                         |
+--------------+------------------------------------------------------+

## output

+-------------+-----------+----------------+------------------------------+----------------------------------------+-----------+-------------+
| code        | namespace | namespace name | list                         | token                                  | lineBreak | file        |
+=============+===========+================+==============================+========================================+===========+=============+
| identifiers | namespace | Id             | lexicon:lexicon              |                                        |           | Identifiers |
| text        | namespace | text::en       | localisation:text            |                                        |           | Text        |
| files       | namespace | files          | binary files:files           | type: string                           |           | Files       |
| hash map    | namespace | map            | banner:banner                | keyType, valueType: string, string     |           | HashMaps    |
| hash map    | namespace | map            | comments:clang comment       | keyType, valueType: id, string         |           | HashMaps    |
| hash map    | namespace | map            | comments:css comment         | keyType, valueType: id, string         |           | HashMaps    |
| hash map    | namespace | map            | comments:html comment        | keyType, valueType: id, string         |           | HashMaps    |
| hash map    | namespace | map            | comments:lua comment         | keyType, valueType: id, string         |           | HashMaps    |
| hash map    | namespace | map            | comments:mermaid comment     | keyType, valueType: id, string         |           | HashMaps    |
| hash map    | namespace | map            | comments:python comment      | keyType, valueType: id, string         |           | HashMaps    |
| hash map    | namespace | map            | comments:ruby comment        | keyType, valueType: id, string         |           | HashMaps    |
| hash map    | namespace | map            | comments:shell comment       | keyType, valueType: id, string         |           | HashMaps    |
| hash map    | namespace | map            | comments:comment syntax      | keyType, valueType: string, commentMap |           | HashMaps    |
| bimap       | namespace | map            | template:template token type | instance: shared                       | break     | Bimaps      |
| bimap       | namespace | map            | template:rules               | instance: shared                       | break     | Bimaps      |
+-------------+-----------+----------------+------------------------------+----------------------------------------+-----------+-------------+

## output index

+------+--------+-------------+-------------+----------------------+-------------+-----------+
| code | struct | struct name | output      | list                 | instance    | file      |
+======+========+=============+=============+======================+=============+===========+
| diff | struct | Generated   | CAST:output | binary files:headers | CAST:output | Generated |
+------+--------+-------------+-------------+----------------------+-------------+-----------+
