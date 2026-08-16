## index

+---------------+------------------------------------------------+
| alias         | symbol                                         |
+===============+================================================+
| #bimap        | ../../jam/cast/template/Bimap.cast             |
| #break        | ../../jam/cast/template/Break.cast             |
| #char         | ../../jam/cast/template/Chars.cast             |
| #generated    | ../../jam/cast/template/Generated.cast         |
| #files        | ../../jam/cast/template/Identifiers.cast       |
| #hashMap      | ../../jam/cast/template/HashMap.cast           |
| #identifiers  | ../../jam/cast/template/Identifiers.cast       |
| #namespace    | ../../jam/cast/template/Namespace.cast         |
| #struct       | ../../jam/cast/template/Struct.cast            |
| #text         | ../../jam/cast/template/Text.cast              |
+---------------+------------------------------------------------+
| #banner       | tables/banner.md                               |
| #binaryFiles  | tables/binary-files.md                         |
| #CAST         | CAST.md                                        |
| #comments     | tables/comments.md                             |
| #lexicon      | tables/lexicon.md                              |
| #localisation | tables/localisation-en.md                      |
| #template     | tables/template.md                             |
+---------------+------------------------------------------------+
| #Bimaps       | ../Source/generated/Bimaps.h                   |
| #Files        | ../Source/generated/Files.h                    |
| #Generated    | ../Source/generated/Generated.h                |
| #HashMaps     | ../Source/generated/HashMaps.h                 |
| #Identifiers  | ../Source/generated/Identifiers.h              |
| #Text         | ../Source/generated/Text.h                     |
+---------------+------------------------------------------------+
| #commentMap   | `jam::HashMap<juce::Identifier, juce::String>` |
| #id           | juce::Identifier                               |
| #string       | juce::String                                   |
+---------------+------------------------------------------------+

## output

+--------------+------------+----------------+-------------------------------+------------------------------------------+-----------+--------------+
| code         | namespace  | namespace name | list                          | token                                    | lineBreak | file         |
+==============+============+================+===============================+==========================================+===========+==============+
| #identifiers | #namespace | Id             | #lexicon:lexicon              |                                          |           | #Identifiers |
| #text        | #namespace | text::en       | #localisation:text            |                                          |           | #Text        |
| #files       | #namespace | files          | #binaryFiles:files            | type: #string                            |           | #Files       |
| #hashMap     | #namespace | map            | #banner:banner                | keyType, valueType: #string, #string     |           | #HashMaps    |
| #hashMap     | #namespace | map            | #comments:clang comment       | keyType, valueType: #id, #string         |           | #HashMaps    |
| #hashMap     | #namespace | map            | #comments:css comment         | keyType, valueType: #id, #string         |           | #HashMaps    |
| #hashMap     | #namespace | map            | #comments:html comment        | keyType, valueType: #id, #string         |           | #HashMaps    |
| #hashMap     | #namespace | map            | #comments:lua comment         | keyType, valueType: #id, #string         |           | #HashMaps    |
| #hashMap     | #namespace | map            | #comments:mermaid comment     | keyType, valueType: #id, #string         |           | #HashMaps    |
| #hashMap     | #namespace | map            | #comments:python comment      | keyType, valueType: #id, #string         |           | #HashMaps    |
| #hashMap     | #namespace | map            | #comments:ruby comment        | keyType, valueType: #id, #string         |           | #HashMaps    |
| #hashMap     | #namespace | map            | #comments:shell comment       | keyType, valueType: #id, #string         |           | #HashMaps    |
| #hashMap     | #namespace | map            | #comments:comment syntax      | keyType, valueType: #string, #commentMap |           | #HashMaps    |
| #bimap       | #namespace | map            | #template:template token type | instance, default: shared, text          | #break    | #Bimaps      |
| #bimap       | #namespace | map            | #template:rules               | instance, default: shared, begin         | #break    | #Bimaps      |
+--------------+------------+----------------+-------------------------------+------------------------------------------+-----------+--------------+

## output index

+------------+---------+-------------+--------------+----------------------+--------------+------------+
| code       | struct  | struct name | output       | list                 | instance     | file       |
+============+=========+=============+==============+======================+==============+============+
| #generated | #struct | Generated   | #CAST:output | #binaryFiles:headers | #CAST:output | #Generated |
+------------+---------+-------------+--------------+----------------------+--------------+------------+
