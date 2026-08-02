#include <JuceHeader.h>
#include <jam_tui/jam_tui.h>
#include "Driver.h"
#include "Help.h"

static const juce::StringArray bannerText
{
    juce::String (juce::CharPointer_UTF8 ("████████████  ████████████  ████████████  ████████████")),
    juce::String (juce::CharPointer_UTF8 ("████░░░░████  ████░░░░████  ████░░░░████  ░░░░████░░░░")),
    juce::String (juce::CharPointer_UTF8 ("████    ░░░░  ████    ████  ████    ░░░░      ████    ")),
    juce::String (juce::CharPointer_UTF8 ("████          ████████████  ████████████      ████    ")),
    juce::String (juce::CharPointer_UTF8 ("████          ████░░░░████  ░░░░░░░░████      ████    ")),
    juce::String (juce::CharPointer_UTF8 ("████    ████  ████    ████  ████    ████      ████    ")),
    juce::String (juce::CharPointer_UTF8 ("████████████  ████    ████  ████████████      ████    ")),
    juce::String (juce::CharPointer_UTF8 ("░░░░░░░░░░░░  ░░░░    ░░░░  ░░░░░░░░░░░░      ░░░░    "))
};

static const std::array<juce::Colour, 8> bannerColours
{
    juce::Colour { 102, 187, 255 },
    juce::Colour { 87, 183, 246 },
    juce::Colour { 73, 180, 238 },
    juce::Colour { 59, 177, 230 },
    juce::Colour { 45, 174, 221 },
    juce::Colour { 31, 171, 213 },
    juce::Colour { 17, 168, 205 },
    juce::Colour { 17, 168, 205 }
};

static void paintBanner (jam::tui::Graphics& g)
{
    for (int row { 0 }; row < bannerText.size(); ++row)
    {
        const auto& line { bannerText[row] };

        for (int col { 0 }; col < line.length(); ++col)
        {
            const auto glyph { line[col] };

            if (glyph == U'█')
            {
                g.setColour (bannerColours.at (static_cast<size_t> (row)));
                g.drawCellText (juce::String::charToString (glyph), col, row, 1);
            }
            else if (glyph == U'░')
            {
                const auto priorRow { row == 0 ? 0 : row - 1 };
                g.setColour (bannerColours.at (static_cast<size_t> (priorRow)));
                g.drawCellText (juce::String::charToString (glyph), col, row, 1);
            }
        }
    }
}

static void printBanner()
{
    jam::tui::Graphics graphics { bannerText[0].length(), bannerText.size() };
    paintBanner (graphics);

    printf ("%s\n", graphics.getLines().joinIntoString ("\n").toRawUTF8());
}

static int runManifest (const juce::File& manifest, const juce::String& outputFilter = {})
{
    cast::Driver driver;
    const auto result { driver.run (manifest, outputFilter) };

    if (result.wasOk())
        return 0;

    fprintf (stderr, "cast: %s\n", result.getErrorMessage().toRawUTF8());
    return 1;
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;

    jam::Stamp stamp;
    jam::Hyperlink hyperlink;
    Id::ColourIdMap colourIdMap;

    jam::Stamp::getInstance()->addIfNotAlreadyThere (jam::Stamp::Entry {});

    if (argc == 2 and juce::String { argv[1] } == "--version")
    {
        printf ("cast %s (" CAST_COMMIT ")\n", ProjectInfo::versionString);
        return 0;
    }

    if (argc == 2 and juce::String { argv[1] } == "--help")
    {
        printBanner();
        printf ("\n");
        cast::printHelp (BinaryData::getString (juce::String { "SPEC.md" }));
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
            : juce::File::getCurrentWorkingDirectory().getChildFile ("CAST.md")
    };

    if (manifestFile.existsAsFile())
        return runManifest (manifestFile);

    printBanner();
    printf ("\n");
    cast::printHelp (BinaryData::getString (juce::String { "SPEC.md" }));
    fprintf (stderr, "cast: no CAST.md found\n");
    return 1;
}
