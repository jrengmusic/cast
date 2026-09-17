/*******************************************************************************
                        Codegen Annotated Source of Truth
————————————————————————————————————————————————————————————————————————————————

            ░░████████████░░████████████░░████████████░░████████████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████        ░░████  ░░████░░████            ░░████
            ░░████        ░░████████████░░████████████    ░░████
            ░░████        ░░████  ░░████        ░░████    ░░████
            ░░████  ░░████░░████  ░░████░░████  ░░████    ░░████
            ░░████████████░░████  ░░████░░████████████    ░░████

————————————————————————————————————————————————————————————————————————————————
                         FOR YOUR EYES ONLY, DO NOT EDIT
********************************************************************************/

/**
 * @file Identifiers.h
 * @brief CAST's identifier and transform-name vocabulary.
 */

#pragma once

namespace Id
{
/*_____________________________________________________________________________*/

/**
 * @brief Identifier and transform-name constants CAST stamps and reads.
 *
 * juce::Identifier entries are the provenance and state keys the engine
 * stamps at parse and reads by name (§11.1). juce::String entries name the
 * transform operations a format cell may declare (§8).
 */

inline const juce::Identifier argument         { juce::String::fromUTF8 ("argument")           };///< Toolchain table argument column.
inline const juce::Identifier banner           { juce::String::fromUTF8 ("banner")             };///< Banner artwork key.
inline const juce::Identifier blockLine        { juce::String::fromUTF8 ("blockLine")          };///< Block-comment continuation-line glyph key.
inline const juce::Identifier syncBoundary     { juce::String::fromUTF8 ("boundary")           };///< Sync identity-row boundary column key.
inline const juce::Identifier brief            { juce::String::fromUTF8 ("brief")              };///< Brief documentation key.
inline const juce::Identifier command          { juce::String::fromUTF8 ("command")            };///< Toolchain table command column.
inline const juce::Identifier filePrefix       { juce::String::fromUTF8 ("filePrefix")         };///< Sync composed identity key — file-name prefix.
inline const juce::Identifier flag             { juce::String::fromUTF8 ("flag")               };///< Toolchain table flag column.
inline const juce::String     fromCodepoint    { juce::String::fromUTF8 ("from codepoint")     };///< Codepoint decode operation.
inline const juce::String     fromUTF8         { juce::String::fromUTF8 ("from UTF8")          };///< UTF-8 decode operation.
inline const juce::Identifier hyphenPrefix     { juce::String::fromUTF8 ("hyphenPrefix")       };///< Sync composed identity key — hyphen-case prefix.
inline const juce::Identifier identity         { juce::String::fromUTF8 ("identity")           };///< Sync info-file identity table name.
inline const juce::Identifier ignore           { juce::String::fromUTF8 ("ignore")             };///< Sync info-file ignore table name.
inline const juce::String     join             { juce::String::fromUTF8 ("join")               };///< Join text operation.
inline const juce::Identifier kernel           { juce::String::fromUTF8 ("kernel")             };///< Sync module-row class keyword — a walked directory.
inline const juce::Identifier list             { juce::String::fromUTF8 ("list")               };///< Reserved expansion token name.
inline const juce::Identifier macroPrefix      { juce::String::fromUTF8 ("macroPrefix")        };///< Sync composed identity key — macro-name prefix.
inline const juce::Identifier noBanner         { juce::String::fromUTF8 ("no-banner")          };///< Banner-suppressing fence-prefix marker.
inline const juce::Identifier noFormat         { juce::String::fromUTF8 ("no-format")          };///< Formatless-column marker.
inline const juce::Identifier placeholder      { juce::String::fromUTF8 ("placeholder")        };///< Placeholder token name.
inline const juce::Identifier separator        { juce::String::fromUTF8 ("separator")          };///< Separator column key.
inline const juce::Identifier structure        { juce::String::fromUTF8 ("structure")          };///< Structure column key.
inline const juce::Identifier symbol           { juce::String::fromUTF8 ("symbol")             };///< Index symbol column key.
inline const juce::Identifier sync             { juce::String::fromUTF8 ("sync")               };///< --sync CLI flag word.
inline const juce::Identifier templatePath     { juce::String::fromUTF8 ("template")           };///< Template file path stamp.
inline const juce::String     toCamel          { juce::String::fromUTF8 ("to camel")           };///< camelCase operation.
inline const juce::String     toCodepoint      { juce::String::fromUTF8 ("to codepoint")       };///< Codepoint encode operation.
inline const juce::String     toFileName       { juce::String::fromUTF8 ("to file name")       };///< File-name transform operation.
inline const juce::String     toHex            { juce::String::fromUTF8 ("to hex")             };///< Hex encode operation.
inline const juce::String     toKebab          { juce::String::fromUTF8 ("to kebab")           };///< kebab-case operation.
inline const juce::String     toLiteral        { juce::String::fromUTF8 ("to literal")         };///< Literal delimiting/escaping operation.
inline const juce::Identifier toolchain        { juce::String::fromUTF8 ("toolchain")          };///< Reserved toolchain manifest table.
inline const juce::String     toPascal         { juce::String::fromUTF8 ("to pascal")          };///< PascalCase operation.
inline const juce::String     toScreamingSnake { juce::String::fromUTF8 ("to screaming snake") };///< SCREAMING_SNAKE_CASE operation.
inline const juce::String     toSnake          { juce::String::fromUTF8 ("to snake")           };///< snake_case operation.
inline const juce::String     toTitle          { juce::String::fromUTF8 ("to title")           };///< Title Case operation.
inline const juce::String     toUpper          { juce::String::fromUTF8 ("to upper")           };///< UPPERCASE operation.
inline const juce::String     toUTF8           { juce::String::fromUTF8 ("to UTF8")            };///< UTF-8 encode operation.
inline const juce::Identifier wiring           { juce::String::fromUTF8 ("wiring")             };///< Manifest wiring-table classification.
inline const juce::Identifier word             { juce::String::fromUTF8 ("word")               };///< Sync identity-row boundary keyword — whole-word matching.

/**______________________________END OF NAMESPACE______________________________*/
}// namespace Id
