/**
 * @file jam_HtmlValidator.h
 * @brief Keyed structural rules for a parsed HTML document.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct HtmlValidator
 * @brief HTML structural validation, keyed into Document::Validator's rule set.
 *
 * Supplies the two rules that make a parsed HTML tree well-formed: a single
 * root element, and no element left open by an end-tag name mismatch. Each
 * rule is keyed by its property name and returns a juce::Result; Document::
 * Validator::isValid() runs every rule and aggregates failures.
 */
struct HtmlValidator : Document::Validator
{
    /**
     * @brief Returns the HTML validation rules.
     *
     * Two rules, keyed by the property each checks:
     *  - `Id::root`   — the document must have exactly one root element: the
     *                  root's first child must exist and must not carry the
     *                  `Id::root` mark that a multiple-root parse leaves behind.
     *  - `Id::endTag` — no element may carry an `Id::endTag` mark, which a
     *                  start/end tag-name mismatch leaves on the still-open
     *                  element.
     *
     * @return The keyed rule set, built once.
     */
    const Rules& getRules() const override;
};

} // namespace jam
