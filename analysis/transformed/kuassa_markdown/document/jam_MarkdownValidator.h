#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct MarkdownValidator
 * @brief Column-rule validation for markdown tables, plus the default
 *        structural Rules (headerRow, tableRow, columns, alignment).
 *
 * Each column-rule function (matches(), unique(), existsIn(), oneOf(),
 * range(), parity(), onePerGroup()) checks one authored rule keyword against
 * every cell of the column it names, and is invoked by the domain's own
 * column-rule dispatch, not by getRules() -- getRules() supplies only the
 * four document-wide structural rules run unconditionally by
 * Document::Validator::isValid().
 */
struct MarkdownValidator : Document::Validator
{
    using Element = Document::Element;

    /**
     * @brief Runs @p function over every cell of @p column, across every table that has it.
     *
     * For each table carrying @p column among its headers, walks its data
     * rows in authored order, calling @p function with (table, row, cell
     * text); the first non-ok Result returned by @p function stops the walk
     * and is returned.
     *
     * @tparam Function  Callable taking (Element& table, Element& row, const juce::String& value), returning juce::Result.
     * @param model    The document whose tables are walked.
     * @param column   Header name of the column to check.
     * @param function Invoked once per matching cell.
     * @return The first non-ok Result from @p function, or juce::Result::ok() if every cell passes.
     */
    template <typename Function>
    static juce::Result
    forEachCell (const MarkdownDocument& model, const juce::String& column, Function&& function)
    {
        for (auto* table : model.getTables())
            if (model.getTableHeaders (*table).contains (column))
                for (auto* row : model.getTableRows (*table))
                    if (const auto result { function (
                            *table, *row, model.getTableValue (*row, juce::Identifier (column)))
                        };
                        not result.wasOk())
                        return result;

        return juce::Result::ok();
    }

    /**
     * @brief Checks that every cell of @p column matches a regular expression.
     * @param model  The document whose tables are checked.
     * @param column Header name of the column to check.
     * @param args   The regular expression pattern every cell value must match.
     * @return The first failing cell's location and diagnostic, or juce::Result::ok() if every cell matches.
     */
    static juce::Result
    matches (const MarkdownDocument& model, const juce::String& column, const juce::String& args);

    /**
     * @brief Checks that every value of @p column is unique within its table.
     * @param model  The document whose tables are checked.
     * @param column Header name of the column to check.
     * @param args   Unused.
     * @return The first duplicate cell's location and diagnostic, or juce::Result::ok() if every value is unique.
     */
    static juce::Result
    unique (const MarkdownDocument& model, const juce::String& column, const juce::String& args);

    /**
     * @brief Checks that every value of @p column matches a row key in another table.
     * @param model  The document whose tables are checked.
     * @param column Header name of the column to check.
     * @param args   The target table's own id, optionally followed by a "." and a target column name (only the table id is used).
     * @return The first unmatched cell's location and diagnostic, or juce::Result::ok() if every value resolves.
     */
    static juce::Result
    existsIn (const MarkdownDocument& model, const juce::String& column, const juce::String& args);

    /**
     * @brief Checks that every value of @p column is one of a pipe-separated set of choices.
     * @param model  The document whose tables are checked.
     * @param column Header name of the column to check.
     * @param args   Pipe-separated ("|") list of allowed values.
     * @return The first out-of-set cell's location and diagnostic, or juce::Result::ok() if every value is allowed.
     */
    static juce::Result
    oneOf (const MarkdownDocument& model, const juce::String& column, const juce::String& args);

    /**
     * @brief Checks that every numeric value of @p column falls within its row's min/max columns.
     * @param model  The document whose tables are checked.
     * @param column Header name of the column to check.
     * @param args   Unused.
     * @return The first out-of-range cell's location and diagnostic, or juce::Result::ok() if every value is in range.
     */
    static juce::Result
    range (const MarkdownDocument& model, const juce::String& column, const juce::String& args);

    /**
     * @brief Checks that @p column's distinct values match a target table's column, both ways.
     *
     * Collects the distinct values of @p column across every table carrying
     * it, and the distinct values of the target table's target column, then
     * fails on the first value present on one side and missing on the
     * other.
     *
     * @param model  The document whose tables are checked.
     * @param column Header name of the local column to check.
     * @param args   The target table's own id, followed by "." and the target column name.
     * @return A diagnostic naming the first mismatched key, or juce::Result::ok() if both sides agree.
     */
    static juce::Result
    parity (const MarkdownDocument& model, const juce::String& column, const juce::String& args);

    /**
     * @brief Checks that at most one row per group column carries a non-empty value in @p column.
     * @param model  The document whose tables are checked.
     * @param column Header name of the column that must be set at most once per group.
     * @param args   Header name of the grouping column.
     * @return The first violating group's location and diagnostic, or juce::Result::ok() if every group complies.
     */
    static juce::Result
    onePerGroup (const MarkdownDocument& model, const juce::String& column, const juce::String& args);

    /**
     * @brief Builds a "path:line (column)" diagnostic location string.
     * @param table  The table Element, read for its Id::path property.
     * @param row    The row Element, read for its Id::line property.
     * @param column Header name of the column being reported.
     * @return "path:line (column)", with path/line empty when their source properties are absent.
     */
    static juce::String getLocation (Element& table, Element& row, const juce::String& column);

    /**
     * @brief Returns the four document-wide structural rules: headerRow, tableRow, columns, alignment.
     *
     * headerRow fails a table with no header cells. tableRow fails a row
     * missing a cell for one of its table's header columns. columns fails a
     * row still carrying Id::columns (a grid-table cell-count mismatch).
     * alignment fails a cell whose Id::alignment property is not "left",
     * "right", or "center".
     *
     * @return The four structural rules, keyed by rule name.
     */
    const Rules& getRules() const override;
};

} // namespace jam
