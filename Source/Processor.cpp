#include "Processor.h"

Processor::Processor (const juce::String& text, const juce::String& origin, const juce::File& directory, const jam::HashMap<juce::Identifier, int>& reflowWidths)
    : model (Model::parse (text, origin, directory, reflowWidths))
    , templateDocument (*model)
    , writer (*model, templateDocument)
{
}

juce::Result Processor::generate (const juce::String& output, const juce::String& toolchainArgument)
{
    if (const auto validation { Validator::isValid (*model, templateDocument) }; not validation.wasOk())
        return validation;

    model->addChains ([this] (const Model::Element& wrapper)
                      { return Items::getNextShapeLine (*model, templateDocument, wrapper); });

    if (const auto written { writer.toFile (model->getFile (output)) }; not written.wasOk())
        return written;

    return run (toolchainArgument);
}

juce::Result Processor::format (const jam::MarkdownWriter& formatter)
{
    if (const auto result { validator.isValid (*model) }; not result.wasOk())
        return result;

    const auto origins { getOrigins() };
    jam::Array<juce::String> formatFailures;
    formatFailures.resize (origins.size());

    Jobs::run (origins.size(),
        [this, &origins, &formatFailures, &formatter] (int index)
        {
            formatFailures.at (index) = writeOriginIfChanged (origins.at (index), formatter);
        });

    return getWriteResult (formatFailures);
}

std::pair<juce::Result, juce::String> Processor::getText (const jam::MarkdownWriter& formatter) const
{
    if (const auto result { validator.isValid (*model) }; not result.wasOk())
        return { result, {} };

    return { juce::Result::ok(), formatter.getText (*model, model->getManifestOrigin()) };
}

juce::Result Processor::run (const juce::String& toolchainArgument)
{
    const auto isToolchainArgumentGiven { toolchainArgument.isNotEmpty() };
    auto hasMatchedToolchainRow { false };

    for (auto* table : model->getTables (Id::toolchain))
    {
        for (auto* row : model->getTableRows (*table))
        {
            const auto argument { model->getColumnValue (*row, Id::argument) };

            if (argument.compare (toolchainArgument) == 0)
            {
                hasMatchedToolchainRow = true;

                if (const auto result { runToolchainRow (*row) }; not result.wasOk())
                    return result;
            }
        }
    }

    if (isToolchainArgumentGiven and not hasMatchedToolchainRow)
        return juce::Result::fail (toolchainArgument + Id::diagnosticSeparator
                                   + text::Diagnostics::failToolchainArgument);

    return juce::Result::ok();
}

jam::Strings Processor::getOrigins() const
{
    jam::Strings origins;

    for (auto* block : *model->getRoot())
    {
        const auto origin { *block->get<juce::String> (Id::path) };
        origins.addIfNotAlreadyThere (origin, false);
    }

    return origins;
}

juce::String Processor::writeOriginIfChanged (const juce::String& origin, const jam::MarkdownWriter& formatter) const
{
    static const auto newlineText { juce::String::charToString (Chars::newline) };

    const auto file { model->getFile (origin) };
    const auto current { file.loadFileAsString() };

    if (const auto canonical { formatter.getText (*model, origin) }; canonical.compare (current) != 0)
        if (not file.replaceWithText (canonical,
            false,
            false,
            newlineText.toRawUTF8()))
            return file.getFullPathName();

    return {};
}

juce::Result Processor::getWriteResult (const jam::Array<juce::String>& formatFailures) const
{
    static const auto newlineText { juce::String::charToString (Chars::newline) };

    jam::Strings failures;

    for (const auto& failedFile : formatFailures)
        if (failedFile.isNotEmpty())
            failures.add (failedFile + Id::diagnosticSeparator + text::Diagnostics::failOutputWrite);

    if (failures.size() > 0)
        return juce::Result::fail (failures.joinIntoString (newlineText, 0, -1));

    return juce::Result::ok();
}

juce::Result Processor::runToolchainRow (const Model::Element& row)
{
    const auto& command { model->getValue (row, Id::command) };

    const auto flag { model->getValue (row, Id::flag) };
    const auto arguments { Toolchain::getToolchainArguments (command, flag) };
    const auto diagnosticLine { flag.isNotEmpty()
                                    ? command + Chars::space + flag
                                    : command };

    return Toolchain::runProcess (arguments, diagnosticLine);
}
