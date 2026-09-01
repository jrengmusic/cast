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
     * @brief Returns @p sourceTable's own rows, excluding a row whose
     *        @c file column value equals @p row's own file -- a file
     *        never lists itself -- and, when @p source is a cell-match
     *        filter address, excluding a row whose @p source's own
     *        filter column value does not equal its filter value (SPEC
     *        §6.5).
     *
     * @param model       The model @p row and @p sourceTable belong to.
     * @param row         The row whose own file's value is excluded.
     * @param sourceTable The table whose rows are collected.
     * @param source      The item source's own address, read for its
     *                     optional cell-match filter.
     * @returns Every row of @p sourceTable whose own @c file column value,
     *          when present, differs from @p row's own file, and whose
     *          filter column value, when @p source names one, equals the
     *          filter value.
     */
    static jam::Array<Element*> getTableSourceRows (const Model& model,
                                                     Element& row,
                                                     Element& sourceTable,
                                                     const juce::String& source)
    {
        const auto currentFile { jam::Format::toFileName (model.getValue (row, Id::file)) };
        const auto isFiltered { Model::isFilteredAddress (source) };
        const auto filterColumn { isFiltered ? Model::getFilterColumn (source) : juce::Identifier{} };
        const auto filterValue { isFiltered ? Model::getFilterValue (source) : juce::String{} };
        jam::Array<Element*> sourceRows;

        for (auto* candidate : model.getTableRows (sourceTable))
        {
            auto* fileCell { model.getTableCell (*candidate, Id::file) };
            const auto candidateFile { fileCell != nullptr
                                           ? jam::Format::toFileName (
                                                 *fileCell->get<juce::String> (Id::value))
                                           : juce::String{} };
            const auto matchesFilter { not isFiltered
                or *model.getTableCell (*candidate, filterColumn)->get<juce::String> (Id::value)
                       == filterValue };

            if (candidateFile != currentFile and matchesFilter)
                sourceRows.add (candidate);
        }

        return sourceRows;
    }

    /**
     * @brief Returns @p line's own arity -- its shape's count of
     *        @c :::list::: occurrences.
     *
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param line             The structure line whose arity is counted.
     * @returns @p line's own shape's @c :::list::: occurrence count.
     */
    static int getArity (const TemplateDocument& templateDocument, Element& line)
    {
        const auto& tokens { *templateDocument.getCodeBlock (line)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };

        return static_cast<int> (std::count (tokens.begin(), tokens.end(), Id::list));
    }

    /**
     * @brief Collects @p row's own structure wiring's shape-valued
     *        bindings' consumed shape ordinals -- every shape ordinal
     *        transitively consumed by a wrapper, walking each consumed
     *        source's own arity-bounded chain in turn (SPEC §6.1, §6.4).
     *
     * @param model            The model @p row belongs to.
     * @param templateDocument The template document each consumed
     *                         source's own arity is read from.
     * @param row              The row whose shape-valued bindings' private
     *                         render data is collected.
     * @returns Every shape ordinal privately consumed by one of @p row's
     *          own shape-valued bindings, in discovery order.
     */
    static jam::Array<int> getPrivateShapes (const Model& model,
                                             const TemplateDocument& templateDocument,
                                             Element& row)
    {
        jam::Array<int> privateShapes;
        const auto arityOf = [&templateDocument] (Element& line) { return getArity (templateDocument, line); };

        if (auto* scope { model.getTableCell (row, Id::structure) })
            scope->applyFunctionRecursively (
                [&model, &privateShapes, &arityOf] (const Element& candidate) -> bool
                {
                    if (candidate.parent->isTag (Id::ul) and candidate.id != Id::list
                        and candidate.contains (Id::templatePath))
                    {
                        auto& line { const_cast<Element&> (candidate) };
                        auto* cursor { model.getNextLine (line) };

                        for (int occurrence { 0 };
                             cursor != nullptr and occurrence < arityOf (line); ++occurrence)
                        {
                            privateShapes.addIfNotAlreadyThere (*cursor->get<int> (Id::shape));
                            cursor = model.getNextShapeLine (*cursor, arityOf);
                        }
                    }

                    return true;
                });

        return privateShapes;
    }

    /**
     * @brief Collects every output row across @p tables whose structure
     *        wiring, at any depth outside a shape-valued binding's own
     *        private render data, declares @p source as a blank-binding
     *        selector.
     *
     * @param model            The model @p tables belong to.
     * @param templateDocument The template document each candidate row's
     *                         private render data is read through.
     * @param tables           The tables searched for rows selecting
     *                         @p source.
     * @param source           The blank binding name a row must declare to
     *                         be collected.
     * @returns Every row whose structure wiring selects @p source, in
     *          discovery order.
     */
    static jam::Array<Element*> getBindingSourceRows (const Model& model,
                                                       const TemplateDocument& templateDocument,
                                                       const jam::Array<Element*>& tables,
                                                       const juce::Identifier& source)
    {
        jam::Array<Element*> sourceRows;

        for (auto* table : tables)
            if (model.isOutputTable (*table))
                for (auto* candidate : model.getTableRows (*table))
                {
                    auto matches { false };
                    const auto privateShapes { getPrivateShapes (model, templateDocument, *candidate) };

                    if (auto* scope { model.getTableCell (*candidate, Id::structure) })
                        scope->applyFunctionRecursively (
                            [&matches, &source, &privateShapes] (const Element& item) -> bool
                            {
                                if (item.parent->isTag (Id::ul) and item.id == source
                                    and item.id != Id::list
                                    and not privateShapes.contains (*item.get<int> (Id::shape)))
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
     *        structure-wiring binding of that name outside a shape-valued
     *        binding's own private render data, or, absent one, the
     *        row's own column value. An @-sigiled @c comment column value
     *        is a reference, not prose (SPEC §4), and resolves empty absent
     *        a binding.
     *
     * @param model            The model @p sourceRow belongs to.
     * @param templateDocument The template document @p sourceRow's own
     *                         private render data is read through.
     * @param sourceRow        The row @p name is resolved against.
     * @param name             The token or column name to resolve.
     * @returns The resolved value, or an empty string when neither a
     *          binding nor a column named @p name exists on @p sourceRow, or
     *          @p name is @c comment and the column value is a reference.
     */
    static juce::String getSourceValue (const Model& model, const TemplateDocument& templateDocument,
        Element& sourceRow, const juce::Identifier& name)
    {
        juce::String deepestValue;
        const auto privateShapes { getPrivateShapes (model, templateDocument, sourceRow) };

        if (auto* scope { model.getTableCell (sourceRow, Id::structure) })
            scope->applyFunctionRecursively (
                [&deepestValue, &name, &privateShapes] (const Element& item) -> bool
                {
                    if (item.parent->isTag (Id::ul) and item.id == name and item.id != Id::list
                        and not privateShapes.contains (*item.get<int> (Id::shape)))
                        deepestValue = *item.get<juce::String> (Id::value);

                    return true;
                });

        if (deepestValue.isNotEmpty())
            return deepestValue;

        juce::String columnValue;

        if (auto* cell { model.getTableCell (sourceRow, name) })
            columnValue = *cell->get<juce::String> (Id::value);

        if (name == Id::comment and Model::isAddress (columnValue))
            return {};

        return name == Id::file ? jam::Format::toFileName (columnValue) : columnValue;
    }

    /**
     * @brief Resolves @p name's value for one item -- @p sourceRow's
     *        binding or column value through getSourceValue() when
     *        @p sourceRow is not @c nullptr, or @p sourceValue itself when
     *        @p name matches @p sourceKey.
     *
     * @param model       The model @p sourceRow, when present, belongs to.
     * @param templateDocument The template document @p sourceRow's own
     *                         private render data is read through, when
     *                         present.
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
                                        const TemplateDocument& templateDocument,
                                        Element* sourceRow,
                                        const juce::String& sourceValue,
                                        const juce::Identifier& sourceKey,
                                        const juce::Identifier& name)
    {
        if (sourceRow != nullptr)
            return getSourceValue (model, templateDocument, *sourceRow, name);

        return name == sourceKey ? sourceValue : juce::String{};
    }

    /**
     * @brief Resolves the @c list token's value for one item -- every
     *        column-address child line authored directly beneath
     *        @p sourceOrdinal, read from @p sourceRow's own column, joined
     *        by @p childJoin.
     *
     * @param model         The model @p row and @p sourceRow belong to.
     * @param row           The structure row @p sourceOrdinal's child
     *                      column addresses are read from.
     * @param indent        The depth @p sourceOrdinal's child column
     *                      addresses are read from.
     * @param sourceOrdinal The structure position after which child column
     *                      addresses are read.
     * @param sourceRow     The item's source row, or @c nullptr when the
     *                      item is a bare column value -- no child value
     *                      resolves.
     * @param childJoin     The child values' join text.
     * @returns Every resolved child column value, in authored order,
     *          joined by @p childJoin.
     */
    static juce::String getChildValue (const Model& model, Element& row, int indent,
                                       int sourceOrdinal, Element* sourceRow,
                                       const juce::String& childJoin)
    {
        jam::Strings childValues;

        if (sourceRow != nullptr)
            for (int childOrdinal { sourceOrdinal + 1 };; ++childOrdinal)
            {
                auto* childSourceLine { model.getSource (row, indent, childOrdinal) };

                if (childSourceLine == nullptr)
                    break;

                const auto& value { *childSourceLine->get<juce::String> (Id::value) };

                if (not Model::isColumnAddress (value))
                    break;

                if (auto* cell { model.getTableCell (*sourceRow, Model::getColumn (value)) })
                    childValues.add (*cell->get<juce::String> (Id::value));
            }

        return childValues.joinIntoString (childJoin, 0, -1);
    }

    /**
     * @brief Renders one item's own plain (unpadded) text -- @p line's
     *        own shape with every placeholder token substituted by its
     *        resolved value, the @c list token filled by getChildValue()
     *        and every other token by getColumnValue(), commenting a
     *        non-empty @c comment value for @p extension.
     *
     * @param model            The model @p sourceRow and @p row belong
     *                         to.
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param sourceRow        The item's source row, or @c nullptr when
     *                         the item is a bare column value.
     * @param sourceValue      The column value resolved when @p sourceRow
     *                         is @c nullptr.
     * @param sourceKey        The token name @p sourceValue answers for.
     * @param line             The structure line whose shape is rendered.
     * @param row              The structure row @p sourceOrdinal's child
     *                         column addresses are read from.
     * @param indent           The depth @p sourceOrdinal's child column
     *                         addresses are read from.
     * @param sourceOrdinal    The structure position after which child
     *                         column addresses are read.
     * @param childJoin        The @c list token's own child values' join
     *                         text.
     * @param extension        The target file extension a comment value
     *                         is commented for.
     * @returns The rendered item text.
     */
    static juce::String getItem (const Model& model,
                                 const TemplateDocument& templateDocument,
                                 Element* sourceRow,
                                 const juce::String& sourceValue,
                                 const juce::Identifier& sourceKey,
                                 Element& line,
                                 Element& row,
                                 int indent,
                                 int sourceOrdinal,
                                 const juce::String& childJoin,
                                 const juce::String& extension)
    {
        const auto& tokens { *templateDocument.getCodeBlock (line)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };
        auto itemText { *templateDocument.getCodeBlock (line)->get<juce::String> (Id::value) };

        for (const auto& name : tokens)
        {
            const auto marker { getMarker (itemText, name) };

            if (marker.isNotEmpty())
            {
                auto value { name == Id::list
                                 ? getChildValue (model, row, indent, sourceOrdinal, sourceRow, childJoin)
                                 : getColumnValue (
                                       model, templateDocument, sourceRow, sourceValue, sourceKey, name) };

                if (name == Id::comment and value.isNotEmpty())
                    value = Transforms::toComment (value, extension);

                itemText = itemText.replace (marker, value);
            }
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
     * @brief Returns one replacement map per item -- @p sourceRows' own
     *        source rows followed by @p sourceValues' own column values,
     *        each mapping @p line's own placeholder tokens to their
     *        resolved values.
     *
     * @param model            The model @p sourceRows and @p line belong
     *                         to.
     * @param templateDocument The template document @p line's shape and
     *                         placeholder tokens are read from.
     * @param sourceRows       The item source rows to resolve.
     * @param sourceValues     The item column values to resolve.
     * @param sourceKey        The token name each of @p sourceValues
     *                         answers for.
     * @param line             The structure line whose placeholder tokens
     *                         are resolved.
     * @param extension        The target file extension a comment value is
     *                         commented for.
     * @returns One replacement map per item, in @p sourceRows then
     *          @p sourceValues order.
     */
    static jam::Array<jam::HashMap<juce::Identifier, juce::String>> getItemReplacements (
        const Model& model,
        const TemplateDocument& templateDocument,
        const jam::Array<Element*>& sourceRows,
        const jam::Strings& sourceValues,
        const juce::Identifier& sourceKey,
        Element& line,
        const juce::String& extension)
    {
        jam::Array<jam::HashMap<juce::Identifier, juce::String>> itemReplacements;
        const auto& tokens { *templateDocument.getCodeBlock (line)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };

        const auto measureItem = [&model, &templateDocument, &tokens, &sourceKey, &itemReplacements,
                                  &extension] (Element* sourceRow, const juce::String& sourceValue)
        {
            itemReplacements.add ({});
            auto& replacements { itemReplacements.last() };

            for (const auto& name : tokens)
            {
                auto value {
                    getColumnValue (model, templateDocument, sourceRow, sourceValue, sourceKey, name)
                };

                if (name == Id::comment and value.isNotEmpty())
                    value = Transforms::toComment (value, extension);

                replacements.emplace (name, value);
            }
        };

        for (auto* sourceRow : sourceRows)
            measureItem (sourceRow, {});

        for (const auto& value : sourceValues)
            measureItem (nullptr, value);

        return itemReplacements;
    }

    /**
     * @brief Returns every @c :::interior::: marker's own interior text
     *        authored in @p text, verbatim, in authored order, through
     *        TemplateDocument::getMarkers() -- the one scan every
     *        marker-reading member reads through.
     *
     * @param text The text scanned for its own markers.
     * @returns @p text's own marker interiors, in authored order.
     */
    static jam::Strings getMarkers (const juce::String& text)
    {
        return TemplateDocument::getMarkers (text);
    }

    /**
     * @brief Returns @p name's own verbatim @c :::interior::: marker as
     *        authored in @p text -- the marker whose interior normalizes,
     *        through jam::Format::toValidID(), to @p name.
     *
     * @param text The text scanned for @p name's marker.
     * @param name The token's normalized identifier to match.
     * @returns @p name's verbatim marker found in @p text, or an empty
     *          string when @p text carries none.
     */
    static juce::String getMarker (const juce::String& text, const juce::Identifier& name)
    {
        for (const auto& interior : getMarkers (text))
            if (jam::Format::toValidID (interior) == name.toString())
                return Id::tripleColon + interior + Id::tripleColon;

        return {};
    }

    /**
     * @brief Returns one item's own column-aligned rendering of @p line's
     *        shape -- each marker replaced by @p replacements' own value,
     *        fill spaces inserted after each literal's first whitespace
     *        run to align every token but the first against @p columnWidths
     *        own byte-width high-water mark (SPEC §7.2).
     *
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param line             The structure line whose shape is rendered.
     * @param replacements     This item's own token-to-value map, from
     *                         getItemReplacements().
     * @param columnWidths     Each token's own widest replacement's byte
     *                         width across the join set.
     * @returns The rendered, column-aligned, trailing-whitespace-trimmed
     *          item text.
     */
    static juce::String getPaddedItem (const TemplateDocument& templateDocument,
                                       Element& line,
                                       const jam::HashMap<juce::Identifier, juce::String>& replacements,
                                       const jam::HashMap<juce::Identifier, size_t>& columnWidths)
    {
        const auto& itemText { *templateDocument.getCodeBlock (line)->get<juce::String> (Id::value) };

        juce::String paddedText;
        juce::String remainingText { itemText };
        auto isFirstToken { true };
        juce::Identifier previousName;
        juce::String previousValue;

        for (const auto& interior : getMarkers (itemText))
        {
            const auto marker { Id::tripleColon + interior + Id::tripleColon };
            const auto name { juce::Identifier (jam::Format::toValidID (interior)) };
            const auto columnValue { replacements.at (name) };
            const auto literal { jam::Format::upTo (remainingText, marker, false) };

            remainingText = jam::Format::from (remainingText, marker, false);

            if (isFirstToken)
                paddedText += literal;
            else
            {
                const auto literalStart { literal.getCharPointer() };
                const auto whitespaceStart { juce::CharacterFunctions::trimBegin (literalStart,
                    literalStart.findTerminatingNull(),
                    [] (const auto& character)
                    { return not juce::CharacterFunctions::isWhitespace (*character); }) };
                const auto whitespaceEnd { juce::CharacterFunctions::findEndOfWhitespace (whitespaceStart) };

                const auto head { literal.substring (
                    0, static_cast<int> (literalStart.lengthUpTo (whitespaceEnd))) };
                const auto tail { juce::String (whitespaceEnd) };
                const auto fillWidth { columnWidths.at (previousName)
                    - static_cast<size_t> (previousValue.getNumBytesAsUTF8()) };

                paddedText += head;
                paddedText += juce::String::repeatedString (
                    juce::String::charToString (Chars::space), static_cast<int> (fillWidth));
                paddedText += tail;
            }

            paddedText += columnValue;

            previousName = name;
            previousValue = columnValue;
            isFirstToken = false;
        }

        paddedText += remainingText;

        return paddedText.trimEnd();
    }

    /**
     * @brief Returns every item's own column-aligned rendering of @p line's
     *        shape -- getItemReplacements()' own maps, each rendered
     *        through getPaddedItem() against the column widths measured
     *        across the whole join set.
     *
     * @param model            The model @p sourceRows and @p line belong
     *                         to.
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param sourceRows       The item source rows to render.
     * @param sourceValues     The item column values to render.
     * @param sourceKey        The token name each of @p sourceValues
     *                         answers for.
     * @param line             The structure line whose shape is rendered.
     * @param extension        The target file extension a comment value is
     *                         commented for.
     * @returns Every non-empty rendered item text, in @p sourceRows then
     *          @p sourceValues order.
     */
    static jam::Strings getPaddedItemTexts (const Model& model,
                                            const TemplateDocument& templateDocument,
                                            const jam::Array<Element*>& sourceRows,
                                            const jam::Strings& sourceValues,
                                            const juce::Identifier& sourceKey,
                                            Element& line,
                                            const juce::String& extension)
    {
        jam::Strings texts;

        const auto itemReplacements { getItemReplacements (
            model, templateDocument, sourceRows, sourceValues, sourceKey, line, extension) };
        jam::HashMap<juce::Identifier, size_t> columnWidths;

        for (const auto& replacements : itemReplacements)
            for (const auto& [name, value] : replacements)
            {
                const auto tokenWidth { static_cast<size_t> (value.getNumBytesAsUTF8()) };
                auto [widthEntry, inserted] { columnWidths.try_emplace (name, tokenWidth) };
                auto& [widthName, columnWidth] { *widthEntry };

                if (not inserted and columnWidth < tokenWidth)
                    columnWidth = tokenWidth;
            }

        for (int index { 0 }; index < itemReplacements.size(); ++index)
        {
            const auto itemText { getPaddedItem (
                templateDocument, line, itemReplacements.at (index), columnWidths) };

            if (itemText.isNotEmpty())
                texts.add (itemText);
        }

        return texts;
    }

    /**
     * @brief Renders every item's own plain (unpadded) text -- each of
     *        @p sourceRows then @p sourceValues rendered through
     *        getItem(), keeping every non-empty rendered item.
     *
     * @param model            The model @p sourceRows and @p line belong
     *                         to.
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param sourceRows       The item source rows to render.
     * @param sourceValues     The item column values to render.
     * @param sourceKey        The token name each of @p sourceValues
     *                         answers for.
     * @param line             The structure line whose shape is rendered.
     * @param row              The structure row @p sourceOrdinal's child
     *                         column addresses are read from.
     * @param indent           The depth @p sourceOrdinal's child column
     *                         addresses are read from.
     * @param sourceOrdinal    The structure position after which child
     *                         column addresses are read.
     * @param childJoin        The @c list token's own child values' join
     *                         text.
     * @param extension        The target file extension a comment value
     *                         is commented for.
     * @returns Every non-empty rendered item text, in @p sourceRows then
     *          @p sourceValues order.
     */
    static jam::Strings getPlainItemTexts (const Model& model,
                                           const TemplateDocument& templateDocument,
                                           const jam::Array<Element*>& sourceRows,
                                           const jam::Strings& sourceValues,
                                           const juce::Identifier& sourceKey,
                                           Element& line,
                                           Element& row,
                                           int indent,
                                           int sourceOrdinal,
                                           const juce::String& childJoin,
                                           const juce::String& extension)
    {
        jam::Strings texts;

        const auto renderItem = [&model, &templateDocument, &sourceKey, &line, &row, indent,
                                 sourceOrdinal, &childJoin, &extension, &texts] (
            Element* sourceRow, const juce::String& sourceValue)
        {
            const auto itemText { getItem (model, templateDocument, sourceRow, sourceValue,
                sourceKey, line, row, indent, sourceOrdinal, childJoin, extension) };

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
     * @brief Renders every item's own text -- getPaddedItemTexts() when
     *        @p line's own shape is single-line, carries more than one
     *        item across @p sourceRows and @p sourceValues, and declares
     *        no @c list token, getPlainItemTexts() otherwise.
     *
     * @param model            The model @p sourceRows and @p line belong
     *                         to.
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param sourceRows       The item source rows to render.
     * @param sourceValues     The item column values to render.
     * @param sourceKey        The token name each of @p sourceValues
     *                         answers for.
     * @param line             The structure line whose shape is rendered.
     * @param row              The structure row @p sourceOrdinal's child
     *                         column addresses are read from.
     * @param indent           The depth @p sourceOrdinal's child column
     *                         addresses are read from.
     * @param sourceOrdinal    The structure position after which child
     *                         column addresses are read.
     * @param childJoin        The @c list token's own child values' join
     *                         text.
     * @param extension        The target file extension a comment value
     *                         is commented for.
     * @returns Every non-empty rendered item text, in @p sourceRows then
     *          @p sourceValues order.
     */
    static jam::Strings getItemTexts (const Model& model,
                                      const TemplateDocument& templateDocument,
                                      const jam::Array<Element*>& sourceRows,
                                      const jam::Strings& sourceValues,
                                      const juce::Identifier& sourceKey,
                                      Element& line,
                                      Element& row,
                                      int indent,
                                      int sourceOrdinal,
                                      const juce::String& childJoin,
                                      const juce::String& extension)
    {
        const auto& shapeText { *templateDocument.getCodeBlock (line)->get<juce::String> (Id::value) };
        const auto usePadding { isSingleLineShape (shapeText)
                                 and sourceRows.size() + sourceValues.size() > 1
                                 and getMarker (shapeText, Id::list).isEmpty() };

        if (usePadding)
            return getPaddedItemTexts (
                model, templateDocument, sourceRows, sourceValues, sourceKey, line, extension);

        return getPlainItemTexts (model, templateDocument, sourceRows, sourceValues, sourceKey,
            line, row, indent, sourceOrdinal, childJoin, extension);
    }

    /**
     * @brief Resolves @p source against @p row -- a wired table's own
     *        rows through getTableSourceRows(), a column shared across
     *        @p tables' own output rows through getColumnSourceValues(),
     *        or a blank-binding selector's own rows through
     *        getBindingSourceRows() -- then renders the resolved items
     *        through getItemTexts().
     *
     * @param model            The model @p tables and @p row belong to.
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param tables           The tables searched for @p source.
     * @param row              The row @p source is resolved against.
     * @param source           The item source's own name or address.
     * @param line             The structure line whose shape is rendered.
     * @param indent           The depth @p sourceOrdinal's child column
     *                         addresses are read from.
     * @param sourceOrdinal    The structure position after which child
     *                         column addresses are read.
     * @param childJoin        The @c list token's own child values' join
     *                         text.
     * @param extension        The target file extension a comment value
     *                         is commented for.
     * @returns Every non-empty rendered item text, in discovery order.
     */
    static jam::Strings getItems (const Model& model,
                                  const TemplateDocument& templateDocument,
                                  const jam::Array<Element*>& tables,
                                  Element& row,
                                  const juce::String& source,
                                  Element& line,
                                  int indent,
                                  int sourceOrdinal,
                                  const juce::String& childJoin,
                                  const juce::String& extension)
    {
        jam::Array<Element*> sourceRows;
        jam::Strings sourceValues;
        juce::Identifier sourceKey { Id::list };

        if (auto* sourceTable { model.getTable (row, source) }; sourceTable != nullptr)
        {
            sourceRows = getTableSourceRows (model, row, *sourceTable, source);
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
                sourceRows = getBindingSourceRows (model, templateDocument, tables, sourceName);
        }

        return getItemTexts (model, templateDocument, sourceRows, sourceValues, sourceKey, line,
            row, indent, sourceOrdinal, childJoin, extension);
    }

    /**
     * @brief Renders @p source's own items through getItems(), joined by
     *        @p separator.
     *
     * @param model            The model @p tables and @p row belong to.
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param tables           The tables searched for @p source.
     * @param row              The row @p source is resolved against.
     * @param source           The item source's own name or address.
     * @param line             The structure line whose shape is rendered.
     * @param separator        The rendered items' own join text.
     * @param indent           The depth @p sourceOrdinal's child column
     *                         addresses are read from.
     * @param sourceOrdinal    The structure position after which child
     *                         column addresses are read.
     * @param childJoin        The @c list token's own child values' join
     *                         text.
     * @param extension        The target file extension a comment value
     *                         is commented for.
     * @returns @p source's own rendered items, joined by @p separator.
     */
    static juce::String getJoinedItems (const Model& model,
                                        const TemplateDocument& templateDocument,
                                        const jam::Array<Element*>& tables,
                                        Element& row,
                                        const juce::String& source,
                                        Element& line,
                                        const juce::String& separator,
                                        int indent,
                                        int sourceOrdinal,
                                        const juce::String& childJoin,
                                        const juce::String& extension)
    {
        const auto texts { getItems (
            model, templateDocument, tables, row, source, line, indent, sourceOrdinal, childJoin, extension) };

        return texts.joinIntoString (separator, 0, -1);
    }
};
