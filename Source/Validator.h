#pragma once
#include <JuceHeader.h>
#include "Operators.h"

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
    return sourceFile + Id::charColon + juce::String (rowNumber) + Id::charSpace + Id::charOpenParen
           + columnName + Id::charCloseParen;
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
getRowNumber (const jam::MarkdownDocument& root, const juce::String& tableName, const juce::String& rowKey)
{
    const auto rowIndex { root.getTableRowKeys (juce::Identifier (tableName)).indexOf (rowKey) };

    return rowIndex < 0 ? 0 : rowIndex + 1;
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
static jam::Array<juce::String> getConstraintTargetTables (const jam::MarkdownDocument& manifestDoc,
                                                    const jam::Array<juce::String>& constraintKeys)
{
    jam::Array<juce::String> targetTables;

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
static jam::Array<juce::String> getScannedTables (const jam::MarkdownDocument& root,
                                           const jam::Array<juce::String>& dispatchKeys,
                                           const jam::Array<juce::String>& targetTables)
{
    jam::Array<juce::String> scannedTables;

    for (const auto& dispatchKey : dispatchKeys)
        if (not root.getTableHeaders (juce::Identifier (dispatchKey)).isEmpty())
            scannedTables.addIfNotAlreadyThere (dispatchKey);

    for (const auto& targetTable : targetTables)
        if (not root.getTableHeaders (juce::Identifier (targetTable)).isEmpty())
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
                                      const jam::MarkdownDocument& root,
                                      const juce::String& tableName,
                                      const jam::Array<jam::MarkdownDocument>&,
                                      const juce::File&,
                                      const jam::MarkdownDocument&,
                                      const juce::String& sourceFile)
{
    const std::regex pattern { args.toStdString() };

    return std::regex_match (value.toStdString(), pattern)
               ? juce::Result::ok()
               : juce::Result::fail (
                     getLocation (sourceFile, getRowNumber (root, tableName, rowKey), columnName)
                     + Id::diagnosticSeparator + Id::matches.toString() + Id::diagnosticSeparator
                     + Id::failNoMatch + Id::diagnosticSeparator + value);
}

/**
 * @brief SPEC §7.2 `unique` — column uniqueness across the declared column set.
 *
 * Scans every table of every relation that declares the constrained column
 * (dispatched or not) and reports the first repeated value in authored scan
 * order, so uniqueness holds across a single table or a shared registry
 * spanning tables, per the column's own scan scope.
 */
static juce::Result predicateUnique (const juce::String&,
                                     const juce::String&,
                                     const juce::String&,
                                     const juce::String& columnName,
                                     const jam::MarkdownDocument&,
                                     const juce::String&,
                                     const jam::Array<jam::MarkdownDocument>& roots,
                                     const juce::File&,
                                     const jam::MarkdownDocument&,
                                     const juce::String&)
{
    jam::HashMap<juce::String, juce::String> seen;
    juce::String duplicateValue;
    juce::String duplicateLocation;

    const auto scanTable = [&seen, &duplicateValue, &duplicateLocation, &columnName] (
                               const jam::MarkdownDocument& candidate,
                               const juce::String& candidateTableName,
                               const juce::String& candidateFile)
    {
        const auto candidateTableId { juce::Identifier (candidateTableName) };
        const auto headers { candidate.getTableHeaders (candidateTableId) };

        if (headers.contains (columnName))
        {
            const auto columnId { juce::Identifier (columnName) };
            const auto rowKeys { candidate.getTableRowKeys (candidateTableId) };

            for (int rowIndex { 0 }; rowIndex < rowKeys.size(); ++rowIndex)
            {
                const auto value { candidate.getTableValue (candidateTableId, columnId, rowKeys.at (rowIndex)) };

                if (duplicateValue.isEmpty() and seen.contains (value))
                {
                    duplicateValue = value;
                    duplicateLocation = getLocation (candidateFile, rowIndex + 1, columnName);
                }

                seen.insert ({ value, getLocation (candidateFile, rowIndex + 1, columnName) });
            }
        }
    };

    for (const auto& root : roots)
    {
        const auto candidateFile { *root.root->get<juce::String> (Id::path) };

        for (auto* table : root.getTables())
            scanTable (root, table->id.toString(), candidateFile);
    }

    return duplicateValue.isEmpty()
               ? juce::Result::ok()
               : juce::Result::fail (duplicateLocation + Id::diagnosticSeparator
                                     + Id::failDuplicate + duplicateValue
                                     + Id::failAlreadyDeclared
                                     + seen.at (duplicateValue));
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
                                       const jam::MarkdownDocument& root,
                                       const juce::String& tableName,
                                       const jam::Array<jam::MarkdownDocument>& roots,
                                       const juce::File&,
                                       const jam::MarkdownDocument&,
                                       const juce::String& sourceFile)
{
    const auto targetTableId { args.upToFirstOccurrenceOf (Id::charDot, false, false) };
    const auto targetTableIdentifier { juce::Identifier (targetTableId) };

    const auto found { std::find_if (roots.begin(),
                                     roots.end(),
                                     [&targetTableIdentifier] (const jam::MarkdownDocument& candidate)
                                     {
                                         return not candidate.getTableHeaders (targetTableIdentifier).isEmpty();
                                     }) };

    const auto matched {
        value.isNotEmpty() and found != roots.end()
        and found->getTableRowKeys (targetTableIdentifier).contains (value)
    };

    return matched
               ? juce::Result::ok()
               : juce::Result::fail (
                     getLocation (sourceFile, getRowNumber (root, tableName, rowKey), columnName)
                     + Id::diagnosticSeparator + Id::existsIn.toString() + Id::diagnosticSeparator
                     + Id::failForeignKeyMissing + args + Id::diagnosticSeparator + value);
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
                                    const jam::MarkdownDocument& root,
                                    const juce::String& tableName,
                                    const jam::Array<jam::MarkdownDocument>&,
                                    const juce::File&,
                                    const jam::MarkdownDocument&,
                                    const juce::String& sourceFile)
{
    const auto matched {
        juce::StringArray::fromTokens (args, Id::charPipe, juce::String()).contains (value)
    };

    return matched
               ? juce::Result::ok()
               : juce::Result::fail (
                     getLocation (sourceFile, getRowNumber (root, tableName, rowKey), columnName)
                     + Id::diagnosticSeparator + Id::oneOf.toString() + Id::diagnosticSeparator
                     + Id::failNotInSet + args + Id::charCloseBrace + Id::diagnosticSeparator
                     + value);
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
                                    const jam::MarkdownDocument& root,
                                    const juce::String& tableName,
                                    const jam::Array<jam::MarkdownDocument>&,
                                    const juce::File&,
                                    const jam::MarkdownDocument&,
                                    const juce::String& sourceFile)
{
    const auto minColumn { args.upToFirstOccurrenceOf (Id::charSpace, false, false) };
    const auto maxColumn { args.fromFirstOccurrenceOf (Id::charSpace, false, false).trim() };

    const auto tableId { juce::Identifier (tableName) };
    const auto lowText { root.getTableValue (tableId, juce::Identifier (minColumn), rowKey) };
    const auto highText { root.getTableValue (tableId, juce::Identifier (maxColumn), rowKey) };

    const auto lowValue { lowText.getDoubleValue() };
    const auto highValue { highText.getDoubleValue() };
    const auto numericValue { value.getDoubleValue() };

    return (numericValue >= lowValue and numericValue <= highValue)
               ? juce::Result::ok()
               : juce::Result::fail (
                     getLocation (sourceFile, getRowNumber (root, tableName, rowKey), columnName)
                     + Id::diagnosticSeparator + Id::range.toString() + Id::diagnosticSeparator
                     + value + Id::failOutOfRange + juce::String (lowValue) + Id::charComma
                     + Id::charSpace + juce::String (highValue) + Id::charCloseBracket);
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
                                     const jam::MarkdownDocument&,
                                     const juce::String&,
                                     const jam::Array<jam::MarkdownDocument>& roots,
                                     const juce::File&,
                                     const jam::MarkdownDocument& manifestDoc,
                                     const juce::String&)
{
    const auto targetTableId { args.upToFirstOccurrenceOf (Id::charDot, false, false) };
    const auto targetColumn { args.fromFirstOccurrenceOf (Id::charDot, false, false) };
    const auto targetTableIdentifier { juce::Identifier (targetTableId) };
    const auto targetColumnIdentifier { juce::Identifier (targetColumn) };

    jam::Array<juce::String> targetValues;
    jam::Array<juce::String> targetLocations;

    for (const auto& root : roots)
    {
        const auto targetRowKeys { root.getTableRowKeys (targetTableIdentifier) };

        if (not targetRowKeys.isEmpty())
        {
            const auto candidateFile { *root.root->get<juce::String> (Id::path) };

            for (int rowIndex { 0 }; rowIndex < targetRowKeys.size(); ++rowIndex)
            {
                targetValues.add (root.getTableValue (
                    targetTableIdentifier, targetColumnIdentifier, targetRowKeys.at (rowIndex)));
                targetLocations.add (getLocation (candidateFile, rowIndex + 1, targetColumn));
            }
        }
    }

    jam::Array<juce::String> localValues;
    juce::String missingLocalValue;
    juce::String missingLocalLocation;

    const auto scanTable =
        [&localValues, &targetValues, &missingLocalValue, &missingLocalLocation, &columnName] (
            const jam::MarkdownDocument& candidate,
            const juce::String& candidateTableName,
            const juce::String& candidateFile)
    {
        const auto candidateTableId { juce::Identifier (candidateTableName) };
        const auto headers { candidate.getTableHeaders (candidateTableId) };

        if (headers.contains (columnName))
        {
            const auto columnId { juce::Identifier (columnName) };
            const auto rowKeys { candidate.getTableRowKeys (candidateTableId) };

            for (int rowIndex { 0 }; rowIndex < rowKeys.size(); ++rowIndex)
            {
                const auto value { candidate.getTableValue (candidateTableId, columnId, rowKeys.at (rowIndex)) };

                localValues.addIfNotAlreadyThere (value);

                if (missingLocalValue.isEmpty() and not targetValues.contains (value))
                {
                    missingLocalValue = value;
                    missingLocalLocation = getLocation (candidateFile, rowIndex + 1, columnName);
                }
            }
        }
    };

    const auto dispatchKeys { manifestDoc.getTableRowKeys (Id::dispatch) };
    const auto targetTablesForScan { getConstraintTargetTables (
        manifestDoc, manifestDoc.getTableRowKeys (Id::constraints)) };

    for (const auto& root : roots)
    {
        const auto candidateFile { *root.root->get<juce::String> (Id::path) };

        for (const auto& scannedTable : getScannedTables (root, dispatchKeys, targetTablesForScan))
            scanTable (root, scannedTable, candidateFile);
    }

    if (missingLocalValue.isNotEmpty())
        return juce::Result::fail (missingLocalLocation + Id::diagnosticSeparator
                                   + Id::parity.toString() + Id::diagnosticSeparator
                                   + Id::failLocalMissing + Id::diagnosticSeparator
                                   + missingLocalValue);

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

    return juce::Result::fail (targetLocations.at (missingIndex) + Id::diagnosticSeparator
                               + Id::parity.toString() + Id::diagnosticSeparator
                               + Id::failRefMissing + Id::diagnosticSeparator + *missingTarget);
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
                                         const jam::MarkdownDocument& root,
                                         const juce::String& tableName,
                                         const jam::Array<jam::MarkdownDocument>&,
                                         const juce::File& dir,
                                         const jam::MarkdownDocument&,
                                         const juce::String& sourceFile)
{
    const auto target { dir.getChildFile (args).getChildFile (value) };

    return target.existsAsFile()
               ? juce::Result::ok()
               : juce::Result::fail (
                     getLocation (sourceFile, getRowNumber (root, tableName, rowKey), columnName)
                     + Id::diagnosticSeparator + Id::fileExists.toString() + Id::diagnosticSeparator
                     + Id::failNotFound + Id::diagnosticSeparator + target.getFullPathName());
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
                                          const jam::MarkdownDocument&,
                                          const juce::String&,
                                          const jam::Array<jam::MarkdownDocument>& roots,
                                          const juce::File&,
                                          const jam::MarkdownDocument& manifestDoc,
                                          const juce::String&)
{
    juce::String badGroupName;
    juce::String badGroupLocation;

    const auto scanTable = [&badGroupName, &badGroupLocation, &columnName, &args] (
                               const jam::MarkdownDocument& candidate,
                               const juce::String& candidateTableName,
                               const juce::String& candidateFile)
    {
        const auto candidateTableId { juce::Identifier (candidateTableName) };
        const auto headers { candidate.getTableHeaders (candidateTableId) };

        if (headers.contains (columnName) and headers.contains (args))
        {
            const auto rowKeysInOrder { candidate.getTableRowKeys (candidateTableId) };
            const auto argColumn { juce::Identifier (args) };
            const auto markColumn { juce::Identifier (columnName) };

            jam::Array<juce::String> groups;

            for (const auto& rowKey : rowKeysInOrder)
                groups.addIfNotAlreadyThere (candidate.getTableValue (candidateTableId, argColumn, rowKey));

            const auto badGroup { std::find_if (
                groups.begin(),
                groups.end(),
                [&candidate, &candidateTableId, &rowKeysInOrder, &argColumn, &markColumn] (const auto& group)
                {
                    const auto markedCount { std::count_if (
                        rowKeysInOrder.begin(),
                        rowKeysInOrder.end(),
                        [&candidate, &candidateTableId, &argColumn, &markColumn, &group] (const auto& rowKey)
                        {
                            const auto groupValue { candidate.getTableValue (candidateTableId, argColumn, rowKey) };
                            const auto markValue { candidate.getTableValue (candidateTableId, markColumn, rowKey) };

                            return groupValue == group and markValue.isNotEmpty();
                        }) };

                    return markedCount != 1;
                }) };

            if (badGroup != groups.end())
            {
                const auto firstMatch { std::find_if (
                    rowKeysInOrder.begin(),
                    rowKeysInOrder.end(),
                    [&candidate, &candidateTableId, &argColumn, &badGroup] (const auto& rowKey)
                    {
                        return candidate.getTableValue (candidateTableId, argColumn, rowKey) == *badGroup;
                    }) };

                badGroupName = *badGroup;
                badGroupLocation = getLocation (candidateFile,
                                                static_cast<int> (std::distance (rowKeysInOrder.begin(), firstMatch))
                                                    + 1,
                                                columnName);
            }
        }
    };

    const auto dispatchKeys { manifestDoc.getTableRowKeys (Id::dispatch) };
    const auto targetTables { getConstraintTargetTables (
        manifestDoc, manifestDoc.getTableRowKeys (Id::constraints)) };

    for (const auto& root : roots)
    {
        const auto candidateFile { *root.root->get<juce::String> (Id::path) };

        for (const auto& scannedTable : getScannedTables (root, dispatchKeys, targetTables))
            if (badGroupName.isEmpty())
                scanTable (root, scannedTable, candidateFile);
    }

    return badGroupName.isEmpty()
               ? juce::Result::ok()
               : juce::Result::fail (badGroupLocation + Id::diagnosticSeparator
                                     + Id::onePerGroup.toString() + Id::diagnosticSeparator
                                     + Id::failGroupOpen + badGroupName + Id::failGroupClose);
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
                const jam::MarkdownDocument&,
                const juce::String&,
                const jam::Array<jam::MarkdownDocument>&,
                const juce::File&,
                const jam::MarkdownDocument&,
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
                              const jam::MarkdownDocument& root,
                              const juce::String& tableName,
                              const jam::Array<jam::MarkdownDocument>& roots,
                              const juce::File& dir,
                              const jam::MarkdownDocument& manifestDoc,
                              const juce::String& sourceFile)
{
    static jam::Function::Map<juce::String, juce::Result> predicates { buildPredicateMap() };

    if (not predicates.contains (predicateName))
        return juce::Result::fail (sourceFile + Id::diagnosticSeparator + Id::failUnknownPredicate
                                   + Id::diagnosticSeparator + predicateName);

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
    {
        const auto columnId { juce::Identifier (columnName) };

        for (const auto& rowKey : root.getTableRowKeys (tableId))
        {
            const auto value { root.getTableValue (tableId, columnId, rowKey) };
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
    }

    return juce::Result::ok();
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace Constraints

/**
 * @brief Detects the SPEC §2 cell hazard rule in one parsed cell.
 *
 * Walks @p cell's parsed subtree, pruning at any code-span node (SPEC §2:
 * code spans unwrap to plain content and are exempt from the hazard rule).
 * Reports the first hazard found: an anchor tag (a URI scheme) or a text
 * node containing `<` or `>`.
 *
 * @param cell The parsed document subtree for one table cell.
 * @return The hazard message, or an empty string when no hazard is found.
 */
static juce::String getHazardMessage (const jam::Document::Element& cell) noexcept
{
    juce::String hazard;

    cell.applyFunctionRecursively (
        [&hazard] (const jam::Document::Element& node) -> bool
        {
            if (node.isTag (Id::a))
                hazard = Id::failHazardURI;
            else if (node.isTag (Id::text)
                     and node.get<juce::String> (Id::text)->containsAnyOf (Id::hazardChars))
                hazard = Id::failHazardAngleBrackets;

            return not node.isTag (Id::code);
        });

    return hazard;
}

/**
 * @brief Scans every cell of one relation for the SPEC §2 hazard rule.
 *
 * @param root       The parsed relation document.
 * @param tableId    The relation (table) identifier to scan.
 * @param sourceFile The relation's source file path, for error locations.
 * @return juce::Result::ok() when no cell is hazardous; otherwise a
 *         SPEC §8 `file:row (column)` failure naming the first hazard.
 */
static juce::Result validateTableHazards (const jam::MarkdownDocument& root,
                                          const juce::Identifier& tableId,
                                          const juce::String& sourceFile)
{
    const auto rowKeys { root.getTableRowKeys (tableId) };

    if (not rowKeys.isEmpty())
    {
        const auto headers { root.getTableHeaders (tableId) };

        for (int rowIndex { 0 }; rowIndex < rowKeys.size(); ++rowIndex)
        {
            const auto& rowKey { rowKeys.at (rowIndex) };

            for (const auto& header : headers)
            {
                const auto* cell { root.getTableCell (
                    tableId, juce::Identifier (header), juce::Identifier (rowKey)) };

                if (cell != nullptr)
                {
                    const auto hazard { getHazardMessage (*cell) };

                    if (hazard.isNotEmpty())
                        return juce::Result::fail (getLocation (sourceFile, rowIndex + 1, header)
                                                   + Id::diagnosticSeparator + hazard);
                }
            }
        }
    }

    return juce::Result::ok();
}

/**
 * @brief Validates the manifest's own structural contracts (SPEC §4).
 *
 * Checks that every `## transforms` row names a transform in the closed
 * vocabulary (SPEC §6), that every template referenced by `## outputs` and
 * `## dispatch` exists on disk, and that no sibling file sharing a
 * referenced template's extension in a referenced template's directory goes
 * unreferenced (orphan-template scan).
 *
 * @param manifestDoc  The parsed manifest.
 * @param manifestFile The manifest file, for error locations.
 * @return juce::Result::ok() when the manifest is well-formed; otherwise the
 *         first SPEC §4/§8 failure found.
 */
static juce::Result
validateManifest (const jam::MarkdownDocument& manifestDoc, const juce::File& manifestFile)
{
    const auto dir { manifestFile.getParentDirectory() };
    const auto path { manifestFile.getFullPathName() };

    const auto transformKeys { manifestDoc.getTableRowKeys (Id::transforms) };

    for (const auto& transformKey : transformKeys)
    {
        const auto transformName { manifestDoc.getTableValue (
            Id::transforms, Id::transform, transformKey) };

        if (not Transforms::contains (transformName))
            return juce::Result::fail (
                getLocation (path, transformKeys.indexOf (transformKey) + 1, transformKey)
                + Id::diagnosticSeparator + Id::failUnknownTransform + Id::diagnosticSeparator
                + transformName);
    }

    const auto outputKeys { manifestDoc.getTableRowKeys (Id::generated) };
    const auto dispatchKeys { manifestDoc.getTableRowKeys (Id::dispatch) };
    juce::StringArray referencedTemplates;

    for (const auto& outputKey : outputKeys)
    {
        const auto templatePath { manifestDoc.getTableValue (
            Id::generated, Id::templatePath, outputKey) };

        if (not dir.getChildFile (templatePath).existsAsFile())
            return juce::Result::fail (
                getLocation (path, outputKeys.indexOf (outputKey) + 1, Id::templatePath.toString())
                + Id::diagnosticSeparator + Id::failTemplateMissing + Id::diagnosticSeparator
                + templatePath);

        referencedTemplates.addIfNotAlreadyThere (templatePath);
    }

    for (const auto& dispatchKey : dispatchKeys)
    {
        const auto fragmentPath { manifestDoc.getTableValue (
            Id::dispatch, Id::templatePath, dispatchKey) };

        if (not dir.getChildFile (fragmentPath).existsAsFile())
            return juce::Result::fail (getLocation (path,
                                                    dispatchKeys.indexOf (dispatchKey) + 1,
                                                    Id::templatePath.toString())
                                       + Id::diagnosticSeparator + Id::failFragmentMissing
                                       + Id::diagnosticSeparator + fragmentPath);

        referencedTemplates.addIfNotAlreadyThere (fragmentPath);
    }

    juce::StringArray referencedExtensions;

    for (const auto& templatePath : referencedTemplates)
        referencedExtensions.addIfNotAlreadyThere (
            dir.getChildFile (templatePath).getFileExtension());

    juce::Array<juce::File> templateDirs;

    for (const auto& templatePath : referencedTemplates)
    {
        const auto templateDir { dir.getChildFile (templatePath).getParentDirectory() };

        if (not templateDirs.contains (templateDir))
            templateDirs.add (templateDir);
    }

    for (const auto& templateDir : templateDirs)
    {
        juce::Array<juce::File> siblingFiles;
        templateDir.findChildFiles (siblingFiles, juce::File::findFiles, false);

        for (const auto& siblingFile : siblingFiles)
            if (not siblingFile.isHidden()
                and referencedExtensions.contains (siblingFile.getFileExtension()))
            {
                const auto referenced { std::any_of (referencedTemplates.begin(),
                                                     referencedTemplates.end(),
                                                     [&dir, &siblingFile] (const auto& templatePath)
                                                     {
                                                         return dir.getChildFile (templatePath)
                                                                == siblingFile;
                                                     }) };

                if (not referenced)
                    return juce::Result::fail (path + Id::diagnosticSeparator + Id::failOrphan
                                               + Id::diagnosticSeparator
                                               + siblingFile.getFullPathName());
            }
    }

    return juce::Result::ok();
}

/**
 * @brief Validates every relation's hazard rule and per-row constraints.
 *
 * For each parsed relation in @p roots, scans its dispatch- and
 * constraint-target tables (Constraints::getScannedTables()) for the SPEC §2
 * hazard rule, then runs every row-scoped constraint predicate (predicates
 * absent from perColumnPredicates) against each scanned table.
 *
 * @param roots          The parsed relation documents for this output.
 * @param manifestDoc    The parsed manifest.
 * @param dispatchKeys   Row keys of the manifest's `## dispatch` table.
 * @param constraintKeys Row keys of the manifest's `## constraints` table.
 * @param dir            The manifest's parent directory.
 * @return juce::Result::ok() on success, or the first hazard or constraint failure.
 */
static juce::Result validateRoots (const jam::Array<jam::MarkdownDocument>& roots,
                                   const jam::MarkdownDocument& manifestDoc,
                                   const jam::Array<juce::String>& dispatchKeys,
                                   const jam::Array<juce::String>& constraintKeys,
                                   const juce::File& dir)
{
    const auto targetTables { getConstraintTargetTables (manifestDoc, constraintKeys) };

    juce::Result firstFailure { juce::Result::ok() };

    {
        juce::ThreadPool validatePool;
        juce::CriticalSection failureLock;

        for (const auto& root : roots)
        {
            validatePool.addJob ([&root, &dispatchKeys, &targetTables, &constraintKeys,
                                  &manifestDoc, &roots, &dir, &firstFailure, &failureLock]
            {
                const auto sourceFile { *root.root->get<juce::String> (Id::path) };
                const auto scannedTables { getScannedTables (root, dispatchKeys, targetTables) };

                for (const auto& scannedTable : scannedTables)
                {
                    const auto hazardResult { validateTableHazards (
                        root, juce::Identifier (scannedTable), sourceFile) };

                    if (not hazardResult.wasOk())
                    {
                        const juce::ScopedLock lock { failureLock };

                        if (firstFailure.wasOk())
                            firstFailure = hazardResult;

                        return;
                    }
                }

                for (const auto& constraintKey : constraintKeys)
                {
                    const auto predicateSpec { manifestDoc.getTableValue (
                        Id::constraints, Id::predicate, constraintKey) };
                    const auto predicateName { predicateSpec.upToFirstOccurrenceOf (
                        Id::charSpace, false, false) };
                    const auto predicateArgs {
                        predicateSpec.fromFirstOccurrenceOf (Id::charSpace, false, false).trim()
                    };

                    if (not perColumnPredicates.contains (predicateName))
                        for (const auto& scannedTable : scannedTables)
                        {
                            const auto constraintResult { Constraints::validate (predicateName,
                                                                                 predicateArgs,
                                                                                 constraintKey,
                                                                                 root,
                                                                                 scannedTable,
                                                                                 roots,
                                                                                 dir,
                                                                                 manifestDoc,
                                                                                 sourceFile) };

                            if (not constraintResult.wasOk())
                            {
                                const juce::ScopedLock lock { failureLock };

                                if (firstFailure.wasOk())
                                    firstFailure = constraintResult;

                                return;
                            }
                        }
                }
            });
        }

        while (validatePool.getNumJobs() > 0)
            juce::Thread::sleep (1);
    }

    return firstFailure;
}

/**
 * @brief Runs every constraint whose predicate operates across the whole scanned scope.
 *
 * Delegates each constraint row naming a predicate in perColumnPredicates
 * (`unique`, `parity`, `onePerGroup`) to Constraints::validate(), which scans
 * relations itself rather than iterating rows of one table.
 *
 * @param manifestDoc    The parsed manifest.
 * @param constraintKeys Row keys of the manifest's `## constraints` table.
 * @param roots          The parsed relation documents for this output.
 * @param dir            The manifest's parent directory.
 * @param manifestPath   The manifest file's path, for error locations.
 * @return juce::Result::ok() on success, or the first constraint failure.
 */
static juce::Result validatePerColumnConstraints (const jam::MarkdownDocument& manifestDoc,
                                                  const jam::Array<juce::String>& constraintKeys,
                                                  const jam::Array<jam::MarkdownDocument>& roots,
                                                  const juce::File& dir,
                                                  const juce::String& manifestPath)
{
    for (const auto& constraintKey : constraintKeys)
    {
        const auto predicateSpec { manifestDoc.getTableValue (
            Id::constraints, Id::predicate, constraintKey) };
        const auto predicateName { predicateSpec.upToFirstOccurrenceOf (
            Id::charSpace, false, false) };
        const auto predicateArgs {
            predicateSpec.fromFirstOccurrenceOf (Id::charSpace, false, false).trim()
        };

        if (perColumnPredicates.contains (predicateName))
        {
            const jam::MarkdownDocument placeholderRoot;
            const juce::String placeholderTableName;

            const auto constraintResult { Constraints::validate (predicateName,
                                                                 predicateArgs,
                                                                 constraintKey,
                                                                 placeholderRoot,
                                                                 placeholderTableName,
                                                                 roots,
                                                                 dir,
                                                                 manifestDoc,
                                                                 manifestPath) };

            if (not constraintResult.wasOk())
                return constraintResult;
        }
    }

    return juce::Result::ok();
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace cast
