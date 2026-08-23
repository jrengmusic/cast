#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Model.h"
#include "Operators.h"
struct TemplateDocument : jam::MarkdownDocument
{
    TemplateDocument() = default;

    explicit TemplateDocument (jam::MarkdownDocument&& parsed)
        : jam::MarkdownDocument (std::move (parsed))
    {
        for (auto* block : *root)
            if (block->contains (Id::type)
                and *block->get<int> (Id::type) == map::BlockType::codeBlock)
            {
                const auto blockText { block->getAllSubText() };
                block->add<juce::String> (Id::value, blockText);
                jam::Document::Identifiers names;
                auto remaining { blockText };

                while (remaining.contains (Id::tripleColon))
                {
                    remaining = jam::Format::from (remaining, Id::tripleColon, false);

                    if (not remaining.contains (Id::tripleColon))
                        break;
                    const auto interior { jam::Format::upTo (remaining, Id::tripleColon, false) };
                    names.addIfNotAlreadyThere (
                        juce::Identifier (jam::Format::getPreColon (interior)));
                    remaining = jam::Format::from (remaining, Id::tripleColon, false);
                }
                block->add<jam::Document::Identifiers> (Id::placeholder, std::move (names));
            }
    }

    static constexpr int indentWidth { 4 };
    const juce::String& getContent (const juce::String& value) const
    {
        const auto id { juce::Identifier (jam::Format::getPostColon (value).trim()) };
        return *getCodeBlock (id)->get<juce::String> (Id::value);
    }

    juce::String getBinding (const Model& model, Element& row, const juce::String& value) const
    {
        if (jam::Format::getPreColon (value).trim() == Id::templatePath.toString())
            return getContent (value);

        if (value.startsWithChar (Chars::at))
        {
            const auto symbol { model.getValue (row, value) };
            return symbol;
        }

        return value;
    }

    juce::String getSeparator (const Model& model, Element& row, Element* separatorScope,
        const juce::Identifier& jack) const
    {
        if (separatorScope != nullptr)
            if (auto* list { model.getList (*separatorScope) })
                if (auto* item { model.getListItem (*list, jack) })
                    return getBinding (model, row, *item->get<juce::String> (Id::value));
        return {};
    }

    jam::HashMap<juce::Identifier, juce::String> getReplacements (const Model& model, Element& row,
        Element& structureScope, Element* placeholderScope, Element* separatorScope,
        const jam::Document::Identifiers& candidates, const juce::String& indent) const
    {
        jam::HashMap<juce::Identifier, juce::String> replacements;
        auto* structureList { model.getList (structureScope) };
        auto* placeholderList { placeholderScope != nullptr
                                    ? model.getList (*placeholderScope)
                                    : nullptr };

        if (structureList != nullptr)
            for (auto* item : *structureList)
                if (placeholderList == nullptr
                    or model.getListItem (*placeholderList, item->id) == nullptr)
                    replacements.try_emplace (
                        item->id, getBinding (model, row, *item->get<juce::String> (Id::value)));

        if (placeholderList != nullptr)
            for (auto* item : *placeholderList)
            {
                auto* structureItem { structureList != nullptr
                                          ? model.getListItem (*structureList, item->id)
                                          : nullptr };
                const auto shapeId { juce::Identifier (
                    structureItem != nullptr
                        ? jam::Format::getPostColon (
                            *structureItem->get<juce::String> (Id::value)).trim()
                        : juce::String()) };
                replacements.try_emplace (
                    item->id,
                    getExpansion (
                        model, row, item->id, *item->get<juce::String> (Id::value),
                        shapeId, separatorScope, indent));
            }

        auto* structureBlockquote { model.getBlockquote (structureScope) };

        if (structureBlockquote != nullptr
            and model.getStructure (*structureBlockquote).isNotEmpty())
            for (const auto& name : candidates)
                if (not replacements.contains (name))
                {
                    const auto nestedIndent { indent
                        + juce::String::repeatedString (
                            juce::String::charToString (Chars::space), indentWidth) };
                    replacements.try_emplace (
                        name,
                        getShape (
                            model, row, *structureBlockquote,
                            placeholderScope != nullptr
                                ? model.getBlockquote (*placeholderScope)
                                : nullptr,
                            separatorScope != nullptr
                                ? model.getBlockquote (*separatorScope)
                                : nullptr,
                            nestedIndent));
                    break;
                }

        return replacements;
    }

    juce::String getEmittedLines (const juce::Identifier& codeId,
        const jam::Document::Identifiers& candidates,
        const jam::HashMap<juce::Identifier, juce::String>& replacements) const
    {
        jam::Strings emittedLines;
        auto previousLineElided { false };

        for (const auto& templateLine : jam::Strings::fromLines (
                 *getCodeBlock (codeId)->get<juce::String> (Id::value)))
        {
            auto lineText { templateLine };
            auto lineHasPlaceholder { false };

            for (const auto& name : candidates)
                if (jam::Format::hasPlaceholder (lineText, name.toString()))
                {
                    lineHasPlaceholder = true;
                    lineText = jam::Format::replaceholder (
                        lineText, name.toString(), replacements.get (name));
                }

            const auto lineIsElided { lineHasPlaceholder and lineText.trim().isEmpty() };

            if (lineIsElided)
                previousLineElided = true;
            else if (previousLineElided and lineText.trim().isEmpty())
                previousLineElided = false;
            else
            {
                emittedLines.add (lineText);
                previousLineElided = false;
            }
        }

        return emittedLines.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
    }

    juce::String getShape (const Model& model, Element& row, Element& structureScope,
        Element* placeholderScope, Element* separatorScope, const juce::String& indent) const
    {
        const auto codeId { juce::Identifier (model.getStructure (structureScope)) };
        const auto& candidates {
            *getCodeBlock (codeId)->get<jam::Document::Identifiers> (Id::placeholder)
        };
        const auto replacements { getReplacements (
            model, row, structureScope, placeholderScope, separatorScope, candidates, indent) };
        return getEmittedLines (codeId, candidates, replacements);
    }

    jam::Strings getColumnSourceValues (const Model& model, Element& row,
        const jam::Array<Element*>& tables, const juce::Identifier& source) const
    {
        const auto currentFile { jam::Format::toFileName (model.getValue (row, Id::file)) };
        jam::Strings seenValues;
        jam::Strings sourceValues;

        for (auto* table : tables)
            if (model.isOutputTable (*table))
                for (auto* candidate : model.getTableRows (*table))
                    if (auto* cell { model.getTableCell (*candidate, source) })
                    {
                        const auto value { jam::Format::toFileName (
                            *cell->get<juce::String> (Id::value)) };

                        if (value.isNotEmpty() and value != currentFile
                            and not seenValues.contains (value, false))
                        {
                            seenValues.add (value);
                            sourceValues.add (value);
                        }
                    }

        return sourceValues;
    }

    jam::Array<Element*> getBindingSourceRows (
        const Model& model, const jam::Array<Element*>& tables,
        const juce::Identifier& source) const
    {
        jam::Array<Element*> sourceRows;

        for (auto* table : tables)
            if (model.isOutputTable (*table))
                for (auto* candidate : model.getTableRows (*table))
                {
                    auto matches { false };

                    for (auto* scope { model.getTableCell (*candidate, Id::structure) };
                         scope != nullptr and not matches; scope = model.getBlockquote (*scope))
                        if (auto* list { model.getList (*scope) })
                            if (model.getListItem (*list, source) != nullptr)
                                matches = true;

                    if (matches)
                        sourceRows.add (candidate);
                }

        return sourceRows;
    }

    jam::Strings getItemTexts (const Model& model, const jam::Array<Element*>& sourceRows,
        const jam::Strings& sourceValues, const juce::Identifier& sourceKey,
        const juce::Identifier& shapeId) const
    {
        jam::Strings texts;

        for (auto* sourceRow : sourceRows)
        {
            const auto itemText { getItem (model, sourceRow, {}, sourceKey, shapeId) };

            if (itemText.isNotEmpty())
                texts.add (itemText);
        }

        for (const auto& value : sourceValues)
        {
            const auto itemText { getItem (model, nullptr, value, sourceKey, shapeId) };

            if (itemText.isNotEmpty())
                texts.add (itemText);
        }

        return texts;
    }

    jam::Strings getItems (const Model& model, Element& row, const juce::String& wiring,
        const juce::Identifier& jack, const juce::Identifier& shapeId) const
    {
        jam::Array<Element*> sourceRows;
        jam::Strings sourceValues;
        juce::Identifier sourceKey { jack };

        if (wiring.startsWithChar (Chars::at))
        {
            sourceRows = model.getTableRows (*model.getTables (row, wiring));
        }
        else
        {
            const auto source { juce::Identifier (wiring) };
            const auto tables { model.getTables() };
            const auto hasColumnSource { std::any_of (tables.begin(), tables.end(),
                [&model, &source] (Element* table)
                {
                    auto* headerRow { getTableHeaderRow (*table) };
                    return model.isOutputTable (*table)
                           and model.getTableCell (*headerRow, source) != nullptr;
                }) };

            if (hasColumnSource)
            {
                sourceKey = source;
                sourceValues = getColumnSourceValues (model, row, tables, source);
            }
            else
                sourceRows = getBindingSourceRows (model, tables, source);
        }

        return getItemTexts (model, sourceRows, sourceValues, sourceKey, shapeId);
    }

    juce::String getExpansion (const Model& model, Element& row, const juce::Identifier& jack,
        const juce::String& wiring, const juce::Identifier& shapeId, Element* separatorScope,
        const juce::String& indent) const
    {
        const auto texts { getItems (model, row, wiring, jack, shapeId) };
        const auto separatorValue { getSeparator (model, row, separatorScope, jack) };
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

    juce::String getItem (const Model& model, Element* sourceRow, const juce::String& sourceValue,
        const juce::Identifier& sourceKey, const juce::Identifier& shapeId) const
    {
        const auto& candidates {
            *getCodeBlock (shapeId)->get<jam::Document::Identifiers> (Id::placeholder)
        };
        auto itemText { *getCodeBlock (shapeId)->get<juce::String> (Id::value) };

        if (sourceRow != nullptr)
        {
            for (const auto& name : candidates)
                if (jam::Format::hasPlaceholder (itemText, name.toString()))
                {
                    juce::String deepestValue;

                    for (auto* scope { model.getTableCell (*sourceRow, Id::structure) };
                         scope != nullptr; scope = model.getBlockquote (*scope))
                        if (auto* list { model.getList (*scope) })
                            if (auto* item { model.getListItem (*list, name) })
                                deepestValue = *item->get<juce::String> (Id::value);
                    juce::String columnValue;

                    if (auto* cell { model.getTableCell (*sourceRow, name) })
                        columnValue = *cell->get<juce::String> (Id::value);
                    itemText = jam::Format::replaceholder (itemText, name.toString(),
                        deepestValue.isNotEmpty() ? deepestValue : columnValue);
                }
        }
        else
            for (const auto& name : candidates)
                if (jam::Format::hasPlaceholder (itemText, name.toString()))
                    itemText = jam::Format::replaceholder (itemText, name.toString(),
                        name == sourceKey ? sourceValue : juce::String());
        return itemText;
    }
};
