#include "AppleDoubleWriter.h"
#if JUCE_MAC
#include <sys/xattr.h>

std::optional<juce::MemoryBlock> AppleDoubleWriter::getAppleDouble (const juce::File& entry)
{
    const auto finderInfo { getAttribute (entry, XATTR_FINDERINFO_NAME) };
    const auto resourceFork { getAttribute (entry, XATTR_RESOURCEFORK_NAME) };
    jassert (finderInfo.isEmpty() or finderInfo.getSize() == IconWriter::finderInfoSize);

    if (finderInfo.isEmpty() and resourceFork.isEmpty())
        return std::nullopt;

    juce::MemoryOutputStream appleDouble;
    appleDouble << getHeader (resourceFork.getSize());

    if (finderInfo.isEmpty())
        appleDouble.writeRepeatedByte (0, IconWriter::finderInfoSize);
    else
        appleDouble << finderInfo;

    appleDouble.writeRepeatedByte (0, paddingSize);
    appleDouble << getAttributeHeader();
    jassert (appleDouble.getDataSize() == static_cast<size_t> (resourceForkOffset));
    appleDouble << resourceFork;
    return appleDouble.getMemoryBlock();
}

juce::MemoryBlock AppleDoubleWriter::getHeader (size_t resourceForkSize)
{
    juce::MemoryOutputStream header;
    header.writeIntBigEndian (static_cast<int> (magic));
    header.writeIntBigEndian (static_cast<int> (version));
    header << filler;
    header.writeShortBigEndian (entryCount);
    header.writeIntBigEndian (finderInfoId);
    header.writeIntBigEndian (finderInfoOffset);
    header.writeIntBigEndian (finderInfoLength);
    header.writeIntBigEndian (resourceForkId);
    header.writeIntBigEndian (resourceForkOffset);
    header.writeIntBigEndian (static_cast<int> (resourceForkSize));
    return header.getMemoryBlock();
}

juce::MemoryBlock AppleDoubleWriter::getAttributeHeader()
{
    juce::MemoryOutputStream attributeHeader;
    attributeHeader << attributeMagic;
    attributeHeader.writeIntBigEndian (noDebugTag);
    attributeHeader.writeIntBigEndian (resourceForkOffset);
    attributeHeader.writeIntBigEndian (resourceForkOffset);
    attributeHeader.writeIntBigEndian (noAttributeData);
    attributeHeader.writeRepeatedByte (0, reservedSize);
    attributeHeader.writeShortBigEndian (noAttributeFlags);
    attributeHeader.writeShortBigEndian (noAttributes);
    return attributeHeader.getMemoryBlock();
}

juce::MemoryBlock AppleDoubleWriter::getAttribute (const juce::File& entry, const char* attributeName)
{
    const auto path { entry.getFullPathName() };
    const auto size { getxattr (path.toRawUTF8(), attributeName, nullptr, 0, 0, XATTR_NOFOLLOW) };
    juce::MemoryBlock attribute;

    if (size > 0)
    {
        attribute.setSize (static_cast<size_t> (size));
        [[maybe_unused]] const auto result { getxattr (path.toRawUTF8(), attributeName, attribute.getData(), attribute.getSize(), 0, XATTR_NOFOLLOW) };
        jassert (result == size);
    }

    return attribute;
}
#endif
