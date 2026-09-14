#pragma once

namespace jam
{ /*____________________________________________________________________________*/

// UTF-8-backed attributed text — std::string storage avoids the O(n^2) rescan
// juce::String::appendCharPointer performs on repeated append() (measured
// 765x slower than std::string at 100k appends). Codepoint count is tracked
// incrementally on append rather than recomputed, and attributes are
// stored as a flat, ascending, non-overlapping sequence (one attribute per
// append()) rather than juce::AttributedString::Attribute's overlap-checked
// list, enabling the attribute-cursor walk in jam::terminal::TextLayout.
class AttributedString
{
public:
    // RFC 3629 UTF-8 lead-byte masks/tags -- shared by CodepointIterator's
    // sequence-length lookup and append()'s continuation-byte scan.
    static constexpr unsigned char asciiMask { 0x80 };
    static constexpr unsigned char asciiTag { 0x00 };
    static constexpr unsigned char continuationMask { 0xC0 };
    static constexpr unsigned char continuationTag { 0x80 };
    static constexpr unsigned char twoByteMask { 0xE0 };
    static constexpr unsigned char twoByteTag { 0xC0 };
    static constexpr unsigned char threeByteMask { 0xF0 };
    static constexpr unsigned char threeByteTag { 0xE0 };
    static constexpr unsigned char continuationDataMask { 0x3F };
    static constexpr int continuationDataBits { 6 };

    // Mirrors juce::AttributedString::Attribute's lexicon. Packs
    // (codepointOffset, codepointLength) into a jam::Union rather than two
    // separate ints to keep the attribute footprint aligned with
    // juce::Range<int>'s size.
    struct Attribute
    {
        jam::Union<uint32_t, uint32_t> span;
        juce::Font font;
        juce::Colour colour;
    };

    struct Link
    {
        juce::Range<int> range;
        juce::String url;
    };

    struct CodeSpan
    {
        juce::Range<int> range;
        juce::Colour background;
    };

    // Decodes one UTF-8 codepoint per increment. Bounds are trusted byte
    // pointers into the owning AttributedString's text (RFC 3629 sequences,
    // 1-4 bytes, from juce-produced source text) -- no validation performed.
    class CodepointIterator
    {
    public:
        explicit CodepointIterator (const char* position) noexcept
            : ptr (position)
        {
        }

        uint32_t operator* () const noexcept
        {
            const auto leadByte { static_cast<unsigned char> (*ptr) };
            const auto length { sequenceLength (leadByte) };

            if (length == 1)
                return leadByte;

            // Lead byte carries (7 - length) data bits below its length-marker prefix.
            uint32_t codepoint { static_cast<uint32_t> (leadByte & (0xFFu >> (length + 1))) };

            for (int i { 1 }; i < length; ++i)
                codepoint = (codepoint << continuationDataBits)
                          | (static_cast<unsigned char> (ptr[i]) & continuationDataMask);

            return codepoint;
        }

        CodepointIterator& operator++ () noexcept
        {
            ptr += sequenceLength (static_cast<unsigned char> (*ptr));
            return *this;
        }

        bool operator!= (const CodepointIterator& other) const noexcept
        {
            return ptr != other.ptr;
        }

    private:
        static int sequenceLength (unsigned char leadByte) noexcept
        {
            if ((leadByte & asciiMask) == asciiTag)
                return 1;

            if ((leadByte & twoByteMask) == twoByteTag)
                return 2;

            if ((leadByte & threeByteMask) == threeByteTag)
                return 3;

            return 4;
        }

        const char* ptr;
    };

    AttributedString() = default;

    // jam::Array is move-only (no accidental deep copies); an explicit
    // element-wise deep copy is provided here.
    AttributedString (const AttributedString& other)
        : text (other.text), numCodepoints (other.numCodepoints)
    {
        for (const auto& attribute : other.attributes)
            attributes.add (attribute);

        for (const auto& link : other.links)
            links.add (link);

        for (const auto& range : other.strikethroughs)
            strikethroughs.add (range);

        for (const auto& codeSpan : other.codeSpans)
            codeSpans.add (codeSpan);
    }

    AttributedString& operator= (const AttributedString& other)
    {
        AttributedString copy { other };
        *this = std::move (copy);
        return *this;
    }

    AttributedString (AttributedString&&) noexcept = default;
    AttributedString& operator= (AttributedString&&) noexcept = default;

    // Converts newText to UTF-8 once, appends the bytes, counts the appended
    // codepoints in the same pass (a byte starts a new codepoint whenever it
    // is not a UTF-8 continuation byte), and records one ascending attribute.
    void append (const juce::String& newText, const juce::Font& font, juce::Colour colour)
    {
        const auto* utf8 { newText.toRawUTF8() };
        const auto numBytes { newText.getNumBytesAsUTF8() };

        int appendedCodepoints { 0 };

        for (size_t i { 0 }; i < numBytes; ++i)
            if ((static_cast<unsigned char> (utf8[i]) & continuationMask) != continuationTag)
                ++appendedCodepoints;

        const auto startCodepoint { static_cast<uint32_t> (numCodepoints) };

        if (not attributes.isEmpty())
        {
            const auto [previousOffset, previousLength] { attributes[attributes.size() - 1].span };
            jassert (previousOffset + previousLength == startCodepoint);
        }

        text.append (utf8, numBytes);
        numCodepoints += appendedCodepoints;

        const auto span { jam::Union<uint32_t, uint32_t>::pack (
            startCodepoint, static_cast<uint32_t> (appendedCodepoints)) };

        attributes.add ({ span, font, colour });
    }

    void addLink (juce::Range<int> range, const juce::String& url)
    {
        links.add ({ range, url });
    }

    void addStrikethrough (juce::Range<int> range)
    {
        strikethroughs.add (range);
    }

    void addCodeSpan (juce::Range<int> range, juce::Colour background)
    {
        codeSpans.add ({ range, background });
    }

    bool isEmpty() const noexcept
    {
        return text.empty();
    }

    int getNumCodepoints() const noexcept
    {
        return numCodepoints;
    }

    const jam::Array<Attribute>& getAttributes() const noexcept
    {
        return attributes;
    }

    const juce::Array<Link>& getLinks() const noexcept
    {
        return links;
    }

    const juce::Array<juce::Range<int>>& getStrikethroughs() const noexcept
    {
        return strikethroughs;
    }

    const juce::Array<CodeSpan>& getCodeSpans() const noexcept
    {
        return codeSpans;
    }

    CodepointIterator begin() const noexcept
    {
        return CodepointIterator { text.data() };
    }

    CodepointIterator end() const noexcept
    {
        return CodepointIterator { text.data() + text.size() };
    }

    // One-pass conversion for the juce::TextLayout fallback path. Codepoint offsets
    // index juce::String identically to jam's codepoint count -- both decode UTF-8/16
    // per character rather than per code unit.
    juce::AttributedString toJuce() const
    {
        juce::AttributedString result;
        const juce::String fullText { juce::String::fromUTF8 (text.data(), static_cast<int> (text.size())) };

        for (const auto& attribute : attributes)
        {
            const auto [offset, length] { attribute.span };
            result.append (fullText.substring (static_cast<int> (offset), static_cast<int> (offset + length)),
                           attribute.font, attribute.colour);
        }

        return result;
    }

private:
    std::string text;
    int numCodepoints { 0 };
    jam::Array<Attribute> attributes;
    juce::Array<Link> links;
    juce::Array<juce::Range<int>> strikethroughs;
    juce::Array<CodeSpan> codeSpans;
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
