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

#include <JuceHeader.h>
#include "Generated.h"

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

    const juce::Identifier argument         { juce::String::fromUTF8 ("argument") };
    const juce::Identifier assumeFilename   { juce::String::fromUTF8 ("assume-filename") };
    const juce::Identifier background       { juce::String::fromUTF8 ("background") };
    const juce::String     backgroundPrefix { juce::String::fromUTF8 (".background") };
    const juce::Identifier banner           { juce::String::fromUTF8 ("banner") };
    const juce::Identifier bannerClose      { juce::String::fromUTF8 ("bannerClose") };
    const juce::Identifier bannerOpen       { juce::String::fromUTF8 ("bannerOpen") };
    const juce::Identifier begin            { juce::String::fromUTF8 ("begin") };
    const juce::Identifier blockClose       { juce::String::fromUTF8 ("blockClose") };
    const juce::Identifier blockLine        { juce::String::fromUTF8 ("blockLine") };
    const juce::Identifier blockOpen        { juce::String::fromUTF8 ("blockOpen") };
    const juce::Identifier bundleColumn     { juce::String::fromUTF8 ("bundle-column") };
    const juce::Identifier comment          { juce::String::fromUTF8 ("comment") };
    const juce::String     dmg              { juce::String::fromUTF8 ("dmg") };
    const juce::Identifier doubleDash       { juce::String::fromUTF8 ("--") };
    const juce::String     dsStore          { juce::String::fromUTF8 (".DS_Store") };
    const juce::Identifier firstRow         { juce::String::fromUTF8 ("first-row") };
    const juce::String     hdiutil          { juce::String::fromUTF8 ("hdiutil") };
    const juce::Identifier help             { juce::String::fromUTF8 ("help") };
    const juce::Identifier iconSize         { juce::String::fromUTF8 ("icon-size") };
    const juce::Identifier link             { juce::String::fromUTF8 ("link") };
    const juce::Identifier linkColumn       { juce::String::fromUTF8 ("link-column") };
    const juce::Identifier p                { juce::String::fromUTF8 ("p") };
    const juce::Identifier pack             { juce::String::fromUTF8 ("pack") };
    const juce::Identifier rowSpacing       { juce::String::fromUTF8 ("row-spacing") };
    const juce::Identifier syncBoundary     { juce::String::fromUTF8 ("boundary") };
    const juce::Identifier brief            { juce::String::fromUTF8 ("brief") };
    const juce::Identifier command          { juce::String::fromUTF8 ("command") };
    const juce::Identifier filePrefix       { juce::String::fromUTF8 ("filePrefix") };
    const juce::Identifier flag             { juce::String::fromUTF8 ("flag") };
    const juce::String     fromCodepoint    { juce::String::fromUTF8 ("from codepoint") };
    const juce::String     fromUTF8         { juce::String::fromUTF8 ("from UTF8") };
    const juce::Identifier identity         { juce::String::fromUTF8 ("identity") };
    const juce::Identifier ignore           { juce::String::fromUTF8 ("ignore") };
    const juce::Identifier inPlace          { juce::String::fromUTF8 ("i") };
    const juce::String     join             { juce::String::fromUTF8 ("join") };
    const juce::Identifier kernel           { juce::String::fromUTF8 ("kernel") };
    const juce::Identifier lineWrap         { juce::String::fromUTF8 ("line-wrap") };
    const juce::Identifier list             { juce::String::fromUTF8 ("list") };
    const juce::Identifier macroPrefix      { juce::String::fromUTF8 ("macroPrefix") };
    const juce::Identifier noBanner         { juce::String::fromUTF8 ("no-banner") };
    const juce::Identifier noFormat         { juce::String::fromUTF8 ("no-format") };
    const juce::Identifier placeholder      { juce::String::fromUTF8 ("placeholder") };
    const juce::Identifier separator        { juce::String::fromUTF8 ("separator") };
    const juce::Identifier structure        { juce::String::fromUTF8 ("structure") };
    const juce::Identifier symbol           { juce::String::fromUTF8 ("symbol") };
    const juce::Identifier sync             { juce::String::fromUTF8 ("sync") };
    const juce::Identifier templatePath     { juce::String::fromUTF8 ("template") };
    const juce::String     toCamel          { juce::String::fromUTF8 ("to camel") };
    const juce::String     toCodepoint      { juce::String::fromUTF8 ("to codepoint") };
    const juce::Identifier toComment        { juce::String::fromUTF8 ("toComment") };
    const juce::Identifier toCommentBlock   { juce::String::fromUTF8 ("toCommentBlock") };
    const juce::String     toFileName       { juce::String::fromUTF8 ("to file name") };
    const juce::String     toHex            { juce::String::fromUTF8 ("to hex") };
    const juce::String     toKebab          { juce::String::fromUTF8 ("to kebab") };
    const juce::Identifier tokenModule      { juce::String::fromUTF8 ("module") };
    const juce::String     toLiteral        { juce::String::fromUTF8 ("to literal") };
    const juce::Identifier toolchain        { juce::String::fromUTF8 ("toolchain") };
    const juce::String     toPascal         { juce::String::fromUTF8 ("to pascal") };
    const juce::String     toScreamingSnake { juce::String::fromUTF8 ("to screaming snake") };
    const juce::String     toSnake          { juce::String::fromUTF8 ("to snake") };
    const juce::String     toTitle          { juce::String::fromUTF8 ("to title") };
    const juce::String     toUpper          { juce::String::fromUTF8 ("to upper") };
    const juce::String     toUTF8           { juce::String::fromUTF8 ("to UTF8") };
    const juce::Identifier windowLeft       { juce::String::fromUTF8 ("window-left") };
    const juce::Identifier windowTop        { juce::String::fromUTF8 ("window-top") };
    const juce::Identifier wiring           { juce::String::fromUTF8 ("wiring") };
    const juce::Identifier word             { juce::String::fromUTF8 ("word") };
    const juce::String     zip              { juce::String::fromUTF8 ("zip") };

/**______________________________END OF NAMESPACE______________________________*/
}// namespace Id
namespace files
{
/*_____________________________________________________________________________*/

/** @brief File and directory names CAST reads and writes: manifest, help, banner, style files, stdin name, sync info. */

const juce::String cast                { juce::String::fromUTF8 ("spell.md") };
const juce::String castDirectory       { juce::String::fromUTF8 ("cast") };
const juce::String castFormat          { juce::String::fromUTF8 (".cast-format") };
const juce::String castFormatAlternate { juce::String::fromUTF8 ("_cast-format") };
const juce::String castHelp            { juce::String::fromUTF8 ("HELP.md") };
const juce::String castOutput          { juce::String::fromUTF8 ("cast-output.md") };
const juce::String standardInput       { juce::String::fromUTF8 ("<stdin>") };
const juce::String userModulesInfo     { juce::String::fromUTF8 ("user-modules-info.md") };

/**______________________________END OF NAMESPACE______________________________*/
}// namespace files
namespace map
{
/*_____________________________________________________________________________*/

const jam::HashMap<juce::String, juce::String> banner
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
const jam::HashMap<juce::Identifier, juce::String> clangComment
{
    { Id::comment,     "///<" },
    { Id::brief,       "@brief" },
    { Id::blockOpen,   "/**" },
    { Id::blockLine,   " *" },
    { Id::blockClose,  "*/" },
    { Id::bannerOpen,  "/*******************************************************************************" },
    { Id::bannerClose, "********************************************************************************/" },
};
const jam::HashMap<juce::Identifier, juce::String> cmakeComment
{
    { Id::comment,     "#" },
    { Id::blockOpen,   "#[[" },
    { Id::blockClose,  "]]" },
    { Id::bannerOpen,  "#[[*****************************************************************************" },
    { Id::bannerClose, "******************************************************************************]]" },
};
const jam::HashMap<juce::Identifier, juce::String> cssComment
{
    { Id::blockOpen,   "/*" },
    { Id::blockClose,  "*/" },
    { Id::bannerOpen,  "/*******************************************************************************" },
    { Id::bannerClose, "********************************************************************************/" },
};
const jam::HashMap<juce::Identifier, juce::String> gomodComment
{
    { Id::comment, "//" },
};
const jam::HashMap<juce::Identifier, juce::String> htmlComment
{
    { Id::blockOpen,   "<!--" },
    { Id::blockClose,  "-->" },
    { Id::bannerOpen,  "<!--" },
    { Id::bannerClose, "-->" },
};
const jam::HashMap<juce::Identifier, juce::String> luaComment
{
    { Id::comment,     "---" },
    { Id::blockOpen,   "--[[" },
    { Id::blockClose,  "]]" },
    { Id::bannerOpen,  "--[[" },
    { Id::bannerClose, "]]" },
};
const jam::HashMap<juce::Identifier, juce::String> mermaidComment
{
    { Id::comment, "%%" },
};
const jam::HashMap<juce::Identifier, juce::String> pythonComment
{
    { Id::comment,     "#" },
    { Id::blockOpen,   "\"\"\"" },
    { Id::blockClose,  "\"\"\"" },
    { Id::bannerOpen,  "\"\"\"" },
    { Id::bannerClose, "\"\"\"" },
};
const jam::HashMap<juce::Identifier, juce::String> rubyComment
{
    { Id::comment,     "#" },
    { Id::blockOpen,   "=begin" },
    { Id::blockClose,  "=end" },
    { Id::bannerOpen,  "=begin" },
    { Id::bannerClose, "=end" },
};
const jam::HashMap<juce::Identifier, juce::String> shellComment
{
    { Id::comment, "#" },
};
const jam::HashMap<juce::Identifier, juce::String> sqlComment
{
    { Id::comment,     "--" },
    { Id::blockOpen,   "/*" },
    { Id::blockClose,  "*/" },
    { Id::bannerOpen,  "/*" },
    { Id::bannerClose, "*/" },
};
const jam::HashMap<juce::String, jam::HashMap<juce::Identifier, juce::String>> commentSyntax
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
const jam::HashMap<juce::String, juce::String> manifestSyntax
{
    { "CMakeLists.txt", ".cmake" },
    { "bunfig.toml",    ".toml" },
    { "Cargo.toml",     ".toml" },
    { "go.mod",         ".mod" },
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace map
