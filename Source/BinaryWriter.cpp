#include "BinaryWriter.h"

std::optional<juce::MemoryBlock> BinaryWriter::getStore (const juce::ArgumentList& arguments,
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

bool BinaryWriter::isLayoutValid (const juce::ArgumentList& arguments)
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

bool BinaryWriter::hasLayout (const juce::ArgumentList& arguments)
{
    for (const auto& key : getLayoutKeys())
        if (arguments.containsOption (Id::doubleDash + key.toString())) return true;

    return false;
}

const std::array<juce::Identifier, 7>& BinaryWriter::getLayoutKeys()
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

int BinaryWriter::getLayoutValue (const juce::ArgumentList& arguments, const juce::Identifier& key, int defaultValue)
{
    const auto flag { Id::doubleDash + key.toString() };
    return arguments.containsOption (flag) ? arguments.getValueForOption (flag).getIntValue() : defaultValue;
}

juce::NamedValueSet BinaryWriter::getWindowSettings (const juce::ArgumentList& arguments, int rowCount)
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

juce::NamedValueSet BinaryWriter::getBackgroundSettings (const juce::String& volumeName, const juce::String& backgroundName)
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

juce::NamedValueSet BinaryWriter::getIconSettings (const juce::ArgumentList& arguments,
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

juce::MemoryBlock BinaryWriter::getLocation (int x, int y)
{
    juce::MemoryOutputStream location;
    location.writeIntBigEndian (x);
    location.writeIntBigEndian (y);
    location.writeIntBigEndian (static_cast<int> (locationFlags));
    location.writeIntBigEndian (static_cast<int> (locationMask));
    return location.getMemoryBlock();
}

juce::MemoryBlock BinaryWriter::getRecord (const juce::String& name,
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

juce::MemoryBlock BinaryWriter::getBlob (const juce::MemoryBlock& bytes)
{
    juce::MemoryOutputStream blob;
    blob.writeIntBigEndian (static_cast<int> (bytes.getSize()));
    blob << bytes;
    return blob.getMemoryBlock();
}

juce::StringArray BinaryWriter::getNames (const juce::StringArray& itemNames, const juce::StringArray& linkNames)
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

juce::MemoryBlock BinaryWriter::getFolderRecords (const juce::ArgumentList& arguments,
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

juce::MemoryBlock BinaryWriter::getLocationRecord (const juce::ArgumentList& arguments,
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

juce::MemoryBlock BinaryWriter::getNode (const juce::ArgumentList& arguments,
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

juce::MemoryBlock BinaryWriter::getHeader()
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

juce::MemoryBlock BinaryWriter::getFreeLists()
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

juce::MemoryBlock BinaryWriter::getBookkeeping()
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

juce::MemoryBlock BinaryWriter::getMaster (int recordCount)
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
