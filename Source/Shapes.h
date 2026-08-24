#pragma once
#include <JuceHeader.h>
#include "Items.h"
#include "Model.h"
#include "TemplateDocument.h"

struct Shapes
{
    using Element = Model::Element;
    using Replacements = jam::HashMap<juce::Identifier, jam::Array<juce::String>>;

    static constexpr int indentWidth { 4 };

    static bool hasMatchingPlaceholders (const TemplateDocument& templateDocument,
                                         const juce::Identifier& shapeId, Element& list)
    {
        for (auto* candidate : list)
            if (not templateDocument.getCodeBlock (shapeId)
                        ->get<jam::Document::Identifiers> (Id::placeholder)
                        ->contains (candidate->id))
                return false;

        return true;
    }

    static bool addStructureItemReplacement (Replacements& replacements,
                                             const Model& model,
                                             const TemplateDocument& templateDocument,
                                             Element& row,
                                             Element& structureScope,
                                             Element* placeholderScope,
                                             Element* separatorScope,
                                             const juce::String& indent,
                                             int depth,
                                             bool matchedNestedShape,
                                             Element& item)
    {
        const auto& value { *item.get<juce::String> (Id::value) };

        if (jam::Format::getPreColon (value).trim() != Id::templatePath.toString())
        {
            replacements[item.id].add (templateDocument.getBinding (model, row, value));
            return matchedNestedShape;
        }

        const auto shapeId { juce::Identifier (jam::Format::getPostColon (value).trim()) };
        auto nestedShapeMatches { false };

        if (auto* scope { model.getBlockquote (structureScope) })
            if (auto* list { model.getList (*scope) })
                nestedShapeMatches = hasMatchingPlaceholders (templateDocument, shapeId, *list);

        replacements[item.id].add (
            getShape (model,
                      templateDocument,
                      row,
                      shapeId,
                      model.getBlockquote (structureScope),
                      placeholderScope != nullptr ? model.getBlockquote (*placeholderScope) : nullptr,
                      separatorScope != nullptr ? model.getBlockquote (*separatorScope) : nullptr,
                      indent
                          + juce::String::repeatedString (
                              juce::String::charToString (Chars::space), indentWidth * (depth + 1))));

        return nestedShapeMatches;
    }

    static void addPlaceholderListReplacements (Replacements& replacements,
                                                 const Model& model,
                                                 const TemplateDocument& templateDocument,
                                                 Element& row,
                                                 Element* structureList,
                                                 Element* separatorScope,
                                                 const juce::String& indent,
                                                 int depth,
                                                 Element& placeholderList)
    {
        for (auto* item : placeholderList)
        {
            auto* structureItem { structureList != nullptr
                                      ? model.getListItem (*structureList, item->id)
                                      : nullptr };
            const auto shapeId { juce::Identifier (
                structureItem != nullptr
                    ? jam::Format::getPostColon (*structureItem->get<juce::String> (Id::value)).trim()
                    : juce::String()) };

            replacements[item->id].add (
                Items::getJoinedItems (model,
                                       templateDocument,
                                       row,
                                       item->id,
                                       *item->get<juce::String> (Id::value),
                                       shapeId,
                                       separatorScope,
                                       indent
                                           + juce::String::repeatedString (
                                               juce::String::charToString (Chars::space),
                                               indentWidth * depth)));
        }
    }

    static bool addStructureListReplacements (Replacements& replacements,
                                              const Model& model,
                                              const TemplateDocument& templateDocument,
                                              Element& row,
                                              Element& structureScope,
                                              Element* placeholderScope,
                                              Element* separatorScope,
                                              const juce::String& indent,
                                              int depth)
    {
        auto* structureList { model.getList (structureScope) };
        auto* placeholderList { placeholderScope != nullptr ? model.getList (*placeholderScope)
                                                             : nullptr };
        auto matchedNestedShape { false };

        if (structureList != nullptr)
            for (auto* item : *structureList)
                if (placeholderList == nullptr
                    or model.getListItem (*placeholderList, item->id) == nullptr)
                    matchedNestedShape = addStructureItemReplacement (replacements, model,
                        templateDocument, row, structureScope, placeholderScope, separatorScope,
                        indent, depth, matchedNestedShape, *item);

        if (placeholderList != nullptr)
            addPlaceholderListReplacements (replacements, model, templateDocument, row,
                structureList, separatorScope, indent, depth, *placeholderList);

        return matchedNestedShape;
    }

    static void addShapeReplacements (Replacements& replacements,
                                      const Model& model,
                                      const TemplateDocument& templateDocument,
                                      Element& row,
                                      Element& structureScope,
                                      Element* placeholderScope,
                                      Element* separatorScope,
                                      const jam::Document::Identifiers& tokens,
                                      const juce::String& indent,
                                      int depth)
    {
        for (const auto& name : tokens)
            if (not replacements.contains (name))
            {
                replacements[name].add (
                    getShape (model,
                              templateDocument,
                              row,
                              juce::Identifier (model.getStructure (structureScope)),
                              &structureScope,
                              placeholderScope,
                              separatorScope,
                              indent
                                  + juce::String::repeatedString (
                                      juce::String::charToString (Chars::space), indentWidth * depth)));
                break;
            }
    }

    static Replacements getReplacements (const Model& model,
                                         const TemplateDocument& templateDocument,
                                         Element& row,
                                         Element* structureScope,
                                         Element* placeholderScope,
                                         Element* separatorScope,
                                         const jam::Document::Identifiers& tokens,
                                         const juce::String& indent)
    {
        Replacements replacements;
        int depth { 0 };

        const auto advance = [&model, &structureScope, &placeholderScope, &separatorScope,
                              &depth]() noexcept
        {
            structureScope = structureScope != nullptr ? model.getBlockquote (*structureScope) : nullptr;
            placeholderScope = placeholderScope != nullptr ? model.getBlockquote (*placeholderScope)
                                                            : nullptr;
            separatorScope = separatorScope != nullptr ? model.getBlockquote (*separatorScope) : nullptr;
            ++depth;
        };

        while (structureScope != nullptr)
        {
            auto matchedNestedShape { false };

            if (depth > 0 and model.getStructure (*structureScope).isNotEmpty())
                addShapeReplacements (replacements, model, templateDocument, row, *structureScope,
                    placeholderScope, separatorScope, tokens, indent, depth);
            else
                matchedNestedShape = addStructureListReplacements (replacements, model,
                    templateDocument, row, *structureScope, placeholderScope, separatorScope,
                    indent, depth);

            advance();

            if (matchedNestedShape and structureScope != nullptr)
                advance();
        }

        return replacements;
    }

    static std::pair<juce::String, bool>
    getSubstitutedLine (const jam::Document::Identifiers& tokens,
                        const Replacements& replacements,
                        jam::HashMap<juce::Identifier, int>& occurrence,
                        const juce::String& templateLine)
    {
        auto lineText { templateLine };
        auto lineHasPlaceholder { false };

        for (const auto& name : tokens)
            if (jam::Format::hasPlaceholder (lineText, name.toString()))
            {
                lineHasPlaceholder = true;
                juce::String value;

                if (replacements.contains (name))
                {
                    const auto& values { replacements.at (name) };

                    if (not values.isEmpty())
                        value = values.at (std::min (occurrence[name], values.size() - 1));
                }

                lineText = jam::Format::replaceholder (lineText, name.toString(), value);
                ++occurrence[name];
            }

        return { lineText, lineHasPlaceholder };
    }

    static juce::String getLines (const TemplateDocument& templateDocument,
                                  const juce::Identifier& shapeId,
                                  const jam::Document::Identifiers& tokens,
                                  const Replacements& replacements)
    {
        jam::Strings lines;
        auto previousLineIsEmpty { false };
        jam::HashMap<juce::Identifier, int> occurrence;

        for (const auto& templateLine : jam::Strings::fromLines (
                 *templateDocument.getCodeBlock (shapeId)->get<juce::String> (Id::value)))
        {
            const auto [lineText, lineHasPlaceholder] { getSubstitutedLine (
                tokens, replacements, occurrence, templateLine) };
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

        const auto composite { lines.joinIntoString (
            juce::String::charToString (Chars::newline), 0, -1) };
        return composite;
    }

    static juce::String getShape (const Model& model,
                                  const TemplateDocument& templateDocument,
                                  Element& row,
                                  const juce::Identifier& shapeId,
                                  Element* structureScope,
                                  Element* placeholderScope,
                                  Element* separatorScope,
                                  const juce::String& indent)
    {
        const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };
        const auto replacements { getReplacements (model, templateDocument, row, structureScope,
            placeholderScope, separatorScope, tokens, indent) };
        return getLines (templateDocument, shapeId, tokens, replacements);
    }
};
