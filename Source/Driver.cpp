#include "Driver.h"
#include "Cells.h"
#include "Constraints.h"
#include "Template.h"
#include "Transforms.h"
#include <jam_markdown/parser/jam_Markdown.h>

namespace cast
{

static juce::String transformedCell (const juce::String& rawValue,
                                     const juce::String& columnName,
                                     const jam::Array<TransformEntry>& transforms)
{
    for (const auto& entry : transforms)
    {
        if (entry.columnName == columnName)
        {
            juce::String result;
            Transforms::apply (entry.transformName, rawValue, result);
            return result;
        }
    }

    return rawValue;
}

static SubstitutionMap buildPerRowMap (const jam::Document& tableDoc, int rowIndex,
                                       const jam::Array<TransformEntry>& transforms)
{
    const auto& rows        { *tableDoc.get<jam::Array<jam::Document>> (Id::children) };
    const auto& headerCells { *rows.at (0).get<jam::Array<jam::Document>> (Id::children) };
    const auto& dataCells   { *rows.at (rowIndex).get<jam::Array<jam::Document>> (Id::children) };

    SubstitutionMap map;

    for (int c { 0 }; c < headerCells.size(); ++c)
    {
        const auto colName { *headerCells.at (c).get<juce::String> (Id::text) };
        const auto rawVal  { (dataCells.size() > c)
                                 ? *dataCells.at (c).get<juce::String> (Id::text)
                                 : juce::String {} };
        map.insert ({ colName, transformedCell (rawVal, colName, transforms) });
    }

    return map;
}

static bool rowMatchesDispatch (const jam::Document& tableDoc, int rowIndex,
                                const DispatchEntry& entry)
{
    const auto colIdx { findColumnIndex (tableDoc, entry.columnName) };

    if (colIdx < 0)
        return false;

    const auto cellValue { *(*tableDoc.get<jam::Array<jam::Document>> (Id::children))
                                .at (rowIndex)
                                .get<jam::Array<jam::Document>> (Id::children)
                                ->at (colIdx)
                                .get<juce::String> (Id::text) };

    return entry.matchValue.isEmpty() ? cellValue.isNotEmpty() : cellValue == entry.matchValue;
}

static juce::Result expandFragments (
    const OutputEntry& output,
    const Manifest& manifest,
    const jam::HashMap<juce::String, const jam::Document*>& tables,
    const juce::File& dir,
    jam::HashMap<juce::String, juce::StringArray>& slotResults)
{
    for (const auto& dispatch : manifest.dispatches)
    {
        const auto found { tables.find (dispatch.tableName) };

        if (found == tables.end())
            continue;

        const auto& [foundTableName, foundTableDocPtr] { *found };
        const auto& tableDoc     { *foundTableDocPtr };
        const auto& rows         { *tableDoc.get<jam::Array<jam::Document>> (Id::children) };
        const auto fragmentFile  { dir.getChildFile (dispatch.fragmentPath) };

        for (int r { 1 }; r < rows.size(); ++r)
        {
            if (not rowMatchesDispatch (tableDoc, r, dispatch))
                continue;

            const auto perRowMap { buildPerRowMap (tableDoc, r, manifest.transforms) };
            juce::String fragmentText;
            const auto er { TemplateEngine::expand (fragmentFile, perRowMap, fragmentText) };

            if (not er.wasOk()) return er;

            slotResults[dispatch.slotName].add (fragmentText);
        }
    }

    return juce::Result::ok();
}

static void loadInputTables (const OutputEntry& output,
                              const juce::File& dir,
                              jam::Array<jam::Document>& parsedDocs,
                              jam::HashMap<juce::String, const jam::Document*>& tables)
{
    parsedDocs.ensureStorageAllocated (output.inputTables.size());

    for (const auto& tablePath : output.inputTables)
        parsedDocs.add (jam::Markdown::parse (dir.getChildFile (tablePath.trim()).loadFileAsString()));

    for (int i { 0 }; i < parsedDocs.size(); ++i)
        extractSections (parsedDocs.at (i), tables);
}

static juce::Result renderOutput (const OutputEntry& output,
                                  const jam::HashMap<juce::String, juce::StringArray>& slotResults,
                                  const juce::File& dir)
{
    SubstitutionMap rootMap;

    for (const auto& [slot, fragments] : slotResults)
        rootMap.insert ({ slot, fragments });

    juce::String outputText;
    const auto rootFile      { dir.getChildFile (output.rootTemplatePath) };
    const auto templateResult { TemplateEngine::expand (rootFile, rootMap, outputText) };

    if (not templateResult.wasOk()) return templateResult;

    const auto outputFile { dir.getChildFile (output.outputPath) };
    const auto existing   { outputFile.loadFileAsString() };

    if (outputText != existing)
        outputFile.replaceWithText (outputText, false, false, "\n");

    return juce::Result::ok();
}

static juce::Result processOutput (const OutputEntry& output,
                                   const Manifest& manifest,
                                   const juce::File& dir)
{
    jam::Array<jam::Document> parsedDocs;
    jam::HashMap<juce::String, const jam::Document*> tables;
    loadInputTables (output, dir, parsedDocs, tables);

    const auto constraintResult { Constraints::validate (manifest.constraints, tables,
                                                          dir.getFullPathName()) };
    if (not constraintResult.wasOk()) return constraintResult;

    jam::HashMap<juce::String, juce::StringArray> slotResults;
    const auto fragmentResult { expandFragments (output, manifest, tables, dir, slotResults) };

    if (not fragmentResult.wasOk()) return fragmentResult;

    return renderOutput (output, slotResults, dir);
}

juce::Result Driver::run (const juce::File& manifestFile, const juce::String& outputFilter)
{
    Manifest manifest;
    const auto parseResult { Manifest::parse (manifestFile, manifest) };

    if (not parseResult.wasOk()) return parseResult;

    const auto dir { manifestFile.getParentDirectory() };
    configureDepends = std::move (manifest.configureDepends);

    if (outputFilter.isNotEmpty())
    {
        for (const auto& output : manifest.outputs)
        {
            if (output.outputPath == outputFilter)
                return processOutput (output, manifest, dir);
        }

        return juce::Result::fail (manifestFile.getFullPathName() + ": output not found: " + outputFilter);
    }

    for (const auto& output : manifest.outputs)
    {
        const auto result { processOutput (output, manifest, dir) };
        if (not result.wasOk()) return result;
    }

    return juce::Result::ok();
}

} // namespace cast
