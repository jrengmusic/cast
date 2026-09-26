#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct ConfigValidator
 * @brief Column-rule validation for ConfigDocument tables -- `type`, `value`, and `choices`.
 *
 * getRules() supplies three rules on top of MarkdownValidator's four structural
 * ones: `type` fails a cell whose value is not a key of
 * ConfigDocument::getValueTypes(); `value` fails a row whose `value` cell is
 * empty when its `type` cell is not Id::string -- an empty cell is a valid
 * string, never a fabricated numeric default; `choices` fails a row whose
 * `value` cell is not one of its own comma-separated `choices` list (a blank
 * `choices` cell is unconstrained). isValid() runs the structural pass
 * first, then this pass, mirroring MarkdownValidator's own two-pass shape.
 */
struct ConfigValidator : MarkdownValidator
{
    /**
     * @brief Returns the three config-table rules: `type`, `value`, and `choices`.
     *
     * `type` fails a cell whose value is not a key of
     * ConfigDocument::getValueTypes(). `value` fails a row whose `value`
     * cell is empty when its `type` cell is not Id::string. `choices` fails
     * a row whose `value` cell is not one of its own row's comma-separated
     * `choices` list, when that list is non-empty.
     *
     * @return The three rules, keyed by rule name.
     */
    const Rules& getRules() const override;

    /**
     * @brief Runs the structural pass, then the config-table pass, over @p document.
     *
     * The structural pass (MarkdownValidator::isValid) runs first; a failure
     * there is returned immediately without running the config-table pass.
     *
     * @param document The parsed ConfigDocument to validate.
     * @return The first failing Result from either pass, or juce::Result::ok() if both pass.
     */
    static juce::Result isValid (const ConfigDocument& document);
};

} // namespace jam
