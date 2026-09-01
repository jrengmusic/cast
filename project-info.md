# CAST Metadata

## index

+---------------+------------------------------------------------+
| alias         | symbol                                         |
+===============+================================================+
| @char         | const char* const                              |
| @juce-path    | ${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE         |
| @user-module  | ${CMAKE_CURRENT_SOURCE_DIR}/../jam             |
| @jam          | ${CAST_USER_MODULE_PATH}                       |
| @source       | ${CMAKE_CURRENT_SOURCE_DIR}/Source             |
| @generated    | ${CMAKE_CURRENT_SOURCE_DIR}/Source/generated   |
| @entitlements | ${CMAKE_CURRENT_SOURCE_DIR}/entitlements.plist |
| @jamSvg       | ${CAST_USER_MODULE_PATH}/resources/svg/*.svg   |
+---------------+------------------------------------------------+

## project info

```
@brief Project metadata — the ProjectInfo namespace, generated.

Every field is a complete literal; nothing downstream derives, concatenates, or restates a value.
```

+------------------+-------+--------------------------------------------------+-----------+--------------------------------------------------+
| name             | type  | value                                            | format    | comment                                          |
+==================+=======+==================================================+===========+==================================================+
| projectName      | @char | cast                                             | toLiteral | Codegen Annotated Source of Truth                |
| companyName      | @char | JRENG                                            | toLiteral | Company name.                                    |
| legalCompanyName | @char | Jubilant Research of Eclectic Novelty Generation | toLiteral | Full legal company name.                         |
| versionString    | @char | 0.1.0                                            | toLiteral | Product version string.                          |
| versionNumber    | int   | 0x100                                            |           | Product version, JUCE hex encoding.              |
| productWebsite   | @char | `https://jrengmusic.com`                         |           | Product website URL.                             |
| companyEmail     | @char | info@jrengmusic.com                              | toLiteral | Company contact email.                           |
| presetExtension  | @char | cast                                             | toLiteral | Preset file extension, without the leading dot.  |
| presetDefault    | @char | INIT                                             | toLiteral | Default init preset name, without the extension. |
+------------------+-------+--------------------------------------------------+-----------+--------------------------------------------------+

## cmake

+--------------------+----------------------------------------------------------+------------------------------------------------+
| key                | value                                                    | comment                                        |
+====================+==========================================================+================================================+
| description        | Code Annotated Source of Truth                           | Project description, printed at configure time |
|                    | ████████████░░████████████░░████████████░░████████████░░ |                                                |
|                    | ████░░  ████░░████░░  ████░░████░░  ████░░    ████░░     |                                                |
|                    | ████░░        ████░░  ████░░████░░            ████░░     |                                                |
|                    | ████░░        ████████████░░████████████░░    ████░░     |                                                |
|                    | ████░░        ████░░  ████░░        ████░░    ████░░     |                                                |
|                    | ████░░  ████░░████░░  ████░░████░░  ████░░    ████░░     |                                                |
|                    | ████████████░░████░░  ████░░████████████░░    ████░░     |                                                |
+--------------------+----------------------------------------------------------+------------------------------------------------+
| minimumVersion     | 3.25                                                     | CMake minimum version                          |
+--------------------+----------------------------------------------------------+------------------------------------------------+
| juceTargetFunction | juce_add_console_app                                     | JUCE target-creation function                  |
+--------------------+----------------------------------------------------------+------------------------------------------------+
| juceVersion        | 8.0.14                                                   | Required JUCE version, exact                   |
+--------------------+----------------------------------------------------------+------------------------------------------------+
| cxxStandard        | 17                                                       | C++ language standard                          |
+--------------------+----------------------------------------------------------+------------------------------------------------+
| deploymentTarget   | `11.0`                                                   | Minimum macOS deployment target                |
+--------------------+----------------------------------------------------------+------------------------------------------------+
| msvcRuntime        | `MultiThreaded$<$<CONFIG:Debug>:Debug>`                  | MSVC runtime library selection                 |
+--------------------+----------------------------------------------------------+------------------------------------------------+
| jucePath           | @juce-path                                               | JUCE root                                      |
+--------------------+----------------------------------------------------------+------------------------------------------------+
| userModulePath     | @user-module                                             | User module root                               |
+--------------------+----------------------------------------------------------+------------------------------------------------+

## architecture

+--------+---------+
| value  | comment |
+========+=========+
| x86_64 |         |
| arm64  |         |
+--------+---------+

## signing

+------------------+------------------------------------------------------+-----------------------------------+
| key              | value                                                | comment                           |
+==================+======================================================+===================================+
| identity         | Developer ID Application: Bayu Ardianto (9BDSN9TDX3) | Code signing identity             |
| entitlementsPath | @entitlements                                        | Entitlements file, project-local  |
| notaryProfile    | notary                                               | Keychain profile for notarization |
+------------------+------------------------------------------------------+-----------------------------------+

## juce module

+------+-----------+
| name | value     |
+======+===========+
| core | juce_core |
+------+-----------+

## user module

+------+--------------+---------------------------------------------------------------+
| root | name         | comment                                                       |
+======+==============+===============================================================+
| @jam | jam_core     | JAM Core                                                      |
| @jam | jam_markdown | Clean-room native CommonMark + GFM markdown parsing/rendering |
+------+--------------+---------------------------------------------------------------+

## source glob

+---------+-----------+-------------------------------------+
| path    | extension | comment                             |
+=========+===========+=====================================+
| @source | cpp       | Source .cpp files                   |
| @source | h         | Source headers, including generated |
+---------+-----------+-------------------------------------+

## define

+--------------------+---------------------------------+-----------------------------------------+
| name               | value                           | comment                                 |
+====================+=================================+=========================================+
| useJuceNamespace   | DONT_SET_USING_JUCE_NAMESPACE=1 | No using namespace juce in JuceHeader.h |
| declareProjectInfo | JUCE_DONT_DECLARE_PROJECTINFO=1 | No auto-generated ProjectInfo namespace |
+--------------------+---------------------------------+-----------------------------------------+

## include

+------------+------------+-------------------------------------------+
| name       | value      | comment                                   |
+============+============+===========================================+
| userModule | @jam       | User module root, for #include resolution |
| generated  | @generated | Generated headers                         |
+------------+------------+-------------------------------------------+

## binary

+------------+---------------------------------+---------------------------+
| name       | value                           | comment                   |
+============+=================================+===========================+
| help       | Source/HELP.md                  | Help document source      |
| style      | Source/style.css                | Stylesheet                |
| castOutput | Source/resources/cast-output.md | Output document resource  |
| jamSvg     | @jamSvg                         | JAM SVG icon assets, glob |
+------------+---------------------------------+---------------------------+

## clang flag

+-------------------------+--------------------------------+---------------------------------------------------------+
| name                    | value                          | comment                                                 |
+=========================+================================+=========================================================+
| floatEqual              | -Wno-float-equal               | DSP exact float comparisons for bypass detection        |
| signConversion          | -Wno-sign-conversion           | Array indexing, safe in this context                    |
| switchEnum              | -Wno-switch-enum               | Not every filter type needs every case handled          |
| unusedParameter         | -Wno-unused-parameter          | Debug/template code may not use all parameters          |
| floatToDoubleConversion | -Wno-implicit-float-conversion | DSP double/float conversions                            |
| shadow                  | -Wno-shadow                    | Lambda captures / declarations may shadow intentionally |
+-------------------------+--------------------------------+---------------------------------------------------------+

## msvc flag

+-------------------------+----------------+---------------------------------------------------------------------------------------+
| name                    | value          | comment                                                                               |
+=========================+================+=======================================================================================+
| unusedParameter         | /wd4100        | Debug/template code may not use all parameters                                        |
| floatToDoubleConversion | /wd4244        | DSP double/float conversions                                                          |
| shadow                  | /wd4456        | Lambda captures / declarations may shadow intentionally                               |
| permissiveMinus         | /permissive-   | Standards conformance matching clang — Function::Map identity-cast deduction needs it |
| rvalueCast              | /Zc:rvalueCast | Off by default, not implied by /permissive- — needed for correct T deduction          |
| warningLevel4           | /W4            | Warning level 4                                                                       |
| fullPathInPdb           | /FC            | Full path in PDB, required for debugger source-line resolution                        |
+-------------------------+----------------+---------------------------------------------------------------------------------------+
