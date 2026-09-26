namespace jam
{
/*____________________________________________________________________________*/

const ConfigValidator::Rules& ConfigValidator::getRules() const
{
    static const auto rules {
        []
        {
            Rules rules;

            rules.add<const Document&> (Id::type.toString(),
                [] (const Document& document) -> juce::Result
                {
                    const auto& config { static_cast<const ConfigDocument&> (document) };

                    return forEachCell (config, Id::type.toString(),
                        [] (const Element& table, const Element& row, const juce::String& value) -> juce::Result
                        {
                            if (not ConfigDocument::getValueTypes().contains (juce::Identifier (value)))
                                return juce::Result::fail (getLocation (table, row, Id::type.toString())
                                                           + Id::diagnosticSeparator + Id::type.toString()
                                                           + Id::diagnosticSeparator
                                                           + text::English::failNotInSet + value);

                            return juce::Result::ok();
                        });
                });

            rules.add<const Document&> (Id::value.toString(),
                [] (const Document& document) -> juce::Result
                {
                    const auto& config { static_cast<const ConfigDocument&> (document) };

                    return forEachCell (config, Id::value.toString(),
                        [&config] (const Element& table, const Element& row, const juce::String& value) -> juce::Result
                        {
                            if (value.isEmpty())
                            {
                                if (config.getTableValue (row, Id::type).compare (Id::string.toString()) != 0)
                                    return juce::Result::fail (getLocation (table, row, Id::value.toString())
                                                               + Id::diagnosticSeparator + Id::value.toString()
                                                               + Id::diagnosticSeparator + text::English::failEmptyValue);
                            }

                            return juce::Result::ok();
                        });
                });

            rules.add<const Document&> (Id::choices.toString(),
                [] (const Document& document) -> juce::Result
                {
                    const auto& config { static_cast<const ConfigDocument&> (document) };

                    return forEachCell (config, Id::choices.toString(),
                        [&config] (const Element& table, const Element& row, const juce::String& choicesText) -> juce::Result
                        {
                            if (choicesText.isNotEmpty())
                            {
                                juce::StringArray choices;
                                choices.addTokens (choicesText, ",", "");
                                choices.trim();

                                const auto value { config.getTableValue (row, Id::value) };

                                if (not choices.contains (value))
                                    return juce::Result::fail (getLocation (table, row, Id::choices.toString())
                                                               + Id::diagnosticSeparator + Id::choices.toString()
                                                               + Id::diagnosticSeparator
                                                               + text::English::failNotInSet + value);
                            }

                            return juce::Result::ok();
                        });
                });

            return rules;
        }()
    };

    return rules;
}

juce::Result ConfigValidator::isValid (const ConfigDocument& document)
{
    static const MarkdownValidator structuralValidator;
    static const ConfigValidator configValidator;

    const auto structuralResult { structuralValidator.isValid (document) };

    return structuralResult.wasOk()
             ? static_cast<const Document::Validator&> (configValidator).isValid (document)
             : structuralResult;
}

} // namespace jam
