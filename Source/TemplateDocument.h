#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Model.h"
#include "Transforms.h"

/**
 * @struct TemplateDocument
 * @brief A parsed template file, addressable by code-block id, whose code
 *        blocks carry their own placeholder-name list stamped once at
 *        parse time.
 *
 * A TemplateDocument is a jam::MarkdownDocument in which every code block --
 * one shape, keyed by the fence's info string -- has been scanned for
 * @c :::token::: placeholders, so a shape's distinct token names are read
 * back rather than re-scanned by every caller. TemplateDocument additionally
 * resolves a binding's authored value and a token's authored separator
 * against a Model row, on behalf of callers that walk a manifest's wiring.
 */
struct TemplateDocument : jam::MarkdownDocument
{
    /** Constructs an empty template document with no code blocks. */
    TemplateDocument() = default;

    /**
     * @brief Adopts an already-parsed jam::MarkdownDocument and stamps
     *        every code block with its authored text and its distinct
     *        @c :::token::: placeholder names, in authored order.
     *
     * @param parsed The parsed markdown document to adopt.
     */
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
                    names.add (juce::Identifier (interior));
                    remaining = jam::Format::from (remaining, Id::tripleColon, false);
                }
                block->add<jam::Document::Identifiers> (Id::placeholder, std::move (names));
            }
    }

    /**
     * @brief Returns the authored text of the code block named by the
     *        @c template:\<id\> portion of @p value.
     *
     * @param value A value of the form @c template:\<id\>, the id being the
     *              target code block's fence info string.
     * @returns The code block's authored text.
     */
    const juce::String& getBlockValue (const juce::String& value) const
    {
        const auto id { juce::Identifier (jam::Format::getPostColon (value).trim()) };
        return *getCodeBlock (id)->get<juce::String> (Id::value);
    }

    /**
     * @brief Resolves a bullet's authored source to the value it names --
     *        a nested shape's text, a resolved @-sigiled address, or the
     *        source text itself, verbatim.
     *
     * @param model The model whose row and index the source is resolved
     *              against.
     * @param row   The row @p value was authored on.
     * @param value The bullet's authored source: @c template:\<id\>, an
     *              @-sigiled address, or plain text.
     * @returns The resolved value.
     */
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

    /**
     * @brief Resolves @p token's authored separator, if any, within
     *        @p separatorScope, through getBinding().
     *
     * @param model           The model whose row the separator is resolved
     *                        against.
     * @param row             The row the separator binding is resolved
     *                        against.
     * @param separatorScope  The blockquote scope carrying separator
     *                        bindings, or @c nullptr when the row declares
     *                        none.
     * @param token           The token whose separator is looked up.
     * @returns The resolved separator, or an empty string when @p token has
     *          no authored separator.
     */
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
