## index

+---------------+------------------------------------------------+
| alias         | symbol                                         |
+===============+================================================+
| @identifiers  | identifiers.md                                 |
| @text         | text.md                                        |
| @binaryFiles  | files.md                                       |
| @banner       | banner.md                                      |
| @comments     | comments.md                                    |
| @template     | ../../jam/cast/template.cast                   |
| @CAST         | CAST.md                                        |
| @project-info | ../project-info.md                             |
| @Identifiers  | ../Source/generated/Identifiers.h              |
| @Text         | ../Source/generated/Text.h                     |
| @Files        | ../Source/generated/Files.h                    |
| @HashMaps     | ../Source/generated/HashMaps.h                 |
| @Generated    | ../Source/generated/Generated.h                |
| @ProjectInfo  | ../Source/generated/ProjectInfo.h              |
| @id           | juce::Identifier                               |
| @string       | juce::String                                   |
| @bimap        | jam::Bimap<int>                                |
| @commentMap   | `jam::HashMap<juce::Identifier, juce::String>` |
+---------------+------------------------------------------------+

## index comment

+--------------+-------------------------------------------------------------------------------+
| alias        | comment                                                                       |
+==============+===============================================================================+
| @Identifiers | ```                                                                           |
|              | @file Identifiers.h                                                           |
|              | @brief CAST's identifier and transform-name vocabulary.                       |
|              | ```                                                                           |
+--------------+-------------------------------------------------------------------------------+
| @Text        | ```                                                                           |
|              | @file Text.h                                                                  |
|              | @brief Engine failure-message strings — one constant per fatal.               |
|              | ```                                                                           |
+--------------+-------------------------------------------------------------------------------+
| @Files       | ```                                                                           |
|              | @file Files.h                                                                 |
|              | @brief CAST's own document file names, referenced by the engine.              |
|              | ```                                                                           |
+--------------+-------------------------------------------------------------------------------+
| @HashMaps    | ```                                                                           |
|              | @file HashMaps.h                                                              |
|              | @brief CAST's banner palette and per-extension comment-syntax tables.         |
|              | ```                                                                           |
+--------------+-------------------------------------------------------------------------------+
| @Generated   | ```                                                                           |
|              | @file Generated.h                                                             |
|              | @brief CAST's generated-header umbrella — re-exports every generated concern. |
|              |                                                                               |
|              | Aggregate of CAST's generated registries. Owns the jam::Generated shared-     |
|              | instance aggregate, giving one construction point for every generated symbol  |
|              | the engine references.                                                        |
|              | ```                                                                           |
+--------------+-------------------------------------------------------------------------------+

## output

+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| list                                 | separator                  | structure                    | file         | comment |
+======================================+============================+==============================+==============+=========+
| - list: @identifiers:identifiers     |                            | template:namespace           | @Identifiers |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: Id                   |              |         |
|                                      |                            | - list: template:identifier  |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| > - list: @text:diagnostics          |                            | template:namespace           | @Text        |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: text                 |              |         |
|                                      |                            |                              |              |         |
|                                      |                            | template:struct              |              |         |
|                                      |                            | - name: Diagnostics          |              |         |
|                                      |                            | > - list: template:char      |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| - list: @binaryFiles:files           |                            | template:namespace           | @Files       |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: files                |              |         |
|                                      |                            | - list: template:identifier  |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| > - list: @banner:banner             | - list: template:linebreak | template:namespace           | @HashMaps    |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: map                  |              |         |
|                                      |                            |                              |              |         |
|                                      |                            | template:hashMap             |              |         |
|                                      |                            | - name: banner               |              |         |
|                                      |                            | - keyType: @string           |              |         |
|                                      |                            | - valueType: @string         |              |         |
|                                      |                            | > - list: template:utf8Entry |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| > - list: @comments:clang comment    | - list: template:linebreak | template:namespace           | @HashMaps    |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: map                  |              |         |
|                                      |                            |                              |              |         |
|                                      |                            | template:hashMap             |              |         |
|                                      |                            | - name: clangComment         |              |         |
|                                      |                            | - keyType: @id               |              |         |
|                                      |                            | - valueType: @string         |              |         |
|                                      |                            | > - list: template:mapEntry  |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| > - list: @comments:cmake comment    | - list: template:linebreak | template:namespace           | @HashMaps    |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: map                  |              |         |
|                                      |                            |                              |              |         |
|                                      |                            | template:hashMap             |              |         |
|                                      |                            | - name: cmakeComment         |              |         |
|                                      |                            | - keyType: @id               |              |         |
|                                      |                            | - valueType: @string         |              |         |
|                                      |                            | > - list: template:mapEntry  |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| > - list: @comments:css comment      | - list: template:linebreak | template:namespace           | @HashMaps    |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: map                  |              |         |
|                                      |                            |                              |              |         |
|                                      |                            | template:hashMap             |              |         |
|                                      |                            | - name: cssComment           |              |         |
|                                      |                            | - keyType: @id               |              |         |
|                                      |                            | - valueType: @string         |              |         |
|                                      |                            | > - list: template:mapEntry  |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| > - list: @comments:gomod comment    | - list: template:linebreak | template:namespace           | @HashMaps    |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: map                  |              |         |
|                                      |                            |                              |              |         |
|                                      |                            | template:hashMap             |              |         |
|                                      |                            | - name: gomodComment         |              |         |
|                                      |                            | - keyType: @id               |              |         |
|                                      |                            | - valueType: @string         |              |         |
|                                      |                            | > - list: template:mapEntry  |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| > - list: @comments:html comment     | - list: template:linebreak | template:namespace           | @HashMaps    |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: map                  |              |         |
|                                      |                            |                              |              |         |
|                                      |                            | template:hashMap             |              |         |
|                                      |                            | - name: htmlComment          |              |         |
|                                      |                            | - keyType: @id               |              |         |
|                                      |                            | - valueType: @string         |              |         |
|                                      |                            | > - list: template:mapEntry  |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| > - list: @comments:lua comment      | - list: template:linebreak | template:namespace           | @HashMaps    |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: map                  |              |         |
|                                      |                            |                              |              |         |
|                                      |                            | template:hashMap             |              |         |
|                                      |                            | - name: luaComment           |              |         |
|                                      |                            | - keyType: @id               |              |         |
|                                      |                            | - valueType: @string         |              |         |
|                                      |                            | > - list: template:mapEntry  |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| > - list: @comments:mermaid comment  | - list: template:linebreak | template:namespace           | @HashMaps    |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: map                  |              |         |
|                                      |                            |                              |              |         |
|                                      |                            | template:hashMap             |              |         |
|                                      |                            | - name: mermaidComment       |              |         |
|                                      |                            | - keyType: @id               |              |         |
|                                      |                            | - valueType: @string         |              |         |
|                                      |                            | > - list: template:mapEntry  |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| > - list: @comments:python comment   | - list: template:linebreak | template:namespace           | @HashMaps    |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: map                  |              |         |
|                                      |                            |                              |              |         |
|                                      |                            | template:hashMap             |              |         |
|                                      |                            | - name: pythonComment        |              |         |
|                                      |                            | - keyType: @id               |              |         |
|                                      |                            | - valueType: @string         |              |         |
|                                      |                            | > - list: template:mapEntry  |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| > - list: @comments:ruby comment     | - list: template:linebreak | template:namespace           | @HashMaps    |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: map                  |              |         |
|                                      |                            |                              |              |         |
|                                      |                            | template:hashMap             |              |         |
|                                      |                            | - name: rubyComment          |              |         |
|                                      |                            | - keyType: @id               |              |         |
|                                      |                            | - valueType: @string         |              |         |
|                                      |                            | > - list: template:mapEntry  |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| > - list: @comments:shell comment    | - list: template:linebreak | template:namespace           | @HashMaps    |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: map                  |              |         |
|                                      |                            |                              |              |         |
|                                      |                            | template:hashMap             |              |         |
|                                      |                            | - name: shellComment         |              |         |
|                                      |                            | - keyType: @id               |              |         |
|                                      |                            | - valueType: @string         |              |         |
|                                      |                            | > - list: template:mapEntry  |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| > - list: @comments:comment syntax   | - list: template:linebreak | template:namespace           | @HashMaps    |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: map                  |              |         |
|                                      |                            |                              |              |         |
|                                      |                            | template:hashMap             |              |         |
|                                      |                            | - name: commentSyntax        |              |         |
|                                      |                            | - keyType: @string           |              |         |
|                                      |                            | - valueType: @commentMap     |              |         |
|                                      |                            | > - list: template:mapEntry  |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| > - list: @comments:manifest syntax  | - list: template:linebreak | template:namespace           | @HashMaps    |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: map                  |              |         |
|                                      |                            |                              |              |         |
|                                      |                            | template:hashMap             |              |         |
|                                      |                            | - name: manifestSyntax       |              |         |
|                                      |                            | - keyType: @string           |              |         |
|                                      |                            | - valueType: @string         |              |         |
|                                      |                            | > - list: template:mapEntry  |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+
| > - list: @project-info:project info |                            | template:namespace           | @ProjectInfo |         |
|                                      |                            | - macro: #pragma once        |              |         |
|                                      |                            | - name: ProjectInfo          |              |         |
|                                      |                            | > - list: template:constant  |              |         |
+--------------------------------------+----------------------------+------------------------------+--------------+---------+

## output index

+--------------------+-----------+-----------------------------------+------------+
| list               | separator | structure                         | file       |
+====================+===========+===================================+============+
| - list: file       |           | template:struct                   | @Generated |
|                    |           | - macro: #pragma once             |            |
| > - list: instance |           | - name: Generated                 |            |
|                    |           | - type: map::Generated            |            |
|                    |           | - instance: generated             |            |
|                    |           | - list: template:include          |            |
|                    |           | > - list: template:sharedInstance |            |
+--------------------+-----------+-----------------------------------+------------+
