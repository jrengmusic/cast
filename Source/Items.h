#pragma once
#include <JuceHeader.h>
#include "Model.h"
#include "TemplateDocument.h"

/**
 * @struct Items
 * @brief Static utility that discovers a placeholder list's source rows or
 *        column values across a Model's tables and renders each one
 *        through a TemplateDocument shape.
 *
 * Items owns no state of its own and derives from neither Model nor
 * TemplateDocument -- every member is a pure function of the arguments it
 * is called with, operating on a Model's rows and a TemplateDocument's code
 * blocks through their public API.
 */
struct Items
{
    using Element = Model::Element;

    /**
     * @brief Collects every distinct, non-empty value of @p source's
     *        column across @p tables' output rows, excluding the value
     *        belonging to @p row's own file.
     *
     * @param model  The model @p row and @p tables belong to.
     * @param row    The row whose own file's value is excluded.
     * @param tables The tables searched for @p source's column.
     * @param source The column name whose values are collected.
     * @returns Every distinct, non-empty column value found, in discovery
     *          order.
     */
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

    /**
     * @brief Collects every output row across @p tables whose structure
     *        wiring, at any depth, declares @p source as a blank-binding
     *        selector.
     *
     * @param model  The model @p tables belong to.
     * @param tables The tables searched for rows selecting @p source.
     * @param source The blank binding name a row must declare to be
     *               collected.
     * @returns Every row whose structure wiring selects @p source, in
     *          discovery order.
     */
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

    /**
     * @brief Resolves @p name's value for @p sourceRow -- the deepest
     *        structure-wiring binding of that name, or, absent one, the
     *        row's own column value.
     *
     * @param model     The model @p sourceRow belongs to.
     * @param sourceRow The row @p name is resolved against.
     * @param name      The token or column name to resolve.
     * @returns The resolved value, or an empty string when neither a
     *          binding nor a column named @p name exists on @p sourceRow.
     */
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

    /**
     * @brief Renders @p shapeId's text for one item -- @p sourceRow's
     *        binding and column values when @p sourceRow is not
     *        @c nullptr, or @p sourceValue substituted into @p shapeId's
     *        @p sourceKey token alone when it is.
     *
     * @param model            The model @p sourceRow, when present,
     *                         belongs to.
     * @param templateDocument The template document @p shapeId is read
     *                         from.
     * @param sourceRow        The item's source row, or @c nullptr when the
     *                         item is a bare column value.
     * @param sourceValue      The column value substituted into
     *                         @p sourceKey when @p sourceRow is @c nullptr.
     * @param sourceKey        The token name @p sourceValue is substituted
     *                         into when @p sourceRow is @c nullptr.
     * @param shapeId          The shape to render.
     * @returns The item's rendered text.
     */
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

    /**
     * @brief Renders @p shapeId's text for every row in @p sourceRows,
     *        then for every value in @p sourceValues, dropping any item
     *        whose rendered text is empty.
     *
     * @param model            The model @p sourceRows belong to.
     * @param templateDocument The template document @p shapeId is read
     *                         from.
     * @param sourceRows       The binding-selected source rows to render.
     * @param sourceValues     The column source values to render.
     * @param sourceKey        The token name each of @p sourceValues is
     *                         substituted into.
     * @param shapeId          The shape to render.
     * @returns Every non-empty rendered item, in the order @p sourceRows
     *          then @p sourceValues were authored.
     */
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

    /**
     * @brief Discovers @p source's items -- an address's table rows, a
     *        column name's distinct values across every output table, or a
     *        blank binding's selected rows -- and renders each through
     *        @p shapeId.
     *
     * @param model            The model @p row belongs to.
     * @param templateDocument The template document @p shapeId is read
     *                         from.
     * @param row              The row @p source is resolved against.
     * @param source           The placeholder list's authored source: an
     *                         @-sigiled address, a column name, or a
     *                         blank binding name.
     * @param token            The token @p source's list feeds, used as
     *                         the substitution key when @p source names a
     *                         column.
     * @param shapeId          The shape to render each item through.
     * @returns Every non-empty rendered item.
     */
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

    /**
     * @brief Renders @p source's items through getItems() and joins them
     *        by @p token's authored separator, indenting every joined line
     *        by @p indent.
     *
     * @param model            The model @p row belongs to.
     * @param templateDocument The template document the items' shape is
     *                         read from.
     * @param row              The row @p source is resolved against.
     * @param token            The placeholder token @p source's list feeds.
     * @param source           The placeholder list's authored source.
     * @param shapeId          The shape to render each item through.
     * @param separatorScope   The blockquote scope carrying @p token's
     *                         separator binding, or @c nullptr.
     * @param indent           The indentation prefix applied to every
     *                         joined line.
     * @returns The joined, indented item text, or an empty string when no
     *          item rendered.
     */
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
