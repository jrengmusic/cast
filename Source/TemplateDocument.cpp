#include "TemplateDocument.h"

TemplateDocument::TemplateDocument (const Model& model)
{
    for (auto* indexRow : model.getTableRows (Id::index))
    {
        const auto symbol { model.getTableValue (*indexRow, Id::symbol) };

        if (juce::File::createFileWithoutCheckingPath (symbol).hasFileExtension (Extensions::cast))
        {
            documents.emplace (symbol,
                jam::MarkdownDocument::parse (model.getFile (symbol).loadFileAsString(), symbol));

            for (auto* block : *documents.at (symbol).getRoot())
                if (block->contains (Id::type)
                    and *block->get<int> (Id::type) == map::BlockType::codeBlock)
                    addStamps (*block);
        }
    }
}

const TemplateDocument::Element* TemplateDocument::getCodeBlock (const Element& line) const
{
    return documents.at (*line.get<juce::String> (Id::templatePath))
        .getCodeBlock (juce::Identifier (*line.get<juce::String> (Id::info)));
}

const TemplateDocument::Element* TemplateDocument::getCodeBlock (const juce::String& templatePath, const juce::Identifier& fence) const
{
    return documents.at (templatePath).getCodeBlock (fence);
}

const juce::String& TemplateDocument::getBlockValue (const Model& model, const Element& row, const juce::String& address) const
{
    const auto templatePath { model.getValue (row, jam::Format::getPreColon (address).trim()) };
    const auto fence { jam::Format::getPostColon (address).trim() };

    return *getCodeBlock (templatePath, juce::Identifier (fence))->get<juce::String> (Id::value);
}

juce::String TemplateDocument::getValue (const Model& model, const Element& row, const juce::String& value) const
{
    if (model.isShape (row, value))
        return getBlockValue (model, row, value);

    if (Model::isAddress (value))
        return model.getValue (row, value);

    return value;
}

jam::Strings TemplateDocument::getMarkers (const juce::String& text)
{
    jam::Strings interiors;
    auto remaining { text };

    while (remaining.contains (Id::tripleColon))
    {
        remaining = jam::Format::from (remaining, Id::tripleColon, false);

        if (not remaining.contains (Id::tripleColon))
            break;

        interiors.add (jam::Format::upTo (remaining, Id::tripleColon, false));
        remaining = jam::Format::from (remaining, Id::tripleColon, false);
    }

    return interiors;
}

void TemplateDocument::addStamps (Element& block)
{
    static const auto bannerMarker { Model::getReservedName (Id::banner) };

    const auto blockText { getDatedText (block.getAllSubText(), getDate()) };
    block.add<juce::String> (Id::value, blockText);
    jam::Document::Identifiers names;

    for (const auto& interior : getMarkers (blockText))
    {
        const auto markerName { juce::Identifier (jam::Format::toValidID (interior)) };

        if (markerName != bannerMarker)
            names.add (markerName);
    }

    block.add<jam::Document::Identifiers> (Id::placeholder, std::move (names));
}

const std::tm& TemplateDocument::getDate() noexcept
{
    static const std::tm date { [] () noexcept
    {
        const auto now { std::time (nullptr) };
        std::tm utc {};

       #if JUCE_WINDOWS
        [[maybe_unused]] const auto isConverted { gmtime_s (&utc, &now) == 0 };
       #else
        [[maybe_unused]] const auto isConverted { gmtime_r (&now, &utc) != nullptr };
       #endif

        jassert (isConverted);

        return utc;
    }() };

    return date;
}

const juce::String& TemplateDocument::getDatePrefix()
{
    static const juce::String datePrefix { juce::String::charToString (Chars::openBracket)
                                           + Id::date.toString()
                                           + juce::String::charToString (Chars::colon) };

    return datePrefix;
}

bool TemplateDocument::isDateMarker (const juce::String& interior)
{
    return interior.startsWith (getDatePrefix()) and interior.endsWithChar (Chars::closeBracket);
}

juce::String TemplateDocument::getDateField (const std::tm& date, const char* conversion)
{
    static constexpr int fieldCapacity { 8 };

    std::array<char, fieldCapacity> field {};
    std::strftime (field.data(), field.size(), conversion, &date);

    return juce::String (field.data());
}

juce::String TemplateDocument::getDateValue (const juce::String& interior, const std::tm& date)
{
    static constexpr std::array datePatterns { std::pair { "yyyy", "%Y" },
                                               std::pair { "yy", "%y" },
                                               std::pair { "mm", "%m" },
                                               std::pair { "dd", "%d" } };

    auto value { interior.fromFirstOccurrenceOf (getDatePrefix(), false, false)
                     .upToLastOccurrenceOf (juce::String::charToString (Chars::closeBracket), false, false) };

    for (const auto& [pattern, conversion] : datePatterns)
        value = value.replace (pattern, getDateField (date, conversion));

    return value;
}

juce::String TemplateDocument::getDatedText (const juce::String& text, const std::tm& date)
{
    auto datedText { text };

    for (const auto& interior : getMarkers (text))
        if (isDateMarker (interior))
            datedText = jam::Format::replaceholder (datedText, interior, getDateValue (interior, date));

    return datedText;
}
