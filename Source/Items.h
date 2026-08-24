#pragma once
#include <JuceHeader.h>
#include "Model.h"
#include "TemplateDocument.h"

struct Items
{
    using Element = Model::Element;

    static jam::Strings getColumnSourceValues (const Model& model,
                                                Element& row,
                                                const jam::Array<Element*>& tables,
                                                const juce::Identifier& source)
    {
        const auto currentFile { jam::Format::toFileName (model.getValue (row, Id::file)) };
        jam::Strings sourceValues;

        for (auto* table : tables)
            if (model.isOutputTable (*table))
                for (auto* candidate : model.getTableRows (*table))
                    if (auto* cell { model.getTableCell (*candidate, source) })
                    {
                        const auto value { jam::Format::toFileName (
                            *cell->get<juce::String> (Id::value)) };

                        if (value.isNotEmpty() and value != currentFile
                            and not sourceValues.contains (value, false))
                            sourceValues.add (value);
                    }

        return sourceValues;
    }

    static jam::Array<Element*> getBindingSourceRows (const Model& model,
                                                       const jam::Array<Element*>& tables,
                                                       const juce::Identifier& source)
    {
        jam::Array<Element*> sourceRows;

        for (auto* table : tables)
            if (model.isOutputTable (*table))
                for (auto* candidate : model.getTableRows (*table))
                {
                    auto matches { false };

                    for (auto* scope { model.getTableCell (*candidate, Id::structure) };
                         scope != nullptr and not matches;
                         scope = model.getBlockquote (*scope))
                        if (auto* list { model.getList (*scope) })
                            if (model.getListItem (*list, source) != nullptr)
                                matches = true;

                    if (matches)
                        sourceRows.add (candidate);
                }

        return sourceRows;
    }

    static juce::String
    getSourceValue (const Model& model, Element& sourceRow, const juce::Identifier& name)
    {
        juce::String deepestValue;

        for (auto* scope { model.getTableCell (sourceRow, Id::structure) };
             scope != nullptr;
             scope = model.getBlockquote (*scope))
            if (auto* list { model.getList (*scope) })
                if (auto* item { model.getListItem (*list, name) })
                    deepestValue = *item->get<juce::String> (Id::value);

        if (deepestValue.isNotEmpty())
            return deepestValue;

        juce::String columnValue;

        if (auto* cell { model.getTableCell (sourceRow, name) })
            columnValue = *cell->get<juce::String> (Id::value);

        return columnValue;
    }

    static juce::String getItem (const Model& model,
                                 const TemplateDocument& templateDocument,
                                 Element* sourceRow,
                                 const juce::String& sourceValue,
                                 const juce::Identifier& sourceKey,
                                 const juce::Identifier& shapeId)
    {
        const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };
        auto itemText { *templateDocument.getCodeBlock (shapeId)->get<juce::String> (Id::value) };

        if (sourceRow != nullptr)
        {
            for (const auto& name : tokens)
                if (jam::Format::hasPlaceholder (itemText, name.toString()))
                    itemText = jam::Format::replaceholder (
                        itemText, name.toString(), getSourceValue (model, *sourceRow, name));
        }
        else
            for (const auto& name : tokens)
                if (jam::Format::hasPlaceholder (itemText, name.toString()))
                    itemText = jam::Format::replaceholder (
                        itemText, name.toString(), name == sourceKey ? sourceValue : juce::String());
        return itemText;
    }

    static jam::Strings getItemTexts (const Model& model,
                                      const TemplateDocument& templateDocument,
                                      const jam::Array<Element*>& sourceRows,
                                      const jam::Strings& sourceValues,
                                      const juce::Identifier& sourceKey,
                                      const juce::Identifier& shapeId)
    {
        jam::Strings texts;

        for (auto* sourceRow : sourceRows)
        {
            const auto itemText { getItem (model, templateDocument, sourceRow, {}, sourceKey, shapeId) };

            if (itemText.isNotEmpty())
                texts.add (itemText);
        }

        for (const auto& value : sourceValues)
        {
            const auto itemText { getItem (model, templateDocument, nullptr, value, sourceKey, shapeId) };

            if (itemText.isNotEmpty())
                texts.add (itemText);
        }

        return texts;
    }

    static jam::Strings getItems (const Model& model,
                                  const TemplateDocument& templateDocument,
                                  Element& row,
                                  const juce::String& source,
                                  const juce::Identifier& token,
                                  const juce::Identifier& shapeId)
    {
        jam::Array<Element*> sourceRows;
        jam::Strings sourceValues;
        juce::Identifier sourceKey { token };

        if (source.startsWithChar (Chars::at))
        {
            sourceRows = model.getTableRows (*model.getTables (row, source));
        }
        else
        {
            const auto sourceName { juce::Identifier (source) };
            const auto tables { model.getTables() };
            const auto hasColumnSource { std::any_of (
                tables.begin(),
                tables.end(),
                [&model, &sourceName] (Element* table)
                {
                    auto* headerRow { Model::getTableHeaderRow (*table) };
                    return model.isOutputTable (*table)
                           and model.getTableCell (*headerRow, sourceName) != nullptr;
                }) };

            if (hasColumnSource)
            {
                sourceKey = sourceName;
                sourceValues = getColumnSourceValues (model, row, tables, sourceName);
            }
            else
                sourceRows = getBindingSourceRows (model, tables, sourceName);
        }

        return getItemTexts (model, templateDocument, sourceRows, sourceValues, sourceKey, shapeId);
    }

    static juce::String getJoinedItems (const Model& model,
                                        const TemplateDocument& templateDocument,
                                        Element& row,
                                        const juce::Identifier& token,
                                        const juce::String& source,
                                        const juce::Identifier& shapeId,
                                        Element* separatorScope,
                                        const juce::String& indent)
    {
        const auto texts { getItems (model, templateDocument, row, source, token, shapeId) };
        const auto separatorValue { templateDocument.getSeparator (model, row, separatorScope, token) };
        const auto separator { separatorValue.isNotEmpty()
                                   ? separatorValue
                                   : juce::String::charToString (Chars::newline) };
        const auto joined { texts.joinIntoString (separator, 0, -1) };
        return joined.isEmpty()
                   ? joined
                   : indent
                         + joined.replace (juce::String::charToString (Chars::newline),
                                           juce::String::charToString (Chars::newline) + indent);
    }
};
