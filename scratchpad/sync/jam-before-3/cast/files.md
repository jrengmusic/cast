```
████████████░░████████████░░████████████░░████████████░░
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░
████░░        ████░░  ████░░████░░            ████░░
████░░        ████████████░░████████████░░    ████░░
████░░        ████░░  ████░░        ████░░    ████░░
████░░  ████░░████░░  ████░░████░░  ████░░    ████░░
████████████░░████░░  ████░░████████████░░    ████░░
```

## index

+---------+------------------+
| alias   | symbol           |
+=========+==================+
| @id     | juce::Identifier |
| @string | juce::String     |
+---------+------------------+

## extensions

```
@brief File-extension strings, keyed by language name.

Each constant is the bare extension (no leading dot) of one supported
source or markup language. Extensions pair with map::languageFamily.
```

+--------+----------+----------------------------+
| name   | value    | comment                    |
+========+==========+============================+
| bash   | `bash`   | Bash script.               |
| c      | `c`      | C source.                  |
| cast   | `cast`   | CAST template block.       |
| cmake  | `cmake`  | CMake script.              |
| cpp    | `cpp`    | C++ source.                |
| css    | `css`    | Cascading stylesheet.      |
| go     | `go`     | Go source.                 |
| h      | `h`      | C/C++ header.              |
| html   | `html`   | HTML markup.               |
| java   | `java`   | Java source.               |
| js     | `js`     | JavaScript source.         |
| json   | `json`   | JSON data.                 |
| jsx    | `jsx`    | JSX markup.                |
| lua    | `lua`    | Lua script.                |
| md     | `md`     | Markdown.                  |
| py     | `py`     | Python source.             |
| python | `python` | Python source (long form). |
| rb     | `rb`     | Ruby source.               |
| rs     | `rs`     | Rust source.               |
| ruby   | `ruby`   | Ruby source (long form).   |
| rust   | `rust`   | Rust source (long form).   |
| sh     | `sh`     | Shell script.              |
| shell  | `shell`  | Shell script (long form).  |
| sql    | `sql`    | SQL script.                |
| svg    | `svg`    | SVG image.                 |
| ts     | `ts`     | TypeScript source.         |
| tsx    | `tsx`    | TSX markup.                |
| xml    | `xml`    | XML markup.                |
| yaml   | `yaml`   | YAML data.                 |
| yml    | `yml`    | YAML data (short form).    |
+--------+----------+----------------------------+

## files

```
@brief Framework asset file names — layouts, shaders, icons, stylesheets.

Each constant is the literal file name of an embedded resource, resolved
against the binary-data / asset search path at load time.
```

+---------+---------------------------+---------+-------------------------------+-----------+------------------------------------------+
| type    | name                      | format  | value                         | format    | comment                                  |
+=========+===========================+=========+===============================+===========+==========================================+
| @string | background combine frag   | toCamel | background_combine.frag.spv   | toLiteral | Background combine fragment shader.      |
| @string | background frag           | toCamel | background.frag.spv           | toLiteral | Background fragment shader.              |
| @string | calibration frag          | toCamel | calibration.frag.spv          | toLiteral | Calibration fragment shader.             |
| @string | calibration vert          | toCamel | calibration.vert.spv          | toLiteral | Calibration vertex shader.               |
| @string | close down                | toCamel | close_down.svg                | toLiteral | Close-button pressed icon (title bar).   |
| @string | close normal              | toCamel | close_normal.svg              | toLiteral | Close-button idle icon (title bar).      |
| @string | close one disabled        | toCamel | close_one_disabled.svg        | toLiteral | Circled close disabled icon (dialogs).   |
| @string | close one down            | toCamel | close_one_down.svg            | toLiteral | Circled close pressed icon (dialogs).    |
| @string | close one normal          | toCamel | close_one_normal.svg          | toLiteral | Circled close idle icon (dialogs).       |
| @string | close one over            | toCamel | close_one_over.svg            | toLiteral | Circled close hover icon (dialogs).      |
| @string | close over                | toCamel | close_over.svg                | toLiteral | Close-button hover icon (title bar).     |
| @string | component layout          | toCamel | component.md                  | toLiteral | Component descriptor tables.             |
| @string | default settings          | toCamel | DefaultSettings.xml           | toLiteral | Embedded default user-settings template. |
| @string | fill rect frag            | toCamel | fill_rect.frag.spv            | toLiteral | Rect-fill fragment shader.               |
| @string | fill rect vert            | toCamel | fill_rect.vert.spv            | toLiteral | Rect-fill vertex shader.                 |
| @string | glyph emoji frag          | toCamel | glyph_emoji.frag.spv          | toLiteral | Emoji glyph fragment shader.             |
| @string | glyph mono frag           | toCamel | glyph_mono.frag.spv           | toLiteral | Mono glyph fragment shader.              |
| @string | gradient fill frag        | toCamel | gradient_fill.frag.spv        | toLiteral | Gradient-fill fragment shader.           |
| @string | image alpha mask frag     | toCamel | image_alpha_mask.frag.spv     | toLiteral | Image alpha-mask fragment shader.        |
| @string | image frag                | toCamel | image.frag.spv                | toLiteral | Image fragment shader.                   |
| @string | inc dec down down         | toCamel | inc_dec_down_down.svg         | toLiteral | Inc/dec down-pressed icon.               |
| @string | inc dec down normal       | toCamel | inc_dec_down_normal.svg       | toLiteral | Inc/dec down-idle icon.                  |
| @string | inc dec down over         | toCamel | inc_dec_down_over.svg         | toLiteral | Inc/dec down-hover icon.                 |
| @string | inc dec up down           | toCamel | inc_dec_up_down.svg           | toLiteral | Inc/dec up-pressed icon.                 |
| @string | inc dec up normal         | toCamel | inc_dec_up_normal.svg         | toLiteral | Inc/dec up-idle icon.                    |
| @string | inc dec up over           | toCamel | inc_dec_up_over.svg           | toLiteral | Inc/dec up-hover icon.                   |
| @string | instanced masked vert     | toCamel | instanced_masked.vert.spv     | toLiteral | Instanced masked vertex shader.          |
| @string | instanced rect vert       | toCamel | instanced_rect.vert.spv       | toLiteral | Instanced rect vertex shader.            |
| @string | instanced vert            | toCamel | instanced.vert.spv            | toLiteral | Instanced vertex shader.                 |
| @string | logo                      | toCamel | logo.svg                      | toLiteral | Framework logo mark.                     |
| @string | manual suffix             | toCamel | ` Manual.pdf`                 |           | User-manual file-name suffix.            |
| @string | masked image frag         | toCamel | masked_image.frag.spv         | toLiteral | Masked-image fragment shader.            |
| @string | matte feather comp        | toCamel | matte_feather.comp.spv        | toLiteral | Matte-feather compute shader.            |
| @string | mermaid style sheet       | toCamel | mermaid.css                   | toLiteral | Mermaid diagram stylesheet.              |
| @string | mesh default frag         | toCamel | mesh_default.frag.spv         | toLiteral | Default mesh fragment shader.            |
| @string | mesh default vertex       | toCamel | mesh_default.vert             | toLiteral | Default mesh vertex shader.              |
| @string | mesh edge frag            | toCamel | mesh_edge.frag.spv            | toLiteral | Edge mesh fragment shader.               |
| @string | mesh edge vertex          | toCamel | mesh_edge.vert                | toLiteral | Edge mesh vertex shader.                 |
| @string | minus disabled            | toCamel | minus_disabled.svg            | toLiteral | Minus-button disabled icon.              |
| @string | minus disabled on         | toCamel | minus_disabledOn.svg          | toLiteral | Minus-button disabled on icon.           |
| @string | minus down                | toCamel | minus_down.svg                | toLiteral | Minus-button down icon.                  |
| @string | minus down on             | toCamel | minus_downOn.svg              | toLiteral | Minus-button down on icon.               |
| @string | minus normal              | toCamel | minus_normal.svg              | toLiteral | Minus-button normal icon.                |
| @string | minus normal on           | toCamel | minus_normalOn.svg            | toLiteral | Minus-button normal on icon.             |
| @string | minus over                | toCamel | minus_over.svg                | toLiteral | Minus-button over icon.                  |
| @string | minus over on             | toCamel | minus_overOn.svg              | toLiteral | Minus-button over on icon.               |
| @string | pane corner menu          | toCamel | pane_corner_menu.svg          | toLiteral | Pane corner-menu icon.                   |
| @string | parameters layout         | toCamel | parameters.md                 | toLiteral | Parameter descriptor tables.             |
| @string | plus disabled             | toCamel | plus_disabled.svg             | toLiteral | Plus-button disabled icon.               |
| @string | plus disabled on          | toCamel | plus_disabledOn.svg           | toLiteral | Plus-button disabled on icon.            |
| @string | plus down                 | toCamel | plus_down.svg                 | toLiteral | Plus-button down icon.                   |
| @string | plus down on              | toCamel | plus_downOn.svg               | toLiteral | Plus-button down on icon.                |
| @string | plus normal               | toCamel | plus_normal.svg               | toLiteral | Plus-button normal icon.                 |
| @string | plus normal on            | toCamel | plus_normalOn.svg             | toLiteral | Plus-button normal on icon.              |
| @string | plus over                 | toCamel | plus_over.svg                 | toLiteral | Plus-button over icon.                   |
| @string | plus over on              | toCamel | plus_overOn.svg               | toLiteral | Plus-button over on icon.                |
| @string | post process combine frag | toCamel | post_process_combine.frag.spv | toLiteral | Post-process combine fragment shader.    |
| @string | selector arrows           | toCamel | selector_arrows.svg           | toLiteral | Selector arrows icon.                    |
| @string | setting extension         | toCamel | .setting                      | toLiteral | User settings file extension.            |
| @string | settings directory        | toCamel | Settings                      | toLiteral | User settings directory name.            |
| @string | shader pass vert          | toCamel | shader_pass.vert.spv          | toLiteral | Shader-pass vertex shader.               |
| @string | shader toy channel macro  | toCamel | shaderToyChannelMacro.frag    | toLiteral | Shadertoy channel macro.                 |
| @string | shader toy scene macro    | toCamel | shaderToySceneMacro.frag      | toLiteral | Shadertoy scene macro.                   |
| @string | shadertoy wrapper         | toCamel | shadertoy_wrapper.frag        | toLiteral | Shadertoy wrapper fragment.              |
| @string | stack blur buffer comp    | toCamel | stack_blur_buffer.comp.spv    | toLiteral | Stack-blur buffer compute shader.        |
| @string | stack blur texture comp   | toCamel | stack_blur_texture.comp.spv   | toLiteral | Stack-blur texture compute shader.       |
| @string | standby                   | toCamel | standby.svg                   | toLiteral | Standby icon.                            |
| @string | straight alpha frag       | toCamel | straight_alpha.frag.spv       | toLiteral | Straight-alpha fragment shader.          |
| @string | style sheet               | toCamel | style.css                     | toLiteral | Application stylesheet.                  |
| @string | tiled image frag          | toCamel | tiled_image.frag.spv          | toLiteral | Tiled-image fragment shader.             |
+---------+---------------------------+---------+-------------------------------+-----------+------------------------------------------+
