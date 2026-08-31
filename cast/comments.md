## clang comment

```
@brief C-family comment syntax — the delimiters for a clang-style target.

Carries every frame the `:::comment:::` marker renders into for a C-like
file: the single-line trailing marker (`Id::comment`, `///<`), the block
brief marker (`Id::brief`, `@brief`), the block delimiters (`Id::blockOpen`
/ `Id::blockClose`), and the banner frame (`Id::bannerOpen` /
`Id::bannerClose`). Transforms.h and Writer.h read these keys to wrap
authored documentation in the output language's own syntax.
```

+-----------------+-------------------------------------------------------------------------------------+
| key             | value                                                                               |
+=================+=====================================================================================+
| Id::comment     | `///<`                                                                              |
| Id::brief       | `@brief`                                                                            |
| Id::blockOpen   | `/**`                                                                               |
| Id::blockLine   | ` *`                                                                                |
| Id::blockClose  | `*/`                                                                                |
| Id::bannerOpen  | `/*******************************************************************************`  |
| Id::bannerClose | `********************************************************************************/` |
+-----------------+-------------------------------------------------------------------------------------+

## css comment

```
@brief CSS comment syntax.

Carries the block frame (`Id::blockOpen` / `Id::blockClose`) and the banner
frame (`Id::bannerOpen` / `Id::bannerClose`) for a `.css` output. CSS has
no single-line marker, so `Id::comment` is absent and documentation renders
as a block.
```

+-----------------+-------------------------------------------------------------------------------------+
| key             | value                                                                               |
+=================+=====================================================================================+
| Id::blockOpen   | `/*`                                                                                |
| Id::blockClose  | `*/`                                                                                |
| Id::bannerOpen  | `/*******************************************************************************`  |
| Id::bannerClose | `********************************************************************************/` |
+-----------------+-------------------------------------------------------------------------------------+

## html comment

```
@brief HTML/XML comment syntax.

Carries the block frame (`<!--` / `-->`), which doubles as the banner frame
for HTML and XML outputs; neither language has a distinct single-line or
banner marker, so both keys map to the same delimiters.
```

+-----------------+--------+
| key             | value  |
+=================+========+
| Id::blockOpen   | `<!--` |
| Id::blockClose  | `-->`  |
| Id::bannerOpen  | `<!--` |
| Id::bannerClose | `-->`  |
+-----------------+--------+

## lua comment

```
@brief Lua comment syntax.

Carries the single-line marker (`Id::comment`, `---`) and the long-bracket
block frame (`--[[` / `]]`), which also serves as the banner frame for
`.lua` outputs.
```

+-----------------+--------+
| key             | value  |
+=================+========+
| Id::comment     | `---`  |
| Id::blockOpen   | `--[[` |
| Id::blockClose  | `]]`   |
| Id::bannerOpen  | `--[[` |
| Id::bannerClose | `]]`   |
+-----------------+--------+

## mermaid comment

```
@brief Mermaid comment syntax.

Carries only the single-line marker (`Id::comment`, `%%`); Mermaid has no
block comment, so documentation renders inline only.
```

+-------------+-------+
| key         | value |
+=============+=======+
| Id::comment | `%%`  |
+-------------+-------+

## python comment

```
@brief Python comment syntax.

Carries the single-line marker (`Id::comment`, `#`) and the docstring block
frame (`"""` / `"""`), which also serves as the banner frame for `.py`
outputs.
```

+-----------------+-------+
| key             | value |
+=================+=======+
| Id::comment     | `#`   |
| Id::blockOpen   | `"""` |
| Id::blockClose  | `"""` |
| Id::bannerOpen  | `"""` |
| Id::bannerClose | `"""` |
+-----------------+-------+

## ruby comment

```
@brief Ruby comment syntax.

Carries the single-line marker (`Id::comment`, `#`) and the `=begin` /
`=end` block frame, which also serves as the banner frame for `.rb`
outputs.
```

+-----------------+----------+
| key             | value    |
+=================+==========+
| Id::comment     | `#`      |
| Id::blockOpen   | `=begin` |
| Id::blockClose  | `=end`   |
| Id::bannerOpen  | `=begin` |
| Id::bannerClose | `=end`   |
+-----------------+----------+

## shell comment

```
@brief Shell comment syntax.

Carries only the single-line marker (`Id::comment`, `#`); shell has no
block comment, so documentation renders inline only.
```

+-------------+-------+
| key         | value |
+=============+=======+
| Id::comment | `#`   |
+-------------+-------+

## comment syntax

```
@brief File extension to comment-syntax table.

Maps an output file's extension to the comment frame the `:::comment:::`
marker renders in. Transforms.h reads one row per output file to wrap
authored documentation, and Writer.h reads it to frame the banner. Each
extension keys the `clangComment`, `cssComment`, `htmlComment`,
`luaComment`, `mermaidComment`, `pythonComment`, `rubyComment`, or
`shellComment` map above.
```

+-----------+---------------+
| key       | value         |
+===========+===============+
| `.h`      | clangComment  |
| `.cpp`    | clangComment  |
| `.c`      | clangComment  |
| `.js`     | clangComment  |
| `.ts`     | clangComment  |
| `.jsx`    | clangComment  |
| `.tsx`    | clangComment  |
| `.java`   | clangComment  |
| `.go`     | clangComment  |
| `.rs`     | clangComment  |
| `.rust`   | clangComment  |
| `.css`    | cssComment    |
| `.html`   | htmlComment   |
| `.xml`    | htmlComment   |
| `.lua`    | luaComment    |
| `.py`     | pythonComment |
| `.python` | pythonComment |
| `.rb`     | rubyComment   |
| `.ruby`   | rubyComment   |
| `.sh`     | shellComment  |
| `.bash`   | shellComment  |
| `.shell`  | shellComment  |
| `.cmake`  | shellComment  |
| `.sql`    | shellComment  |
| `.yaml`   | shellComment  |
| `.yml`    | shellComment  |
+-----------+---------------+
