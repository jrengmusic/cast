## template token type

```
@brief Template placeholder token kinds CAST substitutes during rendering.

Maps the two reserved placeholder names to their enum keys, and exposes
them through the shared-instance registry the engine reads. `text` is the
default kind (`getDefault()` returns its name); `placeholder` marks a
named token inside a `:::token:::` delimiter pair.
```

+-------------+-----+-------------------------+
| name        | key | comment                 |
+=============+=====+=========================+
| text        | 0   | Plain text placeholder. |
| placeholder | 1   | Placeholder token.      |
+-------------+-----+-------------------------+
