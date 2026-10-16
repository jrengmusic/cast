#include "Help.h"

void printHelp (const juce::String& helpText)
{
    jam::SharedInstance<jam::SharedDocuments> documents { std::in_place };
    juce::ValueTree config { Id::toType (Id::config) };

    for (const auto& styleSheet : { files::markdownStyleSheet, files::mermaidStyleSheet })
        jam::Css::getOrCreate (juce::Identifier { styleSheet }).applyTo (config, juce::Identifier { juce::File::createFileWithoutCheckingPath (styleSheet).getFileNameWithoutExtension() });

    jam::terminal::GraphicsEngine engine { *stdout };
    jam::StyleManager styleManager { { BinaryData::fetcher }, config };
    jam::StyleMarkdown styleMarkdown;

    const auto lineHeight { styleMarkdown.getMetrics<float> (map::MarkdownStyleSheet::lineHeight) };
    styleMarkdown.setCellSize (static_cast<float> (jam::AttributedChar::narrowColumns), lineHeight);

    const auto winsize { engine.getWinsize() };
    const auto maxLineWidth { styleMarkdown.getMetrics<float> (map::MarkdownStyleSheet::maxLineWidth) };
    const auto margin { styleMarkdown.getMetrics<float> (map::MarkdownStyleSheet::margin) };
    const auto columnWidth { juce::BorderSize<float> { 0.0f, margin, 0.0f, margin }.addedTo (juce::Rectangle<float> { maxLineWidth, 0.0f }).getSmallestIntegerContainer().getWidth() };
    const auto width { winsize.isValid() ? winsize.cols : columnWidth };
    const auto verticalScale { 1.0f / lineHeight };

    jam::MarkdownComponent component;
    component.setLookAndFeel (&styleMarkdown);
    component.setBounds (0, 0, width, 0);
    component.setDocument (jam::MarkdownDocument::parse (helpText, files::castHelp));

    const auto rows { juce::Rectangle<int> { width, component.getViewedComponent()->getHeight() }.toFloat().transformedBy (juce::AffineTransform::scale (1.0f, verticalScale)).getSmallestIntegerContainer().getHeight() };

    {
        engine.resize (width, rows, verticalScale);

        jam::terminal::GraphicsContext context { engine };
        juce::Graphics g { context };
        component.getViewedComponent()->paintEntireComponent (g, false);
    }
}
