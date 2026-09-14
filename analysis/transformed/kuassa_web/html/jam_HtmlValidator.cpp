namespace jam
{
/*____________________________________________________________________________*/

const HtmlValidator::Rules& HtmlValidator::getRules() const
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

/**______________________________END OF NAMESPACE______________________________*/
}// namespace jam
