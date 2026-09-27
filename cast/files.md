## index

+---------+--------------+
| alias   | symbol       |
+=========+==============+
| @string | juce::String |
+---------+--------------+

## files

```
@brief File and directory names CAST reads and writes: manifest, help, banner, style files, stdin name, sync info.
```

+---------+---------------------+------------------------+-------------------------------------------+
| type    | name                | value                  | comment                                   |
+=========+=====================+========================+===========================================+
| @string | cast                | `spell.md`             | Generation manifest.                      |
| @string | castDirectory       | `cast`                 | Sync style directory under a target root. |
| @string | castFormat          | `.cast-format`         | Style file name, searched first.          |
| @string | castFormatAlternate | `_cast-format`         | Style file name, searched second.         |
| @string | castHelp            | `HELP.md`              | Rendered help text.                       |
| @string | castOutput          | `cast-output.md`       | Banner artwork source.                    |
| @string | standardInput       | `<stdin>`              | Stdin file name in a diagnostic.          |
| @string | userModulesInfo     | `user-modules-info.md` | Sync root info file name.                 |
+---------+---------------------+------------------------+-------------------------------------------+
