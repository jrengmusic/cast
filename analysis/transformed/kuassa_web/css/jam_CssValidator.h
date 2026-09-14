/**
 * @file jam_CssValidator.h
 * @brief Keyed structural rules for a parsed CSS document.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct CssValidator
 * @brief CSS structural validation, keyed into Document::Validator's rule set.
 *
 * Supplies two rules over a parsed CSS token stream: no bad-string/bad-url
 * tokens, and no leftover declaration tokens. Each rule is keyed by its
 * property name and returns a juce::Result; Document::Validator::isValid()
 * runs every rule and aggregates failures.
 */
struct CssValidator : Document::Validator
{
    /**
     * @brief Returns the CSS validation rules.
     *
     * Two rules, keyed by the property each checks:
     *  - `Id::tokens`      — the root must carry a token array, and none of its
     *                       tokens may be `badString` or `badUrl`; each offender
     *                       is reported by its byte offset.
     *  - `Id::declaration` — the root must carry a token array, and none of its
     *                       tokens may carry an `Id::declaration` property; each
     *                       offender is reported by its byte offset.
     *
     * @return The keyed rule set, built once.
     */
    const Rules& getRules() const override;
};

} // namespace jam
