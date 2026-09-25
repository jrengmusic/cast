/**
 * @file main.cpp
 * @brief `cast` CLI entry point: banner/help rendering and manifest dispatch.
 */

#include <JuceHeader.h>
#include "Processor.h"
#include "Sync.h"
#include "Help.h"

#ifdef _WIN32
#include <io.h>
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

static constexpr int flagArgIndex { 1 };
static constexpr int postFlagArgIndex { 2 };
static constexpr int flagOnlyArgumentCount { 2 };
static constexpr int postFlagArgumentCount { 3 };
static constexpr int invalidFlagValue { -1 };///< The value getFlagValue() returns when the flag's own text is not an integer.

static const juce::String& getFormatFlag()
{
    static const juce::String formatFlag { Id::doubleDash + Id::format.toString() };
    return formatFlag;
}

static const juce::String& getNoFormatFlag()
{
    static const juce::String noFormatFlag { Id::doubleDash + Id::noFormat.toString() };
    return noFormatFlag;
}

static const juce::String& getVersionFlag()
{
    static const juce::String versionFlag { Id::doubleDash + Id::version.toString() };
    return versionFlag;
}

static const juce::String& getHelpFlag()
{
    static const juce::String helpFlag { Id::doubleDash + Id::help.toString() };
    return helpFlag;
}

static const juce::String& getSyncFlag()
{
    static const juce::String syncFlag { Id::doubleDash + Id::sync.toString() };
    return syncFlag;
}

static const juce::String& getMaxTableWidthFlag()
{
    static const juce::String maxTableWidthFlag { Id::doubleDash + Id::maxTableWidth.toString() };
    return maxTableWidthFlag;
}

static const juce::String& getLineWrapFlag()
{
    static const juce::String lineWrapFlag { Id::doubleDash + Id::lineWrap.toString() };
    return lineWrapFlag;
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
 *          round-trip as an integer.
 */
static int getFlagValue (int argc, char* argv[], const juce::String& flag, int fallbackValue)
{
    for (int index { 0 }; index < argc - 1; ++index)
        if (juce::String::fromUTF8 (argv[index]).compare (flag) == 0)
        {
            const auto text { juce::String::fromUTF8 (argv[index + 1]) };

            return juce::String (text.getIntValue()).compare (text) == 0 ? text.getIntValue() : invalidFlagValue;
        }

    return fallbackValue;
}

static int getArgumentIndex (int argc, char* argv[], int position)
{
    auto filteredPosition { 0 };
    auto index { 0 };

    while (index < argc)
    {
        const auto isMaxTableWidthPair { juce::String::fromUTF8 (argv[index]).compare (getMaxTableWidthFlag()) == 0
                                         and index + 1 < argc };
        const auto isLineWrapPair { juce::String::fromUTF8 (argv[index]).compare (getLineWrapFlag()) == 0
                                    and index + 1 < argc };

        if (isMaxTableWidthPair or isLineWrapPair)
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
 * @brief Answers whether @p argv declares the @c --format flag at either
 *        the flag or post-flag position.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c true when @p argv declares @c --format.
 */
static bool isFormatOnly (int argc, char* argv[])
{
    return getFlagArgument (argc, argv).compare (getFormatFlag()) == 0
           or getPostFlagArgument (argc, argv).compare (getFormatFlag()) == 0;
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
    static const jam::Strings reservedFlags { getFormatFlag(), getNoFormatFlag(), getVersionFlag(), getHelpFlag(), getSyncFlag() };

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
 * @brief Parses @p documentFile into a Processor, then runs its format()
 *        and generate() as @p skipFormat and @p formatOnly select, and
 *        prints the resulting error to stderr on failure.
 *
 * @param documentFile      The manifest file to process.
 * @param skipFormat        Whether to skip Processor::format().
 * @param formatOnly        Whether to skip Processor::generate().
 * @param outputDirectory   The directory passed through to
 *                          Processor::generate().
 * @param toolchainArgument The CLI-selected toolchain group, passed
 *                          through to Processor::generate().
 * @param maxTableWidth     The grid-table width passed through to
 *                          Processor::format().
 * @param lineWrap          The paragraph wrap column passed through to
 *                          Processor::format().
 * @returns @c 0 when every run step succeeds, or @c 1 after printing the
 *          first failure's error message.
 */
static int runDocument (const juce::File& documentFile, bool skipFormat, bool formatOnly,
    const juce::String& outputDirectory, const juce::String& toolchainArgument, int maxTableWidth, int lineWrap)
{
    Processor processor { documentFile };
    auto result { juce::Result::ok() };

    if (not skipFormat)
        result = processor.format (maxTableWidth, lineWrap);

    if (result.wasOk() and not formatOnly)
        result = processor.generate (outputDirectory, toolchainArgument);

    if (result.wasOk())
    {
        if (isTerminalOutput())
        {
            const auto doneLine { ProjectInfo::projectName + Id::diagnosticSeparator + text::Diagnostics::done };
            printf ("%s\n", doneLine.toRawUTF8());
        }

        return 0;
    }

    const auto errorLine { ProjectInfo::projectName + Id::diagnosticSeparator
                           + result.getErrorMessage() };
    fprintf (stderr, "%s\n", errorLine.toRawUTF8());
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
    const auto notFoundLine { ProjectInfo::projectName + Id::diagnosticSeparator
                              + text::Diagnostics::failNotFound + Id::diagnosticSeparator
                              + documentFile.getFileName() };
    fprintf (stderr, "%s\n", notFoundLine.toRawUTF8());
    return 1;
}

static constexpr int syncArgumentCount { 4 };
static constexpr int syncSourceArgIndex { 2 };
static constexpr int syncTargetArgIndex { 3 };

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
 * @brief Runs Sync between @p sourceRoot and @p targetRoot, under a scoped
 *        JUCE GUI initialiser and the framework's shared-instance stamp,
 *        and prints the resulting error to stderr on failure.
 *
 * @param sourceRoot The sync source root directory.
 * @param targetRoot The sync target root directory.
 * @returns @c 0 when the sync run succeeds, or @c 1 after printing the
 *          failure's error message.
 */
static int runSync (const juce::File& sourceRoot, const juce::File& targetRoot)
{
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;
    Generated generated;
    jam::Stamp stamp;

    const auto result { Sync::run (sourceRoot, targetRoot) };

    if (result.wasOk())
    {
        if (isTerminalOutput())
        {
            const auto doneLine { ProjectInfo::projectName + Id::diagnosticSeparator + text::Diagnostics::done };
            printf ("%s\n", doneLine.toRawUTF8());
        }

        return 0;
    }

    const auto errorLine { ProjectInfo::projectName + Id::diagnosticSeparator + result.getErrorMessage() };
    fprintf (stderr, "%s\n", errorLine.toRawUTF8());
    return 1;
}

/**
 * @brief Answers @c --sync's own arity requirement -- exactly its two root
 *        arguments -- then dispatches to runSync(), or reports the arity
 *        fatal on stderr.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns @c 0 when the sync run succeeds, or @c 1 after printing the
 *          arity fatal or the sync run's own failure.
 */
static int runSyncArguments (int argc, char* argv[])
{
    if (argc == syncArgumentCount)
        return runSync (
            juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[syncSourceArgIndex])),
            juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[syncTargetArgIndex])));

    const auto errorLine { ProjectInfo::projectName + Id::diagnosticSeparator + getSyncFlag()
                           + Id::diagnosticSeparator + text::Diagnostics::failSyncArguments };
    fprintf (stderr, "%s\n", errorLine.toRawUTF8());
    return 1;
}

int main (int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP (CP_UTF8);
#endif

    if (isTerminalOutput())
    {
#ifdef _WIN32
        std::system ("cls");
#else
        std::system ("clear");
#endif
    }

    if (isSyncFlag (argc, argv))
        return runSyncArguments (argc, argv);

    const auto maxTableWidth { getFlagValue (argc, argv, getMaxTableWidthFlag(), jam::MarkdownWriter::defaultMaxTableWidth) };
    const auto lineWrap { getFlagValue (argc, argv, getLineWrapFlag(), jam::MarkdownWriter::defaultLineWrap) };

    const auto isLineWrapValid { lineWrap > 0 };
    const auto isMaxTableWidthValid { maxTableWidth >= 0 };

    if (isLineWrapValid and isMaxTableWidthValid)
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

        const auto documentFile { getDocumentFile (argc, argv) };

        if (documentFile.existsAsFile())
            return runDocument (documentFile, isSkipFormat (argc, argv), isFormatOnly (argc, argv),
                getOutputDirectory (argc, argv), getToolchainArgument (argc, argv),
                maxTableWidth, lineWrap);

        return runDocumentNotFound (documentFile);
    }

    const auto errorLine { ProjectInfo::projectName + Id::diagnosticSeparator + text::Diagnostics::failFlagValue };
    fprintf (stderr, "%s\n", errorLine.toRawUTF8());
    return 1;
}
