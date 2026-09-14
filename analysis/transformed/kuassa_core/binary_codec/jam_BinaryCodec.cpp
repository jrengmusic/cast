/**
 * @file jam_BinaryCodec.cpp
 * @brief Little-endian binary encode/decode primitives — implementation.
 *
 * @see jam_BinaryCodec.h
 */

#include <cstring>

namespace jam
{
/*____________________________________________________________________________*/

// =============================================================================
// Writers
// =============================================================================

void BinaryCodec::writeUint16 (juce::MemoryBlock& block, uint16_t value) noexcept
{
    block.append (&value, sizeof (value));
}

void BinaryCodec::writeUint32 (juce::MemoryBlock& block, uint32_t value) noexcept
{
    block.append (&value, sizeof (value));
}

void BinaryCodec::writeUint64 (juce::MemoryBlock& block, uint64_t value) noexcept
{
    block.append (&value, sizeof (value));
}

void BinaryCodec::writeInt32 (juce::MemoryBlock& block, int32_t value) noexcept
{
    block.append (&value, sizeof (value));
}

void BinaryCodec::writeString (juce::MemoryBlock& block, const juce::String& str) noexcept
{
    const auto* utf8 { str.toRawUTF8() };
    const auto len { static_cast<uint32_t> (str.getNumBytesAsUTF8()) };
    block.append (&len, sizeof (len));
    block.append (utf8, static_cast<size_t> (len));
}

// =============================================================================
// Readers
// =============================================================================

uint16_t BinaryCodec::readUint16 (const uint8_t* data) noexcept
{
    uint16_t val { 0 };
    std::memcpy (&val, data, sizeof (val));
    return val;
}

uint32_t BinaryCodec::readUint32 (const uint8_t* data) noexcept
{
    uint32_t val { 0 };
    std::memcpy (&val, data, sizeof (val));
    return val;
}

uint64_t BinaryCodec::readUint64 (const uint8_t* data) noexcept
{
    uint64_t val { 0 };
    std::memcpy (&val, data, sizeof (val));
    return val;
}

int32_t BinaryCodec::readInt32 (const uint8_t* data) noexcept
{
    int32_t val { 0 };
    std::memcpy (&val, data, sizeof (val));
    return val;
}

int BinaryCodec::readString (const uint8_t* data, int available, juce::String& out) noexcept
{
    int consumed { 0 };

    if (available >= 4)
    {
        uint32_t len { 0 };
        std::memcpy (&len, data, sizeof (len));

        if (available >= static_cast<int> (4 + len))
        {
            // uint8_t* → const char* pun for juce::String UTF-8 API — char-family aliasing-legal.
            out = juce::String::fromUTF8 (reinterpret_cast<const char*> (data + 4),
                                          static_cast<int> (len));
            consumed = static_cast<int> (4 + len);
        }
    }

    return consumed;
}

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
