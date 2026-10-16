#include "ZipWriter.h"
#if not JUCE_WINDOWS
#include <sys/stat.h>
#endif

juce::Result ZipWriter::toFile (const juce::File& archive, const juce::Array<juce::File>& items, const juce::StringPairArray& links)
{
    const auto content { getArchive (items, links) };
    juce::MemoryBlock currentData;
    archive.loadFileAsData (currentData);

    const auto isWritten { content.has_value()
                           and (currentData == *content
                                or (archive.getParentDirectory().createDirectory().wasOk()
                                    and archive.replaceWithData (content->getData(), content->getSize()))) };

    return isWritten
               ? juce::Result::ok()
               : juce::Result::fail (archive.getFullPathName() + Id::diagnosticSeparator + text::Diagnostics::failOutputWrite);
}

juce::uint32 ZipWriter::getChecksum (const juce::MemoryBlock& content)
{
    static constexpr auto checksumTable { getChecksumTable() };
    juce::uint32 checksum { 0xffffffff };

    for (const auto byte : content)
        checksum = checksumTable.at ((checksum ^ static_cast<juce::uint8> (byte)) & 0xffu) ^ (checksum >> 8);

    return checksum ^ 0xffffffff;
}

juce::uint32 ZipWriter::getMode (const juce::File& entry)
{
   #if JUCE_WINDOWS
    if (entry.isSymbolicLink()) return linkMode;
    if (entry.isDirectory()) return folderMode;
    return fileMode;
   #else
    struct stat status {};
    [[maybe_unused]] const auto result { lstat (entry.getFullPathName().toRawUTF8(), &status) };
    jassert (result == 0);
    return static_cast<juce::uint32> (status.st_mode);
   #endif
}

juce::uint32 ZipWriter::getDosAttributes (const juce::File& entry)
{
   #if JUCE_WINDOWS
    const auto fileAttributes { GetFileAttributesW (entry.getFullPathName().toWideCharPointer()) };
    jassert (fileAttributes != INVALID_FILE_ATTRIBUTES);
    return fileAttributes & dosAttributeMask;
   #else
    juce::ignoreUnused (entry);
    return 0;
   #endif
}

juce::uint32 ZipWriter::getAttributes (const juce::File& entry)
{
    return getMode (entry) << modeShift | getDosAttributes (entry);
}

juce::MemoryBlock ZipWriter::getContent (const juce::File& entry)
{
    juce::MemoryBlock content;

    if (entry.isSymbolicLink())
    {
        const auto target { entry.getNativeLinkedTarget().replaceCharacter (Chars::backslash, Chars::slash) };
        content.append (target.toRawUTF8(), target.getNumBytesAsUTF8());
    }
    else if (entry.existsAsFile())
    {
        entry.loadFileAsData (content);
    }

    return content;
}

juce::Array<juce::File> ZipWriter::getEntries (const juce::File& item)
{
    juce::Array<juce::File> entries;
    entries.add (item);

    if (item.isDirectory() and not item.isSymbolicLink())
    {
        auto descendants (item.findChildFiles (juce::File::findFilesAndDirectories, true, "*",
                                               juce::File::FollowSymlinks::no));
        std::sort (descendants.begin(), descendants.end(), [&item] (const juce::File& first, const juce::File& second)
        {
            return getEntryName (first, item).compare (getEntryName (second, item)) < 0;
        });
        entries.addArray (descendants);
    }

    return entries;
}

juce::String ZipWriter::getEntryName (const juce::File& entry, const juce::File& item)
{
    static const auto slashText { juce::String::charToString (Chars::slash) };
    juce::String entryName { item.getFileName() };

    if (entry != item)
        entryName << slashText << entry.getRelativePathFrom (item).replaceCharacter (Chars::backslash, Chars::slash);

    if (entry.isDirectory() and not entry.isSymbolicLink())
        entryName << slashText;

    return entryName;
}

juce::MemoryBlock ZipWriter::getEntryFields (juce::uint16 method, juce::uint32 checksum, size_t compressedSize,
                                             size_t size, const juce::String& entryName)
{
    juce::MemoryOutputStream fields;
    fields.writeShort (static_cast<short> (versionNeeded));
    fields.writeShort (static_cast<short> (utf8Flag));
    fields.writeShort (static_cast<short> (method));
    fields.writeShort (static_cast<short> (dosTime));
    fields.writeShort (static_cast<short> (dosDate));
    fields.writeInt (static_cast<int> (checksum));
    fields.writeInt (static_cast<int> (compressedSize));
    fields.writeInt (static_cast<int> (size));
    fields.writeShort (static_cast<short> (entryName.getNumBytesAsUTF8()));
    fields.writeShort (0);
    return fields.getMemoryBlock();
}

juce::MemoryBlock ZipWriter::getPayload (const juce::MemoryBlock& content, bool isDeflated)
{
    juce::MemoryOutputStream payload;

    if (isDeflated)
    {
        juce::GZIPCompressorOutputStream compressor (payload, compressionLevel,
                                                     juce::GZIPCompressorOutputStream::windowBitsRaw);
        compressor.write (content.getData(), content.getSize());
    }
    else
    {
        payload.write (content.getData(), content.getSize());
    }

    return payload.getMemoryBlock();
}

void ZipWriter::addEntry (juce::MemoryOutputStream& archive, juce::MemoryOutputStream& directory,
                          const juce::String& entryName, const juce::MemoryBlock& content,
                          bool isDeflated, juce::uint32 attributes)
{
    const auto payload { getPayload (content, isDeflated) };
    const auto fields { getEntryFields (isDeflated ? methodDeflated : methodStored, getChecksum (content),
                                        payload.getSize(), content.getSize(), entryName) };

    directory.writeInt (static_cast<int> (directorySignature));
    directory.writeShort (static_cast<short> (versionMadeBy));
    directory.write (fields.getData(), fields.getSize());
    directory.writeShort (0);
    directory.writeShort (0);
    directory.writeShort (0);
    directory.writeInt (static_cast<int> (attributes));
    directory.writeInt (static_cast<int> (archive.getDataSize()));
    directory.write (entryName.toRawUTF8(), entryName.getNumBytesAsUTF8());

    archive.writeInt (static_cast<int> (localHeaderSignature));
    archive.write (fields.getData(), fields.getSize());
    archive.write (entryName.toRawUTF8(), entryName.getNumBytesAsUTF8());
    archive.write (payload.getData(), payload.getSize());
}

void ZipWriter::addFileEntry (juce::MemoryOutputStream& archive, juce::MemoryOutputStream& directory,
                              const juce::File& entry, const juce::File& item)
{
    addEntry (archive, directory, getEntryName (entry, item), getContent (entry),
              entry.existsAsFile() and not entry.isSymbolicLink(), getAttributes (entry));
}

#if JUCE_MAC
juce::String ZipWriter::getAppleDoubleName (const juce::String& entryName)
{
    const auto trimmedName { entryName.endsWithChar (Chars::slash) ? entryName.dropLastCharacters (1) : entryName };
    const auto slashIndex { trimmedName.lastIndexOfChar (Chars::slash) };
    return appleDoubleFolder + trimmedName.substring (0, slashIndex + 1) + appleDoublePrefix
           + trimmedName.substring (slashIndex + 1);
}

int ZipWriter::addAppleDoubleEntries (juce::MemoryOutputStream& archive, juce::MemoryOutputStream& directory,
                                      const juce::Array<juce::File>& entries, const juce::File& item)
{
    int added { 0 };

    for (const auto& entry : entries)
    {
        if (const auto appleDouble { AppleDoubleWriter::getAppleDouble (entry) })
        {
            addEntry (archive, directory, getAppleDoubleName (getEntryName (entry, item)), *appleDouble, true,
                      fileMode << modeShift);
            ++added;
        }
    }

    return added;
}
#endif

int ZipWriter::addItemEntries (juce::MemoryOutputStream& archive, juce::MemoryOutputStream& directory, const juce::File& item)
{
    const auto entries (getEntries (item));
    int added { entries.size() };

    for (const auto& entry : entries)
        addFileEntry (archive, directory, entry, item);

   #if JUCE_MAC
    added += addAppleDoubleEntries (archive, directory, entries, item);
   #endif

    return added;
}

juce::MemoryBlock ZipWriter::getEndRecord (int count, size_t directorySize, size_t directoryOffset)
{
    juce::MemoryOutputStream record;
    record.writeInt (static_cast<int> (endSignature));
    record.writeShort (0);
    record.writeShort (0);
    record.writeShort (static_cast<short> (count));
    record.writeShort (static_cast<short> (count));
    record.writeInt (static_cast<int> (directorySize));
    record.writeInt (static_cast<int> (directoryOffset));
    record.writeShort (0);
    return record.getMemoryBlock();
}

std::optional<juce::MemoryBlock> ZipWriter::getArchive (const juce::Array<juce::File>& items, const juce::StringPairArray& links)
{
    juce::MemoryOutputStream archive;
    juce::MemoryOutputStream directory;
    int count { links.size() };

    for (const auto& item : items)
    {
        count += addItemEntries (archive, directory, item);
    }

    for (const auto& linkName : links.getAllKeys())
    {
        const auto target { links[linkName].replaceCharacter (Chars::backslash, Chars::slash) };
        addEntry (archive, directory, linkName, juce::MemoryBlock (target.toRawUTF8(), target.getNumBytesAsUTF8()), false, linkMode << modeShift);
    }

    if (count <= maxEntryCount and archive.getDataSize() + directory.getDataSize() <= maxArchiveSize)
    {
        const auto endRecord { getEndRecord (count, directory.getDataSize(), archive.getDataSize()) };
        archive << directory.getMemoryBlock() << endRecord;
        return archive.getMemoryBlock();
    }

    return std::nullopt;
}
