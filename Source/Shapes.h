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

    /** Column width of one level of shape-expansion indentation. */
    static constexpr int indentWidth { 4 };

    /**
     * @brief Answers whether every item in @p list is one of @p shapeId's
     *        distinct placeholder names.
     *
     * @param templateDocument The template document @p shapeId is looked
     *                         up against.
     * @param shapeId          The shape whose placeholder names are
     *                         checked against.
     * @param list             The list whose item ids are checked.
     * @returns @c true when every item in @p list matches one of
     *          @p shapeId's placeholder names.
     */
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

    /**
     * @brief Resolves one structure-list item's value into @p replacements
     *        -- a resolved binding when the item names plain data, or a
     *        recursively expanded nested shape when the item names
     *        @c template:\<id\>.
     *
     * @param replacements     The token-to-values map @p item's resolved
     *                         value is added to, keyed by @p item's id.
     * @param model            The model @p row belongs to.
     * @param templateDocument The template document shapes are looked up
     *                         against.
     * @param row              The row being expanded.
     * @param structureScope   The blockquote scope @p item was read from.
     * @param placeholderScope The blockquote scope carrying placeholder
     *                         bindings at this depth, or @c nullptr.
     * @param separatorScope   The blockquote scope carrying separator
     *                         bindings at this depth, or @c nullptr.
     * @param indent           The indentation prefix for lines rendered at
     *                         this depth.
     * @param depth            The current wiring depth.
     * @param matchedNestedShape The nested-shape-match result carried in
     *                         from the caller's fold over the structure
     *                         list, returned unchanged when @p item is not
     *                         itself a nested shape reference.
     * @param item             The structure-list item being resolved.
     * @returns @p matchedNestedShape, or the nested shape's own
     *          placeholder match when @p item names @c template:\<id\>.
     */
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

    /**
     * @brief Resolves every item of a depth's placeholder list into
     *        @p replacements, each item's value the joined, rendered items
     *        of the source it names.
     *
     * @param replacements     The token-to-values map each item's rendered
     *                         text is added to, keyed by the item's id.
     * @param model            The model @p row belongs to.
     * @param templateDocument The template document shapes are looked up
     *                         against.
     * @param row              The row being expanded.
     * @param structureList    The structure list at this depth, sharing
     *                         ids with @p placeholderList's items, or
     *                         @c nullptr when this depth carries no
     *                         structure list.
     * @param separatorScope   The blockquote scope carrying separator
     *                         bindings at this depth, or @c nullptr.
     * @param indent           The indentation prefix for lines rendered at
     *                         this depth.
     * @param depth            The current wiring depth.
     * @param placeholderList  The placeholder list whose items are
     *                         resolved.
     */
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

    /**
     * @brief Resolves one depth's structure and placeholder lists into
     *        @p replacements -- structure-list items not shared with the
     *        placeholder list through addStructureItemReplacement(), then
     *        the placeholder list itself through
     *        addPlaceholderListReplacements().
     *
     * @param replacements     The token-to-values map filled by this
     *                         depth's resolved items.
     * @param model            The model @p row belongs to.
     * @param templateDocument The template document shapes are looked up
     *                         against.
     * @param row              The row being expanded.
     * @param structureScope   The blockquote scope carrying this depth's
     *                         structure list.
     * @param placeholderScope The blockquote scope carrying this depth's
     *                         placeholder list, or @c nullptr.
     * @param separatorScope   The blockquote scope carrying this depth's
     *                         separator bindings, or @c nullptr.
     * @param indent           The indentation prefix for lines rendered at
     *                         this depth.
     * @param depth            The current wiring depth.
     * @returns @c true when a structure-list item resolved to a nested
     *          shape whose placeholders matched the shape's own list.
     */
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

    /**
     * @brief Resolves the single unclaimed token of a depth whose scope
     *        itself names a shape -- @p structureScope's own @c template
     *        head -- to that nested shape's rendered text.
     *
     * Only the first of @p tokens not already present in @p replacements
     * is filled; a depth that names a shape configures exactly one token.
     *
     * @param replacements     The token-to-values map the resolved token is
     *                         added to.
     * @param model            The model @p row belongs to.
     * @param templateDocument The template document shapes are looked up
     *                         against.
     * @param row              The row being expanded.
     * @param structureScope   The blockquote scope whose head names the
     *                         nested shape.
     * @param placeholderScope The blockquote scope carrying this depth's
     *                         placeholder bindings, or @c nullptr.
     * @param separatorScope   The blockquote scope carrying this depth's
     *                         separator bindings, or @c nullptr.
     * @param tokens           The enclosing shape's distinct placeholder
     *                         names.
     * @param indent           The indentation prefix for lines rendered at
     *                         this depth.
     * @param depth            The current wiring depth.
     */
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

    /**
     * @brief Walks @p row's wiring from @p structureScope outward, depth by
     *        depth, resolving every one of @p tokens to its rendered value.
     *
     * @param model            The model @p row belongs to.
     * @param templateDocument The template document shapes are looked up
     *                         against.
     * @param row              The row being expanded.
     * @param structureScope   The first depth's blockquote scope, or
     *                         @c nullptr when @p row has no structure.
     * @param placeholderScope The first depth's placeholder blockquote
     *                         scope, or @c nullptr.
     * @param separatorScope   The first depth's separator blockquote scope,
     *                         or @c nullptr.
     * @param tokens           The shape's distinct placeholder names.
     * @param indent           The indentation prefix for lines rendered at
     *                         the first depth.
     * @returns Every one of @p tokens mapped to its resolved value or
     *          values.
     */
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

    /**
     * @brief Replaces every one of @p tokens' @c :::token::: placeholders
     *        present on @p templateLine with its resolved value, advancing
     *        each token's occurrence count in @p occurrence as it is
     *        consumed.
     *
     * @param tokens        The shape's distinct placeholder names.
     * @param replacements  Every token mapped to its resolved value or
     *                      values.
     * @param occurrence    Each token's next occurrence index into its
     *                      values in @p replacements, advanced in place.
     * @param templateLine  The shape's authored line, before substitution.
     * @returns The substituted line, paired with whether the line carried
     *          at least one placeholder.
     */
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

    /**
     * @brief Substitutes @p shapeId's authored lines against @p tokens and
     *        @p replacements, collapsing a run of blank lines produced by
     *        an unfilled placeholder down to a single blank line.
     *
     * @param templateDocument The template document @p shapeId is read
     *                         from.
     * @param shapeId          The shape whose authored lines are
     *                         substituted.
     * @param tokens           The shape's distinct placeholder names.
     * @param replacements     Every token mapped to its resolved value or
     *                         values.
     * @returns The shape's rendered text, joined by newline.
     */
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

    /**
     * @brief Renders @p shapeId's text for @p row, resolving every one of
     *        the shape's placeholders through getReplacements() and
     *        substituting them through getLines().
     *
     * @param model            The model @p row belongs to.
     * @param templateDocument The template document @p shapeId is read
     *                         from.
     * @param row              The row being rendered.
     * @param shapeId          The shape to render.
     * @param structureScope   The blockquote scope carrying the shape's
     *                         structure wiring, or @c nullptr.
     * @param placeholderScope The blockquote scope carrying the shape's
     *                         placeholder wiring, or @c nullptr.
     * @param separatorScope   The blockquote scope carrying the shape's
     *                         separator wiring, or @c nullptr.
     * @param indent           The indentation prefix for the shape's
     *                         rendered lines.
     * @returns The shape's rendered text.
     */
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
