#pragma once
#include <JuceHeader.h>
#include <jam_core/function_map/jam_Function.h>
#include <jam_markdown/parser/jam_Markdown.h>
#include <regex>

namespace cast
{
/*____________________________________________________________________________*/

/**
 * @brief Formats a SPEC §8 failure location: `file:row (column)`.
 *
 * @param sourceFile The failing file's path.
 * @param rowNumber  The 1-based authored row number.
 * @param columnName The failing column's name.
 * @return The formatted location, e.g. `float:1 (preamp gain)`.
 */
static juce::String
getLocation (const juce::String& sourceFile, int rowNumber, const juce::String& columnName)
{
    return sourceFile + Id::charColon + juce::String (rowNumber) + Id::charSpace + Id::charOpenParen + columnName + Id::charCloseParen;
}

/**
 * @brief Resolves a row key to its 1-based authored row number.
 *
 * @param root      The parsed relation document.
 * @param tableName The relation (table) name.
 * @param rowKey    The row's column-0 key.
 * @return The row's 1-based position in authored order.
 */
static int
getRowNumber (const jam::Document& root, const juce::String& tableName, const juce::String& rowKey)
{
    return root.getTableRowKeys (juce::Identifier (tableName)).indexOf (rowKey) + 1;
}

/**
 * @brief Collects every table named as a foreign-key target by `## constraints`.
 *
 * A constraint's predicate arguments name a target table when they contain
 * `.` (e.g. `existsIn table.column`, `parity table.column`); the table name
 * is everything before the first `.`.
 *
 * @param manifestDoc    The parsed manifest.
 * @param constraintKeys Row keys of the manifest's `## constraints` table.
 * @return The distinct target table names referenced across all constraints.
 */
static juce::StringArray getConstraintTargetTables (const jam::Document& manifestDoc,
                                                    const juce::StringArray& constraintKeys)
{
    juce::StringArray targetTables;

    for (const auto& constraintKey : constraintKeys)
    {
        const auto predicateArgs {
            manifestDoc.getTableValue (Id::constraints, Id::predicate, constraintKey)
                .fromFirstOccurrenceOf (Id::charSpace, false, false)
                .trim()
        };

        if (predicateArgs.containsChar (Chars::dot))
            targetTables.addIfNotAlreadyThere (
                predicateArgs.upToFirstOccurrenceOf (Id::charDot, false, false));
    }

    return targetTables;
}

/**
 * @brief Determines which tables of one relation document need validation.
 *
 * A table is scanned when it is either a dispatch table (named by a
 * `## dispatch` row) or a constraint target table (named by a `## constraints`
 * predicate's FK/parity argument) and it is present in @p root.
 *
 * @param root          The parsed relation document.
 * @param dispatchKeys  Row keys of the manifest's `## dispatch` table.
 * @param targetTables  Constraint target table names (getConstraintTargetTables()).
 * @return The distinct table names present in @p root that require scanning.
 */
static juce::StringArray getScannedTables (const jam::Document& root,
                                           const juce::StringArray& dispatchKeys,
                                           const juce::StringArray& targetTables)
{
    juce::StringArray scannedTables;

    for (const auto& dispatchKey : dispatchKeys)
        if (root.getChildByID (dispatchKey) != nullptr)
            scannedTables.addIfNotAlreadyThere (dispatchKey);

    for (const auto& targetTable : targetTables)
        if (root.getChildByID (targetTable) != nullptr)
            scannedTables.addIfNotAlreadyThere (targetTable);

    return scannedTables;
}

/**
 * @brief SPEC §7.1 `matches \<regex\>` — cell matches the pattern.
 *
 * Row-scoped: Constraints::validate() invokes this once per row of every
 * scanned table that declares the constraint's column.
 */
static juce::Result predicateMatches (const juce::String& value,
                                      const juce::String& args,
                                      const juce::String& rowKey,
                                      const juce::String& columnName,
                                      const jam::Document& root,
                                      const juce::String& tableName,
                                      const jam::Array<jam::Document>&,
                                      const juce::File&,
                                      const jam::Document&,
                                      const juce::String& sourceFile)
{
    const std::regex pattern { args.toStdString() };

    return std::regex_match (value.toStdString(), pattern)
               ? juce::Result::ok()
               : juce::Result::fail (
                     getLocation (sourceFile, getRowNumber (root, tableName, rowKey), columnName)
                     + Id::diagnosticSeparator + Id::matches.toString() + Id::diagnosticSeparator + Id::failNoMatch + Id::diagnosticSeparator + value);
}

/**
 * @brief SPEC §7.2 `unique` — column uniqueness across the declared column set.
 *
 * Scope-scoped (perColumnPredicates): scans every scanned table of every
 * relation for the declared column and reports the first repeated value in
 * authored scan order, so uniqueness holds across a single table or a
 * shared registry spanning tables, per the column's own scan scope.
 */
static juce::Result predicateUnique (const juce::String&,
                                     const juce::String&,
                                     const juce::String&,
                                     const juce::String& columnName,
                                     const jam::Document&,
                                     const juce::String&,
                                     const jam::Array<jam::Document>& roots,
                                     const juce::File&,
                                     const jam::Document& manifestDoc,
                                     const juce::String&)
{
    juce::StringArray seen;
    juce::String duplicateValue;
    juce::String duplicateLocation;

    const auto scanTable = [&seen, &duplicateValue, &duplicateLocation, &columnName] (
                               const jam::Document& candidate,
                               const juce::String& candidateTableName,
                               const juce::String& candidateFile)
    {
        const auto candidateTableId { juce::Identifier (candidateTableName) };

        if (candidate.getChildByID (candidateTableName) != nullptr
            and candidate.getTableHeaders (candidateTableId).contains (columnName))
        {
            const auto rowKeys { candidate.getTableRowKeys (candidateTableId) };

            for (const auto& rowKey : rowKeys)
            {
                const auto value { candidate.getTableValue (
                    candidateTableId, juce::Identifier (columnName), rowKey) };

                if (duplicateValue.isEmpty() and seen.contains (value))
                {
                    duplicateValue = value;
                    duplicateLocation =
                        getLocation (candidateFile, rowKeys.indexOf (rowKey) + 1, columnName);
                }

                seen.add (value);
            }
        }
    };

    const auto dispatchKeys { manifestDoc.getTableRowKeys (Id::dispatch) };
    const auto targetTables { getConstraintTargetTables (
        manifestDoc, manifestDoc.getTableRowKeys (Id::constraints)) };

    for (const auto& root : roots)
    {
        const auto candidateFile { *root.get<juce::String> (Id::path) };

        for (const auto& scannedTable : getScannedTables (root, dispatchKeys, targetTables))
            scanTable (root, scannedTable, candidateFile);
    }

    return duplicateValue.isEmpty()
               ? juce::Result::ok()
               : juce::Result::fail (duplicateLocation + Id::diagnosticSeparator + Id::unique.toString()
                                     + Id::diagnosticSeparator + Id::failDuplicate + Id::diagnosticSeparator + duplicateValue);
}

/**
 * @brief SPEC §7.3 @c existsIn&lt;table&gt;.&lt;column&gt; — FK: cell keys a row in the target table.
 *
 * Row-scoped: resolves the target table's column-0 key against @p value.
 * @c &lt;column&gt; names the target table's key (column 0); resolution is keyed
 * access, not a scan of @c &lt;column&gt;'s own values.
 */
static juce::Result predicateExistsIn (const juce::String& value,
                                       const juce::String& args,
                                       const juce::String& rowKey,
                                       const juce::String& columnName,
                                       const jam::Document& root,
                                       const juce::String& tableName,
                                       const jam::Array<jam::Document>& roots,
                                       const juce::File&,
                                       const jam::Document&,
                                       const juce::String& sourceFile)
{
    const auto targetTable { args.upToFirstOccurrenceOf (Id::charDot, false, false) };

    const auto found { std::find_if (roots.begin(),
                                     roots.end(),
                                     [&targetTable] (const auto& candidate)
                                     {
                                         return candidate.getChildByID (targetTable) != nullptr;
                                     }) };

    const auto matched {
        value.isNotEmpty() and found != roots.end()
        and found->getTableRowKeys (juce::Identifier (targetTable)).contains (value)
    };

    return matched
               ? juce::Result::ok()
               : juce::Result::fail (
                     getLocation (sourceFile, getRowNumber (root, tableName, rowKey), columnName)
                     + Id::diagnosticSeparator + Id::existsIn.toString() + Id::diagnosticSeparator + Id::failForeignKeyMissing + args + Id::diagnosticSeparator
                     + value);
}

/**
 * @brief SPEC §7.4 `oneOf a|b|c` — cell is in the closed value set.
 *
 * Row-scoped. An empty cell passes when the empty value is itself listed
 * between two consecutive `|` delimiters.
 */
static juce::Result predicateOneOf (const juce::String& value,
                                    const juce::String& args,
                                    const juce::String& rowKey,
                                    const juce::String& columnName,
                                    const jam::Document& root,
                                    const juce::String& tableName,
                                    const jam::Array<jam::Document>&,
                                    const juce::File&,
                                    const jam::Document&,
                                    const juce::String& sourceFile)
{
    const auto matched { juce::StringArray::fromTokens (args, Id::charPipe, juce::String()).contains (value) };

    return matched
               ? juce::Result::ok()
               : juce::Result::fail (
                     getLocation (sourceFile, getRowNumber (root, tableName, rowKey), columnName)
                     + Id::diagnosticSeparator + Id::oneOf.toString() + Id::diagnosticSeparator + Id::failNotInSet + args + Id::charCloseBrace + Id::diagnosticSeparator + value);
}

/**
 * @brief SPEC §7.5 `range` — numeric cell within the row's declared min/max columns.
 *
 * Row-scoped. @p args names the row's own min and max column headers
 * (space-separated); the bound values are read from the same row as @p value.
 */
static juce::Result predicateRange (const juce::String& value,
                                    const juce::String& args,
                                    const juce::String& rowKey,
                                    const juce::String& columnName,
                                    const jam::Document& root,
                                    const juce::String& tableName,
                                    const jam::Array<jam::Document>&,
                                    const juce::File&,
                                    const jam::Document&,
                                    const juce::String& sourceFile)
{
    const auto tableId { juce::Identifier (tableName) };
    const auto minColumn { args.upToFirstOccurrenceOf (Id::charSpace, false, false) };
    const auto maxColumn { args.fromFirstOccurrenceOf (Id::charSpace, false, false).trim() };
    const auto lowValue {
        root.getTableValue (tableId, juce::Identifier (minColumn), rowKey).getDoubleValue()
    };
    const auto highValue {
        root.getTableValue (tableId, juce::Identifier (maxColumn), rowKey).getDoubleValue()
    };
    const auto numericValue { value.getDoubleValue() };

    return (numericValue >= lowValue and numericValue <= highValue)
               ? juce::Result::ok()
               : juce::Result::fail (
                     getLocation (sourceFile, getRowNumber (root, tableName, rowKey), columnName)
                     + Id::diagnosticSeparator + Id::range.toString() + Id::diagnosticSeparator + value + Id::failOutOfRange
                     + juce::String (lowValue) + Id::charComma + Id::charSpace + juce::String (highValue) + Id::charCloseBracket);
}

/**
 * @brief SPEC §7.6 @c parity&lt;table&gt;.&lt;column&gt; — key-set equality across tables.
 *
 * Scope-scoped (perColumnPredicates): collects @p args's target table/column
 * values, then scans every scanned table declaring the constraint's column
 * and reports the first local value missing from the target set, or —
 * failing that — the first target value missing from the local set.
 */
static juce::Result predicateParity (const juce::String&,
                                     const juce::String& args,
                                     const juce::String&,
                                     const juce::String& columnName,
                                     const jam::Document&,
                                     const juce::String&,
                                     const jam::Array<jam::Document>& roots,
                                     const juce::File&,
                                     const jam::Document& manifestDoc,
                                     const juce::String&)
{
    const auto targetTable { args.upToFirstOccurrenceOf (Id::charDot, false, false) };
    const auto targetColumn { args.fromFirstOccurrenceOf (Id::charDot, false, false) };

    juce::StringArray targetValues;
    juce::StringArray targetLocations;

    for (const auto& root : roots)
    {
        if (root.getChildByID (targetTable) != nullptr)
        {
            const auto candidateFile { *root.get<juce::String> (Id::path) };
            const auto targetRowKeys { root.getTableRowKeys (juce::Identifier (targetTable)) };

            for (const auto& key : targetRowKeys)
            {
                targetValues.add (root.getTableValue (
                    juce::Identifier (targetTable), juce::Identifier (targetColumn), key));
                targetLocations.add (
                    getLocation (candidateFile, targetRowKeys.indexOf (key) + 1, targetColumn));
            }
        }
    }

    juce::StringArray localValues;
    juce::String missingLocalValue;
    juce::String missingLocalLocation;

    const auto scanTable =
        [&localValues, &targetValues, &missingLocalValue, &missingLocalLocation, &columnName] (
            const jam::Document& candidate,
            const juce::String& candidateTableName,
            const juce::String& candidateFile)
    {
        const auto tableId { juce::Identifier (candidateTableName) };

        if (candidate.getChildByID (candidateTableName) != nullptr
            and candidate.getTableHeaders (tableId).contains (columnName))
        {
            const auto rowKeys { candidate.getTableRowKeys (tableId) };

            for (const auto& rowKey : rowKeys)
            {
                const auto value { candidate.getTableValue (
                    tableId, juce::Identifier (columnName), rowKey) };

                localValues.addIfNotAlreadyThere (value);

                if (missingLocalValue.isEmpty() and not targetValues.contains (value))
                {
                    missingLocalValue = value;
                    missingLocalLocation =
                        getLocation (candidateFile, rowKeys.indexOf (rowKey) + 1, columnName);
                }
            }
        }
    };

    const auto dispatchKeys { manifestDoc.getTableRowKeys (Id::dispatch) };
    const auto targetTablesForScan { getConstraintTargetTables (
        manifestDoc, manifestDoc.getTableRowKeys (Id::constraints)) };

    for (const auto& root : roots)
    {
        const auto candidateFile { *root.get<juce::String> (Id::path) };

        for (const auto& scannedTable : getScannedTables (root, dispatchKeys, targetTablesForScan))
            scanTable (root, scannedTable, candidateFile);
    }

    if (missingLocalValue.isNotEmpty())
        return juce::Result::fail (missingLocalLocation + Id::diagnosticSeparator + Id::parity.toString()
                                   + Id::diagnosticSeparator + Id::failLocalMissing + Id::diagnosticSeparator + missingLocalValue);

    const auto missingTarget { std::find_if (targetValues.begin(),
                                             targetValues.end(),
                                             [&localValues] (const auto& targetValue)
                                             {
                                                 return not localValues.contains (targetValue);
                                             }) };

    if (missingTarget == targetValues.end())
        return juce::Result::ok();

    const auto missingIndex { static_cast<int> (
        std::distance (targetValues.begin(), missingTarget)) };

    return juce::Result::fail (targetLocations[missingIndex] + Id::diagnosticSeparator + Id::parity.toString()
                               + Id::diagnosticSeparator + Id::failRefMissing + Id::diagnosticSeparator + *missingTarget);
}

/**
 * @brief SPEC §7.7 `fileExists \<root\>` — cell resolves to an existing file under the declared root.
 *
 * Row-scoped. @p args is a directory path relative to the manifest's
 * directory; the target file is `\<root\>/\<value\>`.
 */
static juce::Result predicateFileExists (const juce::String& value,
                                         const juce::String& args,
                                         const juce::String& rowKey,
                                         const juce::String& columnName,
                                         const jam::Document& root,
                                         const juce::String& tableName,
                                         const jam::Array<jam::Document>&,
                                         const juce::File& dir,
                                         const jam::Document&,
                                         const juce::String& sourceFile)
{
    const auto target { dir.getChildFile (args).getChildFile (value) };

    return target.existsAsFile()
               ? juce::Result::ok()
               : juce::Result::fail (
                     getLocation (sourceFile, getRowNumber (root, tableName, rowKey), columnName)
                     + Id::diagnosticSeparator + Id::fileExists.toString()
                     + Id::diagnosticSeparator + Id::failNotFound + Id::diagnosticSeparator + target.getFullPathName());
}

/**
 * @brief SPEC §7.8 `onePerGroup \<column\>` — exactly one marked row per distinct group value.
 *
 * Scope-scoped (perColumnPredicates): @p args names the grouping column; for
 * each scanned table declaring both the constraint's column and the group
 * column, groups rows by the group column and reports the first group whose
 * count of non-empty marks in the constraint's column is not exactly one.
 */
static juce::Result predicateOnePerGroup (const juce::String&,
                                          const juce::String& args,
                                          const juce::String&,
                                          const juce::String& columnName,
                                          const jam::Document&,
                                          const juce::String&,
                                          const jam::Array<jam::Document>& roots,
                                          const juce::File&,
                                          const jam::Document& manifestDoc,
                                          const juce::String&)
{
    juce::String badGroupName;
    juce::String badGroupLocation;

    const auto scanTable = [&badGroupName, &badGroupLocation, &columnName, &args] (
                               const jam::Document& candidate,
                               const juce::String& candidateTableName,
                               const juce::String& candidateFile)
    {
        const auto tableId { juce::Identifier (candidateTableName) };
        const auto groupId { juce::Identifier (args) };
        const auto columnId { juce::Identifier (columnName) };
        const auto headers { candidate.getTableHeaders (tableId) };

        if (candidate.getChildByID (candidateTableName) != nullptr and headers.contains (columnName)
            and headers.contains (args))
        {
            const auto rowKeys { candidate.getTableRowKeys (tableId) };

            juce::StringArray groups;

            for (const auto& rowKey : rowKeys)
                groups.addIfNotAlreadyThere (candidate.getTableValue (tableId, groupId, rowKey));

            const auto badGroup { std::find_if (
                groups.begin(),
                groups.end(),
                [&candidate, &tableId, &groupId, &columnId, &rowKeys] (const auto& group)
                {
                    const auto markedCount { std::count_if (
                        rowKeys.begin(),
                        rowKeys.end(),
                        [&candidate, &tableId, &groupId, &columnId, &group] (const auto& rowKey)
                        {
                            return candidate.getTableValue (tableId, groupId, rowKey) == group
                                   and candidate.getTableValue (tableId, columnId, rowKey)
                                           .isNotEmpty();
                        }) };

                    return markedCount != 1;
                }) };

            if (badGroup != groups.end())
            {
                const auto firstRowKey { std::find_if (
                    rowKeys.begin(),
                    rowKeys.end(),
                    [&candidate, &tableId, &groupId, &badGroup] (const auto& rowKey)
                    {
                        return candidate.getTableValue (tableId, groupId, rowKey) == *badGroup;
                    }) };

                badGroupName = *badGroup;
                badGroupLocation =
                    getLocation (candidateFile, rowKeys.indexOf (*firstRowKey) + 1, columnName);
            }
        }
    };

    const auto dispatchKeys { manifestDoc.getTableRowKeys (Id::dispatch) };
    const auto targetTables { getConstraintTargetTables (
        manifestDoc, manifestDoc.getTableRowKeys (Id::constraints)) };

    for (const auto& root : roots)
    {
        const auto candidateFile { *root.get<juce::String> (Id::path) };

        for (const auto& scannedTable : getScannedTables (root, dispatchKeys, targetTables))
            if (badGroupName.isEmpty())
                scanTable (root, scannedTable, candidateFile);
    }

    return badGroupName.isEmpty()
               ? juce::Result::ok()
               : juce::Result::fail (badGroupLocation + Id::diagnosticSeparator + Id::onePerGroup.toString()
                                     + Id::diagnosticSeparator + Id::failGroupOpen + badGroupName
                                     + Id::failGroupClose);
}

/**
 * @brief Predicates scoped across the whole scan, not a single row.
 *
 * `unique`, `parity`, and `onePerGroup` scan tables themselves rather than
 * being invoked once per row; Constraints::validate() dispatches these with
 * an empty value/row-key pair instead of iterating a table's rows.
 */
static const juce::StringArray perColumnPredicates { Id::unique.toString(),
                                                     Id::parity.toString(),
                                                     Id::onePerGroup.toString() };

/**
 * @brief Builds the closed predicate-name-to-function map (SPEC §7).
 *
 * @return A map from each predicate's identifier string to its predicate function.
 */
static jam::Function::Map<juce::String, juce::Result> buildPredicateMap()
{
    jam::Function::Map<juce::String, juce::Result> map;

    const auto addPredicate = [&map] (const juce::Identifier& id, auto predicateFn)
    {
        map.add<const juce::String&,
                const juce::String&,
                const juce::String&,
                const juce::String&,
                const jam::Document&,
                const juce::String&,
                const jam::Array<jam::Document>&,
                const juce::File&,
                const jam::Document&,
                const juce::String&> (id, predicateFn);
    };

    addPredicate (Id::matches, &predicateMatches);
    addPredicate (Id::unique, &predicateUnique);
    addPredicate (Id::existsIn, &predicateExistsIn);
    addPredicate (Id::oneOf, &predicateOneOf);
    addPredicate (Id::range, &predicateRange);
    addPredicate (Id::parity, &predicateParity);
    addPredicate (Id::fileExists, &predicateFileExists);
    addPredicate (Id::onePerGroup, &predicateOnePerGroup);

    return map;
}

namespace Constraints
{
/*____________________________________________________________________________*/

/**
 * @brief Dispatches one constraint row's predicate against its scan scope.
 *
 * Looks @p predicateName up in the closed predicate vocabulary (SPEC §7).
 * Scope-scoped predicates (perColumnPredicates) are invoked once, scanning
 * tables themselves; row-scoped predicates are invoked once per row of
 * @p tableName, skipped entirely when @p tableName does not declare
 * @p columnName.
 *
 * @param predicateName The predicate's identifier string.
 * @param predicateArgs The predicate's arguments (everything after the name).
 * @param columnName    The constrained column name.
 * @param root          The relation document to scan (row-scoped predicates).
 * @param tableName      The relation (table) name to scan (row-scoped predicates).
 * @param roots         Every parsed relation for this output (scope-scoped predicates).
 * @param dir           The manifest's parent directory.
 * @param manifestDoc   The parsed manifest.
 * @param sourceFile    The failing file's path, for error locations.
 * @return juce::Result::ok() when the predicate holds for every checked
 *         value; otherwise the first SPEC §8 failure.
 */
static juce::Result validate (const juce::String& predicateName,
                              const juce::String& predicateArgs,
                              const juce::String& columnName,
                              const jam::Document& root,
                              const juce::String& tableName,
                              const jam::Array<jam::Document>& roots,
                              const juce::File& dir,
                              const jam::Document& manifestDoc,
                              const juce::String& sourceFile)
{
    static jam::Function::Map<juce::String, juce::Result> predicates { buildPredicateMap() };

    if (not predicates.contains (predicateName))
        return juce::Result::fail (sourceFile + Id::diagnosticSeparator + Id::failUnknownPredicate + Id::diagnosticSeparator + predicateName);

    if (perColumnPredicates.contains (predicateName))
    {
        const juce::String emptyValue;
        const juce::String emptyRowKey;

        return predicates.get (predicateName,
                               emptyValue,
                               predicateArgs,
                               emptyRowKey,
                               columnName,
                               root,
                               tableName,
                               roots,
                               dir,
                               manifestDoc,
                               sourceFile);
    }

    const auto tableId { juce::Identifier (tableName) };
    const auto headers { root.getTableHeaders (tableId) };

    if (headers.contains (columnName))
        for (const auto& rowKey : root.getTableRowKeys (tableId))
        {
            const auto value { root.getTableValue (
                tableId, juce::Identifier (columnName), rowKey) };
            const auto result { predicates.get (predicateName,
                                                value,
                                                predicateArgs,
                                                rowKey,
                                                columnName,
                                                root,
                                                tableName,
                                                roots,
                                                dir,
                                                manifestDoc,
                                                sourceFile) };

            if (not result.wasOk())
                return result;
        }

    return juce::Result::ok();
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace Constraints

/**______________________________END OF NAMESPACE______________________________*/
}// namespace cast
