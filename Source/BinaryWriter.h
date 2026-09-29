#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "AliasWriter.h"
#include "PropertyListWriter.h"

/**
 * @struct BinaryWriter
 * @brief Encodes the Finder @c .DS_Store image of a disk-image stage
 *        folder: window bounds, icon-view options, and one icon position
 *        per item and link.
 *
 * BinaryWriter keeps no state. The image holds one allocator header, one
 * bookkeeping block, one master block, and one B-tree node, at fixed
 * offsets.
 */
struct BinaryWriter
{
    /**
     * @brief Builds the @c .DS_Store image.
     *
     * @param arguments      The command line, for the layout options.
     * @param itemNames      The item file names, one per row, in row order.
     * @param linkNames      The link names, one per row, in row order. An
     *                       empty name means the row has no link.
     * @param volumeName     The volume name, for the background alias.
     * @param backgroundName The file name of the background image in the
     *                       stage. An empty name means a white background.
     * @returns The image, or @c std::nullopt when the record node does not
     *          fit in one block.
     */
    static std::optional<juce::MemoryBlock> getStore (const juce::ArgumentList& arguments,
                                                      const juce::StringArray& itemNames,
                                                      const juce::StringArray& linkNames,
                                                      const juce::String& volumeName,
                                                      const juce::String& backgroundName)
    {
        constexpr int folderRecordCount { 3 };
        jassert (itemNames.size() == linkNames.size());

        const auto names { getNames (itemNames, linkNames) };
        const auto recordCount { names.size() - 1 + folderRecordCount };
        const auto node { getNode (arguments, itemNames, linkNames, volumeName, backgroundName, names, recordCount) };

        if (node.getSize() <= static_cast<size_t> (blockSize))
        {
            juce::MemoryBlock store (storeSize, true);
            const auto place = [&store] (const juce::MemoryBlock& block, int offset)
            {
                store.copyFrom (block.getData(), offset, block.getSize());
            };

            place (getHeader(), 0);
            place (getBookkeeping(), blockPrefix + bookkeepingOffset);
            place (getMaster (recordCount), blockPrefix + masterOffset);
            place (node, blockPrefix + nodeOffset);
            return store;
        }

        return std::nullopt;
    }

    /**
     * @brief Answers whether each layout option on the command line is an
     *        integer.
     *
     * @param arguments The command line.
     * @returns True when each layout option that is present has an integer
     *          value.
     */
    static bool isLayoutValid (const juce::ArgumentList& arguments)
    {
        for (const auto& key : getLayoutKeys())
        {
            const auto flag { Id::doubleDash + key.toString() };

            if (arguments.containsOption (flag))
            {
                const auto value { arguments.getValueForOption (flag) };

                if (juce::String (value.getIntValue()).compare (value) != 0)
                    return false;
            }
        }

        return true;
    }

    /**
     * @brief Answers whether the command line has a layout option.
     *
     * @param arguments The command line.
     * @returns True when at least one layout option is present.
     */
    static bool hasLayout (const juce::ArgumentList& arguments)
    {
        for (const auto& key : getLayoutKeys())
            if (arguments.containsOption (Id::doubleDash + key.toString())) return true;

        return false;
    }

private:
    /**
     * @brief Returns the seven layout option keys.
     *
     * @returns The keys, without the leading dashes.
     */
    static const std::array<juce::Identifier, 7>& getLayoutKeys()
    {
        static const std::array<juce::Identifier, 7> layoutKeys { Id::iconSize,
                                                                   Id::bundleColumn,
                                                                   Id::linkColumn,
                                                                   Id::firstRow,
                                                                   Id::rowSpacing,
                                                                   Id::windowLeft,
                                                                   Id::windowTop };
        return layoutKeys;
    }

    /**
     * @brief Returns the value of one layout option.
     *
     * @param arguments    The command line.
     * @param key          The layout option key.
     * @param defaultValue The value when the option is absent.
     * @returns The integer value of the option, or @p defaultValue.
     */
    static int getLayoutValue (const juce::ArgumentList& arguments, const juce::Identifier& key, int defaultValue)
    {
        const auto flag { Id::doubleDash + key.toString() };
        return arguments.containsOption (flag) ? arguments.getValueForOption (flag).getIntValue() : defaultValue;
    }

    /**
     * @brief Returns the window settings of the folder record.
     *
     * @param arguments The command line, for the layout options.
     * @param rowCount  The number of item rows, which sets the window
     *                  height.
     * @returns The settings, with the sidebar, status bar, tab view, and
     *          toolbar hidden and the window bounds set.
     */
    static juce::NamedValueSet getWindowSettings (const juce::ArgumentList& arguments, int rowCount)
    {
        const auto width { getLayoutValue (arguments, Id::bundleColumn, defaultBundleColumn)
                           + getLayoutValue (arguments, Id::linkColumn, defaultLinkColumn) };
        const auto height { getLayoutValue (arguments, Id::firstRow, defaultFirstRow)
                            + rowCount * getLayoutValue (arguments, Id::rowSpacing, defaultRowSpacing) };
        const auto bounds { juce::String::formatted ("{{%d, %d}, {%d, %d}}",
                                                     getLayoutValue (arguments, Id::windowLeft, defaultWindowLeft),
                                                     getLayoutValue (arguments, Id::windowTop, defaultWindowTop),
                                                     width,
                                                     height) };

        return { { "ContainerShowSidebar", false },
                 { "ShowSidebar", false },
                 { "ShowStatusBar", false },
                 { "ShowTabView", false },
                 { "ShowToolbar", false },
                 { "WindowBounds", bounds } };
    }

    /**
     * @brief Returns the background settings of the icon view.
     *
     * @param volumeName     The volume name, for the alias.
     * @param backgroundName The file name of the background image in the
     *                       stage. An empty name selects the white color.
     * @returns The white color components and the color type. With a
     *          background name, also the alias, and the image type in place
     *          of the color type.
     */
    static juce::NamedValueSet getBackgroundSettings (const juce::String& volumeName, const juce::String& backgroundName)
    {
        juce::NamedValueSet settings { { "backgroundColorBlue", whiteColorComponent },
                                       { "backgroundColorGreen", whiteColorComponent },
                                       { "backgroundColorRed", whiteColorComponent },
                                       { backgroundTypeKey, colorBackgroundType } };

        if (backgroundName.isNotEmpty())
        {
            settings.set ("backgroundImageAlias", juce::var (AliasWriter::getAlias (volumeName, backgroundName)));
            settings.set (backgroundTypeKey, backgroundImageType);
        }

        return settings;
    }

    /**
     * @brief Returns the icon-view settings of the folder record.
     *
     * @param arguments      The command line, for the icon size.
     * @param volumeName     The volume name, for the background alias.
     * @param backgroundName The file name of the background image in the
     *                       stage. An empty name selects the white color.
     * @returns The settings, with free arrangement and the icon size. The
     *          background is white, or the image when @p backgroundName is
     *          not empty.
     */
    static juce::NamedValueSet getIconSettings (const juce::ArgumentList& arguments,
                                                const juce::String& volumeName,
                                                const juce::String& backgroundName)
    {
        const juce::NamedValueSet viewSettings { { "gridOffsetX", 0.0 },
                                                 { "gridOffsetY", 0.0 },
                                                 { "gridSpacing", gridSpacing },
                                                 { "iconSize", static_cast<double> (getLayoutValue (arguments, Id::iconSize, defaultIconSize)) },
                                                 { "labelOnBottom", true },
                                                 { "showIconPreview", true },
                                                 { "showItemInfo", false },
                                                 { "textSize", textSize },
                                                 { "viewOptionsVersion", 1 } };
        juce::NamedValueSet settings { { "arrangeBy", "none" } };

        for (const auto& setting : getBackgroundSettings (volumeName, backgroundName))
            settings.set (setting.name, setting.value);

        for (const auto& setting : viewSettings)
            settings.set (setting.name, setting.value);

        return settings;
    }

    /**
     * @brief Returns the icon position blob.
     *
     * @param x The horizontal position.
     * @param y The vertical position.
     * @returns @p x, @p y, and the two fixed flag words, big endian.
     */
    static juce::MemoryBlock getLocation (int x, int y)
    {
        juce::MemoryOutputStream location;
        location.writeIntBigEndian (x);
        location.writeIntBigEndian (y);
        location.writeIntBigEndian (static_cast<int> (locationFlags));
        location.writeIntBigEndian (static_cast<int> (locationMask));
        return location.getMemoryBlock();
    }

    /**
     * @brief Returns one record of the node.
     *
     * @param name  The file name the record belongs to, written as UTF-16.
     * @param code  The four-character property code.
     * @param type  The four-character value type.
     * @param value The encoded value.
     * @returns The name length, the name, @p code, @p type, and
     *          @p value.
     */
    static juce::MemoryBlock getRecord (const juce::String& name,
                                        const char* code,
                                        const char* type,
                                        const juce::MemoryBlock& value)
    {
        const auto units { AliasWriter::getUnicodeUnits (name) };

        juce::MemoryOutputStream record;
        record.writeIntBigEndian (static_cast<int> (units.getSize() / sizeof (juce::CharPointer_UTF16::CharType)));
        record << units;
        record << code << type;

        record << value;
        return record.getMemoryBlock();
    }

    /**
     * @brief Wraps @p bytes as a blob value.
     *
     * @param bytes The blob content.
     * @returns The big-endian size of @p bytes, followed by @p bytes.
     */
    static juce::MemoryBlock getBlob (const juce::MemoryBlock& bytes)
    {
        juce::MemoryOutputStream blob;
        blob.writeIntBigEndian (static_cast<int> (bytes.getSize()));
        blob << bytes;
        return blob.getMemoryBlock();
    }

    /**
     * @brief Returns the names that get a record.
     *
     * @param itemNames The item file names.
     * @param linkNames The link names. Empty names are skipped.
     * @returns The folder name, the item names, and the link names,
     *          sorted case-insensitively.
     */
    static juce::StringArray getNames (const juce::StringArray& itemNames, const juce::StringArray& linkNames)
    {
        juce::StringArray names { folderName };
        names.addArray (itemNames);

        for (const auto& link : linkNames)
            if (link.isNotEmpty()) names.add (link);

        std::stable_sort (names.begin(), names.end(), [] (const juce::String& first, const juce::String& second)
        {
            return first.toLowerCase().compare (second.toLowerCase()) < 0;
        });

        return names;
    }

    /**
     * @brief Returns the three records of the folder itself: window
     *        settings, icon-view settings, and the store revision.
     *
     * @param arguments      The command line, for the layout options.
     * @param rowCount       The number of item rows.
     * @param volumeName     The volume name, for the background alias.
     * @param backgroundName The file name of the background image in the
     *                       stage. An empty name selects the white color.
     * @returns The encoded records.
     */
    static juce::MemoryBlock getFolderRecords (const juce::ArgumentList& arguments,
                                               int rowCount,
                                               const juce::String& volumeName,
                                               const juce::String& backgroundName)
    {
        juce::MemoryOutputStream revision;
        revision.writeIntBigEndian (storeVersion);

        const auto windowList { PropertyListWriter::getPropertyList (getWindowSettings (arguments, rowCount)) };
        const auto iconList { PropertyListWriter::getPropertyList (getIconSettings (arguments, volumeName, backgroundName)) };

        juce::MemoryOutputStream records;
        records << getRecord (folderName, windowCode, blobType, getBlob (windowList));
        records << getRecord (folderName, iconViewCode, blobType, getBlob (iconList));
        records << getRecord (folderName, revisionCode, longType, revision.getMemoryBlock());
        return records.getMemoryBlock();
    }

    /**
     * @brief Returns the icon position record of @p name.
     *
     * An item sits in the bundle column at its own item row. A link sits
     * in the link column at its own link row.
     *
     * @param arguments The command line, for the layout options.
     * @param name      The item or link name.
     * @param itemNames The item file names, in row order.
     * @param linkNames The link names, in row order.
     * @returns The encoded record.
     */
    static juce::MemoryBlock getLocationRecord (const juce::ArgumentList& arguments,
                                                const juce::String& name,
                                                const juce::StringArray& itemNames,
                                                const juce::StringArray& linkNames)
    {
        const auto itemRow { itemNames.indexOf (name) };
        const auto isItem { itemRow >= 0 };
        const auto row { isItem ? itemRow : linkNames.indexOf (name) };
        const auto x { isItem ? getLayoutValue (arguments, Id::bundleColumn, defaultBundleColumn)
                              : getLayoutValue (arguments, Id::linkColumn, defaultLinkColumn) };
        const auto y { getLayoutValue (arguments, Id::firstRow, defaultFirstRow)
                       + row * getLayoutValue (arguments, Id::rowSpacing, defaultRowSpacing) };

        return getRecord (name, locationCode, blobType, getBlob (getLocation (x, y)));
    }

    /**
     * @brief Returns the B-tree node that holds every record.
     *
     * @param arguments      The command line, for the layout options.
     * @param itemNames      The item file names, in row order.
     * @param linkNames      The link names, in row order.
     * @param volumeName     The volume name, for the background alias.
     * @param backgroundName The file name of the background image in the
     *                       stage. An empty name selects the white color.
     * @param names          The sorted names from getNames().
     * @param recordCount    The number of records in the node.
     * @returns The node header followed by the records in @p names order.
     */
    static juce::MemoryBlock getNode (const juce::ArgumentList& arguments,
                                      const juce::StringArray& itemNames,
                                      const juce::StringArray& linkNames,
                                      const juce::String& volumeName,
                                      const juce::String& backgroundName,
                                      const juce::StringArray& names,
                                      int recordCount)
    {
        juce::MemoryOutputStream records;

        for (const auto& name : names)
        {
            if (name.compare (folderName) == 0)
                records << getFolderRecords (arguments, itemNames.size(), volumeName, backgroundName);
            else
                records << getLocationRecord (arguments, name, itemNames, linkNames);
        }

        juce::MemoryOutputStream node;
        node.writeIntBigEndian (0);
        node.writeIntBigEndian (recordCount);
        node << records.getMemoryBlock();
        return node.getMemoryBlock();
    }

    /**
     * @brief Returns the allocator header, which carries the
     *        @c Bud1 magic and the addresses of the bookkeeping block.
     *
     * @returns The encoded header.
     */
    static juce::MemoryBlock getHeader()
    {
        constexpr size_t reservedBytes { 12 };

        juce::MemoryOutputStream header;
        header.writeIntBigEndian (storeVersion);
        header << allocatorMagic;
        header.writeIntBigEndian (bookkeepingOffset);
        header.writeIntBigEndian (blockSize);
        header.writeIntBigEndian (bookkeepingOffset);
        header.writeIntBigEndian (nodeAddress);
        header.writeRepeatedByte (0, reservedBytes);
        return header.getMemoryBlock();
    }

    /**
     * @brief Returns the free lists of the allocator, one per power-of-two
     *        width.
     *
     * @returns The encoded lists.
     */
    static juce::MemoryBlock getFreeLists()
    {
        constexpr int fragmentWidth { 5 };
        constexpr int firstWholeWidth { 7 };
        constexpr int nodeWidth { 12 };
        constexpr int lastWholeWidth { 30 };

        juce::MemoryOutputStream freeLists;

        for (int width { 0 }; width < freeListCount; ++width)
        {
            juce::Array<int> offsets;
            if (width == fragmentWidth) offsets = { 1 << width, 3 << width };
            if (width >= firstWholeWidth and width <= lastWholeWidth and width != nodeWidth) offsets = { 1 << width };

            freeLists.writeIntBigEndian (offsets.size());

            for (const auto offset : offsets)
                freeLists.writeIntBigEndian (offset);
        }

        return freeLists.getMemoryBlock();
    }

    /**
     * @brief Returns the bookkeeping block: the block address table, the
     *        table of contents entry for the directory, and the free
     *        lists.
     *
     * @returns The encoded block.
     */
    static juce::MemoryBlock getBookkeeping()
    {
        constexpr int blockCount { 3 };
        constexpr int directoryCount { 1 };
        constexpr int directoryBlock { 1 };

        juce::MemoryOutputStream bookkeeping;
        bookkeeping.writeIntBigEndian (blockCount);
        bookkeeping.writeIntBigEndian (0);
        bookkeeping.writeIntBigEndian (bookkeepingAddress);
        bookkeeping.writeIntBigEndian (masterAddress);
        bookkeeping.writeIntBigEndian (nodeAddress);
        bookkeeping.writeRepeatedByte (0, sizeof (int) * static_cast<size_t> (addressTableSize - blockCount));
        bookkeeping.writeIntBigEndian (directoryCount);
        bookkeeping.writeByte (static_cast<char> (juce::String (directoryName).length()));
        bookkeeping << directoryName;
        bookkeeping.writeIntBigEndian (directoryBlock);
        bookkeeping << getFreeLists();
        jassert (bookkeeping.getDataSize() <= static_cast<size_t> (blockSize));
        return bookkeeping.getMemoryBlock();
    }

    /**
     * @brief Returns the master block of the B-tree.
     *
     * @param recordCount The number of records in the tree.
     * @returns The root block number, the tree depth, @p recordCount, the
     *          node count, and the page size.
     */
    static juce::MemoryBlock getMaster (int recordCount)
    {
        constexpr int rootBlock { 2 };
        constexpr int levels { 0 };
        constexpr int nodeCount { 1 };

        juce::MemoryOutputStream master;
        master.writeIntBigEndian (rootBlock);
        master.writeIntBigEndian (levels);
        master.writeIntBigEndian (recordCount);
        master.writeIntBigEndian (nodeCount);
        master.writeIntBigEndian (pageSize);
        return master.getMemoryBlock();
    }

    /** Default icon size, in points. */
    static constexpr int defaultIconSize { 64 };
    /** Default x position of the item column. */
    static constexpr int defaultBundleColumn { 150 };
    /** Default x position of the link column. */
    static constexpr int defaultLinkColumn { 450 };
    /** Default y position of row 0. */
    static constexpr int defaultFirstRow { 80 };
    /** Default distance between two rows. */
    static constexpr int defaultRowSpacing { 120 };
    /** Default x position of the window. */
    static constexpr int defaultWindowLeft { 100 };
    /** Default y position of the window. */
    static constexpr int defaultWindowTop { 100 };
    /** Background type value of an image background. */
    static constexpr int backgroundImageType { 2 };
    /** Grid spacing of the icon view. */
    static constexpr double gridSpacing { 100.0 };
    /** Label text size of the icon view. */
    static constexpr double textSize { 12.0 };
    /** Value of each color component of a white background. */
    static constexpr double whiteColorComponent { 1.0 };
    /** Background type value of a color background. */
    static constexpr int colorBackgroundType { 0 };
    /** Key of the background type in the icon-view settings. */
    static constexpr const char* backgroundTypeKey { "backgroundType" };

    /** Total size of the image in bytes. */
    static constexpr int storeSize { 8196 };
    /** Bytes between the image start and the first block offset. */
    static constexpr int blockPrefix { 4 };
    /** Offset of the bookkeeping block. */
    static constexpr int bookkeepingOffset { 0x1800 };
    /** Address of the bookkeeping block: its offset and size class. */
    static constexpr int bookkeepingAddress { 0x180b };
    /** Offset of the master block. */
    static constexpr int masterOffset { 0x40 };
    /** Address of the master block: its offset and size class. */
    static constexpr int masterAddress { 0x45 };
    /** Offset of the record node. */
    static constexpr int nodeOffset { 0x1000 };
    /** Address of the record node: its offset and size class. */
    static constexpr int nodeAddress { 0x100b };
    /** Size of a block. The record node must fit in it. */
    static constexpr int blockSize { 2048 };
    /** Page size written to the master block. */
    static constexpr int pageSize { 4096 };
    /** Slot count of the block address table. */
    static constexpr int addressTableSize { 256 };
    /** Number of free lists, one per width. */
    static constexpr int freeListCount { 32 };
    /** Version written to the header and to the revision record. */
    static constexpr int storeVersion { 1 };
    /** Fixed flag word of an icon position blob. */
    static constexpr juce::uint32 locationFlags { 0xffffffff };
    /** Fixed mask word of an icon position blob. */
    static constexpr juce::uint32 locationMask { 0xffff0000 };
    /** Magic that follows the version in the header. */
    static constexpr const char* allocatorMagic { "Bud1" };
    /** Name of the table of contents entry. */
    static constexpr const char* directoryName { "DSDB" };
    /** Name of the record set that describes the folder itself. */
    static constexpr const char* folderName { "." };
    /** Property code of the window settings record. */
    static constexpr const char* windowCode { "bwsp" };
    /** Property code of the icon-view settings record. */
    static constexpr const char* iconViewCode { "icvp" };
    /** Property code of the store revision record. */
    static constexpr const char* revisionCode { "vSrn" };
    /** Property code of an icon position record. */
    static constexpr const char* locationCode { "Iloc" };
    /** Value type of a blob. */
    static constexpr const char* blobType { "blob" };
    /** Value type of a 32-bit integer. */
    static constexpr const char* longType { "long" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BinaryWriter)
};
