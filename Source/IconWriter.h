#pragma once
#include <JuceHeader.h>
#if JUCE_MAC
#include <sys/xattr.h>
#endif
#include "generated/Generated.h"

#if JUCE_MAC
/**
 * @struct IconWriter
 * @brief Writes a custom Finder icon onto a bundle folder with extended
 *        attributes.
 *
 * The icon is the file that the @c CFBundleIconFile key of the bundle
 * @c Info.plist names. The writer puts the icon bytes into the resource fork
 * of the file @c Icon\\r in the bundle root. It sets the custom-icon flag on
 * the bundle folder. IconWriter keeps no state.
 */
struct IconWriter
{
    /**
     * @brief Writes the custom icon of a bundle.
     *
     * The function does nothing when the @c Info.plist of the bundle has no
     * @c CFBundleIconFile key.
     *
     * @param bundle The bundle folder.
     * @returns Ok, or the failure that names the path of the icon, the icon
     *          file, or the bundle that did not read or write.
     */
    static juce::Result toBundle (const juce::File& bundle)
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

    /** Size of the Finder information attribute, in bytes. */
    static constexpr size_t finderInfoSize { 32 };

private:
    /**
     * @brief Reads the icon file name from an @c Info.plist.
     *
     * @param propertyList The @c Info.plist file.
     * @returns The value of the @c CFBundleIconFile key. It is empty when the
     *          file does not exist or the key is absent.
     */
    static juce::String getIconName (const juce::File& propertyList)
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

    /**
     * @brief Writes the @c Icon\\r file and sets the Finder information of
     *        the bundle.
     *
     * The @c Icon\\r file gets the resource fork of @p icon and the invisible
     * flag. The bundle gets the custom-icon flag. An existing @c Icon\\r file
     * is replaced.
     *
     * @param bundle The bundle folder.
     * @param icon   The icon file, in the format of the icon resource.
     * @returns Ok, or the failure that names the path that did not read or
     *          write.
     */
    static juce::Result toIconFile (const juce::File& bundle, const juce::File& icon)
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

    /**
     * @brief Encodes a resource fork with one icon resource.
     *
     * @param icon The icon bytes.
     * @returns The fork header, the padding to the data offset, the resource
     *          length, @p icon, and the resource map.
     */
    static juce::MemoryBlock getResourceFork (const juce::MemoryBlock& icon)
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

    /**
     * @brief Encodes the resource map of the fork.
     *
     * @param header The fork header. The map starts with a copy of it.
     * @returns The map with one type and one reference, and no name list.
     */
    static juce::MemoryBlock getResourceMap (const juce::MemoryBlock& header)
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

    /**
     * @brief Encodes the Finder information attribute.
     *
     * @param finderFlags The Finder flags.
     * @returns The block of finderInfoSize bytes. It holds @p finderFlags
     *          at its flags offset and zero in each other byte.
     */
    static juce::MemoryBlock getFinderInfo (juce::uint16 finderFlags)
    {
        juce::MemoryOutputStream finderInfo;
        finderInfo.writeRepeatedByte (0, finderFlagsOffset);
        finderInfo.writeShortBigEndian (static_cast<short> (finderFlags));
        finderInfo.writeRepeatedByte (0, finderInfoSize - finderInfo.getDataSize());
        return finderInfo.getMemoryBlock();
    }

    /**
     * @brief Sets one extended attribute on a file.
     *
     * The function does not follow a symbolic link.
     *
     * @param file          The file or folder.
     * @param attributeName The name of the attribute.
     * @param value         The attribute bytes.
     * @returns True when the attribute is set.
     */
    static bool applyAttribute (const juce::File& file, const char* attributeName, const juce::MemoryBlock& value)
    {
        return setxattr (file.getFullPathName().toRawUTF8(), attributeName, value.getData(), value.getSize(), 0,
                         XATTR_NOFOLLOW) == 0;
    }

    /** Name of the icon file in the bundle root. */
    static constexpr const char* iconFileName { "Icon\r" };
    /** Name of the folder that holds the property list and the resources. */
    static constexpr const char* contentsFolder { "Contents" };
    /** Name of the folder that holds the icon file of the bundle. */
    static constexpr const char* resourcesFolder { "Resources" };
    /** Name of the property list file in the contents folder. */
    static constexpr const char* propertyListName { "Info.plist" };
    /** Property list key that names the icon file. */
    static constexpr const char* iconKey { "CFBundleIconFile" };
    /** XML tag of a property list dictionary. */
    static constexpr const char* dictionaryTag { "dict" };
    /** XML tag of a property list key. */
    static constexpr const char* keyTag { "key" };
    /** Type code of the icon resource. */
    static constexpr const char* iconType { "icns" };
    /** Offset of the resource data in the fork, in bytes. */
    static constexpr int dataOffset { 256 };
    /** Length of the resource map, in bytes. */
    static constexpr int mapLength { 50 };
    /** Size of the length field that precedes the resource data, in bytes. */
    static constexpr size_t resourceLengthSize { 4 };
    /** Offset of the type list from the start of the map, in bytes. */
    static constexpr short typeListOffset { 28 };
    /** Offset of the name list from the start of the map, in bytes. It equals the map length, so the list is empty. */
    static constexpr short nameListOffset { 50 };
    /** Offset of the reference list from the start of the type list, in bytes. */
    static constexpr short referenceListOffset { 10 };
    /** Resource identifier of the custom icon. */
    static constexpr short customIconId { -16455 };
    /** Name offset value of a resource with no name. */
    static constexpr short noName { -1 };
    /** Count minus one of the types and of the references. Each count is one. */
    static constexpr short lastEntryIndex { 0 };
    /** Offset of the Finder flags in the Finder information, in bytes. */
    static constexpr size_t finderFlagsOffset { 8 };
    /** Finder flag that marks a folder with a custom icon. */
    static constexpr juce::uint16 customIconFlag { 0x0400 };
    /** Finder flag that hides a file. */
    static constexpr juce::uint16 invisibleFlag { 0x4000 };
    /** Size of the map fields that the fork does not use, in bytes. */
    static constexpr size_t mapReservedSize { 8 };
    /** Size of the reference fields after the name offset, in bytes. */
    static constexpr size_t referenceTailSize { 8 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IconWriter)
};
#endif
