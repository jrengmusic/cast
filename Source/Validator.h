#pragma once
#include <JuceHeader.h>
#include "Model.h"
#include "Operators.h"

struct Validator
{
    using Element = jam::Document::Element;

    static juce::Result isValid (const Document& model)
    {
        juce::Result firstFailure { isManifest (model) };

        if (firstFailure.wasOk())
        {
            juce::ThreadPool validatePool;
            juce::CriticalSection failureLock;

            for (auto* table : model.getTables())
                validatePool.addJob (
                    [&model, table, &failureLock, &firstFailure]
                    {
                        if (const auto result { isTable (model, *table) }; not result.wasOk())
                        {
                            const juce::ScopedLock lock { failureLock };

                            if (firstFailure.wasOk())
                                firstFailure = result;
                        }
                    });

            for (auto* row : model.getTableRows (Id::constraints))
                validatePool.addJob (
                    [&model, row, &failureLock, &firstFailure]
                    {
                        const auto column { model.getTableValue (*row, Id::column) };
                        const auto predicateCell { model.getTableValue (*row, Id::predicate) };
                        const auto name { predicateCell.upToFirstOccurrenceOf (
                            juce::String::charToString (chars::space), false, false) };
                        const auto args {
                            predicateCell
                                .fromFirstOccurrenceOf (
                                    juce::String::charToString (chars::space), false, false)
                                .trim()
                        };

                        const auto result { getPredicates().contains (name)
                                                ? getPredicates().get (name, model, column, args)
                                                : juce::Result::fail (text::en::failUnknownPredicate
                                                                      + Id::diagnosticSeparator
                                                                      + name) };

                        if (not result.wasOk())
                        {
                            const juce::ScopedLock lock { failureLock };

                            if (firstFailure.wasOk())
                                firstFailure = result;
                        }
                    });

            while (validatePool.getNumJobs() > 0)
                juce::Thread::sleep (1);
        }

        return firstFailure;
    }

    static juce::Result
    matches (const Document& model, const juce::String& column, const juce::String& args)
    {
        const std::regex pattern { args.toStdString() };

        for (auto* table : model.getTables())
            if (model.getTableHeaders (*table).contains (column))
                for (auto* row : model.getTableRows (*table))
                {
                    const auto value { model.getTableValue (*row, juce::Identifier (column)) };

                    if (not std::regex_match (value.toStdString(), pattern))
                        return juce::Result::fail (
                            getLocation (*table, *row, column) + Id::diagnosticSeparator
                            + Id::matches + Id::diagnosticSeparator
                            + text::en::failNoMatch + Id::diagnosticSeparator + value);
                }

        return juce::Result::ok();
    }

    static juce::Result
    unique (const Document& model, const juce::String& column, const juce::String& args)
    {
        juce::StringArray seen;

        for (auto* table : model.getTables())
            if (model.getTableHeaders (*table).contains (column))
                for (auto* row : model.getTableRows (*table))
                {
                    const auto value { model.getTableValue (*row, juce::Identifier (column)) };

                    if (seen.contains (value))
                        return juce::Result::fail (getLocation (*table, *row, column)
                                                   + Id::diagnosticSeparator + Id::unique
                                                   + Id::diagnosticSeparator
                                                   + text::en::failDuplicate + value);

                    seen.add (value);
                }

        return juce::Result::ok();
    }

    static juce::Result
    existsIn (const Document& model, const juce::String& column, const juce::String& args)
    {
        const juce::Identifier targetTable { args.upToFirstOccurrenceOf (
            juce::String::charToString (chars::dot), false, false) };
        const auto targetKeys { model.getTableRowKeys (targetTable) };

        for (auto* table : model.getTables())
            if (model.getTableHeaders (*table).contains (column))
                for (auto* row : model.getTableRows (*table))
                {
                    const auto value { model.getTableValue (*row, juce::Identifier (column)) };

                    if (not targetKeys.contains (value))
                        return juce::Result::fail (
                            getLocation (*table, *row, column) + Id::diagnosticSeparator
                            + Id::existsIn + Id::diagnosticSeparator
                            + text::en::failForeignKeyMissing + args);
                }

        return juce::Result::ok();
    }

    static juce::Result
    oneOf (const Document& model, const juce::String& column, const juce::String& args)
    {
        const auto choices { juce::StringArray::fromTokens (
            args, juce::String::charToString (chars::pipe), {}) };

        for (auto* table : model.getTables())
            if (model.getTableHeaders (*table).contains (column))
                for (auto* row : model.getTableRows (*table))
                {
                    const auto value { model.getTableValue (*row, juce::Identifier (column)) };

                    if (not choices.contains (value))
                        return juce::Result::fail (getLocation (*table, *row, column)
                                                   + Id::diagnosticSeparator + Id::oneOf
                                                   + Id::diagnosticSeparator
                                                   + text::en::failNotInSet + value);
                }

        return juce::Result::ok();
    }

    static juce::Result
    range (const Document& model, const juce::String& column, const juce::String& args)
    {
        for (auto* table : model.getTables())
            if (model.getTableHeaders (*table).contains (column))
                for (auto* row : model.getTableRows (*table))
                {
                    const auto value {
                        model.getTableValue (*row, juce::Identifier (column)).getDoubleValue()
                    };
                    const auto minValue { model.getTableValue (*row, Id::min).getDoubleValue() };
                    const auto maxValue { model.getTableValue (*row, Id::max).getDoubleValue() };

                    if (value < minValue or value > maxValue)
                        return juce::Result::fail (
                            getLocation (*table, *row, column) + Id::diagnosticSeparator
                            + Id::range + Id::diagnosticSeparator
                            + text::en::failOutOfRange + juce::String (maxValue));
                }

        return juce::Result::ok();
    }

    static juce::Result
    parity (const Document& model, const juce::String& column, const juce::String& args)
    {
        const juce::Identifier targetTable { args.upToFirstOccurrenceOf (
            juce::String::charToString (chars::dot), false, false) };
        const juce::Identifier targetColumn { args.fromFirstOccurrenceOf (
            juce::String::charToString (chars::dot), false, false) };

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
    fileExists (const Document& model, const juce::String& column, const juce::String& args)
    {
        for (auto* table : model.getTables())
            if (model.getTableHeaders (*table).contains (column))
                for (auto* row : model.getTableRows (*table))
                {
                    const auto value { model.getTableValue (*row, juce::Identifier (column)) };

                    if (not model.getOutput (args).getChildFile (value).existsAsFile())
                        return juce::Result::fail (
                            getLocation (*table, *row, column) + Id::diagnosticSeparator
                            + Id::fileExists + Id::diagnosticSeparator
                            + text::en::failOutputMissing + Id::diagnosticSeparator + value);
                }

        return juce::Result::ok();
    }

    static juce::Result
    onePerGroup (const Document& model, const juce::String& column, const juce::String& args)
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
                            + Id::diagnosticSeparator + Id::onePerGroup
                            + Id::diagnosticSeparator + text::en::failGroupOpen + allGroups[index]
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

                map.add<const Document&, const juce::String&, const juce::String&> (
                    Id::matches, &matches);
                map.add<const Document&, const juce::String&, const juce::String&> (
                    Id::unique, &unique);
                map.add<const Document&, const juce::String&, const juce::String&> (
                    Id::existsIn, &existsIn);
                map.add<const Document&, const juce::String&, const juce::String&> (
                    Id::oneOf, &oneOf);
                map.add<const Document&, const juce::String&, const juce::String&> (
                    Id::range, &range);
                map.add<const Document&, const juce::String&, const juce::String&> (
                    Id::parity, &parity);
                map.add<const Document&, const juce::String&, const juce::String&> (
                    Id::fileExists, &fileExists);
                map.add<const Document&, const juce::String&, const juce::String&> (
                    Id::onePerGroup, &onePerGroup);

                return map;
            }()
        };

        return predicates;
    }

    static juce::Result isTable (const Document& model, Element& table)
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

    static juce::Result isTemplates (const Document& model)
    {
        juce::StringArray referenced;
        Element& generatedTable { **model.getTables (Id::generated).begin() };

        for (auto* row : model.getTableRows (Id::generated))
            for (const auto& fileColumn : { Id::templatePath, Id::separator })
                if (const auto cell { model.getTableValue (*row, fileColumn) }; cell.isNotEmpty())
                {
                    referenced.add (cell);

                    if (not model.getOutput (cell).existsAsFile())
                        return juce::Result::fail (
                            getLocation (generatedTable, *row, fileColumn.toString())
                            + Id::diagnosticSeparator + text::en::failTemplateMissing
                            + Id::diagnosticSeparator + cell);
                }

        for (auto* row : model.getTableRows (Id::patch))
            if (const auto fragmentCell { model.getTableValue (*row, Id::fragment) };
                fragmentCell.isNotEmpty())
                referenced.add (fragmentCell);

        const auto wildcard { juce::String::charToString (chars::asterisk)
                              + juce::String::charToString (chars::dot) + extensions::h };
        const auto templateDirectory { model.getOutput (Id::templatePath.toString()) };

        for (const auto& templateFile :
             templateDirectory.findChildFiles (juce::File::findFiles, false, wildcard))
        {
            const auto relativePath { templateFile.getRelativePathFrom (model.getOutput ({})) };
            if (not referenced.contains (relativePath))
                return juce::Result::fail (text::en::failOrphan + Id::diagnosticSeparator
                                           + templateFile.getFileName());
        }
        return juce::Result::ok();
    }

    static juce::Result isPatch (const Document& model)
    {
        Element& patchTable { **model.getTables (Id::patch).begin() };
        for (auto* row : model.getTableRows (Id::patch))
        {
            const auto sourceCell { model.getTableValue (*row, Id::source) };

            if (const auto fragmentCell { model.getTableValue (*row, Id::fragment) };
                fragmentCell.isNotEmpty())
            {
                if (not model.getOutput (fragmentCell).existsAsFile())
                    return juce::Result::fail (
                        getLocation (patchTable, *row, Id::fragment.toString())
                        + Id::diagnosticSeparator + text::en::failFragmentMissing
                        + Id::diagnosticSeparator + fragmentCell);
            }

            if (model.getTables (juce::Identifier (sourceCell)).isEmpty())
                return juce::Result::fail (getLocation (patchTable, *row, Id::source.toString())
                                           + Id::diagnosticSeparator + text::en::failTableMissing
                                           + Id::diagnosticSeparator + sourceCell);
        }

        return juce::Result::ok();
    }

    static juce::Result isTransforms (const Document& model)
    {
        for (auto* row : model.getTableRows (Id::transforms))
        {
            const auto transformCell { model.getTableValue (*row, Id::transform) };

            if (not Transforms::contains (transformCell))
                return juce::Result::fail (text::en::failUnknownTransform + Id::diagnosticSeparator
                                           + transformCell);
        }

        return juce::Result::ok();
    }

    static juce::Result isManifest (const Document& model)
    {
        if (const auto result { isTemplates (model) }; not result.wasOk())
            return result;

        if (const auto result { isPatch (model) }; not result.wasOk())
            return result;

        return isTransforms (model);
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
