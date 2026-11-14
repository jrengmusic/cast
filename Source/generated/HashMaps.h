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

extern const jam::HashMap<juce::String, juce::String> banner;

//==============================================================================

/**
 * @brief C-family comment syntax — the delimiters for a clang-style target.
 *
 * Carries every frame the `:::[description]:::` marker renders into for a C-like
 * file: the single-line trailing marker (`Id::comment`, `///<`), the block
 * brief marker (`Id::brief`, `@brief`), the block delimiters (`Id::blockOpen`
 * / `Id::blockClose`), and the banner frame (`Id::bannerOpen` /
 * `Id::bannerClose`). Transforms.h and Writer.h read these keys to wrap
 * authored documentation in the output language's own syntax.
 */
struct ClangComment : public jam::HashMap<juce::Identifier, juce::String>
{
    ClangComment() : jam::HashMap<juce::Identifier, juce::String> {
    { Id::comment,     "///<" },
    { Id::brief,       "@brief" },
    { Id::blockOpen,   "/**" },
    { Id::blockLine,   " *" },
    { Id::blockClose,  "*/" },
    { Id::bannerOpen,  "/*******************************************************************************" },
    { Id::bannerClose, "********************************************************************************/" },
    } {}
};

//==============================================================================

/**
 * @brief CMake comment syntax.
 *
 * Carries the single-line marker (`Id::comment`, `#`) and the bracket-comment
 * block frame (`#[[` / `]]`), which also serves as the banner frame for
 * CMakeLists.txt and `.cmake` outputs.
 */
struct CmakeComment : public jam::HashMap<juce::Identifier, juce::String>
{
    CmakeComment() : jam::HashMap<juce::Identifier, juce::String> {
    { Id::comment,     "#" },
    { Id::blockOpen,   "#[[" },
    { Id::blockClose,  "]]" },
    { Id::bannerOpen,  "#[[*****************************************************************************" },
    { Id::bannerClose, "******************************************************************************]]" },
    } {}
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
struct CssComment : public jam::HashMap<juce::Identifier, juce::String>
{
    CssComment() : jam::HashMap<juce::Identifier, juce::String> {
    { Id::blockOpen,   "/*" },
    { Id::blockClose,  "*/" },
    { Id::bannerOpen,  "/*******************************************************************************" },
    { Id::bannerClose, "********************************************************************************/" },
    } {}
};

//==============================================================================

/**
 * @brief go.mod comment syntax.
 *
 * Carries only the single-line marker (`Id::comment`, `//`); go.mod has no
 * block comment, so documentation renders inline only.
 */
struct GomodComment : public jam::HashMap<juce::Identifier, juce::String>
{
    GomodComment() : jam::HashMap<juce::Identifier, juce::String> {
    { Id::comment, "//" },
    } {}
};

//==============================================================================

/**
 * @brief HTML/XML comment syntax.
 *
 * Carries the block frame (`<!--` / `-->`), which doubles as the banner frame
 * for HTML and XML outputs; neither language has a distinct single-line or
 * banner marker, so both keys map to the same delimiters.
 */
struct HtmlComment : public jam::HashMap<juce::Identifier, juce::String>
{
    HtmlComment() : jam::HashMap<juce::Identifier, juce::String> {
    { Id::blockOpen,   "<!--" },
    { Id::blockClose,  "-->" },
    { Id::bannerOpen,  "<!--" },
    { Id::bannerClose, "-->" },
    } {}
};

//==============================================================================

/**
 * @brief Lua comment syntax.
 *
 * Carries the single-line marker (`Id::comment`, `---`) and the long-bracket
 * block frame (`--[[` / `]]`), which also serves as the banner frame for
 * `.lua` outputs.
 */
struct LuaComment : public jam::HashMap<juce::Identifier, juce::String>
{
    LuaComment() : jam::HashMap<juce::Identifier, juce::String> {
    { Id::comment,     "---" },
    { Id::blockOpen,   "--[[" },
    { Id::blockClose,  "]]" },
    { Id::bannerOpen,  "--[[" },
    { Id::bannerClose, "]]" },
    } {}
};

//==============================================================================

/**
 * @brief Mermaid comment syntax.
 *
 * Carries only the single-line marker (`Id::comment`, `%%`); Mermaid has no
 * block comment, so documentation renders inline only.
 */
struct MermaidComment : public jam::HashMap<juce::Identifier, juce::String>
{
    MermaidComment() : jam::HashMap<juce::Identifier, juce::String> {
    { Id::comment, "%%" },
    } {}
};

//==============================================================================

/**
 * @brief Python comment syntax.
 *
 * Carries the single-line marker (`Id::comment`, `#`) and the docstring block
 * frame (`"""` / `"""`), which also serves as the banner frame for `.py`
 * outputs.
 */
struct PythonComment : public jam::HashMap<juce::Identifier, juce::String>
{
    PythonComment() : jam::HashMap<juce::Identifier, juce::String> {
    { Id::comment,     "#" },
    { Id::blockOpen,   "\"\"\"" },
    { Id::blockClose,  "\"\"\"" },
    { Id::bannerOpen,  "\"\"\"" },
    { Id::bannerClose, "\"\"\"" },
    } {}
};

//==============================================================================

/**
 * @brief Ruby comment syntax.
 *
 * Carries the single-line marker (`Id::comment`, `#`) and the `=begin` /
 * `=end` block frame, which also serves as the banner frame for `.rb`
 * outputs.
 */
struct RubyComment : public jam::HashMap<juce::Identifier, juce::String>
{
    RubyComment() : jam::HashMap<juce::Identifier, juce::String> {
    { Id::comment,     "#" },
    { Id::blockOpen,   "=begin" },
    { Id::blockClose,  "=end" },
    { Id::bannerOpen,  "=begin" },
    { Id::bannerClose, "=end" },
    } {}
};

//==============================================================================

/**
 * @brief Shell comment syntax.
 *
 * Carries the single-line marker (`Id::comment`, `#`) alone; shell has no
 * block comment, so Transforms::toCommentBlock() renders a multi-line value
 * one `#`-marked line at a time, with no opening and no closing frame line.
 */
struct ShellComment : public jam::HashMap<juce::Identifier, juce::String>
{
    ShellComment() : jam::HashMap<juce::Identifier, juce::String> {
    { Id::comment, "#" },
    } {}
};

//==============================================================================

/**
 * @brief SQL comment syntax.
 *
 * Carries the single-line marker (`Id::comment`, `--`) and the C-style block
 * frame, which also serves as the banner frame for `.sql` outputs.
 */
struct SqlComment : public jam::HashMap<juce::Identifier, juce::String>
{
    SqlComment() : jam::HashMap<juce::Identifier, juce::String> {
    { Id::comment,     "--" },
    { Id::blockOpen,   "/*" },
    { Id::blockClose,  "*/" },
    { Id::bannerOpen,  "/*" },
    { Id::bannerClose, "*/" },
    } {}
};

//==============================================================================

/**
 * @brief File extension to comment-syntax table.
 *
 * Maps an output file's extension to the comment frame the `:::[description]:::`
 * marker renders in. Transforms.h reads one row per output file to wrap
 * authored documentation, and Writer.h reads it to frame the banner. Each
 * extension keys one of the `ClangComment`, `CmakeComment`, `CssComment`,
 * `GomodComment`, `HtmlComment`, `LuaComment`, `MermaidComment`,
 * `PythonComment`, `RubyComment`, `ShellComment` or `SqlComment` maps above.
 * An extension absent from this table falls back to `ClangComment`.
 */
struct CommentSyntax : public jam::HashMap<juce::String, jam::HashMap<juce::Identifier, juce::String>>
{
    CommentSyntax() : jam::HashMap<juce::String, jam::HashMap<juce::Identifier, juce::String>> {
    { ".h",       ClangComment {} },
    { ".cpp",     ClangComment {} },
    { ".c",       ClangComment {} },
    { ".js",      ClangComment {} },
    { ".ts",      ClangComment {} },
    { ".jsx",     ClangComment {} },
    { ".tsx",     ClangComment {} },
    { ".java",    ClangComment {} },
    { ".go",      ClangComment {} },
    { ".rs",      ClangComment {} },
    { ".rust",    ClangComment {} },
    { ".cmake",   CmakeComment {} },
    { ".css",     CssComment {} },
    { ".mod",     GomodComment {} },
    { ".html",    HtmlComment {} },
    { ".xml",     HtmlComment {} },
    { ".lua",     LuaComment {} },
    { ".mmd",     MermaidComment {} },
    { ".mermaid", MermaidComment {} },
    { ".py",      PythonComment {} },
    { ".python",  PythonComment {} },
    { ".rb",      RubyComment {} },
    { ".ruby",    RubyComment {} },
    { ".sh",      ShellComment {} },
    { ".bash",    ShellComment {} },
    { ".shell",   ShellComment {} },
    { ".toml",    ShellComment {} },
    { ".sql",     SqlComment {} },
    { ".yaml",    ShellComment {} },
    { ".yml",     ShellComment {} },
    } {}
};

//==============================================================================

extern const jam::HashMap<juce::String, juce::String> manifestSyntax;

/**______________________________END OF NAMESPACE______________________________*/
}// namespace map
