#pragma once
#include <JuceHeader.h>
#include "generated/Generated.h"
#include "Model.h"
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
     * @param model     The model that holds the layout table.
     * @param layout    The @c ## pack layout table. Its @c value cells give
     *                  the icon size, the two columns, the first row, the
     *                  row spacing, and the window origin.
     * @param itemNames The item file names, one per row, in row order.
     * @param linkNames The link names, one per row, in row order. An empty
     *                  name means the row has no link.
     * @returns The image, or @c std::nullopt when the record node does not
     *          fit in one block.
     */
    static std::optional<juce::MemoryBlock> getStore (const Model& model,
                                                      const Model::Element& layout,
                                                      const juce::StringArray& itemNames,
                                                      const juce::StringArray& linkNames)
    {
        constexpr int folderRecordCount { 3 };
        jassert (itemNames.size() == linkNames.size());

        const auto names { getNames (itemNames, linkNames) };
        const auto recordCount { names.size() - 1 + folderRecordCount };
        const auto node { getNode (model, layout, itemNames, linkNames, names, recordCount) };

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

private:
    /**
     * @brief Returns the integer @c value cell of the @p key row of
     *        @p layout.
     *
     * @param model  The model that holds @p layout.
     * @param layout The @c ## pack layout table.
     * @param key    The layout key.
     * @returns The integer value of the @p key row.
     */
    static int getLayoutValue (const Model& model, const Model::Element& layout, const juce::Identifier& key)
    {
        return model.getValue (*model.getTableRow (layout, key), Id::value).getIntValue();
    }

    /**
     * @brief Returns the window settings of the folder record.
     *
     * @param model    The model that holds @p layout.
     * @param layout   The @c ## pack layout table.
     * @param rowCount The number of item rows, which sets the window
     *                 height.
     * @returns The settings, with the sidebar, status bar, tab view, and
     *          toolbar hidden and the window bounds set.
     */
    static juce::NamedValueSet getWindowSettings (const Model& model, const Model::Element& layout, int rowCount)
    {
        const auto width { getLayoutValue (model, layout, Id::bundleColumn)
                           + getLayoutValue (model, layout, Id::linkColumn) };
        const auto height { getLayoutValue (model, layout, Id::firstRow)
                            + rowCount * getLayoutValue (model, layout, Id::rowSpacing) };
        const auto bounds { juce::String::formatted ("{{%d, %d}, {%d, %d}}",
                                                     getLayoutValue (model, layout, Id::windowLeft),
                                                     getLayoutValue (model, layout, Id::windowTop),
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
     * @brief Returns the icon-view settings of the folder record.
     *
     * @param model  The model that holds @p layout.
     * @param layout The @c ## pack layout table, which gives the icon
     *               size.
     * @returns The settings, with free arrangement and a white background.
     */
    static juce::NamedValueSet getIconSettings (const Model& model, const Model::Element& layout)
    {
        return { { "arrangeBy", "none" },
                 { "backgroundColorBlue", 1.0 },
                 { "backgroundColorGreen", 1.0 },
                 { "backgroundColorRed", 1.0 },
                 { "backgroundType", 0 },
                 { "gridOffsetX", 0.0 },
                 { "gridOffsetY", 0.0 },
                 { "gridSpacing", 100.0 },
                 { "iconSize", static_cast<double> (getLayoutValue (model, layout, Id::iconSize)) },
                 { "labelOnBottom", true },
                 { "showIconPreview", true },
                 { "showItemInfo", false },
                 { "textSize", 12.0 },
                 { "viewOptionsVersion", 1 } };
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
        const auto units { name.toUTF16() };
        const auto unitCount { juce::CharPointer_UTF16::getBytesRequiredFor (name.getCharPointer())
                               / sizeof (juce::CharPointer_UTF16::CharType) };
        const juce::Span<const juce::CharPointer_UTF16::CharType> nameUnits { units.getAddress(), unitCount };

        juce::MemoryOutputStream record;
        record.writeIntBigEndian (static_cast<int> (unitCount));

        for (const auto unit : nameUnits)
            record.writeShortBigEndian (static_cast<short> (unit));

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

        std::sort (names.begin(), names.end(), [] (const juce::String& first, const juce::String& second)
        {
            return first.toLowerCase().compare (second.toLowerCase()) < 0;
        });

        return names;
    }

    /**
     * @brief Returns the three records of the folder itself: window
     *        settings, icon-view settings, and the store revision.
     *
     * @param model    The model that holds @p layout.
     * @param layout   The @c ## pack layout table.
     * @param rowCount The number of item rows.
     * @returns The encoded records.
     */
    static juce::MemoryBlock getFolderRecords (const Model& model, const Model::Element& layout, int rowCount)
    {
        juce::MemoryOutputStream revision;
        revision.writeIntBigEndian (storeVersion);

        const auto windowList { PropertyListWriter::getPropertyList (getWindowSettings (model, layout, rowCount)) };
        const auto iconList { PropertyListWriter::getPropertyList (getIconSettings (model, layout)) };

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
     * @param model     The model that holds @p layout.
     * @param layout    The @c ## pack layout table.
     * @param name      The item or link name.
     * @param itemNames The item file names, in row order.
     * @param linkNames The link names, in row order.
     * @returns The encoded record.
     */
    static juce::MemoryBlock getLocationRecord (const Model& model,
                                                const Model::Element& layout,
                                                const juce::String& name,
                                                const juce::StringArray& itemNames,
                                                const juce::StringArray& linkNames)
    {
        const auto itemRow { itemNames.indexOf (name) };
        const auto isItem { itemRow >= 0 };
        const auto row { isItem ? itemRow : linkNames.indexOf (name) };
        const auto x { getLayoutValue (model, layout, isItem ? Id::bundleColumn : Id::linkColumn) };
        const auto y { getLayoutValue (model, layout, Id::firstRow)
                       + row * getLayoutValue (model, layout, Id::rowSpacing) };

        return getRecord (name, locationCode, blobType, getBlob (getLocation (x, y)));
    }

    /**
     * @brief Returns the B-tree node that holds every record.
     *
     * @param model       The model that holds @p layout.
     * @param layout      The @c ## pack layout table.
     * @param itemNames   The item file names, in row order.
     * @param linkNames   The link names, in row order.
     * @param names       The sorted names from getNames().
     * @param recordCount The number of records in the node.
     * @returns The node header followed by the records in @p names order.
     */
    static juce::MemoryBlock getNode (const Model& model,
                                      const Model::Element& layout,
                                      const juce::StringArray& itemNames,
                                      const juce::StringArray& linkNames,
                                      const juce::StringArray& names,
                                      int recordCount)
    {
        juce::MemoryOutputStream records;

        for (const auto& name : names)
        {
            if (name.compare (folderName) == 0)
                records << getFolderRecords (model, layout, itemNames.size());
            else
                records << getLocationRecord (model, layout, name, itemNames, linkNames);
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
