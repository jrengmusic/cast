#pragma once
#include <JuceHeader.h>

/**
 * @brief Renders the HELP.md text to stdout through the terminal graphics
 *        context.
 *
 * Loads the markdown and mermaid stylesheets into one config tree, parses
 * @p helpText as markdown, and lays it out once at a width of @p lineWrap
 * cells. One logical unit is one cell, because the markdown sheet declares no
 * font size. The content height of the laid-out component is the row count.
 * It paints the component to stdout through the terminal graphics context.
 *
 * @pre The JUCE GUI initialiser scope is open.
 *
 * @param helpText The HELP.md text, parsed as markdown.
 * @param lineWrap The width of the rendered text, in cells.
 */
static inline void printHelp (const juce::String& helpText, int lineWrap)
{
    jam::SharedInstance<jam::SharedDocuments> documents { std::in_place };
    juce::ValueTree config { Id::toType (Id::config) };

    for (const auto& styleSheet : { files::markdownStyleSheet, files::mermaidStyleSheet })
        for (const auto& child : jam::Css::getOrCreate (juce::Identifier { styleSheet }).getValueTree (Id::toType (Id::config), juce::Identifier { juce::File::createFileWithoutCheckingPath (styleSheet).getFileNameWithoutExtension() }))
            config.appendChild (child.createCopy(), nullptr);

    jam::terminal::GraphicsEngine engine { stdout };
    jam::StyleManager styleManager { { BinaryData::fetcher }, config };
    jam::StyleMarkdown styleMarkdown;
    juce::LookAndFeel::setDefaultLookAndFeel (&styleMarkdown);

    jam::MarkdownComponent component;
    component.setBounds (0, 0, lineWrap, 0);
    component.setDocument (jam::MarkdownDocument::parse (helpText, files::castHelp));

    const auto rows { component.getViewedComponent()->getHeight() };

    {
        engine.resize (lineWrap, rows);

        jam::terminal::GraphicsContext context { engine };
        juce::Graphics g { context };
        component.getViewedComponent()->paintEntireComponent (g, false);
    }

    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
}
