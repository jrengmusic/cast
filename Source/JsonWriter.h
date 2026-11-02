#pragma once
#include <JuceHeader.h>
#include "Model.h"

/**
 * @struct JsonWriter
 * @brief Encodes the tables of one origin file as JSON text, the output of
 *        the tables request.
 *
 * JsonWriter keeps no state. It walks the root blocks of a Model, and reads
 * each cell through Model::getAuthoredText(), so the JSON carries the
 * authored text and no format transform or escape.
 */
struct JsonWriter
{
    /**
     * @brief Encodes every table @p origin declares in @p model as one JSON
     *        object, with @p file as its file member.
     *
     * Each table takes the heading text of the most recent heading block of
     * @p origin that precedes it, or an empty string when no heading precedes
     * it.
     * Tables that another file declares are not encoded.
     *
     * @param model  The parsed model whose root blocks are walked.
     * @param origin The file whose tables are encoded, compared against each
     *               block's own path.
     * @param file   The file name written into the object's file member.
     * @returns A JSON object with a file member and a tables member holding
     *          one object per table.
     */
    juce::String getText (const Model& model, const juce::String& origin, const juce::String& file) const;

private:
    /**
     * @brief Encodes @p text as a JSON string literal.
     *
     * @param text The text to encode.
     * @returns @p text in double quotes, with the escapes that
     *          juce::JSON::escapeString() applies.
     */
    static juce::String getString (const juce::String& text);

    /**
     * @brief Encodes @p table as one JSON object: its name, its columns, and
     *        one object per row.
     *
     * @param model   The model the rows are read through.
     * @param table   The table encoded. Its header row names the columns.
     * @param heading The most recent heading block that precedes @p table,
     *                or @c nullptr when none precedes it.
     * @returns A JSON object with name, columns, and rows members.
     */
    static juce::String getTable (const Model& model, const Model::Element& table, const Model::Element* heading);

    /**
     * @brief Encodes @p row as one JSON object, keyed by each header cell's
     *        text and valued by that cell's authored text.
     *
     * @param model     The model the authored text is read through.
     * @param headerRow The header row of @p row's table.
     * @param row       The row encoded.
     * @returns A JSON object with one member for each cell of @p row.
     */
    static juce::String getRow (const Model& model, const Model::Element& headerRow, const Model::Element& row);
};
