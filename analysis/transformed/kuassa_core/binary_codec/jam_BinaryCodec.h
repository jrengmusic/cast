/**
 * @file jam_BinaryCodec.h
 * @brief Little-endian binary encode/decode primitives.
 *
 * Fixed-width integer and length-prefixed string serialisation over
 * juce::MemoryBlock (writers) and raw uint8_t* (readers).
 * No allocation beyond MemoryBlock::append.
 */

#pragma once

#include <cstdint>

namespace jam
{
/*____________________________________________________________________________*/

struct BinaryCodec
{
    // =============================================================================
    // Writers — append a value to a MemoryBlock.
    // =============================================================================

    /** @brief Appends a little-endian uint16_t to @p block. */
    static void writeUint16 (juce::MemoryBlock& block, uint16_t value) noexcept;

    /** @brief Appends a little-endian uint32_t to @p block. */
    static void writeUint32 (juce::MemoryBlock& block, uint32_t value) noexcept;

    /** @brief Appends a little-endian uint64_t to @p block. */
    static void writeUint64 (juce::MemoryBlock& block, uint64_t value) noexcept;

    /** @brief Appends a little-endian int32_t to @p block. */
    static void writeInt32 (juce::MemoryBlock& block, int32_t value) noexcept;

    /** @brief Appends a length-prefixed (uint32_t LE) UTF-8 string to @p block. */
    static void writeString (juce::MemoryBlock& block, const juce::String& str) noexcept;

    // =============================================================================
    // Readers — decode a value from a raw byte pointer.
    // =============================================================================

    /** @brief Reads a little-endian uint16_t from @p data (caller guarantees >= 2 bytes). */
    static uint16_t readUint16 (const uint8_t* data) noexcept;

    /** @brief Reads a little-endian uint32_t from @p data (caller guarantees >= 4 bytes). */
    static uint32_t readUint32 (const uint8_t* data) noexcept;

    /** @brief Reads a little-endian uint64_t from @p data (caller guarantees >= 8 bytes). */
    static uint64_t readUint64 (const uint8_t* data) noexcept;

    /** @brief Reads a little-endian int32_t from @p data (caller guarantees >= 4 bytes). */
    static int32_t readInt32 (const uint8_t* data) noexcept;

    /**
     * @brief Reads a length-prefixed UTF-8 string from @p data.
     *
     * @param data       Pointer to the uint32_t length field.
     * @param available  Bytes available from @p data.
     * @param out        Populated on success.
     * @return Bytes consumed (4 + len), or 0 if insufficient data.
     */
    static int readString (const uint8_t* data, int available, juce::String& out) noexcept;
};

/**______________________________END OF NAMESPACE______________________________*/
} // namespace jam
