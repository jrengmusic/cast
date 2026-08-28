## banner

```
@brief Banner artwork — a colour name to one row of glyph cells.

The key is a palette colour name, resolved to its ARGB word through
`map::ColourNames::get (name)`; the value is the row's glyph text, each
`█` cell drawn in the row's own colour and each `░` cell blended from the
prior row's colour. Iterated in insertion order by main.cpp's banner
painter, which walks the rows top to bottom.
```

+--------------------+----------------------------------------------------------+
| key                | value                                                    |
+====================+==========================================================+
| `blueMana`         | `████████████  ████████████  ████████████  ████████████` |
| `helloSummer`      | `████░░░░████  ████░░░░████  ████░░░░████  ░░░░████░░░░` |
| `highBlue`         | `████    ░░░░  ████    ████  ████    ░░░░      ████    ` |
| `aquarius`         | `████          ████████████  ████████████      ████    ` |
| `homeworld`        | `████          ████░░░░████  ░░░░░░░░████      ████    ` |
| `oceanBlue`        | `████    ████  ████    ████  ████    ████      ████    ` |
| `swimmer`          | `████████████  ████    ████  ████████████      ████    ` |
| `wayBeyondTheBlue` | `░░░░░░░░░░░░  ░░░░    ░░░░  ░░░░░░░░░░░░      ░░░░    ` |
+--------------------+----------------------------------------------------------+
