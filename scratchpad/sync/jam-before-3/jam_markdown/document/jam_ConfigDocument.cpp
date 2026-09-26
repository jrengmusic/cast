namespace jam
{
/*____________________________________________________________________________*/

ConfigDocument ConfigDocument::parse (const juce::String& documentText, const juce::String& origin)
{
    ConfigDocument document;
    document.MarkdownDocument::operator= (MarkdownDocument::parse (documentText, origin));
    return document;
}

const jam::Function::Map<juce::Identifier, juce::var>& ConfigDocument::getValueTypes()
{
    static const jam::Function::Map<juce::Identifier, juce::var> valueTypes {
        []
        {
            jam::Function::Map<juce::Identifier, juce::var> table;

            table.add<const juce::String&> (Id::integer,
                [] (const juce::String& value) -> juce::var { return juce::var (value.getIntValue()); });

            table.add<const juce::String&> (Id::floatingPoint,
                [] (const juce::String& value) -> juce::var { return juce::var (value.getDoubleValue()); });

            table.add<const juce::String&> (Id::boolean,
                [] (const juce::String& value) -> juce::var { return juce::var (value.compare ("true") == 0); });

            table.add<const juce::String&> (Id::string,
                [] (const juce::String& value) -> juce::var { return juce::var (value); });

            table.add<const juce::String&> (Id::colour,
                [] (const juce::String& value) -> juce::var { return juce::var (value.getHexValue64()); });

            table.add<const juce::String&> (Id::numbers,
                [] (const juce::String& value) -> juce::var
                {
                    juce::StringArray tokens;
                    tokens.addTokens (value, ",", "");
                    tokens.trim();

                    juce::Array<juce::var> numbers;

                    for (const auto& token : tokens)
                        numbers.add (juce::var (token.getIntValue()));

                    return juce::var (numbers);
                });

            return table;
        }()
    };

    return valueTypes;
}

juce::ValueTree ConfigDocument::getValueTree (const juce::Identifier& rootType) const
{
    juce::ValueTree tree { rootType };

    for (auto* table : getTables())
    {
        const auto tableHeaders { getTableHeaders (*table) };

        if (tableHeaders.contains (Id::key.toString()) and tableHeaders.contains (Id::type.toString()))
        {
            juce::ValueTree tableTree { Id::toType (table->id) };

            for (auto* row : getTableRows (*table))
            {
                const juce::Identifier key { getTableValue (*row, Id::key) };
                const auto type { juce::Identifier { getTableValue (*row, Id::type) } };
                const auto value { getTableValue (*row, Id::value) };

                if (getValueTypes().contains (type))
                {
                    tableTree.setProperty (key, getValueTypes().get (type, value), nullptr);
                }
                else
                {
                    throw std::invalid_argument ((juce::String ("ConfigDocument::getValueTree: unknown type ")
                                                  + type.toString() + " for key " + key.toString()
                                                  + " in table " + table->id.toString())
                                                     .toStdString());
                }
            }

            tree.appendChild (tableTree, nullptr);
        }
    }

    return tree;
}

juce::ValueTree ConfigDocument::getValueTree (const juce::Identifier& rootType, const juce::Identifier& valueColumn) const
{
    juce::ValueTree tree { rootType };

    for (auto* table : getTables())
    {
        const auto tableHeaders { getTableHeaders (*table) };

        if (tableHeaders.contains (Id::key.toString()) and tableHeaders.contains (Id::type.toString()))
        {
            juce::ValueTree tableTree { Id::toType (table->id) };

            for (auto* row : getTableRows (*table))
            {
                const juce::Identifier key { getTableValue (*row, Id::key) };
                const auto type { juce::Identifier { getTableValue (*row, Id::type) } };
                const auto value { getTableValue (*row, valueColumn) };

                if (value.isNotEmpty())
                {
                    if (getValueTypes().contains (type))
                    {
                        tableTree.setProperty (key, getValueTypes().get (type, value), nullptr);
                    }
                    else
                    {
                        throw std::invalid_argument (
                            (juce::String ("ConfigDocument::getValueTree: unknown type ")
                             + type.toString() + " for key " + key.toString()
                             + " in table " + table->id.toString())
                                .toStdString());
                    }
                }
            }

            tree.appendChild (tableTree, nullptr);
        }
    }

    return tree;
}

} // namespace jam
