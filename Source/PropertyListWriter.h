#pragma once
#include <JuceHeader.h>

/**
 * @struct PropertyListWriter
 * @brief Encodes a flat dictionary of booleans, integers, reals, ASCII
 *        strings, and binary data as a binary property list (@c bplist00).
 *
 * PropertyListWriter keeps no state. It writes one dictionary with
 * single-byte object references and two-byte offsets.
 */
struct PropertyListWriter
{
    /**
     * @brief Encodes @p properties as a binary property list.
     *
     * @param properties The dictionary entries. The entry count can be any
     *                   value whose object references fit one byte. Each
     *                   value is a boolean, an integer from 0 to 65535, a
     *                   real, an ASCII string, or binary data. A string
     *                   or binary data is shorter than 65536 units.
     * @returns The magic, the dictionary, the object table, and the
     *          trailer.
     */
    static juce::MemoryBlock getPropertyList (const juce::NamedValueSet& properties);

private:
    /**
     * @brief Encodes one scalar object.
     *
     * @param value A boolean, a string, or a number.
     * @returns The marker and payload of @p value.
     */
    static juce::MemoryBlock getObject (const juce::var& value);

    /**
     * @brief Encodes a number.
     *
     * @param value An integer from 0 to 65535, or a real.
     * @returns A one-byte or two-byte integer object, or an eight-byte real
     *          object.
     */
    static juce::MemoryBlock getNumber (const juce::var& value);

    /**
     * @brief Encodes an ASCII string.
     *
     * @param text The string, shorter than 65536 characters.
     * @returns The marker, the length object when @p text has 15
     *          characters or more, and the characters.
     */
    static juce::MemoryBlock getString (const juce::String& text);

    /**
     * @brief Encodes binary data.
     *
     * @param bytes The data, shorter than 65536 bytes.
     * @returns The marker, the length object when @p bytes has 15 bytes or
     *          more, and @p bytes.
     */
    static juce::MemoryBlock getData (const juce::MemoryBlock& bytes);

    /**
     * @brief Encodes the marker byte of an object with a length.
     *
     * @param typeMarker The type bits of the marker.
     * @param length     The length of the object.
     * @returns One byte with the length in the low bits. When @p length is
     *          15 or more, the low bits are 15 and an integer object with
     *          the length follows.
     */
    static juce::MemoryBlock getMarker (int typeMarker, int length);

    /**
     * @brief Returns the trailer.
     *
     * @param objectCount The number of objects in the list.
     * @param tableOffset The offset of the object table.
     * @returns The padding, the offset and reference sizes, @p objectCount,
     *          the top object index, and @p tableOffset.
     */
    static juce::MemoryBlock getTrailer (int objectCount, juce::int64 tableOffset);

    /** Length that the low bits of a marker hold. A longer length follows as an integer object. */
    static constexpr int maxShortLength { 15 };
    /** First value that does not fit in one byte. */
    static constexpr int oneByteLimit { 256 };
    /** First value that does not fit in two bytes. */
    static constexpr int twoByteLimit { 65536 };

    /** Magic that opens every binary property list. */
    static constexpr const char* propertyListMagic { "bplist00" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PropertyListWriter)
};
