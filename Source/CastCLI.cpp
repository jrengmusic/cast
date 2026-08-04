/**
 * @file CastCLI.cpp
 * @brief `cast` CLI entry point: banner/help rendering and manifest dispatch.
 */

#include <JuceHeader.h>
#include <jam_tui/jam_tui.h>
#include "generated/CAST.h"
#include "Driver.h"
#include "Help.h"

/**
 * @brief Paints the generated glyph banner into a TUI graphics context.
 *
 * Each banner row is colour-keyed by its own name and the prior row's name,
 * used to blend the solid (`█`) and shaded (`░`) glyph colours between rows.
 *
 * @param g The TUI graphics context to paint into.
 */
static void paintBanner (jam::tui::Graphics& g)
{
    const auto& [firstName, firstText] { *cast::banner.begin() };
    auto priorName { firstName };
    int row { 0 };

    for (const auto& [name, text] : cast::banner)
    {
        const juce::Colour rowColour { jam::ColourNames::colours[jam::ColourNames::get (name)] };
        const juce::Colour priorColour { jam::ColourNames::colours[jam::ColourNames::get (priorName)] };

        const jam::HashMap<juce::juce_wchar, juce::Colour> glyphColours
        {
            { U'█', rowColour },
            { U'░', priorColour }
        };

        for (int col { 0 }; col < text.length(); ++col)
        {
            const auto glyph { text[col] };

            if (glyphColours.contains (glyph))
            {
                g.setColour (glyphColours.at (glyph));
                g.drawCellText (juce::String::charToString (glyph), col, row, 1);
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
    jam::tui::Graphics graphics { firstText.length(), static_cast<int> (cast::banner.size()) };
    paintBanner (graphics);

    printf ("%s\n", graphics.getLines().joinIntoString ("\n").toRawUTF8());
}

static void printBannerAndHelp()
{
    printBanner();
    printf ("\n");
    cast::printHelp (BinaryData::getString (Id::specification.toString()));
}

/**
 * @brief Derives the CMake configure-dependency file list from a manifest (SPEC §4).
 *
 * The manifest itself, every `## outputs` row's template and input tables,
 * and every `## dispatch` row's fragment template — never hand-maintained.
 *
 * @param manifestFile The `CAST.md` manifest to derive dependencies from.
 * @return The manifest-derived dependency files.
 */
static jam::Array<juce::File> getConfigureDepends (const juce::File& manifestFile)
{
    jam::Array<juce::File> depends;
    depends.add (manifestFile);

    const auto dir { manifestFile.getParentDirectory() };

    const auto bannerFile { dir.getChildFile (cast::outputBannerFileName) };

    if (bannerFile.existsAsFile())
        depends.add (bannerFile);

    const auto manifestDoc { jam::Markdown::parse (manifestFile.loadFileAsString()) };

    for (const auto& outputKey : manifestDoc.getTableRowKeys (Id::outputs))
    {
        depends.add (dir.getChildFile (manifestDoc.getTableValue (Id::outputs, Id::templatePath, outputKey)));

        for (const auto& tablePath : manifestDoc.getTableValues (Id::outputs, Id::tables, outputKey))
            depends.add (dir.getChildFile (tablePath.trim()));
    }

    for (const auto& dispatchKey : manifestDoc.getTableRowKeys (Id::dispatch))
        depends.add (dir.getChildFile (manifestDoc.getTableValue (Id::dispatch, Id::templatePath, dispatchKey)));

    return depends;
}

/**
 * @brief Runs cast::Driver::run() and reports the result on stdout/stderr.
 *
 * On success, prints the manifest's configure-dependency list
 * (getConfigureDepends()) to stdout, one path per line, for CMake to consume.
 *
 * @param manifest     The `CAST.md` manifest to run.
 * @param outputFilter When non-empty, restricts regeneration to the named output row.
 * @return 0 on success; 1 on failure, after printing the SPEC §8 failure to stderr.
 */
static int runManifest (const juce::File& manifest, const juce::String& outputFilter = {})
{
    const auto result { cast::Driver::run (manifest, outputFilter) };

    if (result.wasOk())
    {
        for (const auto& path : getConfigureDepends (manifest))
            printf ("%s\n", path.getFullPathName().toRawUTF8());

        return 0;
    }

    fprintf (stderr, "cast: %s\n", result.getErrorMessage().toRawUTF8());
    return 1;
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;

    jam::Stamp stamp;
    jam::Hyperlink hyperlink;
    jam::ColourNames colourNames;

    jam::Stamp::getInstance()->addIfNotAlreadyThere (jam::Stamp::Entry {});

    if (argc == 2 and juce::String { argv[1] } == "--" + Id::version.toString())
    {
        printf ("cast %s (" CAST_COMMIT ")\n", ProjectInfo::versionString);
        return 0;
    }

    if (argc == 2 and juce::String { argv[1] } == "--" + Id::help.toString())
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

    const juce::File manifestFile
    {
        (argc == 2)
            ? juce::File::getCurrentWorkingDirectory().getChildFile (juce::String { argv[1] })
            : juce::File::getCurrentWorkingDirectory().getChildFile (Id::cast.toString())
    };

    if (manifestFile.existsAsFile())
        return runManifest (manifestFile);

    printBannerAndHelp();
    fprintf (stderr, "cast: no %s found\n", Id::cast.toString().toRawUTF8());
    return 1;
}
