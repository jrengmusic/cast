#pragma once
#include <JuceHeader.h>
#include <string>
#include <tuple>
#include "Validator.h"
#include "Writer.h"
#include "Operators.h"

namespace cast
{
/*____________________________________________________________________________*/

static juce::String getOutputBanner (const juce::File& dir)
{
    const auto bannerFile { dir.getChildFile (files::castOutput.toString()) };

    if (not bannerFile.existsAsFile())
        return {};

    const auto bannerDoc { jam::Markdown::parse (bannerFile.loadFileAsString()) };
    juce::String rawBanner;

    bannerDoc.root->applyFunctionRecursively (
        [&rawBanner] (const jam::Document::Element& element) -> bool
        {
            if (rawBanner.isEmpty() and element.contains (Id::type)
                and *element.get<int> (Id::type) == map::BlockType::codeBlock)
                rawBanner = element.getAllSubText();

            return true;
        });

    if (rawBanner.isEmpty())
        return {};

    juce::StringArray lines;
    lines.addLines (rawBanner);

    for (auto& line : lines)
        line = juce::String::charToString (chars::space) + line;

    return lines.joinIntoString (juce::String::charToString (chars::newline));
}

/**
 * @brief Reports whether a table cell was authored as backticked code.
 *
 * A backticked cell parses to `Id::code` children; SPEC §3.1 treats such a
 * cell as a literal, exempt from cell resolution.
 *
 * @param cell The table cell node.
 * @return True when @p cell has at least one direct `Id::code` child.
 */
static bool isLiteralCell (const jam::Document::Element& cell)
{
    for (auto* child : cell)
        if (child->isTag (Id::code))
            return true;

    return false;
}

/**
 * @brief Returns a backtick-literal cell's true source value, unaffected by
 *        CommonMark code-span padding-strip.
 *
 * Reads the cell's `Id::rawText` property (the pre-inline-parse source text)
 * rather than its rendered subtext, and strips exactly the two backtick
 * delimiter characters -- nothing else. Must only be called when
 * isLiteralCell() has already confirmed the cell is backtick-wrapped.
 *
 * @param cell The table cell node; must satisfy isLiteralCell().
 * @return The cell's literal value, byte-exact to its authored source.
 */
static juce::String getLiteralValue (const jam::Document::Element& cell)
{
    jassert (cell.contains (Id::rawText));

    const auto raw { *cell.get<juce::String> (Id::rawText) };

    jassert (raw.startsWithChar (chars::backtick) and raw.endsWithChar (chars::backtick));

    return raw.substring (1, raw.length() - 1);
}

static const juce::String rowRegionBeginPrefix { "@" + Id::rowRegionBegin };
static const juce::String rowRegionEnd { "@row:end@" };
static const juce::String rowRegionIndex { "row:index" };
static const juce::String rowMarker { "@row@" };
static const juce::String defaultKeyType {
    intType
};///< `@type@` when a dispatch row omits the `type` cell.
static const juce::String formatColumn { "format" };
static const juce::String extensionsNamespace { "extensions::" };

/**
 * @brief Resolves one cell value against the row key (SPEC §3.1).
 *
 * An empty cell resolves to @p rowKey; a cell equal to `@row@` resolves to
 * @p rowIndex as text; a cell naming a transform in the closed vocabulary
 * resolves to that transform applied to @p rowKey; any other cell is its
 * own literal value.
 *
 * @param cellValue The raw cell text.
 * @param rowKey    The row's column-0 key.
 * @param rowIndex  The row's 0-based position among its matched rows.
 * @return The resolved cell text.
 */
static juce::String
getResolvedCell (const juce::String& cellValue, const juce::String& rowKey, int rowIndex)
{
    if (cellValue.isEmpty())
        return rowKey;

    if (cellValue == rowMarker)
        return juce::String { rowIndex };

    if (Transforms::contains (cellValue))
        return Transforms::getTransformed (cellValue, rowKey);

    return cellValue;
}

/**
 * @brief Builds the placeholder substitution map for one relation row.
 *
 * Reads every column of @p tableName's @p row out of @p root, running each
 * value through its declared transform (manifest `## transforms`) where the
 * column name matches a transform row in @p manifestDoc; columns with no
 * matching transform pass through verbatim. The row-key column (column 0)
 * is never resolved. An empty, non-key, non-literal cell resolves to
 * @p projectionValue applied to the row key when @p projectionValue names a
 * transform, or to the row key verbatim when it is empty.
 *
 * @param root            The parsed relation document containing the table.
 * @param tableName       The relation (table) name.
 * @param row             The row to build the map from.
 * @param manifestDoc     The parsed manifest, supplying the `## transforms` table.
 * @param rowIndex        The row's 0-based position among its matched rows.
 * @param projectionValue The dispatch row's `projection` cell, or empty when absent.
 * @return A map from column header to its (possibly transformed) cell value.
 */
static SubstitutionMap buildPerRowMap (
    const jam::MarkdownDocument& root,
    const juce::String& tableName,
    const jam::Document::Element& row,
    const jam::MarkdownDocument& manifestDoc,
    const juce::String& templateText,
    int rowIndex,
    const juce::String& projectionValue,
    const juce::String& fromValue,
    const juce::String& toValue,
    const jam::HashMap<juce::String, std::tuple<juce::String, juce::String, juce::Identifier>>&
        lexicon)
{
    SubstitutionMap map;

    const auto tableId { juce::Identifier (tableName) };
    const auto rowKey { row.id.toString() };
    const auto transformedColumns { manifestDoc.getTableRowKeys (Id::transforms) };

    const auto headers { root.getTableHeaders (tableId) };

    jam::Array<jam::Document::Element*> cells;

    for (auto* cell : row)
        cells.add (cell);

    for (int index { 0 }; index < headers.size(); ++index)
    {
        const auto& header { headers.at (index) };
        const auto* cell { index < cells.size() ? cells.at (index) : nullptr };
        const auto rawValue { cell != nullptr ? cell->getAllSubText() : juce::String() };

        const auto isKeyColumn { index == 0 };
        const auto isBacktickLiteral { cell != nullptr and isLiteralCell (*cell) };
        const auto isProjectable { not isKeyColumn and not isBacktickLiteral and rawValue.isEmpty()
                                   and projectionValue.isNotEmpty() };
        const auto resolved { isBacktickLiteral ? getLiteralValue (*cell)
                              : isKeyColumn     ? rawValue
                              : isProjectable ? Transforms::getTransformed (projectionValue, rowKey)
                                              : getResolvedCell (rawValue, rowKey, rowIndex) };
        const auto value {
            transformedColumns.contains (header)
                ? Transforms::getTransformed (
                      manifestDoc.getTableValue (Id::transforms, Id::transform, header), resolved)
                : resolved
        };

        map.insert ({ header, value });

        for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
        {
            const auto placeholder { header + juce::String::charToString (chars::colon)
                                     + transformName };

            if (templateText.contains (placeholder))
                map.insert ({ placeholder, Transforms::getTransformed (transformName, resolved) });
        }

        if (isKeyColumn)
        {
            map.insert ({ Id::entry.toString(), value });

            for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
            {
                const auto entryPlaceholder {
                    Id::entry.toString() + juce::String::charToString (chars::colon) + transformName
                };

                if (templateText.contains (entryPlaceholder))
                    map.insert (
                        { entryPlaceholder, Transforms::getTransformed (transformName, resolved) });
            }
        }
    }

    if (fromValue.isNotEmpty())
    {
        const auto formatResolved { map.contains (formatColumn)
                                        ? getPlaceholderValue (map.at (formatColumn))
                                        : juce::String() };
        const auto keySource { formatResolved.isNotEmpty()
                                   ? formatResolved
                                   : getPlaceholderValue (map.at (Id::entry.toString())) };
        const auto keySourceFolded { keySource.toLowerCase() };
        const auto isExtensionsDeclared { fromValue == Id::fromId.toString() and not lexicon.empty()
                                          and lexicon.contains (keySourceFolded)
                                          and std::get<2> (lexicon.at (keySourceFolded))
                                                  == Id::extensions };
        const auto fromResolved { isExtensionsDeclared
                                      ? extensionsNamespace + jam::Format::toCamelCase (keySource)
                                      : Transforms::getTransformed (fromValue, keySource) };

        map.insert ({ Id::from.toString(), fromResolved });
        map.insert (
            { Id::from.toString() + juce::String::charToString (chars::colon) + Id::type.toString(),
              Transforms::getTypes().at (fromValue) });
    }

    if (toValue.isNotEmpty())
    {
        const auto valueResolved { getPlaceholderValue (map.at (Id::value.toString())) };

        map.insert ({ Id::to.toString(), Transforms::getTransformed (toValue, valueResolved) });
        map.insert (
            { Id::to.toString() + juce::String::charToString (chars::colon) + Id::type.toString(),
              Transforms::getTypes().at (toValue) });
    }

    return map;
}

/**
 * @brief Builds the table-level placeholder map for a row-region template (SPEC §3.2).
 *
 * `@table@` fills with the table's name and `@brief@` with the prose
 * paragraph authored between the table's heading and the table itself.
 * Every column header additionally fills with the column-0 key of the first
 * row whose raw cell in that column is non-empty, so a marker column such as
 * `default` names its marked row.
 *
 * @param root      The parsed relation document containing the table.
 * @param tableName The relation (table) name.
 * @return A map from table-level placeholder name to its fill text.
 */
static SubstitutionMap buildTableMap (const jam::MarkdownDocument& root,
                                      const juce::String& tableName,
                                      const juce::String& templateText)
{
    SubstitutionMap map;

    const auto tableId { juce::Identifier (tableName) };

    map.insert ({ Id::table.toString(), tableName });

    for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
    {
        const auto placeholder { Id::table.toString() + juce::String::charToString (chars::colon)
                                 + transformName };

        if (templateText.contains (placeholder))
            map.insert ({ placeholder, Transforms::getTransformed (transformName, tableName) });
    }

    juce::String brief;
    bool afterAnchor { false };
    bool reachedTable { false };

    root.root->applyFunctionRecursively (
        [&brief, &afterAnchor, &reachedTable, &tableName] (
            const jam::Document::Element& node) -> bool
        {
            if (node.contains (Id::id)
                and node.get<juce::String> (Id::id)->equalsIgnoreCase (tableName))
                afterAnchor = true;
            else if (afterAnchor and node.contains (Id::type)
                     and *node.get<int> (Id::type) == map::BlockType::table)
                reachedTable = true;
            else if (afterAnchor and not reachedTable and brief.isEmpty() and node.isTag (Id::p))
                brief = node.getAllSubText();

            return not reachedTable;
        });

    map.insert ({ Id::brief.toString(), brief });

    const auto rows { root.getTableRows (tableId) };
    const auto headers { root.getTableHeaders (tableId) };

    for (int headerIndex { 0 }; headerIndex < headers.size(); ++headerIndex)
    {
        const auto& header { headers.at (headerIndex) };
        juce::String marked;

        for (auto* row : rows)
            if (marked.isEmpty())
            {
                int cellIndex { 0 };

                for (auto* cell : *row)
                {
                    if (cellIndex == headerIndex and cell->getAllSubText().isNotEmpty())
                        marked = row->id.toString();

                    ++cellIndex;
                }
            }

        map.insert ({ header, marked });

        for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
        {
            const auto placeholder { header + juce::String::charToString (chars::colon)
                                     + transformName };

            if (templateText.contains (placeholder))
                map.insert ({ placeholder, Transforms::getTransformed (transformName, marked) });
        }
    }

    return map;
}

/**
 * @brief Expands a row-region template once for its whole table (SPEC §3.2).
 *
 * Text between a `@row:begin@` line and its `@row:end@` line repeats once
 * per matching row, each repetition filled from that row's cells; both
 * marker lines are consumed. `@row:begin@` takes no column qualifier --
 * a region scoped to one column opens with that column's own name instead
 * (see the sibling `@\<column\>:begin@` loop). Text outside the regions is
 * filled from the table-level map.
 *
 * @param templateText          The template's raw text.
 * @param root                  The parsed relation document containing the table.
 * @param tableName             The relation (table) name.
 * @param rows                  The rows that matched the dispatch filter.
 * @param manifestDoc           The parsed manifest, supplying `## transforms`.
 * @param columnName            The dispatch row's filter column name (SPEC §3.2 `@column@`/`@cell@`).
 * @param typeValue             The dispatch row's `type` cell, or `int` when absent (SPEC §3.2 `@type@`).
 * @param projectionValue       The dispatch row's `projection` cell, or empty when absent.
 * @param availablePlaceholders Out parameter: every placeholder this expansion filled,
 *                              accumulated for SPEC §8 validation of the same fragment.
 * @param sourceFile            The fragment's file, for SPEC §8 error locations.
 * @param regionResult          Out parameter: FATAL when `@row:begin@` carries a column
 *                              qualifier, or when a `@\<column\>:begin@` region (engine
 *                              feature: slot regions) names neither `row` nor a real column
 *                              of @p tableName; juce::Result::ok() otherwise.
 * @return The expanded text, or an empty string when @p regionResult carries a failure.
 */
static juce::String expandRowRegions (
    const juce::String& templateText,
    const jam::MarkdownDocument& root,
    const juce::String& tableName,
    const jam::Array<jam::Document::Element*>& rows,
    const jam::MarkdownDocument& manifestDoc,
    const juce::String& columnName,
    const juce::String& typeValue,
    const juce::String& projectionValue,
    const juce::String& fromValue,
    const juce::String& toValue,
    const juce::String& symbolValue,
    const jam::HashMap<juce::String, std::tuple<juce::String, juce::String, juce::Identifier>>&
        lexicon,
    SubstitutionMap& availablePlaceholders,
    const juce::File& sourceFile,
    juce::Result& regionResult)
{
    regionResult = juce::Result::ok();

    const auto headers { root.getTableHeaders (juce::Identifier (tableName)) };
    const auto columnIndex { headers.indexOf (columnName) };
    const auto transformedColumns { manifestDoc.getTableRowKeys (Id::transforms) };
    const auto valueColumn { Id::value.toString() };
    const auto valueColumnTransformed { transformedColumns.contains (valueColumn) };

    auto result { templateText };
    auto beginIndex { result.indexOf (rowRegionBeginPrefix) };

    availablePlaceholders.insert (
        { rowRegionEnd.substring (1, rowRegionEnd.length() - 1), juce::String() });
    availablePlaceholders.insert ({ rowRegionIndex, juce::String() });

    while (beginIndex >= 0)
    {
        const auto markerEnd { result.indexOfChar (beginIndex + 1, chars::at) };

        jassert (markerEnd > beginIndex);

        const auto markerKey { result.substring (beginIndex + 1, markerEnd) };

        if (markerKey != Id::rowRegionBegin)
        {
            regionResult = juce::Result::fail (
                getLocation (
                    sourceFile.getFullPathName(), getTextLineNumber (result, beginIndex), markerKey)
                + Id::diagnosticSeparator + text::en::failRowRegionColumn + Id::diagnosticSeparator
                + markerKey);
            return {};
        }

        availablePlaceholders.insert ({ markerKey, juce::String() });

        const auto endIndex { result.indexOf (beginIndex, rowRegionEnd) };

        jassert (endIndex > beginIndex);

        const auto beginLineStart {
            result.substring (0, beginIndex).lastIndexOfChar (chars::newline) + 1
        };
        const auto bodyStart { result.indexOfChar (beginIndex, chars::newline) + 1 };
        const auto bodyEnd { result.substring (0, endIndex).lastIndexOfChar (chars::newline) + 1 };
        const auto afterStart { result.indexOfChar (endIndex, chars::newline) + 1 };

        const auto body { result.substring (bodyStart, bodyEnd) };

        std::string expanded;

        int rowIndex { 0 };

        for (auto* row : rows)
        {
            auto perRowMap { buildPerRowMap (root,
                                             tableName,
                                             *row,
                                             manifestDoc,
                                             body,
                                             rowIndex,
                                             projectionValue,
                                             fromValue,
                                             toValue,
                                             lexicon) };
            perRowMap.insert ({ rowRegionIndex, juce::String { rowIndex } });

            const auto rowKey { row->id.toString() };
            const auto folded { rowKey.toLowerCase() };
            const auto isRegistryHeader { headers.at (0) == Id::entry.toString()
                                          or headers.at (0) == Id::name.toString() };

            if (not lexicon.empty() and isRegistryHeader and lexicon.contains (folded))
            {
                const auto& [name, value, declaringTable] { lexicon.at (folded) };
                juce::ignoreUnused (declaringTable);

                perRowMap.addOrReplace (Id::entry.toString(), name);

                for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
                {
                    const auto placeholder { Id::entry.toString()
                                             + juce::String::charToString (chars::colon)
                                             + transformName };

                    if (body.contains (placeholder))
                        perRowMap.addOrReplace (
                            placeholder, Transforms::getTransformed (transformName, name));
                }

                if (not headers.contains (valueColumn))
                {
                    const auto transformedValue {
                        valueColumnTransformed
                            ? Transforms::getTransformed (
                                  manifestDoc.getTableValue (
                                      Id::transforms, Id::transform, valueColumn),
                                  value)
                            : value
                    };

                    perRowMap.addOrReplace (valueColumn, transformedValue);

                    for (const auto& [transformName, transformFunction] :
                         Transforms::getTransforms())
                    {
                        const auto placeholder {
                            valueColumn + juce::String::charToString (chars::colon) + transformName
                        };

                        if (body.contains (placeholder))
                            perRowMap.insert (
                                { placeholder, Transforms::getTransformed (transformName, value) });
                    }
                }
            }

            int cellIndex { 0 };
            juce::String cellValue;

            for (auto* cell : *row)
            {
                if (cellIndex == columnIndex)
                    cellValue = cell->getAllSubText();

                ++cellIndex;
            }

            perRowMap.insert ({ Id::cell, cellValue });

            for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
            {
                const auto placeholder { Id::cell + juce::String::charToString (chars::colon)
                                         + transformName };

                if (body.contains (placeholder))
                    perRowMap.insert (
                        { placeholder, Transforms::getTransformed (transformName, cellValue) });
            }

            perRowMap.insert ({ Id::type.toString(), typeValue });

            for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
            {
                const auto placeholder {
                    Id::type.toString() + juce::String::charToString (chars::colon) + transformName
                };

                if (body.contains (placeholder))
                    perRowMap.insert (
                        { placeholder, Transforms::getTransformed (transformName, typeValue) });
            }

            perRowMap.insert ({ Id::symbol.toString(), symbolValue });

            for (const auto& [placeholderName, placeholderValue] : perRowMap)
                availablePlaceholders.insert ({ placeholderName, placeholderValue });

            expanded += TemplateEngine::expandText (body, perRowMap).toRawUTF8();
            ++rowIndex;
        }

        result = result.substring (0, beginLineStart)
                 + juce::String::fromUTF8 (expanded.data(), static_cast<int> (expanded.size()))
                 + result.substring (afterStart);
        beginIndex = result.indexOf (rowRegionBeginPrefix);
    }

    // Engine feature: qualified `@<column>:begin@ ... @<column>:end@` regions (MEANING RULE,
    // fragment half) -- every legacy `@row:begin...@` region has already been consumed above,
    // so any remaining `@<name>:begin@` marker here names a filter column directly.
    for (;;)
    {
        const auto namedMarkers { findRegionBeginMarkers (result) };

        RegionMarkerOccurrence namedMarker;
        bool foundNamedMarker { false };

        for (const auto& marker : namedMarkers)
            if (marker.name != Id::row.toString())
            {
                namedMarker = marker;
                foundNamedMarker = true;
                break;
            }

        if (not foundNamedMarker)
            break;

        const auto name { namedMarker.name };

        if (not headers.contains (name))
        {
            regionResult =
                juce::Result::fail (getLocation (sourceFile.getFullPathName(),
                                                 getTextLineNumber (result, namedMarker.index),
                                                 name)
                                    + Id::diagnosticSeparator + text::en::failColumnUnknown
                                    + Id::diagnosticSeparator + name);
            return {};
        }

        const auto namedFilterColumnIndex { headers.indexOf (name) };
        const auto beginMarkerText { juce::String::charToString (chars::at) + name
                                     + regionBeginSuffix };
        const auto endMarkerText { juce::String::charToString (chars::at) + name
                                   + regionEndSuffix };
        const auto namedBeginIndex { result.indexOf (beginMarkerText) };
        const auto namedEndIndex { result.indexOf (namedBeginIndex, endMarkerText) };

        jassert (namedEndIndex > namedBeginIndex);

        const auto namedBeginLineStart {
            result.substring (0, namedBeginIndex).lastIndexOfChar (chars::newline) + 1
        };
        const auto namedBodyStart { result.indexOfChar (namedBeginIndex, chars::newline) + 1 };
        const auto namedBodyEnd {
            result.substring (0, namedEndIndex).lastIndexOfChar (chars::newline) + 1
        };
        const auto namedAfterStart { result.indexOfChar (namedEndIndex, chars::newline) + 1 };
        const auto namedBody { result.substring (namedBodyStart, namedBodyEnd) };

        availablePlaceholders.insert ({ name + regionBeginKeySuffix, juce::String() });
        availablePlaceholders.insert ({ name + regionEndKeySuffix, juce::String() });

        jam::Array<jam::Document::Element*> namedRegionRows;

        for (auto* row : rows)
        {
            int rawCellIndex { 0 };
            juce::String rawCellText;

            for (auto* cell : *row)
            {
                if (rawCellIndex == namedFilterColumnIndex)
                    rawCellText = cell->getAllSubText();

                ++rawCellIndex;
            }

            if (rawCellText.isNotEmpty())
                namedRegionRows.add (row);
        }

        std::string namedExpanded;
        int namedRowIndex { 0 };

        for (auto* row : namedRegionRows)
        {
            auto perRowMap { buildPerRowMap (root,
                                             tableName,
                                             *row,
                                             manifestDoc,
                                             namedBody,
                                             namedRowIndex,
                                             projectionValue,
                                             fromValue,
                                             toValue,
                                             lexicon) };
            perRowMap.insert ({ rowRegionIndex, juce::String { namedRowIndex } });

            const auto rowKey { row->id.toString() };
            const auto folded { rowKey.toLowerCase() };
            const auto isRegistryHeader { headers.at (0) == Id::entry.toString()
                                          or headers.at (0) == Id::name.toString() };

            if (not lexicon.empty() and isRegistryHeader and lexicon.contains (folded))
            {
                const auto& [lexiconName, lexiconValue, lexiconDeclaringTable] { lexicon.at (
                    folded) };
                juce::ignoreUnused (lexiconDeclaringTable);

                perRowMap.addOrReplace (Id::entry.toString(), lexiconName);

                for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
                {
                    const auto placeholder { Id::entry.toString()
                                             + juce::String::charToString (chars::colon)
                                             + transformName };

                    if (namedBody.contains (placeholder))
                        perRowMap.addOrReplace (
                            placeholder, Transforms::getTransformed (transformName, lexiconName));
                }

                if (not headers.contains (valueColumn))
                {
                    const auto transformedValue {
                        valueColumnTransformed
                            ? Transforms::getTransformed (
                                  manifestDoc.getTableValue (
                                      Id::transforms, Id::transform, valueColumn),
                                  lexiconValue)
                            : lexiconValue
                    };

                    perRowMap.addOrReplace (valueColumn, transformedValue);

                    for (const auto& [transformName, transformFunction] :
                         Transforms::getTransforms())
                    {
                        const auto placeholder {
                            valueColumn + juce::String::charToString (chars::colon) + transformName
                        };

                        if (namedBody.contains (placeholder))
                            perRowMap.insert (
                                { placeholder,
                                  Transforms::getTransformed (transformName, lexiconValue) });
                    }
                }
            }

            int cellIndex { 0 };
            juce::String cellValue;

            for (auto* cell : *row)
            {
                if (cellIndex == namedFilterColumnIndex)
                    cellValue = cell->getAllSubText();

                ++cellIndex;
            }

            perRowMap.insert ({ Id::cell, cellValue });

            for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
            {
                const auto placeholder { Id::cell + juce::String::charToString (chars::colon)
                                         + transformName };

                if (namedBody.contains (placeholder))
                    perRowMap.insert (
                        { placeholder, Transforms::getTransformed (transformName, cellValue) });
            }

            perRowMap.insert ({ Id::type.toString(), typeValue });
            perRowMap.insert ({ Id::symbol.toString(), symbolValue });

            for (const auto& [placeholderName, placeholderValue] : perRowMap)
                availablePlaceholders.insert ({ placeholderName, placeholderValue });

            namedExpanded += TemplateEngine::expandText (namedBody, perRowMap).toRawUTF8();
            ++namedRowIndex;
        }

        result =
            result.substring (0, namedBeginLineStart)
            + juce::String::fromUTF8 (namedExpanded.data(), static_cast<int> (namedExpanded.size()))
            + result.substring (namedAfterStart);
    }

    auto tableMap { buildTableMap (root, tableName, result) };

    tableMap.insert ({ Id::column.toString(), columnName });

    for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
    {
        const auto placeholder { Id::column.toString() + juce::String::charToString (chars::colon)
                                 + transformName };

        if (templateText.contains (placeholder))
            tableMap.insert (
                { placeholder, Transforms::getTransformed (transformName, columnName) });
    }

    tableMap.insert ({ Id::type.toString(), typeValue });

    for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
    {
        const auto placeholder { Id::type.toString() + juce::String::charToString (chars::colon)
                                 + transformName };

        if (templateText.contains (placeholder))
            tableMap.insert (
                { placeholder, Transforms::getTransformed (transformName, typeValue) });
    }

    tableMap.insert ({ Id::symbol.toString(), symbolValue });

    if (fromValue.isNotEmpty())
        tableMap.insert (
            { Id::from.toString() + juce::String::charToString (chars::colon) + Id::type.toString(),
              Transforms::getTypes().at (fromValue) });

    if (toValue.isNotEmpty())
        tableMap.insert (
            { Id::to.toString() + juce::String::charToString (chars::colon) + Id::type.toString(),
              Transforms::getTypes().at (toValue) });

    for (const auto& [placeholderName, placeholderValue] : tableMap)
        availablePlaceholders.insert ({ placeholderName, placeholderValue });

    return TemplateEngine::expandText (result, tableMap);
}

/**
 * @brief Engine feature: replaces every root slot region with its slot's collected rows.
 *
 * Scans @p rootText for qualified `@\<slot\>:begin@ ... @\<slot\>:end@` regions and
 * replaces each whole region, markers included, with the direct concatenation
 * (SPEC engine feature: slot regions carry no separator, matching row-region
 * concatenation) of @p slotResults's entries for that slot, in manifest order.
 *
 * @param rootText    The root template's raw text.
 * @param slotResults Per-slot fragment/row text, keyed by slot name.
 * @return @p rootText with every slot region replaced.
 */
static juce::String
substituteSlotRegions (const juce::String& rootText,
                       const jam::HashMap<juce::String, juce::StringArray>& slotResults)
{
    auto result { rootText };

    for (;;)
    {
        const auto markers { findRegionBeginMarkers (result) };

        if (markers.isEmpty())
            break;

        const auto& marker { markers.at (0) };
        const auto beginMarkerText { juce::String::charToString (chars::at) + marker.name
                                     + regionBeginSuffix };
        const auto endMarkerText { juce::String::charToString (chars::at) + marker.name
                                   + regionEndSuffix };
        const auto beginIndex { result.indexOf (beginMarkerText) };
        const auto beginLineStart {
            result.substring (0, beginIndex).lastIndexOfChar (chars::newline) + 1
        };
        const auto endIndex { result.indexOf (beginIndex, endMarkerText) };
        const auto afterStart { result.indexOfChar (endIndex, chars::newline) + 1 };

        const auto replacement { slotResults.contains (marker.name)
                                     ? slotResults.at (marker.name).joinIntoString ({})
                                     : juce::String() };

        result = result.substring (0, beginLineStart) + replacement + result.substring (afterStart);
    }

    return result;
}

/**
 * @brief Builds the slot-to-fragments map from the manifest's dispatch config.
 *
 * Dispatches each relation's rows through their matching fragment templates,
 * collecting the expanded fragments per slot. Identical across every output,
 * so the driver builds it once and shares it across all `processOutput` calls.
 *
 * @param manifestDoc  The parsed manifest.
 * @param manifestFile The manifest file, for error locations.
 * @param dir          The manifest's parent directory.
 * @param dispatchRows Rows of the manifest's `## dispatch` table (row-addressed;
 *                     the same key legitimately labels more than one row).
 * @param rootIndex    Dispatch heading name to its owning relation document.
 * @param roots        Every parsed relation document, for qualified `table::row` reference resolution.
 * @param slotRegions  Every root-owned slot region (engine feature), keyed by slot name.
 * @param slotResults  Out parameter: per-slot fragment/row text, keyed by slot name.
 * @return juce::Result::ok() on success, or the first validation failure.
 */
static juce::Result buildSlotResults (
    const jam::MarkdownDocument& manifestDoc,
    const juce::File& manifestFile,
    const juce::File& dir,
    const jam::Array<jam::Document::Element*>& dispatchRows,
    const jam::HashMap<juce::String, const jam::MarkdownDocument*>& rootIndex,
    const jam::HashMap<juce::String, std::tuple<juce::String, juce::String, juce::Identifier>>&
        lexicon,
    const jam::Array<jam::MarkdownDocument>& roots,
    const SlotRegionMap& slotRegions,
    jam::HashMap<juce::String, juce::StringArray>& slotResults)
{
    jam::HashMap<juce::String, juce::String> fragmentCache;

    for (auto* dispatchRow : dispatchRows)
    {
        const auto dispatchKey { dispatchRow->id.toString() };
        const auto sourceName { getDispatchCell (manifestDoc, *dispatchRow, Id::sourceTable) };
        const auto tableKey { sourceName.isNotEmpty() ? sourceName : dispatchKey };
        const auto found { rootIndex.find (tableKey) };

        if (found == rootIndex.end())
            return juce::Result::fail (manifestFile.getFullPathName() + Id::diagnosticSeparator
                                       + text::en::failTableMissing + Id::diagnosticSeparator
                                       + tableKey + juce::String::charToString (chars::newline)
                                       + Id::dispatchRowLabel + dispatchKey + Id::sourceTableOpen
                                       + tableKey + juce::String::charToString (chars::closeParen));

        const auto& [indexKey, rootPtr] { *found };

        const auto tableId { juce::Identifier (tableKey) };
        const auto columnName { getDispatchCell (manifestDoc, *dispatchRow, Id::column) };
        const auto matchValue { getDispatchCell (manifestDoc, *dispatchRow, Id::value) };
        const auto templateCell { getDispatchCell (manifestDoc, *dispatchRow, Id::templatePath) };
        const auto slotName { getDispatchCell (manifestDoc, *dispatchRow, Id::placeholder) };
        const auto typeCell { getDispatchCell (manifestDoc, *dispatchRow, Id::type) };
        const auto typeValue { typeCell.isNotEmpty() ? typeCell : defaultKeyType };
        const auto projectionValue { getDispatchCell (manifestDoc, *dispatchRow, Id::projection) };
        const auto symbolCell { getDispatchCell (manifestDoc, *dispatchRow, Id::symbol) };
        const auto symbolValue { symbolCell.isEmpty()
                                     ? tableKey
                                     : Transforms::getTransformed (symbolCell, tableKey) };
        const auto fromValue { getDispatchCell (manifestDoc, *dispatchRow, Id::from) };
        const auto toValue { getDispatchCell (manifestDoc, *dispatchRow, Id::to) };

        if (fromValue.isNotEmpty() and not Transforms::contains (fromValue))
            return juce::Result::fail (manifestFile.getFullPathName() + Id::diagnosticSeparator
                                       + text::en::failUnknownTransform + Id::diagnosticSeparator
                                       + fromValue + juce::String::charToString (chars::newline)
                                       + Id::dispatchRowLabel + dispatchKey + Id::sourceTableOpen
                                       + tableKey + juce::String::charToString (chars::closeParen));

        if (toValue.isNotEmpty() and not Transforms::contains (toValue))
            return juce::Result::fail (manifestFile.getFullPathName() + Id::diagnosticSeparator
                                       + text::en::failUnknownTransform + Id::diagnosticSeparator
                                       + toValue + juce::String::charToString (chars::newline)
                                       + Id::dispatchRowLabel + dispatchKey + Id::sourceTableOpen
                                       + tableKey + juce::String::charToString (chars::closeParen));

        // Engine feature rule 3: an empty `template` cell is fed by its slot's root
        // region -- validateDispatchRegionPairing() has already guaranteed slotRegions
        // carries this slot when templateCell is empty.
        const auto isRegionRow { templateCell.isEmpty() };
        const auto fragmentFile { isRegionRow ? dir.getChildFile (
                                                    slotRegions.at (slotName).rootTemplatePath)
                                              : dir.getChildFile (templateCell) };
        const auto fragmentPath { fragmentFile.getFullPathName() };

        if (isRegionRow)
        {
            const auto& region { slotRegions.at (slotName) };

            if (not fragmentCache.contains (fragmentPath + slotName))
            {
                // A body that declares its own regions is already a complete
                // fragment; only a region-free body takes the synthetic per-row wrap.
                const auto hasOwnRegions { not findRegionBeginMarkers (region.body).isEmpty() };

                fragmentCache.insert (
                    { fragmentPath + slotName,
                      hasOwnRegions
                          ? region.body
                          : juce::String::charToString (chars::at) + Id::rowRegionBegin
                                + juce::String::charToString (chars::at)
                                + juce::String::charToString (chars::newline) + region.body
                                + rowRegionEnd + juce::String::charToString (chars::newline) });
            }
        }
        else if (not fragmentCache.contains (fragmentPath))
            fragmentCache.insert ({ fragmentPath, fragmentFile.loadFileAsString() });

        const auto& fragmentText { fragmentCache.at (isRegionRow ? fragmentPath + slotName
                                                                 : fragmentPath) };

        const auto headers { rootPtr->getTableHeaders (tableId) };

        // Engine feature rule 4 (fragment half): `row` is reserved and may not name a
        // dispatched table's own column.
        if (headers.contains (Id::row.toString()))
            return juce::Result::fail (manifestFile.getFullPathName() + Id::diagnosticSeparator
                                       + text::en::failRowReserved + Id::diagnosticSeparator
                                       + tableKey);

        const auto columnIndex { headers.indexOf (columnName) };
        const auto transformedColumns { manifestDoc.getTableRowKeys (Id::transforms) };
        const auto valueColumn { Id::value.toString() };
        const auto valueColumnTransformed { transformedColumns.contains (valueColumn) };

        jam::Array<jam::Document::Element*> matchedRows;

        for (auto* row : rootPtr->getTableRows (tableId))
        {
            if (isSeparatorRow (row->id.toString()))
                continue;

            int cellIndex { 0 };
            juce::String cellValue;

            for (auto* cell : *row)
            {
                if (cellIndex == columnIndex)
                    cellValue = cell->getAllSubText();

                ++cellIndex;
            }

            const auto matches { matchValue.isEmpty() ? cellValue.isNotEmpty()
                                                      : cellValue == matchValue };

            if (matches)
                matchedRows.add (row);
        }

        const auto isRegistryHeader { headers.at (0) == Id::entry.toString()
                                      or headers.at (0) == Id::name.toString() };

        if (not lexicon.empty() and isRegistryHeader)
            for (auto* matchedRow : matchedRows)
            {
                const auto rowKey { matchedRow->id.toString() };
                const auto isQualifiedEntry { rowKey.contains (Id::doubleColon) };

                const auto
                    valid { isQualifiedEntry ? [&rowKey, &roots]
                                {
                                    const auto leftId { juce::Identifier (
                                        rowKey.upToFirstOccurrenceOf (
                                            Id::doubleColon, false, false)) };
                                    const auto rightId { juce::Identifier (
                                        rowKey.fromFirstOccurrenceOf (
                                            Id::doubleColon, false, false)) };
                                    const auto rightPart { rightId.toString() };

                                    return std::any_of (
                                        roots.begin(),
                                        roots.end(),
                                        [&leftId, &rightId, &rightPart] (
                                            const jam::MarkdownDocument& candidate)
                                        {
                                            if (candidate.root->getChildByID (leftId) == nullptr)
                                                return false;

                                            if (candidate.getTableRow (leftId, rightId) != nullptr)
                                                return true;

                                            const auto candidateRowKeys {
                                                candidate.getTableRowKeys (leftId)
                                            };

                                            return std::any_of (
                                                candidateRowKeys.begin(),
                                                candidateRowKeys.end(),
                                                [&rightPart] (const juce::String& candidateRowKey)
                                                {
                                                    return not isSeparatorRow (candidateRowKey)
                                                           and Transforms::getTransformed (
                                                                   Id::toCamel.toString(),
                                                                   candidateRowKey)
                                                                   == rightPart;
                                                });
                                        });
                                }()
                                             : lexicon.contains (rowKey.toLowerCase()) };

                if (not valid)
                {
                    const auto rowOffset { static_cast<uint32_t> (
                        *matchedRow->get<int> (Id::offset)) };
                    const auto rowLine { static_cast<int> (rootPtr->getLineNumber (rowOffset)) };

                    return juce::Result::fail (
                        getLocation (*rootPtr->root->get<juce::String> (Id::path),
                                     rowLine,
                                     Id::entry.toString())
                        + Id::diagnosticSeparator + text::en::failEntityMissing
                        + Id::diagnosticSeparator + rowKey);
                }
            }

        const auto dispatchContext { Id::dispatchRowLabel + dispatchKey + Id::sourceTableOpen
                                     + tableKey + juce::String::charToString (chars::closeParen) };

        const auto fragmentMarkers { findRegionBeginMarkers (fragmentText) };
        const auto hasNamedColumnRegion { std::any_of (fragmentMarkers.begin(),
                                                       fragmentMarkers.end(),
                                                       [] (const RegionMarkerOccurrence& marker)
                                                       {
                                                           return marker.name != Id::row.toString();
                                                       }) };

        if (isRegionRow or fragmentText.contains (rowRegionBeginPrefix) or hasNamedColumnRegion)
        {
            SubstitutionMap availablePlaceholders;
            juce::Result regionResult { juce::Result::ok() };

            const auto expanded { expandRowRegions (fragmentText,
                                                    *rootPtr,
                                                    tableKey,
                                                    matchedRows,
                                                    manifestDoc,
                                                    columnName,
                                                    typeValue,
                                                    projectionValue,
                                                    fromValue,
                                                    toValue,
                                                    symbolValue,
                                                    lexicon,
                                                    availablePlaceholders,
                                                    fragmentFile,
                                                    regionResult) };

            if (not regionResult.wasOk())
                return regionResult;

            const auto placeholderCheck { validatePlaceholders (
                fragmentFile, fragmentText, availablePlaceholders, dispatchContext) };

            if (not placeholderCheck.wasOk())
                return placeholderCheck;

            slotResults[slotName].add (expanded);
        }
        else
        {
            int rowIndex { 0 };

            for (auto* matchedRow : matchedRows)
            {
                const auto rowKey { matchedRow->id.toString() };
                auto perRowMap { buildPerRowMap (*rootPtr,
                                                 tableKey,
                                                 *matchedRow,
                                                 manifestDoc,
                                                 fragmentText,
                                                 rowIndex,
                                                 projectionValue,
                                                 fromValue,
                                                 toValue,
                                                 lexicon) };

                if (not lexicon.empty() and isRegistryHeader
                    and lexicon.contains (rowKey.toLowerCase()))
                {
                    const auto folded { rowKey.toLowerCase() };
                    const auto& [name, value, declaringTable] { lexicon.at (folded) };
                    juce::ignoreUnused (declaringTable);

                    perRowMap.addOrReplace (Id::entry.toString(), name);

                    for (const auto& [transformName, transformFunction] :
                         Transforms::getTransforms())
                    {
                        const auto placeholder { Id::entry.toString()
                                                 + juce::String::charToString (chars::colon)
                                                 + transformName };

                        if (fragmentText.contains (placeholder))
                            perRowMap.addOrReplace (
                                placeholder, Transforms::getTransformed (transformName, name));
                    }

                    if (not headers.contains (valueColumn))
                    {
                        const auto transformedValue {
                            valueColumnTransformed
                                ? Transforms::getTransformed (
                                      manifestDoc.getTableValue (
                                          Id::transforms, Id::transform, valueColumn),
                                      value)
                                : value
                        };

                        perRowMap.addOrReplace (valueColumn, transformedValue);

                        for (const auto& [transformName, transformFunction] :
                             Transforms::getTransforms())
                        {
                            const auto placeholder { valueColumn
                                                     + juce::String::charToString (chars::colon)
                                                     + transformName };

                            if (fragmentText.contains (placeholder))
                                perRowMap.insert (
                                    { placeholder,
                                      Transforms::getTransformed (transformName, value) });
                        }
                    }
                }

                int cellIndex { 0 };
                juce::String cellValue;

                for (auto* cell : *matchedRow)
                {
                    if (cellIndex == columnIndex)
                        cellValue = cell->getAllSubText();

                    ++cellIndex;
                }

                perRowMap.insert ({ Id::cell, cellValue });
                perRowMap.insert ({ Id::column.toString(), columnName });
                perRowMap.insert ({ Id::type.toString(), typeValue });

                for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
                {
                    const auto cellPlaceholder {
                        Id::cell + juce::String::charToString (chars::colon) + transformName
                    };
                    const auto columnPlaceholder { Id::column.toString()
                                                   + juce::String::charToString (chars::colon)
                                                   + transformName };
                    const auto typePlaceholder { Id::type.toString()
                                                 + juce::String::charToString (chars::colon)
                                                 + transformName };

                    if (fragmentText.contains (cellPlaceholder))
                        perRowMap.insert (
                            { cellPlaceholder,
                              Transforms::getTransformed (transformName, cellValue) });

                    if (fragmentText.contains (columnPlaceholder))
                        perRowMap.insert (
                            { columnPlaceholder,
                              Transforms::getTransformed (transformName, columnName) });

                    if (fragmentText.contains (typePlaceholder))
                        perRowMap.insert (
                            { typePlaceholder,
                              Transforms::getTransformed (transformName, typeValue) });
                }

                perRowMap.insert ({ Id::symbol.toString(), symbolValue });

                const auto rowText { TemplateEngine::expandText (fragmentText, perRowMap) };
                const auto placeholderCheck { validatePlaceholders (
                    fragmentFile, fragmentText, perRowMap, dispatchContext) };

                if (not placeholderCheck.wasOk())
                    return placeholderCheck;

                slotResults[slotName].add (rowText);

                ++rowIndex;
            }
        }
    }

    return juce::Result::ok();
}

/**
 * @brief Generates one declared output row from already-built slot results.
 *
 * Expands the root template from @p slotResults.
 *
 * @p writeOutputs gates the write side of SPEC §8 atomicity: the driver
 * calls this function once per output with @p writeOutputs false to validate
 * every output before any bytes are written, then calls it again per output
 * with @p writeOutputs true to perform the identical work and write
 * write-if-different (SPEC §5).
 *
 * @param manifestDoc  The parsed manifest.
 * @param manifestFile The manifest file (for error locations).
 * @param dir          The manifest's parent directory.
 * @param slotResults  Per-slot fragment/row text, keyed by slot name.
 * @param outputKey    The `## outputs` row key identifying this output.
 * @param writeOutputs When true, writes the output file if its bytes differ from disk.
 * @param outputBanner The formatted banner prepended to generated output when non-empty.
 * @return juce::Result::ok() on success, or the first validation failure.
 */
static juce::Result processOutput (const jam::MarkdownDocument& manifestDoc,
                                   const juce::File& manifestFile,
                                   const juce::File& dir,
                                   const jam::HashMap<juce::String, juce::StringArray>& slotResults,
                                   const juce::String& outputKey,
                                   bool writeOutputs,
                                   const juce::String& outputBanner)
{
    const auto rootTemplatePath { manifestDoc.getTableValue (
        Id::generated, Id::templatePath, outputKey) };
    const auto separatorPath { manifestDoc.getTableValue (
        Id::generated, Id::separator, outputKey) };

    juce::String separatorText;

    if (separatorPath.isNotEmpty())
    {
        const auto separatorFile { dir.getChildFile (separatorPath) };

        if (not separatorFile.existsAsFile())
            return juce::Result::fail (manifestFile.getFullPathName() + Id::diagnosticSeparator
                                       + text::en::failTemplateMissing + Id::diagnosticSeparator
                                       + separatorPath);

        separatorText = separatorFile.loadFileAsString();
    }

    const auto rootFile { dir.getChildFile (rootTemplatePath) };
    const auto rawRootText { rootFile.loadFileAsString() };

    SubstitutionMap rootKeys;

    for (const auto& [slot, fragments] : slotResults)
        rootKeys.insert ({ slot, fragments });

    // Engine feature: a slot region's body is validated fragment-side, in
    // buildSlotResults(), not here -- so the root's own text is checked with every
    // region (markers and body alike) stripped out via substituteSlotRegions()
    // against an empty slot-results map, leaving only text root-level placeholders
    // can appear in.
    const jam::HashMap<juce::String, juce::StringArray> noSlotResults;
    const auto rootOwnText { substituteSlotRegions (rawRootText, noSlotResults) };

    const auto placeholderCheck { validatePlaceholders (
        rootFile, rootOwnText, rootKeys, Id::outputRowLabel + outputKey) };

    if (not placeholderCheck.wasOk())
        return placeholderCheck;

    const auto regionExpandedText { substituteSlotRegions (rawRootText, slotResults) };

    SubstitutionMap rootMap;

    for (const auto& [slot, fragments] : slotResults)
        rootMap.insert ({ slot, fragments.joinIntoString (separatorText) });

    const auto outputText { TemplateEngine::expandText (regionExpandedText, rootMap) };

    const auto finalOutput { outputBanner.isNotEmpty()
                                 ? outputBanner + juce::String::charToString (chars::newline)
                                       + juce::String::charToString (chars::newline) + outputText
                                 : outputText };

    if (writeOutputs)
    {
        const auto outputPath { manifestDoc.getTableValue (Id::generated, Id::output, outputKey) };
        const auto outputFile { dir.getChildFile (outputPath) };
        const auto outputDirectory { outputFile.getParentDirectory() };
        const auto existing { outputFile.loadFileAsString() };

        jam::File::getOrCreateDirectory (
            outputDirectory.getParentDirectory(), outputDirectory.getFileName());

        if (finalOutput != existing)
            outputFile.replaceWithText (
                finalOutput, false, false, juce::String::charToString (chars::newline).toRawUTF8());
    }

    return juce::Result::ok();
}

namespace Driver
{
/*____________________________________________________________________________*/

/**
 * @brief Regenerates a manifest's declared outputs.
 *
 * Parses and validates @p manifestFile, then processes every declared
 * `## outputs` row (or only the row matching @p outputFilter, when given) in
 * two passes: a validate-everything pass with no writes, then an identical
 * pass that writes each output write-if-different (SPEC §8 atomicity — no
 * output is written while any output could still fail validation).
 *
 * @param manifestFile The `CAST.md` manifest to run.
 * @param outputFilter When non-empty, restricts regeneration to the named output row.
 * @return juce::Result::ok() on success; failure on the first validation
 *         error, or when @p outputFilter names no declared output.
 */
static juce::Result run (const juce::File& manifestFile, const juce::String& outputFilter = {})
{
    const auto manifestDoc { jam::Markdown::parse (manifestFile.loadFileAsString()) };

    const auto validation { validateManifest (manifestDoc, manifestFile) };

    if (not validation.wasOk())
        return validation;

    const auto dir { manifestFile.getParentDirectory() };
    const auto outputBanner { getOutputBanner (dir) };
    const auto outputKeys { manifestDoc.getTableRowKeys (Id::generated) };
    const auto dispatchRows { manifestDoc.getTableRows (Id::dispatch) };
    const auto constraintKeys { manifestDoc.getTableRowKeys (Id::constraints) };

    jam::Array<juce::String> dispatchKeys;

    for (auto* dispatchRow : dispatchRows)
        dispatchKeys.add (dispatchRow->id.toString());

    jam::Array<juce::String> slotNames;

    for (auto* dispatchRow : dispatchRows)
        slotNames.addIfNotAlreadyThere (
            getDispatchCell (manifestDoc, *dispatchRow, Id::placeholder));

    SlotRegionMap slotRegions;
    const auto slotRegionResult { collectSlotRegions (
        manifestDoc, manifestFile, dir, slotNames, slotRegions) };

    if (not slotRegionResult.wasOk())
        return slotRegionResult;

    const auto regionPairingResult { validateDispatchRegionPairing (
        manifestDoc, manifestFile, dispatchRows, slotRegions) };

    if (not regionPairingResult.wasOk())
        return regionPairingResult;

    jam::Array<jam::MarkdownDocument> roots;

    {
        const auto tableFiles { dir.getChildFile (Id::tables.toString())
                                    .findChildFiles (juce::File::findFiles,
                                                     false,
                                                     Id::asteriskDot.toString() + extensions::md) };
        const auto fileCount { tableFiles.size() };

        roots.resize (fileCount);

        {
            juce::ThreadPool parsePool;

            for (int i { 0 }; i < fileCount; ++i)
                parsePool.addJob (
                    [&tableFiles, &roots, i]
                    {
                        const auto& entry { tableFiles.getReference (i) };

                        roots[i] = jam::Markdown::parse (entry.loadFileAsString());
                        roots[i].root->add<juce::String> (Id::path, entry.getFullPathName());
                    });

            while (parsePool.getNumJobs() > 0)
                juce::Thread::sleep (1);
        }
    }

    jam::HashMap<juce::String, std::tuple<juce::String, juce::String, juce::Identifier>> lexicon;
    static const jam::Array<juce::Identifier> registryTableIds {
        Id::lexicon, Id::chars, Id::files, Id::extensions
    };

    for (const auto& root : roots)
        for (const auto& registryTableId : registryTableIds)
        {
            const auto lexiconKeys { root.getTableRowKeys (registryTableId) };

            for (int ki { 0 }; ki < lexiconKeys.size(); ++ki)
            {
                const auto& canonical { lexiconKeys[ki] };

                if (isSeparatorRow (canonical))
                    continue;

                const auto folded { canonical.toLowerCase() };

                const auto rowLocation = [&root, &registryTableId, &canonical]
                {
                    return getLocation (
                        *root.root->get<juce::String> (Id::path),
                        getRowLineNumber (root, registryTableId.toString(), canonical),
                        Id::name.toString());
                };

                if (lexicon.contains (folded))
                    return juce::Result::fail (
                        rowLocation() + Id::diagnosticSeparator + text::en::failDuplicate
                        + canonical + text::en::failAlreadyDeclared
                        + getLocation (*root.root->get<juce::String> (Id::path),
                                       getRowLineNumber (root,
                                                         registryTableId.toString(),
                                                         std::get<0> (lexicon.at (folded))),
                                       Id::name.toString()));

                const auto nameResult { validateLexiconName (canonical) };

                if (not nameResult.wasOk())
                    return juce::Result::fail (rowLocation() + Id::diagnosticSeparator
                                               + nameResult.getErrorMessage());

                const auto* valueCell { root.getTableCell (
                    registryTableId, Id::value, juce::Identifier (canonical)) };
                const auto isBacktickLiteral { valueCell != nullptr
                                               and isLiteralCell (*valueCell) };
                const auto rawValue {
                    isBacktickLiteral ? getLiteralValue (*valueCell)
                                      : root.getTableValue (registryTableId, Id::value, canonical)
                };

                if (isTooLong (rawValue))
                    return juce::Result::fail (
                        rowLocation() + Id::diagnosticSeparator + text::en::failTooLong
                        + juce::String (maxLexiconNameLength) + text::en::failTooLongSuffix
                        + Id::diagnosticSeparator + rawValue);

                if (not isBacktickLiteral and rawValue.isNotEmpty())
                {
                    const auto redundancyResult { validateLexiconValueRedundancy (
                        canonical, rawValue) };

                    if (not redundancyResult.wasOk())
                        return juce::Result::fail (rowLocation() + Id::diagnosticSeparator
                                                   + redundancyResult.getErrorMessage());
                }

                lexicon.insert ({
                    folded,
                    { canonical, rawValue.isEmpty() ? canonical : rawValue, registryTableId }
                });
            }
        }

    jam::HashMap<juce::String, const jam::MarkdownDocument*> rootIndex;

    for (const auto& root : roots)
        for (const auto& dispatchKey : dispatchKeys)
            if (root.root->getChildByID (juce::Identifier (dispatchKey)) != nullptr)
            {
                const auto existing { rootIndex.find (dispatchKey) };

                if (existing != rootIndex.end() and existing->second != &root)
                    return juce::Result::fail (
                        manifestFile.getFullPathName() + Id::diagnosticSeparator
                        + text::en::failTableAmbiguous + Id::diagnosticSeparator + dispatchKey
                        + Id::diagnosticSeparator
                        + *existing->second->root->get<juce::String> (Id::path)
                        + Id::diagnosticSeparator + *root.root->get<juce::String> (Id::path));

                rootIndex.insert ({ dispatchKey, &root });
            }

    for (auto* dispatchRow : dispatchRows)
    {
        const auto sourceName { getDispatchCell (manifestDoc, *dispatchRow, Id::sourceTable) };

        if (sourceName.isNotEmpty() and not rootIndex.contains (sourceName))
            for (const auto& root : roots)
                if (root.root->getChildByID (juce::Identifier (sourceName)) != nullptr)
                    rootIndex.insert ({ sourceName, &root });
    }

    const auto rootsResult { validateRoots (
        roots, manifestDoc, dispatchKeys, constraintKeys, dir) };

    if (not rootsResult.wasOk())
        return rootsResult;

    const auto perColumnResult { validatePerColumnConstraints (
        manifestDoc, constraintKeys, roots, dir, manifestFile.getFullPathName()) };

    if (not perColumnResult.wasOk())
        return perColumnResult;

    jam::HashMap<juce::String, juce::StringArray> slotResults;

    const auto dispatchResult { buildSlotResults (manifestDoc,
                                                  manifestFile,
                                                  dir,
                                                  dispatchRows,
                                                  rootIndex,
                                                  lexicon,
                                                  roots,
                                                  slotRegions,
                                                  slotResults) };

    if (not dispatchResult.wasOk())
        return dispatchResult;

    for (const auto& outputKey : outputKeys)
        if (outputFilter.isEmpty() or outputKey == outputFilter)
        {
            const auto result { processOutput (
                manifestDoc, manifestFile, dir, slotResults, outputKey, false, outputBanner) };

            if (not result.wasOk())
                return result;
        }

    for (const auto& outputKey : outputKeys)
        if (outputFilter.isEmpty() or outputKey == outputFilter)
        {
            const auto result { processOutput (
                manifestDoc, manifestFile, dir, slotResults, outputKey, true, outputBanner) };

            if (not result.wasOk())
                return result;
        }

    return (outputFilter.isEmpty() or outputKeys.contains (outputFilter))
               ? juce::Result::ok()
               : juce::Result::fail (manifestFile.getFullPathName() + Id::diagnosticSeparator
                                     + text::en::failOutputMissing + Id::diagnosticSeparator
                                     + outputFilter);
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace Driver

/**______________________________END OF NAMESPACE______________________________*/
}// namespace cast
