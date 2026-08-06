#pragma once
#include <JuceHeader.h>
#include "Constraints.h"
#include "Transforms.h"

namespace cast
{
/*____________________________________________________________________________*/

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
static juce::String getHazardMessage (const jam::Document& cell) noexcept
{
    juce::String hazard;

    cell.applyFunctionRecursively (
        [&hazard] (const jam::Document& node) -> bool
        {
            if (node.contains (Id::tag) and *node.get<juce::Identifier> (Id::tag) == Id::a)
                hazard = Id::failHazardUri;
            else if (not node.contains (Id::tag) and node.contains (Id::text)
                     and node.get<juce::String> (Id::text)->containsAnyOf (Id::hazardChars))
                hazard = Id::failHazardAngleBrackets;

            return not(node.contains (Id::tag)
                       and *node.get<juce::Identifier> (Id::tag) == Id::code);
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
static juce::Result validateTableHazards (const jam::Document& root,
                                          const juce::Identifier& tableId,
                                          const juce::String& sourceFile)
{
    const auto rowKeys { root.getTableRowKeys (tableId) };
    const auto headers { root.getTableHeaders (tableId) };

    for (const auto& rowKey : rowKeys)
        for (const auto& header : headers)
        {
            const auto* cell { root.getTableCell (
                tableId, juce::Identifier (header), juce::Identifier (rowKey)) };

            if (cell != nullptr)
            {
                const auto hazard { getHazardMessage (*cell) };

                if (hazard.isNotEmpty())
                    return juce::Result::fail (
                        getLocation (sourceFile, rowKeys.indexOf (rowKey) + 1, header)
                        + Id::diagnosticSeparator + hazard);
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
validateManifest (const jam::Document& manifestDoc, const juce::File& manifestFile)
{
    const auto dir { manifestFile.getParentDirectory() };
    const auto path { manifestFile.getFullPathName() };

    const auto transformKeys { manifestDoc.getTableRowKeys (Id::transforms) };

    for (const auto& transformKey : transformKeys)
    {
        const auto transformName { manifestDoc.getTableValue (
            Id::transforms, Id::transform, transformKey) };
        const auto found { std::find_if (transforms.begin(),
                                         transforms.end(),
                                         [&transformName] (const auto& entry)
                                         {
                                             return entry.first == transformName;
                                         }) };

        if (found == transforms.end())
            return juce::Result::fail (
                getLocation (path, transformKeys.indexOf (transformKey) + 1, transformKey)
                + Id::diagnosticSeparator + Id::failUnknownTransform + Id::diagnosticSeparator
                + transformName);
    }

    const auto outputKeys { manifestDoc.getTableRowKeys (Id::outputs) };
    const auto dispatchKeys { manifestDoc.getTableRowKeys (Id::dispatch) };
    juce::StringArray referencedTemplates;

    for (const auto& outputKey : outputKeys)
    {
        const auto templatePath { manifestDoc.getTableValue (
            Id::outputs, Id::templatePath, outputKey) };

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
static juce::Result validateRoots (const jam::Array<jam::Document>& roots,
                                   const jam::Document& manifestDoc,
                                   const juce::StringArray& dispatchKeys,
                                   const juce::StringArray& constraintKeys,
                                   const juce::File& dir)
{
    const auto targetTables { getConstraintTargetTables (manifestDoc, constraintKeys) };

    for (const auto& root : roots)
    {
        const auto sourceFile { *root.get<juce::String> (Id::path) };
        const auto scannedTables { getScannedTables (root, dispatchKeys, targetTables) };

        for (const auto& scannedTable : scannedTables)
        {
            const auto hazardResult { validateTableHazards (
                root, juce::Identifier (scannedTable), sourceFile) };

            if (not hazardResult.wasOk())
                return hazardResult;
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
                        return constraintResult;
                }
        }
    }

    return juce::Result::ok();
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
static juce::Result validatePerColumnConstraints (const jam::Document& manifestDoc,
                                                  const juce::StringArray& constraintKeys,
                                                  const jam::Array<jam::Document>& roots,
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
            const jam::Document placeholderRoot;
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
