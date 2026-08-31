# CAST Metadata

## project info

```
@brief Project metadata — the ProjectInfo namespace, generated.

Every field is a complete literal; nothing downstream derives, concatenates, or restates a value.
```

+-------------------+------------------+--------+--------------------------+-----------+--------------------------------------------------+
| type              | name             | format | value                    | format    | comment                                          |
+===================+==================+========+==========================+===========+==================================================+
| const char* const | projectName      |        | cast                     | toLiteral | Product name.                                    |
| const char* const | companyName      |        | JRENG                    | toLiteral | Company name.                                    |
| const char* const | legalCompanyName |        | PT JRENG Teknika         | toLiteral | Full legal company name.                         |
| const char* const | versionString    |        | 0.1.0                    | toLiteral | Product version string.                          |
| int               | versionNumber    |        | 0x100                    |           | Product version, JUCE hex encoding.              |
| const char* const | productWebsite   |        | `https://jrengmusic.com` |           | Product website URL.                             |
| const char* const | presetExtension  |        | cast                     | toLiteral | Preset file extension, without the leading dot.  |
| const char* const | presetDefault    |        | INIT                     | toLiteral | Default init preset name, without the extension. |
+-------------------+------------------+--------+--------------------------+-----------+--------------------------------------------------+
