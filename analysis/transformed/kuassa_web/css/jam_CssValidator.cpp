namespace jam
{
/*____________________________________________________________________________*/

const CssValidator::Rules& CssValidator::getRules() const
{
    static const auto rules {
        []
        {
            Rules rules;

            rules.add<const Document&> (Id::tokens.toString(),
                [] (const Document& document) -> juce::Result
                {
                    if (not document.root->contains (Id::tokens))
                        return juce::Result::fail (Id::tokens.toString());

                    const auto& tokens { *document.root->get<jam::Array<Document::Token>> (Id::tokens) };
                    jam::Strings failures;

                    for (int index { 0 }; index < tokens.size(); ++index)
                    {
                        const auto& token { tokens.at (index) };

                        if (token.type == map::CssTokenType::badString
                            or token.type == map::CssTokenType::badUrl)
                            failures.add (Id::tokens.toString() + Id::diagnosticSeparator
                                         + juce::String (token.offset()));
                    }

                    if (failures.size() > 0)
                        return juce::Result::fail (failures.joinIntoString (
                            juce::String::charToString (Chars::newline), 0, -1));

                    return juce::Result::ok();
                });

            rules.add<const Document&> (Id::declaration.toString(),
                [] (const Document& document) -> juce::Result
                {
                    if (not document.root->contains (Id::tokens))
                        return juce::Result::fail (Id::declaration.toString());

                    const auto& tokens { *document.root->get<jam::Array<Document::Token>> (Id::tokens) };
                    jam::Strings failures;

                    for (int index { 0 }; index < tokens.size(); ++index)
                    {
                        const auto& token { tokens.at (index) };

                        if (token.contains (Id::declaration))
                            failures.add (Id::declaration.toString() + Id::diagnosticSeparator
                                         + juce::String (token.offset()));
                    }

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
