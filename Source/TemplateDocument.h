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
    explicit TemplateDocument (const Model& model);

    /**
     * @brief Returns @p line's own shape's code block -- the fence named
     *        by @p line's own info, read from @p line's own resolved
     *        template file.
     *
     * @param line The structure line whose shape's code block is read.
     * @returns @p line's own shape's code block.
     */
    const Element* getCodeBlock (const Element& line) const;

    /**
     * @brief Returns @p fence's code block, read from @p templatePath's
     *        own parsed template file.
     *
     * @param templatePath The resolved @c .cast file @p fence is read
     *                     from.
     * @param fence        The fence name to read.
     * @returns @p fence's code block.
     */
    const Element* getCodeBlock (const juce::String& templatePath, const juce::Identifier& fence) const;

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
    const juce::String& getBlockValue (const Model& model, const Element& row, const juce::String& address) const;

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
    juce::String getValue (const Model& model, const Element& row, const juce::String& value) const;

    /**
     * @brief Returns every @c :::interior::: marker's own interior text
     *        authored in @p text, verbatim, in authored order -- the one
     *        scan every marker-reading member reads through.
     *
     * @param text The text scanned for its own markers.
     * @returns @p text's own marker interiors, in authored order.
     */
    static jam::Strings getMarkers (const juce::String& text);

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
    static void addStamps (Element& block);

    /**
     * @brief Returns the UTC calendar date, read from the clock one time
     *        for the process; every date token of the run renders from it.
     *
     * @returns The process's own UTC calendar date.
     */
    static const std::tm& getDate() noexcept;

    /**
     * @brief Returns the text that opens a date token's interior -- the
     *        open bracket, the reserved word, the colon.
     *
     * @returns The date token interior's own opening text.
     */
    static const juce::String& getDatePrefix();

    /**
     * @brief Returns true when @p interior opens with getDatePrefix() and
     *        closes with the close bracket.
     *
     * @param interior A marker's own interior text.
     * @returns True when @p interior is a date token's interior.
     */
    static bool isDateMarker (const juce::String& interior);

    /**
     * @brief Returns the digits that @c std::strftime writes for one
     *        conversion word.
     *
     * @param date       The calendar date written.
     * @param conversion The @c std::strftime conversion word.
     * @returns The digits @p conversion writes for @p date.
     */
    static juce::String getDateField (const std::tm& date, const char* conversion);

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
    static juce::String getDateValue (const juce::String& interior, const std::tm& date);

    /**
     * @brief Returns @p text with every date token replaced by its date
     *        value. Text with no date token is returned unchanged.
     *
     * @param text The text whose date tokens are replaced.
     * @param date The calendar date the date tokens render.
     * @returns @p text with its own date tokens rendered from @p date.
     */
    static juce::String getDatedText (const juce::String& text, const std::tm& date);

    /** Every parsed @c .cast template file, keyed by its own index symbol. */
    jam::HashMap<juce::String, jam::MarkdownDocument> documents;
};
