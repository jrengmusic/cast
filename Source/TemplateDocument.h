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

                              const auto interior { jam::Format::upTo (remaining, Id::tripleColon, false)
                                                        .trimCharactersAtStart (
                                                            juce::String::charToString (Chars::colon)) };
                              names.addIfNotAlreadyThere (
                                  juce::Identifier (jam::Format::getPreColon (interior)));
                              remaining = jam::Format::from (remaining, Id::tripleColon, false);
                          }

                          found.try_emplace (block->id, std::move (names));
                      }

                  return found;
              }())
    {
    }

    static constexpr int indentWidth { 4 };

    juce::String getContent (const juce::String& value) const
    {
        const auto id { juce::Identifier (jam::Format::getPostColon (value).trim()) };
        auto* block { getCodeBlock (id) };

        jassert (block != nullptr);

        if (block == nullptr)
            jam::debug::Log::write (text::Diagnostics::failTemplateMissing + Id::diagnosticSeparator
                                    + id.toString());

        return block != nullptr ? block->getAllSubText() : juce::String();
    }

    juce::String getBinding (const Model& model,
                             Element& row,
                             int depth,
                             const juce::String& value,
                             const juce::Identifier& jack) const
    {
        if (jam::Format::getPreColon (value).trim() == Id::templatePath.toString())
            return getContent (value);

        if (value.startsWithChar (Chars::at)
            and jam::Format::getPostColon (value).containsChar (Chars::colon))
        {
            const auto alias { jam::Format::getPreColon (value) };
            const auto remainder { jam::Format::getPostColon (value) };
            const auto tableName { jam::Format::getPreColon (remainder) };
            const auto entry { jam::Format::getPostColon (remainder) };

            if (auto* entryTable { model.getTables (
                    row, alias + juce::String::charToString (Chars::colon) + tableName) })
                return model.getTableValue (*entryTable, Id::value, juce::Identifier (entry));

            jassertfalse;
            jam::debug::Log::write (
                jam::MarkdownValidator::getLocation (*row.parent, row, Id::structure.toString())
                + Id::diagnosticSeparator + text::Diagnostics::failTableMissing
                + Id::diagnosticSeparator + tableName);
            return {};
        }

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

    juce::String
    getExpansion (const Model& model, Element& row, int depth, const juce::Identifier& jack) const
    {
        const auto wiring { model.getSource (row, depth, Id::placeholder) };
        jassert (wiring.contains (jack));

        const auto wiringValue { wiring.at (jack) };
        const auto shapeBindings { model.getSource (row, depth, Id::structure) };
        jassert (shapeBindings.contains (jack));

        const auto shapeId { juce::Identifier (
            jam::Format::getPostColon (shapeBindings.at (jack)).trim()) };
        const auto isTableReference { wiringValue.startsWithChar (Chars::at)
                                      and wiringValue.containsChar (Chars::colon) };

        jam::Array<Element*> sourceRows;

        if (isTableReference)
        {
            if (auto* wiredTable { model.getTables (row, wiringValue) })
                sourceRows = model.getTableRows (*wiredTable);
        }
        else
        {
            const auto source { juce::Identifier (wiringValue) };
            const auto tables { model.getTables() };

            const auto hasColumnSource { std::any_of (
                tables.begin(),
                tables.end(),
                [&model, &source] (Element* table)
                {
                    return model.isOutputTable (*table)
                           and model.getTableHeaders (*table).contains (source.toString());
                }) };

            const auto currentFile { model.getValue (row, model.getTableValue (row, Id::file)) };
            jam::Strings seenValues;

            for (auto* table : tables)
                if (model.isOutputTable (*table))
                    for (auto* candidate : model.getTableRows (*table))
                    {
                        bool matches { false };

                        if (hasColumnSource)
                        {
                            const auto value { model.getValue (
                                *candidate, model.getTableValue (*candidate, source)) };

                            matches = value.isNotEmpty() and value != currentFile
                                      and not seenValues.contains (value, false);

                            if (matches)
                                seenValues.add (value);
                        }
                        else
                        {
                            const auto candidateBindings { model.getSource (
                                *candidate, Id::structure) };

                            matches = std::any_of (
                                candidateBindings.begin(),
                                candidateBindings.end(),
                                [&source] (const auto& entry)
                                {
                                    const auto& [entryDepth, entryKey, entryValue] { entry };
                                    juce::ignoreUnused (entryDepth, entryValue);
                                    return entryKey == source;
                                });
                        }

                        if (matches)
                            sourceRows.add (candidate);
                    }
        }

        jam::Strings texts;

        for (auto* sourceRow : sourceRows)
        {
            const auto rowText { build (model, row, *sourceRow, depth, {}, shapeId)
                                     .trimCharactersAtEnd (
                                         juce::String::charToString (Chars::newline)) };

            if (rowText.isNotEmpty())
                texts.add (rowText);
        }

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

    juce::String build (const Model& model,
                        Element& row,
                        Element& sourceRow,
                        int depth,
                        const jam::HashMap<juce::Identifier, juce::String>& tokens,
                        const juce::Identifier& codeId) const
    {
        constexpr int lineIsVacant { 0 };
        constexpr int lineIsBlank { 1 };
        constexpr int lineIsContent { 2 };

        auto* block { getCodeBlock (codeId) };
        jassert (block != nullptr);

        if (block == nullptr)
        {
            jam::debug::Log::write (text::Diagnostics::failTemplateMissing + Id::diagnosticSeparator
                                    + codeId.toString());
            return {};
        }

        const auto templateText { block->getAllSubText() };
        static const jam::Array<juce::Identifier> empty;
        const auto& candidates { placeholders.contains (codeId) ? placeholders.at (codeId)
                                                                 : empty };

        jam::Strings assembledLines;
        juce::Array<int> lineClassifications;
        jam::HashMap<juce::Identifier, int> occurrences;
        const auto extension { jam::Format::onlyExtensionFromFilename (
            model.getValue (row, model.getTableValue (row, Id::file))) };

        for (const auto& templateLine : jam::Strings::fromLines (templateText))
        {
            auto lineText { templateLine };
            bool lineHasPlaceholder { false };

            for (const auto& name : candidates)
            {
                const auto marker { Id::tripleColon + name.toString() + Id::tripleColon };

                if (jam::Format::hasPlaceholder (lineText, name.toString()))
                {
                    occurrences.try_emplace (name, 0);
                    ++occurrences.at (name);

                    const auto value { getReplacement (model,
                                                       row,
                                                       sourceRow,
                                                       depth,
                                                       tokens,
                                                       name,
                                                       occurrences.at (name),
                                                       extension) };
                    lineHasPlaceholder = true;

                    if (value.isNotEmpty() and value != marker)
                        lineText = jam::Format::replaceholder (lineText, name.toString(), value);
                    else if (lineText.trim() != marker)
                    {
                        const auto spacedMarker { juce::String::charToString (Chars::space)
                                                  + marker };
                        lineText = lineText.replace (spacedMarker, juce::String());
                        lineText =
                            jam::Format::replaceholder (lineText, name.toString(), juce::String());
                    }
                }

                for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
                {
                    juce::ignoreUnused (transformFunction);
                    const auto tagged { name.toString() + juce::String::charToString (Chars::colon)
                                        + transformName };

                    if (jam::Format::hasPlaceholder (lineText, tagged))
                    {
                        lineHasPlaceholder = true;

                        const auto replacement { getReplacement (
                            model, row, sourceRow, depth, tokens, name, 1, extension) };
                        const auto taggedMarker { Id::tripleColon + tagged + Id::tripleColon };

                        const auto value { replacement != marker and replacement.isNotEmpty()
                                               ? Transforms::getTransformed (
                                                     transformName, replacement, extension)
                                               : juce::String() };

                        if (value.isNotEmpty())
                            lineText = jam::Format::replaceholder (lineText, tagged, value);
                        else
                        {
                            const auto spacedMarker { juce::String::charToString (Chars::space)
                                                      + taggedMarker };
                            lineText = lineText.replace (spacedMarker, juce::String());
                            lineText =
                                jam::Format::replaceholder (lineText, tagged, juce::String());
                        }
                    }
                }
            }

            lineClassifications.add (lineText.trim().isEmpty()
                                         ? (lineHasPlaceholder ? lineIsVacant : lineIsBlank)
                                         : lineIsContent);

            assembledLines.add (lineText);
        }

        bool hasEmittedContent { false };
        bool pendingBlank { false };
        bool suppressNextBlank { false };
        jam::Strings emittedLines;

        for (int lineIndex { 0 }; lineIndex < assembledLines.size(); ++lineIndex)
        {
            const auto lineText { assembledLines.at (lineIndex) };
            const auto classification { lineClassifications[lineIndex] };

            if (classification == lineIsContent)
            {
                if (pendingBlank and hasEmittedContent)
                    emittedLines.add (juce::String());

                emittedLines.add (lineText);
                hasEmittedContent = true;
                pendingBlank = false;
                suppressNextBlank = false;
            }
            else if (classification == lineIsBlank)
            {
                if (suppressNextBlank)
                    suppressNextBlank = false;
                else
                    pendingBlank = true;
            }
            else
            {
                pendingBlank = false;
                suppressNextBlank = true;
            }
        }

        return emittedLines.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
    }

    const jam::HashMap<juce::Identifier, jam::Array<juce::Identifier>> placeholders {};

private:
    juce::String getReplacement (const Model& model,
                                 Element& row,
                                 Element& sourceRow,
                                 int depth,
                                 const jam::HashMap<juce::Identifier, juce::String>& tokens,
                                 const juce::Identifier& name,
                                 int occurrence,
                                 const juce::String& extension) const
    {
        const auto marker { Id::tripleColon + name.toString() + Id::tripleColon };

        if (tokens.contains (name))
            return tokens.at (name);

        const auto hasNested { model.getStructure (row, depth + 1).isNotEmpty() };
        const auto upperBound { hasNested ? depth + 1 : std::numeric_limits<int>::max() };

        jam::Array<std::tuple<int, juce::Identifier, juce::String>> ownedWiring;

        for (auto& entry : model.getSource (row, Id::placeholder))
        {
            const auto& [entryDepth, entryKey, entryValue] { entry };

            if (entryKey == name and entryDepth >= depth and entryDepth < upperBound)
                ownedWiring.add (entry);
        }

        if (occurrence >= 1 and occurrence <= ownedWiring.size())
        {
            const auto& [jackDepth, jackName, jackValue] { ownedWiring.at (occurrence - 1) };
            juce::ignoreUnused (jackValue);
            return getExpansion (model, row, jackDepth, jackName);
        }

        const auto bindings { model.getSource (row, depth, Id::structure) };

        if (bindings.contains (name))
            return getBinding (model, row, depth, bindings.at (name), name);

        const auto sourceHeaders { model.getTableHeaders (*sourceRow.parent) };
        const auto isEntryTable { not sourceHeaders.isEmpty()
                                  and *sourceHeaders.begin() == Id::entry.toString() };

        if (sourceHeaders.contains (name.toString())
            or (isEntryTable and (name == Id::entry or name == Id::value)))
            return getCell (model, sourceRow, name, extension);

        for (auto& entry : model.getSource (row, Id::placeholder))
        {
            const auto& [entryDepth, entryKey, entryValue] { entry };
            juce::ignoreUnused (entryDepth, entryKey);

            if (entryValue.startsWithChar (Chars::at) and entryValue.containsChar (Chars::colon))
                if (auto* wiredTable { model.getTables (row, entryValue) })
                    if (model.getTableHeaders (*wiredTable).contains (name.toString()))
                        if (auto* firstRow { model.getTableRows (*wiredTable).first() })
                            return getCell (model, *firstRow, name, extension);
        }

        const auto indexed { model.getValue (
            row, juce::String::charToString (Chars::at) + name.toString()) };

        if (indexed.isNotEmpty())
            return indexed;

        {
            Element* preceding { nullptr };

            for (auto* sibling : *sourceRow.parent->parent)
            {
                if (sibling == sourceRow.parent)
                {
                    if (preceding != nullptr and preceding->isTag (Id::p))
                        return preceding->getAllSubText();

                    break;
                }

                preceding = sibling;
            }
        }

        return marker;
    }

    juce::String getCell (const Model& model,
                          Element& sourceRow,
                          const juce::Identifier& name,
                          const juce::String& extension) const
    {
        const auto headers { model.getTableHeaders (*sourceRow.parent) };
        const auto isEntryTable { not headers.isEmpty()
                                  and *headers.begin() == Id::entry.toString() };

        if (isEntryTable and (name == Id::entry or name == Id::value))
        {
            const auto lexiconTables { model.getTables (Id::lexicon) };

            if (not lexiconTables.isEmpty())
            {
                auto* lexiconRow { model.getTableRow (*lexiconTables.first(), sourceRow.id) };

                jassert (lexiconRow != nullptr);

                if (lexiconRow == nullptr)
                {
                    jam::debug::Log::write (jam::MarkdownValidator::getLocation (
                                                *sourceRow.parent, sourceRow, name.toString())
                                            + Id::diagnosticSeparator
                                            + text::Diagnostics::failEntityMissing
                                            + Id::diagnosticSeparator + sourceRow.id.toString());
                    return {};
                }

                return name == Id::entry ? lexiconRow->id.toString()
                                         : getCell (model, *lexiconRow, Id::value, extension);
            }
        }

        if (not headers.contains (name.toString()))
            return {};

        if (*headers.begin() == name.toString())
            return model.getTableValue (sourceRow, name);

        const auto rowKey { model.getTableValue (sourceRow,
                                                 juce::Identifier (*headers.begin())) };
        auto* cellElement { model.getTableCell (sourceRow, name) };
        const auto isLiteral { cellElement != nullptr and cellElement->firstChild != nullptr
                               and cellElement->firstChild->isTag (Id::code)
                               and cellElement->firstChild->nextSibling == nullptr };
        const auto cell { model.getTableValue (sourceRow, name) };

        juce::String value;

        if (isLiteral)
            value = cell;
        else if (cell.isEmpty())
            value = rowKey;
        else if (Transforms::contains (cell))
            value = Transforms::getTransformed (cell, rowKey, extension);
        else
            value = cell;

        if (const auto format { model.getFormat (sourceRow, name) }; format.isNotEmpty())
            value = Transforms::getTransformed (format, value, extension);

        return value;
    }
};
