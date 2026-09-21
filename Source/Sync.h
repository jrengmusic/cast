#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Jobs.h"
#include "Model.h"
#include "Transforms.h"
#include "Validator.h"

/**
 * @struct Sync
 * @brief Mirrors one framework's kernel onto another's, per SPEC §2.2 --
 *        parses each root's own @c user-modules-info.md, validates the
 *        sync-specific gates through Validator, then walks, transforms,
 *        writes, and mirror-deletes the source's own kernel scopes into
 *        the target root.
 *
 * Sync owns no state of its own -- every member is static, matching
 * Validator and Transforms. run() is the one public entry point; every
 * other member is a step of its own pipeline, threaded by value through
 * the parsed Model of each root and the ordered identity-pair sequence
 * getIdentityRows() and the two composed namespace pairs together
 * describe.
 */
struct Sync
{
    /**
     * @brief Runs one sync between @p sourceRoot and @p targetRoot --
     *        the roots-distinct, info-file-present, composed-key, and
     *        unique-pair gates, then runGates() for the rest.
     *
     * @param sourceRoot The framework root sync reads from.
     * @param targetRoot The framework root sync writes and mirror-deletes
     *                   into.
     * @returns juce::Result::ok() when the sync run succeeds, or the
     *          first failing gate's or step's result.
     */
    static juce::Result run (const juce::File& sourceRoot, const juce::File& targetRoot)
    {
        const auto sourceInfoFile { getInfoFile (sourceRoot) };
        const auto targetInfoFile { getInfoFile (targetRoot) };

        if (const auto result { Validator::isRootsDistinct (sourceRoot, targetRoot) }; not result.wasOk())
            return result;

        if (const auto result { Validator::isInfoPresent (sourceInfoFile) }; not result.wasOk())
            return result;

        if (const auto result { Validator::isInfoPresent (targetInfoFile) }; not result.wasOk())
            return result;

        const auto sourceInfo { Model::parse (sourceInfoFile) };
        const auto targetInfo { Model::parse (targetInfoFile) };

        if (const auto result {
                Validator::isComposedComplete (sourceInfoFile, targetInfoFile, *sourceInfo, *targetInfo) };
            not result.wasOk())
            return result;

        if (const auto result { Validator::isUniquePair (sourceInfoFile, *sourceInfo, *targetInfo) };
            not result.wasOk())
            return result;

        return runGates (sourceRoot, targetRoot, *sourceInfo, *targetInfo);
    }

private:
    /**
     * @brief Resolves @p root's own @c user-modules-info.md file.
     *
     * @param root The sync root the info file is resolved against.
     * @returns @p root's own info file, whether or not it exists.
     */
    static juce::File getInfoFile (const juce::File& root)
    {
        return root.getChildFile (files::userModulesInfo);
    }

    /**
     * @brief Normalizes @p relativePath's own directory separators to
     *        @c / (SPEC §2.2: "root-relative paths use / on every host").
     *
     * @param relativePath The root-relative path to normalize.
     * @returns @p relativePath with every backslash replaced by a slash.
     */
    static juce::String getNormalizedPath (const juce::String& relativePath)
    {
        return relativePath.replaceCharacter (Chars::backslash, Chars::slash);
    }

    /**
     * @brief Resolves both roots' own @c filePrefix and module-name sets,
     *        checks isKernelDeclared(), then runs runWalk().
     *
     * @param sourceRoot The framework root sync reads from.
     * @param targetRoot The framework root sync writes and mirror-deletes
     *                   into.
     * @param sourceInfo @p sourceRoot's own parsed info file.
     * @param targetInfo @p targetRoot's own parsed info file.
     * @returns juce::Result::ok() when every gate and step succeeds, or
     *          the first failing one's result.
     */
    static juce::Result runGates (const juce::File& sourceRoot, const juce::File& targetRoot,
        const Model& sourceInfo, const Model& targetInfo)
    {
        static const auto filePrefixKeyText { Id::filePrefix.toString() };
        const auto sourceFilePrefix { getIdentityValue (sourceInfo, filePrefixKeyText) };
        const auto targetFilePrefix { getIdentityValue (targetInfo, filePrefixKeyText) };

        const auto sourceDeclaredNames { getDeclaredModuleNames (sourceInfo) };
        const auto targetDeclaredNames { getDeclaredModuleNames (targetInfo) };
        const auto sourceKernelNames { getKernelModuleNames (sourceInfo) };
        const auto targetKernelNames { getKernelModuleNames (targetInfo) };

        if (const auto result { isKernelDeclared (sourceRoot, targetRoot, sourceFilePrefix, targetFilePrefix,
                sourceDeclaredNames, targetDeclaredNames, sourceKernelNames, targetKernelNames) };
            not result.wasOk())
            return result;

        return runWalk (sourceRoot, targetRoot, sourceInfo, targetInfo, sourceFilePrefix, targetFilePrefix,
            sourceKernelNames, targetKernelNames);
    }

    /**
     * @brief Checks the kernel-presence, prefix-declared, and
     *        correspondence gates against both roots, through Validator.
     *
     * @param sourceRoot          The framework root sync reads from.
     * @param targetRoot          The framework root sync writes into.
     * @param sourceFilePrefix    @p sourceRoot's own identity @c filePrefix
     *                            value.
     * @param targetFilePrefix    @p targetRoot's own identity @c filePrefix
     *                            value.
     * @param sourceDeclaredNames Every module row's own name declared in
     *                            @p sourceRoot's own info file.
     * @param targetDeclaredNames Every module row's own name declared in
     *                            @p targetRoot's own info file.
     * @param sourceKernelNames   @p sourceRoot's own @c kernel-classed
     *                            module names.
     * @param targetKernelNames   @p targetRoot's own @c kernel-classed
     *                            module names.
     * @returns juce::Result::ok() when every gate holds at both roots, or
     *          the first failing gate's result.
     */
    static juce::Result isKernelDeclared (const juce::File& sourceRoot, const juce::File& targetRoot,
        const juce::String& sourceFilePrefix, const juce::String& targetFilePrefix,
        const jam::HashSet<juce::String>& sourceDeclaredNames, const jam::HashSet<juce::String>& targetDeclaredNames,
        const jam::HashSet<juce::String>& sourceKernelNames, const jam::HashSet<juce::String>& targetKernelNames)
    {
        if (const auto result { Validator::isKernelPresent (sourceRoot, sourceKernelNames) }; not result.wasOk())
            return result;

        if (const auto result { Validator::isKernelPresent (targetRoot, targetKernelNames) }; not result.wasOk())
            return result;

        if (const auto result {
                Validator::isPrefixDeclared (sourceRoot, sourceFilePrefix, sourceDeclaredNames) };
            not result.wasOk())
            return result;

        if (const auto result {
                Validator::isPrefixDeclared (targetRoot, targetFilePrefix, targetDeclaredNames) };
            not result.wasOk())
            return result;

        return Validator::isCorresponding (
            sourceRoot, targetRoot, sourceKernelNames, targetKernelNames, sourceFilePrefix, targetFilePrefix);
    }

    /**
     * @brief Builds the ordered replacement pairs and the source's own
     *        walked file list, runs runContamination() over them, then,
     *        once clean, runTransform().
     *
     * @param sourceRoot        The framework root sync reads from.
     * @param targetRoot        The framework root sync writes into.
     * @param sourceInfo        @p sourceRoot's own parsed info file.
     * @param targetInfo        @p targetRoot's own parsed info file.
     * @param sourceFilePrefix  @p sourceRoot's own identity @c filePrefix
     *                          value.
     * @param targetFilePrefix  @p targetRoot's own identity @c filePrefix
     *                          value.
     * @param sourceKernelNames @p sourceRoot's own @c kernel-classed
     *                          module names.
     * @param targetKernelNames @p targetRoot's own @c kernel-classed
     *                          module names.
     * @returns juce::Result::ok() when the walk, contamination check, and
     *          transform all succeed, or the first failure's result.
     */
    static juce::Result runWalk (const juce::File& sourceRoot, const juce::File& targetRoot,
        const Model& sourceInfo, const Model& targetInfo, const juce::String& sourceFilePrefix,
        const juce::String& targetFilePrefix, const jam::HashSet<juce::String>& sourceKernelNames,
        const jam::HashSet<juce::String>& targetKernelNames)
    {
        const auto identityRows { getIdentityRows (sourceInfo, targetInfo) };
        const auto namespaceValues { getIdentityValues (sourceInfo, targetInfo, Id::tokenNamespace.toString()) };
        const auto namespaceSpacePair { getNamespaceSpacePair (namespaceValues.first, namespaceValues.second) };
        const auto namespaceColonPair { getNamespaceColonPair (namespaceValues.first, namespaceValues.second) };

        const auto sourceIgnorePatterns { getIgnorePatterns (sourceInfo) };
        const auto sourceFiles { getSyncFiles (sourceRoot, sourceKernelNames, sourceIgnorePatterns) };

        if (const auto result { runContamination (sourceFiles, sourceRoot, sourceInfo, targetInfo, identityRows,
                namespaceSpacePair, namespaceColonPair) };
            not result.wasOk())
            return result;

        return runTransform (sourceFiles, sourceRoot, targetRoot, sourceFilePrefix, targetFilePrefix, sourceInfo,
            targetInfo, identityRows, namespaceSpacePair, namespaceColonPair, targetKernelNames);
    }

    /**
     * @brief Runs getContaminationCheck() over every one of @p sourceFiles,
     *        one juce::ThreadPool job per file, reading each file solely
     *        for the check and discarding its content -- the first pass
     *        of Sync's own two-pass walk (SPEC §2.2's contamination fatal
     *        must stop the run before any write, never after).
     *
     * @param sourceFiles         The source's own walked, ignore-filtered
     *                            files.
     * @param sourceRoot          The framework root @p sourceFiles are
     *                            resolved under.
     * @param sourceInfo          @p sourceRoot's own parsed info file.
     * @param targetInfo          The target root's own parsed info file.
     * @param identityRows        The sorted, namespace-excluded identity
     *                            rows getIdentityRows() returns.
     * @param namespaceSpacePair  The @c "namespace <source>" to
     *                            @c "namespace <target>" pair.
     * @param namespaceColonPair  The @c "<source>::" to @c "<target>::"
     *                            pair.
     * @returns juce::Result::ok() when every file passes the contamination
     *          check, or the first failing file's diagnostic.
     */
    static juce::Result runContamination (const jam::Array<juce::File>& sourceFiles, const juce::File& sourceRoot,
        const Model& sourceInfo, const Model& targetInfo, const jam::Array<const Model::Element*>& identityRows,
        const std::pair<juce::String, juce::String>& namespaceSpacePair,
        const std::pair<juce::String, juce::String>& namespaceColonPair)
    {
        jam::Array<juce::String> checkResults;
        checkResults.resize (sourceFiles.size());

        Jobs::run (sourceFiles.size(),
            [&sourceFiles, &sourceRoot, &sourceInfo, &targetInfo, &identityRows, &namespaceSpacePair,
                &namespaceColonPair, &checkResults] (int index)
            {
                checkResults.at (index) = getContaminationCheck (sourceFiles.at (index), sourceRoot, sourceInfo,
                    targetInfo, identityRows, namespaceSpacePair, namespaceColonPair);
            });

        for (const auto& error : checkResults)
            if (error.isNotEmpty())
                return juce::Result::fail (error);

        return juce::Result::ok();
    }

    /**
     * @brief Runs getSyncOutcome() over every one of @p sourceFiles, one
     *        juce::ThreadPool job per file -- the second pass of Sync's
     *        own two-pass walk, run only once runContamination() has
     *        cleared every file.
     *
     * @param sourceFiles        The source's own walked, ignore-filtered
     *                           files.
     * @param sourceRoot         The framework root @p sourceFiles are
     *                           resolved under.
     * @param targetRoot         The framework root each file is written
     *                           under.
     * @param sourceFilePrefix   The source's own identity @c filePrefix
     *                           value.
     * @param targetFilePrefix   The target's own identity @c filePrefix
     *                           value.
     * @param sourceInfo         The source's own parsed info file.
     * @param targetInfo         The target's own parsed info file.
     * @param identityRows       The sorted, namespace-excluded identity
     *                           rows getIdentityRows() returns.
     * @param namespaceSpacePair The @c "namespace <source>" to
     *                           @c "namespace <target>" pair.
     * @param namespaceColonPair The @c "<source>::" to @c "<target>::"
     *                           pair.
     * @returns Each file's own getSyncOutcome() result, in @p sourceFiles'
     *          own order.
     */
    static jam::Array<std::pair<juce::String, juce::String>> getSyncResults (const jam::Array<juce::File>& sourceFiles,
        const juce::File& sourceRoot, const juce::File& targetRoot, const juce::String& sourceFilePrefix,
        const juce::String& targetFilePrefix, const Model& sourceInfo, const Model& targetInfo,
        const jam::Array<const Model::Element*>& identityRows, const std::pair<juce::String, juce::String>& namespaceSpacePair,
        const std::pair<juce::String, juce::String>& namespaceColonPair)
    {
        jam::Array<std::pair<juce::String, juce::String>> syncResults;
        syncResults.resize (sourceFiles.size());

        Jobs::run (sourceFiles.size(),
            [&sourceFiles, &sourceRoot, &targetRoot, &sourceFilePrefix, &targetFilePrefix, &sourceInfo, &targetInfo,
                &identityRows, &namespaceSpacePair, &namespaceColonPair, &syncResults] (int index)
            {
                syncResults.at (index) = getSyncOutcome (sourceFiles.at (index), sourceRoot, targetRoot,
                    sourceFilePrefix, targetFilePrefix, sourceInfo, targetInfo, identityRows, namespaceSpacePair,
                    namespaceColonPair);
            });

        return syncResults;
    }

    /**
     * @brief Runs getSyncResults(), collects every written path, and, when
     *        every file wrote or was already current, runs runReport().
     *
     * @param sourceFiles        The source's own walked, ignore-filtered
     *                           files.
     * @param sourceRoot         The framework root @p sourceFiles are
     *                           resolved under.
     * @param targetRoot         The framework root each file is written
     *                           under.
     * @param sourceFilePrefix   The source's own identity @c filePrefix
     *                           value.
     * @param targetFilePrefix   The target's own identity @c filePrefix
     *                           value.
     * @param sourceInfo         The source's own parsed info file.
     * @param targetInfo         The target's own parsed info file.
     * @param identityRows       The sorted, namespace-excluded identity
     *                           rows getIdentityRows() returns.
     * @param namespaceSpacePair The @c "namespace <source>" to
     *                           @c "namespace <target>" pair.
     * @param namespaceColonPair The @c "<source>::" to @c "<target>::"
     *                           pair.
     * @param targetKernelNames  The target's own @c kernel-classed module
     *                           names, passed through to runReport().
     * @returns juce::Result::ok() when every file writes successfully and
     *          runReport() succeeds, or the first failure's result.
     */
    static juce::Result runTransform (const jam::Array<juce::File>& sourceFiles, const juce::File& sourceRoot,
        const juce::File& targetRoot, const juce::String& sourceFilePrefix, const juce::String& targetFilePrefix,
        const Model& sourceInfo, const Model& targetInfo, const jam::Array<const Model::Element*>& identityRows,
        const std::pair<juce::String, juce::String>& namespaceSpacePair,
        const std::pair<juce::String, juce::String>& namespaceColonPair,
        const jam::HashSet<juce::String>& targetKernelNames)
    {
        const auto syncResults { getSyncResults (sourceFiles, sourceRoot, targetRoot, sourceFilePrefix,
            targetFilePrefix, sourceInfo, targetInfo, identityRows, namespaceSpacePair, namespaceColonPair) };

        jam::Array<juce::String> writtenPaths;

        for (const auto& [writtenPath, error] : syncResults)
        {
            if (error.isNotEmpty())
                return juce::Result::fail (error);

            if (writtenPath.isNotEmpty())
                writtenPaths.add (writtenPath);
        }

        return runReport (targetRoot, targetInfo, targetKernelNames, sourceFiles, sourceRoot, sourceFilePrefix,
            targetFilePrefix, writtenPaths);
    }

    /**
     * @brief Mirror-deletes every target kernel file with no transformed
     *        source counterpart, through getMirrorDeletions(), then prints
     *        the written and deleted paths through printReport().
     *
     * @param targetRoot        The framework root mirror-delete walks.
     * @param targetInfo        @p targetRoot's own parsed info file.
     * @param targetKernelNames @p targetRoot's own @c kernel-classed
     *                          module names.
     * @param sourceFiles       The source's own walked, ignore-filtered
     *                          files, read for their own transformed
     *                          target-relative paths.
     * @param sourceRoot        The framework root @p sourceFiles are
     *                          resolved under.
     * @param sourceFilePrefix  The source's own identity @c filePrefix
     *                          value.
     * @param targetFilePrefix  The target's own identity @c filePrefix
     *                          value.
     * @param writtenPaths      Every file runTransform() actually wrote,
     *                          reported verbatim.
     * @returns juce::Result::ok() when mirror-delete succeeds, or the
     *          first delete failure's result.
     */
    static juce::Result runReport (const juce::File& targetRoot, const Model& targetInfo,
        const jam::HashSet<juce::String>& targetKernelNames, const jam::Array<juce::File>& sourceFiles,
        const juce::File& sourceRoot, const juce::String& sourceFilePrefix, const juce::String& targetFilePrefix,
        const jam::Array<juce::String>& writtenPaths)
    {
        jam::HashSet<juce::String> expectedTargetPaths;

        for (const auto& sourceFile : sourceFiles)
            expectedTargetPaths.insert (getTransformedPath (
                getNormalizedPath (sourceFile.getRelativePathFrom (sourceRoot)), sourceFilePrefix, targetFilePrefix));

        const auto targetIgnorePatterns { getIgnorePatterns (targetInfo) };
        auto [deleteResult, deletedPaths] {
            getMirrorDeletions (targetRoot, targetKernelNames, targetIgnorePatterns, expectedTargetPaths) };

        if (not deleteResult.wasOk())
            return deleteResult;

        printReport (writtenPaths, deletedPaths);

        return juce::Result::ok();
    }

    /**
     * @brief Returns @p info's own identity row keyed @p key's own
     *        @c value cell.
     *
     * @param info The parsed info file whose identity table is read.
     * @param key  The identity row's own key.
     * @returns @p key's own resolved value in @p info.
     */
    static juce::String getIdentityValue (const Model& info, const juce::String& key)
    {
        return info.getValue (*info.getTableRow (Id::identity, juce::Identifier (key)), Id::value);
    }

    /**
     * @brief Returns @p key's own value in each of @p sourceInfo and
     *        @p targetInfo.
     *
     * @param sourceInfo The source's own parsed info file.
     * @param targetInfo The target's own parsed info file.
     * @param key        The identity row's own key.
     * @returns @p key's own source value paired with its own target
     *          value.
     */
    static std::pair<juce::String, juce::String>
    getIdentityValues (const Model& sourceInfo, const Model& targetInfo, const juce::String& key)
    {
        return { getIdentityValue (sourceInfo, key), getIdentityValue (targetInfo, key) };
    }

    /**
     * @brief Builds the @c "namespace <source>" to @c "namespace <target>"
     *        composed pair (SPEC §2.2).
     *
     * @param sourceValue The source's own @c namespace identity value.
     * @param targetValue The target's own @c namespace identity value.
     * @returns The composed source text paired with the composed target
     *          text.
     */
    static std::pair<juce::String, juce::String>
    getNamespaceSpacePair (const juce::String& sourceValue, const juce::String& targetValue)
    {
        static const auto namespaceSpaceText {
            Id::tokenNamespace.toString() + juce::String::charToString (Chars::space) };

        return { namespaceSpaceText + sourceValue, namespaceSpaceText + targetValue };
    }

    /**
     * @brief Builds the @c "<source>::" to @c "<target>::" composed pair
     *        (SPEC §2.2).
     *
     * @param sourceValue The source's own @c namespace identity value.
     * @param targetValue The target's own @c namespace identity value.
     * @returns The composed source text paired with the composed target
     *          text.
     */
    static std::pair<juce::String, juce::String>
    getNamespaceColonPair (const juce::String& sourceValue, const juce::String& targetValue)
    {
        return { sourceValue + Id::doubleColon.toString(), targetValue + Id::doubleColon.toString() };
    }

    /**
     * @brief Returns every identity row present in both @p sourceInfo and
     *        @p targetInfo, the @c namespace row excluded (it contributes
     *        no plain pair, SPEC §2.2), sorted longest-source-first, ties
     *        broken by source text descending.
     *
     * The parsed @p sourceInfo Model is the store (SPEC §11.1) -- this
     * returns pointers into its own identity table, never a copy of the
     * cell values themselves; getRowPair() and isRowWordBoundary() read
     * those cells at application time.
     *
     * @param sourceInfo The source's own parsed info file.
     * @param targetInfo The target's own parsed info file, read only to
     *                   test whether each source key resolves there too.
     * @returns @p sourceInfo's own qualifying identity rows, sorted for
     *          application.
     */
    static jam::Array<const Model::Element*> getIdentityRows (const Model& sourceInfo, const Model& targetInfo)
    {
        static const auto namespaceKeyText { Id::tokenNamespace.toString() };

        jam::Array<const Model::Element*> rows;

        for (auto* sourceRow : sourceInfo.getTableRows (Id::identity))
        {
            const auto key { sourceInfo.getValue (*sourceRow, Id::key) };

            if (key.compare (namespaceKeyText) != 0
                and targetInfo.getTableRow (Id::identity, juce::Identifier (key)) != nullptr)
                rows.add (sourceRow);
        }

        std::sort (rows.begin(), rows.end(),
            [&sourceInfo] (const Model::Element* first, const Model::Element* second)
            {
                const auto firstValue { sourceInfo.getValue (*first, Id::value) };
                const auto secondValue { sourceInfo.getValue (*second, Id::value) };

                return firstValue.length() != secondValue.length()
                           ? firstValue.length() > secondValue.length()
                           : firstValue.compare (secondValue) > 0;
            });

        return rows;
    }

    /**
     * @brief Resolves @p row's own source value and its matching target
     *        row's own value, read fresh from @p sourceInfo and
     *        @p targetInfo.
     *
     * @param sourceInfo The source's own parsed info file @p row belongs
     *                   to.
     * @param targetInfo The target's own parsed info file, searched for
     *                   @p row's own key.
     * @param row        The source identity row to resolve.
     * @returns @p row's own source value paired with its own target
     *          value.
     */
    static std::pair<juce::String, juce::String>
    getRowPair (const Model& sourceInfo, const Model& targetInfo, const Model::Element* row)
    {
        const auto key { sourceInfo.getValue (*row, Id::key) };
        const auto sourceValue { sourceInfo.getValue (*row, Id::value) };
        const auto targetValue {
            targetInfo.getValue (*targetInfo.getTableRow (Id::identity, juce::Identifier (key)), Id::value) };

        return { sourceValue, targetValue };
    }

    /**
     * @brief Answers whether @p row's own @c boundary cell is @c word --
     *        the row matches whole words only (SPEC §2.2).
     *
     * @param sourceInfo The parsed info file @p row belongs to.
     * @param row        The identity row to test.
     * @returns @c true when @p row's own boundary cell reads @c word.
     */
    static bool isRowWordBoundary (const Model& sourceInfo, const Model::Element* row)
    {
        static const auto wordBoundaryText { Id::word.toString() };

        return sourceInfo.getValue (*row, Id::syncBoundary).compare (wordBoundaryText) == 0;
    }

    /**
     * @brief Invokes @p visit with @p row's own resolved pair and its own
     *        word-boundary flag.
     *
     * @tparam Visitor Callable invoked as
     *                 @c visit(sourceValue,targetValue,isWordBoundary).
     * @param sourceInfo The source's own parsed info file @p row belongs
     *                   to.
     * @param targetInfo The target's own parsed info file.
     * @param row        The identity row to visit.
     * @param visit      The callable invoked with @p row's own resolved
     *                   pair.
     */
    template <typename Visitor>
    static void visitRow (const Model& sourceInfo, const Model& targetInfo, const Model::Element* row, Visitor&& visit)
    {
        const auto [sourceValue, targetValue] { getRowPair (sourceInfo, targetInfo, row) };
        visit (sourceValue, targetValue, isRowWordBoundary (sourceInfo, row));
    }

    /**
     * @brief Returns @p source's own length when @p pending, or a
     *        sentinel below every real length otherwise -- the length
     *        forEachIdentityPair()'s own merge compares to decide which
     *        pending pair applies next.
     *
     * @param pending Whether the pair @p source belongs to is still
     *                unapplied.
     * @param source  The pair's own source text.
     * @returns @p source's own length, or @c -1 when @p pending is
     *          @c false.
     */
    static int getPendingLength (bool pending, const juce::String& source) noexcept
    {
        return pending ? source.length() : -1;
    }

    /**
     * @brief Returns @p identityRows' own row at @p rowIndex's own source
     *        length, or a sentinel below every real length once
     *        @p rowIndex runs past the array.
     *
     * @param sourceInfo   The parsed info file @p identityRows belong to.
     * @param identityRows The sorted identity rows getIdentityRows()
     *                     returns.
     * @param rowIndex     The row's own position in @p identityRows.
     * @returns @p rowIndex's own row's source length, or @c -1 when
     *          @p rowIndex is out of range.
     */
    static int getRowLength (const Model& sourceInfo, const jam::Array<const Model::Element*>& identityRows, int rowIndex)
    {
        return rowIndex < identityRows.size() ? sourceInfo.getValue (*identityRows.at (rowIndex), Id::value).length()
                                               : -1;
    }

    /**
     * @brief Walks @p identityRows and the two composed namespace pairs
     *        as one merged, longest-source-first sequence, invoking
     *        @p visit once per pair in that order -- SPEC §2.2's "one
     *        ordered replacement list, longest source first," ties broken
     *        by source text descending.
     *
     * @tparam Visitor Callable invoked as
     *                 @c visit(sourceValue,targetValue,isWordBoundary) for
     *                 each pair, in merged order.
     * @param sourceInfo         The source's own parsed info file.
     * @param targetInfo         The target's own parsed info file.
     * @param identityRows       The sorted, namespace-excluded identity
     *                           rows getIdentityRows() returns.
     * @param namespaceSpacePair The @c "namespace <source>" to
     *                           @c "namespace <target>" pair.
     * @param namespaceColonPair The @c "<source>::" to @c "<target>::"
     *                           pair.
     * @param visit              The callable invoked once per pair, in
     *                           merged order.
     */
    template <typename Visitor>
    static void forEachIdentityPair (const Model& sourceInfo, const Model& targetInfo,
        const jam::Array<const Model::Element*>& identityRows, const std::pair<juce::String, juce::String>& namespaceSpacePair,
        const std::pair<juce::String, juce::String>& namespaceColonPair, Visitor&& visit)
    {
        int rowIndex { 0 };
        auto spacePending { true };
        auto colonPending { true };
        while (rowIndex < identityRows.size() or spacePending or colonPending)
        {
            const auto rowLength { getRowLength (sourceInfo, identityRows, rowIndex) };
            const auto spaceLength { getPendingLength (spacePending, namespaceSpacePair.first) };
            const auto colonLength { getPendingLength (colonPending, namespaceColonPair.first) };
            if (spaceLength >= rowLength and spaceLength >= colonLength)
            {
                visit (namespaceSpacePair.first, namespaceSpacePair.second, false);
                spacePending = false;
            }
            else if (colonLength >= rowLength)
            {
                visit (namespaceColonPair.first, namespaceColonPair.second, false);
                colonPending = false;
            }
            else
            {
                visitRow (sourceInfo, targetInfo, identityRows.at (rowIndex), visit);
                ++rowIndex;
            }
        }
    }

    /**
     * @brief Applies every pair forEachIdentityPair() visits to @p text,
     *        in merged order -- word-boundary pairs through
     *        Transforms::getWordBoundaryReplaced(), every other pair
     *        through plain juce::String::replace().
     *
     * @param sourceInfo         The source's own parsed info file.
     * @param targetInfo         The target's own parsed info file.
     * @param identityRows       The sorted, namespace-excluded identity
     *                           rows getIdentityRows() returns.
     * @param namespaceSpacePair The @c "namespace <source>" to
     *                           @c "namespace <target>" pair.
     * @param namespaceColonPair The @c "<source>::" to @c "<target>::"
     *                           pair.
     * @param text               The text every pair is applied to.
     * @returns @p text, transformed by every pair in merged order.
     */
    static juce::String getTransformedText (const juce::String& text, const Model& sourceInfo,
        const Model& targetInfo, const jam::Array<const Model::Element*>& identityRows,
        const std::pair<juce::String, juce::String>& namespaceSpacePair,
        const std::pair<juce::String, juce::String>& namespaceColonPair)
    {
        auto transformed { text };

        forEachIdentityPair (sourceInfo, targetInfo, identityRows, namespaceSpacePair, namespaceColonPair,
            [&transformed] (const juce::String& sourceValue, const juce::String& targetValue, bool isWordBoundary)
            {
                transformed = isWordBoundary
                                  ? Transforms::getWordBoundaryReplaced (transformed, sourceValue, targetValue)
                                  : transformed.replace (sourceValue, targetValue);
            });

        return transformed;
    }

    /**
     * @brief Returns the first pair's own target value that @p text
     *        already contains -- SPEC §2.2's contamination check, word
     *        pairs tested through Transforms::containsWholeWord(), every
     *        other pair through plain containment.
     *
     * @param sourceInfo         The source's own parsed info file.
     * @param targetInfo         The target's own parsed info file.
     * @param identityRows       The sorted, namespace-excluded identity
     *                           rows getIdentityRows() returns.
     * @param namespaceSpacePair The @c "namespace <source>" to
     *                           @c "namespace <target>" pair.
     * @param namespaceColonPair The @c "<source>::" to @c "<target>::"
     *                           pair.
     * @param text               The text tested for contamination.
     * @returns The first target value @p text already contains, or an
     *          empty string when @p text is clean.
     */
    static juce::String getContaminationToken (const juce::String& text, const Model& sourceInfo,
        const Model& targetInfo, const jam::Array<const Model::Element*>& identityRows,
        const std::pair<juce::String, juce::String>& namespaceSpacePair,
        const std::pair<juce::String, juce::String>& namespaceColonPair)
    {
        juce::String token;

        forEachIdentityPair (sourceInfo, targetInfo, identityRows, namespaceSpacePair, namespaceColonPair,
            [&text, &token] (const juce::String&, const juce::String& targetValue, bool isWordBoundary)
            {
                if (token.isEmpty() and targetValue.isNotEmpty()
                    and (isWordBoundary ? Transforms::containsWholeWord (text, targetValue)
                                        : text.contains (targetValue)))
                    token = targetValue;
            });

        return token;
    }

    /**
     * @brief Returns every module row's own name declared in @p info,
     *        every class alike.
     *
     * @param info The parsed info file whose module table is read.
     * @returns @p info's own declared module names.
     */
    static jam::HashSet<juce::String> getDeclaredModuleNames (const Model& info)
    {
        jam::HashSet<juce::String> names;

        for (auto* row : info.getTableRows (Id::tokenModule))
            names.insert (info.getValue (*row, Id::name));

        return names;
    }

    /**
     * @brief Returns every module row's own name whose @c class cell is
     *        @c kernel -- every other class value is provision (SPEC
     *        §2.2) and is never walked.
     *
     * @param info The parsed info file whose module table is read.
     * @returns @p info's own @c kernel-classed module names.
     */
    static jam::HashSet<juce::String> getKernelModuleNames (const Model& info)
    {
        static const auto kernelClassText { Id::kernel.toString() };

        jam::HashSet<juce::String> names;

        for (auto* row : info.getTableRows (Id::tokenModule))
            if (info.getValue (*row, Id::codeClass).compare (kernelClassText) == 0)
                names.insert (info.getValue (*row, Id::name));

        return names;
    }

    /**
     * @brief Returns every @c ## ignore row's own value in @p info.
     *
     * @param info The parsed info file whose ignore table is read.
     * @returns @p info's own ignore patterns, in authored order.
     */
    static jam::Array<juce::String> getIgnorePatterns (const Model& info)
    {
        jam::Array<juce::String> patterns;

        for (auto* row : info.getTableRows (Id::ignore))
            patterns.add (info.getValue (*row, Id::value));

        return patterns;
    }

    /**
     * @brief Answers whether @p relativePath matches any of @p patterns
     *        (SPEC §2.2's @c * wildcard ignore match).
     *
     * @param relativePath The root-relative, @c /-normalized path to
     *                     test.
     * @param patterns     The ignore patterns to match against.
     * @returns @c true when @p relativePath matches at least one pattern.
     */
    static bool isIgnored (const juce::String& relativePath, const jam::Array<juce::String>& patterns)
    {
        static constexpr bool ignoreCaseInMatch { false };

        for (const auto& pattern : patterns)
            if (relativePath.matchesWildcard (pattern, ignoreCaseInMatch))
                return true;

        return false;
    }

    /**
     * @brief Returns every file under every one of @p kernelNames' own
     *        directories at @p root, walked recursively.
     *
     * @param root        The framework root @p kernelNames are resolved
     *                    against.
     * @param kernelNames The @c kernel-classed module names to walk.
     * @returns Every file found under @p kernelNames' own directories.
     */
    static jam::Array<juce::File> getKernelFiles (const juce::File& root, const jam::HashSet<juce::String>& kernelNames)
    {
        jam::Array<juce::File> files;

        for (const auto& name : kernelNames)
            for (const auto& file : root.getChildFile (name).findChildFiles (juce::File::findFiles, true))
                files.add (file);

        return files;
    }

    /**
     * @brief Returns getKernelFiles()'s own result, @p ignorePatterns'
     *        own matches excluded.
     *
     * @param root           The framework root @p kernelNames are
     *                       resolved against.
     * @param kernelNames    The @c kernel-classed module names to walk.
     * @param ignorePatterns The ignore patterns tested against each
     *                       file's own root-relative path.
     * @returns Every non-ignored file found under @p kernelNames' own
     *          directories.
     */
    static jam::Array<juce::File> getSyncFiles (const juce::File& root,
        const jam::HashSet<juce::String>& kernelNames, const jam::Array<juce::String>& ignorePatterns)
    {
        jam::Array<juce::File> files;

        for (const auto& file : getKernelFiles (root, kernelNames))
            if (not isIgnored (getNormalizedPath (file.getRelativePathFrom (root)), ignorePatterns))
                files.add (file);

        return files;
    }

    /**
     * @brief Normalizes @p text's own line endings to LF (SPEC §10).
     *
     * @param text The text to normalize.
     * @returns @p text with every CRLF pair replaced by a bare LF.
     */
    static juce::String getNewlineNormalized (const juce::String& text)
    {
        static const auto crlfText {
            juce::String::charToString (Chars::carriageReturn) + juce::String::charToString (Chars::newline) };
        static const auto newlineText { juce::String::charToString (Chars::newline) };

        return text.replace (crlfText, newlineText);
    }

    /**
     * @brief Parses @p text fresh and renders it back through the
     *        markdown formatter (SPEC §3.3) -- the re-canonicalization a
     *        prefix-less @c .md kernel scope's own files receive after
     *        the transform.
     *
     * @param text The already-transformed markdown text to canonicalize.
     * @returns @p text's own canonical rendering.
     */
    static juce::String getCanonicalMarkdown (const juce::String& text)
    {
        static const jam::MarkdownWriter formatter;

        const auto document { jam::MarkdownDocument::parse (text) };
        return formatter.getText (document);
    }

    /**
     * @brief Replaces @p sourceFilePrefix with @p targetFilePrefix in
     *        every segment of @p relativePath (SPEC §2.2: @c filePrefix
     *        "transforms every path segment -- directory names and file
     *        names alike").
     *
     * @param relativePath      The root-relative path to transform.
     * @param sourceFilePrefix  The source's own identity @c filePrefix
     *                          value.
     * @param targetFilePrefix  The target's own identity @c filePrefix
     *                          value.
     * @returns @p relativePath with @p sourceFilePrefix replaced by
     *          @p targetFilePrefix.
     */
    static juce::String getTransformedPath (
        const juce::String& relativePath, const juce::String& sourceFilePrefix, const juce::String& targetFilePrefix)
    {
        return relativePath.replace (sourceFilePrefix, targetFilePrefix);
    }

    /**
     * @brief Returns @p relativePath's own leading path segment -- the
     *        kernel scope's own name the re-canonicalization gate (SPEC
     *        §2.2) reads.
     *
     * @param relativePath The root-relative path whose own kernel scope
     *                     is read.
     * @returns @p relativePath's own first segment, up to its first
     *          @c /.
     */
    static juce::String getKernelScopeName (const juce::String& relativePath)
    {
        static const auto slashText { juce::String::charToString (Chars::slash) };

        return relativePath.upToFirstOccurrenceOf (slashText, false, false);
    }

    /**
     * @brief Answers whether @p data carries a NUL byte in its first 8000
     *        bytes (SPEC §2.2's binary-file test).
     *
     * @param data The raw file bytes to scan.
     * @returns @c true when a NUL byte appears within @p data's own first
     *          8000 bytes.
     */
    static bool hasBinaryMarker (const juce::MemoryBlock& data) noexcept
    {
        static constexpr size_t binaryScanLimit { 8000 };

        const auto* bytes { static_cast<const char*> (data.getData()) };
        const auto scanLength { std::min (data.getSize(), binaryScanLimit) };

        for (size_t index { 0 }; index < scanLength; ++index)
            if (bytes[index] == static_cast<char> (Chars::nullCharacter))
                return true;

        return false;
    }

    /**
     * @brief Reads @p sourceFile solely to check it for contamination --
     *        the first pass's own per-file step, its content discarded
     *        once the check completes.
     *
     * @param sourceFile         The source file to check.
     * @param sourceRoot         The framework root @p sourceFile is
     *                           resolved under.
     * @param sourceInfo         The source's own parsed info file.
     * @param targetInfo         The target's own parsed info file.
     * @param identityRows       The sorted, namespace-excluded identity
     *                           rows getIdentityRows() returns.
     * @param namespaceSpacePair The @c "namespace <source>" to
     *                           @c "namespace <target>" pair.
     * @param namespaceColonPair The @c "<source>::" to @c "<target>::"
     *                           pair.
     * @returns An empty string when @p sourceFile is unreadable-safe or
     *          binary or clean, or a diagnostic naming @p sourceFile and
     *          the offending token or the read failure.
     */
    static juce::String getContaminationCheck (const juce::File& sourceFile, const juce::File& sourceRoot,
        const Model& sourceInfo, const Model& targetInfo, const jam::Array<const Model::Element*>& identityRows,
        const std::pair<juce::String, juce::String>& namespaceSpacePair,
        const std::pair<juce::String, juce::String>& namespaceColonPair)
    {
        const auto rootRelativePath { getNormalizedPath (sourceFile.getRelativePathFrom (sourceRoot)) };
        juce::MemoryBlock rawData;

        if (not sourceFile.loadFileAsData (rawData))
            return rootRelativePath + Id::diagnosticSeparator + text::Diagnostics::failSyncRead;

        if (hasBinaryMarker (rawData))
            return {};

        const auto text { juce::String::fromUTF8 (
            static_cast<const char*> (rawData.getData()), static_cast<int> (rawData.getSize())) };
        const auto token { getContaminationToken (
            text, sourceInfo, targetInfo, identityRows, namespaceSpacePair, namespaceColonPair) };

        if (token.isEmpty())
            return {};

        return rootRelativePath + Id::diagnosticSeparator + text::Diagnostics::failSyncContamination
             + Id::diagnosticSeparator + token;
    }

    /**
     * @brief Builds @p targetFile's own write-failure diagnostic.
     *
     * @param targetFile The output file that failed to write.
     * @returns @p targetFile's own full path, the generic output-write
     *          diagnostic text.
     */
    static juce::String getWriteFailure (const juce::File& targetFile)
    {
        return targetFile.getFullPathName() + Id::diagnosticSeparator + text::Diagnostics::failOutputWrite;
    }

    /**
     * @brief Renames @p targetFile's own case-differing sibling, if any,
     *        onto @p targetFile -- a case-sensitive host's own reconciler
     *        for a case-insensitive-authored path (SPEC §2.2).
     *
     * @param targetFile The output file whose parent directory is
     *                   scanned for a case-differing sibling.
     * @returns An empty string when no sibling differs by case only or
     *          the rename succeeded, or getWriteFailure()'s own
     *          diagnostic when the rename failed.
     */
    static juce::String getCaseReconciliationFailure (const juce::File& targetFile)
    {
        for (const auto& sibling : targetFile.getParentDirectory().findChildFiles (juce::File::findFiles, false))
            if (sibling.getFileName().compareIgnoreCase (targetFile.getFileName()) == 0
                and sibling.getFileName().compare (targetFile.getFileName()) != 0)
                if (not sibling.moveFileTo (targetFile))
                    return getWriteFailure (targetFile);

        return juce::String {};
    }

    /**
     * @brief Writes @p targetFile write-if-different from @p binaryContent.
     *
     * @param targetFile        The file to write.
     * @param targetRelativePath @p targetFile's own root-relative path,
     *                          reported on a successful write.
     * @param binaryContent     The raw bytes to write.
     * @returns @p targetRelativePath paired with an empty string when the
     *          write succeeded or was unneeded, or an empty string paired
     *          with getWriteFailure()'s own diagnostic when the write
     *          failed.
     */
    static std::pair<juce::String, juce::String> getBinaryWriteOutcome (const juce::File& targetFile,
        const juce::String& targetRelativePath, const juce::MemoryBlock& binaryContent)
    {
        juce::MemoryBlock currentData;
        targetFile.loadFileAsData (currentData);

        if (currentData == binaryContent)
            return { juce::String {}, juce::String {} };

        return targetFile.replaceWithData (binaryContent.getData(), binaryContent.getSize())
                   ? std::make_pair (targetRelativePath, juce::String {})
                   : std::make_pair (juce::String {}, getWriteFailure (targetFile));
    }

    /**
     * @brief Writes @p targetFile write-if-different from @p textContent,
     *        LF-normalized.
     *
     * @param targetFile        The file to write.
     * @param targetRelativePath @p targetFile's own root-relative path,
     *                          reported on a successful write.
     * @param textContent       The transformed text to write.
     * @returns @p targetRelativePath paired with an empty string when the
     *          write succeeded or was unneeded, or an empty string paired
     *          with getWriteFailure()'s own diagnostic when the write
     *          failed.
     */
    static std::pair<juce::String, juce::String> getTextWriteOutcome (const juce::File& targetFile,
        const juce::String& targetRelativePath, const juce::String& textContent)
    {
        static const auto newlineText { juce::String::charToString (Chars::newline) };

        if (targetFile.loadFileAsString().compare (textContent) == 0)
            return { juce::String {}, juce::String {} };

        return targetFile.replaceWithText (textContent, false, false, newlineText.toRawUTF8())
                   ? std::make_pair (targetRelativePath, juce::String {})
                   : std::make_pair (juce::String {}, getWriteFailure (targetFile));
    }

    /**
     * @brief Writes @p targetFile write-if-different, binary or text per
     *        @p isBinary, creating its parent directory first and
     *        reconciling any case-differing sibling onto it.
     *
     * @param targetFile        The file to write.
     * @param targetRelativePath @p targetFile's own root-relative path,
     *                          reported on a successful write.
     * @param isBinary          Whether @p binaryContent, rather than
     *                          @p textContent, is the content to write.
     * @param textContent       The transformed text to write, read when
     *                          @p isBinary is @c false.
     * @param binaryContent     The raw bytes to write, read when
     *                          @p isBinary is @c true.
     * @returns @p targetRelativePath paired with an empty string when the
     *          write succeeded or was unneeded, or an empty string paired
     *          with getWriteFailure()'s own or getCaseReconciliationFailure()'s
     *          own diagnostic when the write or the reconciliation failed.
     */
    static std::pair<juce::String, juce::String> getWriteOutcome (const juce::File& targetFile,
        const juce::String& targetRelativePath, bool isBinary, const juce::String& textContent,
        const juce::MemoryBlock& binaryContent)
    {
        targetFile.getParentDirectory().createDirectory();

        if (const auto reconciliationFailure { getCaseReconciliationFailure (targetFile) };
            reconciliationFailure.isNotEmpty())
            return { juce::String {}, reconciliationFailure };

        if (isBinary)
            return getBinaryWriteOutcome (targetFile, targetRelativePath, binaryContent);

        return getTextWriteOutcome (targetFile, targetRelativePath, textContent);
    }

    /**
     * @brief Reads @p sourceFile, transforms it, and writes it under
     *        @p targetRoot -- the second pass's own per-file step: a
     *        binary file copies byte-for-byte; a text file is
     *        transformed, LF-normalized, and, when it is a prefix-less
     *        @c .md file in its own kernel scope, re-canonicalized.
     *
     * @param sourceFile         The source file to sync.
     * @param sourceRoot         The framework root @p sourceFile is
     *                           resolved under.
     * @param targetRoot         The framework root @p sourceFile is
     *                           written under.
     * @param sourceFilePrefix   The source's own identity @c filePrefix
     *                           value.
     * @param targetFilePrefix   The target's own identity @c filePrefix
     *                           value.
     * @param sourceInfo         The source's own parsed info file.
     * @param targetInfo         The target's own parsed info file.
     * @param identityRows       The sorted, namespace-excluded identity
     *                           rows getIdentityRows() returns.
     * @param namespaceSpacePair The @c "namespace <source>" to
     *                           @c "namespace <target>" pair.
     * @param namespaceColonPair The @c "<source>::" to @c "<target>::"
     *                           pair.
     * @returns getWriteOutcome()'s own result, or an empty string paired
     *          with a read-failure diagnostic naming @p sourceFile when it
     *          cannot be read.
     */
    static std::pair<juce::String, juce::String> getSyncOutcome (const juce::File& sourceFile,
        const juce::File& sourceRoot, const juce::File& targetRoot, const juce::String& sourceFilePrefix,
        const juce::String& targetFilePrefix, const Model& sourceInfo, const Model& targetInfo,
        const jam::Array<const Model::Element*>& identityRows, const std::pair<juce::String, juce::String>& namespaceSpacePair,
        const std::pair<juce::String, juce::String>& namespaceColonPair)
    {
        const auto rootRelativePath { getNormalizedPath (sourceFile.getRelativePathFrom (sourceRoot)) };
        const auto targetRelativePath { getTransformedPath (rootRelativePath, sourceFilePrefix, targetFilePrefix) };
        const auto targetFile { targetRoot.getChildFile (targetRelativePath) };

        juce::MemoryBlock rawData;

        if (not sourceFile.loadFileAsData (rawData))
            return { juce::String {}, rootRelativePath + Id::diagnosticSeparator + text::Diagnostics::failSyncRead };

        if (hasBinaryMarker (rawData))
            return getWriteOutcome (targetFile, targetRelativePath, true, juce::String {}, rawData);

        const auto text { juce::String::fromUTF8 (
            static_cast<const char*> (rawData.getData()), static_cast<int> (rawData.getSize())) };
        auto transformed { getNewlineNormalized (getTransformedText (
            text, sourceInfo, targetInfo, identityRows, namespaceSpacePair, namespaceColonPair)) };
        const auto kernelScopeName { getKernelScopeName (rootRelativePath) };

        if (sourceFile.hasFileExtension (Extensions::md) and not kernelScopeName.startsWith (sourceFilePrefix))
            transformed = getCanonicalMarkdown (transformed);

        return getWriteOutcome (targetFile, targetRelativePath, false, transformed, juce::MemoryBlock {});
    }

    /**
     * @brief Deletes every file under @p targetKernelNames' own
     *        directories at @p targetRoot with no matching entry in
     *        @p expectedTargetPaths and no @p targetIgnorePatterns match
     *        (SPEC §2.2's mirror-delete).
     *
     * @param targetRoot           The framework root mirror-delete walks.
     * @param targetKernelNames    @p targetRoot's own @c kernel-classed
     *                             module names.
     * @param targetIgnorePatterns @p targetRoot's own ignore patterns.
     * @param expectedTargetPaths  Every root-relative path the sync run's
     *                             own transformed source files resolve
     *                             to.
     * @returns juce::Result::ok() paired with every deleted path, or a
     *          failure naming the first file that could not be deleted,
     *          paired with an empty array.
     */
    static std::pair<juce::Result, jam::Array<juce::String>> getMirrorDeletions (const juce::File& targetRoot,
        const jam::HashSet<juce::String>& targetKernelNames, const jam::Array<juce::String>& targetIgnorePatterns,
        const jam::HashSet<juce::String>& expectedTargetPaths)
    {
        jam::Array<juce::String> deletions;

        for (const auto& file : getKernelFiles (targetRoot, targetKernelNames))
        {
            const auto rootRelativePath { getNormalizedPath (file.getRelativePathFrom (targetRoot)) };

            if (not isIgnored (rootRelativePath, targetIgnorePatterns)
                and not expectedTargetPaths.contains (rootRelativePath))
            {
                if (not file.deleteFile())
                    return std::make_pair (juce::Result::fail (rootRelativePath + Id::diagnosticSeparator
                                               + text::Diagnostics::failSyncDelete),
                        jam::Array<juce::String> {});

                deletions.add (rootRelativePath);
            }
        }

        return std::make_pair (juce::Result::ok(), std::move (deletions));
    }

    /**
     * @brief Returns @p paths' own non-empty entries, sorted ascending --
     *        the report's own deterministic line order (SPEC §2.2).
     *
     * @param paths The candidate paths, some possibly empty.
     * @returns @p paths' own non-empty entries, sorted.
     */
    static jam::Array<juce::String> getReportLines (const jam::Array<juce::String>& paths)
    {
        jam::Array<juce::String> filtered;

        for (const auto& path : paths)
            if (path.isNotEmpty())
                filtered.add (path);

        std::sort (filtered.begin(), filtered.end());
        return filtered;
    }

    /**
     * @brief Prints @p writtenPaths then @p deletedPaths to stdout, each
     *        sorted through getReportLines(), one path per line -- the
     *        run's own report (SPEC §2.2). Zero lines means the roots
     *        are already in sync.
     *
     * @param writtenPaths The paths the run actually wrote.
     * @param deletedPaths The paths the run actually deleted.
     */
    static void printReport (const jam::Array<juce::String>& writtenPaths, const jam::Array<juce::String>& deletedPaths)
    {
        for (const auto& path : getReportLines (writtenPaths))
            printf ("%s\n", path.toRawUTF8());

        for (const auto& path : getReportLines (deletedPaths))
            printf ("%s\n", path.toRawUTF8());
    }
};
