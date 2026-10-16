#include "Items.h"

jam::Strings Items::getColumnSourceValues (const Model& model,
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

jam::Array<const Items::Element*> Items::getTableSourceRows (const Model& model,
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

int Items::getArity (const TemplateDocument& templateDocument, const Element& line)
{
    static const auto listMarker { Model::getReservedName (Id::list) };

    const auto& tokens { *templateDocument.getCodeBlock (line)
                               ->get<jam::Document::Identifiers> (Id::placeholder) };

    return static_cast<int> (std::count (tokens.begin(), tokens.end(), listMarker));
}

const Items::Element* Items::getNextSourceLine (const Model& model, const TemplateDocument& templateDocument, const Element& line)
{
    auto* cursor { model.getNextLine (line) };

    if (cursor != nullptr and Model::isWrapper (*cursor))
        cursor = getNextShapeLine (model, templateDocument, *cursor);

    return cursor;
}

const Items::Element* Items::getNextShapeLine (const Model& model, const TemplateDocument& templateDocument, const Element& line)
{
    auto* cursor { getNextSourceLine (model, templateDocument, line) };

    if (line.isTag (Id::p) or Model::isWrapper (line))
        for (int occurrence { 0 }; cursor != nullptr and occurrence < getArity (templateDocument, line); ++occurrence)
            cursor = getNextShapeLine (model, templateDocument, *cursor);

    return cursor;
}

jam::Array<const Items::Element*> Items::getBindingSourceRows (const Model& model,
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

                if (auto* scope { model.getTableCell (*candidate, Id::structure) })
                    scope->applyFunctionRecursively (
                        [&matches, &source] (const Element& item) -> bool
                        {
                            if (item.parent->isTag (Id::ul) and item.id == source
                                and item.id != listMarker
                                and not item.contains (Id::chain))
                                matches = true;

                            return not matches;
                        });

                if (matches)
                    sourceRows.add (candidate);
            }

    return sourceRows;
}

juce::String Items::getSourceValue (const Model& model,
    const Element& row, const Element& shapeLine, const Element& sourceRow, const juce::Identifier& name)
{
    static const auto listMarker { Model::getReservedName (Id::list) };
    static const auto descriptionMarker { Model::getReservedName (Id::description) };

    juce::String deepestValue;

    if (auto* scope { model.getTableCell (sourceRow, Id::structure) })
        scope->applyFunctionRecursively (
            [&deepestValue, &name] (const Element& item) -> bool
            {
                if (item.parent->isTag (Id::ul) and item.id == name and item.id != listMarker
                    and not item.contains (Id::chain))
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

juce::String Items::getColumnValue (const Model& model,
                                    const Element& row,
                                    const Element& shapeLine,
                                    const Element* sourceRow,
                                    const juce::String& sourceValue,
                                    const juce::Identifier& sourceKey,
                                    const juce::Identifier& name)
{
    if (sourceRow != nullptr)
        return getSourceValue (model, row, shapeLine, *sourceRow, name);

    if (name == sourceKey)
        return sourceValue;

    if (auto* cell { model.getMapCell (row, shapeLine, name) })
        return *cell->get<juce::String> (Id::value);

    return {};
}

juce::String Items::getChildValue (const Model& model, const Element& row, int indent,
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

juce::String Items::getItem (const Model& model,
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
                                   model, row, shapeLine, sourceRow, sourceValue,
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

bool Items::isSingleLineShape (const juce::String& itemText) noexcept
{
    return not itemText.trimEnd().containsChar (Chars::newline);
}

jam::HashMap<juce::Identifier, juce::String> Items::getItemReplacement (const Model& model,
    const Element& row, const Element& shapeLine,
    const jam::Document::Identifiers& tokens, const Element* sourceRow, const juce::String& sourceValue, const juce::Identifier& sourceKey,
    const juce::String& extension)
{
    static const auto descriptionMarker { Model::getReservedName (Id::description) };

    jam::HashMap<juce::Identifier, juce::String> replacements;

    for (const auto& name : tokens)
    {
        auto value {
            getColumnValue (model, row, shapeLine, sourceRow, sourceValue, sourceKey, name)
        };

        if (name == descriptionMarker and value.isNotEmpty())
            value = Transforms::toComment (value, extension);

        replacements.emplace (name, value);
    }

    return replacements;
}

jam::Array<jam::HashMap<juce::Identifier, juce::String>> Items::getItemReplacements (
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
                model, row, shapeLine, tokens, sourceRow, {}, sourceKey, extension));

    for (const auto& value : sourceValues)
        itemReplacements.add (
            getItemReplacement (
                model, row, shapeLine, tokens, nullptr, value, sourceKey, extension));

    return itemReplacements;
}

jam::Strings Items::getMarkers (const juce::String& text)
{
    return TemplateDocument::getMarkers (text);
}

juce::String Items::getMarker (const juce::String& text, const juce::Identifier& name)
{
    for (const auto& interior : getMarkers (text))
        if (jam::Format::toValidID (interior).compare (name.toString()) == 0)
            return Id::tripleColon + interior + Id::tripleColon;

    return {};
}

juce::String Items::getPaddedLiteral (const juce::String& literal, const juce::Identifier& previousName,
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

juce::String Items::getPaddedItem (const TemplateDocument& templateDocument,
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

jam::Strings Items::getPaddedItemTexts (const Model& model,
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

jam::Strings Items::getPlainItemTexts (const Model& model,
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

jam::Strings Items::getItemTexts (const Model& model,
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

bool Items::isColumnSource (const Model& model, const jam::Array<const Element*>& tables,
    const juce::Identifier& sourceName)
{
    return std::any_of (tables.begin(), tables.end(),
        [&model, &sourceName] (const Element* table)
        {
            const auto* headerRow { Model::getTableHeaderRow (*table) };
            return model.isOutputTable (*table)
                   and headerRow != nullptr
                   and model.getTableCell (*headerRow, sourceName) != nullptr;
        });
}

jam::Strings Items::getItems (const Model& model,
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
            sourceRows = getBindingSourceRows (model, tables, sourceName);
    }

    return getItemTexts (model, templateDocument, sourceRows, sourceValues, sourceKey, line,
        shapeLine, row, indent, sourceOrdinal, childJoin, extension);
}

juce::String Items::getJoinedItems (const Model& model,
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
