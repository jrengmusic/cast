/**
 * @file jam_XmlValidator.h
 * @brief Keyed structural rules for a parsed XML document.
 */

#pragma once

namespace jam
{
/*____________________________________________________________________________*/

/**
 * @struct XmlValidator
 * @brief XML structural validation, keyed into Document::Validator's rule set.
 *
 * Supplies the two rules that make a parsed XML tree well-formed: a single
 * root element, and no element left open by an end-tag name mismatch. Each
 * rule is keyed by its property name and returns a juce::Result; Document::
 * Validator::isValid() runs every rule and aggregates failures.
 */
struct XmlValidator : Document::Validator
{
    /**
     * @brief Returns the XML validation rules.
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
    const Rules& getRules() const override
    {
        static const auto rules {
            []
            {
                Rules rules;

                rules.add<const Document&> (Id::root.toString(),
                    [] (const Document& document) -> juce::Result
                    {
                        if (not document.root->id.isNull())
                        {
                            if (not document.root->contains (Id::root))
                                return juce::Result::ok();

                            return juce::Result::fail (Id::root.toString() + Id::diagnosticSeparator
                                                       + juce::String ("multiple root elements"));
                        }

                        return juce::Result::fail (
                            Id::root.toString() + Id::diagnosticSeparator + juce::String ("no root element"));
                    });

                rules.add<const Document&> (Id::endTag.toString(),
                    [] (const Document& document) -> juce::Result
                    {
                        jam::Strings failures;

                        if (not document.root->id.isNull())
                            document.root->applyFunctionRecursively (
                                [&failures] (const Document::Element& element)
                                {
                                    if (element.contains (Id::endTag))
                                        failures.add (Id::endTag.toString() + Id::diagnosticSeparator
                                                     + element.id.toString());
                                });

                        if (failures.size() > 0)
                            return juce::Result::fail (failures.joinIntoString (
                                juce::String::charToString (Chars::newline), 0, -1));

                        return juce::Result::ok();
                    });

                return rules;
            }()
        };

        return rules;
    }
};

} // namespace jam
