/**
 * @file main.cpp
 * @brief `cast` CLI entry point: banner/help rendering and manifest dispatch.
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
 * @brief Prints the banner and help text to stdout.
 */
static void printBannerAndHelp()
{
    static const auto newlineText { juce::String::charToString (Chars::newline) };

    printBanner();
    printf ("%s", newlineText.toRawUTF8());
    printHelp (BinaryData::getString (files::castHelp));
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
 * @brief The lone `-` argument that reads stdin.
 * @returns The `-` argument text.
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
 * @brief Resolves @p argv's own manifest file against the current working
 *        directory -- the argument at getManifestIndex(), or, absent one,
 *        the default @c spell.md file name.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns The resolved manifest file.
 */
static juce::File getDocumentFile (int argc, char* argv[])
{
    const auto manifestIndex { getArgumentIndex (argc, argv, getManifestIndex (argc, argv)) };

    return manifestIndex < argc
               ? juce::File::getCurrentWorkingDirectory().getChildFile (
                     juce::String::fromUTF8 (argv[manifestIndex]))
               : juce::File::getCurrentWorkingDirectory().getChildFile (files::cast);
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
 * @brief Returns the first dash argument in @p arguments outside the
 *        @c --format grammar -- exactly @c --format, @c -i,
 *        @c --line-wrap, or @c -, a @c --style=file[:\<path\>] value, or a
 *        @c --assume-filename=\<path\> value.
 *
 * @param arguments The parsed command-line arguments.
 * @returns The first unknown flag's own text, or an empty string when
 *          every dash argument matches the @c --format grammar.
 */
static juce::String getUnknownFlag (const juce::ArgumentList& arguments)
{
    static const jam::Strings formatFlags { getFormatFlag(), getInPlaceFlag(), getLineWrapFlag(), getStandardInputArgument() };
    static const juce::String styleFileFlag { getStyleFlag() + juce::String::charToString (Chars::equals) + Id::file.toString() };

    for (const auto& argument : arguments.arguments)
    {
        const auto isStyleArgument { argument.text.startsWith (styleFileFlag)
                                     and (argument.text.compare (styleFileFlag) == 0
                                          or argument.text.startsWith (styleFileFlag + juce::String::charToString (Chars::colon))) };

        if (argument.isOption() and not formatFlags.contains (argument.text, false) and not isStyleArgument
            and not argument.text.startsWith (getAssumeFilenameFlag() + juce::String::charToString (Chars::equals)))
            return argument.text;
    }

    return {};
}

/**
 * @brief Answers whether @p input is the lone @c - stdin argument.
 *
 * @param input One @c --format input argument.
 * @returns @c true when @p input is the stdin argument.
 */
static bool isStandardInput (const juce::String& input)
{
    return input.compare (getStandardInputArgument()) == 0;
}

/**
 * @brief Returns every @c --format file argument in @p arguments, in
 *        order -- every argument that is not an option, or that is the
 *        stdin argument, skipping the @c --line-wrap value.
 *
 * @param arguments The parsed command-line arguments.
 * @returns @p arguments' own file arguments, in authored order.
 */
static jam::Strings getFormatFiles (const juce::ArgumentList& arguments)
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

            if (not argument.isOption() or isStandardInput (argument.text))
                files.add (argument.text);
        }
    }

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
 * @brief Returns @p input's own decoded text -- stdin's own text for the
 *        stdin argument, or @p input's own file loaded from the current
 *        working directory.
 *
 * @param input One @c --format input argument.
 * @returns juce::Result::ok() paired with the decoded text, or a failure
 *          naming the read error paired with an empty string.
 */
static std::pair<juce::Result, juce::String> getInputText (const juce::String& input)
{
    if (isStandardInput (input))
        return getStandardInputText();

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
 * @brief Resolves @p styleFile into a style Model and, when it validates,
 *        the reflow widths and writer it declares, then invokes
 *        @p function with them.
 *
 * @pre The JUCE GUI initialiser and jam::Stamp scope is open.
 *
 * @tparam Function A callable invoked as
 *                  @c function(formatter,reflowWidths), returning a
 *                  juce::Result.
 * @param lineWrap  The @c --line-wrap value, read once by main(); used
 *                  when positive, the style's own line wrap otherwise.
 * @param styleFile The style file to parse and validate.
 * @param function  The callable invoked with the resolved writer and
 *                  reflow widths.
 * @returns @p function's own result, or the style's own validation
 *          failure.
 */
template <typename Function>
static juce::Result runStyled (int lineWrap, const juce::File& styleFile, Function&& function)
{
    const auto style { Model::parse (styleFile) };
    const auto result { Validator::isFormat (*style) };

    if (result.wasOk())
    {
        const auto reflowWidths { style->getReflowWidths() };
        const jam::MarkdownWriter formatter { lineWrap > 0 ? lineWrap : style->getLineWrap(), reflowWidths };

        return function (formatter, reflowWidths);
    }

    return result;
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
    jam::Stamp stamp;

    const auto result { runStyled (lineWrap, getNearestStyleFile (documentFile.getParentDirectory()),
        [&documentFile, skipFormat, &outputDirectory, &toolchainArgument]
        (const jam::MarkdownWriter& formatter, const jam::HashMap<juce::Identifier, int>& reflowWidths)
        {
            Processor processor { documentFile.loadFileAsString(),
                documentFile.getRelativePathFrom (documentFile.getParentDirectory()),
                documentFile.getParentDirectory(), reflowWidths };

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
        const auto file { workingDirectory.getChildFile (name) };
        const auto directory { file.getParentDirectory() };
        const auto styleFile { getStyleFile (stylePath, getNearestStyleFile (directory)) };

        const auto formatResult { runStyled (lineWrap, styleFile,
            [&file, &directory] (const jam::MarkdownWriter& formatter, const jam::HashMap<juce::Identifier, int>& reflowWidths)
            {
                Processor processor { file.loadFileAsString(), file.getRelativePathFrom (directory), directory, reflowWidths };
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
        const auto file { workingDirectory.getChildFile (name) };
        const auto directory { file.getParentDirectory() };
        const auto styleFile { getStyleFile (stylePath, getNearestStyleFile (directory)) };

        const auto formatResult { runStyled (lineWrap, styleFile,
            [&file, &directory] (const jam::MarkdownWriter& formatter, const jam::HashMap<juce::Identifier, int>& reflowWidths)
            {
                Processor processor { file.loadFileAsString(), file.getRelativePathFrom (directory), directory, reflowWidths };
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
    jam::Stamp stamp;

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
 * @brief Resolves @p input's own origin, directory, and style, then
 *        returns its canonicalized text.
 *
 * @param lineWrap      The @c --line-wrap value, read once by main().
 * @param stylePath     An explicit @c --style=file:\<path\> path, or an
 *                      empty string to resolve @p input's own nearest
 *                      style file.
 * @param assumeFilename The @c --assume-filename value, read for the
 *                      stdin input only.
 * @param input         One @c --format input argument.
 * @returns The style's own validation or read result, paired with the
 *          canonicalized text on success.
 */
static std::pair<juce::Result, juce::String> getFormattedText (int lineWrap, const juce::String& stylePath,
    const juce::String& assumeFilename, const juce::String& input)
{
    const auto workingDirectory { juce::File::getCurrentWorkingDirectory() };
    const auto assumedFile { workingDirectory.getChildFile (assumeFilename) };
    const auto inputFile { workingDirectory.getChildFile (input) };
    const auto directory { isStandardInput (input)
                           ? (assumeFilename.isNotEmpty() ? assumedFile.getParentDirectory() : workingDirectory)
                           : inputFile.getParentDirectory() };
    const auto origin { isStandardInput (input)
                        ? (assumeFilename.isNotEmpty() ? assumedFile.getFileName() : juce::String (files::standardInput))
                        : inputFile.getRelativePathFrom (directory) };
    juce::String canonicalText;

    const auto result { runStyled (lineWrap, getStyleFile (stylePath, getNearestStyleFile (directory)),
        [&input, &origin, &directory, &canonicalText] (const jam::MarkdownWriter& formatter, const jam::HashMap<juce::Identifier, int>& reflowWidths)
        {
            const auto [textResult, text] { getInputText (input) };

            if (textResult.wasOk())
            {
                Processor processor { text, origin, directory, reflowWidths };
                const auto [processResult, processedText] { processor.getText (formatter) };
                canonicalText = processedText;
                return processResult;
            }

            return textResult;
        }) };

    return { result, canonicalText };
}

/**
 * @brief Writes every one of @p texts to stdout, in binary mode on
 *        Windows.
 *
 * @param texts The canonicalized texts to write, in order.
 */
static void writeTexts (const jam::Strings& texts)
{
#ifdef _WIN32
    _setmode (_fileno (stdout), _O_BINARY);
#endif

    for (const auto& text : texts)
        fwrite (text.toRawUTF8(), 1, text.getNumBytesAsUTF8(), stdout);
}

/**
 * @brief Renders every one of @p files, or one stdin input when @p files
 *        is empty, to stdout, per input's own nearest style file, once
 *        every file and any explicit style path exist.
 *
 * @param lineWrap  The @c --line-wrap value, read once by main().
 * @param arguments The parsed command-line arguments, read for
 *                  @c --style and @c --assume-filename.
 * @param files     The files to render, or empty for one stdin input.
 * @returns @c 0 when every input renders successfully, or @c 1 after
 *          printing the first failure's error message.
 */
static int runFormatOutput (int lineWrap, const juce::ArgumentList& arguments, const jam::Strings& files)
{
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;
    jam::Stamp stamp;

    const auto stylePath { getStylePath (arguments) };
    const auto assumeFilename { getAssumeFilename (arguments) };
    const auto filesFound { isFound (files) };
    const auto styleFound { isFound (stylePath) };

    if (filesFound.wasOk() and styleFound.wasOk())
    {
        jam::Strings texts;

        for (const auto& input : files.size() > 0 ? files : jam::Strings { getStandardInputArgument() })
        {
            const auto [formatResult, text] { getFormattedText (lineWrap, stylePath, assumeFilename, input) };

            if (not formatResult.wasOk())
            {
                printError (formatResult.getErrorMessage());
                return 1;
            }

            texts.add (text);
        }

        writeTexts (texts);
        return 0;
    }

    printError (filesFound.wasOk() ? styleFound.getErrorMessage() : filesFound.getErrorMessage());
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
    const juce::ArgumentList arguments { argc, argv };
    const auto unknownFlag { getUnknownFlag (arguments) };

    if (unknownFlag.isEmpty())
    {
        const auto files { getFormatFiles (arguments) };
        const auto isInPlaceInput { isInPlace (arguments)
                                    and (files.size() == 0 or files.contains (getStandardInputArgument(), false)) };

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
    jam::Stamp stamp;

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

/**
 * @brief Returns the arguments of @p argv that are not options, in
 *        authored order.
 *
 * The first path is the archive. Each path after it belongs to a row.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns The arguments without a leading dash, program name excluded.
 */
static juce::StringArray getPackPaths (int argc, char* argv[])
{
    juce::StringArray paths;

    for (const auto& text : juce::StringArray (argv + 1, argc - 1))
        if (not juce::ArgumentList::Argument { text }.isOption())
            paths.add (text);

    return paths;
}

/**
 * @brief Runs Pack::toArchive() under a scoped JUCE GUI initialiser and
 *        the framework's shared-instance stamp, and prints the resulting
 *        error to stderr on failure.
 *
 * @param arguments The parsed command-line arguments.
 * @param archive   The archive to write.
 * @param triples   The rows, flat: item, link name, link target for each
 *                  row.
 * @returns @c 0 when the pack succeeds, or @c 1 after printing the
 *          failure's error message.
 */
static int runPack (const juce::ArgumentList& arguments, const juce::File& archive, const juce::StringArray& triples)
{
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;
    jam::Stamp stamp;

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
 *        complete triples -- and its layout options requirement -- an
 *        integer value for each -- then dispatches to runPack(), or reports
 *        the failure on stderr.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c 0 when the pack succeeds, or @c 1 after printing the arity
 *          fatal, the layout value fatal, or the pack's own failure.
 */
static int runPackArguments (int argc, char* argv[])
{
    const juce::ArgumentList arguments { argc, argv };
    const auto paths { getPackPaths (argc, argv) };

    if (paths.size() > 1 and (paths.size() - 1) % Pack::tripleSize == 0)
    {
        if (BinaryWriter::isLayoutValid (arguments))
        {
            const auto archive { juce::File::getCurrentWorkingDirectory().getChildFile (paths[0]) };
            juce::StringArray triples;

            triples.addArray (paths, 1);
            return runPack (arguments, archive, triples);
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

    const auto documentFile { getDocumentFile (argc, argv) };

    if (documentFile.existsAsFile())
        return runDocument (lineWrap, documentFile, isSkipFormat (argc, argv),
            getOutputDirectory (argc, argv), getToolchainArgument (argc, argv));

    return runDocumentNotFound (documentFile);
}

/**
 * @brief `cast`'s own entry point -- sets up UTF-8 console output, clears
 *        the terminal outside a `--format` or `--pack` run, then
 *        dispatches @p argv to runPackArguments() for `--pack`, to
 *        runSyncArguments() for `--sync`, and to runCommandLine()
 *        otherwise.
 *
 * @param argc The argument count.
 * @param argv The argument vector.
 * @returns The dispatched function's own result, or @c 1 after printing
 *          the failure when the `--line-wrap` value is invalid.
 */
int main (int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP (CP_UTF8);
#endif

    if (isTerminalOutput() and not isFormatOnly (argc, argv) and not isPackFlag (argc, argv))
    {
#ifdef _WIN32
        std::system ("cls");
#else
        std::system ("clear");
#endif
    }

    const auto lineWrap { getFlagValue (argc, argv, getLineWrapFlag(), absentFlagValue) };

    if (isPackFlag (argc, argv))
        return runPackArguments (argc, argv);

    if (isSyncFlag (argc, argv))
        return runSyncArguments (argc, argv, lineWrap);

    if (lineWrap != invalidFlagValue)
        return runCommandLine (argc, argv, lineWrap);

    printError (text::Diagnostics::failFlagValue);
    return 1;
}
