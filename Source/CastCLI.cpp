/**
 * @file CastCLI.cpp
 * @brief `cast` CLI entry point: banner/help rendering and manifest dispatch.
 */

#include <JuceHeader.h>
#include "generated/CAST.h"
#include "Generator.h"
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
    const auto& [firstName, firstText] { *cast::banner.begin() };
    auto priorName { firstName };
    int row { 0 };

    for (const auto& [name, text] : cast::banner)
    {
        const juce::Colour rowColour { map::ColourNames::colours[map::ColourNames::get (name)] };
        const juce::Colour priorColour {
            map::ColourNames::colours[map::ColourNames::get (priorName)]
        };

        auto* stamp { jam::Stamp::getInstance() };

        const jam::HashMap<juce::juce_wchar, uint16_t> glyphStyles {
            { U'█', static_cast<uint16_t> (stamp->addIfNotAlreadyThere (jam::Stamp::Entry { rowColour, {}, {}, 0 }))   },
            { U'░', static_cast<uint16_t> (stamp->addIfNotAlreadyThere (jam::Stamp::Entry { priorColour, {}, {}, 0 })) }
        };

        for (int col { 0 }; col < text.length(); ++col)
        {
            const auto glyph { text[col] };

            if (glyphStyles.contains (glyph))
            {
                const jam::AttributedChar cell { jam::AttributedChar::make (
                    static_cast<uint32_t> (glyph), jam::AttributedChar::contentCodepoint,
                    jam::AttributedChar::narrow, glyphStyles.at (glyph)) };

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
    const auto& [firstName, firstText] { *cast::banner.begin() };
    const auto cols { firstText.length() };
    const auto rows { static_cast<int> (cast::banner.size()) };

    jam::terminal::GraphicsEngine engine { stdout };
    engine.resize (cast::widthCap, rows);

    const juce::Rectangle<int> pageRect { 0, 0, cast::widthCap, rows };
    const juce::Rectangle<int> bannerRect { 0, 0, cols, rows };
    const auto placed { juce::Justification (juce::Justification::centred)
                             .appliedToRectangle (bannerRect, pageRect) };

    {
        jam::terminal::GraphicsContext context { engine };
        context.setOrigin ({ placed.getX(), 0 });
        paintBanner (context);
    }
}

static void printBannerAndHelp()
{
    printBanner();
    printf ("%s", Id::charNewline.toRawUTF8());
    cast::printHelp (BinaryData::getString (Id::castHelp.toString()));
}

/**
 * @brief Runs cast::Driver::run() and reports the result on stdout/stderr.
 *
 * On success, renders the manifest itself through cast::printHelp(), so the
 * run reports the relations, dispatch, transforms, and constraints it was
 * driven by.
 *
 * @param manifest     The `CAST.md` manifest to run.
 * @param outputFilter When non-empty, restricts regeneration to the named output row.
 * @return 0 on success; 1 on failure, after printing the SPEC §8 failure to stderr.
 */
static int runManifest (const juce::File& manifest, const juce::String& outputFilter = {})
{
#if JUCE_DEBUG
    const jam::debug::Log::Scope logScope { jam::File::getDebugLog() };
#endif
    const auto result { cast::Driver::run (manifest, outputFilter) };

    if (result.wasOk())
    {
        cast::printHelp (manifest.loadFileAsString());

        return 0;
    }

    const auto errorLine { Id::programName + Id::diagnosticSeparator + result.getErrorMessage() };
    fprintf (stderr, "%s\n", errorLine.toRawUTF8());
    return 1;
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;

    jam::Stamp stamp;
    jam::Hyperlink hyperlink;
    map::ColourNames colourNames;
    jam::Grapheme grapheme;
    Id::Instances instances;

    jam::Stamp::getInstance()->addIfNotAlreadyThere (jam::Stamp::Entry {});

    if (argc == 2 and juce::String { argv[1] } == Id::cliPrefix + Id::version.toString())
    {
        const auto versionLine { Id::programName + Id::charSpace + ProjectInfo::versionString
                                 + Id::charSpace + Id::charOpenParen + CAST_COMMIT
                                 + Id::charCloseParen };
        printf ("%s\n", versionLine.toRawUTF8());
        return 0;
    }

    if (argc == 2 and juce::String { argv[1] } == Id::cliPrefix + Id::castHelp.toString())
    {
        printBannerAndHelp();
        return 0;
    }

    if (argc == 3)
    {
        return runManifest (
            juce::File::getCurrentWorkingDirectory().getChildFile (juce::String { argv[1] }),
            juce::String { argv[2] });
    }

    const juce::File manifestFile {
        (argc == 2)
            ? juce::File::getCurrentWorkingDirectory().getChildFile (juce::String { argv[1] })
            : juce::File::getCurrentWorkingDirectory().getChildFile (Id::cast.toString())
    };

    if (manifestFile.existsAsFile())
        return runManifest (manifestFile);

    printBannerAndHelp();
    const auto noManifestLine { Id::programName + Id::diagnosticSeparator + Id::failNotFound
                                + Id::diagnosticSeparator + Id::cast.toString() };
    fprintf (stderr, "%s\n", noManifestLine.toRawUTF8());
    return 1;
}
