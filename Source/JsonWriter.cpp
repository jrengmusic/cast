#include "JsonWriter.h"

juce::String JsonWriter::getText (const Model& model, const juce::String& origin, const juce::String& file) const
{
    auto text { juce::String::charToString (Chars::openBrace) + getString (Id::file.toString()) + Chars::colon
                + getString (file) + Chars::comma + getString (Id::tables.toString()) + Chars::colon
                + Chars::openBracket };
    const Model::Element* heading { nullptr };

    for (auto* block : *model.getRoot())
        if (block->get<juce::String> (Id::path)->compare (origin) == 0)
        {
            if (*block->get<int> (Id::type) == map::BlockType::heading)
                heading = block;

            if (Model::isTable (*block))
            {
                if (not text.endsWithChar (Chars::openBracket))
                    text += Chars::comma;

                text += getTable (model, *block, heading);
            }
        }

    return text + Chars::closeBracket + Chars::closeBrace;
}

juce::String JsonWriter::getString (const juce::String& text)
{
    static const auto quoteText { juce::String::charToString (Chars::doubleQuote) };

    return quoteText + juce::JSON::escapeString (text) + quoteText;
}

juce::String JsonWriter::getTable (const Model& model, const Model::Element& table, const Model::Element* heading)
{
    const auto* headerRow { Model::getTableHeaderRow (table) };
    auto text { juce::String::charToString (Chars::openBrace) + getString (Id::name.toString()) + Chars::colon
                + getString (heading != nullptr ? *heading->get<juce::String> (Id::id) : juce::String {})
                + Chars::comma + getString (Id::columns.toString()) + Chars::colon + Chars::openBracket };

    for (auto* headerCell : *headerRow)
    {
        if (not text.endsWithChar (Chars::openBracket))
            text += Chars::comma;

        text += getString (headerCell->getAllSubText());
    }

    text += juce::String::charToString (Chars::closeBracket) + Chars::comma + getString (Id::rows.toString())
            + Chars::colon + Chars::openBracket;

    for (auto* row : model.getTableRows (table))
    {
        if (not text.endsWithChar (Chars::openBracket))
            text += Chars::comma;

        text += getRow (model, *headerRow, *row);
    }

    return text + Chars::closeBracket + Chars::closeBrace;
}

juce::String JsonWriter::getRow (const Model& model, const Model::Element& headerRow, const Model::Element& row)
{
    auto text { juce::String::charToString (Chars::openBrace) };

    for (auto* cell : row)
    {
        const auto* headerCell { headerRow.getChildByID (cell->id) };

        if (not text.endsWithChar (Chars::openBrace))
            text += Chars::comma;

        text += getString (headerCell->getAllSubText()) + Chars::colon
                + getString (model.getAuthoredText (*headerCell, row, *cell));
    }

    return text + Chars::closeBrace;
}
