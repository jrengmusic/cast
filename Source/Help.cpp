#include "Help.h"

void printHelp (const juce::String& helpText)
{
    jam::SharedInstance<jam::SharedDocuments> documents { std::in_place };
    jam::Typeface typeface;

    juce::MemoryOutputStream frame;
    jam::terminal::GraphicsEngine engine;
    const jam::GlyphArrangement glyphArrangement { jam::terminal::GraphicsEngine::getCellLookup(), jam::terminal::GraphicsEngine::getCellMeasure() };
    jam::StyleManager styleManager { { BinaryData::fetcher }, jam::Strings { files::markdownStyleSheet, files::mermaidStyleSheet } };
    jam::StyleMarkdown styleMarkdown;

    const auto columnWidth { styleMarkdown.getColumnWidth() };

    jam::MarkdownComponent component;
    component.setLookAndFeel (&styleMarkdown);
    component.setBounds (0, 0, columnWidth, 0);
    component.setDocument (jam::MarkdownDocument::parse (helpText, files::castHelp));

    const auto rows { component.getViewedComponent()->getHeight() };

    {
        engine.prepare (columnWidth, rows);

        jam::terminal::GraphicsContext context { engine };
        juce::Graphics g { context };
        component.getViewedComponent()->paintEntireComponent (g, false);
    }

    engine.process (frame, frame, 0);

    fwrite (frame.getData(), sizeof (char), frame.getDataSize(), stdout);
}
