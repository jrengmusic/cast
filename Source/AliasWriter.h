#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"

/**
 * @struct AliasWriter
 * @brief Builds a version-2 Mac alias record to a file at the root of a
 *        volume that is not mounted yet.
 *
 * The record holds the volume name and the file name. The parent node
 * identifier is 2, the HFS+ root folder. The file node identifier is unknown,
 * so it is the value 0xffffffff. The dates are zero. The file system is HFS+.
 * AliasWriter keeps no state.
 */
struct AliasWriter
{
    /**
     * @brief Builds the alias record.
     *
     * @param volumeName The name of the volume.
     * @param fileName   The name of the file at the root of the volume.
     * @returns The fixed record followed by the tags. The record length is
     *          written into the fixed record.
     */
    static juce::MemoryBlock getAlias (const juce::String& volumeName, const juce::String& fileName)
    {
        juce::MemoryOutputStream alias;
        alias << getFixedRecord (volumeName, fileName);
        alias << getTags (volumeName, fileName);

        const auto recordLength { static_cast<short> (alias.getDataSize()) };
        alias.setPosition (recordLengthOffset);
        alias.writeShortBigEndian (recordLength);
        return alias.getMemoryBlock();
    }

    /**
     * @brief Encodes text as UTF-16 code units.
     *
     * @param text The text.
     * @returns Each code unit as two bytes, big endian, without a length.
     */
    static juce::MemoryBlock getUnicodeUnits (const juce::String& text)
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

private:
    /**
     * @brief Returns the fixed part of the alias record.
     *
     * @param volumeName The name of the volume.
     * @param fileName   The name of the file.
     * @returns The record from the application information to the reserved
     *          bytes. The record length holds a placeholder.
     */
    static juce::MemoryBlock getFixedRecord (const juce::String& volumeName, const juce::String& fileName)
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

    /**
     * @brief Returns the variable part of the alias record.
     *
     * @param volumeName The name of the volume.
     * @param fileName   The name of the file.
     * @returns The tags for the folder name, the dates, the paths, the
     *          Unicode names, and the mount point, then the end tag.
     */
    static juce::MemoryBlock getTags (const juce::String& volumeName, const juce::String& fileName)
    {
        const auto getBytes = [] (const juce::String& text)
        {
            return juce::MemoryBlock (text.toRawUTF8(), text.getNumBytesAsUTF8());
        };
        const juce::MemoryBlock zeroDate (dateSize, true);

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

    /**
     * @brief Encodes a name as a fixed-size Pascal string.
     *
     * @param text The name. The function shortens it to fit.
     * @param size The size of the result in bytes, with the length byte.
     * @returns The length byte, the UTF-8 bytes of the Carbon name, and zero
     *          padding.
     */
    static juce::MemoryBlock getPascalString (const juce::String& text, int size)
    {
        const auto carbonName { getCarbonName (text) };
        const auto length { juce::jmin (carbonName.getNumBytesAsUTF8(), static_cast<size_t> (size - 1)) };
        const auto lengthByte { static_cast<char> (length) };

        juce::MemoryBlock pascal (static_cast<size_t> (size), true);
        pascal.copyFrom (&lengthByte, 0, sizeof (lengthByte));
        pascal.copyFrom (carbonName.toRawUTF8(), 1, length);
        return pascal;
    }

    /**
     * @brief Encodes one tag.
     *
     * @param tag   The tag number.
     * @param bytes The tag content.
     * @returns The tag number, the content size, @p bytes, and one zero
     *          byte when the size is odd.
     */
    static juce::MemoryBlock getTag (int tag, const juce::MemoryBlock& bytes)
    {
        juce::MemoryOutputStream record;
        record.writeShortBigEndian (static_cast<short> (tag));
        record.writeShortBigEndian (static_cast<short> (bytes.getSize()));
        record << bytes;
        record.writeRepeatedByte (0, bytes.getSize() % 2);
        return record.getMemoryBlock();
    }

    /**
     * @brief Encodes a name as a Unicode tag content.
     *
     * @param text The name.
     * @returns The count of code units, then the code units of the Carbon
     *          name.
     */
    static juce::MemoryBlock getUnicodeName (const juce::String& text)
    {
        const auto units { getUnicodeUnits (getCarbonName (text)) };

        juce::MemoryOutputStream unicode;
        unicode.writeShortBigEndian (static_cast<short> (units.getSize() / sizeof (juce::CharPointer_UTF16::CharType)));
        unicode << units;
        return unicode.getMemoryBlock();
    }

    /**
     * @brief Returns the name in the Carbon form.
     *
     * @param text The name.
     * @returns @p text with each colon replaced by a slash.
     */
    static juce::String getCarbonName (const juce::String& text)
    {
        return text.replaceCharacter (Chars::colon, Chars::slash);
    }

    /** Record length before the real length is known. */
    static constexpr short recordLengthPlaceholder { 0 };
    /** Date value of an unknown date. */
    static constexpr int noDate { 0 };
    /** Volume flags value of an unknown volume. */
    static constexpr int noVolumeFlags { 0 };
    /** Content length of the end tag. */
    static constexpr short tagEndLength { 0 };
    /** Offset of the record length in the record. */
    static constexpr int recordLengthOffset { 4 };
    /** Size of the application information, in bytes. */
    static constexpr size_t appInfoSize { 4 };
    /** Alias record version. */
    static constexpr short aliasVersion { 2 };
    /** Alias target kind: a file. */
    static constexpr short fileKind { 0 };
    /** Volume type: a fixed disk. */
    static constexpr short fixedDiskType { 0 };
    /** Folder level value of an unknown level. */
    static constexpr short noLevel { -1 };
    /** Node identifier value of an unknown node. */
    static constexpr juce::uint32 noNodeId { 0xffffffff };
    /** Node identifier of the HFS+ root folder, the parent of the file. */
    static constexpr juce::uint32 rootFolderId { 2 };
    /** Size of the volume name field, with the length byte. */
    static constexpr int volumeNameSize { 28 };
    /** Size of the file name field, with the length byte. */
    static constexpr int fileNameSize { 64 };
    /** Size of the creator code, in bytes. */
    static constexpr size_t creatorCodeSize { 4 };
    /** Size of the type code, in bytes. */
    static constexpr size_t typeCodeSize { 4 };
    /** Size of the file system identifier, in bytes. */
    static constexpr size_t fileSystemIdSize { 2 };
    /** Size of the reserved field, in bytes. */
    static constexpr size_t reservedSize { 10 };
    /** Size of a date tag content, in bytes. */
    static constexpr size_t dateSize { 8 };
    /** Tag number of the folder name. */
    static constexpr int tagFolderName { 0 };
    /** Tag number of the Carbon path. */
    static constexpr int tagCarbonPath { 2 };
    /** Tag number of the Unicode file name. */
    static constexpr int tagUnicodeFileName { 14 };
    /** Tag number of the Unicode volume name. */
    static constexpr int tagUnicodeVolumeName { 15 };
    /** Tag number of the volume date. */
    static constexpr int tagVolumeDate { 16 };
    /** Tag number of the creation date. */
    static constexpr int tagCreationDate { 17 };
    /** Tag number of the POSIX path. */
    static constexpr int tagPosixPath { 18 };
    /** Tag number of the mount point. */
    static constexpr int tagMountPoint { 19 };
    /** Tag number of the end tag. */
    static constexpr short tagEnd { -1 };
    /** File system signature: HFS+. */
    static constexpr const char* fileSystemType { "H+" };
    /** Path that precedes the volume name in the mount point. */
    static constexpr const char* mountPointPrefix { "/Volumes/" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AliasWriter)
};
