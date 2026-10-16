#include "PropertyListWriter.h"

juce::MemoryBlock PropertyListWriter::getPropertyList (const juce::NamedValueSet& properties)
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

juce::MemoryBlock PropertyListWriter::getObject (const juce::var& value)
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

juce::MemoryBlock PropertyListWriter::getNumber (const juce::var& value)
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

juce::MemoryBlock PropertyListWriter::getString (const juce::String& text)
{
    constexpr int stringMarker { 0x50 };

    jassert (juce::CharPointer_ASCII::isValidString (text.toRawUTF8(), static_cast<int> (text.getNumBytesAsUTF8()))
             and text.length() < twoByteLimit);

    juce::MemoryOutputStream string;
    string << getMarker (stringMarker, text.length());
    string << text.toRawUTF8();
    return string.getMemoryBlock();
}

juce::MemoryBlock PropertyListWriter::getData (const juce::MemoryBlock& bytes)
{
    constexpr int dataMarker { 0x40 };

    const auto length { static_cast<int> (bytes.getSize()) };
    jassert (length < twoByteLimit);

    juce::MemoryOutputStream data;
    data << getMarker (dataMarker, length);
    data << bytes;
    return data.getMemoryBlock();
}

juce::MemoryBlock PropertyListWriter::getMarker (int typeMarker, int length)
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

juce::MemoryBlock PropertyListWriter::getTrailer (int objectCount, juce::int64 tableOffset)
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
