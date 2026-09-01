#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Model.h"
#include "Transforms.h"

/**
 * @struct TemplateDocument
 * @brief A pool of every @c .cast template file the manifest's index
 *        declares, each parsed once, keyed by its own index symbol, and
 *        each code block pre-stamped with its own text and placeholder
 *        tokens.
 */
struct TemplateDocument
{
    using Element = Model::Element;

    /** Constructs an empty template document, holding no parsed files. */
    TemplateDocument() = default;

    /**
     * @brief Parses every @c .cast file @p model's index declares,
     *        keyed by its own symbol, and stamps each code block with
     *        its own text and the @c :::token::: names it places.
     *
     * @param model The model whose index declares the template files to
     *              parse.
     */
    explicit TemplateDocument (const Model& model)
    {
        for (auto* indexRow : model.getTableRows (Id::index))
        {
            const auto symbol { model.getTableValue (*indexRow, Id::symbol) };

            if (juce::File::createFileWithoutCheckingPath (symbol).hasFileExtension (Extensions::cast))
            {
                documents.emplace (symbol,
                    jam::MarkdownDocument::parse (model.getFile (symbol).loadFileAsString(), symbol));

                for (auto* block : *documents.at (symbol).root)
                    if (block->contains (Id::type)
                        and *block->get<int> (Id::type) == map::BlockType::codeBlock)
                    {
                        const auto blockText { block->getAllSubText() };
                        block->add<juce::String> (Id::value, blockText);
                        jam::Document::Identifiers names;

                        for (const auto& interior : getMarkers (blockText))
                            names.add (juce::Identifier (jam::Format::toValidID (interior)));

                        block->add<jam::Document::Identifiers> (Id::placeholder, std::move (names));
                    }
            }
        }
    }

    /**
     * @brief Returns @p line's own shape's code block -- the fence named
     *        by @p line's own info, read from @p line's own resolved
     *        template file.
     *
     * @param line The structure line whose shape's code block is read.
     * @returns @p line's own shape's code block.
     */
    Element* getCodeBlock (const Element& line) const
    {
        return documents.at (*line.get<juce::String> (Id::templatePath))
            .getCodeBlock (juce::Identifier (*line.get<juce::String> (Id::info)));
    }

    /**
     * @brief Returns @p fence's code block, read from @p templatePath's
     *        own parsed template file.
     *
     * @param templatePath The resolved @c .cast file @p fence is read
     *                     from.
     * @param fence        The fence name to read.
     * @returns @p fence's code block.
     */
    Element* getCodeBlock (const juce::String& templatePath, const juce::Identifier& fence) const
    {
        return documents.at (templatePath).getCodeBlock (fence);
    }

    /**
     * @brief Resolves @p address -- @c \@alias:fence -- to its named
     *        code block's own text.
     *
     * @param model   The model @p address's alias part is resolved
     *                against.
     * @param row     The row @p address's alias part is resolved
     *                against.
     * @param address The address: an alias naming a @c .cast file,
     *                followed by @c :fence.
     * @returns @p address's resolved code block's own text.
     */
    const juce::String& getBlockValue (const Model& model, Element& row, const juce::String& address) const
    {
        const auto templatePath { model.getValue (row, jam::Format::getPreColon (address).trim()) };
        const auto fence { jam::Format::getPostColon (address).trim() };

        return *getCodeBlock (templatePath, juce::Identifier (fence))->get<juce::String> (Id::value);
    }

    /**
     * @brief Resolves @p value to its text -- a shape's code block
     *        through getBlockValue() when @p value is a shape address, an
     *        @-sigiled index alias through Model::getValue() otherwise,
     *        or @p value itself when it is plain text.
     *
     * @param model The model @p row and @p value belong to.
     * @param row   The row @p value is resolved against.
     * @param value The authored value to resolve.
     * @returns @p value's resolved text.
     */
    juce::String getValue (const Model& model, Element& row, const juce::String& value) const
    {
        if (model.isShape (row, value))
            return getBlockValue (model, row, value);

        if (Model::isAddress (value))
            return model.getValue (row, value);

        return value;
    }

    /**
     * @brief Returns every @c :::interior::: marker's own interior text
     *        authored in @p text, verbatim, in authored order -- the one
     *        scan every marker-reading member reads through.
     *
     * @param text The text scanned for its own markers.
     * @returns @p text's own marker interiors, in authored order.
     */
    static jam::Strings getMarkers (const juce::String& text)
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

private:
    jam::HashMap<juce::String, jam::MarkdownDocument> documents;
};
