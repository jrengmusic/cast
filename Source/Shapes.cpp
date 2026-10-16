#include "Shapes.h"

const Shapes::Element* Shapes::getSourceLine (const Model& model, const TemplateDocument& templateDocument,
    const Element& line, int occurrence)
{
    const Element* cursor { Items::getNextSourceLine (model, templateDocument, line) };

    for (int index { 0 }; cursor != nullptr and index < occurrence; ++index)
        cursor = Items::getNextShapeLine (model, templateDocument, *cursor);

    return cursor;
}

const Shapes::Element* Shapes::getFirstLine (const Model& model, const Element& row)
{
    static const auto listMarker { Model::getReservedName (Id::list) };

    const Element* line { nullptr };

    model.getTableCell (row, Id::structure)
        ->applyFunctionRecursively (
            [&line] (const Element& candidate) -> bool
            {
                if (line == nullptr and candidate.contains (Id::shape)
                    and *candidate.get<int> (Id::shape) == 0
                    and (candidate.isTag (Id::p) or candidate.id == listMarker))
                    line = &candidate;

                return line == nullptr;
            });

    return line;
}

juce::String Shapes::getRegionValue (const Model& model, const TemplateDocument& templateDocument,
    const Element& row, const juce::Identifier& word)
{
    auto* binding { model.getBinding (
        row, Id::structure, *getFirstLine (model, row), Model::getReservedName (word)) };

    return templateDocument.getValue (model, row, *binding->get<juce::String> (Id::value));
}

const Shapes::Element* Shapes::getSourceTable (const Model& model, const TemplateDocument& templateDocument,
    const Element& row, const Element& line)
{
    const Element* table { nullptr };

    for (int occurrence { 0 };
         table == nullptr and occurrence < Items::getArity (templateDocument, line); ++occurrence)
    {
        auto* sourceLine { getSourceLine (model, templateDocument, line, occurrence) };

        if (sourceLine != nullptr and not sourceLine->isTag (Id::p))
            table = model.getTable (row,
                *model.getSource (row, *sourceLine->get<int> (Id::level),
                     *sourceLine->get<int> (Id::line))
                     ->get<juce::String> (Id::value));
    }

    return table;
}

const Shapes::Element* Shapes::getDescriptionTable (const Model& model, const TemplateDocument& templateDocument,
    const Element& row, const Element& line)
{
    const Element* table { nullptr };
    auto* description { line.isTag (Id::p) or Model::isWrapper (line) ? model.getDescription (row, line) : nullptr };

    if (not line.isTag (Id::p) and not Model::isWrapper (line))
    {
        if (line.contains (Id::line))
            table = model.getTable (row,
                *model.getSource (row, *line.get<int> (Id::level), *line.get<int> (Id::line))
                     ->get<juce::String> (Id::value));
    }
    else if (description != nullptr)
    {
        const auto& value { *description->get<juce::String> (Id::value) };

        table = model.getTable (row, value);

        if (table == nullptr)
            return model.getCodeBlock (juce::Identifier (jam::Format::getPostColon (value).trim()));
    }
    else
    {
        table = getSourceTable (model, templateDocument, row, line);
    }

    if (table != nullptr)
        return table;

    if (row.parent->get<juce::String> (Id::description)->isNotEmpty())
        return row.parent;

    return nullptr;
}

const Shapes::Element* Shapes::getItemSourceRow (const Model& model, const Element& row, const Element& line)
{
    auto* source { model.getSource (row, *line.get<int> (Id::level), *line.get<int> (Id::line)) };
    const auto& sourceValue { *source->get<juce::String> (Id::value) };
    auto* sourceTable { model.getTable (row, sourceValue) };
    auto sourceRows { Items::getTableSourceRows (model, row, *sourceTable, sourceValue) };

    return sourceRows.isEmpty() ? nullptr : sourceRows.at (0);
}

juce::String Shapes::getTokenValue (const Model& model, const TemplateDocument& templateDocument,
    const jam::Array<const Element*>& tables, const Element& row, const Element& line, const Element* descriptionTable,
    const juce::Identifier& name, const juce::String& joinText, int parentIndent,
    bool isAtColumnZero, const juce::String& extension)
{
    static const auto descriptionMarker { Model::getReservedName (Id::description) };
    static const auto listMarker { Model::getReservedName (Id::list) };

    if (name == descriptionMarker)
        return descriptionTable != nullptr ? *descriptionTable->get<juce::String> (Id::description)
                                       : juce::String{};

    if (auto* binding { model.getBinding (row, Id::structure, line, name) })
    {
        if (binding->contains (Id::templatePath))
            return getShapeText (model, templateDocument, tables, row, *binding, joinText,
                parentIndent, isAtColumnZero, extension);

        return templateDocument.getValue (model, row, *binding->get<juce::String> (Id::value));
    }

    if (auto* cell { model.getMapCell (row, line, name) })
        return *cell->get<juce::String> (Id::value);

    auto* columnRow { line.id == listMarker ? getItemSourceRow (model, row, line) : &row };

    if (columnRow == nullptr)
        return {};

    if (auto* cell { model.getTableCell (*columnRow, name) })
    {
        const auto& columnValue { *cell->get<juce::String> (Id::value) };
        return name == Id::file ? model.getFileName (columnValue) : columnValue;
    }

    return {};
}

juce::String Shapes::getMarkerValue (const Model& model, const TemplateDocument& templateDocument,
    const jam::Array<const Element*>& tables, const jam::Array<const Element*>& rows,
    const jam::Array<const Element*>& lines, const Element* descriptionTable, const juce::Identifier& name,
    int tokenOccurrence, const juce::String& joinText, int parentIndent, bool isAtColumnZero,
    const juce::String& extension, const juce::String& templateLine, const juce::String& marker)
{
    static const auto listMarker { Model::getReservedName (Id::list) };
    static const auto descriptionMarker { Model::getReservedName (Id::description) };

    auto value { name == listMarker
                     ? getFill (model, templateDocument, tables, rows, lines,
                           tokenOccurrence, joinText, parentIndent, isAtColumnZero, extension)
                     : getTokenValue (model, templateDocument, tables, *rows.first(),
                           *lines.first(), descriptionTable, name, joinText, parentIndent,
                           isAtColumnZero, extension) };

    if (name == descriptionMarker and value.isNotEmpty())
        value = templateLine.trim().compare (marker) == 0
                    ? Transforms::toCommentBlock (value, extension)
                    : Transforms::toComment (value, extension);

    return value;
}

juce::String Shapes::getSubstitutedLine (const Model& model,
    const TemplateDocument& templateDocument, const jam::Array<const Element*>& tables,
    const jam::Array<const Element*>& rows, const jam::Array<const Element*>& lines,
    const jam::Document::Identifiers& tokens, jam::HashMap<juce::Identifier, int>& occurrence,
    const juce::String& templateLine, const juce::String& joinText, int parentIndent,
    const juce::String& extension)
{
    static const auto listMarker { Model::getReservedName (Id::list) };

    auto lineText { templateLine };
    auto* descriptionTable { getDescriptionTable (model, templateDocument, *rows.first(), *lines.first()) };

    for (const auto& name : tokens)
    {
        const auto marker { Items::getMarker (lineText, name) };

        if (marker.isNotEmpty())
        {
            auto [occurrenceEntry, inserted] { occurrence.try_emplace (name, 0) };
            auto& [occurrenceName, tokenOccurrence] { *occurrenceEntry };

            if (name == listMarker)
            {
                for (auto position { lineText.indexOf (marker) }; position >= 0;)
                {
                    const auto isAtColumnZero { position == 0 and templateLine.indexOf (marker) == 0 };
                    const auto value { getMarkerValue (model, templateDocument, tables, rows, lines,
                        descriptionTable, name, tokenOccurrence, joinText, parentIndent, isAtColumnZero,
                        extension, templateLine, marker) };
                    lineText = lineText.replaceSection (position, marker.length(), value);
                    ++tokenOccurrence;
                    position = lineText.indexOf (position + value.length(), marker);
                }
            }
            else
            {
                const auto isAtColumnZero { templateLine.indexOf (marker) == 0 };
                const auto value { getMarkerValue (model, templateDocument, tables, rows, lines,
                    descriptionTable, name, tokenOccurrence, joinText, parentIndent, isAtColumnZero,
                    extension, templateLine, marker) };
                lineText = jam::Format::replaceholder (lineText, marker.substring (
                    Id::tripleColon.length(), marker.length() - Id::tripleColon.length()), value);
                ++tokenOccurrence;
            }
        }
    }

    return lineText;
}

juce::String Shapes::getLines (const Model& model, const TemplateDocument& templateDocument,
    const jam::Array<const Element*>& tables, const jam::Array<const Element*>& rows,
    const jam::Array<const Element*>& lines, const Element& line,
    const jam::Document::Identifiers& tokens, const juce::String& joinText, int parentIndent,
    const juce::String& extension)
{
    static const auto newlineText { juce::String::charToString (Chars::newline) };

    jam::Strings textLines;
    auto previousLineIsEmpty { false };
    jam::HashMap<juce::Identifier, int> occurrence;

    for (const auto& templateLine : jam::Strings::fromLines (
             *templateDocument.getCodeBlock (line)->get<juce::String> (Id::value)))
    {
        const auto lineText { getSubstitutedLine (model, templateDocument, tables, rows, lines,
            tokens, occurrence, templateLine, joinText, parentIndent, extension) };
        const auto lineHasPlaceholder { Items::getMarkers (templateLine).size() > 0 };
        const auto lineIsEmpty { lineHasPlaceholder and lineText.trim().isEmpty() };

        if (lineIsEmpty)
            previousLineIsEmpty = true;
        else if (previousLineIsEmpty and lineText.trim().isEmpty())
            previousLineIsEmpty = false;
        else
        {
            textLines.add (lineText);
            previousLineIsEmpty = false;
        }
    }

    return textLines.joinIntoString (newlineText, 0, -1);
}

juce::String Shapes::getIndentedText (const juce::String& text, int level, int parentIndent,
    bool isAtColumnZero)
{
    static const auto spaceText { juce::String::charToString (Chars::space) };
    static const auto newlineText { juce::String::charToString (Chars::newline) };

    auto indentedText { text };

    if (isAtColumnZero)
    {
        const auto indent { level * indentWidth - parentIndent };

        if (indent != 0)
        {
            const auto prefix { juce::String::repeatedString (spaceText, indent) };
            jam::Strings prefixedLines;

            for (const auto& textLine : jam::Strings::fromLines (text))
                prefixedLines.add (textLine.isNotEmpty() ? prefix + textLine : textLine);

            indentedText = prefixedLines.joinIntoString (newlineText, 0, -1);
        }
    }

    return indentedText;
}

juce::String Shapes::getShapeText (const Model& model, const TemplateDocument& templateDocument,
    const jam::Array<const Element*>& tables, const Element& row, const Element& line, const juce::String& joinText,
    int parentIndent, bool isAtColumnZero, const juce::String& extension)
{
    jam::Array<const Element*> rows;
    jam::Array<const Element*> sourceLines;
    rows.add (&row);
    sourceLines.add (&line);

    return getShapeText (model, templateDocument, tables, rows, sourceLines, joinText, parentIndent,
        isAtColumnZero, extension);
}

juce::String Shapes::getShapeText (const Model& model, const TemplateDocument& templateDocument,
    const jam::Array<const Element*>& tables, const jam::Array<const Element*>& rows,
    const jam::Array<const Element*>& sourceLines, const juce::String& joinText, int parentIndent,
    bool isAtColumnZero, const juce::String& extension)
{
    const auto text { getShape (model, templateDocument, tables, rows, sourceLines, joinText,
        *sourceLines.first()->get<int> (Id::level) * indentWidth, extension) };

    return getIndentedText (
        text, *sourceLines.first()->get<int> (Id::level), parentIndent, isAtColumnZero);
}

bool Shapes::isListMarkerInline (const TemplateDocument& templateDocument, const Element& sourceLine)
{
    static const auto listMarker { Model::getReservedName (Id::list) };

    const auto& shapeTokens { *templateDocument.getCodeBlock (sourceLine)
                                    ->get<jam::Document::Identifiers> (Id::placeholder) };

    if (std::find (shapeTokens.begin(), shapeTokens.end(), listMarker) == shapeTokens.end())
        return false;

    const auto& shapeValue {
        *templateDocument.getCodeBlock (sourceLine)->get<juce::String> (Id::value)
    };
    const auto marker { Items::getMarker (shapeValue, listMarker) };

    for (const auto& shapeLine : jam::Strings::fromLines (shapeValue))
    {
        const auto markerPosition { shapeLine.indexOf (marker) };

        if (markerPosition >= 0)
            return markerPosition != 0;
    }

    return false;
}

juce::String Shapes::getItemText (const Model& model, const TemplateDocument& templateDocument,
    const jam::Array<const Element*>& tables, const jam::Array<const Element*>& rows,
    const jam::Array<const Element*>& sourceLines, const jam::Array<const Element*>& shapeLines,
    const juce::String& joinText, int parentIndent, bool isAtColumnZero, const juce::String& extension)
{
    static const auto newlineText { juce::String::charToString (Chars::newline) };

    jam::Strings itemTexts;

    for (int index { 0 }; index < rows.size(); ++index)
    {
        const auto& row { *rows.at (index) };
        const auto& sourceLine { *sourceLines.at (index) };
        const auto indent { *sourceLine.get<int> (Id::level) };
        const auto ordinal { *sourceLine.get<int> (Id::line) };
        const auto sourceValue {
            *model.getSource (row, indent, ordinal)->get<juce::String> (Id::value)
        };
        const auto listMarkerIsInline { isListMarkerInline (templateDocument, sourceLine) };
        auto* separatorLine { model.getSeparator (row, indent, ordinal) };
        const auto value { separatorLine != nullptr
                                ? templateDocument.getValue (
                                      model, row, *separatorLine->get<juce::String> (Id::value))
                                : juce::String{} };
        const auto join { not listMarkerIsInline and value.isNotEmpty()
                               ? value
                               : newlineText };
        const auto childJoin { value };
        const auto itemText { Items::getJoinedItems (model, templateDocument, tables, row,
            sourceValue, sourceLine, *shapeLines.at (index), join, indent, ordinal, childJoin, extension) };

        if (not itemTexts.contains (itemText, false))
            itemTexts.add (itemText);
    }

    const auto text { itemTexts.joinIntoString (joinText, 0, -1) };

    return getIndentedText (
        text, *sourceLines.first()->get<int> (Id::level), parentIndent, isAtColumnZero);
}

int Shapes::getAvailableCount (const Model& model, const TemplateDocument& templateDocument,
    const Element& structureLine, int arity)
{
    auto availableCount { 0 };

    while (availableCount < arity
           and getSourceLine (model, templateDocument, structureLine, availableCount) != nullptr)
        ++availableCount;

    return availableCount;
}

juce::String Shapes::getFill (const Model& model, const TemplateDocument& templateDocument,
    const jam::Array<const Element*>& tables, const jam::Array<const Element*>& rows,
    const jam::Array<const Element*>& lines, int occurrence, const juce::String& joinText,
    int parentIndent, bool isAtColumnZero, const juce::String& extension)
{
    jam::Array<const Element*> shapeRows;
    jam::Array<const Element*> shapeSourceLines;
    jam::Array<const Element*> itemRows;
    jam::Array<const Element*> itemSourceLines;
    jam::Array<const Element*> itemShapeLines;

    for (int index { 0 }; index < rows.size(); ++index)
    {
        auto* structureLine { lines.at (index) };
        const auto arity { Items::getArity (templateDocument, *structureLine) };
        const auto availableCount { getAvailableCount (model, templateDocument, *structureLine, arity) };
        const auto skippedCount { arity - availableCount };

        if (occurrence >= skippedCount)
        {
            auto* sourceLine { getSourceLine (
                model, templateDocument, *structureLine, occurrence - skippedCount) };
            jassert (sourceLine != nullptr);

            if (sourceLine->isTag (Id::p))
            {
                shapeRows.add (rows.at (index));
                shapeSourceLines.add (sourceLine);
            }
            else
            {
                itemRows.add (rows.at (index));
                itemSourceLines.add (sourceLine);
                itemShapeLines.add (structureLine);
            }
        }
    }

    jam::Strings texts;

    if (not shapeRows.isEmpty())
        texts.add (getShapeText (model, templateDocument, tables, shapeRows, shapeSourceLines,
            joinText, parentIndent, isAtColumnZero, extension));

    if (not itemRows.isEmpty())
        texts.add (getItemText (model, templateDocument, tables, itemRows, itemSourceLines,
            itemShapeLines, joinText, parentIndent, isAtColumnZero, extension));

    return texts.joinIntoString (joinText, 0, -1);
}

juce::String Shapes::getGroupKey (const Model& model, const TemplateDocument& templateDocument,
    const jam::Array<const Element*>& tables, const Element& row, const Element& line, const juce::String& joinText,
    int parentIndent, const juce::String& extension)
{
    static const auto newlineText { juce::String::charToString (Chars::newline) };
    static const auto listMarker { Model::getReservedName (Id::list) };

    auto* descriptionTable { getDescriptionTable (model, templateDocument, row, line) };
    const auto& tokens { *templateDocument.getCodeBlock (line)
                               ->get<jam::Document::Identifiers> (Id::placeholder) };
    jam::Strings key;
    key.add (*line.get<juce::String> (Id::templatePath));
    key.add (*line.get<juce::String> (Id::info));

    for (const auto& name : tokens)
        if (name != listMarker)
            key.add (getTokenValue (model, templateDocument, tables, row, line, descriptionTable,
                name, joinText, parentIndent, false, extension));

    return key.joinIntoString (newlineText, 0, -1);
}

juce::String Shapes::getGroupText (const Model& model, const TemplateDocument& templateDocument,
    const jam::Array<const Element*>& tables, const jam::Array<const Element*>& rows,
    const jam::Array<const Element*>& lines, const jam::Array<int>& indices, const juce::String& joinText,
    int parentIndent, const juce::String& extension)
{
    jam::Array<const Element*> groupRows;
    jam::Array<const Element*> groupLines;

    for (const auto candidateIndex : indices)
    {
        groupRows.add (rows.at (candidateIndex));
        groupLines.add (lines.at (candidateIndex));
    }

    const auto& tokens { *templateDocument.getCodeBlock (*groupLines.first())
                               ->get<jam::Document::Identifiers> (Id::placeholder) };

    return getLines (model, templateDocument, tables, groupRows, groupLines,
        *groupLines.first(), tokens, joinText, parentIndent, extension);
}

juce::String Shapes::getShape (const Model& model, const TemplateDocument& templateDocument,
    const jam::Array<const Element*>& tables, const jam::Array<const Element*>& rows,
    const jam::Array<const Element*>& lines, const juce::String& joinText, int parentIndent,
    const juce::String& extension)
{
    jam::Strings keys;

    for (int index { 0 }; index < rows.size(); ++index)
        keys.add (getGroupKey (model, templateDocument, tables, *rows.at (index), *lines.at (index),
            joinText, parentIndent, extension));

    jam::HashMap<juce::String, jam::Array<int>> groupIndices;

    for (int index { 0 }; index < rows.size(); ++index)
        groupIndices[keys.at (index)].add (index);

    jam::Strings groupTexts;

    for (const auto& [key, indices] : groupIndices)
        groupTexts.add (getGroupText (model, templateDocument, tables, rows, lines, indices,
            joinText, parentIndent, extension));

    return groupTexts.joinIntoString (joinText, 0, -1);
}
