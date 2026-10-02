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
    /** Model's own element type, brought into Items's own scope. */
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
                                                const Element& row,
                                                const jam::Array<const Element*>& tables,
                                                const juce::Identifier& source)
    {
        const auto currentFile { model.getFileName (model.getValue (row, Id::file)) };
        jam::Strings sourceValues;

        for (auto* table : tables)
            if (model.isOutputTable (*table))
                for (auto* candidate : model.getTableRows (*table))
                    if (auto* cell { model.getTableCell (*candidate, source) })
                    {
                        const auto value { model.getFileName (
                            *cell->get<juce::String> (Id::value)) };

                        if (value.isNotEmpty() and value.compare (currentFile) != 0
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
     *        filter column value does not equal its filter value.
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
    static jam::Array<const Element*> getTableSourceRows (const Model& model,
                                                     const Element& row,
                                                     const Element& sourceTable,
                                                     const juce::String& source)
    {
        const auto currentFile { model.getFileName (model.getValue (row, Id::file)) };
        const auto isFiltered { model.isFilteredAddress (row, source) };
        const auto filterColumn { isFiltered ? model.getFilterColumn (row, source) : juce::Identifier{} };
        const auto filterValue { isFiltered ? model.getFilterValue (row, source) : juce::String{} };
        jam::Array<const Element*> sourceRows;

        for (auto* candidate : model.getTableRows (sourceTable))
        {
            auto* fileCell { model.getTableCell (*candidate, Id::file) };
            const auto candidateFile { fileCell != nullptr
                                           ? model.getFileName (
                                                 *fileCell->get<juce::String> (Id::value))
                                           : juce::String{} };
            const auto matchesFilter { not isFiltered
                or model.getTableCell (*candidate, filterColumn)->get<juce::String> (Id::value)
                       ->compare (filterValue) == 0 };

            if (candidateFile.compare (currentFile) != 0 and matchesFilter)
                sourceRows.add (candidate);
        }

        return sourceRows;
    }

    /**
     * @brief Returns @p line's own arity -- its shape's count of
     *        @c :::\[list\]::: occurrences.
     *
     * @param templateDocument The template document @p line's shape is
     *                         read from.
     * @param line             The structure line whose arity is counted.
     * @returns @p line's own shape's @c :::\[list\]::: occurrence count.
     */
    static int getArity (const TemplateDocument& templateDocument, const Element& line)
    {
        static const auto listMarker { Model::getReservedName (Id::list) };

        const auto& tokens { *templateDocument.getCodeBlock (line)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };

        return static_cast<int> (std::count (tokens.begin(), tokens.end(), listMarker));
    }

    /**
     * @brief Collects @p row's own structure wiring's shape-valued
     *        bindings' consumed shape ordinals -- every shape ordinal
     *        transitively consumed by a wrapper, walking each consumed
     *        source's own arity-bounded chain in turn.
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
                                             const Element& row)
    {
        static const auto listMarker { Model::getReservedName (Id::list) };

        jam::Array<int> privateShapes;
        const auto arityOf = [&templateDocument] (const Element& line) { return getArity (templateDocument, line); };

        if (auto* scope { model.getTableCell (row, Id::structure) })
            scope->applyFunctionRecursively (
                [&model, &privateShapes, &arityOf] (const Element& candidate) -> bool
                {
                    if (candidate.parent->isTag (Id::ul) and candidate.id != listMarker
                        and candidate.contains (Id::templatePath))
                    {
                        auto* cursor { model.getNextLine (candidate) };

                        for (int occurrence { 0 };
                             cursor != nullptr and occurrence < arityOf (candidate); ++occurrence)
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
    static jam::Array<const Element*> getBindingSourceRows (const Model& model,
                                                       const TemplateDocument& templateDocument,
                                                       const jam::Array<const Element*>& tables,
                                                       const juce::Identifier& source)
    {
        static const auto listMarker { Model::getReservedName (Id::list) };

        jam::Array<const Element*> sourceRows;

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
                                    and item.id != listMarker
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
     *        row's own column value. An @-sigiled @c description binding is a
     *        reference, not prose, and resolves empty; a @c description
     *        column value is prose, whatever its first character.
     *
     * @param model            The model @p sourceRow belongs to.
     * @param templateDocument The template document @p sourceRow's own
     *                         private render data is read through.
     * @param row              The row whose map tables are read.
     * @param shapeLine        The structure line whose own @c shape
     *                         ordinal selects the map tables.
     * @param sourceRow        The row @p name is resolved against.
     * @param name             The token or column name to resolve.
     * @returns The resolved value, or an empty string when neither a
     *          binding nor a column named @p name exists on @p sourceRow, or
     *          @p name is @c description and the binding value is a
     *          reference.
     */
    static juce::String getSourceValue (const Model& model, const TemplateDocument& templateDocument,
        const Element& row, const Element& shapeLine, const Element& sourceRow, const juce::Identifier& name)
    {
        static const auto listMarker { Model::getReservedName (Id::list) };
        static const auto descriptionMarker { Model::getReservedName (Id::description) };

        juce::String deepestValue;
        const auto privateShapes { getPrivateShapes (model, templateDocument, sourceRow) };

        if (auto* scope { model.getTableCell (sourceRow, Id::structure) })
            scope->applyFunctionRecursively (
                [&deepestValue, &name, &privateShapes] (const Element& item) -> bool
                {
                    if (item.parent->isTag (Id::ul) and item.id == name and item.id != listMarker
                        and not privateShapes.contains (*item.get<int> (Id::shape)))
                        deepestValue = *item.get<juce::String> (Id::value);

                    return true;
                });

        if (name == descriptionMarker and Model::isAddress (deepestValue))
            return {};

        if (deepestValue.isNotEmpty())
            return deepestValue;

        if (name != descriptionMarker)
            if (auto* cell { model.getMapCell (row, shapeLine, name) })
                return *cell->get<juce::String> (Id::value);

        juce::String columnValue;

        if (auto* cell { model.getTableCell (sourceRow, name == descriptionMarker ? Id::description : name) })
            columnValue = *cell->get<juce::String> (Id::value);

        return name == Id::file ? model.getFileName (columnValue) : columnValue;
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
     * @param row         The row whose map tables are read.
     * @param shapeLine   The structure line whose own @c shape ordinal
     *                    selects the map tables.
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
                                        const Element& row,
                                        const Element& shapeLine,
                                        const Element* sourceRow,
                                        const juce::String& sourceValue,
                                        const juce::Identifier& sourceKey,
                                        const juce::Identifier& name)
    {
        if (sourceRow != nullptr)
            return getSourceValue (model, templateDocument, row, shapeLine, *sourceRow, name);

        if (name == sourceKey)
            return sourceValue;

        if (auto* cell { model.getMapCell (row, shapeLine, name) })
            return *cell->get<juce::String> (Id::value);

        return {};
    }

    /**
     * @brief Resolves the @c list token's value for one item -- when
     *        @p arity is 1, every column-address child line authored
     *        directly beneath @p sourceOrdinal, read from @p sourceRow's
     *        own column, joined by @p childJoin; when @p arity is 2 or
     *        more, @p occurrence's own child column value, the authored
     *        values filling the trailing slots and any leading slot
     *        rendering empty.
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
     * @param childJoin     The child values' join text, used when @p arity
     *                      is 1.
     * @param arity         The item's shape's own @c :::[list]::: occurrence
     *                      count.
     * @param occurrence    The @c list token's own occurrence index,
     *                      consumed when @p arity is 2 or more.
     * @returns When @p arity is 1, every resolved child column value, in
     *          authored order, joined by @p childJoin. When @p arity is 2
     *          or more, @p occurrence's own child column value, or an
     *          empty string for a leading slot the authored values do not
     *          reach.
     */
    static juce::String getChildValue (const Model& model, const Element& row, int indent,
                                       int sourceOrdinal, const Element* sourceRow,
                                       const juce::String& childJoin, int arity, int occurrence)
    {
        jam::Strings childValues;

        if (sourceRow != nullptr)
            for (int index { 0 };; ++index)
            {
                auto* childSourceLine { model.getColumnAddress (row, indent, sourceOrdinal, index) };

                if (childSourceLine == nullptr)
                    break;

                const auto& value { *childSourceLine->get<juce::String> (Id::value) };

                if (auto* cell { model.getTableCell (*sourceRow, model.getColumn (row, value)) })
                    childValues.add (*cell->get<juce::String> (Id::value));
            }

        if (arity < 2)
            return childValues.joinIntoString (childJoin, 0, -1);

        return occurrence >= arity - childValues.size()
                   ? childValues.at (occurrence - (arity - childValues.size()))
                   : juce::String{};
    }

    /**
     * @brief Renders one item's own plain (unpadded) text -- @p line's
     *        own shape with every placeholder token substituted by its
     *        resolved value, the @c list token filled by getChildValue()
     *        -- once per its own occurrence, in authored order, when
     *        @p line's own shape's arity is 2 or more -- and every other
     *        token by getColumnValue(), commenting a non-empty @c description
     *        value for @p extension.
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
     * @param shapeLine        The structure line whose own @c shape
     *                         ordinal selects the map tables.
     * @param indent           The depth @p sourceOrdinal's child column
     *                         addresses are read from.
     * @param sourceOrdinal    The structure position after which child
     *                         column addresses are read.
     * @param childJoin        The @c list token's own child values' join
     *                         text.
     * @param extension        The target file extension a description value
     *                         is commented for.
     * @returns The rendered item text.
     */
    static juce::String getItem (const Model& model,
                                 const TemplateDocument& templateDocument,
                                 const Element* sourceRow,
                                 const juce::String& sourceValue,
                                 const juce::Identifier& sourceKey,
                                 const Element& line,
                                 const Element& row,
                                 const Element& shapeLine,
                                 int indent,
                                 int sourceOrdinal,
                                 const juce::String& childJoin,
                                 const juce::String& extension)
    {
        static const auto listMarker { Model::getReservedName (Id::list) };
        static const auto descriptionMarker { Model::getReservedName (Id::description) };

        const auto& tokens { *templateDocument.getCodeBlock (line)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };
        auto itemText { *templateDocument.getCodeBlock (line)->get<juce::String> (Id::value) };
        const auto arity { getArity (templateDocument, line) };
        int occurrence { 0 };

        for (const auto& name : tokens)
        {
            const auto marker { getMarker (itemText, name) };

            if (marker.isNotEmpty())
            {
                auto value { name == listMarker
                                 ? getChildValue (
                                       model, row, indent, sourceOrdinal, sourceRow, childJoin, arity, occurrence)
                                 : getColumnValue (
                                       model, templateDocument, row, shapeLine, sourceRow, sourceValue,
                                       sourceKey, name) };

                if (name == descriptionMarker and value.isNotEmpty())
                    value = Transforms::toComment (value, extension);

                if (name == listMarker)
                {
                    itemText = jam::Format::upTo (itemText, marker, false)
                             + value
                             + jam::Format::from (itemText, marker, false);
                    ++occurrence;
                }
                else
                {
                    itemText = itemText.replace (marker, value);
                }
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
     * @brief Resolves one item's own replacement map -- @p tokens' own
     *        names, each mapped to its resolved value through
     *        getColumnValue(), commenting a non-empty @c description value.
     *
     * @param model            The model @p sourceRow, when present,
     *                         belongs to.
     * @param templateDocument The template document @p sourceRow's own
     *                         private render data is read through, when
     *                         present.
     * @param row              The row whose map tables are read.
     * @param shapeLine        The structure line whose own @c shape
     *                         ordinal selects the map tables.
     * @param tokens           The placeholder tokens to resolve.
     * @param sourceRow        The item's source row, or @c nullptr when
     *                         the item is a bare column value.
     * @param sourceValue      The column value resolved when @p sourceRow
     *                         is @c nullptr and a token matches
     *                         @p sourceKey.
     * @param sourceKey        The token name @p sourceValue answers for.
     * @param extension        The target file extension a description value
     *                         is commented for.
     * @returns @p tokens' own name-to-value map for this item.
     */
    static jam::HashMap<juce::Identifier, juce::String> getItemReplacement (const Model& model,
        const TemplateDocument& templateDocument, const Element& row, const Element& shapeLine,
        const jam::Document::Identifiers& tokens, const Element* sourceRow, const juce::String& sourceValue, const juce::Identifier& sourceKey,
        const juce::String& extension)
    {
        static const auto descriptionMarker { Model::getReservedName (Id::description) };

        jam::HashMap<juce::Identifier, juce::String> replacements;

        for (const auto& name : tokens)
        {
            auto value {
                getColumnValue (model, templateDocument, row, shapeLine, sourceRow, sourceValue, sourceKey, name)
            };

            if (name == descriptionMarker and value.isNotEmpty())
                value = Transforms::toComment (value, extension);

            replacements.emplace (name, value);
        }

        return replacements;
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
     * @param row              The row whose map tables are read.
     * @param shapeLine        The structure line whose own @c shape
     *                         ordinal selects the map tables.
     * @param sourceRows       The item source rows to resolve.
     * @param sourceValues     The item column values to resolve.
     * @param sourceKey        The token name each of @p sourceValues
     *                         answers for.
     * @param line             The structure line whose placeholder tokens
     *                         are resolved.
     * @param extension        The target file extension a description value is
     *                         commented for.
     * @returns One replacement map per item, in @p sourceRows then
     *          @p sourceValues order.
     */
    static jam::Array<jam::HashMap<juce::Identifier, juce::String>> getItemReplacements (
        const Model& model,
        const TemplateDocument& templateDocument,
        const Element& row,
        const Element& shapeLine,
        const jam::Array<const Element*>& sourceRows,
        const jam::Strings& sourceValues,
        const juce::Identifier& sourceKey,
        const Element& line,
        const juce::String& extension)
    {
        jam::Array<jam::HashMap<juce::Identifier, juce::String>> itemReplacements;
        const auto& tokens { *templateDocument.getCodeBlock (line)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };

        for (auto* sourceRow : sourceRows)
            itemReplacements.add (
                getItemReplacement (
                    model, templateDocument, row, shapeLine, tokens, sourceRow, {}, sourceKey, extension));

        for (const auto& value : sourceValues)
            itemReplacements.add (
                getItemReplacement (
                    model, templateDocument, row, shapeLine, tokens, nullptr, value, sourceKey, extension));

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
            if (jam::Format::toValidID (interior).compare (name.toString()) == 0)
                return Id::tripleColon + interior + Id::tripleColon;

        return {};
    }

    /**
     * @brief Returns @p literal with fill spaces inserted after its first
     *        whitespace run, aligning @p previousName's own value against
     *        @p columnWidths' own byte-width high-water mark.
     *
     * @param literal       The literal text between the previous marker
     *                      and the next.
     * @param previousName  The preceding token's own name, whose column
     *                      width is read from @p columnWidths.
     * @param previousValue The preceding token's own resolved value,
     *                      whose byte width is padded up to
     *                      @p previousName's own column width.
     * @param columnWidths  Each token's own widest replacement's byte
     *                      width across the join set.
     * @returns @p literal, its first whitespace run widened by the fill
     *          spaces @p previousName's own column width demands.
     */
    static juce::String getPaddedLiteral (const juce::String& literal, const juce::Identifier& previousName,
        const juce::String& previousValue, const jam::HashMap<juce::Identifier, size_t>& columnWidths)
    {
        static const auto spaceText { juce::String::charToString (Chars::space) };

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

        return head
             + juce::String::repeatedString (spaceText, static_cast<int> (fillWidth))
             + tail;
    }

    /**
     * @brief Returns one item's own column-aligned rendering of @p line's
     *        shape -- each marker replaced by @p replacements' own value,
     *        fill spaces inserted after each literal's first whitespace
     *        run to align every token but the first against @p columnWidths
     *        own byte-width high-water mark. A marker absent
     *        from @p replacements is emitted verbatim and takes no part
     *        in the column alignment.
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
                                       const Element& line,
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
            const auto literal { jam::Format::upTo (remainingText, marker, false) };
            remainingText = jam::Format::from (remainingText, marker, false);
            paddedText += isFirstToken
                              ? literal
                              : getPaddedLiteral (literal, previousName, previousValue, columnWidths);

            if (auto replacementEntry { replacements.find (name) };
                replacementEntry != replacements.end())
            {
                const auto& [replacementName, columnValue] { *replacementEntry };

                paddedText += columnValue;
                previousName = name;
                previousValue = columnValue;
                isFirstToken = false;
            }
            else
            {
                paddedText += marker;
            }
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
     * @param row              The row whose map tables are read.
     * @param shapeLine        The structure line whose own @c shape
     *                         ordinal selects the map tables.
     * @param sourceRows       The item source rows to render.
     * @param sourceValues     The item column values to render.
     * @param sourceKey        The token name each of @p sourceValues
     *                         answers for.
     * @param line             The structure line whose shape is rendered.
     * @param extension        The target file extension a description value is
     *                         commented for.
     * @returns Every non-empty rendered item text, in @p sourceRows then
     *          @p sourceValues order.
     */
    static jam::Strings getPaddedItemTexts (const Model& model,
                                            const TemplateDocument& templateDocument,
                                            const Element& row,
                                            const Element& shapeLine,
                                            const jam::Array<const Element*>& sourceRows,
                                            const jam::Strings& sourceValues,
                                            const juce::Identifier& sourceKey,
                                            const Element& line,
                                            const juce::String& extension)
    {
        jam::Strings texts;

        const auto itemReplacements { getItemReplacements (
            model, templateDocument, row, shapeLine, sourceRows, sourceValues, sourceKey, line,
            extension) };
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
     * @param shapeLine        The structure line whose own @c shape
     *                         ordinal selects the map tables.
     * @param indent           The depth @p sourceOrdinal's child column
     *                         addresses are read from.
     * @param sourceOrdinal    The structure position after which child
     *                         column addresses are read.
     * @param childJoin        The @c list token's own child values' join
     *                         text.
     * @param extension        The target file extension a description value
     *                         is commented for.
     * @returns Every non-empty rendered item text, in @p sourceRows then
     *          @p sourceValues order.
     */
    static jam::Strings getPlainItemTexts (const Model& model,
                                           const TemplateDocument& templateDocument,
                                           const jam::Array<const Element*>& sourceRows,
                                           const jam::Strings& sourceValues,
                                           const juce::Identifier& sourceKey,
                                           const Element& line,
                                           const Element& row,
                                           const Element& shapeLine,
                                           int indent,
                                           int sourceOrdinal,
                                           const juce::String& childJoin,
                                           const juce::String& extension)
    {
        jam::Strings texts;

        const auto renderItem = [&model, &templateDocument, &sourceKey, &line, &row, &shapeLine, indent,
                                 sourceOrdinal, &childJoin, &extension, &texts] (
            const Element* sourceRow, const juce::String& sourceValue)
        {
            const auto itemText { getItem (model, templateDocument, sourceRow, sourceValue,
                sourceKey, line, row, shapeLine, indent, sourceOrdinal, childJoin, extension) };

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
     * @param shapeLine        The structure line whose own @c shape
     *                         ordinal selects the map tables.
     * @param indent           The depth @p sourceOrdinal's child column
     *                         addresses are read from.
     * @param sourceOrdinal    The structure position after which child
     *                         column addresses are read.
     * @param childJoin        The @c list token's own child values' join
     *                         text.
     * @param extension        The target file extension a description value
     *                         is commented for.
     * @returns Every non-empty rendered item text, in @p sourceRows then
     *          @p sourceValues order.
     */
    static jam::Strings getItemTexts (const Model& model,
                                      const TemplateDocument& templateDocument,
                                      const jam::Array<const Element*>& sourceRows,
                                      const jam::Strings& sourceValues,
                                      const juce::Identifier& sourceKey,
                                      const Element& line,
                                      const Element& shapeLine,
                                      const Element& row,
                                      int indent,
                                      int sourceOrdinal,
                                      const juce::String& childJoin,
                                      const juce::String& extension)
    {
        static const auto listMarker { Model::getReservedName (Id::list) };

        const auto& shapeText { *templateDocument.getCodeBlock (line)->get<juce::String> (Id::value) };
        const auto usePadding { isSingleLineShape (shapeText)
                                 and sourceRows.size() + sourceValues.size() > 1
                                 and getMarker (shapeText, listMarker).isEmpty() };

        if (usePadding)
            return getPaddedItemTexts (model, templateDocument, row, shapeLine, sourceRows, sourceValues,
                sourceKey, line, extension);

        return getPlainItemTexts (model, templateDocument, sourceRows, sourceValues, sourceKey,
            line, row, shapeLine, indent, sourceOrdinal, childJoin, extension);
    }

    /**
     * @brief Answers whether @p sourceName names a column any of
     *        @p tables' own output tables declares.
     *
     * @param model      The model @p tables belong to.
     * @param tables     The tables searched for @p sourceName's own
     *                   column.
     * @param sourceName The column name to test.
     * @returns @c true when at least one output table among @p tables
     *          declares a @p sourceName column.
     */
    static bool isColumnSource (const Model& model, const jam::Array<const Element*>& tables,
        const juce::Identifier& sourceName)
    {
        return std::any_of (tables.begin(), tables.end(),
            [&model, &sourceName] (const Element* table)
            {
                auto* headerRow { Model::getTableHeaderRow (*table) };
                return model.isOutputTable (*table)
                       and model.getTableCell (*headerRow, sourceName) != nullptr;
            });
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
     * @param shapeLine        The structure line whose own @c shape
     *                         ordinal selects the map tables.
     * @param indent           The depth @p sourceOrdinal's child column
     *                         addresses are read from.
     * @param sourceOrdinal    The structure position after which child
     *                         column addresses are read.
     * @param childJoin        The @c list token's own child values' join
     *                         text.
     * @param extension        The target file extension a description value
     *                         is commented for.
     * @returns Every non-empty rendered item text, in discovery order.
     */
    static jam::Strings getItems (const Model& model,
                                  const TemplateDocument& templateDocument,
                                  const jam::Array<const Element*>& tables,
                                  const Element& row,
                                  const juce::String& source,
                                  const Element& line,
                                  const Element& shapeLine,
                                  int indent,
                                  int sourceOrdinal,
                                  const juce::String& childJoin,
                                  const juce::String& extension)
    {
        static const auto listMarker { Model::getReservedName (Id::list) };

        jam::Array<const Element*> sourceRows;
        jam::Strings sourceValues;
        juce::Identifier sourceKey { listMarker };

        if (auto* sourceTable { model.getTable (row, source) }; sourceTable != nullptr)
            sourceRows = getTableSourceRows (model, row, *sourceTable, source);
        else
        {
            const auto sourceName { juce::Identifier (source) };

            if (isColumnSource (model, tables, sourceName))
            {
                sourceKey = sourceName;
                sourceValues = getColumnSourceValues (model, row, tables, sourceName);
            }
            else
                sourceRows = getBindingSourceRows (model, templateDocument, tables, sourceName);
        }

        return getItemTexts (model, templateDocument, sourceRows, sourceValues, sourceKey, line,
            shapeLine, row, indent, sourceOrdinal, childJoin, extension);
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
     * @param shapeLine        The structure line whose own @c shape
     *                         ordinal selects the map tables.
     * @param separator        The rendered items' own join text.
     * @param indent           The depth @p sourceOrdinal's child column
     *                         addresses are read from.
     * @param sourceOrdinal    The structure position after which child
     *                         column addresses are read.
     * @param childJoin        The @c list token's own child values' join
     *                         text.
     * @param extension        The target file extension a description value
     *                         is commented for.
     * @returns @p source's own rendered items, joined by @p separator.
     */
    static juce::String getJoinedItems (const Model& model,
                                        const TemplateDocument& templateDocument,
                                        const jam::Array<const Element*>& tables,
                                        const Element& row,
                                        const juce::String& source,
                                        const Element& line,
                                        const Element& shapeLine,
                                        const juce::String& separator,
                                        int indent,
                                        int sourceOrdinal,
                                        const juce::String& childJoin,
                                        const juce::String& extension)
    {
        const auto texts { getItems (
            model, templateDocument, tables, row, source, line, shapeLine, indent, sourceOrdinal, childJoin,
            extension) };

        return texts.joinIntoString (separator, 0, -1);
    }
};
