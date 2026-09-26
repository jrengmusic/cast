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

+---------+---------------------+
| alias   | symbol              |
+=========+=====================+
| @cLang  | map::Family::cLang  |
| @python | map::Family::python |
| @shell  | map::Family::shell  |
| @markup | map::Family::markup |
| @data   | map::Family::data   |
| @js     | map::Family::js     |
+---------+---------------------+

## SyntaxTokenType

+-------------+-------+
| key         | value |
+=============+=======+
| keyword     | 0     |
| string      | 1     |
| comment     | 2     |
| number      | 3     |
| syntax type | 4     |
| punctuation | 5     |
+-------------+-------+

## CppKeyword

+------------------------+-------+
| key                    | value |
+========================+=======+
| token alignas          | 0     |
| token alignof          | 1     |
| token and              | 2     |
| token and_eq           | 3     |
| token asm              | 4     |
| token auto             | 5     |
| token bitand           | 6     |
| token bitor            | 7     |
| token bool             | 8     |
| token break            | 9     |
| token case             | 10    |
| token catch            | 11    |
| token char             | 12    |
| token class            | 13    |
| token compl            | 14    |
| token const            | 15    |
| token const_cast       | 16    |
| token constexpr        | 17    |
| token consteval        | 18    |
| token constinit        | 19    |
| token continue         | 20    |
| token co_await         | 21    |
| token co_return        | 22    |
| token co_yield         | 23    |
| token decltype         | 24    |
| token default          | 25    |
| token delete           | 26    |
| token do               | 27    |
| token double           | 28    |
| token dynamic_cast     | 29    |
| token else             | 30    |
| token enum             | 31    |
| token explicit         | 32    |
| token export           | 33    |
| token extern           | 34    |
| token false            | 35    |
| token float            | 36    |
| token for              | 37    |
| token friend           | 38    |
| token goto             | 39    |
| token if               | 40    |
| token inline           | 41    |
| token int              | 42    |
| token long             | 43    |
| token mutable          | 44    |
| token namespace        | 45    |
| token new              | 46    |
| token noexcept         | 47    |
| token not              | 48    |
| token not_eq           | 49    |
| token nullptr          | 50    |
| token operator         | 51    |
| token or               | 52    |
| token or_eq            | 53    |
| token private          | 54    |
| token protected        | 55    |
| token public           | 56    |
| token register         | 57    |
| token reinterpret_cast | 58    |
| token requires         | 59    |
| token return           | 60    |
| token short            | 61    |
| token signed           | 62    |
| token sizeof           | 63    |
| token static           | 64    |
| token static_assert    | 65    |
| token static_cast      | 66    |
| token struct           | 67    |
| token switch           | 68    |
| token template         | 69    |
| token this             | 70    |
| token thread_local     | 71    |
| token throw            | 72    |
| token true             | 73    |
| token try              | 74    |
| token typedef          | 75    |
| token typeid           | 76    |
| token typename         | 77    |
| token union            | 78    |
| token unsigned         | 79    |
| token using            | 80    |
| token virtual          | 81    |
| token void             | 82    |
| token volatile         | 83    |
| token wchar_t          | 84    |
| token while            | 85    |
| token xor              | 86    |
| token xor_eq           | 87    |
| token override         | 88    |
| token final            | 89    |
| import                 | 90    |
| token module           | 91    |
| token concept          | 92    |
| token char8_t          | 93    |
| token char16_t         | 94    |
| token char32_t         | 95    |
+------------------------+-------+

## JsKeyword

+-----------------+-------+
| key             | value |
+=================+=======+
| await           | 0     |
| token break     | 1     |
| token case      | 2     |
| token catch     | 3     |
| token class     | 4     |
| token const     | 5     |
| token continue  | 6     |
| debugger        | 7     |
| token default   | 8     |
| token delete    | 9     |
| token do        | 10    |
| token else      | 11    |
| token export    | 12    |
| extends         | 13    |
| token false     | 14    |
| finally         | 15    |
| token for       | 16    |
| function        | 17    |
| token if        | 18    |
| import          | 19    |
| in              | 20    |
| instanceof      | 21    |
| let             | 22    |
| token new       | 23    |
| null            | 24    |
| of              | 25    |
| token return    | 26    |
| super           | 27    |
| token switch    | 28    |
| token this      | 29    |
| token throw     | 30    |
| token true      | 31    |
| token try       | 32    |
| typeof          | 33    |
| undefined       | 34    |
| var             | 35    |
| token void      | 36    |
| token while     | 37    |
| yield           | 38    |
| async           | 39    |
| from            | 40    |
| as              | 41    |
| type            | 42    |
| token interface | 43    |
| token enum      | 44    |
| implements      | 45    |
+-----------------+-------+

## PythonKeyword

+----------------+-------+
| key            | value |
+================+=======+
| token false    | 0     |
| none           | 1     |
| token true     | 2     |
| token and      | 3     |
| as             | 4     |
| token assert   | 5     |
| async          | 6     |
| await          | 7     |
| token break    | 8     |
| token class    | 9     |
| token continue | 10    |
| def            | 11    |
| del            | 12    |
| elif           | 13    |
| token else     | 14    |
| except         | 15    |
| finally        | 16    |
| token for      | 17    |
| from           | 18    |
| global         | 19    |
| token if       | 20    |
| import         | 21    |
| in             | 22    |
| is             | 23    |
| lambda         | 24    |
| nonlocal       | 25    |
| token not      | 26    |
| token or       | 27    |
| pass           | 28    |
| raise          | 29    |
| token return   | 30    |
| token try      | 31    |
| token while    | 32    |
| with           | 33    |
| yield          | 34    |
+----------------+-------+

## languageFamily

```
@brief File extension to language-family classification.

Maps each supported file extension to the `map::Family` key of the language
family whose scanner should tokenize that file. Consumed at document load
time to pick the right syntax highlighter for a path's extension.
```

+--------------------+---------+
| key                | value   |
+====================+=========+
| Extensions::cpp    | @cLang  |
| Extensions::c      | @cLang  |
| Extensions::h      | @cLang  |
| Extensions::js     | @js     |
| Extensions::ts     | @js     |
| Extensions::jsx    | @js     |
| Extensions::tsx    | @js     |
| Extensions::java   | @cLang  |
| Extensions::go     | @cLang  |
| Extensions::rust   | @cLang  |
| Extensions::rs     | @cLang  |
| Extensions::css    | @cLang  |
| Extensions::lua    | @cLang  |
| Extensions::python | @python |
| Extensions::py     | @python |
| Extensions::ruby   | @python |
| Extensions::rb     | @python |
| Extensions::bash   | @shell  |
| Extensions::sh     | @shell  |
| Extensions::shell  | @shell  |
| Extensions::cmake  | @shell  |
| Extensions::sql    | @shell  |
| Extensions::html   | @markup |
| Extensions::xml    | @markup |
| Extensions::json   | @data   |
| Extensions::yaml   | @data   |
| Extensions::yml    | @data   |
+--------------------+---------+

## Family

+--------+-------+
| key    | value |
+========+=======+
| c lang | 0     |
| python | 1     |
| shell  | 2     |
| markup | 3     |
| data   | 4     |
| js     | 5     |
+--------+-------+
