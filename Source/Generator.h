#pragma once
#include <JuceHeader.h>
#include <string>
#include "Validator.h"
#include "Writer.h"
#include "Operators.h"

namespace cast
{
/*____________________________________________________________________________*/

static juce::String getOutputBanner (const juce::File& dir)
{
    const auto bannerFile { dir.getChildFile (Id::castOutput.toString()) };

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
        line = Id::charSpace + line;

    lines.insert (0, Id::bannerRuleOpen);
    lines.add (Id::bannerRuleClose);

    return lines.joinIntoString (Id::charNewline);
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

    jassert (raw.startsWithChar (Chars::backtick) and raw.endsWithChar (Chars::backtick));

    return raw.substring (1, raw.length() - 1);
}

/**
 * @brief Resolves one cell value against the row key (SPEC §3.1).
 *
 * An empty cell resolves to @p rowKey; a cell naming a transform in the
 * closed vocabulary resolves to that transform applied to @p rowKey; any
 * other cell is its own literal value.
 *
 * @param cellValue The raw cell text.
 * @param rowKey    The row's column-0 key.
 * @return The resolved cell text.
 */
static juce::String getResolvedCell (const juce::String& cellValue, const juce::String& rowKey)
{
    if (cellValue.isEmpty())
        return rowKey;

    if (Transforms::contains (cellValue))
        return Transforms::getTransformed (cellValue, rowKey);

    return cellValue;
}

/**
 * @brief Builds the placeholder substitution map for one relation row.
 *
 * Reads every column of @p tableName's row keyed by @p rowKey out of @p root,
 * running each value through its declared transform (manifest `## transforms`)
 * where the column name matches a transform row in @p manifestDoc; columns
 * with no matching transform pass through verbatim. The row-key column
 * (column 0) is never resolved.
 *
 * @param root        The parsed relation document containing the table.
 * @param tableName   The relation (table) name.
 * @param rowKey      The row's column-0 key.
 * @param manifestDoc The parsed manifest, supplying the `## transforms` table.
 * @return A map from column header to its (possibly transformed) cell value.
 */
static SubstitutionMap buildPerRowMap (const jam::MarkdownDocument& root,
                                       const juce::String& tableName,
                                       const juce::String& rowKey,
                                       const jam::MarkdownDocument& manifestDoc,
                                       const juce::String& templateText)
{
    SubstitutionMap map;

    const auto tableId { juce::Identifier (tableName) };
    const auto transformedColumns { manifestDoc.getTableRowKeys (Id::transforms) };

    const auto headers { root.getTableHeaders (tableId) };

    for (int index { 0 }; index < headers.size(); ++index)
    {
        const auto& header { headers.at (index) };
        const auto columnId { juce::Identifier (header) };
        const auto rawValue { root.getTableValue (tableId, columnId, rowKey) };
        const auto* cell { root.getTableCell (tableId, columnId, juce::Identifier (rowKey)) };

        const auto isKeyColumn { index == 0 };
        const auto isBacktickLiteral { cell != nullptr and isLiteralCell (*cell) };
        const auto resolved { isBacktickLiteral ? getLiteralValue (*cell)
                              : isKeyColumn      ? rawValue
                                                 : getResolvedCell (rawValue, rowKey) };
        const auto value {
            transformedColumns.contains (header)
                ? Transforms::getTransformed (
                      manifestDoc.getTableValue (Id::transforms, Id::transform, header), resolved)
                : resolved
        };

        map.insert ({ header, value });

        for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
        {
            const auto placeholder { header + Id::charColon + transformName };

            if (templateText.contains (placeholder))
                map.insert ({ placeholder, Transforms::getTransformed (transformName, resolved) });
        }
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
        const auto placeholder { Id::table.toString() + Id::charColon + transformName };

        if (templateText.contains (placeholder))
            map.insert ({ placeholder, Transforms::getTransformed (transformName, tableName) });
    }

    juce::String brief;
    bool afterAnchor { false };
    bool reachedTable { false };

    root.root->applyFunctionRecursively (
        [&brief, &afterAnchor, &reachedTable, &tableName] (const jam::Document::Element& node) -> bool
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

    const auto rowKeys { root.getTableRowKeys (tableId) };

    for (const auto& header : root.getTableHeaders (tableId))
    {
        juce::String marked;

        for (const auto& rowKey : rowKeys)
            if (marked.isEmpty()
                and root.getTableValue (tableId, juce::Identifier (header), rowKey).isNotEmpty())
                marked = rowKey;

        map.insert ({ header, marked });
    }

    return map;
}

static const juce::String rowRegionBegin { "@row:begin@" };
static const juce::String rowRegionEnd { "@row:end@" };
static const juce::String rowRegionIndex { "row:index" };

/**
 * @brief Expands a row-region template once for its whole table (SPEC §3.2).
 *
 * Text between a `@row:begin@` line and its `@row:end@` line repeats once
 * per matching row, each repetition filled from that row's cells; both
 * marker lines are consumed. Text outside the regions is filled from the
 * table-level map.
 *
 * @param templateText          The template's raw text.
 * @param root                  The parsed relation document containing the table.
 * @param tableName             The relation (table) name.
 * @param rowKeys               The keys of the rows that matched the dispatch filter.
 * @param manifestDoc           The parsed manifest, supplying `## transforms`.
 * @param columnName            The dispatch row's filter column name (SPEC §3.2 `@column@`/`@cell@`).
 * @param availablePlaceholders Out parameter: every placeholder this expansion filled,
 *                              accumulated for SPEC §8 validation of the same fragment.
 * @return The expanded text.
 */
static juce::String expandRowRegions (const juce::String& templateText,
                                      const jam::MarkdownDocument& root,
                                      const juce::String& tableName,
                                      const jam::Array<juce::String>& rowKeys,
                                      const jam::MarkdownDocument& manifestDoc,
                                      const juce::String& columnName,
                                      const jam::HashMap<juce::String, std::pair<juce::String, juce::String>>& lexicon,
                                      SubstitutionMap& availablePlaceholders)
{
    const auto headers { root.getTableHeaders (juce::Identifier (tableName)) };

    auto result { templateText };
    auto beginIndex { result.indexOf (rowRegionBegin) };

    availablePlaceholders.insert ({ rowRegionBegin.substring (1, rowRegionBegin.length() - 1), juce::String() });
    availablePlaceholders.insert ({ rowRegionEnd.substring (1, rowRegionEnd.length() - 1), juce::String() });
    availablePlaceholders.insert ({ rowRegionIndex, juce::String() });

    while (beginIndex >= 0)
    {
        const auto endIndex { result.indexOf (beginIndex, rowRegionEnd) };

        jassert (endIndex > beginIndex);

        const auto beginLineStart { result.substring (0, beginIndex).lastIndexOfChar (Chars::newline) + 1 };
        const auto bodyStart { result.indexOfChar (beginIndex, Chars::newline) + 1 };
        const auto bodyEnd { result.substring (0, endIndex).lastIndexOfChar (Chars::newline) + 1 };
        const auto afterStart { result.indexOfChar (endIndex, Chars::newline) + 1 };

        const auto body { result.substring (bodyStart, bodyEnd) };

        std::string expanded;

        int rowIndex { 0 };

        for (const auto& rowKey : rowKeys)
        {
            auto perRowMap { buildPerRowMap (root, tableName, rowKey, manifestDoc, body) };
            perRowMap.insert ({ rowRegionIndex, juce::String { rowIndex } });

            if (not lexicon.empty() and headers.at (0) == Id::entry.toString())
            {
                const auto folded { rowKey.toLowerCase() };
                const auto& [name, value] { lexicon.at (folded) };

                perRowMap.addOrReplace (Id::entry.toString(), name);
                perRowMap.addOrReplace (Id::value.toString(), value);

                for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
                {
                    const auto placeholder { Id::value.toString() + Id::charColon + transformName };

                    if (body.contains (placeholder))
                        perRowMap.insert ({ placeholder, Transforms::getTransformed (transformName, value) });
                }
            }

            const auto cellValue { root.getTableValue (
                juce::Identifier (tableName), juce::Identifier (columnName), rowKey) };

            perRowMap.insert ({ Id::cell, cellValue });

            for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
            {
                const auto placeholder { Id::cell + Id::charColon + transformName };

                if (body.contains (placeholder))
                    perRowMap.insert ({ placeholder, Transforms::getTransformed (transformName, cellValue) });
            }

            for (const auto& [placeholderName, placeholderValue] : perRowMap)
                availablePlaceholders.insert ({ placeholderName, placeholderValue });

            expanded += TemplateEngine::expandText (body, perRowMap).toRawUTF8();
            ++rowIndex;
        }

        result = result.substring (0, beginLineStart)
                 + juce::String::fromUTF8 (expanded.data(), static_cast<int> (expanded.size()))
                 + result.substring (afterStart);
        beginIndex = result.indexOf (rowRegionBegin);
    }

    auto tableMap { buildTableMap (root, tableName, templateText) };

    tableMap.insert ({ Id::column.toString(), columnName });

    for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
    {
        const auto placeholder { Id::column.toString() + Id::charColon + transformName };

        if (templateText.contains (placeholder))
            tableMap.insert ({ placeholder, Transforms::getTransformed (transformName, columnName) });
    }

    for (const auto& [placeholderName, placeholderValue] : tableMap)
        availablePlaceholders.insert ({ placeholderName, placeholderValue });

    return TemplateEngine::expandText (result, tableMap);
}

/**
 * @brief Expands the root template for one output, filling its slot placeholders.
 *
 * Every entry of @p slotResults becomes an aggregate placeholder in the root
 * template, expanded in the authored row order the fragments were collected in.
 *
 * @param dir              The manifest's parent directory (template paths are relative to it).
 * @param rootTemplatePath The root template's path, relative to @p dir.
 * @param slotResults      Per-slot fragment text, keyed by slot name.
 * @return The expanded output text, LF-normalized.
 */
static juce::String getOutput (const juce::File& dir,
                               const juce::String& rootTemplatePath,
                               const jam::HashMap<juce::String, juce::StringArray>& slotResults)
{
    SubstitutionMap rootMap;

    for (const auto& [slot, fragments] : slotResults)
        rootMap.insert ({ slot, fragments });

    return TemplateEngine::expand (dir.getChildFile (rootTemplatePath), rootMap);
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
 * @param dispatchKeys Row keys of the manifest's `## dispatch` table.
 * @param rootIndex    Dispatch heading name to its owning relation document.
 * @param slotResults  Out parameter: per-slot fragment text, keyed by slot name.
 * @return juce::Result::ok() on success, or the first validation failure.
 */
static juce::Result buildSlotResults (
    const jam::MarkdownDocument& manifestDoc,
    const juce::File& manifestFile,
    const juce::File& dir,
    const jam::Array<juce::String>& dispatchKeys,
    const jam::HashMap<juce::String, const jam::MarkdownDocument*>& rootIndex,
    const jam::HashMap<juce::String, std::pair<juce::String, juce::String>>& lexicon,
    jam::HashMap<juce::String, juce::StringArray>& slotResults)
{
    jam::HashMap<juce::String, juce::String> fragmentCache;

    for (const auto& dispatchKey : dispatchKeys)
    {
        const auto sourceName { manifestDoc.getTableValue (Id::dispatch, Id::sourceTable, dispatchKey) };
        const auto tableKey { sourceName.isNotEmpty() ? sourceName : dispatchKey };
        const auto found { rootIndex.find (tableKey) };

        if (found == rootIndex.end())
            return juce::Result::fail (manifestFile.getFullPathName() + Id::diagnosticSeparator
                                       + Id::failTableMissing + Id::diagnosticSeparator + tableKey
                                       + Id::charNewline + Id::dispatchRowLabel + dispatchKey
                                       + Id::sourceTableOpen + tableKey + Id::charCloseParen);

        const auto& [indexKey, rootPtr] { *found };

        const auto tableId { juce::Identifier (tableKey) };
        const auto columnName { manifestDoc.getTableValue (
            Id::dispatch, Id::column, dispatchKey) };
        const auto matchValue { manifestDoc.getTableValue (
            Id::dispatch, Id::value, dispatchKey) };
        const auto fragmentFile { dir.getChildFile (
            manifestDoc.getTableValue (Id::dispatch, Id::templatePath, dispatchKey)) };
        const auto slotName { manifestDoc.getTableValue (Id::dispatch, Id::slot, dispatchKey) };

        const auto fragmentPath { fragmentFile.getFullPathName() };

        if (not fragmentCache.contains (fragmentPath))
            fragmentCache.insert ({ fragmentPath, fragmentFile.loadFileAsString() });

        const auto& fragmentText { fragmentCache.at (fragmentPath) };

        jam::Array<juce::String> matchedKeys;

        for (const auto& rowKey : rootPtr->getTableRowKeys (tableId))
        {
            const auto cellValue { rootPtr->getTableValue (
                tableId, juce::Identifier (columnName), rowKey) };
            const auto matches { matchValue.isEmpty() ? cellValue.isNotEmpty()
                                                      : cellValue == matchValue };

            if (matches)
                matchedKeys.add (rowKey);
        }

        const auto headers { rootPtr->getTableHeaders (tableId) };

        if (not lexicon.empty() and headers.at (0) == Id::entry.toString())
            for (const auto& rowKey : matchedKeys)
            {
                const auto folded { rowKey.toLowerCase() };

                if (not lexicon.contains (folded))
                    return juce::Result::fail (
                        getLocation (*rootPtr->root->get<juce::String> (Id::path),
                                    getRowNumber (*rootPtr, tableKey, rowKey),
                                    Id::entry.toString())
                        + Id::diagnosticSeparator + Id::failEntityMissing + Id::diagnosticSeparator
                        + rowKey);
            }

        const auto dispatchContext { Id::dispatchRowLabel + dispatchKey + Id::sourceTableOpen
                                    + tableKey + Id::charCloseParen };

        if (fragmentText.contains (rowRegionBegin))
        {
            SubstitutionMap availablePlaceholders;

            const auto expanded { expandRowRegions (
                fragmentText, *rootPtr, tableKey, matchedKeys, manifestDoc, columnName, lexicon, availablePlaceholders) };

            const auto placeholderCheck { validatePlaceholders (
                fragmentFile, fragmentText, availablePlaceholders, dispatchContext) };

            if (not placeholderCheck.wasOk())
                return placeholderCheck;

            slotResults[slotName].add (expanded);
        }
        else
        {
            for (const auto& rowKey : matchedKeys)
            {
                auto perRowMap { buildPerRowMap (
                    *rootPtr, tableKey, rowKey, manifestDoc, fragmentText) };

                if (not lexicon.empty() and headers.at (0) == Id::entry.toString())
                {
                    const auto folded { rowKey.toLowerCase() };
                    const auto& [name, value] { lexicon.at (folded) };

                    perRowMap.addOrReplace (Id::entry.toString(), name);
                    perRowMap.addOrReplace (Id::value.toString(), value);

                    for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
                    {
                        const auto placeholder { Id::value.toString() + Id::charColon + transformName };

                        if (fragmentText.contains (placeholder))
                            perRowMap.insert ({ placeholder, Transforms::getTransformed (transformName, value) });
                    }
                }

                const auto cellValue { rootPtr->getTableValue (
                    tableId, juce::Identifier (columnName), rowKey) };

                perRowMap.insert ({ Id::cell, cellValue });
                perRowMap.insert ({ Id::column.toString(), columnName });

                for (const auto& [transformName, transformFunction] : Transforms::getTransforms())
                {
                    const auto cellPlaceholder { Id::cell + Id::charColon + transformName };
                    const auto columnPlaceholder { Id::column.toString() + Id::charColon + transformName };

                    if (fragmentText.contains (cellPlaceholder))
                        perRowMap.insert (
                            { cellPlaceholder, Transforms::getTransformed (transformName, cellValue) });

                    if (fragmentText.contains (columnPlaceholder))
                        perRowMap.insert (
                            { columnPlaceholder, Transforms::getTransformed (transformName, columnName) });
                }

                const auto rowText { TemplateEngine::expandText (fragmentText, perRowMap) };
                const auto placeholderCheck { validatePlaceholders (
                    fragmentFile, fragmentText, perRowMap, dispatchContext) };

                if (not placeholderCheck.wasOk())
                    return placeholderCheck;

                slotResults[slotName].add (rowText);
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
 * @param slotResults  Per-slot fragment text, keyed by slot name.
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

    const auto outputText { getOutput (dir, rootTemplatePath, slotResults) };
    const auto rootFile { dir.getChildFile (rootTemplatePath) };

    SubstitutionMap rootKeys;

    for (const auto& [slot, fragments] : slotResults)
        rootKeys.insert ({ slot, fragments });

    const auto placeholderCheck { validatePlaceholders (
        rootFile, rootFile.loadFileAsString(), rootKeys, Id::outputRowLabel + outputKey) };

    if (not placeholderCheck.wasOk())
        return placeholderCheck;

    const auto finalOutput { outputBanner.isNotEmpty()
                                 ? outputBanner + Id::charNewline + Id::charNewline + outputText
                                 : outputText };

    if (writeOutputs)
    {
        const auto outputPath { manifestDoc.getTableValue (
            Id::generated, Id::outputPath, outputKey) };
        const auto outputFile { dir.getChildFile (outputPath) };
        const auto outputDirectory { outputFile.getParentDirectory() };
        const auto existing { outputFile.loadFileAsString() };

        jam::File::getOrCreateDirectory (outputDirectory.getParentDirectory(),
                                         outputDirectory.getFileName());

        if (finalOutput != existing)
            outputFile.replaceWithText (finalOutput, false, false, Id::charNewline.toRawUTF8());
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
    const auto dispatchKeys { manifestDoc.getTableRowKeys (Id::dispatch) };
    const auto constraintKeys { manifestDoc.getTableRowKeys (Id::constraints) };

    jam::Array<jam::MarkdownDocument> roots;

    {
        const auto tableFiles { dir.getChildFile (Id::tables.toString())
                                    .findChildFiles (
                                        juce::File::findFiles, false, Id::markdownWildcard) };
        const auto fileCount { tableFiles.size() };

        roots.resize (fileCount);

        {
            juce::ThreadPool parsePool;

            for (int i { 0 }; i < fileCount; ++i)
                parsePool.addJob ([&tableFiles, &roots, i]
                {
                    const auto& entry { tableFiles.getReference (i) };

                    roots[i] = jam::Markdown::parse (entry.loadFileAsString());
                    roots[i].root->add<juce::String> (Id::path, entry.getFullPathName());
                });

            while (parsePool.getNumJobs() > 0)
                juce::Thread::sleep (1);
        }
    }

    jam::HashMap<juce::String, std::pair<juce::String, juce::String>> lexicon;

    for (const auto& root : roots)
        for (const auto& canonical : root.getTableRowKeys (Id::lexicon))
        {
            const auto folded { canonical.toLowerCase() };

            if (lexicon.contains (folded))
                return juce::Result::fail (
                    getLocation (*root.root->get<juce::String> (Id::path),
                                getRowNumber (root, Id::lexicon.toString(), canonical),
                                Id::name.toString())
                    + Id::diagnosticSeparator + Id::failDuplicate + canonical + Id::failAlreadyDeclared
                    + getLocation (*root.root->get<juce::String> (Id::path),
                                  getRowNumber (root, Id::lexicon.toString(), lexicon.at (folded).first),
                                  Id::name.toString()));

            const auto* valueCell { root.getTableCell (Id::lexicon, Id::value, juce::Identifier (canonical)) };
            const auto rawValue { valueCell != nullptr and isLiteralCell (*valueCell)
                                      ? getLiteralValue (*valueCell)
                                      : root.getTableValue (Id::lexicon, Id::value, canonical) };

            lexicon.insert (
                { folded, { canonical, rawValue.isEmpty() ? canonical : rawValue } });
        }

    jam::HashMap<juce::String, const jam::MarkdownDocument*> rootIndex;

    for (const auto& root : roots)
        for (const auto& dispatchKey : dispatchKeys)
            if (root.root->getChildByID (juce::Identifier (dispatchKey)) != nullptr)
                rootIndex.insert ({ dispatchKey, &root });

    for (const auto& dispatchKey : dispatchKeys)
    {
        const auto sourceName { manifestDoc.getTableValue (Id::dispatch, Id::sourceTable, dispatchKey) };

        if (sourceName.isNotEmpty() and not rootIndex.contains (sourceName))
            for (const auto& root : roots)
                if (root.root->getChildByID (juce::Identifier (sourceName)) != nullptr)
                    rootIndex.insert ({ sourceName, &root });
    }

    const auto rootsResult { validateRoots (roots, manifestDoc, dispatchKeys, constraintKeys, dir) };

    if (not rootsResult.wasOk())
        return rootsResult;

    const auto perColumnResult { validatePerColumnConstraints (
        manifestDoc, constraintKeys, roots, dir, manifestFile.getFullPathName()) };

    if (not perColumnResult.wasOk())
        return perColumnResult;

    jam::HashMap<juce::String, juce::StringArray> slotResults;

    const auto dispatchResult { buildSlotResults (manifestDoc, manifestFile, dir, dispatchKeys, rootIndex, lexicon, slotResults) };

    if (not dispatchResult.wasOk())
        return dispatchResult;

    for (const auto& outputKey : outputKeys)
        if (outputFilter.isEmpty() or outputKey == outputFilter)
        {
            const auto result { processOutput (manifestDoc,
                                               manifestFile,
                                               dir,
                                               slotResults,
                                               outputKey,
                                               false,
                                               outputBanner) };

            if (not result.wasOk())
                return result;
        }

    for (const auto& outputKey : outputKeys)
        if (outputFilter.isEmpty() or outputKey == outputFilter)
        {
            const auto result { processOutput (manifestDoc,
                                               manifestFile,
                                               dir,
                                               slotResults,
                                               outputKey,
                                               true,
                                               outputBanner) };

            if (not result.wasOk())
                return result;
        }

    return (outputFilter.isEmpty() or outputKeys.contains (outputFilter))
               ? juce::Result::ok()
               : juce::Result::fail (manifestFile.getFullPathName() + Id::diagnosticSeparator
                                     + Id::failOutputMissing + Id::diagnosticSeparator
                                     + outputFilter);
}

/**______________________________END OF NAMESPACE______________________________*/
}// namespace Driver

/**______________________________END OF NAMESPACE______________________________*/
}// namespace cast
