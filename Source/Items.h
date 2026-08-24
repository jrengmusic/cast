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
     * @brief Resolves @p name's value for one item -- @p sourceRow's
     *        binding or column value through getSourceValue() when
     *        @p sourceRow is not @c nullptr, or @p sourceValue itself when
     *        @p name matches @p sourceKey.
     *
     * @param model       The model @p sourceRow, when present, belongs to.
     * @param sourceRow   The item's source row, or @c nullptr when the item
     *                    is a bare column value.
     * @param sourceValue The column value returned when @p sourceRow is
     *                    @c nullptr and @p name matches @p sourceKey.
     * @param sourceKey   The token name @p sourceValue answers for when
     *                    @p sourceRow is @c nullptr.
     * @param name        The token or column name to resolve.
     * @returns The resolved value, or an empty string when neither
     *          @p sourceRow nor @p sourceKey resolves @p name.
     */
    static juce::String getColumnValue (const Model& model,
                                        Element* sourceRow,
                                        const juce::String& sourceValue,
                                        const juce::Identifier& sourceKey,
                                        const juce::Identifier& name)
    {
        if (sourceRow != nullptr)
            return getSourceValue (model, *sourceRow, name);

        return name == sourceKey ? sourceValue : juce::String();
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

        for (const auto& name : tokens)
            if (jam::Format::hasPlaceholder (itemText, name.toString()))
                itemText = jam::Format::replaceholder (itemText,
                    name.toString(), getColumnValue (model, sourceRow, sourceValue, sourceKey, name));

        return itemText;
    }

    /**
     * @brief Answers whether @p itemText, with trailing whitespace trimmed,
     *        carries no newline.
     *
     * @param itemText The rendered item text to check.
     * @returns @c true when @p itemText spans a single line.
     */
    static bool isSingleLineShape (const juce::String& itemText) noexcept
    {
        return not itemText.trimEnd().containsChar (Chars::newline);
    }

    /**
     * @brief Measures, in UTF-8 bytes, the widest resolved value of every
     *        one of @p shapeId's placeholder tokens across @p sourceRows
     *        and @p sourceValues.
     *
     * @param model            The model @p sourceRows belong to.
     * @param templateDocument The template document @p shapeId is read
     *                         from.
     * @param sourceRows       The binding-selected source rows measured.
     * @param sourceValues     The column source values measured.
     * @param sourceKey        The token name each of @p sourceValues is
     *                         resolved against.
     * @param shapeId          The shape whose placeholder tokens are
     *                         measured.
     * @returns Every placeholder token mapped to its widest resolved
     *          value's byte width.
     */
    static jam::HashMap<juce::Identifier, size_t>
    getMeasuredWidths (const Model& model,
                       const TemplateDocument& templateDocument,
                       const jam::Array<Element*>& sourceRows,
                       const jam::Strings& sourceValues,
                       const juce::Identifier& sourceKey,
                       const juce::Identifier& shapeId)
    {
        const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };

        jam::HashMap<juce::Identifier, size_t> columnWidths;

        const auto measureItem = [&model, &tokens, &sourceKey, &columnWidths] (
            Element* sourceRow, const juce::String& sourceValue)
        {
            jam::HashMap<juce::Identifier, juce::String> itemReplacements;

            for (const auto& name : tokens)
                itemReplacements.emplace (
                    name, getColumnValue (model, sourceRow, sourceValue, sourceKey, name));

            for (const auto& [name, value] : itemReplacements)
            {
                const auto tokenWidth { static_cast<size_t> (value.getNumBytesAsUTF8()) };

                if (columnWidths[name] < tokenWidth)
                    columnWidths[name] = tokenWidth;
            }
        };

        for (auto* sourceRow : sourceRows)
            measureItem (sourceRow, {});

        for (const auto& value : sourceValues)
            measureItem (nullptr, value);

        return columnWidths;
    }

    /**
     * @brief Widens @p literal's first run of spaces by @p fillWidth
     *        spaces, or, when @p literal carries no space run, appends
     *        @p fillWidth spaces to its end.
     *
     * @param literal   The literal text between two consecutive
     *                  placeholders.
     * @param fillWidth The number of spaces to insert.
     * @returns @p literal, widened by @p fillWidth spaces.
     */
    static juce::String getFilledLiteral (const juce::String& literal, size_t fillWidth)
    {
        const auto fill { juce::String::repeatedString (juce::String::charToString (Chars::space),
                                                         static_cast<int> (fillWidth)) };

        for (int index { 0 }; index < literal.length(); ++index)
            if (literal[index] == Chars::space)
            {
                int runEnd { index };

                while (runEnd < literal.length() and literal[runEnd] == Chars::space)
                    ++runEnd;

                return literal.substring (0, runEnd) + fill + literal.substring (runEnd);
            }

        return literal + fill;
    }

    /**
     * @brief Returns @p name's authored @c :::token::: placeholder marker.
     *
     * @param name The token name to wrap.
     * @returns @p name, wrapped in triple-colon markers.
     */
    static juce::String getMarker (const juce::Identifier& name)
    {
        return Id::tripleColon + name.toString() + Id::tripleColon;
    }

    /**
     * @brief Renders @p shapeId's text for one item, widening the literal
     *        text before each placeholder but the first so every column's
     *        values, across the caller's items, align at @p columnWidths.
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
     * @param columnWidths     Every placeholder token's widest resolved
     *                         value width, from getMeasuredWidths().
     * @returns The item's rendered, column-aligned text.
     */
    static juce::String getPaddedItem (const Model& model,
                                       const TemplateDocument& templateDocument,
                                       Element* sourceRow,
                                       const juce::String& sourceValue,
                                       const juce::Identifier& sourceKey,
                                       const juce::Identifier& shapeId,
                                       const jam::HashMap<juce::Identifier, size_t>& columnWidths)
    {
        const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };
        auto itemText { *templateDocument.getCodeBlock (shapeId)->get<juce::String> (Id::value) };

        juce::String paddedText;
        int cursor { 0 };
        auto isFirstToken { true };
        juce::Identifier previousName;
        juce::String previousValue;

        for (;;)
        {
            juce::Identifier bestName;
            int bestPosition { -1 };

            for (const auto& name : tokens)
            {
                const auto marker { getMarker (name) };
                const auto position { itemText.indexOf (cursor, marker) };

                if (position >= 0 and (bestPosition < 0 or position < bestPosition))
                {
                    bestName = name;
                    bestPosition = position;
                }
            }

            if (bestPosition < 0)
                break;

            const auto marker { getMarker (bestName) };
            const auto position { bestPosition };
            const auto literal { itemText.substring (cursor, position) };
            const auto columnValue { getColumnValue (model, sourceRow, sourceValue, sourceKey, bestName) };

            paddedText += isFirstToken
                              ? literal
                              : getFilledLiteral (literal,
                                    columnWidths.at (previousName)
                                        - static_cast<size_t> (previousValue.getNumBytesAsUTF8()));
            paddedText += columnValue;

            previousName = bestName;
            previousValue = columnValue;
            cursor = position + marker.length();
            isFirstToken = false;
        }

        return paddedText + itemText.substring (cursor);
    }

    /**
     * @brief Renders @p shapeId's text for every row in @p sourceRows,
     *        then for every value in @p sourceValues, dropping any item
     *        whose rendered text is empty.
     *
     * When @p shapeId is a single-line shape and more than one item is
     * rendered, every item is column-aligned through getPaddedItem();
     * otherwise each item is rendered through getItem().
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

        const auto usePadding { isSingleLineShape (
                                     *templateDocument.getCodeBlock (shapeId)->get<juce::String> (
                                         Id::value))
                                 and sourceRows.size() + sourceValues.size() > 1 };
        jam::HashMap<juce::Identifier, size_t> columnWidths;

        if (usePadding)
            columnWidths = getMeasuredWidths (
                model, templateDocument, sourceRows, sourceValues, sourceKey, shapeId);

        const auto renderItem = [&model, &templateDocument, &sourceKey, &shapeId, &columnWidths,
                                 usePadding, &texts] (Element* sourceRow, const juce::String& sourceValue)
        {
            const auto itemText { usePadding
                                       ? getPaddedItem (model, templateDocument, sourceRow,
                                             sourceValue, sourceKey, shapeId, columnWidths)
                                       : getItem (
                                             model, templateDocument, sourceRow, sourceValue, sourceKey, shapeId) };

            if (itemText.isNotEmpty())
                texts.add (itemText);
        };

        for (auto* sourceRow : sourceRows)
            renderItem (sourceRow, {});

        for (const auto& value : sourceValues)
            renderItem (nullptr, value);

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
