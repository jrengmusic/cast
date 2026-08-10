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
    return sourceFile + juce::String::charToString (chars::colon) + juce::String (rowNumber) + juce::String::charToString (chars::space) + juce::String::charToString (chars::openParen)
           + columnName + juce::String::charToString (chars::closeParen);
}

/**
 * @brief Resolves a row key to its 1-based physical file line number.
 *
 * Reads the row's own byte offset (jam::MarkdownDocument stamps every table
 * row with Id::offset at parse time) via a keyed getTableRow() probe, then
 * resolves it to a line number against jam::Document's line-start cache
 * (built once per parse()) -- O(1) probe plus a binary search, zero
 * allocation, no re-scan of the source text.
 *
 * @param root      The parsed relation document.
 * @param tableName The relation (table) name.
 * @param rowKey    The row's column-0 key.
 * @return The row's 1-based physical file line number.
 */
static int
getRowLineNumber (const jam::MarkdownDocument& root, const juce::String& tableName, const juce::String& rowKey)
{
    const auto* row { root.getTableRow (juce::Identifier (tableName), juce::Identifier (rowKey)) };
    jassert (row != nullptr);

    const auto offset { static_cast<uint32_t> (*row->get<int> (Id::offset)) };
    return static_cast<int> (root.getLineNumber (offset));
}

/**
 * @brief SPEC (Canon Files): reports whether a row is a visual separator.
 *
 * A row whose column-0 key consists solely of dash characters (one or
 * more `-`, nothing else) is authored for readability, not data -- GFM
 * parses it as an ordinary row, but CAST skips it everywhere a row is
 * enumerated: the lexicon registry, dispatch row-matching, and
 * uniqueness/constraint scans.
 *
 * @param rowKey The row's column-0 key.
 * @return True when @p rowKey is a non-empty run of dash characters.
 */
static bool isSeparatorRow (const juce::String& rowKey) noexcept
{
    return rowKey.isNotEmpty() and rowKey.containsOnly (juce::String::charToString (chars::dash));
}

static constexpr int maxLexiconNameLength { 40 };

/** @brief SPEC (Canon Files): reports whether @p text exceeds maxLexiconNameLength. */
static bool isTooLong (const juce::String& text) noexcept
{
    return text.length() > maxLexiconNameLength;
}

/**
 * @brief SPEC (Canon Files): validates a lexicon `name` column entry.
 *
 * FATAL when the name is a plain number, starts with a digit, contains a
 * character outside [a-zA-Z0-9 space], or exceeds maxLexiconNameLength.
 * The returned failure carries no location prefix -- the caller wraps
 * getErrorMessage() with a lazily computed SPEC §8 location.
 *
 * @param name The lexicon entry's row key (its declared name).
 * @return juce::Result::ok() when @p name satisfies every rule; otherwise
 *         the first violated rule's failure.
 */
static juce::Result validateLexiconName (const juce::String& name)
{
    const auto isDigit { [] (juce::juce_wchar character) noexcept
                         { return character >= chars::zero and character <= chars::nine; } };
    const auto isNameChar { [&isDigit] (juce::juce_wchar character) noexcept
                            { return isDigit (character)
                                     or (character >= chars::lowerA and character <= chars::lowerZ)
                                     or (character >= chars::upperA and character <= chars::upperZ)
                                     or character == chars::space; } };

    if (std::all_of (name.begin(), name.end(), isDigit))
        return juce::Result::fail (text::en::failNameNumeric + Id::diagnosticSeparator + name);

    if (isDigit (name[0]))
        return juce::Result::fail (text::en::failNameLeadingDigit + Id::diagnosticSeparator + name);

    if (not std::all_of (name.begin(), name.end(), isNameChar))
        return juce::Result::fail (text::en::failNameInvalidChar + Id::diagnosticSeparator + name);

    if (isTooLong (name))
        return juce::Result::fail (text::en::failTooLong + juce::String (maxLexiconNameLength)
                                   + text::en::failTooLongSuffix + Id::diagnosticSeparator + name);

    return juce::Result::ok();
}


/**
 * @brief SPEC (Canon Files): FATAL when a lexicon value byte-equals a
 *        case-family projection of its own name.
 *
 * The value is then redundant -- the author should delete it and let the
 * template project the name via a transform tag instead. Checks all six
 * case-family projections (toTitle, toPascal, toCamel, toKebab, toSnake,
 * toScreamingSnake) via the registered Transforms::.
 *
 * @param name  The lexicon entry's declared name.
 * @param value The entry's stored, non-empty, non-backtick-literal value.
 * @return juce::Result::ok() when @p value matches none of the six
 *         projections; otherwise the first match's failure, naming the
 *         matching transform. The returned failure carries no location
 *         prefix -- the caller wraps getErrorMessage() with a lazily
 *         computed SPEC §8 location.
 */
static juce::Result
validateLexiconValueRedundancy (const juce::String& name, const juce::String& value)
{
    for (const auto& transformName :
        { Id::toTitle.toString(), Id::toPascal.toString(), Id::toCamel.toString(), Id::toKebab.toString(),
          Id::toSnake.toString(), Id::toScreamingSnake.toString() })
        if (value == Transforms::getTransformed (transformName, name))
            return juce::Result::fail (text::en::failRedundantValue + transformName + text::en::failRedundantValueSuffix);

    return juce::Result::ok();
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
                .fromFirstOccurrenceOf (juce::String::charToString (chars::space), false, false)
                .trim()
        };

        if (predicateArgs.containsChar (chars::dot))
            targetTables.addIfNotAlreadyThere (
                predicateArgs.upToFirstOccurrenceOf (juce::String::charToString (chars::dot), false, false));
    }

    return targetTables;
}

/**
 * @brief Reads one named column's cell text from a `## dispatch` row.
 *
 * `## dispatch` rows are addressed by Element*, not by key: the same key
 * legitimately labels more than one row (a table dispatched twice, once per
 * template). Resolves @p columnId's position in the dispatch table's header
 * row, then reads the cell at that position out of @p row, following
 * buildPerRowMap()'s cells-array-plus-bounds-guard pattern.
 *
 * @param manifestDoc The parsed manifest, supplying the `## dispatch` headers.
 * @param row         The dispatch row to read (see jam::MarkdownDocument::getTableRows()).
 * @param columnId    The column to read.
 * @return The cell's plain text, or an empty string when @p columnId is not a
 *         dispatch column or @p row has fewer cells than that column's index.
 */
static juce::String getDispatchCell (const jam::MarkdownDocument& manifestDoc,
                                     const jam::Document::Element& row,
                                     const juce::Identifier& columnId)
{
    const auto headers { manifestDoc.getTableHeaders (Id::dispatch) };
    const auto columnIndex { headers.indexOf (columnId.toString()) };

    jam::Array<jam::Document::Element*> cells;

    for (auto* cell : row)
        cells.add (cell);

    return columnIndex >= 0 and columnIndex < cells.size()
               ? cells.at (columnIndex)->getAllSubText()
               : juce::String();
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

static const juce::String regionBeginSuffix { ":begin@" };
static const juce::String regionEndSuffix { ":end@" };
static const juce::String regionBeginKeySuffix { ":begin" };
static const juce::String regionEndKeySuffix { ":end" };
static const std::regex regionBeginPattern { "@([A-Za-z][A-Za-z0-9]*):begin@" };

/// @brief One `@\<name\>:begin@` occurrence found while scanning a root or fragment template.
struct RegionMarkerOccurrence
{
    juce::String name;
    int index;
};

/// @brief A root-owned slot region: the root that declares it, and its per-row body text.
struct SlotRegion
{
    juce::String rootTemplatePath;
    juce::String body;
};

using SlotRegionMap = jam::HashMap<juce::String, SlotRegion>;

/**
 * @brief Resolves a byte offset in plain (non-markdown) template text to its 1-based line number.
 *
 * @param text       The template text.
 * @param charOffset The 0-based character offset within @p text.
 * @return The 1-based line number containing @p charOffset.
 */
static int getTextLineNumber (const juce::String& text, int charOffset) noexcept
{
    return juce::jmax (1, juce::StringArray::fromLines (text.substring (0, charOffset)).size());
}

/**
 * @brief Finds every `@\<name\>:begin@` region-begin marker in @p text.
 *
 * @param text The template text to scan.
 * @return Every marker found, each carrying its captured name and byte offset.
 */
static jam::Array<RegionMarkerOccurrence> findRegionBeginMarkers (const juce::String& text)
{
    jam::Array<RegionMarkerOccurrence> markers;
    const std::string source { text.toStdString() };

    for (auto match { std::sregex_iterator (source.begin(), source.end(), regionBeginPattern) };
         match != std::sregex_iterator();
         ++match)
        markers.add ({ juce::String (match->str (1)), static_cast<int> (match->position (0)) });

    return markers;
}

/**
 * @brief Engine feature: locates every root-owned slot region and validates its declaration.
 *
 * Rule 1: a region's name must be a slot declared in `## dispatch`'s placeholder column.
 * Rule 4 (root half): a slot named `row` is reserved.
 * Rule 5: every declared slot must appear, in exactly one root, either as a plain `@slot@`
 * placeholder or as a region.
 *
 * @param manifestDoc  The parsed manifest.
 * @param manifestFile The manifest file, for error locations.
 * @param dir          The manifest's parent directory.
 * @param slotNames    Every distinct slot name declared across `## dispatch`, in manifest order.
 * @param slotRegions  Out parameter: every region found, keyed by slot name.
 * @return juce::Result::ok() on success, or the first SPEC §8 failure.
 */
static juce::Result collectSlotRegions (const jam::MarkdownDocument& manifestDoc,
                                        const juce::File& manifestFile,
                                        const juce::File& dir,
                                        const jam::Array<juce::String>& slotNames,
                                        SlotRegionMap& slotRegions)
{
    for (const auto& slotName : slotNames)
        if (slotName == Id::row.toString())
            return juce::Result::fail (manifestFile.getFullPathName() + Id::diagnosticSeparator
                                       + text::en::failRowReserved + Id::diagnosticSeparator + slotName);

    jam::Array<juce::String> rootPaths;

    for (const auto& outputKey : manifestDoc.getTableRowKeys (Id::generated))
        rootPaths.addIfNotAlreadyThere (
            manifestDoc.getTableValue (Id::generated, Id::templatePath, outputKey));

    jam::HashMap<juce::String, int> slotRootCount;

    for (const auto& rootPath : rootPaths)
    {
        const auto rootFile { dir.getChildFile (rootPath) };
        const auto rootText { rootFile.loadFileAsString() };

        jam::Array<juce::String> slotsSeenInThisRoot;
        jam::Array<juce::Range<int>> regionSpans;///< Full line spans of every collected region, for the plain-placeholder scan.
        int regionBodyEnd { 0 };///< Markers before this offset sit inside a collected region's body -- fragment-domain, not root slots.

        for (const auto& marker : findRegionBeginMarkers (rootText))
        {
            if (marker.index < regionBodyEnd)
                continue;

            if (marker.name == Id::row.toString())
                return juce::Result::fail (
                    getLocation (rootFile.getFullPathName(), getTextLineNumber (rootText, marker.index), marker.name)
                    + Id::diagnosticSeparator + text::en::failRowReserved + Id::diagnosticSeparator + marker.name);

            if (not slotNames.contains (marker.name))
                return juce::Result::fail (
                    getLocation (rootFile.getFullPathName(), getTextLineNumber (rootText, marker.index), marker.name)
                    + Id::diagnosticSeparator + text::en::failSlotUnknown + Id::diagnosticSeparator + marker.name);

            if (slotsSeenInThisRoot.contains (marker.name))
                continue;

            slotsSeenInThisRoot.add (marker.name);
            slotRootCount.addOrReplace (marker.name,
                                        (slotRootCount.contains (marker.name) ? slotRootCount.at (marker.name) : 0) + 1);

            const auto endMarkerText { juce::String::charToString (chars::at) + marker.name + regionEndSuffix };
            const auto beginIndex { marker.index };
            const auto bodyStart { rootText.indexOfChar (beginIndex, chars::newline) + 1 };
            const auto endIndex { rootText.indexOf (bodyStart, endMarkerText) };

            jassert (endIndex >= bodyStart);

            regionBodyEnd = endIndex + endMarkerText.length();

            const auto bodyEnd { rootText.substring (0, endIndex).lastIndexOfChar (chars::newline) + 1 };
            const auto body { rootText.substring (bodyStart, bodyEnd) };

            const auto spanStart { rootText.substring (0, beginIndex).lastIndexOfChar (chars::newline) + 1 };
            const auto afterEndLine { rootText.indexOfChar (endIndex, chars::newline) };
            const auto spanEnd { afterEndLine < 0 ? rootText.length() : afterEndLine + 1 };

            regionSpans.add ({ spanStart, spanEnd });
            slotRegions.insert ({ marker.name, { rootPath, body } });
        }

        // Rule 5's plain-placeholder scan reads the root with every region (markers
        // and body alike) removed -- a region body is fragment-domain, so a plain
        // @name@ inside one is a column placeholder, never a root slot.
        juce::String rootOwnText;
        int copyFrom { 0 };

        for (const auto& span : regionSpans)
        {
            rootOwnText += rootText.substring (copyFrom, span.getStart());
            copyFrom = span.getEnd();
        }

        rootOwnText += rootText.substring (copyFrom);

        for (const auto& slotName : slotNames)
        {
            const auto plainMarker { juce::String::charToString (chars::at) + slotName
                                    + juce::String::charToString (chars::at) };

            if (not slotsSeenInThisRoot.contains (slotName) and rootOwnText.contains (plainMarker))
                slotRootCount.addOrReplace (slotName,
                                            (slotRootCount.contains (slotName) ? slotRootCount.at (slotName) : 0) + 1);
        }
    }

    for (const auto& slotName : slotNames)
    {
        const auto count { slotRootCount.contains (slotName) ? slotRootCount.at (slotName) : 0 };

        if (count == 0)
            return juce::Result::fail (manifestFile.getFullPathName() + Id::diagnosticSeparator
                                       + text::en::failSlotMissing + Id::diagnosticSeparator + slotName);

        if (count > 1)
            return juce::Result::fail (manifestFile.getFullPathName() + Id::diagnosticSeparator
                                       + text::en::failSlotAmbiguous + Id::diagnosticSeparator + slotName);
    }

    return juce::Result::ok();
}

/**
 * @brief Engine feature rule 3: every dispatch row must be fed by exactly one source.
 *
 * A row with an empty `template` cell is fed by its slot's region; a row with a
 * non-empty `template` cell is fed by its fragment file. FATAL when a slot has no
 * region and some row's `template` cell is empty ("neither"), or when a slot has a
 * region that no row ever feeds because every row for that slot carries a
 * non-empty `template` cell ("both").
 *
 * @param manifestDoc  The parsed manifest.
 * @param manifestFile The manifest file, for error locations.
 * @param dispatchRows Rows of the manifest's `## dispatch` table.
 * @param slotRegions  Every root-owned slot region, keyed by slot name.
 * @return juce::Result::ok() on success, or the first SPEC §8 failure.
 */
static juce::Result validateDispatchRegionPairing (const jam::MarkdownDocument& manifestDoc,
                                                    const juce::File& manifestFile,
                                                    const jam::Array<jam::Document::Element*>& dispatchRows,
                                                    const SlotRegionMap& slotRegions)
{
    jam::Array<juce::String> slotNames;
    jam::Array<juce::String> slotsWithEmptyTemplate;
    jam::Array<juce::String> slotsWithNonEmptyTemplate;
    jam::HashMap<juce::String, jam::Document::Element*> firstEmptyRow;
    jam::HashMap<juce::String, jam::Document::Element*> firstNonEmptyRow;

    for (auto* dispatchRow : dispatchRows)
    {
        const auto slotName { getDispatchCell (manifestDoc, *dispatchRow, Id::placeholder) };
        const auto templateCell { getDispatchCell (manifestDoc, *dispatchRow, Id::templatePath) };

        slotNames.addIfNotAlreadyThere (slotName);

        if (templateCell.isEmpty())
        {
            slotsWithEmptyTemplate.addIfNotAlreadyThere (slotName);

            if (not firstEmptyRow.contains (slotName))
                firstEmptyRow.insert ({ slotName, dispatchRow });
        }
        else
        {
            slotsWithNonEmptyTemplate.addIfNotAlreadyThere (slotName);

            if (not firstNonEmptyRow.contains (slotName))
                firstNonEmptyRow.insert ({ slotName, dispatchRow });
        }
    }

    for (const auto& slotName : slotNames)
    {
        const auto isRegionOwned { slotRegions.contains (slotName) };
        const auto hasEmpty { slotsWithEmptyTemplate.contains (slotName) };
        const auto hasNonEmpty { slotsWithNonEmptyTemplate.contains (slotName) };

        if (hasEmpty and not isRegionOwned)
        {
            const auto* row { firstEmptyRow.at (slotName) };
            const auto rowOffset { static_cast<uint32_t> (*row->get<int> (Id::offset)) };
            const auto rowLine { static_cast<int> (manifestDoc.getLineNumber (rowOffset)) };

            return juce::Result::fail (
                getLocation (manifestFile.getFullPathName(), rowLine, Id::templatePath.toString())
                + Id::diagnosticSeparator + text::en::failRegionSourceMissing + Id::diagnosticSeparator + slotName);
        }

        if (isRegionOwned and hasNonEmpty and not hasEmpty)
        {
            const auto* row { firstNonEmptyRow.at (slotName) };
            const auto rowOffset { static_cast<uint32_t> (*row->get<int> (Id::offset)) };
            const auto rowLine { static_cast<int> (manifestDoc.getLineNumber (rowOffset)) };

            return juce::Result::fail (
                getLocation (manifestFile.getFullPathName(), rowLine, Id::templatePath.toString())
                + Id::diagnosticSeparator + text::en::failRegionUnfed + Id::diagnosticSeparator + slotName);
        }
    }

    return juce::Result::ok();
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
                     getLocation (sourceFile, getRowLineNumber (root, tableName, rowKey), columnName)
                     + Id::diagnosticSeparator + Id::matches.toString() + Id::diagnosticSeparator
                     + text::en::failNoMatch + Id::diagnosticSeparator + value);
}

/**
 * @brief SPEC §7.2 `unique` — key uniqueness across the declared column set.
 *
 * The constrained column is always a table's row-keying column (column 0 --
 * a row's own id is its column-0 cell's trimmed text, per
 * jam::MarkdownDocument's table convention), so duplicate detection is a
 * keyed getTableRow() probe rather than a value scan: iterate every table of
 * every relation that declares the constrained column (dispatched or not,
 * in authored scan order) and, for each row, probe first its own table then
 * every earlier-scanned table for a prior row keyed identically. Canon-table
 * duplicates (the lexicon/chars/files registry union) are already FATALed
 * during registry construction (Generator::run()) and are not re-detected
 * here.
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
    struct ScannedTable
    {
        const jam::MarkdownDocument* document;
        const jam::Document::Element* table;
        juce::String sourceFile;
    };

    jam::Array<ScannedTable> constrainedTables;

    for (const auto& root : roots)
    {
        const auto sourceFile { *root.root->get<juce::String> (Id::path) };

        for (auto* table : root.getTables())
            if (root.getTableHeaders (table->id).contains (columnName))
                constrainedTables.add ({ &root, table, sourceFile });
    }

    const auto reportDuplicate = [&columnName] (const ScannedTable& laterTable,
                                                const jam::Document::Element& laterRow,
                                                const ScannedTable& earlierTable,
                                                const jam::Document::Element& earlierRow)
    {
        return juce::Result::fail (
            getLocation (laterTable.sourceFile,
                        getRowLineNumber (*laterTable.document, laterTable.table->id.toString(), laterRow.id.toString()),
                        columnName)
            + Id::diagnosticSeparator + text::en::failDuplicate + laterRow.id.toString()
            + text::en::failAlreadyDeclared
            + getLocation (earlierTable.sourceFile,
                          getRowLineNumber (*earlierTable.document, earlierTable.table->id.toString(), earlierRow.id.toString()),
                          columnName));
    };

    for (int tableIndex { 0 }; tableIndex < constrainedTables.size(); ++tableIndex)
    {
        const auto& scanned { constrainedTables.at (tableIndex) };

        for (auto* row : *scanned.table)
        {
            if (row->isTag (Id::headerRow) or isSeparatorRow (row->id.toString()))
                continue;

            const auto* withinTableFirst { scanned.document->getTableRow (scanned.table->id, row->id) };

            if (withinTableFirst != row)
                return reportDuplicate (scanned, *row, scanned, *withinTableFirst);

            for (int earlierIndex { 0 }; earlierIndex < tableIndex; ++earlierIndex)
            {
                const auto& earlierTable { constrainedTables.at (earlierIndex) };
                const auto* crossTableMatch { earlierTable.document->getTableRow (earlierTable.table->id, row->id) };

                if (crossTableMatch != nullptr)
                    return reportDuplicate (scanned, *row, earlierTable, *crossTableMatch);
            }
        }
    }

    return juce::Result::ok();
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
    const auto targetTableId { args.upToFirstOccurrenceOf (juce::String::charToString (chars::dot), false, false) };
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
                     getLocation (sourceFile, getRowLineNumber (root, tableName, rowKey), columnName)
                     + Id::diagnosticSeparator + Id::existsIn.toString() + Id::diagnosticSeparator
                     + text::en::failForeignKeyMissing + args + Id::diagnosticSeparator + value);
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
        juce::StringArray::fromTokens (args, juce::String::charToString (chars::pipe), juce::String()).contains (value)
    };

    return matched
               ? juce::Result::ok()
               : juce::Result::fail (
                     getLocation (sourceFile, getRowLineNumber (root, tableName, rowKey), columnName)
                     + Id::diagnosticSeparator + Id::oneOf.toString() + Id::diagnosticSeparator
                     + text::en::failNotInSet + args + juce::String::charToString (chars::closeBrace) + Id::diagnosticSeparator
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
    const auto minColumn { args.upToFirstOccurrenceOf (juce::String::charToString (chars::space), false, false) };
    const auto maxColumn { args.fromFirstOccurrenceOf (juce::String::charToString (chars::space), false, false).trim() };

    const auto tableId { juce::Identifier (tableName) };
    const auto lowText { root.getTableValue (tableId, juce::Identifier (minColumn), rowKey) };
    const auto highText { root.getTableValue (tableId, juce::Identifier (maxColumn), rowKey) };

    const auto lowValue { lowText.getDoubleValue() };
    const auto highValue { highText.getDoubleValue() };
    const auto numericValue { value.getDoubleValue() };

    return (numericValue >= lowValue and numericValue <= highValue)
               ? juce::Result::ok()
               : juce::Result::fail (
                     getLocation (sourceFile, getRowLineNumber (root, tableName, rowKey), columnName)
                     + Id::diagnosticSeparator + Id::range.toString() + Id::diagnosticSeparator
                     + value + text::en::failOutOfRange + juce::String (lowValue) + juce::String::charToString (chars::comma)
                     + juce::String::charToString (chars::space) + juce::String (highValue) + juce::String::charToString (chars::closeBracket));
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
    const auto targetTableId { args.upToFirstOccurrenceOf (juce::String::charToString (chars::dot), false, false) };
    const auto targetColumn { args.fromFirstOccurrenceOf (juce::String::charToString (chars::dot), false, false) };
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

            for (const auto& targetRowKey : targetRowKeys)
            {
                if (isSeparatorRow (targetRowKey))
                    continue;

                targetValues.add (
                    root.getTableValue (targetTableIdentifier, targetColumnIdentifier, targetRowKey));
                targetLocations.add (
                    getLocation (candidateFile, getRowLineNumber (root, targetTableId, targetRowKey), targetColumn));
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

            for (const auto& rowKey : rowKeys)
            {
                if (isSeparatorRow (rowKey))
                    continue;

                const auto value { candidate.getTableValue (candidateTableId, columnId, rowKey) };

                localValues.addIfNotAlreadyThere (value);

                if (missingLocalValue.isEmpty() and not targetValues.contains (value))
                {
                    missingLocalValue = value;
                    missingLocalLocation =
                        getLocation (candidateFile, getRowLineNumber (candidate, candidateTableName, rowKey), columnName);
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
                                   + text::en::failLocalMissing + Id::diagnosticSeparator
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
                               + text::en::failRefMissing + Id::diagnosticSeparator + *missingTarget);
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
                     getLocation (sourceFile, getRowLineNumber (root, tableName, rowKey), columnName)
                     + Id::diagnosticSeparator + Id::fileExists.toString() + Id::diagnosticSeparator
                     + text::en::failNotFound + Id::diagnosticSeparator + target.getFullPathName());
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
            jam::Array<juce::String> rowKeysInOrder;

            for (const auto& rowKey : candidate.getTableRowKeys (candidateTableId))
                if (not isSeparatorRow (rowKey))
                    rowKeysInOrder.add (rowKey);

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
                badGroupLocation = getLocation (
                    candidateFile, getRowLineNumber (candidate, candidateTableName, *firstMatch), columnName);
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
                                     + text::en::failGroupOpen + badGroupName + text::en::failGroupClose);
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
        return juce::Result::fail (sourceFile + Id::diagnosticSeparator + text::en::failUnknownPredicate
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
            if (isSeparatorRow (rowKey))
                continue;

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
            if (node.isTag (Id::code))
                return false;

            if (node.isTag (Id::a))
                hazard = text::en::failHazardUri;
            else if (node.isTag (Id::text)
                     and node.get<juce::String> (Id::text)->containsAnyOf (Id::hazardChars))
                hazard = text::en::failHazardAngleBrackets;

            return true;
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

        for (const auto& rowKey : rowKeys)
        {
            if (isSeparatorRow (rowKey))
                continue;

            for (const auto& header : headers)
            {
                const auto* cell { root.getTableCell (
                    tableId, juce::Identifier (header), juce::Identifier (rowKey)) };

                if (cell != nullptr
                    and not std::any_of (cell->begin(), cell->end(),
                                         [] (const auto* child) { return child->isTag (Id::code); }))
                {
                    const auto hazard { getHazardMessage (*cell) };

                    if (hazard.isNotEmpty())
                        return juce::Result::fail (
                            getLocation (sourceFile, getRowLineNumber (root, tableId.toString(), rowKey), header)
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
                + Id::diagnosticSeparator + text::en::failUnknownTransform + Id::diagnosticSeparator
                + transformName);
    }

    const auto outputKeys { manifestDoc.getTableRowKeys (Id::generated) };
    const auto dispatchRows { manifestDoc.getTableRows (Id::dispatch) };
    juce::StringArray referencedTemplates;

    for (const auto& outputKey : outputKeys)
    {
        const auto templatePath { manifestDoc.getTableValue (
            Id::generated, Id::templatePath, outputKey) };

        if (not dir.getChildFile (templatePath).existsAsFile())
            return juce::Result::fail (
                getLocation (path, outputKeys.indexOf (outputKey) + 1, Id::templatePath.toString())
                + Id::diagnosticSeparator + text::en::failTemplateMissing + Id::diagnosticSeparator
                + templatePath);

        referencedTemplates.addIfNotAlreadyThere (templatePath);

        const auto separatorPath { manifestDoc.getTableValue (
            Id::generated, Id::separator, outputKey) };

        if (separatorPath.isNotEmpty())
        {
            if (not dir.getChildFile (separatorPath).existsAsFile())
                return juce::Result::fail (
                    getLocation (path, outputKeys.indexOf (outputKey) + 1, Id::separator.toString())
                    + Id::diagnosticSeparator + text::en::failTemplateMissing + Id::diagnosticSeparator
                    + separatorPath);

            referencedTemplates.addIfNotAlreadyThere (separatorPath);
        }
    }

    for (auto* dispatchRow : dispatchRows)
    {
        const auto fragmentPath { getDispatchCell (manifestDoc, *dispatchRow, Id::templatePath) };

        if (fragmentPath.isEmpty())
            continue;

        if (not dir.getChildFile (fragmentPath).existsAsFile())
        {
            const auto rowOffset { static_cast<uint32_t> (*dispatchRow->get<int> (Id::offset)) };
            const auto rowLine { static_cast<int> (manifestDoc.getLineNumber (rowOffset)) };

            return juce::Result::fail (getLocation (path, rowLine, Id::templatePath.toString())
                                       + Id::diagnosticSeparator + text::en::failFragmentMissing
                                       + Id::diagnosticSeparator + fragmentPath);
        }

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
        if (not templateDir.isAChildOf (dir))
            continue;

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
                    return juce::Result::fail (path + Id::diagnosticSeparator + text::en::failOrphan
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
                        juce::String::charToString (chars::space), false, false) };
                    const auto predicateArgs {
                        predicateSpec.fromFirstOccurrenceOf (juce::String::charToString (chars::space), false, false).trim()
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
            juce::String::charToString (chars::space), false, false) };
        const auto predicateArgs {
            predicateSpec.fromFirstOccurrenceOf (juce::String::charToString (chars::space), false, false).trim()
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
