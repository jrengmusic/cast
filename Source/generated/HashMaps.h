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
 * @file HashMaps.h
 * @brief CAST's banner palette and per-extension comment-syntax tables.
 */

#pragma once

namespace map
{
/*_____________________________________________________________________________*/

/**
 * @brief Banner artwork — a colour name to one row of glyph cells.
 *
 * The key is a palette colour name, resolved to its ARGB word through
 * `map::ColourNames::get (name)`; the value is the row's glyph text, each
 * `█` cell drawn in the row's own colour and each `░` cell blended from the
 * prior row's colour. Iterated in insertion order by main.cpp's banner
 * painter, which walks the rows top to bottom.
 */
inline const jam::HashMap<juce::String, juce::String> banner
{
    { juce::String::fromUTF8 ("blueMana"),         juce::String::fromUTF8 ("\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88") },
    { juce::String::fromUTF8 ("helloSummer"),      juce::String::fromUTF8 ("\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91") },
    { juce::String::fromUTF8 ("highBlue"),         juce::String::fromUTF8 ("\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88    \xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88    \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88    \xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91      \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88    ") },
    { juce::String::fromUTF8 ("aquarius"),         juce::String::fromUTF8 ("\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88          \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88      \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88    ") },
    { juce::String::fromUTF8 ("homeworld"),        juce::String::fromUTF8 ("\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88          \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88      \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88    ") },
    { juce::String::fromUTF8 ("oceanBlue"),        juce::String::fromUTF8 ("\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88    \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88    \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88    \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88      \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88    ") },
    { juce::String::fromUTF8 ("swimmer"),          juce::String::fromUTF8 ("\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88    \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88      \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88    ") },
    { juce::String::fromUTF8 ("wayBeyondTheBlue"), juce::String::fromUTF8 ("\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91  \xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91    \xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91  \xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91      \xe2\x96\x91\xe2\x96\x91\xe2\x96\x91\xe2\x96\x91    ") },
};

//==============================================================================

/**
 * @brief C-family comment syntax — the delimiters for a clang-style target.
 *
 * Carries every frame the `:::[comment]:::` marker renders into for a C-like
 * file: the single-line trailing marker (`Id::comment`, `///<`), the block
 * brief marker (`Id::brief`, `@brief`), the block delimiters (`Id::blockOpen`
 * / `Id::blockClose`), and the banner frame (`Id::bannerOpen` /
 * `Id::bannerClose`). Transforms.h and Writer.h read these keys to wrap
 * authored documentation in the output language's own syntax.
 */
inline const jam::HashMap<juce::Identifier, juce::String> clangComment
{
    { Id::comment,     "///<" },
    { Id::brief,       "@brief" },
    { Id::blockOpen,   "/**" },
    { Id::blockLine,   " *" },
    { Id::blockClose,  "*/" },
    { Id::bannerOpen,  "/*******************************************************************************" },
    { Id::bannerClose, "********************************************************************************/" },
};

//==============================================================================

/**
 * @brief CMake comment syntax.
 *
 * Carries the single-line marker (`Id::comment`, `#`) and the bracket-comment
 * block frame (`#[[` / `]]`), which also serves as the banner frame for
 * CMakeLists.txt and `.cmake` outputs.
 */
inline const jam::HashMap<juce::Identifier, juce::String> cmakeComment
{
    { Id::comment,     "#" },
    { Id::blockOpen,   "#[[" },
    { Id::blockClose,  "]]" },
    { Id::bannerOpen,  "#[[*****************************************************************************" },
    { Id::bannerClose, "******************************************************************************]]" },
};

//==============================================================================

/**
 * @brief CSS comment syntax.
 *
 * Carries the block frame (`Id::blockOpen` / `Id::blockClose`) and the banner
 * frame (`Id::bannerOpen` / `Id::bannerClose`) for a `.css` output. CSS has
 * no single-line marker, so `Id::comment` is absent and documentation renders
 * as a block.
 */
inline const jam::HashMap<juce::Identifier, juce::String> cssComment
{
    { Id::blockOpen,   "/*" },
    { Id::blockClose,  "*/" },
    { Id::bannerOpen,  "/*******************************************************************************" },
    { Id::bannerClose, "********************************************************************************/" },
};

//==============================================================================

/**
 * @brief go.mod comment syntax.
 *
 * Carries only the single-line marker (`Id::comment`, `//`); go.mod has no
 * block comment, so documentation renders inline only.
 */
inline const jam::HashMap<juce::Identifier, juce::String> gomodComment
{
    { Id::comment, "//" },
};

//==============================================================================

/**
 * @brief HTML/XML comment syntax.
 *
 * Carries the block frame (`<!--` / `-->`), which doubles as the banner frame
 * for HTML and XML outputs; neither language has a distinct single-line or
 * banner marker, so both keys map to the same delimiters.
 */
inline const jam::HashMap<juce::Identifier, juce::String> htmlComment
{
    { Id::blockOpen,   "<!--" },
    { Id::blockClose,  "-->" },
    { Id::bannerOpen,  "<!--" },
    { Id::bannerClose, "-->" },
};

//==============================================================================

/**
 * @brief Lua comment syntax.
 *
 * Carries the single-line marker (`Id::comment`, `---`) and the long-bracket
 * block frame (`--[[` / `]]`), which also serves as the banner frame for
 * `.lua` outputs.
 */
inline const jam::HashMap<juce::Identifier, juce::String> luaComment
{
    { Id::comment,     "---" },
    { Id::blockOpen,   "--[[" },
    { Id::blockClose,  "]]" },
    { Id::bannerOpen,  "--[[" },
    { Id::bannerClose, "]]" },
};

//==============================================================================

/**
 * @brief Mermaid comment syntax.
 *
 * Carries only the single-line marker (`Id::comment`, `%%`); Mermaid has no
 * block comment, so documentation renders inline only.
 */
inline const jam::HashMap<juce::Identifier, juce::String> mermaidComment
{
    { Id::comment, "%%" },
};

//==============================================================================

/**
 * @brief Python comment syntax.
 *
 * Carries the single-line marker (`Id::comment`, `#`) and the docstring block
 * frame (`"""` / `"""`), which also serves as the banner frame for `.py`
 * outputs.
 */
inline const jam::HashMap<juce::Identifier, juce::String> pythonComment
{
    { Id::comment,     "#" },
    { Id::blockOpen,   "\"\"\"" },
    { Id::blockClose,  "\"\"\"" },
    { Id::bannerOpen,  "\"\"\"" },
    { Id::bannerClose, "\"\"\"" },
};

//==============================================================================

/**
 * @brief Ruby comment syntax.
 *
 * Carries the single-line marker (`Id::comment`, `#`) and the `=begin` /
 * `=end` block frame, which also serves as the banner frame for `.rb`
 * outputs.
 */
inline const jam::HashMap<juce::Identifier, juce::String> rubyComment
{
    { Id::comment,     "#" },
    { Id::blockOpen,   "=begin" },
    { Id::blockClose,  "=end" },
    { Id::bannerOpen,  "=begin" },
    { Id::bannerClose, "=end" },
};

//==============================================================================

/**
 * @brief Shell comment syntax.
 *
 * Carries the single-line marker (`Id::comment`, `#`) alone; shell has no
 * block comment, so Transforms::toCommentBlock() renders a multi-line value
 * one `#`-marked line at a time, with no opening and no closing frame line.
 */
inline const jam::HashMap<juce::Identifier, juce::String> shellComment
{
    { Id::comment, "#" },
};

//==============================================================================

/**
 * @brief SQL comment syntax.
 *
 * Carries the single-line marker (`Id::comment`, `--`) and the C-style block
 * frame, which also serves as the banner frame for `.sql` outputs.
 */
inline const jam::HashMap<juce::Identifier, juce::String> sqlComment
{
    { Id::comment,     "--" },
    { Id::blockOpen,   "/*" },
    { Id::blockClose,  "*/" },
    { Id::bannerOpen,  "/*" },
    { Id::bannerClose, "*/" },
};

//==============================================================================

/**
 * @brief File extension to comment-syntax table.
 *
 * Maps an output file's extension to the comment frame the `:::[comment]:::`
 * marker renders in. Transforms.h reads one row per output file to wrap
 * authored documentation, and Writer.h reads it to frame the banner. Each
 * extension keys one of the `clangComment`, `cmakeComment`, `cssComment`,
 * `gomodComment`, `htmlComment`, `luaComment`, `mermaidComment`,
 * `pythonComment`, `rubyComment`, `shellComment` or `sqlComment` maps above.
 * An extension absent from this table falls back to `clangComment`.
 */
inline const jam::HashMap<juce::String, jam::HashMap<juce::Identifier, juce::String>> commentSyntax
{
    { ".h",       clangComment },
    { ".cpp",     clangComment },
    { ".c",       clangComment },
    { ".js",      clangComment },
    { ".ts",      clangComment },
    { ".jsx",     clangComment },
    { ".tsx",     clangComment },
    { ".java",    clangComment },
    { ".go",      clangComment },
    { ".rs",      clangComment },
    { ".rust",    clangComment },
    { ".cmake",   cmakeComment },
    { ".css",     cssComment },
    { ".mod",     gomodComment },
    { ".html",    htmlComment },
    { ".xml",     htmlComment },
    { ".lua",     luaComment },
    { ".mmd",     mermaidComment },
    { ".mermaid", mermaidComment },
    { ".py",      pythonComment },
    { ".python",  pythonComment },
    { ".rb",      rubyComment },
    { ".ruby",    rubyComment },
    { ".sh",      shellComment },
    { ".bash",    shellComment },
    { ".shell",   shellComment },
    { ".toml",    shellComment },
    { ".sql",     sqlComment },
    { ".yaml",    shellComment },
    { ".yml",     shellComment },
};

//==============================================================================

/**
 * @brief Manifest file name to comment-syntax key table.
 *
 * Build-manifest files carry deterministic names whose extensions do not name
 * their language — `CMakeLists.txt` is CMake, not text. The comment-syntax key
 * reads this table first, by exact output file name; a hit replaces the
 * extension as the `commentSyntax` key. Every other output falls through to
 * its extension.
 */
inline const jam::HashMap<juce::String, juce::String> manifestSyntax
{
    { "CMakeLists.txt", ".cmake" },
    { "bunfig.toml",    ".toml" },
    { "Cargo.toml",     ".toml" },
    { "go.mod",         ".mod" },
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace map
