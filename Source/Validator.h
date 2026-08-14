#pragma once
#include <JuceHeader.h>
#include "Model.h"
#include "TemplateDocument.h"

struct Validator
{
    using Element = jam::Document::Element;

    static juce::Result isValid (const Model& model)
    {
        juce::Result firstFailure { isManifest (model) };

        if (firstFailure.wasOk())
        {
            juce::CriticalSection failureLock;
            const auto tables { model.getTables() };

            runJobs (tables.size(),
                [&model, &tables, &failureLock, &firstFailure] (int index)
                {
                    if (const auto result { isTable (model, *tables.at (index)) }; not result.wasOk())
                    {
                        const juce::ScopedLock lock { failureLock };

                        if (firstFailure.wasOk())
                            firstFailure = result;
                    }
                });

            const auto constraintRows { model.getTableRows (Id::constraints) };

            runJobs (constraintRows.size(),
                [&model, &constraintRows, &failureLock, &firstFailure] (int index)
                {
                    auto* row { constraintRows.at (index) };
                    const auto column { model.getTableValue (*row, Id::column) };
                    const auto predicateCell { model.getTableValue (*row, Id::predicate) };
                    const auto name { jam::Format::upTo (
                        predicateCell, juce::String::charToString (chars::space), false) };
                    const auto args { jam::Format::from (
                        predicateCell, juce::String::charToString (chars::space), false).trim() };

                    const auto result { getPredicates().contains (name)
                                            ? getPredicates().get (name, model, column, args)
                                            : juce::Result::fail (text::en::failUnknownPredicate
                                                                  + Id::diagnosticSeparator + name) };

                    if (not result.wasOk())
                    {
                        const juce::ScopedLock lock { failureLock };

                        if (firstFailure.wasOk())
                            firstFailure = result;
                    }
                });
        }

        return firstFailure;
    }

    template <typename Function>
    static juce::Result
    forEachCell (const Model& model, const juce::String& column, Function&& function)
    {
        for (auto* table : model.getTables())
            if (model.getTableHeaders (*table).contains (column))
                for (auto* row : model.getTableRows (*table))
                    if (const auto result { function (
                            *table, *row, model.getTableValue (*row, juce::Identifier (column))) };
                        not result.wasOk())
                        return result;

        return juce::Result::ok();
    }

    static juce::Result
    matches (const Model& model, const juce::String& column, const juce::String& args)
    {
        const std::regex pattern { args.toStdString() };

        return forEachCell (model, column,
            [&pattern, &column] (Element& table, Element& row, const juce::String& value) -> juce::Result
            {
                if (not std::regex_match (value.toStdString(), pattern))
                    return juce::Result::fail (getLocation (table, row, column)
                                               + Id::diagnosticSeparator + Id::matches
                                               + Id::diagnosticSeparator + text::en::failNoMatch
                                               + Id::diagnosticSeparator + value);

                return juce::Result::ok();
            });
    }

    static juce::Result
    unique (const Model& model, const juce::String& column, const juce::String& args)
    {
        for (auto* table : model.getTables())
            if (model.getTableHeaders (*table).contains (column))
            {
                juce::StringArray seen;

                for (auto* row : model.getTableRows (*table))
                {
                    const auto value { model.getTableValue (*row, juce::Identifier (column)) };

                    if (seen.contains (value))
                        return juce::Result::fail (getLocation (*table, *row, column)
                                                   + Id::diagnosticSeparator + Id::unique
                                                   + Id::diagnosticSeparator
                                                   + text::en::failDuplicate + value
                                                   + juce::String::charToString (chars::doubleQuote));

                    seen.add (value);
                }
            }

        return juce::Result::ok();
    }

    static juce::Result
    existsIn (const Model& model, const juce::String& column, const juce::String& args)
    {
        const juce::Identifier targetTable { jam::Format::upTo (
            args, juce::String::charToString (chars::dot), false) };
        const auto targetKeys { model.getTableRowKeys (targetTable) };

        return forEachCell (model, column,
            [&targetKeys, &column, &args] (Element& table, Element& row, const juce::String& value) -> juce::Result
            {
                if (not targetKeys.contains (value))
                    return juce::Result::fail (getLocation (table, row, column)
                                               + Id::diagnosticSeparator + Id::existsIn
                                               + Id::diagnosticSeparator
                                               + text::en::failForeignKeyMissing + args);

                return juce::Result::ok();
            });
    }

    static juce::Result
    oneOf (const Model& model, const juce::String& column, const juce::String& args)
    {
        const auto choices { juce::StringArray::fromTokens (
            args, juce::String::charToString (chars::pipe), {}) };

        return forEachCell (model, column,
            [&choices, &column] (Element& table, Element& row, const juce::String& value) -> juce::Result
            {
                if (not choices.contains (value))
                    return juce::Result::fail (
                        getLocation (table, row, column) + Id::diagnosticSeparator + Id::oneOf
                        + Id::diagnosticSeparator + text::en::failNotInSet + value);

                return juce::Result::ok();
            });
    }

    static juce::Result
    range (const Model& model, const juce::String& column, const juce::String& args)
    {
        return forEachCell (model, column,
            [&model, &column] (Element& table, Element& row, const juce::String& value) -> juce::Result
            {
                const auto doubleValue { value.getDoubleValue() };
                const auto minValue { model.getTableValue (row, Id::min).getDoubleValue() };
                const auto maxValue { model.getTableValue (row, Id::max).getDoubleValue() };

                if (doubleValue < minValue or doubleValue > maxValue)
                    return juce::Result::fail (
                        getLocation (table, row, column) + Id::diagnosticSeparator + Id::range
                        + Id::diagnosticSeparator + text::en::failOutOfRange
                        + juce::String (maxValue));

                return juce::Result::ok();
            });
    }

    static juce::Result
    parity (const Model& model, const juce::String& column, const juce::String& args)
    {
        const juce::Identifier targetTable { jam::Format::upTo (
            args, juce::String::charToString (chars::dot), false) };
        const juce::Identifier targetColumn { jam::Format::from (
            args, juce::String::charToString (chars::dot), false) };

        juce::StringArray localKeys;
        for (auto* table : model.getTables())
            if (model.getTableHeaders (*table).contains (column))
                for (auto* row : model.getTableRows (*table))
                    localKeys.addIfNotAlreadyThere (
                        model.getTableValue (*row, juce::Identifier (column)));

        juce::StringArray targetKeys;
        for (auto* row : model.getTableRows (targetTable))
            targetKeys.addIfNotAlreadyThere (model.getTableValue (*row, targetColumn));

        for (const auto& key : localKeys)
            if (not targetKeys.contains (key))
                return juce::Result::fail (Id::parity + Id::diagnosticSeparator
                                           + text::en::failRefMissing + Id::diagnosticSeparator
                                           + key);

        for (const auto& key : targetKeys)
            if (not localKeys.contains (key))
                return juce::Result::fail (Id::parity + Id::diagnosticSeparator
                                           + text::en::failLocalMissing + Id::diagnosticSeparator
                                           + key);

        return juce::Result::ok();
    }

    static juce::Result
    fileExists (const Model& model, const juce::String& column, const juce::String& args)
    {
        return forEachCell (model, column,
            [&model, &column, &args] (Element& table, Element& row, const juce::String& value) -> juce::Result
            {
                if (not model.getOutput (args).getChildFile (value).existsAsFile())
                    return juce::Result::fail (
                        getLocation (table, row, column) + Id::diagnosticSeparator
                        + Id::fileExists + Id::diagnosticSeparator + text::en::failOutputMissing
                        + Id::diagnosticSeparator + value);

                return juce::Result::ok();
            });
    }

    static juce::Result
    onePerGroup (const Model& model, const juce::String& column, const juce::String& args)
    {
        const juce::Identifier groupColumn { args };

        for (auto* table : model.getTables())
            if (model.getTableHeaders (*table).contains (column)
                and model.getTableHeaders (*table).contains (args))
            {
                juce::StringArray markedGroups;
                juce::StringArray allGroups;
                jam::Array<Element*> firstRows;

                for (auto* row : model.getTableRows (*table))
                {
                    const auto group { model.getTableValue (*row, groupColumn) };

                    if (not allGroups.contains (group))
                    {
                        allGroups.add (group);
                        firstRows.add (row);
                    }

                    if (model.hasTableValue (*row, juce::Identifier (column)))
                    {
                        if (markedGroups.contains (group))
                            return juce::Result::fail (
                                getLocation (*table, *row, column) + Id::diagnosticSeparator
                                + Id::onePerGroup + Id::diagnosticSeparator
                                + text::en::failGroupOpen + group + text::en::failGroupClose);

                        markedGroups.add (group);
                    }
                }

                for (int index { 0 }; index < allGroups.size(); ++index)
                    if (not markedGroups.contains (allGroups[index]))
                        return juce::Result::fail (
                            getLocation (*table, *firstRows.at (index), column)
                            + Id::diagnosticSeparator + Id::onePerGroup + Id::diagnosticSeparator
                            + text::en::failGroupOpen + allGroups[index]
                            + text::en::failGroupClose);
            }

        return juce::Result::ok();
    }

    static const jam::Function::Map<juce::String, juce::Result>& getPredicates() noexcept
    {
        static const jam::Function::Map<juce::String, juce::Result> predicates {
            []()
            {
                jam::Function::Map<juce::String, juce::Result> map;

                map.add<const Model&, const juce::String&, const juce::String&> (
                    Id::matches, &matches);
                map.add<const Model&, const juce::String&, const juce::String&> (
                    Id::unique, &unique);
                map.add<const Model&, const juce::String&, const juce::String&> (
                    Id::existsIn, &existsIn);
                map.add<const Model&, const juce::String&, const juce::String&> (Id::oneOf, &oneOf);
                map.add<const Model&, const juce::String&, const juce::String&> (Id::range, &range);
                map.add<const Model&, const juce::String&, const juce::String&> (
                    Id::parity, &parity);
                map.add<const Model&, const juce::String&, const juce::String&> (
                    Id::fileExists, &fileExists);
                map.add<const Model&, const juce::String&, const juce::String&> (
                    Id::onePerGroup, &onePerGroup);

                return map;
            }()
        };

        return predicates;
    }

    static bool isOutputTable (const Model& model, Element& table) noexcept
    {
        return not table.isTag (Id::index)
               and model.getTableHeaders (table).contains (Id::file.toString());
    }

    static juce::Result isTable (const Model& model, Element& table)
    {
        const auto headers { model.getTableHeaders (table) };

        for (auto* row : model.getTableRows (table))
        {
            int columnIndex { 0 };

            for (auto* cell : *row)
            {
                const auto hazard { getHazard (*cell) };

                if (not hazard.wasOk())
                    return juce::Result::fail (getLocation (table, *row, headers[columnIndex])
                                               + Id::diagnosticSeparator
                                               + hazard.getErrorMessage());

                ++columnIndex;
            }
        }

        return juce::Result::ok();
    }

    static juce::Result getHazard (Element& node)
    {
        juce::Result result { juce::Result::ok() };

        if (node.isTag (Id::a))
            result = juce::Result::fail (text::en::failHazardUri);

        if (result.wasOk() and node.isTag (Id::text)
            and node.get<juce::String> (Id::text)->containsAnyOf (Id::hazardChars))
            result = juce::Result::fail (text::en::failHazardAngleBrackets);

        if (result.wasOk() and not node.isTag (Id::code))
            for (auto* child = node.firstChild; child != nullptr and result.wasOk();
                 child = child->nextSibling)
                result = getHazard (*child);

        return result;
    }

    static juce::Result isTemplates (const Model& model)
    {
        for (auto* table : model.getTables())
            if (isOutputTable (model, *table))
            {
                const auto headers { model.getTableHeaders (*table) };

                for (auto* row : model.getTableRows (*table))
                    for (const auto& header : headers)
                        if (header.compare (Id::file.toString()) != 0)
                        {
                            const auto cell {
                                model.getTableValue (*row, juce::Identifier (header))
                            };

                            if (model.isTemplatePath (cell) and not model.getFile (cell).existsAsFile())
                                return juce::Result::fail (
                                    getLocation (*table, *row, header) + Id::diagnosticSeparator
                                    + text::en::failTemplateMissing + Id::diagnosticSeparator
                                    + cell);
                        }
            }

        return juce::Result::ok();
    }

    static jam::Array<juce::Identifier> getPlaceholders (const Model& model)
    {
        jam::Array<juce::Identifier> allPlaceholders;

        for (auto* table : model.getTables())
            if (isOutputTable (model, *table))
            {
                const auto columns { model.getTableHeaders (*table) };
                const auto& firstColumn { *columns.begin() };

                for (auto* row : model.getTableRows (*table))
                {
                    const auto bodyAlias { model.getTableValue (*row, firstColumn) };
                    const auto& document { TemplateDocument::getOrCreate (
                        model.getFile (bodyAlias)) };
                    auto placeholders { document.getPlaceholders (model, *row) };

                    juce::StringArray wrapperColumns;

                    for (const auto& column : columns)
                        if (column != firstColumn)
                        {
                            const auto cell {
                                model.getTableValue (*row, juce::Identifier (column))
                            };

                            if (model.isTemplatePath (cell))
                                wrapperColumns.addIfNotAlreadyThere (column);
                        }

                    for (const auto& wrapperColumn : wrapperColumns)
                    {
                        const auto wrapperAlias {
                            model.getTableValue (*row, juce::Identifier (wrapperColumn))
                        };
                        const auto& wrapper { TemplateDocument::getOrCreate (
                            model.getFile (wrapperAlias)) };

                        for (const auto& placeholder : wrapper.getPlaceholders (model, *row))
                            placeholders.addIfNotAlreadyThere (placeholder);
                    }

                    for (const auto& placeholder : placeholders)
                        allPlaceholders.addIfNotAlreadyThere (placeholder);
                }
            }

        return allPlaceholders;
    }

    static juce::Result
    isOrphanFree (const Model& model, const jam::Array<juce::Identifier>& allPlaceholders)
    {
        for (auto* table : model.getTables())
            if (isOutputTable (model, *table))
            {
                const auto columns { model.getTableHeaders (*table) };
                const auto& firstColumn { *columns.begin() };

                juce::StringArray reserved { firstColumn, Id::file.toString(),
                                             Id::capacity.toString(), Id::token.toString(),
                                             Id::special.toString() };

                for (const auto& column : columns)
                    if (column.endsWith (Id::lineBreak.toString()))
                        reserved.addIfNotAlreadyThere (column);

                for (auto* row : model.getTableRows (*table))
                {
                    juce::StringArray rowReserved { reserved };

                    for (const auto& column : columns)
                        if (column != firstColumn)
                        {
                            const auto cell {
                                model.getTableValue (*row, juce::Identifier (column))
                            };

                            if (model.isTemplatePath (cell))
                                rowReserved.addIfNotAlreadyThere (column);
                        }

                    for (const auto& column : columns)
                    {
                        const auto cell { model.getTableValue (*row, juce::Identifier (column)) };

                        if (cell.isNotEmpty() and not rowReserved.contains (column)
                            and not allPlaceholders.contains (juce::Identifier (column)))
                            return juce::Result::fail (getLocation (*table, *row, column)
                                                       + Id::diagnosticSeparator
                                                       + text::en::failOrphan);
                    }
                }
            }

        return juce::Result::ok();
    }

    static juce::Result isPlaceholders (const Model& model)
    {
        return isOrphanFree (model, getPlaceholders (model));
    }

    static juce::Result isIndex (const Model& model)
    {
        const auto indexTables { model.getTables (Id::index) };

        if (indexTables.isEmpty())
            return juce::Result::fail (Id::index.toString() + Id::diagnosticSeparator
                                       + text::en::failTableMissing);

        Element& indexTable { *indexTables.at (0) };
        const auto markdownExtension { juce::String::charToString (chars::dot) + extensions::md };
        const auto templateExtension { juce::String::charToString (chars::dot)
                                       + extensions::cast };

        for (auto* row : model.getTableRows (indexTable))
        {
            const auto pathCell { model.getTableValue (*row, Id::path) };

            if (pathCell.isEmpty())
                return juce::Result::fail (getLocation (indexTable, *row, Id::path.toString())
                                           + Id::diagnosticSeparator + text::en::failNotFound);

            if (pathCell.endsWith (markdownExtension)
                and not model.getOutput (pathCell).existsAsFile())
                return juce::Result::fail (getLocation (indexTable, *row, Id::path.toString())
                                           + Id::diagnosticSeparator
                                           + text::en::failOutputMissing + Id::diagnosticSeparator
                                           + pathCell);

            if (pathCell.endsWith (templateExtension)
                and not model.getOutput (pathCell).existsAsFile())
                return juce::Result::fail (getLocation (indexTable, *row, Id::path.toString())
                                           + Id::diagnosticSeparator
                                           + text::en::failTemplateMissing
                                           + Id::diagnosticSeparator + pathCell);
        }

        return juce::Result::ok();
    }

    static juce::Result isManifest (const Model& model)
    {
        if (const auto result { isIndex (model) }; not result.wasOk())
            return result;

        if (const auto result { isTemplates (model) }; not result.wasOk())
            return result;

        for (auto* table : model.getTables())
            if (model.getTableHeaders (*table).contains (Id::name.toString()))
            {
                juce::StringArray seen;

                for (auto* row : model.getTableRows (*table))
                {
                    const auto value { model.getTableValue (*row, Id::name) };

                    if (seen.contains (value))
                        return juce::Result::fail (getLocation (*table, *row, Id::name.toString())
                                                   + Id::diagnosticSeparator + Id::unique
                                                   + Id::diagnosticSeparator
                                                   + text::en::failDuplicate + value
                                                   + juce::String::charToString (chars::doubleQuote));

                    seen.add (value);
                }
            }

        if (const auto result { unique (model, Id::alias.toString(), {}) }; not result.wasOk())
            return result;

        return isPlaceholders (model);
    }

    static juce::String getLocation (Element& table, Element& row, const juce::String& column)
    {
        juce::String path;
        juce::String line;

        if (const auto* pathProperty { table.get<juce::String> (Id::path) };
            pathProperty != nullptr)
            path = *pathProperty;

        if (const auto* lineProperty { row.get<int> (Id::line) }; lineProperty != nullptr)
            line = juce::String (*lineProperty);

        return path + juce::String::charToString (chars::colon) + line
               + juce::String::charToString (chars::space)
               + juce::String::charToString (chars::openParen) + column
               + juce::String::charToString (chars::closeParen);
    }
};
