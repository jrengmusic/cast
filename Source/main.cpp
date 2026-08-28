/**
 * @file main.cpp
 * @brief `cast` CLI entry point: banner/help rendering and manifest dispatch.
 */

#include <JuceHeader.h>
#include "Processor.h"
#include "Help.h"

/**
 * @brief Paints the generated glyph banner into a terminal graphics context.
 *
 * Each banner row is colour-keyed by its own name and the prior row's name,
 * used to blend the solid (`█`) and shaded (`░`) glyph colours between rows.
 *
 * @param context The terminal graphics context to paint into.
 */
static void paintBanner (jam::terminal::GraphicsContext& context)
{
    const auto& [firstName, firstText] { *map::banner.begin() };
    juce::ignoreUnused (firstText);
    auto priorName { firstName };
    int row { 0 };

    for (const auto& [name, text] : map::banner)
    {
        const juce::Colour rowColour { map::ColourNames::getInstance()->get (name) };
        const juce::Colour priorColour { map::ColourNames::getInstance()->get (priorName) };

        auto* stamp { jam::Stamp::getInstance() };

        const jam::HashMap<juce::juce_wchar, uint16_t> glyphStyles {
            { U'█',
             static_cast<uint16_t> (
                  stamp->addIfNotAlreadyThere (jam::Stamp::Entry { rowColour, {}, {}, 0 }))   },
            { U'░',
             static_cast<uint16_t> (
                  stamp->addIfNotAlreadyThere (jam::Stamp::Entry { priorColour, {}, {}, 0 })) }
        };

        for (int col { 0 }; col < text.length(); ++col)
        {
            const auto glyph { text[col] };

            if (glyphStyles.contains (glyph))
            {
                const jam::AttributedChar cell { jam::AttributedChar::make (
                    static_cast<uint32_t> (glyph),
                    jam::AttributedChar::contentCodepoint,
                    jam::AttributedChar::narrow,
                    glyphStyles.at (glyph)) };

                context.drawCells (jam::Cell { col }, jam::Cell { row }, { &cell, 1 });
            }
        }

        priorName = name;
        ++row;
    }
}

/// @brief Renders the generated banner offscreen and prints it to stdout.
static void printBanner()
{
    const auto& [firstName, firstText] { *map::banner.begin() };
    juce::ignoreUnused (firstName);
    const auto cols { firstText.length() };
    const auto rows { static_cast<int> (map::banner.size()) };

    jam::terminal::GraphicsEngine engine { stdout };
    engine.resize (widthCap, rows);

    const juce::Rectangle<int> pageRect { 0, 0, widthCap, rows };
    const juce::Rectangle<int> bannerRect { 0, 0, cols, rows };
    const auto placed {
        juce::Justification (juce::Justification::centred).appliedToRectangle (bannerRect, pageRect)
    };

    {
        jam::terminal::GraphicsContext context { engine };
        context.setOrigin ({ placed.getX(), 0 });
        paintBanner (context);
    }
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
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;
    jam::Stamp stamp;
    jam::Hyperlink hyperlink;
    jam::Grapheme grapheme;
    Generated generated;

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

// #if JUCE_DEBUG
//     const jam::debug::Log::Scope logScope { jam::File::getDebugLog() };
// #endif

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
