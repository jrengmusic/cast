#pragma once
#include <JuceHeader.h>
#include "Items.h"
#include "Model.h"
#include "TemplateDocument.h"

struct Shapes
{
    using Element = Model::Element;

    static constexpr int indentWidth { 4 };

    static int getArity (const TemplateDocument& templateDocument, Element& line)
    {
        const juce::Identifier shapeId { *line.get<juce::String> (Id::templatePath) };
        const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                   ->get<jam::Document::Identifiers> (Id::placeholder) };

        return static_cast<int> (std::count (tokens.begin(), tokens.end(), Id::list));
    }

    static Element*
    getLineAfter (const Model& model, const TemplateDocument& templateDocument, Element& line)
    {
        auto* cursor { model.getNextLine (line) };

        if (line.isTag (Id::p))
            for (int occurrence { 0 }; occurrence < getArity (templateDocument, line); ++occurrence)
                cursor = getLineAfter (model, templateDocument, *cursor);

        return cursor;
    }

    static Element* getSourceLine (const Model& model, const TemplateDocument& templateDocument,
        Element& line, int occurrence)
    {
        auto* cursor { model.getNextLine (line) };

        for (int index { 0 }; index < occurrence; ++index)
            cursor = getLineAfter (model, templateDocument, *cursor);

        return cursor;
    }

    static Element* getFirstLine (const Model& model, Element& row)
    {
        Element* line { nullptr };

        model.getTableCell (row, Id::structure)
            ->applyFunctionRecursively (
                [&line] (const Element& candidate) -> bool
                {
                    if (line == nullptr and candidate.contains (Id::shape)
                        and *candidate.get<int> (Id::shape) == 0
                        and (candidate.isTag (Id::p) or candidate.id == Id::list))
                        line = const_cast<Element*> (&candidate);

                    return line == nullptr;
                });

        return line;
    }

    static Element* getCommentTable (const Model& model, const TemplateDocument& templateDocument,
        Element& row, Element& line, int listOccurrence)
    {
        if (not line.isTag (Id::p))
            return model.getTable (row,
                *model.getSource (row, *line.get<int> (Id::level), *line.get<int> (Id::line))
                     ->get<juce::String> (Id::value));

        if (listOccurrence < getArity (templateDocument, line))
        {
            auto* sourceLine { getSourceLine (model, templateDocument, line, listOccurrence) };

            if (not sourceLine->isTag (Id::p))
                return getCommentTable (model, templateDocument, row, *sourceLine, 0);
        }

        if (row.parent->contains (Id::comment) and row.parent->get<juce::String> (Id::comment)->isNotEmpty())
            return row.parent;

        return nullptr;
    }

    static juce::String getTokenValue (const Model& model, const TemplateDocument& templateDocument,
        Element& row, Element& line, Element* commentTable, const juce::Identifier& name)
    {
        if (auto* binding { model.getBinding (row, Id::structure, line, name) })
            return templateDocument.getBinding (model, row, *binding->get<juce::String> (Id::value));

        if (name == Id::comment)
            return commentTable != nullptr ? *commentTable->get<juce::String> (Id::comment)
                                           : juce::String();

        if (auto* cell { model.getTableCell (row, name) })
            return *cell->get<juce::String> (Id::value);

        return {};
    }

    static std::pair<juce::String, bool> getSubstitutedLine (const Model& model,
        const TemplateDocument& templateDocument, const jam::Array<Element*>& tables,
        const jam::Array<Element*>& rows, const jam::Array<Element*>& lines, Element* commentTable,
        const jam::Document::Identifiers& tokens, jam::HashMap<juce::Identifier, int>& occurrence,
        const juce::String& templateLine, const juce::String& joinText, int parentIndent,
        const juce::String& extension)
    {
        auto lineText { templateLine };
        auto lineHasPlaceholder { false };

        for (const auto& name : tokens)
            if (jam::Format::hasPlaceholder (lineText, name.toString()))
            {
                lineHasPlaceholder = true;
                const auto isAtColumnZero { templateLine.indexOf (Items::getMarker (name)) == 0 };
                const auto listOccurrence { occurrence[Id::list] };
                auto& tokenOccurrence { occurrence[name] };
                auto* markerCommentTable { name == Id::comment
                                               ? getCommentTable (model, templateDocument, *rows.first(),
                                                     *lines.first(), listOccurrence)
                                               : commentTable };
                auto value { name == Id::list
                                 ? getFill (model, templateDocument, tables, rows, lines,
                                       tokenOccurrence, joinText, parentIndent, isAtColumnZero, extension)
                                 : getTokenValue (model, templateDocument, *rows.first(), *lines.first(),
                                       markerCommentTable, name) };

                if (name == Id::comment and value.isNotEmpty())
                    value = templateLine.trim() == Items::getMarker (name)
                                ? Transforms::toCommentBlock (value, extension)
                                : Transforms::toComment (value, extension);

                lineText = jam::Format::replaceholder (lineText, name.toString(), value);
                ++tokenOccurrence;
            }

        return { lineText, lineHasPlaceholder };
    }

    static juce::String getLines (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<Element*>& tables, const jam::Array<Element*>& rows,
        const jam::Array<Element*>& lines, Element* commentTable, const juce::Identifier& shapeId,
        const jam::Document::Identifiers& tokens, const juce::String& joinText, int parentIndent,
        const juce::String& extension)
    {
        jam::Strings textLines;
        auto previousLineIsEmpty { false };
        jam::HashMap<juce::Identifier, int> occurrence;

        for (const auto& templateLine : jam::Strings::fromLines (
                 *templateDocument.getCodeBlock (shapeId)->get<juce::String> (Id::value)))
        {
            const auto [lineText, lineHasPlaceholder] { getSubstitutedLine (model, templateDocument,
                tables, rows, lines, commentTable, tokens, occurrence, templateLine, joinText,
                parentIndent, extension) };
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

        return textLines.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
    }

    static juce::String getFill (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<Element*>& tables, const jam::Array<Element*>& rows,
        const jam::Array<Element*>& lines, int occurrence, const juce::String& joinText,
        int parentIndent, bool isAtColumnZero, const juce::String& extension)
    {
        jam::Array<Element*> sourceLines;

        for (auto* structureLine : lines)
            sourceLines.add (getSourceLine (model, templateDocument, *structureLine, occurrence));

        juce::String text;

        if (sourceLines.first()->isTag (Id::p))
        {
            text = getShape (model, templateDocument, tables, rows, sourceLines, joinText,
                *sourceLines.first()->get<int> (Id::level) * indentWidth, extension);
        }
        else
        {
            jam::Strings itemTexts;

            for (int index { 0 }; index < rows.size(); ++index)
            {
                auto& row { *rows.at (index) };
                auto& sourceLine { *sourceLines.at (index) };
                const auto indent { *sourceLine.get<int> (Id::level) };
                const auto ordinal { *sourceLine.get<int> (Id::line) };
                const auto sourceValue {
                    *model.getSource (row, indent, ordinal)->get<juce::String> (Id::value)
                };
                const auto shapeId { juce::Identifier (*sourceLine.get<juce::String> (Id::templatePath)) };
                auto* separatorLine { model.getSeparator (row, indent, ordinal) };
                const auto join { separatorLine != nullptr
                                       ? templateDocument.getBinding (
                                             model, row, *separatorLine->get<juce::String> (Id::value))
                                       : juce::String::charToString (Chars::newline) };
                auto* childSourceLine { model.getSource (row, indent + 1, 0) };
                const auto childSource { childSourceLine != nullptr
                                              ? *childSourceLine->get<juce::String> (Id::value)
                                              : juce::String() };
                auto* childSeparatorLine { model.getSeparator (row, indent + 1, 0) };
                const auto childJoin { childSeparatorLine != nullptr
                                            ? templateDocument.getBinding (model, row,
                                                  *childSeparatorLine->get<juce::String> (Id::value))
                                            : juce::String() };
                const auto itemText { Items::getJoinedItems (model, templateDocument, tables, row,
                    sourceValue, shapeId, join, childSource, childJoin, extension) };

                if (not itemTexts.contains (itemText, false))
                    itemTexts.add (itemText);
            }

            text = itemTexts.joinIntoString (joinText, 0, -1);
        }

        if (isAtColumnZero)
        {
            const auto indent { *sourceLines.first()->get<int> (Id::level) * indentWidth - parentIndent };

            if (indent != 0)
            {
                const auto prefix { juce::String::repeatedString (
                    juce::String::charToString (Chars::space), indent) };
                jam::Strings prefixedLines;

                for (const auto& textLine : jam::Strings::fromLines (text))
                    prefixedLines.add (textLine.isNotEmpty() ? prefix + textLine : textLine);

                text = prefixedLines.joinIntoString (juce::String::charToString (Chars::newline), 0, -1);
            }
        }

        return text;
    }

    static juce::String getShape (const Model& model, const TemplateDocument& templateDocument,
        const jam::Array<Element*>& tables, const jam::Array<Element*>& rows,
        const jam::Array<Element*>& lines, const juce::String& joinText, int parentIndent,
        const juce::String& extension)
    {
        jam::Strings keys;

        for (int index { 0 }; index < rows.size(); ++index)
        {
            auto& row { *rows.at (index) };
            auto& line { *lines.at (index) };
            auto* commentTable { getCommentTable (model, templateDocument, row, line, 0) };
            const auto shapeId { juce::Identifier (*line.get<juce::String> (Id::templatePath)) };
            const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                       ->get<jam::Document::Identifiers> (Id::placeholder) };
            jam::Strings key;
            key.add (shapeId.toString());

            for (const auto& name : tokens)
                if (name != Id::list)
                    key.add (getTokenValue (model, templateDocument, row, line, commentTable, name));

            keys.add (key.joinIntoString (juce::String::charToString (Chars::newline), 0, -1));
        }

        jam::Strings groupTexts;
        jam::Strings seenKeys;

        for (int index { 0 }; index < rows.size(); ++index)
            if (not seenKeys.contains (keys.at (index), true))
            {
                seenKeys.add (keys.at (index));

                jam::Array<Element*> groupRows;
                jam::Array<Element*> groupLines;

                for (int candidate { 0 }; candidate < rows.size(); ++candidate)
                    if (keys.at (candidate) == keys.at (index))
                    {
                        groupRows.add (rows.at (candidate));
                        groupLines.add (lines.at (candidate));
                    }

                auto* commentTable {
                    getCommentTable (model, templateDocument, *groupRows.first(), *groupLines.first(), 0)
                };
                const auto shapeId {
                    juce::Identifier (*groupLines.first()->get<juce::String> (Id::templatePath))
                };
                const auto& tokens { *templateDocument.getCodeBlock (shapeId)
                                           ->get<jam::Document::Identifiers> (Id::placeholder) };

                groupTexts.add (getLines (model, templateDocument, tables, groupRows, groupLines,
                    commentTable, shapeId, tokens, joinText, parentIndent, extension));
            }

        return groupTexts.joinIntoString (joinText, 0, -1);
    }
};
