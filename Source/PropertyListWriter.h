#pragma once
#include <JuceHeader.h>

/**
 * @struct PropertyListWriter
 * @brief Encodes a flat dictionary of booleans, integers, reals, and
 *        ASCII strings as a binary property list (@c bplist00).
 *
 * PropertyListWriter keeps no state. It writes one dictionary with
 * single-byte object references and two-byte offsets.
 */
struct PropertyListWriter
{
    /**
     * @brief Encodes @p properties as a binary property list.
     *
     * @param properties The dictionary entries. It holds fewer than 15
     *                   entries. Each value is a boolean, an integer from
     *                   0 to 255, a real, or an ASCII string shorter than
     *                   256 characters.
     * @returns The magic, the dictionary, the object table, and the
     *          trailer.
     */
    static juce::MemoryBlock getPropertyList (const juce::NamedValueSet& properties)
    {
        constexpr int maxCount { 15 };
        constexpr int dictMarker { 0xd0 };
        const auto count { properties.size() };
        jassert (count < maxCount);

        juce::MemoryOutputStream list;
        juce::MemoryOutputStream table;
        const auto addObject = [&list, &table] (const juce::var& object)
        {
            table.writeShortBigEndian (static_cast<short> (list.getDataSize()));
            list << getObject (object);
        };

        list << propertyListMagic;
        table.writeShortBigEndian (static_cast<short> (list.getDataSize()));
        list.writeByte (static_cast<char> (dictMarker | count));

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
        constexpr int falseMarker { 0x08 };
        constexpr int trueMarker { 0x09 };

        juce::MemoryOutputStream object;

        if (value.isBool())
            object.writeByte (static_cast<char> (value ? trueMarker : falseMarker));
        else if (value.isString())
            object << getString (value.toString());
        else
            object << getNumber (value);

        return object.getMemoryBlock();
    }

    /**
     * @brief Encodes a number.
     *
     * @param value An integer from 0 to 255, or a real.
     * @returns A one-byte integer object or an eight-byte real object.
     */
    static juce::MemoryBlock getNumber (const juce::var& value)
    {
        constexpr int maxLength { 256 };
        constexpr int intMarker { 0x10 };
        constexpr int realMarker { 0x23 };

        juce::MemoryOutputStream number;

        if (value.isInt())
        {
            jassert (static_cast<int> (value) >= 0 and static_cast<int> (value) < maxLength);
            number.writeByte (static_cast<char> (intMarker));
            number.writeByte (static_cast<char> (static_cast<int> (value)));
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
     * @param text The string, shorter than 256 characters.
     * @returns The marker, the length object when @p text has 15
     *          characters or more, and the characters.
     */
    static juce::MemoryBlock getString (const juce::String& text)
    {
        constexpr int maxShortLength { 15 };
        constexpr int maxLength { 256 };
        constexpr int stringMarker { 0x50 };
        constexpr int longStringMarker { 0x5f };

        const auto length { text.length() };
        const auto isShort { length < maxShortLength };
        jassert (text.isAscii() and length < maxLength);

        juce::MemoryOutputStream string;
        string.writeByte (static_cast<char> (isShort ? stringMarker | length : longStringMarker));
        string << (isShort ? juce::MemoryBlock() : getObject (length));
        string << text.toRawUTF8();
        return string.getMemoryBlock();
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

    /** Magic that opens every binary property list. */
    static constexpr const char* propertyListMagic { "bplist00" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PropertyListWriter)
};
