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

                    if (auto* scope { model.getTableCell (*candidate, Id::structure) })
                        scope->applyFunctionRecursively (
                            [&matches, &source] (const Element& item) -> bool
                            {
                                if (item.parent != nullptr and item.parent->isTag (Id::ul)
                                    and item.id == source)
                                    matches = true;

                                return not matches;
                            });

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

        if (auto* scope { model.getTableCell (sourceRow, Id::structure) })
            scope->applyFunctionRecursively (
                [&deepestValue, &name] (const Element& item) -> bool
                {
                    if (item.parent != nullptr and item.parent->isTag (Id::ul) and item.id == name
                        and item.id != Id::list)
                        deepestValue = *item.get<juce::String> (Id::value);

                    return true;
                });

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

    static juce::String getChildValue (Element* sourceRow,
                                       const juce::String& childSource,
                                       const juce::String& childJoin,
                                       const jam::Document::Identifiers& tokens)
    {
        if (sourceRow != nullptr and childSource == Id::cells.toString())
            return getCells (*sourceRow, tokens).joinIntoString (childJoin, 0, -1);

        return {};
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
     * @param childSource      The shape's own nested @c - list: source
     *                         word, or an empty string when it owns no
     *                         child expansion.
     * @param childJoin        The child expansion's join text.
     * @param extension        The target file extension the @c Id::comment
     *                         token's value is commented for.
     * @returns The item's rendered text.
     */
    static juce::String getItem (const Model& model,
                                 const TemplateDocument& templateDocument,
                                 Element* sourceRow,
                                 const juce::String& sourceValue,
                                 const juce::Identifier& sourceKey,
                                 const juce::Identifier& shapeId,
                                 const juce::String& childSource,
                                 const juce::String& childJoin,
                                 const juce::String& extension)
    {
        const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };
        auto itemText { *templateDocument.getCodeBlock (shapeId)->get<juce::String> (Id::value) };

        for (const auto& name : tokens)
            if (jam::Format::hasPlaceholder (itemText, name.toString()))
            {
                auto value { name == Id::list
                                 ? getChildValue (sourceRow, childSource, childJoin, tokens)
                                 : getColumnValue (model, sourceRow, sourceValue, sourceKey, name) };

                if (name == Id::comment and value.isNotEmpty())
                    value = Transforms::toComment (value, extension);

                itemText = jam::Format::replaceholder (itemText, name.toString(), value);
            }

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
     * @brief Fills @p itemReplacements with every item's resolved token
     *        replacements, and @p columnWidths with each placeholder
     *        token's widest resolved value width in UTF-8 bytes, across
     *        @p sourceRows and @p sourceValues, in one pass.
     *
     * @param itemReplacements Filled with one entry per item, mapping each
     *                         of @p shapeId's placeholder tokens to its
     *                         resolved value.
     * @param columnWidths     Filled with every placeholder token's widest
     *                         resolved value width across all items.
     * @param model            The model @p sourceRows belong to.
     * @param templateDocument The template document @p shapeId is read
     *                         from.
     * @param sourceRows       The binding-selected source rows measured.
     * @param sourceValues     The column source values measured.
     * @param sourceKey        The token name each of @p sourceValues is
     *                         resolved against.
     * @param shapeId          The shape whose placeholder tokens are
     *                         measured.
     * @param extension        The target file extension the @c Id::comment
     *                         token's value is commented for.
     */
    static void
    addItemReplacements (jam::Array<jam::HashMap<juce::Identifier, juce::String>>& itemReplacements,
                       jam::HashMap<juce::Identifier, size_t>& columnWidths,
                       const Model& model,
                       const TemplateDocument& templateDocument,
                       const jam::Array<Element*>& sourceRows,
                       const jam::Strings& sourceValues,
                       const juce::Identifier& sourceKey,
                       const juce::Identifier& shapeId,
                       const juce::String& extension)
    {
        const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };

        const auto measureItem = [&model, &tokens, &sourceKey, &columnWidths, &itemReplacements,
                                  &extension] (Element* sourceRow, const juce::String& sourceValue)
        {
            itemReplacements.add ({});
            auto& replacements { itemReplacements.last() };

            for (const auto& name : tokens)
            {
                auto value { getColumnValue (model, sourceRow, sourceValue, sourceKey, name) };

                if (name == Id::comment and value.isNotEmpty())
                    value = Transforms::toComment (value, extension);

                replacements.emplace (name, value);
            }

            for (const auto& [name, value] : replacements)
            {
                const auto tokenWidth { static_cast<size_t> (value.getNumBytesAsUTF8()) };
                auto& columnWidth { columnWidths[name] };

                if (columnWidth < tokenWidth)
                    columnWidth = tokenWidth;
            }
        };

        for (auto* sourceRow : sourceRows)
            measureItem (sourceRow, {});

        for (const auto& value : sourceValues)
            measureItem (nullptr, value);
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
     * @param templateDocument The template document @p shapeId is read
     *                         from.
     * @param shapeId          The shape to render.
     * @param replacements     The item's already-resolved placeholder
     *                         token values, from addItemReplacements().
     * @param columnWidths     Every placeholder token's widest resolved
     *                         value width, from addItemReplacements().
     * @returns The item's rendered, column-aligned text.
     */
    static juce::String getPaddedItem (const TemplateDocument& templateDocument,
                                       const juce::Identifier& shapeId,
                                       const jam::HashMap<juce::Identifier, juce::String>& replacements,
                                       const jam::HashMap<juce::Identifier, size_t>& columnWidths)
    {
        const auto& itemText { *templateDocument.getCodeBlock (shapeId)->get<juce::String> (Id::value) };

        std::string paddedText;
        paddedText.reserve (static_cast<size_t> (itemText.getNumBytesAsUTF8()));

        int cursor { 0 };
        auto isFirstToken { true };
        juce::Identifier previousName;
        juce::String previousValue;

        for (auto markerStart { itemText.indexOf (cursor, Id::tripleColon) };
             markerStart >= 0;
             markerStart = itemText.indexOf (cursor, Id::tripleColon))
        {
            const auto nameStart { markerStart + Id::tripleColon.length() };
            const auto nameEnd { itemText.indexOf (nameStart, Id::tripleColon) };
            const auto name { juce::Identifier (itemText.substring (nameStart, nameEnd)) };
            const auto literal { itemText.substring (cursor, markerStart) };
            const auto columnValue { replacements.at (name) };

            if (isFirstToken)
                paddedText.append (literal.toRawUTF8(),
                                   static_cast<size_t> (literal.getNumBytesAsUTF8()));
            else
            {
                const auto filledLiteral { getFilledLiteral (literal,
                    columnWidths.at (previousName)
                        - static_cast<size_t> (previousValue.getNumBytesAsUTF8())) };
                paddedText.append (filledLiteral.toRawUTF8(),
                                   static_cast<size_t> (filledLiteral.getNumBytesAsUTF8()));
            }

            paddedText.append (columnValue.toRawUTF8(),
                               static_cast<size_t> (columnValue.getNumBytesAsUTF8()));

            previousName = name;
            previousValue = columnValue;
            cursor = nameEnd + Id::tripleColon.length();
            isFirstToken = false;
        }

        const auto trailingText { itemText.substring (cursor) };
        paddedText.append (trailingText.toRawUTF8(),
                           static_cast<size_t> (trailingText.getNumBytesAsUTF8()));

        return juce::String::fromUTF8 (paddedText.data(), static_cast<int> (paddedText.size()));
    }

    /**
     * @brief Renders @p shapeId's text for every row in @p sourceRows,
     *        then for every value in @p sourceValues, column-aligning
     *        every item's placeholder values at each token's widest
     *        resolved width, dropping any item whose rendered text is
     *        empty.
     *
     * @param model            The model @p sourceRows belong to.
     * @param templateDocument The template document @p shapeId is read
     *                         from.
     * @param sourceRows       The binding-selected source rows to render.
     * @param sourceValues     The column source values to render.
     * @param sourceKey        The token name each of @p sourceValues is
     *                         substituted into.
     * @param shapeId          The shape to render.
     * @param extension        The target file extension the @c Id::comment
     *                         token's value is commented for.
     * @returns Every non-empty rendered item, in the order @p sourceRows
     *          then @p sourceValues were authored.
     */
    static jam::Strings getPaddedItemTexts (const Model& model,
                                            const TemplateDocument& templateDocument,
                                            const jam::Array<Element*>& sourceRows,
                                            const jam::Strings& sourceValues,
                                            const juce::Identifier& sourceKey,
                                            const juce::Identifier& shapeId,
                                            const juce::String& extension)
    {
        jam::Strings texts;

        jam::Array<jam::HashMap<juce::Identifier, juce::String>> itemReplacements;
        jam::HashMap<juce::Identifier, size_t> columnWidths;
        addItemReplacements (itemReplacements, columnWidths, model, templateDocument,
            sourceRows, sourceValues, sourceKey, shapeId, extension);

        for (int index { 0 }; index < itemReplacements.size(); ++index)
        {
            const auto itemText { getPaddedItem (
                templateDocument, shapeId, itemReplacements.at (index), columnWidths) };

            if (itemText.isNotEmpty())
                texts.add (itemText);
        }

        return texts;
    }

    /**
     * @brief Renders @p shapeId's text for every row in @p sourceRows,
     *        then for every value in @p sourceValues, each item rendered
     *        independently through getItem(), dropping any item whose
     *        rendered text is empty.
     *
     * @param model            The model @p sourceRows belong to.
     * @param templateDocument The template document @p shapeId is read
     *                         from.
     * @param sourceRows       The binding-selected source rows to render.
     * @param sourceValues     The column source values to render.
     * @param sourceKey        The token name each of @p sourceValues is
     *                         substituted into.
     * @param shapeId          The shape to render.
     * @param childSource      The shape's own nested @c - list: source
     *                         word, or an empty string when it owns no
     *                         child expansion.
     * @param childJoin        The child expansion's join text.
     * @param extension        The target file extension the @c Id::comment
     *                         token's value is commented for.
     * @returns Every non-empty rendered item, in the order @p sourceRows
     *          then @p sourceValues were authored.
     */
    static jam::Strings getPlainItemTexts (const Model& model,
                                           const TemplateDocument& templateDocument,
                                           const jam::Array<Element*>& sourceRows,
                                           const jam::Strings& sourceValues,
                                           const juce::Identifier& sourceKey,
                                           const juce::Identifier& shapeId,
                                           const juce::String& childSource,
                                           const juce::String& childJoin,
                                           const juce::String& extension)
    {
        jam::Strings texts;

        const auto renderItem = [&model, &templateDocument, &sourceKey, &shapeId, &childSource,
                                 &childJoin, &extension, &texts] (
            Element* sourceRow, const juce::String& sourceValue)
        {
            const auto itemText { getItem (model, templateDocument, sourceRow, sourceValue,
                sourceKey, shapeId, childSource, childJoin, extension) };

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
     * @brief Renders @p shapeId's text for @p sourceRows and
     *        @p sourceValues, dispatching to getPaddedItemTexts() when
     *        @p shapeId is a single-line shape rendering more than one
     *        item with no @c list token, or to getPlainItemTexts()
     *        otherwise.
     *
     * @param model            The model @p sourceRows belong to.
     * @param templateDocument The template document @p shapeId is read
     *                         from.
     * @param sourceRows       The binding-selected source rows to render.
     * @param sourceValues     The column source values to render.
     * @param sourceKey        The token name each of @p sourceValues is
     *                         substituted into.
     * @param shapeId          The shape to render.
     * @param childSource      The shape's own nested @c - list: source
     *                         word, or an empty string when it owns no
     *                         child expansion.
     * @param childJoin        The child expansion's join text.
     * @param extension        The target file extension the @c Id::comment
     *                         token's value is commented for.
     * @returns Every non-empty rendered item, in the order @p sourceRows
     *          then @p sourceValues were authored.
     */
    static jam::Strings getItemTexts (const Model& model,
                                      const TemplateDocument& templateDocument,
                                      const jam::Array<Element*>& sourceRows,
                                      const jam::Strings& sourceValues,
                                      const juce::Identifier& sourceKey,
                                      const juce::Identifier& shapeId,
                                      const juce::String& childSource,
                                      const juce::String& childJoin,
                                      const juce::String& extension)
    {
        const auto& shapeText { *templateDocument.getCodeBlock (shapeId)->get<juce::String> (Id::value) };
        const auto usePadding { isSingleLineShape (shapeText)
                                 and sourceRows.size() + sourceValues.size() > 1
                                 and not jam::Format::hasPlaceholder (shapeText, Id::list.toString()) };

        if (usePadding)
            return getPaddedItemTexts (
                model, templateDocument, sourceRows, sourceValues, sourceKey, shapeId, extension);

        return getPlainItemTexts (model, templateDocument, sourceRows, sourceValues, sourceKey,
            shapeId, childSource, childJoin, extension);
    }

    static jam::Strings getCells (Element& row, const jam::Document::Identifiers& tokens)
    {
        jam::Strings cells;

        for (auto* cell : row)
            if (cell->id != Id::format and cell->id != Id::comment and not tokens.contains (cell->id))
                cells.add (*cell->get<juce::String> (Id::value));

        return cells;
    }

    static jam::Strings getItems (const Model& model,
                                  const TemplateDocument& templateDocument,
                                  const jam::Array<Element*>& tables,
                                  Element& row,
                                  const juce::String& source,
                                  const juce::Identifier& shapeId,
                                  const juce::String& childSource,
                                  const juce::String& childJoin,
                                  const juce::String& extension)
    {
        jam::Array<Element*> sourceRows;
        jam::Strings sourceValues;
        juce::Identifier sourceKey { Id::list };

        if (source == Id::cells.toString())
        {
            const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                       ->get<jam::Document::Identifiers> (Id::placeholder) };
            sourceValues = getCells (row, tokens);
        }
        else if (auto* sourceTable { model.getTable (row, source) }; sourceTable != nullptr)
        {
            sourceRows = model.getTableRows (*sourceTable);
        }
        else
        {
            const auto sourceName { juce::Identifier (source) };
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

        return getItemTexts (model, templateDocument, sourceRows, sourceValues, sourceKey, shapeId,
            childSource, childJoin, extension);
    }

    static juce::String getJoinedItems (const Model& model,
                                        const TemplateDocument& templateDocument,
                                        const jam::Array<Element*>& tables,
                                        Element& row,
                                        const juce::String& source,
                                        const juce::Identifier& shapeId,
                                        const juce::String& separator,
                                        const juce::String& childSource,
                                        const juce::String& childJoin,
                                        const juce::String& extension)
    {
        const auto texts { getItems (
            model, templateDocument, tables, row, source, shapeId, childSource, childJoin, extension) };

        return texts.joinIntoString (separator, 0, -1);
    }
};
