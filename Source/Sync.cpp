#include "Sync.h"

juce::Result Sync::run (const juce::File& sourceRoot, const juce::File& targetRoot, const jam::MarkdownWriter& formatter)
{
    const auto sourceInfoFile { getInfoFile (sourceRoot) };
    const auto targetInfoFile { getInfoFile (targetRoot) };

    if (const auto result { isRootsDistinct (sourceRoot, targetRoot) }; not result.wasOk())
        return result;

    if (const auto result { isInfoPresent (sourceInfoFile) }; not result.wasOk())
        return result;

    if (const auto result { isInfoPresent (targetInfoFile) }; not result.wasOk())
        return result;

    const auto sourceInfo { Model::parse (sourceInfoFile) };
    const auto targetInfo { Model::parse (targetInfoFile) };

    if (const auto result {
            isComposedComplete (sourceInfoFile, targetInfoFile, *sourceInfo, *targetInfo) };
        not result.wasOk())
        return result;

    return runGates (sourceRoot, targetRoot, *sourceInfo, *targetInfo, formatter);
}

juce::File Sync::getInfoFile (const juce::File& root)
{
    return root.getChildFile (files::userModulesInfo);
}

juce::String Sync::getNormalizedPath (const juce::String& relativePath)
{
    return relativePath.replaceCharacter (Chars::backslash, Chars::slash);
}

juce::Result Sync::runGates (const juce::File& sourceRoot, const juce::File& targetRoot,
    const Model& sourceInfo, const Model& targetInfo, const jam::MarkdownWriter& formatter)
{
    static const auto filePrefixKeyText { Id::filePrefix.toString() };
    const auto sourceFilePrefix { getIdentityValue (sourceInfo, filePrefixKeyText) };
    const auto targetFilePrefix { getIdentityValue (targetInfo, filePrefixKeyText) };

    const auto sourceDeclaredNames (getDeclaredModuleNames (sourceInfo));
    const auto targetDeclaredNames (getDeclaredModuleNames (targetInfo));
    const auto sourceKernelNames (getKernelModuleNames (sourceInfo));
    const auto targetKernelNames (getKernelModuleNames (targetInfo));
    const auto sourceKernelSubmodulePaths (getKernelSubmodulePaths (sourceInfo));
    const auto targetKernelSubmodulePaths (getKernelSubmodulePaths (targetInfo));

    if (const auto result { isKernelDeclared (sourceRoot, targetRoot, sourceFilePrefix, targetFilePrefix,
            sourceDeclaredNames, targetDeclaredNames, sourceKernelNames, targetKernelNames) };
        not result.wasOk())
        return result;

    if (const auto result { isSubmoduleDeclared (sourceRoot, targetRoot, sourceInfo, targetInfo,
            sourceKernelNames, targetKernelNames, sourceKernelSubmodulePaths, targetKernelSubmodulePaths,
            sourceFilePrefix, targetFilePrefix) };
        not result.wasOk())
        return result;

    return runWalk (sourceRoot, targetRoot, sourceInfo, targetInfo, sourceFilePrefix, targetFilePrefix, formatter);
}

juce::Result Sync::isSubmoduleDeclared (const juce::File& sourceRoot, const juce::File& targetRoot,
    const Model& sourceInfo, const Model& targetInfo, const jam::HashSet<juce::String>& sourceKernelNames,
    const jam::HashSet<juce::String>& targetKernelNames,
    const jam::HashSet<juce::String>& sourceKernelSubmodulePaths,
    const jam::HashSet<juce::String>& targetKernelSubmodulePaths, const juce::String& sourceFilePrefix,
    const juce::String& targetFilePrefix)
{
    if (const auto result { isSubmoduleListed (sourceRoot,
            getDiskPaths (sourceRoot, sourceKernelNames, getIgnorePatterns (sourceInfo)),
            getDeclaredSubmodulePaths (sourceInfo), jam::HashSet<juce::String> {}) };
        not result.wasOk())
        return result;

    if (const auto result { isSubmoduleListed (targetRoot,
            getDiskPaths (targetRoot, targetKernelNames, getIgnorePatterns (targetInfo)),
            getDeclaredSubmodulePaths (targetInfo), targetKernelSubmodulePaths) };
        not result.wasOk())
        return result;

    return isCorresponding (sourceRoot, targetRoot, sourceKernelSubmodulePaths,
        targetKernelSubmodulePaths, sourceFilePrefix, targetFilePrefix);
}

juce::Result Sync::isKernelDeclared (const juce::File& sourceRoot, const juce::File& targetRoot,
    const juce::String& sourceFilePrefix, const juce::String& targetFilePrefix,
    const jam::HashSet<juce::String>& sourceDeclaredNames, const jam::HashSet<juce::String>& targetDeclaredNames,
    const jam::HashSet<juce::String>& sourceKernelNames, const jam::HashSet<juce::String>& targetKernelNames)
{
    if (const auto result { isKernelPresent (sourceRoot, sourceKernelNames) }; not result.wasOk())
        return result;

    if (const auto result { isKernelPresent (targetRoot, targetKernelNames) }; not result.wasOk())
        return result;

    if (const auto result {
            isPrefixDeclared (sourceRoot, sourceFilePrefix, sourceDeclaredNames) };
        not result.wasOk())
        return result;

    if (const auto result {
            isPrefixDeclared (targetRoot, targetFilePrefix, targetDeclaredNames) };
        not result.wasOk())
        return result;

    return isCorresponding (
        sourceRoot, targetRoot, sourceKernelNames, targetKernelNames, sourceFilePrefix, targetFilePrefix);
}

juce::Result Sync::runWalk (const juce::File& sourceRoot, const juce::File& targetRoot,
    const Model& sourceInfo, const Model& targetInfo, const juce::String& sourceFilePrefix,
    const juce::String& targetFilePrefix, const jam::MarkdownWriter& formatter)
{
    const auto identityRows (getIdentityRows (sourceInfo, targetInfo));
    const auto [sourceNamespace, targetNamespace] {
        getIdentityValues (sourceInfo, targetInfo, Id::tokenNamespace.toString()) };
    const auto namespaceSpacePair { getNamespaceSpacePair (sourceNamespace, targetNamespace) };
    const auto namespaceColonPair { getNamespaceColonPair (sourceNamespace, targetNamespace) };

    const auto sourceFiles (getSyncFiles (sourceRoot, sourceInfo));

    if (const auto result { runContamination (sourceFiles, sourceRoot, sourceInfo, targetInfo, identityRows,
            namespaceSpacePair, namespaceColonPair) };
        not result.wasOk())
        return result;

    return runTransform (sourceFiles, sourceRoot, targetRoot, sourceFilePrefix, targetFilePrefix, sourceInfo,
        targetInfo, identityRows, namespaceSpacePair, namespaceColonPair, formatter);
}

juce::Result Sync::runContamination (const jam::Array<juce::File>& sourceFiles, const juce::File& sourceRoot,
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

jam::Array<std::pair<juce::String, juce::String>> Sync::getSyncResults (const jam::Array<juce::File>& sourceFiles,
    const juce::File& sourceRoot, const juce::File& targetRoot, const juce::String& sourceFilePrefix,
    const juce::String& targetFilePrefix, const Model& sourceInfo, const Model& targetInfo,
    const jam::Array<const Model::Element*>& identityRows, const std::pair<juce::String, juce::String>& namespaceSpacePair,
    const std::pair<juce::String, juce::String>& namespaceColonPair, const jam::MarkdownWriter& formatter)
{
    jam::Array<std::pair<juce::String, juce::String>> syncResults;
    syncResults.resize (sourceFiles.size());

    Jobs::run (sourceFiles.size(),
        [&sourceFiles, &sourceRoot, &targetRoot, &sourceFilePrefix, &targetFilePrefix, &sourceInfo, &targetInfo,
            &identityRows, &namespaceSpacePair, &namespaceColonPair, &formatter,
            &syncResults] (int index)
        {
            syncResults.at (index) = getSyncOutcome (sourceFiles.at (index), sourceRoot, targetRoot,
                sourceFilePrefix, targetFilePrefix, sourceInfo, targetInfo, identityRows, namespaceSpacePair,
                namespaceColonPair, formatter);
        });

    return syncResults;
}

juce::Result Sync::runTransform (const jam::Array<juce::File>& sourceFiles, const juce::File& sourceRoot,
    const juce::File& targetRoot, const juce::String& sourceFilePrefix, const juce::String& targetFilePrefix,
    const Model& sourceInfo, const Model& targetInfo, const jam::Array<const Model::Element*>& identityRows,
    const std::pair<juce::String, juce::String>& namespaceSpacePair,
    const std::pair<juce::String, juce::String>& namespaceColonPair, const jam::MarkdownWriter& formatter)
{
    const auto syncResults (getSyncResults (sourceFiles, sourceRoot, targetRoot, sourceFilePrefix,
        targetFilePrefix, sourceInfo, targetInfo, identityRows, namespaceSpacePair, namespaceColonPair,
        formatter));

    jam::Array<juce::String> writtenPaths;

    for (const auto& [writtenPath, error] : syncResults)
    {
        if (error.isNotEmpty())
            return juce::Result::fail (error);

        if (writtenPath.isNotEmpty())
            writtenPaths.add (writtenPath);
    }

    printReport (writtenPaths);

    return juce::Result::ok();
}

juce::String Sync::getIdentityValue (const Model& info, const juce::String& key)
{
    return info.getValue (*info.getTableRow (Id::identity, juce::Identifier (key)), Id::value);
}

std::pair<juce::String, juce::String>
Sync::getIdentityValues (const Model& sourceInfo, const Model& targetInfo, const juce::String& key)
{
    return { getIdentityValue (sourceInfo, key), getIdentityValue (targetInfo, key) };
}

std::pair<juce::String, juce::String>
Sync::getNamespaceSpacePair (const juce::String& sourceValue, const juce::String& targetValue)
{
    static const auto namespaceSpaceText {
        Id::tokenNamespace.toString() + juce::String::charToString (Chars::space) };

    return { namespaceSpaceText + sourceValue, namespaceSpaceText + targetValue };
}

std::pair<juce::String, juce::String>
Sync::getNamespaceColonPair (const juce::String& sourceValue, const juce::String& targetValue)
{
    return { sourceValue + Id::doubleColon.toString(), targetValue + Id::doubleColon.toString() };
}

jam::Array<const Model::Element*> Sync::getIdentityRows (const Model& sourceInfo, const Model& targetInfo)
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

    std::stable_sort (rows.begin(), rows.end(),
        [&sourceInfo] (const Model::Element* first, const Model::Element* second)
        {
            const auto firstValue { sourceInfo.getValue (*first, Id::value) };
            const auto secondValue { sourceInfo.getValue (*second, Id::value) };

            return firstValue.length() != secondValue.length()
                       ? firstValue.length() > secondValue.length()
                       : firstValue.compare (secondValue) > 0;
        });

    jam::Array<const Model::Element*> sortedRows;

    for (auto* row : rows)
        if (sortedRows.isEmpty()
            or sourceInfo.getValue (*sortedRows.last(), Id::value).compare (sourceInfo.getValue (*row, Id::value))
                   != 0)
            sortedRows.add (row);

    return sortedRows;
}

std::pair<juce::String, juce::String>
Sync::getRowPair (const Model& sourceInfo, const Model& targetInfo, const Model::Element* row)
{
    const auto key { sourceInfo.getValue (*row, Id::key) };
    const auto sourceValue { sourceInfo.getValue (*row, Id::value) };
    const auto targetValue {
        targetInfo.getValue (*targetInfo.getTableRow (Id::identity, juce::Identifier (key)), Id::value) };

    return { sourceValue, targetValue };
}

bool Sync::isRowWordBoundary (const Model& sourceInfo, const Model::Element* row)
{
    static const auto wordBoundaryText { Id::word.toString() };

    return sourceInfo.getValue (*row, Id::syncBoundary).compare (wordBoundaryText) == 0;
}

int Sync::getPendingLength (bool pending, const juce::String& source) noexcept
{
    return pending ? source.length() : -1;
}

int Sync::getRowLength (const Model& sourceInfo, const jam::Array<const Model::Element*>& identityRows, int rowIndex)
{
    return rowIndex < identityRows.size() ? sourceInfo.getValue (*identityRows.at (rowIndex), Id::value).length()
                                           : -1;
}

juce::String Sync::getTransformedText (const juce::String& text, const Model& sourceInfo,
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

juce::String Sync::getContaminationToken (const juce::String& text, const Model& sourceInfo,
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

jam::HashSet<juce::String> Sync::getDeclaredModuleNames (const Model& info)
{
    jam::HashSet<juce::String> names;

    for (auto* row : info.getTableRows (Id::tokenModule))
        names.insert (info.getValue (*row, Id::name));

    return names;
}

jam::HashSet<juce::String> Sync::getKernelModuleNames (const Model& info)
{
    static const auto kernelClassText { Id::kernel.toString() };

    jam::HashSet<juce::String> names;

    for (auto* row : info.getTableRows (Id::tokenModule))
        if (info.getValue (*row, Id::codeClass).compare (kernelClassText) == 0)
            names.insert (info.getValue (*row, Id::name));

    return names;
}

jam::Array<juce::String> Sync::getIgnorePatterns (const Model& info)
{
    jam::Array<juce::String> patterns;

    for (auto* row : info.getTableRows (Id::ignore))
        patterns.add (info.getValue (*row, Id::value));

    return patterns;
}

juce::String Sync::getSubmodulePath (const Model& info, const Model::Element& row)
{
    static const auto slashText { juce::String::charToString (Chars::slash) };

    return info.getValue (row, Id::tokenModule) + slashText + info.getValue (row, Id::path);
}

jam::HashSet<juce::String> Sync::getDeclaredSubmodulePaths (const Model& info)
{
    jam::HashSet<juce::String> paths;

    for (auto* row : info.getTableRows (Id::submodule))
        paths.insert (getSubmodulePath (info, *row));

    return paths;
}

jam::HashSet<juce::String> Sync::getKernelSubmodulePaths (const Model& info)
{
    static const auto kernelClassText { Id::kernel.toString() };

    jam::HashSet<juce::String> paths;

    for (auto* row : info.getTableRows (Id::submodule))
        if (info.getValue (*row, Id::codeClass).compare (kernelClassText) == 0)
            paths.insert (getSubmodulePath (info, *row));

    return paths;
}

bool Sync::isIgnored (const juce::String& relativePath, const jam::Array<juce::String>& patterns)
{
    static constexpr bool ignoreCaseInMatch { false };

    for (const auto& pattern : patterns)
        if (relativePath.matchesWildcard (pattern, ignoreCaseInMatch))
            return true;

    return false;
}

jam::Array<juce::File> Sync::getKernelFiles (const juce::File& root, const jam::HashSet<juce::String>& kernelNames)
{
    jam::Array<juce::File> files;

    for (const auto& name : kernelNames)
        for (const auto& file : root.getChildFile (name).findChildFiles (juce::File::findFiles, true))
            files.add (file);

    return files;
}

jam::HashSet<juce::String> Sync::getDiskPaths (const juce::File& root,
    const jam::HashSet<juce::String>& kernelNames, const jam::Array<juce::String>& ignorePatterns)
{
    jam::HashSet<juce::String> paths;

    for (const auto& file : getKernelFiles (root, kernelNames))
    {
        const auto rootRelativePath { getNormalizedPath (file.getRelativePathFrom (root)) };

        if (not isIgnored (rootRelativePath, ignorePatterns))
            paths.insert (rootRelativePath);
    }

    return paths;
}

jam::Array<juce::File> Sync::getSyncFiles (const juce::File& root, const Model& info)
{
    static const auto kernelClassText { Id::kernel.toString() };

    jam::Array<juce::File> files;

    for (auto* row : info.getTableRows (Id::submodule))
        if (info.getValue (*row, Id::codeClass).compare (kernelClassText) == 0)
            files.add (root.getChildFile (getSubmodulePath (info, *row)));

    return files;
}

juce::String Sync::getNewlineNormalized (const juce::String& text)
{
    static const auto crlfText {
        juce::String::charToString (Chars::carriageReturn) + juce::String::charToString (Chars::newline) };
    static const auto newlineText { juce::String::charToString (Chars::newline) };

    return text.replace (crlfText, newlineText);
}

juce::String Sync::getCanonicalMarkdown (const juce::String& text, const jam::MarkdownWriter& formatter)
{
    const auto document { jam::MarkdownDocument::parse (text) };
    return formatter.getText (document);
}

juce::String Sync::getKernelScopeName (const juce::String& relativePath)
{
    static const auto slashText { juce::String::charToString (Chars::slash) };

    return relativePath.upToFirstOccurrenceOf (slashText, false, false);
}

bool Sync::hasBinaryMarker (const juce::MemoryBlock& data) noexcept
{
    static constexpr size_t binaryScanLimit { 8000 };

    const auto* bytes { static_cast<const char*> (data.getData()) };
    const auto scanLength { std::min (data.getSize(), binaryScanLimit) };

    for (size_t index { 0 }; index < scanLength; ++index)
        if (bytes[index] == static_cast<char> (Chars::nullCharacter))
            return true;

    return false;
}

juce::String Sync::getContaminationCheck (const juce::File& sourceFile, const juce::File& sourceRoot,
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

juce::String Sync::getWriteFailure (const juce::File& targetFile)
{
    return targetFile.getFullPathName() + Id::diagnosticSeparator + text::Diagnostics::failOutputWrite;
}

juce::String Sync::getCaseReconciliationFailure (const juce::File& targetFile)
{
    for (const auto& sibling : targetFile.getParentDirectory().findChildFiles (juce::File::findFiles, false))
        if (sibling.getFileName().compareIgnoreCase (targetFile.getFileName()) == 0
            and sibling.getFileName().compare (targetFile.getFileName()) != 0)
            if (not sibling.moveFileTo (targetFile))
                return getWriteFailure (targetFile);

    return juce::String {};
}

std::pair<juce::String, juce::String> Sync::getBinaryWriteOutcome (const juce::File& targetFile,
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

std::pair<juce::String, juce::String> Sync::getTextWriteOutcome (const juce::File& targetFile,
    const juce::String& targetRelativePath, const juce::String& textContent)
{
    static const auto newlineText { juce::String::charToString (Chars::newline) };

    if (targetFile.loadFileAsString().compare (textContent) == 0)
        return { juce::String {}, juce::String {} };

    return targetFile.replaceWithText (textContent, false, false, newlineText.toRawUTF8())
               ? std::make_pair (targetRelativePath, juce::String {})
               : std::make_pair (juce::String {}, getWriteFailure (targetFile));
}

std::pair<juce::String, juce::String> Sync::getWriteOutcome (const juce::File& targetFile,
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

jam::Strings Sync::getRegionKeptLines (const jam::Strings& lines, const jam::Strings& targetLines,
    const juce::String& beginValue, const juce::String& endValue)
{
    const auto [sourceBegin, sourceEnd] { Transforms::getDelimiterLines (lines, beginValue, endValue) };
    const auto [targetBegin, targetEnd] { Transforms::getDelimiterLines (targetLines, beginValue, endValue) };

    if (sourceEnd >= 0 and targetEnd >= 0)
        return Transforms::getSplicedLines (lines, sourceBegin, sourceEnd, targetLines, targetBegin + 1, targetEnd);

    return lines;
}

juce::String Sync::getRegionKeptText (
    const juce::String& text, const juce::File& targetFile, const Model& targetInfo)
{
    if (targetFile.existsAsFile())
    {
        const auto targetLines (jam::Strings::fromLines (targetFile.loadFileAsString()));
        auto lines (jam::Strings::fromLines (text));

        for (auto* row : targetInfo.getTableRows (Id::region))
            lines = getRegionKeptLines (
                lines, targetLines, targetInfo.getValue (*row, Id::begin), targetInfo.getValue (*row, Id::end));

        return Transforms::getJoinedText (lines, text);
    }

    return text;
}

std::pair<juce::String, juce::String> Sync::getSyncOutcome (const juce::File& sourceFile,
    const juce::File& sourceRoot, const juce::File& targetRoot, const juce::String& sourceFilePrefix,
    const juce::String& targetFilePrefix, const Model& sourceInfo, const Model& targetInfo,
    const jam::Array<const Model::Element*>& identityRows, const std::pair<juce::String, juce::String>& namespaceSpacePair,
    const std::pair<juce::String, juce::String>& namespaceColonPair, const jam::MarkdownWriter& formatter)
{
    const auto rootRelativePath { getNormalizedPath (sourceFile.getRelativePathFrom (sourceRoot)) };
    const auto targetRelativePath { Transforms::getTransformedPath (rootRelativePath, sourceFilePrefix, targetFilePrefix) };
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
        transformed = getCanonicalMarkdown (transformed, formatter);

    transformed = getRegionKeptText (transformed, targetFile, targetInfo);

    return getWriteOutcome (targetFile, targetRelativePath, false, transformed, juce::MemoryBlock {});
}

jam::Array<juce::String> Sync::getReportLines (const jam::Array<juce::String>& paths)
{
    jam::Array<juce::String> filtered;

    for (const auto& path : paths)
        if (path.isNotEmpty())
            filtered.add (path);

    filtered.sort();
    return filtered;
}

void Sync::printReport (const jam::Array<juce::String>& writtenPaths)
{
    for (const auto& path : getReportLines (writtenPaths))
        printf ("%s\n", path.toRawUTF8());
}

juce::Result Sync::isRootsDistinct (const juce::File& sourceRoot, const juce::File& targetRoot)
{
    if (sourceRoot == targetRoot)
        return juce::Result::fail (sourceRoot.getFullPathName()
                                   + Id::diagnosticSeparator + text::Diagnostics::failSyncRoot);

    return juce::Result::ok();
}

juce::Result Sync::isInfoPresent (const juce::File& infoFile)
{
    if (not infoFile.existsAsFile())
        return juce::Result::fail (infoFile.getFullPathName()
                                   + Id::diagnosticSeparator + text::Diagnostics::failSyncInfo);

    return juce::Result::ok();
}

juce::Result Sync::isComposedComplete (const juce::File& sourceInfoFile, const juce::File& targetInfoFile,
    const Model& sourceInfo, const Model& targetInfo)
{
    static const jam::Strings composedKeys { Id::tokenNamespace.toString(), Id::filePrefix.toString(),
        Id::macroPrefix.toString() };

    for (const auto& key : composedKeys)
    {
        const juce::Identifier keyId { key };

        if (sourceInfo.getTableRow (Id::identity, keyId) == nullptr)
            return juce::Result::fail (sourceInfoFile.getFullPathName() + Id::diagnosticSeparator
                                       + key + Id::diagnosticSeparator + text::Diagnostics::failSyncIdentity);

        if (targetInfo.getTableRow (Id::identity, keyId) == nullptr)
            return juce::Result::fail (targetInfoFile.getFullPathName() + Id::diagnosticSeparator
                                       + key + Id::diagnosticSeparator + text::Diagnostics::failSyncIdentity);
    }

    return juce::Result::ok();
}

juce::Result Sync::isKernelPresent (const juce::File& root, const jam::HashSet<juce::String>& kernelNames)
{
    jam::Array<juce::String> missingNames;

    for (const auto& name : kernelNames)
        if (not root.getChildFile (name).isDirectory())
            missingNames.add (name);

    missingNames.sort();

    if (missingNames.isEmpty())
        return juce::Result::ok();

    return juce::Result::fail (root.getFullPathName() + Id::diagnosticSeparator + missingNames.at (0)
                               + Id::diagnosticSeparator + text::Diagnostics::failSyncModule);
}

juce::Result Sync::isPrefixDeclared (const juce::File& root, const juce::String& filePrefix,
    const jam::HashSet<juce::String>& declaredNames)
{
    for (const auto& directory : root.findChildFiles (juce::File::findDirectories, false))
    {
        const auto directoryName { directory.getFileName() };

        if (directoryName.startsWith (filePrefix) and not declaredNames.contains (directoryName))
            return juce::Result::fail (root.getFullPathName() + Id::diagnosticSeparator + directoryName
                                       + Id::diagnosticSeparator + text::Diagnostics::failSyncModule);
    }

    return juce::Result::ok();
}

juce::Result Sync::isSubmoduleListed (const juce::File& root, const jam::HashSet<juce::String>& diskPaths,
    const jam::HashSet<juce::String>& declaredPaths, const jam::HashSet<juce::String>& writablePaths)
{
    jam::Array<juce::String> differences;

    for (const auto& path : diskPaths)
        if (not declaredPaths.contains (path))
            differences.add (path);

    for (const auto& path : declaredPaths)
        if (not diskPaths.contains (path) and not writablePaths.contains (path))
            differences.add (path);

    differences.sort();

    if (differences.isEmpty())
        return juce::Result::ok();

    return juce::Result::fail (root.getFullPathName() + Id::diagnosticSeparator + differences.at (0)
                               + Id::diagnosticSeparator + text::Diagnostics::failSyncSubmodule);
}

juce::Result Sync::isCorresponding (const juce::File& sourceRoot, const juce::File& targetRoot,
    const jam::HashSet<juce::String>& sourceKernelNames, const jam::HashSet<juce::String>& targetKernelNames,
    const juce::String& sourceFilePrefix, const juce::String& targetFilePrefix)
{
    jam::HashSet<juce::String> transformedSourceNames;

    for (const auto& name : sourceKernelNames)
        transformedSourceNames.insert (Transforms::getTransformedPath (name, sourceFilePrefix, targetFilePrefix));

    jam::Array<juce::String> differences;

    for (const auto& name : transformedSourceNames)
        if (not targetKernelNames.contains (name))
            differences.add (name);

    for (const auto& name : targetKernelNames)
        if (not transformedSourceNames.contains (name))
            differences.add (name);

    differences.sort();

    if (differences.isEmpty())
        return juce::Result::ok();

    return juce::Result::fail (sourceRoot.getFullPathName() + Chars::space + targetRoot.getFullPathName()
                               + Id::diagnosticSeparator + text::Diagnostics::failSyncCorrespondence
                               + Id::diagnosticSeparator + differences.at (0));
}
