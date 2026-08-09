#pragma once
#include <JuceHeader.h>

namespace cast
{
/*____________________________________________________________________________*/

static constexpr int asciiMax { 127 };
static constexpr int byteHexDigits { 2 };
static constexpr int codepointHexDigits { 4 };
static constexpr size_t hexEscapeCharsPerByte { 4 };///< Bytes per `\xNN` escape, for reserve() sizing.

/// @brief SPEC §6.9 `symbolFromFile` — filename with dots replaced by underscores (BinaryData symbol).
static juce::String symbolFromFile (const juce::String& input) noexcept
{
    return input.replaceCharacter (Chars::dot, Chars::underscore);
}

/// @brief SPEC §6.8 `qualifySymbol` — two-part `A::b` symbol to `juce::A::b`; three-plus parts verbatim.
static juce::String qualifySymbol (const juce::String& input) noexcept
{
    const auto count { (input.length() - input.replace (Id::scopeResolution, juce::String()).length()) / 2 };
    return count == 1 ? Id::juceNamespace + input : input;
}

/**
 * @brief Parses a glyph or `0xNN` cell into its codepoint.
 *
 * @param input A single glyph, or a `0x`/`0X`-prefixed hex codepoint literal.
 * @return The parsed codepoint.
 */
static juce::juce_wchar parseCodepoint (const juce::String& input) noexcept
{
    const auto isHex { input.startsWithIgnoreCase (Id::hexPrefix) };
    return isHex ? static_cast<juce::juce_wchar> (input.substring (2).getHexValue32())
                 : input.getCharPointer().getAndAdvance();
}

/**
 * @brief Encodes a codepoint as a sequence of `\xNN` UTF-8 byte escapes.
 *
 * @param codepoint The codepoint to encode.
 * @return The codepoint's UTF-8 bytes, each rendered as `\xNN`.
 */
static juce::String encodeCodepointAsEscapedUtf8 (juce::juce_wchar codepoint) noexcept
{
    const auto encoded { juce::String::charToString (codepoint) };
    const auto* raw { encoded.getCharPointer().getAddress() };

    std::string result;
    result.reserve (static_cast<size_t> (encoded.getNumBytesAsUTF8()) * hexEscapeCharsPerByte);

    for (int i { 0 }; raw[i] != 0; ++i)
    {
        const auto byte { static_cast<unsigned char> (raw[i]) };
        const auto escaped { Id::hexEscapePrefix + juce::String::toHexString (static_cast<int> (byte)).paddedLeft (Chars::zero, byteHexDigits) };
        result += escaped.toRawUTF8();
    }

    return juce::String::fromUTF8 (result.data(), static_cast<int> (result.size()));
}

/// @brief SPEC §6.4 `escapeCpp` — C string literal escaping: `"` to `\"`, `\` to `\\`, non-ASCII bytes to `\xNN`.
static juce::String escapeCpp (const juce::String& input) noexcept
{
    const auto* raw { input.getCharPointer().getAddress() };

    std::string result;
    result.reserve (static_cast<size_t> (input.getNumBytesAsUTF8()) * hexEscapeCharsPerByte);

    for (int i { 0 }; raw[i] != 0; ++i)
    {
        const auto byte { static_cast<unsigned char> (raw[i]) };

        if (byte == Chars::doubleQuote)
            result += Id::escapedDoubleQuote.toRawUTF8();
        else if (byte == Chars::backslash)
            result += Id::escapedBackslash.toRawUTF8();
        else if (byte > asciiMax)
        {
            const auto escaped { Id::hexEscapePrefix + juce::String::toHexString (static_cast<int> (byte)).paddedLeft (Chars::zero, byteHexDigits) };
            result += escaped.toRawUTF8();
        }
        else
            result += static_cast<char> (byte);
    }

    return juce::String::fromUTF8 (result.data(), static_cast<int> (result.size()));
}

/**
 * @brief Encodes one `U+XXXX` codepoint token as `\xNN` UTF-8 byte escapes.
 *
 * @param token A single `U+XXXX` codepoint token.
 * @return The token's UTF-8 byte escape sequence.
 */
static juce::String encodeUPlusToken (const juce::String& token) noexcept
{
    jassert (token.startsWith (Id::codepointPrefix));

    const auto cp { static_cast<juce::juce_wchar> (token.substring (2).getHexValue32()) };
    return encodeCodepointAsEscapedUtf8 (cp);
}

/// @brief SPEC §6.5 `utf8Bytes` — `U+XXXX` codepoint token(s) to UTF-8 `\xNN` byte escape sequence.
static juce::String utf8Bytes (const juce::String& input) noexcept
{
    const auto tokens { juce::StringArray::fromTokens (input, false) };

    std::string result;

    for (const auto& token : tokens)
        result += encodeUPlusToken (token).toRawUTF8();

    return juce::String::fromUTF8 (result.data(), static_cast<int> (result.size()));
}

/// @brief SPEC §6.6 `codepointHex` — glyph or `0xNN` cell to a minimal lowercase `0xNN` integer literal.
static juce::String codepointHex (const juce::String& input) noexcept
{
    const auto cp { parseCodepoint (input) };
    return Id::hexPrefix + juce::String::toHexString (static_cast<int> (cp));
}

/// @brief SPEC §6.7 `codepointLabel` — codepoint to zero-padded uppercase `U+XXXX` notation.
static juce::String codepointLabel (const juce::String& input) noexcept
{
    const auto cp { parseCodepoint (input) };
    return Id::codepointPrefix + juce::String::toHexString (static_cast<int> (cp)).paddedLeft (Chars::zero, codepointHexDigits).toUpperCase();
}

/// @brief SPEC §6.10 `toString` — bare identifier word to `Id::word.toString()`.
static juce::String toString (const juce::String& input) noexcept
{
    return Id::idNamespace + input + Id::toStringSuffix;
}

/// @brief SPEC §6 transform vocabulary lookup.
struct Transforms
{
    /** @brief Reports whether @p name is in the closed transform vocabulary (SPEC §6). */
    static bool contains (const juce::String& name) noexcept
    {
        return getTransforms().contains (name);
    }

    /**
     * @brief Applies the named transform to @p input.
     *
     * @param name  The transform's identifier string; must be in the closed
     *              vocabulary (SPEC §6).
     * @param input The value to transform.
     * @return The transformed value.
     * @note Asserts @p name is a known transform.
     */
    static juce::String getTransformed (const juce::String& name, const juce::String& input)
    {
        jassert (contains (name));

        return getTransforms().get (name, input);
    }

    /** @brief The closed transform vocabulary (SPEC §6), keyed by identifier string. */
    static const jam::Function::Map<juce::String, juce::String>& getTransforms() noexcept
    {
        static const jam::Function::Map<juce::String, juce::String> transforms { [] ()
        {
            jam::Function::Map<juce::String, juce::String> map;

            map.add<const juce::String&> (Id::toUpper.toString(),
                [] (const juce::String& input) { return input.toUpperCase(); });
            map.add<const juce::String&> (Id::toTitle.toString(), &jam::Format::toTitleCase);
            map.add<const juce::String&> (Id::toKebab.toString(), &jam::Format::toKebabCase);
            map.add<const juce::String&> (Id::toPascal.toString(), &jam::Format::toPascalCase);
            map.add<const juce::String&> (Id::toCamel.toString(), &jam::Format::toCamelCase);
            map.add<const juce::String&> (Id::escapeCpp.toString(), &escapeCpp);
            map.add<const juce::String&> (Id::utf8Bytes.toString(), &utf8Bytes);
            map.add<const juce::String&> (Id::codepointHex.toString(), &codepointHex);
            map.add<const juce::String&> (Id::codepointLabel.toString(), &codepointLabel);
            map.add<const juce::String&> (Id::qualifySymbol.toString(), &qualifySymbol);
            map.add<const juce::String&> (Id::symbolFromFile.toString(), &symbolFromFile);
            map.add<const juce::String&> (Id::toString.toString(), &toString);

            return map;
        }() };

        return transforms;
    }
};

/**______________________________END OF NAMESPACE______________________________*/
}// namespace cast
