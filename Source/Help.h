#pragma once
#include <JuceHeader.h>
#include <jam_tui/jam_tui.h>

namespace cast
{

static constexpr int widthCap { 120 };
static constexpr int documentMargin { 2 };

/**
 * @brief Renders and prints the SPEC.md text as the `--help` banner body.
 *
 * Parses @p specText as markdown and renders it through the TUI markdown
 * document pipeline, word-wrapped to the terminal width (capped at
 * @c widthCap columns) with a left/right margin of @c documentMargin columns.
 *
 * @param specText The specification text (SPEC §9: printed verbatim on `--help`
 *                 and when no manifest is found).
 */
static inline void printHelp (const juce::String& specText)
{
    auto tree { jam::Markdown::parse (specText) };
    const auto terminalCols { jam::tui::Metrics::getBounds().getWidth().value };
    const auto widthCols { juce::jmin (terminalCols, widthCap) - documentMargin * 2 };

    jam::SharedInstance<jam::SharedDocuments> documents { std::in_place };
    jam::StyleManager styleManager { {} };

    jam::tui::LookAndFeel lookAndFeel;
    styleManager.setAppearance (lookAndFeel, Id::dark);

    jam::MarkdownDocument markdownDoc { std::move (tree), lookAndFeel.getMonoFont() };

    const auto ansi { jam::tui::toAnsiString (markdownDoc, lookAndFeel, widthCols) };
    const auto margin { juce::String::repeatedString (" ", documentMargin) };

    juce::StringArray lines;
    lines.addLines (ansi);

    for (auto& line : lines)
        line = margin + line;

    printf ("%s\n", lines.joinIntoString ("\n").toRawUTF8());
}

} // namespace cast
