#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Jobs.h"
#include "Model.h"
#include "Transforms.h"

/**
 * @struct Sync
 * @brief Mirrors one framework's kernel onto another's -- parses each
 *        root's own @c user-modules-info.md, validates the
 *        sync-specific gates through its own check members, then walks the source's
 *        own kernel @c ## submodule rows, transforms each file, and
 *        writes it into the target root. A file the target holds in a
 *        @c ## region keeps its own region lines. Sync deletes nothing.
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
     * @param targetRoot The framework root sync writes into.
     * @param formatter  The writer every re-canonicalized .md file renders through.
     * @returns juce::Result::ok() when the sync run succeeds, or the
     *          first failing gate's or step's result.
     */
    static juce::Result run (const juce::File& sourceRoot, const juce::File& targetRoot, const jam::MarkdownWriter& formatter);

private:
    /**
     * @brief Resolves @p root's own @c user-modules-info.md file.
     *
     * @param root The sync root the info file is resolved against.
     * @returns @p root's own info file, whether or not it exists.
     */
    static juce::File getInfoFile (const juce::File& root);

    /**
     * @brief Normalizes @p relativePath's own directory separators to
     *        @c / -- root-relative paths use @c / on every host.
     *
     * @param relativePath The root-relative path to normalize.
     * @returns @p relativePath with every backslash replaced by a slash.
     */
    static juce::String getNormalizedPath (const juce::String& relativePath);

    /**
     * @brief Resolves both roots' own @c filePrefix, module-name sets, and
     *        kernel submodule path sets, checks isKernelDeclared() and
     *        isSubmoduleDeclared(), then runs runWalk().
     *
     * @param sourceRoot The framework root sync reads from.
     * @param targetRoot The framework root sync writes into.
     * @param sourceInfo @p sourceRoot's own parsed info file.
     * @param targetInfo @p targetRoot's own parsed info file.
     * @param formatter  The writer every re-canonicalized .md file renders through.
     * @returns juce::Result::ok() when every gate and step succeeds, or
     *          the first failing one's result.
     */
    static juce::Result runGates (const juce::File& sourceRoot, const juce::File& targetRoot,
        const Model& sourceInfo, const Model& targetInfo, const jam::MarkdownWriter& formatter);

    /**
     * @brief Checks the listing gate at both roots, then the
     *        correspondence gate between the source's and the target's
     *        kernel submodule paths, through isSubmoduleListed() and
     *        isCorresponding(). At the target, a
     *        kernel row's file can be absent, because sync writes it.
     *
     * @param sourceRoot                 The framework root sync reads from.
     * @param targetRoot                 The framework root sync writes into.
     * @param sourceInfo                 @p sourceRoot's own parsed info
     *                                   file.
     * @param targetInfo                 @p targetRoot's own parsed info
     *                                   file.
     * @param sourceKernelNames          @p sourceRoot's own
     *                                   @c kernel-classed module names.
     * @param targetKernelNames          @p targetRoot's own
     *                                   @c kernel-classed module names.
     * @param sourceKernelSubmodulePaths @p sourceRoot's own
     *                                   @c kernel-classed submodule paths.
     * @param targetKernelSubmodulePaths @p targetRoot's own
     *                                   @c kernel-classed submodule paths.
     * @param sourceFilePrefix           @p sourceRoot's own identity
     *                                   @c filePrefix value.
     * @param targetFilePrefix           @p targetRoot's own identity
     *                                   @c filePrefix value.
     * @returns juce::Result::ok() when both gates hold, or the first
     *          failing gate's result.
     */
    static juce::Result isSubmoduleDeclared (const juce::File& sourceRoot, const juce::File& targetRoot,
        const Model& sourceInfo, const Model& targetInfo, const jam::HashSet<juce::String>& sourceKernelNames,
        const jam::HashSet<juce::String>& targetKernelNames,
        const jam::HashSet<juce::String>& sourceKernelSubmodulePaths,
        const jam::HashSet<juce::String>& targetKernelSubmodulePaths, const juce::String& sourceFilePrefix,
        const juce::String& targetFilePrefix);

    /**
     * @brief Checks the kernel-presence, prefix-declared, and
     *        correspondence gates against both roots, through
     *        isKernelPresent(), isPrefixDeclared(), and
     *        isCorresponding().
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
        const jam::HashSet<juce::String>& sourceKernelNames, const jam::HashSet<juce::String>& targetKernelNames);

    /**
     * @brief Builds the ordered replacement pairs and the source's own
     *        kernel submodule file list, runs runContamination() over
     *        them, then, once clean, runTransform().
     *
     * @param sourceRoot        The framework root sync reads from.
     * @param targetRoot        The framework root sync writes into.
     * @param sourceInfo        @p sourceRoot's own parsed info file.
     * @param targetInfo        @p targetRoot's own parsed info file.
     * @param sourceFilePrefix  @p sourceRoot's own identity @c filePrefix
     *                          value.
     * @param targetFilePrefix  @p targetRoot's own identity @c filePrefix
     *                          value.
     * @param formatter         The writer every re-canonicalized .md file renders through.
     * @returns juce::Result::ok() when the walk, contamination check, and
     *          transform all succeed, or the first failure's result.
     */
    static juce::Result runWalk (const juce::File& sourceRoot, const juce::File& targetRoot,
        const Model& sourceInfo, const Model& targetInfo, const juce::String& sourceFilePrefix,
        const juce::String& targetFilePrefix, const jam::MarkdownWriter& formatter);

    /**
     * @brief Runs getContaminationCheck() over every one of @p sourceFiles,
     *        one juce::ThreadPool job per file, reading each file solely
     *        for the check and discarding its content -- the first pass
     *        of Sync's own two-pass walk. The contamination fatal
     *        must stop the run before any write, never after.
     *
     * @param sourceFiles         The source's own kernel submodule rows'
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
        const std::pair<juce::String, juce::String>& namespaceColonPair);

    /**
     * @brief Runs getSyncOutcome() over every one of @p sourceFiles, one
     *        juce::ThreadPool job per file -- the second pass of Sync's
     *        own two-pass walk, run only once runContamination() has
     *        cleared every file.
     *
     * @param sourceFiles        The source's own kernel submodule rows'
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
     * @param formatter          The writer every re-canonicalized .md file renders through.
     * @returns Each file's own getSyncOutcome() result, in @p sourceFiles'
     *          own order.
     */
    static jam::Array<std::pair<juce::String, juce::String>> getSyncResults (const jam::Array<juce::File>& sourceFiles,
        const juce::File& sourceRoot, const juce::File& targetRoot, const juce::String& sourceFilePrefix,
        const juce::String& targetFilePrefix, const Model& sourceInfo, const Model& targetInfo,
        const jam::Array<const Model::Element*>& identityRows, const std::pair<juce::String, juce::String>& namespaceSpacePair,
        const std::pair<juce::String, juce::String>& namespaceColonPair, const jam::MarkdownWriter& formatter);

    /**
     * @brief Runs getSyncResults(), collects every written path, and, when
     *        every file wrote or was already current, prints the report.
     *
     * @param sourceFiles        The source's own kernel submodule rows'
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
     * @param formatter          The writer every re-canonicalized .md file renders through.
     * @returns juce::Result::ok() when every file writes successfully, or
     *          the first failure's result.
     */
    static juce::Result runTransform (const jam::Array<juce::File>& sourceFiles, const juce::File& sourceRoot,
        const juce::File& targetRoot, const juce::String& sourceFilePrefix, const juce::String& targetFilePrefix,
        const Model& sourceInfo, const Model& targetInfo, const jam::Array<const Model::Element*>& identityRows,
        const std::pair<juce::String, juce::String>& namespaceSpacePair,
        const std::pair<juce::String, juce::String>& namespaceColonPair, const jam::MarkdownWriter& formatter);

    /**
     * @brief Returns @p info's own identity row keyed @p key's own
     *        @c value cell.
     *
     * @param info The parsed info file whose identity table is read.
     * @param key  The identity row's own key.
     * @returns @p key's own resolved value in @p info.
     */
    static juce::String getIdentityValue (const Model& info, const juce::String& key);

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
    getIdentityValues (const Model& sourceInfo, const Model& targetInfo, const juce::String& key);

    /**
     * @brief Builds the @c "namespace <source>" to @c "namespace <target>"
     *        composed pair.
     *
     * @param sourceValue The source's own @c namespace identity value.
     * @param targetValue The target's own @c namespace identity value.
     * @returns The composed source text paired with the composed target
     *          text.
     */
    static std::pair<juce::String, juce::String>
    getNamespaceSpacePair (const juce::String& sourceValue, const juce::String& targetValue);

    /**
     * @brief Builds the @c "<source>::" to @c "<target>::" composed pair.
     *
     * @param sourceValue The source's own @c namespace identity value.
     * @param targetValue The target's own @c namespace identity value.
     * @returns The composed source text paired with the composed target
     *          text.
     */
    static std::pair<juce::String, juce::String>
    getNamespaceColonPair (const juce::String& sourceValue, const juce::String& targetValue);

    /**
     * @brief Returns every identity row present in both @p sourceInfo and
     *        @p targetInfo, the @c namespace row excluded (it contributes
     *        no plain pair), sorted longest-source-first, ties
     *        broken by source text descending.
     *
     * The parsed @p sourceInfo Model is the store -- this
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
    static jam::Array<const Model::Element*> getIdentityRows (const Model& sourceInfo, const Model& targetInfo);

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
    getRowPair (const Model& sourceInfo, const Model& targetInfo, const Model::Element* row);

    /**
     * @brief Answers whether @p row's own @c boundary cell is @c word --
     *        the row matches whole words only.
     *
     * @param sourceInfo The parsed info file @p row belongs to.
     * @param row        The identity row to test.
     * @returns @c true when @p row's own boundary cell reads @c word.
     */
    static bool isRowWordBoundary (const Model& sourceInfo, const Model::Element* row);

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
    static int getPendingLength (bool pending, const juce::String& source) noexcept;

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
    static int getRowLength (const Model& sourceInfo, const jam::Array<const Model::Element*>& identityRows, int rowIndex);

    /**
     * @brief Walks @p identityRows and the two composed namespace pairs
     *        as one merged, longest-source-first sequence, invoking
     *        @p visit once per pair in that order -- one
     *        ordered replacement list, longest source first, ties broken
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
        const std::pair<juce::String, juce::String>& namespaceColonPair);

    /**
     * @brief Returns the first pair's own target value that @p text
     *        already contains -- the contamination check, word
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
        const std::pair<juce::String, juce::String>& namespaceColonPair);

    /**
     * @brief Returns every module row's own name declared in @p info,
     *        every class alike.
     *
     * @param info The parsed info file whose module table is read.
     * @returns @p info's own declared module names.
     */
    static jam::HashSet<juce::String> getDeclaredModuleNames (const Model& info);

    /**
     * @brief Returns every module row's own name whose @c class cell is
     *        @c kernel -- every other class value is provision
     *        and is never walked.
     *
     * @param info The parsed info file whose module table is read.
     * @returns @p info's own @c kernel-classed module names.
     */
    static jam::HashSet<juce::String> getKernelModuleNames (const Model& info);

    /**
     * @brief Returns every @c ## ignore row's own value in @p info.
     *
     * @param info The parsed info file whose ignore table is read.
     * @returns @p info's own ignore patterns, in authored order.
     */
    static jam::Array<juce::String> getIgnorePatterns (const Model& info);

    /**
     * @brief Returns @p row's own root-relative submodule path -- its
     *        module name, a slash, and its own @c path cell.
     *
     * @param info The parsed info file @p row belongs to.
     * @param row  The @c ## submodule row whose path is built.
     * @returns The root-relative path of @p row's own file.
     */
    static juce::String getSubmodulePath (const Model& info, const Model::Element& row);

    /**
     * @brief Returns every @c ## submodule row's own root-relative path in
     *        @p info, every class alike.
     *
     * @param info The parsed info file whose submodule table is read.
     * @returns @p info's own declared submodule paths.
     */
    static jam::HashSet<juce::String> getDeclaredSubmodulePaths (const Model& info);

    /**
     * @brief Returns every @c ## submodule row's own root-relative path
     *        whose @c class cell is @c kernel -- every other class value is
     *        provision and is never walked.
     *
     * @param info The parsed info file whose submodule table is read.
     * @returns @p info's own @c kernel-classed submodule paths.
     */
    static jam::HashSet<juce::String> getKernelSubmodulePaths (const Model& info);

    /**
     * @brief Answers whether @p relativePath matches any of @p patterns,
     *        each tested as a @c * wildcard ignore match.
     *
     * @param relativePath The root-relative, @c /-normalized path to
     *                     test.
     * @param patterns     The ignore patterns to match against.
     * @returns @c true when @p relativePath matches at least one pattern.
     */
    static bool isIgnored (const juce::String& relativePath, const jam::Array<juce::String>& patterns);

    /**
     * @brief Returns every file under every one of @p kernelNames' own
     *        directories at @p root, walked recursively.
     *
     * @param root        The framework root @p kernelNames are resolved
     *                    against.
     * @param kernelNames The @c kernel-classed module names to walk.
     * @returns Every file found under @p kernelNames' own directories.
     */
    static jam::Array<juce::File> getKernelFiles (const juce::File& root, const jam::HashSet<juce::String>& kernelNames);

    /**
     * @brief Returns the root-relative path of every file under
     *        @p kernelNames' own directories at @p root, minus the files
     *        @p ignorePatterns match.
     *
     * @param root           The framework root @p kernelNames are resolved
     *                       against.
     * @param kernelNames    The @c kernel-classed module names to walk.
     * @param ignorePatterns The @c ## ignore patterns that drop a path.
     * @returns The @c /-normalized, root-relative paths on disk.
     */
    static jam::HashSet<juce::String> getDiskPaths (const juce::File& root,
        const jam::HashSet<juce::String>& kernelNames, const jam::Array<juce::String>& ignorePatterns);

    /**
     * @brief Returns the file of every @c kernel-classed @c ## submodule
     *        row in @p info, in authored order.
     *
     * @param root The framework root each row's path is resolved against.
     * @param info The parsed info file whose submodule table is read.
     * @returns The files sync reads, in the rows' own order.
     */
    static jam::Array<juce::File> getSyncFiles (const juce::File& root, const Model& info);

    /**
     * @brief Normalizes @p text's own line endings to LF.
     *
     * @param text The text to normalize.
     * @returns @p text with every CRLF pair replaced by a bare LF.
     */
    static juce::String getNewlineNormalized (const juce::String& text);

    /**
     * @brief Parses @p text fresh and renders it back through the
     *        markdown formatter -- the re-canonicalization a
     *        prefix-less @c .md kernel scope's own files receive after
     *        the transform.
     *
     * @param text      The already-transformed markdown text to canonicalize.
     * @param formatter The writer every re-canonicalized .md file renders through.
     * @returns @p text's own canonical rendering.
     */
    static juce::String getCanonicalMarkdown (const juce::String& text, const jam::MarkdownWriter& formatter);

    /**
     * @brief Returns @p relativePath's own leading path segment -- the
     *        kernel scope's own name the re-canonicalization gate reads.
     *
     * @param relativePath The root-relative path whose own kernel scope
     *                     is read.
     * @returns @p relativePath's own first segment, up to its first
     *          @c /.
     */
    static juce::String getKernelScopeName (const juce::String& relativePath);

    /**
     * @brief Answers whether @p data carries a NUL byte in its first 8000
     *        bytes -- the binary-file test.
     *
     * @param data The raw file bytes to scan.
     * @returns @c true when a NUL byte appears within @p data's own first
     *          8000 bytes.
     */
    static bool hasBinaryMarker (const juce::MemoryBlock& data) noexcept;

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
        const std::pair<juce::String, juce::String>& namespaceColonPair);

    /**
     * @brief Builds @p targetFile's own write-failure diagnostic.
     *
     * @param targetFile The output file that failed to write.
     * @returns @p targetFile's own full path, the generic output-write
     *          diagnostic text.
     */
    static juce::String getWriteFailure (const juce::File& targetFile);

    /**
     * @brief Renames @p targetFile's own case-differing sibling, if any,
     *        onto @p targetFile -- a case-sensitive host's own reconciler
     *        for a case-insensitive-authored path.
     *
     * @param targetFile The output file whose parent directory is
     *                   scanned for a case-differing sibling.
     * @returns An empty string when no sibling differs by case only or
     *          the rename succeeded, or getWriteFailure()'s own
     *          diagnostic when the rename failed.
     */
    static juce::String getCaseReconciliationFailure (const juce::File& targetFile);

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
        const juce::String& targetRelativePath, const juce::MemoryBlock& binaryContent);

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
        const juce::String& targetRelativePath, const juce::String& textContent);

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
        const juce::MemoryBlock& binaryContent);

    /**
     * @brief Splices the target's own lines of one region into the source
     *        lines, when both hold the delimiter pair.
     *
     * @param lines       The transformed source text, split into lines.
     * @param targetLines The target file's own content, split into lines.
     * @param beginValue  The region's own @c \[begin\] delimiter text.
     * @param endValue    The region's own @c \[end\] delimiter text.
     * @returns @p lines with the lines between the delimiters replaced by
     *          @p targetLines' own, or @p lines when either side lacks the
     *          pair.
     */
    static jam::Strings getRegionKeptLines (const jam::Strings& lines, const jam::Strings& targetLines,
        const juce::String& beginValue, const juce::String& endValue);

    /**
     * @brief Keeps @p targetFile's own lines of every @c ## region of
     *        @p targetInfo in @p text, when @p targetFile exists.
     *
     * @param text       The transformed source text.
     * @param targetFile The file sync writes.
     * @param targetInfo The target's own parsed info file, whose region
     *                   rows name the delimiter pairs.
     * @returns @p text with each region's lines taken from @p targetFile,
     *          or @p text unchanged when @p targetFile does not exist.
     */
    static juce::String getRegionKeptText (
        const juce::String& text, const juce::File& targetFile, const Model& targetInfo);

    /**
     * @brief Reads @p sourceFile, transforms it, and writes it under
     *        @p targetRoot -- the second pass's own per-file step: a
     *        binary file copies byte-for-byte; a text file is
     *        transformed, LF-normalized, when it is a prefix-less
     *        @c .md file in its own kernel scope re-canonicalized, and,
     *        when the target file exists, given the target's own
     *        @c ## region lines through getRegionKeptText().
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
     * @param formatter          The writer every re-canonicalized .md file renders through.
     * @returns getWriteOutcome()'s own result, or an empty string paired
     *          with a read-failure diagnostic naming @p sourceFile when it
     *          cannot be read.
     */
    static std::pair<juce::String, juce::String> getSyncOutcome (const juce::File& sourceFile,
        const juce::File& sourceRoot, const juce::File& targetRoot, const juce::String& sourceFilePrefix,
        const juce::String& targetFilePrefix, const Model& sourceInfo, const Model& targetInfo,
        const jam::Array<const Model::Element*>& identityRows, const std::pair<juce::String, juce::String>& namespaceSpacePair,
        const std::pair<juce::String, juce::String>& namespaceColonPair, const jam::MarkdownWriter& formatter);

    /**
     * @brief Returns @p paths' own non-empty entries, sorted ascending --
     *        the report's own deterministic line order.
     *
     * @param paths The candidate paths, some possibly empty.
     * @returns @p paths' own non-empty entries, sorted.
     */
    static jam::Array<juce::String> getReportLines (const jam::Array<juce::String>& paths);

    /**
     * @brief Prints @p writtenPaths to stdout, sorted through
     *        getReportLines(), one path per line -- the
     *        run's own report. Zero lines means the roots
     *        are already in sync.
     *
     * @param writtenPaths The paths the run actually wrote.
     */
    static void printReport (const jam::Array<juce::String>& writtenPaths);

    /**
     * @brief Checks that @p sourceRoot and @p targetRoot resolve to
     *        different directories.
     *
     * @param sourceRoot The sync source root.
     * @param targetRoot The sync target root.
     * @returns juce::Result::ok() when the two roots differ, or a failure
     *          naming @p sourceRoot.
     */
    static juce::Result isRootsDistinct (const juce::File& sourceRoot, const juce::File& targetRoot);

    /**
     * @brief Checks that @p infoFile exists -- the
     *        @c user-modules-info.md presence fatal.
     *
     * @param infoFile The sync root's own resolved info file.
     * @returns juce::Result::ok() when @p infoFile exists, or a failure
     *          naming it.
     */
    static juce::Result isInfoPresent (const juce::File& infoFile);

    /**
     * @brief Checks that each of the three composed identity keys --
     *        @c namespace, @c filePrefix, @c macroPrefix -- is present
     *        in both @p sourceInfo and @p targetInfo.
     *
     * @param sourceInfoFile The source's own resolved info file, named in
     *                       a failure when @p sourceInfo is missing the
     *                       key.
     * @param targetInfoFile The target's own resolved info file, named in
     *                       a failure when @p targetInfo is missing the
     *                       key.
     * @param sourceInfo     The source's own parsed info file.
     * @param targetInfo     The target's own parsed info file.
     * @returns juce::Result::ok() when every composed key resolves in
     *          both files, or a failure naming the missing file and key.
     */
    static juce::Result isComposedComplete (const juce::File& sourceInfoFile, const juce::File& targetInfoFile,
        const Model& sourceInfo, const Model& targetInfo);

    /**
     * @brief Checks that every one of @p kernelNames' own directories
     *        exists at @p root -- the kernel-row existence fatal,
     *        checked at each root.
     *
     * @param root        The sync root @p kernelNames are resolved
     *                    against.
     * @param kernelNames @p root's own @c kernel-classed module names.
     * @returns juce::Result::ok() when every kernel name's own directory
     *          exists, or a failure naming @p root and the first missing
     *          name in ascending order.
     */
    static juce::Result isKernelPresent (const juce::File& root, const jam::HashSet<juce::String>& kernelNames);

    /**
     * @brief Checks that every directory at @p root whose own name starts
     *        with @p filePrefix is declared in @p declaredNames -- the
     *        @c \<filePrefix\>* declaration fatal, checked at each
     *        root).
     *
     * @param root          The sync root walked for @p filePrefix's own
     *                      directories.
     * @param filePrefix    @p root's own identity @c filePrefix value.
     * @param declaredNames Every module row's own name declared at
     *                      @p root.
     * @returns juce::Result::ok() when every @p filePrefix directory is
     *          declared, or a failure naming @p root and the undeclared
     *          directory.
     */
    static juce::Result isPrefixDeclared (const juce::File& root, const juce::String& filePrefix,
        const jam::HashSet<juce::String>& declaredNames);

    /**
     * @brief Checks that the files on disk under a root's own kernel
     *        scopes are exactly the files its @c ## submodule rows name --
     *        except that a declared file listed in @p writablePaths can be
     *        absent, because sync writes it.
     *
     * @param root          The sync root, named in a failure.
     * @param diskPaths     The root-relative paths on disk under the
     *                      root's own kernel scopes, after the ignore
     *                      globs.
     * @param declaredPaths The root-relative paths the root's own
     *                      @c ## submodule rows name.
     * @param writablePaths The declared paths that can be absent from disk.
     * @returns juce::Result::ok() when the two sets agree, or a failure
     *          naming @p root and the first differing path, in ascending
     *          order.
     */
    static juce::Result isSubmoduleListed (const juce::File& root, const jam::HashSet<juce::String>& diskPaths,
        const jam::HashSet<juce::String>& declaredPaths, const jam::HashSet<juce::String>& writablePaths);

    /**
     * @brief Checks that @p sourceKernelNames, each name transformed by
     *        the @p sourceFilePrefix-to-@p targetFilePrefix pair,
     *        corresponds one to one with @p targetKernelNames. A name is a
     *        module name or a submodule path.
     *
     * @param sourceRoot        The sync source root, named in a failure.
     * @param targetRoot        The sync target root, named in a failure.
     * @param sourceKernelNames The source's own @c kernel-classed module
     *                          names or submodule paths.
     * @param targetKernelNames The target's own @c kernel-classed module
     *                          names or submodule paths.
     * @param sourceFilePrefix  The source's own identity @c filePrefix
     *                          value.
     * @param targetFilePrefix  The target's own identity @c filePrefix
     *                          value.
     * @returns juce::Result::ok() when the two sets correspond, or a
     *          failure naming both roots and the first differing name, in
     *          ascending order.
     */
    static juce::Result isCorresponding (const juce::File& sourceRoot, const juce::File& targetRoot,
        const jam::HashSet<juce::String>& sourceKernelNames, const jam::HashSet<juce::String>& targetKernelNames,
        const juce::String& sourceFilePrefix, const juce::String& targetFilePrefix);
};
