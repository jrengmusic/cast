/**
 * @file main.cpp
 * @brief `cast` CLI entry point: banner/help rendering and dispatch of
 *        the manifest, format, tables, sync, and pack requests.
 */

#include <JuceHeader.h>
#include "Processor.h"
#include "Sync.h"
#include "Pack.h"
#include "Help.h"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

/// @brief Prints the generated banner rows to stdout.
static void printBanner()
{
    for (const auto& [name, text] : map::banner)
        printf ("%s\n", text.toRawUTF8());
}

/**
 * @brief Prints @p message to stderr, prefixed with the project name and
 *        the diagnostic separator.
 *
 * @param message The diagnostic text to print.
 */
static void printError (const juce::String& message)
{
    fprintf (stderr, "%s\n", (ProjectInfo::projectName + Id::diagnosticSeparator + message).toRawUTF8());
}

static constexpr int flagArgIndex { 1 };///< argv's own index for a flag authored as the first argument.
static constexpr int postFlagArgIndex { 2 };///< argv's own index for a flag authored after the manifest argument.
static constexpr int flagOnlyArgumentCount { 2 };///< argc's own value when argv carries a flag and nothing else.
static constexpr int postFlagArgumentCount { 3 };///< argc's own value when argv carries a manifest and a following flag.
static constexpr int invalidFlagValue { -1 };///< The value getFlagValue() returns when the flag's own text does not round-trip as a positive integer.
static constexpr int absentFlagValue { 0 };///< The value getFlagValue() returns when the flag is absent from argv.

/**
 * @brief The `--format` flag word.
 * @returns The `--format` flag text.
 */
static const juce::String& getFormatFlag()
{
    static const juce::String formatFlag { Id::doubleDash + Id::format.toString() };
    return formatFlag;
}

/**
 * @brief The `--tables` flag word.
 * @returns The `--tables` flag text.
 */
static const juce::String& getTablesFlag()
{
    static const juce::String tablesFlag { Id::doubleDash + Id::tables.toString() };
    return tablesFlag;
}

/**
 * @brief The `--no-format` flag word.
 * @returns The `--no-format` flag text.
 */
static const juce::String& getNoFormatFlag()
{
    static const juce::String noFormatFlag { Id::doubleDash + Id::noFormat.toString() };
    return noFormatFlag;
}

/**
 * @brief The `--version` flag word.
 * @returns The `--version` flag text.
 */
static const juce::String& getVersionFlag()
{
    static const juce::String versionFlag { Id::doubleDash + Id::version.toString() };
    return versionFlag;
}

/**
 * @brief The `--help` flag word.
 * @returns The `--help` flag text.
 */
static const juce::String& getHelpFlag()
{
    static const juce::String helpFlag { Id::doubleDash + Id::help.toString() };
    return helpFlag;
}

/**
 * @brief The `--sync` flag word.
 * @returns The `--sync` flag text.
 */
static const juce::String& getSyncFlag()
{
    static const juce::String syncFlag { Id::doubleDash + Id::sync.toString() };
    return syncFlag;
}

/**
 * @brief The `--pack` flag word.
 * @returns The `--pack` flag text.
 */
static const juce::String& getPackFlag()
{
    static const juce::String packFlag { Id::doubleDash + Id::pack.toString() };
    return packFlag;
}

/**
 * @brief The `--link` flag word, without its `=\<name\>=\<target\>` value.
 * @returns The `--link` flag text.
 */
static const juce::String& getLinkFlag()
{
    static const juce::String linkFlag { Id::doubleDash + Id::link.toString() };
    return linkFlag;
}

/**
 * @brief The `--line-wrap` flag word.
 * @returns The `--line-wrap` flag text.
 */
static const juce::String& getLineWrapFlag()
{
    static const juce::String lineWrapFlag { Id::doubleDash + Id::lineWrap.toString() };
    return lineWrapFlag;
}

/**
 * @brief The `-i` flag word.
 * @returns The `-i` flag text.
 */
static const juce::String& getInPlaceFlag()
{
    static const juce::String inPlaceFlag { juce::String::charToString (Chars::dash) + Id::inPlace.toString() };
    return inPlaceFlag;
}

/**
 * @brief The `--style` flag word, without its `=file[:\<path\>]` value.
 * @returns The `--style` flag text.
 */
static const juce::String& getStyleFlag()
{
    static const juce::String styleFlag { Id::doubleDash + Id::style.toString() };
    return styleFlag;
}

/**
 * @brief The `file:` prefix a `--style=file:\<path\>` value carries before
 *        its path.
 * @returns The `file:` prefix text.
 */
static const juce::String& getStyleFilePrefix()
{
    static const juce::String styleFilePrefix { Id::file.toString() + juce::String::charToString (Chars::colon) };
    return styleFilePrefix;
}

/**
 * @brief The `--assume-filename` flag word.
 * @returns The `--assume-filename` flag text.
 */
static const juce::String& getAssumeFilenameFlag()
{
    static const juce::String assumeFilenameFlag { Id::doubleDash + Id::assumeFilename.toString() };
    return assumeFilenameFlag;
}

/**
 * @brief The stdin argument: the input that getInputFiles() adds when the
 *        line names no file argument. A `-` that the caller types is an
 *        unknown flag.
 * @returns The stdin argument text.
 */
static const juce::String& getStandardInputArgument()
{
    static const juce::String standardInputArgument { juce::String::charToString (Chars::dash) };
    return standardInputArgument;
}

/**
 * @brief The integer that follows @p flag in @p argv.
 *
 * @param argc          The argument count.
 * @param argv          The argument vector.
 * @param flag          The flag whose following value is read.
 * @param fallbackValue The value returned when @p flag is absent from @p argv.
 * @returns The integer following @p flag, @p fallbackValue when @p flag is
 *          absent, or invalidFlagValue when the following text does not
 *          round-trip as a positive integer.
 */
static int getFlagValue (int argc, char* argv[], const juce::String& flag, int fallbackValue)
{
    for (int index { 0 }; index < argc - 1; ++index)
        if (juce::String::fromUTF8 (argv[index]).compare (flag) == 0)
        {
            const auto text { juce::String::fromUTF8 (argv[index + 1]) };
            const auto value { text.getIntValue() };

            return juce::String (value).compare (text) == 0 and value > 0 ? value : invalidFlagValue;
        }

    return fallbackValue;
}

/**
 * @brief Returns @p argv's own real index at filtered @p position, a
 *        `--line-wrap value` pair skipped as a single position.
 *
 * @param argc     The argument count.
 * @param argv     The argument vector.
 * @param position The 0-based position among @p argv's own arguments,
 *                  counting a `--line-wrap value` pair once.
 * @returns @p argv's own real index at @p position, or @p argc when
 *          @p argv carries fewer than @p position + 1 filtered arguments.
 */
static int getArgumentIndex (int argc, char* argv[], int position)
{
    auto filteredPosition { 0 };
    auto index { 0 };

    while (index < argc)
    {
        const auto isLineWrapPair { juce::String::fromUTF8 (argv[index]).compare (getLineWrapFlag()) == 0
                                    and index + 1 < argc };

        if (isLineWrapPair)
        {
            index += 2;
        }
        else
        {
            if (filteredPosition == position) return index;

            ++filteredPosition;
            ++index;
        }
    }

    return argc;
}

/**
 * @brief Returns @p argv's own flag-position argument -- the first CLI
 *        argument after the executable name.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @p argv's own flag-position argument, or an empty string when
 *          @p argc declares none.
 */
static juce::String getFlagArgument (int argc, char* argv[])
{
    const auto index { getArgumentIndex (argc, argv, flagArgIndex) };
    return index < argc ? juce::String::fromUTF8 (argv[index]) : juce::String {};
}

/**
 * @brief Returns @p argv's own post-flag-position argument -- the CLI
 *        argument following the flag position.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @p argv's own post-flag-position argument, or an empty string
 *          when @p argc declares none.
 */
static juce::String getPostFlagArgument (int argc, char* argv[])
{
    const auto index { getArgumentIndex (argc, argv, postFlagArgIndex) };
    return index < argc ? juce::String::fromUTF8 (argv[index]) : juce::String {};
}

/**
 * @brief Answers whether @p argv declares the @c --format flag, at any
 *        position on the line.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c true when @p argv declares @c --format.
 */
static bool isFormatOnly (int argc, char* argv[])
{
    return juce::ArgumentList { argc, argv }.containsOption (getFormatFlag());
}

/**
 * @brief Answers whether @p argv declares the @c --tables flag, at any
 *        position on the line.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c true when @p argv declares @c --tables.
 */
static bool isTablesFlag (int argc, char* argv[])
{
    return juce::ArgumentList { argc, argv }.containsOption (getTablesFlag());
}

/**
 * @brief Answers whether @p argv declares the @c --no-format flag at
 *        either the flag or post-flag position.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c true when @p argv declares @c --no-format.
 */
static bool isSkipFormat (int argc, char* argv[])
{
    return getFlagArgument (argc, argv).compare (getNoFormatFlag()) == 0
           or getPostFlagArgument (argc, argv).compare (getNoFormatFlag()) == 0;
}

/**
 * @brief Returns the argv index the manifest argument sits at -- the
 *        post-flag position when the flag position declares @c --format
 *        or @c --no-format, the flag position otherwise.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns The manifest argument's own argv index.
 */
static int getManifestIndex (int argc, char* argv[])
{
    const auto flagArgument { getFlagArgument (argc, argv) };

    return (flagArgument.compare (getFormatFlag()) == 0 or flagArgument.compare (getNoFormatFlag()) == 0)
               ? postFlagArgIndex : flagArgIndex;
}

/**
 * @brief Returns @p argv's own manifest-slot argument -- the CLI argument
 *        naming a manifest path, a toolchain flag, or the version/help
 *        flags -- present only when @p argv carries exactly the post-flag
 *        argument count and getManifestIndex() places it at the flag
 *        position and @p argv does not declare @c --no-format there.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @p argv's own manifest-slot argument, or an empty string when
 *          @p argv declares none.
 */
static juce::String getManifestArgument (int argc, char* argv[])
{
    const auto isExactManifestSlot { getArgumentIndex (argc, argv, postFlagArgIndex) < argc
                                     and getArgumentIndex (argc, argv, postFlagArgumentCount) == argc
                                     and getManifestIndex (argc, argv) == flagArgIndex
                                     and not isSkipFormat (argc, argv) };

    return isExactManifestSlot ? getPostFlagArgument (argc, argv) : juce::String {};
}

/**
 * @brief Answers whether @p manifestArgument is a toolchain selector --
 *        double-dash-prefixed and none of the reserved flags.
 *
 * @param manifestArgument The manifest-slot argument to test.
 * @returns @c true when @p manifestArgument is a toolchain selector.
 */
static bool isToolchainArgument (const juce::String& manifestArgument)
{
    static const jam::Strings reservedFlags { getFormatFlag(), getNoFormatFlag(), getVersionFlag(), getHelpFlag(), getSyncFlag(), getPackFlag() };

    return manifestArgument.startsWith (Id::doubleDash.toString())
           and not reservedFlags.contains (manifestArgument, false);
}

/**
 * @brief Returns @p argv's own toolchain argument -- its manifest-slot
 *        argument's own text after the double dash, when that argument is
 *        a toolchain selector.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @p argv's own toolchain argument, or an empty string when its
 *          manifest-slot argument is not a toolchain selector.
 */
static juce::String getToolchainArgument (int argc, char* argv[])
{
    const auto manifestArgument { getManifestArgument (argc, argv) };

    return isToolchainArgument (manifestArgument)
               ? manifestArgument.substring (Id::doubleDash.toString().length())
               : juce::String {};
}

/**
 * @brief Returns @p argv's own output directory argument -- its
 *        manifest-slot argument, when that argument is not a toolchain
 *        selector.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @p argv's own output directory argument, or an empty string
 *          when its manifest-slot argument is a toolchain selector.
 */
static juce::String getOutputDirectory (int argc, char* argv[])
{
    const auto manifestArgument { getManifestArgument (argc, argv) };

    return isToolchainArgument (manifestArgument) ? juce::String {} : manifestArgument;
}

/**
 * @brief Answers whether @p argv declares a manifest argument at the
 *        position getManifestIndex() names.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c true when the manifest argument exists.
 */
static bool isManifestDeclared (int argc, char* argv[])
{
    return getArgumentIndex (argc, argv, getManifestIndex (argc, argv)) < argc;
}

/**
 * @brief Resolves the manifest argument of @p argv at getManifestIndex()
 *        against the current working directory, then follows a symbolic
 *        link to its target with juce::File::getLinkedTarget().
 *
 * A path that is not a link comes back unchanged on macOS and Linux.
 *
 * @pre isManifestDeclared() is @c true.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns The resolved manifest file.
 */
static juce::File getDocumentFile (int argc, char* argv[])
{
    const auto manifestIndex { getArgumentIndex (argc, argv, getManifestIndex (argc, argv)) };

    jassert (manifestIndex < argc);

    return juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[manifestIndex])).getLinkedTarget();
}

/**
 * @brief Answers whether @p argv requests the @c --version flag, either
 *        alone or at the manifest slot.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c true when @p argv requests @c --version.
 */
static bool isVersion (int argc, char* argv[])
{
    const auto isFlagOnly { getArgumentIndex (argc, argv, flagArgIndex) < argc
                            and getArgumentIndex (argc, argv, flagOnlyArgumentCount) == argc };

    return (getFlagArgument (argc, argv).compare (getVersionFlag()) == 0 and isFlagOnly)
           or getManifestArgument (argc, argv).compare (getVersionFlag()) == 0;
}

/**
 * @brief Answers whether @p argv requests the @c --help flag, either
 *        alone or at the manifest slot.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c true when @p argv requests @c --help.
 */
static bool isHelp (int argc, char* argv[])
{
    const auto isFlagOnly { getArgumentIndex (argc, argv, flagArgIndex) < argc
                            and getArgumentIndex (argc, argv, flagOnlyArgumentCount) == argc };

    return (getFlagArgument (argc, argv).compare (getHelpFlag()) == 0 and isFlagOnly)
           or getManifestArgument (argc, argv).compare (getHelpFlag()) == 0;
}

/**
 * @brief Prints the project name, version string, and commit hash to
 *        stdout.
 */
static void writeVersion()
{
    const auto versionLine { ProjectInfo::projectName
                             + juce::String::charToString (Chars::space)
                             + ProjectInfo::versionString
                             + Chars::space
                             + Chars::openParen + CAST_COMMIT
                             + Chars::closeParen };
    printf ("%s\n", versionLine.toRawUTF8());
}

/**
 * @brief Whether stdout is a terminal -- the success line prints only then.
 * @returns @c true when stdout is a TTY.
 */
static bool isTerminalOutput() noexcept
{
#ifdef _WIN32
    return _isatty (_fileno (stdout)) != 0;
#else
    return isatty (fileno (stdout)) != 0;
#endif
}

/**
 * @brief Prints `cast: done` to stdout when stdout is a terminal.
 */
static void printDone()
{
    if (isTerminalOutput())
    {
        const auto doneLine { ProjectInfo::projectName + Id::diagnosticSeparator + text::Diagnostics::done };
        printf ("%s\n", doneLine.toRawUTF8());
    }
}

/**
 * @brief Returns the first of @c .cast-format or @c _cast-format that
 *        exists directly under @p directory.
 *
 * @param directory The directory searched for a style file.
 * @returns The matching style file, or an empty juce::File when neither
 *          name exists under @p directory.
 */
static juce::File getStyleFile (const juce::File& directory)
{
    static const jam::Strings styleNames { files::castFormat, files::castFormatAlternate };

    for (const auto& name : styleNames)
        if (const auto file { directory.getChildFile (name) }; file.existsAsFile())
            return file;

    return juce::File {};
}

/**
 * @brief Walks @p directory and each parent directory, in order, for the
 *        nearest style file getStyleFile() finds.
 *
 * @param directory The directory the walk starts from.
 * @returns The nearest style file, or an empty juce::File when no
 *          directory up to and including the file-system root declares
 *          one.
 */
static juce::File getNearestStyleFile (const juce::File& directory)
{
    for (auto current { directory }; ; current = current.getParentDirectory())
    {
        if (const auto styleFile { getStyleFile (current) }; styleFile.existsAsFile())
            return styleFile;

        if (current.isRoot())
            return juce::File {};
    }
}

/**
 * @brief Returns @p searchedStyleFile's own style file, or @p stylePath's
 *        own file under the current working directory when @p stylePath
 *        is not empty.
 *
 * @param stylePath          An explicit @c --style=file:\<path\> path, or
 *                            an empty string.
 * @param searchedStyleFile  The style file a directory search already
 *                            resolved.
 * @returns @p stylePath's own file when @p stylePath is not empty,
 *          @p searchedStyleFile otherwise.
 */
static juce::File getStyleFile (const juce::String& stylePath, const juce::File& searchedStyleFile)
{
    return stylePath.isNotEmpty() ? juce::File::getCurrentWorkingDirectory().getChildFile (stylePath) : searchedStyleFile;
}

/**
 * @brief Returns @p arguments' own explicit @c --style=file:\<path\> path.
 *
 * @param arguments The parsed command-line arguments.
 * @returns The path segment after @c file: in the @c --style option's own
 *          value, or an empty string for @c --style=file or an absent
 *          flag.
 */
static juce::String getStylePath (const juce::ArgumentList& arguments)
{
    return arguments.getValueForOption (getStyleFlag()).fromFirstOccurrenceOf (getStyleFilePrefix(), false, false);
}

/**
 * @brief Returns @p arguments' own @c --assume-filename value.
 *
 * @param arguments The parsed command-line arguments.
 * @returns The @c --assume-filename value, or an empty string when
 *          @p arguments declares none.
 */
static juce::String getAssumeFilename (const juce::ArgumentList& arguments)
{
    return arguments.getValueForOption (getAssumeFilenameFlag());
}

/**
 * @brief Answers whether @p arguments declares the @c -i flag.
 *
 * @param arguments The parsed command-line arguments.
 * @returns @c true when @p arguments declares @c -i.
 */
static bool isInPlace (const juce::ArgumentList& arguments)
{
    return arguments.containsOption (getInPlaceFlag());
}

/**
 * @brief Returns the first option in @p arguments that is not in @p flags,
 *        and is not a @c --style=file[:\<path\>] value or an
 *        @c --assume-filename=\<path\> value.
 *
 * @param arguments The parsed command-line arguments.
 * @param flags     The option words the calling grammar accepts. runFormat()
 *                  passes its @c --format grammar, and runTables() its
 *                  @c --tables grammar.
 * @returns The first unknown option's own text, or an empty string when
 *          every option is accepted.
 */
static juce::String getUnknownFlag (const juce::ArgumentList& arguments, const jam::Strings& flags)
{
    static const juce::String styleFileFlag { getStyleFlag() + juce::String::charToString (Chars::equals) + Id::file.toString() };

    for (const auto& argument : arguments.arguments)
    {
        const auto isStyleArgument { argument.text.startsWith (styleFileFlag)
                                     and (argument.text.compare (styleFileFlag) == 0
                                          or argument.text.startsWith (styleFileFlag + juce::String::charToString (Chars::colon))) };

        if (argument.isOption() and not flags.contains (argument.text, false) and not isStyleArgument
            and not argument.text.startsWith (getAssumeFilenameFlag() + juce::String::charToString (Chars::equals)))
            return argument.text;
    }

    return {};
}

/**
 * @brief Answers whether @p input is the stdin argument.
 *
 * @param input One @c --format or @c --tables input argument.
 * @returns @c true when @p input is the stdin argument.
 */
static bool isStandardInput (const juce::String& input)
{
    return input.compare (getStandardInputArgument()) == 0;
}

/**
 * @brief Returns the input arguments of @p arguments, in authored order:
 *        every argument that is not an option, skipping the value that
 *        follows the last @c --line-wrap.
 *
 * When no input argument is authored, the stdin argument is the only input.
 *
 * @param arguments The parsed command-line arguments.
 * @returns @p arguments' own input arguments, in authored order, or the
 *          stdin argument alone when none is authored.
 */
static jam::Strings getInputFiles (const juce::ArgumentList& arguments)
{
    jam::Strings files;
    auto lineWrapIndex { invalidFlagValue };

    for (int index { 0 }; index < arguments.size(); ++index)
        if (arguments[index].text.compare (getLineWrapFlag()) == 0)
            lineWrapIndex = index;

    for (int index { 0 }; index < arguments.size(); ++index)
    {
        const auto isLineWrapValue { lineWrapIndex >= 0 and index == lineWrapIndex + 1 };

        if (not isLineWrapValue)
        {
            const auto argument { arguments[index] };

            if (not argument.isOption())
                files.add (argument.text);
        }
    }

    if (files.size() == 0)
        files.add (getStandardInputArgument());

    return files;
}

/**
 * @brief Reads stdin to end of stream, in binary mode on Windows.
 *
 * @returns juce::Result::ok() paired with stdin's own decoded text, or a
 *          failure naming the read error paired with an empty string.
 */
static std::pair<juce::Result, juce::String> getStandardInputText()
{
#ifdef _WIN32
    _setmode (_fileno (stdin), _O_BINARY);
#endif

    juce::MemoryOutputStream output;
    std::array<char, BUFSIZ> buffer {};
    auto bytesRead { fread (buffer.data(), 1, buffer.size(), stdin) };

    while (bytesRead > 0)
    {
        output.write (buffer.data(), bytesRead);
        bytesRead = fread (buffer.data(), 1, buffer.size(), stdin);
    }

    if (ferror (stdin) != 0)
        return { juce::Result::fail (text::Diagnostics::failStandardInput), {} };

    return { juce::Result::ok(), output.toString() };
}

/**
 * @brief Returns @p input's own text: the stdin text for the stdin argument,
 *        or the text of @p input's own file, loaded from the current working
 *        directory.
 *
 * @param input             One input argument: a file path or the stdin argument.
 * @param standardInputText The stdin read result, read once by
 *                          getProcessedTexts() before dispatch.
 * @returns For the stdin argument, @p standardInputText. Otherwise
 *          juce::Result::ok() paired with the text that
 *          juce::File::loadFileAsString() returns for the file.
 */
static std::pair<juce::Result, juce::String> getInputText (const juce::String& input, const std::pair<juce::Result, juce::String>& standardInputText)
{
    if (isStandardInput (input))
        return standardInputText;

    const juce::File inputFile { juce::File::getCurrentWorkingDirectory().getChildFile (input) };
    return { juce::Result::ok(), inputFile.loadFileAsString() };
}

/**
 * @brief Answers whether @p name exists as a file under the current
 *        working directory -- vacuously true for an empty name or the
 *        stdin argument.
 *
 * @param name One candidate name to check.
 * @returns juce::Result::ok() when @p name qualifies, or a failure naming
 *          @p name.
 */
static juce::Result isFound (const juce::String& name)
{
    const auto found { name.isEmpty() or isStandardInput (name)
                       or juce::File::getCurrentWorkingDirectory().getChildFile (name).existsAsFile() };

    return found ? juce::Result::ok() : juce::Result::fail (text::Diagnostics::failNotFound + Id::diagnosticSeparator + name);
}

/**
 * @brief Answers whether every one of @p names qualifies per isFound().
 *
 * @param names The candidate names to check.
 * @returns juce::Result::ok() when every name qualifies, or a failure
 *          naming the first one that does not.
 */
static juce::Result isFound (const jam::Strings& names)
{
    for (const auto& name : names)
    {
        const auto result { isFound (name) };

        if (not result.wasOk())
            return result;
    }

    return juce::Result::ok();
}

/**
 * @brief Returns the line wrap to use.
 *
 * @param lineWrap The @c --line-wrap value, read once by main().
 * @param style    The style whose own line wrap applies when @p lineWrap is not positive.
 * @returns @p lineWrap when positive, otherwise Model::getLineWrap() of @p style.
 */
static int getLineWrap (int lineWrap, const Model& style)
{
    return lineWrap > 0 ? lineWrap : style.getLineWrap();
}

/**
 * @brief Resolves @p styleFile into a style Model and, when it validates,
 *        the reflow widths it declares and a formatter built from them,
 *        then invokes @p function with both.
 *
 * @pre The JUCE GUI initialiser scope is open.
 *
 * @tparam Function A callable invoked as
 *                  @c function(formatter,reflowWidths), returning a
 *                  juce::Result.
 * @param lineWrap  The @c --line-wrap value, read once by main(); used
 *                  when positive, the style's own line wrap otherwise.
 * @param styleFile The style file to parse and validate.
 * @param function  The callable invoked with the resolved formatter and
 *                  reflow widths.
 * @returns @p function's own result, or the style's own validation
 *          failure.
 */
template <typename Function>
static juce::Result runStyled (int lineWrap, const juce::File& styleFile, Function&& function)
{
    const auto style { Model::load (styleFile) };
    const auto result { Validator::isFormat (*style) };

    if (result.wasOk())
    {
        const auto reflowWidths { style->getReflowWidths() };
        const jam::MarkdownWriter formatter { getLineWrap (lineWrap, *style), reflowWidths };

        return function (formatter, reflowWidths);
    }

    return result;
}

/**
 * @brief Prints the banner and help text to stdout.
 *
 * Opens its own JUCE GUI initialiser scope, because printHelp() renders
 * through the terminal graphics context.
 */
static void printBannerAndHelp()
{
    static const auto newlineText { juce::String::charToString (Chars::newline) };
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    printBanner();
    printf ("%s", newlineText.toRawUTF8());
    printHelp (BinaryData::getString (files::castHelp));
}

/**
 * @brief Resolves @p documentFile's own nearest style file, parses it into
 *        a Processor, runs its format() and generate() as @p skipFormat
 *        selects, and prints the resulting error to stderr on failure.
 *
 * @param lineWrap          The @c --line-wrap value, read once by main().
 * @param documentFile      The manifest file to process.
 * @param skipFormat        Whether to skip Processor::format().
 * @param outputDirectory   The directory passed through to
 *                          Processor::generate().
 * @param toolchainArgument The CLI-selected toolchain group, passed
 *                          through to Processor::generate().
 * @returns @c 0 when every run step succeeds, or @c 1 after printing the
 *          first failure's error message.
 */
static int runDocument (int lineWrap, const juce::File& documentFile, bool skipFormat,
    const juce::String& outputDirectory, const juce::String& toolchainArgument)
{
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;

    const auto result { runStyled (lineWrap, getNearestStyleFile (documentFile.getParentDirectory()),
        [&documentFile, skipFormat, &outputDirectory, &toolchainArgument]
        (const jam::MarkdownWriter& formatter, const jam::HashMap<juce::Identifier, int>& reflowWidths)
        {
            Processor processor { documentFile.loadFileAsString(), documentFile, reflowWidths };

            if (skipFormat)
                return processor.generate (outputDirectory, toolchainArgument);

            const auto formatResult { processor.format (formatter) };

            return formatResult.wasOk() ? processor.generate (outputDirectory, toolchainArgument) : formatResult;
        }) };

    if (result.wasOk())
    {
        printDone();
        return 0;
    }

    printError (result.getErrorMessage());
    return 1;
}

/**
 * @brief Prints the banner and help text, then reports @p documentFile as
 *        not found on stderr.
 *
 * @param documentFile The manifest file that was not found.
 * @returns @c 1.
 */
static int runDocumentNotFound (const juce::File& documentFile)
{
    printBannerAndHelp();
    printError (text::Diagnostics::failNotFound + Id::diagnosticSeparator + documentFile.getFileName());
    return 1;
}

static constexpr int syncRootCount { 2 };///< The number of positional root arguments a `--sync` invocation takes.

/**
 * @brief Answers whether @p argv requests @c --sync at the flag position.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c true when @p argv declares @c --sync.
 */
static bool isSyncFlag (int argc, char* argv[])
{
    return getFlagArgument (argc, argv).compare (getSyncFlag()) == 0;
}

/**
 * @brief Answers whether @p argv requests @c --pack at the flag position.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c true when @p argv declares @c --pack.
 */
static bool isPackFlag (int argc, char* argv[])
{
    return getFlagArgument (argc, argv).compare (getPackFlag()) == 0;
}

/**
 * @brief Validates every one of @p files against its own resolved style,
 *        without writing -- the first pass of an in-place run.
 *
 * Each file path resolves to the target of a symbolic link before the
 * directory is derived. The style file search and the Processor directory
 * start at the target's directory.
 *
 * @param lineWrap  The @c --line-wrap value, read once by main().
 * @param files     The files to validate.
 * @param stylePath An explicit @c --style=file:\<path\> path, or an empty
 *                  string to resolve each file's own nearest style file.
 * @returns @c 0 when every file validates successfully, or @c 1 after
 *          printing the first failure's error message.
 */
static int runFormatValidation (int lineWrap, const jam::Strings& files, const juce::String& stylePath)
{
    const auto workingDirectory { juce::File::getCurrentWorkingDirectory() };

    for (const auto& name : files)
    {
        const auto file { workingDirectory.getChildFile (name).getLinkedTarget() };
        const auto directory { file.getParentDirectory() };
        const auto styleFile { getStyleFile (stylePath, getNearestStyleFile (directory)) };

        const auto formatResult { runStyled (lineWrap, styleFile,
            [&file] (const jam::MarkdownWriter& formatter, const jam::HashMap<juce::Identifier, int>& reflowWidths)
            {
                Processor processor { file.loadFileAsString(), file, reflowWidths };
                return processor.getText (formatter).first;
            }) };

        if (not formatResult.wasOk())
        {
            printError (formatResult.getErrorMessage());
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Reformats every one of @p files in place, per file's own
 *        resolved style.
 *
 * Each file path resolves to the target of a symbolic link before the
 * directory is derived. The style file search and the Processor directory
 * start at the target's directory.
 *
 * @param lineWrap  The @c --line-wrap value, read once by main().
 * @param stylePath An explicit @c --style=file:\<path\> path, or an empty
 *                  string to resolve each file's own nearest style file.
 * @param files     The files to reformat in place.
 * @returns juce::Result::ok() when every file reformats successfully, or
 *          the first failure.
 */
static juce::Result runFormatWrite (int lineWrap, const juce::String& stylePath, const jam::Strings& files)
{
    const auto workingDirectory { juce::File::getCurrentWorkingDirectory() };

    for (const auto& name : files)
    {
        const auto file { workingDirectory.getChildFile (name).getLinkedTarget() };
        const auto directory { file.getParentDirectory() };
        const auto styleFile { getStyleFile (stylePath, getNearestStyleFile (directory)) };

        const auto formatResult { runStyled (lineWrap, styleFile,
            [&file] (const jam::MarkdownWriter& formatter, const jam::HashMap<juce::Identifier, int>& reflowWidths)
            {
                Processor processor { file.loadFileAsString(), file, reflowWidths };
                return processor.format (formatter);
            }) };

        if (not formatResult.wasOk())
            return formatResult;
    }

    return juce::Result::ok();
}

/**
 * @brief Reformats every one of @p files in place, once every file and
 *        any explicit style path exist and every file validates.
 *
 * @param lineWrap  The @c --line-wrap value, read once by main().
 * @param arguments The parsed command-line arguments, read for
 *                  @c --style.
 * @param files     The files to reformat in place.
 * @returns @c 0 when every file reformats successfully, or @c 1 after
 *          printing the first failure's error message.
 */
static int runFormatInPlace (int lineWrap, const juce::ArgumentList& arguments, const jam::Strings& files)
{
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;

    const auto stylePath { getStylePath (arguments) };
    const auto filesFound { isFound (files) };
    const auto styleFound { isFound (stylePath) };

    if (filesFound.wasOk() and styleFound.wasOk())
    {
        if (const auto validationResult { runFormatValidation (lineWrap, files, stylePath) }; validationResult != 0)
            return validationResult;

        if (const auto writeResult { runFormatWrite (lineWrap, stylePath, files) }; not writeResult.wasOk())
        {
            printError (writeResult.getErrorMessage());
            return 1;
        }

        return 0;
    }

    printError (filesFound.wasOk() ? styleFound.getErrorMessage() : filesFound.getErrorMessage());
    return 1;
}

/**
 * @brief Resolves @p input's own origin, directory, and style, parses
 *        @p input into a Processor, and returns the text that @p getText
 *        produces for it.
 *
 * A file input path resolves to the target of a symbolic link before the
 * directory is derived. The style file search and the Processor directory
 * start at the target's directory. A stdin input takes its directory and
 * origin from @p assumeFilename when it is given, and from the working
 * directory and the stdin origin name otherwise.
 *
 * @tparam Function A callable invoked as
 *                  @c getText(processor,formatter,file), returning
 *                  std::pair<juce::Result, juce::String>. @c file is the
 *                  origin for stdin, and @p input as authored otherwise.
 * @param lineWrap       The @c --line-wrap value, read once by main(); used
 *                       when positive, the style's own line wrap otherwise.
 * @param stylePath      An explicit @c --style=file:\<path\> path, or an
 *                       empty string to resolve @p input's own nearest style
 *                       file.
 * @param assumeFilename The @c --assume-filename value, read for the stdin
 *                       input only.
 * @param input          One input argument: a file path or the stdin
 *                       argument.
 * @param standardInputText The stdin read result, read once by
 *                          getProcessedTexts() before dispatch. A stdin input
 *                          uses it. A file input does not.
 * @param getText        The callable invoked once the Processor is built.
 * @returns The result of the style, the input read, or @p getText, paired
 *          with the text @p getText returned. The text is empty when the
 *          style or the input read fails.
 */
template <typename Function>
static std::pair<juce::Result, juce::String> getProcessedText (int lineWrap, const juce::String& stylePath,
    const juce::String& assumeFilename, const juce::String& input, const std::pair<juce::Result, juce::String>& standardInputText, Function&& getText)
{
    const auto workingDirectory { juce::File::getCurrentWorkingDirectory() };
    const auto assumedFile { workingDirectory.getChildFile (assumeFilename) };
    const auto inputFile { workingDirectory.getChildFile (input).getLinkedTarget() };
    const auto documentFile { isStandardInput (input) ? (assumeFilename.isNotEmpty() ? assumedFile : workingDirectory.getChildFile (files::standardInput)) : inputFile };
    juce::String outputText;

    const auto result { runStyled (lineWrap, getStyleFile (stylePath, getNearestStyleFile (documentFile.getParentDirectory())),
        [&input, &standardInputText, &documentFile, &outputText, &getText] (const jam::MarkdownWriter& formatter, const jam::HashMap<juce::Identifier, int>& reflowWidths)
        {
            const auto [textResult, text] { getInputText (input, standardInputText) };

            if (textResult.wasOk())
            {
                Processor processor { text, documentFile, reflowWidths };
                const auto [processResult, processedText] { getText (processor, formatter, isStandardInput (input) ? documentFile.getFileName() : input) };
                outputText = processedText;
                return processResult;
            }

            return textResult;
        }) };

    return { result, outputText };
}

/**
 * @brief Writes every one of @p texts to stdout, in binary mode on
 *        Windows.
 *
 * @param texts     The per-input texts, in input order.
 * @param separator The bytes written between two consecutive texts. An empty
 *                  separator writes the texts back to back.
 */
static void writeTexts (const jam::Array<std::string, juce::CriticalSection>& texts, const std::string& separator)
{
#ifdef _WIN32
    _setmode (_fileno (stdout), _O_BINARY);
#endif

    for (int index { 0 }; index < texts.size(); ++index)
    {
        if (index > 0)
            fwrite (separator.data(), 1, separator.size(), stdout);

        const auto& text { texts.at (index) };
        fwrite (text.data(), 1, text.size(), stdout);
    }
}

/**
 * @brief Runs getProcessedText() over every one of @p files and collects the
 *        texts, once every file and any explicit style path exist.
 *
 * Opens its own JUCE GUI initialiser scope. When @p files names the stdin
 * argument, stdin is read once on the calling thread before dispatch. The
 * per-input work runs in parallel through Jobs::run(). Each job writes only
 * its own slot of the text array and of the failure array, so the texts keep
 * input order. On failure, the result is the first failure by input order,
 * paired with an empty list.
 *
 * @tparam Function The callable passed through to getProcessedText().
 * @param lineWrap  The @c --line-wrap value, read once by main().
 * @param arguments The parsed command-line arguments, read for @c --style
 *                  and @c --assume-filename.
 * @param files     The inputs to process, in order.
 * @param getText   The callable passed through to getProcessedText().
 * @returns juce::Result::ok() paired with one text per input, in order, or
 *          the first failure paired with an empty list.
 */
template <typename Function>
static std::pair<juce::Result, jam::Array<std::string, juce::CriticalSection>> getProcessedTexts (int lineWrap, const juce::ArgumentList& arguments,
    const jam::Strings& files, Function&& getText)
{
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;

    const auto stylePath { getStylePath (arguments) };
    const auto assumeFilename { getAssumeFilename (arguments) };
    const auto filesFound { isFound (files) };
    const auto styleFound { isFound (stylePath) };
    jam::Array<std::string, juce::CriticalSection> processedTexts;

    if (filesFound.wasOk() and styleFound.wasOk())
    {
        const auto standardInputText { files.contains (getStandardInputArgument(), false) ? getStandardInputText() : std::pair { juce::Result::ok(), juce::String() } };
        jam::Array<std::string, juce::CriticalSection> processFailures;
        processedTexts.resize (files.size());
        processFailures.resize (files.size());

        Jobs::run (files.size(),
            [&files, &stylePath, &assumeFilename, &standardInputText, &processedTexts, &processFailures, &getText, lineWrap] (int index)
            {
                const auto [processResult, text] { getProcessedText (lineWrap, stylePath, assumeFilename, files.at (index), standardInputText, getText) };
                processedTexts.at (index) = text.toStdString();
                processFailures.at (index) = processResult.getErrorMessage().toStdString();
            });

        for (const auto& error : processFailures)
            if (not error.empty())
                return { juce::Result::fail (juce::String (error)), decltype (processedTexts) {} };

        return { juce::Result::ok(), std::move (processedTexts) };
    }

    return { filesFound.wasOk() ? styleFound : filesFound, decltype (processedTexts) {} };
}

/**
 * @brief Renders every one of @p files to stdout, through
 *        Processor::getText(), per input's own nearest style file, once
 *        every file and any explicit style path exist.
 *
 * The files come from getInputFiles(), which supplies the stdin argument
 * when the command line names no file.
 *
 * @param lineWrap  The @c --line-wrap value, read once by main().
 * @param arguments The parsed command-line arguments, read for
 *                  @c --style and @c --assume-filename.
 * @param files     The inputs to render, in order.
 * @returns @c 0 when every input renders successfully, or @c 1 after
 *          printing the first failure's error message.
 */
static int runFormatOutput (int lineWrap, const juce::ArgumentList& arguments, const jam::Strings& files)
{
    const auto [formatResult, texts] { getProcessedTexts (lineWrap, arguments, files,
        [] (const Processor& processor, const jam::MarkdownWriter& formatter, const juce::String&)
        { return processor.getText (formatter); }) };

    if (formatResult.wasOk())
    {
        writeTexts (texts, {});
        return 0;
    }

    printError (formatResult.getErrorMessage());
    return 1;
}

/**
 * @brief Dispatches a @c --format line: rejects an unknown flag or an
 *        in-place stdin request, then runs runFormatInPlace() or
 *        runFormatOutput() per @c -i.
 *
 * @param argc     The CLI argument count.
 * @param argv     The CLI argument vector.
 * @param lineWrap The @c --line-wrap value, read once by main().
 * @returns @c 0 when the selected run succeeds, or @c 1 after printing
 *          the first failure's error message.
 */
static int runFormat (int argc, char* argv[], int lineWrap)
{
    static const jam::Strings formatFlags { getFormatFlag(), getInPlaceFlag(), getLineWrapFlag() };

    const juce::ArgumentList arguments { argc, argv };
    const auto unknownFlag { getUnknownFlag (arguments, formatFlags) };

    if (unknownFlag.isEmpty())
    {
        const auto files { getInputFiles (arguments) };
        const auto isInPlaceInput { isInPlace (arguments) and files.contains (getStandardInputArgument(), false) };

        if (not isInPlaceInput)
            return isInPlace (arguments) ? runFormatInPlace (lineWrap, arguments, files)
                                          : runFormatOutput (lineWrap, arguments, files);

        printError (text::Diagnostics::failInPlaceInput);
        return 1;
    }

    printError (unknownFlag + Id::diagnosticSeparator + text::Diagnostics::failFlagUnknown);
    return 1;
}

/**
 * @brief Renders every one of @p files to stdout as JSON, through
 *        Processor::getText(JsonWriter, file), as one JSON array.
 *
 * The output is the per-input JSON objects, comma-separated, inside one
 * pair of brackets, followed by a newline. The style still resolves through
 * runStyled(), because its reflow widths tell the Model which column rows
 * to join into one value. The JSON callable does not use the formatter.
 *
 * @param arguments The parsed command-line arguments, read for @c --style
 *                  and @c --assume-filename.
 * @param files     The inputs to render, in order.
 * @returns @c 0 when every input renders successfully, or @c 1 after
 *          printing the first failure's error message.
 */
static int runTablesOutput (const juce::ArgumentList& arguments, const jam::Strings& files)
{
    const auto [tablesResult, texts] { getProcessedTexts (absentFlagValue, arguments, files,
        [] (const Processor& processor, const jam::MarkdownWriter&, const juce::String& file)
        { return processor.getText (JsonWriter {}, file); }) };

    if (tablesResult.wasOk())
    {
        fputc (static_cast<int> (Chars::openBracket), stdout);
        writeTexts (texts, std::string (1, static_cast<char> (Chars::comma)));
        fputc (static_cast<int> (Chars::closeBracket), stdout);
        fputc (static_cast<int> (Chars::newline), stdout);
        return 0;
    }

    printError (tablesResult.getErrorMessage());
    return 1;
}

/**
 * @brief Dispatches a @c --tables line: rejects an option outside the
 *        @c --tables grammar, then runs runTablesOutput() over
 *        getInputFiles().
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c 0 when the tables run succeeds, or @c 1 after printing the
 *          first failure's error message.
 */
static int runTables (int argc, char* argv[])
{
    static const jam::Strings tablesFlags { getTablesFlag() };

    const juce::ArgumentList arguments { argc, argv };
    const auto unknownFlag { getUnknownFlag (arguments, tablesFlags) };

    if (unknownFlag.isEmpty())
        return runTablesOutput (arguments, getInputFiles (arguments));

    printError (unknownFlag + Id::diagnosticSeparator + text::Diagnostics::failFlagUnknown);
    return 1;
}

/**
 * @brief Runs Sync between @p sourceRoot and @p targetRoot, under a
 *        scoped JUCE GUI initialiser and the framework's shared-instance
 *        stamp, resolving @p stylePath or the target's own sync style
 *        file, and prints the resulting error to stderr on failure.
 *
 * @param lineWrap   The @c --line-wrap value, read once by main().
 * @param sourceRoot The sync source root directory.
 * @param targetRoot The sync target root directory.
 * @param stylePath  The explicit @c --style=file:\<path\> path, or an empty
 *                   string to resolve the target's own sync style file.
 * @returns @c 0 when the sync run succeeds, or @c 1 after printing the
 *          failure's error message.
 */
static int runSync (int lineWrap, const juce::File& sourceRoot, const juce::File& targetRoot, const juce::String& stylePath)
{
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;

    const auto styleFile { getStyleFile (stylePath, getStyleFile (targetRoot.getChildFile (files::castDirectory))) };
    const auto found { isFound (stylePath) };

    if (found.wasOk())
    {
        const auto result { runStyled (lineWrap, styleFile,
            [&sourceRoot, &targetRoot] (const jam::MarkdownWriter& formatter, const jam::HashMap<juce::Identifier, int>&)
            { return Sync::run (sourceRoot, targetRoot, formatter); }) };

        if (result.wasOk())
        {
            printDone();
            return 0;
        }

        printError (result.getErrorMessage());
        return 1;
    }

    printError (found.getErrorMessage());
    return 1;
}

/**
 * @brief Answers @c --sync's own arity requirement -- exactly its two
 *        root arguments, an optional well-formed @c --style=file:\<path\>
 *        removed first -- then dispatches to runSync(), or reports the
 *        arity fatal on stderr.
 *
 * @param argc     The CLI argument count.
 * @param argv     The CLI argument vector.
 * @param lineWrap The @c --line-wrap value, read once by main().
 * @returns @c 0 when the sync run succeeds, or @c 1 after printing the
 *          arity fatal or the sync run's own failure.
 */
static int runSyncArguments (int argc, char* argv[], int lineWrap)
{
    juce::ArgumentList arguments { argc, argv };
    const auto styleValue { arguments.getValueForOption (getStyleFlag()) };
    const auto stylePath { getStylePath (arguments) };
    const auto isStyleValid { not arguments.containsOption (getStyleFlag())
                              or (styleValue.startsWith (getStyleFilePrefix()) and stylePath.isNotEmpty()) };

    arguments.removeOptionIfFound (getSyncFlag());
    arguments.removeValueForOption (getStyleFlag());

    if (isStyleValid and arguments.size() == syncRootCount and not arguments[0].isOption() and not arguments[1].isOption())
        return runSync (lineWrap, arguments[0].resolveAsFile(), arguments[1].resolveAsFile(), stylePath);

    printError (getSyncFlag() + Id::diagnosticSeparator + text::Diagnostics::failSyncArguments);
    return 1;
}

static constexpr int packArchiveCount { 1 };///< The number of leading positional paths a `--pack` invocation uses for the archive.

/**
 * @brief Returns the arguments of @p arguments that are not options, in
 *        authored order.
 *
 * The first path is the archive. Each path after it belongs to a row.
 *
 * @param arguments The parsed command-line arguments.
 * @returns The arguments without a leading dash, program name excluded.
 */
static juce::StringArray getPackPaths (const juce::ArgumentList& arguments)
{
    juce::StringArray paths;

    for (const auto& argument : arguments.arguments)
        if (not argument.isOption())
            paths.add (argument.text);

    return paths;
}

/**
 * @brief Answers whether every @c --link of @p arguments is well formed.
 *
 * A @c --link is well formed when it follows an item path, is the only
 * @c --link of that item, and has a non-empty name and a non-empty target
 * in its @c \<name\>=\<target\> value.
 *
 * @param arguments The parsed command-line arguments.
 * @returns @c true when every @c --link is well formed.
 */
static bool isLinkValid (const juce::ArgumentList& arguments)
{
    const auto linkSeparator { juce::String::charToString (Chars::equals) };
    int pathCount { 0 };
    bool isRowLinked { false };

    for (const auto& argument : arguments.arguments)
    {
        if (not argument.isOption())
        {
            ++pathCount;
            isRowLinked = false;
        }
        else if (argument.isLongOption (getLinkFlag()))
        {
            const auto value { argument.getLongOptionValue() };

            if (pathCount <= packArchiveCount or isRowLinked
                or value.upToFirstOccurrenceOf (linkSeparator, false, false).isEmpty()
                or value.fromFirstOccurrenceOf (linkSeparator, false, false).isEmpty()) return false;

            isRowLinked = true;
        }
    }

    return true;
}

/**
 * @brief Returns the rows of @p arguments, flat.
 *
 * Each path after the archive adds one row of Pack::tripleSize values: the
 * item path, the link name, and the link target. The link name and the link
 * target are empty when the item has no @c --link.
 *
 * @param arguments The parsed command-line arguments. Every @c --link is
 *                  well formed.
 * @returns The rows, flat.
 */
static juce::StringArray getPackRows (const juce::ArgumentList& arguments)
{
    jassert (isLinkValid (arguments));

    const auto linkSeparator { juce::String::charToString (Chars::equals) };
    juce::StringArray rows;
    int pathCount { 0 };

    for (const auto& argument : arguments.arguments)
    {
        if (not argument.isOption())
        {
            ++pathCount;

            if (pathCount > packArchiveCount)
            {
                rows.add (argument.text);
                rows.add (juce::String());
                rows.add (juce::String());
            }
        }
        else if (argument.isLongOption (getLinkFlag()))
        {
            const auto value { argument.getLongOptionValue() };
            const auto rowStart { (rows.size() / Pack::tripleSize - 1) * Pack::tripleSize };

            rows.set (rowStart + Pack::linkNameOffset, value.upToFirstOccurrenceOf (linkSeparator, false, false));
            rows.set (rowStart + Pack::linkTargetOffset, value.fromFirstOccurrenceOf (linkSeparator, false, false));
        }
    }

    return rows;
}

/**
 * @brief Runs Pack::toArchive() under a scoped JUCE GUI initialiser and
 *        the framework's shared-instance stamp, and prints the resulting
 *        error to stderr on failure.
 *
 * @param arguments The parsed command-line arguments.
 * @param archive   The archive to write.
 * @param triples   The rows, flat: item path, link name, link target for
 *                  each row.
 * @returns @c 0 when the pack succeeds, or @c 1 after printing the
 *          failure's error message.
 */
static int runPack (const juce::ArgumentList& arguments, const juce::File& archive, const juce::StringArray& triples)
{
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;

    const auto result { Pack::toArchive (arguments, archive, triples) };

    if (result.wasOk())
    {
        printDone();
        return 0;
    }

    printError (result.getErrorMessage());
    return 1;
}

/**
 * @brief Answers @c --pack's own arity requirement -- one archive path and
 *        at least one item, each item with at most one well-formed
 *        @c --link=\<name\>=\<target\> -- and its layout options requirement
 *        -- an integer value for each -- then dispatches to runPack(), or
 *        reports the failure on stderr.
 *
 * The arity fails when no archive or no item is given, when a @c --link
 * precedes the first item, when one item has a second @c --link, or when a
 * @c --link has an empty name or an empty target.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c 0 when the pack succeeds, or @c 1 after printing the arity
 *          fatal, the layout value fatal, or the pack's own failure.
 */
static int runPackArguments (int argc, char* argv[])
{
    const juce::ArgumentList arguments { argc, argv };
    const auto paths { getPackPaths (arguments) };

    if (paths.size() > packArchiveCount and isLinkValid (arguments))
    {
        if (BinaryWriter::isLayoutValid (arguments))
        {
            const auto archive { juce::File::getCurrentWorkingDirectory().getChildFile (paths[0]) };

            return runPack (arguments, archive, getPackRows (arguments));
        }

        printError (getPackFlag() + Id::diagnosticSeparator + text::Diagnostics::failFlagValue);
        return 1;
    }

    printError (getPackFlag() + Id::diagnosticSeparator + text::Diagnostics::failPackArguments);
    return 1;
}

/**
 * @brief Dispatches @p argv's own version, help, format, or document
 *        request.
 *
 * When @p argv declares no manifest, prints the banner and the guide and
 * returns @c 0, the same as the help request. A declared manifest that does
 * not exist goes to runDocumentNotFound(). main() dispatches the pack, sync,
 * and tables requests before this function is reached.
 *
 * @param argc     The CLI argument count.
 * @param argv     The CLI argument vector.
 * @param lineWrap The @c --line-wrap value, read once by main().
 * @returns @c 0 when the selected dispatch succeeds, or @c 1 after
 *          printing its own failure.
 */
static int runCommandLine (int argc, char* argv[], int lineWrap)
{
    if (isVersion (argc, argv))
    {
        writeVersion();
        return 0;
    }

    if (isHelp (argc, argv))
    {
        printBannerAndHelp();
        return 0;
    }

    if (isFormatOnly (argc, argv))
        return runFormat (argc, argv, lineWrap);

    if (isManifestDeclared (argc, argv))
    {
        const auto documentFile { getDocumentFile (argc, argv) };

        if (documentFile.existsAsFile())
            return runDocument (lineWrap, documentFile, isSkipFormat (argc, argv),
                getOutputDirectory (argc, argv), getToolchainArgument (argc, argv));

        return runDocumentNotFound (documentFile);
    }

    printBannerAndHelp();
    return 0;
}

/**
 * @brief `cast`'s own entry point -- sets the Windows console to UTF-8,
 *        clears the terminal unless the run is a `--format`, `--pack`, or
 *        `--tables` run, then dispatches @p argv to runPackArguments() for
 *        `--pack`, to runSyncArguments() for `--sync`, to runTables() for
 *        `--tables`, and to runCommandLine() otherwise.
 *
 * An invalid `--line-wrap` value fails with a flag-value error before
 * runCommandLine() is reached. The `--pack`, `--sync`, and `--tables` runs
 * return before that check.
 *
 * @param argc The argument count.
 * @param argv The argument vector.
 * @returns The dispatched function's own result, or @c 1 after printing
 *          the flag-value error.
 */
int main (int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP (CP_UTF8);
#endif

    const auto lineWrap { getFlagValue (argc, argv, getLineWrapFlag(), absentFlagValue) };

    if (isTerminalOutput() and not isFormatOnly (argc, argv) and not isPackFlag (argc, argv) and not isTablesFlag (argc, argv))
    {
        juce::MemoryOutputStream erase;
        jam::TerminalEncoder::writeEraseScreen (erase);
        fwrite (erase.getData(), sizeof (char), erase.getDataSize(), stdout);
        fflush (stdout);
    }

    if (isPackFlag (argc, argv))
        return runPackArguments (argc, argv);

    if (isSyncFlag (argc, argv))
        return runSyncArguments (argc, argv, lineWrap);

    if (isTablesFlag (argc, argv))
        return runTables (argc, argv);

    if (lineWrap != invalidFlagValue)
        return runCommandLine (argc, argv, lineWrap);

    printError (text::Diagnostics::failFlagValue);
    return 1;
}
