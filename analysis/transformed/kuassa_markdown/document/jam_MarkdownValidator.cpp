namespace jam
{
/*____________________________________________________________________________*/

juce::Result
MarkdownValidator::matches (const MarkdownDocument& model, const juce::String& column, const juce::String& args)
{
    const std::regex pattern { args.toStdString() };

    return forEachCell (model, column,
        [&pattern, &column] (Element& table, Element& row, const juce::String& value) -> juce::Result
        {
            if (not std::regex_match (value.toStdString(), pattern))
                return juce::Result::fail (getLocation (table, row, column)
                                           + Id::diagnosticSeparator + Id::matches
                                           + Id::diagnosticSeparator + text::English::failNoMatch
                                           + Id::diagnosticSeparator + value);

            return juce::Result::ok();
        });
}

juce::Result
MarkdownValidator::unique (const MarkdownDocument& model, const juce::String& column, const juce::String& args)
{
    for (auto* table : model.getTables())
        if (model.getTableHeaders (*table).contains (column))
        {
            jam::Strings seen;

            for (auto* row : model.getTableRows (*table))
            {
                const auto value { model.getTableValue (*row, juce::Identifier (column)) };

                if (seen.contains (value, false))
                    return juce::Result::fail (getLocation (*table, *row, column)
                                               + Id::diagnosticSeparator + Id::unique
                                               + Id::diagnosticSeparator
                                               + text::English::failDuplicate + value
                                               + juce::String::charToString (Chars::doubleQuote));

                seen.add (value);
            }
        }

    return juce::Result::ok();
}

juce::Result
MarkdownValidator::existsIn (const MarkdownDocument& model, const juce::String& column, const juce::String& args)
{
    const juce::Identifier targetTable { jam::Format::upTo (
        args, juce::String::charToString (Chars::dot), false) };
    const auto targetKeys { model.getTableRowKeys (targetTable) };

    return forEachCell (model, column,
        [&targetKeys, &column, &args] (Element& table, Element& row, const juce::String& value) -> juce::Result
        {
            if (not targetKeys.contains (value))
                return juce::Result::fail (getLocation (table, row, column)
                                           + Id::diagnosticSeparator + Id::existsIn
                                           + Id::diagnosticSeparator
                                           + text::English::failForeignKeyMissing + args);

            return juce::Result::ok();
        });
}

juce::Result
MarkdownValidator::oneOf (const MarkdownDocument& model, const juce::String& column, const juce::String& args)
{
    const auto choices { jam::Strings::fromTokens (
        args, juce::String::charToString (Chars::pipe), {}) };

    return forEachCell (model, column,
        [&choices, &column] (Element& table, Element& row, const juce::String& value) -> juce::Result
        {
            if (not choices.contains (value, false))
                return juce::Result::fail (
                    getLocation (table, row, column) + Id::diagnosticSeparator + Id::oneOf
                    + Id::diagnosticSeparator + text::English::failNotInSet + value);

            return juce::Result::ok();
        });
}

juce::Result
MarkdownValidator::range (const MarkdownDocument& model, const juce::String& column, const juce::String& args)
{
    return forEachCell (model, column,
        [&model, &column] (Element& table, Element& row, const juce::String& value) -> juce::Result
        {
            const auto doubleValue { value.getDoubleValue() };
            const auto minValue { model.getTableValue (row, Id::min).getDoubleValue() };
            const auto maxValue { model.getTableValue (row, Id::max).getDoubleValue() };

            if (doubleValue < minValue or doubleValue > maxValue)
                return juce::Result::fail (
                    getLocation (table, row, column) + Id::diagnosticSeparator + Id::range
                    + Id::diagnosticSeparator + text::English::failOutOfRange
                    + juce::String (maxValue));

            return juce::Result::ok();
        });
}

juce::Result
MarkdownValidator::parity (const MarkdownDocument& model, const juce::String& column, const juce::String& args)
{
    const juce::Identifier targetTable { jam::Format::upTo (
        args, juce::String::charToString (Chars::dot), false) };
    const juce::Identifier targetColumn { jam::Format::from (
        args, juce::String::charToString (Chars::dot), false) };

    jam::Strings localKeys;
    for (auto* table : model.getTables())
        if (model.getTableHeaders (*table).contains (column))
            for (auto* row : model.getTableRows (*table))
                localKeys.addIfNotAlreadyThere (
                    model.getTableValue (*row, juce::Identifier (column)), false);

    jam::Strings targetKeys;
    for (auto* row : model.getTableRows (targetTable))
        targetKeys.addIfNotAlreadyThere (model.getTableValue (*row, targetColumn), false);

    for (const auto& key : localKeys)
        if (not targetKeys.contains (key, false))
            return juce::Result::fail (Id::parity + Id::diagnosticSeparator
                                       + text::English::failRefMissing + Id::diagnosticSeparator
                                       + key);

    for (const auto& key : targetKeys)
        if (not localKeys.contains (key, false))
            return juce::Result::fail (Id::parity + Id::diagnosticSeparator
                                       + text::English::failLocalMissing + Id::diagnosticSeparator
                                       + key);

    return juce::Result::ok();
}

juce::Result
MarkdownValidator::onePerGroup (const MarkdownDocument& model, const juce::String& column, const juce::String& args)
{
    const juce::Identifier groupColumn { args };

    for (auto* table : model.getTables())
        if (model.getTableHeaders (*table).contains (column)
            and model.getTableHeaders (*table).contains (args))
        {
            jam::Strings markedGroups;
            jam::Strings allGroups;
            jam::Array<Element*> firstRows;

            for (auto* row : model.getTableRows (*table))
            {
                const auto group { model.getTableValue (*row, groupColumn) };

                if (not allGroups.contains (group, false))
                {
                    allGroups.add (group);
                    firstRows.add (row);
                }

                if (model.hasTableValue (*row, juce::Identifier (column)))
                {
                    if (markedGroups.contains (group, false))
                        return juce::Result::fail (
                            getLocation (*table, *row, column) + Id::diagnosticSeparator
                            + Id::onePerGroup + Id::diagnosticSeparator
                            + text::English::failGroupOpen + group + text::English::failGroupClose);

                    markedGroups.add (group);
                }
            }

            for (int index { 0 }; index < allGroups.size(); ++index)
                if (not markedGroups.contains (allGroups.at (index), false))
                    return juce::Result::fail (
                        getLocation (*table, *firstRows.at (index), column)
                        + Id::diagnosticSeparator + Id::onePerGroup + Id::diagnosticSeparator
                        + text::English::failGroupOpen + allGroups.at (index)
                        + text::English::failGroupClose);
        }

    return juce::Result::ok();
}

juce::String MarkdownValidator::getLocation (Element& table, Element& row, const juce::String& column)
{
    juce::String path;
    juce::String line;

    if (table.contains (Id::path))
        path = *table.get<juce::String> (Id::path);

    if (row.contains (Id::line))
        line = juce::String (*row.get<int> (Id::line));

    return path + juce::String::charToString (Chars::colon) + line
           + juce::String::charToString (Chars::space)
           + jam::Format::withEnclosure (column, Chars::openParen);
}

const MarkdownValidator::Rules& MarkdownValidator::getRules() const
{
    static const auto rules {
        []
        {
            Rules rules;

            rules.add<const Document&> (Id::headerRow.toString(),
                [] (const Document& document) -> juce::Result
                {
                    const auto& markdown { static_cast<const MarkdownDocument&> (document) };
                    jam::Strings failures;

                    for (auto* table : markdown.getTables())
                        if (markdown.getTableHeaders (*table).isEmpty())
                            failures.add (getLocation (*table, *table, Id::headerRow.toString()));

                    if (failures.size() > 0)
                        return juce::Result::fail (failures.joinIntoString (
                            juce::String::charToString (Chars::newline), 0, -1));

                    return juce::Result::ok();
                });

            rules.add<const Document&> (Id::tableRow.toString(),
                [] (const Document& document) -> juce::Result
                {
                    const auto& markdown { static_cast<const MarkdownDocument&> (document) };
                    jam::Strings failures;

                    for (auto* table : markdown.getTables())
                    {
                        for (auto* row : markdown.getTableRows (*table))
                            for (const auto& header : markdown.getTableHeaders (*table))
                                if (markdown.getTableCell (*row, juce::Identifier (header)) == nullptr)
                                    failures.add (getLocation (*table, *row, header));
                    }

                    if (failures.size() > 0)
                        return juce::Result::fail (failures.joinIntoString (
                            juce::String::charToString (Chars::newline), 0, -1));

                    return juce::Result::ok();
                });

            rules.add<const Document&> (Id::columns.toString(),
                [] (const Document& document) -> juce::Result
                {
                    const auto& markdown { static_cast<const MarkdownDocument&> (document) };
                    jam::Strings failures;

                    for (auto* table : markdown.getTables())
                        for (auto* row : markdown.getTableRows (*table))
                            if (row->contains (Id::columns))
                                failures.add (
                                    getLocation (*table, *row, Id::columns.toString()));

                    if (failures.size() > 0)
                        return juce::Result::fail (failures.joinIntoString (
                            juce::String::charToString (Chars::newline), 0, -1));

                    return juce::Result::ok();
                });

            rules.add<const Document&> (Id::alignment.toString(),
                [] (const Document& document) -> juce::Result
                {
                    const auto& markdown { static_cast<const MarkdownDocument&> (document) };
                    jam::Strings failures;

                    for (auto* table : markdown.getTables())
                        for (auto* row : *table)
                            for (auto* cell : *row)
                                if (cell->contains (Id::alignment))
                                {
                                    const auto value { *cell->get<juce::String> (Id::alignment) };
                                    static const jam::Strings alignments { Id::right.toString(), Id::left.toString(), Id::center.toString() };

                                    if (not alignments.contains (value, false))
                                        failures.add (getLocation (
                                            *table, *row, Id::alignment.toString()));
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

/*____________________________________________________________________________*/
} // namespace jam
