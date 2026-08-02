#pragma once
#include <JuceHeader.h>
#include <jam_markdown/parser/jam_Markdown.h>

namespace cast
{

static inline juce::String getCellHazard (const jam::Document& cell) noexcept
{
    const auto& flat { *cell.get<juce::String> (Id::text) };
    const auto* runsPtr { cell.contains (Id::runs) ? cell.get<juce::Array<juce::var>> (Id::runs) : nullptr };
    const int runsTotal { runsPtr != nullptr ? runsPtr->size() / 3 : 0 };
    auto cursor { flat.getCharPointer() };
    int pos { 0 };
    int runsIndex { 0 };

    while (not cursor.isEmpty())
    {
        while (runsIndex < runsTotal
               and pos >= static_cast<int> ((*runsPtr)[runsIndex * 3 + 1]))
            ++runsIndex;

        const auto inCodeSpan { runsIndex < runsTotal
                               and pos >= static_cast<int> ((*runsPtr)[runsIndex * 3])
                               and pos < static_cast<int> ((*runsPtr)[runsIndex * 3 + 1]) };

        if (not inCodeSpan)
        {
            const auto ch { *cursor };

            if (ch == Chars::lessThan or ch == Chars::greaterThan)
                return "contains '<' or '>'";

            if (jam::Markdown::getSchemeLength (cursor) > 0)
                return "contains URI scheme";
        }

        cursor.getAndAdvance();
        ++pos;
    }

    return {};
}

static inline int findColumnIndex (const jam::Document& tableDoc,
                                   const juce::String& columnName) noexcept
{
    const auto& rows { *tableDoc.get<jam::Array<jam::Document>> (Id::children) };

    if (rows.size() < 1)
        return -1;

    const auto& headerCells { *rows.at (0).get<jam::Array<jam::Document>> (Id::children) };

    for (int c { 0 }; c < headerCells.size(); ++c)
    {
        if (*headerCells.at (c).get<juce::String> (Id::text) == columnName)
            return c;
    }

    return -1;
}

static inline void extractSections (const jam::Document& doc,
                                    jam::HashMap<juce::String, const jam::Document*>& tables)
{
    const auto& blocks { *doc.get<jam::Array<jam::Document>> (Id::children) };
    juce::String pendingHeading;

    for (const auto& block : blocks)
    {
        const auto blockType { *block.get<int> (Id::type) };
        const auto isLevel2 { blockType == Id::BlockType::heading
                              and *block.get<juce::String> (Id::level) == "2" };

        if (isLevel2)
        {
            pendingHeading = *block.get<juce::String> (Id::text);
        }
        else if (blockType == Id::BlockType::table and pendingHeading.isNotEmpty())
        {
            tables[pendingHeading] = &block;
            pendingHeading.clear();
        }
        else
        {
            pendingHeading.clear();
        }
    }
}

} // namespace cast
