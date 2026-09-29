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
    static juce::MemoryBlock getPropertyList (const juce::NamedValueSet& properties)
    {
        constexpr int dictMarker { 0xd0 };
        const auto count { properties.size() };
        jassert (2 * count + 1 < oneByteLimit);

        juce::MemoryOutputStream list;
        juce::MemoryOutputStream table;
        const auto addObject = [&list, &table] (const juce::var& object)
        {
            table.writeShortBigEndian (static_cast<short> (list.getDataSize()));
            list << getObject (object);
        };

        list << propertyListMagic;
        table.writeShortBigEndian (static_cast<short> (list.getDataSize()));
        list << getMarker (dictMarker, count);

        for (int ref { 1 }; ref <= 2 * count; ++ref) list.writeByte (static_cast<char> (ref));
        for (const auto& property : properties) addObject (property.name.toString());
        for (const auto& property : properties) addObject (property.value);

        const auto tableOffset { static_cast<juce::int64> (list.getDataSize()) };
        list << table.getMemoryBlock();
        list << getTrailer (2 * count + 1, tableOffset);
        return list.getMemoryBlock();
    }

private:
    /**
     * @brief Encodes one scalar object.
     *
     * @param value A boolean, a string, or a number.
     * @returns The marker and payload of @p value.
     */
    static juce::MemoryBlock getObject (const juce::var& value)
    {
        static constexpr int falseMarker { 0x08 };
        static constexpr int trueMarker { 0x09 };
        static const std::array<std::pair<bool (*) (const juce::var&), juce::MemoryBlock (*) (const juce::var&)>, 3> encodings {
            { { [] (const juce::var& candidate) { return candidate.isBool(); },
                [] (const juce::var& boolean)
                {
                    const auto marker { static_cast<char> (boolean ? trueMarker : falseMarker) };
                    return juce::MemoryBlock (&marker, 1);
                } },
              { [] (const juce::var& candidate) { return candidate.isString(); },
                [] (const juce::var& text) { return getString (text.toString()); } },
              { [] (const juce::var& candidate) { return candidate.isBinaryData(); },
                [] (const juce::var& bytes) { return getData (*bytes.getBinaryData()); } } }
        };

        for (const auto& [isType, encode] : encodings)
            if (isType (value)) return encode (value);

        return getNumber (value);
    }

    /**
     * @brief Encodes a number.
     *
     * @param value An integer from 0 to 65535, or a real.
     * @returns A one-byte or two-byte integer object, or an eight-byte real
     *          object.
     */
    static juce::MemoryBlock getNumber (const juce::var& value)
    {
        constexpr int intMarker { 0x10 };
        constexpr int shortIntMarker { 0x11 };
        constexpr int realMarker { 0x23 };

        juce::MemoryOutputStream number;

        if (value.isInt())
        {
            const auto integer { static_cast<int> (value) };
            jassert (integer >= 0 and integer < twoByteLimit);

            if (integer < oneByteLimit)
            {
                number.writeByte (static_cast<char> (intMarker));
                number.writeByte (static_cast<char> (integer));
            }
            else
            {
                number.writeByte (static_cast<char> (shortIntMarker));
                number.writeShortBigEndian (static_cast<short> (integer));
            }
        }
        else
        {
            number.writeByte (static_cast<char> (realMarker));
            number.writeDoubleBigEndian (static_cast<double> (value));
        }

        return number.getMemoryBlock();
    }

    /**
     * @brief Encodes an ASCII string.
     *
     * @param text The string, shorter than 65536 characters.
     * @returns The marker, the length object when @p text has 15
     *          characters or more, and the characters.
     */
    static juce::MemoryBlock getString (const juce::String& text)
    {
        constexpr int stringMarker { 0x50 };

        jassert (text.isAscii() and text.length() < twoByteLimit);

        juce::MemoryOutputStream string;
        string << getMarker (stringMarker, text.length());
        string << text.toRawUTF8();
        return string.getMemoryBlock();
    }

    /**
     * @brief Encodes binary data.
     *
     * @param bytes The data, shorter than 65536 bytes.
     * @returns The marker, the length object when @p bytes has 15 bytes or
     *          more, and @p bytes.
     */
    static juce::MemoryBlock getData (const juce::MemoryBlock& bytes)
    {
        constexpr int dataMarker { 0x40 };

        const auto length { static_cast<int> (bytes.getSize()) };
        jassert (length < twoByteLimit);

        juce::MemoryOutputStream data;
        data << getMarker (dataMarker, length);
        data << bytes;
        return data.getMemoryBlock();
    }

    /**
     * @brief Encodes the marker byte of an object with a length.
     *
     * @param typeMarker The type bits of the marker.
     * @param length     The length of the object.
     * @returns One byte with the length in the low bits. When @p length is
     *          15 or more, the low bits are 15 and an integer object with
     *          the length follows.
     */
    static juce::MemoryBlock getMarker (int typeMarker, int length)
    {
        juce::MemoryOutputStream marker;

        if (length < maxShortLength)
        {
            marker.writeByte (static_cast<char> (typeMarker | length));
        }
        else
        {
            marker.writeByte (static_cast<char> (typeMarker | maxShortLength));
            marker << getNumber (length);
        }

        return marker.getMemoryBlock();
    }

    /**
     * @brief Returns the trailer.
     *
     * @param objectCount The number of objects in the list.
     * @param tableOffset The offset of the object table.
     * @returns The padding, the offset and reference sizes, @p objectCount,
     *          the top object index, and @p tableOffset.
     */
    static juce::MemoryBlock getTrailer (int objectCount, juce::int64 tableOffset)
    {
        constexpr size_t trailerPadding { 6 };
        constexpr int offsetIntSize { 2 };
        constexpr int objectRefSize { 1 };

        juce::MemoryOutputStream trailer;
        trailer.writeRepeatedByte (0, trailerPadding);
        trailer.writeByte (static_cast<char> (offsetIntSize));
        trailer.writeByte (static_cast<char> (objectRefSize));
        trailer.writeInt64BigEndian (objectCount);
        trailer.writeInt64BigEndian (0);
        trailer.writeInt64BigEndian (tableOffset);
        return trailer.getMemoryBlock();
    }

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
