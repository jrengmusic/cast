#include "JsonWriter.h"

juce::String JsonWriter::getText (const Model& model, const juce::String& origin, const juce::String& file) const
{
    std::string json;
    const Model::Element* heading { nullptr };

    jam::JsonWriter::appendObject (json, [&model, &origin, &file, &heading] (std::string& object)
    {
        jam::JsonWriter::appendKey (object, Id::file);
        jam::JsonWriter::appendString (object, jam::Strings::getView (file));
        jam::JsonWriter::appendKey (object, Id::tables);

        jam::JsonWriter::appendArray (object, [&model, &origin, &heading] (std::string& tables)
        {
            for (auto* block : *model.getRoot())
                if (block->get<juce::String> (Id::path)->compare (origin) == 0)
                {
                    if (*block->get<int> (Id::type) == map::BlockType::heading)
                        heading = block;

                    if (Model::isTable (*block))
                    {
                        jam::JsonWriter::appendSeparator (tables);
                        appendTable (tables, model, *block, heading);
                    }
                }
        });
    });

    return jam::Strings::fromView (json);
}

void JsonWriter::appendTable (std::string& json, const Model& model, const Model::Element& table, const Model::Element* heading)
{
    const auto* headerRow { Model::getTableHeaderRow (table) };

    jam::JsonWriter::appendObject (json, [&model, &table, heading, headerRow] (std::string& object)
    {
        jam::JsonWriter::appendKey (object, Id::name);
        jam::JsonWriter::appendString (object, jam::Strings::getView (heading != nullptr ? *heading->get<juce::String> (Id::id) : juce::String {}));
        jam::JsonWriter::appendKey (object, Id::columns);

        jam::JsonWriter::appendArray (object, [headerRow] (std::string& columns)
        {
            for (auto* headerCell : *headerRow)
            {
                jam::JsonWriter::appendSeparator (columns);
                jam::JsonWriter::appendString (columns, jam::Strings::getView (headerCell->getAllSubText()));
            }
        });

        jam::JsonWriter::appendKey (object, Id::rows);

        jam::JsonWriter::appendArray (object, [&model, &table, headerRow] (std::string& rows)
        {
            for (auto* row : model.getTableRows (table))
            {
                jam::JsonWriter::appendSeparator (rows);
                appendRow (rows, model, *headerRow, *row);
            }
        });
    });
}

void JsonWriter::appendRow (std::string& json, const Model& model, const Model::Element& headerRow, const Model::Element& row)
{
    jam::JsonWriter::appendObject (json, [&model, &headerRow, &row] (std::string& object)
    {
        for (auto* cell : row)
        {
            const auto* headerCell { headerRow.getChildByID (cell->id) };

            jam::JsonWriter::appendKey (object, jam::Strings::getView (headerCell->getAllSubText()));
            jam::JsonWriter::appendString (object, jam::Strings::getView (model.getAuthoredText (*headerCell, row, *cell)));
        }
    });
}
