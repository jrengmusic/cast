/**
 * @file main.cpp
 * @brief `cast` CLI entry point: banner/help rendering and manifest dispatch.
 */

#include <JuceHeader.h>
#include "Processor.h"
#include "Help.h"

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
    printBanner();
    printf ("%s", juce::String::charToString (Chars::newline).toRawUTF8());
    printHelp (BinaryData::getString (files::castHelp));
}

static constexpr int flagArgIndex { 1 };
static constexpr int postFlagArgIndex { 2 };
static constexpr int flagOnlyArgumentCount { 2 };
static constexpr int postFlagArgumentCount { 3 };

static const juce::String formatFlag { Id::doubleDash + Id::format.toString() };
static const juce::String noFormatFlag { Id::doubleDash + Id::noFormat.toString() };
static const juce::String versionFlag { Id::doubleDash + Id::version.toString() };
static const juce::String helpFlag { Id::doubleDash + Id::help.toString() };

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
    return argc >= flagOnlyArgumentCount ? juce::String::fromUTF8 (argv[flagArgIndex]) : juce::String {};
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
    return argc >= postFlagArgumentCount ? juce::String::fromUTF8 (argv[postFlagArgIndex]) : juce::String {};
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
    return getFlagArgument (argc, argv) == formatFlag or getPostFlagArgument (argc, argv) == formatFlag;
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
    return getFlagArgument (argc, argv) == noFormatFlag or getPostFlagArgument (argc, argv) == noFormatFlag;
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

    return (flagArgument == formatFlag or flagArgument == noFormatFlag) ? postFlagArgIndex : flagArgIndex;
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
    const auto isExactManifestSlot { argc == postFlagArgumentCount
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
    return manifestArgument.startsWith (Id::doubleDash.toString())
           and manifestArgument != formatFlag and manifestArgument != noFormatFlag
           and manifestArgument != versionFlag and manifestArgument != helpFlag;
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
 *        the default @c CAST.md file name.
 *
 * @param argc The CLI argument count.
 * @param argv The CLI argument vector.
 * @returns The resolved manifest file.
 */
static juce::File getDocumentFile (int argc, char* argv[])
{
    const auto manifestIndex { getManifestIndex (argc, argv) };

    return argc > manifestIndex
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
    return (getFlagArgument (argc, argv) == versionFlag and argc == flagOnlyArgumentCount)
           or getManifestArgument (argc, argv) == versionFlag;
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
    return (getFlagArgument (argc, argv) == helpFlag and argc == flagOnlyArgumentCount)
           or getManifestArgument (argc, argv) == helpFlag;
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
                             + juce::String::charToString (Chars::space)
                             + juce::String::charToString (Chars::openParen) + CAST_COMMIT
                             + juce::String::charToString (Chars::closeParen) };
    printf ("%s\n", versionLine.toRawUTF8());
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
 * @returns @c 0 when every run step succeeds, or @c 1 after printing the
 *          first failure's error message.
 */
static int runDocument (const juce::File& documentFile, bool skipFormat, bool formatOnly,
    const juce::String& outputDirectory, const juce::String& toolchainArgument)
{
    Processor processor { documentFile };
    auto result { juce::Result::ok() };

    if (not skipFormat)
        result = processor.format();

    if (result.wasOk() and not formatOnly)
        result = processor.generate (outputDirectory, toolchainArgument);

    if (result.wasOk())
        return 0;

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

int main (int argc, char* argv[])
{
#ifdef _WIN32
    std::system ("cls");
#else
    std::system ("clear");
#endif

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
            getOutputDirectory (argc, argv), getToolchainArgument (argc, argv));

    return runDocumentNotFound (documentFile);
}
