#pragma once
#include <JuceHeader.h>
#include "Model.h"
#include "TemplateDocument.h"

struct Validator : jam::MarkdownValidator
{
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

                            if (model.isTemplatePath (*row, cell)
                                and not model.getFile (*row, cell).existsAsFile())
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
                        model.getFile (*row, bodyAlias)) };
                    auto placeholders { document.getPlaceholders (model, *row) };

                    jam::Strings wrapperColumns;

                    for (const auto& column : columns)
                        if (column != firstColumn)
                        {
                            const auto cell {
                                model.getTableValue (*row, juce::Identifier (column))
                            };

                            if (model.isTemplatePath (*row, cell))
                                wrapperColumns.addIfNotAlreadyThere (column, false);
                        }

                    for (const auto& wrapperColumn : wrapperColumns)
                    {
                        const auto wrapperAlias {
                            model.getTableValue (*row, juce::Identifier (wrapperColumn))
                        };
                        const auto& wrapper { TemplateDocument::getOrCreate (
                            model.getFile (*row, wrapperAlias)) };

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

                jam::Strings reserved { firstColumn, Id::file.toString(),
                                        Id::capacity.toString(), Id::token.toString(),
                                        Id::special.toString() };

                for (const auto& column : columns)
                    if (column.endsWith (Id::lineBreak.toString()))
                        reserved.addIfNotAlreadyThere (column, false);

                for (auto* row : model.getTableRows (*table))
                {
                    jam::Strings rowReserved { reserved };

                    for (const auto& column : columns)
                        if (column != firstColumn)
                        {
                            const auto cell {
                                model.getTableValue (*row, juce::Identifier (column))
                            };

                            if (model.isTemplatePath (*row, cell))
                                rowReserved.addIfNotAlreadyThere (column, false);
                        }

                    for (const auto& column : columns)
                    {
                        const auto cell { model.getTableValue (*row, juce::Identifier (column)) };

                        if (cell.isNotEmpty() and not rowReserved.contains (column, false)
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
            const auto pathCell { model.getTableValue (*row, Id::symbol) };

            if (pathCell.isEmpty())
                return juce::Result::fail (getLocation (indexTable, *row, Id::symbol.toString())
                                           + Id::diagnosticSeparator + text::en::failNotFound);

            if (pathCell.endsWith (markdownExtension)
                and not model.getOutput (pathCell).existsAsFile())
                return juce::Result::fail (getLocation (indexTable, *row, Id::symbol.toString())
                                           + Id::diagnosticSeparator
                                           + text::en::failOutputMissing + Id::diagnosticSeparator
                                           + pathCell);

            if (pathCell.endsWith (templateExtension)
                and not model.getOutput (pathCell).existsAsFile())
                return juce::Result::fail (getLocation (indexTable, *row, Id::symbol.toString())
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
                jam::Strings seen;

                for (auto* row : model.getTableRows (*table))
                {
                    const auto value { model.getTableValue (*row, Id::name) };

                    if (seen.contains (value, false))
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

    const Rules& getRules() const override
    {
        static const Rules rules {
            []
            {
                Rules rules;

                rules.add<const jam::Document&> (Id::index.toString(),
                    [] (const jam::Document& document) -> juce::Result
                    {
                        return isValid (static_cast<const Model&> (document));
                    });

                return rules;
            }()
        };

        return rules;
    }
};
