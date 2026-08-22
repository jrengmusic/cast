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

    static juce::Result isStructure (const Model& model, const TemplateDocument& templateDocument)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
            {
                jam::HashMap<juce::String, juce::String> depthZeroByFile;
                jam::HashMap<juce::String, Element*> firstRowByFile;

                for (auto* row : model.getTableRows (*table))
                {
                    const auto shapeId { model.getStructure (*row, 0) };

                    if (shapeId.isEmpty())
                        return juce::Result::fail (getLocation (*table, *row, Id::structure.toString())
                                                   + Id::diagnosticSeparator
                                                   + text::Diagnostics::failNotFound);

                    for (int depth { 0 }; model.getStructure (*row, depth).isNotEmpty(); ++depth)
                    {
                        const auto depthShapeId { model.getStructure (*row, depth) };
                        const juce::Identifier codeId { depthShapeId };

                        if (templateDocument.getCodeBlock (codeId) == nullptr)
                            return juce::Result::fail (
                                getLocation (*table, *row, Id::structure.toString())
                                + Id::diagnosticSeparator + text::Diagnostics::failTemplateMissing
                                + Id::diagnosticSeparator + depthShapeId);
                    }

                    const auto origin { *table->get<juce::String> (Id::path) };
                    const auto file { model.getValue (origin, model.getTableValue (*row, Id::file)) };

                    if (not depthZeroByFile.contains (file))
                    {
                        depthZeroByFile.try_emplace (file, shapeId);
                        firstRowByFile.try_emplace (file, row);
                    }
                    else if (depthZeroByFile.at (file) != shapeId)
                        return juce::Result::fail (
                            getLocation (*table, *firstRowByFile.at (file), Id::structure.toString())
                            + Id::diagnosticSeparator
                            + getLocation (*table, *row, Id::structure.toString())
                            + Id::diagnosticSeparator + text::Diagnostics::failNoMatch);
                }
            }

        return juce::Result::ok();
    }

    static juce::Result
    isPlaceholders (const Model& model, const TemplateDocument& templateDocument)
    {
        for (auto* table : model.getTables())
            if (model.isOutputTable (*table))
                for (auto* row : model.getTableRows (*table))
                    for (const auto& column : { Id::placeholder, Id::structure })
                        for (auto& entry : model.getSource (*row, column))
                        {
                            const auto& [entryDepth, entryKey, entryValue] { entry };
                            juce::ignoreUnused (entryDepth, entryKey);

                            if (jam::Format::getPreColon (entryValue).trim()
                                == Id::templatePath.toString())
                            {
                                const juce::Identifier codeId {
                                    jam::Format::getPostColon (entryValue).trim()
                                };

                                if (templateDocument.getCodeBlock (codeId) == nullptr)
                                    return juce::Result::fail (
                                        getLocation (*table, *row, column.toString())
                                        + Id::diagnosticSeparator
                                        + text::Diagnostics::failTemplateMissing
                                        + Id::diagnosticSeparator + codeId.toString());
                            }
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

    static juce::Result isIndex (const Model& model)
    {
        const auto indexTables { model.getTables (Id::index) };

        if (indexTables.isEmpty())
            return juce::Result::fail (Id::index.toString() + Id::diagnosticSeparator
                                       + text::Diagnostics::failTableMissing);

        Element& indexTable { *indexTables.at (0) };

        for (auto* row : model.getTableRows (indexTable))
        {
            const auto pathCell { model.getTableValue (*row, Id::symbol) };

            if (pathCell.isEmpty())
                return juce::Result::fail (getLocation (indexTable, *row, Id::symbol.toString())
                                           + Id::diagnosticSeparator + text::Diagnostics::failNotFound);

            if (juce::File::createFileWithoutCheckingPath (pathCell).hasFileExtension (Extensions::md)
                and not model.getOutput (pathCell).existsAsFile())
                return juce::Result::fail (getLocation (indexTable, *row, Id::symbol.toString())
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failOutputMissing + Id::diagnosticSeparator
                                           + pathCell);

            if (juce::File::createFileWithoutCheckingPath (pathCell).hasFileExtension (Extensions::cast)
                and not model.getOutput (pathCell).existsAsFile())
                return juce::Result::fail (getLocation (indexTable, *row, Id::symbol.toString())
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failTemplateMissing
                                           + Id::diagnosticSeparator + pathCell);
        }

        return juce::Result::ok();
    }

    static juce::Result isManifest (const Model& model)
    {
        if (const auto result { isIndex (model) }; not result.wasOk())
            return result;

        if (const auto result { isDeclared (model) }; not result.wasOk())
            return result;

        juce::File templateFile;

        for (auto* indexRow : model.getTableRows (Id::index))
        {
            const auto pathCell { model.getTableValue (*indexRow, Id::symbol) };

            if (juce::File::createFileWithoutCheckingPath (pathCell).hasFileExtension (
                    Extensions::cast))
                templateFile = model.getOutput (pathCell);
        }

        const TemplateDocument templateDocument {
            jam::MarkdownDocument::parse (templateFile.loadFileAsString())
        };

        if (const auto result { isStructure (model, templateDocument) }; not result.wasOk())
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

        return isPlaceholders (model, templateDocument);
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
