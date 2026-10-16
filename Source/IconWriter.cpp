#include "IconWriter.h"
#if JUCE_MAC
#include <sys/xattr.h>

juce::Result IconWriter::toBundle (const juce::File& bundle)
{
    const auto propertyList { bundle.getChildFile (contentsFolder).getChildFile (propertyListName) };
    const auto iconName { getIconName (propertyList) };

    if (iconName.isNotEmpty())
    {
        const auto icon { bundle.getChildFile (contentsFolder).getChildFile (resourcesFolder).getChildFile (iconName) };

        return toIconFile (bundle, icon);
    }

    return juce::Result::ok();
}

juce::String IconWriter::getIconName (const juce::File& propertyList)
{
    if (propertyList.existsAsFile())
    {
        const auto document { juce::XmlDocument::parse (propertyList) };
        jassert (document != nullptr);

        if (document != nullptr)
        {
            if (const auto* dictionary { document->getChildByName (dictionaryTag) })
            {
                for (const auto* element { dictionary->getFirstChildElement() }; element != nullptr;
                     element = element->getNextElement())
                {
                    if (element->hasTagName (keyTag) and element->getAllSubText().compare (iconKey) == 0)
                    {
                        jassert (element->getNextElement() != nullptr);

                        if (const auto* value { element->getNextElement() })
                            return value->getAllSubText();
                    }
                }
            }
        }
    }

    return juce::String();
}

juce::Result IconWriter::toIconFile (const juce::File& bundle, const juce::File& icon)
{
    juce::MemoryBlock iconData;

    if (icon.loadFileAsData (iconData))
    {
        const auto iconFile { bundle.getChildFile (iconFileName) };

        if (iconFile.deleteFile()
            and iconFile.create().wasOk()
            and applyAttribute (iconFile, XATTR_RESOURCEFORK_NAME, getResourceFork (iconData))
            and applyAttribute (iconFile, XATTR_FINDERINFO_NAME, getFinderInfo (invisibleFlag)))
        {
            if (applyAttribute (bundle, XATTR_FINDERINFO_NAME, getFinderInfo (customIconFlag)))
                return juce::Result::ok();

            return juce::Result::fail (bundle.getFullPathName() + Id::diagnosticSeparator
                                       + text::Diagnostics::failOutputWrite);
        }

        return juce::Result::fail (iconFile.getFullPathName() + Id::diagnosticSeparator
                                   + text::Diagnostics::failOutputWrite);
    }

    return juce::Result::fail (icon.getFullPathName() + Id::diagnosticSeparator
                               + text::Diagnostics::failNotFound);
}

juce::MemoryBlock IconWriter::getResourceFork (const juce::MemoryBlock& icon)
{
    const auto dataLength { static_cast<int> (resourceLengthSize + icon.getSize()) };

    juce::MemoryOutputStream header;
    header.writeIntBigEndian (dataOffset);
    header.writeIntBigEndian (dataOffset + dataLength);
    header.writeIntBigEndian (dataLength);
    header.writeIntBigEndian (mapLength);

    juce::MemoryOutputStream fork;
    fork << header.getMemoryBlock();
    fork.writeRepeatedByte (0, static_cast<size_t> (dataOffset) - fork.getDataSize());
    fork.writeIntBigEndian (static_cast<int> (icon.getSize()));
    fork << icon;
    fork << getResourceMap (header.getMemoryBlock());
    return fork.getMemoryBlock();
}

juce::MemoryBlock IconWriter::getResourceMap (const juce::MemoryBlock& header)
{
    juce::MemoryOutputStream map;
    map << header;
    map.writeRepeatedByte (0, mapReservedSize);
    map.writeShortBigEndian (typeListOffset);
    map.writeShortBigEndian (nameListOffset);
    map.writeShortBigEndian (lastEntryIndex);
    map << iconType;
    map.writeShortBigEndian (lastEntryIndex);
    map.writeShortBigEndian (referenceListOffset);
    map.writeShortBigEndian (customIconId);
    map.writeShortBigEndian (noName);
    map.writeRepeatedByte (0, referenceTailSize);
    jassert (map.getDataSize() == static_cast<size_t> (mapLength));
    return map.getMemoryBlock();
}

juce::MemoryBlock IconWriter::getFinderInfo (juce::uint16 finderFlags)
{
    juce::MemoryOutputStream finderInfo;
    finderInfo.writeRepeatedByte (0, finderFlagsOffset);
    finderInfo.writeShortBigEndian (static_cast<short> (finderFlags));
    finderInfo.writeRepeatedByte (0, finderInfoSize - finderInfo.getDataSize());
    return finderInfo.getMemoryBlock();
}

bool IconWriter::applyAttribute (const juce::File& file, const char* attributeName, const juce::MemoryBlock& value)
{
    return setxattr (file.getFullPathName().toRawUTF8(), attributeName, value.getData(), value.getSize(), 0,
                     XATTR_NOFOLLOW) == 0;
}
#endif
