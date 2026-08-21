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
                        predicateCell, juce::String::charToString (Chars::space), false) };
                    const auto args { jam::Format::from (
                        predicateCell, juce::String::charToString (Chars::space), false).trim() };

                    const auto result { getPredicates().contains (name)
                                            ? getPredicates().get (name, model, column, args)
                                            : juce::Result::fail (text::Diagnostics::failUnknownPredicate
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
                        + Id::fileExists + Id::diagnosticSeparator + text::Diagnostics::failOutputMissing
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
            result = juce::Result::fail (text::Diagnostics::failHazardUri);

        if (result.wasOk() and node.isTag (Id::text)
            and node.get<juce::String> (Id::text)->containsAnyOf (Id::hazardChars))
            result = juce::Result::fail (text::Diagnostics::failHazardAngleBrackets);

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
                                    + text::Diagnostics::failTemplateMissing + Id::diagnosticSeparator
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
                const auto firstColumn { *model.getTableHeaders (*table).begin() };

                for (auto* row : model.getTableRows (*table))
                {
                    const auto bodyAlias { model.getTableValue (*row, firstColumn) };
                    const auto& document { TemplateDocument::getOrCreate (
                        model.getFile (*row, bodyAlias)) };

                    for (const auto& placeholder : document.getPlaceholders (model, *row))
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

                const jam::Strings reserved { firstColumn, Id::file.toString(),
                                              Id::structure.toString(), Id::separator.toString(),
                                              Id::capacity.toString(), Id::special.toString() };

                for (auto* row : model.getTableRows (*table))
                    for (const auto& column : columns)
                    {
                        const auto cell { model.getTableValue (*row, juce::Identifier (column)) };

                        if (cell.isNotEmpty() and not reserved.contains (column, false)
                            and not allPlaceholders.contains (juce::Identifier (column)))
                            return juce::Result::fail (getLocation (*table, *row, column)
                                                       + Id::diagnosticSeparator
                                                       + text::Diagnostics::failOrphan);
                    }
            }

        return juce::Result::ok();
    }

    static juce::Result isStructure (const Model& model)
    {
        for (auto* table : model.getTables())
            if (isOutputTable (model, *table))
            {
                jam::HashMap<juce::String, Element*> outermostByFile;
                jam::HashMap<juce::String, Element*> firstRowByFile;

                for (auto* row : model.getTableRows (*table))
                    if (auto* structure { model.getStructure (*row) })
                        for (auto* block : *structure)
                            if (block->isTag (Id::blockquote))
                            {
                                if (const auto result { TemplateDocument::isWrapHead (
                                        model, *table, *row, *block) };
                                    not result.wasOk())
                                    return result;

                                const auto origin { *table->get<juce::String> (Id::path) };
                                const auto file { model.getValue (
                                    origin, model.getTableValue (*row, Id::file)) };
                                auto* outermost { TemplateDocument::getOutermostWrap (*block) };

                                if (not outermostByFile.contains (file))
                                {
                                    outermostByFile.try_emplace (file, outermost);
                                    firstRowByFile.try_emplace (file, row);
                                }
                                else if (outermostByFile.at (file)->getAllSubText()
                                         != outermost->getAllSubText())
                                    return juce::Result::fail (
                                        getLocation (*table,
                                                     *firstRowByFile.at (file),
                                                     Id::structure.toString())
                                        + Id::diagnosticSeparator
                                        + getLocation (*table, *row, Id::structure.toString())
                                        + Id::diagnosticSeparator + text::Diagnostics::failNoMatch);
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
                                       + text::Diagnostics::failTableMissing);

        Element& indexTable { *indexTables.at (0) };
        const auto markdownExtension { juce::String::charToString (Chars::dot) + Extensions::md };
        const auto templateExtension { juce::String::charToString (Chars::dot)
                                       + Extensions::cast };

        for (auto* row : model.getTableRows (indexTable))
        {
            const auto pathCell { model.getTableValue (*row, Id::symbol) };

            if (pathCell.isEmpty())
                return juce::Result::fail (getLocation (indexTable, *row, Id::symbol.toString())
                                           + Id::diagnosticSeparator + text::Diagnostics::failNotFound);

            if (pathCell.endsWith (markdownExtension)
                and not model.getOutput (pathCell).existsAsFile())
                return juce::Result::fail (getLocation (indexTable, *row, Id::symbol.toString())
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failOutputMissing + Id::diagnosticSeparator
                                           + pathCell);

            if (pathCell.endsWith (templateExtension)
                and not model.getOutput (pathCell).existsAsFile())
                return juce::Result::fail (getLocation (indexTable, *row, Id::symbol.toString())
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failTemplateMissing
                                           + Id::diagnosticSeparator + pathCell);
        }

        return juce::Result::ok();
    }

    static bool hasCodeSpan (const Element& node)
    {
        if (node.isTag (Id::code))
            return true;

        for (auto* child = node.firstChild; child != nullptr; child = child->nextSibling)
            if (hasCodeSpan (*child))
                return true;

        return false;
    }

    static juce::Result isDeclared (const Model& model)
    {
        for (auto* table : model.getTables())
        {
            const auto headers { model.getTableHeaders (*table) };
            const auto origin { table->contains (Id::path) ? *table->get<juce::String> (Id::path)
                                                            : juce::String() };

            for (auto* row : model.getTableRows (*table))
            {
                int columnIndex { 0 };

                for (auto* cell : *row)
                {
                    const auto isLiteral { hasCodeSpan (*cell) };

                    if (not isLiteral)
                    {
                        const auto cellText { cell->getAllSubText() };

                        if (cellText.startsWithChar (Chars::at))
                        {
                            const auto alias { jam::Format::getPreColon (cellText) };
                            const auto indexTables { model.getTables (Id::index) };

                            const auto declared { std::any_of (indexTables.begin(),
                                indexTables.end(),
                                [&model, &alias] (Element* indexTable)
                                {
                                    const auto indexRows { model.getTableRows (*indexTable) };

                                    return std::any_of (indexRows.begin(), indexRows.end(),
                                               [&model, &alias] (Element* indexRow)
                                               {
                                                   return model.getTableValue (*indexRow, Id::alias)
                                                          == alias;
                                               });
                                }) };

                            if (not declared)
                                return juce::Result::fail (
                                    getLocation (*table, *row, headers[columnIndex])
                                    + Id::diagnosticSeparator + text::Diagnostics::failAliasMissing
                                    + Id::diagnosticSeparator + alias);
                        }
                    }

                    ++columnIndex;
                }
            }
        }

        return juce::Result::ok();
    }

    static juce::Result isManifest (const Model& model)
    {
        if (const auto result { isIndex (model) }; not result.wasOk())
            return result;

        if (const auto result { isDeclared (model) }; not result.wasOk())
            return result;

        if (const auto result { isTemplates (model) }; not result.wasOk())
            return result;

        if (const auto result { isStructure (model) }; not result.wasOk())
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
                                                   + text::Diagnostics::failDuplicate + value
                                                   + juce::String::charToString (Chars::doubleQuote));

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
