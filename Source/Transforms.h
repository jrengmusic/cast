#pragma once
#include <JuceHeader.h>

namespace cast
{

static constexpr int asciiMax { 127 };
static constexpr int byteHexDigits { 2 };
static constexpr int codepointHexDigits { 4 };

/// @brief SPEC §6.1 `toUpper` — uppercase the value.
static juce::String toUpper (const juce::String& input) noexcept
{
    return input.toUpperCase();
}

/// @brief SPEC §6.2 `toTitle` — titlecase display names.
static juce::String toTitle (const juce::String& input) noexcept
{
    return jam::Format::toTitleCase (input);
}

/// @brief SPEC §6.3 `toKebab` — kebab-case.
static juce::String toKebab (const juce::String& input) noexcept
{
    return jam::Format::toKebab (input);
}

/// @brief SPEC §6.9 `symbolFromFile` — filename with dots replaced by underscores (BinaryData symbol).
static juce::String symbolFromFile (const juce::String& input) noexcept
{
    return input.replaceCharacter ('.', '_');
}

/// @brief SPEC §6.8 `qualifySymbol` — two-part `A::b` symbol to `juce::A::b`; three-plus parts verbatim.
static juce::String qualifySymbol (const juce::String& input) noexcept
{
    const auto count { (input.length() - input.replace ("::", "").length()) / 2 };
    return count == 1 ? juce::String ("juce::") + input : input;
}

/**
 * @brief Parses a glyph or `0xNN` cell into its codepoint.
 *
 * @param input A single glyph, or a `0x`/`0X`-prefixed hex codepoint literal.
 * @return The parsed codepoint.
 */
static juce::juce_wchar parseCodepoint (const juce::String& input) noexcept
{
    const auto isHex { input.startsWith ("0x") or input.startsWith ("0X") };
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
    const auto* bytes { reinterpret_cast<const unsigned char*> (juce::String::charToString (codepoint).toRawUTF8()) };

    juce::String result;

    for (int i { 0 }; bytes[i] != 0; ++i)
        result += "\\x" + juce::String::toHexString (static_cast<int> (bytes[i])).paddedLeft ('0', byteHexDigits).toUpperCase();

    return result;
}

/// @brief SPEC §6.4 `escapeCpp` — C string literal escaping: `"` to `\"`, `\` to `\\`, non-ASCII bytes to `\xNN`.
static juce::String escapeCpp (const juce::String& input) noexcept
{
    juce::String result;
    const auto* bytes { reinterpret_cast<const unsigned char*> (input.toRawUTF8()) };

    for (int i { 0 }; bytes[i] != 0; ++i)
    {
        const auto byte { bytes[i] };

        if (byte == '"')
            result += "\\\"";
        else if (byte == '\\')
            result += "\\\\";
        else if (byte > asciiMax)
            result += "\\x" + juce::String::toHexString (static_cast<int> (byte)).paddedLeft ('0', byteHexDigits).toUpperCase();
        else
            result += static_cast<juce::juce_wchar> (byte);
    }

    return result;
}

/**
 * @brief Encodes one `U+XXXX` codepoint token as `\xNN` UTF-8 byte escapes.
 *
 * @param token A single `U+XXXX` codepoint token.
 * @return The token's UTF-8 byte escape sequence.
 */
static juce::String encodeUPlusToken (const juce::String& token) noexcept
{
    jassert (token.startsWith ("U+"));

    const auto cp { static_cast<juce::juce_wchar> (token.substring (2).getHexValue32()) };
    return encodeCodepointAsEscapedUtf8 (cp);
}

/// @brief SPEC §6.5 `utf8Bytes` — `U+XXXX` codepoint token(s) to UTF-8 `\xNN` byte escape sequence.
static juce::String utf8Bytes (const juce::String& input) noexcept
{
    const auto tokens { juce::StringArray::fromTokens (input, false) };

    juce::String result;

    for (const auto& token : tokens)
        result += encodeUPlusToken (token);

    return result;
}

/// @brief SPEC §6.6 `codepointHex` — glyph or `0xNN` cell to `0xNNNN` integer literal.
static juce::String codepointHex (const juce::String& input) noexcept
{
    const auto cp { parseCodepoint (input) };
    return "0x" + juce::String::toHexString (static_cast<int> (cp)).paddedLeft ('0', codepointHexDigits).toUpperCase();
}

/// @brief SPEC §6.7 `codepointLabel` — codepoint to zero-padded uppercase `U+XXXX` notation.
static juce::String codepointLabel (const juce::String& input) noexcept
{
    const auto cp { parseCodepoint (input) };
    return "U+" + juce::String::toHexString (static_cast<int> (cp)).paddedLeft ('0', codepointHexDigits).toUpperCase();
}

using TransformFn = juce::String (*) (const juce::String&);

/// @brief The closed transform vocabulary (SPEC §6), keyed by identifier string.
static const std::array<std::pair<juce::String, TransformFn>, 9> transforms
{{
    { Id::toUpper.toString(),        &toUpper },
    { Id::toTitle.toString(),        &toTitle },
    { Id::toKebab.toString(),        &toKebab },
    { Id::escapeCpp.toString(),      &escapeCpp },
    { Id::utf8Bytes.toString(),      &utf8Bytes },
    { Id::codepointHex.toString(),   &codepointHex },
    { Id::codepointLabel.toString(), &codepointLabel },
    { Id::qualifySymbol.toString(),  &qualifySymbol },
    { Id::symbolFromFile.toString(), &symbolFromFile }
}};

/// @brief SPEC §6 transform vocabulary lookup.
struct Transforms
{
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
        const auto found { std::find_if (transforms.begin(), transforms.end(),
            [&name] (const auto& entry) { return entry.first == name; }) };

        jassert (found != transforms.end());

        return found->second (input);
    }
};

} // namespace cast
