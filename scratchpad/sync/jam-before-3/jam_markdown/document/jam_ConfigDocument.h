#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct ConfigDocument
 * @brief Typed key/value config tables over MarkdownDocument -- parses a
 *        markdown source into tables of `key`/`type`/`value` rows and
 *        converts each row into a typed juce::var.
 *
 * Every table whose header row carries both a `key` and a `type` column is a
 * config table; its rows become properties of one child ValueTree, keyed by
 * the table's own id. Row conversion is a single dispatch table
 * (getValueTypes()) keyed by the `type` cell -- one typing point, no
 * per-caller conversion logic.
 */
struct ConfigDocument : MarkdownDocument
{
    /**
     * @brief Parses config markdown source into a fresh ConfigDocument, setting table provenance.
     *
     * Delegates to MarkdownDocument::parse (documentText, origin) and adopts
     * its result -- same parse, same provenance stamping (Id::path/Id::line),
     * viewed through the config-table API below.
     *
     * @param documentText The markdown source text, owned by the caller.
     * @param origin       The resource path or name set as Id::path on every direct-child Element.
     * @return The parsed ConfigDocument, with a childless root sentinel on parse failure.
     */
    static ConfigDocument parse (const juce::String& documentText, const juce::String& origin);

    /**
     * @brief The `type`-cell dispatch table converting a row's raw text into a typed juce::var.
     *
     * Keyed by Id::integer/floatingPoint/boolean/string/colour/numbers.
     * `integer` and `floatingPoint` parse the text as a number; `boolean`
     * compares the text against "true"; `string` passes the text through
     * unchanged; `colour` parses a hex literal; `numbers` splits the text on
     * commas into an array of integers.
     *
     * @return The type-keyed conversion table.
     */
    static const jam::Function::Map<juce::Identifier, juce::var>& getValueTypes();

    /**
     * @brief Builds a @p rootType-rooted ValueTree, one child per config table, one property per declared row.
     *
     * Walks every table returned by getTables(). A table qualifies as a
     * config table when its header row carries both a `key` and a `type`
     * column; its rows are read via `key`/`type`/`value` and converted
     * through getValueTypes(), one property per row on a child tree keyed by
     * the table's own id. Every declared row yields a property, whether its
     * `value` cell is set or empty -- an empty cell is a valid string value,
     * not an absent one. Tables without both columns are skipped.
     *
     * @param rootType The type of the returned root ValueTree.
     * @return The root tree with one child per config table.
     * @throws std::invalid_argument if a row's `type` cell names a type absent from getValueTypes().
     */
    juce::ValueTree getValueTree (const juce::Identifier& rootType) const;

    /**
     * @brief Builds a @p rootType-rooted ValueTree, one child per config table, one
     *        property per row with a non-empty @p valueColumn cell.
     *
     * Same table walk and conversion as getValueTree (const juce::Identifier&), but
     * reads each row's raw text from @p valueColumn instead of the `value` column --
     * lets a caller build the tree from an alternate cell column (e.g. a light/dark
     * variant column), read as a sparse record of overrides. A row whose
     * @p valueColumn cell is empty yields no property for that row.
     *
     * @param rootType     The type of the returned root ValueTree.
     * @param valueColumn  The cell column read for each row's raw text.
     * @return The root tree with one child per config table, carrying only its overridden properties.
     * @throws std::invalid_argument if a declared row's `type` cell names a type absent from getValueTypes().
     */
    juce::ValueTree getValueTree (const juce::Identifier& rootType, const juce::Identifier& valueColumn) const;
};

} // namespace jam
