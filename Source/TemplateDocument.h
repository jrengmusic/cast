#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Model.h"
#include "Transforms.h"

struct TemplateDocument : jam::MarkdownDocument
{
    TemplateDocument() = default;

    explicit TemplateDocument (jam::MarkdownDocument&& parsed)
        : jam::MarkdownDocument (std::move (parsed))
    {
        for (auto* block : *root)
            if (block->contains (Id::type)
                and *block->get<int> (Id::type) == map::BlockType::codeBlock)
            {
                const auto blockText { block->getAllSubText() };
                block->add<juce::String> (Id::value, blockText);
                jam::Document::Identifiers names;
                auto remaining { blockText };

                while (remaining.contains (Id::tripleColon))
                {
                    remaining = jam::Format::from (remaining, Id::tripleColon, false);

                    if (not remaining.contains (Id::tripleColon))
                        break;
                    const auto interior { jam::Format::upTo (remaining, Id::tripleColon, false) };
                    names.add (juce::Identifier (jam::Format::getPreColon (interior)));
                    remaining = jam::Format::from (remaining, Id::tripleColon, false);
                }
                block->add<jam::Document::Identifiers> (Id::placeholder, std::move (names));
            }
    }

    const juce::String& getBlockValue (const juce::String& value) const
    {
        const auto id { juce::Identifier (jam::Format::getPostColon (value).trim()) };
        return *getCodeBlock (id)->get<juce::String> (Id::value);
    }

    juce::String getBinding (const Model& model, Element& row, const juce::String& value) const
    {
        if (jam::Format::getPreColon (value).trim() == Id::templatePath.toString())
            return getBlockValue (value);

        if (value.startsWithChar (Chars::at))
        {
            const auto symbol { model.getValue (row, value) };
            return symbol;
        }

        return value;
    }

    juce::String getSeparator (const Model& model,
                               Element& row,
                               Element* separatorScope,
                               const juce::Identifier& token) const
    {
        if (separatorScope != nullptr)
            if (auto* list { model.getList (*separatorScope) })
                if (auto* item { model.getListItem (*list, token) })
                    return getBinding (model, row, *item->get<juce::String> (Id::value));
        return {};
    }
};
