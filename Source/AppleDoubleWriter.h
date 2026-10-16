#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "IconWriter.h"

#if JUCE_MAC
/**
 * @struct AppleDoubleWriter
 * @brief Encodes the Finder information and the resource fork of a file as a
 *        version-2 AppleDouble file.
 *
 * The file has the Finder information entry and the resource fork entry. The
 * Finder information entry ends with an attribute header that holds no
 * attribute. AppleDoubleWriter keeps no state.
 */
struct AppleDoubleWriter
{
    /**
     * @brief Builds the AppleDouble file of an entry.
     *
     * @param entry The file, folder, or symbolic link to read. The function
     *              does not follow a symbolic link.
     * @returns The AppleDouble bytes, or @c std::nullopt when @p entry has
     *          no Finder information and no resource fork.
     */
    static std::optional<juce::MemoryBlock> getAppleDouble (const juce::File& entry);

private:
    /**
     * @brief Encodes the header of the AppleDouble file.
     *
     * @param resourceForkSize The size of the resource fork in bytes.
     * @returns The magic number, the version, the filler, the entry count,
     *          and the two entry descriptors.
     */
    static juce::MemoryBlock getHeader (size_t resourceForkSize);

    /**
     * @brief Encodes the attribute header that follows the Finder
     *        information.
     *
     * @returns The header of an attribute list with no attribute.
     */
    static juce::MemoryBlock getAttributeHeader();

    /**
     * @brief Reads one extended attribute of an entry.
     *
     * @param entry         The file, folder, or symbolic link to read.
     * @param attributeName The name of the attribute.
     * @returns The attribute bytes. The block is empty when @p entry has no
     *          such attribute.
     */
    static juce::MemoryBlock getAttribute (const juce::File& entry, const char* attributeName);

    /** Magic number of an AppleDouble file. */
    static constexpr juce::uint32 magic { 0x00051607 };
    /** Version number of the AppleDouble format. */
    static constexpr juce::uint32 version { 0x00020000 };
    /** Home file system text that fills 16 bytes of the header. */
    static constexpr const char* filler { "Mac OS X        " };
    /** Number of entries in the header. */
    static constexpr short entryCount { 2 };
    /** Entry identifier of the Finder information. */
    static constexpr int finderInfoId { 9 };
    /** Entry identifier of the resource fork. */
    static constexpr int resourceForkId { 2 };
    /** Offset of the Finder information entry, in bytes. */
    static constexpr int finderInfoOffset { 50 };
    /** Length of the Finder information entry: the Finder information, the padding, and the attribute header. */
    static constexpr int finderInfoLength { 70 };
    /** Offset of the resource fork entry, in bytes. */
    static constexpr int resourceForkOffset { 120 };
    /** Size of the padding between the Finder information and the attribute header, in bytes. */
    static constexpr size_t paddingSize { 2 };
    /** Magic text of the attribute header. */
    static constexpr const char* attributeMagic { "ATTR" };
    /** Size of the reserved field of the attribute header, in bytes. */
    static constexpr size_t reservedSize { 12 };
    /** Debug tag value of the attribute header. */
    static constexpr int noDebugTag { 0 };
    /** Attribute data size of an attribute header with no attribute. */
    static constexpr int noAttributeData { 0 };
    /** Flags value of an attribute header with no attribute. */
    static constexpr short noAttributeFlags { 0 };
    /** Attribute count of an attribute header with no attribute. */
    static constexpr short noAttributes { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AppleDoubleWriter)
};
#endif
