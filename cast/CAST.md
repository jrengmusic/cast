## index

+---------------+----------------------------------------------+
| alias         | symbol                                       |
+===============+==============================================+
| @identifiers  | identifiers.md                               |
| @text         | text.md                                      |
| @binaryFiles  | files.md                                     |
| @banner       | banner.md                                    |
| @comments     | comments.md                                  |
| @code         | ../../jam/cast/code.cast                     |
| @CAST         | CAST.md                                      |
| @headers      | CAST.md                                      |
| @project-info | ../project-info.md                           |
| @Identifiers  | ../Source/generated/Identifiers.h            |
| @Text         | ../Source/generated/Text.h                   |
| @Files        | ../Source/generated/Files.h                  |
| @HashMaps     | ../Source/generated/HashMaps.h               |
| @Generated    | ../Source/generated/Generated.h              |
| @ProjectInfo  | ../Source/generated/ProjectInfo.h            |
| @id           | juce::Identifier                             |
| @string       | juce::String                                 |
| @bimap        | jam::Bimap<int>                              |
| @commentMap   | jam::HashMap<juce::Identifier, juce::String> |
| @cmake        | cmake.cast                                   |
| @CMakeLists   | ../CMakeLists.txt                            |
| @semicolon    | ;                                            |
+---------------+----------------------------------------------+

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

+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| list                                 | separator               | structure                   | file         | comment |
+======================================+=========================+=============================+==============+=========+
| - list: @project-info:cmake          | - list: @semicolon      | @cmake:cmake                | @CMakeLists  |         |
| - list: @project-info:project info   |                         |                             |              |         |
| - list: @project-info:signing        |                         |                             |              |         |
| - list: @project-info:architecture   | - list: @semicolon      | - list: @cmake:value        |              |         |
| - list: @project-info:clang flag     |                         | - list: @cmake:value        |              |         |
| - list: @project-info:msvc flag      |                         | - list: @cmake:value        |              |         |
| - list: @project-info:user module    |                         | - list: @cmake:module       |              |         |
| - list: @project-info:source glob    |                         | - list: @cmake:glob-pattern |              |         |
| - list: @project-info:define         |                         | - list: @cmake:value        |              |         |
| - list: @project-info:include        |                         | - list: @cmake:value        |              |         |
| - list: @project-info:juce module    |                         | - list: @cmake:value        |              |         |
| - list: @project-info:user module    |                         | - list: @cmake:link         |              |         |
| - list: @project-info:binary         |                         | - list: @cmake:value        |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| > - list: @project-info:project info |                         | @code:namespace             | @ProjectInfo |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: ProjectInfo         |              |         |
|                                      |                         | > - list: @code:constant    |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| - list: @identifiers:identifiers     |                         | @code:namespace             | @Identifiers |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: Id                  |              |         |
|                                      |                         | - list: @code:identifier    |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| > - list: @text:diagnostics          |                         | @code:namespace             | @Text        |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: text                |              |         |
|                                      |                         |                             |              |         |
|                                      |                         | @code:struct                |              |         |
|                                      |                         | - name: Diagnostics         |              |         |
|                                      |                         | > - list: @code:char        |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| - list: @binaryFiles:files           |                         | @code:namespace             | @Files       |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: files               |              |         |
|                                      |                         | - list: @code:identifier    |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| > - list: @banner:banner             | - list: @code:linebreak | @code:namespace             | @HashMaps    |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: map                 |              |         |
|                                      |                         |                             |              |         |
|                                      |                         | @code:hash-map              |              |         |
|                                      |                         | - name: banner              |              |         |
|                                      |                         | - keyType: @string          |              |         |
|                                      |                         | - valueType: @string        |              |         |
|                                      |                         | > - list: @code:utf8-entry  |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| > - list: @comments:clang comment    | - list: @code:linebreak | @code:namespace             | @HashMaps    |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: map                 |              |         |
|                                      |                         |                             |              |         |
|                                      |                         | @code:hash-map              |              |         |
|                                      |                         | - name: clangComment        |              |         |
|                                      |                         | - keyType: @id              |              |         |
|                                      |                         | - valueType: @string        |              |         |
|                                      |                         | > - list: @code:map-entry   |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| > - list: @comments:cmake comment    | - list: @code:linebreak | @code:namespace             | @HashMaps    |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: map                 |              |         |
|                                      |                         |                             |              |         |
|                                      |                         | @code:hash-map              |              |         |
|                                      |                         | - name: cmakeComment        |              |         |
|                                      |                         | - keyType: @id              |              |         |
|                                      |                         | - valueType: @string        |              |         |
|                                      |                         | > - list: @code:map-entry   |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| > - list: @comments:css comment      | - list: @code:linebreak | @code:namespace             | @HashMaps    |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: map                 |              |         |
|                                      |                         |                             |              |         |
|                                      |                         | @code:hash-map              |              |         |
|                                      |                         | - name: cssComment          |              |         |
|                                      |                         | - keyType: @id              |              |         |
|                                      |                         | - valueType: @string        |              |         |
|                                      |                         | > - list: @code:map-entry   |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| > - list: @comments:gomod comment    | - list: @code:linebreak | @code:namespace             | @HashMaps    |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: map                 |              |         |
|                                      |                         |                             |              |         |
|                                      |                         | @code:hash-map              |              |         |
|                                      |                         | - name: gomodComment        |              |         |
|                                      |                         | - keyType: @id              |              |         |
|                                      |                         | - valueType: @string        |              |         |
|                                      |                         | > - list: @code:map-entry   |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| > - list: @comments:html comment     | - list: @code:linebreak | @code:namespace             | @HashMaps    |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: map                 |              |         |
|                                      |                         |                             |              |         |
|                                      |                         | @code:hash-map              |              |         |
|                                      |                         | - name: htmlComment         |              |         |
|                                      |                         | - keyType: @id              |              |         |
|                                      |                         | - valueType: @string        |              |         |
|                                      |                         | > - list: @code:map-entry   |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| > - list: @comments:lua comment      | - list: @code:linebreak | @code:namespace             | @HashMaps    |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: map                 |              |         |
|                                      |                         |                             |              |         |
|                                      |                         | @code:hash-map              |              |         |
|                                      |                         | - name: luaComment          |              |         |
|                                      |                         | - keyType: @id              |              |         |
|                                      |                         | - valueType: @string        |              |         |
|                                      |                         | > - list: @code:map-entry   |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| > - list: @comments:mermaid comment  | - list: @code:linebreak | @code:namespace             | @HashMaps    |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: map                 |              |         |
|                                      |                         |                             |              |         |
|                                      |                         | @code:hash-map              |              |         |
|                                      |                         | - name: mermaidComment      |              |         |
|                                      |                         | - keyType: @id              |              |         |
|                                      |                         | - valueType: @string        |              |         |
|                                      |                         | > - list: @code:map-entry   |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| > - list: @comments:python comment   | - list: @code:linebreak | @code:namespace             | @HashMaps    |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: map                 |              |         |
|                                      |                         |                             |              |         |
|                                      |                         | @code:hash-map              |              |         |
|                                      |                         | - name: pythonComment       |              |         |
|                                      |                         | - keyType: @id              |              |         |
|                                      |                         | - valueType: @string        |              |         |
|                                      |                         | > - list: @code:map-entry   |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| > - list: @comments:ruby comment     | - list: @code:linebreak | @code:namespace             | @HashMaps    |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: map                 |              |         |
|                                      |                         |                             |              |         |
|                                      |                         | @code:hash-map              |              |         |
|                                      |                         | - name: rubyComment         |              |         |
|                                      |                         | - keyType: @id              |              |         |
|                                      |                         | - valueType: @string        |              |         |
|                                      |                         | > - list: @code:map-entry   |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| > - list: @comments:shell comment    | - list: @code:linebreak | @code:namespace             | @HashMaps    |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: map                 |              |         |
|                                      |                         |                             |              |         |
|                                      |                         | @code:hash-map              |              |         |
|                                      |                         | - name: shellComment        |              |         |
|                                      |                         | - keyType: @id              |              |         |
|                                      |                         | - valueType: @string        |              |         |
|                                      |                         | > - list: @code:map-entry   |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| > - list: @comments:comment syntax   | - list: @code:linebreak | @code:namespace             | @HashMaps    |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: map                 |              |         |
|                                      |                         |                             |              |         |
|                                      |                         | @code:hash-map              |              |         |
|                                      |                         | - name: commentSyntax       |              |         |
|                                      |                         | - keyType: @string          |              |         |
|                                      |                         | - valueType: @commentMap    |              |         |
|                                      |                         | > - list: @code:map-entry   |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+
| > - list: @comments:manifest syntax  | - list: @code:linebreak | @code:namespace             | @HashMaps    |         |
|                                      |                         | - macro: #pragma once       |              |         |
|                                      |                         | - name: map                 |              |         |
|                                      |                         |                             |              |         |
|                                      |                         | @code:hash-map              |              |         |
|                                      |                         | - name: manifestSyntax      |              |         |
|                                      |                         | - keyType: @string          |              |         |
|                                      |                         | - valueType: @string        |              |         |
|                                      |                         | > - list: @code:map-entry   |              |         |
+--------------------------------------+-------------------------+-----------------------------+--------------+---------+

## output index

+--------------------------+-----------+---------------------------------+------------+
| list                     | separator | structure                       | file       |
+==========================+===========+=================================+============+
| - list: @headers:headers |           | @code:struct                    | @Generated |
|                          |           | - macro: #pragma once           |            |
| > - list: instance       |           | - name: Generated               |            |
|                          |           | - type: map::Generated          |            |
|                          |           | - instance: generated           |            |
|                          |           | - list: @code:include           |            |
|                          |           | > - list: @code:shared-instance |            |
+--------------------------+-----------+---------------------------------+------------+

## headers

+---------------+
| file          |
+===============+
| ProjectInfo.h |
| Identifiers.h |
| Text.h        |
| Files.h       |
| HashMaps.h    |
+---------------+
