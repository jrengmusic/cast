#include "Manifest.h"
#include "Cells.h"
#include "Transforms.h"
#include <jam_markdown/parser/jam_Markdown.h>
#include <jam_core/function_map/jam_Function.h>

namespace cast
{

static juce::String location (const juce::String& path, int row, int col)
{
    return path + ":" + juce::String (row) + ":" + juce::String (col);
}

static juce::String getCellText (const jam::Document& row, int colIdx) noexcept
{
    const auto& cells { *row.get<jam::Array<jam::Document>> (Id::children) };
    return (cells.size() > colIdx)
               ? *cells.at (colIdx).get<juce::String> (Id::text)
               : juce::String {};
}

static juce::Result checkCellHazard (const jam::Document& row, int colIdx,
                                     const juce::String& path, int rowNum)
{
    const auto& cells { *row.get<jam::Array<jam::Document>> (Id::children) };

    if (colIdx < cells.size())
    {
        const auto hazard { getCellHazard (cells.at (colIdx)) };

        if (hazard.isNotEmpty())
            return juce::Result::fail (location (path, rowNum, colIdx + 1) + ": " + hazard);
    }

    return juce::Result::ok();
}

static juce::Result processOutputs (const jam::Document& tableDoc,
                                    const juce::String& path, Manifest& result)
{
    const auto& rows { *tableDoc.get<jam::Array<jam::Document>> (Id::children) };

    for (int r { 1 }; r < rows.size(); ++r)
    {
        const auto& row { rows.at (r) };

        for (int c { 0 }; c < 3; ++c)
        {
            const auto hr { checkCellHazard (row, c, path, r + 1) };
            if (not hr.wasOk()) return hr;
        }

        OutputEntry entry {};
        entry.outputPath       = getCellText (row, 0);
        entry.rootTemplatePath = getCellText (row, 1);
        entry.inputTables      = juce::StringArray::fromTokens (getCellText (row, 2), ",", "");

        result.outputs.add (std::move (entry));
    }

    return juce::Result::ok();
}

static juce::Result processDispatch (const jam::Document& tableDoc,
                                     const juce::String& path, Manifest& result)
{
    const auto& rows { *tableDoc.get<jam::Array<jam::Document>> (Id::children) };

    for (int r { 1 }; r < rows.size(); ++r)
    {
        const auto& row { rows.at (r) };

        for (int c { 0 }; c < 5; ++c)
        {
            const auto hr { checkCellHazard (row, c, path, r + 1) };
            if (not hr.wasOk()) return hr;
        }

        DispatchEntry entry {};
        entry.tableName    = getCellText (row, 0);
        entry.columnName   = getCellText (row, 1);
        entry.matchValue   = getCellText (row, 2);
        entry.fragmentPath = getCellText (row, 3);
        entry.slotName     = getCellText (row, 4);

        result.dispatches.add (std::move (entry));
    }

    return juce::Result::ok();
}

static juce::Result processTransforms (const jam::Document& tableDoc,
                                       const juce::String& path, Manifest& result)
{
    const auto& rows { *tableDoc.get<jam::Array<jam::Document>> (Id::children) };

    for (int r { 1 }; r < rows.size(); ++r)
    {
        const auto& row { rows.at (r) };

        for (int c { 0 }; c < 2; ++c)
        {
            const auto hr { checkCellHazard (row, c, path, r + 1) };
            if (not hr.wasOk()) return hr;
        }

        TransformEntry entry {};
        entry.columnName    = getCellText (row, 0);
        entry.transformName = getCellText (row, 1);

        juce::String validationResult;
        const auto tr { Transforms::apply (entry.transformName, {}, validationResult) };

        if (not tr.wasOk())
            return juce::Result::fail (location (path, r + 1, 2) + ": " + tr.getErrorMessage());

        result.transforms.add (std::move (entry));
    }

    return juce::Result::ok();
}

static juce::Result processConstraints (const jam::Document& tableDoc,
                                        const juce::String& path, Manifest& result)
{
    const auto& rows { *tableDoc.get<jam::Array<jam::Document>> (Id::children) };

    for (int r { 1 }; r < rows.size(); ++r)
    {
        const auto& row { rows.at (r) };

        for (int c { 0 }; c < 2; ++c)
        {
            const auto hr { checkCellHazard (row, c, path, r + 1) };
            if (not hr.wasOk()) return hr;
        }

        ConstraintEntry entry {};
        entry.columnName = getCellText (row, 0);
        entry.predicate  = getCellText (row, 1);

        result.constraints.add (std::move (entry));
    }

    return juce::Result::ok();
}

static jam::Function::Map<juce::String, juce::Result> buildSectionDispatch()
{
    jam::Function::Map<juce::String, juce::Result> map;
    map.add<const jam::Document&, const juce::String&, Manifest&> ("outputs",     &processOutputs);
    map.add<const jam::Document&, const juce::String&, Manifest&> ("dispatch",    &processDispatch);
    map.add<const jam::Document&, const juce::String&, Manifest&> ("transforms",  &processTransforms);
    map.add<const jam::Document&, const juce::String&, Manifest&> ("constraints", &processConstraints);
    return map;
}

static juce::Result processSection (const jam::Document& tableDoc,
                                    const juce::String& sectionName,
                                    const juce::String& path, Manifest& result)
{
    static jam::Function::Map<juce::String, juce::Result> dispatch { buildSectionDispatch() };

    if (dispatch.contains (sectionName))
    {
        return dispatch.get (sectionName, tableDoc, path, result);
    }

    return juce::Result::ok();
}

static void buildConfigureDepends (const juce::File& castFile, Manifest& result)
{
    const auto dir { castFile.getParentDirectory() };
    result.configureDepends.add (castFile.getFullPathName());

    for (const auto& output : result.outputs)
    {
        result.configureDepends.add (dir.getChildFile (output.rootTemplatePath).getFullPathName());

        for (const auto& table : output.inputTables)
            result.configureDepends.add (dir.getChildFile (table.trim()).getFullPathName());
    }

    for (const auto& dispatch : result.dispatches)
        result.configureDepends.add (dir.getChildFile (dispatch.fragmentPath).getFullPathName());
}

static juce::Result validateManifest (const juce::File& castFile, const Manifest& result)
{
    const auto dir  { castFile.getParentDirectory() };
    const auto path { castFile.getFullPathName() };

    for (const auto& output : result.outputs)
    {
        if (not dir.getChildFile (output.rootTemplatePath).existsAsFile())
            return juce::Result::fail (path + ": template not found: " + output.rootTemplatePath);
    }

    for (const auto& dispatch : result.dispatches)
    {
        if (not dir.getChildFile (dispatch.fragmentPath).existsAsFile())
            return juce::Result::fail (path + ": fragment not found: " + dispatch.fragmentPath);

        const auto tableReferenced { [&]
        {
            for (const auto& output : result.outputs)
            {
                jam::Array<jam::Document> parsedDocs;
                jam::HashMap<juce::String, const jam::Document*> inputSections;

                for (const auto& inputTablePath : output.inputTables)
                {
                    const auto inputFile { dir.getChildFile (inputTablePath.trim()) };
                    parsedDocs.add (jam::Markdown::parse (inputFile.loadFileAsString()));
                }

                for (int i { 0 }; i < parsedDocs.size(); ++i)
                    extractSections (parsedDocs.at (i), inputSections);

                if (inputSections.find (dispatch.tableName) != inputSections.end())
                    return true;
            }

            return false;
        }() };

        if (not tableReferenced)
            return juce::Result::fail (path + ": unmapped fragment: " + dispatch.fragmentPath
                                       + " (table not referenced: " + dispatch.tableName + ")");
    }

    return juce::Result::ok();
}

juce::Result Manifest::parse (const juce::File& castFile, Manifest& result)
{
    const auto text { castFile.loadFileAsString() };
    const auto doc  { jam::Markdown::parse (text) };
    const auto path { castFile.getFullPathName() };

    jam::HashMap<juce::String, const jam::Document*> sections;
    extractSections (doc, sections);

    for (const auto& [sectionName, sectionDocPtr] : sections)
    {
        const auto sr { processSection (*sectionDocPtr, sectionName, path, result) };
        if (not sr.wasOk()) return sr;
    }

    buildConfigureDepends (castFile, result);

    const auto validationResult { validateManifest (castFile, result) };
    if (not validationResult.wasOk()) return validationResult;

    return juce::Result::ok();
}

} // namespace cast
