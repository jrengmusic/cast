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
    const auto& [firstName, firstText] { *banner.begin() };
    auto priorName { firstName };
    int row { 0 };

    for (const auto& [name, text] : banner)
    {
        const juce::Colour rowColour { map::ColourNames::get (name) };
        const juce::Colour priorColour { map::ColourNames::get (priorName) };

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
    const auto& [firstName, firstText] { *banner.begin() };
    const auto cols { firstText.length() };
    const auto rows { static_cast<int> (banner.size()) };

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

static void printBannerAndHelp()
{
    printBanner();
    printf ("%s", juce::String::charToString (chars::newline).toRawUTF8());
    printHelp (BinaryData::getString (files::castHelp));
}

int main (int argc, char* argv[])
{
#if JUCE_DEBUG
    const jam::debug::Log::Scope logScope { jam::File::getDebugLog() };
#endif

    const juce::File documentFile {
        (argc >= 2) ? juce::File::getCurrentWorkingDirectory().getChildFile (
                          juce::String::fromUTF8 (argv[1]))
                    : juce::File::getCurrentWorkingDirectory().getChildFile (files::cast)
    };

    Processor processor { documentFile };

    if (argc == 2 and juce::String::fromUTF8 (argv[1]) == Id::doubleDash + Id::version.toString())
    {
        const auto versionLine { ProjectInfo::projectName
                                 + juce::String::charToString (chars::space)
                                 + ProjectInfo::versionString
                                 + juce::String::charToString (chars::space)
                                 + juce::String::charToString (chars::openParen) + CAST_COMMIT
                                 + juce::String::charToString (chars::closeParen) };
        printf ("%s\n", versionLine.toRawUTF8());
        return 0;
    }

    if (argc == 2
        and juce::String::fromUTF8 (argv[1]) == Id::doubleDash + files::castHelp)
    {
        printBannerAndHelp();
        return 0;
    }

    if (documentFile.existsAsFile())
    {
        const auto result { processor.generate ((argc == 3) ? juce::String::fromUTF8 (argv[2])
                                                       : juce::String {}) };

        if (result.wasOk())
            return 0;

        const auto errorLine { ProjectInfo::projectName + Id::diagnosticSeparator
                               + result.getErrorMessage() };
        fprintf (stderr, "%s\n", errorLine.toRawUTF8());
        return 1;
    }

    printBannerAndHelp();
    const auto notFoundLine { ProjectInfo::projectName + Id::diagnosticSeparator
                              + text::en::failNotFound + Id::diagnosticSeparator
                              + documentFile.getFileName() };
    fprintf (stderr, "%s\n", notFoundLine.toRawUTF8());
    return 1;
}
