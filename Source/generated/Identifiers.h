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

extern const juce::Identifier argument;        ///< Toolchain table argument column.
extern const juce::Identifier assumeFilename;  ///< --assume-filename CLI flag word — stdin style search start.
extern const juce::Identifier background;      ///< --pack background option word.
extern const juce::String     backgroundPrefix;///< Dmg background image file-name prefix.
extern const juce::Identifier banner;          ///< Banner artwork key.
extern const juce::Identifier bannerClose;
extern const juce::Identifier bannerOpen;
extern const juce::Identifier begin;
extern const juce::Identifier blockClose;
extern const juce::Identifier blockLine;       ///< Block-comment continuation-line glyph key.
extern const juce::Identifier blockOpen;
extern const juce::Identifier bundleColumn;    ///< --pack layout option word.
extern const juce::Identifier comment;
extern const juce::String     dmg;             ///< Dmg archive extension.
extern const juce::Identifier doubleDash;      ///< Double-dash delimiter.
extern const juce::String     dsStore;         ///< Finder layout file name.
extern const juce::Identifier firstRow;        ///< --pack layout option word.
extern const juce::String     hdiutil;         ///< Dmg image tool command.
extern const juce::Identifier help;
extern const juce::Identifier iconSize;        ///< --pack layout option word.
extern const juce::Identifier linkColumn;      ///< --pack layout option word.
extern const juce::Identifier p;
extern const juce::Identifier pack;            ///< --pack CLI flag word.
extern const juce::Identifier rowSpacing;      ///< --pack layout option word.
extern const juce::Identifier syncBoundary;    ///< Sync identity-row boundary column key.
extern const juce::Identifier brief;           ///< Brief documentation key.
extern const juce::Identifier command;         ///< Toolchain table command column.
extern const juce::Identifier filePrefix;      ///< Sync composed identity key — file-name prefix.
extern const juce::Identifier flag;            ///< Toolchain table flag column.
extern const juce::String     fromCodepoint;   ///< Codepoint decode operation.
extern const juce::String     fromUTF8;        ///< UTF-8 decode operation.
extern const juce::Identifier identity;        ///< Sync info-file identity table name.
extern const juce::Identifier ignore;          ///< Sync info-file ignore table name.
extern const juce::Identifier inPlace;         ///< -i CLI flag word — format in place.
extern const juce::String     join;            ///< Join text operation.
extern const juce::Identifier kernel;          ///< Sync module-row class keyword — a walked directory.
extern const juce::Identifier lineWrap;        ///< Prose reflow width CLI flag word.
extern const juce::Identifier list;            ///< Reserved expansion token name.
extern const juce::Identifier macroPrefix;     ///< Sync composed identity key — macro-name prefix.
extern const juce::Identifier noBanner;        ///< Banner-suppressing fence-prefix marker.
extern const juce::Identifier noFormat;        ///< Formatless-column marker.
extern const juce::Identifier placeholder;     ///< Placeholder token name.
extern const juce::Identifier separator;       ///< Separator column key.
extern const juce::Identifier structure;       ///< Structure column key.
extern const juce::Identifier symbol;          ///< Index symbol column key.
extern const juce::Identifier sync;            ///< --sync CLI flag word.
extern const juce::Identifier templatePath;    ///< Template file path stamp.
extern const juce::String     toCamel;         ///< camelCase operation.
extern const juce::String     toCodepoint;     ///< Codepoint encode operation.
extern const juce::Identifier toComment;
extern const juce::Identifier toCommentBlock;
extern const juce::String     toFileName;      ///< File-name transform operation.
extern const juce::String     toHex;           ///< Hex encode operation.
extern const juce::String     toKebab;         ///< kebab-case operation.
extern const juce::Identifier tokenModule;
extern const juce::String     toLiteral;       ///< Literal delimiting/escaping operation.
extern const juce::Identifier toolchain;       ///< Reserved toolchain manifest table.
extern const juce::String     toPascal;        ///< PascalCase operation.
extern const juce::String     toScreamingSnake;///< SCREAMING_SNAKE_CASE operation.
extern const juce::String     toSnake;         ///< snake_case operation.
extern const juce::String     toTitle;         ///< Title Case operation.
extern const juce::String     toUpper;         ///< UPPERCASE operation.
extern const juce::String     toUTF8;          ///< UTF-8 encode operation.
extern const juce::Identifier windowLeft;      ///< --pack layout option word.
extern const juce::Identifier windowTop;       ///< --pack layout option word.
extern const juce::Identifier wiring;          ///< Manifest wiring-table classification.
extern const juce::Identifier word;            ///< Sync identity-row boundary keyword — whole-word matching.
extern const juce::String     zip;             ///< Zip archive extension.

/**______________________________END OF NAMESPACE______________________________*/
}// namespace Id
