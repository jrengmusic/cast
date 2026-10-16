#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @brief Renders the HELP.md text to stdout through the terminal graphics
 *        context.
 *
 * Loads the markdown and mermaid stylesheets into one config tree and parses
 * @p helpText as markdown. The width, in cells, is the column count of the
 * terminal device when the device is known. Otherwise it is the smallest whole
 * number of cells that holds `maxLineWidth` plus the margin on both sides.
 * The function tells the style the cell size: one narrow column wide and one
 * `lineHeight` tall. The vertical scale of the engine is 1 / `lineHeight`.
 * The component is laid out once at the width. The row count is the content
 * height of the component times the vertical scale, rounded up to a whole
 * row. The function paints the component to stdout through the terminal
 * graphics context.
 *
 * @pre The JUCE GUI initialiser scope is open.
 *
 * @param helpText The HELP.md text, parsed as markdown.
 */
void printHelp (const juce::String& helpText);
