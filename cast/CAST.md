## index

+--------------+------------------------------------------------+-------------+
| alias        | symbol                                         | format      |
+==============+================================================+=============+
| @lexicon     | lexicon.md                                     |             |
| @text        | text.md                                        |             |
| @binaryFiles | files.md                                       |             |
| @banner      | banner.md                                      |             |
| @comments    | comments.md                                    |             |
| @tokens      | tokens.md                                      |             |
| @template    | template.cast                                  |             |
| @CAST        | CAST.md                                        |             |
| @Identifiers | ../Source/generated/Identifiers.h              |             |
| @Text        | ../Source/generated/Text.h                     |             |
| @Files       | ../Source/generated/Files.h                    |             |
| @HashMaps    | ../Source/generated/HashMaps.h                 |             |
| @Bimaps      | ../Source/generated/Bimaps.h                   |             |
| @Generated   | ../Source/generated/Generated.h                |             |
| @id          | juce::Identifier                               | fromLiteral |
| @string      | juce::String                                   | fromLiteral |
| @commentMap  | `jam::HashMap<juce::Identifier, juce::String>` |             |
+--------------+------------------------------------------------+-------------+

## output

+-------------------------------------------+----------------------------+---------------------------------+--------------+
| list                                      | separator                  | structure                       | file         |
+===========================================+============================+=================================+==============+
| - list: @lexicon:lexicon                  |                            | template:namespace              | @Identifiers |
|                                           |                            | - name: Id                      |              |
|                                           |                            | - list: template:identifier     |              |
+-------------------------------------------+----------------------------+---------------------------------+--------------+
| > - list: @text:diagnostics               |                            | template:namespace              | @Text        |
|                                           |                            | - name: text                    |              |
|                                           |                            |                                 |              |
|                                           |                            | template:struct                 |              |
|                                           |                            | - name: Diagnostics             |              |
|                                           |                            | > - list: template:char         |              |
+-------------------------------------------+----------------------------+---------------------------------+--------------+
| - list: @binaryFiles:files                |                            | template:namespace              | @Files       |
|                                           |                            | - name: files                   |              |
|                                           |                            | - list: template:identifier     |              |
|                                           |                            | - type: @string                 |              |
+-------------------------------------------+----------------------------+---------------------------------+--------------+
| > - list: @banner:banner                  | - list: template:separator | template:namespace              | @HashMaps    |
|                                           |                            | - name: map                     |              |
|                                           |                            |                                 |              |
|                                           |                            | template:hashMap                |              |
|                                           |                            | - name: banner                  |              |
|                                           |                            | - keyType: @string              |              |
|                                           |                            | - valueType: @string            |              |
|                                           |                            | > - list: template:stringPair   |              |
+-------------------------------------------+----------------------------+---------------------------------+--------------+
| > - list: @comments:clang comment         | - list: template:separator | template:namespace              | @HashMaps    |
|                                           |                            | - name: map                     |              |
|                                           |                            |                                 |              |
|                                           |                            | template:hashMap                |              |
|                                           |                            | - name: clangComment            |              |
|                                           |                            | - keyType: @id                  |              |
|                                           |                            | - valueType: @string            |              |
|                                           |                            | > - list: template:mapEntry     |              |
+-------------------------------------------+----------------------------+---------------------------------+--------------+
| > - list: @comments:css comment           | - list: template:separator | template:namespace              | @HashMaps    |
|                                           |                            | - name: map                     |              |
|                                           |                            |                                 |              |
|                                           |                            | template:hashMap                |              |
|                                           |                            | - name: cssComment              |              |
|                                           |                            | - keyType: @id                  |              |
|                                           |                            | - valueType: @string            |              |
|                                           |                            | > - list: template:mapEntry     |              |
+-------------------------------------------+----------------------------+---------------------------------+--------------+
| > - list: @comments:html comment          | - list: template:separator | template:namespace              | @HashMaps    |
|                                           |                            | - name: map                     |              |
|                                           |                            |                                 |              |
|                                           |                            | template:hashMap                |              |
|                                           |                            | - name: htmlComment             |              |
|                                           |                            | - keyType: @id                  |              |
|                                           |                            | - valueType: @string            |              |
|                                           |                            | > - list: template:mapEntry     |              |
+-------------------------------------------+----------------------------+---------------------------------+--------------+
| > - list: @comments:lua comment           | - list: template:separator | template:namespace              | @HashMaps    |
|                                           |                            | - name: map                     |              |
|                                           |                            |                                 |              |
|                                           |                            | template:hashMap                |              |
|                                           |                            | - name: luaComment              |              |
|                                           |                            | - keyType: @id                  |              |
|                                           |                            | - valueType: @string            |              |
|                                           |                            | > - list: template:mapEntry     |              |
+-------------------------------------------+----------------------------+---------------------------------+--------------+
| > - list: @comments:mermaid comment       | - list: template:separator | template:namespace              | @HashMaps    |
|                                           |                            | - name: map                     |              |
|                                           |                            |                                 |              |
|                                           |                            | template:hashMap                |              |
|                                           |                            | - name: mermaidComment          |              |
|                                           |                            | - keyType: @id                  |              |
|                                           |                            | - valueType: @string            |              |
|                                           |                            | > - list: template:mapEntry     |              |
+-------------------------------------------+----------------------------+---------------------------------+--------------+
| > - list: @comments:python comment        | - list: template:separator | template:namespace              | @HashMaps    |
|                                           |                            | - name: map                     |              |
|                                           |                            |                                 |              |
|                                           |                            | template:hashMap                |              |
|                                           |                            | - name: pythonComment           |              |
|                                           |                            | - keyType: @id                  |              |
|                                           |                            | - valueType: @string            |              |
|                                           |                            | > - list: template:mapEntry     |              |
+-------------------------------------------+----------------------------+---------------------------------+--------------+
| > - list: @comments:ruby comment          | - list: template:separator | template:namespace              | @HashMaps    |
|                                           |                            | - name: map                     |              |
|                                           |                            |                                 |              |
|                                           |                            | template:hashMap                |              |
|                                           |                            | - name: rubyComment             |              |
|                                           |                            | - keyType: @id                  |              |
|                                           |                            | - valueType: @string            |              |
|                                           |                            | > - list: template:mapEntry     |              |
+-------------------------------------------+----------------------------+---------------------------------+--------------+
| > - list: @comments:shell comment         | - list: template:separator | template:namespace              | @HashMaps    |
|                                           |                            | - name: map                     |              |
|                                           |                            |                                 |              |
|                                           |                            | template:hashMap                |              |
|                                           |                            | - name: shellComment            |              |
|                                           |                            | - keyType: @id                  |              |
|                                           |                            | - valueType: @string            |              |
|                                           |                            | > - list: template:mapEntry     |              |
+-------------------------------------------+----------------------------+---------------------------------+--------------+
| > - list: @comments:comment syntax        |                            | template:namespace              | @HashMaps    |
|                                           |                            | - name: map                     |              |
|                                           |                            |                                 |              |
|                                           |                            | template:hashMap                |              |
|                                           |                            | - name: commentSyntax           |              |
|                                           |                            | - keyType: @string              |              |
|                                           |                            | - valueType: @commentMap        |              |
|                                           |                            | > - list: template:stringEntry  |              |
+-------------------------------------------+----------------------------+---------------------------------+--------------+
| > > > - list: @tokens:template token type |                            | template:namespace              | @Bimaps      |
| > > - list: @tokens:template token type   |                            | - name: map                     |              |
|                                           |                            |                                 |              |
|                                           |                            | template:bimap                  |              |
|                                           |                            | - name: TemplateTokenType       |              |
|                                           |                            | - type: int                     |              |
|                                           |                            | - instance:                     |              |
|                                           |                            | - value: text                   |              |
|                                           |                            | > > > - list: template:mapEntry |              |
|                                           |                            | > > - list: template:enum       |              |
+-------------------------------------------+----------------------------+---------------------------------+--------------+

## output index

+--------------------+-----------+-----------------------------------+------------+
| list               | separator | structure                         | file       |
+====================+===========+===================================+============+
| - list: file       |           | template:generated                | @Generated |
| > - list: instance |           | - list: template:include          |            |
|                    |           | > - list: template:sharedInstance |            |
+--------------------+-----------+-----------------------------------+------------+
