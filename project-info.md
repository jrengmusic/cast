# Project Info

## index

+--------------+--------------------------------------------------------+
| alias        | symbol                                                 |
+==============+========================================================+
| @char        | const char* const                                      |
| @juce-path   | ${CMAKE_CURRENT_SOURCE_DIR}/../../JUCE                 |
| @user-module | ${CMAKE_CURRENT_SOURCE_DIR}/../jam                     |
| @source      | ${CMAKE_CURRENT_SOURCE_DIR}/Source                     |
| @generated   | ${CMAKE_CURRENT_SOURCE_DIR}/Source/generated           |
| @jamSvg      | ${CMAKE_CURRENT_SOURCE_DIR}/../jam/resources/svg/*.svg |
+--------------+--------------------------------------------------------+

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

+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| key                         | value                                                    | comment                                   |
+=============================+==========================================================+===========================================+
| description                 | Codegen Annotated Source of Truth                        | Project description, single line          |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| banner                      | ████████████░░████████████░░████████████░░████████████░░ | Banner artwork, printed at configure time |
|                             | ████░░  ████░░████░░  ████░░████░░  ████░░    ████░░     |                                           |
|                             | ████░░        ████░░  ████░░████░░            ████░░     |                                           |
|                             | ████░░        ████████████░░████████████░░    ████░░     |                                           |
|                             | ████░░        ████░░  ████░░        ████░░    ████░░     |                                           |
|                             | ████░░  ████░░████░░  ████░░████░░  ████░░    ████░░     |                                           |
|                             | ████████████░░████░░  ████░░████████████░░    ████░░     |                                           |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| minimumVersion              | 3.25                                                     | CMake minimum version                     |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| juceTargetFunction          | juce_add_console_app                                     | JUCE target-creation function             |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| juceVersion                 | 8.0.14                                                   | Required JUCE version, exact              |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| cxxStandard                 | 17                                                       | C++ language standard                     |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| deploymentTarget            | `11.0`                                                   | Minimum macOS deployment target           |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| msvcRuntime                 | `MultiThreaded$<$<CONFIG:Debug>:Debug>`                  | MSVC runtime library selection            |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| jucePath                    | @juce-path                                               | JUCE root                                 |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| userModulePath              | @user-module                                             | User module root                          |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| installDirectory            | $ENV{HOME}/.local/bin                                    | Installed binary directory                |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| bundleIdentifier            | com.jrengmusic.cast                                      | JUCE BUNDLE_ID                            |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| companyCopyright            | © 2026 JRENG                                             | JUCE COMPANY_COPYRIGHT                    |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| productName                 | cast                                                     | JUCE PRODUCT_NAME                         |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| buildVersion                | 0.1.0                                                    | JUCE BUILD_VERSION                        |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| documentExtensions          | cast                                                     | JUCE DOCUMENT_EXTENSIONS                  |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| binaryDataNamespace         | BinaryData                                               | juce_add_binary_data NAMESPACE            |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| needsCurl                   | OFF                                                      | JUCE NEEDS_CURL                           |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| needsWebBrowser             | OFF                                                      | JUCE NEEDS_WEB_BROWSER                    |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| needsWebview2               | OFF                                                      | JUCE NEEDS_WEBVIEW2                       |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| needsStoreKit               | OFF                                                      | JUCE NEEDS_STORE_KIT                      |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+
| interproceduralOptimization | ON                                                       | CMAKE_INTERPROCEDURAL_OPTIMIZATION        |
+-----------------------------+----------------------------------------------------------+-------------------------------------------+

## signing

+------------------+--------------------------------------------------------------+---------------------------------+
| key              | value                                                        | comment                         |
+==================+==============================================================+=================================+
| identity         | Developer ID Application: Bayu Ardianto \\\\(9BDSN9TDX3\\\\) | Code signing identity           |
| entitlementsPath | entitlements.plist                                           | Entitlements file, project root |
| notaryProfile    | notary                                                       | Keychain notarization profile   |
+------------------+--------------------------------------------------------------+---------------------------------+

## architecture

+--------+
| value  |
+========+
| x86_64 |
| arm64  |
+--------+

## juce module

+------+-----------+
| name | value     |
+======+===========+
| core | juce_core |
+------+-----------+

## user module

+--------------+--------------+---------------------------------------------------------------+
| root         | name         | comment                                                       |
+==============+==============+===============================================================+
| @user-module | jam_core     | JAM Core                                                      |
| @user-module | jam_markdown | Clean-room native CommonMark + GFM markdown parsing/rendering |
+--------------+--------------+---------------------------------------------------------------+

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

+------------+--------------+-------------------------------------------+
| name       | value        | comment                                   |
+============+==============+===========================================+
| userModule | @user-module | User module root, for #include resolution |
| generated  | @generated   | Generated headers                         |
+------------+--------------+-------------------------------------------+

## binary

+------------+---------------------------------+---------------------------+
| name       | value                           | comment                   |
+============+=================================+===========================+
| help       | Source/HELP.md                  | Help document source      |
| style      | Source/style.css                | Stylesheet                |
| castOutput | Source/resources/cast-output.md | Output document resource  |
| jamSvg     | @jamSvg                         | JAM SVG icon assets, glob |
+------------+---------------------------------+---------------------------+

## toolchain

+----------+---------+------------------------------------------------------------+
| argument | command | flag                                                       |
+==========+=========+============================================================+
| debug    | cmake   | -S . -B Builds/Release -G Ninja -DCMAKE_BUILD_TYPE=Release |
| debug    | ninja   | -C Builds/Release                                          |
|          | cmake   | -S . -B Builds/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug     |
|          | ninja   | -C Builds/Debug                                            |
+----------+---------+------------------------------------------------------------+

## release

+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------------------------------------+
| name                    | mac                            | win            | stage  | comment                                                                               |
+=========================+================================+================+========+=======================================================================================+
| shadow                  | -Wno-shadow                    | /wd4456        |        | Lambda captures / declarations may shadow intentionally                               |
| unusedParameter         | -Wno-unused-parameter          | /wd4100        |        | Debug/template code may not use all parameters                                        |
| floatEqual              | -Wno-float-equal               |                |        | DSP exact float comparisons for bypass detection                                      |
| signConversion          | -Wno-sign-conversion           |                |        | Array indexing, safe in this context                                                  |
| switchEnum              | -Wno-switch-enum               |                |        | Not every filter type needs every case handled                                        |
| floatToDoubleConversion | -Wno-implicit-float-conversion | /wd4244        |        | DSP double/float conversions                                                          |
| permissiveMinus         |                                | /permissive-   |        | Standards conformance matching clang — Function::Map identity-cast deduction needs it |
| rvalueCast              |                                | /Zc:rvalueCast |        | Off by default, not implied by /permissive- — needed for correct T deduction          |
| warningLevel4           |                                | /W4            |        | Warning level 4                                                                       |
| fullPathInPdb           |                                | /FC            |        | Full path in PDB, required for debugger source-line resolution                        |
| optimization            | -O3                            | /O2            |        | Full optimization                                                                     |
| linkTimeOptimization    | -flto=thin                     | /GL            |        | Link-time optimization codegen                                                        |
| linkTimeCodegen         |                                | /LTCG          | linker | MSVC whole-program link-time codegen                                                  |
| deadCodeStripping       | -dead_strip                    | /OPT:REF       | linker | Strip unreferenced functions and data                                                 |
| identicalCodeFolding    |                                | /OPT:ICF       | linker | Fold identical COMDATs                                                                |
+-------------------------+--------------------------------+----------------+--------+---------------------------------------------------------------------------------------+

## debug

+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------------------------------------+
| name                    | mac                            | win            | stage | comment                                                                               |
+=========================+================================+================+=======+=======================================================================================+
| shadow                  | -Wno-shadow                    | /wd4456        |       | Lambda captures / declarations may shadow intentionally                               |
| unusedParameter         | -Wno-unused-parameter          | /wd4100        |       | Debug/template code may not use all parameters                                        |
| floatEqual              | -Wno-float-equal               |                |       | DSP exact float comparisons for bypass detection                                      |
| signConversion          | -Wno-sign-conversion           |                |       | Array indexing, safe in this context                                                  |
| switchEnum              | -Wno-switch-enum               |                |       | Not every filter type needs every case handled                                        |
| floatToDoubleConversion | -Wno-implicit-float-conversion | /wd4244        |       | DSP double/float conversions                                                          |
| permissiveMinus         |                                | /permissive-   |       | Standards conformance matching clang — Function::Map identity-cast deduction needs it |
| rvalueCast              |                                | /Zc:rvalueCast |       | Off by default, not implied by /permissive- — needed for correct T deduction          |
| warningLevel4           |                                | /W4            |       | Warning level 4                                                                       |
| fullPathInPdb           |                                | /FC            |       | Full path in PDB, required for debugger source-line resolution                        |
| optimization            | -O0                            | /Od            |       | No optimization                                                                       |
| debugSymbols            | -g                             | /Zi            |       | Debug symbols                                                                         |
+-------------------------+--------------------------------+----------------+-------+---------------------------------------------------------------------------------------+
