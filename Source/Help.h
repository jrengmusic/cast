#pragma once
#include <JuceHeader.h>

namespace cast
{
/*____________________________________________________________________________*/

static constexpr int widthCap { 120 };
static constexpr int documentMargin { 2 };

/**
 * @brief Renders and prints the SPEC.md text as the `--help` banner body.
 *
 * Parses @p specText as markdown, projects it through jam::MarkdownComponent
 * onto a headless jam::terminal::GraphicsEngine frame (word-wrapped to
 * @c widthCap columns, minus a left/right margin of @c documentMargin
 * columns), and prints the resulting cell grid to stdout.
 *
 * @param specText The specification text (SPEC §9: printed verbatim on `--help`
 *                 and when no manifest is found).
 */
static inline void printHelp (const juce::String& specText)
{
    const auto document { jam::Markdown::parse (specText) };
    const auto cols { widthCap - documentMargin * 2 };

    jam::SharedInstance<jam::SharedDocuments> documents { std::in_place };
    jam::StyleManager styleManager { {} };

    juce::LookAndFeel_V4 lookAndFeel;
    styleManager.setAppearance (lookAndFeel, Id::dark);

    jam::MarkdownComponent component { document };
    component.setLookAndFeel (&lookAndFeel);
    component.setBounds (0, 0, cols, 0);

    const auto rows { component.getTextHeight() + documentMargin };
    component.setBounds (0, 0, cols, rows);

    jam::terminal::GraphicsEngine engine { stdout };
    engine.resize (widthCap, rows);

    {
        jam::terminal::GraphicsContext context { engine };
        juce::Graphics graphics { context };
        graphics.setOrigin (documentMargin, 0);
        component.paint (graphics);
    }
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace cast
