#pragma once
#include <JuceHeader.h>
#if JUCE_MAC
#include <copyfile.h>
#endif
#include "generated/Generated.h"
#include "Toolchain.h"
#include "BinaryWriter.h"
#include "IconWriter.h"
#include "ZipWriter.h"

/**
 * @struct Pack
 * @brief Packs one archive from the @c --pack command line.
 *
 * The extension of the archive selects the format. A @c zip archive goes
 * through ZipWriter. A @c dmg archive goes through a stage folder beside the
 * archive, a @c .DS_Store file written by BinaryWriter, and one @c hdiutil
 * process on macOS. On macOS, both formats pack from a stage, and each staged
 * bundle gets its icon from IconWriter. Pack keeps no state.
 */
struct Pack
{
    /** Number of values in one row of the flat row array: item path, link name, link target. */
    static constexpr int tripleSize { 3 };
    /** Position of the link name in a row. */
    static constexpr int linkNameOffset { 1 };
    /** Position of the link target in a row. */
    static constexpr int linkTargetOffset { 2 };

    /**
     * @brief Returns the @c --background flag text.
     *
     * @returns The flag that names the background image.
     */
    static const juce::String& getBackgroundFlag()
    {
        static const juce::String backgroundFlag { Id::doubleDash + Id::background.toString() };
        return backgroundFlag;
    }

    /**
     * @brief Packs one archive.
     *
     * Fails when the extension of @p archive is not a known format, when an
     * item or the background image does not exist, or when two rows give the
     * archive root one name.
     *
     * @param arguments The command line, for the layout options and
     *                  @c --background.
     * @param archive   The archive to write. Its extension selects the format.
     * @param triples   The rows, flat: item path, link name, link target for
     *                  each row. The size is a multiple of tripleSize.
     * @returns Ok, or the failure with the path and the reason.
     */
    static juce::Result toArchive (const juce::ArgumentList& arguments, const juce::File& archive, const juce::StringArray& triples)
    {
        jassert (triples.size() > 0 and triples.size() % tripleSize == 0);

        static const jam::Function::Map<juce::String, juce::Result> archiveFormats {
            []()
            {
                jam::Function::Map<juce::String, juce::Result> formats;

                formats.add<const juce::ArgumentList&, const juce::File&, const juce::Array<juce::File>&, const juce::StringArray&> (Id::zip, &Pack::toZip);
                formats.add<const juce::ArgumentList&, const juce::File&, const juce::Array<juce::File>&, const juce::StringArray&> (Id::dmg, &Pack::toDmg);

                return formats;
            }()
        };

        const auto extension { archive.getFileExtension().substring (1) };

        if (not archiveFormats.contains (extension))
            return juce::Result::fail (archive.getFullPathName() + Id::diagnosticSeparator
                                       + text::Diagnostics::failArchiveExtension);

        const auto items (getItems (triples));
        const auto found { isFound (arguments, items) };
        const auto unique { found.wasOk() ? isUnique (archive, items, triples) : found };

        return unique.wasOk()
                   ? archiveFormats.get (extension, arguments, archive, items, triples)
                   : unique;
    }

private:
    /**
     * @brief Returns the item files, in row order.
     *
     * @param triples The rows, flat.
     * @returns One file per row, resolved against the working directory.
     */
    static juce::Array<juce::File> getItems (const juce::StringArray& triples)
    {
        juce::Array<juce::File> items;

        for (int row { 0 }; row < triples.size() / tripleSize; ++row)
            items.add (juce::File::getCurrentWorkingDirectory().getChildFile (triples[row * tripleSize]));

        return items;
    }

    /**
     * @brief Returns the background image file.
     *
     * @param arguments The command line, with @c --background.
     * @returns The file, resolved against the working directory.
     */
    static juce::File getBackground (const juce::ArgumentList& arguments)
    {
        return juce::File::getCurrentWorkingDirectory().getChildFile (arguments.getValueForOption (getBackgroundFlag()));
    }

    /**
     * @brief Returns the link name of a row.
     *
     * @param triples The rows, flat.
     * @param row     The row index.
     * @returns The link name. It is empty when the row has no link.
     */
    static juce::String getLinkName (const juce::StringArray& triples, int row)
    {
        return triples[row * tripleSize + linkNameOffset];
    }

    /**
     * @brief Returns the link target of a row.
     *
     * @param triples The rows, flat.
     * @param row     The row index.
     * @returns The link target text. It is empty when the row has no link.
     */
    static juce::String getLinkTarget (const juce::StringArray& triples, int row)
    {
        return triples[row * tripleSize + linkTargetOffset];
    }

    /**
     * @brief Answers whether a row has a link.
     *
     * @param triples The rows, flat.
     * @param row     The row index.
     * @returns True when the link name and the link target are not empty.
     */
    static bool hasLink (const juce::StringArray& triples, int row)
    {
        return getLinkName (triples, row).isNotEmpty() and getLinkTarget (triples, row).isNotEmpty();
    }

    /**
     * @brief Answers whether each input file exists.
     *
     * @param arguments The command line. When it has @c --background, the
     *                  background image must exist as a file.
     * @param items     The item files.
     * @returns Ok, or the failure that names the first missing path.
     */
    static juce::Result isFound (const juce::ArgumentList& arguments, const juce::Array<juce::File>& items)
    {
        for (const auto& item : items)
            if (not item.exists())
                return juce::Result::fail (item.getFullPathName() + Id::diagnosticSeparator
                                           + text::Diagnostics::failNotFound);

        if (arguments.containsOption (getBackgroundFlag()))
        {
            const auto background { getBackground (arguments) };

            return background.existsAsFile()
                       ? juce::Result::ok()
                       : juce::Result::fail (background.getFullPathName() + Id::diagnosticSeparator
                                             + text::Diagnostics::failNotFound);
        }

        return juce::Result::ok();
    }

    /**
     * @brief Answers whether each name at the archive root is unique.
     *
     * The names are the item file names and the link names.
     *
     * @param archive The archive, named in the failure.
     * @param items   The item files.
     * @param triples The rows, flat.
     * @returns Ok, or the failure that names the first repeated name.
     */
    static juce::Result isUnique (const juce::File& archive, const juce::Array<juce::File>& items, const juce::StringArray& triples)
    {
        juce::StringArray rootNames;

        for (int row { 0 }; row < items.size(); ++row)
        {
            rootNames.add (items.getReference (row).getFileName());

            if (hasLink (triples, row))
                rootNames.add (getLinkName (triples, row));
        }

        for (int index { 0 }; index < rootNames.size(); ++index)
            if (rootNames.indexOf (rootNames[index]) != index)
                return juce::Result::fail (archive.getFullPathName()
                                           + Id::diagnosticSeparator
                                           + text::Diagnostics::failDuplicate + rootNames[index]
                                           + Chars::doubleQuote);

        return juce::Result::ok();
    }

    /**
     * @brief Writes a zip archive through ZipWriter.
     *
     * Fails when the command line has a layout option or @c --background.
     * Those options apply to a @c dmg archive only. On macOS, the items are
     * cloned into a stage and ZipWriter packs the staged items.
     *
     * @param arguments The command line.
     * @param archive   The zip file to write.
     * @param items     The item files.
     * @param triples   The rows, flat. Each row with a link adds one
     *                  symbolic link entry.
     * @returns Ok, or the failure.
     */
    static juce::Result toZip (const juce::ArgumentList& arguments, const juce::File& archive, const juce::Array<juce::File>& items, const juce::StringArray& triples)
    {
        if (BinaryWriter::hasLayout (arguments) or arguments.containsOption (getBackgroundFlag()))
            return juce::Result::fail (archive.getFullPathName() + Id::diagnosticSeparator
                                       + text::Diagnostics::failPackOption);

        juce::StringPairArray links { false };

        for (int row { 0 }; row < items.size(); ++row)
            if (hasLink (triples, row))
                links.set (getLinkName (triples, row), getLinkTarget (triples, row));

       #if JUCE_MAC
        return toStage (archive, items, [&archive, &links] (const juce::File&, const juce::Array<juce::File>& stagedItems)
        {
            return ZipWriter::toFile (archive, stagedItems, links);
        });
       #else
        return ZipWriter::toFile (archive, items, links);
       #endif
    }

    /**
     * @brief Writes a disk image.
     *
     * On macOS, the function makes a new stage folder beside @p archive,
     * fills it, runs @c hdiutil, and deletes the stage. The items come from
     * toStage(). On another host, it fails.
     *
     * @param arguments The command line, for the layout options and
     *                  @c --background.
     * @param archive   The disk image to write.
     * @param items     The item files.
     * @param triples   The rows, flat.
     * @returns Ok, or the failure. A stage that cannot be deleted is a
     *          failure.
     */
    static juce::Result toDmg (const juce::ArgumentList& arguments, const juce::File& archive, const juce::Array<juce::File>& items, const juce::StringArray& triples)
    {
       #if JUCE_MAC
        return toStage (archive, items, [&arguments, &archive, &items, &triples] (const juce::File& stage, const juce::Array<juce::File>&)
        {
            const auto imageArguments { getImageArguments (stage, archive) };
            const auto staged { addStage (arguments, stage, archive, items, triples) };

            return staged.wasOk()
                       ? Toolchain::runProcess (imageArguments, imageArguments.joinIntoString (juce::String::charToString (Chars::space)))
                       : staged;
        });
       #else
        juce::ignoreUnused (arguments, items, triples);

        return juce::Result::fail (archive.getFullPathName() + Id::diagnosticSeparator
                                   + text::Diagnostics::failArchiveHost);
       #endif
    }

   #if JUCE_MAC
    /**
     * @brief Makes a stage, clones the items into it, and runs a callable on
     *        the stage.
     *
     * The stage is a new folder beside @p archive. Each staged item gets its
     * icon. The function deletes the stage after the callable returns.
     *
     * @tparam ArchiveStage The callable type. It takes the stage folder and
     *                      the staged items, and returns a juce::Result.
     * @param archive      The archive. The stage folder is named after it.
     * @param items        The item files.
     * @param archiveStage The callable that packs the stage.
     * @returns The result of @p archiveStage, or the failure when the stage
     *          cannot be made, filled, or deleted.
     */
    template <typename ArchiveStage>
    static juce::Result toStage (const juce::File& archive, const juce::Array<juce::File>& items, ArchiveStage&& archiveStage)
    {
        const auto stage { archive.getSiblingFile (archive.getFileNameWithoutExtension()).getNonexistentSibling (false) };
        const auto stageFailure { juce::Result::fail (stage.getFullPathName() + Id::diagnosticSeparator
                                                      + text::Diagnostics::failOutputWrite) };

        if (stage.createDirectory().wasOk())
        {
            const auto stagedItems (getStagedItems (stage, items));
            const auto staged { addStageItems (items, stagedItems) };
            const auto created { staged.wasOk() ? archiveStage (stage, stagedItems) : staged };
            const auto removed { stage.deleteRecursively() };

            return created.wasOk() ? (removed ? juce::Result::ok() : stageFailure) : created;
        }

        return stageFailure;
    }

    /**
     * @brief Returns the @c hdiutil command line that creates a
     *        compressed image with the file system HFS+.
     *
     * The command line has @c -ov, @c -srcfolder, @c -volname,
     * @c -fs @c HFS+, and @c -format @c UDZO.
     *
     * @param stage       The folder that the image is built from.
     * @param archiveFile The disk image to create. Its name without
     *                    extension is the volume name.
     * @returns The argv, with @c hdiutil first.
     */
    static juce::StringArray getImageArguments (const juce::File& stage, const juce::File& archiveFile)
    {
        static constexpr const char* createVerb { "create" };
        static constexpr const char* overwriteFlag { "-ov" };
        static constexpr const char* sourceFolderFlag { "-srcfolder" };
        static constexpr const char* volumeNameFlag { "-volname" };
        static constexpr const char* fileSystemFlag { "-fs" };
        static constexpr const char* fileSystem { "HFS+" };
        static constexpr const char* formatFlag { "-format" };
        static constexpr const char* compressedFormat { "UDZO" };

        return juce::StringArray { Id::hdiutil, createVerb, overwriteFlag, sourceFolderFlag, stage.getFullPathName(),
                                   volumeNameFlag, archiveFile.getFileNameWithoutExtension(),
                                   fileSystemFlag, fileSystem,
                                   formatFlag, compressedFormat, archiveFile.getFullPathName() };
    }

    /**
     * @brief Fills the stage folder: the links, the background image, and
     *        the @c .DS_Store file.
     *
     * The items are already cloned into the stage.
     *
     * @param arguments The command line.
     * @param stage     The stage folder.
     * @param archive   The disk image. Its name without extension is the
     *                  volume name.
     * @param items     The item files.
     * @param triples   The rows, flat.
     * @returns Ok, or the first failure.
     */
    static juce::Result addStage (const juce::ArgumentList& arguments, const juce::File& stage, const juce::File& archive, const juce::Array<juce::File>& items, const juce::StringArray& triples)
    {
        if (const auto result { addStageLinks (stage, triples, items.size()) }; not result.wasOk())
            return result;

        if (const auto result { addStageBackground (arguments, stage) }; not result.wasOk())
            return result;

        return addStageLayout (arguments, stage, archive, items, triples);
    }

    /**
     * @brief Clones each item into the stage and writes its icon.
     *
     * @param items       The item files.
     * @param stagedItems The clone target of each item, in the order of
     *                    @p items.
     * @returns Ok, or the first failure.
     */
    static juce::Result addStageItems (const juce::Array<juce::File>& items, const juce::Array<juce::File>& stagedItems)
    {
        for (int index { 0 }; index < items.size(); ++index)
        {
            const auto& item { items.getReference (index) };
            const auto& copy { stagedItems.getReference (index) };

            if (copyfile (item.getFullPathName().toRawUTF8(), copy.getFullPathName().toRawUTF8(), nullptr,
                          COPYFILE_CLONE | COPYFILE_RECURSIVE) != 0)
                return juce::Result::fail (copy.getFullPathName() + Id::diagnosticSeparator
                                           + text::Diagnostics::failOutputWrite);

            if (const auto result { IconWriter::toBundle (copy) }; not result.wasOk())
                return result;
        }

        return juce::Result::ok();
    }

    /**
     * @brief Adds one symbolic link to the stage for each row that has a
     *        link.
     *
     * @param stage    The stage folder.
     * @param triples  The rows, flat.
     * @param rowCount The number of rows.
     * @returns Ok, or the failure that names the link that did not write.
     */
    static juce::Result addStageLinks (const juce::File& stage, const juce::StringArray& triples, int rowCount)
    {
        for (int row { 0 }; row < rowCount; ++row)
        {
            if (hasLink (triples, row))
            {
                const auto linkFile { stage.getChildFile (getLinkName (triples, row)) };

                if (not juce::File::createSymbolicLink (linkFile, getLinkTarget (triples, row), true))
                    return juce::Result::fail (linkFile.getFullPathName() + Id::diagnosticSeparator
                                               + text::Diagnostics::failOutputWrite);
            }
        }

        return juce::Result::ok();
    }

    /**
     * @brief Returns the stage path of each item.
     *
     * @param stage The stage folder.
     * @param items The item files.
     * @returns One file per item, named like the item, in @p stage.
     */
    static juce::Array<juce::File> getStagedItems (const juce::File& stage, const juce::Array<juce::File>& items)
    {
        juce::Array<juce::File> stagedItems;

        for (const auto& item : items)
            stagedItems.add (stage.getChildFile (item.getFileName()));

        return stagedItems;
    }

    /**
     * @brief Returns the file name of the background image in the stage.
     *
     * @param arguments The command line.
     * @returns The background prefix with the extension of the image. It is
     *          empty when the command line has no @c --background.
     */
    static juce::String getBackgroundName (const juce::ArgumentList& arguments)
    {
        return arguments.containsOption (getBackgroundFlag())
                   ? Id::backgroundPrefix + getBackground (arguments).getFileExtension()
                   : juce::String();
    }

    /**
     * @brief Clones the background image into the stage.
     *
     * The function does nothing when the command line has no
     * @c --background.
     *
     * @param arguments The command line.
     * @param stage     The stage folder.
     * @returns Ok, or the failure that names the path that did not write.
     */
    static juce::Result addStageBackground (const juce::ArgumentList& arguments, const juce::File& stage)
    {
        if (arguments.containsOption (getBackgroundFlag()))
        {
            const auto background { getBackground (arguments) };
            const auto backgroundCopy { stage.getChildFile (getBackgroundName (arguments)) };

            if (copyfile (background.getFullPathName().toRawUTF8(), backgroundCopy.getFullPathName().toRawUTF8(), nullptr, COPYFILE_CLONE) != 0)
                return juce::Result::fail (backgroundCopy.getFullPathName() + Id::diagnosticSeparator
                                           + text::Diagnostics::failOutputWrite);
        }

        return juce::Result::ok();
    }

    /**
     * @brief Writes the @c .DS_Store file of the stage through BinaryWriter.
     *
     * @param arguments The command line, for the layout options.
     * @param stage     The stage folder.
     * @param archive   The disk image. Its name without extension is the
     *                  volume name.
     * @param items     The item files.
     * @param triples   The rows, flat.
     * @returns Ok, or the failure when the records do not fit or the file
     *          does not write.
     */
    static juce::Result addStageLayout (const juce::ArgumentList& arguments, const juce::File& stage, const juce::File& archive, const juce::Array<juce::File>& items, const juce::StringArray& triples)
    {
        const auto dsStore { stage.getChildFile (Id::dsStore) };
        juce::StringArray itemNames;
        juce::StringArray linkNames;

        for (int row { 0 }; row < items.size(); ++row)
        {
            itemNames.add (items.getReference (row).getFileName());
            linkNames.add (hasLink (triples, row) ? getLinkName (triples, row) : juce::String());
        }

        const auto store { BinaryWriter::getStore (arguments, itemNames, linkNames, archive.getFileNameWithoutExtension(), getBackgroundName (arguments)) };

        return store.has_value() and dsStore.replaceWithData (store->getData(), store->getSize())
                   ? juce::Result::ok()
                   : juce::Result::fail (dsStore.getFullPathName() + Id::diagnosticSeparator
                                         + text::Diagnostics::failOutputWrite);
    }
   #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pack)
};
