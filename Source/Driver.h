#pragma once
#include <JuceHeader.h>
#include "Constraints.h"
#include "Validation.h"
#include "Template.h"
#include "Transforms.h"

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

    bannerDoc.applyFunctionRecursively (
        [&rawBanner] (const jam::Document& element) -> bool
        {
            if (rawBanner.isEmpty() and element.contains (Id::type)
                and *element.get<int> (Id::type) == Id::BlockType::codeBlock)
                rawBanner = element.getAllSubText();

            return true;
        });

    if (rawBanner.isEmpty())
        return {};

    juce::StringArray lines;
    lines.addLines (rawBanner);

    for (auto& line : lines)
        line = Id::commentPrefix + line;

    return lines.joinIntoString (Id::charNewline);
}

/**
 * @brief Builds the hole substitution map for one relation row.
 *
 * Reads every column of @p tableName's row keyed by @p rowKey out of @p root,
 * running each value through its declared transform (manifest `## transforms`)
 * where the column name matches a transform row in @p manifestDoc; columns
 * with no matching transform pass through verbatim.
 *
 * @param root        The parsed relation document containing the table.
 * @param tableName   The relation (table) name.
 * @param rowKey      The row's column-0 key.
 * @param manifestDoc The parsed manifest, supplying the `## transforms` table.
 * @return A map from column header to its (possibly transformed) cell value.
 */
static SubstitutionMap buildPerRowMap (const jam::Document& root,
                                       const juce::String& tableName,
                                       const juce::String& rowKey,
                                       const jam::Document& manifestDoc)
{
    SubstitutionMap map;

    const auto tableId { juce::Identifier (tableName) };
    const auto transformedColumns { manifestDoc.getTableRowKeys (Id::transforms) };

    for (const auto& header : root.getTableHeaders (tableId))
    {
        const auto rawValue { root.getTableValue (tableId, juce::Identifier (header), rowKey) };
        const auto value {
            transformedColumns.contains (header)
                ? Transforms::getTransformed (
                      manifestDoc.getTableValue (Id::transforms, Id::transform, header), rawValue)
                : rawValue
        };

        map.insert ({ header, value });
    }

    return map;
}

/**
 * @brief Expands the root template for one output, filling its slot holes.
 *
 * Every entry of @p slotResults becomes an aggregate hole in the root
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
 * @brief Validates and generates one declared output row.
 *
 * Parses the output's declared relation tables, validates them (hazards,
 * per-row and per-column constraints), dispatches each relation's rows
 * through their matching fragment templates, and expands the root template.
 *
 * @p writeOutputs gates the write side of SPEC §8 atomicity: the driver
 * calls this function once per output with @p writeOutputs false to validate
 * every output before any bytes are written, then calls it again per output
 * with @p writeOutputs true to perform the identical work and write
 * write-if-different (SPEC §5).
 *
 * @param manifestDoc    The parsed manifest.
 * @param manifestFile   The manifest file (for error locations).
 * @param dir            The manifest's parent directory.
 * @param dispatchKeys   Row keys of the manifest's `## dispatch` table.
 * @param constraintKeys Row keys of the manifest's `## constraints` table.
 * @param outputKey      The `## outputs` row key identifying this output.
 * @param writeOutputs   When true, writes the output file if its bytes differ from disk.
 * @param outputBanner   The formatted banner prepended to generated output when non-empty.
 * @return juce::Result::ok() on success, or the first validation failure.
 */
static juce::Result processOutput (const jam::Document& manifestDoc,
                                   const juce::File& manifestFile,
                                   const juce::File& dir,
                                   const juce::StringArray& dispatchKeys,
                                   const juce::StringArray& constraintKeys,
                                   const juce::String& outputKey,
                                   bool writeOutputs,
                                   const juce::String& outputBanner)
{
    const auto rootTemplatePath { manifestDoc.getTableValue (
        Id::outputs, Id::templatePath, outputKey) };
    const auto tablePaths { manifestDoc.getTableValues (Id::outputs, Id::tables, outputKey) };

    jam::Array<jam::Document> roots;

    for (const auto& tablePath : tablePaths)
    {
        auto root { jam::Markdown::parse (dir.getChildFile (tablePath.trim()).loadFileAsString()) };
        root.add<juce::String> (Id::path, dir.getChildFile (tablePath.trim()).getFullPathName());
        roots.add (std::move (root));
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

    for (const auto& dispatchKey : dispatchKeys)
    {
        const auto found { std::find_if (roots.begin(),
                                         roots.end(),
                                         [&dispatchKey] (const auto& candidate)
                                         {
                                             return candidate.getChildByID (dispatchKey) != nullptr;
                                         }) };

        if (found != roots.end())
        {
            const auto tableId { juce::Identifier (dispatchKey) };
            const auto columnName { manifestDoc.getTableValue (
                Id::dispatch, Id::column, dispatchKey) };
            const auto matchValue { manifestDoc.getTableValue (
                Id::dispatch, Id::value, dispatchKey) };
            const auto fragmentFile { dir.getChildFile (
                manifestDoc.getTableValue (Id::dispatch, Id::templatePath, dispatchKey)) };
            const auto slotName { manifestDoc.getTableValue (Id::dispatch, Id::slot, dispatchKey) };

            for (const auto& rowKey : found->getTableRowKeys (tableId))
            {
                const auto cellValue { found->getTableValue (
                    tableId, juce::Identifier (columnName), rowKey) };
                const auto matches { matchValue.isEmpty() ? cellValue.isNotEmpty()
                                                          : cellValue == matchValue };

                if (matches)
                {
                    const auto perRowMap { buildPerRowMap (
                        *found, dispatchKey, rowKey, manifestDoc) };
                    const auto fragmentText { TemplateEngine::expand (fragmentFile, perRowMap) };
                    const auto orphanCheck { validateHoles (fragmentFile, fragmentText) };

                    if (not orphanCheck.wasOk())
                        return orphanCheck;

                    slotResults[slotName].add (fragmentText);
                }
            }
        }
    }

    const auto outputText { getOutput (dir, rootTemplatePath, slotResults) };
    const auto rootFile { dir.getChildFile (rootTemplatePath) };
    const auto orphanCheck { validateHoles (rootFile, outputText) };

    if (not orphanCheck.wasOk())
        return orphanCheck;

    const auto finalOutput { outputBanner.isNotEmpty() ? outputBanner + Id::charNewline + outputText
                                                       : outputText };

    if (writeOutputs)
    {
        const auto outputFile { dir.getChildFile (outputKey) };
        const auto existing { outputFile.loadFileAsString() };

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
    const auto outputKeys { manifestDoc.getTableRowKeys (Id::outputs) };
    const auto dispatchKeys { manifestDoc.getTableRowKeys (Id::dispatch) };
    const auto constraintKeys { manifestDoc.getTableRowKeys (Id::constraints) };

    for (const auto& outputKey : outputKeys)
        if (outputFilter.isEmpty() or outputKey == outputFilter)
        {
            const auto result { processOutput (manifestDoc,
                                               manifestFile,
                                               dir,
                                               dispatchKeys,
                                               constraintKeys,
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
                                               dispatchKeys,
                                               constraintKeys,
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
