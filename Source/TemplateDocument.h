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
        , placeholders (
              [this]
              {
                  jam::HashMap<juce::Identifier, jam::Array<juce::Identifier>> found;

                  for (auto* block : *root)
                      if (block->contains (Id::type)
                          and *block->get<int> (Id::type) == map::BlockType::codeBlock)
                      {
                          const auto blockText { block->getAllSubText() };
                          jam::Array<juce::Identifier> names;
                          auto remaining { blockText };

                          while (remaining.contains (Id::tripleColon))
                          {
                              remaining = jam::Format::from (remaining, Id::tripleColon, false);

                              if (not remaining.contains (Id::tripleColon))
                                  break;

                              const auto interior { jam::Format::upTo (remaining, Id::tripleColon, false) };
                              names.add (juce::Identifier (jam::Format::getPreColon (interior)));
                              remaining = jam::Format::from (remaining, Id::tripleColon, false);
                          }

                          found.try_emplace (block->id, std::move (names));
                      }

                  return found;
              }())
    {
    }

    static constexpr int indentWidth { 4 };
    static inline const jam::Array<juce::Identifier> empty {};

    const jam::HashMap<juce::Identifier, jam::Array<juce::Identifier>>& getPlaceholders() const
    {
        return placeholders;
    }

    juce::String getContent (const juce::String& value) const
    {
        const auto id { juce::Identifier (jam::Format::getPostColon (value).trim()) };
        auto* block { getCodeBlock (id) };

        jassert (block != nullptr);

        return block->getAllSubText();
    }

    juce::String getBinding (const Model& model,
                             Element& row,
                             int depth,
                             const juce::String& value,
                             const juce::Identifier& jack) const
    {
        if (jam::Format::getPreColon (value).trim() == Id::templatePath.toString())
            return getContent (value);

        if (value.startsWithChar (Chars::at))
        {
            const auto symbol { model.getValue (row, value) };
            return symbol.isNotEmpty() ? symbol : value;
        }

        juce::ignoreUnused (depth, jack);
        return value;
    }

    juce::String
    getSeparator (const Model& model, Element& row, int depth, const juce::Identifier& jack) const
    {
        const auto bindings { model.getSource (row, depth, Id::separator) };

        return bindings.contains (jack) ? getBinding (model, row, depth, bindings.at (jack), jack)
                                        : juce::String();
    }

    juce::String getShape (const Model& model, Element& row, int depth) const
    {
        const auto codeId { juce::Identifier (model.getStructure (row, depth)) };
        auto* block { getCodeBlock (codeId) };
        jassert (block != nullptr);

        const auto placeholderBindings { model.getSource (row, depth, Id::placeholder) };
        jam::HashMap<juce::Identifier, juce::String> replacements;

        for (auto& [key, value] : model.getSource (row, depth, Id::structure))
            if (not placeholderBindings.contains (key))
                replacements.try_emplace (key, getBinding (model, row, depth, value, key));

        for (auto& [key, value] : placeholderBindings)
        {
            juce::ignoreUnused (value);
            replacements.try_emplace (key, getExpansion (model, row, depth, key));
        }

        const auto& candidates {
            getPlaceholders().contains (codeId) ? getPlaceholders().at (codeId) : empty
        };
        const auto nestedShapeId { model.getStructure (row, depth + 1) };

        if (nestedShapeId.isNotEmpty())
            for (const auto& name : candidates)
                if (not replacements.contains (name))
                {
                    replacements.try_emplace (name, getShape (model, row, depth + 1));
                    break;
                }

        jam::Strings emittedLines;
        auto previousLineElided { false };

        for (const auto& templateLine : jam::Strings::fromLines (block->getAllSubText()))
        {
            auto lineText { templateLine };
            auto lineHasPlaceholder { false };

            for (const auto& name : candidates)
                if (jam::Format::hasPlaceholder (lineText, name.toString()))
                {
                    lineHasPlaceholder = true;
                    lineText = jam::Format::replaceholder (lineText, name.toString(),
                        replacements.contains (name) ? replacements.at (name) : juce::String());
                }

            const auto lineIsElided { lineHasPlaceholder and lineText.trim().isEmpty() };

            if (lineIsElided)
            {
                previousLineElided = true;
            }
            else if (previousLineElided and lineText.trim().isEmpty())
            {
                previousLineElided = false;
            }
            else
            {
                emittedLines.add (lineText);
                previousLineElided = false;
            }
        }

        return emittedLines.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
    }

    jam::Strings getItems (const Model& model,
                           Element& row,
                           const juce::String& wiring,
                           const juce::Identifier& jack,
                           const juce::Identifier& shapeId) const
    {
        const auto isTableReference { wiring.startsWithChar (Chars::at) };

        jam::Array<Element*> sourceRows;
        jam::Strings sourceValues;
        juce::Identifier sourceKey { jack };

        if (isTableReference)
        {
            if (auto* wiredTable { model.getTables (row, wiring) })
                sourceRows = model.getTableRows (*wiredTable);
        }
        else
        {
            const auto source { juce::Identifier (wiring) };
            const auto tables { model.getTables() };

            const auto hasColumnSource { std::any_of (tables.begin(),
                tables.end(),
                [&model, &source] (Element* table)
                {
                    return model.isOutputTable (*table)
                           and model.getTableHeaders (*table).contains (source.toString());
                }) };

            if (hasColumnSource)
            {
                sourceKey = source;
                const auto currentFile { model.getValue (row, model.getTableValue (row, Id::file)) };
                jam::Strings seenValues;

                for (auto* table : tables)
                    if (model.isOutputTable (*table))
                        for (auto* candidate : model.getTableRows (*table))
                        {
                            const auto value { model.getValue (
                                *candidate, model.getTableValue (*candidate, source)) };

                            if (value.isNotEmpty() and value != currentFile
                                and not seenValues.contains (value, false))
                            {
                                seenValues.add (value);
                                sourceValues.add (value);
                            }
                        }
            }
            else
            {
                for (auto* table : tables)
                    if (model.isOutputTable (*table))
                        for (auto* candidate : model.getTableRows (*table))
                        {
                            const auto candidateBindings { model.getSource (*candidate, Id::structure) };

                            const auto matches { std::any_of (candidateBindings.begin(),
                                candidateBindings.end(),
                                [&source] (const auto& entry)
                                {
                                    const auto& [entryDepth, entryKey, entryValue] { entry };
                                    juce::ignoreUnused (entryDepth, entryValue);
                                    return entryKey == source;
                                }) };

                            if (matches)
                                sourceRows.add (candidate);
                        }
            }
        }

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

    juce::String
    getExpansion (const Model& model, Element& row, int depth, const juce::Identifier& jack) const
    {
        const auto wiring { model.getSource (row, depth, Id::placeholder).at (jack) };
        const auto structureBindings { model.getSource (row, depth, Id::structure) };
        const auto shapeBinding { structureBindings.contains (jack) ? structureBindings.at (jack)
                                                                     : juce::String() };
        const auto shapeId { juce::Identifier (jam::Format::getPostColon (shapeBinding).trim()) };
        const auto texts { getItems (model, row, wiring, jack, shapeId) };

        const auto separatorValue { getSeparator (model, row, depth, jack) };
        const auto separator { separatorValue.isNotEmpty()
                                   ? separatorValue
                                   : juce::String::charToString (Chars::newline) };
        const auto joined { texts.joinIntoString (separator, 0, -1) };
        const auto indent { juce::String::repeatedString (
            juce::String::charToString (Chars::space), depth * indentWidth) };

        return joined.isEmpty()
                   ? joined
                   : indent
                         + joined.replace (juce::String::charToString (Chars::newline),
                                           juce::String::charToString (Chars::newline) + indent);
    }

    juce::String getItem (const Model& model,
                          Element* sourceRow,
                          const juce::String& sourceValue,
                          const juce::Identifier& sourceKey,
                          const juce::Identifier& shapeId) const
    {
        auto* block { getCodeBlock (shapeId) };

        const auto& candidates {
            getPlaceholders().contains (shapeId) ? getPlaceholders().at (shapeId) : empty
        };

        auto itemText { block != nullptr ? block->getAllSubText() : juce::String() };

        for (const auto& name : candidates)
        {
            if (sourceRow != nullptr)
            {
                int deepestDepth { -1 };
                juce::String deepestValue;

                for (auto& entry : model.getSource (*sourceRow, Id::structure))
                {
                    const auto& [entryDepth, entryKey, entryValue] { entry };

                    if (entryKey == name and entryDepth >= deepestDepth)
                    {
                        deepestDepth = entryDepth;
                        deepestValue = entryValue;
                    }
                }

                const auto value { deepestDepth >= 0 ? deepestValue
                                                      : model.getValue (*sourceRow, name) };

                if (jam::Format::hasPlaceholder (itemText, name.toString()))
                    itemText = jam::Format::replaceholder (itemText, name.toString(), value);
            }
            else if (jam::Format::hasPlaceholder (itemText, name.toString()))
                itemText = jam::Format::replaceholder (itemText, name.toString(),
                    name == sourceKey ? sourceValue : juce::String());
        }

        return itemText;
    }

private:
    const jam::HashMap<juce::Identifier, jam::Array<juce::Identifier>> placeholders {};
};
