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
    /** Model's own element type, brought into TemplateDocument's own scope. */
    using Element = Model::Element;

    /** Constructs an empty template document, holding no parsed files. */
    TemplateDocument() = default;

    /**
     * @brief Parses every @c .cast file @p model's index declares,
     *        keyed by its own symbol, and stamps each of its code blocks
     *        through addStamps().
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

                for (auto* block : *documents.at (symbol).getRoot())
                    if (block->contains (Id::type)
                        and *block->get<int> (Id::type) == map::BlockType::codeBlock)
                        addStamps (*block);
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
    const Element* getCodeBlock (const Element& line) const
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
    const Element* getCodeBlock (const juce::String& templatePath, const juce::Identifier& fence) const
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
    const juce::String& getBlockValue (const Model& model, const Element& row, const juce::String& address) const
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
    juce::String getValue (const Model& model, const Element& row, const juce::String& value) const
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
    /**
     * @brief Stamps @p block with its own text, in which each date token
     *        is already rendered through getDatedText() with getDate(),
     *        under @c Id::value, and with the normalized names of the
     *        markers that text places, excluding the @c :::\[banner\]:::
     *        marker, under @c Id::placeholder. A date token therefore
     *        never appears among the placeholder names.
     *
     * @param block The code block stamped.
     */
    static void addStamps (Element& block)
    {
        static const juce::Identifier bannerMarker { jam::Format::toValidID (
            jam::Format::withEnclosure (Id::banner.toString(), Chars::openBracket)) };

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

    /**
     * @brief Returns the UTC calendar date, read from the clock one time
     *        for the process; every date token of the run renders from it.
     *
     * @returns The process's own UTC calendar date.
     */
    static const std::tm& getDate() noexcept
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

    /**
     * @brief Returns the text that opens a date token's interior -- the
     *        open bracket, the reserved word, the colon.
     *
     * @returns The date token interior's own opening text.
     */
    static const juce::String& getDatePrefix()
    {
        static const juce::String datePrefix { juce::String::charToString (Chars::openBracket)
                                               + Id::date.toString()
                                               + juce::String::charToString (Chars::colon) };

        return datePrefix;
    }

    /**
     * @brief Returns true when @p interior opens with getDatePrefix() and
     *        closes with the close bracket.
     *
     * @param interior A marker's own interior text.
     * @returns True when @p interior is a date token's interior.
     */
    static bool isDateMarker (const juce::String& interior)
    {
        return interior.startsWith (getDatePrefix()) and interior.endsWithChar (Chars::closeBracket);
    }

    /**
     * @brief Returns the digits that @c std::strftime writes for one
     *        conversion word.
     *
     * @param date       The calendar date written.
     * @param conversion The @c std::strftime conversion word.
     * @returns The digits @p conversion writes for @p date.
     */
    static juce::String getDateField (const std::tm& date, const char* conversion)
    {
        static constexpr int fieldCapacity { 8 };

        std::array<char, fieldCapacity> field {};
        std::strftime (field.data(), field.size(), conversion, &date);

        return juce::String (field.data());
    }

    /**
     * @brief Returns the format between the prefix and the last close
     *        bracket of @p interior, with each pattern of the date pattern
     *        table replaced by its date field, in table order (@c yyyy
     *        before @c yy), every other character unchanged. An empty
     *        format gives an empty value.
     *
     * @param interior A date token's own interior text.
     * @param date     The calendar date the patterns render.
     * @returns @p interior's own format, rendered from @p date.
     */
    static juce::String getDateValue (const juce::String& interior, const std::tm& date)
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

    /**
     * @brief Returns @p text with every date token replaced by its date
     *        value. Text with no date token is returned unchanged.
     *
     * @param text The text whose date tokens are replaced.
     * @param date The calendar date the date tokens render.
     * @returns @p text with its own date tokens rendered from @p date.
     */
    static juce::String getDatedText (const juce::String& text, const std::tm& date)
    {
        auto datedText { text };

        for (const auto& interior : getMarkers (text))
            if (isDateMarker (interior))
                datedText = jam::Format::replaceholder (datedText, interior, getDateValue (interior, date));

        return datedText;
    }

    /** Every parsed @c .cast template file, keyed by its own index symbol. */
    jam::HashMap<juce::String, jam::MarkdownDocument> documents;
};
