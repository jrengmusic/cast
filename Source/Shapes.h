#pragma once
#include <JuceHeader.h>
#include "Items.h"
#include "Model.h"
#include "TemplateDocument.h"

/**
 * @struct Shapes
 * @brief Static utility that expands a manifest row's structure into
 *        rendered text by walking the row's wiring depth by depth and
 *        substituting each depth's shape against a Model and a
 *        TemplateDocument.
 *
 * Shapes owns no state of its own and derives from neither Model nor
 * TemplateDocument -- every member is a pure function of the arguments it
 * is called with, operating on a Model's rows and a TemplateDocument's code
 * blocks through their public API.
 */
struct Shapes
{
    using Element = Model::Element;
    using Replacements = jam::HashMap<juce::Identifier, jam::Array<juce::String>>;
    using Sources = jam::Array<Element*>;

    /** Column width of one level of shape-expansion indentation. */
    static constexpr int indentWidth { 4 };

    static Element* getScopeAtDepth (const Model& model, Element& cellRoot, int depth)
    {
        auto* scope { &cellRoot };

        for (int hop { 0 }; hop < depth and scope != nullptr; ++hop)
            scope = model.getBlockquote (*scope);

        return scope;
    }

    static Element* getListLine (const Model& model, Element& cellRoot, int depth, int ordinal)
    {
        auto* scope { getScopeAtDepth (model, cellRoot, depth) };
        auto* list { scope != nullptr ? model.getList (*scope) : nullptr };
        auto index { 0 };

        if (list != nullptr)
            for (auto* item : *list)
                if (item->id == Id::list)
                {
                    if (index == ordinal)
                        return item;

                    ++index;
                }

        return nullptr;
    }

    static void addListSources (Sources& sources,
                                jam::Array<int>& sourceDepths,
                                jam::Array<int>& sourceOrdinals,
                                Replacements& bindings,
                                jam::HashMap<int, int>& ordinals,
                                Element& list,
                                int depth)
    {
        for (auto* item : list)
            if (item->id == Id::list)
            {
                sources.add (item);
                sourceDepths.add (depth);
                sourceOrdinals.add (ordinals[depth]++);
            }
            else
                bindings[item->id].add (*item->get<juce::String> (Id::value));
    }

    static void addSources (Sources& sources,
                            jam::Array<int>& sourceDepths,
                            jam::Array<int>& sourceOrdinals,
                            Replacements& bindings,
                            jam::HashMap<int, int>& ordinals,
                            const Model& model,
                            Element& scope,
                            int depth)
    {
        for (auto* child : scope)
        {
            if (child->isTag (Id::ul))
                addListSources (
                    sources, sourceDepths, sourceOrdinals, bindings, ordinals, *child, depth);

            if (child->isTag (Id::blockquote))
            {
                if (model.getStructure (*child).isNotEmpty())
                {
                    sources.add (child);
                    sourceDepths.add (depth + 1);
                    sourceOrdinals.add (0);
                }
                else
                    addSources (sources, sourceDepths, sourceOrdinals, bindings, ordinals, model,
                        *child, depth + 1);
            }
        }
    }

    static juce::String getJoin (const Model& model,
                                 const TemplateDocument& templateDocument,
                                 Element& row,
                                 Element* separatorItem)
    {
        juce::String join;

        if (separatorItem != nullptr)
            join = templateDocument.getBinding (
                model, row, *separatorItem->get<juce::String> (Id::value));

        return join.isNotEmpty() ? join : juce::String::charToString (Chars::newline);
    }

    static juce::String getChildSource (const Model& model, Element& row, int depth)
    {
        auto* childItem { getListLine (model, *model.getTableCell (row, Id::list), depth + 1, 0) };

        return childItem != nullptr ? *childItem->get<juce::String> (Id::value) : juce::String();
    }

    static juce::String getChildJoin (const Model& model,
                                      const TemplateDocument& templateDocument,
                                      Element& row,
                                      int depth)
    {
        auto* separatorItem {
            getListLine (model, *model.getTableCell (row, Id::separator), depth + 1, 0)
        };

        return getJoin (model, templateDocument, row, separatorItem);
    }

    static juce::String getListSourceValue (Element*& commentTable,
                                            const Model& model,
                                            const TemplateDocument& templateDocument,
                                            const jam::Array<Element*>& tables,
                                            Element& row,
                                            const juce::String& structureValue,
                                            int depth,
                                            int ordinal,
                                            const juce::String& extension)
    {
        const auto shapeId { juce::Identifier (jam::Format::getPostColon (structureValue).trim()) };
        auto* listItem { getListLine (model, *model.getTableCell (row, Id::list), depth, ordinal) };
        auto* separatorItem {
            getListLine (model, *model.getTableCell (row, Id::separator), depth, ordinal)
        };
        const auto& source { *listItem->get<juce::String> (Id::value) };
        const auto join { getJoin (model, templateDocument, row, separatorItem) };
        const auto childSource { getChildSource (model, row, depth) };
        const auto childJoin { getChildJoin (model, templateDocument, row, depth) };

        if (commentTable == nullptr and source.startsWithChar (Chars::at))
            commentTable = model.getTable (row, source);

        return Items::getJoinedItems (model, templateDocument, tables, row, source, shapeId, join,
            childSource, childJoin, extension);
    }

    static juce::String getShape (const Model& model,
                                  const TemplateDocument& templateDocument,
                                  const jam::Array<Element*>& tables,
                                  Element& row,
                                  const juce::Identifier& shapeId,
                                  Element& structureScope,
                                  int depth,
                                  const juce::String& extension)
    {
        const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };
        jam::Array<int> listDepths;
        Element* commentTable { nullptr };
        Replacements replacements;
        addReplacements (listDepths, commentTable, replacements, model, templateDocument,
            tables, row, structureScope, depth, extension);
        return getLines (model, row, commentTable, templateDocument, shapeId, tokens, replacements,
            listDepths, extension);
    }

    static void addReplacements (jam::Array<int>& listDepths,
                                 Element*& commentTable,
                                 Replacements& replacements,
                                 const Model& model,
                                 const TemplateDocument& templateDocument,
                                 const jam::Array<Element*>& tables,
                                 Element& row,
                                 Element& structureScope,
                                 int depth,
                                 const juce::String& extension)
    {
        Sources sources;
        jam::Array<int> sourceDepths;
        jam::Array<int> sourceOrdinals;
        jam::HashMap<int, int> ordinals;
        Replacements bindings;

        addSources (sources, sourceDepths, sourceOrdinals, bindings, ordinals, model, structureScope, depth);

        for (const auto& [name, values] : bindings)
            replacements[name].add (templateDocument.getBinding (model, row, values.at (0)));

        for (int index { 0 }; index < sources.size(); ++index)
            addSourceReplacement (replacements, listDepths, commentTable, model, templateDocument,
                tables, row, *sources.at (index), sourceDepths.at (index), sourceOrdinals.at (index),
                extension);
    }

    static void addSourceReplacement (Replacements& replacements,
                                      jam::Array<int>& listDepths,
                                      Element*& commentTable,
                                      const Model& model,
                                      const TemplateDocument& templateDocument,
                                      const jam::Array<Element*>& tables,
                                      Element& row,
                                      Element& source,
                                      int depth,
                                      int ordinal,
                                      const juce::String& extension)
    {
        if (source.id == Id::list)
        {
            replacements[Id::list].add (getListSourceValue (commentTable, model, templateDocument,
                tables, row, *source.get<juce::String> (Id::value), depth, ordinal, extension));
            listDepths.add (depth);
        }
        else
        {
            const auto shapeId { juce::Identifier (model.getStructure (source)) };
            replacements[Id::list].add (
                getShape (model, templateDocument, tables, row, shapeId, source, depth, extension));
            listDepths.add (0);
        }
    }

    static juce::String getListValue (const Replacements& replacements,
                                      const jam::Array<int>& listDepths,
                                      int occurrenceIndex,
                                      bool isAtColumnZero)
    {
        const auto& values { replacements.at (Id::list) };
        const auto index { std::min (occurrenceIndex, values.size() - 1) };
        const auto& value { values.at (index) };

        if (not isAtColumnZero or listDepths.at (index) == 0)
            return value;

        const auto indent { juce::String::repeatedString (
            juce::String::charToString (Chars::space), indentWidth * listDepths.at (index)) };

        return indent
               + jam::Strings::fromLines (value).joinIntoString (
                     juce::String::charToString (Chars::newline) + indent, 0, -1);
    }

    static juce::String getTokenValue (const Model& model,
                                       Element& row,
                                       Element* commentTable,
                                       const Replacements& replacements,
                                       const juce::Identifier& name)
    {
        if (replacements.contains (name))
            return replacements.at (name).at (0);

        if (auto* cell { model.getTableCell (row, name) })
            return *cell->get<juce::String> (Id::value);

        if (name == Id::comment and commentTable != nullptr)
            return *commentTable->get<juce::String> (Id::comment);

        return {};
    }

    static std::pair<juce::String, bool> getSubstitutedLine (const Model& model,
                                                              Element& row,
                                                              Element* commentTable,
                                                              const jam::Document::Identifiers& tokens,
                                                              const Replacements& replacements,
                                                              const jam::Array<int>& listDepths,
                                                              jam::HashMap<juce::Identifier, int>& occurrence,
                                                              const juce::String& templateLine,
                                                              const juce::String& extension)
    {
        auto lineText { templateLine };
        auto lineHasPlaceholder { false };

        for (const auto& name : tokens)
            if (jam::Format::hasPlaceholder (lineText, name.toString()))
            {
                lineHasPlaceholder = true;
                const auto isAtColumnZero { templateLine.indexOf (Items::getMarker (name)) == 0 };
                auto& tokenOccurrence { occurrence[name] };
                auto value { name == Id::list
                                 ? getListValue (
                                       replacements, listDepths, tokenOccurrence, isAtColumnZero)
                                 : getTokenValue (model, row, commentTable, replacements, name) };

                if (name == Id::comment and value.isNotEmpty())
                    value = templateLine.trim() == Items::getMarker (name)
                                ? Transforms::toBrief (value, extension)
                                : Transforms::toComment (value, extension);

                lineText = jam::Format::replaceholder (lineText, name.toString(), value);
                ++tokenOccurrence;
            }

        return { lineText, lineHasPlaceholder };
    }

    static juce::String getLines (const Model& model,
                                  Element& row,
                                  Element* commentTable,
                                  const TemplateDocument& templateDocument,
                                  const juce::Identifier& shapeId,
                                  const jam::Document::Identifiers& tokens,
                                  const Replacements& replacements,
                                  const jam::Array<int>& listDepths,
                                  const juce::String& extension)
    {
        jam::Strings lines;
        auto previousLineIsEmpty { false };
        jam::HashMap<juce::Identifier, int> occurrence;

        for (const auto& templateLine : jam::Strings::fromLines (
                 *templateDocument.getCodeBlock (shapeId)->get<juce::String> (Id::value)))
        {
            const auto [lineText, lineHasPlaceholder] { getSubstitutedLine (model, row, commentTable,
                tokens, replacements, listDepths, occurrence, templateLine, extension) };
            const auto lineIsEmpty { lineHasPlaceholder and lineText.trim().isEmpty() };

            if (lineIsEmpty)
                previousLineIsEmpty = true;
            else if (previousLineIsEmpty and lineText.trim().isEmpty())
                previousLineIsEmpty = false;
            else
            {
                lines.add (lineText);
                previousLineIsEmpty = false;
            }
        }

        return lines.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
    }

    /**
     * @brief Resolves one occurrence's merged value across @p
     *        occurrenceValues -- the single shared value when every row
     *        that carries the token agrees, or every row's value joined by
     *        @p joinText when they diverge.
     *
     * @param occurrenceValues Every merged row's own value for the same
     *                         token occurrence.
     * @param joinText         The text joining diverging rows' values.
     * @returns The occurrence's merged value.
     */
    static juce::String getMergedOccurrenceValue (const jam::Strings& occurrenceValues,
                                                  const juce::String& joinText)
    {
        return std::all_of (occurrenceValues.begin(), occurrenceValues.end(),
                   [&occurrenceValues] (const juce::String& value)
                   { return value == occurrenceValues.at (0); })
                   ? occurrenceValues.at (0)
                   : occurrenceValues.joinIntoString (joinText, 0, -1);
    }

    /**
     * @brief Fills @p mergedReplacements with every one of @p tokens
     *        mapped to its merged occurrence values, merging every row's
     *        own Replacements in @p rowReplacements through
     *        getMergedOccurrenceValue().
     *
     * @param mergedReplacements Filled with every one of @p tokens mapped
     *                           to its merged occurrence values.
     * @param rowReplacements    Every row's own Replacements, one per
     *                           merged row.
     * @param tokens             The shape's distinct placeholder names.
     * @param joinText           The text joining diverging rows' values.
     */
    static void addMergedReplacements (Replacements& mergedReplacements,
                                       const jam::Array<Replacements>& rowReplacements,
                                       const jam::Document::Identifiers& tokens,
                                       const juce::String& joinText)
    {
        jam::HashMap<juce::Identifier, jam::Array<jam::Strings>> occurrenceValues;

        for (const auto& rowReplacement : rowReplacements)
            for (const auto& [name, values] : rowReplacement)
                for (int occurrence { 0 }; occurrence < values.size(); ++occurrence)
                {
                    if (occurrenceValues[name].size() <= occurrence)
                        occurrenceValues[name].resize (occurrence + 1);

                    occurrenceValues[name].at (occurrence).add (values.at (occurrence));
                }

        for (const auto& name : tokens)
            for (const auto& occurrenceValue : occurrenceValues[name])
                mergedReplacements[name].add (getMergedOccurrenceValue (occurrenceValue, joinText));
    }

    static juce::String getShape (const Model& model,
                                  const TemplateDocument& templateDocument,
                                  const jam::Array<Element*>& tables,
                                  const jam::Array<Element*>& rows,
                                  const juce::String& joinText)
    {
        jassert (rows.size() > 0);

        auto* firstRow { rows.first() };
        auto& firstStructureScope { *model.getTableCell (*firstRow, Id::structure) };
        const auto shapeId { juce::Identifier (model.getStructure (firstStructureScope)) };
        const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };

        const auto extension { jam::Format::onlyExtensionFromFilename (
            jam::Format::toFileName (model.getValue (*firstRow, Id::file))) };

        jam::Array<Replacements> rowReplacements;
        jam::Array<int> listDepths;
        Element* commentTable { nullptr };

        for (auto* row : rows)
        {
            jam::Array<int> rowListDepths;
            Element* rowCommentTable { nullptr };
            rowReplacements.add ({});
            addReplacements (rowListDepths, rowCommentTable, rowReplacements.last(), model,
                templateDocument, tables, *row, *model.getTableCell (*row, Id::structure), 0, extension);

            if (row == firstRow)
            {
                listDepths = std::move (rowListDepths);
                commentTable = rowCommentTable;
            }
        }

        Replacements mergedReplacements;
        addMergedReplacements (mergedReplacements, rowReplacements, tokens, joinText);

        return getLines (model, *firstRow, commentTable, templateDocument, shapeId, tokens,
            mergedReplacements, listDepths, extension);
    }
};
