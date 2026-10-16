#include "AliasWriter.h"

juce::MemoryBlock AliasWriter::getAlias (const juce::String& volumeName, const juce::String& fileName)
{
    juce::MemoryOutputStream alias;
    alias << getFixedRecord (volumeName, fileName);
    alias << getTags (volumeName, fileName);

    const auto recordLength { static_cast<short> (alias.getDataSize()) };
    alias.setPosition (recordLengthOffset);
    alias.writeShortBigEndian (recordLength);
    return alias.getMemoryBlock();
}

juce::MemoryBlock AliasWriter::getUnicodeUnits (const juce::String& text)
{
    const auto units { text.toUTF16() };
    const auto unitCount { juce::CharPointer_UTF16::getBytesRequiredFor (text.getCharPointer())
                           / sizeof (juce::CharPointer_UTF16::CharType) };
    const juce::Span<const juce::CharPointer_UTF16::CharType> textUnits { units.getAddress(), unitCount };

    juce::MemoryOutputStream unicode;

    for (const auto unit : textUnits)
        unicode.writeShortBigEndian (static_cast<short> (unit));

    return unicode.getMemoryBlock();
}

juce::MemoryBlock AliasWriter::getFixedRecord (const juce::String& volumeName, const juce::String& fileName)
{
    juce::MemoryOutputStream record;
    record.writeRepeatedByte (0, appInfoSize);
    record.writeShortBigEndian (recordLengthPlaceholder);
    record.writeShortBigEndian (aliasVersion);
    record.writeShortBigEndian (fileKind);
    record << getPascalString (volumeName, volumeNameSize);
    record.writeIntBigEndian (noDate);
    record << fileSystemType;
    record.writeShortBigEndian (fixedDiskType);
    record.writeIntBigEndian (static_cast<int> (rootFolderId));
    record << getPascalString (fileName, fileNameSize);
    record.writeIntBigEndian (static_cast<int> (noNodeId));
    record.writeIntBigEndian (noDate);
    record.writeRepeatedByte (0, creatorCodeSize);
    record.writeRepeatedByte (0, typeCodeSize);
    record.writeShortBigEndian (noLevel);
    record.writeShortBigEndian (noLevel);
    record.writeIntBigEndian (noVolumeFlags);
    record.writeRepeatedByte (0, fileSystemIdSize);
    record.writeRepeatedByte (0, reservedSize);
    return record.getMemoryBlock();
}

juce::MemoryBlock AliasWriter::getTags (const juce::String& volumeName, const juce::String& fileName)
{
    const auto getBytes = [] (const juce::String& text)
    {
        return juce::MemoryBlock (text.toRawUTF8(), text.getNumBytesAsUTF8());
    };
    const juce::MemoryBlock zeroDate { dateSize, true };

    juce::MemoryOutputStream tags;
    tags << getTag (tagFolderName, getBytes (getCarbonName (volumeName)));
    tags << getTag (tagVolumeDate, zeroDate);
    tags << getTag (tagCreationDate, zeroDate);
    tags << getTag (tagCarbonPath, getBytes (getCarbonName (volumeName) + juce::String::charToString (Chars::colon) + getCarbonName (fileName)));
    tags << getTag (tagUnicodeFileName, getUnicodeName (fileName));
    tags << getTag (tagUnicodeVolumeName, getUnicodeName (volumeName));
    tags << getTag (tagPosixPath, getBytes (juce::String::charToString (Chars::slash) + fileName));
    tags << getTag (tagMountPoint, getBytes (mountPointPrefix + volumeName));
    tags.writeShortBigEndian (tagEnd);
    tags.writeShortBigEndian (tagEndLength);
    return tags.getMemoryBlock();
}

juce::MemoryBlock AliasWriter::getPascalString (const juce::String& text, int size)
{
    const auto carbonName { getCarbonName (text) };
    const auto length { juce::jmin (carbonName.getNumBytesAsUTF8(), static_cast<size_t> (size - lengthByteSize)) };
    const auto lengthByte { static_cast<char> (length) };

    juce::MemoryBlock pascalString { static_cast<size_t> (size), true };
    pascalString.copyFrom (&lengthByte, 0, sizeof (lengthByte));
    pascalString.copyFrom (carbonName.toRawUTF8(), lengthByteSize, length);
    return pascalString;
}

juce::MemoryBlock AliasWriter::getTag (int tag, const juce::MemoryBlock& bytes)
{
    juce::MemoryOutputStream record;
    record.writeShortBigEndian (static_cast<short> (tag));
    record.writeShortBigEndian (static_cast<short> (bytes.getSize()));
    record << bytes;
    record.writeRepeatedByte (0, bytes.getSize() % 2);
    return record.getMemoryBlock();
}

juce::MemoryBlock AliasWriter::getUnicodeName (const juce::String& text)
{
    const auto units { getUnicodeUnits (getCarbonName (text)) };

    juce::MemoryOutputStream unicode;
    unicode.writeShortBigEndian (static_cast<short> (units.getSize() / sizeof (juce::CharPointer_UTF16::CharType)));
    unicode << units;
    return unicode.getMemoryBlock();
}

juce::String AliasWriter::getCarbonName (const juce::String& text)
{
    return text.replaceCharacter (Chars::colon, Chars::slash);
}
