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
 * @brief Initializes the terminal graphics stack, then prints the banner
 *        and help text to stdout.
 *
 * The library and rendering singletons are scoped to this call -- each
 * is constructed fresh for the one banner-and-help print and released
 * once it returns.
 */
static void printBannerAndHelp()
{
    printBanner();
    printf ("%s", juce::String::charToString (Chars::newline).toRawUTF8());
    printHelp (BinaryData::getString (files::castHelp));
}

static constexpr int flagArgIndex { 1 };
static constexpr int postFlagArgIndex { 2 };

int main (int argc, char* argv[])
{
#ifdef _WIN32
    std::system ("cls");
#else
    std::system ("clear");
#endif

    const auto formatFlag { Id::doubleDash + Id::format.toString() };
    const auto noFormatFlag { Id::doubleDash + Id::noFormat.toString() };
    const auto isFormatOnly {
        (argc >= 2 and juce::String::fromUTF8 (argv[flagArgIndex]) == formatFlag)
        or (argc >= 3 and juce::String::fromUTF8 (argv[postFlagArgIndex]) == formatFlag)
    };
    const auto isSkipFormat {
        (argc >= 2 and juce::String::fromUTF8 (argv[flagArgIndex]) == noFormatFlag)
        or (argc >= 3 and juce::String::fromUTF8 (argv[postFlagArgIndex]) == noFormatFlag)
    };
    const auto manifestIndex { (argc >= 2
                                and (juce::String::fromUTF8 (argv[flagArgIndex]) == formatFlag
                                     or juce::String::fromUTF8 (argv[flagArgIndex]) == noFormatFlag))
                                   ? postFlagArgIndex
                                   : flagArgIndex };

    const juce::File documentFile {
        (argc > manifestIndex) ? juce::File::getCurrentWorkingDirectory().getChildFile (
                                     juce::String::fromUTF8 (argv[manifestIndex]))
                               : juce::File::getCurrentWorkingDirectory().getChildFile (files::cast)
    };

    if (argc == 2
        and juce::String::fromUTF8 (argv[flagArgIndex]) == Id::doubleDash + Id::version.toString())
    {
        const auto versionLine { ProjectInfo::projectName
                                 + juce::String::charToString (Chars::space)
                                 + ProjectInfo::versionString
                                 + juce::String::charToString (Chars::space)
                                 + juce::String::charToString (Chars::openParen) + CAST_COMMIT
                                 + juce::String::charToString (Chars::closeParen) };
        printf ("%s\n", versionLine.toRawUTF8());
        return 0;
    }

    if (argc == 2
        and juce::String::fromUTF8 (argv[flagArgIndex]) == Id::doubleDash + Id::help.toString())
    {
        printBannerAndHelp();
        return 0;
    }

    if (documentFile.existsAsFile())
    {
        Processor processor { documentFile };
        auto result { juce::Result::ok() };

        if (not isSkipFormat)
            result = processor.format();

        if (result.wasOk() and not isFormatOnly)
            result = processor.generate (
                (argc == 3 and manifestIndex == flagArgIndex and not isSkipFormat)
                    ? juce::String::fromUTF8 (argv[postFlagArgIndex])
                    : juce::String {});

        if (result.wasOk())
            return 0;

        const auto errorLine { ProjectInfo::projectName + Id::diagnosticSeparator
                               + result.getErrorMessage() };
        fprintf (stderr, "%s\n", errorLine.toRawUTF8());
        return 1;
    }

    printBannerAndHelp();
    const auto notFoundLine { ProjectInfo::projectName + Id::diagnosticSeparator
                              + text::Diagnostics::failNotFound + Id::diagnosticSeparator
                              + documentFile.getFileName() };
    fprintf (stderr, "%s\n", notFoundLine.toRawUTF8());
    return 1;
}
