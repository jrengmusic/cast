#include "Transforms.h"

namespace cast
{

static juce::String applyToUpper (const juce::String& input) noexcept
{
    return input.toUpperCase();
}

static juce::String applyToTitle (const juce::String& input) noexcept
{
    return jam::Format::toTitleCase (input);
}

static juce::String applyToKebab (const juce::String& input) noexcept
{
    return jam::Format::toKebab (input);
}

static juce::String applySymbolFromFile (const juce::String& input) noexcept
{
    return input.replaceCharacter ('.', '_');
}

static juce::String applyQualifySymbol (const juce::String& input) noexcept
{
    const auto count { (input.length() - input.replace ("::", "").length()) / 2 };
    return count == 1 ? juce::String ("juce::") + input : input;
}

static juce::juce_wchar parseCodepoint (const juce::String& input) noexcept
{
    const auto isHex { input.startsWith ("0x") or input.startsWith ("0X") };
    return isHex ? static_cast<juce::juce_wchar> (input.substring (2).getHexValue32())
                 : input.getCharPointer().getAndAdvance();
}

static juce::String encodeCodepointAsEscapedUtf8 (juce::juce_wchar codepoint) noexcept
{
    char buf[8] {};
    juce::CharPointer_UTF8 writer { buf };
    writer.write (codepoint);

    juce::String result;

    for (int i { 0 }; i < 8 and buf[i] != 0; ++i)
        result += "\\x" + juce::String::toHexString (static_cast<int> (static_cast<unsigned char> (buf[i]))).paddedLeft ('0', 2).toUpperCase();

    return result;
}

static juce::String applyEscapeCpp (const juce::String& input) noexcept
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
        else if (byte > 127)
            result += "\\x" + juce::String::toHexString (static_cast<int> (byte)).paddedLeft ('0', 2).toUpperCase();
        else
            result += static_cast<juce::juce_wchar> (byte);
    }

    return result;
}

static juce::String encodeUPlusToken (const juce::String& token) noexcept
{
    assert (token.startsWith ("U+") and "utf8Bytes: token must be U+XXXX");

    const auto cp { static_cast<juce::juce_wchar> (token.substring (2).getHexValue32()) };
    return encodeCodepointAsEscapedUtf8 (cp);
}

static juce::String applyUtf8Bytes (const juce::String& input) noexcept
{
    juce::String result;
    juce::String token;
    auto cursor { input.getCharPointer() };

    while (not cursor.isEmpty())
    {
        const auto ch { cursor.getAndAdvance() };

        if (ch == ' ' or ch == '\t')
        {
            result += encodeUPlusToken (token);
            token.clear();
        }
        else
        {
            token += ch;
        }
    }

    result += encodeUPlusToken (token);
    return result;
}

static juce::String applyCodepointHex (const juce::String& input) noexcept
{
    const auto cp { parseCodepoint (input) };
    return "0x" + juce::String::toHexString (static_cast<int> (cp)).paddedLeft ('0', 4).toUpperCase();
}

static juce::String applyCodepointLabel (const juce::String& input) noexcept
{
    const auto cp { parseCodepoint (input) };
    return "U+" + juce::String::toHexString (static_cast<int> (cp)).paddedLeft ('0', 4).toUpperCase();
}

using TransformFn = juce::String (*) (const juce::String&);

static const std::array<std::pair<const char*, TransformFn>, 9> transforms
{{
    { "toUpper",        &applyToUpper },
    { "toTitle",        &applyToTitle },
    { "toKebab",        &applyToKebab },
    { "escapeCpp",      &applyEscapeCpp },
    { "utf8Bytes",      &applyUtf8Bytes },
    { "codepointHex",   &applyCodepointHex },
    { "codepointLabel", &applyCodepointLabel },
    { "qualifySymbol",  &applyQualifySymbol },
    { "symbolFromFile", &applySymbolFromFile }
}};

juce::Result Transforms::apply (const juce::String& name, const juce::String& input, juce::String& output)
{
    for (const auto& [entryName, entryFn] : transforms)
    {
        if (juce::StringRef (entryName) == name)
        {
            output = entryFn (input);
            return juce::Result::ok();
        }
    }

    return juce::Result::fail ("unknown transform: " + name);
}

} // namespace cast
