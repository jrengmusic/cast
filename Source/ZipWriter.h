#pragma once
#include <JuceHeader.h>
#if JUCE_WINDOWS
#include <windows.h>
#else
#include <sys/stat.h>
#endif
#include "generated/Generated.h"
#include "AppleDoubleWriter.h"

/**
 * @struct ZipWriter
 * @brief Encodes files, folders, and symbolic links into one ZIP archive
 *        and writes it to disk, write-if-different.
 *
 * ZipWriter keeps no state. It stores symbolic links and folders without
 * compression and deflates regular files. On macOS, each entry with Finder
 * information or a resource fork adds an AppleDouble entry. On Windows, each
 * entry records its DOS attribute bits. Entry timestamps are fixed, so the
 * same inputs always give the same archive bytes.
 */
struct ZipWriter
{
    /**
     * @brief Builds the archive of @p items and @p links and writes it to
     *        @p archive when its bytes differ from the file on disk.
     *
     * @param archive The archive file to write. Its parent directory is
     *                created when absent.
     * @param items   The files, folders, and symbolic links to store. A
     *                folder is stored with all its descendants, and each
     *                entry name starts with the item's own file name.
     * @param links   The extra symbolic-link entries, each key the entry
     *                name and each value the link target.
     * @returns juce::Result::ok() when the archive is written or already
     *          identical, or a failure naming @p archive when the archive
     *          exceeds the entry-count or size limit or the write fails.
     */
    static juce::Result toFile (const juce::File& archive, const juce::Array<juce::File>& items, const juce::StringPairArray& links)
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

private:
    /** Signature that opens each local file header. */
    static constexpr juce::uint32 localHeaderSignature { 0x04034b50 };
    /** Signature that opens each central directory record. */
    static constexpr juce::uint32 directorySignature { 0x02014b50 };
    /** Signature that opens the end-of-directory record. */
    static constexpr juce::uint32 endSignature { 0x06054b50 };
    /** Host system and version stamped on each central directory record. */
    static constexpr juce::uint16 versionMadeBy { 0x0314 };
    /** Minimum version needed to extract an entry. */
    static constexpr juce::uint16 versionNeeded { 20 };
    /** General-purpose flag that marks entry names as UTF-8. */
    static constexpr juce::uint16 utf8Flag { 0x0800 };
    /** Compression method of an entry stored as is. */
    static constexpr juce::uint16 methodStored { 0 };
    /** Compression method of a deflated entry. */
    static constexpr juce::uint16 methodDeflated { 8 };
    /** Fixed modification time written to every entry. */
    static constexpr juce::uint16 dosTime { 0 };
    /** Fixed modification date written to every entry. */
    static constexpr juce::uint16 dosDate { 0x21 };
    /** Deflate level used for regular files. */
    static constexpr int compressionLevel { 9 };
    /** Reflected CRC-32 polynomial. */
    static constexpr juce::uint32 checksumPolynomial { 0xedb88320 };
    /** Unix mode of a regular file on Windows hosts, and of an AppleDouble entry on every host. */
    static constexpr juce::uint32 fileMode { 0100644 };
    /** Unix mode of a folder on Windows hosts. */
    static constexpr juce::uint32 folderMode { 0040755 };
    /** Unix mode of a symbolic-link entry. */
    static constexpr juce::uint32 linkMode { 0120755 };
    /** Bit position of the Unix mode in the external attributes. */
    static constexpr int modeShift { 16 };
   #if JUCE_WINDOWS
    /** Windows file attribute bits that the low byte of the external attributes records. */
    static constexpr juce::uint32 dosAttributeMask { FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY };
   #endif
   #if JUCE_MAC
    /** Root folder of the AppleDouble entries. */
    static constexpr const char* appleDoubleFolder { "__MACOSX/" };
    /** Prefix of the file name of an AppleDouble entry. */
    static constexpr const char* appleDoublePrefix { "._" };
   #endif
    /** Largest entry count the end record can hold. */
    static constexpr int maxEntryCount { (std::numeric_limits<juce::uint16>::max)() };
    /** Largest archive size the 32-bit offsets can address. */
    static constexpr juce::uint64 maxArchiveSize { (std::numeric_limits<juce::uint32>::max)() };

    /**
     * @brief Builds the 256-entry CRC-32 lookup table at compile time.
     *
     * @returns The table indexed by the low byte of the running checksum.
     */
    static constexpr std::array<juce::uint32, 256> getChecksumTable() noexcept
    {
        std::array<juce::uint32, 256> table {};

        for (juce::uint32 index { 0 }; index < table.size(); ++index)
        {
            juce::uint32 value { index };

            for (int bit { 0 }; bit < 8; ++bit)
                value = (value & 1u) != 0 ? (value >> 1) ^ checksumPolynomial : value >> 1;

            table.at (index) = value;
        }

        return table;
    }

    /**
     * @brief Computes the CRC-32 of @p content.
     *
     * @param content The bytes to checksum.
     * @returns The CRC-32 of @p content.
     */
    static juce::uint32 getChecksum (const juce::MemoryBlock& content)
    {
        static constexpr auto checksumTable { getChecksumTable() };
        juce::uint32 checksum { 0xffffffff };

        for (const auto byte : content)
            checksum = checksumTable.at ((checksum ^ static_cast<juce::uint8> (byte)) & 0xffu) ^ (checksum >> 8);

        return checksum ^ 0xffffffff;
    }

    /**
     * @brief Returns the Unix mode of @p entry.
     *
     * On Windows the mode comes from the entry type. On other hosts it is
     * the @c st_mode that @c lstat reports.
     *
     * @param entry The file, folder, or symbolic link to read.
     * @returns The Unix mode of @p entry.
     */
    static juce::uint32 getMode (const juce::File& entry)
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

    /**
     * @brief Returns the DOS attribute bits of @p entry.
     *
     * On Windows the bits are the ReadOnly, Hidden, System, and Directory
     * attributes. On other hosts the result is zero.
     *
     * @param entry The file, folder, or symbolic link to read.
     * @returns The bits for the low byte of the external attributes.
     */
    static juce::uint32 getDosAttributes (const juce::File& entry)
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

    /**
     * @brief Returns the external attributes of @p entry.
     *
     * @param entry The file, folder, or symbolic link to read.
     * @returns The Unix mode in the high 16 bits and the DOS attribute bits
     *          in the low byte.
     */
    static juce::uint32 getAttributes (const juce::File& entry)
    {
        return getMode (entry) << modeShift | getDosAttributes (entry);
    }

    /**
     * @brief Returns the bytes stored for @p entry.
     *
     * @param entry The entry to read.
     * @returns The link target text of a symbolic link, the file bytes of
     *          a regular file, or an empty block for a folder.
     */
    static juce::MemoryBlock getContent (const juce::File& entry)
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

    /**
     * @brief Lists @p item and, when it is a real folder, all its
     *        descendants.
     *
     * @param item The root of the walk.
     * @returns @p item first, then its descendants sorted by entry name.
     *          Symbolic links are not followed.
     */
    static juce::Array<juce::File> getEntries (const juce::File& item)
    {
        juce::Array<juce::File> entries;
        entries.add (item);

        if (item.isDirectory() and not item.isSymbolicLink())
        {
            auto descendants { item.findChildFiles (juce::File::findFilesAndDirectories, true, "*",
                                                    juce::File::FollowSymlinks::no) };
            std::sort (descendants.begin(), descendants.end(), [&item] (const juce::File& first, const juce::File& second)
            {
                return getEntryName (first, item).compare (getEntryName (second, item)) < 0;
            });
            entries.addArray (descendants);
        }

        return entries;
    }

    /**
     * @brief Returns the archive name of @p entry.
     *
     * @param entry The entry to name.
     * @param item  The item @p entry belongs to. Its file name is the
     *              first path segment.
     * @returns The name with forward slashes, with a trailing slash when
     *          @p entry is a real folder.
     */
    static juce::String getEntryName (const juce::File& entry, const juce::File& item)
    {
        static const auto slashText { juce::String::charToString (Chars::slash) };
        juce::String entryName { item.getFileName() };

        if (entry != item)
            entryName << slashText << entry.getRelativePathFrom (item).replaceCharacter (Chars::backslash, Chars::slash);

        if (entry.isDirectory() and not entry.isSymbolicLink())
            entryName << slashText;

        return entryName;
    }

    /**
     * @brief Returns the header fields that a local header and a central
     *        directory record share.
     *
     * @param method         The compression method.
     * @param checksum       The CRC-32 of the uncompressed bytes.
     * @param compressedSize The stored payload size.
     * @param size           The uncompressed size.
     * @param entryName      The entry name, whose UTF-8 length is written.
     * @returns The encoded fields.
     */
    static juce::MemoryBlock getEntryFields (juce::uint16 method, juce::uint32 checksum, size_t compressedSize,
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

    /**
     * @brief Returns the payload of one entry.
     *
     * @param content    The uncompressed bytes.
     * @param isDeflated @c true to compress @p content as raw deflate,
     *                   @c false to copy it.
     * @returns The bytes stored in the archive.
     */
    static juce::MemoryBlock getPayload (const juce::MemoryBlock& content, bool isDeflated)
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

    /**
     * @brief Appends one entry to @p archive and its record to
     *        @p directory.
     *
     * @param archive    The stream that receives the local header, name,
     *                   and payload.
     * @param directory  The stream that receives the central directory
     *                   record.
     * @param entryName  The entry name.
     * @param content    The uncompressed bytes.
     * @param isDeflated @c true to deflate @p content.
     * @param attributes The external attributes: the Unix mode in the high
     *                   16 bits and the DOS attribute bits in the low byte.
     */
    static void addEntry (juce::MemoryOutputStream& archive, juce::MemoryOutputStream& directory,
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

    /**
     * @brief Appends @p entry through addEntry(). Only regular files are
     *        deflated.
     *
     * @param archive   The stream that receives the entry.
     * @param directory The stream that receives the directory record.
     * @param entry     The entry to append.
     * @param item      The item @p entry belongs to.
     */
    static void addFileEntry (juce::MemoryOutputStream& archive, juce::MemoryOutputStream& directory,
                              const juce::File& entry, const juce::File& item)
    {
        addEntry (archive, directory, getEntryName (entry, item), getContent (entry),
                  entry.existsAsFile() and not entry.isSymbolicLink(), getAttributes (entry));
    }

   #if JUCE_MAC
    /**
     * @brief Returns the name of the AppleDouble entry of an entry.
     *
     * @param entryName The archive name of the entry.
     * @returns The name under the AppleDouble root folder, with the prefix
     *          on the last path segment. A trailing slash is dropped.
     */
    static juce::String getAppleDoubleName (const juce::String& entryName)
    {
        const auto trimmedName { entryName.endsWithChar (Chars::slash) ? entryName.dropLastCharacters (1) : entryName };
        const auto slashIndex { trimmedName.lastIndexOfChar (Chars::slash) };
        return appleDoubleFolder + trimmedName.substring (0, slashIndex + 1) + appleDoublePrefix
               + trimmedName.substring (slashIndex + 1);
    }

    /**
     * @brief Appends the AppleDouble entry of each entry that has one.
     *
     * Each AppleDouble entry is deflated and records the mode of a regular
     * file.
     *
     * @param archive   The stream that receives the entries.
     * @param directory The stream that receives the directory records.
     * @param entries   The entries of @p item.
     * @param item      The item that @p entries belong to.
     * @returns The number of AppleDouble entries appended.
     */
    static int addAppleDoubleEntries (juce::MemoryOutputStream& archive, juce::MemoryOutputStream& directory,
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

    /**
     * @brief Appends all entries of @p item.
     *
     * The entries of the item come first. On macOS, their AppleDouble
     * entries follow.
     *
     * @param archive   The stream that receives the entries.
     * @param directory The stream that receives the directory records.
     * @param item      The file, folder, or symbolic link to store.
     * @returns The number of entries appended, AppleDouble entries
     *          included.
     */
    static int addItemEntries (juce::MemoryOutputStream& archive, juce::MemoryOutputStream& directory, const juce::File& item)
    {
        const auto entries { getEntries (item) };
        int added { entries.size() };

        for (const auto& entry : entries)
            addFileEntry (archive, directory, entry, item);

       #if JUCE_MAC
        added += addAppleDoubleEntries (archive, directory, entries, item);
       #endif

        return added;
    }

    /**
     * @brief Returns the end-of-directory record.
     *
     * @param count           The total entry count.
     * @param directorySize   The central directory size in bytes.
     * @param directoryOffset The offset of the central directory.
     * @returns The encoded record.
     */
    static juce::MemoryBlock getEndRecord (int count, size_t directorySize, size_t directoryOffset)
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

    /**
     * @brief Encodes the whole archive.
     *
     * @param items The files, folders, and symbolic links to store.
     * @param links The extra symbolic-link entries.
     * @returns The archive bytes, or @c std::nullopt when the entry count
     *          or the size exceeds the format limit. The entry count
     *          includes the AppleDouble entries.
     */
    static std::optional<juce::MemoryBlock> getArchive (const juce::Array<juce::File>& items, const juce::StringPairArray& links)
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZipWriter)
};
