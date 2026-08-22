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

+-------------------------------------------+----------------------------------+--------------------+--------------+
| placeholder                               | structure                        | separator          | file         |
+===========================================+==================================+====================+==============+
| - line: @lexicon:lexicon                  | template:namespace               |                    | @Identifiers |
|                                           | - name: Id                       |                    |              |
|                                           | > - line: template:identifier    |                    |              |
+-------------------------------------------+----------------------------------+--------------------+--------------+
| > - line: @text:diagnostics               | template:namespace               |                    | @Text        |
|                                           | - name: text                     |                    |              |
|                                           | > template:struct                |                    |              |
|                                           | > - name: Diagnostics            |                    |              |
|                                           | > > - line: template:char        |                    |              |
+-------------------------------------------+----------------------------------+--------------------+--------------+
| - line: @binaryFiles:files                | template:namespace               |                    | @Files       |
|                                           | - name: files                    |                    |              |
|                                           | > - line: template:identifier    |                    |              |
|                                           | > - type: @string                |                    |              |
+-------------------------------------------+----------------------------------+--------------------+--------------+
| > - line: @banner:banner                  | template:namespace               | template:separator | @HashMaps    |
|                                           | - name: map                      |                    |              |
|                                           | > template:hashMap               |                    |              |
|                                           | > - name: banner                 |                    |              |
|                                           | > - keyType: @string             |                    |              |
|                                           | > - valueType: @string           |                    |              |
|                                           | > > - line: template:stringPair  |                    |              |
+-------------------------------------------+----------------------------------+--------------------+--------------+
| > - line: @comments:clang comment         | template:namespace               | template:separator | @HashMaps    |
|                                           | - name: map                      |                    |              |
|                                           | > template:hashMap               |                    |              |
|                                           | > - name: clangComment           |                    |              |
|                                           | > - keyType: @id                 |                    |              |
|                                           | > - valueType: @string           |                    |              |
|                                           | > > - line: template:mapEntry    |                    |              |
+-------------------------------------------+----------------------------------+--------------------+--------------+
| > - line: @comments:css comment           | template:namespace               | template:separator | @HashMaps    |
|                                           | - name: map                      |                    |              |
|                                           | > template:hashMap               |                    |              |
|                                           | > - name: cssComment             |                    |              |
|                                           | > - keyType: @id                 |                    |              |
|                                           | > - valueType: @string           |                    |              |
|                                           | > > - line: template:mapEntry    |                    |              |
+-------------------------------------------+----------------------------------+--------------------+--------------+
| > - line: @comments:html comment          | template:namespace               | template:separator | @HashMaps    |
|                                           | - name: map                      |                    |              |
|                                           | > template:hashMap               |                    |              |
|                                           | > - name: htmlComment            |                    |              |
|                                           | > - keyType: @id                 |                    |              |
|                                           | > - valueType: @string           |                    |              |
|                                           | > > - line: template:mapEntry    |                    |              |
+-------------------------------------------+----------------------------------+--------------------+--------------+
| > - line: @comments:lua comment           | template:namespace               | template:separator | @HashMaps    |
|                                           | - name: map                      |                    |              |
|                                           | > template:hashMap               |                    |              |
|                                           | > - name: luaComment             |                    |              |
|                                           | > - keyType: @id                 |                    |              |
|                                           | > - valueType: @string           |                    |              |
|                                           | > > - line: template:mapEntry    |                    |              |
+-------------------------------------------+----------------------------------+--------------------+--------------+
| > - line: @comments:mermaid comment       | template:namespace               | template:separator | @HashMaps    |
|                                           | - name: map                      |                    |              |
|                                           | > template:hashMap               |                    |              |
|                                           | > - name: mermaidComment         |                    |              |
|                                           | > - keyType: @id                 |                    |              |
|                                           | > - valueType: @string           |                    |              |
|                                           | > > - line: template:mapEntry    |                    |              |
+-------------------------------------------+----------------------------------+--------------------+--------------+
| > - line: @comments:python comment        | template:namespace               | template:separator | @HashMaps    |
|                                           | - name: map                      |                    |              |
|                                           | > template:hashMap               |                    |              |
|                                           | > - name: pythonComment          |                    |              |
|                                           | > - keyType: @id                 |                    |              |
|                                           | > - valueType: @string           |                    |              |
|                                           | > > - line: template:mapEntry    |                    |              |
+-------------------------------------------+----------------------------------+--------------------+--------------+
| > - line: @comments:ruby comment          | template:namespace               | template:separator | @HashMaps    |
|                                           | - name: map                      |                    |              |
|                                           | > template:hashMap               |                    |              |
|                                           | > - name: rubyComment            |                    |              |
|                                           | > - keyType: @id                 |                    |              |
|                                           | > - valueType: @string           |                    |              |
|                                           | > > - line: template:mapEntry    |                    |              |
+-------------------------------------------+----------------------------------+--------------------+--------------+
| > - line: @comments:shell comment         | template:namespace               | template:separator | @HashMaps    |
|                                           | - name: map                      |                    |              |
|                                           | > template:hashMap               |                    |              |
|                                           | > - name: shellComment           |                    |              |
|                                           | > - keyType: @id                 |                    |              |
|                                           | > - valueType: @string           |                    |              |
|                                           | > > - line: template:mapEntry    |                    |              |
+-------------------------------------------+----------------------------------+--------------------+--------------+
| > - line: @comments:comment syntax        | template:namespace               |                    | @HashMaps    |
|                                           | - name: map                      |                    |              |
|                                           | > template:hashMap               |                    |              |
|                                           | > - name: commentSyntax          |                    |              |
|                                           | > - keyType: @string             |                    |              |
|                                           | > - valueType: @commentMap       |                    |              |
|                                           | > > - line: template:stringEntry |                    |              |
+-------------------------------------------+----------------------------------+--------------------+--------------+
| > > > - line: @tokens:template token type | template:namespace               |                    | @Bimaps      |
| > > - line: @tokens:template token type   | - name: map                      |                    |              |
|                                           | > template:bimap                 |                    |              |
|                                           | > - name: TemplateTokenType      |                    |              |
|                                           | > - type: int                    |                    |              |
|                                           | > - instance:                    |                    |              |
|                                           | > > > - line: template:mapEntry  |                    |              |
|                                           | > > - line: template:enum        |                    |              |
+-------------------------------------------+----------------------------------+--------------------+--------------+

## output index

+--------------------+-----------------------------------+-----------+------------+
| placeholder        | structure                         | separator | file       |
+====================+===================================+===========+============+
| - files: file      | template:generated                |           | @Generated |
| > - line: instance | - files: template:include         |           |            |
|                    | > - line: template:sharedInstance |           |            |
+--------------------+-----------------------------------+-----------+------------+
